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
//	Weapon sprite animation, weapon objects.
//	Action functions for weapons.
//
//-----------------------------------------------------------------------------

#include <stdlib.h>

#include "doomdef.h"
#include "d_event.h"

#include "c_cvars.h"

#include "m_random.h"
#include "p_local.h"
#include "s_sound.h"

#include "g_warmup.h"

// State.
#include "doomstat.h"
#include "p_pspr.h"

#include "p_unlag.h"
#include "m_wdlstats.h"

#define LOWERSPEED				FRACUNIT*6
#define RAISESPEED				FRACUNIT*6
#define WEAPONBOTTOM			128*FRACUNIT
#define WEAPONTOP				32*FRACUNIT

void A_FireRailgun(AActor* mo);

EXTERN_CVAR(sv_infiniteammo)
EXTERN_CVAR(sv_freelook)
EXTERN_CVAR(sv_allowpwo)
EXTERN_CVAR(co_fineautoaim)

const char *weaponnames[] =
{
	"Fist",
	"Pistol",
	"Shotgun",
	"Chaingun",
	"Rocket Launcher",
	"Plasma Gun",
	"BFG9000",
	"Chainsaw",
	"Super Shotgun"
};


enum weaponstate_t
{
	upstate,
	downstate,
	readystate,
	atkstate,
	unknownstate
};

void A_WeaponReady(AActor* mo);
void A_Raise(AActor* mo);
void A_Lower(AActor* mo);


//
// P_GetWeaponState
//
// Returns which state the player's ready weapon is in.
//
static weaponstate_t P_GetWeaponState(player_t* player)
{
	struct pspdef_s* psp = &player->psprites[player->psprnum];

	if (psp->state == NULL)
		return unknownstate;

	if (psp->state->action == A_WeaponReady)
		return readystate;
	if (psp->state->action == A_Lower)
		return downstate;
	if (psp->state->action == A_Raise)
		return upstate;

	// must be in one of the many attack states...
	return atkstate;
}


//
// P_CalculateWeaponBobX
//
// Returns the player's weapon position in the x-axis after applying movebob
// scaling.
//
fixed_t P_CalculateWeaponBobX(player_t* player, float scale_amount)
{
	struct pspdef_s* psp = &player->psprites[player->psprnum];

	weaponstate_t weaponstate = P_GetWeaponState(player);

	fixed_t center_sx = FRACUNIT;
	if (weaponstate != readystate && psp->state && psp->state->misc1)
		center_sx = psp->state->misc1 << FRACBITS;

	if (weaponstate == readystate)
	{
		unsigned int angle_index = (128 * level.time) & FINEMASK;
		return center_sx + scale_amount * FixedMul(player->bob, finecosine[angle_index]);
	}

	// scale the weapon's distance away from center
	return center_sx + scale_amount * (psp->sx - center_sx);
}


//
// P_CalculateWeaponBobY
//
// Returns the player's weapon position in the y-axis after applying movebob
// scaling.
//
fixed_t P_CalculateWeaponBobY(player_t* player, float scale_amount)
{
	struct pspdef_s* psp = &player->psprites[player->psprnum];

	weaponstate_t weaponstate = P_GetWeaponState(player);

	// return the actual weapon y-position when raising or lowering
	if (weaponstate == upstate || weaponstate == downstate)
		return psp->sy;

	fixed_t center_sy = WEAPONTOP;
	if (weaponstate != readystate && psp->state && psp->state->misc1)
		center_sy = psp->state->misc2 << FRACBITS;

	if (weaponstate == readystate)
	{
		unsigned int angle_index = ((128 * level.time) & FINEMASK) & (FINEANGLES / 2 - 1);
		return center_sy + scale_amount * FixedMul(player->bob, finesine[angle_index]);
	}

	// scale the weapon's distance away from center
	return center_sy + scale_amount * (psp->sy - center_sy);
}



