// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1998-2006 by Randy Heit (ZDoom).
// Copyright (C) 2006-2021 by The Odamex Team.
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
//  Handlers for messages sent from the server.
//
//-----------------------------------------------------------------------------

#include "cl_svc.h"

#include <bitset>

#include "server.pb.h"

#include "c_effect.h"
#include "cl_main.h"
#include "cmdlib.h"
#include "d_main.h"
#include "d_player.h"
#include "g_gametype.h"
#include "g_level.h"
#include "g_levelstate.h"
#include "m_argv.h"
#include "m_random.h"
#include "m_resfile.h"
#include "p_ctf.h"
#include "p_inter.h"
#include "p_lnspec.h"
#include "p_mobj.h"
#include "r_state.h"
#include "s_sound.h"

// Extern data from other files.

EXTERN_CVAR(cl_autorecord)
EXTERN_CVAR(cl_autorecord_coop)
EXTERN_CVAR(cl_autorecord_ctf)
EXTERN_CVAR(cl_autorecord_deathmatch)
EXTERN_CVAR(cl_autorecord_duel)
EXTERN_CVAR(cl_autorecord_teamdm)
EXTERN_CVAR(cl_disconnectalert)
EXTERN_CVAR(cl_netdemoname)
EXTERN_CVAR(cl_splitnetdemos)

extern std::string digest;
extern bool forcenetdemosplit;
extern int last_svgametic;
extern int last_player_update;
extern NetCommand localcmds[MAXSAVETICS];
extern std::map<unsigned short, SectorSnapshotManager> sector_snaps;
extern std::set<byte> teleported_players;

void CL_CheckDisplayPlayer(void);
void CL_ClearPlayerJustTeleported(player_t* player);
void CL_ClearSectorSnapshots();
player_t& CL_FindPlayer(size_t id);
std::string CL_GenerateNetDemoFileName(
    const std::string& filename = cl_netdemoname.cstring());
bool CL_PlayerJustTeleported(player_t* player);
void CL_QuitAndTryDownload(const OWantFile& missing_file);
void CL_ResyncWorldIndex();
void P_ExplodeMissile(AActor* mo);
void P_PlayerLeavesGame(player_s* player);
void P_SetPsprite(player_t* player, int position, statenum_t stnum);
void CL_SpectatePlayer(player_t& player, bool spectate);

/**
 * @brief Unpack a bitfield into an array of booleans.
 */
static void UnpackBoolArray(bool* bools, size_t count, uint32_t in)
{
	for (size_t i = 0; i < count; i++)
	{
		bools[i] = in & BIT(i);
	}
}

/**
 * @brief Common code for activating a line.
 */
static void ActivateLine(AActor* mo, line_s* line, byte side,
                         LineActivationType activationType, byte special = 0,
                         int arg0 = 0, int arg1 = 0, int arg2 = 0, int arg3 = 0,
                         int arg4 = 0)
{
	// [SL] 2012-03-07 - If this is a player teleporting, add this player to
	// the set of recently teleported players.  This is used to flush past
	// positions since they cannot be used for interpolation.
	if (line && (mo && mo->player) &&
	    (line->special == Teleport || line->special == Teleport_NoFog ||
	     line->special == Teleport_NoStop || line->special == Teleport_Line))
	{
		teleported_players.insert(mo->player->id);

		// [SL] 2012-03-21 - Server takes care of moving players that teleport.
		// Don't allow client to process it since it screws up interpolation.
		return;
	}

	// [SL] 2012-04-25 - Clients will receive updates for sectors so they do not
	// need to create moving sectors on their own in response to svc_activateline
	if (line && P_LineSpecialMovesSector(line->special))
		return;

	s_SpecialFromServer = true;

	switch (activationType)
	{
	case LineCross:
		if (line)
			P_CrossSpecialLine(line - lines, side, mo);
		break;
	case LineUse:
		if (line)
			P_UseSpecialLine(mo, line, side);
		break;
	case LineShoot:
		if (line)
			P_ShootSpecialLine(mo, line);
		break;
	case LinePush:
		if (line)
			P_PushSpecialLine(mo, line, side);
		break;
	case LineACS:
		LineSpecials[special](line, mo, arg0, arg1, arg2, arg3, arg4);
		break;
	default:
		break;
	}

	s_SpecialFromServer = false;
}

/**
 * @brief svc_noop - Nothing to see here. Move along.
 */
void CL_Noop()
{
}

/**
 * @brief svc_disconnect - Disconnect a client from the server.
 */
void CL_Disconnect(const odaproto::svc::Disconnect& msg)
{
	std::string buffer;
	if (!msg.message().empty())
	{
		StrFormat(buffer, "Disconnected from server: %s", msg.message().c_str());
	}
	else
	{
		StrFormat(buffer, "Disconnected from server\n");
	}

	Printf("%s", msg.message().c_str());
	CL_QuitNetGame();
}

/**
 * @brief svc_playerinfo - Your personal arsenal, as supplied by the server.
 */
void CL_PlayerInfo(const odaproto::svc::PlayerInfo& msg)
{
	player_t& p = consoleplayer();

	uint32_t weaponowned = msg.player().weaponowned();
	UnpackBoolArray(p.weaponowned, NUMWEAPONS, weaponowned);

	uint32_t cards = msg.player().cards();
	UnpackBoolArray(p.cards, NUMCARDS, cards);

	p.backpack = msg.player().backpack();

	for (int i = 0; i < NUMAMMO; i++)
	{
		if (i < msg.player().ammo_size())
			p.ammo[i] = msg.player().ammo(i);
		else
			p.ammo[i] = 0;

		if (i < msg.player().maxammo_size())
			p.maxammo[i] = msg.player().maxammo(i);
		else
			p.maxammo[i] = 0;
	}

	p.health = msg.player().health();
	p.armorpoints = msg.player().armorpoints();
	p.armortype = msg.player().armortype();
	p.lives = msg.player().lives();

	weapontype_t pending = static_cast<weapontype_t>(msg.player().pendingweapon());
	if (pending != wp_nochange && pending < NUMWEAPONS)
	{
		p.pendingweapon = pending;
	}
	weapontype_t readyweapon = static_cast<weapontype_t>(msg.player().readyweapon());
	if (readyweapon != p.readyweapon && readyweapon < NUMWEAPONS)
	{
		p.pendingweapon = readyweapon;
	}

	for (int i = 0; i < NUMPOWERS; i++)
	{
		if (i < msg.player().powers_size())
		{
			p.powers[i] = msg.player().powers(i);
		}
		else
		{
			p.powers[i] = 0;
		}
	}
}

