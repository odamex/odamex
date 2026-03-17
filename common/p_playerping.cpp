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
#include "v_video.h"
extern byte* Ranges;
EXTERN_CVAR(cl_showpings)
EXTERN_CVAR(cl_ping_pickups)
EXTERN_CVAR(cl_ping_monsters)
EXTERN_CVAR(cl_ping_flags)
EXTERN_CVAR(cl_mouselook)
EXTERN_CVAR(sv_freelook)
EXTERN_CVAR(hud_marker_teammates)
EXTERN_CVAR(hud_marker_hordeboss)
#endif
#include "../client/sdl/afxres.h"

EXTERN_CVAR(sv_ping_spam_enabled)
EXTERN_CVAR(sv_ping_spam_max_tokens)
EXTERN_CVAR(sv_ping_spam_window)
EXTERN_CVAR(sv_pingsystem)
EXTERN_CVAR(sv_marker_teammates)
EXTERN_CVAR(sv_marker_hordeboss)

//------------------------------------------------------------------------------

namespace
{
struct pingSpamState_t
{
	float tokens = 0.0f;
	int lasttic = 0;
	bool initialized = false;
};

std::array<pingSpamState_t, MAXPLAYERS> g_pingSpamState{};

void P_ResetPingSpamState(player_t& player)
{
	if (player.id < MAXPLAYERS)
	{
		g_pingSpamState[player.id] = pingSpamState_t{};
	}
}

void P_ResetAllPingSpamState()
{
	for (pingSpamState_t& state : g_pingSpamState)
	{
		state = pingSpamState_t{};
	}
}

bool P_ConsumePingToken(player_t& player)
{
	if (!serverside || !sv_ping_spam_enabled)
		return true;
	if (player.id >= MAXPLAYERS)
		return true;

	pingSpamState_t& state = g_pingSpamState[player.id];
	const float maxTokens = (std::max)(1.0f, static_cast<float>(sv_ping_spam_max_tokens));
	const float refillWindow = (std::max)(1.0f, static_cast<float>(sv_ping_spam_window));
	const float refillPerTic = maxTokens / (refillWindow * TICRATE);

	if (!state.initialized)
	{
		state.tokens = maxTokens;
		state.lasttic = ::gametic;
		state.initialized = true;
	}
	else
	{
		const int elapsed = (std::max)(0, ::gametic - state.lasttic);
		state.tokens = (std::min)(maxTokens, state.tokens + refillPerTic * elapsed);
		state.lasttic = ::gametic;
	}

	if (state.tokens < 1.0f)
		return false;

	state.tokens -= 1.0f;
	return true;
}

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
	case PING_DROP:
		return W_GetNumForName("FONTB01");
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

	// Prefer explicit mobj type matching for CTF flag actors.
	switch (actor->type)
	{
	case MT_BFLG:
	case MT_BDWN:
	case MT_BCAR:
		return TEAM_BLUE;
	case MT_RFLG:
	case MT_RDWN:
	case MT_RCAR:
		return TEAM_RED;
	case MT_GFLG:
	case MT_GDWN:
	case MT_GCAR:
		return TEAM_GREEN;
	default:
		break;
	}

	for (int i = TEAM_BLUE; i < NUMTEAMS; i++)
	{
		const TeamInfo* team = GetTeamInfo(static_cast<team_t>(i));
		if (!team)
			continue;

		// Most reliable path: current CTF tracked flag actor for this team.
		if (team->FlagData.actor && team->FlagData.actor == actor)
			return static_cast<team_t>(i);

		if (actor->sprite == team->FlagSprite || actor->sprite == team->FlagDownSprite ||
		    actor->sprite == team->FlagCarrySprite)
		{
			return static_cast<team_t>(i);
		}
	}

	return TEAM_NONE;
}

translationref_t P_PingTeamTranslationInternal(team_t team)
{
#ifdef CLIENT_APP
	static std::array<std::array<byte, 256>, NUMTEAMS> teamTrans{};
	static bool built = false;
	if (!built)
	{
		for (int t = 0; t < NUMTEAMS; t++)
		{
			for (int i = 0; i < 256; i++)
				teamTrans[t][i] = static_cast<byte>(i);
		}

		const palette_t* pal = V_GetDefaultPalette();
		auto buildRamp = [&](team_t t)
		{
			const TeamInfo* teamInfo = GetTeamInfo(t);
			if (!teamInfo)
				return;
			const int tr = teamInfo->Color.getr();
			const int tg = teamInfo->Color.getg();
			const int tb = teamInfo->Color.getb();

			for (int i = 0; i < 16; i++)
			{
				const argb_t src = pal->basecolors[0x70 + i];
				// Preserve source ramp lighting and tint it toward the team hue.
				const int intensity = (src.getr() * 54 + src.getg() * 183 + src.getb() * 19) >> 8;
				const int r = (tr * intensity) / 255;
				const int g = (tg * intensity) / 255;
				const int b = (tb * intensity) / 255;
				teamTrans[t][0x70 + i] = static_cast<byte>(V_BestColor(pal->basecolors, r, g, b));
			}
		};

		buildRamp(TEAM_BLUE);
		buildRamp(TEAM_RED);
		buildRamp(TEAM_GREEN);
		built = true;
	}

	if (team >= TEAM_BLUE && team < NUMTEAMS)
		return translationref_t(teamTrans[team].data());
#endif
	return {};
}

