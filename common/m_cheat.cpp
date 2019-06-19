// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 2006-2015 by The Odamex Team.
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
//	Cheat code checking.
//
//-----------------------------------------------------------------------------


#include <stdlib.h>
#include <math.h>

#include "c_dispatch.h"
#include "d_items.h"
#include "d_player.h"
#include "doomstat.h"
#include "gstrings.h"
#include "m_cheat.h"
#include "p_inter.h"
#include "p_local.h"

#ifdef CLIENT_APP
void C_DoCommand(const char *cmd);

extern bool	automapactive;
extern buf_t     net_buffer;
#elif SERVER_APP
#include "../server/src/sv_main.h"
#endif
extern bool simulated_connection;

EXTERN_CVAR(sv_allowcheats)

CheatManager cht;

//
// CHEAT SEQUENCE PACKAGE
//
extern void A_PainDie(AActor *);

// Checks if all the conditions to enable cheats are set.
bool CheatManager::AreCheatsEnabled(void)
{
	// [SL] 2012-04-04 - Don't allow cheat codes to be entered while playing
	// back a netdemo
	if (simulated_connection)
		return false;

	// [Russell] - Allow vanilla style "no message" in singleplayer when cheats
	// are disabled
	if (sv_skill == sk_nightmare && !multiplayer)
		return false;

	if ((multiplayer || sv_gametype != GM_COOP) && !sv_allowcheats)
	{
		Printf(PRINT_HIGH, "You must run the server with '+set sv_allowcheats 1' to enable this command.\n");
		return false;
	}

	return true;
}

//
// Key handler for cheats
//
bool CheatManager::AddKey(cheatseq_t *cheat, unsigned char key, bool *eat)
{
	if (cheat->Pos == NULL)
	{
		cheat->Pos = cheat->Sequence;
		cheat->CurrentArg = 0;
	}
	if (*cheat->Pos == 0)
	{
		*eat = true;
		cheat->Args[cheat->CurrentArg++] = key;
		cheat->Pos++;
	}
	else if (key == *cheat->Pos)
	{
		cheat->Pos++;
	}
	else
	{
		cheat->Pos = cheat->Sequence;
		cheat->CurrentArg = 0;
	}
	if (*cheat->Pos == 0xff)
	{
		cheat->Pos = cheat->Sequence;
		cheat->CurrentArg = 0;
		return true;
	}
	return false;
}