/**
 * @brief svc_moveplayer - Move a player.
 */
void CL_MovePlayer(const odaproto::svc::MovePlayer& msg)
{
	byte who = msg.player().playerid();
	player_t* p = &idplayer(who);

	fixed_t x = msg.actor().pos().x();
	fixed_t y = msg.actor().pos().y();
	fixed_t z = msg.actor().pos().z();

	angle_t angle = msg.actor().angle();
	angle_t pitch = msg.actor().pitch();

	int frame = msg.frame();
	fixed_t momx = msg.actor().mom().x();
	fixed_t momy = msg.actor().mom().y();
	fixed_t momz = msg.actor().mom().z();

	int invisibility = 0;
	if (msg.player().powers_size() >= pw_invisibility)
		invisibility = msg.player().powers().Get(pw_invisibility);

	if (!validplayer(*p) || !p->mo)
		return;

	// Mark the gametic this update arrived in for prediction code
	p->tic = gametic;

	// GhostlyDeath -- Servers will never send updates on spectators
	if (p->spectator && (p != &consoleplayer()))
		p->spectator = 0;

	// [Russell] - hack, read and set invisibility flag
	p->powers[pw_invisibility] = invisibility;
	if (p->powers[pw_invisibility])
		p->mo->flags |= MF_SHADOW;
	else
		p->mo->flags &= ~MF_SHADOW;

	// This is a very bright frame. Looks cool :)
	if (frame == PLAYER_FULLBRIGHTFRAME)
		frame = 32773;

	// denis - fixme - security
	if (!p->mo->sprite ||
	    (p->mo->frame & FF_FRAMEMASK) >= sprites[p->mo->sprite].numframes)
		return;

	p->last_received = gametic;
	::last_player_update = gametic;

	// [SL] 2012-02-21 - Save the position information to a snapshot
	int snaptime = ::last_svgametic;
	PlayerSnapshot newsnap(snaptime);
	newsnap.setAuthoritative(true);

	newsnap.setX(x);
	newsnap.setY(y);
	newsnap.setZ(z);
	newsnap.setMomX(momx);
	newsnap.setMomY(momy);
	newsnap.setMomZ(momz);
	newsnap.setAngle(angle);
	newsnap.setPitch(pitch);
	newsnap.setFrame(frame);

	// Mark the snapshot as continuous unless the player just teleported
	// and lerping should be disabled
	newsnap.setContinuous(!CL_PlayerJustTeleported(p));
	CL_ClearPlayerJustTeleported(p);

	p->snapshots.addSnapshot(newsnap);
}

void CL_UpdateLocalPlayer(const odaproto::svc::UpdateLocalPlayer& msg)
{
	player_t& p = consoleplayer();

	// The server has processed the ticcmd that the local client sent
	// during the the tic referenced below
	p.tic = msg.tic();

	fixed_t x = msg.actor().pos().x();
	fixed_t y = msg.actor().pos().y();
	fixed_t z = msg.actor().pos().z();

	fixed_t momx = msg.actor().mom().x();
	fixed_t momy = msg.actor().mom().y();
	fixed_t momz = msg.actor().mom().z();

	byte waterlevel = msg.actor().waterlevel();

	int snaptime = ::last_svgametic;
	PlayerSnapshot newsnapshot(snaptime);
	newsnapshot.setAuthoritative(true);
	newsnapshot.setX(x);
	newsnapshot.setY(y);
	newsnapshot.setZ(z);
	newsnapshot.setMomX(momx);
	newsnapshot.setMomY(momy);
	newsnapshot.setMomZ(momz);
	newsnapshot.setWaterLevel(waterlevel);

	// Mark the snapshot as continuous unless the player just teleported
	// and lerping should be disabled
	newsnapshot.setContinuous(!CL_PlayerJustTeleported(&p));
	CL_ClearPlayerJustTeleported(&p);

	consoleplayer().snapshots.addSnapshot(newsnapshot);
}

// Set level locals.
void CL_LevelLocals(const odaproto::svc::LevelLocals& msg)
{
	uint32_t flags = msg.flags();

	if (flags & SVC_LL_TIME)
	{
		::level.time = msg.time();
	}

	if (flags & SVC_LL_TOTALS)
	{
		::level.total_secrets = msg.total_secrets();
		::level.total_items = msg.total_items();
		::level.total_monsters = msg.total_monsters();
	}

	if (flags & SVC_LL_SECRETS)
	{
		::level.found_secrets = msg.found_secrets();
	}

	if (flags & SVC_LL_ITEMS)
	{
		::level.found_items = msg.found_items();
	}

	if (flags & SVC_LL_MONSTERS)
	{
		::level.killed_monsters = msg.killed_monsters();
	}

	if (flags & SVC_LL_MONSTER_RESPAWNS)
	{
		::level.respawned_monsters = msg.respawned_monsters();
	}
}

//
// CL_SendPingReply
//
// Replies to a server's ping request
//
// [SL] 2011-05-11 - Changed from CL_ResendSvGametic to CL_SendPingReply
// for clarity since it sends timestamps, not gametics.
//
void CL_PingRequest(const odaproto::svc::PingRequest& msg)
{
	MSG_WriteMarker(&net_buffer, clc_pingreply);
	MSG_WriteLong(&net_buffer, msg.ms_time());
}