fixed_t P_PingItemTopOffset(const AActor* actor)
{
	if (!actor)
		return 0;
	if (actor->sprite < 0 || actor->sprite >= numsprites)
		return actor->height;

	const spritedef_t& sprdef = sprites[actor->sprite];
	const int frame = actor->frame & FF_FRAMEMASK;
	if (frame < 0 || frame >= sprdef.numframes || sprdef.spriteframes == nullptr)
		return actor->height;

	const spriteframe_t& sprframe = sprdef.spriteframes[frame];
	fixed_t topoff = 0;
	if (!sprframe.rotate)
	{
		topoff = sprframe.topoffset[0];
	}
	else
	{
		for (int i = 0; i < 16; i++)
		{
			if (sprframe.lump[i] == -1)
				continue;
			topoff = (std::max)(topoff, sprframe.topoffset[i]);
		}
	}

	if (topoff <= 0)
		return actor->height;

	return (std::max)(actor->height, topoff);
}

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
	bool allow_autoaim = false;
	fixed_t autoaim_dist = 0;
	ping_filter_t filter{};
} g_pingTrace;

bool PTR_PingTraverse(intercept_t* in)
{
	if (!in)
		return true;

	static constexpr fixed_t PickupMaxPingDist = 1536 * FRACUNIT;
	static constexpr fixed_t PickupAimSpanToleranceMouselook = FRACUNIT / 32;
	static constexpr fixed_t PickupAimCenterToleranceMouselook = FRACUNIT / 28;

	const fixed_t z =
	    g_pingTrace.shootz + FixedMul(g_pingTrace.slope, FixedMul(in->frac, g_pingTrace.range));

	if (!in->isaline)
	{
		AActor* thing = in->d.thing;
		if (!thing || thing == g_pingTrace.source)
			return true;
		if (thing->player)
			return true;
		const bool isPickup = (thing->flags & MF_SPECIAL) != 0;
		const team_t flagTeam = P_PingFlagTeamForActor(thing);
		const bool isMonsterTarget =
		    ((G_IsHordeMode() && P_IsHordeBossForPing(thing)) ||
		     (!thing->player && (thing->flags & MF_SHOOTABLE) != 0));

		if ((flagTeam != TEAM_NONE && !g_pingTrace.filter.flags) ||
		    (flagTeam == TEAM_NONE && isPickup && !g_pingTrace.filter.pickups) ||
		    (isMonsterTarget && !g_pingTrace.filter.monsters))
		{
			// Ignore filtered actor targets so pinging can continue through to
			// other valid targets/world geometry.
			return true;
		}

		if (flagTeam != TEAM_NONE)
		{
			// Flags are rare/high-priority callouts: if the ray intersects a flag actor,
			// accept it directly instead of requiring strict vertical slope matching.
			const fixed_t dist = FixedMul(g_pingTrace.range, in->frac);
			g_pingTrace.target = thing;
			g_pingTrace.hitfrac = std::clamp(in->frac, 0, FRACUNIT);
			g_pingTrace.hit.x = thing->x;
			g_pingTrace.hit.y = thing->y;
			g_pingTrace.hit.z = thing->z + (thing->height >> 1);
			if (dist > 0)
				g_pingTrace.hit.z = g_pingTrace.shootz + FixedMul(g_pingTrace.slope, dist);
			return false;
		}

		if ((thing->flags & MF_SHOOTABLE) == 0 && !isPickup &&
		    flagTeam == TEAM_NONE && thing->player == nullptr)
			return true;

		const fixed_t dist = FixedMul(g_pingTrace.range, in->frac);
		if (dist <= 0)
			return true;

		if (isPickup)
		{
			// Keep item pinging deliberate: no long-range item grabs.
			if (dist > PickupMaxPingDist)
				return true;

			// When freelook-style aiming is active, keep selection close to
			// where the player is actually looking.
			if (g_pingTrace.filter.mouselook)
			{
				const fixed_t itemCenterSlope =
				    FixedDiv((thing->z + (thing->height >> 1)) - g_pingTrace.shootz, dist);
				const fixed_t itemTopSlope =
				    FixedDiv(thing->z + thing->height - g_pingTrace.shootz, dist);
				const fixed_t itemBottomSlope =
				    FixedDiv(thing->z - g_pingTrace.shootz, dist);
				if (g_pingTrace.slope > itemTopSlope + PickupAimSpanToleranceMouselook ||
				    g_pingTrace.slope < itemBottomSlope - PickupAimSpanToleranceMouselook)
					return true;
				if (std::abs(itemCenterSlope - g_pingTrace.slope) >
				    PickupAimCenterToleranceMouselook)
					return true;
			}
		}

		const fixed_t topSlope = FixedDiv(thing->z + thing->height - g_pingTrace.shootz, dist);
		const fixed_t bottomSlope = FixedDiv(thing->z - g_pingTrace.shootz, dist);
		fixed_t hitSlope = g_pingTrace.slope;
		if (topSlope < g_pingTrace.slope || bottomSlope > g_pingTrace.slope)
		{
			if (!g_pingTrace.allow_autoaim || g_pingTrace.autoaim_dist <= 0)
				return true;

			// Clamp desired slope into actor's vertical span and only accept
			// if within the player's configured autoaim range.
			hitSlope = g_pingTrace.slope;
			if (hitSlope > topSlope)
				hitSlope = topSlope;
			else if (hitSlope < bottomSlope)
				hitSlope = bottomSlope;

			fixed_t autoAimAllowance = g_pingTrace.autoaim_dist;
			if (flagTeam != TEAM_NONE)
			{
				// Slightly taller vertical capture window for CTF flags.
				autoAimAllowance = FixedMul(autoAimAllowance, FRACUNIT + (FRACUNIT >> 1));
			}

			if (std::abs(hitSlope - g_pingTrace.slope) > autoAimAllowance)
				return true;
		}

		g_pingTrace.target = thing;
		g_pingTrace.hitfrac = std::clamp(in->frac, 0, FRACUNIT);
		g_pingTrace.hit.x = g_pingTrace.x1 + FixedMul(g_pingTrace.dx, g_pingTrace.hitfrac);
		g_pingTrace.hit.y = g_pingTrace.y1 + FixedMul(g_pingTrace.dy, g_pingTrace.hitfrac);
		g_pingTrace.hit.z = g_pingTrace.shootz + FixedMul(hitSlope, dist);
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

bool P_ClampPingToWorld(v3fixed_t& point, const fixed_t shootz, const fixed_t slope,
                        const fixed_t range, const fixed_t x1, const fixed_t y1,
                        const fixed_t dx, const fixed_t dy, const fixed_t maxfrac)
{
	// Step along the ray to find first floor/ceiling intersection so mouselook
	// floor/ceiling pings land where the shot would hit.
	static constexpr int Steps = 32;
	fixed_t prevFrac = 0;

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
			return true;
		}

		prevFrac = frac;
	}

	return false;
}

