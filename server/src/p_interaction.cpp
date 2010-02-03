// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
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
//	Handling interactions (i.e., collisions).
//
//-----------------------------------------------------------------------------


// Data.
#include "doomdef.h"
#include "dstrings.h"

#include "doomstat.h"

#include "m_random.h"
#include "i_system.h"

#include "c_console.h"
#include "c_dispatch.h"

#include "p_local.h"

#include "s_sound.h"

#include "p_inter.h"
#include "p_lnspec.h"

#include "sv_main.h"

#include "sv_ctf.h"

#define BONUSADD		6

EXTERN_CVAR (friendlyfire)
EXTERN_CVAR (allowexit)
EXTERN_CVAR (fragexitswitch)              // [ML] 04/4/06: Added comprimise for older exit method
EXTERN_CVAR (doubleammo)

int shotclock = 0;

// a weapon is found with two clip loads,
// a big item has five clip loads
int 	maxammo[NUMAMMO] = {200, 50, 300, 50};
int 	clipammo[NUMAMMO] = {10, 4, 20, 1};



//
// GET STUFF
//

//
// P_GiveAmmo
// Num is the number of clip loads,
// not the individual count (0= 1/2 clip).
// Returns false if the ammo can't be picked up at all
//

BOOL P_GiveAmmo (player_t *player, ammotype_t ammo, int num)
{
	int oldammo;

	if (ammo == am_noammo)
		return false;

	if (ammo < 0 || ammo > NUMAMMO)
		I_Error ("P_GiveAmmo: bad type %i", ammo);

	if ( player->ammo[ammo] == player->maxammo[ammo]  )
		return false;

	if (num)
		num *= clipammo[ammo];
	else
		num = clipammo[ammo]/2;

	if (skill == sk_baby
		|| skill == sk_nightmare || doubleammo)
	{
		// give double ammo in trainer mode,
		// you'll need in nightmare
		num <<= 1;
	}


	oldammo = player->ammo[ammo];
	player->ammo[ammo] += num;

	if (player->ammo[ammo] > player->maxammo[ammo])
		player->ammo[ammo] = player->maxammo[ammo];

	// If non zero ammo,
	// don't change up weapons,
	// player was lower on purpose.
	if (oldammo)
		return true;

	// We were down to zero,
	// so select a new weapon.
	// Preferences are not user selectable.
	switch (ammo)
	{
	  case am_clip:
		if (player->readyweapon == wp_fist)
		{
			if (player->weaponowned[wp_chaingun])
				player->pendingweapon = wp_chaingun;
			else
				player->pendingweapon = wp_pistol;
		}
		break;

	  case am_shell:
		if (player->readyweapon == wp_fist
			|| player->readyweapon == wp_pistol)
		{
			if (player->weaponowned[wp_shotgun])
				player->pendingweapon = wp_shotgun;
		}
		break;

	  case am_cell:
		if (player->readyweapon == wp_fist
			|| player->readyweapon == wp_pistol)
		{
			if (player->weaponowned[wp_plasma])
				player->pendingweapon = wp_plasma;
		}
		break;

	  case am_misl:
		if (player->readyweapon == wp_fist)
		{
			if (player->weaponowned[wp_missile])
				player->pendingweapon = wp_missile;
		}
	  default:
		break;
	}

	return true;
}
EXTERN_CVAR (weaponstay)