//
// CL_UpdatePing
// Update ping value
//
void CL_UpdatePing(const odaproto::svc::UpdatePing& msg)
{
	player_t& p = idplayer(msg.pid());
	if (!validplayer(p))
		return;

	p.ping = msg.ping();
}

//
// CL_SpawnMobj
//
void CL_SpawnMobj(const odaproto::svc::SpawnMobj& msg)
{
	fixed_t x = msg.actor().pos().x();
	fixed_t y = msg.actor().pos().y();
	fixed_t z = msg.actor().pos().z();
	angle_t angle = msg.actor().angle();

	mobjtype_t type = static_cast<mobjtype_t>(msg.actor().type());
	size_t netid = msg.actor().netid();
	byte rndindex = msg.actor().rndindex();
	statenum_t state = static_cast<statenum_t>(msg.actor().statenum());

	if (type < MT_PLAYER || type >= NUMMOBJTYPES)
		return;

	P_ClearId(netid);

	AActor* mo = new AActor(x, y, z, type);

	// denis - puff hack
	if (mo->type == MT_PUFF)
	{
		mo->momz = FRACUNIT;
		mo->tics -= M_Random() & 3;
		if (mo->tics < 1)
			mo->tics = 1;
	}

	mo->angle = angle;
	P_SetThingId(mo, netid);
	mo->rndindex = rndindex;

	if (state >= S_NULL && state < NUMSTATES)
	{
		P_SetMobjState(mo, state);
	}

	if (mo->flags & MF_MISSILE)
	{
		AActor* target = P_FindThingById(msg.target_netid());
		if (target)
		{
			mo->target = target->ptr();
		}

		mo->momx = msg.actor().mom().x();
		mo->momx = msg.actor().mom().x();
		mo->momx = msg.actor().mom().x();
		mo->angle = msg.actor().angle();
	}

	if (serverside && mo->flags & MF_COUNTKILL)
	{
		level.total_monsters++;
	}

	if (connected && (mo->flags & MF_MISSILE) && mo->info->seesound)
	{
		S_Sound(mo, CHAN_VOICE, mo->info->seesound, 1, ATTN_NORM);
	}

	if (mo->type == MT_IFOG)
	{
		S_Sound(mo, CHAN_VOICE, "misc/spawn", 1, ATTN_IDLE);
	}

	if (mo->type == MT_TFOG)
	{
		if (level.time) // don't play sound on first tic of the level
		{
			S_Sound(mo, CHAN_VOICE, "misc/teleport", 1, ATTN_NORM);
		}
	}

	if (type == MT_FOUNTAIN)
	{
		if (msg.args_size() >= 1)
			mo->effects = msg.args().Get(0) << FX_FOUNTAINSHIFT;
	}

	if (type == MT_ZDOOMBRIDGE)
	{
		if (msg.args_size() >= 1)
			mo->radius = msg.args().Get(0) << FRACBITS;
		if (msg.args_size() >= 2)
			mo->height = msg.args().Get(1) << FRACBITS;
	}

	if (msg.flags() & SVC_SM_FLAGS)
	{
		mo->flags = msg.actor().flags();
	}

	if (msg.flags() & SVC_SM_CORPSE)
	{
		int frame = msg.actor().frame();
		int tics = msg.actor().tics();

		if (tics == 0xFF)
			tics = -1;

		// already spawned as gibs?
		if (!mo || mo->state - states == S_GIBS)
			return;

		if ((frame & FF_FRAMEMASK) >= sprites[mo->sprite].numframes)
			return;

		mo->frame = frame;
		mo->tics = tics;

		// from P_KillMobj
		mo->flags &= ~(MF_SHOOTABLE | MF_FLOAT | MF_SKULLFLY);
		mo->flags |= MF_CORPSE | MF_DROPOFF;
		mo->height >>= 2;
		mo->flags &= ~MF_SOLID;

		if (mo->player)
			mo->player->playerstate = PST_DEAD;
	}
}

//
// CL_DisconnectClient
//
void CL_DisconnectClient(const odaproto::svc::DisconnectClient& msg)
{
	player_t& player = idplayer(msg.pid());
	if (players.empty() || !validplayer(player))
		return;

	if (player.mo)
	{
		P_DisconnectEffect(player.mo);

		// [AM] Destroying the player mobj is not our responsibility.  However, we do want
		//      to make sure that the mobj->player doesn't point to an invalid player.
		player.mo->player = NULL;
	}

	// Remove the player from the players list.
	for (Players::iterator it = players.begin(); it != players.end(); ++it)
	{
		if (it->id == player.id)
		{
			if (::cl_disconnectalert && &player != &consoleplayer())
				S_Sound(CHAN_INTERFACE, "misc/plpart", 1, ATTN_NONE);
			if (!it->spectator)
				P_PlayerLeavesGame(&(*it));
			players.erase(it);
			break;
		}
	}

	// if this was our displayplayer, update camera
	CL_CheckDisplayPlayer();
}