v3fixed_t P_TracePingEndpoint(const AActor* source, fixed_t shootz, angle_t angle, fixed_t slope,
                              fixed_t range, AActor*& outTarget, bool& outHit,
                              bool allowAutoAim, const ping_filter_t& filter)
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
	g_pingTrace.allow_autoaim = allowAutoAim;
	g_pingTrace.autoaim_dist =
	    source && source->player ? source->player->userinfo.aimdist : 0;
	g_pingTrace.filter = filter;
	g_pingTrace.hit = {x2, y2, g_pingTrace.shootz + FixedMul(range, slope)};
	g_pingTrace.hitfrac = FRACUNIT;

	P_PathTraverse(x1, y1, x2, y2, PT_ADDLINES | PT_ADDTHINGS, PTR_PingTraverse);
	outHit = g_pingTrace.target != nullptr || g_pingTrace.blocked;
	if (!g_pingTrace.target)
	{
		const bool worldHit = P_ClampPingToWorld(g_pingTrace.hit, g_pingTrace.shootz,
		                                         g_pingTrace.slope, g_pingTrace.range,
		                                         g_pingTrace.x1, g_pingTrace.y1, g_pingTrace.dx,
		                                         g_pingTrace.dy, g_pingTrace.hitfrac);
		outHit = outHit || worldHit;
	}

	outTarget = g_pingTrace.target;
	return g_pingTrace.hit;
}

