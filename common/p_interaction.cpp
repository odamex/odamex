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
#include "c_dispatch.h"
#include "p_local.h"
#include "s_sound.h"
#include "p_inter.h"
#include "p_lnspec.h"
#include "p_ctf.h"
#include "p_acs.h"
#include "g_warmup.h"

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
EXTERN_CVAR (cl_predictpickup)

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
bool SV_FlagTouch(player_t &player, flag_t f, bool firstgrab);
void SV_SocketTouch(player_t &player, flag_t f);
void SV_SendKillMobj(AActor *source, AActor *target, AActor *inflictor, bool joinkill);
void SV_SendDamagePlayer(player_t *player, int pain);
void SV_SendDamageMobj(AActor *target, int pain);
void SV_ActorTarget(AActor *actor);
void PickupMessage(AActor *toucher, const char *message);
void WeaponPickupMessage(AActor *toucher, weapontype_t &Weapon);

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

BOOL P_GiveAmmo(player_t *player, ammotype_t ammotype, int num)
{
	int oldammotype;

	if (ammotype == am_noammo)
    {
		return false;
    }

	if (ammotype < 0 || ammotype > NUMAMMO)
    {
		I_Error("P_GiveAmmo: bad type %i", ammotype);
    }

	if (player->ammo[ammotype] == player->maxammo[ammotype])
    {
		return false;
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
		return true;
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

	return true;
}

//
// P_GiveWeapon
// The weapon name may have a MF_DROPPED flag ored in.
//
bool P_CheckSwitchWeapon(player_t *player, weapontype_t weapon);

BOOL P_GiveWeapon(player_t *player, weapontype_t weapon, BOOL dropped)
{
	bool gaveammo;
	bool gaveweapon;

	// [RH] Don't get the weapon if no graphics for it
	state_t *state = states + weaponinfo[weapon].readystate;
	if ((state->frame & FF_FRAMEMASK) >= sprites[state->sprite].numframes)
    {
		return false;
    }

	// [Toke - dmflags] old location of DF_WEAPONS_STAY
	if (multiplayer && sv_weaponstay && !dropped)
	{
		// leave placed weapons forever on net games
		if (player->weaponowned[weapon])
        {
			return false;
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

		S_Sound(player->mo, CHAN_ITEM, "misc/w_pkup", 1, ATTN_NONE);

        WeaponPickupMessage(player->mo, weapon);

		return false;
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

	return (gaveweapon || gaveammo);
}

//
// P_GiveBody
// Returns false if the body isn't needed at all
//
BOOL P_GiveBody(player_t *player, int num)
{
	if (player->health >= MAXHEALTH)
    {
		return false;
    }

	player->health += num;
	if (player->health > MAXHEALTH)
    {
		player->health = MAXHEALTH;
    }
	player->mo->health = player->health;

	return true;
}

//
// P_GiveArmor
// Returns false if the armor is worse
// than the current armor.
//
BOOL P_GiveArmor(player_t *player, int armortype)
{
	int hits;

	hits = armortype * 100;
	if (player->armorpoints >= hits)
    {
		return false;	// don't pick up
    }

	player->armortype = armortype;
	player->armorpoints = hits;

	return true;
}

//
// P_GiveCard
//
void P_GiveCard(player_t *player, card_t card)
{
	if (player->cards[card])
    {
		return;
    }

	player->bonuscount = BONUSADD;
	player->cards[card] = 1;
}

//
// P_GivePower
//
BOOL P_GivePower(player_t *player, int /*powertype_t*/ power)
{
	if (power == pw_invulnerability)
	{
		player->powers[power] = INVULNTICS;
		return true;
	}

	if (power == pw_invisibility)
	{
		player->powers[power] = INVISTICS;
		player->mo->flags |= MF_SHADOW;
		return true;
	}

	if (power == pw_infrared)
	{
		player->powers[power] = INFRATICS;
		return true;
	}

	if (power == pw_ironfeet)
	{
		player->powers[power] = IRONTICS;
		return true;
	}

	if (power == pw_strength)
	{
		P_GiveBody(player, 100);
		player->powers[power] = 1;
		return true;
	}

	if (player->powers[power])
    {
		return false;	// already got it
    }

	player->powers[power] = 1;
	return true;
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

void P_GiveSpecial(player_t *player, AActor *special)
{
	if (!player || !player->mo || !special)
		return;

	AActor *toucher = player->mo;
	int sound = 0;
	bool firstgrab = false;

	// Identify by sprite.
	switch (special->sprite)
	{
		// armor
	    case SPR_ARM1:
            if (!P_GiveArmor(player, deh.GreenAC))
            {
                return;
            }
            SV_TouchSpecial(special, player);
            PickupMessage(toucher, GStrings(GOTARMOR));
            break;

	    case SPR_ARM2:
            if (!P_GiveArmor(player, deh.BlueAC))
            {
                return;
            }
            SV_TouchSpecial(special, player);
            PickupMessage(toucher, GStrings(GOTMEGA));
            break;

		// bonus items
	    case SPR_BON1:
            player->health++;				// can go over 100%
            if (player->health > deh.MaxSoulsphere)
            {
                player->health = deh.MaxSoulsphere;
            }
            player->mo->health = player->health;
            SV_TouchSpecial(special, player);
            PickupMessage(toucher, GStrings(GOTHTHBONUS));
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
            SV_TouchSpecial(special, player);
            PickupMessage(toucher, GStrings(GOTARMBONUS));
            break;

	    case SPR_SOUL:
            player->health += deh.SoulsphereHealth;
            if (player->health > deh.MaxSoulsphere)
            {
                player->health = deh.MaxSoulsphere;
            }
            player->mo->health = player->health;
            PickupMessage(toucher, GStrings(GOTSUPER));
            sound = 1;
            SV_TouchSpecial(special, player);
            break;

	    case SPR_MEGA:
            player->health = deh.MegasphereHealth;
            player->mo->health = player->health;
            P_GiveArmor(player,deh.BlueAC);
            PickupMessage(toucher, GStrings(GOTMSPHERE));
            sound = 1;
            SV_TouchSpecial(special, player);
            break;

		// cards
		// leave cards for everyone
	    case SPR_BKEY:
            if (!player->cards[it_bluecard])
            {
                PickupMessage(toucher, GStrings(GOTBLUECARD));
            }
            P_GiveCard(player, it_bluecard);
            sound = 3;
            if (!multiplayer)
            {
                break;
            }
            SV_TouchSpecial(special, player);
            return;

	    case SPR_YKEY:
            if (!player->cards[it_yellowcard])
            {
                PickupMessage(toucher, GStrings(GOTYELWCARD));
            }
            P_GiveCard(player, it_yellowcard);
            sound = 3;
            if (!multiplayer)
            {
                break;
            }
            SV_TouchSpecial(special, player);
            return;

	    case SPR_RKEY:
            if (!player->cards[it_redcard])
            {
                PickupMessage(toucher, GStrings(GOTREDCARD));
            }
            P_GiveCard(player, it_redcard);
            sound = 3;
            if (!multiplayer)
            {
                break;
            }
            SV_TouchSpecial(special, player);
            return;

	    case SPR_BSKU:
            if (!player->cards[it_blueskull])
            {
                PickupMessage(toucher, GStrings(GOTBLUESKUL));
            }
            P_GiveCard(player, it_blueskull);
            sound = 3;
            if (!multiplayer)
            {
                break;
            }
            SV_TouchSpecial(special, player);
            return;

	    case SPR_YSKU:
            if (!player->cards[it_yellowskull])
            {
                PickupMessage(toucher, GStrings(GOTYELWSKUL));
            }
            P_GiveCard(player, it_yellowskull);
            sound = 3;
            if (!multiplayer)
            {
                break;
            }
            SV_TouchSpecial(special, player);
            return;

	    case SPR_RSKU:
            if (!player->cards[it_redskull])
            {
                PickupMessage(toucher, GStrings(GOTREDSKUL));
            }
            P_GiveCard(player, it_redskull);
            sound = 3;
            if (!multiplayer)
            {
                break;
            }
            SV_TouchSpecial(special, player);
            return;

		// medikits, heals
	    case SPR_STIM:
            if (!P_GiveBody(player, 10))
            {
                return;
            }
            PickupMessage(toucher, GStrings(GOTSTIM));
            SV_TouchSpecial(special, player);
            break;

	    case SPR_MEDI:
            if (player->health < 25)
            {
                PickupMessage(toucher, GStrings(GOTMEDINEED));
            }
            else if (player->health < 100)
            {
                PickupMessage(toucher, GStrings(GOTMEDIKIT));
            }
            if (!P_GiveBody(player, 25))
            {
                return;
            }
            SV_TouchSpecial(special, player);
            break;

		// power ups
	    case SPR_PINV:
            if (!P_GivePower(player, pw_invulnerability))
            {
                return;
            }
            PickupMessage(toucher, GStrings(GOTINVUL));
            sound = 1;
            SV_TouchSpecial(special, player);
            break;

	    case SPR_PSTR:
            if (!P_GivePower(player, pw_strength))
            {
                return;
            }
            PickupMessage(toucher, GStrings(GOTBERSERK));
            if (player->readyweapon != wp_fist)
            {
                player->pendingweapon = wp_fist;
            }
            sound = 1;
            SV_TouchSpecial(special, player);
            break;

	    case SPR_PINS:
            if (!P_GivePower(player, pw_invisibility))
            {
                return;
            }
            PickupMessage(toucher, GStrings(GOTINVIS));
            sound = 1;
            SV_TouchSpecial(special, player);
            break;

	    case SPR_SUIT:
            if (!P_GivePower(player, pw_ironfeet))
            {
                return;
            }
            PickupMessage(toucher, GStrings(GOTSUIT));
            sound = 1;
            SV_TouchSpecial(special, player);
            break;

	    case SPR_PMAP:
            if (!P_GivePower(player, pw_allmap))
            {
                return;
            }
            PickupMessage(toucher, GStrings(GOTMAP));
            sound = 1;
            SV_TouchSpecial(special, player);
            break;

	    case SPR_PVIS:
            if (!P_GivePower(player, pw_infrared))
            {
                return;
            }
            PickupMessage(toucher, GStrings(GOTVISOR));
            sound = 1;
            SV_TouchSpecial(special, player);
            break;

		// ammo
	    case SPR_CLIP:
            if (special->flags & MF_DROPPED)
            {
                if (!P_GiveAmmo(player, am_clip, 0))
                {
                    return;
                }
            }
            else
            {
                if (!P_GiveAmmo(player, am_clip, 1))
                {
                    return;
                }
            }
            PickupMessage(toucher, GStrings(GOTCLIP));
            SV_TouchSpecial(special, player);
            break;

	    case SPR_AMMO:
            if (!P_GiveAmmo(player, am_clip, 5))
            {
                return;
            }
            PickupMessage(toucher, GStrings(GOTCLIPBOX));
            SV_TouchSpecial(special, player);
            break;

	    case SPR_ROCK:
            if (!P_GiveAmmo(player, am_misl, 1))
            {
                return;
            }
            PickupMessage(toucher, GStrings(GOTROCKET));
            SV_TouchSpecial(special, player);
            break;

	    case SPR_BROK:
            if (!P_GiveAmmo(player, am_misl, 5))
            {
                return;
            }
            PickupMessage(toucher, GStrings(GOTROCKBOX));
            SV_TouchSpecial(special, player);
            break;

	    case SPR_CELL:
            if (!P_GiveAmmo(player, am_cell, 1))
            {
                return;
            }
            PickupMessage(toucher, GStrings(GOTCELL));
            SV_TouchSpecial(special, player);
            break;

	    case SPR_CELP:
            if (!P_GiveAmmo(player, am_cell, 5))
            {
                return;
            }
            PickupMessage(toucher, GStrings(GOTCELLBOX));
            SV_TouchSpecial(special, player);
            break;

	    case SPR_SHEL:
            if (!P_GiveAmmo(player, am_shell, 1))
            {
                return;
            }
            PickupMessage(toucher, GStrings(GOTSHELLS));
            SV_TouchSpecial(special, player);
            break;

	    case SPR_SBOX:
            if (!P_GiveAmmo(player, am_shell, 5))
            {
                return;
            }
            PickupMessage(toucher, GStrings(GOTSHELLBOX));
            SV_TouchSpecial(special, player);
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
            PickupMessage(toucher, GStrings(GOTBACKPACK));
            SV_TouchSpecial(special, player);
            break;

		// weapons
	    case SPR_BFUG:
            SV_TouchSpecial(special, player);
            if (!P_GiveWeapon(player, wp_bfg, special->flags & MF_DROPPED))
            {
                return;
            }
            PickupMessage(toucher, GStrings(GOTBFG9000));
            sound = 2;
            break;

	    case SPR_MGUN:
            SV_TouchSpecial(special, player);
            if (!P_GiveWeapon(player, wp_chaingun, special->flags & MF_DROPPED))
            {
                return;
            }
            PickupMessage(toucher, GStrings(GOTCHAINGUN));
            sound = 2;
            break;

	    case SPR_CSAW:
            SV_TouchSpecial(special, player);
            if (!P_GiveWeapon(player, wp_chainsaw, special->flags & MF_DROPPED))
            {
                return;
            }
            PickupMessage(toucher, GStrings(GOTCHAINSAW));
            sound = 2;
            break;

	    case SPR_LAUN:
            SV_TouchSpecial(special, player);
            if (!P_GiveWeapon(player, wp_missile, special->flags & MF_DROPPED))
            {
                return;
            }
            PickupMessage(toucher, GStrings(GOTLAUNCHER));
            sound = 2;
            break;

	    case SPR_PLAS:
            SV_TouchSpecial(special, player);
            if (!P_GiveWeapon(player, wp_plasma, special->flags & MF_DROPPED))
            {
                return;
            }
            PickupMessage(toucher, GStrings(GOTPLASMA));
            sound = 2;
            break;

	    case SPR_SHOT:
            SV_TouchSpecial(special, player);
            if (!P_GiveWeapon(player, wp_shotgun, special->flags & MF_DROPPED))
            {
                return;
            }
            PickupMessage(toucher, GStrings(GOTSHOTGUN));
            sound = 2;
            break;

	    case SPR_SGN2:
            SV_TouchSpecial(special, player);
            if (!P_GiveWeapon(player, wp_supershotgun, special->flags & MF_DROPPED))
            {
                return;
            }
            PickupMessage(toucher, GStrings(GOTSHOTGUN2));
            sound = 2;
            break;

	// [Toke - CTF - Core]
        case SPR_BFLG: // Player touches the blue flag at its base
            firstgrab = true;

        case SPR_BDWN: // Player touches the blue flag after it's been dropped
            if (!SV_FlagTouch(*player, it_blueflag, firstgrab))
            {
                return;
            }
            sound = 3;
            break;

        case SPR_BSOK:
            SV_SocketTouch(*player, it_blueflag);
            return;

        case SPR_RFLG: // Player touches the red flag at its base
            firstgrab = true;

        case SPR_RDWN: // Player touches the red flag after its been dropped
            if (!SV_FlagTouch(*player, it_redflag, firstgrab))
            {
                return;
            }
            sound = 3;
            break;

        case SPR_RSOK:
            SV_SocketTouch(*player, it_redflag);
            return;

        default:
            // I_Error ("P_SpecialThing: Unknown gettable thing %d: %s\n", special->sprite,special->info->name);
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

	special->Destroy();

	player->bonuscount = BONUSADD;

    if (clientside)
	{
		AActor *ent = player->mo;

		// denis - only play own pickup sounds
		switch (sound)
        {
			case 0:
			case 3:
				S_Sound(ent, CHAN_ITEM, "misc/i_pkup", 1, ATTN_NONE);
				break;
			case 1:
				S_Sound(ent, CHAN_ITEM, "misc/p_pkup", 1, ATTN_NONE);
				break;
			case 2:
				S_Sound(ent, CHAN_ITEM, "misc/w_pkup", 1, ATTN_NONE);
				break;
		}
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
        !(demoplayback && democlassic) && !joinkill)
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
	int 		saved;
	player_t*   splayer; // shorthand for source->player
	player_t*   tplayer; // shorthand for target->player
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
	}

	// do the damage
	// [RH] Only if not immune
	if (!(target->flags2 & (MF2_INVULNERABLE | MF2_DORMANT)))
	{
		target->health -= damage;
		if (target->health <= 0)
		{
			P_KillMobj(source, target, inflictor, false);
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

VERSION_CONTROL (p_interaction_cpp, "$Id$")