//
// P_SetPsprite
//
void P_SetPsprite(player_t* player, int position, statenum_t stnum)
{
	pspdef_t* psp = &player->psprites[position];
    player->psprnum = position;

	do
	{
		if (!stnum)
		{
			// object removed itself
			psp->state = NULL;
			break;
		}

		if (stnum >= NUMSTATES)
			return;

		psp->state = &states[stnum];
		psp->tics = psp->state->tics;		// could be 0

		if (psp->state->misc1)
		{
			// coordinate set
			psp->sx = psp->state->misc1 << FRACBITS;
			psp->sy = psp->state->misc2 << FRACBITS;
		}

		// Call action routine.
		// Modified handling.
		if (psp->state->action)
		{
			if (!player->spectator && player->mo != NULL)
				psp->state->action(player->mo);

			if (!psp->state)
				break;
		}

		stnum = psp->state->nextstate;

	} while (!psp->tics);
	// an initial state of 0 could cycle through
}

//
// A_FireSound
//
void A_FireSound (player_t *player, const char *sound)
{
	if (!serverside && player->id != consoleplayer_id)
		return;

 	UV_SoundAvoidPlayer (player->mo, CHAN_WEAPON, sound, ATTN_NORM);
}

//
// P_BringUpWeapon
// Starts bringing the pending weapon up
// from the bottom of the screen.
// Uses player
//
void P_BringUpWeapon(player_t *player)
{
	if (player->pendingweapon == wp_nochange)
		player->pendingweapon = player->readyweapon;

	if (player->pendingweapon == wp_chainsaw)
		A_FireSound(player, "weapons/sawup");

	statenum_t newstate = weaponinfo[player->pendingweapon].upstate;

	player->pendingweapon = wp_nochange;
	player->psprites[ps_weapon].sy = WEAPONBOTTOM;
	P_SetPsprite(player, ps_weapon, newstate);
}

//
// P_EnoughAmmo
//
// Returns true if the player has enough ammo to switch to the specified
// weapon.  Note that this emulates vanilla Doom bugs relating to the amount
// of ammo needed to switch to the BFG and SSG if switching == true.
//
bool P_EnoughAmmo(player_t *player, weapontype_t weapon, bool switching = false)
{
	ammotype_t		ammotype = weaponinfo[weapon].ammotype;
	int				count = 1;	// default amount of ammo for most weapons

	// [SL] Fix for when DeHackEd doesn't patch minammo
	count = MAX(weaponinfo[weapon].minammo, weaponinfo[weapon].ammouse);

	// Vanilla Doom requires > 40 cells to switch to BFG and > 2 shells to
	// switch to SSG when current weapon is out of ammo due to a bug.
	if (switching && (weapon == wp_bfg || weapon == wp_supershotgun))
		count++;

	// Some do not need ammunition anyway.
	// Return if current ammunition sufficient.
	if (ammotype == am_noammo || player->ammo[ammotype] >= count)
		return true;

	return false;
}

//
// P_SwitchWeapon
//
// Changes to the player's most preferred weapon based on availibilty and ammo.
// Note that this emulates vanilla Doom bugs relating to the amount of ammo
// needed to switch to the BFG and SSG.
//
void P_SwitchWeapon(player_t *player)
{
	const byte *prefs;

	if ((multiplayer && !sv_allowpwo) || demoplayback || demorecording)
		prefs = UserInfo::weapon_prefs_default;
	else
		prefs = player->userinfo.weapon_prefs;

	// find which weapon has the highest preference among availible weapons
	size_t best_weapon_num = 0;
	for (size_t i = 0; i < NUMWEAPONS; i++)
	{
		if (player->weaponowned[i] &&
			P_EnoughAmmo(player, static_cast<weapontype_t>(i), true) &&
			prefs[i] > prefs[best_weapon_num])
		{
			best_weapon_num = i;
		}
	}

	weapontype_t best_weapon = static_cast<weapontype_t>(best_weapon_num);
	if (best_weapon != player->readyweapon && player->weaponowned[best_weapon])
	{
		// Switch to this weapon
		player->pendingweapon = best_weapon;
		// Now set appropriate weapon overlay.
		P_SetPsprite(player, ps_weapon, weaponinfo[player->readyweapon].downstate);
	}
}