// Sets a cheat given to the player.
void CheatManager::DoCheat (player_s *player, ECheatFlags cheat, bool silent)
{
	const char *msg = "";
	char msgbuild[64];
	bool bGlobalMSG = false;

	switch (cheat) {
		case CHT_IDDQD:
			if (player->health <= 0 || !player || player->spectator)
				return;

			if (!(player->cheats & CF_GODMODE)) {
				if (player->mo)
					player->mo->health = deh.GodHealth;

				player->health = deh.GodHealth;
			}
		case CHT_GOD:
			if (player->health <= 0 || !player || player->spectator)
				return;

			player->cheats ^= CF_GODMODE;
			msg = (player->cheats & CF_GODMODE) ? GStrings(STSTR_DQDON) : GStrings(STSTR_DQDOFF);
			break;

		case CHT_NOCLIP:
			
			if (player->health <= 0 || !player || player->spectator)
				return;

			player->cheats ^= CF_NOCLIP;
			msg = (player->cheats & CF_NOCLIP) ? GStrings(STSTR_NCON) : GStrings(STSTR_NCOFF);
			break;

		case CHT_FLY:
			if (player->health <= 0 || !player)
				return;

			if (player->spectator)
				silent = true;

			player->cheats ^= CF_FLY;
			if (player->cheats & CF_FLY)
				msg = "You feel lighter";
			else
				msg = "Gravity weighs you down";
			break;

		case CHT_NOTARGET:
			if (player->health <= 0 || !player || player->spectator)
				return;

				player->cheats ^= CF_NOTARGET;
				if (player->cheats & CF_NOTARGET)
					msg = "notarget ON";
				else
					msg = "notarget OFF";
			
			break;

		case CHT_CHASECAM:

			if (player->spectator)	
				silent = true;	// Unlike players, spectators are allowed to do it so...

			player->cheats ^= CF_CHASECAM;
			if (player->cheats & CF_CHASECAM)
				msg = "chasecam ON";
			else
				msg = "chasecam OFF";
			break;

		case CHT_CHAINSAW:
			if (player->spectator)
				return;

			player->weaponowned[wp_chainsaw] = true;
			player->powers[pw_invulnerability] = true;
			msg = GStrings(STSTR_CHOPPERS);
			break;

		case CHT_IDKFA:
			if (player->spectator)
				return;

			GiveTo (player, "all");
			player->armorpoints = deh.KFAArmor;
			player->armortype = deh.KFAAC;
			msg = GStrings(STSTR_KFAADDED);
			break;

		case CHT_IDFA:
			if (player->spectator)
				return;

			GiveTo (player, "backpack");
			GiveTo (player, "weapons");
			GiveTo (player, "ammo");
			player->armorpoints = deh.FAArmor;
			player->armortype = deh.FAAC;
			msg = GStrings(STSTR_FAADDED);
			break;

		case CHT_BEHOLDV:
		case CHT_BEHOLDS:
		case CHT_BEHOLDI:
		case CHT_BEHOLDR:
		case CHT_BEHOLDA:
		case CHT_BEHOLDL:
			{
				if (player->spectator)
					return;

				int i = cheat - CHT_BEHOLDV;

				if (!player->powers[i])
					P_GivePower (player, i);
				else if (i!=pw_strength)
					player->powers[i] = 1;
				else
					player->powers[i] = 0;
			}
			msg = GStrings(STSTR_BEHOLDX);
			break;

		case CHT_MASSACRE:
			{
				// jff 02/01/98 'em' cheat - kill all monsters
				// partially taken from Chi's .46 port
				//
				// killough 2/7/98: cleaned up code and changed to use dprintf;
				// fixed lost soul bug (LSs left behind when PEs are killed)

				// This cheat should only be usable by admins.
				if (multiplayer && !player->client.allow_rcon)	
					return;

				int killcount = 0;
				AActor *actor;
				TThinkerIterator<AActor> iterator;

				while ( (actor = iterator.Next ()) )
				{
					if (actor->flags & MF_COUNTKILL || actor->type == MT_SKULL)
					{
						// killough 3/6/98: kill even if PE is dead
						if (actor->health > 0)
						{
							killcount++;
							P_DamageMobj (actor, NULL, NULL, 10000, MOD_UNKNOWN);
						}
						if (actor->type == MT_PAIN)
						{
							A_PainDie (actor);    // killough 2/8/98
							P_SetMobjState (actor, S_PAIN_DIE6);
						}
					}
				}
				// killough 3/22/98: make more intelligent about plural
				// Ty 03/27/98 - string(s) *not* externalized
				sprintf (msgbuild, "NUCLEAR MASSACRE (%d Monster%s Killed)", killcount, killcount==1 ? "" : "s");
				msg = msgbuild;

				bGlobalMSG = true;	// Notify everyone, user included
			}
			break;
	}

#ifdef CLIENT_APP
		Printf (PRINT_HIGH, "%s\n", msg);
#else
	if (!silent) {
		if (bGlobalMSG) {
			SV_BroadcastPrintf(PRINT_HIGH, "%s is a cheater: %s\n", player->userinfo.netname.c_str(), msg);
		} else {
			SV_BroadcastButPlayerPrintf(PRINT_HIGH, player->id, "%s is a cheater: %s\n", player->userinfo.netname.c_str(), msg);
		}
	}
#endif
}

// ! UNUSED !
// Gives an item to a player.
// Can be one hardcoded value (all/ammo/backpack/keys/...), or any itemname (weapon_shotgun/ammo_cells/...)
// ToDo: Could we actually make that for ACS scripting, INSIDE the player_s class ?
void CheatManager::GiveTo (player_s *player, const char *item)
{
	bool giveall;
	int i;
	gitem_t *it;

	if (player != &consoleplayer())
		Printf (PRINT_HIGH, "%s is a cheater: give %s\n", player->userinfo.netname.c_str(), item);

	if (stricmp (item, "all") == 0)
		giveall = true;
	else
		giveall = false;

	if (giveall || strnicmp (item, "health", 6) == 0) {
		int h;

		if (0 < (h = atoi (item + 6))) {
			if (player->mo) {
				player->mo->health += h;
	  			player->health = player->mo->health;
			} else {
				player->health += h;
			}
		} else {
			if (player->mo)
				player->mo->health = deh.GodHealth;
	  
			player->health = deh.GodHealth;
		}

		if (!giveall)
			return;
	}

	if (giveall || stricmp (item, "backpack") == 0) {
		if (!player->backpack) {
			for (i=0 ; i<NUMAMMO ; i++)
			player->maxammo[i] *= 2;
			player->backpack = true;
		}
		for (i=0 ; i<NUMAMMO ; i++)
			P_GiveAmmo (player, (ammotype_t)i, 1);

		if (!giveall)
			return;
	}

	if (giveall || stricmp (item, "weapons") == 0) {
		weapontype_t pendweap = player->pendingweapon;
		for (i = 0; i<NUMWEAPONS; i++)
			P_GiveWeapon (player, (weapontype_t)i, false);
		player->pendingweapon = pendweap;

		if (!giveall)
			return;
	}

	if (giveall || stricmp (item, "ammo") == 0) {
		for (i=0;i<NUMAMMO;i++)
			player->ammo[i] = player->maxammo[i];

		if (!giveall)
			return;
	}

	if (giveall || stricmp (item, "armor") == 0) {
		player->armorpoints = 200;
		player->armortype = 2;

		if (!giveall)
			return;
	}

	if (giveall || stricmp (item, "keys") == 0) {
		for (i=0;i<NUMCARDS;i++)
			player->cards[i] = true;

		if (!giveall)
			return;
	}

	if (giveall)
		return;

	it = FindItem (item);
	if (!it) {
		it = FindItemByClassname (item);
		if (!it) {
			if (player == &consoleplayer())
				Printf (PRINT_HIGH, "Unknown item\n");
			return;
		}
	}

	if (it->flags & IT_AMMO) {
		int howmuch;

	/*	if (argc == 3)
			howmuch = atoi (argv[2]);
		else */
			howmuch = it->quantity;

		P_GiveAmmo (player, (ammotype_t)it->offset, howmuch);
	} else if (it->flags & IT_WEAPON) {
		P_GiveWeapon (player, (weapontype_t)it->offset, 0);
	} else if (it->flags & IT_KEY) {
		P_GiveCard (player, (card_t)it->offset);
	} else if (it->flags & IT_POWERUP) {
		P_GivePower (player, it->offset);
	} else if (it->flags & IT_ARMOR) {
		P_GiveArmor (player, it->offset);
	}
}