//
// LoadMap
//
// Read wad & deh filenames and map name from the server and loads
// the appropriate wads & map.
//
void CL_LoadMap(const odaproto::svc::LoadMap& msg)
{
	bool splitnetdemo =
	    (netdemo.isRecording() && ::cl_splitnetdemos) || ::forcenetdemosplit;
	::forcenetdemosplit = false;

	if (splitnetdemo)
		netdemo.stopRecording();

	size_t wadcount = msg.wadnames_size();
	OWantFiles newwadfiles;
	newwadfiles.reserve(wadcount);
	for (size_t i = 0; i < wadcount; i++)
	{
		std::string name = msg.wadnames().Get(i).name();
		std::string hash = msg.wadnames().Get(i).hash();

		OWantFile file;
		if (!OWantFile::makeWithHash(file, name, OFILE_WAD, hash))
		{
			Printf(PRINT_WARNING,
			       "Could not construct wanted file \"%s\" that server requested.\n",
			       name.c_str());
			CL_QuitNetGame();
			return;
		}
		newwadfiles.push_back(file);
	}

	size_t patchcount = msg.patchnames_size();
	OWantFiles newpatchfiles;
	newpatchfiles.reserve(patchcount);
	for (size_t i = 0; i < patchcount; i++)
	{
		std::string name = msg.patchnames().Get(i).name();
		std::string hash = msg.patchnames().Get(i).hash();

		OWantFile file;
		if (!OWantFile::makeWithHash(file, name, OFILE_DEH, hash))
		{
			Printf(PRINT_WARNING,
			       "Could not construct wanted patch \"%s\" that server requested.\n",
			       name.c_str());
			CL_QuitNetGame();
			return;
		}
		newpatchfiles.push_back(file);
	}

	std::string mapname = msg.mapname();
	int server_level_time = msg.time();

	// Load the specified WAD and DEH files and change the level.
	// if any WADs are missing, reconnect to begin downloading.
	G_LoadWad(newwadfiles, newpatchfiles);

	if (!missingfiles.empty())
	{
		OWantFile missing_file = missingfiles.front();
		CL_QuitAndTryDownload(missing_file);
		return;
	}

	// [SL] 2012-12-02 - Force the music to stop when the new map uses
	// the same music lump name that is currently playing. Otherwise,
	// the music from the old wad continues to play...
	S_StopMusic();

	G_InitNew(mapname.c_str());

	// [AM] Sync the server's level time with the client.
	::level.time = server_level_time;

	::movingsectors.clear();
	::teleported_players.clear();

	CL_ClearSectorSnapshots();
	for (Players::iterator it = players.begin(); it != players.end(); ++it)
		it->snapshots.clearSnapshots();

	// reset the world_index (force it to sync)
	CL_ResyncWorldIndex();
	::last_svgametic = 0;

	CTF_CheckFlags(consoleplayer());

	gameaction = ga_nothing;

	// Autorecord netdemo or continue recording in a new file
	if (!(netdemo.isPlaying() || netdemo.isRecording() || netdemo.isPaused()))
	{
		std::string filename;

		bool isFFA = sv_gametype == GM_DM && !G_IsDuelGame();
		bool isDuel = sv_gametype == GM_DM && G_IsDuelGame();

		bool bCanAutorecord = (sv_gametype == GM_COOP && cl_autorecord_coop) ||
		                      (isFFA && cl_autorecord_deathmatch) ||
		                      (isDuel && cl_autorecord_duel) ||
		                      (sv_gametype == GM_TEAMDM && cl_autorecord_teamdm) ||
		                      (sv_gametype == GM_CTF && cl_autorecord_ctf);

		size_t param = Args.CheckParm("-netrecord");
		if (param && Args.GetArg(param + 1))
			filename = Args.GetArg(param + 1);

		if (((splitnetdemo || cl_autorecord) && bCanAutorecord) || param)
		{
			if (filename.empty())
				filename = CL_GenerateNetDemoFileName();
			else
				filename = CL_GenerateNetDemoFileName(filename);

			// NOTE(jsd): Presumably a warning is already printed.
			if (filename.empty())
			{
				netdemo.stopRecording();
				return;
			}

			netdemo.startRecording(filename);
		}
	}

	// write the map index to the netdemo
	if (netdemo.isRecording())
		netdemo.writeMapChange();
}

void CL_ConsolePlayer(const odaproto::svc::ConsolePlayer& msg)
{
	::displayplayer_id = ::consoleplayer_id = msg.pid();
	::digest = msg.digest();
}

//
// CL_ExplodeMissile
//
void CL_ExplodeMissile(const odaproto::svc::ExplodeMissile& msg)
{
	int netid = msg.netid();

	AActor* mo = P_FindThingById(netid);

	if (!mo)
		return;

	P_ExplodeMissile(mo);
}

void CL_UpdateMobj(const odaproto::svc::UpdateMobj& msg)
{
	int netid = msg.actor().netid();
	AActor* mo = P_FindThingById(netid);

	if (!mo)
		return;

	fixed_t x = msg.actor().pos().x();
	fixed_t y = msg.actor().pos().y();
	fixed_t z = msg.actor().pos().z();
	byte rndindex = msg.actor().rndindex();
	fixed_t momx = msg.actor().mom().x();
	fixed_t momy = msg.actor().mom().y();
	fixed_t momz = msg.actor().mom().z();
	angle_t angle = msg.actor().angle();
	byte movedir = msg.actor().movedir();
	int movecount = msg.actor().movecount();

	if (mo->player)
	{
		// [SL] 2013-07-21 - Save the position information to a snapshot
		int snaptime = last_svgametic;
		PlayerSnapshot newsnap(snaptime);
		newsnap.setAuthoritative(true);

		if (msg.flags() & SVC_UM_POS_RND)
		{
			newsnap.setX(x);
			newsnap.setY(y);
			newsnap.setZ(z);
			mo->rndindex = rndindex;
		}

		if (msg.flags() & SVC_UM_MOM_ANGLE)
		{
			newsnap.setAngle(angle);
			newsnap.setMomX(momx);
			newsnap.setMomY(momy);
			newsnap.setMomZ(momz);
		}

		mo->player->snapshots.addSnapshot(newsnap);
	}
	else
	{
		if (msg.flags() & SVC_UM_POS_RND)
		{
			CL_MoveThing(mo, x, y, z);
			mo->rndindex = rndindex;
		}

		if (msg.flags() & SVC_UM_MOM_ANGLE)
		{
			mo->angle = angle;
			mo->momx = momx;
			mo->momy = momy;
			mo->momz = momz;
		}
	}

	if (msg.flags() & SVC_UM_MOVEDIR)
	{
		mo->movedir = movedir;
		mo->movecount = movecount;
	}

	if (msg.flags() & SVC_UM_TARGET)
	{
		AActor* target = P_FindThingById(msg.actor().targetid());
		mo->target = target->ptr();
	}

	if (msg.flags() & SVC_UM_TRACER)
	{
		AActor* tracer = P_FindThingById(msg.actor().tracerid());
		mo->tracer = tracer->ptr();
	}
}

