// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2012 by Alex Mayfield.
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

#include "msg_server.h"

#include "d_main.h"

/**
 * @brief Send information about a player.
 */
void SVC_PlayerInfo(buf_t& b, player_t& player)
{
	MSG_WriteMarker(&b, svc_playerinfo);

	// [AM] 9 weapons, 6 cards, 1 backpack = 16 bits
	uint16_t booleans = 0;
	for (int i = 0; i < NUMWEAPONS; i++)
	{
		if (player.weaponowned[i])
			booleans |= (1 << i);
	}
	for (int i = 0; i < NUMCARDS; i++)
	{
		if (player.cards[i])
			booleans |= (1 << (i + NUMWEAPONS));
	}
	if (player.backpack)
		booleans |= (1 << (NUMWEAPONS + NUMCARDS));
	MSG_WriteShort(&b, booleans);

	for (int i = 0; i < NUMAMMO; i++)
	{
		MSG_WriteVarint(&b, player.maxammo[i]);
		MSG_WriteVarint(&b, player.ammo[i]);
	}

	MSG_WriteVarint(&b, player.health);
	MSG_WriteVarint(&b, player.armorpoints);
	MSG_WriteVarint(&b, player.armortype);
	MSG_WriteVarint(&b, player.lives);

	// If the player has a pending weapon then tell the client it has changed
	// The client will set the pendingweapon to this weapon if it doesn't match the
	// readyweapon
	if (player.pendingweapon == wp_nochange)
		MSG_WriteVarint(&b, player.readyweapon);
	else
		MSG_WriteVarint(&b, player.pendingweapon);

	for (int i = 0; i < NUMPOWERS; i++)
		MSG_WriteVarint(&b, player.powers[i]);
}

/**
 * @brief Persist level locals to the client.
 *
 * @param b Buffer to write to.
 * @param locals Level locals struct to send.
 * @param flags SVC_LL_* bit flags to designate what gets sent.
 */
void SVC_LevelLocals(buf_t& b, const level_locals_t& locals, byte flags)
{
	MSG_WriteMarker(&b, svc_levellocals);
	MSG_WriteByte(&b, flags);

	if (flags & SVC_LL_TIME)
		MSG_WriteVarint(&b, locals.time);

	if (flags & SVC_LL_TOTALS)
	{
		MSG_WriteVarint(&b, locals.total_secrets);
		MSG_WriteVarint(&b, locals.total_items);
		MSG_WriteVarint(&b, locals.total_monsters);
	}

	if (flags & SVC_LL_SECRETS)
		MSG_WriteVarint(&b, locals.found_secrets);

	if (flags & SVC_LL_ITEMS)
		MSG_WriteVarint(&b, locals.found_items);

	if (flags & SVC_LL_MONSTERS)
		MSG_WriteVarint(&b, locals.killed_monsters);
}

/**
 * @brief Sends a message to a player telling them to change to the specified
 *        WAD and DEH patch files and load a map.
 */
void SVC_LoadMap(buf_t& b, const std::vector<std::string>& wadnames,
                 const std::vector<std::string>& wadhashes,
                 const std::vector<std::string>& patchnames,
                 const std::vector<std::string>& patchhashes, const std::string& mapname,
                 int time)
{
	MSG_WriteMarker(&b, svc_loadmap);

	// send list of wads (skip over wadnames[0] == odamex.wad)
	size_t wadcount = wadnames.size() - 1;
	MSG_WriteUnVarint(&b, wadcount);
	for (size_t i = 1; i < wadcount + 1; i++)
	{
		MSG_WriteString(&b, D_CleanseFileName(wadnames[i], "wad").c_str());
		MSG_WriteString(&b, wadhashes[i].c_str());
	}

	// send list of DEH/BEX patches
	size_t patchcount = patchnames.size();
	MSG_WriteUnVarint(&b, patchcount);
	for (size_t i = 0; i < patchcount; i++)
	{
		MSG_WriteString(&b, D_CleanseFileName(patchnames[i]).c_str());
		MSG_WriteString(&b, patchhashes[i].c_str());
	}

	MSG_WriteString(&b, mapname.c_str());
	MSG_WriteVarint(&b, time);
}

/**
 * @brief Kill a mobj.
 */
void SVC_KillMobj(buf_t& b, AActor* source, AActor* target, AActor* inflictor, int mod,
                  bool joinkill)
{
	MSG_WriteMarker(&b, svc_killmobj);

	if (source)
		MSG_WriteVarint(&b, source->netid);
	else
		MSG_WriteVarint(&b, 0);

	MSG_WriteVarint(&b, target->netid);
	MSG_WriteVarint(&b, inflictor ? inflictor->netid : 0);
	MSG_WriteVarint(&b, target->health);
	MSG_WriteVarint(&b, mod);
	MSG_WriteBool(&b, joinkill);

	// [AM] Confusingly, we send the lives _before_ we take it away, so
	//      the lives logic can live in the kill function.
	if (target->player)
		MSG_WriteVarint(&b, target->player->lives);
	else
		MSG_WriteVarint(&b, -1);
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
	MSG_WriteMarker(&b, svc_playermembers);
	MSG_WriteByte(&b, player.id);
	MSG_WriteByte(&b, flags);

	if (flags & SVC_PM_SPECTATOR)
		MSG_WriteBool(&b, player.spectator);

	if (flags & SVC_PM_READY)
		MSG_WriteBool(&b, player.ready);

	if (flags & SVC_PM_LIVES)
		MSG_WriteVarint(&b, player.lives);

	if (flags & SVC_PM_SCORE)
	{
		// [AM] Just send everything instead of trying to be clever about
		//      what we send based on game modes.  There's less room for
		//      breakage when adding game modes, and we have varints now,
		//      which means we're probably saving bandwidth anyway.
		MSG_WriteVarint(&b, player.roundwins);
		MSG_WriteVarint(&b, player.points);
		MSG_WriteVarint(&b, player.fragcount);
		MSG_WriteVarint(&b, player.deathcount);
		MSG_WriteVarint(&b, player.killcount);
		// [AM] Is there any reason we would ever care about itemcount?
		MSG_WriteVarint(&b, player.secretcount);
	}
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

	for (int i = 0; i < NUMAMMO; i++)
		MSG_WriteVarint(&b, player.ammo[i]);

	for (int i = 0; i < NUMPSPRITES; i++)
	{
		pspdef_t* psp = &player.psprites[i];
		if (psp->state)
			MSG_WriteByte(&b, psp->state - states);
		else
			MSG_WriteByte(&b, 0xFF);
	}
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

VERSION_CONTROL(msg_server, "$Id$")
