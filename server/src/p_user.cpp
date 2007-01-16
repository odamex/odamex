// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id:$
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
//	Player related stuff.
//	Bobbing POV/weapon, movement.
//	Pending weapon.
//
//-----------------------------------------------------------------------------


#include "doomdef.h"
#include "d_event.h"
#include "p_local.h"
#include "doomstat.h"
#include "s_sound.h"
#include "i_system.h"
#include "sv_main.h"


// Index of the special effects (INVUL inverse) map.
#define INVERSECOLORMAP 		32


//
// Movement.
//

// 16 pixels of bob
#define MAXBOB			0x100000

BOOL onground;

EXTERN_CVAR (allowjump)

//
// P_Thrust
// Moves the given origin along a given angle.
//
void P_SideThrust (player_t *player, angle_t angle, fixed_t move)
{
	angle = (angle - ANG90) >> ANGLETOFINESHIFT;

	player->mo->momx += FixedMul (move, finecosine[angle]);
	player->mo->momy += FixedMul (move, finesine[angle]);
}

void P_ForwardThrust (player_t *player, angle_t angle, fixed_t move)
{
	angle >>= ANGLETOFINESHIFT;

	if ((player->mo->waterlevel)
		&& player->mo->pitch != 0)
	{
		angle_t pitch = (angle_t)player->mo->pitch >> ANGLETOFINESHIFT;
		fixed_t zpush = FixedMul (move, finesine[pitch]);
		if (player->mo->waterlevel && player->mo->waterlevel < 2 && zpush < 0)
			zpush = 0;
		player->mo->momz -= zpush;
		move = FixedMul (move, finecosine[pitch]);
	}
	player->mo->momx += FixedMul (move, finecosine[angle]);
	player->mo->momy += FixedMul (move, finesine[angle]);
}

//
// P_Bob
// Same as P_Thrust, but only affects bobbing.
//
// killough 10/98: We apply thrust separately between the real physical player
// and the part which affects bobbing. This way, bobbing only comes from player
// motion, nothing external, avoiding many problems, e.g. bobbing should not
// occur on conveyors, unless the player walks on one, and bobbing should be
// reduced at a regular rate, even on ice (where the player coasts).
//

/*void P_Bob (player_t *player, angle_t angle, fixed_t move)
{
	angle >>= ANGLETOFINESHIFT;

	player->momx += FixedMul(move,finecosine[angle]);
	player->momy += FixedMul(move,finesine[angle]);
}*/

//
// P_CalcHeight
// Calculate the walking / running height adjustment
//
void P_CalcHeight (player_t *player) 
{
	int 		angle;
	fixed_t 	bob;

	// Regular movement bobbing
	// (needs to be calculated for gun swing even if not on ground)
	// OPTIMIZE: tablify angle

	// killough 10/98: Make bobbing depend only on player-applied motion.
	//
	// Note: don't reduce bobbing here if on ice: if you reduce bobbing here,
	// it causes bobbing jerkiness when the player moves from ice to non-ice,
	// and vice-versa.

	{
		player->bob = FixedMul (player->mo->momx, player->mo->momx)
					+ FixedMul (player->mo->momy, player->mo->momy);
		player->bob >>= 2;

		if (player->bob > MAXBOB)
			player->bob = MAXBOB;
	}

    if ((player->cheats & CF_NOMOMENTUM) || !onground)
	{
		player->viewz = player->mo->z + VIEWHEIGHT;

		if (player->viewz > player->mo->ceilingz-4*FRACUNIT)
			player->viewz = player->mo->ceilingz-4*FRACUNIT;

		return;
	}
	
	angle = (FINEANGLES/20*level.time) & FINEMASK;
	bob = FixedMul (player->bob>>(player->mo->waterlevel > 1 ? 2 : 1), finesine[angle]);

	// move viewheight
	if (player->playerstate == PST_LIVE)
	{
		player->viewheight += player->deltaviewheight;

		if (player->viewheight > VIEWHEIGHT)
		{
			player->viewheight = VIEWHEIGHT;
			player->deltaviewheight = 0;
		}
		else if (player->viewheight < VIEWHEIGHT/2)
		{
			player->viewheight = VIEWHEIGHT/2;
			if (player->deltaviewheight <= 0)
				player->deltaviewheight = 1;
		}
		
		if (player->deltaviewheight)	
		{
			player->deltaviewheight += FRACUNIT/4;
			if (!player->deltaviewheight)
				player->deltaviewheight = 1;
		}
	}
	player->viewz = player->mo->z + player->viewheight + bob;

	if (player->viewz > player->mo->ceilingz-4*FRACUNIT)
		player->viewz = player->mo->ceilingz-4*FRACUNIT;
}