extern int MeansOfDeath;

//
// CL_KillMobj
//
void CL_KillMobj(const odaproto::svc::KillMobj& msg)
{
	int srcid = msg.source_netid();
	int tgtid = msg.target().netid();
	int infid = msg.inflictor_netid();
	int health = msg.health();
	::MeansOfDeath = msg.mod();
	bool joinkill = msg.joinkill();
	int lives = msg.lives();

	AActor* source = P_FindThingById(srcid);
	AActor* target = P_FindThingById(tgtid);
	AActor* inflictor = P_FindThingById(infid);

	if (!target)
		return;

	// This used to be bundled with a svc_movemobj and svc_mobjspeedangle,
	// so emulate them here.
	target->rndindex = msg.target().rndindex();

	if (target->player)
	{
		// [SL] 2013-07-21 - Save the position information to a snapshot
		int snaptime = last_svgametic;
		PlayerSnapshot newsnap(snaptime);
		newsnap.setAuthoritative(true);

		newsnap.setX(msg.target().pos().x());
		newsnap.setY(msg.target().pos().y());
		newsnap.setZ(msg.target().pos().z());
		newsnap.setAngle(msg.target().angle());
		newsnap.setMomX(msg.target().mom().x());
		newsnap.setMomY(msg.target().mom().y());
		newsnap.setMomZ(msg.target().mom().z());

		target->player->snapshots.addSnapshot(newsnap);
	}
	else
	{
		target->x = msg.target().pos().x();
		target->y = msg.target().pos().y();
		target->z = msg.target().pos().z();
		target->angle = msg.target().angle();
		target->momx = msg.target().mom().x();
		target->momy = msg.target().mom().y();
		target->momz = msg.target().mom().z();
	}

	target->health = health;

	if (!serverside && target->flags & MF_COUNTKILL)
		level.killed_monsters++;

	if (target->player == &consoleplayer())
		for (size_t i = 0; i < MAXSAVETICS; i++)
			localcmds[i].clear();

	if (target->player && lives >= 0)
		target->player->lives = lives;

	P_KillMobj(source, target, inflictor, joinkill);
}

/**
 * @brief Updates less-vital members of a player struct.
 */
void CL_PlayerMembers(const odaproto::svc::PlayerMembers& msg)
{
	player_t& p = CL_FindPlayer(msg.pid());
	byte flags = msg.flags();

	if (flags & SVC_PM_SPECTATOR)
	{
		CL_SpectatePlayer(p, msg.spectator());
	}

	if (flags & SVC_PM_READY)
	{
		p.ready = msg.ready();
	}

	if (flags & SVC_PM_LIVES)
	{
		p.lives = msg.lives();
	}

	if (flags & SVC_PM_SCORE)
	{
		p.roundwins = msg.roundwins();
		p.points = msg.points();
		p.fragcount = msg.fragcount();
		p.deathcount = msg.deathcount();
		p.killcount = msg.killcount();
		p.secretcount = msg.secretcount();
		p.totalpoints = msg.totalpoints();
		p.totaldeaths = msg.totaldeaths();
	}
}

//
// [deathz0r] Receive team frags/captures
//
void CL_TeamMembers(const odaproto::svc::TeamMembers& msg)
{
	team_t team = static_cast<team_t>(msg.team());
	int points = msg.points();
	int roundWins = msg.roundwins();

	// Ensure our team is valid.
	TeamInfo* info = GetTeamInfo(team);
	if (info->Team >= NUMTEAMS)
		return;

	info->Points = points;
	info->RoundWins = roundWins;
}

void CL_ActivateLine(const odaproto::svc::ActivateLine& msg)
{
	int linenum = msg.linenum();
	AActor* mo = P_FindThingById(msg.activator_netid());
	byte side = msg.side();
	LineActivationType activationType =
	    static_cast<LineActivationType>(msg.activation_type());

	if (!::lines || linenum >= ::numlines || linenum < 0)
		return;

	ActivateLine(mo, &::lines[linenum], side, activationType);
}

