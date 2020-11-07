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
//  Handling interactions (i.e., collisions).
//
//-----------------------------------------------------------------------------

// Data.
#include "doomdef.h"
#include "gstrings.h"
#include "doomstat.h"
#include "m_random.h"
#include "i_system.h"
#include "c_console.h"
#include "p_local.h"
#include "s_sound.h"
#include "p_inter.h"
#include "p_lnspec.h"
#include "p_ctf.h"
#include "p_acs.h"
#include "g_warmup.h"
#include "m_wdlstats.h"

extern bool predicting;

EXTERN_CVAR(sv_doubleammo)
EXTERN_CVAR(sv_weaponstay)
EXTERN_CVAR(sv_weapondamage)
EXTERN_CVAR(sv_monsterdamage)
EXTERN_CVAR(sv_fraglimit)
EXTERN_CVAR(sv_fragexitswitch) // [ML] 04/4/06: Added compromise for older exit method
EXTERN_CVAR(sv_friendlyfire)
EXTERN_CVAR(sv_allowexit)
EXTERN_CVAR(sv_forcerespawn)
EXTERN_CVAR(sv_forcerespawntime)
EXTERN_CVAR(co_zdoomphys)
EXTERN_CVAR(cl_predictpickup)
EXTERN_CVAR(co_zdoomsound)
EXTERN_CVAR(co_globalsound)

int shotclock = 0;
int MeansOfDeath;

// a weapon is found with two clip loads,
// a big item has five clip loads
int maxammo[NUMAMMO] = {200, 50, 300, 50};
int clipammo[NUMAMMO] = {10, 4, 20, 1};

void AM_Stop(void);
void SV_SpawnMobj(AActor *mobj);
void STACK_ARGS SV_BroadcastPrintf(int level, const char *fmt, ...);
void ClientObituary(AActor *self, AActor *inflictor, AActor *attacker);
void SV_UpdateFrags(player_t &player);
void SV_CTFEvent(flag_t f, flag_score_t event, player_t &who);
void SV_TouchSpecial(AActor *special, player_t *player);
ItemEquipVal SV_FlagTouch(player_t &player, flag_t f, bool firstgrab);
void SV_SocketTouch(player_t &player, flag_t f);
void SV_SendKillMobj(AActor *source, AActor *target, AActor *inflictor, bool joinkill);
void SV_SendDamagePlayer(player_t *player, int pain);
void SV_SendDamageMobj(AActor *target, int pain);
void SV_ActorTarget(AActor *actor);
void PickupMessage(AActor *toucher, const char *message);
void WeaponPickupMessage(AActor *toucher, weapontype_t &Weapon);

#ifdef SERVER_APP
void SV_ShareKeys(card_t card, player_t& player);
#endif

//
// GET STUFF
//

// Give frags to a player
void P_GiveFrags(player_t* player, int num)
{
	if (!warmup.checkscorechange())
		return;
	player->fragcount += num;
}

// Give coop kills to a player
void P_GiveKills(player_t* player, int num)
{
	if (!warmup.checkscorechange())
		return;
	player->killcount += num;
}

// Give coop kills to a player
void P_GiveDeaths(player_t* player, int num)
{
	if (!warmup.checkscorechange())
		return;
	player->deathcount += num;
}

// Give a specific number of points to a player's team
void P_GiveTeamPoints(player_t* player, int num)
{
	if (!warmup.checkscorechange())
		return;
	TEAMpoints[player->userinfo.team] += num;
}

//
// P_GiveAmmo
// Num is the number of clip loads,
// not the individual count (0= 1/2 clip).
// Returns false if the ammo can't be picked up at all
//

ItemEquipVal P_GiveAmmo(player_t *player, ammotype_t ammotype, int num)
{
	int oldammotype;

	if (ammotype == am_noammo)
    {
		return IEV_NotEquipped;
    }

	if (ammotype < 0 || ammotype > NUMAMMO)
    {
		I_Error("P_GiveAmmo: bad type %i", ammotype);
    }

	if (player->ammo[ammotype] == player->maxammo[ammotype])
    {
		return IEV_NotEquipped;
    }

	if (num)
    {
		num *= clipammo[ammotype];
    }
	else
    {
		num = clipammo[ammotype]/2;
    }

	if (sv_skill == sk_baby || sv_skill == sk_nightmare || sv_doubleammo)
	{
		// give double ammo in trainer mode,
		// you'll need in nightmare
		num <<= 1;
	}

	oldammotype = player->ammo[ammotype];
	player->ammo[ammotype] += num;

	if (player->ammo[ammotype] > player->maxammo[ammotype])
    {
		player->ammo[ammotype] = player->maxammo[ammotype];
    }

	// If non zero ammo,
	// don't change up weapons,
	// player was lower on purpose.
	if (oldammotype)
    {
		return IEV_EquipRemove;
    }

	// We were down to zero,
	// so select a new weapon.
	// Preferences are not user selectable.
	switch (ammotype)
	{
        case am_clip:
            if (player->readyweapon == wp_fist)
            {
                if (player->weaponowned[wp_chaingun])
                {
                    player->pendingweapon = wp_chaingun;
                }
                else
                {
                    player->pendingweapon = wp_pistol;
                }
            }
            break;

	    case am_shell:
            if (player->readyweapon == wp_fist ||
                player->readyweapon == wp_pistol)
            {
                if (player->weaponowned[wp_shotgun])
                {
                    player->pendingweapon = wp_shotgun;
                }
            }
            break;

	    case am_cell:
            if (player->readyweapon == wp_fist
                || player->readyweapon == wp_pistol)
            {
                if (player->weaponowned[wp_plasma])
                {
                    player->pendingweapon = wp_plasma;
                }
            }
            break;

	    case am_misl:
            if (player->readyweapon == wp_fist)
            {
                if (player->weaponowned[wp_missile])
                {
                    player->pendingweapon = wp_missile;
                }
            }
	    default:
            break;
	}

	return IEV_EquipRemove;
}