//
// P_GetNextWeapon
//
// Returns the weapon that comes after the player's current weapon.
// If forward is true, the next higher numbered weapon is chosen.  If forward
// is false, the next lower numbered weapon is chosen.
//
weapontype_t P_GetNextWeapon(player_t *player, bool forward)
{
	if (player->readyweapon == NUMWEAPONS || player->pendingweapon == NUMWEAPONS)
		return wp_nochange;

	gitem_t *item;

	if (player->pendingweapon != wp_nochange)
		item = FindItem(weaponnames[player->pendingweapon]);
	else
		item = FindItem(weaponnames[player->readyweapon]);

	if (!item)
		return wp_nochange;

	int selected_weapon = ITEM_INDEX(item);

	for (int i = 1; i <= num_items; i++)
	{
		int index;
		if (forward)
			index = (selected_weapon + i) % num_items;
		else	// traverse backwards
			index = (selected_weapon + num_items - i) % num_items;

		if (!(itemlist[index].flags & IT_WEAPON))
			continue;
		if (!player->weaponowned[itemlist[index].offset])
			continue;
		if (!player->ammo[weaponinfo[itemlist[index].offset].ammotype])
			continue;
		if (itemlist[index].offset == wp_plasma && gamemode == shareware)
			continue;
		if (itemlist[index].offset == wp_bfg && gamemode == shareware)
			continue;
		if (itemlist[index].offset == wp_supershotgun && gamemode != commercial && gamemode != commercial_bfg )
			continue;
		return (weapontype_t)itemlist[index].offset;
	}

	return wp_nochange;
}

//
// P_CheckSwitchWeapon
//
// Returns true if the player's current weapon should be switched
// to the one indicated by weapon, based on the player's preference.
// Called by P_GiveWeapon when a player touches a weapon mapthing.
//
bool P_CheckSwitchWeapon(player_t *player, weapontype_t weapon)
{
	// Always switch - vanilla Doom behavior
	if ((multiplayer && !sv_allowpwo) ||
		player->userinfo.switchweapon == WPSW_ALWAYS ||
		demoplayback || demorecording)
	{
		return true;
	}

	// Never switch - player has to manually change themselves
	// Having no weapons because of ClearInventory/TakeInventory overrides this
	if (player->userinfo.switchweapon == WPSW_NEVER && player->readyweapon != NUMWEAPONS && player->pendingweapon != NUMWEAPONS)
		return false;

	weapontype_t currentweapon = (player->pendingweapon == wp_nochange)
			? player->readyweapon
			: player->pendingweapon;

	if (currentweapon == NUMWEAPONS)
		return true;

	// Use player's weapon preferences
	byte *prefs = player->userinfo.weapon_prefs;
	if (prefs[weapon] > prefs[currentweapon])
		return true;

	return false;
}


//
// P_CheckAmmo
// Returns true if there is enough ammo to shoot.
// If not, selects the next weapon to use.
//
BOOL P_CheckAmmo (player_t *player)
{
	if (P_EnoughAmmo(player, player->readyweapon))
		return true;

	// no enough ammo with the current weapon, choose another one
	P_SwitchWeapon(player);
	return false;
}

// denis - from Chocolate Doom
// Doom does not check the bounds of the ammo array.  As a result,
// it is possible to use an ammo type > 4 that overflows into the
// maxammo array and affects that instead.  Through dehacked, for
// example, it is possible to make a weapon that decreases the max
// number of ammo for another weapon.  Emulate this.

static void DecreaseAmmo(player_t *player)
{
	// [SL] 2012-06-17 - Don't decrease ammo for players we are viewing
	// The server will send the correct ammo
	if (!serverside && player->id != consoleplayer_id)
		return;

	if (!sv_infiniteammo)
	{
		ammotype_t ammonum = weaponinfo[player->readyweapon].ammotype;
		int amount = weaponinfo[player->readyweapon].ammouse;

		if (ammonum < NUMAMMO)
			player->ammo[ammonum] -= amount;
		else if (ammonum - NUMAMMO < NUMAMMO)
			player->maxammo[ammonum - NUMAMMO] -= amount;
	}
}