//
// P_GiveWeapon
// The weapon name may have a MF_DROPPED flag ored in.
//
BOOL P_GiveWeapon (player_t *player, weapontype_t weapon, BOOL dropped)
{
	BOOL gaveammo;
	BOOL gaveweapon;

	// [RH] Don't get the weapon if no graphics for it
	state_t *state = states + weaponinfo[weapon].readystate;
	if ((state->frame & FF_FRAMEMASK) >= sprites[state->sprite].numframes)
		return false;

	// [Toke - dmflags] old location of DF_WEAPONS_STAY
	if (multiplayer && weaponstay && !dropped)
	{
		// leave placed weapons forever on net games
		if (player->weaponowned[weapon])
			return false;

		player->bonuscount = BONUSADD;
		player->weaponowned[weapon] = true;

		if (gametype != GM_COOP)
			P_GiveAmmo (player, weaponinfo[weapon].ammo, 5);
		else
			P_GiveAmmo (player, weaponinfo[weapon].ammo, 2);

		player->pendingweapon = weapon;

		S_Sound (player->mo, CHAN_ITEM, "misc/w_pkup", 1, ATTN_NORM);

		return false;
	}

	if (weaponinfo[weapon].ammo != am_noammo)
	{
		// give one clip with a dropped weapon,
		// two clips with a found weapon
		if (dropped)
			gaveammo = P_GiveAmmo (player, weaponinfo[weapon].ammo, 1);
		else
			gaveammo = P_GiveAmmo (player, weaponinfo[weapon].ammo, 2);
	}
	else
		gaveammo = false;

	if (player->weaponowned[weapon])
		gaveweapon = false;
	else
	{
		gaveweapon = true;
		player->weaponowned[weapon] = true;
		player->pendingweapon = weapon;
	}

	return (gaveweapon || gaveammo);
}



//
// P_GiveBody
// Returns false if the body isn't needed at all
//
BOOL P_GiveBody (player_t *player, int num)
{
	if (player->health >= MAXHEALTH)
		return false;

	player->health += num;
	if (player->health > MAXHEALTH)
		player->health = MAXHEALTH;
	player->mo->health = player->health;

	return true;
}



//
// P_GiveArmor
// Returns false if the armor is worse
// than the current armor.
//
BOOL P_GiveArmor (player_t *player, int armortype)
{
	int hits;

	hits = armortype*100;
	if (player->armorpoints >= hits)
		return false;	// don't pick up

	player->armortype = armortype;
	player->armorpoints = hits;

	return true;
}



//
// P_GiveCard
//
void P_GiveCard (player_t *player, card_t card)
{
	if (player->cards[card])
		return;

	player->bonuscount = BONUSADD;
	player->cards[card] = 1;
}


//
// P_GivePower
//
BOOL P_GivePower (player_t *player, int /*powertype_t*/ power)
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
		P_GiveBody (player, 100);
		player->powers[power] = 1;
		return true;
	}

	if (player->powers[power])
		return false;	// already got it

	player->powers[power] = 1;
	return true;
}