bool P_IsSpecificPingTarget(const AActor* target)
{
	if (!target)
		return false;
	if ((target->flags & MF_SPECIAL) != 0)
		return true;
	if (P_PingFlagTeamForActor(target) != TEAM_NONE)
		return true;
	if (G_IsHordeMode() && P_IsHordeBossForPing(target))
		return true;
	return !target->player && (target->flags & MF_SHOOTABLE) != 0;
}

bool P_IsMonsterPingTarget(const AActor* target)
{
	if (!target || target->player)
		return false;
	if (P_PingFlagTeamForActor(target) != TEAM_NONE)
		return false;
	if (G_IsHordeMode() && P_IsHordeBossForPing(target))
		return true;
	return (target->flags & MF_SHOOTABLE) != 0;
}

bool P_IsFlagPingTarget(const AActor* target)
{
	return P_PingFlagTeamForActor(target) != TEAM_NONE;
}

AActor* P_FindAssistFlagTarget(const player_t& player, const fixed_t shootz, const fixed_t pitchSlope,
                               const fixed_t maxRange)
{
	if (!player.mo)
		return nullptr;

	AActor* best = nullptr;
	fixed_t bestDist = maxRange + FRACUNIT;
	static constexpr angle_t FlagAssistCone = ANG(9);

	for (int i = TEAM_BLUE; i < NUMTEAMS; i++)
	{
		const TeamInfo* team = GetTeamInfo(static_cast<team_t>(i));
		if (!team || !team->FlagData.actor)
			continue;

		const AActor* actor = team->FlagData.actor.operator->();
		if (!actor || actor == player.mo)
			continue;

		const fixed_t dx = actor->x - player.mo->x;
		const fixed_t dy = actor->y - player.mo->y;
		const fixed_t dist = P_AproxDistance(dx, dy);
		if (dist <= 0 || dist > maxRange)
			continue;

		const angle_t to = R_PointToAngle2(player.mo->x, player.mo->y, actor->x, actor->y);
		const int32_t delta = static_cast<int32_t>(to - player.mo->angle);
		if (std::abs(delta) > static_cast<int32_t>(FlagAssistCone))
			continue;

		const fixed_t centerz = actor->z + (actor->height >> 1);
		const fixed_t desiredSlope = FixedDiv(centerz - shootz, dist);
		fixed_t allowance = player.userinfo.aimdist;
		if (allowance <= 0)
			allowance = FRACUNIT / 2;
		allowance = FixedMul(allowance, FRACUNIT + (FRACUNIT >> 2)); // 1.25x tolerance
		if (std::abs(desiredSlope - pitchSlope) > allowance)
			continue;

		if (!P_CheckSight(player.mo, const_cast<AActor*>(actor)))
			continue;

		if (dist < bestDist)
		{
			bestDist = dist;
			best = const_cast<AActor*>(actor);
		}
	}

	return best;
}
} // namespace

//------------------------------------------------------------------------------

#ifdef CLIENT_APP
translationref_t P_PingReadablePlayerTranslation(const player_t& pl)
{
	if (pl.id < MAXPLAYERS)
		return translationref_t(translationtables + 256 * pl.id, pl.id);

	return pl.mo ? pl.mo->translation : translationref_t{};
}

translationref_t P_PingTeamTranslation(team_t team)
{
	return P_PingTeamTranslationInternal(team);
}
#endif

//------------------------------------------------------------------------------

