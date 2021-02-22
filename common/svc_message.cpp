// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2021 by Alex Mayfield.
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
//   Server message functions.
//   - Functions should send exactly one message.
//   - Functions should be named after the message they send.
//   - Functions should be self-contained and not rely on global state.
//   - Functions should take a buf_t& as a first parameter.
//
//-----------------------------------------------------------------------------

#include <bitset>

#include "svc_message.h"

#include "d_main.h"
#include "i_system.h"
#include "p_lnspec.h"
#include "p_local.h"
#include "server.pb.h"

void SVC_Disconnect(buf_t& b, const char* message)
{
	svc::DisconnectMsg msg;
	if (message != NULL)
	{
		msg.set_message(message);
	}

	MSG_WriteMarker(&b, svc_disconnect);
	MSG_WriteProto(&b, msg);
}

/**
 * @brief Send information about a player.
 */
void SVC_PlayerInfo(buf_t& b, player_t& player)
{
	svc::PlayerInfoMsg msg;

	// [AM] 9 weapons, 6 cards, 1 backpack = 16 bits
	uint16_t booleans = 0;
	for (int i = 0; i < NUMWEAPONS; i++)
	{
		if (player.weaponowned[i])
		{
			booleans |= (1 << i);
		}
	}
	for (int i = 0; i < NUMCARDS; i++)
	{
		if (player.cards[i])
		{
			booleans |= (1 << (i + NUMWEAPONS));
		}
	}
	if (player.backpack)
	{
		booleans |= (1 << (NUMWEAPONS + NUMCARDS));
	}
	msg.set_inventory(booleans);

	for (int i = 0; i < NUMAMMO; i++)
	{
		svc::PlayerInfoMsg_PlayerAmmo* ammo = msg.add_ammo();
		ammo->set_maxammo(player.maxammo[i]);
		ammo->set_ammo(player.ammo[i]);
	}

	msg.set_health(player.health);
	msg.set_armorpoints(player.armorpoints);
	msg.set_armortype(player.armortype);
	msg.set_lives(player.lives);

	// If the player has a pending weapon then tell the client it has changed
	// The client will set the pendingweapon to this weapon if it doesn't match the
	// readyweapon
	if (player.pendingweapon == wp_nochange)
	{
		msg.set_weapon(player.readyweapon);
	}
	else
	{
		msg.set_weapon(player.pendingweapon);
	}

	for (int i = 0; i < NUMPOWERS; i++)
	{
		msg.add_powers(player.powers[i]);
	}

	MSG_WriteMarker(&b, svc_playerinfo);
	MSG_WriteProto(&b, msg);
}

/**
 * @brief Change the location of a player.
 */
void SVC_MovePlayer(buf_t& b, player_t& player, const int tic)
{
	svc::MovePlayerMsg msg;

	msg.set_pid(player.id); // player number

	// [SL] 2011-09-14 - the most recently processed ticcmd from the
	// client we're sending this message to.
	msg.set_tic(tic);

	svc::Vec3* pos = msg.mutable_pos();
	pos->set_x(player.mo->x);
	pos->set_y(player.mo->y);
	pos->set_z(player.mo->z);

	msg.set_angle(player.mo->angle);
	msg.set_pitch(player.mo->pitch);

	if (player.mo->frame == 32773)
	{
		msg.set_frame(PLAYER_FULLBRIGHTFRAME);
	}
	else
	{
		msg.set_frame(player.mo->frame);
	}

	// write velocity
	svc::Vec3* mom = msg.mutable_mom();
	mom->set_x(player.mo->momx);
	mom->set_y(player.mo->momy);
	mom->set_z(player.mo->momz);

	// [Russell] - hack, tell the client about the partial
	// invisibility power of another player.. (cheaters can disable
	// this but its all we have for now)
	msg.set_invisibility(player.powers[pw_invisibility] > 0);

	MSG_WriteMarker(&b, svc_moveplayer);
	MSG_WriteProto(&b, msg);
}

/**
 * @brief Send the local player position for a client.
 */