//
// CL_UpdateMovingSector
// Updates floorheight and ceilingheight of a sector.
//
void CL_MovingSector(const odaproto::svc::MovingSector& msg)
{
	int sectornum = msg.sector();

	fixed_t ceilingheight = msg.ceiling_height();
	fixed_t floorheight = msg.floor_height();

	uint32_t movers = msg.movers();
	movertype_t ceiling_mover = static_cast<movertype_t>(movers & BIT_MASK(0, 3));
	movertype_t floor_mover = static_cast<movertype_t>((movers & BIT_MASK(4, 7)) >> 4);

	if (ceiling_mover == SEC_ELEVATOR)
	{
		floor_mover = SEC_INVALID;
	}
	if (ceiling_mover == SEC_PILLAR)
	{
		floor_mover = SEC_INVALID;
	}

	SectorSnapshot snap(::last_svgametic);

	snap.setCeilingHeight(ceilingheight);
	snap.setFloorHeight(floorheight);

	if (floor_mover == SEC_FLOOR)
	{
		const odaproto::svc::MovingSector_Snapshot& floor = msg.floor_mover();

		// Floors/Stairbuilders
		snap.setFloorMoverType(SEC_FLOOR);
		snap.setFloorType(static_cast<DFloor::EFloor>(floor.floor_type()));
		snap.setFloorStatus(floor.floor_status());
		snap.setFloorCrush(floor.floor_crush());
		snap.setFloorDirection(floor.floor_dir());
		snap.setFloorSpecial(floor.floor_speed());
		snap.setFloorTexture(floor.floor_tex());
		snap.setFloorDestination(floor.floor_dest());
		snap.setFloorSpeed(floor.floor_speed());
		snap.setResetCounter(floor.reset_counter());
		snap.setOrgHeight(floor.orig_height());
		snap.setDelay(floor.delay());
		snap.setPauseTime(floor.pause_time());
		snap.setStepTime(floor.step_time());
		snap.setPerStepTime(floor.per_step_time());
		snap.setFloorOffset(floor.floor_offset());
		snap.setFloorChange(floor.floor_change());

		int LineIndex = floor.floor_line();

		if (!lines || LineIndex >= numlines || LineIndex < 0)
			snap.setFloorLine(NULL);
		else
			snap.setFloorLine(&lines[LineIndex]);
	}

	if (floor_mover == SEC_PLAT)
	{
		const odaproto::svc::MovingSector_Snapshot& floor = msg.floor_mover();

		// Platforms/Lifts
		snap.setFloorMoverType(SEC_PLAT);
		snap.setFloorSpeed(floor.floor_speed());
		snap.setFloorLow(floor.floor_low());
		snap.setFloorHigh(floor.floor_high());
		snap.setFloorWait(floor.floor_wait());
		snap.setFloorCounter(floor.floor_counter());
		snap.setFloorStatus(floor.floor_status());
		snap.setOldFloorStatus(floor.floor_old_status());
		snap.setFloorCrush(floor.floor_crush());
		snap.setFloorTag(floor.floor_tag());
		snap.setFloorType(floor.floor_type());
		snap.setFloorOffset(floor.floor_offset());
		snap.setFloorLip(floor.floor_lip());
	}

	if (ceiling_mover == SEC_CEILING)
	{
		const odaproto::svc::MovingSector_Snapshot& ceil = msg.ceiling_mover();

		// Ceilings / Crushers
		snap.setCeilingMoverType(SEC_CEILING);
		snap.setCeilingType(ceil.ceil_type());
		snap.setCeilingLow(ceil.ceil_low());
		snap.setCeilingHigh(ceil.ceil_high());
		snap.setCeilingSpeed(ceil.ceil_speed());
		snap.setCrusherSpeed1(ceil.crusher_speed_1());
		snap.setCrusherSpeed2(ceil.crusher_speed_2());
		snap.setCeilingCrush(ceil.ceil_crush());
		snap.setSilent(ceil.silent());
		snap.setCeilingDirection(ceil.ceil_dir());
		snap.setCeilingTexture(ceil.ceil_tex());
		snap.setCeilingSpecial(ceil.ceil_new_special());
		snap.setCeilingTag(ceil.ceil_tag());
		snap.setCeilingOldDirection(ceil.ceil_old_dir());
	}

	if (ceiling_mover == SEC_DOOR)
	{
		const odaproto::svc::MovingSector_Snapshot& ceil = msg.ceiling_mover();

		// Doors
		snap.setCeilingMoverType(SEC_DOOR);
		snap.setCeilingType(static_cast<DDoor::EVlDoor>(ceil.ceil_type()));
		snap.setCeilingHigh(ceil.ceil_height());
		snap.setCeilingSpeed(ceil.ceil_speed());
		snap.setCeilingWait(ceil.ceil_wait());
		snap.setCeilingCounter(ceil.ceil_counter());
		snap.setCeilingStatus(ceil.ceil_status());

		int LineIndex = ceil.ceil_line();

		// If the moving sector's line is -1, it is likely a type 666 door
		if (!lines || LineIndex >= numlines || LineIndex < 0)
			snap.setCeilingLine(NULL);
		else
			snap.setCeilingLine(&lines[LineIndex]);
	}

	if (ceiling_mover == SEC_ELEVATOR)
	{
		const odaproto::svc::MovingSector_Snapshot& ceil = msg.ceiling_mover();

		// Elevators
		snap.setCeilingMoverType(SEC_ELEVATOR);
		snap.setFloorMoverType(SEC_ELEVATOR);
		snap.setCeilingType(static_cast<DElevator::EElevator>(ceil.ceil_type()));
		snap.setFloorType(snap.getCeilingType());
		snap.setCeilingStatus(ceil.ceil_status());
		snap.setFloorStatus(snap.getCeilingStatus());
		snap.setCeilingDirection(ceil.ceil_dir());
		snap.setFloorDirection(snap.getCeilingDirection());
		snap.setFloorDestination(ceil.floor_dest());
		snap.setCeilingDestination(ceil.ceil_dest());
		snap.setCeilingSpeed(ceil.ceil_speed());
		snap.setFloorSpeed(snap.getCeilingSpeed());
	}

	if (ceiling_mover == SEC_PILLAR)
	{
		const odaproto::svc::MovingSector_Snapshot& ceil = msg.ceiling_mover();

		// Pillars
		snap.setCeilingMoverType(SEC_PILLAR);
		snap.setFloorMoverType(SEC_PILLAR);
		snap.setCeilingType(static_cast<DPillar::EPillar>(ceil.ceil_type()));
		snap.setFloorType(snap.getCeilingType());
		snap.setCeilingStatus(ceil.ceil_status());
		snap.setFloorStatus(snap.getCeilingStatus());
		snap.setFloorSpeed(ceil.floor_speed());
		snap.setCeilingSpeed(ceil.ceil_speed());
		snap.setFloorDestination(ceil.floor_dest());
		snap.setCeilingDestination(ceil.ceil_dest());
		snap.setCeilingCrush(ceil.ceil_crush());
		snap.setFloorCrush(snap.getCeilingCrush());
	}

	if (!sectors || sectornum >= numsectors)
	{
		return;
	}

	snap.setSector(&sectors[sectornum]);

	sector_snaps[sectornum].addSnapshot(snap);
}

