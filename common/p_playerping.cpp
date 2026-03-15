// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 2006-2026 by The Odamex Team.
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
//     Player Ping system
//
// ORIGINAL AUTHOR:
//     Lexi Mayfield
//
//-----------------------------------------------------------------------------


#include "odamex.h"

#include "p_playerping.h"

#include "c_effect.h"
#include "c_dispatch.h"
#include "g_gametype.h"
#include "p_local.h"
#include "p_mobj.h"
#include "teaminfo.h"
#include "w_wad.h"
#ifdef CLIENT_APP
#include <unordered_map>

#include "../client/src/cl_main.h"
#include "r_main.h"
#include "v_text.h"
extern byte* Ranges;
#endif
#include "../client/sdl/afxres.h"

//------------------------------------------------------------------------------

namespace
{
int P_PingLumpForType(const ping_type_t type)
{
	switch (type)
	{
	case PING_ITEM:
		return W_GetNumForName("OPNG_ITM");
	case PING_MONSTER:
		return W_GetNumForName("OPNG_MON");
	case PING_BOSS:
		return W_GetNumForName("OPNG_BOS");
	case PING_FLAG:
		return W_GetNumForName("OPNG_FLG");
	case PING_TEAMMATE:
		return W_GetNumForName("OPNG_TM");
	case PING_WARNING:
		return W_GetNumForName("OPNG_WRN");
	case PING_GENERAL:
	default:
		return W_GetNumForName("OPNG_GEN");
	}
}

bool P_IsHordeBossForPing(const AActor* actor)
{
	if (!actor || actor->health <= 0)
		return false;
	if (actor->flags & MF_MISSILE)
		return false;

	return (actor->oflags & MFO_BOSSPOOL) != 0 || (actor->effects & FX_YELLOWFOUNTAIN) != 0;
}

team_t P_PingFlagTeamForActor(const AActor* actor)
{
	if (!actor)
		return TEAM_NONE;

	for (int i = TEAM_BLUE; i < NUMTEAMS; i++)
	{
		const TeamInfo* team = GetTeamInfo(static_cast<team_t>(i));
		if (!team)
			continue;

		if (actor->sprite == team->FlagSprite || actor->sprite == team->FlagDownSprite ||
		    actor->sprite == team->FlagCarrySprite)
		{
			return static_cast<team_t>(i);
		}
	}

	return TEAM_NONE;
}

translationref_t P_PingTeamTranslation(team_t team)
{
#ifdef CLIENT_APP
	for (const player_t& pl : players)
	{
		if (!pl.ingame() || !pl.mo)
			continue;
		if (pl.userinfo.team == team)
			return pl.mo->translation;
	}

	if (team == TEAM_RED)
		return translationref_t(translationtables + (MAXPLAYERS + 2) * 256);
#endif
	return {};
}

#ifdef CLIENT_APP
translationref_t P_PingReadablePlayerTranslation(const player_t& pl)
{
	const int r = pl.userinfo.color[1];
	const int g = pl.userinfo.color[2];
	const int b = pl.userinfo.color[3];
	const int maxc = (std::max)((std::max)(r, g), b);
	const int minc = (std::min)((std::min)(r, g), b);
	const int luma = (r * 54 + g * 183 + b * 19) >> 8;
	const bool tooDark = luma < 48 || maxc < 48;
	const bool tooBright = luma > 224 || minc > 224;

	if (tooDark || tooBright)
		return translationref_t(Ranges + CR_GREY * 256);

	return pl.mo ? pl.mo->translation : translationref_t{};
}
#endif

void P_PingFollowTargetPos(const AActor* target, const AActor* view, v3fixed_t& pos)
{
	static constexpr fixed_t FollowPingHeadOffset = 4 * FRACUNIT;
	static constexpr fixed_t FollowPingNearDist = 512 * FRACUNIT;
	static constexpr fixed_t FollowPingFarDist = 4096 * FRACUNIT;

	if (!target || !view)
		return;

	fixed_t targetx = target->x;
	fixed_t targety = target->y;
	fixed_t targetz = target->z;
#ifdef CLIENT_APP
	targetx = target->prevx + FixedMul(render_lerp_amount, target->x - target->prevx);
	targety = target->prevy + FixedMul(render_lerp_amount, target->y - target->prevy);
	targetz = target->prevz + FixedMul(render_lerp_amount, target->z - target->prevz);
#endif

	const fixed_t dist = P_AproxDistance(view->x - targetx, view->y - targety);
	const fixed_t clamped_dist = std::clamp(dist, FollowPingNearDist, FollowPingFarDist);
	const fixed_t t =
	    FixedDiv(clamped_dist - FollowPingNearDist, FollowPingFarDist - FollowPingNearDist);

	const fixed_t near_z = targetz + target->height + FollowPingHeadOffset;
	const fixed_t far_z = targetz + (target->height >> 1);

	pos.x = targetx;
	pos.y = targety;
	pos.z = near_z - FixedMul(near_z - far_z, t);
}

struct pingTrace_t
{
	AActor* source = nullptr;
	AActor* target = nullptr;
	fixed_t x1 = 0;
	fixed_t y1 = 0;
	fixed_t dx = 0;
	fixed_t dy = 0;
	fixed_t shootz = 0;
	fixed_t slope = 0;
	fixed_t range = 0;
	fixed_t hitfrac = FRACUNIT;
	v3fixed_t hit{};
	bool blocked = false;
} g_pingTrace;

bool PTR_PingTraverse(intercept_t* in)
{
	if (!in)
		return true;

	const fixed_t z =
	    g_pingTrace.shootz + FixedMul(g_pingTrace.slope, FixedMul(in->frac, g_pingTrace.range));

	if (!in->isaline)
	{
		AActor* thing = in->d.thing;
		if (!thing || thing == g_pingTrace.source)
			return true;
		const bool isPickup = (thing->flags & MF_SPECIAL) != 0;
		if ((thing->flags & MF_SHOOTABLE) == 0 && !isPickup &&
		    thing->player == nullptr)
			return true;

		if (isPickup)
		{
			g_pingTrace.target = thing;
			g_pingTrace.hitfrac = std::clamp(in->frac, 0, FRACUNIT);
			g_pingTrace.hit.x = g_pingTrace.x1 + FixedMul(g_pingTrace.dx, g_pingTrace.hitfrac);
			g_pingTrace.hit.y = g_pingTrace.y1 + FixedMul(g_pingTrace.dy, g_pingTrace.hitfrac);
			g_pingTrace.hit.z = z;
			return false;
		}

		const fixed_t dist = FixedMul(g_pingTrace.range, in->frac);
		if (dist <= 0)
			return true;

		const fixed_t topSlope = FixedDiv(thing->z + thing->height - g_pingTrace.shootz, dist);
		const fixed_t bottomSlope = FixedDiv(thing->z - g_pingTrace.shootz, dist);
		if (topSlope < g_pingTrace.slope || bottomSlope > g_pingTrace.slope)
			return true;

		g_pingTrace.target = thing;
		g_pingTrace.hitfrac = std::clamp(in->frac, 0, FRACUNIT);
		g_pingTrace.hit.x = g_pingTrace.x1 + FixedMul(g_pingTrace.dx, g_pingTrace.hitfrac);
		g_pingTrace.hit.y = g_pingTrace.y1 + FixedMul(g_pingTrace.dy, g_pingTrace.hitfrac);
		g_pingTrace.hit.z = z;
		return false;
	}

	if (!in->isaline)
		return true;

	line_t* li = in->d.line;
	const fixed_t crossx = g_pingTrace.x1 + FixedMul(g_pingTrace.dx, in->frac);
	const fixed_t crossy = g_pingTrace.y1 + FixedMul(g_pingTrace.dy, in->frac);

	if (!(li->flags & ML_TWOSIDED))
	{
		g_pingTrace.blocked = true;
	}
	else
	{
		P_LineOpening(li, crossx, crossy);
		g_pingTrace.blocked = (z >= opentop || z <= openbottom);
	}

	if (!g_pingTrace.blocked)
		return true;

	fixed_t frac = in->frac;
	if (g_pingTrace.range > 0)
	{
		frac -= FixedDiv(4 * FRACUNIT, g_pingTrace.range);
	}
	frac = std::clamp(frac, 0, FRACUNIT);

	g_pingTrace.hit.x = g_pingTrace.x1 + FixedMul(g_pingTrace.dx, frac);
	g_pingTrace.hit.y = g_pingTrace.y1 + FixedMul(g_pingTrace.dy, frac);
	g_pingTrace.hit.z =
	    g_pingTrace.shootz + FixedMul(g_pingTrace.slope, FixedMul(frac, g_pingTrace.range));
	g_pingTrace.hitfrac = frac;
	return false;
}

void P_ClampPingToWorld(v3fixed_t& point, const fixed_t shootz, const fixed_t slope,
                        const fixed_t range, const fixed_t x1, const fixed_t y1,
                        const fixed_t dx, const fixed_t dy, const fixed_t maxfrac)
{
	// Step along the ray to find first floor/ceiling intersection so mouselook
	// floor/ceiling pings land where the shot would hit.
	static constexpr int Steps = 32;
	fixed_t prevFrac = 0;
	fixed_t prevX = x1;
	fixed_t prevY = y1;
	fixed_t prevZ = shootz;
	bool hadPrev = false;

	for (int i = 1; i <= Steps; i++)
	{
		const fixed_t frac = FixedMul(maxfrac, FixedDiv(i * FRACUNIT, Steps * FRACUNIT));
		const fixed_t x = x1 + FixedMul(dx, frac);
		const fixed_t y = y1 + FixedMul(dy, frac);
		const fixed_t z = shootz + FixedMul(slope, FixedMul(frac, range));
		const fixed_t floorz = P_FloorHeight(x, y);
		const fixed_t ceilz = P_CeilingHeight(x, y);

		if (z <= floorz || z >= ceilz)
		{
			const bool hitFloor = z <= floorz;
			fixed_t lo = prevFrac;
			fixed_t hi = frac;
			for (int iter = 0; iter < 5; iter++)
			{
				const fixed_t mid = (lo + hi) >> 1;
				const fixed_t mx = x1 + FixedMul(dx, mid);
				const fixed_t my = y1 + FixedMul(dy, mid);
				const fixed_t mz = shootz + FixedMul(slope, FixedMul(mid, range));
				const fixed_t mfloor = P_FloorHeight(mx, my);
				const fixed_t mceil = P_CeilingHeight(mx, my);
				if ((hitFloor && mz <= mfloor) || (!hitFloor && mz >= mceil))
					hi = mid;
				else
					lo = mid;
			}

			const fixed_t hitFrac = hi;
			point.x = x1 + FixedMul(dx, hitFrac);
			point.y = y1 + FixedMul(dy, hitFrac);
			point.z = hitFloor ? P_FloorHeight(point.x, point.y) : P_CeilingHeight(point.x, point.y);
			return;
		}

		prevFrac = frac;
		prevX = x;
		prevY = y;
		prevZ = z;
		hadPrev = true;
	}

	if (hadPrev)
	{
		point.x = prevX;
		point.y = prevY;
		point.z = prevZ;
	}
}

v3fixed_t P_TracePingEndpoint(const AActor* source, fixed_t shootz, angle_t angle, fixed_t slope,
                              fixed_t range, AActor*& outTarget)
{
	const int angleidx = angle >> ANGLETOFINESHIFT;
	const fixed_t x1 = source->x;
	const fixed_t y1 = source->y;
	const fixed_t x2 = x1 + FixedMul(range, finecosine[angleidx]);
	const fixed_t y2 = y1 + FixedMul(range, finesine[angleidx]);

	g_pingTrace = pingTrace_t{};
	g_pingTrace.source = const_cast<AActor*>(source);
	g_pingTrace.x1 = x1;
	g_pingTrace.y1 = y1;
	g_pingTrace.dx = x2 - x1;
	g_pingTrace.dy = y2 - y1;
	g_pingTrace.shootz = shootz;
	g_pingTrace.slope = slope;
	g_pingTrace.range = range;
	g_pingTrace.hit = {x2, y2, g_pingTrace.shootz + FixedMul(range, slope)};
	g_pingTrace.hitfrac = FRACUNIT;

	P_PathTraverse(x1, y1, x2, y2, PT_ADDLINES | PT_ADDTHINGS, PTR_PingTraverse);
	if (!g_pingTrace.target)
	{
		P_ClampPingToWorld(g_pingTrace.hit, g_pingTrace.shootz, g_pingTrace.slope, g_pingTrace.range,
		                   g_pingTrace.x1, g_pingTrace.y1, g_pingTrace.dx, g_pingTrace.dy,
		                   g_pingTrace.hitfrac);
	}

	outTarget = g_pingTrace.target;
	return g_pingTrace.hit;
}
} // namespace