//
// P_FireWeapon.
//
void P_FireWeapon(player_t* player)
{
	// Prevent fire if you don't have any weapon, including fist. See DoClearInv - PCD_CLEARINVENTORY
	if (!P_CheckAmmo(player) || player->readyweapon == NUMWEAPONS)
		return;

	// [tm512] Send the client the weapon they just fired so
	// that they can fix any weapon desyncs that they get - apr 14 2012
	if (serverside && !clientside)
	{
		MSG_WriteMarker (&player->client.reliablebuf, svc_fireweapon);
		MSG_WriteByte (&player->client.reliablebuf, player->readyweapon);
		MSG_WriteLong (&player->client.reliablebuf, player->tic);
	}

	P_SetMobjState(player->mo, S_PLAY_ATK1);
	statenum_t newstatenum = weaponinfo[player->readyweapon].atkstate;
	P_SetPsprite(player, ps_weapon, newstatenum);
	P_NoiseAlert(player->mo, player->mo);
}


//
// P_DropWeapon
// Player died, so put the weapon away.
//
void P_DropWeapon(player_t* player)
{
	P_SetPsprite(player, ps_weapon, weaponinfo[player->readyweapon].downstate);
}


//
// A_WeaponReady
//
// The player can fire the weapon or change to another weapon at this time.
// Follows after getting weapon up, or after previous attack/fire sequence.
//
void A_WeaponReady(AActor* mo)
{
    player_t* player = mo->player;
    struct pspdef_s* psp = &player->psprites[player->psprnum];

	// get out of attack state
	if (player->mo->state == &states[S_PLAY_ATK1] || player->mo->state == &states[S_PLAY_ATK2])
		P_SetMobjState(player->mo, S_PLAY);

	if (player->readyweapon == wp_chainsaw && psp->state == &states[S_SAW])
		A_FireSound(player, "weapons/sawidle");

	// check for change -  if player is dead, put the weapon away
	if (player->pendingweapon != wp_nochange || player->health <= 0)
	{
		// change weapon (pending weapon should already be validated)
		statenum_t newstatenum = weaponinfo[player->readyweapon].downstate;
		P_SetPsprite(player, ps_weapon, newstatenum);
		return;
	}

	// check for fire - the missile launcher and bfg do not auto fire
	// [AM] Allow warmup to disallow weapon firing.
	if (player->cmd.buttons & BT_ATTACK && warmup.checkfireweapon())
	{
		if (!player->attackdown || (player->readyweapon != wp_missile && player->readyweapon != wp_bfg))
		{
			player->attackdown = true;
			P_FireWeapon(player);
			return;
		}
	}
	else
		player->attackdown = false;

	// bob the weapon based on movement speed
	psp->sx = P_CalculateWeaponBobX(player, 1.0f);
	psp->sy = P_CalculateWeaponBobY(player, 1.0f);
}

//
// A_ReFire
// The player can re-fire the weapon
// without lowering it entirely.
//
void A_ReFire(AActor* mo)
{
    player_t *player = mo->player;

	// check for fire
	//	(if a weaponchange is pending, let it go through instead)
	// [AM] Allow warmup to disallow weapon refiring.
	if ((player->cmd.buttons & BT_ATTACK && warmup.checkfireweapon())
		 && player->pendingweapon == wp_nochange
		 && player->health)
	{
		player->refire++;
		P_FireWeapon(player);
	}
	else
	{
		player->refire = 0;
		P_CheckAmmo(player);
	}
}


void A_CheckReload(AActor* mo)
{
	P_CheckAmmo(mo->player);
}

//
// A_Lower
// Lowers current weapon,
//  and changes weapon at bottom.
//
void A_Lower(AActor* mo)
{
    player_t* player = mo->player;
    struct pspdef_s* psp = &player->psprites[player->psprnum];

	psp->sy += LOWERSPEED;

	// Not yet lowered to the bottom
	if (psp->sy < WEAPONBOTTOM)
		return;

	// Player is dead.
	if (player->playerstate == PST_DEAD)
	{
		psp->sy = WEAPONBOTTOM;
		return;
	}

	// The old weapon has been lowered off the screen,
	// so change the weapon and start raising it
	if (player->health <= 0)
	{
		// Player is dead, so keep the weapon off screen.
		P_SetPsprite(player, ps_weapon, S_NULL);
		return;
	}

	// haleyjd 03/28/10: do not assume pendingweapon is valid - include NUMWEAPONS from ClearInventory
	if (player->pendingweapon < NUMWEAPONS + 1)
		player->readyweapon = player->pendingweapon;

	P_BringUpWeapon(player);
}

