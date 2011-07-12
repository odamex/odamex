// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 2006-2010 by The Odamex Team.
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

// State.
#include "doomstat.h"
#include "p_pspr.h"

#include "p_unlag.h"

#define LOWERSPEED				FRACUNIT*6
#define RAISESPEED				FRACUNIT*6

#define WEAPONBOTTOM			128*FRACUNIT
#define WEAPONTOP				32*FRACUNIT

EXTERN_CVAR(sv_infiniteammo)
EXTERN_CVAR(sv_freelook)
EXTERN_CVAR(sv_allownobob)
EXTERN_CVAR(cl_nobob)

//
// P_SetPsprite
//
void
P_SetPsprite
( player_t*	player,
  int		position,
  statenum_t	stnum )
{
	pspdef_t*	psp;
	state_t*	state;

	psp = &player->psprites[position];

	do
	{
		if (!stnum)
		{
			// object removed itself
			psp->state = NULL;
			break;
		}

		if(stnum >= NUMSTATES)
			return;

		state = &states[stnum];
		psp->state = state;
		psp->tics = state->tics;		// could be 0

		if (state->misc1)
		{
			// coordinate set
			psp->sx = state->misc1 << FRACBITS;
			psp->sy = state->misc2 << FRACBITS;
		}

		// Call action routine.
		// Modified handling.
		if (state->action.acp2)
		{
			if(!player->spectator)
				state->action.acp2(player, psp);
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
 	UV_SoundAvoidPlayer (player->mo, CHAN_WEAPON, sound, ATTN_NORM);
}

//
// P_BringUpWeapon
// Starts bringing the pending weapon up
// from the bottom of the screen.
// Uses player
//
void P_BringUpWeapon (player_t *player)
{
	statenum_t	newstate;

	if (player->pendingweapon == wp_nochange)
		player->pendingweapon = player->readyweapon;

	if (player->pendingweapon == wp_chainsaw)
		A_FireSound(player, "weapons/sawup");

	newstate = weaponinfo[player->pendingweapon].upstate;

	player->pendingweapon = wp_nochange;
	player->psprites[ps_weapon].sy = WEAPONBOTTOM;

	P_SetPsprite (player, ps_weapon, newstate);
}

//
// P_CheckAmmo
// Returns true if there is enough ammo to shoot.
// If not, selects the next weapon to use.
//
BOOL P_CheckAmmo (player_t *player)
{
	ammotype_t			ammo;
	int 				count;

	ammo = weaponinfo[player->readyweapon].ammo;

	// Minimal amount for one shot varies.
	if (player->readyweapon == wp_bfg)
		count = deh.BFGCells;
	else if (player->readyweapon == wp_supershotgun)
		count = 2;		// Double barrel.
	else
		count = 1;		// Regular.

	// Some do not need ammunition anyway.
	// Return if current ammunition sufficient.
	if (ammo == am_noammo || player->ammo[ammo] >= count)
		return true;

	// Out of ammo, pick a weapon to change to.
	// Preferences are set here.
	do
	{
		if (player->weaponowned[wp_plasma]
			&& player->ammo[am_cell])
		{
			player->pendingweapon = wp_plasma;
		}
		else if (player->weaponowned[wp_supershotgun]
				 && player->ammo[am_shell]>2)
		{
			player->pendingweapon = wp_supershotgun;
		}
		else if (player->weaponowned[wp_chaingun]
				 && player->ammo[am_clip])
		{
			player->pendingweapon = wp_chaingun;
		}
		else if (player->weaponowned[wp_shotgun]
				 && player->ammo[am_shell])
		{
			player->pendingweapon = wp_shotgun;
		}
		else if (player->ammo[am_clip])
		{
			player->pendingweapon = wp_pistol;
		}
		else if (player->weaponowned[wp_chainsaw])
		{
			player->pendingweapon = wp_chainsaw;
		}
		else if (player->weaponowned[wp_missile]
				 && player->ammo[am_misl])
		{
			player->pendingweapon = wp_missile;
		}
		else if (player->weaponowned[wp_bfg]
				 && player->ammo[am_cell]>40)
		{
			player->pendingweapon = wp_bfg;
		}
		else
		{
			// If everything fails.
			player->pendingweapon = wp_fist;
		}

	} while (player->pendingweapon == wp_nochange);


	// Now set appropriate weapon overlay.
	P_SetPsprite (player,
				  ps_weapon,
				  weaponinfo[player->readyweapon].downstate);

	return false;
}

// denis - from Chocolate Doom
// Doom does not check the bounds of the ammo array.  As a result,
// it is possible to use an ammo type > 4 that overflows into the
// maxammo array and affects that instead.  Through dehacked, for
// example, it is possible to make a weapon that decreases the max
// number of ammo for another weapon.  Emulate this.

static void DecreaseAmmo(player_t *player, int amount)
{
	if (!sv_infiniteammo)
	{
		ammotype_t ammonum = weaponinfo[player->readyweapon].ammo;

		if (ammonum < NUMAMMO)
		{
			player->ammo[ammonum] -= amount;
		}
		else
		{
			// denis - within reason!
			if (ammonum - NUMAMMO < NUMAMMO)
			{
				player->maxammo[ammonum - NUMAMMO] -= amount;
			}
		}
	}
}

//
// P_FireWeapon.
//
void P_FireWeapon (player_t *player)
{
	statenum_t	newstate;

	if (!P_CheckAmmo (player))
		return;

	P_SetMobjState (player->mo, S_PLAY_ATK1);
	newstate = weaponinfo[player->readyweapon].atkstate;
	P_SetPsprite (player, ps_weapon, newstate);
	P_NoiseAlert (player->mo, player->mo);
}



//
// P_DropWeapon
// Player died, so put the weapon away.
//
void P_DropWeapon (player_t *player)
{
	P_SetPsprite (player,
				  ps_weapon,
				  weaponinfo[player->readyweapon].downstate);
}



//
// A_WeaponReady
// The player can fire the weapon
// or change to another weapon at this time.
// Follows after getting weapon up,
// or after previous attack/fire sequence.
//
void
A_WeaponReady
( player_t*	player,
  pspdef_t*	psp )
{
	statenum_t	newstate;
	int 		angle;

	// get out of attack state
	if (player->mo->state == &states[S_PLAY_ATK1]
		|| player->mo->state == &states[S_PLAY_ATK2] )
	{
		P_SetMobjState (player->mo, S_PLAY);
	}

	if (player->readyweapon == wp_chainsaw
		&& psp->state == &states[S_SAW])
	{
		A_FireSound(player, "weapons/sawidle");
	}

	// check for change
	//	if player is dead, put the weapon away
	if (player->pendingweapon != wp_nochange || player->health <= 0)
	{
		// change weapon
		//	(pending weapon should already be validated)
		newstate = weaponinfo[player->readyweapon].downstate;
		P_SetPsprite (player, ps_weapon, newstate);
		return;
	}

	// check for fire
	//	the missile launcher and bfg do not auto fire
	if (player->cmd.ucmd.buttons & BT_ATTACK)
	{
		if ( !player->attackdown
			 || (player->readyweapon != wp_missile
				 && player->readyweapon != wp_bfg) )
		{
			player->attackdown = true;
			P_FireWeapon (player);
			return;
		}
	}
	else
		player->attackdown = false;

	// bob the weapon based on movement speed
	if ((!multiplayer && !cl_nobob) || (multiplayer && (!sv_allownobob || !cl_nobob))) 
	{
		angle = (128*level.time)&FINEMASK;
		psp->sx = FRACUNIT + FixedMul (player->bob, finecosine[angle]);
		angle &= FINEANGLES/2-1;
		psp->sy = WEAPONTOP + FixedMul (player->bob, finesine[angle]);
	}
}

//
// A_ReFire
// The player can re-fire the weapon
// without lowering it entirely.
//
void A_ReFire
( player_t*	player,
  pspdef_t*	psp )
{

	// check for fire
	//	(if a weaponchange is pending, let it go through instead)
	if ( (player->cmd.ucmd.buttons & BT_ATTACK)
		 && player->pendingweapon == wp_nochange
		 && player->health)
	{
		player->refire++;
		P_FireWeapon (player);
	}
	else
	{
		player->refire = 0;
		P_CheckAmmo (player);
	}
}


void
A_CheckReload
( player_t*	player,
  pspdef_t*	psp )
{
	P_CheckAmmo (player);
#if 0
	if (player->ammo[am_shell]<2)
		P_SetPsprite (player, ps_weapon, S_DSNR1);
#endif
}

//
// A_Lower
// Lowers current weapon,
//  and changes weapon at bottom.
//
void
A_Lower
( player_t*	player,
  pspdef_t*	psp )
{
	psp->sy += LOWERSPEED;

	// Is already down.
	if (psp->sy < WEAPONBOTTOM )
		return;

	// Player is dead.
	if (player->playerstate == PST_DEAD)
	{
		psp->sy = WEAPONBOTTOM;

		// don't bring weapon back up
		return;
	}

	// The old weapon has been lowered off the screen,
	// so change the weapon and start raising it
	if (player->health <= 0)
	{
		// Player is dead, so keep the weapon off screen.
		P_SetPsprite (player,  ps_weapon, S_NULL);
		return;
	}
	
	// haleyjd 03/28/10: do not assume pendingweapon is valid
	if (player->pendingweapon < NUMWEAPONS)
		player->readyweapon = player->pendingweapon;

	P_BringUpWeapon (player);
}

//
// A_Raise
//
void
A_Raise
( player_t*	player,
  pspdef_t*	psp )
{
	statenum_t	newstate;

	psp->sy -= RAISESPEED;

	if (psp->sy > WEAPONTOP )
		return;

	psp->sy = WEAPONTOP;

	// The weapon has been raised all the way,
	//	so change to the ready state.
	newstate = weaponinfo[player->readyweapon].readystate;

	P_SetPsprite (player, ps_weapon, newstate);
}



//
// A_GunFlash
//
void A_GunFlash (player_t *player, pspdef_t *psp)
{
	P_SetMobjState (player->mo, S_PLAY_ATK2);
	P_SetPsprite (player, ps_flash, weaponinfo[player->readyweapon].flashstate);
}



//
// WEAPON ATTACKS
//

//
// A_Punch
//
void A_Punch (player_t *player, pspdef_t *psp)
{
	angle_t 	angle;
	int 		damage;
	int 		slope;

	// [SL] 2011-07-12 - Move players and sectors back to their positions when
	// this player hit the fire button clientside.
	Unlag::getInstance().reconcile(player->id);

	damage = (P_Random (player->mo)%10+1)<<1;

	if (player->powers[pw_strength])
		damage *= 10;

	angle = player->mo->angle;

	angle += P_RandomDiff(player->mo) << 18;
	slope = P_AimLineAttack (player->mo, angle, MELEERANGE);
	P_LineAttack (player->mo, angle, MELEERANGE, slope, damage);

	// turn to face target
	if (linetarget)
	{
		A_FireSound (player, "player/male/fist");
		//S_Sound (player->mo, CHAN_VOICE, "*fist", 1, ATTN_NORM);
		player->mo->angle = P_PointToAngle (player->mo->x,
											player->mo->y,
											linetarget->x,
											linetarget->y);
	}

	// [SL] 2011-07-12 - Restore players and sectors to their current position
	// according to the server.
	Unlag::getInstance().restore(player->id);
}


//
// A_Saw
//
void A_Saw (player_t *player, pspdef_t *psp)
{
	angle_t 	angle;
	int 		damage;

	// [SL] 2011-07-12 - Move players and sectors back to their positions when
	// this player hit the fire button clientside.
	Unlag::getInstance().reconcile(player->id);

	damage = 2 * (P_Random (player->mo)%10+1);
	angle = player->mo->angle;
	angle += P_RandomDiff(player->mo) << 18;

	// use meleerange + 1 so the puff doesn't skip the flash
	P_LineAttack (player->mo, angle, MELEERANGE+1,
				  P_AimLineAttack (player->mo, angle, MELEERANGE+1), damage);

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

	// [SL] 2011-07-12 - Restore players and sectors to their current position
	// according to the server.
	Unlag::getInstance().restore(player->id);
}



//
// A_FireMissile
//
void A_FireMissile (player_t *player, pspdef_t *psp)
{
	DecreaseAmmo(player, 1);

	if(serverside)
		P_SpawnPlayerMissile (player->mo, MT_ROCKET);
}


//
// A_FireBFG
//

void A_FireBFG (player_t *player, pspdef_t *psp)
{
	// [RH] bfg can be forced to not use freeaim
	angle_t storedpitch = player->mo->pitch;
	int storedaimdist = player->userinfo.aimdist;

	DecreaseAmmo(player, deh.BFGCells);

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
void A_FirePlasma (player_t *player, pspdef_t *psp)
{
	DecreaseAmmo(player, 1);

	P_SetPsprite (player,
				  ps_flash,
				  (statenum_t)(weaponinfo[player->readyweapon].flashstate+(P_Random (player->mo)&1)));

	if(serverside)
		P_SpawnPlayerMissile (player->mo, MT_PLASMA);
}


//
// P_BulletSlope
// Sets a slope so a near miss is at aproximately
// the height of the intended target
//
fixed_t bulletslope;

void P_BulletSlope (AActor *mo)
{
	angle_t 	an;
	fixed_t		pitchslope;

	pitchslope = finetangent[FINEANGLES/4-(mo->pitch>>ANGLETOFINESHIFT)];

	// see which target is to be aimed at
	an = mo->angle;
	bulletslope = P_AimLineAttack (mo, an, 16*64*FRACUNIT);

	// GhostlyDeath <June 19, 2008> -- Autoaim bug here!
	if (!linetarget)	// Autoaim missed something
	{
		an += 1<<26;
		bulletslope = P_AimLineAttack (mo, an, 16*64*FRACUNIT);
		if (!linetarget)	// Still missing something
		{
			an -= 2<<26;
			bulletslope = P_AimLineAttack (mo, an, 16*64*FRACUNIT);
			// [RH] If we never found a target, use actor's pitch to
			// determine bulletslope
			if (sv_freelook && !linetarget)
			{
				an = mo->angle;
				bulletslope = pitchslope;
			}
		}
	}

	// GhostlyDeath -- If sv_freelook was on and a line target was found
	// and the shooting object is a player, we use the looking, so shouldn't
	// linetarget become !linetarget and moved up!?
	if (sv_freelook && !linetarget && mo->player)
	{
		if (abs(bulletslope - pitchslope) > mo->player->userinfo.aimdist)
		{
			bulletslope = pitchslope;
			an = mo->angle;
		}
	}
}


//
// P_GunShot
//
void P_GunShot (AActor *mo, BOOL accurate)
{
	angle_t 	angle;
	int 		damage;

	damage = 5*(P_Random (mo)%3+1);
	angle = mo->angle;

	if (!accurate)
		angle += P_RandomDiff(mo) << 18;

	P_LineAttack (mo, angle, MISSILERANGE, bulletslope, damage);
}

// P_FireHitscan
//
// [SL] - Factored out common code from the P_Fire procedures for the
// hitscan weapons.  
//   quantity:     number of bullets/pellets to fire.
//   accurate:     spread out bullets because player is re-firing (or using shotgun)
//   ssg_spread:   spread the pellets for the super shotgun
//
// It takes care of reconciling the players and sectors to account for the
// shooter's network lag.
//

void P_FireHitscan (player_t *player, size_t quantity, bool accurate, bool ssg_spread)
{
	if (!serverside)
		return;

	// [SL] 2011-05-11 - Move players and sectors back to their positions when
	// this player hit the fire button clientside.
	// NOTE: Important to reconcile sectors and players BEFORE calculating
	// bulletslope!
	Unlag::getInstance().reconcile(player->id);

	P_BulletSlope (player->mo);
	for (size_t i=0; i<quantity; i++)
	{
		int damage = 5 * (P_Random(player->mo) % 3 + 1);

		angle_t angle = player->mo->angle;
		fixed_t slope = bulletslope;
		if (ssg_spread)		// for super shotgun
		{
			angle += P_RandomDiff(player->mo) << 19;
			slope += P_RandomDiff(player->mo) << 5;
		}            
		if (!accurate)
		{
			// single-barrel shotgun or re-firing pistol/chaingun 
			angle += P_RandomDiff(player->mo) << 18;
		}
		P_LineAttack(player->mo, angle, MISSILERANGE, slope, damage);
	}
    
	// [SL] 2011-05-11 - Restore players and sectors to their current position
	// according to the server.
	Unlag::getInstance().restore(player->id);
}

//
// A_FirePistol
//
void A_FirePistol (player_t *player, pspdef_t *psp)
{
	A_FireSound (player, "weapons/pistol");

	P_SetMobjState (player->mo, S_PLAY_ATK2);

	DecreaseAmmo(player, 1);

	P_SetPsprite (player,
				  ps_flash,
				  weaponinfo[player->readyweapon].flashstate);

	P_FireHitscan (player, 1, !player->refire, false);	// [SL] 2011-05-11
}


//
// A_FireShotgun
//
void A_FireShotgun (player_t *player, pspdef_t *psp)
{
	A_FireSound (player, "weapons/shotgf");
	P_SetMobjState (player->mo, S_PLAY_ATK2);

	DecreaseAmmo(player, 1);

	P_SetPsprite (player,
				  ps_flash,
				  weaponinfo[player->readyweapon].flashstate);

	P_FireHitscan(player, 7, false, false);		// [SL] 2011-05-11
}



//
// A_FireShotgun2
//
void A_FireShotgun2 (player_t *player, pspdef_t *psp)
{
	A_FireSound (player, "weapons/sshotf");
	P_SetMobjState (player->mo, S_PLAY_ATK2);

	DecreaseAmmo(player, 2);

	P_SetPsprite (player,
				  ps_flash,
				  weaponinfo[player->readyweapon].flashstate);

	P_FireHitscan(player, 20, true, true);		// [SL] 2011-05-11
}

//
// A_FireCGun
//
void A_FireCGun (player_t *player, pspdef_t *psp)
{
	A_FireSound (player, "weapons/chngun");

	if (weaponinfo[player->readyweapon].ammo != am_noammo
		&& !player->ammo[weaponinfo[player->readyweapon].ammo])
		return;

	P_SetMobjState (player->mo, S_PLAY_ATK2);

	DecreaseAmmo(player, 1);

	P_SetPsprite (player,
				  ps_flash,
				  (statenum_t)(weaponinfo[player->readyweapon].flashstate
				  + psp->state
				  - &states[S_CHAIN1]) );

	P_FireHitscan(player, 1, !player->refire, false);	// [SL] 2011-05-11
}



void A_Light0 (player_t *player, pspdef_t *psp)
{
	player->extralight = 0;
}

void A_Light1 (player_t *player, pspdef_t *psp)
{
	player->extralight = 1;
}

void A_Light2 (player_t *player, pspdef_t *psp)
{
	player->extralight = 2;
}


//
// A_BFGSpray
// Spawn a BFG explosion on every monster in view
//
void A_BFGSpray (AActor *mo)
{
	int 				i;
	int 				j;
	int 				damage;
	angle_t 			an;

	if(!serverside)
		return;

	// note: mo->target is the player who fired the BFG
	if (!mo->target || !mo->target->player)
		return;

	// [SL] 2011-07-12 - Move players and sectors back to their positions when
	// this player hit the fire button clientside.
	player_t *player = mo->target->player;	// player who fired BFG
	Unlag::getInstance().reconcile(player->id);

	// offset angles from its attack angle
	for (i=0 ; i<40 ; i++)
	{
		an = mo->angle - ANG90/2 + ANG90/40*i;

		// mo->target is the originator (player)
		//	of the missile
		P_AimLineAttack (mo->target, an, 16*64*FRACUNIT);

		if (!linetarget)
			continue;

		fixed_t xoffs = 0, yoffs = 0, zoffs = 0;
		// [SL] 2011-07-12 - In unlagged games, spawn BFG tracers at the 
		// opponent's current position, not at their reconciled position
		if (linetarget->player)
		{
			Unlag::getInstance().getReconciliationOffset(   player->id,
															linetarget->player->id,
															xoffs, yoffs, zoffs);
		}

		new AActor (linetarget->x + xoffs,
					linetarget->y + yoffs,
					linetarget->z + zoffs+ (linetarget->height>>2),
					MT_EXTRABFG);

		damage = 0;
		for (j=0;j<15;j++)
			damage += (P_Random (mo) & 7) + 1;

		P_DamageMobj (linetarget, mo->target,mo->target, damage, MOD_BFG_SPLASH);
	}

	// [SL] 2011-07-12 - Restore players and sectors to their current position
	// according to the server.
	Unlag::getInstance().restore(player->id);
}


//
// A_BFGsound
//
void A_BFGsound (player_t *player, pspdef_t *psp)
{
	A_FireSound(player, "weapons/bfgf");
}



//
// P_SetupPsprites
// Called at start of level for each player.
//
void P_SetupPsprites (player_t* player)
{
	int i;

	// remove all psprites
	for (i=0 ; i<NUMPSPRITES ; i++)
		player->psprites[i].state = NULL;

	// spawn the gun
	player->pendingweapon = player->readyweapon;
	P_BringUpWeapon (player);
}




//
// P_MovePsprites
// Called every tic by player thinking routine.
//
void P_MovePsprites (player_t* player)
{
	int 		i;
	pspdef_t*	psp;
	state_t*	state;

	psp = &player->psprites[0];
	for (i=0 ; i<NUMPSPRITES ; i++, psp++)
	{
		// a null state means not active
		if ( (state = psp->state) )
		{
			// drop tic count and possibly change state

			// a -1 tic count never changes
			if (psp->tics != -1)
			{
				psp->tics--;
				if (!psp->tics)
					P_SetPsprite (player, i, psp->state->nextstate);
			}
		}
	}

	player->psprites[ps_flash].sx = player->psprites[ps_weapon].sx;
	player->psprites[ps_flash].sy = player->psprites[ps_weapon].sy;
}

void A_OpenShotgun2 (player_t *player, pspdef_t *psp)
{
	A_FireSound(player, "weapons/sshoto");
}

void A_LoadShotgun2 (player_t *player, pspdef_t *psp)
{
	A_FireSound(player, "weapons/sshotl");
}

void A_ReFire (player_t *player, pspdef_t *psp);

void A_CloseShotgun2 (player_t *player, pspdef_t *psp)
{
	A_FireSound(player, "weapons/sshotc");
	A_ReFire(player,psp);
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