//
// P_MovePlayer
//
void P_MovePlayer (player_t *player)
{
	ticcmd_t *cmd = &player->cmd;
	AActor *mo = player->mo;

//	mo->angle += cmd->ucmd.yaw << 16;

	onground = (mo->z <= mo->floorz);

	// [RH] Don't let frozen players move
	if (player->cheats & CF_FROZEN)
		return;

	// killough 10/98:
	//
	// We must apply thrust to the player and bobbing separately, to avoid
	// anomalies. The thrust applied to bobbing is always the same strength on
	// ice, because the player still "works just as hard" to move, while the
	// thrust applied to the movement varies with 'movefactor'.

	if (cmd->ucmd.forwardmove | cmd->ucmd.sidemove)
	{
		fixed_t forwardmove, sidemove;
		int bobfactor;
		int friction, movefactor;

		movefactor = P_GetMoveFactor (mo, &friction);
		bobfactor = friction < ORIG_FRICTION ? movefactor : ORIG_FRICTION_FACTOR;
		if (!onground && !player->mo->waterlevel)
		{
			// [RH] allow very limited movement if not on ground.
			movefactor >>= 8;
			bobfactor >>= 8;
		}
		forwardmove = (cmd->ucmd.forwardmove * movefactor) >> 8;
		sidemove = (cmd->ucmd.sidemove * movefactor) >> 8;

		if (forwardmove && onground)
		{
			//P_Bob (player, mo->angle, (cmd->ucmd.forwardmove * bobfactor) >> 8);
			P_ForwardThrust (player, mo->angle, forwardmove);
		}
		if (sidemove && onground)
		{
			//P_Bob (player, mo->angle-ANG90, (cmd->ucmd.sidemove * bobfactor) >> 8);
			P_SideThrust (player, mo->angle, sidemove);
		}
	}

	if (mo->state == &states[S_PLAY])
	{
		P_SetMobjState (player->mo, S_PLAY_RUN1); // denis - fixme - this function might destoy player->mo without setting it to 0
	}
	
	if (player->cheats & CF_REVERTPLEASE)
	{
		player->cheats &= ~CF_REVERTPLEASE;
		player->camera = player->mo;
	}
}		

//
// P_DeathThink
// Fall on your face when dying.
// Decrease POV height to floor height.
//
#define ANG5	(ANG90/18)

void P_DeathThink (player_t *player)
{
	angle_t 			angle;
	angle_t 			delta;

	P_MovePsprites (player);
	onground = (player->mo->z <= player->mo->floorz);
		
	// fall to the ground
	if (player->viewheight > 6*FRACUNIT)
		player->viewheight -= FRACUNIT;

	if (player->viewheight < 6*FRACUNIT)
		player->viewheight = 6*FRACUNIT;

	player->deltaviewheight = 0;
	P_CalcHeight (player);
		
	if(!serverside)
	{
		if (player->damagecount)
			player->damagecount--;
		
		return;
	}

	if (player->attacker && player->attacker != player->mo)
	{
		angle_t old_angle = player->mo->angle;
		
		angle = R_PointToAngle2 (player->mo->x,
								 player->mo->y,
								 player->attacker->x,
								 player->attacker->y);
		
		delta = angle - player->mo->angle;
		
		if (delta < ANG5 || delta > (unsigned)-ANG5)
		{
			// Looking at killer,
			//	so fade damage flash down.
			player->mo->angle = angle;

			if (player->damagecount)
				player->damagecount--;
		}
		else if (delta < ANG180)
			player->mo->angle += ANG5;
		else
			player->mo->angle -= ANG5;
		
		if(player->mo->angle != old_angle)
		{
			client_t *cl = &player->client;
			
			MSG_WriteMarker(&cl->netbuf, svc_moveplayer);
			MSG_WriteByte(&cl->netbuf, player->id);     // player number
			MSG_WriteLong(&cl->netbuf, player->mo->x);
			MSG_WriteLong(&cl->netbuf, player->mo->y);
			MSG_WriteLong(&cl->netbuf, player->mo->z);
			MSG_WriteLong(&cl->netbuf, player->mo->angle);
			if (player->mo->frame == 32773)
				MSG_WriteByte(&cl->netbuf, PLAYER_FULLBRIGHTFRAME);
			else
				MSG_WriteByte(&cl->netbuf, player->mo->frame);

				// write velocity
			MSG_WriteLong(&cl->netbuf, player->mo->momx);
			MSG_WriteLong(&cl->netbuf, player->mo->momy);
			MSG_WriteLong(&cl->netbuf, player->mo->momz);
		}
	}
	else if (player->damagecount)
		player->damagecount--;

	if(serverside)
	{
		// [Toke - dmflags] Old location of DF_FORCE_RESPAWN
		if (player->ingame() && (player->cmd.ucmd.buttons & BT_USE
								 || level.time >= player->respawn_time)) // forced respawn
			player->playerstate = PST_REBORN;
	}
}

