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
//		Player related stuff.
//		Bobbing POV/weapon, movement.
//		Pending weapon.
//
//-----------------------------------------------------------------------------

#include <limits.h>

#include "cmdlib.h"
#include "doomdef.h"
#include "d_event.h"
#include "p_local.h"
#include "doomstat.h"
#include "s_sound.h"
#include "i_system.h"
#include "i_net.h"
#include "gi.h"
#include "g_warmup.h"

#include "p_snapshot.h"

// Index of the special effects (INVUL inverse) map.
#define INVERSECOLORMAP 		32

//
// Movement.
//

// 16 pixels of bob
#define MAXBOB			0x100000

EXTERN_CVAR (sv_allowjump)
EXTERN_CVAR (cl_mouselook)
EXTERN_CVAR (sv_freelook)
EXTERN_CVAR (co_zdoomphys)
EXTERN_CVAR (cl_deathcam)
EXTERN_CVAR (sv_forcerespawn)
EXTERN_CVAR (sv_forcerespawntime)
EXTERN_CVAR (sv_spawndelaytime)

extern bool predicting, step_mode;

static player_t nullplayer;		// used to indicate 'player not found' when searching
EXTERN_CVAR (sv_allowmovebob)
EXTERN_CVAR (cl_movebob)
EXTERN_CVAR(cl_spectator_freelook_force)
EXTERN_CVAR(cl_spectator_autofly)

player_t &idplayer(byte id)
{
	// Put a cached lookup mechanism in here.

	// full search
	for (Players::iterator it = players.begin();it != players.end();++it)
	{
		// Add to the cache while we search
		if (it->id == id)
			return *it;
	}

	return nullplayer;
}

/**
 * Find player by netname.  Note that this search is case-insensitive.
 * 
 * @param  netname Name of player to look for.
 * @return         Player reference of found player, or nullplayer.
 */
player_t &nameplayer(const std::string &netname)
{
	for (Players::iterator it = players.begin();it != players.end();++it)
	{
		if (iequals(netname, it->userinfo.netname))
			return *it;
	}

	return nullplayer;
}

bool validplayer(player_t &ref)
{
	if (&ref == &nullplayer)
		return false;

	if (players.empty())
		return false;

	return true;
}

//
// P_NumPlayersInGame()
//
// Returns the number of players who are active in the current game.  This does
// not include spectators or downloaders.
//
size_t P_NumPlayersInGame()
{
	size_t num_players = 0;

	for (Players::const_iterator it = players.begin();it != players.end();++it)
		if (it->ingame() && !it->spectator)
			num_players++;

	return num_players;
}

//
// P_NumReadyPlayersInGame()
//
// Returns the number of players who are marked ready in the current game.
// This does not include spectators or downloaders.
//
size_t P_NumReadyPlayersInGame()
{
	size_t num_players = 0;

	for (Players::const_iterator it = players.begin();it != players.end();++it)
		if (it->ingame() && !it->spectator && it->ready)
			num_players++;

	return num_players;
}

// P_NumPlayersOnTeam()
//
// Returns the number of active players on a team.  No specs or downloaders.
size_t P_NumPlayersOnTeam(team_t team)
{
	size_t num_players = 0;

	for (Players::const_iterator it = players.begin();it != players.end();++it)
		if (it->ingame() && !it->spectator && it->userinfo.team == team)
			num_players++;

	return num_players;
}

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

	if ((player->mo->waterlevel || (player->mo->flags2 & MF2_FLY))
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

	if ((player->mo->flags2 & MF2_FLY) && !player->mo->onground)
	{
		player->bob = FRACUNIT / 2;
	}

	if ((serverside || !predicting) && !player->spectator)
	{
		player->bob = FixedMul (player->mo->momx, player->mo->momx)
					+ FixedMul (player->mo->momy, player->mo->momy);
		player->bob >>= 2;

		if (player->bob > MAXBOB)
			player->bob = MAXBOB;
	}

    if (player->cheats & CF_NOMOMENTUM || (!co_zdoomphys && !player->mo->onground))
	{
		player->viewz = player->mo->z + VIEWHEIGHT;

		if (player->viewz > player->mo->ceilingz-4*FRACUNIT)
			player->viewz = player->mo->ceilingz-4*FRACUNIT;

		return;
	}

	angle = (FINEANGLES/20*level.time) & FINEMASK;
	if (!player->spectator)
		bob = FixedMul (player->bob>>(player->mo->waterlevel > 1 ? 2 : 1), finesine[angle]);
	else
		bob = 0;

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

	// [SL] Scale view-bobbing based on user's preference (if the server allows)
	if (sv_allowmovebob || (clientside && serverside))
		bob *= cl_movebob;

	player->viewz = player->mo->z + player->viewheight + bob;

	if (player->viewz > player->mo->ceilingz-4*FRACUNIT)
		player->viewz = player->mo->ceilingz-4*FRACUNIT;
	if (player->viewz < player->mo->floorz + 4*FRACUNIT)
		player->viewz = player->mo->floorz + 4*FRACUNIT;
}

