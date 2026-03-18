// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2000-2006 by Sergey Makovkin (CSDoom .62).
// Copyright (C) 2006-2026 by The Odamex Team.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// DESCRIPTION:
//	SV_MAIN
//
//-----------------------------------------------------------------------------


#include "odamex.h"

#include "win32inc.h"
#ifdef _WIN32
#   define WIN32_LEAN_AND_MEAN
#   include <winsock2.h>
#   include <time.h>
#endif

#ifdef UNIX
#   include <unistd.h>
#   include <sys/time.h>
#endif

#include "gstrings.h"
#include "d_player.h"
#include "s_sound.h"
#include "g_game.h"
#include "p_tick.h"
#include "p_local.h"
#include "p_inter.h"
#include "sv_main.h"
#include "sv_sqp.h"
#include "sv_sqpold.h"
#include "sv_master.h"
#include "i_system.h"
#include "i_time.h"
#include "c_console.h"
#include "c_dispatch.h"
#include "m_argv.h"
#include "m_random.h"
#include "p_ctf.h"
#include "w_wad.h"
#include "w_ident.h"
#include "md5.h"
#include "p_mobj.h"
#include "p_unlag.h"
#include "sv_vote.h"
#include "sv_maplist.h"
#include "g_levelstate.h"
#include "g_gametype.h"
#include "sv_banlist.h"
#include "d_main.h"
#include "v_textcolors.h"
#include "p_lnspec.h"
#include "m_wdlstats.h"
#include "m_cheat.h"
#include "m_instrumentation.h"
#include "p_playerping.h"

#include <algorithm>
#include <condition_variable>
#include <future>
#include <mutex>
#include <sstream>
#include <thread>

#include "server.pb.h"

#include "CanarySocket.h"

#include "clc_message.h"
#include "svc_message.h"
#include "svc_parse.h"

extern void G_DeferedInitNew (const OLumpName& mapname);
extern level_locals_t level;

constexpr int MAX_HIDDEN_MOBJ_UPDATES = 16;


// Unnatural Level Progression.  True if we've used 'map' or another command
// to switch to a specific map out of order, otherwise false.
bool unnatural_level_progression;

// denis - game manipulation, but no fancy gfx
bool clientside = false, serverside = true;
bool predicting = false;
baseapp_t baseapp = server;

extern int mapchange;

bool step_mode = false;

std::set<byte> free_player_ids;

bool keysfound[NUMCARDS];		// Ch0wW : Found keys

// General server settings
EXTERN_CVAR(sv_motd)
EXTERN_CVAR(sv_hostname)
EXTERN_CVAR(sv_email)
EXTERN_CVAR(sv_maxrate)
EXTERN_CVAR(sv_emptyreset)
EXTERN_CVAR(sv_emptyfreeze)
EXTERN_CVAR(sv_clientcount)
EXTERN_CVAR(sv_globalspectatorchat)
EXTERN_CVAR(sv_allowtargetnames)
EXTERN_CVAR(sv_flooddelay)
EXTERN_CVAR(sv_ticbuffer)
EXTERN_CVAR(sv_warmup)
EXTERN_CVAR(sv_sharekeys)
EXTERN_CVAR(sv_teamsinplay)
EXTERN_CVAR(g_winnerstays)
EXTERN_CVAR(debug_disconnect)
EXTERN_CVAR(g_resetinvonexit)
EXTERN_CVAR(port)

void SexMessage (const char *from, char *to, gender_t gender,
	std::string_view victim, std::string_view killer, std::string_view spree);
Players::iterator SV_RemoveDisconnectedPlayer(Players::iterator it);
void P_PlayerLeavesGame(player_t* player);
bool P_LineSpecialMovesSector(short special);

void SV_UpdateShareKeys(player_t& player);
std::string SV_BuildKillsDeathsStatusString(const player_t& player);
std::string V_GetTeamColor(UserInfo userinfo);
void SV_SendPlayerPing(const player_t& source, client_t* dest);
void SV_BroadcastPlayerPing(const player_t& source);

CVAR_FUNC_IMPL (sv_maxclients)
{
	// Describes the max number of clients that are allowed to connect.
	int count = var.asInt();
	Players::iterator it = players.begin();
	while (it != players.end())
	{
		if (count <= 0)
		{
			MSG_WriteSVC(
			    it->client.messenger.ReliableBuf(),
			    SVC_Print(PRINT_CHAT,
			              "Client limit reduced. Please try connecting again later.\n"));

			SV_DropClient(*it);
			it = SV_RemoveDisconnectedPlayer(it);
		}
		else
		{
			count -= 1;
			++it;
		}
	}
}


CVAR_FUNC_IMPL (sv_maxplayers)
{
	int normalcount = 0;
	bool queueExists = false;

	for (Players::iterator it = players.begin();it != players.end();++it)
	{
		bool spectator = it->spectator || !it->ingame();

		if (it->QueuePosition > 0)
			queueExists = true;

		if (!spectator)
		{
			normalcount++;

			if (normalcount > var)
			{
				it->spectator = true;
				it->playerstate = PST_LIVE;
				it->joindelay = 0;

				for (Players::iterator pit = players.begin(); pit != players.end(); ++pit)
				{
					MSG_WriteSVC(pit->client.messenger.ReliableBuf(),
					             SVC_PlayerMembers(*it, SVC_PM_SPECTATOR));
				}

				std::string status = SV_BuildKillsDeathsStatusString(*it);
				SV_BroadcastPrintFmt(PRINT_HIGH, "{} became a spectator. ({})\n",
					it->userinfo.netname, status);

				MSG_WriteSVC(
				    it->client.messenger.ReliableBuf(),
				    SVC_Print(PRINT_HIGH,
				              "Active player limit reduced. You are now a spectator!\n"));
			}
		}
	}

	if (queueExists)
		SV_ClearPlayerQueue();
}

// [AM] - Force extras on a team to become spectators.
CVAR_FUNC_IMPL (sv_maxplayersperteam)
{
	// 0 is unlimited
	if (!var)
		return;

	for (int i = 0; i < NUMTEAMS;i++)
	{
		int normalcount = 0;
		for (auto& player : players)
		{
			bool spectator = player.spectator || !player.ingame();
			if (player.userinfo.team == i && player.ingame() && !spectator)
			{
				normalcount++;

				if (normalcount > var)
				{
					SV_SetPlayerSpec(player, true);
					SV_PlayerPrintFmt(player.id, PRINT_HIGH, "Active player limit reduced. You are now a spectator!\n");
				}
			}
		}
	}
}

EXTERN_CVAR (sv_allowcheats)
EXTERN_CVAR (sv_fraglimit)
EXTERN_CVAR (sv_timelimit)
EXTERN_CVAR (sv_intermissionlimit)
EXTERN_CVAR (sv_maxcorpses)

// Action rules
EXTERN_CVAR (sv_allowexit)
EXTERN_CVAR (sv_fragexitswitch)
EXTERN_CVAR (sv_allowjump)
EXTERN_CVAR (sv_freelook)
EXTERN_CVAR (sv_infiniteammo)

// Teamplay/CTF
EXTERN_CVAR (sv_scorelimit)
EXTERN_CVAR (sv_friendlyfire)

// Survival
EXTERN_CVAR (g_lives)

// Private server settings
CVAR_FUNC_IMPL (join_password)
{
	if (strlen(var.cstring()))
		PrintFmt("Join password set.");
	else
		PrintFmt("Join password cleared.");
}

CVAR_FUNC_IMPL (rcon_password) // Remote console password.
{
	if(strlen(var.cstring()) < 5)
	{
		if(!strlen(var.cstring()))
			PrintFmt("RCON password cleared.");
		else
		{
			PrintFmt("RCON password must be at least 5 characters.");
			var.Set("");
		}
	}
	else
		PrintFmt(PRINT_HIGH, "RCON password set.");
}

CVAR_FUNC_IMPL(sv_maxrate)
{
	for (auto& player : players)
		player.client.messenger.SetMaxRate(int(sv_maxrate));
}

CVAR_FUNC_IMPL(sv_sharekeys)
{
	if (var)
	{
		// Refresh it to everyone
		for (auto& player : players) {
			SV_UpdateShareKeys(player);
		}
	}
}

client_c clients;


#define CLIENT_TIMEOUT 65 // 65 seconds

void SV_UpdateConsolePlayer(player_t &player);

void SV_CheckTeam (player_t & playernum);
team_t SV_GoodTeam (void);

static void SendServerSettings(player_t& pl);

// some doom functions
size_t P_NumPlayersOnTeam(team_t team);
size_t P_NumPlayersInGame();

// [AM] Flip a coin between heads and tails
BEGIN_COMMAND (coinflip) {
	std::string result;
	CMD_CoinFlip(result);

	SV_BroadcastPrintFmt("{}\n", result);
} END_COMMAND (coinflip)

void CMD_CoinFlip(std::string &result) {
	result = (P_Random() % 2 == 0) ? "Coin came up Heads." : "Coin came up Tails.";
	return;
}

// denis - kick player
BEGIN_COMMAND (kick) {
	std::vector<std::string> arguments = VectorArgs(argc, argv);
	std::string error;

	size_t pid;
	std::string reason;

	if (!CMD_KickCheck(arguments, error, pid, reason)) {
		PrintFmt("Kick: {}.\n", error);
		return;
	}

	SV_KickPlayer(idplayer(pid), reason);
} END_COMMAND (kick)

// Kick command check.
bool CMD_KickCheck(std::vector<std::string> arguments, std::string &error,
				   size_t &pid, std::string &reason) {
	// Did we pass enough arguments?
	if (arguments.size() < 1) {
		error = "Need a player ID (try 'players') and optionally a reason.";
		return false;
	}

	// Did we actually pass a player number?
	std::istringstream buffer(arguments[0]);
	buffer >> pid;

	if (!buffer) {
		error = "Need a valid player number.";
		return false;
	}

	// Verify that the player we found is a valid client.
	player_t &player = idplayer(pid);

	if (!validplayer(player)) {
		std::ostringstream error_ss;
		error_ss << "could not find client " << pid << ".";
		error = error_ss.str();
		return false;
	}

	// Anything that is not the first argument is the reason.
	arguments.erase(arguments.begin());
	reason = JoinStrings(arguments, " ");

	return true;
}

// Kick a player.
void SV_KickPlayer(player_t &player, const std::string &reason) {
	// Avoid a segfault from an invalid player.
	if (!validplayer(player)) {
		return;
	}

	if (reason.empty())
		SV_BroadcastPrintFmt("{} was kicked from the server!\n", player.userinfo.netname);
	else
		SV_BroadcastPrintFmt("{} was kicked from the server! (Reason: {})\n",
					player.userinfo.netname, reason);

	player.client.displaydisconnect = false;
	SV_DropClient(player);
}

// Invalidate a player.
//
// Usually happens when corrupted messages are passed to the server by the
// client.  Reasons should only be seen by the server admin.  Player and client
// are both presumed unusable after function is done.
void SV_InvalidateClient(player_t &player, const std::string& reason)
{
	PrintFmt("{} fails security check ({}), dropping client.\n", NET_AdrToString(player.client.address), reason);
	SV_PlayerPrintFmt(PRINT_ERROR, player.id,
	                  "The server closed your connection for the following reason: {}.\n",
	                  reason);
	SV_DropClient(player);
}

BEGIN_COMMAND (stepmode)
{
    if (step_mode)
        step_mode = false;
    else
        step_mode = true;

    return;
}
END_COMMAND (stepmode)

BEGIN_COMMAND (say)
{
	if (argc > 1)
	{
		std::string chat = C_ArgCombine(argc - 1, (const char **)(argv + 1));
		SV_BroadcastPrintFmt(PRINT_SERVERCHAT, "[console]: {}\n", chat);
	}
}
END_COMMAND (say)

void STACK_ARGS call_terms (void);

void SV_QuitCommand()
{
	call_terms();
	exit(EXIT_SUCCESS);
}

BEGIN_COMMAND (rquit)
{
	SV_SendAndFlushReconnectSignal();

	SV_QuitCommand();
}
END_COMMAND (rquit)

BEGIN_COMMAND (quit)
{
	SV_QuitCommand();
}
END_COMMAND (quit)

// An alias for 'quit'
BEGIN_COMMAND (exit)
{
	SV_QuitCommand();
}
END_COMMAND (exit)

static void SendLevelState(SerializedLevelState sls)
{
	for (auto& player : players)
	{
		client_t& cl = player.client;
		MSG_WriteSVC(cl.messenger.ReliableBuf(), SVC_LevelState(sls));
	}
}

static player_t &SV_FindPlayerByAddr(const netadr_t& netAddr)
{
	for (auto& player : players)
	{
		if (NET_CompareAdr(player.client.address, netAddr))
		   return player;
	}

	return idplayer(0);
}

namespace
{
    struct BaseWorkerCommand
    {
        virtual ~BaseWorkerCommand() {}

        virtual void Run() = 0;
    };



    template <typename TaskType>
    struct WorkerCommand : BaseWorkerCommand
    {
        TaskType task;

        WorkerCommand(TaskType&& i_task) :
            task(std::move(i_task))
        {
        }

        void Run() override
        {
            task();
        }
    };

    struct WorkerQuitCommand : WorkerCommand<std::packaged_task<std::thread::id ()>>
    {
        WorkerQuitCommand() : WorkerCommand(std::packaged_task<std::thread::id ()>(std::this_thread::get_id)) {}
    };

    class WorkerPool
    {
        public:
            WorkerPool()
            {
            }

            ~WorkerPool()
            {
                std::unique_lock lock {m_commandMutex};

                m_commandQueue.clear();

                for (size_t i = 0; i < m_threads.size(); ++i)
                {
                    m_commandQueue.emplace_back(std::make_unique<WorkerQuitCommand>());
                }

                lock.unlock();
                m_commandCondition.notify_all();

                for (auto& thread : m_threads)
                {
                    thread.join();
                }
            }

            template <typename TaskType>
            void MoveCommand(TaskType&& i_command)
            {
                {
                    std::unique_lock lock {m_commandMutex};
                    m_commandQueue.emplace_back(std::make_unique<WorkerCommand<TaskType>>(std::move(i_command)));
                }

                m_commandCondition.notify_one();
            }

            void Resize(int i_threadCount)
            {
                int deltaSize = i_threadCount - static_cast<int>(m_threads.size());

                if (deltaSize > 0)
                {
                    while (deltaSize)
                    {
                        --deltaSize;
                        m_threads.emplace_back(&WorkerPool::EntryPoint, this);
                    }
                }
                else if (deltaSize < 0)
                {
                    while (deltaSize)
                    {
                        ++deltaSize;

                        auto quitCommandPtr = std::make_unique<WorkerQuitCommand>();
                        auto quitFuture     = quitCommandPtr->task.get_future();
                        {
                            std::unique_lock lock {m_commandMutex};
                            m_commandQueue.emplace_back(std::move(quitCommandPtr));
                        }
                        m_commandCondition.notify_one();

                        const std::thread::id threadId = quitFuture.get();
                        for (auto iter = m_threads.begin(); iter != m_threads.end(); ++iter)
                        {
                            if (threadId == iter->get_id())
                            {
                                iter->join();
                                m_threads.erase(iter);
                                break;
                            }
                        }
                    }
                }
            }

            size_t ThreadCount() const { return m_threads.size(); }

        protected:

            void EntryPoint()
            {
                while (1)
                {
                    std::unique_ptr<BaseWorkerCommand> command = GetCommand();

                    command->Run();

                    if (IsQuit(command))
                    {
                        break;
                    }
                }
            }

            std::unique_ptr<BaseWorkerCommand> GetCommand()
            {
                std::unique_lock lock {m_commandMutex};

                while (m_commandQueue.empty())
                {
                    m_commandCondition.wait(lock);
                }

                std::unique_ptr<BaseWorkerCommand> result = std::move(m_commandQueue.front());
                m_commandQueue.pop_front();
                return result;
            }

        protected:

            bool IsQuit(const std::unique_ptr<BaseWorkerCommand>& i_ptr) { return dynamic_cast<WorkerQuitCommand*>(i_ptr.get()); }

            std::mutex                                      m_commandMutex;
            std::condition_variable                         m_commandCondition;
            std::deque<std::unique_ptr<BaseWorkerCommand> > m_commandQueue;

            std::vector<std::thread> m_threads;
    };

    WorkerPool s_workers;
}

CVAR_FUNC_IMPL(net_maxthreads)
{
	int threadCount = var.asInt();
	if (threadCount > 0)
	{
		s_workers.Resize(threadCount);
	}
	else
	{
		if (threadCount < 0)
		{
			PrintFmt("Invalid thread count: {}.  Resetting to default...\n", threadCount);
		}

		s_workers.Resize(std::thread::hardware_concurrency());
	}
	PrintFmt("net_maxthreads pool has {} threads\n", s_workers.ThreadCount());
}

namespace
{
	/// This class takes ownership of messengers that belonged to clients that are disconnecting
	/// and makes sure that their Acknowledgements and Retransmits are serviced to the point
	/// where the server can be certain that the clients receive their final reliable messages.
	/// This is a key part of having an orderly disconnect even under high loads.
	class DepartingMessengerManager
	{
		public:

			void TakeMessengerFrom(client_t& client)
			{
				m_deadEndMessengers.insert({client.address, std::move(client.messenger)});
			}

			void ServiceMessenger(int currentTic, std::map<netadr_t, OdaMessenger>::iterator iter, buf_t& packetBuffer)
			{
				// Because we still want to honor acks from a disconnecting client,
				// we must service them immediately upon receipt from the socket because
				// they are not queued by the receiver.
				while (iter->second.NextReceivedPacket(packetBuffer))
				{
					iter->second.HandleAcks(packetBuffer);
				}

				iter->second.HandleRetransmissions(currentTic, iter->first);
				iter->second.SendAll(currentTic, iter->first);
			}

			size_t CheckMessengers(int currentTic)
			{
				buf_t throwaway;

				for (auto iter = m_deadEndMessengers.begin(); iter != m_deadEndMessengers.end(); ++iter)
				{
					ServiceMessenger(currentTic, iter, throwaway);
				}

				return m_deadEndMessengers.size();
			}

			bool HandlePacket(int currentTic, const netadr_t& address, buf_t& packetBuffer)
			{
				auto iter = m_deadEndMessengers.find(address);
				if (iter != m_deadEndMessengers.end())
				{
					iter->second.Receive(packetBuffer);

					ServiceMessenger(currentTic, iter, packetBuffer);

					// Any messenger that's in the dead-end collection is there because we put it there directly
					// after sending the client's last reliable message.  Therefore, we know the pending Ack
					// count is going to be > 0.  If we see it go to 0, it's because the client has unambiguously
					// seen it and moved on.
					//
					// Also, we have to remove the old messenger immediately because it's 100% possible that the
					// client has an immediate reconnection attempt as the very next packet, and we want to handle
					// it in the `else` case below without delay.
					if (iter->second.GetPendingAckCount() <= 0)
					{
						m_deadEndMessengers.erase(iter);
					}
					return true;
				}
				return false;
			}