EXTERN_CVAR (freelook)


//
// P_PlayerThink
//
void P_PlayerThink (player_t *player)
{
	ticcmd_t *cmd;
	weapontype_t newweapon;

	// [RH] Error out if player doesn't have an mobj, but just make
	//		it a warning if the player trying to spawn is a bot
	if (!player->mo)
		I_Error ("No player %d start\n", player->id);

	// fixme: do this in the cheat code
	if (player->cheats & CF_NOCLIP)
		player->mo->flags |= MF_NOCLIP;
	else
		player->mo->flags &= ~MF_NOCLIP;
	
        // joek - 01/14/06 - removed flying-cheat code 

	// chain saw run forward
	cmd = &player->cmd;
	if (player->mo->flags & MF_JUSTATTACKED)
	{
		cmd->ucmd.yaw = 0;
		cmd->ucmd.forwardmove = 0xc800/2;
		cmd->ucmd.sidemove = 0;
		player->mo->flags &= ~MF_JUSTATTACKED;
	}

	if (player->playerstate == PST_DEAD)
	{
		P_DeathThink (player);
		return;
	}

	// [RH] Look up/down stuff
	if (!freelook)
		player->mo->pitch = 0;

	else
	{
		int look = cmd->ucmd.pitch << 16;

		// The player's view pitch is clamped between -32 and +56 degrees,
		// which translates to about half a screen height up and (more than)
		// one full screen height down from straight ahead when view panning
		// is used.
		if (look)
		{
			if (look == -32768 << 16)
			{ // center view
				player->mo->pitch = 0;
			}
			else if (look > 0)
			{ // look up
				player->mo->pitch -= look;
				if (player->mo->pitch < -ANG(32))
					player->mo->pitch = -ANG(32);
			}
			else
			{ // look down
				player->mo->pitch -= look;
				if (player->mo->pitch > ANG(56))
					player->mo->pitch = ANG(56);
			}
		}
	}

	if(serverside)
	{
		// Move around.
		// Reactiontime is used to prevent movement
		//	for a bit after a teleport.
		if (player->mo->reactiontime)
			player->mo->reactiontime--;
		else
		{
			// [RH] check for swim/jump
			if ((cmd->ucmd.buttons & BT_JUMP) == BT_JUMP)
			{
				if (player->mo->waterlevel >= 2)
				{
					player->mo->momz = 4*FRACUNIT;
				}
				else if (allowjump && onground && !player->mo->momz)
				{
					player->mo->momz += 7*FRACUNIT;
					S_Sound (player->mo, CHAN_BODY, "*jump1", 1, ATTN_NORM);
				}
			}

			if (cmd->ucmd.upmove &&
				(player->mo->waterlevel >= 2))
			{
				player->mo->momz = cmd->ucmd.upmove << 8;
			}

			P_MovePlayer (player);
		}

		P_CalcHeight (player);
	}

	if (player->mo->subsector->sector->special || player->mo->subsector->sector->damage)
		P_PlayerInSpecialSector (player);

	// Check for weapon change.

	// A special event has no other buttons.
	if (cmd->ucmd.buttons & BT_SPECIAL)
		cmd->ucmd.buttons = 0;						
	
	if ((cmd->ucmd.buttons & BT_CHANGE) || cmd->ucmd.impulse >= 50)
	{
		// [RH] Support direct weapon changes
		if (cmd->ucmd.impulse) {
			newweapon = (weapontype_t)(cmd->ucmd.impulse - 50);
		} else {
			// The actual changing of the weapon is done
			//	when the weapon psprite can do it
			//	(read: not in the middle of an attack).
			newweapon = (weapontype_t)((cmd->ucmd.buttons&BT_WEAPONMASK)>>BT_WEAPONSHIFT);
			
			if (newweapon == wp_fist
				&& player->weaponowned[wp_chainsaw]
				&& !(player->readyweapon == wp_chainsaw
					 && player->powers[pw_strength]))
			{
				newweapon = wp_chainsaw;
			}
			
			if (newweapon == wp_shotgun 
				&& player->weaponowned[wp_supershotgun]
				&& player->readyweapon != wp_supershotgun)
			{
				newweapon = wp_supershotgun;
			}
		}

		if ((newweapon >= 0 && newweapon < NUMWEAPONS)
			&& player->weaponowned[newweapon]
			&& newweapon != player->readyweapon)
		{
			player->pendingweapon = newweapon;
		}
	}

	// check for use
	if (cmd->ucmd.buttons & BT_USE)
	{
		if (!player->usedown)
		{
			P_UseLines (player);
			player->usedown = true;
		}
	}
	else
		player->usedown = false;

	// cycle psprites
	P_MovePsprites (player);

	// Counters, time dependent power ups.

	// Strength counts up to diminish fade.
	if (player->powers[pw_strength])
		player->powers[pw_strength]++;	
				
	if (player->powers[pw_invulnerability])
		player->powers[pw_invulnerability]--;

	if (player->powers[pw_invisibility])
		if (! --player->powers[pw_invisibility] )
			player->mo->flags &= ~MF_SHADOW;
						
	if (player->powers[pw_infrared])
		player->powers[pw_infrared]--;
				
	if (player->powers[pw_ironfeet])
		player->powers[pw_ironfeet]--;
				
	if (player->damagecount)
		player->damagecount--;
				
	if (player->bonuscount)
		player->bonuscount--;


	// Handling colormaps.
	if (player->powers[pw_invulnerability])
	{
		if (player->powers[pw_invulnerability] > 4*32
			|| (player->powers[pw_invulnerability]&8) )
			player->fixedcolormap = INVERSECOLORMAP;
		else
			player->fixedcolormap = 0;
	}
	else if (player->powers[pw_infrared])		
	{
		if (player->powers[pw_infrared] > 4*32
			|| (player->powers[pw_infrared]&8) )
		{
			// almost full bright
			player->fixedcolormap = 1;
		}
		else
			player->fixedcolormap = 0;
	}
	else
		player->fixedcolormap = 0;

	// Handle air supply
	if (player->mo->waterlevel < 3 || player->powers[pw_ironfeet])
	{
		player->air_finished = level.time + 10*TICRATE;
	}
	else if (player->air_finished <= level.time && !(level.time & 31))
	{
		P_DamageMobj (player->mo, NULL, NULL, 2 + 2*((level.time-player->air_finished)/TICRATE), MOD_WATER, DMG_NO_ARMOR);
	}
}