// ! UNUSED !
// This "cheat" (probably ported from Heretic/Hexen) immediately kills the player if trying to use it.
void CheatManager::SuicidePlayer (player_s *player)
{
	player->mo->flags |= MF_SHOOTABLE;
	while (player->health > 0)
		P_DamageMobj (player->mo, player->mo, player->mo, 10000, MOD_SUICIDE);
	player->mo->flags &= ~MF_SHOOTABLE;
}

#ifdef CLIENT_APP
// Sends the cheat to the server (to keep everyone in sync)
void CheatManager::SendCheatToServer(int cheats)
{
	MSG_WriteMarker(&net_buffer, clc_cheat);
	MSG_WriteByte(&net_buffer, cheats);
}

// Sends the cheat to the server (to keep everyone in sync)
void CheatManager::SendGiveCheatToServer(const char *item)
{
	MSG_WriteMarker(&net_buffer, clc_cheatgive);
	MSG_WriteString(&net_buffer, item);
}

//-------------
// THESE ARE MAINLY FOR THE CLIENT
// Smashing Pumpkins Into Small Piles Of Putrid Debris. 
bool CHEAT_AutoMap(cheatseq_t *cheat)
{

	if (automapactive)
	{
		if (!multiplayer || sv_gametype == GM_COOP)
			cht.AutoMapCheat = (cht.AutoMapCheat + 1) % 3;
		return true;
	}
	return false;

}

bool CHEAT_ChangeLevel(cheatseq_t *cheat)
{
	char buf[16];

	// [ML] Chex mode: always set the episode number to 1.
	// FIXME: This is probably a horrible hack, it sure looks like one at least
	if (gamemode == retail_chex)
		sprintf(buf, "1%c", cheat->Args[1]);

	snprintf(buf, sizeof(buf), "map %c%c\n", cheat->Args[0], cheat->Args[1]);
	AddCommandString(buf);

	return true;
}

bool CHEAT_IdMyPos(cheatseq_t *cheat)
{
	C_DoCommand("toggle idmypos");

	return true;
}

bool CHEAT_BeholdMenu(cheatseq_t *cheat)
{
	Printf(PRINT_HIGH, "%s\n", GStrings(STSTR_BEHOLD));

	return false;
}

bool CHEAT_ChangeMusic(cheatseq_t *cheat)
{

	char buf[9] = "idmus xx";

	buf[6] = cheat->Args[0];
	buf[7] = cheat->Args[1];
	C_DoCommand(buf);

	return true;
}

//
// Sets clientside the new cheat flag
// and also requests its new status serverside
//
bool CHEAT_SetGeneric(cheatseq_t *cheat)
{
	if (!cht.AreCheatsEnabled())
		return true;

	if (multiplayer)
		return true;

	if (cheat->Args[0] == CHT_NOCLIP)
	{
		if (cheat->Args[1] == 0 && gamemode != shareware && gamemode != registered &&
			gamemode != retail && gamemode != retail_bfg)
			return true;
		else if (cheat->Args[1] == 1 && gamemode != commercial && gamemode != commercial_bfg)
			return true;
	}

	cht.DoCheat(&consoleplayer(), (ECheatFlags)cheat->Args[0]);

	return true;
}

#endif

VERSION_CONTROL (m_cheat_cpp, "$Id$")