			void Drop(const netadr_t& address)
			{
				m_deadEndMessengers.erase(address);
			}

		protected:
			// Intentionally use a map here instead of an unordered_map, because a map (a binary tree) tends to be
			// faster for iterating over smaller element counts than an unordered_map (a hash table), the latter
			// of which may require iterating over some number of completely unused buckets.
			std::map<netadr_t, OdaMessenger> m_deadEndMessengers;
	};

	DepartingMessengerManager s_departingMessengers;
}

static void SV_DepartMessenger(client_t& client)
{
	s_departingMessengers.TakeMessengerFrom(client);
}

static size_t SV_CheckDepartingMessengers(int currentTic)
{
	return s_departingMessengers.CheckMessengers(currentTic);
}

static bool SV_HandleDepartingMessengerPacket(int currentTic, const netadr_t& address, buf_t& packetBuffer)
{
	return s_departingMessengers.HandlePacket(currentTic, address, packetBuffer);
}

/// This function handles the case where a client needs to be dropped and we're not counting on it
/// to acknowledge anything - we know it's going to be unresponsive.
static void SV_DropClientUngracefully(player_t& playerRef, const char* disconnectPrintVerb)
{
	if (validplayer(playerRef) and playerRef.playerstate != PST_DISCONNECT)
	{
		SV_BroadcastPrintFmt("{} {} ({})\n",
		                     playerRef.userinfo.netname,
		                     disconnectPrintVerb,
		                     SV_BuildKillsDeathsStatusString(playerRef));

		playerRef.client.displaydisconnect = false;
		SV_DropClient(playerRef);
	}

	// This is a special case where we know for certain that the other end is either
	// truly terminated or timedout.  There's no point in letting the dead-end messenger
	// handler drive the packet sequence to completion.
	s_departingMessengers.Drop(playerRef.client.address);
}

static std::unique_ptr<CanarySocketServer> s_canaries;

static void SV_CheckCanaries()
{
	if (s_canaries)
	{
		auto deadCanaryIter = s_canaries->FindDead();

		while (deadCanaryIter != s_canaries->end())
		{
			netadr_t netAddr;

			SockadrToNetadr(& deadCanaryIter->udpAddr, & netAddr);

			player_t& playerRef = SV_FindPlayerByAddr(netAddr);

			SV_DropClientUngracefully(playerRef, "disconnected abnormally");

			deadCanaryIter = s_canaries->PutOnCart(deadCanaryIter);
		}
	}
}

//
// SV_InitNetwork
//
void SV_InitNetwork (void)
{
    network_game = true;

	const char *v = Args.CheckValue ("-port");
    if (v)
    {
       localport = atoi (v);
       PrintFmt(PRINT_HIGH, "using alternate port {}\n", localport);
    }
	else
	   localport = SERVERPORT;

	// set up a socket and net_message buffer
	InitNetCommon();

	PrintFmt("UDP Initialized.\n");

	s_canaries = std::make_unique<CanarySocketServer>(port.asInt());

	const char *w = Args.CheckValue ("-maxclients");
	if (w)
	{
		sv_maxclients.Set(w); // denis - todo
	}

	step_mode = Args.CheckParm ("-stepmode");

	// [AM] Set up levelstate so it calls a netmessage broadcast function on change.
	::levelstate.setStateCB(SendLevelState);

	// Nes - Connect with the master servers. (If valid)
	SV_InitMasters();
}

//Get next free player. Will use the lowest available player id.
Players::iterator SV_GetFreeClient(void)
{
	if (players.size() >= sv_maxclients)
		return players.end();

	if (free_player_ids.empty())
	{
		// list of free ids needs to be initialized
		for (int i = 1; i < MAXPLAYERS; i++)
			free_player_ids.insert(i);
	}

	players.emplace_back().playerstate = PST_CONTACT;

	// generate player id
	std::set<byte>::iterator id = free_player_ids.begin();
	players.back().id = *id;
	free_player_ids.erase(id);

	// update tracking cvar
	sv_clientcount.ForceSet(players.size());

	// Return iterator pointing to the just-inserted player
	Players::iterator it = players.end();
	return --it;
}

//
// SV_CheckTimeouts
// If a packet has not been received from a client in CLIENT_TIMEOUT
// seconds, drop the conneciton.
//
void SV_CheckTimeouts()
{
	for (auto& player : players)
	{
		if (gametic - player.client.last_received == CLIENT_TIMEOUT * 35)
		{
			SV_DropClientUngracefully(player, "timed out");
		}
	}
}

//
// SV_RemoveDisconnectedPlayer
//
// [SL] 2011-05-18 - Destroy a player's mo actor and remove the player_t
// from the global players vector.  Update mo->player pointers.
// [AM] Updated to remove from a list instead.  Eventually, we should turn
//      this into Players::erase().
Players::iterator SV_RemoveDisconnectedPlayer(Players::iterator it)
{
	if (it == players.end() || !validplayer(*it))
		return players.end();

	int player_id = it->id;

	if (!it->spectator)
	{
		P_PlayerLeavesGame(&(*it));
		SV_UpdatePlayerQueuePositions(G_CanJoinGame, &(*it));
	}

	// remove player awareness from all actors
	AActor* mo;
	TThinkerIterator<AActor> iterator;
	while ((mo = iterator.Next()))
		mo->players_aware.unset(it->id);

	// remove this player's actor object
	if (it->mo)
	{
		if (sv_gametype == GM_CTF) //  [Toke - CTF]
			CTF_CheckFlags(*it);

		// [AM] AActor->Destroy() does not destroy the AActor for good, and also
		//      does not null the player reference.  We have to do it here to
		//      prevent actions on a zombie mobj from using a player that we
		//      already erased from the players list.
		it->mo->player = NULL;

		it->mo->Destroy();
		it->mo = AActor::AActorPtr();
	}

	// remove this player from the global players vector
	Players::iterator next;
	next = players.erase(it);
	free_player_ids.insert(player_id);

	Unlag::getInstance().unregisterPlayer(player_id);

	// update tracking cvar
	sv_clientcount.ForceSet(players.size());

	return next;
}

//
// SV_GetPackets
//
void SV_GetPackets()
{
	while (NET_GetPacket())
	{
		if (SV_HandleDepartingMessengerPacket(gametic, net_from, ::net_message))
		{
			continue;
		}

		player_t &player = SV_FindPlayerByAddr(net_from);

		if (!validplayer(player)) // no client with net_from address
		{
			// apparently, someone is trying to connect
			if (gamestate == GS_LEVEL || gamestate == GS_INTERMISSION)
			{
				SV_ConnectClient();
			}
		}
		else
		{
			player.client.messenger.Receive(::net_message);
			player.client.last_received = gametic;
			SV_ParseCommands(player);
		}
	}
}

// Print a midscreen message to a client
void SV_MidPrint(const char* msg, player_t* p, int msgtime)
{
	client_t* cl = &p->client;

	MSG_WriteSVC(cl->messenger.ReliableBuf(), SVC_MidPrint(msg, msgtime));
}

void SV_BasePrint(client_t* cl, const int printlevel, const std::string& str)
{
	MSG_WriteSVC(cl->messenger.ReliableBuf(), SVC_Print(static_cast<printlevel_t>(printlevel), str));
}

void SV_BasePrintAllPlayers(const int printlevel, const std::string& str)
{
	for (auto& player : players)
		MSG_WriteSVC(player.client.messenger.ReliableBuf(), SVC_Print(static_cast<printlevel_t>(printlevel), str));
}

void SV_BasePrintButPlayer(const int printlevel, const int player_id, const std::string& str)
{
	for (auto& player : players)
	{
		client_t* cl = &(player.client);

		client_t* excluded_client = &idplayer(player_id).client;

		if (cl == excluded_client)
			continue;

		MSG_WriteSVC(cl->messenger.ReliableBuf(), SVC_Print(static_cast<printlevel_t>(printlevel), str));
	}
}

//
// SV_Sound
//
void SV_Sound (const AActor *mo, byte channel, const char *name, byte attenuation)
{
	const int sfx_id = S_FindSound (name);

	if (sfx_id >= static_cast<int>(S_sfx.size()) || sfx_id < 0)
	{
		PrintFmt(PRINT_HIGH, "SV_StartSound: range error. Sfx_id = {}\n", sfx_id);
		return;
	}

	for (auto& player : players)
	{
		client_t* cl = &(player.client);

		MSG_WriteSVC(cl->messenger.ReliableBuf(), SVC_PlaySound(PlaySoundType(mo), channel, sfx_id,
		                                             1.0f, attenuation));
	}
}

void SV_Sound(player_t& pl, const AActor* mo, const byte channel, const char* name,
              const byte attenuation)
{
	const int sfx_id = S_FindSound (name);

	if (sfx_id >= static_cast<int>(S_sfx.size()) || sfx_id < 0)
	{
		PrintFmt(PRINT_HIGH, "SV_StartSound: range error. Sfx_id = {}\n", sfx_id);
		return;
	}

	client_t *cl = &pl.client;

	MSG_WriteSVC(cl->messenger.ReliableBuf(),
	             SVC_PlaySound(PlaySoundType(mo), channel, sfx_id, 1.0f, attenuation));
}

//
// UV_SoundAvoidPlayer
// Sends a sound to clients, but doesn't send it to client 'player'.
//
void UV_SoundAvoidPlayer (const AActor *mo, byte channel, const char *name, byte attenuation)
{
	if (!mo || !mo->player)
		return;

	player_t &pl = *mo->player;

	const int sfx_id = S_FindSound (name);

	if (sfx_id >= static_cast<int>(S_sfx.size()) || sfx_id < 0)
	{
		PrintFmt(PRINT_HIGH, "SV_StartSound: range error. Sfx_id = {}\n", sfx_id);
		return;
	}

	for (auto& player : players)
	{
		if(&pl == &player)
			continue;

		client_t* cl = &(player.client);

		MSG_WriteSVC(cl->messenger.ReliableBuf(), SVC_PlaySound(PlaySoundType(mo), channel, sfx_id,
		                                             1.0f, attenuation));
	}
}

//
//	SV_SoundTeam
//	Sends a sound to players on the specified teams
//
void SV_SoundTeam (byte channel, const char* name, byte attenuation, int team)
{
	const int sfx_id = S_FindSound( name );

	if (sfx_id >= static_cast<int>(S_sfx.size()) || sfx_id < 0)
	{
		PrintFmt("SV_StartSound: range error. Sfx_id = {}\n", sfx_id );
		return;
	}

	for (auto& player : players)
	{
		if (player.ingame() && player.userinfo.team == team)
		{
			client_t* cl = &(player.client);

			MSG_WriteSVC(cl->messenger.ReliableBuf(), SVC_PlaySound(PlaySoundType(), channel, sfx_id,
			                                             1.0f, attenuation));
		}
	}
}

void SV_Sound (fixed_t x, fixed_t y, byte channel, const char *name, byte attenuation)
{
	const int sfx_id = S_FindSound (name);

	if (sfx_id >= static_cast<int>(S_sfx.size()) || sfx_id < 0)
	{
		PrintFmt(PRINT_HIGH, "SV_StartSound: range error. Sfx_id = {}\n", sfx_id);
		return;
	}

	for (auto& player : players)
	{
		if (!(player.ingame()))
			continue;

		client_t* cl = &(player.client);

		MSG_WriteSVC(cl->messenger.ReliableBuf(), SVC_PlaySound(PlaySoundType(x, y), channel, sfx_id,
		                                             1.0f, attenuation));
	}
}

//
// SV_UpdateFrags
//
void SV_UpdateFrags(const player_t &player)
{
	for (Players::iterator it = players.begin();it != players.end();++it)
	{
		client_t *cl = &(it->client);
		MSG_WriteSVC(cl->messenger.ReliableBuf(), SVC_PlayerMembers(player, SVC_PM_SCORE));
	}
}

//
// SV_SendUserInfo
//
void SV_SendUserInfo (const player_t &player, client_t* cl)
{
	MSG_WriteSVC(cl->messenger.ReliableBuf(), SVC_UserInfo(player, time(NULL) - player.JoinTime));
}

/**
Spreads a player's userinfo to every client.
@param player Player to parse info for.
 */
void SV_BroadcastUserInfo(const player_t &player)
{
	for (Players::iterator it = players.begin();it != players.end();++it)
		SV_SendUserInfo(player, &(it->client));
}

/**
 * Stores a players userinfo.
 *
 * @param player Player to parse info for.
 * @return False if the client was kicked because of something seriously
 *         screwy going on with their info.
 */
bool SV_SetupUserInfo(player_t &player)
{
	// read in userinfo from packet
	std::string old_netname(player.userinfo.netname);
	std::string new_netname(MSG_ReadString());
	StripColorCodes(new_netname);

	if (new_netname.length() > MAXPLAYERNAME)
		new_netname.erase(MAXPLAYERNAME);

	if (!ValidString(new_netname))
	{
		SV_InvalidateClient(player, "Name contains invalid characters");
		return false;
	}

	team_t old_team = static_cast<team_t>(player.userinfo.team);
	team_t new_team = static_cast<team_t>(MSG_ReadByte());

	if (new_team >= NUMTEAMS || new_team < 0)
	{
		SV_InvalidateClient(player, "Team preference is invalid");
		return false;
	}
	if (new_team == TEAM_NONE || (new_team == TEAM_GREEN && sv_teamsinplay < NUMTEAMS))
		new_team = TEAM_BLUE; // Set the default team to the player.
	const bool team_changed = new_team != old_team;

	gender_t gender = static_cast<gender_t>(MSG_ReadLong());

	colorpreset_t colorpreset = static_cast<colorpreset_t>(MSG_ReadLong());

	byte color[4];
	for (int i = 3; i >= 0; i--)
		color[i] = MSG_ReadByte();

	MSG_ReadString();	// [SL] place holder for deprecated skins

	fixed_t aimdist = MSG_ReadLong();
	MSG_ReadBool();		// [SL] Read and ignore deprecated cl_unlag setting
	bool predict_weapons = MSG_ReadBool();

	weaponswitch_t switchweapon = static_cast<weaponswitch_t>(MSG_ReadByte());

	byte weapon_prefs[NUMWEAPONS];
	for (size_t i = 0; i < NUMWEAPONS; i++)
	{
		// sanitize the weapon preference input
		byte preflevel = MSG_ReadByte();
		if (preflevel >= NUMWEAPONS)
			preflevel = NUMWEAPONS - 1;

		weapon_prefs[i] = preflevel;
	}

	// ensure sane values for userinfo
	if (gender < 0 || gender >= NUMGENDER)
		gender = GENDER_OTHER;

	if (colorpreset < 0 || colorpreset >= NUMCOLOR)
		colorpreset = COLOR_CUSTOM;

	aimdist = clamp(aimdist, 0, 5000 * 16384);

	if (switchweapon >= WPSW_NUMTYPES || switchweapon < 0)
		switchweapon = WPSW_ALWAYS;

	// [SL] 2011-12-02 - Players can update these parameters whenever they like
	player.userinfo.predict_weapons	= predict_weapons;
	player.userinfo.aimdist			= aimdist;
	player.userinfo.switchweapon	= switchweapon;
	memcpy(player.userinfo.weapon_prefs, weapon_prefs, sizeof(weapon_prefs));

	player.userinfo.gender			= gender;
	player.userinfo.team			= new_team;
	if (team_changed)
	{
		P_ClearPlayerPingState(player);
		SV_BroadcastPlayerPing(player);
	}

	player.userinfo.colorpreset		= colorpreset;

	memcpy(player.userinfo.color, color, 4);
	memcpy(player.prefcolor, color, 4);

	// sanitize the client's name
	new_netname = TrimString(new_netname);
	if (new_netname.empty())
	{
		if (old_netname.empty())
			new_netname = "PLAYER";
		else
			new_netname = old_netname;
	}

	// Make sure that we're not duplicating any player name
	player_t& other = nameplayer(new_netname);
	if (validplayer(other))
	{
		// Give the player an enumerated name (Player2, Player3, etc.)
		if (player.id != other.id)
		{
			std::string test_netname = std::string(new_netname);

			for (int num = 2;num < MAXPLAYERS + 2;num++)
			{
				std::ostringstream buffer;
				buffer << num;
				std::string strnum = buffer.str();

				// If the enumerated name would be greater than the maximum
				// allowed netname, have the enumeration eat into the last few
				// letters of their netname.
				if (new_netname.length() + strnum.length() >= MAXPLAYERNAME)
					test_netname = new_netname.substr(0, MAXPLAYERNAME - strnum.length()) + strnum;
				else
					test_netname = new_netname + strnum;

				// Check to see if the enumerated name is taken.
				player_t& test = nameplayer(test_netname);
				if (!validplayer(test))
					break;

				// Check to see if player already has a given enumerated name.
				// Prevents `cl_name Player` from changing their own name from
				// Player2 to Player3.
				if (player.id == test.id)
					break;
			}

			new_netname = test_netname;
		}
		else
		{
			// Player is trying to be cute and change their name to an
			// automatically enumerated name that they already have.  Prevents
			// `cl_name Player2` from changing their own name from Player2 to
			// Player22.
			new_netname = old_netname;
		}
	}

	if (new_netname.length() > MAXPLAYERNAME)
		new_netname.erase(MAXPLAYERNAME);

	player.userinfo.netname = new_netname;

	// Compare names and broadcast if different.
	if (!old_netname.empty() && !iequals(new_netname, old_netname))
	{
		std::string	gendermessage;
		switch (gender) {
			case GENDER_MALE:	gendermessage = "his";  break;
			case GENDER_FEMALE:	gendermessage = "her";  break;
			case GENDER_CYBORG:	gendermessage = "its";  break;
			default:			gendermessage = "their";  break;
		}

		SV_BroadcastPrintFmt("{} changed {} name to {}.\n",
			old_netname, gendermessage, player.userinfo.netname);

		team_t team = TEAM_NONE;
		if (player.mo && player.userinfo.team && player.ingame() && !player.spectator &&
		    !G_IsLevelState(LevelState::WARMUP))
		{
			M_HandleWDLNameChange(team, old_netname,
			                      player.userinfo.netname, player.id);
		}
	}

	if (G_IsTeamGame())
	{
		SV_CheckTeam(player);

		if (player.mo && player.userinfo.team != old_team && player.ingame() &&
		    !player.spectator && !G_IsLevelState(LevelState::WARMUP))
		{
			// kill player if team is changed
			P_DamageMobj(player.mo, 0, 0, 1000, 0);
			M_LogWDLEvent(WDL_EVENT_DISCONNECT, &player, NULL, old_team,
			              M_GetPlayerId(player, old_team), 0, 0);
			M_LogWDLEvent(WDL_EVENT_JOINGAME, &player, NULL, player.userinfo.team,
			              M_GetPlayerId(player, player.userinfo.team), 0,
			              0);
			SV_BroadcastPrintFmt("{} switched to the {} team.\n",
			                     player.userinfo.netname,
			                     V_GetTeamColor(player.userinfo.team));
		}
	}

	return true;
}