//
// P_TouchSpecialThing
//
void P_TouchSpecialThing (AActor *special, AActor *toucher)
{
	player_t*	player;
	size_t		i;
	int			sound;
	bool		firstgrab = false;

	if (!toucher || !special) // [Toke - fix99]
		return;

	// GhostlyDeath -- Spectators can't pick up things
	if (toucher->player && toucher->player->spectator)
		return;

    // Dead thing touching.
    // Can happen with a sliding player corpse.
    if (toucher->health <= 0)
		return;

	fixed_t delta = special->z - toucher->z;

	if (delta > toucher->height || delta < -8*FRACUNIT)
	{
		// out of reach
		return;
	}

	sound = 0;

	if(!toucher->player)
		return;

	player = toucher->player;

	// Identify by sprite.
	switch (special->sprite)
	{
		// armor
	  case SPR_ARM1:
		if (!P_GiveArmor (player, deh.GreenAC))
			return;
		break;

	  case SPR_ARM2:
		if (!P_GiveArmor (player, deh.BlueAC))
			return;
		break;

		// bonus items
	  case SPR_BON1:
		player->health++;				// can go over 100%
		if (player->health > deh.MaxSoulsphere)
			player->health = deh.MaxSoulsphere;
		player->mo->health = player->health;
		break;

	  case SPR_BON2:
		player->armorpoints++;			// can go over 100%
		if (player->armorpoints > deh.MaxArmor)
			player->armorpoints = deh.MaxArmor;
		if (!player->armortype)
			player->armortype = deh.GreenAC;
		break;

	  case SPR_SOUL:
		player->health += deh.SoulsphereHealth;
		if (player->health > deh.MaxSoulsphere)
			player->health = deh.MaxSoulsphere;
		player->mo->health = player->health;
		sound = 1;
		break;

	  case SPR_MEGA:
		player->health = deh.MegasphereHealth;
		player->mo->health = player->health;
		P_GiveArmor (player,deh.BlueAC);
		sound = 1;
		break;

		// cards
		// leave cards for everyone
	  case SPR_BKEY:
		P_GiveCard (player, it_bluecard);
		sound = 3;
		if (!multiplayer)
			break;
		return;

	  case SPR_YKEY:
		P_GiveCard (player, it_yellowcard);
		sound = 3;
		if (!multiplayer)
			break;
		return;

	  case SPR_RKEY:
		P_GiveCard (player, it_redcard);
		sound = 3;
		if (!multiplayer)
			break;
		return;

	  case SPR_BSKU:
		P_GiveCard (player, it_blueskull);
		sound = 3;
		if (!multiplayer)
			break;
		return;

	  case SPR_YSKU:
		P_GiveCard (player, it_yellowskull);
		sound = 3;
		if (!multiplayer)
			break;
		return;

	  case SPR_RSKU:
		P_GiveCard (player, it_redskull);
		sound = 3;
		if (!multiplayer)
			break;
		return;

		// medikits, heals
	  case SPR_STIM:
		if (!P_GiveBody (player, 10))
			return;
		break;

	  case SPR_MEDI:
		if (!P_GiveBody (player, 25))
			return;
		break;


		// power ups
	  case SPR_PINV:
		if (!P_GivePower (player, pw_invulnerability))
			return;
		sound = 1;
		break;

	  case SPR_PSTR:
		if (!P_GivePower (player, pw_strength))
			return;
		if (player->readyweapon != wp_fist)
			player->pendingweapon = wp_fist;
		sound = 1;
		break;

	  case SPR_PINS:
		if (!P_GivePower (player, pw_invisibility))
			return;
		sound = 1;
		break;

	  case SPR_SUIT:
		if (!P_GivePower (player, pw_ironfeet))
			return;
		sound = 1;
		break;

	  case SPR_PMAP:
		if (!P_GivePower (player, pw_allmap))
			return;
		sound = 1;
		break;

	  case SPR_PVIS:
		if (!P_GivePower (player, pw_infrared))
			return;
		sound = 1;
		break;

		// ammo
	  case SPR_CLIP:
		if (special->flags & MF_DROPPED)
		{
			if (!P_GiveAmmo (player,am_clip,0))
				return;
		}
		else
		{
			if (!P_GiveAmmo (player,am_clip,1))
				return;
		}
		break;

	  case SPR_AMMO:
		if (!P_GiveAmmo (player, am_clip,5))
			return;
		break;

	  case SPR_ROCK:
		if (!P_GiveAmmo (player, am_misl,1))
			return;
		break;

	  case SPR_BROK:
		if (!P_GiveAmmo (player, am_misl,5))
			return;
		break;

	  case SPR_CELL:
		if (!P_GiveAmmo (player, am_cell,1))
			return;
		break;

	  case SPR_CELP:
		if (!P_GiveAmmo (player, am_cell,5))
			return;
		break;

	  case SPR_SHEL:
		if (!P_GiveAmmo (player, am_shell,1))
			return;
		break;

	  case SPR_SBOX:
		if (!P_GiveAmmo (player, am_shell,5))
			return;
		break;

	  case SPR_BPAK:
		if (!player->backpack)
		{
			for (i=0 ; i<NUMAMMO ; i++)
				player->maxammo[i] *= 2;
			player->backpack = true;
		}
		for (i=0 ; i<NUMAMMO ; i++)
			P_GiveAmmo (player, (ammotype_t)i, 1);
		break;

		// weapons
	  case SPR_BFUG:
		if (!P_GiveWeapon (player, wp_bfg, special->flags & MF_DROPPED))
			return;
		sound = 2;
		break;

	  case SPR_MGUN:
		if (!P_GiveWeapon (player, wp_chaingun, special->flags & MF_DROPPED))
			return;
		sound = 2;
		break;

	  case SPR_CSAW:
		if (!P_GiveWeapon (player, wp_chainsaw, special->flags & MF_DROPPED))
			return;
		sound = 2;
		break;

	  case SPR_LAUN:
		if (!P_GiveWeapon (player, wp_missile, special->flags & MF_DROPPED))
			return;
		sound = 2;
		break;

	  case SPR_PLAS:
		if (!P_GiveWeapon (player, wp_plasma, special->flags & MF_DROPPED))
			return;
		sound = 2;
		break;

	  case SPR_SHOT:
		if (!P_GiveWeapon (player, wp_shotgun, special->flags & MF_DROPPED))
			return;
		sound = 2;
		break;

	  case SPR_SGN2:
		if (!P_GiveWeapon (player, wp_supershotgun, special->flags & MF_DROPPED))
			return;
		sound = 2;
		break;

		// Core CTF Logic - everything else stems from this	[Toke - CTF]
		case SPR_BFLG:	// Player touches the BLUE flag at its base
		firstgrab = true;
		case SPR_BDWN:	// Player touches the BLUE flag after its been dropped

			if(!SV_FlagTouch(*player, it_blueflag, firstgrab))
				return;
			sound = 3;

			break;
		case SPR_BSOK:
			SV_SocketTouch(*player, it_blueflag);
			return;
		case SPR_RFLG:	// Player touches the RED flag at its base
		firstgrab = true;
		case SPR_RDWN:	// Player touches the RED flag after its been dropped

			if(!SV_FlagTouch(*player, it_redflag, firstgrab))
				return;
			sound = 3;

			break;
		case SPR_RSOK:
			SV_SocketTouch(*player, it_redflag);
			return;
		case SPR_GFLG: // Remove me in 0.5
		case SPR_GDWN: // Remove me in 0.5
		case SPR_GSOK: // Remove me in 0.5
			return; // Remove me in 0.5
	  default:
		I_Error ("P_SpecialThing: Unknown gettable thing %d\n", special->sprite);
	}

	if (special->flags & MF_COUNTITEM)
	{
		level.found_items++;
	}

	special->Destroy ();

	player->bonuscount = BONUSADD;
}


