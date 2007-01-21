// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 2006-2007 by The Odamex Team.
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
//		Handling interactions (i.e., collisions).
//
//-----------------------------------------------------------------------------

// Data.
#include "doomdef.h"
#include "dstrings.h"

#include "doomstat.h"

#include "m_random.h"
#include "i_system.h"

#include "am_map.h"

#include "c_console.h"
#include "c_dispatch.h"

#include "p_local.h"

#include "s_sound.h"

#include "p_inter.h"
#include "p_lnspec.h"

#include "p_interaction.h"

#include "cl_ctf.h"


static void PickupMessage (AActor *toucher, const char *message)
{
	// Some maps have multiple items stacked on top of each other.
	// It looks odd to display pickup messages for all of them.
	static int lastmessagetic;
	static const char *lastmessage = NULL;

	if (toucher == consoleplayer().camera
		&& (lastmessagetic != gametic || lastmessage != message))
	{
		lastmessagetic = gametic;
		lastmessage = message;
		Printf (PRINT_LOW, "%s\n", message);
	}
}

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
		|| skill == sk_nightmare)
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

	if (multiplayer && weaponstay && !dropped)
	{
		if (player->weaponowned[weapon])
			return false;

		player->bonuscount = BONUSADD;
		player->weaponowned[weapon] = true;

		if (deathmatch || ctfmode)
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
	int 		i;
	int 		sound;
	
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
	player = toucher->player;

	if(!player)
		return;

	// Identify by sprite.
	switch (special->sprite)
	{
		// armor
	  case SPR_ARM1:
		if (!P_GiveArmor (player, deh.GreenAC))
			return;
		PickupMessage (toucher, GOTARMOR);
		break;
				
	  case SPR_ARM2:
		if (!P_GiveArmor (player, deh.BlueAC))
			return;
		PickupMessage (toucher, GOTMEGA);
		break;
		
		// bonus items
	  case SPR_BON1:
		player->health++;				// can go over 100%
		if (player->health > deh.MaxSoulsphere)
			player->health = deh.MaxSoulsphere;
		player->mo->health = player->health;
		PickupMessage (toucher, GOTHTHBONUS);
		break;
		
	  case SPR_BON2:
		player->armorpoints++;			// can go over 100%
		if (player->armorpoints > deh.MaxArmor)
			player->armorpoints = deh.MaxArmor;
		if (!player->armortype)
			player->armortype = deh.GreenAC;
		PickupMessage (toucher, GOTARMBONUS);
		break;
		
	  case SPR_SOUL:
		player->health += deh.SoulsphereHealth;
		if (player->health > deh.MaxSoulsphere)
			player->health = deh.MaxSoulsphere;
		player->mo->health = player->health;
		PickupMessage (toucher, GOTSUPER);
		sound = 1;
		break;
		
	  case SPR_MEGA:
		player->health = deh.MegasphereHealth;
		player->mo->health = player->health;
		P_GiveArmor (player,deh.BlueAC);
		PickupMessage (toucher, GOTMSPHERE);
		sound = 1;
		break;
		
		// cards
		// leave cards for everyone
	  case SPR_BKEY:
		if (!player->cards[it_bluecard])
			PickupMessage (toucher, GOTBLUECARD);
		P_GiveCard (player, it_bluecard);
		sound = 3;
		if (!multiplayer)
			break;
		return;
		
	  case SPR_YKEY:
		if (!player->cards[it_yellowcard])
			PickupMessage (toucher, GOTYELWCARD);
		P_GiveCard (player, it_yellowcard);
		sound = 3;
		if (!multiplayer)
			break;
		return;
		
	  case SPR_RKEY:
		if (!player->cards[it_redcard])
			PickupMessage (toucher, GOTREDCARD);
		P_GiveCard (player, it_redcard);
		sound = 3;
		if (!multiplayer)
			break;
		return;
		
	  case SPR_BSKU:
		if (!player->cards[it_blueskull])
			PickupMessage (toucher, GOTBLUESKUL);
		P_GiveCard (player, it_blueskull);
		sound = 3;
		if (!multiplayer)
			break;
		return;
		
	  case SPR_YSKU:
		if (!player->cards[it_yellowskull])
			PickupMessage (toucher, GOTYELWSKUL);
		P_GiveCard (player, it_yellowskull);
		sound = 3;
		if (!multiplayer)
			break;
		return;
		
	  case SPR_RSKU:
		if (!player->cards[it_redskull])
			PickupMessage (toucher, GOTREDSKULL);
		P_GiveCard (player, it_redskull);
		sound = 3;
		if (!multiplayer)
			break;
		return;
		
		// medikits, heals
	  case SPR_STIM:
		if (!P_GiveBody (player, 10))
			return;
		PickupMessage (toucher, GOTSTIM);
		break;
		
	  case SPR_MEDI:
		if (!P_GiveBody (player, 25))
			return;

		if (player->health < 25)
			PickupMessage (toucher, GOTMEDINEED);
		else
			PickupMessage (toucher, GOTMEDIKIT);
		break;

		
		// power ups
	  case SPR_PINV:
		if (!P_GivePower (player, pw_invulnerability))
			return;
		PickupMessage (toucher, GOTINVUL);
		sound = 1;
		break;
		
	  case SPR_PSTR:
		if (!P_GivePower (player, pw_strength))
			return;
		PickupMessage (toucher, GOTBERSERK);
		if (player->readyweapon != wp_fist)
			player->pendingweapon = wp_fist;
		sound = 1;
		break;
		
	  case SPR_PINS:
		if (!P_GivePower (player, pw_invisibility))
			return;
		PickupMessage (toucher, GOTINVIS);
		sound = 1;
		break;
		
	  case SPR_SUIT:
		if (!P_GivePower (player, pw_ironfeet))
			return;
		PickupMessage (toucher, GOTSUIT);
		sound = 1;
		break;
		
	  case SPR_PMAP:
		if (!P_GivePower (player, pw_allmap))
			return;
		PickupMessage (toucher, GOTMAP);
		sound = 1;
		break;
		
	  case SPR_PVIS:
		if (!P_GivePower (player, pw_infrared))
			return;
		PickupMessage (toucher, GOTVISOR);
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
		PickupMessage (toucher, GOTCLIP);
		break;
		
	  case SPR_AMMO:
		if (!P_GiveAmmo (player, am_clip,5))
			return;
		PickupMessage (toucher, GOTCLIPBOX);
		break;
		
	  case SPR_ROCK:
		if (!P_GiveAmmo (player, am_misl,1))
			return;
		PickupMessage (toucher, GOTROCKET);
		break;
		
	  case SPR_BROK:
		if (!P_GiveAmmo (player, am_misl,5))
			return;
		PickupMessage (toucher, GOTROCKBOX);
		break;
		
	  case SPR_CELL:
		if (!P_GiveAmmo (player, am_cell,1))
			return;
		PickupMessage (toucher, GOTCELL);
		break;
		
	  case SPR_CELP:
		if (!P_GiveAmmo (player, am_cell,5))
			return;
		PickupMessage (toucher, GOTCELLBOX);
		break;
		
	  case SPR_SHEL:
		if (!P_GiveAmmo (player, am_shell,1))
			return;
		PickupMessage (toucher, GOTSHELLS);
		break;
		
	  case SPR_SBOX:
		if (!P_GiveAmmo (player, am_shell,5))
			return;
		PickupMessage (toucher, GOTSHELLBOX);
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
		PickupMessage (toucher, GOTBACKPACK);
		break;
		
		// weapons
	  case SPR_BFUG:
		if (!P_GiveWeapon (player, wp_bfg, special->flags & MF_DROPPED))
			return;
		PickupMessage (toucher, GOTBFG9000);
		sound = 2;
		break;
		
	  case SPR_MGUN:
		if (!P_GiveWeapon (player, wp_chaingun, special->flags & MF_DROPPED))
			return;
		PickupMessage (toucher, GOTCHAINGUN);
		sound = 2;
		break;
		
	  case SPR_CSAW:
		if (!P_GiveWeapon (player, wp_chainsaw, special->flags & MF_DROPPED))
			return;
		PickupMessage (toucher, GOTCHAINSAW);
		sound = 2;
		break;
		
	  case SPR_LAUN:
		if (!P_GiveWeapon (player, wp_missile, special->flags & MF_DROPPED))
			return;
		PickupMessage (toucher, GOTLAUNCHER);
		sound = 2;
		break;
		
	  case SPR_PLAS:
		if (!P_GiveWeapon (player, wp_plasma, special->flags & MF_DROPPED))
			return;
		PickupMessage (toucher, GOTPLASMA);
		sound = 2;
		break;
		
	  case SPR_SHOT:
		if (!P_GiveWeapon (player, wp_shotgun, special->flags & MF_DROPPED))
			return;
		PickupMessage (toucher, GOTSHOTGUN);
		sound = 2;
		break;
				
	  case SPR_SGN2:
		if (!P_GiveWeapon (player, wp_supershotgun, special->flags & MF_DROPPED))
			return;
		PickupMessage (toucher, GOTSHOTGUN2);
		sound = 2;
		break;

	// [Toke - CTF - Core]

	  case SPR_BFLG: // [Toke - CTF] Blue flag
	  case SPR_RFLG: // [Toke - CTF] Red flag
	  case SPR_GFLG: // [Toke - CTF] Gold flag
	  case SPR_BDWN:
	  case SPR_RDWN:
	  case SPR_GDWN:
		return;														

	  default:
		I_Error ("P_SpecialThing: Unknown gettable thing %d\n", special->sprite);
	}
	
	if (special->flags & MF_COUNTITEM)
	{
		level.found_items++;
	}
	
	special->Destroy();

	player->bonuscount = BONUSADD;

	{
		AActor *ent = player->mo;

		// denis - only play own pickup sounds
		switch (sound) {
			case 0:
			case 3:
				S_Sound (ent, CHAN_ITEM, "misc/i_pkup", 1, ATTN_NORM);
				break;
			case 1:
				S_Sound (ent, CHAN_ITEM, "misc/p_pkup", 1,
					!ent ? ATTN_SURROUND : ATTN_NORM);
				break;
			case 2:
				S_Sound (ent, CHAN_ITEM, "misc/w_pkup", 1, ATTN_NORM);
				break;
		}
	}
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
	int	 mod;
	char *message;
	char gendermessage[1024];
	BOOL friendly;
	int  gender;

	if (!self->player)
		return;

	gender = self->player->userinfo.gender;

	// Treat voodoo dolls as unknown deaths
	if (inflictor && inflictor->player == self->player)
		MeansOfDeath = MOD_UNKNOWN;

	if (multiplayer && !deathmatch)
		MeansOfDeath |= MOD_FRIENDLY_FIRE;

	friendly = MeansOfDeath & MOD_FRIENDLY_FIRE;
	mod = MeansOfDeath & ~MOD_FRIENDLY_FIRE;
	message = NULL;

	switch (mod) {
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
			switch (mod) {
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
					if (mod == MOD_HIT) {
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
		Printf (PRINT_MEDIUM, "%s %s.\n", self->player->userinfo.netname, gendermessage);
		return;
	}

	if (attacker && attacker->player) {
		if (friendly) {
			int rnum = M_Random ();

			attacker->player->fragcount -= 2;
//			attacker->player->frags[attacker->player - players]++;
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
			switch (mod) {
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
		char work[256];

		SexMessage (message, gendermessage, gender);
		sprintf (work, "%%s %s\n", gendermessage);
		Printf (PRINT_MEDIUM, work, self->player->userinfo.netname,
				attacker->player->userinfo.netname);
		return;
	}

	SexMessage (OB_DEFAULT, gendermessage, gender);
	Printf (PRINT_MEDIUM, "%s %s.\n", self->player->userinfo.netname, gendermessage);
}


//
// P_KillMobj
//
void P_KillMobj (AActor *source, AActor *target, AActor *inflictor)
{
	AActor *mo;

	target->flags &= ~(MF_SHOOTABLE|MF_FLOAT|MF_SKULLFLY);

	if (target->type != MT_SKULL)
		target->flags &= ~MF_NOGRAVITY;

	target->flags |= MF_CORPSE|MF_DROPOFF;
	target->height >>= 2;

	// [RH] Also set the thing's tid to 0. [why?]
	target->tid = 0;

	if (target->player)
	{
		CTF_CheckFlags (*target->player);
		
		// [NightFang] - Added this, thought it would be cooler
		// [Fly] - No, it's not cooler
		// target->player->cheats = CF_CHASECAM;
		
		// [RH] Force a delay between death and respawn
		// [Toke] Lets not fuck up deathmatch tactics ok randy?
		
		target->flags &= ~MF_SOLID;
		target->player->playerstate = PST_DEAD;
		target->player->respawn_time = level.time; // vanilla immediate respawn
		P_DropWeapon (target->player);

		if (target == consoleplayer().camera && automapactive)
		{
			// don't die in auto map, switch view prior to dying
			AM_Stop ();
		}

		// play die sound
		if (target->player != &consoleplayer())
			A_PlayerScream(target);
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
	if (target->player && level.time)
		ClientObituary (target, inflictor, source);

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

	// denis - dropped things will spawn serverside
	if(serverside)
	{
		mo = new AActor (target->x, target->y, ONFLOORZ, item);
		mo->flags |= MF_DROPPED;	// special versions of items
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

// [Toke] This is no longer needed client-side
void P_DamageMobj (AActor *target, AActor *inflictor, AActor *source, int damage, int mod, int flags)
{
	if(!serverside)
		return;

	unsigned	ang;
	int 		saved;
	player_t*	player;
	fixed_t 	thrust;
	
	if ( !(target->flags & MF_SHOOTABLE) )
		return; // shouldn't happen...
				
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
		ang = R_PointToAngle2 ( inflictor->x,
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
		if(!deathmatch /*|| allowexit*/)
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
		//if (!friendlyfire && (teamplay || !deathmatch) && source && source->player && target != source &&
		//	target->player->userinfo.team == source->player->userinfo.team && (mod != MOD_TELEFRAG))
		//		damage = 0;
		
		player->health -= damage;		// mirror mobj health here for Dave
		
		if (player->health <= 0)
			player->health = 0;
		
		player->attacker = source ? source->ptr() : AActor::AActorPtr();		
		player->damagecount += damage;	// add damage after armor / invuln
		
		if (player->damagecount > 100)
			player->damagecount = 100;	// teleport stomp does 10k points...
	}
	
	// do the damage
	{
		target->health -= damage;
		if (target->health <= 0)
		{
			P_KillMobj (source, target, inflictor);
			return;
		}
	}
	
	{
		if ( (P_Random() < target->info->painchance)
			 && !(target->flags&MF_SKULLFLY) )
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
		}
	}
}


VERSION_CONTROL (p_interaction_cpp, "$Id$")