//
// P_GiveWeapon
// The weapon name may have a MF_DROPPED flag ored in.
//
bool P_CheckSwitchWeapon(player_t *player, weapontype_t weapon);
//
// P_GiveWeapon
// The weapon name may have a MF_DROPPED flag ored in.
//
ItemEquipVal P_GiveWeapon(player_t *player, weapontype_t weapon, BOOL dropped)
{
	bool gaveammo;
	bool gaveweapon;

	// [RH] Don't get the weapon if no graphics for it
	state_t *state = states + weaponinfo[weapon].readystate;
	if ((state->frame & FF_FRAMEMASK) >= sprites[state->sprite].numframes)
	{
		return IEV_NotEquipped;
	}

	// [Toke - dmflags] old location of DF_WEAPONS_STAY
	if (multiplayer && sv_weaponstay && !dropped)
	{
		// leave placed weapons forever on net games
		if (player->weaponowned[weapon])
		{
			return IEV_NotEquipped;
		}

		player->bonuscount = BONUSADD;
		player->weaponowned[weapon] = true;

		if (sv_gametype != GM_COOP)
		{
			P_GiveAmmo(player, weaponinfo[weapon].ammotype, 5);
		}
		else
		{
			P_GiveAmmo(player, weaponinfo[weapon].ammotype, 2);
		}

		if (P_CheckSwitchWeapon(player, weapon))
			player->pendingweapon = weapon;

		WeaponPickupMessage(player->mo, weapon);

		return IEV_EquipStay;
	}

	if (weaponinfo[weapon].ammotype != am_noammo)
	{
		// give one clip with a dropped weapon,
		// two clips with a found weapon
		if (dropped)
		{
			gaveammo = ((P_GiveAmmo(player, weaponinfo[weapon].ammotype, 1)) != 0);
		}
		else
		{
			gaveammo = ((P_GiveAmmo(player, weaponinfo[weapon].ammotype, 2)) != 0);
		}
	}
	else
	{
		gaveammo = false;
	}

	if (player->weaponowned[weapon])
	{
		gaveweapon = false;
	}
	else
	{
		gaveweapon = true;
		player->weaponowned[weapon] = true;
		if (P_CheckSwitchWeapon(player, weapon))
			player->pendingweapon = weapon;
	}

	if (gaveweapon || gaveammo)
		return IEV_EquipRemove;

	return IEV_NotEquipped;
}

//
// P_GiveBody
// Returns false if the body isn't needed at all
//
ItemEquipVal P_GiveBody(player_t *player, int num)
{
	if (player->health >= MAXHEALTH)
	{
		return IEV_NotEquipped;
	}

	player->health += num;
	if (player->health > MAXHEALTH)
	{
		player->health = MAXHEALTH;
	}
	player->mo->health = player->health;

	return IEV_EquipRemove;
}

//
// P_GiveArmor
// Returns false if the armor is worse
// than the current armor.
//
ItemEquipVal P_GiveArmor(player_t *player, int armortype)
{
	int hits;

	hits = armortype * 100;
	if (player->armorpoints >= hits)
	{
		return IEV_NotEquipped;	// don't pick up
	}

	player->armortype = armortype;
	player->armorpoints = hits;

	return IEV_EquipRemove;
}

//
// P_GiveCard
//
ItemEquipVal P_GiveCard(player_t *player, card_t card)
{
	if (player->cards[card])
	{
		return IEV_NotEquipped;
	}

	player->bonuscount = BONUSADD;
	player->cards[card] = 1;

	if (multiplayer)
	{
#ifdef SERVER_APP
		// Register the key
		SV_ShareKeys(card, *player);	
#endif


		return IEV_EquipStay;
	}

	return IEV_EquipRemove;
}