//------------------------------------------------------------------------------

void P_PlayerPing(player_t &player)
{
	if (!player.mo || player.spectator)
		return;

	static constexpr fixed_t PingRange = 16 * 64 * FRACUNIT;
	static constexpr angle_t ItemAssistStep = 1 << 24;
	static constexpr int ItemAssistSweeps = 6;

	playerPing_s ping{};
	ping.translation = player.mo->translation;
	ping.pingtic = ::gametic;

	angle_t aimAngle = player.mo->angle;
	const fixed_t pitchSlope =
	    finetangent[FINEANGLES / 4 - (player.mo->pitch >> ANGLETOFINESHIFT)];
	const fixed_t shootz = player.mo->z + player.viewheight;
	AActor* pingTarget = nullptr;
	ping.pos = P_TracePingEndpoint(player.mo, shootz, aimAngle, pitchSlope, PingRange, pingTarget);

	// Item ping assist: sweep nearby angles and auto-select a pickup if visible.
	if (!(pingTarget && (pingTarget->flags & MF_SPECIAL)))
	{
		for (int sweep = 1; sweep <= ItemAssistSweeps; sweep++)
		{
			for (int dir = -1; dir <= 1; dir += 2)
			{
				angle_t testAngle = player.mo->angle + dir * sweep * ItemAssistStep;
				AActor* testTarget = nullptr;
				v3fixed_t testPos = P_TracePingEndpoint(player.mo, shootz, testAngle, pitchSlope,
				                                        PingRange, testTarget);
				if (testTarget && (testTarget->flags & MF_SPECIAL))
				{
					aimAngle = testAngle;
					pingTarget = testTarget;
					ping.pos = testPos;
					break;
				}
			}

			if (pingTarget && (pingTarget->flags & MF_SPECIAL))
				break;
		}
	}

	if (pingTarget)
	{
		team_t flagTeam = P_PingFlagTeamForActor(pingTarget);
		ping.flag_team = flagTeam;

		ping.pos.x = pingTarget->x;
		ping.pos.y = pingTarget->y;
		ping.pos.z = pingTarget->z;
		ping.target_netid = pingTarget->netid;

		if (flagTeam != TEAM_NONE)
		{
			ping.type = PING_FLAG;
			ping.follow_target = false;
			ping.target_netid = 0;
		}
	else if (pingTarget->player && pingTarget->player != &player &&
	         (G_IsCoopGame() ||
	          (G_IsTeamGame() && pingTarget->player->userinfo.team == player.userinfo.team)))
	{
		// Teammate markers are now always-on; player ping stays a location ping.
		ping.type = PING_GENERAL;
		ping.follow_target = false;
		ping.target_netid = 0;
	}
		else if (pingTarget->player)
		{
			// Enemy player pings are location snapshots, not follow-target.
			ping.type = PING_GENERAL;
			ping.follow_target = false;
			ping.target_netid = 0;
		}
		else if ((pingTarget->flags & MF_SPECIAL) != 0)
		{
			ping.type = PING_ITEM;
			ping.follow_target = false;
			ping.pos.x = pingTarget->x;
			ping.pos.y = pingTarget->y;
			ping.pos.z = pingTarget->z + pingTarget->height + 8 * FRACUNIT;
		}
		else if (G_IsHordeMode() && P_IsHordeBossForPing(pingTarget))
		{
			ping.type = PING_BOSS;
			ping.follow_target = ping.target_netid != 0;
		}
		else if (!pingTarget->player && (pingTarget->flags & MF_SHOOTABLE))
		{
			ping.type = PING_MONSTER;
			ping.follow_target = ping.target_netid != 0;
		}
		else
		{
			ping.type = PING_GENERAL;
			ping.follow_target = ping.target_netid != 0;
		}

		PrintFmt("Ping: {}\n", ping.pos);
	}
	else
	{
		ping.target_netid = 0;
		ping.follow_target = false;
		ping.type = PING_GENERAL;
		ping.flag_team = TEAM_NONE;

		// Double-tap world ping: upgrade to warning if quickly repeated nearby.
		static constexpr int WarningRetapWindow = TICRATE / 2;
		static constexpr fixed_t WarningRetapRadius = 96 * FRACUNIT;
		if (player.player_ping)
		{
			const playerPing_s& prev = *player.player_ping;
			if (!P_IsPingExpired(prev) &&
			    (prev.type == PING_GENERAL || prev.type == PING_WARNING) &&
			    (::gametic - prev.pingtic) <= WarningRetapWindow)
			{
				const fixed_t dist =
				    P_AproxDistance(ping.pos.x - prev.pos.x, ping.pos.y - prev.pos.y);
				if (dist <= WarningRetapRadius)
					ping.type = PING_WARNING;
			}
		}

		PrintFmt("World ping: {}\n", ping.pos);
	}

	int lump = P_PingLumpForType(ping.type);
	if (lump == -1)
		return;
	ping.lump = lump;

	player.player_ping = std::make_unique<playerPing_s>(std::move(ping));
}