void SVC_UpdateLocalPlayer(buf_t& b, AActor& mo, const int tic)
{
	// client player will update his position if packets were missed
	svc::UpdateLocalPlayerMsg msg;

	// client-tic of the most recently processed ticcmd for this client
	msg.set_tic(tic);

	svc::Vec3* pos = msg.mutable_pos();
	pos->set_x(mo.x);
	pos->set_y(mo.y);
	pos->set_z(mo.z);

	svc::Vec3* mom = msg.mutable_mom();
	mom->set_x(mo.momx);
	mom->set_y(mo.momy);
	mom->set_z(mo.momz);

	msg.set_waterlevel(mo.waterlevel);

	MSG_WriteMarker(&b, svc_updatelocalplayer);
	MSG_WriteProto(&b, msg);
}

/**
 * @brief Persist level locals to the client.
 *
 * @param b Buffer to write to.
 * @param locals Level locals struct to send.
 * @param flags SVC_LL_* bit flags to designate what gets sent.
 */
void SVC_LevelLocals(buf_t& b, const level_locals_t& locals, uint32_t flags)
{
	svc::LevelLocalsMsg msg;

	msg.set_flags(flags);

	if (flags & SVC_LL_TIME)
	{
		msg.set_time(locals.time);
	}

	if (flags & SVC_LL_TOTALS)
	{
		msg.set_total_secrets(locals.total_secrets);
		msg.set_total_items(locals.total_items);
		msg.set_total_monsters(locals.total_monsters);
	}

	if (flags & SVC_LL_SECRETS)
	{
		msg.set_found_secrets(locals.found_secrets);
	}

	if (flags & SVC_LL_ITEMS)
	{
		msg.set_found_items(locals.found_items);
	}

	if (flags & SVC_LL_MONSTERS)
	{
		msg.set_killed_monsters(locals.killed_monsters);
	}

	if (flags & SVC_LL_MONSTER_RESPAWNS)
	{
		msg.set_respawned_monsters(locals.respawned_monsters);
	}

	MSG_WriteMarker(&b, svc_levellocals);
	MSG_WriteProto(&b, msg);
}

/**
 * @brief Send the server's current time in MS to the client.
 */
void SVC_PingRequest(buf_t& b)
{
	svc::PingRequestMsg msg;
	msg.set_ms_time(I_MSTime());
	MSG_WriteMarker(&b, svc_pingrequest);
	MSG_WriteProto(&b, msg);
}

/**
 * @brief Sends a message to a player telling them to change to the specified
 *        WAD and DEH patch files and load a map.
 */
void SVC_LoadMap(buf_t& b, const OResFiles& wadnames, const OResFiles& patchnames,
                 const std::string& mapname, int time)
{
	svc::LoadMapMsg msg;

	// send list of wads (skip over wadnames[0] == odamex.wad)
	size_t wadcount = wadnames.size() - 1;
	for (size_t i = 1; i < wadcount + 1; i++)
	{
		svc::LoadMapMsg_Resource* wad = msg.add_wadnames();
		wad->set_name(wadnames[i].getBasename());
		wad->set_hash(wadnames[i].getHash());
	}

	// send list of DEH/BEX patches
	size_t patchcount = patchnames.size();
	for (size_t i = 0; i < patchcount; i++)
	{
		svc::LoadMapMsg_Resource* patch = msg.add_patchnames();
		patch->set_name(patchnames[i].getBasename());
		patch->set_hash(patchnames[i].getHash());
	}

	msg.set_mapname(mapname);
	msg.set_time(time);

	MSG_WriteMarker(&b, svc_loadmap);
	MSG_WriteProto(&b, msg);
}

/**
 * @brief Kill a mobj.
 */
void SVC_KillMobj(buf_t& b, AActor* source, AActor* target, AActor* inflictor, int mod,
                  bool joinkill)
{
	svc::KillMobjMsg msg;

	if (source)
	{
		msg.set_source_netid(source->netid);
	}
	else
	{
		msg.set_source_netid(0);
	}

	msg.set_target_netid(target->netid);
	msg.set_inflictor_netid(inflictor ? inflictor->netid : 0);
	msg.set_health(target->health);
	msg.set_mod(mod);
	msg.set_joinkill(joinkill);

	// [AM] Confusingly, we send the lives _before_ we take it away, so
	//      the lives logic can live in the kill function.
	if (target->player)
	{
		msg.set_lives(target->player->lives);
	}
	else
	{
		msg.set_lives(-1);
	}

	MSG_WriteMarker(&b, svc_killmobj);
	MSG_WriteProto(&b, msg);
}