//
// A_Raise
//
void A_Raise(AActor* mo)
{
    player_t *player = mo->player;
    struct pspdef_s *psp = &player->psprites[player->psprnum];

	psp->sy -= RAISESPEED;

	if (psp->sy > WEAPONTOP)
		return;

	psp->sy = WEAPONTOP;

	// The weapon has been raised all the way,
	//	so change to the ready state.
	statenum_t newstate = weaponinfo[player->readyweapon].readystate;

	P_SetPsprite(player, ps_weapon, newstate);
}



//
// A_GunFlash
//
void A_GunFlash(AActor* mo)
{
    player_t *player = mo->player;

	P_SetMobjState(player->mo, S_PLAY_ATK2);
	P_SetPsprite(player, ps_flash, weaponinfo[player->readyweapon].flashstate);
}



//
// WEAPON ATTACKS
//

//
// A_Punch
//
void A_Punch(AActor* mo)
{
	angle_t 	angle;
	int 		damage;
	int 		slope;

    player_t *player = mo->player;

	damage = (P_Random (player->mo)%10+1)<<1;

	if (player->powers[pw_strength])
		damage *= 10;

	angle = player->mo->angle;
	angle += P_RandomDiff(player->mo) << 18;

	// [SL] 2011-07-12 - Move players and sectors back to their positions when
	// this player hit the fire button clientside.
	Unlag::getInstance().reconcile(player->id);

	slope = P_AimLineAttack (player->mo, angle, MELEERANGE);
	P_LineAttack (player->mo, angle, MELEERANGE, slope, damage);

	// [SL] 2011-07-12 - Restore players and sectors to their current position
	// according to the server.
	Unlag::getInstance().restore(player->id);

	// turn to face target
	if (linetarget)
	{
		A_FireSound (player, "player/male/fist");
		player->mo->angle = P_PointToAngle (player->mo->x, player->mo->y, linetarget->x, linetarget->y);
	}
}


//
// A_Saw
//
void A_Saw(AActor* mo)
{
	angle_t 	angle;
	int 		damage;

    player_t *player = mo->player;

	damage = 2 * (P_Random (player->mo)%10+1);
	angle = player->mo->angle;
	angle += P_RandomDiff(player->mo) << 18;

	// [SL] 2011-07-12 - Move players and sectors back to their positions when
	// this player hit the fire button clientside.
	Unlag::getInstance().reconcile(player->id);

	// use meleerange + 1 so the puff doesn't skip the flash
	P_LineAttack (player->mo, angle, MELEERANGE+1,
				  P_AimLineAttack (player->mo, angle, MELEERANGE+1), damage);

	// [SL] 2011-07-12 - Restore players and sectors to their current position
	// according to the server.
	Unlag::getInstance().restore(player->id);

	if (!linetarget)
	{
		A_FireSound (player, "weapons/sawfull");
		return;
	}
	A_FireSound (player, "weapons/sawhit");

	// turn to face target
	angle = P_PointToAngle (player->mo->x, player->mo->y,
							 linetarget->x, linetarget->y);
	if (angle - player->mo->angle > ANG180)
	{
		if (angle - player->mo->angle < (angle_t)(-ANG90/20))
			player->mo->angle = angle + ANG90/21;
		else
			player->mo->angle -= ANG90/20;
	}
	else
	{
		if (angle - player->mo->angle > ANG90/20)
			player->mo->angle = angle - ANG90/21;
		else
			player->mo->angle += ANG90/20;
	}
	player->mo->flags |= MF_JUSTATTACKED;
}



//
// A_FireMissile
//
void A_FireMissile(AActor* mo)
{
    player_t *player = mo->player;

	DecreaseAmmo(player);

	if(serverside)
		P_SpawnPlayerMissile (player->mo, MT_ROCKET);
}


//
// A_FireBFG
//