void player_s::Serialize (FArchive &arc)
{
	size_t i;

	if (arc.IsStoring ())
	{ // saving to archive
		arc << mo
			<< playerstate
			<< cmd
			<< userinfo
			<< viewz
			<< viewheight
			<< deltaviewheight
			<< bob
			<< health
			<< armorpoints
			<< armortype
			<< backpack
			<< fragcount
			<< readyweapon
			<< pendingweapon
			<< attackdown
			<< usedown
			<< cheats
			<< refire
			<< killcount
			<< damagecount
			<< bonuscount
			<< attacker
			<< extralight
			<< fixedcolormap
			<< respawn_time
			<< air_finished;
		for (i = 0; i < NUMPOWERS; i++)
			arc << powers[i];
		for (i = 0; i < NUMCARDS; i++)
			arc << cards[i];
//		for (i = 0; i < players.size(); i++)
//			arc << frags[i];
		for (i = 0; i < NUMWEAPONS; i++)
			arc << weaponowned[i];
		for (i = 0; i < NUMAMMO; i++)
			arc << ammo[i] << maxammo[i];
		for (i = 0; i < NUMPSPRITES; i++)
			arc << psprites[i];
		for (i = 0; i < 3; i++)
			arc << oldvelocity[i];
	}
	else
	{ // Restoring from archive
		userinfo_t dummyuserinfo;

		arc >> id
			>> mo->netid
			>> playerstate
			>> cmd
			>> dummyuserinfo // Q: Would it be better to restore the userinfo from the archive?
			>> viewz
			>> viewheight
			>> deltaviewheight
			>> bob
			>> health
			>> armorpoints
			>> armortype
			>> backpack
			>> fragcount
			>> readyweapon
			>> pendingweapon
			>> attackdown
			>> usedown
			>> cheats
			>> refire
			>> killcount
			>> damagecount
			>> bonuscount
			>> attacker->netid
			>> extralight
			>> fixedcolormap
			>> respawn_time
			>> air_finished;
		for (i = 0; i < NUMPOWERS; i++)
			arc >> powers[i];
		for (i = 0; i < NUMCARDS; i++)
			arc >> cards[i];
//		for (i = 0; i < players.size(); i++)
//			arc >> frags[i];
		for (i = 0; i < NUMWEAPONS; i++)
			arc >> weaponowned[i];
		for (i = 0; i < NUMAMMO; i++)
			arc >> ammo[i] >> maxammo[i];
		for (i = 0; i < NUMPSPRITES; i++)
			arc >> psprites[i];
		for (i = 0; i < 3; i++)
			arc >> oldvelocity[i];

		camera = mo;

		if (consoleplayer().id != id)
			userinfo = dummyuserinfo;
	}
}
