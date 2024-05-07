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
//		Player related stuff.
//		Bobbing POV/weapon, movement.
//		Pending weapon.
//
//-----------------------------------------------------------------------------


#include "odamex.h"

#include <ctime>

#include <limits.h>

#include "cmdlib.h"
#include "c_dispatch.h"
#include "d_event.h"
#include "p_local.h"
#include "s_sound.h"
#include "i_system.h"
#include "p_tick.h"
#include "gi.h"
#include "m_wdlstats.h"

#include "p_snapshot.h"
#include "g_gametype.h"

#include "p_mapformat.h"

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
EXTERN_CVAR (g_lives)
EXTERN_CVAR (r_softinvulneffect)

extern bool predicting, step_mode;

static player_t nullplayer;		// used to indicate 'player not found' when searching
EXTERN_CVAR (sv_allowmovebob)
EXTERN_CVAR (cl_movebob)

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

/**
 * @brief Clear all cards from a player.
 * 
 * @param p Player to clear.
*/
void P_ClearPlayerCards(player_t& p)
{
	for (size_t i = 0; i < NUMCARDS; i++)
		p.cards[i] = false;
}

/**
 * @brief Clear all powerups from a player.
 *
 * @param p Player to clear.
 */
void P_ClearPlayerPowerups(player_t& p)
{
	for (size_t i = 0; i < NUMPOWERS; i++)
		p.powers[i] = 0;
}

/**
 * @brief Clear all scores from a player.
 * 
 * @param p Player to clear.
 * @param wins True if a player's wins should be cleared as well - should
 *             usually be True unless it's a reset across rounds.
 */
void P_ClearPlayerScores(player_t& p, byte flags)
{
	if (flags & SCORES_CLEAR_WINS)
		p.roundwins = 0;

	if (flags & SCORES_CLEAR_POINTS)
	{
		p.lives = g_lives.asInt();
		p.fragcount = 0;
		p.itemcount = 0;
		p.secretcount = 0;
		p.deathcount = 0; // [Toke - Scores - deaths]
		p.monsterdmgcount = 0;
		p.killcount = 0;  // [deathz0r] Coop kills
		p.points = 0;
	}

	if (flags & SCORES_CLEAR_TOTALPOINTS)
	{
		p.totalpoints = 0;
		p.totaldeaths = 0;
	}
}

static bool cmpFrags(player_t* a, player_t* b)
{
	return a->fragcount < b->fragcount;
}

static bool cmpLives(player_t* a, player_t* b)
{
	return a->lives < b->lives;
}

static bool cmpWins(player_t* a, const player_t* b)
{
	return a->roundwins < b->roundwins;
}

/**
 * @brief Execute the query.
 *
 * @return Results of the query.
 */
PlayerResults PlayerQuery::execute()
{
	PlayerResults results;
	int maxscore = 0;

	// Construct a base result set from all ingame players, possibly filtered.
	for (Players::iterator it = ::players.begin(); it != players.end(); ++it)
	{
		if (!it->ingame() || it->spectator)
			continue;

		results.total += 1;
		if (it->userinfo.team != TEAM_NONE)
		{
			results.teamTotal[it->userinfo.team] += 1;
		}

		if (m_ready && !it->ready)
			continue;

		if (m_health && it->health <= 0)
			continue;

		if (m_lives && it->lives <= 0)
			continue;

		if (m_notLives && it->lives > 0)
			continue;

		if (m_team != TEAM_NONE && it->userinfo.team != m_team)
			continue;

		results.players.push_back(&*it);
	}

	// We have no filtered players, we have our totals, there is no more
	// useful work to be done.
	if (results.players.size() == 0)
	{
		return results;
	}

	// Sort our list of players if needed.
	switch (m_sort)
	{
	case SORT_NONE:
		break;
	case SORT_FRAGS:
		std::sort(results.players.rbegin(), results.players.rend(), cmpFrags);
		if (m_sortFilter == SFILTER_MAX || m_sortFilter == SFILTER_NOT_MAX)
		{
			// Since it's sorted, we know the top fragger is at the front.
			int top = results.players.at(0)->fragcount;
			for (PlayersView::iterator it = results.players.begin();
			     it != results.players.end();)
			{
				bool cmp = (m_sortFilter == SFILTER_MAX) ? (*it)->fragcount != top
				                                         : (*it)->fragcount == top;
				if (cmp)
				{
					it = results.players.erase(it);
				}
				else
				{
					++it;
				}
			}
		}
		break;
	case SORT_LIVES:
		std::sort(results.players.rbegin(), results.players.rend(), cmpLives);
		if (m_sortFilter == SFILTER_MAX || m_sortFilter == SFILTER_NOT_MAX)
		{
			// Since it's sorted, we know the top fragger is at the front.
			int top = results.players.at(0)->lives;
			for (PlayersView::iterator it = results.players.begin();
			     it != results.players.end();)
			{
				bool cmp = (m_sortFilter == SFILTER_MAX) ? (*it)->lives != top
				                                         : (*it)->lives == top;
				if (cmp)
				{
					it = results.players.erase(it);
				}
				else
				{
					++it;
				}
			}
		}
		break;
	case SORT_WINS:
		std::sort(results.players.rbegin(), results.players.rend(), cmpWins);
		if (m_sortFilter == SFILTER_MAX || m_sortFilter == SFILTER_NOT_MAX)
		{
			// Since it's sorted, we know the top winner is at the front.
			int top = results.players.at(0)->roundwins;
			for (PlayersView::iterator it = results.players.begin();
			     it != results.players.end();)
			{
				bool cmp = (m_sortFilter == SFILTER_MAX) ? (*it)->roundwins != top
				                                         : (*it)->roundwins == top;
				if (cmp)
				{
					it = results.players.erase(it);
				}
				else
				{
					++it;
				}
			}
		}
		break;
	}

	// Get the final totals.
	for (PlayersView::iterator it = results.players.begin(); it != results.players.end();
	     ++it)
	{
		results.count += 1;
		results.teamCount[(*it)->userinfo.team] += 1;
	}

	return results;
}