void A_FireBFG(AActor* mo)
{
    player_t *player = mo->player;

	// [RH] bfg can be forced to not use freeaim
	angle_t storedpitch = player->mo->pitch;
	int storedaimdist = player->userinfo.aimdist;

	DecreaseAmmo(player);

	player->mo->pitch = 0;
	player->userinfo.aimdist = 81920000;

	if(serverside)
		P_SpawnPlayerMissile (player->mo, MT_BFG);

	player->mo->pitch = storedpitch;
	player->userinfo.aimdist = storedaimdist;
}



//
// A_FirePlasma
//
void A_FirePlasma(AActor* mo)
{
    player_t *player = mo->player;

	DecreaseAmmo(player);

	P_SetPsprite (player,
				  ps_flash,
				  (statenum_t)(weaponinfo[player->readyweapon].flashstate+(P_Random (player->mo)&1)));

	if(serverside)
		P_SpawnPlayerMissile (player->mo, MT_PLASMA);
}

//
// [RH] A_FireRailgun
//
static int RailOffset;

void A_FireRailgun(AActor* mo)
{
	int damage;

    player_t *player = mo->player;
	DecreaseAmmo(player);

	P_SetPsprite (player,
				  ps_flash,
				  (statenum_t)(weaponinfo[player->readyweapon].flashstate+(P_Random (player->mo)&1)));

	if (sv_gametype > 0)
		damage = 100;
	else
		damage = 150;

	// [SL] 2012-04-18 - Move players and sectors back to their positions when
	// this player hit the fire button clientside.
	Unlag::getInstance().reconcile(player->id);

	P_RailAttack (player->mo, damage, RailOffset);

	// [SL] 2012-04-18 - Restore players and sectors to their current position
	// according to the server.
	Unlag::getInstance().restore(player->id);

	RailOffset = 0;
}

void A_FireRailgunRight(AActor* mo)
{
	RailOffset = 10;
	A_FireRailgun (mo);
}

void A_FireRailgunLeft(AActor* mo)
{
	RailOffset = -10;
	A_FireRailgun (mo);
}

void A_RailWait(AActor* mo)
{
}

//
// P_BulletSlope
// Sets a slope so a near miss is at aproximately
// the height of the intended target
//

fixed_t P_BulletSlope(AActor* mo)
{
	fixed_t pitchslope = finetangent[FINEANGLES/4 - (mo->pitch>>ANGLETOFINESHIFT)];
	fixed_t bulletslope;

	// see which target is to be aimed at
	angle_t an = mo->angle;

	// [AM] Refactored autoaim into a single function.
	if (co_fineautoaim)
		bulletslope = P_AutoAimLineAttack(mo, an, 1 << 26, 10, 16 * 64 * FRACUNIT);
	else
		bulletslope = P_AutoAimLineAttack(mo, an, 1 << 26, 1, 16 * 64 * FRACUNIT);

	// If a target was not found, or one was found, but outside the
	// player's autoaim range, use the actor's pitch for the slope.
	if (sv_freelook &&
		(!linetarget || // target not found, or:
		 (mo->player && // target found but outside of player's autoaim range
		  abs(bulletslope - pitchslope) >= mo->player->userinfo.aimdist)))
	{
		bulletslope = pitchslope;
	}

	return bulletslope;
}

//
// P_FireHitscan
//
// [SL] - Factored out common code from the P_Fire procedures for the
// hitscan weapons.
//   quantity:	number of bullets/pellets to fire.
//   spread:	spread the fire in a pattern dictated by weapon type
//
// It takes care of reconciling the players and sectors to account for the
// shooter's network lag.
//

typedef enum
{
	SPREAD_NONE,
	SPREAD_NORMAL,
	SPREAD_SUPERSHOTGUN
} spreadtype_t;