void CL_PlayerState(const odaproto::svc::PlayerState& msg)
{
	byte id = msg.player().playerid();
	int health = msg.player().health();
	int armortype = msg.player().armortype();
	int armorpoints = msg.player().armorpoints();
	int lives = msg.player().lives();
	weapontype_t weap = static_cast<weapontype_t>(msg.player().readyweapon());

	byte cardByte = msg.player().cards();
	std::bitset<6> cardBits(cardByte);

	int ammo[NUMAMMO];
	for (int i = 0; i < NUMAMMO; i++)
	{
		if (i < msg.player().ammo_size())
		{
			ammo[i] = msg.player().ammo().Get(i);
		}
		else
		{
			ammo[i] = 0;
		}
	}

	statenum_t stnum[NUMPSPRITES] = {S_NULL, S_NULL};
	for (int i = 0; i < NUMPSPRITES; i++)
	{
		if (i < msg.player().psprites_size())
		{
			unsigned int state = msg.player().psprites().Get(i).statenum();
			if (state >= NUMSTATES)
			{
				continue;
			}
			stnum[i] = static_cast<statenum_t>(state);
		}
	}

	int powerups[NUMPOWERS];
	for (int i = 0; i < NUMPOWERS; i++)
	{
		if (i < msg.player().powers_size())
		{
			powerups[i] = msg.player().powers().Get(i);
		}
		else
		{
			powerups[i] = 0;
		}
	}

	player_t& player = idplayer(id);
	if (!validplayer(player) || !player.mo)
		return;

	player.health = player.mo->health = health;
	player.armortype = armortype;
	player.armorpoints = armorpoints;
	player.lives = lives;

	player.readyweapon = weap;
	player.pendingweapon = wp_nochange;

	for (int i = 0; i < NUMCARDS; i++)
		player.cards[i] = cardBits[i];

	if (!player.weaponowned[weap])
		P_GiveWeapon(&player, weap, false);

	for (int i = 0; i < NUMAMMO; i++)
		player.ammo[i] = ammo[i];

	for (int i = 0; i < NUMPSPRITES; i++)
		P_SetPsprite(&player, i, stnum[i]);

	for (int i = 0; i < NUMPOWERS; i++)
		player.powers[i] = powerups[i];
}

/**
 * @brief Set local levelstate.
 */
void CL_LevelState(const odaproto::svc::LevelState& msg)
{
	// Set local levelstate.
	SerializedLevelState sls;
	sls.state = static_cast<LevelState::States>(msg.state());
	sls.countdown_done_time = msg.countdown_done_time();
	sls.ingame_start_time = msg.ingame_start_time();
	sls.round_number = msg.round_number();
	sls.last_wininfo_type = static_cast<WinInfo::WinType>(msg.last_wininfo_type());
	sls.last_wininfo_id = msg.last_wininfo_id();
	::levelstate.unserialize(sls);
}

/**
 * @brief Update sector properties dynamically.
 */
void CL_SectorProperties(const odaproto::svc::SectorProperties& msg)
{
	int secnum = msg.sector();
	uint32_t changes = msg.changes();

	sector_t* sector;
	sector_t empty;

	if (secnum > -1 && secnum < numsectors)
	{
		sector = &sectors[secnum];
	}
	else
	{
		sector = &empty;
		extern dyncolormap_t NormalLight;
		empty.colormap = &NormalLight;
	}

	for (int i = 0, prop = 1; prop < SPC_Max; i++)
	{
		prop = 1 << i;
		if ((prop & changes) == 0)
			continue;

		switch (prop)
		{
		case SPC_FlatPic:
			sector->floorpic = msg.floorpic();
			sector->ceilingpic = msg.ceilingpic();
			break;
		case SPC_LightLevel:
			sector->lightlevel = msg.lightlevel();
			break;
		case SPC_Color: {
			byte r = msg.color().r();
			byte g = msg.color().g();
			byte b = msg.color().b();
			sector->colormap = GetSpecialLights(r, g, b, sector->colormap->fade.getr(),
			                                    sector->colormap->fade.getg(),
			                                    sector->colormap->fade.getb());
			break;
		}
		case SPC_Fade: {
			byte r = msg.color().r();
			byte g = msg.color().g();
			byte b = msg.color().b();
			sector->colormap = GetSpecialLights(sector->colormap->color.getr(),
			                                    sector->colormap->color.getg(),
			                                    sector->colormap->color.getb(), r, g, b);
			break;
		}
		case SPC_Gravity:
			*(int*)&sector->gravity = msg.gravity();
			break;
		case SPC_Panning:
			sector->ceiling_xoffs = msg.ceiling_offs().x();
			sector->ceiling_yoffs = msg.ceiling_offs().y();
			sector->floor_xoffs = msg.floor_offs().x();
			sector->floor_yoffs = msg.floor_offs().y();
			break;
		case SPC_Scale:
			sector->ceiling_xscale = msg.ceiling_scale().x();
			sector->ceiling_yscale = msg.ceiling_scale().y();
			sector->floor_xscale = msg.floor_scale().x();
			sector->floor_yscale = msg.floor_scale().y();
			break;
		case SPC_Rotation:
			sector->floor_angle = msg.floor_angle();
			sector->ceiling_angle = msg.ceiling_angle();
			break;
		case SPC_AlignBase:
			sector->base_ceiling_angle = msg.base_ceiling_angle();
			sector->base_ceiling_yoffs = msg.base_ceiling_offs().y();
			sector->base_floor_angle = msg.floor_angle();
			sector->base_floor_yoffs = msg.floor_offs().y();
		default:
			break;
		}
	}
}

void CL_ExecuteLineSpecial(const odaproto::svc::ExecuteLineSpecial& msg)
{
	byte special = msg.special();
	int linenum = msg.linenum();
	AActor* activator = P_FindThingById(msg.activator_netid());
	int arg0 = msg.arg0();
	int arg1 = msg.arg1();
	int arg2 = msg.arg2();
	int arg3 = msg.arg3();
	int arg4 = msg.arg4();

	if (linenum != -1 && linenum >= ::numlines)
		return;

	line_s* line = NULL;
	if (linenum != -1)
		line = &::lines[linenum];

	ActivateLine(activator, line, 0, LineACS, special, arg0, arg1, arg2, arg3, arg4);
}