/**
 * @brief Persist player information to the client that is less vital.
 *
 * @param b Buffer to write to.
 * @param player Player to send information about.
 * @param flags SVC_PM_* flags to designate what gets sent.
 */
void SVC_PlayerMembers(buf_t& b, player_t& player, byte flags)
{
	svc::PlayerMembersMsg msg;

	msg.set_pid(player.id);
	msg.set_flags(flags);

	if (flags & SVC_PM_SPECTATOR)
	{
		msg.set_spectator(player.spectator);
	}

	if (flags & SVC_PM_READY)
	{
		msg.set_ready(player.ready);
	}

	if (flags & SVC_PM_LIVES)
	{
		msg.set_lives(player.lives);
	}

	if (flags & SVC_PM_SCORE)
	{
		// [AM] Just send everything instead of trying to be clever about
		//      what we send based on game modes.  There's less room for
		//      breakage when adding game modes, and we have varints now,
		//      which means we're probably saving bandwidth anyway.
		msg.set_roundwins(player.roundwins);
		msg.set_points(player.points);
		msg.set_fragcount(player.fragcount);
		msg.set_deathcount(player.deathcount);
		msg.set_killcount(player.killcount);
		// [AM] Is there any reason we would ever care about itemcount?
		msg.set_secretcount(player.secretcount);
		msg.set_totalpoints(player.totalpoints);
		msg.set_totaldeaths(player.totaldeaths);
	}

	MSG_WriteMarker(&b, svc_playermembers);
	MSG_WriteProto(&b, msg);
}

/**
 * Persist mutable team information to the client.
 */
void SVC_TeamMembers(buf_t& b, team_t team)
{
	svc::TeamMembersMsg msg;

	TeamInfo* info = GetTeamInfo(team);

	msg.set_team(team);
	msg.set_points(info->Points);
	msg.set_roundwins(info->RoundWins);

	MSG_WriteMarker(&b, svc_teammembers);
	MSG_WriteProto(&b, msg);
}

/**
 * @brief Send information about a player
 */
void SVC_PlayerState(buf_t& b, player_t& player)
{
	MSG_WriteMarker(&b, svc_playerstate);

	MSG_WriteByte(&b, player.id);
	MSG_WriteVarint(&b, player.health);
	MSG_WriteVarint(&b, player.armortype);
	MSG_WriteVarint(&b, player.armorpoints);
	MSG_WriteVarint(&b, player.lives);
	MSG_WriteVarint(&b, player.readyweapon);

	std::bitset<6> cardBits;
	for (int i = 0; i < NUMCARDS; i++)
		cardBits.set(i, player.cards[i]);

	MSG_WriteByte(&b, cardBits.to_ulong());

	for (int i = 0; i < NUMAMMO; i++)
		MSG_WriteVarint(&b, player.ammo[i]);

	for (int i = 0; i < NUMPSPRITES; i++)
	{
		pspdef_t* psp = &player.psprites[i];
		unsigned int state = psp->state - states;
		MSG_WriteUnVarint(&b, state);
	}

	for (int i = 0; i < NUMPOWERS; i++)
		MSG_WriteVarint(&b, player.powers[i]);
}

/**
 * @brief Send information about the server's LevelState to the client.
 */
void SVC_LevelState(buf_t& b, const SerializedLevelState& sls)
{
	MSG_WriteMarker(&b, svc_levelstate);
	MSG_WriteVarint(&b, sls.state);
	MSG_WriteVarint(&b, sls.countdown_done_time);
	MSG_WriteVarint(&b, sls.ingame_start_time);
	MSG_WriteVarint(&b, sls.round_number);
	MSG_WriteVarint(&b, sls.last_wininfo_type);
	MSG_WriteVarint(&b, sls.last_wininfo_id);
}

/**
 * @brief Send information about a player who discovered a secret.
 */
void SVC_SecretFound(buf_t& b, int playerid, int sectornum)
{
	sector_t* sector = &sectors[sectornum];

	// Only update secret sectors that've been discovered.
	if (sector != NULL && (!(sector->special & SECRET_MASK) && sector->secretsector))
	{
		MSG_WriteMarker(&b, svc_secretevent);
		MSG_WriteByte(&b, playerid); // ID of player who discovered it
		MSG_WriteShort(&b, sectornum);
		MSG_WriteShort(&b, sector->special);
	}
}

VERSION_CONTROL(svc_message, "$Id$")