void P_FireHitscan (player_t *player, size_t quantity, spreadtype_t spread)
{
	if (!player || !player->mo)
		return;

	bool predict_puffs = clientside && !serverside &&
						 consoleplayer().userinfo.predict_weapons;

	if (!serverside && !predict_puffs)
		return;

	// [SL] 2012-10-02 - To ensure bullet puff prediction on the client matches
	// the server, use the client's gametic as the initial PRNG index
	if (serverside && !clientside)
		player->mo->rndindex = player->tic;
	else if (predict_puffs)
		player->mo->rndindex = gametic;

	// [SL] 2011-05-11 - Move players and sectors back to their positions when
	// this player hit the fire button clientside.
	// NOTE: Important to reconcile sectors and players BEFORE calculating
	// bulletslope!
	if (serverside)
		Unlag::getInstance().reconcile(player->id);

	fixed_t bulletslope = P_BulletSlope(player->mo);

	for (size_t i=0; i<quantity; i++)
	{
		int damage = 5 * (P_Random(player->mo) % 3 + 1);

		// [SL] Don't do damage if the client is predicting bullet puffs
		if (predict_puffs)
			damage = 0;

		angle_t angle = player->mo->angle;
		fixed_t slope = bulletslope;
		if (spread == SPREAD_SUPERSHOTGUN)
		{
			angle += P_RandomDiff(player->mo) << 19;
			slope += P_RandomDiff(player->mo) << 5;
		}
		else if (spread == SPREAD_NORMAL)
		{
			// single-barrel shotgun or re-firing pistol/chaingun
			angle += P_RandomDiff(player->mo) << 18;
		}
		P_LineAttack(player->mo, angle, MISSILERANGE, slope, damage);
	}

	// [SL] 2011-05-11 - Restore players and sectors to their current position
	// according to the server.
	if (serverside)
		Unlag::getInstance().restore(player->id);
}

//
// A_FirePistol
//
void A_FirePistol(AActor* mo)
{
    player_t *player = mo->player;

	A_FireSound (player, "weapons/pistol");

	P_SetMobjState (player->mo, S_PLAY_ATK2);

	DecreaseAmmo(player);

	P_SetPsprite (player,
				  ps_flash,
				  weaponinfo[player->readyweapon].flashstate);

	spreadtype_t accuracy = player->refire ? SPREAD_NORMAL : SPREAD_NONE;
	P_FireHitscan (player, 1, accuracy);

	M_MaybeLogWDLAccuracyMiss(player, player->mo->angle / 4, wp_pistol);
}


//
// A_FireShotgun
//
void A_FireShotgun(AActor* mo)
{
    player_t *player = mo->player;

	A_FireSound (player, "weapons/shotgf");
	P_SetMobjState (player->mo, S_PLAY_ATK2);

	DecreaseAmmo(player);

	P_SetPsprite (player,
				  ps_flash,
				  weaponinfo[player->readyweapon].flashstate);

	P_FireHitscan (player, 7, SPREAD_NORMAL);

	M_MaybeLogWDLAccuracyMiss(player, player->mo->angle / 4, wp_shotgun);
}



//
// A_FireShotgun2
//
void A_FireShotgun2(AActor* mo)
{
    player_t *player = mo->player;

	A_FireSound (player, "weapons/sshotf");
	P_SetMobjState (player->mo, S_PLAY_ATK2);

	DecreaseAmmo(player);

	P_SetPsprite (player,
				  ps_flash,
				  weaponinfo[player->readyweapon].flashstate);

	P_FireHitscan (player, 20, SPREAD_SUPERSHOTGUN);

	M_MaybeLogWDLAccuracyMiss(player, player->mo->angle / 4, wp_supershotgun);
}

//
// A_FireCGun
//
void A_FireCGun(AActor* mo)
{
    player_t *player = mo->player;
    struct pspdef_s *psp = &player->psprites[player->psprnum];

	A_FireSound (player, "weapons/chngun");

	if (weaponinfo[player->readyweapon].ammotype != am_noammo
		&& !player->ammo[weaponinfo[player->readyweapon].ammotype])
		return;

	P_SetMobjState (player->mo, S_PLAY_ATK2);

	DecreaseAmmo(player);

	P_SetPsprite (player,
				  ps_flash,
				  (statenum_t)(weaponinfo[player->readyweapon].flashstate
				  + psp->state
				  - &states[S_CHAIN1]) );

	spreadtype_t accuracy = player->refire ? SPREAD_NORMAL : SPREAD_NONE;
	P_FireHitscan (player, 1, accuracy);

	M_MaybeLogWDLAccuracyMiss(player, player->mo->angle / 4, wp_chaingun);
}