//------------------------------------------------------------------------------

bool P_IsPingExpired(const playerPing_s& ping)
{
	static constexpr int PingLifetimeTics = 10 * TICRATE;
	if (ping.pingtic < 0)
		return true;

	return (::gametic - ping.pingtic) > PingLifetimeTics;
}

//------------------------------------------------------------------------------

bool P_ResolvePingPosition(const playerPing_s& ping, v3fixed_t& outPos)
{
	if (ping.type == PING_ITEM && ping.target_netid != 0)
	{
		AActor* target = P_FindThingById(ping.target_netid);
		if (!target)
			return false;

		outPos.x = target->x;
		outPos.y = target->y;
		outPos.z = target->z + target->height + 8 * FRACUNIT;
		return true;
	}

	if (ping.follow_target && ping.target_netid != 0)
	{
		AActor* target = P_FindThingById(ping.target_netid);
		if (!target || target->health <= 0)
			return false;

		outPos.x = target->x;
		outPos.y = target->y;
		outPos.z = target->z + (target->height >> 1);
		return true;
	}

	outPos = ping.pos;
	return true;
}

//------------------------------------------------------------------------------

void R_AddPingSprites()
{
#ifdef CLIENT_APP
	static constexpr fixed_t StrictHeadOffset = 12 * FRACUNIT;
	static constexpr fixed_t FollowPingSmoothing = FRACUNIT / 3;
	static constexpr int FixedPingScreenPx = 64;
	static std::array<v3fixed_t, MAXPLAYERS> followSmoothPos{};
	static std::array<bool, MAXPLAYERS> followSmoothValid{};
	static std::unordered_map<uint32_t, v3fixed_t> actorSmoothPos{};

	auto smoothFixed = [](fixed_t from, fixed_t to) -> fixed_t
	{
		return from + FixedMul(FollowPingSmoothing, to - from);
	};

	auto smoothActorPos = [&](uint32_t netid, const v3fixed_t& targetPos) -> v3fixed_t
	{
		if (netid == 0)
			return targetPos;

		auto it = actorSmoothPos.find(netid);
		if (it == actorSmoothPos.end())
		{
			actorSmoothPos.emplace(netid, targetPos);
			return targetPos;
		}

		v3fixed_t& cur = it->second;
		cur.x = smoothFixed(cur.x, targetPos.x);
		cur.y = smoothFixed(cur.y, targetPos.y);
		cur.z = smoothFixed(cur.z, targetPos.z);
		return cur;
	};

	for (auto &pl : players)
	{
		if (!pl.player_ping)
			continue;

		if (G_IsTeamGame() && pl.id != consoleplayer().id &&
		    pl.userinfo.team != consoleplayer().userinfo.team)
			continue;

		auto& ping = *pl.player_ping;
		if (P_IsPingExpired(ping))
			continue;
		if (consoleplayer().mo && ping.target_netid == consoleplayer().mo->netid)
			continue;

		v3fixed_t pos{};
		if (!P_ResolvePingPosition(ping, pos))
			continue;

		if (ping.follow_target && ping.target_netid != 0)
		{
			AActor* target = P_FindThingById(ping.target_netid);
			AActor* view = consoleplayer().camera ? consoleplayer().camera : consoleplayer().mo;
			if (target && view)
			{
				if (ping.type == PING_MONSTER || ping.type == PING_BOSS)
				{
					pos.x = target->prevx + FixedMul(render_lerp_amount, target->x - target->prevx);
					pos.y = target->prevy + FixedMul(render_lerp_amount, target->y - target->prevy);
					const fixed_t tzt =
					    target->prevz + FixedMul(render_lerp_amount, target->z - target->prevz);
					pos.z = tzt + target->height + StrictHeadOffset;
				}
				else
				{
					P_PingFollowTargetPos(target, view, pos);
				}
			}

			if (pl.id < MAXPLAYERS)
			{
				if (!followSmoothValid[pl.id])
				{
					followSmoothPos[pl.id] = pos;
					followSmoothValid[pl.id] = true;
				}
				else
				{
					followSmoothPos[pl.id].x = smoothFixed(followSmoothPos[pl.id].x, pos.x);
					followSmoothPos[pl.id].y = smoothFixed(followSmoothPos[pl.id].y, pos.y);
					followSmoothPos[pl.id].z = smoothFixed(followSmoothPos[pl.id].z, pos.z);
				}

				pos = followSmoothPos[pl.id];
			}
		}

		translationref_t translation = ping.translation;
		if (ping.type == PING_ITEM)
			translation = {};
		else if (ping.type == PING_GENERAL || ping.type == PING_WARNING)
			translation = P_PingReadablePlayerTranslation(pl);
		else if (!translation && pl.mo)
			translation = pl.mo->translation;
		if (ping.type == PING_FLAG && ping.flag_team != TEAM_NONE)
			translation = P_PingTeamTranslation(ping.flag_team);

		R_Add3DHUDSprite(ping.lump, pos, translation, 1.0f, FixedPingScreenPx,
		                 FixedPingScreenPx, false);
	}

	if (G_IsHordeMode())
	{
		AActor* view = consoleplayer().camera ? consoleplayer().camera : consoleplayer().mo;
		const int bossLump = P_PingLumpForType(PING_BOSS);
		if (view && bossLump != -1)
		{
			TThinkerIterator<AActor> iterator;
			while (AActor* actor = iterator.Next())
			{
				if (!P_IsHordeBossForPing(actor))
					continue;

				const fixed_t ax = actor->prevx + FixedMul(render_lerp_amount, actor->x - actor->prevx);
				const fixed_t ay = actor->prevy + FixedMul(render_lerp_amount, actor->y - actor->prevy);
				const fixed_t az = actor->prevz + FixedMul(render_lerp_amount, actor->z - actor->prevz);
				v3fixed_t pos{ax, ay, az + actor->height + StrictHeadOffset};
				pos = smoothActorPos(actor->netid, pos);
				R_Add3DHUDSprite(bossLump, pos, {}, 1.0f, FixedPingScreenPx,
				                 FixedPingScreenPx, false);
			}
		}
	}

	if (G_IsCoopGame() || G_IsTeamGame())
	{
		const int teammateLump = P_PingLumpForType(PING_TEAMMATE);
		if (teammateLump != -1)
		{
			for (const player_t& pl : players)
			{
				if (pl.id == consoleplayer().id || !pl.ingame() || pl.spectator || !pl.mo)
					continue;
				if (pl.playerstate != PST_LIVE)
					continue;
				if (G_IsTeamGame() && pl.userinfo.team != consoleplayer().userinfo.team)
					continue;

				const fixed_t px = pl.mo->prevx + FixedMul(render_lerp_amount, pl.mo->x - pl.mo->prevx);
				const fixed_t py = pl.mo->prevy + FixedMul(render_lerp_amount, pl.mo->y - pl.mo->prevy);
				const fixed_t pz = pl.mo->prevz + FixedMul(render_lerp_amount, pl.mo->z - pl.mo->prevz);
				v3fixed_t pos{px, py, pz + pl.mo->height + StrictHeadOffset};
				pos = smoothActorPos(pl.mo->netid, pos);
				translationref_t trans = P_PingReadablePlayerTranslation(pl);
				R_Add3DHUDSprite(teammateLump, pos, trans, 1.0f, FixedPingScreenPx,
				                 FixedPingScreenPx, false);
			}
		}
	}
#endif
}

//------------------------------------------------------------------------------

BEGIN_COMMAND(player_ping)
{
	if (::gamestate != GS_LEVEL)
	{
		return;
	}

	if (consoleplayer().spectator)
	{
		return;
	}
	
	if (consoleplayer().playerstate != PST_LIVE)
	{
		// Dead players tell no tales.
		return;
	}

#ifdef CLIENT_APP
	// Connected clients must request ping creation from the server so that
	// other clients receive the replicated ping message.
	if (connected)
	{
		buf_t& netBuf = messenger.NetBuf().Obtain();
		MSG_WriteMarker(&netBuf, clc_netcmd);
		MSG_WriteString(&netBuf, "player_ping");
		MSG_WriteByte(&netBuf, 0);
		return;
	}
#endif

	P_PlayerPing(consoleplayer());
}
END_COMMAND(player_ping)