/**
 * @brief Update a thinker.
 */
void CL_ThinkerUpdate(const odaproto::svc::ThinkerUpdate& msg)
{
	switch (msg.thinker_case())
	{
	case odaproto::svc::ThinkerUpdate::kScroller: {
		DScroller::EScrollType scrollType =
		    static_cast<DScroller::EScrollType>(msg.scroller().type());
		fixed_t dx = msg.scroller().scroll_x();
		fixed_t dy = msg.scroller().scroll_y();
		int affectee = msg.scroller().affectee();
		if (numsides <= 0 || numsectors <= 0)
			break;
		if (affectee < 0)
			break;
		if (scrollType == DScroller::sc_side && affectee > numsides)
			break;
		if (scrollType != DScroller::sc_side && affectee > numsectors)
			break;

		new DScroller(scrollType, dx, dy, -1, affectee, 0);
		break;
	}
	case odaproto::svc::ThinkerUpdate::kFireFlicker: {
		short secnum = msg.fire_flicker().sector();
		int min = msg.fire_flicker().min_light();
		int max = msg.fire_flicker().max_light();
		if (numsectors <= 0)
			break;
		if (secnum < numsectors)
			new DFireFlicker(&sectors[secnum], max, min);
		break;
	}
	case odaproto::svc::ThinkerUpdate::kFlicker: {
		short secnum = msg.flicker().sector();
		int min = msg.flicker().min_light();
		int max = msg.flicker().max_light();
		if (numsectors <= 0)
			break;
		if (secnum < numsectors)
			new DFlicker(&sectors[secnum], max, min);
		break;
	}
	case odaproto::svc::ThinkerUpdate::kLightFlash: {
		short secnum = msg.light_flash().sector();
		int min = msg.light_flash().min_light();
		int max = msg.light_flash().max_light();
		if (numsectors <= 0)
			break;
		if (secnum < numsectors)
			new DLightFlash(&sectors[secnum], min, max);
		break;
	}
	case odaproto::svc::ThinkerUpdate::kStrobe: {
		short secnum = msg.strobe().sector();
		int min = msg.strobe().min_light();
		int max = msg.strobe().max_light();
		int dark = msg.strobe().dark_time();
		int bright = msg.strobe().bright_time();
		int count = msg.strobe().count();
		if (numsectors <= 0)
			break;
		if (secnum < numsectors)
		{
			DStrobe* strobe = new DStrobe(&sectors[secnum], max, min, bright, dark);
			strobe->SetCount(count);
		}
		break;
	}
	case odaproto::svc::ThinkerUpdate::kGlow: {
		short secnum = msg.glow().sector();
		if (numsectors <= 0)
			break;
		if (secnum < numsectors)
			new DGlow(&sectors[secnum]);
		break;
	}
	case odaproto::svc::ThinkerUpdate::kGlow2: {
		short secnum = msg.glow2().sector();
		int start = msg.glow2().start();
		int end = msg.glow2().end();
		int tics = msg.glow2().max_tics();
		bool oneShot = msg.glow2().one_shot();
		if (numsectors <= 0)
			break;
		if (secnum < numsectors)
			new DGlow2(&sectors[secnum], start, end, tics, oneShot);
		break;
	}
	case odaproto::svc::ThinkerUpdate::kPhased: {
		short secnum = msg.phased().sector();
		int base = msg.phased().base_level();
		int phase = msg.phased().phase();
		if (numsectors <= 0)
			break;
		if (secnum < numsectors)
			new DPhased(&sectors[secnum], base, phase);
		break;
	}
	default:
		break;
	}
}

void CL_NetdemoCap(const odaproto::svc::NetdemoCap& msg)
{
	player_t* clientPlayer = &consoleplayer();
	fixed_t x, y, z;
	fixed_t momx, momy, momz;
	fixed_t pitch, viewz, viewheight, deltaviewheight;
	angle_t angle;
	int jumpTics, reactiontime;
	byte waterlevel;

	clientPlayer->cmd.clear();
	clientPlayer->cmd.unserialize(msg.player_cmd());

	waterlevel = msg.actor().waterlevel();
	x = msg.actor().pos().x();
	y = msg.actor().pos().y();
	z = msg.actor().pos().z();
	momx = msg.actor().mom().x();
	momy = msg.actor().mom().y();
	momz = msg.actor().mom().z();
	angle = msg.actor().angle();
	pitch = msg.actor().pitch();
	viewz = msg.player().viewz();
	viewheight = msg.player().viewheight();
	deltaviewheight = msg.player().deltaviewheight();
	jumpTics = msg.player().jumptics();
	reactiontime = msg.actor().reactiontime();
	clientPlayer->readyweapon = static_cast<weapontype_t>(msg.player().readyweapon());
	clientPlayer->pendingweapon = static_cast<weapontype_t>(msg.player().pendingweapon());

	if (clientPlayer->mo)
	{
		clientPlayer->mo->x = x;
		clientPlayer->mo->y = y;
		clientPlayer->mo->z = z;
		clientPlayer->mo->momx = momx;
		clientPlayer->mo->momy = momy;
		clientPlayer->mo->momz = momz;
		clientPlayer->mo->angle = angle;
		clientPlayer->mo->pitch = pitch;
		clientPlayer->viewz = viewz;
		clientPlayer->viewheight = viewheight;
		clientPlayer->deltaviewheight = deltaviewheight;
		clientPlayer->jumpTics = jumpTics;
		clientPlayer->mo->reactiontime = reactiontime;
		clientPlayer->mo->waterlevel = waterlevel;
	}
}