//
//	SV_ForceSetTeam
//
//	Forces a client to join a specified team
//
void SV_ForceSetTeam (player_t &who, team_t team)
{
	client_t *cl = &who.client;

	const team_t oldTeam = who.userinfo.team;
	who.userinfo.team = team;
	if (team != oldTeam)
	{
		P_ClearPlayerPingState(who);
		SV_BroadcastPlayerPing(who);
	}
	PrintFmt(PRINT_HIGH, "Forcing {} to {} team\n", who.userinfo.netname.c_str(), team == TEAM_NONE ? "NONE" : V_GetTeamColor(team).c_str());

	MSG_WriteSVC(cl->messenger.ReliableBuf(), SVC_ForceTeam(team));
}

//
//	SV_CheckTeam
//
//	Checks to see if a players team is allowed and corrects if not
//
void SV_CheckTeam (player_t &player)
{
	if (!player.spectator && (player.userinfo.team < 0 || player.userinfo.team >= sv_teamsinplay))
		SV_ForceSetTeam(player, SV_GoodTeam());
}

//
//	SV_GoodTeam
//
//	Finds a working team
//
team_t SV_GoodTeam (void)
{
	int teamcount = sv_teamsinplay;

	// Unsure how this can be triggered?
	if (teamcount == 0)
	{
		I_Error ("Teamplay is set and no teams are enabled!\n");
		return TEAM_NONE;
	}

	// Find the smallest team
	size_t smallest_team_size = MAXPLAYERS;
	team_t smallest_team = (team_t)0;
	for (int i = 0;i < teamcount;i++)
	{
		size_t team_size = P_NumPlayersOnTeam((team_t)i);
		if (team_size < smallest_team_size)
		{
			smallest_team_size = team_size;
			smallest_team = (team_t)i;
		}
	}

	if (sv_maxplayersperteam && smallest_team_size >= sv_maxplayersperteam)
		return TEAM_NONE;

	return smallest_team;
}

//
// SV_SendMobjToClient
//
void SV_SendMobjToClient(AActor *mo, client_t *cl)
{
	if (!mo)
		return;

	MSG_WriteSVC(cl->messenger.ReliableBuf(), SVC_SpawnMobj(mo));
}

//
// SV_IsTeammate
//
bool SV_IsTeammate(player_t &a, player_t &b)
{
	// same player isn't own teammate
	if(&a == &b)
		return false;

	if (G_IsTeamGame())
	{
		if (a.userinfo.team == b.userinfo.team)
			return true;
		else
			return false;
	}
	else if (G_IsCoopGame())
	{
		return true;
	}

	return false;
}

//
// [denis] SV_AwarenessUpdate
//
bool SV_AwarenessUpdate(player_t &player, AActor *mo, const std::optional<bool> forcedAwareness)
{
	bool ok = false;

	if (!mo)
		return false;

	if(player.mo == mo)
		ok = true;
    else if(forcedAwareness.has_value())    // else if because players are ALWAYS aware of themselves.
        ok = forcedAwareness.value();
	else if(!mo->player)
		ok = true;
	else if (mo->oflags & MFO_SPECTATOR)      // GhostlyDeath -- Spectating things
		ok = false;
	else if(player.mo && mo->player && mo->player->spectator)
		ok = false;
	else if(player.mo && mo->player && SV_IsTeammate(player, *mo->player))
		ok = true;
	else if(player.mo && mo->player && true)
		ok = true;

	bool previously_ok = mo->players_aware.get(player.id);

	client_t *cl = &player.client;

	if(!ok && previously_ok)
	{
		mo->players_aware.unset(player.id);

		MSG_WriteSVC(cl->messenger.ReliableBuf(), SVC_RemoveMobj(*mo));

		return true;
	}
	else if(!previously_ok && ok)
	{
		mo->players_aware.set(player.id);

		if(!mo->player || mo->player->playerstate != PST_LIVE)
		{
			SV_SendMobjToClient(mo, cl);
		}
		else
		{
			MSG_WriteSVC(cl->messenger.ReliableBuf(), SVC_SpawnPlayer(*mo->player));
		}

		return true;
	}
	return false;
}

bool SV_AwarenessUpdate(player_t &player, AActor *mo)
{
    return SV_AwarenessUpdate(player, mo, std::nullopt);
}

//
// SV_SpawnMobj functions
// ----------------------
// These exist because we can't expect the constructors to send network messages.
//
// This function does the actual prep for sending the given actor to clients.
// If i_allowDirectSpawnQueue is true, it goes on the higher-priority queue for
// runtime-spawned actors (i.e. missiles and such).  We do this because on great
// big slaughter maps, it's really helpful to see shots coming in from a distance
// even if the monster that fired it isn't visible yet.  We also do this because a
// map-load sends some non-monster entities that really must be sent in as-created
// order and we can't just defer and let the player-distance algorithm do it later.

static void SV_SpawnMobjPrepareForClients(AActor* mo, bool i_allowDirectSpawnQueue)
{
	if (!mo)
		return;

	P_SetMobjBaseline(*mo);

	for (auto& player : players)
	{
		if (mo->player)
		{
			SV_AwarenessUpdate(player, mo);
		}
		else
		{
			if (i_allowDirectSpawnQueue)
			{
				player.to_spawn.push(mo->ptr());
			}
		}
	}
}

// This function sends the Mobj to clients through an immediate runtime-spawned higher-priority queue.
void SV_SpawnMobj(AActor *mo)
{
	SV_SpawnMobjPrepareForClients(mo, true);
}

// This function does the work of preparing the mobj for transmission to clients, but it defers the
// Spawn Mobj message for the player-distance sort algorithm.  This allows us to send some mobjs
// immediately during map load (i.e. things that have an important effect on the client state), but
// defer lower-priority things like idle monsters.
void SV_SpawnMapMobj(AActor *mo)
{
	SV_SpawnMobjPrepareForClients(mo, false);
}

//
// [denis] SV_IsPlayerAllowedToSee
// determine if a client should be able to see an actor
//
bool SV_IsPlayerAllowedToSee(const player_t &p, const AActor *mo)
{
	if (!mo)
		return false;

	if (mo->oflags & MFO_SPECTATOR)
		return false; // GhostlyDeath -- always false, as usual!
	else
		return mo->players_aware.get(p.id);
}

//
// SV_UpdateHiddenMobj
//

namespace
{
	std::mutex s_spawnSzpMutex;
}

int SV_UpdateHiddenMobj(player_t& pl, AActor *mo, int updated)
{
	if (pl.mo)
	{
		if (updated == 0)
		{
			while (!pl.to_spawn.empty())
			{
				mo = pl.to_spawn.front();

				// The following lock is needed to dodge a contention issue in the
				// non-safe portion of the szp utility where it manipulates an
				// internal linked list.  Arguably the fix belongs in szp itself,
				// but if we're willing to accept a global overhead hit, then the
				// right thing to do would be to drop szp and use C++'s shared_ptr
				// and weak_ptr instead.  For now, the "minimal viable" fix is to
				// do the locking in the one place that really needs it.
				{
					std::unique_lock lock {s_spawnSzpMutex};
					pl.to_spawn.pop();
				}

				if (mo && !mo->WasDestroyed())
					updated += SV_AwarenessUpdate(pl, mo);

				if (updated > MAX_HIDDEN_MOBJ_UPDATES)
					break;
			}
		}
		updated += SV_AwarenessUpdate(pl, mo);
	}
	return updated;
}

bool SV_SendPacket(player_t &pl)
{
	return pl.client.messenger.SendAll(gametic, pl.client.address) != MessageResultEnum::ABORT;
}

void SV_UpdateSector(client_t* cl, int sectornum)
{
	sector_t* sector = &sectors[sectornum];

	// Only update moveable sectors to clients
	if (sector != nullptr && sector->moveable)
	{
		MSG_WriteSVC(cl->messenger.ReliableBuf(), SVC_UpdateSector(*sector));
	}
}

void SV_BroadcastSector(int sectornum)
{
	for (auto& player : players)
		SV_UpdateSector(&(player.client), sectornum);
}

void SV_UpdateSectorProperties(client_t* cl, int sectornum)
{
	sector_t* sector = &sectors[sectornum];

	// Only update sectors with changes
	if (sector != nullptr && sector->SectorChanges)
	{
		MSG_WriteSVC(cl->messenger.ReliableBuf(), SVC_SectorProperties(*sector));
	}
}

void SV_BroadcastSectorProperties(int sectornum)
{
	for (auto& player : players)
		SV_UpdateSectorProperties(&(player.client), sectornum);
}

//
// SV_UpdateSectors
// Update doors, floors, ceilings etc... that have at some point moved
//
void SV_UpdateSectors(client_t* cl)
{
	for (int sectornum = 0; sectornum < numsectors; sectornum++)
	{
		SV_UpdateSector(cl, sectornum);

		sector_t& sector = ::sectors[sectornum];
		if (!sector.SectorChanges)
			continue;

		MSG_WriteSVC(cl->messenger.ReliableBuf(), SVC_SectorProperties(sector));
	}
}

//
// SV_DestroyFinishedMovingSectors
//
// Calls Destroy() on moving sectors that are done moving.
//
void SV_DestroyFinishedMovingSectors()
{
	std::list<movingsector_t>::iterator itr;
	itr = movingsectors.begin();

	while (itr != movingsectors.end())
	{
		sector_t *sector = itr->sector;

		if (P_MovingCeilingCompleted(sector))
		{
			itr->moving_ceiling = false;
			if (sector->ceilingdata)
				sector->ceilingdata->Destroy();
			sector->ceilingdata = NULL;
		}
		if (P_MovingFloorCompleted(sector))
		{
			itr->moving_floor = false;
			if (sector->floordata)
				sector->floordata->Destroy();
			sector->floordata = NULL;
		}

		if (!itr->moving_ceiling && !itr->moving_floor)
			movingsectors.erase(itr++);
		else
			++itr;
	}
}

//
// SV_SendMovingSectorUpdate
//
//
void SV_SendMovingSectorUpdate(player_t &player, sector_t *sector)
{
	if (!sector || !validplayer(player))
		return;

	int sectornum = sector - sectors;
	if (sectornum < 0 || sectornum >= numsectors)
		return;

	odaproto::svc::MovingSector msg = SVC_MovingSector(*sector);
	if (!msg.movers())
	{
		// No movers in the packet, don't send.
		return;
	}
	MSG_WriteSVC(player.client.messenger.NetBuf(), msg);
}

//
// SV_UpdateMovingSectors
// Update doors, floors, ceilings etc... that are actively moving
//
void SV_UpdateMovingSectors(player_t &player)
{
	std::list<movingsector_t>::iterator itr;
	for (itr = movingsectors.begin(); itr != movingsectors.end(); ++itr)
	{
		sector_t *sector = itr->sector;

		SV_SendMovingSectorUpdate(player, sector);
	}
}


//
// SV_SendGametic
// Sends gametic to synchronize with the client
//
// [SL] 2011-05-11 - Instead of sending the whole gametic (4 bytes),
// send only the least significant byte to save bandwidth.
void SV_SendGametic(client_t* cl)
{
	const byte tic = static_cast<byte>(gametic & 0xFF);

	MSG_WriteSVC(cl->messenger.NetBuf(), SVC_ServerGametic(tic, cl->messenger.GetPendingAckCount()));
}

void SV_LineStateUpdate(client_t *cl)
{
	for (int lineNum = 0; lineNum < numlines; lineNum++)
	{
		line_t* line = &lines[lineNum];

		if (line->PropertiesChanged)
		{
			MSG_WriteSVC(cl->messenger.ReliableBuf(), SVC_LineUpdate(*line));
		}

		if (!line->SidedefChanged)
			continue;

		for (int sideNum = 0; sideNum < 2; sideNum++)
		{
			if (line->sidenum[sideNum] != R_NOSIDE)
			{
				side_t* currentSideDef = sides + line->sidenum[sideNum];
				if (!currentSideDef->SidedefChanges)
					continue;

				MSG_WriteSVC(cl->messenger.ReliableBuf(), SVC_LineSideUpdate(*line, sideNum));
			}
		}
	}
}

void SV_ThinkerUpdate(client_t* cl)
{
	TThinkerIterator<DScroller> scrollIter;
	DScroller* scroller;
	while ((scroller = scrollIter.Next()))
	{
		MSG_WriteSVC(cl->messenger.ReliableBuf(), SVC_ThinkerUpdate(scroller));
	}

	TThinkerIterator<DFireFlicker> fireIter;
	DFireFlicker* fireFlicker;
	while ((fireFlicker = fireIter.Next()))
	{
		MSG_WriteSVC(cl->messenger.ReliableBuf(), SVC_ThinkerUpdate(fireFlicker));
	}

	TThinkerIterator<DFlicker> flickerIter;
	DFlicker* flicker;
	while ((flicker = flickerIter.Next()))
	{
		MSG_WriteSVC(cl->messenger.ReliableBuf(), SVC_ThinkerUpdate(flicker));
	}

	TThinkerIterator<DLightFlash> lightFlashIter;
	DLightFlash* lightFlash;
	while ((lightFlash = lightFlashIter.Next()))
	{
		MSG_WriteSVC(cl->messenger.ReliableBuf(), SVC_ThinkerUpdate(lightFlash));
	}

	TThinkerIterator<DStrobe> strobeIter;
	DStrobe* strobe;
	while ((strobe = strobeIter.Next()))
	{
		MSG_WriteSVC(cl->messenger.ReliableBuf(), SVC_ThinkerUpdate(strobe));
	}

	TThinkerIterator<DGlow> glowIter;
	DGlow* glow;
	while ((glow = glowIter.Next()))
	{
		MSG_WriteSVC(cl->messenger.ReliableBuf(), SVC_ThinkerUpdate(glow));
	}

	TThinkerIterator<DGlow2> glow2Iter;
	DGlow2* glow2;
	while ((glow2 = glow2Iter.Next()))
	{
		MSG_WriteSVC(cl->messenger.ReliableBuf(), SVC_ThinkerUpdate(glow2));
	}

	TThinkerIterator<DPhased> phasedIter;
	DPhased* phased;
	while ((phased = phasedIter.Next()))
	{
		MSG_WriteSVC(cl->messenger.ReliableBuf(), SVC_ThinkerUpdate(phased));
	}
}

//
// SV_ClientFullUpdate
//
void SV_ClientFullUpdate(player_t &pl)
{
	client_t *cl = &pl.client;

	MSG_WriteSVC(cl->messenger.ReliableBuf(), odaproto::svc::FullUpdateStart());

	// Send the player all level locals.
	MSG_WriteSVC(cl->messenger.ReliableBuf(), SVC_LevelLocals(::level, SVC_MSG_ALL));

	// send player's info to the client
	for (Players::iterator it = players.begin();it != players.end();++it)
	{
		if (it->mo)
			SV_AwarenessUpdate(pl, it->mo);

		SV_SendUserInfo(*it, cl);
	}

	// update levelstate
	MSG_WriteSVC(cl->messenger.ReliableBuf(), SVC_LevelState(::levelstate.serialize()));

	// update all player members
	for (Players::iterator it = players.begin(); it != players.end(); ++it)
		MSG_WriteSVC(cl->messenger.ReliableBuf(), SVC_PlayerMembers(*it, SVC_MSG_ALL));

	// send active pings so newly connected players can see them
	for (Players::iterator it = players.begin(); it != players.end(); ++it)
		SV_SendPlayerPing(*it, cl);

	// [deathz0r] send team frags/captures if teamplay is enabled
	if (G_IsTeamGame())
	{
		for (int i = 0; i < NUMTEAMS; i++)
			MSG_WriteSVC(cl->messenger.ReliableBuf(), SVC_TeamMembers(static_cast<team_t>(i)));
	}

	int hiddenUpdates = 0;
	TThinkerIterator<AActor> iterator;
	AActor* mo;

	while ((mo = iterator.Next()))
	{
		hiddenUpdates = SV_UpdateHiddenMobj(pl, mo, hiddenUpdates);
		if (hiddenUpdates >= MAX_HIDDEN_MOBJ_UPDATES)
		{
			break;
		}
	}

	// update flags
	if (sv_gametype == GM_CTF)
		CTF_Connect(pl);

	SV_UpdateSectors(cl);

	P_UpdateButtons(cl);

	SV_LineStateUpdate(cl);

	SV_ThinkerUpdate(cl);

	SV_SendPlayerInfo(pl);

	MSG_WriteSVC(cl->messenger.ReliableBuf(), odaproto::svc::FullUpdateDone());

	SV_SendPacket(pl);
}

//===========================
// SV_UpdateSecret
// Updates a sector to a client and the number of secrets found.
//===========================
void SV_UpdateSecret(sector_t& sector, player_t &player)
{
	// Don't announce secrets on PvP gamemodes
	if (!G_IsCoopGame())
		return;

	for (Players::iterator it = players.begin(); it != players.end(); ++it)
	{
		client_t* cl = &(it->client);

		MSG_WriteSVC(cl->messenger.ReliableBuf(), SVC_LevelLocals(::level, SVC_LL_SECRETS));
		MSG_WriteSVC(cl->messenger.ReliableBuf(), SVC_PlayerMembers(player, SVC_PM_SCORE));

		if (&*it == &player)
			continue;

		if (!(sector.special & SECRET_MASK) && sector.secretsector)
			MSG_WriteSVC(cl->messenger.ReliableBuf(), SVC_SecretEvent(player, sector));
	}
}

//
//	SendServerSettings
//
//	Sends server setting info
//

void SV_SendPackets(void);

static void SendServerSettings(player_t& pl)
{
	client_t* cl = &pl.client;

	// GhostlyDeath <June 19, 2008> -- Loop through all CVARs and send the CVAR_SERVERINFO
	// stuff only
	cvar_t* var = GetFirstCvar();

	while (var)
	{
		if (var->flags() & CVAR_SERVERINFO)
		{
			odaproto::svc::ServerSettings settings = SVC_ServerSettings(*var);

			MSG_WriteSVC(cl->messenger.ReliableBuf(), settings);
		}

		var = var->GetNext();
	}
}

//
//	SV_ServerSettingChange
//
//	Sends server settings to clients when changed
//
void SV_ServerSettingChange()
{
	if (gamestate != GS_LEVEL)
	{
		return;
	}

	for (auto& player : players)
	{
		SendServerSettings(player);
	}
}