//
// P_GivePower
//
ItemEquipVal P_GivePower(player_t *player, int /*powertype_t*/ power)
{
	if (power == pw_invulnerability)
	{
		player->powers[power] = INVULNTICS;
		return IEV_EquipRemove;
	}

	if (power == pw_invisibility)
	{
		player->powers[power] = INVISTICS;
		player->mo->flags |= MF_SHADOW;
		return IEV_EquipRemove;
	}

	if (power == pw_infrared)
	{
		player->powers[power] = INFRATICS;
		return IEV_EquipRemove;
	}

	if (power == pw_ironfeet)
	{
		player->powers[power] = IRONTICS;
		return IEV_EquipRemove;
	}

	if (power == pw_strength)
	{
		P_GiveBody(player, 100);
		player->powers[power] = 1;
		return IEV_EquipRemove;
	}

	if (player->powers[power])
	{
		return IEV_NotEquipped;	// already got it
	}

	player->powers[power] = 1;
	return IEV_EquipRemove;
}
static bool P_SpecialIsWeapon(AActor *special)
{
	if (!special)
		return false;

	return (special->type == MT_CHAINGUN ||
			special->type == MT_SHOTGUN  ||
			special->type == MT_SUPERSHOTGUN ||
			special->type == MT_MISC25 ||
			special->type == MT_MISC26 ||
			special->type == MT_MISC27 ||
			special->type == MT_MISC28);
}

void P_PickupSound(AActor *ent, int channel, const char *name)
{
	if (serverside && co_globalsound) //Send pickup sound to all other players
		UV_SoundAvoidPlayer(ent, channel, name, co_zdoomsound ? ATTN_NORM : ATTN_NONE);
	else if (clientside && ent == consoleplayer().mo) //Only play our own pickup sounds, the server will send other players pickup sounds if needed
		S_Sound(ent, channel, name, 1, ATTN_NONE);
}