// [RH]
// SexMessage: Replace parts of strings with gender-specific pronouns
//
// The following expansions are performed:
//		%g -> he/she/it
//		%h -> him/her/it
//		%p -> his/her/its
//
void SexMessage (const char *from, char *to, int gender)
{
	static const char *genderstuff[3][3] = {
		{ "he",  "him", "his" },
		{ "she", "her", "her" },
		{ "it",  "it",  "its" }
	};
	static const int gendershift[3][3] = {
		{ 2, 3, 3 },
		{ 3, 3, 3 },
		{ 2, 2, 3 }
	};
	int gendermsg;

	do {
		if (*from != '%') {
			*to++ = *from;
		} else {
			switch (from[1]) {
				case 'g':	gendermsg = 0;	break;
				case 'h':	gendermsg = 1;	break;
				case 'p':	gendermsg = 2;	break;
				default:	gendermsg = -1;	break;
			}
			if (gendermsg < 0) {
				*to++ = '%';
			} else {
				strcpy (to, genderstuff[gender][gendermsg]);
				to += gendershift[gender][gendermsg];
				from++;
			}
		}
	} while (*from++);
}

// [RH]
// ClientObituary: Show a message when a player dies
//
void ClientObituary (AActor *self, AActor *inflictor, AActor *attacker)
{
	const char *message;
	char gendermessage[1024];
	int  gender;

	if (!self->player)
		return;

	gender = self->player->userinfo.gender;

	// Treat voodoo dolls as unknown deaths
	if (inflictor && inflictor->player == self->player)
		MeansOfDeath = MOD_UNKNOWN;

	message = NULL;

	switch (MeansOfDeath) {
		case MOD_SUICIDE:
			message = OB_SUICIDE;
			break;
		case MOD_FALLING:
			message = OB_FALLING;
			break;
		case MOD_CRUSH:
			message = OB_CRUSH;
			break;
		case MOD_EXIT:
			message = OB_EXIT;
			break;
		case MOD_WATER:
			message = OB_WATER;
			break;
		case MOD_SLIME:
			message = OB_SLIME;
			break;
		case MOD_LAVA:
			message = OB_LAVA;
			break;
		case MOD_BARREL:
			message = OB_BARREL;
			break;
		case MOD_SPLASH:
			message = OB_SPLASH;
			break;
	}

	if (attacker && !message) {
		if (attacker == self) {
			switch (MeansOfDeath) {
				case MOD_R_SPLASH:
					message = OB_R_SPLASH;
					break;
				case MOD_ROCKET:
					message = OB_ROCKET;
					break;
				default:
					message = OB_KILLEDSELF;
					break;
			}
		} else if (!attacker->player) {
					if (MeansOfDeath == MOD_HIT) {
						switch (attacker->type) {
							case MT_UNDEAD:
								message = OB_UNDEADHIT;
								break;
							case MT_TROOP:
								message = OB_IMPHIT;
								break;
							case MT_HEAD:
								message = OB_CACOHIT;
								break;
							case MT_SERGEANT:
								message = OB_DEMONHIT;
								break;
							case MT_SHADOWS:
								message = OB_SPECTREHIT;
								break;
							case MT_BRUISER:
								message = OB_BARONHIT;
								break;
							case MT_KNIGHT:
								message = OB_KNIGHTHIT;
								break;
							default:
								break;
						}
					} else {
						switch (attacker->type) {
							case MT_POSSESSED:
								message = OB_ZOMBIE;
								break;
							case MT_SHOTGUY:
								message = OB_SHOTGUY;
								break;
							case MT_VILE:
								message = OB_VILE;
								break;
							case MT_UNDEAD:
								message = OB_UNDEAD;
								break;
							case MT_FATSO:
								message = OB_FATSO;
								break;
							case MT_CHAINGUY:
								message = OB_CHAINGUY;
								break;
							case MT_SKULL:
								message = OB_SKULL;
								break;
							case MT_TROOP:
								message = OB_IMP;
								break;
							case MT_HEAD:
								message = OB_CACO;
								break;
							case MT_BRUISER:
								message = OB_BARON;
								break;
							case MT_KNIGHT:
								message = OB_KNIGHT;
								break;
							case MT_SPIDER:
								message = OB_SPIDER;
								break;
							case MT_BABY:
								message = OB_BABY;
								break;
							case MT_CYBORG:
								message = OB_CYBORG;
								break;
							case MT_WOLFSS:
								message = OB_WOLFSS;
								break;
							default:
								break;
						}
					}
		}
	}

	if (message) {
		SexMessage (message, gendermessage, gender);
		SV_BroadcastPrintf (PRINT_MEDIUM, "%s %s.\n", self->player->userinfo.netname, gendermessage);
		return;
	}

	if (attacker && attacker->player) {
		if (((gametype == GM_TEAMDM || gametype == GM_CTF) && self->player->userinfo.team == attacker->player->userinfo.team) || gametype == GM_COOP) {
			int rnum = P_Random ();

			self = attacker;
			gender = self->player->userinfo.gender;

			if (rnum < 64)
				message = OB_FRIENDLY1;
			else if (rnum < 128)
				message = OB_FRIENDLY2;
			else if (rnum < 192)
				message = OB_FRIENDLY3;
			else
				message = OB_FRIENDLY4;
		} else {
			switch (MeansOfDeath) {
				case MOD_FIST:
					message = OB_MPFIST;
					break;
				case MOD_CHAINSAW:
					message = OB_MPCHAINSAW;
					break;
				case MOD_PISTOL:
					message = OB_MPPISTOL;
					break;
				case MOD_SHOTGUN:
					message = OB_MPSHOTGUN;
					break;
				case MOD_SSHOTGUN:
					message = OB_MPSSHOTGUN;
					break;
				case MOD_CHAINGUN:
					message = OB_MPCHAINGUN;
					break;
				case MOD_ROCKET:
					message = OB_MPROCKET;
					break;
				case MOD_R_SPLASH:
					message = OB_MPR_SPLASH;
					break;
				case MOD_PLASMARIFLE:
					message = OB_MPPLASMARIFLE;
					break;
				case MOD_BFG_BOOM:
					message = OB_MPBFG_BOOM;
					break;
				case MOD_BFG_SPLASH:
					message = OB_MPBFG_SPLASH;
					break;
				case MOD_TELEFRAG:
					message = OB_MPTELEFRAG;
					break;
			}
		}
	}

	if (message) {
		SexMessage (message, gendermessage, gender);

		std::string work = "%s ";
		work += gendermessage;
		work += ".\n";

		SV_BroadcastPrintf (PRINT_MEDIUM, work.c_str(), self->player->userinfo.netname,
							attacker->player->userinfo.netname);
		return;
	}

	SexMessage (OB_DEFAULT, gendermessage, gender);
	SV_BroadcastPrintf (PRINT_MEDIUM, "%s %s.\n", self->player->userinfo.netname, gendermessage);
}