// SV_CheckClientVersion
bool SV_CheckClientVersion(client_t *cl, Players::iterator it)
{
	int GameVer = 0;
	std::string VersionStr;
	std::string OurVersionStr(DOTVERSIONSTR);
	bool AllowConnect = true;
	int cl_major = 0;
	int cl_minor = 0;
	int cl_patch = 0;

	switch (cl->version)
	{
	case VERSION:
		GameVer = MSG_ReadLong();
		BREAKVER(GameVer, cl_major, cl_minor, cl_patch);

		VersionStr = fmt::sprintf("%d.%d.%d", cl_major, cl_minor, cl_patch);

		cl->packedversion = GameVer;

		// Major and minor versions must be identical, client is allowed
		// to have a newer patch.
		if (VersionCompat(GAMEVER, GameVer) == 0)
			AllowConnect = true;
		else
			AllowConnect = false;
		break;
	case 64:
		VersionStr = "0.2a or 0.3";
		break;
	case 63:
		VersionStr = "Pre-0.2";
		break;
	case 62:
		VersionStr = "0.1a";
		break;
	default:
		VersionStr = "Unknown";
		break;
	}

	// GhostlyDeath -- removes the constant AllowConnects above
	if (cl->version != VERSION)
		AllowConnect = false;

	// GhostlyDeath -- boot em
	if (!AllowConnect)
	{
		std::string msg = VersionMessage(GAMEVER, GameVer, ::sv_email.cstring());
		if (msg.empty())
		{
			// Failsafe.
			msg = fmt::sprintf(
			    "Your version of Odamex does not match the server %s.\nFor updates, "
			    "visit https://odamex.net/\n",
			    DOTVERSIONSTR);
		}

		// GhostlyDeath -- Now we tell them our built up message and boot em
		cl->displaydisconnect = false;	// Don't spam the players

		MSG_WriteSVC(cl->messenger.ReliableBuf(), SVC_Print(PRINT_WARNING, msg));

		MSG_WriteSVC(cl->messenger.ReliableBuf(), SVC_Disconnect());

		SV_SendPacket(*it);

		// GhostlyDeath -- And we tell the server
		PrintFmt("{} disconnected (version mismatch {}).\n", NET_AdrToString(::net_from),
		         VersionStr);
	}

	return AllowConnect;
}

/**
 * @brief Disconnect an older client using the older protocol.
 */
static void SV_DisconnectOldClient()
{
	int cl_version = MSG_ReadShort();
	MSG_ReadByte(); //connection_type (unused)
	std::string VersionStr;

	int GameVer = 0;
	if (cl_version == VERSION)
	{
		GameVer = MSG_ReadLong();
	}
	else
	{
		// Assume anything older is 0.3.0.
		GameVer = MAKEVER(0, 3, 0);
	}

	int cl_maj, cl_min, cl_pat;
	BREAKVER(GameVer, cl_maj, cl_min, cl_pat);

	std::string msg = VersionMessage(GAMEVER, GameVer, ::sv_email.cstring());
	if (msg.empty())
	{
		// Failsafe.
		msg = fmt::sprintf(
		          "Your version of Odamex does not match the server %s.\nFor updates, "
		          "visit https://odamex.net/\n",
		          DOTVERSIONSTR);
	}

	// Send using the old protocol mechanism without relying on any defines
	const byte old_svc_disconnect = 2;
	const byte old_svc_print = 28;
	const int old_PRINT_HIGH = 2;

	static buf_t smallbuf(1024);

	MSG_WriteLong(&smallbuf, 0);

	MSG_WriteByte(&smallbuf, old_svc_print);
	MSG_WriteByte(&smallbuf, old_PRINT_HIGH);
	MSG_WriteString(&smallbuf, msg.c_str());

	MSG_WriteByte(&smallbuf, old_svc_disconnect);

	NET_SendPacket(smallbuf, ::net_from);

	PrintFmt("{} disconnected (version mismatch {}.{}.{}).\n", NET_AdrToString(::net_from),
	         cl_maj, cl_min, cl_pat);
}

void G_DoReborn(player_t& playernum);

//
//	SV_ConnectClient
//
//	Called when a client connects
//
void SV_ConnectClient()
{
	int challenge = MSG_ReadLong();

	// New querying system
	if (SV_QryParseEnquiry(challenge) == 0)
		return;

	if (challenge == LAUNCHER_CHALLENGE)  // for Launcher
	{
		SV_SendServerInfo();
		return;
	}

	if (challenge != PROTO_CHALLENGE && challenge != MSG_CHALLENGE)
		return;

	if (!SV_IsValidToken(MSG_ReadLong()))
		return;

	PrintFmt("{} is trying to connect...\n", NET_AdrToString (net_from));

	// Show old challenges the door only after we've validated their token.
	if (challenge == MSG_CHALLENGE)
	{
		SV_DisconnectOldClient();
		return;
	}

	// find an open slot
	Players::iterator it = SV_GetFreeClient();

	if (it == players.end()) // a server is full
	{
		PrintFmt("{} disconnected (server full).\n", NET_AdrToString (net_from));

		static buf_t smallbuf(1024);
		if (smallbuf.size() == 0)
		{
            PacketHeaderType header(0);
            header.Pack(smallbuf);
			MSG_WriteSVCBuffer(&smallbuf, SVC_Disconnect("Server is full\n"));
		}

		NET_SendPacket(smallbuf, net_from);
		return;
	}

	player_t* player = &(*it);
	client_t* cl = &(player->client);

	// clear and reinitialize client network info
	cl->address = net_from;
	cl->last_received = gametic;
	cl->lastclientcmdtic = 0;
	cl->allow_rcon = false;
	cl->displaydisconnect = false;

	cl->messenger = OdaMessenger();

	// generate a random string
	std::stringstream ss;
	ss << time(NULL) << level.time << VERSION << NET_AdrToString(net_from);
	cl->digest = MD5SUM(ss.str());

	// Set player time
	player->JoinTime = time(NULL);

	cl->version = MSG_ReadShort();
	MSG_ReadByte(); //connection_type (unused)

	// [SL] 2011-05-11 - Register the player with the reconciliation system
	// for unlagging
	Unlag::getInstance().registerPlayer(player->id);

	// Check if the client uses the same version as the server.
	if (!SV_CheckClientVersion(cl, it))
	{
		SV_DropClient(*player);
		return;
	}

	// Get the userinfo from the client.
	clc_t userinfo = (clc_t)MSG_ReadByte();
	if (userinfo != clc_userinfo)
	{
		SV_InvalidateClient(*player, "Client didn't send any userinfo");
		return;
	}

	if (!SV_SetupUserInfo(*player))
		return;

	// [SL] Read and ignore deprecated client rate. Clients now always use sv_maxrate.
	MSG_ReadLong();
	cl->messenger.SetMaxRate(int(sv_maxrate));

	// Check if the IP is banned from our list or not.
	if (SV_BanCheck(cl))
	{
		cl->displaydisconnect = false;
		SV_DropClient(*player);
		return;
	}

	// Check if the user entered a good password (if any)
	std::string passhash = MSG_ReadString();
	if (strlen(join_password.cstring()) && MD5SUM(join_password.cstring()) != passhash)
	{
		PrintFmt("{} disconnected (password failed).\n", NET_AdrToString(net_from));

		MSG_WriteSVC(
		    cl->messenger.ReliableBuf(),
		    SVC_Print(PRINT_HIGH,
		              "Server is passworded, no password specified or bad password.\n"));

		SV_SendPacket(*player);
		SV_DropClient(*player);
		return;
	}

	// send consoleplayer number
	MSG_WriteSVC(cl->messenger.ReliableBuf(), SVC_ConsolePlayer(*player, cl->digest));
	SV_SendPacket(*player);
}

void SV_ConnectClient2(player_t& player)
{
	client_t* cl = &player.client;

	// [AM] FIXME: I don't know if it's safe to set players as PST_ENTER
	//             this early.
	player.playerstate = PST_LIVE;

	// [Toke] send server settings
	SendServerSettings(player);

	cl->displaydisconnect = true;

	cl->download.name = "";
	cl->download.md5 = "";

	SV_BroadcastUserInfo(player);

	// Newly connected players get ENTER state.
	P_ClearPlayerScores(player, SCORES_CLEAR_ALL);
	player.playerstate = PST_ENTER;

	if (!step_mode)
	{
		player.spectator = true;
		for (Players::iterator pit = players.begin(); pit != players.end(); ++pit)
		{
			MSG_WriteSVC(pit->client.messenger.ReliableBuf(),
			             SVC_PlayerMembers(player, SVC_PM_SPECTATOR));
		}
	}

	// Send a map name
	MSG_WriteSVC(player.client.messenger.ReliableBuf(),
	             SVC_LoadMap(::wadfiles, ::patchfiles, level.mapname.c_str(), level.time));

	// [SL] 2011-12-07 - Force the player to jump to intermission if not in a level
	if (gamestate == GS_INTERMISSION)
	{
		MSG_WriteSVC(cl->messenger.ReliableBuf(), odaproto::svc::ExitLevel());
	}

	G_DoReborn(player);
	SV_ClientFullUpdate(player);

	SV_BroadcastPrintFmt("{} has connected.\n", player.userinfo.netname);

	// tell others clients about it
	for (Players::iterator pit = players.begin(); pit != players.end(); ++pit)
	{
		MSG_WriteSVC(pit->client.messenger.ReliableBuf(), SVC_ConnectClient(player));
	}

	// Notify this player of other player's queue positions
	SV_SendPlayerQueuePositions(&player, true);

	// Send out the server's MOTD.
	SV_MidPrint((char*)sv_motd.cstring(), &player, 6);
}


//
// SV_BuildKillsDeathsStatusString
//
std::string SV_BuildKillsDeathsStatusString(const player_t& player)
{
	std::string status;

	if (player.playerstate == PST_DOWNLOAD)
		status = "downloading";
	else if (player.playerstate == PST_DISCONNECT && player.spectator)
		status = "SPECTATOR";
	else
	{
		if (G_IsTeamGame())
		{
			status += fmt::format("{} TEAM, ", GetTeamInfo(player.userinfo.team)->ColorStringUpper);
		}

		// Points (CTF).
		if (sv_gametype == GM_CTF)
		{
			status += fmt::format("{} POINTS, ", player.points);
		}

		// Frags (DM/TDM/CTF) or Kills (Coop).
		if (G_IsCoopGame())
			status += fmt::format("{} KILLS, ", player.killcount);
		else
			status += fmt::format("{} FRAGS, ", player.fragcount);

		// Deaths.
		status += fmt::format("{} DEATHS", player.deathcount);
	}
	return status;
}


//
// SV_DisconnectClient
//
void SV_DisconnectClient(player_t &who)
{
	std::string disconnectmessage;

	// already gone though this procedure?
	if (who.playerstate == PST_DISCONNECT)
		return;

	// tell others clients about it
	for (auto& player : players)
	{
		client_t &cl = player.client;
		MSG_WriteSVC(cl.messenger.ReliableBuf(), SVC_DisconnectClient(who));
	}

	// Put the disconnecting client's final message on the wire right away.
	// We do this so that we don't have to wait for the next tic before we get
	// the message out and we put the messenger into the dead-end collection with a
	// pending Ack count > 0.
	who.client.messenger.SendAll(gametic, who.client.address);
	SV_DepartMessenger(who.client);

	Maplist_Disconnect(who);
	Vote_Disconnect(who);

	who.playerstate = PST_DISCONNECT;

	if (who.client.displaydisconnect)
	{
		// print some final stats for the disconnected player
		SV_BroadcastPrintFmt("{} disconnected. ({})\n",
		                     who.userinfo.netname,
		                     SV_BuildKillsDeathsStatusString(who));
	}

	SV_UpdatePlayerQueuePositions(G_CanJoinGame, &who);
}

//
// SV_DropClient
// Called when the player is leaving the server unwillingly.
//
void SV_DropClient(player_t &who)
{
	client_t *cl = &who.client;

	MSG_WriteSVC(cl->messenger.ReliableBuf(), SVC_Disconnect());

	SV_SendPacket(who);

	SV_DisconnectClient(who);

	if (::debug_disconnect)
		PrintFmt("{}\n", M_GetStacktrace("Disconnect location:", false));
}

//
// SV_SendAndFlushClientsFinalSignal
//
static void SV_SendAndFlushClientsFinalSignal(const google::protobuf::Message& finalMessage)
{
	// Push out one last reliable message - the Disconnect command.
	for (auto& player : players)
	{
		player.client.messenger.Clear();
		MSG_WriteSVC(player.client.messenger.ReliableBuf(), finalMessage);
		SV_SendPacket(player);

		// Move the client's messenger to the departing messenger manager.
		SV_DepartMessenger(player.client);

		if (player.mo)
			player.mo->Destroy();
	}

	// Now flush the messengers until everyone's acked every reliable message we've sent, including the
	// final Disconnect command.
	int fakeTic = gametic;
	size_t remainingMessengerCount = players.size();
	const dtime_t timeoutDeadline = I_GetTime() + I_ConvertTimeFromMs(2000); // .. but don't try for very long!
	while (remainingMessengerCount > 0 and I_GetTime() < timeoutDeadline)
	{
		I_WaitVBL(1);
		while (NET_GetPacket())
		{
			SV_HandleDepartingMessengerPacket(++fakeTic, net_from, ::net_message);
		}

		remainingMessengerCount = SV_CheckDepartingMessengers(fakeTic);
	}

	if (remainingMessengerCount > 0)
	{
		PrintFmt(PRINT_WARNING, "{} clients did not acknowledge the server shutdown signal\n", remainingMessengerCount);
	}

	players.clear();
}

//
// SV_SendAndFlushDisconnectSignal
// All clients will leave and idle at the console.
//
void SV_SendAndFlushDisconnectSignal()
{
    SV_SendAndFlushClientsFinalSignal(SVC_Disconnect("Shutting down\n"));
}

//
// SV_SendAndFlushReconnectSignal
// All clients will reconnect.
//
void SV_SendAndFlushReconnectSignal()
{
    SV_SendAndFlushClientsFinalSignal(odaproto::svc::Reconnect());
}

//
// SV_ExitLevel
// Called when sv_timelimit or sv_fraglimit hit.
//
void SV_ExitLevel()
{
	for (auto& player : players)
	{
		MSG_WriteSVC((player.client.messenger.ReliableBuf()), odaproto::svc::ExitLevel());
	}
}

//
// Comparison functors for sorting vectors of players
//
struct compare_player_frags
{
	bool operator()(const player_t* arg1, const player_t* arg2) const
	{
		if (!G_IsDuelGame() && G_IsRoundsGame())
		{
			return arg2->totalpoints < arg1->totalpoints;
		}

		return arg2->fragcount < arg1->fragcount;
	}
};

struct compare_player_kills
{
	bool operator()(const player_t* arg1, const player_t* arg2) const
	{
		return arg2->killcount < arg1->killcount;
	}
};

struct compare_player_points
{
	bool operator()(const player_t* arg1, const player_t* arg2) const
	{
		if (G_IsRoundsGame())
		{
			return arg2->totalpoints < arg1->totalpoints;
		}

		return arg2->points < arg1->points;
	}
};

struct compare_player_names
{
	bool operator()(const player_t* arg1, const player_t* arg2) const
	{
		return arg1->userinfo.netname < arg2->userinfo.netname;
	}
};


static float SV_CalculateKillDeathRatio(const player_t* player)
{
	if (player->killcount <= 0)	// Copied from HU_DMScores1.
		return 0.0f;
	else if (player->killcount >= 1 && player->deathcount == 0)
		return float(player->killcount);
	else
		return float(player->killcount) / float(player->deathcount);
}

static float SV_CalculateFragDeathRatio(const player_t* player)
{

	int frags = 0;
	int deaths = 0;

	if (G_IsRoundsGame() && !G_IsDuelGame())
	{
		frags = player->totalpoints;
		deaths = player->totaldeaths;
	}
	else
	{
		frags = player->fragcount;
		deaths = player->deathcount;
	}

	if (frags <= 0) // Copied from HU_DMScores1.
		return 0.0f;
	else if (frags >= 1 && deaths == 0)
		return float(frags);
	else
		return float(frags) / float(deaths);
}