void P_GiveSpecial(player_t *player, AActor *special)
{
	if (!player || !player->mo || !special)
		return;

	AActor *toucher = player->mo;
	int sound = 0;
	const OString* msg = NULL;
	bool firstgrab = false;
	ItemEquipVal val = IEV_EquipRemove;

	// Identify by sprite.
	switch (special->sprite)
	{
		// armor
	    case SPR_ARM1:
			val = P_GiveArmor(player, deh.GreenAC);
			msg = &GOTARMOR;
			if (val == IEV_EquipRemove)
				M_LogWDLEvent(WDL_EVENT_POWERPICKUP, player, NULL,
					WDL_PICKUP_GREENARMOR, 0, 0);
            break;

	    case SPR_ARM2:
			val = P_GiveArmor(player, deh.BlueAC);
			msg = &GOTMEGA;
			if (val == IEV_EquipRemove)
				M_LogWDLEvent(WDL_EVENT_POWERPICKUP, player, NULL,
					WDL_PICKUP_BLUEARMOR, 0, 0);
            break;

		// bonus items
	    case SPR_BON1:
            player->health++;				// can go over 100%
            if (player->health > deh.MaxSoulsphere)
            {
                player->health = deh.MaxSoulsphere;
            }
            player->mo->health = player->health;
			msg = &GOTHTHBONUS;
			M_LogWDLEvent(WDL_EVENT_POWERPICKUP, player, NULL,
				WDL_PICKUP_HEALTHBONUS, 0, 0);
            break;

	    case SPR_BON2:
            player->armorpoints++;			// can go over 100%
            if (player->armorpoints > deh.MaxArmor)
            {
                player->armorpoints = deh.MaxArmor;
            }
            if (!player->armortype)
            {
                player->armortype = deh.GreenAC;
            }
			msg = &GOTARMBONUS;
			M_LogWDLEvent(WDL_EVENT_POWERPICKUP, player, NULL,
				WDL_PICKUP_ARMORBONUS, 0, 0);
            break;

	    case SPR_SOUL:
            player->health += deh.SoulsphereHealth;
            if (player->health > deh.MaxSoulsphere)
            {
                player->health = deh.MaxSoulsphere;
            }
            player->mo->health = player->health;
			msg = &GOTSUPER;
            sound = 1;
			M_LogWDLEvent(WDL_EVENT_POWERPICKUP, player, NULL,
				WDL_PICKUP_SOULSPHERE, 0, 0);
            break;

	    case SPR_MEGA:
            player->health = deh.MegasphereHealth;
            player->mo->health = player->health;
            P_GiveArmor(player,deh.BlueAC);
			msg = &GOTMSPHERE;
            sound = 1;
			M_LogWDLEvent(WDL_EVENT_POWERPICKUP, player, NULL,
				WDL_PICKUP_MEGASPHERE, 0, 0);
            break;

		// cards
	    case SPR_BKEY:
			val = P_GiveCard(player, it_bluecard);
			msg = &GOTBLUECARD;
            sound = 3;
            break;

	    case SPR_YKEY:
            val = P_GiveCard(player, it_yellowcard);
			msg = &GOTYELWCARD;
            sound = 3;
            break;

	    case SPR_RKEY:
            val = P_GiveCard(player, it_redcard);
			msg = &GOTREDCARD;
            sound = 3;
            break;

	    case SPR_BSKU:
            val = P_GiveCard(player, it_blueskull);
			msg = &GOTBLUESKUL;
            sound = 3;
            break;

	    case SPR_YSKU:
            val = P_GiveCard(player, it_yellowskull);
			msg = &GOTYELWSKUL;
            sound = 3;
            break;

	    case SPR_RSKU:
            val = P_GiveCard(player, it_redskull);
			msg = &GOTREDSKUL;
            sound = 3;
            break;

		// medikits, heals
	    case SPR_STIM:
			val = P_GiveBody(player, 10);
			msg = &GOTSTIM;
			if (val == IEV_EquipRemove)
				M_LogWDLEvent(WDL_EVENT_POWERPICKUP, player, NULL,
					WDL_PICKUP_STIMPACK, 0, 0);
            break;

	    case SPR_MEDI:
            if (player->health < 25)
            {
				msg = &GOTMEDINEED;
            }
            else if (player->health < 100)
            {
                msg = &GOTMEDIKIT;
            }
			val = P_GiveBody(player, 25);
			if (val == IEV_EquipRemove)
				M_LogWDLEvent(WDL_EVENT_POWERPICKUP, player, NULL,
					WDL_PICKUP_MEDKIT, 0, 0);
            break;

		// power ups
	    case SPR_PINV:
            val = P_GivePower(player, pw_invulnerability);
			msg = &GOTINVUL;
            sound = 1;
            break;

	    case SPR_PSTR:
			val = P_GivePower(player, pw_strength);
			msg = &GOTBERSERK;
            if (player->readyweapon != wp_fist)
            {
                player->pendingweapon = wp_fist;
            }
            sound = 1;
			if (val == IEV_EquipRemove)
				M_LogWDLEvent(WDL_EVENT_POWERPICKUP, player, NULL,
					WDL_PICKUP_BERSERK, 0, 0);
            break;

	    case SPR_PINS:
            val = P_GivePower(player, pw_invisibility);
			msg = &GOTINVIS;
            sound = 1;
            break;

	    case SPR_SUIT:
            val = P_GivePower(player, pw_ironfeet);
			msg = &GOTSUIT;
            sound = 1;
            break;

	    case SPR_PMAP:
			val = P_GivePower(player, pw_allmap);
			msg = &GOTMAP;
            sound = 1;
            break;

	    case SPR_PVIS:
            val = P_GivePower(player, pw_infrared);
			msg = &GOTVISOR;
            sound = 1;
            break;

		// ammo
	    case SPR_CLIP:
            if (special->flags & MF_DROPPED)
            {
				val = P_GiveAmmo(player, am_clip, 0);
            }
            else
            {
				val = P_GiveAmmo(player, am_clip, 1);
            }
			msg = &GOTCLIP;
            break;

	    case SPR_AMMO:
			val = P_GiveAmmo(player, am_clip, 5);
			msg = &GOTCLIPBOX;
            break;

	    case SPR_ROCK:
            val = P_GiveAmmo(player, am_misl, 1);
			msg = &GOTROCKET;
            break;

	    case SPR_BROK:
            val = P_GiveAmmo(player, am_misl, 5);
			msg = &GOTROCKBOX;
            break;

	    case SPR_CELL:
            val = P_GiveAmmo(player, am_cell, 1);
			msg = &GOTCELL;
            break;

	    case SPR_CELP:
            val = P_GiveAmmo(player, am_cell, 5);
			msg = &GOTCELLBOX;
            break;

	    case SPR_SHEL:
            val = P_GiveAmmo(player, am_shell, 1);
			msg = &GOTSHELLS;
            break;

	    case SPR_SBOX:
			val = P_GiveAmmo(player, am_shell, 5);
			msg = &GOTSHELLBOX;
            break;

	    case SPR_BPAK:
            if (!player->backpack)
            {
                for (int i=0 ; i<NUMAMMO ; i++)
                {
                    player->maxammo[i] *= 2;
                }
                player->backpack = true;
            }
            for (int i=0 ; i<NUMAMMO ; i++)
            {
                P_GiveAmmo(player, (ammotype_t)i, 1);
            }
			msg = &GOTBACKPACK;
            break;

		// weapons
	    case SPR_BFUG:
            val = P_GiveWeapon(player, wp_bfg, special->flags & MF_DROPPED);
			msg = &GOTBFG9000;
            sound = 2;
            break;

	    case SPR_MGUN:
            val = P_GiveWeapon(player, wp_chaingun, special->flags & MF_DROPPED);
			msg = &GOTCHAINGUN;
            sound = 2;
            break;

	    case SPR_CSAW:
			val = P_GiveWeapon(player, wp_chainsaw, special->flags & MF_DROPPED);
			msg = &GOTCHAINSAW;
            sound = 2;
            break;

	    case SPR_LAUN:
            val = P_GiveWeapon(player, wp_missile, special->flags & MF_DROPPED);
			msg = &GOTLAUNCHER;
            sound = 2;
            break;

	    case SPR_PLAS:
			val = P_GiveWeapon(player, wp_plasma, special->flags & MF_DROPPED);
			msg = &GOTPLASMA;
            sound = 2;
            break;

	    case SPR_SHOT:
            val = P_GiveWeapon(player, wp_shotgun, special->flags & MF_DROPPED);
			msg = &GOTSHOTGUN;
            sound = 2;
            break;

	    case SPR_SGN2:
			val = P_GiveWeapon(player, wp_supershotgun, special->flags & MF_DROPPED);
			msg = &GOTSHOTGUN2;
            sound = 2;
            break;

	// [Toke - CTF - Core]
        case SPR_BFLG: // Player touches the blue flag at its base
            firstgrab = true;
			//Fall through to flag touch
        case SPR_BDWN: // Player touches the blue flag after it's been dropped
			val = SV_FlagTouch(*player, it_blueflag, firstgrab);
			sound = -1;
            break;

        case SPR_BSOK:
            SV_SocketTouch(*player, it_blueflag);
            return;

        case SPR_RFLG: // Player touches the red flag at its base
            firstgrab = true;
			//Fall through to flag touch
        case SPR_RDWN: // Player touches the red flag after its been dropped
			val = SV_FlagTouch(*player, it_redflag, firstgrab);
			sound = -1;
            break;

        case SPR_RSOK:
            SV_SocketTouch(*player, it_redflag);
            return;

        default:
            Printf(
                PRINT_HIGH,
                "P_SpecialThing: Unknown gettable thing %d: %s\n",
                special->sprite,
                special->info->name
            );
            return;
	}

	if (special->flags & MF_COUNTITEM)
	{
		player->itemcount++;
		if (serverside)
			level.found_items++;
	}

	if (val == IEV_NotEquipped)
		return;

	//the player equipped/picked up an item
	player->bonuscount = BONUSADD;
	SV_TouchSpecial(special, player);

	if (msg != NULL)
		PickupMessage(toucher, GStrings(*msg));

	if (val == IEV_EquipRemove)
		special->Destroy();

	AActor *ent = player->mo;
	switch (sound)
	{
	case 0:
	case 3:
		P_PickupSound(ent, CHAN_ITEM, "misc/i_pkup");
		break;
	case 1:
		P_PickupSound(ent, CHAN_ITEM, "misc/p_pkup");
		break;
	case 2:
		P_PickupSound(ent, CHAN_ITEM, "misc/w_pkup");
		break;
	}
}