ping_submit_result_t P_PlayerPing(player_t &player, const ping_filter_t& filter, bool dropAtSelf)
{
	if (!sv_pingsystem)
		return PING_SUBMIT_NONE;

	if (!player.mo || player.spectator)
		return PING_SUBMIT_NONE;

	static constexpr fixed_t PingRange = 64 * 64 * FRACUNIT;
	static constexpr angle_t SpecificAssistStep = ANG(1);
	static constexpr int SpecificAssistSweeps = 5;
	static constexpr int FlagAssistExtraSweeps = 2;

	const int32_t signedPitch = static_cast<int32_t>(player.mo->pitch);
	ping_filter_t effectiveFilter = filter;
	// If the player is pitched away from 0, treat this as freelook-style aiming
	// even when explicit client mouselook state is unavailable.
	if (signedPitch != 0)
		effectiveFilter.mouselook = true;
	const bool nearLevelPitch = std::abs(signedPitch) <= static_cast<int32_t>(ANG(3));
	const bool nearLevelPitchFlags = std::abs(signedPitch) <= static_cast<int32_t>(ANG(6));
	const bool enableActorAutoAim = player.userinfo.aimdist > 0 && nearLevelPitch;

	playerPing_s ping{};
	ping.translation = player.mo->translation;
	ping.pingtic = ::gametic;

	angle_t aimAngle = player.mo->angle;
	const fixed_t pitchSlope =
	    finetangent[FINEANGLES / 4 - (player.mo->pitch >> ANGLETOFINESHIFT)];
	const fixed_t shootz = player.mo->z + player.viewheight;
	AActor* pingTarget = nullptr;
	bool pingHit = false;
	ping.pos = P_TracePingEndpoint(player.mo, shootz, aimAngle, pitchSlope, PingRange, pingTarget,
	                               pingHit, enableActorAutoAim, effectiveFilter);

	// Specific-target assist (~10 degree cone): sweep nearby angles and select
	// a specific ping target (item/monster/boss/flag).
	//
	// We don't get a direct cl_mouselook userinfo bit on the server, so use
	// autoaim range + near-level pitch as a practical proxy for "autoaim mode".
	const bool useSpecificAssist =
	    enableActorAutoAim && !P_IsSpecificPingTarget(pingTarget);
	if (useSpecificAssist)
	{
		for (int sweep = 1; sweep <= SpecificAssistSweeps; sweep++)
		{
			for (int dir = -1; dir <= 1; dir += 2)
			{
				angle_t testAngle = player.mo->angle + dir * sweep * SpecificAssistStep;
				AActor* testTarget = nullptr;
				bool testHit = false;
				v3fixed_t testPos = P_TracePingEndpoint(player.mo, shootz, testAngle, pitchSlope,
				                                        PingRange, testTarget, testHit,
				                                        enableActorAutoAim, effectiveFilter);
				if (P_IsSpecificPingTarget(testTarget))
				{
					aimAngle = testAngle;
					pingTarget = testTarget;
					ping.pos = testPos;
					pingHit = true;
					break;
				}
			}

			if (P_IsSpecificPingTarget(pingTarget))
				break;
		}
	}

	// Slightly wider/taller assist for flags to make quick CTF flag pings easier.
	if (effectiveFilter.flags && !P_IsFlagPingTarget(pingTarget) && player.userinfo.aimdist > 0 &&
	    nearLevelPitchFlags)
	{
		const int flagAssistSweeps = SpecificAssistSweeps + FlagAssistExtraSweeps;
		for (int sweep = 1; sweep <= flagAssistSweeps; sweep++)
		{
			for (int dir = -1; dir <= 1; dir += 2)
			{
				const angle_t testAngle = player.mo->angle + dir * sweep * SpecificAssistStep;
				AActor* testTarget = nullptr;
				bool testHit = false;
				const v3fixed_t testPos = P_TracePingEndpoint(player.mo, shootz, testAngle, pitchSlope,
				                                              PingRange, testTarget, testHit, true,
				                                              effectiveFilter);
				if (P_IsFlagPingTarget(testTarget))
				{
					aimAngle = testAngle;
					pingTarget = testTarget;
					ping.pos = testPos;
					pingHit = true;
					break;
				}
			}

			if (P_IsFlagPingTarget(pingTarget))
				break;
		}
	}

	// Final fallback: search active CTF flag actors directly using cone/slope/sight.
	if (effectiveFilter.flags && !P_IsFlagPingTarget(pingTarget))
	{
		if (AActor* assistFlag = P_FindAssistFlagTarget(player, shootz, pitchSlope, PingRange))
		{
			pingTarget = assistFlag;
			ping.pos.x = assistFlag->x;
			ping.pos.y = assistFlag->y;
			ping.pos.z = assistFlag->z + (assistFlag->height >> 1);
			pingHit = true;
		}
	}

	// Prefer monsters over pickups when both are in the targeting cone.
	if (effectiveFilter.monsters && effectiveFilter.pickups && pingTarget &&
	    (pingTarget->flags & MF_SPECIAL) != 0)
	{
		AActor* const pickupTarget = pingTarget;
		ping_filter_t monsterOnlyFilter = effectiveFilter;
		monsterOnlyFilter.pickups = false;
		monsterOnlyFilter.flags = false;

		AActor* monsterTarget = nullptr;
		bool monsterHit = false;
		v3fixed_t monsterPos =
		    P_TracePingEndpoint(player.mo, shootz, aimAngle, pitchSlope, PingRange, monsterTarget,
		                       monsterHit, enableActorAutoAim, monsterOnlyFilter);

		if (!P_IsMonsterPingTarget(monsterTarget) && enableActorAutoAim)
		{
			for (int sweep = 1; sweep <= SpecificAssistSweeps; sweep++)
			{
				for (int dir = -1; dir <= 1; dir += 2)
				{
					const angle_t testAngle = player.mo->angle + dir * sweep * SpecificAssistStep;
					AActor* testTarget = nullptr;
					bool testHit = false;
					const v3fixed_t testPos = P_TracePingEndpoint(
					    player.mo, shootz, testAngle, pitchSlope, PingRange, testTarget, testHit,
					    enableActorAutoAim, monsterOnlyFilter);

					if (P_IsMonsterPingTarget(testTarget))
					{
						aimAngle = testAngle;
						monsterTarget = testTarget;
						monsterPos = testPos;
						monsterHit = true;
						break;
					}
				}

				if (P_IsMonsterPingTarget(monsterTarget))
					break;
			}
		}

		if (P_IsMonsterPingTarget(monsterTarget))
		{
			// Keep the pickup if it is substantially closer than the monster.
			static constexpr fixed_t PickupCloserRatioNum = 3 * FRACUNIT; // 0.60
			static constexpr fixed_t PickupCloserRatioDen = 5 * FRACUNIT;
			const fixed_t pickupDist =
			    P_AproxDistance(pickupTarget->x - player.mo->x, pickupTarget->y - player.mo->y);
			const fixed_t monsterDist =
			    P_AproxDistance(monsterTarget->x - player.mo->x, monsterTarget->y - player.mo->y);
			const fixed_t pickupThreshold =
			    FixedDiv(FixedMul(monsterDist, PickupCloserRatioNum), PickupCloserRatioDen);

			if (pickupDist > pickupThreshold)
			{
				pingTarget = monsterTarget;
				ping.pos = monsterPos;
				pingHit = monsterHit;
			}
		}
	}

	if (dropAtSelf)
	{
		pingTarget = nullptr;
		pingHit = true;
		ping.pos.x = player.mo->x;
		ping.pos.y = player.mo->y;
		ping.pos.z = player.mo->z + 8 * FRACUNIT;
		ping.target_netid = 0;
		ping.follow_target = false;
		ping.type = PING_DROP;
		ping.flag_team = TEAM_NONE;
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
			ping.target_netid = pingTarget->netid;
			const fixed_t flagTop = P_PingItemTopOffset(pingTarget);
			ping.pos.z = pingTarget->z + flagTop + 8 * FRACUNIT;
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
			const fixed_t itemTop = P_PingItemTopOffset(pingTarget);
			ping.pos.x = pingTarget->x;
			ping.pos.y = pingTarget->y;
			ping.pos.z = pingTarget->z + itemTop + 8 * FRACUNIT;
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

	}
	else
	{
		if (!pingHit)
			return PING_SUBMIT_NONE;

		if (dropAtSelf)
		{
			// Hold-to-drop is always a dedicated self marker, never upgraded.
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
				{
					ping.type = PING_WARNING;
					// Keep warning anchored at the original world ping location.
					ping.pos = prev.pos;
				}
			}
		}
		}

	}

	const bool retapWarning = ping.type == PING_WARNING;
	if (!retapWarning && !P_ConsumePingToken(player))
	{
		return PING_SUBMIT_RATE_LIMITED;
	}

	int lump = P_PingLumpForType(ping.type);
	if (lump == -1)
		return PING_SUBMIT_NONE;
	ping.lump = lump;

	player.player_ping = std::make_unique<playerPing_s>(std::move(ping));
	return retapWarning ? PING_SUBMIT_PLACED_RETAP_WARNING : PING_SUBMIT_PLACED;
}