//
// SV_DrawScores
// Draws scoreboard to console. Used during level exit or a command.
//
// [AM] Commonize this with client.
//
void SV_DrawScores()
{
	std::string str;

	typedef std::list<const player_t*> PlayerPtrList;
	PlayerPtrList sortedplayers;
	PlayerPtrList sortedspectators;

	for (const auto& player : players)
		if (!player.spectator && player.ingame())
			sortedplayers.push_back(&player);
		else
			sortedspectators.push_back(&player);

	PrintFmt_Bold("\n");

	if (sv_gametype == GM_CTF)
	{
		compare_player_points comparison_functor;
		sortedplayers.sort(comparison_functor);

        PrintFmt_Bold("                    CAPTURE THE FLAG");
        PrintFmt_Bold("-----------------------------------------------------------");

		if (sv_scorelimit)
			str = fmt::format("Scorelimit: {:<6d}", sv_scorelimit.asInt());
		else
			str = fmt::format("Scorelimit: N/A   ");

		PrintFmt_Bold("{}  ", str);

		if (sv_timelimit)
			str = fmt::format("Timelimit: {:<7d}", sv_timelimit.asInt());
		else
			str = fmt::format("Timelimit: N/A");

		PrintFmt_Bold("{:18s}\n", str);

		for (int team_num = 0; team_num < sv_teamsinplay; team_num++)
		{
			if (team_num == TEAM_BLUE)
                PrintFmt_Bold("--------------------------------------------------BLUE TEAM");
			else if (team_num == TEAM_RED)
                PrintFmt_Bold("---------------------------------------------------RED TEAM");
			else if (team_num == TEAM_GREEN)
				PrintFmt_Bold("-------------------------------------------------GREEN TEAM");
			else		// shouldn't happen
                PrintFmt_Bold("-----------------------------------------------UNKNOWN TEAM");

            PrintFmt_Bold("ID  Address          Name            Points Caps Frags Time");
            PrintFmt_Bold("-----------------------------------------------------------");

			for (const auto& player : sortedplayers)
			{
				if (player->userinfo.team == team_num)
				{
					PrintFmt_Bold("{:<3d} {:<16s} {:<15s} {:<6d} N/A  {:<5d} {:<3d}",
					              player->id,
					              NET_AdrToString(player->client.address),
					              player->userinfo.netname,
					              P_GetPointCount(*player),
					              //itplayer->captures,
					              P_GetFragCount(*player),
					              player->GameTime / 60);
				}
			}
		}
	}

	else if (sv_gametype == GM_TEAMDM)
	{
		compare_player_frags comparison_functor;
		sortedplayers.sort(comparison_functor);

        PrintFmt_Bold("                     TEAM DEATHMATCH");
        PrintFmt_Bold("-----------------------------------------------------------");

		if (sv_fraglimit)
			str = fmt::format("Fraglimit: {:<7d}", sv_fraglimit.asInt());
		else
			str = fmt::format("Fraglimit: N/A    ");

		PrintFmt_Bold("{}  ", str);

		if (sv_timelimit)
			str = fmt::format("Timelimit: {:<7d}", sv_timelimit.asInt());
		else
			str = fmt::format("Timelimit: N/A");

		PrintFmt_Bold("{:18s}\n", str);

		for (int team_num = 0; team_num < sv_teamsinplay; team_num++)
		{
			if (team_num == TEAM_BLUE)
                PrintFmt_Bold("--------------------------------------------------BLUE TEAM");
			else if (team_num == TEAM_RED)
                PrintFmt_Bold("---------------------------------------------------RED TEAM");
			else if (team_num == TEAM_GREEN)
				PrintFmt_Bold("-------------------------------------------------GREEN TEAM");
			else		// shouldn't happen
                PrintFmt_Bold("-----------------------------------------------UNKNOWN TEAM");

            PrintFmt_Bold("ID  Address          Name            Frags Deaths  K/D Time");
            PrintFmt_Bold("-----------------------------------------------------------");

			for (const auto& player : sortedplayers)
			{
				if (player->userinfo.team == team_num)
				{
					PrintFmt_Bold("{:<3d} {:<16s} {:<15s} {:<5d} {:<6d} {:2.1f} {:<3d}",
					              player->id,
					              NET_AdrToString(player->client.address),
					              player->userinfo.netname,
					              P_GetFragCount(*player),
					              P_GetDeathCount(*player),
					              SV_CalculateFragDeathRatio(player),
					              player->GameTime / 60);
				}
			}
		}
	}

	else if (sv_gametype == GM_DM)
	{
		compare_player_frags comparison_functor;
		sortedplayers.sort(comparison_functor);

        PrintFmt_Bold("                        DEATHMATCH");
        PrintFmt_Bold("-----------------------------------------------------------");

		if (sv_fraglimit)
			str = fmt::format("Fraglimit: {:<7d}", sv_fraglimit.asInt());
		else
			str = fmt::format("Fraglimit: N/A    ");

		PrintFmt_Bold("{}  ", str);

		if (sv_timelimit)
			str = fmt::format("Timelimit: {:<7d}", sv_timelimit.asInt());
		else
			str = fmt::format("Timelimit: N/A");

		PrintFmt_Bold("{:18s}\n", str);

        PrintFmt_Bold("ID  Address          Name            Frags Deaths  K/D Time");
        PrintFmt_Bold("-----------------------------------------------------------");

		for (const auto& player : sortedplayers)
		{
			PrintFmt_Bold("{:<3d} {:<16s} {:<15s} {:<5d} {:<6d} {:2.1f} {:<3d}",
			              player->id,
			              NET_AdrToString(player->client.address),
			              player->userinfo.netname,
			              P_GetFragCount(*player),
			              P_GetDeathCount(*player),
			              SV_CalculateFragDeathRatio(player),
			              player->GameTime / 60);
		}

	}

	else if (G_IsCoopGame())
	{
		compare_player_kills comparison_functor;
		sortedplayers.sort(comparison_functor);

        PrintFmt_Bold("                       COOPERATIVE");
        PrintFmt_Bold("-----------------------------------------------------------");
        PrintFmt_Bold("ID  Address          Name            Kills Deaths  K/D Time");
        PrintFmt_Bold("-----------------------------------------------------------");

		for (const auto& player : sortedplayers)
		{
			PrintFmt_Bold("{:<3d} {:<16s} {:<15s} {:<5d} {:<6d} {:2.1f} {:<3d}",
			              player->id,
			              NET_AdrToString(player->client.address),
			              player->userinfo.netname,
			              player->killcount,
			              player->deathcount,
			              SV_CalculateKillDeathRatio(player),
			              player->GameTime / 60);
		}
	}

	if (!sortedspectators.empty())
	{
		compare_player_names comparison_functor;
		sortedspectators.sort(comparison_functor);

    	PrintFmt_Bold("-------------------------------------------------SPECTATORS");

		for (const auto& spec : sortedspectators)
		{
			PrintFmt_Bold("{:<3d} {:<16s} {:<15s}\n",
			              spec->id,
			              NET_AdrToString(spec->client.address),
			              spec->userinfo.netname);
		}
	}

	PrintFmt_Bold("\n");
}

BEGIN_COMMAND (showscores)
{
    SV_DrawScores();
}
END_COMMAND (showscores)

/**
 * Send a message to teammates of a player.
 *
 * @param player  Player who said the message.
 * @param message Message that the player said.
 */
void SVC_TeamSay(player_t &player, const char* message)
{
	for (Players::iterator it = players.begin(); it != players.end(); ++it)
	{
		// Player needs to be valid.
		if (!validplayer(*it))
			continue;

		bool spectator = it->spectator || !it->ingame();

		// Player needs to be on the same team
		if (spectator || it->userinfo.team != player.userinfo.team)
			continue;

		MSG_WriteSVC(it->client.messenger.ReliableBuf(), SVC_Say(true, player.id, message));
	}
}

/**
 * Send a message to all spectators.
 *
 * @param player  Player who said the message.
 * @param message Message that the player said.
 */
void SVC_SpecSay(player_t &player, const char* message)
{
	if (strnicmp(message, "/me ", 4) == 0)
		PrintFmt(PRINT_TEAMCHAT, "<SPEC> * {} {}\n", player.userinfo.netname, &message[4]);
	else
		PrintFmt(PRINT_TEAMCHAT, "<SPEC> {}: {}\n", player.userinfo.netname, message);

	for (Players::iterator it = players.begin(); it != players.end(); ++it)
	{
		// Player needs to be valid.
		if (!validplayer(*it))
			continue;

		bool spectator = it->spectator || !it->ingame();

		if (!spectator)
			continue;

		MSG_WriteSVC(it->client.messenger.ReliableBuf(), SVC_Say(true, player.id, message));
	}
}

/**
 * Send a message to everybody.
 *
 * @param player  Player who said the message.
 * @param message Message that the player said.
 */
void SVC_Say(player_t &player, const char* message)
{
	if (strnicmp(message, "/me ", 4) == 0)
		PrintFmt(PRINT_CHAT, "<CHAT> * {} {}\n", player.userinfo.netname, &message[4]);
	else
		PrintFmt(PRINT_CHAT, "<CHAT> {}: {}\n", player.userinfo.netname, message);

	for (Players::iterator it = players.begin(); it != players.end(); ++it)
	{
		// Player needs to be valid.
		if (!validplayer(*it))
			continue;

		MSG_WriteSVC(it->client.messenger.ReliableBuf(), SVC_Say(false, player.id, message));
	}
}

/**
 * Send a message to a specific player from a specific other player.
 *
 * @param player  Sending player.
 * @param dplayer Player to send to.
 * @param message Message to send.
 */
void SVC_PrivMsg(player_t &player, player_t &dplayer, const char* message)
{
	if (strnicmp(message, "/me ", 4) == 0)
		PrintFmt(PRINT_CHAT, "<PRIVMSG> * {} (to {}) {}\n",
				player.userinfo.netname, dplayer.userinfo.netname, &message[4]);
	else
		PrintFmt(PRINT_CHAT, "<PRIVMSG> {} (to {}): {}\n",
				player.userinfo.netname, dplayer.userinfo.netname, message);

	MSG_WriteSVC(dplayer.client.messenger.ReliableBuf(), SVC_Say(true, player.id, message));

	// [AM] Send a duplicate message to the sender, so he knows the message
	//      went through.
	if (player.id != dplayer.id)
	{
		MSG_WriteSVC(player.client.messenger.ReliableBuf(), SVC_Say(true, player.id, message));
	}
}

//
// SV_Say
// Show a chat string and send it to others clients.
//
bool SV_Say(player_t &player)
{
	byte message_visibility = MSG_ReadByte();

	std::string message(MSG_ReadString());
	StripColorCodes(message);

	if (!ValidString(message))
	{
		SV_InvalidateClient(player, "Chatstring contains invalid characters");
		return false;
	}

	if (message.empty() || message.length() > MAX_CHATSTR_LEN)
		return true;

	// Flood protection
	if (player.LastMessage.Time)
	{
		const dtime_t min_diff = I_ConvertTimeFromMs(1000) * sv_flooddelay;
		dtime_t diff = I_GetTime() - player.LastMessage.Time;

		if (diff < min_diff)
			return true;

		player.LastMessage.Time = 0;
	}

	if (player.LastMessage.Time == 0)
	{
		player.LastMessage.Time = I_GetTime();
		player.LastMessage.Message = message;
	}

	bool spectator = player.spectator || !player.ingame();

	if (message_visibility == 0)
	{
		if (spectator && !sv_globalspectatorchat)
			SVC_SpecSay(player, message.c_str());
		else
			SVC_Say(player, message.c_str());
	}
	else if (message_visibility == 1)
	{
		if (spectator)
			SVC_SpecSay(player, message.c_str());
		else if (G_IsTeamGame())
			SVC_TeamSay(player, message.c_str());
		else
			SVC_Say(player, message.c_str());
	}

	return true;
}

//
// SV_PrivMsg
// Show a chat string and show it to a single other client.
//
bool SV_PrivMsg(player_t &player)
{
	player_t& dplayer = idplayer(MSG_ReadByte());

	std::string str(MSG_ReadString());
	StripColorCodes(str);

	if (!ValidString(str))
	{
		SV_InvalidateClient(player, "Private Message contains invalid characters");
		return false;
	}

	if (!validplayer(dplayer))
		return true;

	if (str.empty() || str.length() > MAX_CHATSTR_LEN)
		return true;

	// In competitive gamemodes, don't allow spectators to message players.
	if (!G_IsCoopGame() && player.spectator && !dplayer.spectator)
		return true;

	// Flood protection
	if (player.LastMessage.Time)
	{
		const dtime_t min_diff = I_ConvertTimeFromMs(1000) * sv_flooddelay;
		dtime_t diff = I_GetTime() - player.LastMessage.Time;

		if (diff < min_diff)
			return true;

		player.LastMessage.Time = 0;
	}

	if (player.LastMessage.Time == 0)
	{
		player.LastMessage.Time = I_GetTime();
		player.LastMessage.Message = str;
	}

	SVC_PrivMsg(player, dplayer, str.c_str());

	return true;
}

//
// SV_UpdateMissiles
// Updates missiles position sometimes.
//
void SV_UpdateMissiles(player_t &pl, AActor *mo)
{
	if (!(mo->flags & MF_MISSILE) || mo->flags & MF_SKULLFLY)
		return;

	if (mo->type == MT_PLASMA)
		return;

	// update missile position every 30 tics
	if (((gametic+mo->netid) % 30) && (mo->type != MT_TRACER) && (mo->type != MT_FATSHOT) && !(mo->flags2 & MF2_SEEKERMISSILE))
		return;

	// Revenant tracers and Mancubus fireballs need to be updated more often (and custom tracers)
	if (((gametic+mo->netid) % 5) && (mo->type == MT_TRACER || mo->type == MT_FATSHOT || mo->flags2 & MF2_SEEKERMISSILE))
		return;

	if(SV_IsPlayerAllowedToSee(pl, mo))
	{
		MSG_WriteSVC(pl.client.messenger.NetBuf(), SVC_UpdateMobj(*mo));
	}
}

// Update the given actors data immediately.
void SV_UpdateMobj(const AActor* mo)
{
	// Don't use this function to update players.
	if (mo->player)
		return;

	for (auto& player : players)
	{
		if (!(player.ingame()))
			continue;

		if (SV_IsPlayerAllowedToSee(player, mo))
		{
			MSG_WriteSVC(player.client.messenger.ReliableBuf(), SVC_UpdateMobj(*mo));
		}
	}
}

// Update the given actors state immediately.
void SV_UpdateMobjState(const AActor* mo)
{
	for (auto& player : players)
	{
		if (!(player.ingame()))
			continue;

		if (SV_IsPlayerAllowedToSee(player, mo))
		{
			MSG_WriteSVC(player.client.messenger.ReliableBuf(), SVC_MobjState(mo));
		}
	}
}

// Keep tabs on monster positions and angles.
void SV_UpdateMonsters(player_t &pl, AActor *mo)
{
	// Ignore corpses.
	if (mo->flags & MF_CORPSE)
		return;

	// We don't handle updating non-monsters here.
	if (!(mo->flags & MF_COUNTKILL || mo->type == MT_SKULL))
		return;

	// update monster position every 7 tics
	if ((gametic+mo->netid) % 7)
		return;

	if (SV_IsPlayerAllowedToSee(pl, mo) && mo->target)
	{
		MSG_WriteSVC(pl.client.messenger.NetBuf(), SVC_UpdateMobj(*mo));
	}
}

void SV_UpdateGametype(player_t& pl)
{
	if (G_IsHordeMode())
	{
		static hordeInfo_t lastInfo = {HS_STARTING, -1, -1, -1, 0, -1, -1, -1, -1, -1};
		static int ticsent;

		// If the hordeinfo has changed since last tic, save and send it.
		if (ticsent != ::gametic)
		{
			const hordeInfo_t info = P_HordeInfo();
			if (!info.equals(lastInfo))
			{
				memcpy(&lastInfo, &info, sizeof(hordeInfo_t));
				ticsent = ::gametic;
			}
		}

		// Send it if we're on the tic it mutated on or to a fresh player.
		if (ticsent == ::gametic || (pl.GameTime == 0 && pl.ingame()))
		{
			MSG_WriteSVC(pl.client.messenger.NetBuf(), SVC_HordeInfo(lastInfo));
		}
	}
}

//
// SV_ActorTarget
//
void SV_ActorTarget(const AActor *actor)
{
	if (actor->player)
		return;

	for (auto& player : players)
	{
		if (!(player.ingame()))
			continue;

		client_t *cl = &(player.client);

		if(!SV_IsPlayerAllowedToSee(player, actor))
			continue;

		MSG_WriteSVC(cl->messenger.ReliableBuf(), SVC_UpdateMobj(*actor));
	}
}

//
// SV_ActorTracer
//
void SV_ActorTracer(const AActor *actor)
{
	for (auto& player : players)
	{
		if (!(player.ingame()))
			continue;

		client_t *cl = &(player.client);

		MSG_WriteSVC(cl->messenger.ReliableBuf(), SVC_UpdateMobj(*actor));
	}
}

//
// SV_RemoveCorpses
// Removes some corpses
//
void SV_RemoveCorpses (void)
{
	AActor *mo;
	int     corpses = 0;

	// joek - Number of corpses infinite
	if(sv_maxcorpses <= 0)
		return;

	if (!P_AtInterval(TICRATE))
		return;
	else
	{
		TThinkerIterator<AActor> iterator;
		while ( (mo = iterator.Next() ) )
		{
			if (mo->type == MT_PLAYER && (!mo->player || mo->health <=0) )
				corpses++;
		}
	}

	TThinkerIterator<AActor> iterator;
	while (corpses > sv_maxcorpses && (mo = iterator.Next() ) )
	{
		if (mo->type == MT_PLAYER && !mo->player)
		{
			mo->Destroy();
			corpses--;
		}
	}
}

//
// SV_SendPingRequest
// Pings the client and requests a reply
//
// [SL] 2011-05-11 - Changed from SV_SendGametic to SV_SendPingRequest
//
void SV_SendPingRequest(client_t* cl)
{
	if (!P_AtInterval(100))
		return;

	MSG_WriteSVC(cl->messenger.ReliableBuf(), SVC_PingRequest());
}

void SV_UpdateMonsterRespawnCount()
{
	if (!G_IsCoopGame())
		return;

	for (auto& player : players)
	{
		client_t* cl = &(player.client);
		MSG_WriteSVC(cl->messenger.ReliableBuf(), SVC_LevelLocals(::level, SVC_LL_MONSTER_RESPAWNS));
	}
}

// calculates ping using gametic which was sent by SV_SendGametic and
// current gametic
void SV_CalcPing(player_t &player)
{
	unsigned int ping = I_MSTime() - MSG_ReadLong();

	if(ping > MAX_PING)
		ping = MAX_PING;

	player.ping = ping;
}

//
// SV_UpdatePing
// send pings to a client
//
void SV_UpdatePing(client_t* cl)
{
	if (!P_AtInterval(101))
		return;

	for (auto& player : players)
	{
		if (!(player.ingame()))
			continue;

		MSG_WriteSVC(cl->messenger.ReliableBuf(), SVC_UpdatePing(player));
	}
}


//
// SV_UpdateDeadPlayers
// Update player's frame while he's dying.
//
void SV_UpdateDeadPlayers()
{
 /*   AActor *mo;

    TThinkerIterator<AActor> iterator;
    while ( (mo = iterator.Next() ) )
    {
        if (mo->type != MT_PLAYER || mo->player)
			continue;

		if (mo->oldframe != mo->frame)
			for (size_t i = 0; i < players.size(); i++)
			{
				client_t *cl = &clients[i];

				MSG_WriteMarker (&cl->messenger.ReliableBuf(), svc_mobjframe);
				MSG_WriteUnVarint (&cl->messenger.ReliableBuf(), mo->netid);
				MSG_WriteByte (&cl->messenger.ReliableBuf(), mo->frame);
			}

		mo->oldframe = mo->frame;
    }
*/
}


//
// SV_SendPackets
//
void SV_SendPackets()
{
	if (players.empty())
		return;


	std::vector<std::future<void>> futures;

	for (auto& player : players)
	{
		// Disconnecting players' messengers send their packets via the dead-end messenger collection.
		if (player.playerstate != PST_DISCONNECT)
		{
			std::packaged_task<void ()> task { [&player] () { SV_SendPacket(player); } };

			futures.emplace_back(task.get_future());

			s_workers.MoveCommand(std::move(task));
		}
	}

	for (auto& future : futures)
	{
		future.wait();
	}
}

void SV_SendPlayerStateUpdate(client_t *client, player_t *player)
{
	if (!client || !player || !player->mo)
		return;

	MSG_WriteSVC(client->messenger.NetBuf(), SVC_PlayerState(*player));
}

void SV_SpyPlayer(player_t &viewer)
{
	byte id = MSG_ReadByte();

	player_t &other = idplayer(id);
	if (!validplayer(other) || !P_CanSpy(viewer, other))
		return;

	viewer.spying = id;
	SV_SendPlayerStateUpdate(&viewer.client, &other);
}