//
// P_TouchSpecialThing
//
void P_TouchSpecialThing(AActor *special, AActor *toucher)
{
	if (!toucher || !toucher->player || toucher->player->spectator || !special ) // [Toke - fix99]
		return;

    if (predicting)
        return;

    // Dead thing touching.
    // Can happen with a sliding player corpse.
    if (toucher->health <= 0)
		return;

	// out of reach?
	fixed_t delta = special->z - toucher->z;
	fixed_t lowerbound = co_zdoomphys ? -32*FRACUNIT : -8*FRACUNIT;

	if (delta > toucher->height || delta < lowerbound)
		return;

	// Only allow clients to predict touching weapons, not health, armor, etc
	if (!serverside && (!cl_predictpickup || !P_SpecialIsWeapon(special)))
		return;

	P_GiveSpecial(toucher->player, special);
}


// [RH]
// SexMessage: Replace parts of strings with gender-specific pronouns
//
// The following expansions are performed:
//		%g -> he/she/it
//		%h -> him/her/it
//		%p -> his/her/its
//		%o -> other (victim)
//		%k -> killer
//
void SexMessage (const char *from, char *to, int gender, const char *victim, const char *killer)
{
	static const char *genderstuff[3][3] =
	{
		{ "he",  "him", "his" },
		{ "she", "her", "her" },
		{ "it",  "it",  "its" }
	};
	static const int gendershift[3][3] =
	{
		{ 2, 3, 3 },
		{ 3, 3, 3 },
		{ 2, 2, 3 }
	};
	const char *subst = NULL;

	do
	{
		if (*from != '%')
		{
			*to++ = *from;
		}
		else
		{
			int gendermsg = -1;

			switch (from[1])
			{
			case 'g':	gendermsg = 0;	break;
			case 'h':	gendermsg = 1;	break;
			case 'p':	gendermsg = 2;	break;
			case 'o':	subst = victim;	break;
			case 'k':	subst = killer;	break;
			}
			if (subst != NULL)
			{
				int len = strlen (subst);
				memcpy (to, subst, len);
				to += len;
				from++;
				subst = NULL;
			}
			else if (gendermsg < 0)
			{
				*to++ = '%';
			}
			else
			{
				strcpy (to, genderstuff[gender][gendermsg]);
				to += gendershift[gender][gendermsg];
				from++;
			}
		}
	} while (*from++);
}