//
// P_PlayerLookUpDown
//
void P_PlayerLookUpDown (player_t *p)
{
	// [RH] Look up/down stuff
	if (!sv_freelook && (p->spectator && !cl_spectator_freelook_force))
	{
		p->mo->pitch = 0;
	}
	else
	{
		int look = p->cmd.pitch << 16;

		// The player's view pitch is clamped between -32 and +56 degrees,
		// which translates to about half a screen height up and (more than)
		// one full screen height down from straight ahead when view panning
		// is used.
		if (look)
		{
			if (look == INT_MIN)
			{ // center view
				p->mo->pitch = 0;
			}
			else if (look > 0)
			{ // look up
				p->mo->pitch -= look;
				if (p->mo->pitch < -ANG(32))
					p->mo->pitch = -ANG(32);
			}
			else
			{ // look down
				p->mo->pitch -= look;
				if (p->mo->pitch > ANG(56))
					p->mo->pitch = ANG(56);
			}
		}
	}
}

CVAR_FUNC_IMPL (sv_aircontrol)
{
	level.aircontrol = (fixed_t)((float)var * 65536.f);
	G_AirControlChanged ();
}

//
// P_MovePlayer
//
void P_MovePlayer (player_t *player)
{
	if (!player || !player->mo || player->playerstate == PST_DEAD)
		return;

	AActor *mo = player->mo;

	mo->onground = (mo->z <= mo->floorz) || (mo->flags2 & MF2_ONMOBJ);

	// [RH] Don't let frozen players move
	if (player->cheats & CF_FROZEN)
		return;

	// Move around.
	// Reactiontime is used to prevent movement
	//	for a bit after a teleport.
	if (player->mo->reactiontime)
	{
		player->mo->reactiontime--;
		return;
	}

	if (player->cmd.upmove == -32768)
	{
		// Only land if in the air
		if ((player->mo->flags2 & MF2_FLY) && player->mo->waterlevel < 2)
		{
			player->mo->flags2 &= ~MF2_FLY;
			player->mo->flags &= ~MF_NOGRAVITY;
		}
	}
	else if (player->cmd.upmove != 0)
	{
		if (player->mo->waterlevel >= 2 || player->mo->flags2 & MF2_FLY)
		{
			player->mo->momz = player->cmd.upmove << 9;

			if (player->mo->waterlevel < 2 && !(player->mo->flags2 & MF2_FLY))
			{
				player->mo->flags2 |= MF2_FLY;
				player->mo->flags |= MF_NOGRAVITY;
				if (player->mo->momz <= -39*FRACUNIT)
					S_StopSound(player->mo, CHAN_VOICE);	// Stop falling scream
			}
		}
	}

	// Look left/right
	if(clientside || step_mode)
	{
		mo->angle += player->cmd.yaw << 16;

		// Look up/down stuff
		P_PlayerLookUpDown(player);
	}

	// killough 10/98:
	//
	// We must apply thrust to the player and bobbing separately, to avoid
	// anomalies. The thrust applied to bobbing is always the same strength on
	// ice, because the player still "works just as hard" to move, while the
	// thrust applied to the movement varies with 'movefactor'.

	if (player->cmd.forwardmove | player->cmd.sidemove)
	{
		fixed_t forwardmove, sidemove;
		int bobfactor;
		int friction, movefactor;

		movefactor = P_GetMoveFactor (mo, &friction);
		bobfactor = friction < ORIG_FRICTION ? movefactor : ORIG_FRICTION_FACTOR;
		if (!mo->onground && !(mo->flags2 & MF2_FLY) && !mo->waterlevel)
		{
			// [RH] allow very limited movement if not on ground.
			if (co_zdoomphys)
			{
				movefactor = FixedMul (movefactor, level.aircontrol);
				bobfactor = FixedMul (bobfactor, level.aircontrol);
			}
			else
			{
				movefactor >>= 8;
				bobfactor >>= 8;
			}
		}
		forwardmove = (player->cmd.forwardmove * movefactor) >> 8;
		sidemove = (player->cmd.sidemove * movefactor) >> 8;

		// [ML] Check for these conditions unless advanced physics is on
		if(co_zdoomphys ||
			(!co_zdoomphys && (mo->onground || (mo->flags2 & MF2_FLY) || mo->waterlevel)))
		{
			if (forwardmove)
			{
				P_ForwardThrust (player, mo->angle, forwardmove);
			}
			if (sidemove)
			{
				P_SideThrust (player, mo->angle, sidemove);
			}
		}

		if (mo->state == &states[S_PLAY])
		{
			// denis - fixme - this function might destoy player->mo without setting it to 0
			P_SetMobjState (player->mo, S_PLAY_RUN1);
		}

		if (player->cheats & CF_REVERTPLEASE)
		{
			player->cheats &= ~CF_REVERTPLEASE;
			player->camera = player->mo;
		}
	}

	// [RH] check for jump
	if (player->jumpTics)
		player->jumpTics--;

	if ((player->cmd.buttons & BT_JUMP) == BT_JUMP)
	{
		if (player->mo->waterlevel >= 2)
		{
			player->mo->momz = 4*FRACUNIT;
		}
		else if (player->mo->flags2 & MF2_FLY)
		{
			player->mo->momz = 3*FRACUNIT;
		}
		else if (sv_allowjump && player->mo->onground && !player->jumpTics)
		{
			player->mo->momz += 8*FRACUNIT;

//			[SL] No jumping sound...
//			if(!player->spectator)
//				UV_SoundAvoidPlayer(player->mo, CHAN_VOICE, "player/male/jump1", ATTN_NORM);

            player->mo->flags2 &= ~MF2_ONMOBJ;
            player->jumpTics = 18;
		}
	}
}