static void SV_SortMobjsForPlayer(player_t& player)
{
	// Put in a static assert for assurance that the vector-of-pointers clear() will
	// actually be constant-time.
	static_assert(std::is_trivially_destructible_v<decltype(player.sortedMobjs)::value_type>);
	player.sortedMobjs.clear();

	AActor* playerViewPosition = player.camera;
	if (not playerViewPosition)
	{
		playerViewPosition = player.mo;
		if (not playerViewPosition)
		{
			// This operation only makes sense if the player has a position.
			return;
		}
	}

	auto& unsortedThinkers = DThinker::GetThinkerVectorRef();

	for (DThinker* thinker : unsortedThinkers)
	{
		if (thinker->IsKindOf(RUNTIME_CLASS(AActor)))
		{
			player.sortedMobjs.emplace_back(static_cast<AActor*>(thinker), 0);
		}
	}

	// In testing a 22000 mobj firefight (No Time To Freeze map32) on a Ryzen 9800x3d,
	// Windows 11, MSVC 2019, looking at JUST the core sort operation itself:
	//
	//      - std::sort:                    600-700 usec.
	//      - Boost spreadsort:             ~300 usec.
	//      - 3-partition std::nth_element:  40-70 usec.
	//
	// We go with dividing up the mobjs into 3 partitions with two calls to std::nth_element
	// because for the purposes of prioritizing mobj messages to clients, we don't need fine
	// precision between mobjs by distance.  Three coarse buckets based on approximate distance
	// is enough.  This gives us three categories of entities based on range:
	//
	//      1. The closest 25% of mobjs - we really want to see frequent updates to these.
	//      2. The next closest 25%     - no problem if these somewhat-distant guys stutter.
	//      3. Everything else          - we don't care if we don't see them.
	//
	//
	// The end result works well for the heavy-load test case, and only rarely do we see
	// nearby enemies behave like there's any packet loss.

	// The following block is used for sorting on approximate, relative distance.
	const int playerMostSignificantX = (playerViewPosition->x >> 16);
	const int playerMostSignificantY = (playerViewPosition->y >> 16);

	for (auto& mobjInfo : player.sortedMobjs)
	{
		// We go with the below block because it's just a bit faster in MSVC (~170 usec) than
		// P_AproxDistance2 (~200 usec) when looking at 22k mobjs, and we don't need "real"
		// distance - just comparable values that correlate with distance.

		const int dx = playerMostSignificantX - (mobjInfo.actorPtr->x >> 16);
		const int dy = playerMostSignificantY - (mobjInfo.actorPtr->y >> 16);
		mobjInfo.distance = dx*dx + dy*dy;
	}
	auto distanceCompare = [](const auto& mo1, const auto& mo2) { return mo1.distance < mo2.distance; };

	std::nth_element(player.sortedMobjs.begin(),
	                 player.sortedMobjs.begin() + player.sortedMobjs.size()/2,
	                 player.sortedMobjs.end(),
	                 distanceCompare);
	std::nth_element(player.sortedMobjs.begin(),
	                 player.sortedMobjs.begin() + player.sortedMobjs.size()/4,
	                 player.sortedMobjs.begin() + player.sortedMobjs.size()/2,
	                 distanceCompare);
}

void SV_WriteCommandsForPlayer(player_t& player)
{
	// [SL] 2011-05-11 - Send the client the server's gametic
	// this gametic is returned to the server with the client's
	// next cmd
	if (player.ingame())
		SV_SendGametic(&player.client);

	for (player_t& otherPlayer : players)
	{
		if (!(otherPlayer.ingame()) || !(otherPlayer.mo))
			continue;

		// a player is updated about their own position elsewhere
		if (&player == &otherPlayer)
			continue;

		// GhostlyDeath -- Screw spectators
		if (otherPlayer.spectator)
			continue;

		if(not SV_IsPlayerAllowedToSee(player, otherPlayer.mo))
			continue;

		MSG_WriteSVC(player.client.messenger.NetBuf(), SVC_MovePlayer(otherPlayer, player.tic));
	}

	// [SL] Send client info about player he is spying on
	player_t& target = idplayer(player.spying);
	if (validplayer(target) && &player != &target && P_CanSpy(player, target))
	{
		SV_SendPlayerStateUpdate(&(player.client), &target);
	}

	SV_UpdateConsolePlayer(player);

	const size_t previousSortedMobjCount = player.sortedMobjs.size();
	SV_SortMobjsForPlayer(player);

	// We ultimately temporarily allow up to an additional MAX while tic-to-tic new Mobjs exceed MAX.
	// Combined with the high-priority to_spawn queue being directly limited in SV_UpdateHiddenMobj,
	// we can be sure that both high-priority things like new missiles and deferred map-defined mobjs
	// get serviced under high-load situations.
	const int temporaryGrowthBonus = std::min(std::max(0,
	                                                   static_cast<int>(player.sortedMobjs.size() - previousSortedMobjCount)),
	                                          MAX_HIDDEN_MOBJ_UPDATES);
	const int maxForThisTic = MAX_HIDDEN_MOBJ_UPDATES + temporaryGrowthBonus;

	int hiddenUpdateCount = 0;

	// The following code is commented out pending the implementation of a real Mobj throttle.

//		int throttleCount = std::numeric_limits<int>::max();

//		if (SV_MustThrottleTransmissionsForClient(player.client))
//		{
//			const auto mobjCountFixed = INT2FIXED64  (player.sortedMobjs.size());
//			const auto fractionFixed  = FIXED2FIXED64(player.client.messenger.ThrottleFraction());
//			throttleCount = FIXED642INT(FixedMul64(mobjCountFixed, fractionFixed));
//		}

	for (auto& sortedMobj : player.sortedMobjs)
	{
//            if (throttleCount-- > 0)
//            {
//                break;
//            }
		SV_UpdateMissiles(player, sortedMobj.actorPtr);

		SV_UpdateMonsters(player, sortedMobj.actorPtr);

		if (hiddenUpdateCount <= maxForThisTic)
		{
			hiddenUpdateCount = SV_UpdateHiddenMobj(player, sortedMobj.actorPtr, hiddenUpdateCount);
		}
	}

	SV_UpdateGametype(player);     // update gametype stuff

	SV_SendPingRequest(& player.client);     // request ping reply

	SV_UpdatePing(& player.client);          // send the ping value of all cients to this client
}

//
// SV_WriteCommands
//
void SV_WriteCommands(void)
{
	// [SL] 2011-05-11 - Save player positions and moving sector heights so
	// they can be reconciled later for unlagging
	Unlag::getInstance().recordPlayerPositions();
	Unlag::getInstance().recordSectorPositions();

	// Palm off the job of writing the player messages onto the worker threads.
	std::vector<std::future<void> > futures;

	for (player_t& player : players)
	{
		std::packaged_task<void ()> task { [&player]()
			{
				SV_WriteCommandsForPlayer(player);
			} };
		futures.emplace_back(task.get_future());
		s_workers.MoveCommand(std::move(task));
	}

	for (auto& future : futures)
	{
		future.wait();
	}

	SV_UpdateDeadPlayers(); // Update dying players.
}


void SV_PlayerTriedToCheat(player_t &player)
{
	SV_BroadcastPrintFmt("{} tried to cheat!\n", player.userinfo.netname);
	SV_DropClient(player);
}

//
// SV_CalculateNumTiccmds
//
// [SL] 2011-09-16 - Calculate how many ticcmds should be processed.  Under
// most circumstances, it should be 1 per gametic to have the smoothest
// player movement possible.
//
int SV_CalculateNumTiccmds(player_t &player)
{
	if (!player.mo || player.cmdqueue.empty())
		return 0;

	static const size_t maximum_queue_size = TICRATE / 4;

	if (!sv_ticbuffer || player.spectator || player.playerstate == PST_DEAD)
	{
		// Process all queued ticcmds.
		return maximum_queue_size;
	}
	if (player.mo->momx == 0 && player.mo->momy == 0 && player.mo->momz == 0)
	{
		// Player is not moving
		return 2;
	}
	if (player.cmdqueue.size() > 2 && gametic % 2*TICRATE == player.id % 2*TICRATE)
	{
		// Process an extra ticcmd once every 2 seconds to reduce the
		// queue size. Use player id to stagger the timing to prevent everyone
		// from running an extra ticcmd at the same time.
		return 2;
	}
	if (player.cmdqueue.size() > maximum_queue_size)
	{
		// The player experienced a large latency spike so try to catch up by
		// processing more than one ticcmd at the expense of appearing perfectly
		//  smooth
		return 2;
	}

	// always run at least 1 ticcmd if possible
	return 1;
}

//
// SV_ProcessPlayerCmd
//
// Decides how many of a player's queued ticcmds should be processed and
// prepares the player.cmd structure for P_PlayerThink().  Also has the
// responsibility of ensuring the Unlag class has the appropriate latency
// for the player whose ticcmd we're processing.
//
void SV_ProcessPlayerCmd(player_t &player)
{
	const int max_forward_move = 50 << 8;
	#if 0
	const int max_sr40_side_move = 40 << 8;
	#endif
	const int max_sr50_side_move = 50 << 8;

	if (player.joindelay)
		player.joindelay--;
	if (player.suicidedelay)
		player.suicidedelay--;

	if (!validplayer(player) || !player.mo)
		return;

	#ifdef _TICCMD_QUEUE_DEBUG_
	DPrintFmt("Cmd queue size for {}: {}\n",
				player.userinfo.netname, player.cmdqueue.size());
	#endif	// _TICCMD_QUEUE_DEBUG_

	int num_cmds = SV_CalculateNumTiccmds(player);

	for (int i = 0; i < num_cmds && !player.cmdqueue.empty(); i++)
	{
		odaproto::clc::PlayerInput& netcmd = player.cmdqueue.front();
		player.cmd = ticcmd_t();
		player.tic = netcmd.tic();

		// Set the latency amount for Unlagging
		Unlag::getInstance().setRoundtripDelay(player.id, netcmd.world_index() & 0xFF);

		if ((netcmd.has_move_forward() && abs(netcmd.move_forward()) > max_forward_move) ||
		    (netcmd.has_move_side() && abs(netcmd.move_side()) > max_sr50_side_move))
		{
			SV_PlayerTriedToCheat(player);
			return;
		}

		#if 0
		if ((netcmd->hasSideMove() && abs(netcmd->getSideMove()) > max_sr40_side_move) &&
		    (player.mo && player.mo->prevangle != netcmd->getAngle()))
		{
			// verify SR50 isn't combined with yaw
			SV_PlayerTriedToCheat(player);
			return;
		}
		#endif

		CLC_UnpackPlayerInputMessageToPlayer(netcmd, player);

		if (!sv_freelook)
			player.mo->pitch = 0;

		// Apply this ticcmd using the game logic
		if (gamestate == GS_LEVEL)
		{
			P_PlayerThink(player);
			player.mo->RunThink();
		}

		player.cmdqueue.pop();		// remove this tic from the queue after being processed
	}
}

void SV_UpdateConsolePlayer(player_t &player)
{
	AActor *mo = player.mo;
	client_t *cl = &player.client;

	if (!mo)
		return;

	// GhostlyDeath -- Spectators are on their own really
	if (player.spectator)
	{
        SV_UpdateMovingSectors(player);
		return;
	}

	// client player will update his position if packets were missed
	MSG_WriteSVC(cl->messenger.NetBuf(), SVC_UpdateLocalPlayer(*mo, player.tic));
    SV_UpdateMovingSectors(player);
}

//
//	SV_ChangeTeam
//																							[Toke - CTF]
//	Allows players to change teams properly in teamplay and CTF
//
void SV_ChangeTeam (player_t &player)  // [Toke - Teams]
{
	team_t team = (team_t)MSG_ReadByte();

	if (team >= TEAM_NONE || team < 0)
		return;

	if (team >= sv_teamsinplay)
		return;

	team_t old_team = player.userinfo.team;
	player.userinfo.team = team;
	if (player.userinfo.team != old_team)
	{
		P_ClearPlayerPingState(player);
		SV_BroadcastPlayerPing(player);
	}

	if (G_IsTeamGame() && player.mo && player.userinfo.team != old_team &&
	    !G_IsLevelState(LevelState::WARMUP))
	{
		P_DamageMobj(player.mo, 0, 0, 1000, 0);

		M_LogWDLEvent(WDL_EVENT_DISCONNECT, &player, NULL, old_team,
		              M_GetPlayerId(player, old_team), 0, 0);
		M_LogWDLEvent(WDL_EVENT_JOINGAME, &player, NULL, team, M_GetPlayerId(player, team), 0,
		              0);
	}
	SV_BroadcastPrintFmt("{} has joined the {} team.\n", player.userinfo.netname,
	                   V_GetTeamColor(team));

	// Team changes can result with not enough players on a team.
	G_AssertValidPlayerCount();
}

//
// SV_Spectate
//
void SV_Spectate(player_t &player)
{
	// [AM] Code has three possible values; true, false and 5.  True specs the
	//      player, false unspecs him and 5 updates the server with the spec's
	//      new position.
	byte Code = MSG_ReadByte();

	if (!player.ingame())
		return;

	if (Code == 5)
	{
		// GhostlyDeath -- Prevent Cheaters
		if (!player.spectator || !player.mo)
		{
			for (int i = 0; i < 3; i++)
				MSG_ReadLong();
			return;
		}

		// GhostlyDeath -- Code 5! Anyway, this just updates the player for "antiwallhack" fun
		player.mo->x = MSG_ReadLong();
		player.mo->y = MSG_ReadLong();
		player.mo->z = MSG_ReadLong();
	}
	else
	{
		SV_SetPlayerSpec(player, Code);
	}
}

// Change a player into a spectator or vice-versa.  Pass 'true' for silent
// param to spec or unspec the player without a broadcasted message.
void P_SetSpectatorFlags(player_t &player);

void SV_SetPlayerSpec(player_t &player, bool setting, bool silent)
{
	if (player.ingame() == false)
		return;

	if (!setting && player.spectator)
	{
		// Join delay means they're mashing buttons too fast.
		if (player.joindelay > 0)
			return;

		if (G_CanJoinGame() != JOIN_OK)
		{
			// Not allowed to join yet - add them to the queue.
			if (player.QueuePosition == 0)
				SV_AddPlayerToQueue(&player);

			return;
		}
		SV_JoinPlayer(player, silent);
	}
	else if (setting && !player.spectator)
		SV_SpecPlayer(player, silent);
	else if (setting && player.spectator && player.QueuePosition > 0)
		SV_RemovePlayerFromQueue(&player);
}

/**
 * @brief Have a player join the game.  Note that this function does no
 *        checking against maxplayers or round limits or whatever, that's
 *        the job of the caller.
 *
 * @param player Player that should join the game.
 * @param silent True if the join should be done "silently".
*/
void SV_JoinPlayer(player_t& player, bool silent)
{
	// Figure out which team the player should be assigned to.
	if (G_IsTeamGame())
	{
		bool invalidteam = player.userinfo.team >= sv_teamsinplay;
		bool toomanyplayers =
		    sv_maxplayersperteam &&
		    P_NumPlayersOnTeam(player.userinfo.team) >= sv_maxplayersperteam;
		if (invalidteam || toomanyplayers)
		{
			// If this check fails, our "CanJoin" function didn't do a good-enough
			// job of scoping out a potential team.
			team_t newteam = SV_GoodTeam();
			if (newteam == TEAM_NONE)
				return;

			SV_ForceSetTeam(player, newteam);
			SV_CheckTeam(player);
		}
	}

	// [SL] 2011-09-01 - Clear any previous SV_MidPrint (sv_motd for example)
	SV_MidPrint("", &player, 0);

	// Warn everyone we're not a spectator anymore.
	player.spectator = false;

	// Whatever mobj we had it doesn't matter anymore.
	if (player.mo)
		P_KillMobj(NULL, player.mo, NULL, true);

	// Fresh joins get fresh player scores.
	P_ClearPlayerScores(player, SCORES_CLEAR_ALL);

	// Ensure our player is in the ENTER state.
	player.playerstate = PST_ENTER;

	// Set player unready if we're in warmup mode.
	if (sv_warmup)
	{
		SV_SetReady(player, false, true);
		player.timeout_ready = 0;
	}

	// Finally, persist info about our freshly-joining player to the world.
	for (Players::iterator it = ::players.begin(); it != ::players.end(); ++it)
	{
		if (!it->ingame())
			continue;

		MSG_WriteSVC(it->client.messenger.ReliableBuf(), SVC_PlayerMembers(player, SVC_MSG_ALL));
	}

	// Everything is set, now warn everyone the player joined.
	if (!silent)
	{
		if (sv_gametype != GM_TEAMDM && sv_gametype != GM_CTF)
			SV_BroadcastPrintFmt("{} joined the game.\n",
			                     player.userinfo.netname);
		else
			SV_BroadcastPrintFmt("{} joined the game on the {} team.\n",
			                     player.userinfo.netname,
			                     V_GetTeamColor(player.userinfo.team));
	}

	M_LogWDLEvent(WDL_EVENT_JOINGAME, &player, NULL, player.userinfo.team,
	              M_GetPlayerId(player, player.userinfo.team), 0, 0);
}

void SV_SpecPlayer(player_t &player, bool silent)
{
	// call CTF_CheckFlags _before_ the player becomes a spectator.
	// Otherwise a flag carrier will drop his flag at (0,0), which
	// is often right next to one of the bases...
	if (sv_gametype == GM_CTF)
		CTF_CheckFlags(player);

	// [tm512 2014/04/18] Avoid setting spectator flags on a dead player
	// Instead we respawn the player, move him back, and immediately spectate him afterwards
	if (player.playerstate == PST_DEAD)
		G_DoReborn(player);

	player.spectator = true;
	for (Players::iterator it = ::players.begin(); it != ::players.end(); ++it)
	{
		MSG_WriteSVC(it->client.messenger.ReliableBuf(),
		             SVC_PlayerMembers(player, SVC_PM_SPECTATOR));
	}

	// [AM] Set player unready if we're in warmup mode.
	if (sv_warmup)
	{
		SV_SetReady(player, false, true);
		player.timeout_ready = 0;
	}

	player.playerstate = PST_LIVE;
	player.joindelay = ReJoinDelay;

	P_SetSpectatorFlags(player);

	if (!silent)
	{
		std::string status = SV_BuildKillsDeathsStatusString(player);
		SV_BroadcastPrintFmt(PRINT_HIGH, "{} became a spectator. ({})\n",
			player.userinfo.netname, status);
	}

	P_PlayerLeavesGame(&player);
	SV_UpdatePlayerQueuePositions(G_CanJoinGame, &player);
}

bool CMD_ForcespecCheck(const std::vector<std::string> &arguments,
						std::string &error, size_t &pid) {
	if (arguments.empty()) {
		error = "need a player id (try 'players').";
		return false;
	}

	std::istringstream buffer(arguments[0]);
	buffer >> pid;

	if (!buffer) {
		error = "player id needs to be a numeric value.";
		return false;
	}

	// Verify the player actually exists.
	player_t &player = idplayer(pid);
	if (!validplayer(player)) {
		std::ostringstream error_ss;
		error_ss << "could not find client " << pid << ".";
		error = error_ss.str();
		return false;
	}

	// Verify that the player is actually in a spectatable state.
	if (!player.ingame()) {
		std::ostringstream error_ss;
		error_ss << "cannot spectate client " << pid << ".";
		error = error_ss.str();
		return false;
	}

	// Verify that the player isn't already spectating.
	if (player.spectator) {
		std::ostringstream error_ss;
		error_ss << "client " << pid << " is already a spectator.";
		error = error_ss.str();
		return false;
	}

	return true;
}

BEGIN_COMMAND (forcespec) {
	std::vector<std::string> arguments = VectorArgs(argc, argv);
	std::string error;

	size_t pid;

	if (!CMD_ForcespecCheck(arguments, error, pid)) {
		PrintFmt("forcespec: {}\n", error);
		return;
	}

	// Actually spec the given player
	player_t &player = idplayer(pid);
	SV_SetPlayerSpec(player, true);
} END_COMMAND (forcespec)

