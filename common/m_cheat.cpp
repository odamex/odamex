// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 2006-2020 by The Odamex Team.
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


#include "odamex.h"

#include <stdlib.h>
#include <math.h>

#include "g_gametype.h"
#include "m_cheat.h"
#include "d_player.h"
#include "gstrings.h"
#include "p_inter.h"
#include "d_items.h"
#include "g_skill.h"
#include "p_local.h"

extern bool simulated_connection;
EXTERN_CVAR(sv_allowcheats)

#ifdef CLIENT_APP
#include "am_map.h"
#include "cl_main.h"
#include "c_dispatch.h"
extern bool automapactive;
#endif

void C_DoCommand(const char* cmd, uint32_t key = 0);

//
// CHEAT SEQUENCE PACKAGE
//

#ifdef CLIENT_APP

//-------------
// THESE ARE MAINLY FOR THE CLIENT
// Smashing Pumpkins Into Small Piles Of Putrid Debris.
bool CHEAT_AutoMap(cheatseq_t* cheat)
{
	if (automapactive)
	{
		if (!multiplayer || G_IsCoopGame())
			am_cheating = (am_cheating + 1) % 3;

		return true;
	}
	return false;
}

bool CHEAT_ChangeLevel(cheatseq_t* cheat)
{
	char buf[16];

	// What were you trying to achieve?
	if (multiplayer)
		return false;

	// [ML] Chex mode: always set the episode number to 1.
	// FIXME: This is probably a horrible hack, it sure looks like one at least
	if (gamemode == retail_chex)
		snprintf(buf, sizeof(buf), "map 1%c", cheat->Args[1]);
	else
		snprintf(buf, sizeof(buf), "map %c%c\n", cheat->Args[0], cheat->Args[1]);
	
	AddCommandString(buf);
	return true;
}

bool CHEAT_IdMyPos(cheatseq_t* cheat)
{
	C_DoCommand("toggle idmypos", 0);
	return true;
}

bool CHEAT_BeholdMenu(cheatseq_t* cheat)
{
	Printf(PRINT_HIGH, "%s\n", GStrings(STSTR_BEHOLD));
	return false;
}

bool CHEAT_ChangeMusic(cheatseq_t* cheat)
{
	char buf[9] = "idmus xx";

	buf[6] = cheat->Args[0];
	buf[7] = cheat->Args[1];
	C_DoCommand(buf, 0);
	return true;
}

//
// Sets clientside the new cheat flag
// and also requests its new status serverside
//
bool CHEAT_SetGeneric(cheatseq_t* cheat)
{
	if (!CHEAT_AreCheatsEnabled())
		return true;

	if (cheat->Args[0] == CHT_NOCLIP)
	{
		if (cheat->Args[1] == 0 && gamemode != shareware && gamemode != registered &&
		    gamemode != retail && gamemode != retail_bfg)
			return true;
		else if (cheat->Args[1] == 1 && gamemode != commercial &&
		         gamemode != commercial_bfg)
			return true;
	}

	CHEAT_DoCheat(&consoleplayer(), (ECheatFlags)cheat->Args[0]);
	CL_SendCheat((ECheatFlags)cheat->Args[0]);

	return true;
}

// [RH] Actually handle the cheat. The cheat code in st_stuff.c now just
// writes some bytes to the network data stream, and the network code
// later calls us.

bool CHEAT_AddKey(cheatseq_t* cheat, unsigned char key, bool* eat)
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

BEGIN_COMMAND(tntem)
{
	if (!CHEAT_AreCheatsEnabled())
		return;

	if (multiplayer && !G_IsCoopGame())
		return;

	CHEAT_DoCheat(&consoleplayer(), CHT_MASSACRE);
	CL_SendCheat(CHT_MASSACRE);
}
END_COMMAND(tntem)

BEGIN_COMMAND(mdk)
{
	if (!CHEAT_AreCheatsEnabled())
		return;

	if (multiplayer && !G_IsCoopGame())
		return;

	CHEAT_DoCheat(&consoleplayer(), CHT_MDK);
	CL_SendCheat(CHT_MDK);
}
END_COMMAND(mdk)

#endif

// Checks if all the conditions to enable cheats are met.
bool CHEAT_AreCheatsEnabled()
{
	// [SL] 2012-04-04 - Don't allow cheat codes to be entered while playing
	// back a netdemo
	if (simulated_connection)
		return false;

	// Disallow cheats within any state other than ingame.
	if (gamestate != GS_LEVEL)
		return false;

	// [Russell] - Allow vanilla style "no message" in singleplayer when cheats
	// are disabled
	if (!multiplayer && G_GetCurrentSkill().disable_cheats)
	{
		if (!sv_allowcheats)
		{
			Printf(PRINT_WARNING,
			       "You must 'set sv_allowcheats 1' in the console to enable "
			       "this command on this difficulty.\n");
			return false;
		}
	}

	if ((multiplayer || !G_IsCoopGame()) && !sv_allowcheats)
	{
		Printf(PRINT_WARNING, "You must run the server with '+set sv_allowcheats 1' to "
		                      "enable this command.\n");
		return false;
	}

	return true;
}

extern void A_PainDie(AActor*);