// [RH] (Adapted from Q2)
// P_FallingDamage
//
void P_FallingDamage (AActor *ent)
{
	float	delta;
	int		damage;

	if (!ent->player)
		return;		// not a player

	if (ent->flags & MF_NOCLIP)
		return;

	if ((ent->player->oldvelocity[2] < 0)
		&& (ent->momz > ent->player->oldvelocity[2])
		&& (!(ent->flags2 & MF2_ONMOBJ)
			|| !(ent->z <= ent->floorz)))
	{
		delta = (float)ent->player->oldvelocity[2];
	}
	else
	{
		if (!(ent->flags2 & MF2_ONMOBJ))
			return;
		delta = (float)(ent->momz - ent->player->oldvelocity[2]);
	}
	delta = delta*delta * 2.03904313e-11f;

	if (delta < 1)
		return;

	if (delta < 15)
	{
		//ent->s.event = EV_FOOTSTEP;
		return;
	}

	if (delta > 30)
	{
		damage = (int)((delta-30)/2);
		if (damage < 1)
			damage = 1;

		if (0)
			P_DamageMobj (ent, NULL, NULL, damage, MOD_FALLING);
	}
	else
	{
		//ent->s.event = EV_FALLSHORT;
		return;
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
	bool reduce_redness = true;

	P_MovePsprites (player);
	player->mo->onground = (player->mo->z <= player->mo->floorz);

	// fall to the ground
	if (player->viewheight > 6*FRACUNIT)
		player->viewheight -= FRACUNIT;

	if (player->viewheight < 6*FRACUNIT)
		player->viewheight = 6*FRACUNIT;

	player->deltaviewheight = 0;
	P_CalcHeight (player);

	// adjust the player's view to follow its attacker
	if (cl_deathcam && clientside &&
		player->attacker && player->attacker != player->mo)
	{
		angle_t angle = P_PointToAngle (player->mo->x,
								 		player->mo->y,
								 		player->attacker->x,
								 		player->attacker->y);

		angle_t delta = angle - player->mo->angle;

		if (delta < ANG5 || delta > (unsigned)-ANG5)
			player->mo->angle = angle;
		else
		{
			if (delta < ANG180)
				player->mo->angle += ANG5;
			else
				player->mo->angle -= ANG5;

			// not yet looking at killer so keep the red tinting
			reduce_redness = false;
		}
	}

	if (player->damagecount && reduce_redness && !predicting)
		player->damagecount--;

	if(serverside)
	{
		bool overtime_respawn = sv_gametype == GM_CTF && warmup.get_overtime();

		bool force_respawn =	(!clientside && sv_forcerespawn &&
								level.time >= player->death_time + sv_forcerespawntime * TICRATE);

		int respawn_time;
		// [SL] Can we respawn yet?
		if (overtime_respawn)
			respawn_time = player->death_time + warmup.get_ctf_overtime_penalty() * TICRATE;
		else
			respawn_time = player->death_time + sv_spawndelaytime * TICRATE;
		bool delay_respawn =	(!clientside && level.time < respawn_time);

		// [Toke - dmflags] Old location of DF_FORCE_RESPAWN
		if (player->ingame() && ((player->cmd.buttons & BT_USE && !delay_respawn) || (!delay_respawn && overtime_respawn) || force_respawn))
		{
			player->playerstate = PST_REBORN;
		}
	}
}

bool P_AreTeammates(player_t &a, player_t &b)
{
	// not your own teammate (at least for friendly fire, etc)
	if (a.id == b.id)
		return false;

	return (sv_gametype == GM_COOP) ||
		  ((a.userinfo.team == b.userinfo.team) &&
		   (sv_gametype == GM_TEAMDM || sv_gametype == GM_CTF));
}

bool P_CanSpy(player_t &viewer, player_t &other, bool demo)
{
	// Viewers can always spy themselves.
	if (viewer.id == other.id)
		return true;

	// You cannot view those without bodies or spectators.
	if (!other.mo || other.spectator)
		return false;

	// Demo-watchers and spectators can view anybody.
	if (demo || viewer.spectator)
		return true;

	// A teammate can see their other teammates
	if (P_AreTeammates(viewer, other))
		return true;

	return false;
}

void SV_SendPlayerInfo(player_t &);

//
// P_PlayerThink
//
void P_PlayerThink (player_t *player)
{
	weapontype_t newweapon;

	// [SL] 2011-10-31 - Thinker called before the client has received a message
	// to spawn a mobj from the server.  Just bail from this function and
	// hope the client receives the spawn message at a later time.
	if (!player->mo && clientside && multiplayer)
	{
		DPrintf("Warning: P_PlayerThink called for player %s without a valid Actor.\n",
				player->userinfo.netname.c_str());
		return;
	}
	else if (!player->mo)
		I_Error ("No player %d start\n", player->id);

	player->xviewshift = 0;		// [RH] Make sure view is in right place
	player->prevviewz = player->viewz;
	player->mo->prevangle = player->mo->angle;
	player->mo->prevpitch = player->mo->pitch;

	// fixme: do this in the cheat code
	if (player->cheats & CF_NOCLIP)
		player->mo->flags |= MF_NOCLIP;
	else
		player->mo->flags &= ~MF_NOCLIP;

	if (player->spectator)
	{
		if (cl_spectator_autofly)
			player->mo->flags |= MF_NOGRAVITY, player->mo->flags2 |= MF2_FLY;	// Fly by default as a spectator
	}
	else
	{
		if (player->cheats & CF_FLY)
			player->mo->flags |= MF_NOGRAVITY, player->mo->flags2 |= MF2_FLY;
		else
			player->mo->flags &= ~MF_NOGRAVITY, player->mo->flags2 &= ~MF2_FLY;
	}

	// chain saw run forward
	if (player->mo->flags & MF_JUSTATTACKED)
	{
		player->cmd.yaw = 0;
		player->cmd.forwardmove = 0xc800/2;
		player->cmd.sidemove = 0;
		player->mo->flags &= ~MF_JUSTATTACKED;
	}

	if (player->playerstate == PST_DEAD)
	{
		P_DeathThink(player);
		return;
	}

	P_MovePlayer (player);
	P_CalcHeight (player);

	if (player->mo->subsector && (player->mo->subsector->sector->special || player->mo->subsector->sector->damage))
		P_PlayerInSpecialSector (player);

	// Check for weapon change.

	// A special event has no other buttons.
	if (player->cmd.buttons & BT_SPECIAL)
		player->cmd.buttons = 0;

	if ((player->cmd.buttons & BT_CHANGE) || player->cmd.impulse >= 50)
	{
		// [RH] Support direct weapon changes
		if (player->cmd.impulse) {
			newweapon = (weapontype_t)(player->cmd.impulse - 50);
		} else {
			// The actual changing of the weapon is done
			//	when the weapon psprite can do it
			//	(read: not in the middle of an attack).
			newweapon = (weapontype_t)((player->cmd.buttons&BT_WEAPONMASK)>>BT_WEAPONSHIFT);

			if (newweapon == wp_fist
				&& player->weaponowned[wp_chainsaw]
				&& !(player->readyweapon == wp_chainsaw
					 && player->powers[pw_strength]))
			{
				newweapon = wp_chainsaw;
			}

			if ((gameinfo.flags & GI_MAPxx)
				&& newweapon == wp_shotgun
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
			// NEVER go to plasma or BFG in shareware,
			if ((newweapon != wp_plasma && newweapon != wp_bfg)
			|| (gamemode != shareware) )
			{
				player->pendingweapon = newweapon;
			}
		}
	}

	// check for use
	if (player->cmd.buttons & BT_USE)
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
	if (displayplayer().powers[pw_invulnerability])
	{
		if (displayplayer().powers[pw_invulnerability] > 4*32
			|| (displayplayer().powers[pw_invulnerability]&8) )
			displayplayer().fixedcolormap = INVERSECOLORMAP;
		else
			displayplayer().fixedcolormap = 0;
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
		arc << id
			<< playerstate
			<< spectator
//			<< deadspectator
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
			<< itemcount
			<< secretcount
			<< damagecount
			<< bonuscount
			<< points
			/*<< attacker->netid*/
			<< extralight
			<< fixedcolormap
			<< xviewshift
			<< jumpTics
			<< death_time
			<< air_finished;
		for (i = 0; i < NUMPOWERS; i++)
			arc << powers[i];
		for (i = 0; i < NUMCARDS; i++)
			arc << cards[i];
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
		UserInfo dummyuserinfo;

		arc >> id
			>> playerstate
			>> spectator
//			>> deadspectator
			>> cmd
			>> userinfo // Q: Would it be better to restore the userinfo from the archive?
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
			>> itemcount
			>> secretcount
			>> damagecount
			>> bonuscount
			>> points
			/*>> attacker->netid*/
			>> extralight
			>> fixedcolormap
			>> xviewshift
			>> jumpTics
			>> death_time
			>> air_finished;
		for (i = 0; i < NUMPOWERS; i++)
			arc >> powers[i];
		for (i = 0; i < NUMCARDS; i++)
			arc >> cards[i];
		for (i = 0; i < NUMWEAPONS; i++)
			arc >> weaponowned[i];
		for (i = 0; i < NUMAMMO; i++)
			arc >> ammo[i] >> maxammo[i];
		for (i = 0; i < NUMPSPRITES; i++)
			arc >> psprites[i];
		for (i = 0; i < 3; i++)
			arc >> oldvelocity[i];

		camera = mo;

//		if (&consoleplayer() != this)
//			userinfo = dummyuserinfo;
	}
}

player_s::player_s()
{
	size_t i;

	// GhostlyDeath -- Initialize EVERYTHING
	id = 0;
	playerstate = PST_LIVE;
	mo = AActor::AActorPtr();
	cmd.clear();
	fov = 90.0;
	viewz = 0 << FRACBITS;
	viewheight = 0 << FRACBITS;
	deltaviewheight = 0 << FRACBITS;
	bob = 0 << FRACBITS;
	health = 0;
	armorpoints = 0;
	armortype = 0;
	for (i = 0; i < NUMPOWERS; i++)
		powers[i] = 0;
	for (i = 0; i < NUMCARDS; i++)
		cards[i] = false;
	backpack = false;
	points = 0;
	for (i = 0; i < NUMFLAGS; i++)
		flags[i] = false;
	fragcount = 0;
	deathcount = 0;
	killcount = 0;
	pendingweapon = wp_nochange;
	readyweapon = wp_nochange;
	for (i = 0; i < NUMWEAPONS; i++)
		weaponowned[i] = false;
	for (i = 0; i < NUMAMMO; i++)
	{
		ammo[i] = 0;
		maxammo[i] = 0;
	}
	attackdown = 0;
	usedown = 0;
	cheats = 0;
	refire = 0;
	damagecount = 0;
	bonuscount = 0;
	attacker = AActor::AActorPtr();
	extralight = 0;
	fixedcolormap = 0;
	xviewshift = 0;
	memset(psprites, 0, sizeof(pspdef_t) * NUMPSPRITES);
	jumpTics = 0;
	death_time = 0;
	memset(oldvelocity, 0, sizeof(oldvelocity));
	camera = AActor::AActorPtr();
	air_finished = 0;
	GameTime = 0;
	JoinTime = 0;
	ping = 0;
	last_received = 0;
	tic = 0;
	spying = id;
	spectator = false;
//	deadspectator = false;

	joinafterspectatortime = level.time - TICRATE*5;
	timeout_callvote = 0;
	timeout_vote = 0;

	ready = false;
	timeout_ready = 0;

	memset(prefcolor, 0, 4);

	LastMessage.Time = 0;
	LastMessage.Message = "";

	blend_color = argb_t(0, 0, 0, 0);

	memset(netcmds, 0, sizeof(ticcmd_t) * BACKUPTICS);
}

player_s &player_s::operator =(const player_s &other)
{
	size_t i;

	id = other.id;
	playerstate = other.playerstate;
	mo = other.mo;
	cmd = other.cmd;
	cmdqueue = other.cmdqueue;
	userinfo = other.userinfo;
	fov = other.fov;
	viewz = other.viewz;
	viewheight = other.viewheight;
	deltaviewheight = other.deltaviewheight;
	bob = other.bob;

	health = other.health;
	armorpoints = other.armorpoints;
	armortype = other.armortype;

	for(i = 0; i < NUMPOWERS; i++)
		powers[i] = other.powers[i];

	for(i = 0; i < NUMCARDS; i++)
		cards[i] = other.cards[i];

	for(i = 0; i < NUMFLAGS; i++)
		flags[i] = other.flags[i];

	points = other.points;
	backpack = other.backpack;

	fragcount = other.fragcount;
	deathcount = other.deathcount;
	killcount = other.killcount;

	pendingweapon = other.pendingweapon;
	readyweapon = other.readyweapon;

	for(i = 0; i < NUMWEAPONS; i++)
		weaponowned[i] = other.weaponowned[i];
	for(i = 0; i < NUMAMMO; i++)
		ammo[i] = other.ammo[i];
	for(i = 0; i < NUMAMMO; i++)
		maxammo[i] = other.maxammo[i];

	attackdown = other.attackdown;
	usedown = other.usedown;

	cheats = other.cheats;

	refire = other.refire;

	damagecount = other.damagecount;
	bonuscount = other.bonuscount;

	attacker = other.attacker;

	extralight = other.extralight;
	fixedcolormap = other.fixedcolormap;

	xviewshift = other.xviewshift;

	for(i = 0; i < NUMPSPRITES; i++)
		psprites[i] = other.psprites[i];

    jumpTics = other.jumpTics;

	death_time = other.death_time;

	memcpy(oldvelocity, other.oldvelocity, sizeof(oldvelocity));

	camera = other.camera;
	air_finished = other.air_finished;

	GameTime = other.GameTime;
	JoinTime = other.JoinTime;
	ping = other.ping;

	last_received = other.last_received;

	tic = other.tic;
	spying = other.spying;
	spectator = other.spectator;
//	deadspectator = other.deadspectator;
	joinafterspectatortime = other.joinafterspectatortime;
	timeout_callvote = other.timeout_callvote;
	timeout_vote = other.timeout_vote;

	ready = other.ready;
	timeout_ready = other.timeout_ready;

	memcpy(prefcolor, other.prefcolor, 4);

	for(i = 0; i < BACKUPTICS; i++)
		netcmds[i] = other.netcmds[i];

    LastMessage.Time = other.LastMessage.Time;
	LastMessage.Message = other.LastMessage.Message;

	blend_color = other.blend_color;

	client = other.client;

	snapshots = other.snapshots;

	to_spawn = other.to_spawn;

	return *this;
}

player_s::~player_s()
{
}

VERSION_CONTROL (p_user_cpp, "$Id$")