//------------------------------------------------------------------------------

bool P_IsPingExpired(const playerPing_s& ping)
{
	static constexpr int PingLifetimeTics = 10 * TICRATE;
	if (ping.pingtic < 0)
		return true;
	if (ping.type == PING_FLAG)
	{
		// Expire flag pings when their original flag actor disappears (e.g. a
		// dropped flag gets grabbed/returned and replaced), otherwise use the
		// normal ping lifetime.
		if (ping.target_netid != 0 && P_FindThingById(ping.target_netid) == nullptr)
			return true;
	}

	return (::gametic - ping.pingtic) > PingLifetimeTics;
}

void P_ClearAllPlayerPings()
{
	for (player_t& pl : players)
	{
		pl.player_ping.reset();
	}

	P_ResetAllPingSpamState();
}

void P_ClearPlayerPingState(player_t& player)
{
	player.player_ping.reset();
	P_ResetPingSpamState(player);
}

//------------------------------------------------------------------------------

bool P_ResolvePingPosition(const playerPing_s& ping, v3fixed_t& outPos)
{
	if (ping.type == PING_ITEM && ping.target_netid != 0)
	{
		AActor* target = P_FindThingById(ping.target_netid);
		if (!target)
			return false;

		const fixed_t itemTop = P_PingItemTopOffset(target);
		outPos.x = target->x;
		outPos.y = target->y;
		outPos.z = target->z + itemTop + 8 * FRACUNIT;
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
	if (!sv_pingsystem || !cl_showpings)
		return;

	const bool allowTeammateMarkers = sv_marker_teammates && hud_marker_teammates;
	const bool allowHordeBossMarkers = sv_marker_hordeboss && hud_marker_hordeboss;

	static constexpr fixed_t StrictHeadOffset = 12 * FRACUNIT;
	static constexpr fixed_t BossHeadOffset = 24 * FRACUNIT;
	static constexpr fixed_t FollowPingSmoothing = FRACUNIT / 3;
	static constexpr int FixedPingScreenPx = 56;
	static constexpr int FixedBossScreenPx = 84;
	static constexpr fixed_t PingScaleNearDist = 512 * FRACUNIT;
	static constexpr fixed_t ItemScaleNearDist = 128 * FRACUNIT;
	static constexpr fixed_t ItemScaleFarDist = 768 * FRACUNIT;
	static constexpr fixed_t FlagScaleNearDist = 256 * FRACUNIT;
	static constexpr fixed_t FlagScaleFarDist = 768 * FRACUNIT;
	static constexpr fixed_t MonsterScaleNearDist = 1024 * FRACUNIT;
	static constexpr fixed_t PingScaleFarDist = 2048 * FRACUNIT;
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

	auto scaledPingPx = [&](const ping_type_t type, const v3fixed_t& pos) -> int
	{
		// Only scale item/monster/flag/general/warning pings.
		if (type != PING_ITEM && type != PING_MONSTER && type != PING_FLAG &&
		    type != PING_GENERAL && type != PING_WARNING && type != PING_DROP)
			return FixedPingScreenPx;

		AActor* view = consoleplayer().camera ? consoleplayer().camera : consoleplayer().mo;
		if (!view)
			return FixedPingScreenPx;

		const fixed_t dist = P_AproxDistance(view->x - pos.x, view->y - pos.y);
		const int nearPx = FixedPingScreenPx;
		int farPx = nearPx / 2;

		fixed_t nearDist = PingScaleNearDist;
		fixed_t farDist = PingScaleFarDist;
		if (type == PING_ITEM)
		{
			nearDist = ItemScaleNearDist;
			farDist = ItemScaleFarDist;
			farPx = (nearPx * 2) / 5; // 40%
		}
		else if (type == PING_FLAG)
		{
			nearDist = FlagScaleNearDist;
			farDist = FlagScaleFarDist;
			farPx = nearPx / 2; // 50%
		}
		else if (type == PING_MONSTER)
		{
			nearDist = MonsterScaleNearDist;
		}

		if (dist <= nearDist)
			return nearPx;
		if (dist >= farDist)
			return farPx;

		const fixed_t t = FixedDiv(dist - nearDist, farDist - nearDist);
		const int deltaPx = nearPx - farPx;
		return nearPx - ((deltaPx * t) >> FRACBITS);
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
		if (ping.type == PING_TEAMMATE && !allowTeammateMarkers)
			continue;
		if (ping.type == PING_BOSS && !allowHordeBossMarkers)
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
					const fixed_t headOffset =
					    ping.type == PING_BOSS ? BossHeadOffset : StrictHeadOffset;
					pos.z = tzt + target->height + headOffset;
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
		else if (ping.type == PING_GENERAL || ping.type == PING_WARNING || ping.type == PING_DROP)
			translation = P_PingReadablePlayerTranslation(pl);
		else if (!translation && pl.mo)
			translation = pl.mo->translation;
		if (ping.type == PING_FLAG && ping.flag_team != TEAM_NONE)
			translation = P_PingTeamTranslationInternal(ping.flag_team);

		const int pingPx = ping.type == PING_BOSS ? FixedBossScreenPx : scaledPingPx(ping.type, pos);

		R_Add3DHUDSprite(ping.lump, pos, translation, 1.0f, pingPx, pingPx, false);
	}

	if (G_IsHordeMode() && allowHordeBossMarkers)
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
				v3fixed_t pos{ax, ay, az + actor->height + BossHeadOffset};
				pos = smoothActorPos(actor->netid, pos);
				R_Add3DHUDSprite(bossLump, pos, {}, 1.0f, FixedBossScreenPx,
				                 FixedBossScreenPx, false);
			}
		}
	}

	if ((G_IsCoopGame() || G_IsTeamGame()) && allowTeammateMarkers)
	{
		const int teammateLump = P_PingLumpForType(PING_TEAMMATE);
		if (teammateLump != -1)
		{
			static constexpr int TeammateNearPx = 56;
			static constexpr int TeammateFarPx = 22;
			static constexpr fixed_t TeammateNearDist = 512 * FRACUNIT;
			static constexpr fixed_t TeammateFarDist = 4096 * FRACUNIT;
			AActor* view = consoleplayer().camera ? consoleplayer().camera : consoleplayer().mo;

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

				int teammatePx = TeammateNearPx;
				if (view)
				{
					const fixed_t dist = P_AproxDistance(view->x - px, view->y - py);
					const fixed_t clamped =
					    std::clamp(dist, TeammateNearDist, TeammateFarDist);
					const fixed_t t = FixedDiv(clamped - TeammateNearDist,
					                           TeammateFarDist - TeammateNearDist);
					teammatePx = TeammateNearPx -
					             ((TeammateNearPx - TeammateFarPx) * t >> FRACBITS);

					// Keep teammate icon from exceeding the projected sprite width.
					// This adapts automatically to modded player radii/sprite scales.
					fixed_t projTx = 0, projTy = 0;
					R_RotatePoint(px - viewx, py - viewy, ANG90 - viewangle, projTx, projTy);
					if (projTy > FRACUNIT)
					{
						const int left = R_ProjectPointX(projTx - pl.mo->radius, projTy);
						const int right = R_ProjectPointX(projTx + pl.mo->radius, projTy);
						const int projectedWidth = right >= left ? (right - left) : (left - right);
						if (projectedWidth > 0)
							teammatePx = (std::min)(teammatePx, projectedWidth);
					}
				}
				teammatePx = (std::max)(24, teammatePx);

				translationref_t trans = P_PingReadablePlayerTranslation(pl);
				R_Add3DHUDSprite(teammateLump, pos, trans, 1.0f, teammatePx,
				                 teammatePx, false);
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

	if (!multiplayer)
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
	ping_filter_t filter{};
	if (!cl_showpings)
		return;
	filter.pickups = cl_ping_pickups;
	filter.monsters = cl_ping_monsters;
	filter.flags = cl_ping_flags;
	filter.mouselook = cl_mouselook && sv_freelook;

	// Connected clients must request ping creation from the server so that
	// other clients receive the replicated ping message.
	if (connected)
	{
		buf_t& netBuf = messenger.NetBuf().Obtain();
		MSG_WriteMarker(&netBuf, clc_netcmd);
		MSG_WriteString(&netBuf, "player_ping");
		MSG_WriteByte(&netBuf, 4);
		MSG_WriteString(&netBuf, filter.pickups ? "1" : "0");
		MSG_WriteString(&netBuf, filter.monsters ? "1" : "0");
		MSG_WriteString(&netBuf, filter.flags ? "1" : "0");
		MSG_WriteString(&netBuf, filter.mouselook ? "1" : "0");
		return;
	}
#else
	ping_filter_t filter{};
#endif

	const ping_submit_result_t result = P_PlayerPing(consoleplayer(), filter);
	if (result == PING_SUBMIT_RATE_LIMITED)
	{
		PrintFmt(PRINT_HIGH, "Ping cooling down. Please wait.\n");
	}
}
END_COMMAND(player_ping)

BEGIN_COMMAND(player_ping_self)
{
	if (::gamestate != GS_LEVEL)
		return;
	if (!multiplayer)
		return;
	if (consoleplayer().spectator)
		return;
	if (consoleplayer().playerstate != PST_LIVE)
		return;

#ifdef CLIENT_APP
	ping_filter_t filter{};
	if (!cl_showpings)
		return;
	filter.pickups = cl_ping_pickups;
	filter.monsters = cl_ping_monsters;
	filter.flags = cl_ping_flags;
	filter.mouselook = cl_mouselook && sv_freelook;

	if (connected)
	{
		buf_t& netBuf = messenger.NetBuf().Obtain();
		MSG_WriteMarker(&netBuf, clc_netcmd);
		MSG_WriteString(&netBuf, "player_ping");
		MSG_WriteByte(&netBuf, 5);
		MSG_WriteString(&netBuf, filter.pickups ? "1" : "0");
		MSG_WriteString(&netBuf, filter.monsters ? "1" : "0");
		MSG_WriteString(&netBuf, filter.flags ? "1" : "0");
		MSG_WriteString(&netBuf, filter.mouselook ? "1" : "0");
		MSG_WriteString(&netBuf, "1");
		return;
	}
#else
	ping_filter_t filter{};
#endif

	const ping_submit_result_t result = P_PlayerPing(consoleplayer(), filter, true);
	if (result == PING_SUBMIT_RATE_LIMITED)
	{
		PrintFmt(PRINT_HIGH, "Ping cooling down. Please wait.\n");
	}
}
END_COMMAND(player_ping_self)