//
// P_KillMobj
//
void P_KillMobj(AActor *source, AActor *target, AActor *inflictor, bool joinkill)
{
	SV_SendKillMobj(source, target, inflictor, joinkill);
	AActor *mo;
	player_t *splayer;
	player_t *tplayer;

	target->flags &= ~(MF_SHOOTABLE|MF_FLOAT|MF_SKULLFLY);

	// GhostlyDeath -- Joinkill is only set on players, so we should be safe!
	if (joinkill)
	{
		target->flags |= MF_SPECTATOR;
		target->flags2 |= MF2_FLY;
	}

	if (target->type != MT_SKULL)
    {
		target->flags &= ~MF_NOGRAVITY;
    }

	target->flags |= MF_CORPSE|MF_DROPOFF;
	target->height >>= 2;

	// [RH] If the thing has a special, execute and remove it
	//		Note that the thing that killed it is considered
	//		the activator of the script.
	if ((target->flags & MF_COUNTKILL) && target->special)
	{
		LineSpecials[target->special] (NULL, source, target->args[0],
									   target->args[1], target->args[2],
									   target->args[3], target->args[4]);
		target->special = 0;
	}
	// [RH] Also set the thing's tid to 0. [why?]
	target->tid = 0;

	if (serverside && target->flags & MF_COUNTKILL)
		level.killed_monsters++;

	if (source)
	{
		splayer = source->player;
	}
	else
	{
		splayer = 0;
	}

	tplayer = target->player;

	// [SL] 2011-06-26 - Set the player's attacker.  For some reason this
	// was not being set clientside
	if (tplayer)
	{
		tplayer->attacker = source ? source->ptr() : AActor::AActorPtr();
	}

	if (source && source->player)
	{
		// Don't count any frags at level start, because they're just telefrags
		// resulting from insufficient deathmatch starts, and it wouldn't be
		// fair to count them toward a player's score.
		if (target->player && level.time)
		{
			if (!joinkill && !shotclock)
			{
				if (target->player == source->player) // [RH] Cumulative frag count
				{
					P_GiveFrags(splayer, -1);
					// [Toke] Minus a team frag for suicide
					if (sv_gametype == GM_TEAMDM)
					{
						P_GiveTeamPoints(splayer, -1);
					}
				}
				// [Toke] Minus a team frag for killing teammate
				else if ((sv_gametype == GM_TEAMDM || sv_gametype == GM_CTF) &&
				         (splayer->userinfo.team == tplayer->userinfo.team))
				{
					// [Toke - Teamplay || deathz0r - updated]
					P_GiveFrags(splayer, -1);
					if (sv_gametype == GM_TEAMDM)
					{
						P_GiveTeamPoints(splayer, -1);
					}
					else if (sv_gametype == GM_CTF)
					{
						SV_CTFEvent((flag_t)0, SCORE_BETRAYAL, *splayer);
					}
				}
				else
				{
					P_GiveFrags(splayer, 1);
					// [Toke] Add a team frag
					if (sv_gametype == GM_TEAMDM)
					{
						P_GiveTeamPoints(splayer, 1);
					}
					else if (sv_gametype == GM_CTF)
					{
						if (tplayer->flags[(flag_t)splayer->userinfo.team])
						{
							SV_CTFEvent((flag_t)0, SCORE_CARRIERKILL, *splayer);
						}
						else
						{
							SV_CTFEvent((flag_t)0, SCORE_KILL, *splayer);
						}
					}
				}
			}
			SV_UpdateFrags(*splayer);
		}
		// [deathz0r] Stats for co-op scoreboard
		if (sv_gametype == GM_COOP &&
            ((target->flags & MF_COUNTKILL) || (target->type == MT_SKULL)))
		{
			P_GiveKills(splayer, 1);
			SV_UpdateFrags(*splayer);
		}
	}

	if (target->player)
	{
		// [Toke - CTF]
		if (sv_gametype == GM_CTF)
			CTF_CheckFlags(*target->player);

		if (!joinkill && !shotclock)
		{
			P_GiveDeaths(tplayer, 1);
		}

		// Death script execution, care of Skull Tag
		if (level.behavior != NULL)
		{
			level.behavior->StartTypedScripts (SCRIPT_Death, target);
		}

		// count environment kills against you
		if (!source && !joinkill && !shotclock)
		{
			// [RH] Cumulative frag count
			P_GiveFrags(tplayer, -1);
		}

		// [NightFang] - Added this, thought it would be cooler
		// [Fly] - No, it's not cooler
		// target->player->cheats = CF_CHASECAM;

		SV_UpdateFrags(*tplayer);

		target->flags &= ~MF_SOLID;
		target->player->playerstate = PST_DEAD;
		P_DropWeapon(target->player);

		tplayer->suicide_time = level.time;
		tplayer->death_time = level.time;

		if (target == consoleplayer().camera)
		{
			// don't die in auto map, switch view prior to dying
			AM_Stop();
		}
	}

	if (target->health > 0) // denis - when this function is used standalone
	{
		target->health = 0;
	}

    if (target->health < -target->info->spawnhealth
        && target->info->xdeathstate)
    {
        P_SetMobjState(target, target->info->xdeathstate);
    }
    else
    {
        P_SetMobjState(target, target->info->deathstate);
    }

	target->tics -= P_Random(target) & 3;

	if (target->tics < 1)
	{
		target->tics = 1;
	}

	// [RH] Death messages
	// Nes - Server now broadcasts obituaries.
	// [CG] Since this is a stub, no worries anymore.
	if (target->player && level.time && multiplayer &&
        !demoplayback && !joinkill)
	{
		ClientObituary(target, inflictor, source);
	}
	// Check sv_fraglimit.
	if (source && source->player && target->player && level.time)
	{
		// [Toke] Better sv_fraglimit
		if (sv_gametype == GM_DM && sv_fraglimit &&
            splayer->fragcount >= sv_fraglimit && !shotclock)
		{
            // [ML] 04/4/06: Added !sv_fragexitswitch
            SV_BroadcastPrintf(
                PRINT_HIGH,
                "Frag limit hit. Game won by %s!\n",
                splayer->userinfo.netname.c_str()
            );
            shotclock = TICRATE*2;
		}

		// [Toke] TeamDM sv_fraglimit
		if (sv_gametype == GM_TEAMDM && sv_fraglimit && !shotclock)
		{
			for (size_t i = 0; i < NUMFLAGS; i++)
			{
				if (TEAMpoints[i] >= sv_fraglimit)
				{
					SV_BroadcastPrintf(
                        PRINT_HIGH,
                        "Frag limit hit. %s team wins!\n",
                        team_names[i]
                    );
					shotclock = TICRATE * 2;
					break;
				}
			}
		}
	}

	if (gamemode == retail_chex)	// [ML] Chex Quest mode - monsters don't drop items
    {
		return;
    }

	// Drop stuff.
	// This determines the kind of object spawned
	// during the death frame of a thing.
	mobjtype_t item = (mobjtype_t)0;

	switch (target->type)
	{
        case MT_WOLFSS:
        case MT_POSSESSED:
            item = MT_CLIP;
            break;

        case MT_SHOTGUY:
            item = MT_SHOTGUN;
            break;

        case MT_CHAINGUY:
            item = MT_CHAINGUN;
            break;

        default:
            return;
	}

	if (!item)
    {
		return; //Happens if bot or player killed when using fists.
    }

	// denis - dropped things will spawn serverside
	if (serverside)
	{
		mo = new AActor(target->x, target->y, ONFLOORZ, item);
		mo->flags |= MF_DROPPED;	// special versions of items
		SV_SpawnMobj(mo);
	}
}