// Change the player's ready state and broadcast it to all connected players.
void SV_SetReady(player_t &player, bool setting, bool silent)
{
	if (!validplayer(player) || !player.ingame())
		return;

	// Change the player's ready state only if the new state is different from
	// the current state.
	bool changed = true;
	if (player.ready && !setting) {
		player.ready = false;
		if (!silent) {
			if (player.spectator)
				SV_PlayerPrintFmt(PRINT_HIGH, player.id, "You are no longer willing to play.\n");
			else
				SV_PlayerPrintFmt(PRINT_HIGH, player.id, "You are no longer ready to play.\n");
		}
	} else if (!player.ready && setting) {
		player.ready = true;
		if (!silent) {
			if (player.spectator)
				SV_PlayerPrintFmt(PRINT_HIGH, player.id, "You are now willing to play.\n");
			else
				SV_PlayerPrintFmt(PRINT_HIGH, player.id, "You are now ready to play.\n");
		}
	} else {
		changed = false;
	}

	if (changed) {
		// Broadcast the new ready state to all connected players.
		for (Players::iterator it = players.begin();it != players.end();++it)
		{
			MSG_WriteSVC(it->client.messenger.ReliableBuf(),
			             SVC_PlayerMembers(player, SVC_PM_READY));
		}
	}

	::levelstate.readyToggle();
}

/**
 * @brief Tell the client about any custom commands we have.
 *
 * @detail A stock server is not expected to have any custom commands.
 *         Custom servers can implement their own features, and this is
 *         where you tell players about it.
 *
 * @param player Player who asked for help.
 */
static void HelpCmd(player_t& player)
{
	SV_PlayerPrintFmt(PRINT_HIGH, player.id,
	                  "odasrv v{}\n\n"
	                  "This server has no custom commands\n",
	                  GitShortHash());
}

/**
 * @brief Toggle a player as ready/unready.
 *
 * @param player Player to toggle.
 */
static void ReadyCmd(player_t &player)
{
	// If the player is not ingame, he shouldn't be sending us ready packets.
	if (!player.ingame()) {
		return;
	}

	if (player.timeout_ready > level.time) {
		// We must be on a new map.  Reset the timeout.
		player.timeout_ready = 0;
	}

	// Check to see if warmup will allow us to toggle our ready state.
	if (!::G_CanReadyToggle())
	{
		SV_PlayerPrintFmt(PRINT_HIGH, player.id, "You can't ready in the middle of a match!\n");
		return;
	}

	// Check to see if the player's timeout has expired.
	if (player.timeout_ready > 0) {
		int timeout = level.time - player.timeout_ready;

		// Players can only toggle their ready state every 3 seconds.
		int timeout_check = 3 * TICRATE;
		int timeout_waitsec = 3 - (timeout / TICRATE);

		if (timeout < timeout_check) {
			SV_PlayerPrintFmt(PRINT_HIGH, player.id, "Please wait another {} second{} to change your ready state.\n",
			                  timeout_waitsec, timeout_waitsec != 1 ? "s" : "");
			return;
		}
	}

	// Set the timeout.
	player.timeout_ready = level.time;

	// Toggle ready state
	SV_SetReady(player, !player.ready);
}

/**
 * @brief Send the player a MOTD on demand.
 *
 * @param player Player who wants the MOTD.
 */
void MOTDCmd(player_t& player)
{
	SV_MidPrint((char*)sv_motd.cstring(), &player, 6);
}

/**
 * @brief Interpret a "netcmd" string from a client.
 *
 * @param player Player who sent the netcmd.
 */
void SV_NetCmd(player_t& player)
{
	std::vector<std::string> netargs;

	// Parse arguments into a vector.
	netargs.push_back(MSG_ReadString());
	size_t netargc = MSG_ReadByte();

	for (size_t i = 0; i < netargc; i++)
	{
		netargs.push_back(MSG_ReadString());
	}
	
	uint32_t arg0 = CONST_HASH(netargs.at(0));
	switch (arg0)
	{
	case CONST_HASH("help"):
		HelpCmd(player);
		break;
	case CONST_HASH("motd"):
		MOTDCmd(player);
		break;
	case CONST_HASH("ready"):
		ReadyCmd(player);
		break;
	case CONST_HASH("vote"):
		SV_VoteCmd(player, netargs);
		break;
	case CONST_HASH("player_ping"):
		if (player.spectator)
			break;
		{
			ping_filter_t filter{};
			bool dropAtSelf = false;
			if (netargs.size() >= 4)
			{
				filter.pickups = netargs[1] != "0";
				filter.monsters = netargs[2] != "0";
				filter.flags = netargs[3] != "0";
				if (netargs.size() >= 5)
					filter.mouselook = netargs[4] != "0";
				if (netargs.size() >= 6)
					dropAtSelf = netargs[5] != "0";
			}

			switch (P_PlayerPing(player, filter, dropAtSelf))
			{
			case PING_SUBMIT_RATE_LIMITED:
				SV_PlayerPrintFmt(PRINT_HIGH, player.id, "Ping cooling down. Please wait.\n");
				break;
			case PING_SUBMIT_PLACED:
			case PING_SUBMIT_PLACED_RETAP_WARNING:
				SV_BroadcastPlayerPing(player);
				break;
			default:
				break;
			}
		}
		break;
	default: break;
	}
}

//
// SV_RConLogout
//
//   Removes rcon privileges
//
void SV_RConLogout (player_t &player)
{
	client_t *cl = &player.client;

	// read and ignore the password field since rcon_logout doesn't use a password
	MSG_ReadString();

	if (cl->allow_rcon)
	{
		PrintFmt("RCON logout from {} - {}", player.userinfo.netname, NET_AdrToString(cl->address));
		cl->allow_rcon = false;
	}
}


//
// SV_RConPassword
// denis
//
void SV_RConPassword (player_t &player)
{
	client_t *cl = &player.client;

	std::string challenge = MSG_ReadString();
	std::string password = rcon_password.cstring();

	// Don't display login messages again if the client is already logged in
	if (cl->allow_rcon)
		return;

	if (!password.empty() && MD5SUM(password + cl->digest) == challenge)
	{
		cl->allow_rcon = true;
		PrintFmt(PRINT_HIGH, "RCON login from {} - {}", player.userinfo.netname, NET_AdrToString(cl->address));
	}
	else
	{
		PrintFmt(PRINT_HIGH, "RCON login failure from {} - {}", player.userinfo.netname, NET_AdrToString(cl->address));
		MSG_WriteSVC(cl->messenger.ReliableBuf(), SVC_Print(PRINT_HIGH, "Bad password\n"));
	}
}

//
// SV_Suicide
//
void SV_Suicide(player_t &player)
{
	if (!player.mo)
		return;

	// WHY do you want to commit suicide in the intermission screen ?!?!
	if (gamestate != GS_LEVEL)
		return;

	// merry suicide!
	P_DamageMobj (player.mo, NULL, NULL, 10000, MOD_SUICIDE);
	//player.mo->player = NULL;
	//player.mo = NULL;
}

//
// SV_Cheat
//
void SV_Cheat(player_t &player)
{
	byte cheatType = MSG_ReadByte();

	if (cheatType == 0)
	{
		unsigned int cheat = MSG_ReadShort();

		if (!cheat::AreCheatsEnabled())
			return;

		int oldCheats = player.cheats;
		cheat::DoCheat(player, cheat);

		if (player.cheats != oldCheats)
		{
			for (Players::iterator it = players.begin(); it != players.end(); ++it)
			{
				client_t* cl = &it->client;
				SV_SendPlayerStateUpdate(cl, &player);
			}
		}

	}
	else if (cheatType == 1)
	{
		const char* wantcmd = MSG_ReadString();

		if (!cheat::AreCheatsEnabled())
			return;

		cheat::GiveTo(player, wantcmd);

		for (Players::iterator it = players.begin(); it != players.end(); ++it)
		{
			client_t* cl = &it->client;
			SV_SendPlayerStateUpdate(cl, &player);
		}

	}
	else if (cheatType == 2)
	{
		const char* wantsummon = MSG_ReadString();

		if (!cheat::AreCheatsEnabled())
			return;

		AActor* actor = cheat::Summon(player, wantsummon, false);

		if (actor == NULL)
			return;

		for (Players::iterator it = players.begin(); it != players.end(); ++it)
		{
			client_t* cl = &it->client;
			SV_SendMobjToClient(actor, cl);
		}
	}
	else if (cheatType == 3)
	{
		const char* wantsummon = MSG_ReadString();

		if (!cheat::AreCheatsEnabled())
			return;

		AActor* actor = cheat::Summon(player, wantsummon, true);

		if (actor == NULL)
			return;

		for (Players::iterator it = players.begin(); it != players.end(); ++it)
		{
			client_t* cl = &it->client;
			SV_SendMobjToClient(actor, cl);
		}
	}
}

void SV_WantWad(player_t &player)
{
	client_t *cl = &player.client;

	// read and ignore the rest of the wad request
	MSG_ReadString();
	MSG_ReadString();
	MSG_ReadLong();

	MSG_WriteSVC(cl->messenger.ReliableBuf(),
		            SVC_Print(PRINT_HIGH, "Server: Downloading is disabled\n"));

	SV_DropClient(player);
	return;
}

void SV_HandlePlayerInput(odaproto::clc::PlayerInput& msg, player_t &player)
{
	if (gamestate == GS_LEVEL)
	{
		if (!player.spectator)
		{
			player.cmdqueue.push(std::move(msg));
		}
	}
}

//
// SV_ParseCommands
//

parseError_e SV_ParseCommandSVC(const byte cmd, player_t& player)
{
    google::protobuf::Message* msgPtrRaw = nullptr;
    const parseError_e result = SVC_ParseMessage(msgPtrRaw, cmd);

    std::unique_ptr<google::protobuf::Message> msgPtr(msgPtrRaw);

    if (result == PERR_OK)
    {
        switch (cmd)
        {
            case clc_playerinput:
                SV_HandlePlayerInput(*static_cast<odaproto::clc::PlayerInput*>(msgPtrRaw), player);
                break;
            default:
                // This case happens when a message was received, parsed, but not handled.
                PrintFmt(PRINT_WARNING, "SV_ParseCommandSVC: Did not handle decoded message {}\n", cmd);
                return PERR_BAD_DECODE;
        }
    }
    return result;
}

void SV_ParseCommands(player_t &player)
{
	while(validplayer(player))
	{
		if (not player.client.messenger.NextReceivedPacket(::net_message))
		{
			break;
		}
		while (::net_message.BytesLeftToRead() > 0)
		{
            const byte cmdRaw = MSG_ReadByte();

			clc_t cmd = static_cast<clc_t>(cmdRaw);

			if(cmd == (clc_t)-1)
				continue;

			switch(cmd)
			{
			case clc_disconnect:
				SV_DisconnectClient(player);
				return;

			case clc_userinfo:
				if (!SV_SetupUserInfo(player))
					return;
				SV_BroadcastUserInfo(player);
				break;

			case clc_getplayerinfo:
				SV_SendPlayerInfo (player);
				break;

			case clc_say:
				if (!SV_Say(player))
					return;
				break;

			case clc_privmsg:
				if (!SV_PrivMsg(player))
					return;
				break;

			case clc_pingreply:  // [SL] 2011-05-11 - Changed to clc_pingreply
				SV_CalcPing(player);
				break;

			case clc_rate:
				MSG_ReadLong();		// [SL] Read and ignore. Clients now always use sv_maxrate.
				break;

			case msg_ack:
				SV_AcknowledgePacket(player);
				break;

			case clc_rcon:
				{
					std::string str(MSG_ReadString());
					StripColorCodes(str);

					if (player.client.allow_rcon)
					{
						PrintFmt(PRINT_HIGH, "RCON command from {} - {} -> {}",
								player.userinfo.netname, NET_AdrToString(net_from), str);
						AddCommandString(str);
					}
				}
				break;

			case clc_rcon_password:
				{
					bool login = MSG_ReadByte();

					if (login)
						SV_RConPassword(player);
					else
						SV_RConLogout(player);

					break;
				}

			case clc_changeteam:
				SV_ChangeTeam(player);
				break;

			case clc_spectate:
            {
                SV_Spectate (player);
            }
				break;

			case clc_netcmd:
				SV_NetCmd(player);
				break;

			case clc_kill:
				if(player.mo && player.suicidedelay == 0 && gamestate == GS_LEVEL &&
               (sv_allowcheats || G_IsCoopGame()))
            {
					SV_Suicide (player);
            }
				break;

			case clc_wantwad:
				SV_WantWad(player);
				break;

			case clc_cheat:
				SV_Cheat(player);
				break;

			case clc_abort:
				PrintFmt("Client abort.\n");
				SV_DropClient(player);
				return;

			case clc_spy:
				SV_SpyPlayer(player);
				break;

			// [AM] Vote
			case clc_callvote:
				SV_Callvote(player);
				break;

			// [AM] Maplist
			case clc_maplist:
				SV_Maplist(player);
				break;
			case clc_maplist_update:
				SV_MaplistUpdate(player);
				break;

			default:
                // It's important to allow the clc_ enum to have priority
                // over svc_ if we have both types of messages.
                switch (SV_ParseCommandSVC(cmdRaw, player))
                {
                    case PERR_OK:
                        continue;
                    case PERR_UNKNOWN_HEADER:   // Data still readable from buffer
                        PrintFmt(PRINT_WARNING, "SV_ParseCommands: Unmappable command {}\n", cmdRaw);
                        break;
                    case PERR_UNKNOWN_MESSAGE:  // Data still readable from buffer
                        PrintFmt(PRINT_WARNING, "SV_ParseCommands: Command {} unknown to protobuf\n", cmdRaw);
                        break;
                    case PERR_BAD_DECODE:       // Data no longer in buffer.  Welp.
                        PrintFmt(PRINT_WARNING, "SV_ParseCommands: Bad protobuf decode for {}\n", cmdRaw);
                        break;
                    default:
                        break;
                }
				PrintFmt("SV_ParseCommands: Unknown client message {}.\n", cmd);
				SV_DropClient(player);
				return;
			}

			if (net_message.overflowed)
			{
				PrintFmt("SV_ReadClientMessage: badread {}({})\n",
						    cmd,
						    clc_info[cmd].getName());
				SV_DropClient(player);
				return;
			}
		}
	}
}


static void TimeCheck()
{
	G_TimeCheckEndGame();

	// [SL] 2011-10-25 - Send the clients the remaining time (measured in seconds)
	if (P_AtInterval(1 * TICRATE)) // every second
	{
		for (auto& player : players)
			MSG_WriteSVC(player.client.messenger.NetBuf(), SVC_LevelLocals(level, SVC_LL_TIME));
	}
}

static void IntermissionTimeCheck()
{
	level.inttimeleft = mapchange / TICRATE;

	// [SL] 2011-10-25 - Send the clients the remaining time (measured in seconds)
	// [ML] 2012-2-1 - Copy it for intermission fun
	if (P_AtInterval(1 * TICRATE)) // every second
	{
		for (auto& player : players)
		{
			MSG_WriteSVC((player.client.messenger.NetBuf()), SVC_IntTimeLeft(level.inttimeleft));
		}
	}
}

//
// SV_GameTics
//
//	Runs once gametic (35 per second gametic)
//
void SV_GameTics (void)
{
	if (sv_gametype == GM_CTF)
		CTF_RunTics();

	switch (gamestate)
	{
		case GS_LEVEL:
			SV_RemoveCorpses();
			::levelstate.tic();
			TimeCheck();
			Vote_Runtic();
		break;
		case GS_INTERMISSION:
			IntermissionTimeCheck();
		break;

		default:
		break;
	}

	for (auto& player : players)
		SV_ProcessPlayerCmd(player);
}

void SV_TouchSpecial(const AActor& special, player_t& player)
{
	client_t *cl = &player.client;

	if (cl == nullptr)
		return;

	MSG_WriteSVC(cl->messenger.ReliableBuf(), SVC_TouchSpecial(special));
}

void SV_PlayerTimes (void)
{
	for (auto& player : players)
	{
		if (player.ingame())
			(player.GameTime) += 1;
	}
}


//
// SV_Frozen
//
// Returns true if the game state should be frozen (not advance).
//
bool SV_Frozen()
{
	return sv_emptyfreeze && players.empty() && gamestate == GS_LEVEL;
}

auto writeCommandsStopwatch = TimingInstr::Get().CreateStopwatch("SV_WriteCommands");
auto sendPacketsStopwatch   = TimingInstr::Get().CreateStopwatch("SV_SendPackets");
auto gTickerStopwatch       = TimingInstr::Get().CreateStopwatch("G_Ticker");
auto gameTicsStopwatch      = TimingInstr::Get().CreateStopwatch("SV_GameTics");

//
// SV_StepTics
//
void SV_StepTics(QWORD count)
{
	DObject::BeginFrame();

	// run the newtime tics
	while (count--)
	{
		gameTicsStopwatch->Start();
		SV_GameTics();
		gameTicsStopwatch->Stop();

		gTickerStopwatch->Start();
		G_Ticker();
		gTickerStopwatch->Stop();

		writeCommandsStopwatch->Start();
		SV_WriteCommands();
		writeCommandsStopwatch->Stop();

		sendPacketsStopwatch->Start();
		SV_SendPackets();
		sendPacketsStopwatch->Stop();

		SV_CheckTimeouts();
		SV_DestroyFinishedMovingSectors();

		// increment player_t::GameTime for all players once a second
		static int TicCount = 0;
		// Only do this once a second.
		if (TicCount++ >= 35)
		{
			SV_PlayerTimes();
			TicCount = 0;
		}

		gametic++;
	}

	DObject::EndFrame();
}

//
// SV_DisplayTics
//
// Nothing to display...
//
void SV_DisplayTics()
{
}

auto frameStopwatch      = TimingInstr::Get().CreateStopwatch("FrameTime");
auto getPacketsStopwatch = TimingInstr::Get().CreateStopwatch("SV_GetPackets");
auto retransmitStopwatch = TimingInstr::Get().CreateStopwatch("SV_HandleReliableRetransmissions");

//
// SV_RunTics
//
// Checks for incoming packets, processes console usage, and calls SV_StepTics.
//
void SV_RunTics()
{
	frameStopwatch->Start();

	getPacketsStopwatch->Start();
	SV_GetPackets();
	getPacketsStopwatch->Stop();

	SV_CheckCanaries();
	SV_CheckDepartingMessengers(gametic);

	retransmitStopwatch->Start();
	SV_HandleReliableRetransmissions();
	retransmitStopwatch->Stop();

	std::string cmd = I_ConsoleInput();
	if (cmd.length())
		AddCommandString(cmd);

	SV_BanlistTics();
	SV_UpdateMaster();

	// only run game-related tickers if the server isn't frozen
	// (sv_emptyfreeze enabled and no clients)
	if (!step_mode && !SV_Frozen())
		SV_StepTics(1);

	// Remove any recently disconnected clients
	for (Players::iterator it = players.begin(); it != players.end();)
	{
		if (it->playerstate == PST_DISCONNECT)
			it = SV_RemoveDisconnectedPlayer(it);
		else
			++it;
	}

	// [SL] 2011-05-18 - Handle sv_emptyreset
	static size_t last_player_count = players.size();
	if (gamestate == GS_LEVEL && sv_emptyreset && players.empty() &&
			last_player_count > 0)
	{
		// The last player just disconnected so reset the level.
		// [SL] Ordinarily we should call G_DeferedInitNew but this is called
		// at the end of a gametic and the level reset should take place now
		// rather than at the start of the next gametic.
		maplist_entry_t lobby_entry;
		lobby_entry = Maplist::instance().get_lobbymap();

		if (!Maplist::instance().lobbyempty())
		{
			std::string wadstr = C_EscapeWadList(lobby_entry.wads);
			G_LoadWadString(wadstr, lobby_entry.map);
		}
		else
		{
			// [AM] Make a copy of mapname for safety's sake.
			OLumpName mapname = ::level.mapname;
			G_InitNew(mapname);
		}
	}
	last_player_count = players.size();

	frameStopwatch->Stop();

	TimingInstr::Get().ManageRecording(gametic);
}