/**
 * @brief Execute the query.
 *
 * @return Results of the query.
 */
PlayersView SpecQuery::execute()
{
	PlayersView rvo;

	for (Players::iterator it = ::players.begin(); it != ::players.end(); ++it)
	{
		if (!it->ingame() || !it->spectator)
			continue;

		if (m_onlyInQueue && it->QueuePosition == 0)
			continue;

		rvo.push_back(&*it);
	}

	return rvo;
}


//
// P_NumPlayersInGame()
//
// Returns the number of players who are active in the current game.  This does
// not include spectators or downloaders.
//
size_t P_NumPlayersInGame()
{
	return PlayerQuery().execute().count;
}

//
// P_NumReadyPlayersInGame()
//
// Returns the number of players who are marked ready in the current game.
// This does not include spectators or downloaders.
//
size_t P_NumReadyPlayersInGame()
{
	return PlayerQuery().isReady().execute().count;
}

// P_NumPlayersOnTeam()
//
// Returns the number of active players on a team.  No specs or downloaders.
size_t P_NumPlayersOnTeam(team_t team)
{
	return PlayerQuery().onTeam(team).execute().teamTotal[team];
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
	if (!sv_freelook && (!p->spectator))
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
		bool force_respawn =	(!clientside && sv_forcerespawn &&
								level.time >= player->death_time + sv_forcerespawntime * TICRATE);

		// [SL] Can we respawn yet?
		int respawn_time = player->death_time + sv_spawndelaytime * TICRATE;
		bool delay_respawn =	(!clientside && level.time < respawn_time);

		// [Toke - dmflags] Old location of DF_FORCE_RESPAWN
		if (player->ingame() &&
		    ((player->cmd.buttons & BT_USE && !delay_respawn) || force_respawn) &&
		    ((g_lives && player->lives > 0) || !g_lives))
		{
			player->playerstate = PST_REBORN;
		}
	}

	if (clientside)
	{
		// [AM] If the player runs out of lives in an LMS gamemode, having
		//      them spectate another player after a beat is expected.
		if (::g_lives && player->lives < 1 && &consoleplayer() == &displayplayer() &&
		    level.time >= player->death_time + (TICRATE * 2))
		{
			// CL_SpyCycle is located in cl and templated, so we use this
			// instead.
			AddCommandString("spynext");
		}
	}
}

bool P_AreTeammates(player_t &a, player_t &b)
{
	// not your own teammate (at least for friendly fire, etc)
	if (a.id == b.id)
		return false;

	return G_IsCoopGame() ||
		  ((a.userinfo.team == b.userinfo.team) &&
		   G_IsTeamGame());
}