//
// P_DamageMobj
// Damages both enemies and players
// "inflictor" is the thing that caused the damage
//	creature or missile, can be NULL (slime, etc)
// "source" is the thing to target after taking damage
//	creature or NULL
// Source and inflictor are the same for melee attacks.
// Source can be NULL for slime, barrel explosions
// and other environmental stuff.
//
// [Toke] This is no longer needed client-side
void P_DamageMobj(AActor *target, AActor *inflictor, AActor *source, int damage, int mod, int flags)
{
    unsigned	ang;
	int 		saved = 0;
	player_t*   splayer = NULL; // shorthand for source->player
	player_t*   tplayer = NULL; // shorthand for target->player
	fixed_t 	thrust;

	if (!serverside)
    {
		return;
    }

    if (source)
        splayer = source->player;

    tplayer = target->player;

	if (!(target->flags & MF_SHOOTABLE))
    {
		return; // shouldn't happen...
    }

	// GhostlyDeath -- Spectators can't get hurt!
	if (tplayer && tplayer->spectator)
    {
		return;
    }

	if (target->health <= 0)
    {
		return;
    }

	MeansOfDeath = mod;
	bool targethasflag = (
		&idplayer(CTFdata[TEAM_BLUE].flagger) == tplayer ||
		&idplayer(CTFdata[TEAM_RED].flagger) == tplayer
	);

	if (target->flags & MF_SKULLFLY)
	{
		target->momx = target->momy = target->momz = 0;
	}

	if (tplayer && sv_skill == sk_baby)
    {
		damage >>= 1;	// take half damage in trainer mode
    }

	// [AM] Weapon and monster damage scaling.
	if (source && splayer && target)
		damage *= sv_weapondamage;
	else if (source && target && tplayer)
		damage *= sv_monsterdamage;

	// Some close combat weapons should not
	// inflict thrust and push the victim out of reach,
	// thus kick away unless using the chainsaw.
	if (inflictor && !(target->flags & MF_NOCLIP) &&
        (!source || !splayer || splayer->readyweapon != wp_chainsaw))
	{
		ang = P_PointToAngle(inflictor->x, inflictor->y, target->x, target->y);

		thrust = damage * (FRACUNIT >> 3) * 100 / target->info->mass;

		// make fall forwards sometimes
		if (damage < 40
			&& damage > target->health
			&& target->z - inflictor->z > 64 * FRACUNIT
			&& (P_Random() & 1))
		{
			ang += ANG180;
			thrust *= 4;
		}

		ang >>= ANGLETOFINESHIFT;
		target->momx += FixedMul(thrust, finecosine[ang]);
		target->momy += FixedMul(thrust, finesine[ang]);
	}

	// player specific
	if (tplayer)
	{
		// end of game hell hack
		if (sv_gametype == GM_COOP || sv_allowexit)
        {
            if ((target->subsector->sector->special & 255) == dDamage_End
                && damage >= target->health)
            {
                damage = target->health - 1;
            }
        }

		// Below certain threshold,
		// ignore damage in GOD mode, or with INVUL power.
		if (damage < 1000 &&
			((tplayer->cheats & CF_GODMODE) ||
             tplayer->powers[pw_invulnerability]))
		{
			return;
		}

		// [AM] No damage with sv_friendlyfire (was armor-only)
		if (!sv_friendlyfire && source && splayer && target != source &&
			 mod != MOD_TELEFRAG)
		{
			if (sv_gametype == GM_COOP ||
			  ((sv_gametype == GM_TEAMDM || sv_gametype == GM_CTF) &&
				tplayer->userinfo.team == splayer->userinfo.team))
			{
				damage = 0;
			}
		}

		if (tplayer->armortype && !(flags & DMG_NO_ARMOR))
		{
			if (tplayer->armortype == deh.GreenAC)
            {
				saved = damage / 3;
            }
			else
            {
				saved = damage / 2;
            }

			if (tplayer->armorpoints <= saved)
			{
				// armor is used up
				saved = tplayer->armorpoints;
				tplayer->armortype = 0;
			}
			tplayer->armorpoints -= saved;
			damage -= saved;
		}

		tplayer->health -= damage;		// mirror mobj health here for Dave

		if (tplayer->health <= 0)
        {
			tplayer->health = 0;
        }

		tplayer->attacker = source ? source->ptr() : AActor::AActorPtr();
		tplayer->damagecount += damage;	// add damage after armor / invuln

		if (tplayer->damagecount > 100)
        {
			tplayer->damagecount = 100;	// teleport stomp does 10k points...
        }
		SV_SendDamagePlayer(tplayer, target->health - damage);

		// WDL damage events - they have to be up here to ensure we know how
		// much armor is subtracted.
		int low = std::max(target->health - damage, 0);
		int actualdamage = target->health - low;

		if (source == NULL)
		{
			int emod = (mod >= MOD_FIST && mod <= MOD_SSHOTGUN) ? MOD_UNKNOWN : mod;
			M_LogActorWDLEvent(WDL_EVENT_ENVIRODAMAGE, source, target, actualdamage, saved, emod);
		}
		else if (targethasflag)
		{
			if (mod == MOD_PISTOL || mod == MOD_SHOTGUN || mod == MOD_SSHOTGUN || mod == MOD_CHAINGUN)
				M_LogActorWDLEvent(WDL_EVENT_ACCURACY, source, target, source->angle / 4,
					M_MODToWeapon(mod), 0);

			M_LogActorWDLEvent(WDL_EVENT_CARRIERDAMAGE, source, target, actualdamage, saved, mod);
		}
		else
		{
			if (mod == MOD_PISTOL || mod == MOD_SHOTGUN || mod == MOD_SSHOTGUN || mod == MOD_CHAINGUN)
				M_LogActorWDLEvent(WDL_EVENT_ACCURACY, source, target, source->angle / 4,
					M_MODToWeapon(mod), 0);

			M_LogActorWDLEvent(WDL_EVENT_DAMAGE, source, target, actualdamage, saved, mod);
		}
	}

	// do the damage
	// [RH] Only if not immune
	if (!(target->flags2 & (MF2_INVULNERABLE | MF2_DORMANT)))
	{
		target->health -= damage;
		if (target->health <= 0)
		{
			P_KillMobj(source, target, inflictor, false);

			// WDL damage events.
			if (source == NULL && targethasflag)
			{
				int emod = (mod >= MOD_FIST && mod <= MOD_SSHOTGUN) ? MOD_UNKNOWN : mod;
				M_LogActorWDLEvent(WDL_EVENT_ENVIROCARRIERKILL, source, target, 0, 0, emod);
			}
			else if (source == NULL)
			{
				int emod = (mod >= MOD_FIST && mod <= MOD_SSHOTGUN) ? MOD_UNKNOWN : mod;
				M_LogActorWDLEvent(WDL_EVENT_ENVIROKILL, source, target, 0, 0, emod);
			}
			else if (targethasflag)
				M_LogActorWDLEvent(WDL_EVENT_CARRIERKILL, source, target, 0, 0, mod);
			else
				M_LogActorWDLEvent(WDL_EVENT_KILL, source, target, 0, 0, mod);
			return;
		}
	}

    if (!(target->flags2 & MF2_DORMANT))
	{
		int pain = P_Random();

		if (!tplayer)
		{
			SV_SendDamageMobj(target, pain);
		}
		if (pain < target->info->painchance &&
		    !(target->flags & MF_SKULLFLY) &&
			!(tplayer && !damage))
		{
			target->flags |= MF_JUSTHIT;	// fight back!
			P_SetMobjState(target, target->info->painstate);
		}

		target->reactiontime = 0;			// we're awake now...

		if ((!target->threshold || target->type == MT_VILE)
			 && source && source != target
			 && source->type != MT_VILE)
		{
			// if not intent on another player, chase after this one

			// killough 2/15/98: remember last enemy, to prevent
			// sleeping early; 2/21/98: Place priority on players

			if (!target->lastenemy || !target->lastenemy->player ||
				target->lastenemy->health <= 0)
            {
				target->lastenemy = target->target; // remember last enemy - killough
            }

			target->target = source->ptr();
			target->threshold = BASETHRESHOLD;
			if (target->state == &states[target->info->spawnstate]
				&& target->info->seestate != S_NULL)
            {
				P_SetMobjState(target, target->info->seestate);
            }
            SV_ActorTarget(target);
		}
	}
}

//The player has left the game (in-game to spectator, or in-game disconnect)
void P_PlayerLeavesGame(player_s* player)
{
	if (level.behavior)
	{
		level.behavior->StartTypedScripts(SCRIPT_Disconnect, player->mo, player->GetPlayerNumber());
	}
}

VERSION_CONTROL (p_interaction_cpp, "$Id$")