BEGIN_COMMAND(step)
{
        QWORD newtics = argc > 1 ? atoi(argv[1]) : 1;

	extern unsigned char prndindex;

	SV_StepTics(newtics);

	// debugging output
	if (players.size() && players.begin() != players.end())
		PrintFmt("level.time {}, prndindex {}, {} {} {}\n", level.time, prndindex, players.begin()->mo->x, players.begin()->mo->y, players.begin()->mo->z);
	else
		PrintFmt("level.time {}, prndindex {}\n", level.time, prndindex);
}
END_COMMAND(step)


// For Debugging
BEGIN_COMMAND (playerinfo)
{
	player_t *player = &consoleplayer();

	if (argc > 1)
	{
		player_t &p = idplayer(atoi(argv[1]));

		if (!validplayer(p))
		{
			PrintFmt(PRINT_HIGH, "Bad player number.\n");
			return;
		}
		else
			player = &p;
	}
	else
	{
		PrintFmt("Usage : playerinfo <#playerid>\n");
		PrintFmt("Gives additional infos about the selected player (use \"playerlist\" to display player IDs).\n");
		return;
	}

	if (!validplayer(*player))
	{
		PrintFmt("Not a valid player\n");
		return;
	}

	const std::string ip = fmt::format("{:d}.{:d}.{:d}.{:d}",
			player->client.address.ip[0], player->client.address.ip[1],
			player->client.address.ip[2], player->client.address.ip[3]);

	const std::string color = fmt::format("#{:02X}{:02X}{:02X}",
			player->userinfo.color[1], player->userinfo.color[2], player->userinfo.color[3]);

	const std::string& team = GetTeamInfo(player->userinfo.team)->ColorStringUpper;

	PrintFmt("---------------[player info]----------- \n");
	PrintFmt(" IP Address           - {:s} \n",		ip);
	PrintFmt(" userinfo.netname     - {:s} \n",		player->userinfo.netname);
	if (sv_gametype == GM_CTF || sv_gametype == GM_TEAMDM) {
		PrintFmt(" userinfo.team        - {:s} \n", team);
	}
	PrintFmt(" userinfo.aimdist     - {:d} \n",		player->userinfo.aimdist >> FRACBITS);
	PrintFmt(" userinfo.colorpreset - {:d} \n",		player->userinfo.colorpreset);
	PrintFmt(" userinfo.color       - {:s} \n",		color);
	PrintFmt(" userinfo.gender      - {:d} \n",		player->userinfo.gender);
	PrintFmt(" time                 - {:d} \n",		player->GameTime);
	PrintFmt(" spectator            - {:d} \n",		player->spectator);
	if (G_IsCoopGame())
	{
		PrintFmt(" kills - {:d}  deaths - {:d}\n", player->killcount, player->deathcount);
	}
	else
	{
		PrintFmt(" frags - {:d}  deaths - {:d}  points - %d\n", player->fragcount,
		       player->deathcount, player->points);
	}
	if (g_lives)
	{
		PrintFmt(" lives - {:d}  wins - {:d}\n", player->lives, player->roundwins);
	}
	PrintFmt("--------------------------------------- \n");
}
END_COMMAND (playerinfo)

BEGIN_COMMAND(playerlist)
{
	bool anybody = false;
	int frags = 0;
	int deaths = 0;
	int points = 0;

	for (Players::reverse_iterator it = players.rbegin(); it != players.rend(); ++it)
	{

		if (G_IsRoundsGame() && !G_IsDuelGame())
		{
			frags = it->totalpoints;
			deaths = it->totaldeaths;
			points = it->totalpoints;
		}
		else
		{
			frags = it->fragcount;
			deaths = it->deathcount;
			points = it->points;
		}

		std::string strMain, strScore;
		strMain = fmt::sprintf("(%02d): %s %s - %s - time:%d - ping:%d", it->id,
		                       it->userinfo.netname, it->spectator ? "(SPEC)" : "",
		                       NET_AdrToString(it->client.address), it->GameTime, it->ping);

		if (G_IsCoopGame())
		{
			if (G_IsLivesGame())
			{
				// Kills and Lives
				strScore = fmt::sprintf(" - kills:%d - lives:%d", it->killcount, it->lives);
			}
			else
			{
				// Kills and Deaths
				strScore = fmt::sprintf(" - kills:%d - deaths:%d", it->killcount, it->deathcount);
			}
		}
		else if (sv_gametype == GM_DM)
		{
			if (G_IsLivesGame())
			{
				// Wins, Lives, and Frags
				strScore = fmt::sprintf(" - wins:%d - lives:%d - frags:%d", it->roundwins,
				          it->lives, frags);
			}
			else
			{
				// Frags, Deaths
				strScore = fmt::sprintf(" - frags:%d - deaths:%d", frags, deaths);
			}
		}
		else if (sv_gametype == GM_TEAMDM)
		{
			if (G_IsLivesGame())
			{
				// Frags and Lives
				strScore = fmt::sprintf(" - frags:%d - lives:%d", frags, it->lives);
			}
			else
			{
				// Frags
				strScore = fmt::sprintf(" - frags:%d", frags);
			}
		}
		else if (sv_gametype == GM_CTF)
		{
			if (G_IsLivesGame())
			{
				// Points and Lives
				strScore = fmt::sprintf(" - points:%d - lives:%d", points, it->lives);
			}
			else
			{
				// Points and Frags
				// Special case here: frags will only be from the current round, not global.
				strScore = fmt::sprintf(" - points:%d - frags:%d", points, it->fragcount);
			}
		}

		PrintFmt("{}{}\n", strMain, strScore);
		anybody = true;
	}

	if (!anybody)
	{
		PrintFmt("There are no players on the server.\n");
		return;
	}
}
END_COMMAND(playerlist)

BEGIN_COMMAND (players)
{
	AddCommandString("playerlist");
}
END_COMMAND (players)

void OnChangedSwitchTexture (line_t *line, int useAgain)
{
	unsigned state = 0, time = 0;
	P_GetButtonInfo(line, state, time);

	for (auto& player : players)
	{
		client_t *cl = &(player.client);

		MSG_WriteSVC(cl->messenger.ReliableBuf(), SVC_Switch(*line, state, time));
	}
}

void SV_OnActivatedLine(line_t* line, AActor* mo, const int side,
                        const LineActivationType activationType, const bool bossaction)
{
	if (P_LineSpecialMovesSector(line->special))
		return;

	for (auto& player : players)
	{
		if (!(player.ingame()))
			continue;

		client_t *cl = &(player.client);

		MSG_WriteSVC(cl->messenger.ReliableBuf(), SVC_ActivateLine(line, mo, side, activationType));
	}
}

void SV_SendDamagePlayer(player_t *player, const AActor* inflictor, int healthDamage, int armorDamage)
{
	for (Players::iterator it = players.begin();it != players.end();++it)
	{
		client_t *cl = &(it->client);

		MSG_WriteSVC(cl->messenger.ReliableBuf(),
		             SVC_DamagePlayer(*player, inflictor, healthDamage, armorDamage));
	}
}

void SV_SendDamageMobj(const AActor *target, int pain)
{
	if (!target)
		return;

	for (auto& player : players)
	{
		client_t *cl = &(player.client);

		MSG_WriteSVC(cl->messenger.ReliableBuf(), SVC_DamageMobj(target, pain));
		if (!target->player)
			MSG_WriteSVC(cl->messenger.NetBuf(), SVC_UpdateMobj(*target));
	}
}

void SV_SendKillMobj(const AActor *source, const AActor *target, const AActor *inflictor,
				     bool joinkill)
{
	if (!target)
		return;

	for (auto& player : players)
	{
		client_t *cl = &(player.client);

		if (!SV_IsPlayerAllowedToSee(player, target))
			continue;

		MSG_WriteSVC(cl->messenger.ReliableBuf(),
		             SVC_KillMobj(source, target, inflictor, ::MeansOfDeath, joinkill));
	}
}

void SV_SendRaiseMobj(const AActor* source, const AActor* corpse)
{
	if (!corpse)
		return;

	for (auto& player : players)
	{
		client_t* cl = &(player.client);

		if (!SV_IsPlayerAllowedToSee(player, corpse))
			continue;

		MSG_WriteSVC(cl->messenger.ReliableBuf(), SVC_RaiseMobj(source, corpse));
	}
}

// Tells clients to remove an actor from the world as it doesn't exist anymore
void SV_SendDestroyActor(const AActor *mo)
{
	if (mo->netid && mo->type != MT_PUFF)
	{
		for (auto& player : players)
		{
			if (mo->players_aware.get(player.id))
			{
				client_t *cl = &(player.client);

				// denis - todo - need a queue for destroyed (lost awareness)
				// objects, as a flood of destroyed things could easily overflow a
				// buffer
				MSG_WriteSVC(cl->messenger.ReliableBuf(), SVC_RemoveMobj(*mo));
			}
		}
	}
}

// Missile exploded so tell clients about it
void SV_ExplodeMissile(const AActor *mo)
{
	for (auto& player : players)
	{
		client_t *cl = &(player.client);

		if (!SV_IsPlayerAllowedToSee(player, mo))
			continue;

		MSG_WriteSVC(cl->messenger.ReliableBuf(), SVC_UpdateMobj(*mo));
		MSG_WriteSVC(cl->messenger.ReliableBuf(), SVC_ExplodeMissile(*mo));
	}
}

//
// SV_SendPlayerInfo
//
// Sends a player their current inventory
//
void SV_SendPlayerInfo(player_t &player)
{
	client_t *cl = &player.client;
	MSG_WriteSVC(cl->messenger.ReliableBuf(), SVC_PlayerInfo(player));
}

void SV_SendPlayerPing(const player_t& source, client_t* dest)
{
	if (!dest || !source.player_ping || P_IsPingExpired(*source.player_ping))
		return;

	MSG_WriteSVC(dest->messenger.ReliableBuf(), SVC_PlayerPing(source));
}

void SV_BroadcastPlayerPing(const player_t& source)
{
	for (auto& player : players)
	{
		MSG_WriteSVC(player.client.messenger.ReliableBuf(), SVC_PlayerPing(source));
	}
}

//
// SV_PreservePlayer
//
void SV_PreservePlayer(player_t &player)
{
	if (!serverside || sv_gametype != GM_COOP || !validplayer(player) || !player.ingame())
		return;

	if (!::unnatural_level_progression && !::g_resetinvonexit)
	{
		// denis - carry weapons and keys over to next level
		player.playerstate = PST_LIVE;
	}

	G_DoReborn(player);
}

void SV_AddPlayerToQueue(player_t* player)
{
	player->QueuePosition = 255;
	SV_UpdatePlayerQueuePositions(G_CanJoinGame, NULL);
}

void SV_RemovePlayerFromQueue(player_t* player)
{
	player->joindelay = ReJoinDelay;
	SV_UpdatePlayerQueuePositions(G_CanJoinGame, player);
}

void SV_UpdatePlayerQueueLevelChange(const WinInfo& win)
{
	if (::g_winnerstays)
	{
		std::vector<player_t*> loserPlayers;

		PlayerResults pr = PlayerQuery().execute();
		for (PlayersView::iterator it = pr.players.begin(); it != pr.players.end(); ++it)
		{
			switch (win.type)
			{
			case WinInfo::WIN_PLAYER:
				// Boot everybody but the winner.
				if ((*it)->id != win.id)
					loserPlayers.push_back(*it);
				break;
			case WinInfo::WIN_TEAM:
				// Boot everybody except the winning team.
				if ((*it)->userinfo.team != win.id)
					loserPlayers.push_back(*it);
				break;
			case WinInfo::WIN_DRAW:
			case WinInfo::WIN_NOBODY:
				// Draws are just another way of saying there were no winners.
				loserPlayers.push_back(*it);
				break;
			default:
				// Everyone won, or something strange happened.
				break;
			}

			// NOBODY/UNKNOWN should default to never touching the queue.
		}

		std::vector<std::string> names;
		for (PlayersView::iterator it = loserPlayers.begin(); it != loserPlayers.end();
		     ++it)
		{
			SV_SetPlayerSpec(**it, true, true);
			names.push_back((*it)->userinfo.netname);

			// Allow this player to queue up immediately without waiting for
			// ReJoinDelay
			(*it)->joindelay = 0;
		}

		if (names.size() > 2)
		{
			names.back() = std::string("and ") + names.back();
			SV_BroadcastPrintFmt("{} lost the last game and were forced to spectate.\n",
			                     JoinStrings(names, ", "));
		}
		else if (names.size() == 2)
		{
			SV_BroadcastPrintFmt(
			    "{} and {} lost the last game and were forced to spectate.\n",
			    names.at(0), names.at(1));
		}
		else if (names.size() == 1)
		{
			SV_BroadcastPrintFmt("{} lost the last game and was forced to spectate.\n",
			                     names.at(0));
		}
	}

	SV_UpdatePlayerQueuePositions(G_CanJoinGameStart, NULL);
}

void SV_UpdatePlayerQueuePositions(JoinTest joinTest, player_t* disconnectPlayer)
{
	int queuePos = 1;
	PlayersView queued;
	PlayersView queueUpdates;

	for (auto& player : ::players)
	{
		if (player.QueuePosition > 0 && disconnectPlayer != &(player))
			queued.push_back(&(player));
	}

	std::sort(queued.begin(), queued.end(), CompareQueuePosition);

	for (size_t i = 0; i < queued.size(); i++)
	{
		player_t* p = queued.at(i);

		if (p->QueuePosition == 0)
			continue;

		if (joinTest() == JOIN_OK)
		{
			p->QueuePosition = 0;
			SV_JoinPlayer(*p, false);
			queueUpdates.push_back(p);
		}
		else
		{
			if (p->QueuePosition != queuePos)
				queueUpdates.push_back(p);
			p->QueuePosition = queuePos++;
		}
	}

	if (disconnectPlayer && disconnectPlayer->QueuePosition > 0)
	{
		disconnectPlayer->QueuePosition = 0;
		queueUpdates.push_back(disconnectPlayer);
	}

	for (auto& dest : ::players)
	{
		for (const auto& source : queueUpdates)
		{
			SV_SendPlayerQueuePosition(source, &dest);
		}
	}
}

void SV_SendPlayerQueuePositions(player_t* dest, bool initConnect)
{
	for (const auto& player : players)
	{
		if (initConnect && player.QueuePosition == 0)
			continue;
		SV_SendPlayerQueuePosition(&player, dest);
	}
}

void SV_SendPlayerQueuePosition(const player_t* source, player_t* dest)
{
	MSG_WriteSVC((dest->client.messenger.ReliableBuf()), SVC_PlayerQueuePos(*source));
}

bool CompareQueuePosition(const player_t* p1, const player_t* p2)
{
	return p1->QueuePosition < p2->QueuePosition;
}

void SV_ClearPlayerQueue()
{
	for (auto& player : players)
		player.QueuePosition = 0;

	for (auto& player : players)
		SV_SendPlayerQueuePositions(&player, false);
}

void SV_SendExecuteLineSpecial(byte special, const line_t* line, const AActor* activator, int arg0,
                               int arg1, int arg2, int arg3, int arg4)
{
	if (P_LineSpecialMovesSector(special))
		return;

	for (auto& player : players)
	{
		if (!(player.ingame()))
			continue;

		client_t* cl = &player.client;

		int args[5] = { arg0, arg1, arg2, arg3, arg4 };
		MSG_WriteSVC(cl->messenger.ReliableBuf(),
		             SVC_ExecuteLineSpecial(special, line, activator, args));
	}
}

//
// If playerOnly is true and the activator is a player, then it will only be
// sent to the activating player.
//
void SV_ACSExecuteSpecial(byte special, const AActor* activator, const char* print,
                          bool playerOnly, const std::vector<int>& args)
{
	player_t* sendPlayer = nullptr;
	if (playerOnly && activator != nullptr && activator->player != nullptr)
		sendPlayer = activator->player;

	for (auto& player : players)
	{
		if (!(player.ingame()) || (sendPlayer != nullptr && sendPlayer != &player))
			continue;

		client_t* cl = &player.client;

		MSG_WriteSVC(cl->messenger.ReliableBuf(),
		             SVC_ExecuteACSSpecial(special, activator, print, args));
	}
}

void SV_UpdateShareKeys(player_t& player)
{
	// Player needs to be valid.
	if (!validplayer(player))
		return;

	// Disallow to spectators
	if (player.spectator)
		return;

	// Don't send to dead players... Yet, since they'll get it upon respawning
	if (player.health <= 0)
		return;

	// Update their keys informations
	for (int i = 0; i < NUMCARDS; i++) {
		player.cards[i] = keysfound[i];
	}

	// Refresh that new data to the client
	SV_SendPlayerInfo(player);
}

void SV_ShareKeys(card_t card, const player_t &player)
{
	// Add it to the KeysCheck array
	keysfound[card] = true;
	const char* coloritem = NULL;

	// If the server hasn't accepted to share keys yet, stop it.
	if (!sv_sharekeys)
		return;

	// Broadcast the key shared to
	gitem_t* item = FindCardItem(card);
	if (item != NULL)
	{
		switch (card)
		{
		case it_bluecard:
		case it_blueskull:
			coloritem = TEXTCOLOR_BLUE;
			break;
		case it_redcard:
		case it_redskull:
			coloritem = TEXTCOLOR_RED;
			break;
		case it_yellowcard:
		case it_yellowskull:
			coloritem = TEXTCOLOR_GOLD;
			break;
		default:
			coloritem = TEXTCOLOR_NORMAL;
		}

		SV_BroadcastPrintFmt("{} found the {}{}{}!\n", player.userinfo.netname,
		                     coloritem, item->pickup_name, TEXTCOLOR_NORMAL);
	}
	else
	{
		SV_BroadcastPrintFmt("{} found a key!\n", player.userinfo.netname);
	}

	// Refresh the inventory to everyone
	// ToDo: If we're the player who picked it, don't refresh our own inventory
	for (Players::iterator it = players.begin(); it != players.end(); ++it)
	{
		SV_UpdateShareKeys(*it);
	}
}

VERSION_CONTROL (sv_main_cpp, "$Id$")