void A_Light0(AActor* mo)
{
	if (mo->player)
        mo->player->extralight = 0;
}

void A_Light1(AActor* mo)
{
	if (mo->player)
        mo->player->extralight = 1;
}

void A_Light2(AActor* mo)
{
	if (mo->player)
        mo->player->extralight = 2;
}


//
// A_BFGSpray
// Spawn a BFG explosion on every monster in view
//
void A_BFGSpray(AActor* mo)
{
	int 				i;
	int 				j;
	int 				damage;
	angle_t 			an;

	if(!serverside)
		return;

	// note: mo->target is the AActor of the player who fired the BFG
	if (!mo->target)
		return;

	// offset angles from its attack angle
	for (i=0 ; i<40 ; i++)
	{
		an = mo->angle - ANG90/2 + ANG90/40*i;

		// mo->target is the originator (player)
		//	of the missile
		P_AimLineAttack (mo->target, an, 16*64*FRACUNIT);

		if (!linetarget)
			continue;

		new AActor (linetarget->x,
					linetarget->y,
					linetarget->z + (linetarget->height>>2),
					MT_EXTRABFG);

		damage = 0;
		for (j=0;j<15;j++)
			damage += (P_Random (mo) & 7) + 1;

		P_DamageMobj (linetarget, mo->target,mo->target, damage, MOD_BFG_SPLASH);
	}
}


//
// A_BFGsound
//
void A_BFGsound(AActor* mo)
{
	A_FireSound(mo->player, "weapons/bfgf");
}


//
// P_SetupPsprites
// Called at start of level for each player.
//
void P_SetupPsprites(player_t* player)
{
	// remove all psprites
	for (int i = 0; i < NUMPSPRITES; i++)
		player->psprites[i].state = NULL;

	// spawn the gun
	player->pendingweapon = player->readyweapon;
	P_BringUpWeapon(player);
}

//
// P_MovePsprites
//
// Called every tic by player thinking routine.
//
void P_MovePsprites(player_t* player)
{
	for (int i = 0; i < NUMPSPRITES; i++)
	{
		pspdef_t* psp = &player->psprites[i];

		// a null state means not active
		if (psp->state)
		{
			// drop tic count and possibly change state
			// a -1 tic count never changes

			if (psp->tics != -1)
			{
				psp->tics--;

				if (psp->tics == 0)
					P_SetPsprite(player, i, psp->state->nextstate);
			}
		}
	}

	player->psprites[ps_flash].sx = player->psprites[ps_weapon].sx;
	player->psprites[ps_flash].sy = player->psprites[ps_weapon].sy;
}

void A_OpenShotgun2(AActor* mo)
{
    player_t *player = mo->player;

	A_FireSound(player, "weapons/sshoto");
}

void A_LoadShotgun2(AActor* mo)
{
    player_t *player = mo->player;

	A_FireSound(player, "weapons/sshotl");
}

void A_CloseShotgun2(AActor* mo)
{
    player_t *player = mo->player;

	A_FireSound(player, "weapons/sshotc");
	A_ReFire(mo);
}


//
// A_ForceWeaponFire
//
// Immediately changes a players weapon to a new weapon and new animation state
//
void A_ForceWeaponFire(AActor* mo, weapontype_t weapon, int tic)
{
	if (!mo || !mo->player)
		return;

	player_t *player = mo->player;

	player->weaponowned[weapon] = true;
	player->readyweapon = weapon;

	P_SetPsprite(player, ps_weapon, weaponinfo[player->readyweapon].atkstate);

	while (tic++ < gametic)
		P_MovePsprites(player);
}

FArchive &operator<< (FArchive &arc, pspdef_t &def)
{
	return arc << def.state << def.tics << def.sx << def.sy;
}

FArchive &operator>> (FArchive &arc, pspdef_t &def)
{
	return arc >> def.state >> def.tics >> def.sx >> def.sy;
}

VERSION_CONTROL (p_pspr_cpp, "$Id$")
