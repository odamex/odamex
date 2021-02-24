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

#include "cl_main.h"
#include "cmdlib.h"
#include "d_main.h"
#include "d_player.h"
#include "g_gametype.h"
#include "g_level.h"
#include "g_levelstate.h"
#include "m_argv.h"
#include "m_resfile.h"
#include "p_ctf.h"
#include "p_inter.h"
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
EXTERN_CVAR(cl_netdemoname)
EXTERN_CVAR(cl_splitnetdemos)

extern bool forcenetdemosplit;
extern int last_svgametic;
extern int last_player_update;
extern NetCommand localcmds[MAXSAVETICS];
extern std::map<unsigned short, SectorSnapshotManager> sector_snaps;
extern std::set<byte> teleported_players;

void CL_ClearPlayerJustTeleported(player_t* player);
void CL_ClearSectorSnapshots();
player_t& CL_FindPlayer(size_t id);
std::string CL_GenerateNetDemoFileName(
    const std::string& filename = cl_netdemoname.cstring());
bool CL_PlayerJustTeleported(player_t* player);
void CL_QuitAndTryDownload(const OWantFile& missing_file);
void CL_ResyncWorldIndex();
void P_SetPsprite(player_t* player, int position, statenum_t stnum);
void CL_SpectatePlayer(player_t& player, bool spectate);

/**
 * @brief svc_noop - Nothing to see here. Move along.
 */
void CL_Noop()
{
}

/**
 * @brief svc_disconnect - Disconnect a client from the server.
 */
void CL_Disconnect(const svc::DisconnectMsg& msg)
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

	Printf("%s", msg);
	CL_QuitNetGame();
}

/**
 * @brief svc_playerinfo - Your personal arsenal, as supplied by the server.
 */
void CL_PlayerInfo(const svc::PlayerInfoMsg& msg)
{
	player_t* p = &consoleplayer();

	uint16_t booleans = msg.inventory();
	for (int i = 0; i < NUMWEAPONS; i++)
	{
		p->weaponowned[i] = booleans & 1 << i;
	}
	for (int i = 0; i < NUMCARDS; i++)
	{
		p->cards[i] = booleans & 1 << (i + NUMWEAPONS);
	}
	p->backpack = booleans & 1 << (NUMWEAPONS + NUMCARDS);

	for (int i = 0; i < NUMAMMO; i++)
	{
		if (i < msg.ammo_size())
		{
			p->maxammo[i] = msg.ammo(i).maxammo();
			p->ammo[i] = msg.ammo(i).ammo();
		}
		else
		{
			p->maxammo[i] = 0;
			p->ammo[i] = 0;
		}
	}

	p->health = msg.health();
	p->armorpoints = msg.armorpoints();
	p->armortype = msg.armortype();
	p->lives = msg.lives();

	weapontype_t newweapon = static_cast<weapontype_t>(msg.weapon());
	if (newweapon > NUMWEAPONS) // bad weapon number, choose something else
	{
		newweapon = wp_fist;
	}
	if (newweapon != p->readyweapon)
	{
		p->pendingweapon = newweapon;
	}

	for (int i = 0; i < NUMPOWERS; i++)
	{
		if (i < msg.powers_size())
		{
			p->powers[i] = msg.powers(i);
		}
		else
		{
			p->powers[i] = 0;
		}
	}
}

/**
 * @brief svc_moveplayer - Move a player.
 */