extern BOOL singleplayerjustdied;

//
// KillMobj
//
EXTERN_CVAR (fraglimit)

void P_KillMobj (AActor *source, AActor *target, AActor *inflictor, bool joinkill)
{
	for (size_t i = 0; i < players.size(); i++)
	{
		client_t *cl = &clients[i];

		if(!SV_IsPlayerAllowedToSee(players[i], target))
			continue;

		// send death location first
		MSG_WriteMarker (&cl->netbuf, svc_movemobj);
		MSG_WriteShort (&cl->netbuf, target->netid);
		MSG_WriteByte (&cl->netbuf, target->rndindex);
		MSG_WriteLong (&cl->netbuf, target->x);
		MSG_WriteLong (&cl->netbuf, target->y);
		MSG_WriteLong (&cl->netbuf, target->z);
		MSG_WriteMarker (&cl->reliablebuf, svc_killmobj);

		if (source)
			MSG_WriteShort (&cl->reliablebuf, source->netid);
		else
			MSG_WriteShort (&cl->reliablebuf, 0);

		MSG_WriteShort (&cl->reliablebuf, target->netid);
		MSG_WriteShort (&cl->reliablebuf, inflictor ? inflictor->netid : 0);
		MSG_WriteShort (&cl->reliablebuf, target->health);
		MSG_WriteLong (&cl->reliablebuf, MeansOfDeath);
		MSG_WriteByte (&cl->reliablebuf, joinkill);
	}

	AActor *mo;

	target->flags &= ~(MF_SHOOTABLE|MF_FLOAT|MF_SKULLFLY);

	// GhostlyDeath -- Joinkill is only set on players, so we should be safe!
	if (joinkill)
		target->flags |= MF_SPECTATOR;

	if (target->type != MT_SKULL)
		target->flags &= ~MF_NOGRAVITY;

	target->flags |= MF_CORPSE|MF_DROPOFF;
	target->height >>= 2;

	// [RH] Also set the thing's tid to 0. [why?]
	target->tid = 0;

	player_t *splayer = source ? source->player : 0;
	player_t *tplayer = target->player;

	if (source && source->player)
	{
		// Don't count any frags at level start, because they're just telefrags
		// resulting from insufficient deathmatch starts, and it wouldn't be
		// fair to count them toward a player's score.
		if (target->player && level.time)
		{
			if (!joinkill && !shotclock)
			{
				if (target->player == source->player)	// [RH] Cumulative frag count
				{
					splayer->fragcount--;
					// [Toke] Minus a team frag for suicide
					if (gametype == GM_TEAMDM)
						TEAMpoints[splayer->userinfo.team]--;
				}
				// [Toke] Minus a team frag for killing team mate
				else if ((gametype == GM_TEAMDM || gametype == GM_CTF) && (splayer->userinfo.team == tplayer->userinfo.team)) // [Toke - Teamplay || deathz0r - updated]
				{
					splayer->fragcount--;

					if (gametype == GM_TEAMDM)
						TEAMpoints[splayer->userinfo.team]--;
					else if (gametype == GM_CTF)
						SV_CTFEvent ((flag_t)0, SCORE_BETRAYAL, *splayer);
				}
				else
				{
					splayer->fragcount++;

					// [Toke] Add a team frag
					if (gametype == GM_TEAMDM)
						TEAMpoints[splayer->userinfo.team]++;
					else if (gametype == GM_CTF) {
						if (tplayer->flags[(flag_t)splayer->userinfo.team])
							SV_CTFEvent ((flag_t)0, SCORE_CARRIERKILL, *splayer);
						else
							SV_CTFEvent ((flag_t)0, SCORE_KILL, *splayer);
					}
				}
			}

			SV_UpdateFrags (*splayer);
		}

		// [deathz0r] Stats for co-op scoreboard
		if (gametype == GM_COOP && (target->flags & MF_COUNTKILL) || (target->type == MT_SKULL))
		{
			splayer->killcount++;
			SV_UpdateFrags (*splayer);
		}

	}

	// [Toke - CTF]
	if (gametype == GM_CTF && target->player)
		CTF_CheckFlags ( *target->player );

	if (target->player)
	{
		if (!joinkill && !shotclock)
			tplayer->deathcount++;

		// count environment kills against you
		if (!source && !joinkill && !shotclock)
		{
			tplayer->fragcount--;	// [RH] Cumulative frag count

			// [JDC] Minus a team frag
			if (gametype == GM_TEAMDM)
				TEAMpoints[tplayer->userinfo.team]--;
		}

		SV_UpdateFrags (*tplayer);

		target->flags &= ~MF_SOLID;
		tplayer->playerstate = PST_DEAD;
		if (!multiplayer)
			singleplayerjustdied = true;
		tplayer->respawn_time = level.time + 60*TICRATE; // 1 minute forced respawn
		P_DropWeapon (tplayer);
	}

	if(target->health > 0) // denis - when this function is used standalone
		target->health = 0;

		if (target->health < -target->info->spawnhealth
			&& target->info->xdeathstate)
		{
			P_SetMobjState (target, target->info->xdeathstate);
		}
		else
			P_SetMobjState (target, target->info->deathstate);

	target->tics -= P_Random (target) & 3;

	if (target->tics < 1)
		target->tics = 1;

	// [RH] Death messages
	if (target->player && level.time && !joinkill && !shotclock)
		ClientObituary (target, inflictor, source);

	// Check fraglimit.
	if (source && source->player && target->player && level.time)
	{
		// [Toke] Better fraglimit
		if (gametype == GM_DM && fraglimit && splayer->fragcount >= (int)fraglimit && !fragexitswitch) // [ML] 04/4/06: Added !fragexitswitch
		{
				SV_BroadcastPrintf (PRINT_HIGH, "Frag limit hit. Game won by %s!\n", splayer->userinfo.netname);
				shotclock = TICRATE*2;
		}

		// [Toke] TeamDM fraglimit
		if (gametype == GM_TEAMDM && fraglimit)
		{
			for(size_t i = 0; i < NUMFLAGS; i++)
			{
				if (TEAMpoints[i] >= (int)fraglimit)
				{
					SV_BroadcastPrintf (PRINT_HIGH, "Frag limit hit. %s team wins!\n", team_names[i]);
					shotclock = TICRATE*2;
					break;
				}
			}
		}
	}

	if (gamemode == retail_chex)	// [ML] Chex Quest mode - monsters don't drop items
		return;

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
		return; //Happens if bot or player killed when using fists.

	if(serverside)
	{
		mo = new AActor (target->x, target->y, ONFLOORZ, item);
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
int MeansOfDeath;

void P_DamageMobj (AActor *target, AActor *inflictor, AActor *source, int damage, int mod, int flags)
{
	unsigned	ang;
	int 		saved;
	player_t*	player;
	fixed_t 	thrust;

	if ( !(target->flags & MF_SHOOTABLE) )
		return; // shouldn't happen...

	// GhostlyDeath -- Spectators can't get hurt!
	if (target->player && target->player->spectator)
		return;

	if (target->health <= 0)
		return;

	MeansOfDeath = mod;

	if ( target->flags & MF_SKULLFLY )
	{
		target->momx = target->momy = target->momz = 0;
	}

	player = target->player;
	if (player && skill == sk_baby)
		damage >>= 1;	// take half damage in trainer mode

	// Some close combat weapons should not
	// inflict thrust and push the victim out of reach,
	// thus kick away unless using the chainsaw.
	if (inflictor
		&& !(target->flags & MF_NOCLIP)
		&& (!source
			|| !source->player
			|| source->player->readyweapon != wp_chainsaw))
	{
		ang = P_PointToAngle ( inflictor->x,
								inflictor->y,
								target->x,
								target->y);

		thrust = damage*(FRACUNIT>>3)*100/target->info->mass;

		// make fall forwards sometimes
		if ( damage < 40
			 && damage > target->health
			 && target->z - inflictor->z > 64*FRACUNIT
			 && (P_Random ()&1) )
		{
			ang += ANG180;
			thrust *= 4;
		}

		ang >>= ANGLETOFINESHIFT;
		target->momx += FixedMul (thrust, finecosine[ang]);
		target->momy += FixedMul (thrust, finesine[ang]);
	}

	// player specific
	if (player)
	{
		// end of game hell hack
		if(gametype == GM_COOP || allowexit)
		if ((target->subsector->sector->special & 255) == dDamage_End
			&& damage >= target->health)
		{
			damage = target->health - 1;
		}

		// Below certain threshold,
		// ignore damage in GOD mode, or with INVUL power.
		if ( damage < 1000
			 && ( (player->cheats&CF_GODMODE)
				  || player->powers[pw_invulnerability] ) )
		{
			return;
		}

		if (player->armortype && !(flags & DMG_NO_ARMOR))
		{
			if (player->armortype == deh.GreenAC)
				saved = damage/3;
			else
				saved = damage/2;

			if (player->armorpoints <= saved)
			{
				// armor is used up
				saved = player->armorpoints;
				player->armortype = 0;
			}
			player->armorpoints -= saved;
			damage -= saved;
		}

		// only armordamage with friendlyfire
		if (!friendlyfire && source && source->player && target != source && mod != MOD_TELEFRAG &&
			(((gametype == GM_TEAMDM || gametype == GM_CTF) && target->player->userinfo.team == source->player->userinfo.team)
			|| gametype == GM_COOP))
			damage = 0;

		player->health -= damage;		// mirror mobj health here for Dave
		if (player->health <= 0)
			player->health = 0;

		player->attacker = source ? source->ptr() : AActor::AActorPtr();
		player->damagecount += damage;	// add damage after armor / invuln

		if (player->damagecount > 100)
			player->damagecount = 100;	// teleport stomp does 10k points...

        for (size_t i=0; i < players.size(); i++)
		{
            client_t *cl = &clients[i];

			MSG_WriteMarker (&cl->reliablebuf, svc_damageplayer);
			MSG_WriteByte (&cl->reliablebuf, player->id);
            MSG_WriteByte (&cl->reliablebuf, player->armorpoints);
            MSG_WriteShort (&cl->reliablebuf, target->health - damage);
        }
	}

	// do the damage
	{
		target->health -= damage;
		if (target->health <= 0)
		{
			P_KillMobj (source, target, inflictor, false);
			return;
		}
	}

	{
		int pain = P_Random();

		if(!player)
		{
			for (size_t i = 0; i < players.size(); i++)
			{
				client_t *cl = &clients[i];

				MSG_WriteMarker (&cl->reliablebuf, svc_damagemobj);
				MSG_WriteShort (&cl->reliablebuf, target->netid);
				MSG_WriteShort (&cl->reliablebuf, target->health);
				MSG_WriteByte (&cl->reliablebuf, pain);
			}
		}

		if ( pain < target->info->painchance
			 && !(target->flags&MF_SKULLFLY)
			 && !(player && !damage))
		{
			target->flags |= MF_JUSTHIT;	// fight back!

			P_SetMobjState (target, target->info->painstate);
		}

		target->reactiontime = 0;			// we're awake now...

		if ( (!target->threshold || target->type == MT_VILE)
			 && source && source != target
			 && source->type != MT_VILE)
		{
			// if not intent on another player, chase after this one

			// killough 2/15/98: remember last enemy, to prevent
			// sleeping early; 2/21/98: Place priority on players

			if (!target->lastenemy || !target->lastenemy->player ||
				 target->lastenemy->health <= 0)
				target->lastenemy = target->target; // remember last enemy - killough

			target->target = source->ptr();
			target->threshold = BASETHRESHOLD;
			if (target->state == &states[target->info->spawnstate]
				&& target->info->seestate != S_NULL)
				P_SetMobjState (target, target->info->seestate);

			SV_ActorTarget(target);
		}
	}
}

BOOL CheckCheatmode (void);

VERSION_CONTROL (p_interaction_cpp, "$Id$")