bool P_CanSpy(player_t &viewer, player_t &other, bool demo)
{
	// Viewers can always spy themselves.
	if (viewer.id == other.id)
		return true;

	// You cannot view those without bodies or spectators.
	if (!other.mo || other.spectator)
		return false;

	// Demo-watchers can view anybody.
	if (demo)
		return true;

	// Is the player a teammate?
	bool isTeammate = false;
	if (G_IsCoopGame())
	{
		// You are everyone's teammate in a coop game.
		isTeammate = true;
	}
	else if (G_IsTeamGame())
	{
		if (viewer.userinfo.team == other.userinfo.team)
		{
			// You are on the same team.
			isTeammate = true;
		}
		else if (G_IsLivesGame())
		{
			PlayerResults pr =
			    PlayerQuery().hasLives().onTeam(viewer.userinfo.team).execute();
			if (pr.count == 0)
			{
				// You are on a different team but your teammates are dead, so
				// it doesn't really matter if you spectate them.
				isTeammate = true;
			}
		}
	}

	if (isTeammate || viewer.spectator)
	{
		// If a player has no more lives, don't show him.
		if (::g_lives && other.lives < 1)
			return false;

		return true;
	}

	// A player who is out of lives in LMS can see everyone else
	if (::sv_gametype == GM_DM && ::g_lives && viewer.lives < 1)
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

	if (player->cheats & CF_FLY)
		player->mo->flags |= MF_NOGRAVITY, player->mo->flags2 |= MF2_FLY;
	else
		player->mo->flags &= ~MF_NOGRAVITY, player->mo->flags2 &= ~MF2_FLY;

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

	if (player->mo->subsector &&
		(player->mo->subsector->sector->special ||
		player->mo->subsector->sector->damageamount ||
		player->mo->subsector->sector->flags & SECF_SECRET))
		map_format.player_in_special_sector(player);

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

	if (player->hazardcount)
	{
		player->hazardcount--;
		if (!(::level.time % player->hazardinterval) &&
		    player->hazardcount > 16 * TICRATE)
			P_DamageMobj(player->mo, NULL, NULL, 5);
	}

	// Handling colormaps.
	if (displayplayer().powers[pw_invulnerability])
	{
		if (displayplayer().powers[pw_invulnerability] > 4*32
			|| (displayplayer().powers[pw_invulnerability]&8) )
			if (r_softinvulneffect)
				displayplayer().fixedcolormap = 1;
			else
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
	if (player->mo->waterlevel < 3 || player->powers[pw_ironfeet] || player->cheats & CF_GODMODE)
	{
		player->air_finished = level.time + 10*TICRATE;
	}
	else if (player->air_finished <= level.time && !(level.time & 31))
	{
		P_DamageMobj (player->mo, NULL, NULL, 2 + 2*((level.time-player->air_finished)/TICRATE), MOD_WATER, DMG_NO_ARMOR);
	}

	// [BC] Handle WDL Beacon
	if (serverside && player->ingame() && !player->spectator && player->mo->health > 0)
	{
		if (P_AtInterval(5))
		{
			M_LogWDLEvent(WDL_EVENT_PLAYERBEACON, player, NULL, player->mo->angle / 4, 0,
			              0, 0);
		}
	}
}

#define CASE_STR(str) case str : return #str

const char* PlayerState(size_t state)
{
	statenum_t st = static_cast<statenum_t>(state);

	switch (st)
	{
		CASE_STR(S_PLAY);
		CASE_STR(S_PLAY_RUN1);
		CASE_STR(S_PLAY_RUN2);
		CASE_STR(S_PLAY_RUN3);
		CASE_STR(S_PLAY_RUN4);
		CASE_STR(S_PLAY_ATK1);
		CASE_STR(S_PLAY_ATK2);
		CASE_STR(S_PLAY_PAIN);
		CASE_STR(S_PLAY_PAIN2);
		CASE_STR(S_PLAY_DIE1);
		CASE_STR(S_PLAY_DIE2);
		CASE_STR(S_PLAY_DIE3);
		CASE_STR(S_PLAY_DIE4);
		CASE_STR(S_PLAY_DIE5);
		CASE_STR(S_PLAY_DIE6);
		CASE_STR(S_PLAY_DIE7);
		CASE_STR(S_PLAY_XDIE1);
		CASE_STR(S_PLAY_XDIE2);
		CASE_STR(S_PLAY_XDIE3);
		CASE_STR(S_PLAY_XDIE4);
		CASE_STR(S_PLAY_XDIE5);
		CASE_STR(S_PLAY_XDIE6);
		CASE_STR(S_PLAY_XDIE7);
		CASE_STR(S_PLAY_XDIE8);
		CASE_STR(S_PLAY_XDIE9);
	default:
		return "Unknown";
	}
}

#define STATE_NUM(mo) (mo -> state - states)

BEGIN_COMMAND(cheat_players)
{
	Printf("== PLAYERS ==");

	int dead = 0;

	AActor* mo;
	TThinkerIterator<AActor> iterator;
	while ((mo = iterator.Next()))
	{

		if (mo->type == MT_PLAYER)
		{
			if (STATE_NUM(mo) == S_PLAY_DIE7 || STATE_NUM(mo) == S_PLAY_XDIE9)
			{
				dead += 1;
				continue;
			}

			if (mo->player)
			{
				Printf("%.3u: %s\n", mo->player->id,
				       mo->player->userinfo.netname.c_str());
			}
			else
			{
				Printf("???: ???\n");
			}
			Printf("State: %s\n", PlayerState(mo->state - states));
			Printf("%f, %f, %f\n", FIXED2FLOAT(mo->x), FIXED2FLOAT(mo->y),
			       FIXED2FLOAT(mo->z));
		}
	}

	Printf("== Skipped %d dead players ==\n", dead);
}
END_COMMAND(cheat_players)

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
			<< lives
			<< roundwins
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
			>> lives
			>> roundwins
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

player_s::player_s() :
	id(0),
	playerstate(PST_LIVE),
	mo(AActor::AActorPtr()),
	cmd(ticcmd_t()),
	cmdqueue(std::queue<NetCommand>()),
	userinfo(UserInfo()),
	fov(90.0),
	viewz(0 << FRACBITS),
	prevviewz(0 << FRACBITS),
	viewheight(0 << FRACBITS),
	deltaviewheight(0 << FRACBITS),
	bob(0 << FRACBITS),
	health(0),
	armorpoints(0),
	armortype(0),
	backpack(false),
	lives(0),
	roundwins(0),
	points(0),
	fragcount(0),
	deathcount(0),
	monsterdmgcount(0),
	killcount(0),
	itemcount(0),
	secretcount(0),
	pendingweapon(wp_fist),
	readyweapon(wp_fist),
	attackdown(0),
	usedown(0),
	cheats(0),
	refire(0),
	damagecount(0),
	bonuscount(0),
	extralight(0),
	fixedcolormap(0),
	xviewshift(0),
	psprnum(0),
	jumpTics(0),
	death_time(0),
	suicidedelay(0),
	camera(AActor::AActorPtr()),
	air_finished(0),
	GameTime(0),
	JoinTime(time_t()),
	ping(0),
	last_received(0),
	tic(0),
	snapshots(PlayerSnapshotManager()),
	spying(0),
	spectator(false),
	joindelay(0),
	timeout_callvote(0),
	timeout_vote(0),
	ready(false),
	timeout_ready(0),
	blend_color(argb_t(0, 0, 0, 0)),
	doreborn(false),
	QueuePosition(0),
	hazardcount(0),
	hazardinterval(0),
	LastMessage(LastMessage_s()),
	to_spawn(std::queue<AActor::AActorPtr>()),
	client(player_s::client_t())
{
	cmd.clear();
	ArrayInit(powers, 0);
	ArrayInit(cards, false);
	ArrayInit(flags, false);
	ArrayInit(weaponowned, false);
	ArrayInit(ammo, false);
	ArrayInit(maxammo, false);

	// Can't put this in initializer list?
	attacker = AActor::AActorPtr();

	pspdef_t zeropsp = { NULL, 0, 0, 0 };
	ArrayInit(psprites, zeropsp);
	ArrayInit(oldvelocity, 0);
	ArrayInit(prefcolor, 0);

	LastMessage.Time = 0;
	LastMessage.Message = "";

	ArrayInit(netcmds, ticcmd_t());
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

	ArrayCopy(powers, other.powers);
	ArrayCopy(cards, other.cards);

	lives = other.lives;
	roundwins = other.roundwins;

	ArrayCopy(flags, other.flags);

	points = other.points;
	backpack = other.backpack;

	fragcount = other.fragcount;
	deathcount = other.deathcount;
	monsterdmgcount = other.monsterdmgcount;
	killcount = other.killcount;
	totalpoints = other.totalpoints;
	totaldeaths = other.totaldeaths;

	pendingweapon = other.pendingweapon;
	readyweapon = other.readyweapon;

	ArrayCopy(weaponowned, other.weaponowned);
	ArrayCopy(ammo, other.ammo);
	ArrayCopy(maxammo, other.maxammo);

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

	ArrayCopy(psprites, other.psprites);

    jumpTics = other.jumpTics;

	death_time = other.death_time;

	ArrayCopy(oldvelocity, other.oldvelocity);

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
	joindelay = other.joindelay;
	timeout_callvote = other.timeout_callvote;
	timeout_vote = other.timeout_vote;

	ready = other.ready;
	timeout_ready = other.timeout_ready;

	ArrayCopy(prefcolor, other.prefcolor);
	ArrayCopy(netcmds, other.netcmds);

    LastMessage.Time = other.LastMessage.Time;
	LastMessage.Message = other.LastMessage.Message;

	blend_color = other.blend_color;

	client = other.client;

	snapshots = other.snapshots;

	to_spawn = other.to_spawn;

	doreborn = other.doreborn;
	QueuePosition = other.QueuePosition;

	return *this;
}

player_s::~player_s()
{
}

VERSION_CONTROL (p_user_cpp, "$Id$")