void CL_MovePlayer(const svc::MovePlayerMsg& msg)
{
	byte who = msg.pid();
	player_t* p = &idplayer(who);

	fixed_t x = msg.pos().x();
	fixed_t y = msg.pos().y();
	fixed_t z = msg.pos().z();

	angle_t angle = msg.angle();
	angle_t pitch = msg.pitch();

	int frame = msg.frame();
	fixed_t momx = msg.mom().x();
	fixed_t momy = msg.mom().y();
	fixed_t momz = msg.mom().z();

	int invisibility = msg.invisibility();

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

void CL_UpdateLocalPlayer(const svc::UpdateLocalPlayerMsg& msg)
{
	player_t& p = consoleplayer();

	// The server has processed the ticcmd that the local client sent
	// during the the tic referenced below
	p.tic = msg.tic();

	fixed_t x = msg.pos().x();
	fixed_t y = msg.pos().y();
	fixed_t z = msg.pos().z();

	fixed_t momx = msg.mom().x();
	fixed_t momy = msg.mom().y();
	fixed_t momz = msg.mom().z();

	byte waterlevel = msg.waterlevel();

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
void CL_LevelLocals(const svc::LevelLocalsMsg& msg)
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
void CL_PingRequest(const svc::PingRequestMsg& msg)
{
	MSG_WriteMarker(&net_buffer, clc_pingreply);
	MSG_WriteLong(&net_buffer, msg.ms_time());
}

//
// LoadMap
//
// Read wad & deh filenames and map name from the server and loads
// the appropriate wads & map.
//
void CL_LoadMap(const svc::LoadMapMsg& msg)
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

extern int MeansOfDeath;

//
// CL_KillMobj
//
void CL_KillMobj(const svc::KillMobjMsg& msg)
{
	int srcid = msg.source_netid();
	int tgtid = msg.target_netid();
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

	target->health = health;

	if (!serverside && target->flags & MF_COUNTKILL)
		level.killed_monsters++;

	if (target->player == &consoleplayer())
		for (size_t i = 0; i < MAXSAVETICS; i++)
			::localcmds[i].clear();

	if (lives >= 0)
		target->player->lives = lives;

	P_KillMobj(source, target, inflictor, joinkill);
}

/**
 * @brief Updates less-vital members of a player struct.
 */
void CL_PlayerMembers(const svc::PlayerMembersMsg& msg)
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
void CL_TeamMembers(const svc::TeamMembersMsg& msg)
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

//
// CL_UpdateMovingSector
// Updates floorheight and ceilingheight of a sector.
//
void CL_MovingSector(const svc::MovingSectorMsg& msg)
{
	int sectornum = msg.sectornum();

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
		const svc::MovingSectorMsg_Snapshot& floor = msg.floor_mover();

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
		const svc::MovingSectorMsg_Snapshot& floor = msg.floor_mover();

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
		const svc::MovingSectorMsg_Snapshot& ceil = msg.ceiling_mover();

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
		const svc::MovingSectorMsg_Snapshot& ceil = msg.ceiling_mover();

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
		const svc::MovingSectorMsg_Snapshot& ceil = msg.ceiling_mover();

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
		const svc::MovingSectorMsg_Snapshot& ceil = msg.ceiling_mover();

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

void CL_PlayerState(const svc::PlayerStateMsg& msg)
{
	byte id = msg.pid();
	int health = msg.health();
	int armortype = msg.armortype();
	int armorpoints = msg.armorpoints();
	int lives = msg.lives();
	weapontype_t weap = static_cast<weapontype_t>(msg.readyweapon());

	byte cardByte = msg.cards();
	std::bitset<6> cardBits(cardByte);

	int ammo[NUMAMMO];
	for (int i = 0; i < NUMAMMO; i++)
	{
		if (i < msg.ammos_size())
		{
			ammo[i] = msg.ammos().Get(i);
		}
		else
		{
			ammo[i] = 0;
		}
	}

	statenum_t stnum[NUMPSPRITES] = {S_NULL, S_NULL};
	for (int i = 0; i < NUMPSPRITES; i++)
	{
		if (i < msg.pspstate_size())
		{
			unsigned int state = msg.pspstate().Get(i);
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
		if (i < msg.powers_size())
		{
			powerups[i] = msg.powers().Get(i);
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
void CL_LevelState(const svc::LevelStateMsg& msg)
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