void CHEAT_DoCheat(player_t* player, int cheat, bool silentmsg)
{
	const char *msg = "";
	char msgbuild[32];

	if (player->health <= 0 || !player)
		return;

	switch (cheat) {
		case CHT_IDDQD:

			if (player->spectator)
			    return;

			if (!(player->cheats & CF_GODMODE)) {
				if (player->mo)
					player->mo->health = deh.GodHealth;

				player->health = deh.GodHealth;
			}
		case CHT_GOD:

			if (player->spectator)
			    return;

			player->cheats ^= CF_GODMODE;
		    msg = (player->cheats & CF_GODMODE) ? GStrings(STSTR_DQDON)
		                                        : GStrings(STSTR_DQDOFF);
			break;

		case CHT_NOCLIP:
			if (player->spectator)
			    silentmsg = true;

			player->cheats ^= CF_NOCLIP;
		    msg = (player->cheats & CF_NOCLIP) ? GStrings(STSTR_NCON)
		                                       : GStrings(STSTR_NCOFF);
			break;

		case CHT_FLY:
			player->cheats ^= CF_FLY;
		    msg = (player->cheats & CF_FLY) ? "You feel lighter"
		                                    : "Gravity weighs you down";
			break;

		case CHT_NOTARGET:

			if (player->spectator)
			    return;

			player->cheats ^= CF_NOTARGET;
		    msg = (player->cheats & CF_NOTARGET) ? "notarget ON" 
				                                 : "notarget OFF";
			break;

		case CHT_CHASECAM:

			if (player->spectator)
			    return;

			player->cheats ^= CF_CHASECAM;
			msg = (player->cheats & CF_CHASECAM) ? "chasecam ON" 
				                                 : "chasecam OFF";
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

		    CHEAT_GiveTo(player, "all");
			player->armorpoints = deh.KFAArmor;
			player->armortype = deh.KFAAC;
			msg = GStrings(STSTR_KFAADDED);
			break;

		case CHT_IDFA:

			if (player->spectator)
			    return;

		    CHEAT_GiveTo(player, "backpack");
		    CHEAT_GiveTo(player, "weapons");
		    CHEAT_GiveTo(player, "ammo");
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

				int killcount = 0;
				AActor *actor;
				TThinkerIterator<AActor> iterator;

				if (multiplayer && !player->client.allow_rcon)
			        return;

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
				snprintf (msgbuild, 32, "%d Monster%s Killed", killcount, killcount==1 ? "" : "s");
				msg = msgbuild;
			}
			break;

		case CHT_MDK: 
		{
			if (multiplayer && !player->client.allow_rcon)
				return;

			if (player->spectator)
			    return;

			// Never enable that in PvP, are you crazy?
			if (!G_IsCoopGame())
			    return;

			if (serverside)
		    {
			    P_LineAttack(
			        player->mo, player->mo->angle, 8192 * FRACUNIT,
			        P_AimLineAttack(player->mo, player->mo->angle, 8192 * FRACUNIT),
			        10000);

				if (multiplayer)
					msg = "MDK";
		    }
		}
	    break;

		case CHT_BUDDHA: 
		{
		        player->cheats ^= CF_BUDDHA;
		        msg = (player->cheats & CF_BUDDHA) ? GStrings(TXT_BUDDHAON)
		                                           : GStrings(TXT_BUDDHAOFF);
	    }
	}

	if (!silentmsg)
	{
		if (player == &consoleplayer())
		{
			if (msg != NULL)
				Printf("%s\n", msg);
		}
				
			
#ifdef SERVER_APP
			SV_BroadcastPrintfButPlayer(PRINT_HIGH, player->id, "%s is a cheater: %s\n",
			                            player->userinfo.netname.c_str(), msg);
			#endif
	}
}

void CHEAT_GiveTo(player_t* player, const char* name)
{
	BOOL giveall;
	int i;
	gitem_t *it;

	if (player != &consoleplayer())
		Printf (PRINT_HIGH, "%s is a cheater: give %s\n", player->userinfo.netname.c_str(), name);

	if (stricmp (name, "all") == 0)
		giveall = true;
	else
		giveall = false;

	if (giveall || strnicmp (name, "health", 6) == 0) {
		int h;

		if (0 < (h = atoi (name + 6))) {
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

	if (giveall || stricmp (name, "backpack") == 0) {
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

	if (giveall || stricmp (name, "weapons") == 0) {
		weapontype_t pendweap = player->pendingweapon;
		for (i = 0; i<NUMWEAPONS; i++)
			P_GiveWeapon (player, (weapontype_t)i, false);
		player->pendingweapon = pendweap;

		if (!giveall)
			return;
	}

	if (giveall || stricmp (name, "ammo") == 0) {
		for (i=0;i<NUMAMMO;i++)
			player->ammo[i] = player->maxammo[i];

		if (!giveall)
			return;
	}

	if (giveall || stricmp (name, "armor") == 0) {
		player->armorpoints = 200;
		player->armortype = 2;

		if (!giveall)
			return;
	}

	if (giveall || stricmp (name, "keys") == 0) {
		for (i=0;i<NUMCARDS;i++)
			player->cards[i] = true;

		if (!giveall)
			return;
	}

	if (giveall)
		return;

	it = FindItem (name);
	if (!it) {
		it = FindItemByClassname (name);
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

// Heretic cheat code (unused!)
#if 0
void CHEAT_Suicide(player_t* plyr)
{
	plyr->mo->flags |= MF_SHOOTABLE;
	while (plyr->health > 0)
		P_DamageMobj (plyr->mo, plyr->mo, plyr->mo, 10000, MOD_SUICIDE);
	plyr->mo->flags &= ~MF_SHOOTABLE;
}
#endif

VERSION_CONTROL (m_cheat_cpp, "$Id$")
