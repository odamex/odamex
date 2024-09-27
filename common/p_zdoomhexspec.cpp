// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1998-2006 by Randy Heit (ZDoom).
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
//  [Blair] Define the ZDoom (Doom in Hexen) format doom map spec.
//  Includes sector specials, linedef types, line crossing.
//
//-----------------------------------------------------------------------------

#include "odamex.h"
#include "p_local.h"
#include "p_lnspec.h"
#include "m_wdlstats.h"
#include "c_cvars.h"
#include "d_player.h"
#include "p_zdoomhexspec.h"

EXTERN_CVAR(sv_allowexit)
EXTERN_CVAR(sv_fragexitswitch)
EXTERN_CVAR(sv_forcewater)

bool P_CrossZDoomSpecialLine(line_t* line, int side, AActor* thing,
                                     bool bossaction)
{
	// Do not teleport on the wrong side
	if (side)
	{
		switch (line->special)
		{
		case Teleport:
		case Teleport_NoFog:
		case Teleport_NewMap:
		case Teleport_EndGame:
		case Teleport_NoStop:
		case Teleport_Line:
			return false;
			break;
		default:
			break;
		}
	}

	if (thing->player)
	{
		return P_ActivateZDoomLine(line, thing, side, ML_SPAC_CROSS);
	}
	else if (thing->flags2 & MF2_MCROSS)
	{
		return P_ActivateZDoomLine(line, thing, side, ML_SPAC_MCROSS);
	}
	else if (thing->flags2 & MF2_PCROSS)
	{
		return P_ActivateZDoomLine(line, thing, side, ML_SPAC_PCROSS);
	}
	else if (line->special == Teleport || line->special == Teleport_NoFog ||
	         line->special == Teleport_Line)
	{ // [RH] Just a little hack for BOOM compatibility
		return P_ActivateZDoomLine(line, thing, side, ML_SPAC_MCROSS);
	}
	return false;
}

bool P_ActivateZDoomLine(line_t* line, AActor* mo, int side,
                                 unsigned int activationType)
{
	bool buttonSuccess;

	// Err...
	// Use the back sides of VERY SPECIAL lines...
	if (side && (line->flags & (ML_SPAC_PUSH | ML_SPAC_USE)))
	{
		switch (line->special)
		{
		case Exit_Secret:
			// Sliding door open&close
			// UNUSED?
			break;

		default:
			return false;
		}
	}

	if (!P_TestActivateZDoomLine(line, mo, side, activationType) ||
	    !P_CanActivateSpecials(mo, line))
	{
		return false;
	}

	buttonSuccess = P_ExecuteZDoomLineSpecial(line->special, line->args, line, side, mo);

	return buttonSuccess;
}

LineActivationType P_LineActivationTypeForSPACFlag(const unsigned int activationType)
{
	if (activationType & (ML_SPAC_CROSS | ML_SPAC_MCROSS | ML_SPAC_PCROSS | ML_SPAC_CROSSTHROUGH))
		return LineCross;
	else if (activationType & ML_SPAC_IMPACT)
		return LineShoot;
	else if (activationType & ML_SPAC_PUSH)
		return LinePush;
	else if (activationType & (ML_SPAC_USE | ML_SPAC_USETHROUGH))
		return LineUse;

	return LineUse;
}

void P_CollectSecretZDoom(sector_t* sector, player_t* player)
{
	P_CollectSecretCommon(sector, player);
}

bool P_TestActivateZDoomLine(line_t* line, AActor* mo, int side,
                             unsigned int activationType)
{
	unsigned int lineActivation;

	lineActivation = line->flags & ML_SPAC_MASK;

	if (line->special == Teleport &&
	    (lineActivation & ML_SPAC_CROSS || lineActivation & ML_SPAC_CROSSTHROUGH) &&
	    activationType & ML_SPAC_PCROSS && mo && mo->flags & MF_MISSILE)
	{ // Let missiles use regular player teleports
		lineActivation |= ML_SPAC_PCROSS;
	}

	if (!(lineActivation & activationType))
	{
		if (!(activationType & ML_SPAC_MCROSS) || !(lineActivation & ML_SPAC_CROSS))
		{
			return false;
		}
	}

	if (mo && !mo->player && mo->type != MT_AVATAR && !(mo->flags & MF_MISSILE) &&
	    !(line->flags & ML_MONSTERSCANACTIVATE) &&
	    (!(activationType & ML_SPAC_MCROSS) || !(lineActivation & ML_SPAC_MCROSS)))
	{
		bool noway = true;

		if ((activationType == ML_SPAC_USE || activationType == ML_SPAC_PUSH) &&
		    line->flags & ML_SECRET)
			return false; // never open secret doors

		switch (activationType)
		{
		case ML_SPAC_USE:
		case ML_SPAC_PUSH:
			switch (line->special)
			{
			case Door_Raise:
				if (line->args[0] == 0 && line->args[1] < 64)
					noway = false;
				break;
			case Teleport:
			case Teleport_NoFog:
				noway = false;
			}
			break;

		case ML_SPAC_MCROSS:
			if (!(lineActivation & ML_SPAC_MCROSS))
			{
				switch (line->special)
				{
				case Door_Raise:
					if (line->args[1] >= 64)
						break;
				case Teleport:
				case Teleport_NoFog:
				case Teleport_Line:
				case Plat_DownWaitUpStay:
				case Plat_DownWaitUpStayLip:
					noway = false;
				}
			}
			else
				noway = false;
			break;

		default:
			noway = false;
		}
		return !noway;
	}

	if (activationType & ML_SPAC_MCROSS && !(lineActivation & ML_SPAC_MCROSS) &&
	    !(line->flags & ML_MONSTERSCANACTIVATE))
	{
		return false;
	}

	return true;
}

//
// P_PlayerInZDoomSector
// Called every tic frame
// that the player origin is in a special sector
// Only runs in ZDoom Doom in Hexen format
//
void P_PlayerInZDoomSector(player_t* player)
{
	// Spectators should not be affected by special sectors
	if (player->spectator)
		return;

	// Falling, not all the way down yet?
	if (player->mo->z != P_FloorHeight(player->mo) && !player->mo->waterlevel)
		return;

	sector_t* sector = player->mo->subsector->sector;

	static const int heretic_carry[5] = {2048 * 5, 2048 * 10, 2048 * 25, 2048 * 30,
	                                     2048 * 35};

	static const int hexen_carry[3] = {2048 * 5, 2048 * 10, 2048 * 25};

	if (sector->damageamount > 0)
	{
		if (sector->flags & SECF_ENDGODMODE)
		{
			player->cheats &= ~CF_GODMODE;
		}

		if (sector->flags & SECF_DMGUNBLOCKABLE || !player->powers[pw_ironfeet] ||
		    (sector->leakrate && P_Random(player->mo) < sector->leakrate))
		{
			if (sector->flags & SECF_HAZARD)
			{
				player->hazardcount += sector->damageamount;
				player->hazardinterval = sector->damageinterval;
			}

			if (sector->special == 0) // ZDoom Static Init Damage
			{
				if (sector->damageamount < 20)
				{
					P_ApplySectorDamageNoRandom(player, sector->damageamount,
					 MOD_UNKNOWN);
				}
				else if (sector->damageamount < 50)
				{
					P_ApplySectorDamage(player, sector->damageamount, 5, MOD_UNKNOWN);
				}
				else
				{
					P_ApplySectorDamageNoWait(player, sector->damageamount,
					 MOD_UNKNOWN);
				}
			}
			else if (level.time % sector->damageinterval == 0)
			{
				P_DamageMobj(player->mo, NULL, NULL, sector->damageamount);
			}

			if (sector->flags & SECF_ENDLEVEL && player->health <= 10)
			{
				if (serverside && sv_allowexit)
				{
					G_ExitLevel(0, 1);
				}
			}

			if (sector->flags & SECF_DMGTERRAINFX)
			{
				// MAP_FORMAT_TODO: damage special effects
			}
		}
	}

	switch (sector->special)
	{
	case dScroll_EastLavaDamage:
		P_ThrustMobj(player->mo, 0, 2048 * 28);
		break;
	case Scroll_Strife_Current:
		int anglespeed;
		fixed_t carryspeed;
		angle_t angle;

		anglespeed = sector->tag - 100;
		carryspeed = (anglespeed % 10) * 4096;
		angle = (anglespeed / 10) * ANG45;
		P_ThrustMobj(player->mo, angle, carryspeed);
		break;
	case Scroll_Carry_East5:
	case Scroll_Carry_East10:
	case Scroll_Carry_East25:
	case Scroll_Carry_East30:
	case Scroll_Carry_East35:
		P_ThrustMobj(player->mo, 0, heretic_carry[sector->special - Scroll_Carry_East5]);
		break;
	case Scroll_Carry_North5:
	case Scroll_Carry_North10:
	case Scroll_Carry_North25:
	case Scroll_Carry_North30:
	case Scroll_Carry_North35:
		P_ThrustMobj(player->mo, ANG90,
		             heretic_carry[sector->special - Scroll_Carry_North5]);
		break;
	case Scroll_Carry_South5:
	case Scroll_Carry_South10:
	case Scroll_Carry_South25:
	case Scroll_Carry_South30:
	case Scroll_Carry_South35:
		P_ThrustMobj(player->mo, ANG270,
		             heretic_carry[sector->special - Scroll_Carry_South5]);
		break;
	case Scroll_Carry_West5:
	case Scroll_Carry_West10:
	case Scroll_Carry_West25:
	case Scroll_Carry_West30:
	case Scroll_Carry_West35:
		P_ThrustMobj(player->mo, ANG180,
		             heretic_carry[sector->special - Scroll_Carry_West5]);
		break;
	case Scroll_North_Slow:
	case Scroll_North_Medium:
	case Scroll_North_Fast:
		P_ThrustMobj(player->mo, ANG90, hexen_carry[sector->special - Scroll_North_Slow]);
		break;
	case Scroll_East_Slow:
	case Scroll_East_Medium:
	case Scroll_East_Fast:
		P_ThrustMobj(player->mo, 0, hexen_carry[sector->special - Scroll_East_Slow]);
		break;
	case Scroll_South_Slow:
	case Scroll_South_Medium:
	case Scroll_South_Fast:
		P_ThrustMobj(player->mo, ANG270,
		             hexen_carry[sector->special - Scroll_South_Slow]);
		break;
	case Scroll_West_Slow:
	case Scroll_West_Medium:
	case Scroll_West_Fast:
		P_ThrustMobj(player->mo, ANG180, hexen_carry[sector->special - Scroll_West_Slow]);
		break;
	case Scroll_NorthWest_Slow:
	case Scroll_NorthWest_Medium:
	case Scroll_NorthWest_Fast:
		P_ThrustMobj(player->mo, ANG135,
		             hexen_carry[sector->special - Scroll_NorthWest_Slow]);
		break;
	case Scroll_NorthEast_Slow:
	case Scroll_NorthEast_Medium:
	case Scroll_NorthEast_Fast:
		P_ThrustMobj(player->mo, ANG45,
		             hexen_carry[sector->special - Scroll_NorthEast_Slow]);
		break;
	case Scroll_SouthEast_Slow:
	case Scroll_SouthEast_Medium:
	case Scroll_SouthEast_Fast:
		P_ThrustMobj(player->mo, ANG315,
		             hexen_carry[sector->special - Scroll_SouthEast_Slow]);
		break;
	case Scroll_SouthWest_Slow:
	case Scroll_SouthWest_Medium:
	case Scroll_SouthWest_Fast:
		P_ThrustMobj(player->mo, ANG225,
		             hexen_carry[sector->special - Scroll_SouthWest_Slow]);
		break;
	default:
		break;
	}

	if (sector->flags & SECF_SECRET)
	{
		P_CollectSecretZDoom(sector, player);
	}
}

bool P_ActorInZDoomSector(AActor* actor)
{
	return false;
}

void P_SpawnZDoomGeneralizedSpecials(sector_t* sector)
{
	int damage_bits = (sector->special & ZDOOM_DAMAGE_MASK) >> 8;

	switch (damage_bits & 3)
	{
	case 0:
		break;
	case 1:
		P_SetupSectorDamage(sector, 5, 32, 0, 0);
		break;
	case 2:
		P_SetupSectorDamage(sector, 10, 32, 0, 0);
		break;
	case 3:
		P_SetupSectorDamage(sector, 20, 32, 5, 0);
		break;
	}

	if (sector->special & ZDOOM_SECRET_MASK)
		P_AddSectorSecret(sector);

	if (sector->special & ZDOOM_FRICTION_MASK)
		sector->flags |= SECF_FRICTION;

	if (sector->special & ZDOOM_PUSH_MASK)
		sector->flags |= SECF_PUSH;
}

void P_SpawnZDoomLights(sector_t* sector)
{
	switch (sector->special)
	{
	// [RH] Hexen-like phased lighting
	case Light_Phased:
		if (IgnoreSpecial)
			break;
		P_SpawnPhasedLight(sector, 80, -1);
		break;
	case LightSequenceStart:
		if (IgnoreSpecial)
			break;
		P_SpawnLightSequence(sector);
		break;
	case dLight_Flicker:
		// FLICKERING LIGHTS
		if (IgnoreSpecial)
			break;
		P_SpawnLightFlash(sector);
		break;
	case dLight_StrobeFast:
		// STROBE FAST
		if (IgnoreSpecial)
			break;
		P_SpawnStrobeFlash(sector, STROBEBRIGHT, FASTDARK, false);
		break;
	case dLight_StrobeSlow:
		// STROBE SLOW
		if (IgnoreSpecial)
			break;
		P_SpawnStrobeFlash(sector, STROBEBRIGHT, SLOWDARK, false);
		break;
	case dLight_Strobe_Hurt:
		// STROBE FAST/DEATH SLIME
		if (IgnoreSpecial)
			break;
		P_SpawnStrobeFlash(sector, STROBEBRIGHT, FASTDARK, false);
		sector->special |= dLight_Strobe_Hurt;
		break;
	case dLight_Glow:
		// GLOWING LIGHT
		if (IgnoreSpecial)
			break;
		P_SpawnGlowingLight(sector);
		break;
	case dLight_StrobeSlowSync:
		// SYNC STROBE SLOW
		if (IgnoreSpecial)
			break;
		P_SpawnStrobeFlash(sector, STROBEBRIGHT, SLOWDARK, true);
		break;
	case dLight_StrobeFastSync:
		// SYNC STROBE FAST
		if (IgnoreSpecial)
			break;
		P_SpawnStrobeFlash(sector, STROBEBRIGHT, FASTDARK, true);
		break;
	case dLight_FireFlicker:
		// fire flickering
		if (IgnoreSpecial)
			break;
		P_SpawnFireFlicker(sector);
		break;

	case dScroll_EastLavaDamage:
		if (IgnoreSpecial)
			break;
		P_SpawnStrobeFlash(sector, STROBEBRIGHT, FASTDARK, false);
		sector->special |= dScroll_EastLavaDamage;
		break;
	case sLight_Strobe_Hurt:
		if (IgnoreSpecial)
			break;
		P_SpawnStrobeFlash(sector, STROBEBRIGHT, FASTDARK, false);
		sector->special |= sLight_Strobe_Hurt;
		break;
	default:
		break;
	}
}

//
// P_SpawnZDoomSectorSpecial
// After the map has been loaded, scan for specials
// that spawn thinkers.
// Only runs for maps in ZDoom (Doom in Hexen) format.
//

void P_SpawnZDoomSectorSpecial(sector_t* sector)
{
	if (!sector->special)
		return;

	P_SpawnZDoomGeneralizedSpecials(sector);

	sector->special &= 0xff;

	P_SpawnZDoomLights(sector);

	switch (sector->special)
	{
	case dScroll_EastLavaDamage:
		if (IgnoreSpecial)
			break;
		P_SetupSectorDamage(sector, 5, 32, 0, SECF_DMGTERRAINFX | SECF_DMGUNBLOCKABLE);
		new DScroller(DScroller::sc_carry, -4, 0, -1, sector - sectors, 0);
		break;
	case sLight_Strobe_Hurt:
	case dDamage_Nukage:
		if (IgnoreSpecial)
			break;
		P_SetupSectorDamage(sector, 5, 32, 0, 0);
		sector->special = 0;
		break;
	case dDamage_Hellslime:
		if (IgnoreSpecial)
			break;
		P_SetupSectorDamage(sector, 10, 32, 0, 0);
		sector->special = 0;
		break;
	case dLight_Strobe_Hurt:
	case dDamage_SuperHellslime:
		if (IgnoreSpecial)
			break;
		P_SetupSectorDamage(sector, 20, 32, 5, 0);
		sector->special = 0;
		break;
	case dDamage_End:
		if (IgnoreSpecial)
			break;
		P_SetupSectorDamage(sector, 20, 32, 0,
		                    SECF_ENDGODMODE | SECF_ENDLEVEL | SECF_DMGUNBLOCKABLE);
		sector->special = 0;
		break;
	case Damage_InstantDeath:
		if (IgnoreSpecial)
			break;
		P_SetupSectorDamage(sector, 10000, 1, 0, SECF_DMGUNBLOCKABLE);
		sector->special = 0;
		break;
	case hDamage_Sludge:
		if (IgnoreSpecial)
			break;
		P_SetupSectorDamage(sector, 4, 32, 0, 0);
		sector->special = 0;
		break;
	case dDamage_LavaWimpy:
		if (IgnoreSpecial)
			break;
		P_SetupSectorDamage(sector, 5, 32, 0, SECF_DMGTERRAINFX | SECF_DMGUNBLOCKABLE);
		sector->special = 0;
		break;
	case dDamage_LavaHefty:
		if (IgnoreSpecial)
			break;
		P_SetupSectorDamage(sector, 8, 32, 0, SECF_DMGTERRAINFX | SECF_DMGUNBLOCKABLE);
		sector->special = 0;
		break;
	case sDamage_Hellslime:
		if (IgnoreSpecial)
			break;
		P_SetupSectorDamage(sector, 2, 32, 0, SECF_HAZARD);
		sector->special = 0;
		break;
	case sDamage_SuperHellslime:
		if (IgnoreSpecial)
			break;
		P_SetupSectorDamage(sector, 4, 32, 0, SECF_HAZARD);
		sector->special = 0;
		break;
	case Sector_Heal:
		if (IgnoreSpecial)
			break;
		P_SetupSectorDamage(sector, -1, 32, 0, 0);
		sector->special = 0;
		break;
	case dSector_DoorCloseIn30:
		if (!serverside)
			break;
		// DOOR CLOSE IN 30 SECONDS
		P_SpawnDoorCloseIn30(sector);
		break;

	case dSector_DoorRaiseIn5Mins:
		if (!serverside)
			break;
		// DOOR RAISE IN 5 MINUTES
		P_SpawnDoorRaiseIn5Mins(sector);
		break;
	case dSector_LowFriction:
		sector->friction = FRICTION_LOW;
		sector->movefactor = 0x269;
		sector->flags |= SECF_FRICTION;
		break;
	case Sector_Hidden:
		sector->flags |= SECF_HIDDEN;
		sector->special = 0;
		break;
	case Sky2:
		sector->sky = PL_SKYFLAT;
		break;
	default:
		if (IgnoreSpecial)
			break;
		// [RH] Try for normal Hexen scroller
		if (sector->special >= Scroll_North_Slow &&
		    sector->special <= Scroll_SouthWest_Fast)
		{
			static signed char hexenScrollies[24][2] = {
			    {0, 1},   {0, 2},   {0, 4},   {-1, 0}, {-2, 0}, {-4, 0},
			    {0, -1},  {0, -2},  {0, -4},  {1, 0},  {2, 0},  {4, 0},
			    {1, 1},   {2, 2},   {4, 4},   {-1, 1}, {-2, 2}, {-4, 4},
			    {-1, -1}, {-2, -2}, {-4, -4}, {1, -1}, {2, -2}, {4, -4}};
			int i = sector->special - Scroll_North_Slow;
			int dx = hexenScrollies[i][0] * (FRACUNIT / 2);
			int dy = hexenScrollies[i][1] * (FRACUNIT / 2);

			new DScroller(DScroller::sc_floor, dx, dy, -1, sector - sectors, 0);
			// Hexen scrolling floors cause the player to move
			// faster than they scroll. I do this just for compatibility
			// with Hexen and recommend using Killough's more-versatile
			// scrollers instead.
			dx = FixedMul(-dx, CARRYFACTOR * 2);
			dy = FixedMul(dy, CARRYFACTOR * 2);
			new DScroller(DScroller::sc_carry, dx, dy, -1, sector - sectors, 0);
		}
		else if (sector->special >= Scroll_Carry_East5 &&
		         sector->special <= Scroll_Carry_East35)
		{ // Heretic scroll special
			// Only east scrollers also scroll the texture
			fixed_t dx =
			    FixedDiv((1 << (sector->special - Scroll_Carry_East5)) << FRACBITS, 2);
			new DScroller(DScroller::sc_floor, dx, 0, -1, sector - sectors, 0);
		}
		break;
	}
}

void P_SpawnZDoomExtra(int i)
{
	switch (lines[i].special)
	{
		int s;
		sector_t* sec;

	// killough 3/7/98:
	// support for drawn heights coming from different sector
	case Transfer_Heights:
		sec = sides[*lines[i].sidenum].sector;
		DPrintf("Sector tagged %d: TransferHeights \n", sec->tag);
		if (sv_forcewater)
		{
			sec->waterzone = 2;
		}
		if (lines[i].args[1] & 2)
		{
			sec->MoreFlags |= SECF_FAKEFLOORONLY;
		}
		if (lines[i].args[1] & 4)
		{
			sec->MoreFlags |= SECF_CLIPFAKEPLANES;
			DPrintf("Sector tagged %d: CLIPFAKEPLANES \n", sec->tag);
		}
		if (lines[i].args[1] & 8)
		{
			sec->waterzone = 1;
			DPrintf("Sector tagged %d: Sets waterzone=1 \n", sec->tag);
		}
		if (lines[i].args[1] & 16)
		{
			sec->MoreFlags |= SECF_IGNOREHEIGHTSEC;
			DPrintf("Sector tagged %d: IGNOREHEIGHTSEC \n", sec->tag);
		}
		if (lines[i].args[1] & 32)
		{
			sec->MoreFlags |= SECF_NOFAKELIGHT;
			DPrintf("Sector tagged %d: NOFAKELIGHTS \n", sec->tag);
		}
		for (s = -1; (s = P_FindSectorFromTag(lines[i].args[0], s)) >= 0;)
		{
			sectors[s].heightsec = sec;
		}

		DPrintf("Sector tagged %d: MoreFlags: %u \n", sec->tag, sec->MoreFlags);
		break;

	// killough 3/16/98: Add support for setting
	// floor lighting independently (e.g. lava)
	case Transfer_FloorLight:
		sec = sides[*lines[i].sidenum].sector;
		for (s = -1; (s = P_FindSectorFromTag(lines[i].args[0], s)) >= 0;)
			sectors[s].floorlightsec = sec;
		break;

	// killough 4/11/98: Add support for setting
	// ceiling lighting independently
	case Transfer_CeilingLight:
		sec = sides[*lines[i].sidenum].sector;
		for (s = -1; (s = P_FindSectorFromTag(lines[i].args[0], s)) >= 0;)
			sectors[s].ceilinglightsec = sec;
		break;

	// [RH] ZDoom Static_Init settings
	case Static_Init:
		switch (lines[i].args[1])
		{
		case Init_Gravity: {
			if (IgnoreSpecial)
				break;
			float grav =
			    ((float)P_AproxDistance(lines[i].dx, lines[i].dy)) / (FRACUNIT * 100.0f);
			for (s = -1; (s = P_FindSectorFromTag(lines[i].args[0], s)) >= 0;)
				sectors[s].gravity = grav;
		}
		break;

			// case Init_Color:
			// handled in P_LoadSideDefs2()

		case Init_Damage: {
			if (IgnoreSpecial)
				break;

			int damage = P_AproxDistance(lines[i].dx, lines[i].dy) >> FRACBITS;
			for (s = -1; (s = P_FindSectorFromTag(lines[i].args[0], s)) >= 0;)
			{
				sectors[s].damageamount = damage;
				sectors[s].damageinterval = 32;
				sectors[s].leakrate = 0;
				sectors[s].mod = MOD_UNKNOWN;
			}
		}
		break;

			// killough 10/98:
			//
			// Support for sky textures being transferred from sidedefs.
			// Allows scrolling and other effects (but if scrolling is
			// used, then the same sector tag needs to be used for the
			// sky sector, the sky-transfer linedef, and the scroll-effect
			// linedef). Still requires user to use F_SKY1 for the floor
			// or ceiling texture, to distinguish floor and ceiling sky.

		case Init_TransferSky:
			for (s = -1; (s = P_FindSectorFromTag(lines[i].args[0], s)) >= 0;)
				sectors[s].sky = (i + 1) | PL_SKYFLAT;
			break;
		}
		break;
	}
}

// Initialize the scrollers
void P_SpawnZDoomScroller(line_t* l, int i)
{
	fixed_t dx = 0; // direction and speed of scrolling
	fixed_t dy = 0;
	int control = -1, accel = 0; // no control sector or acceleration
	int special = l->special;

	// killough 3/7/98: Types 245-249 are same as 250-254 except that the
	// first side's sector's heights cause scrolling when they change, and
	// this linedef controls the direction and speed of the scrolling. The
	// most complicated linedef since donuts, but powerful :)
	//
	// killough 3/15/98: Add acceleration. Types 214-218 are the same but
	// are accelerative.

	if (special == Scroll_Ceiling || special == Scroll_Floor ||
	    special == Scroll_Texture_Model)
	{
		if (l->args[1] & 3)
		{
			// if 1, then displacement
			// if 2, then accelerative (also if 3)
			control = sides[*l->sidenum].sector - sectors;
			if (l->args[1] & 2)
				accel = 1;
		}
		if (special == Scroll_Texture_Model || l->args[1] & 4)
		{
			// The line housing the special controls the
			// direction and speed of scrolling.
			dx = l->dx >> SCROLL_SHIFT;
			dy = l->dy >> SCROLL_SHIFT;
		}
		else
		{
			// The speed and direction are parameters to the special.
			dx = (l->args[3] - 128) * (FRACUNIT / 32);
			dy = (l->args[4] - 128) * (FRACUNIT / 32);
		}
	}

	switch (special)
	{
		register int s;

	case Scroll_Ceiling:
		if (IgnoreSpecial)
			break;

		for (s = -1; (s = P_FindSectorFromTag(l->args[0], s)) >= 0;)
			new DScroller(DScroller::sc_ceiling, -dx, dy, control, s, accel);
		break;

	case Scroll_Floor:
		if (IgnoreSpecial)
			break;

		if (l->args[2] != 1)
			// scroll the floor
			for (s = -1; (s = P_FindSectorFromTag(l->args[0], s)) >= 0;)
				new DScroller(DScroller::sc_floor, -dx, dy, control, s, accel);

		if (l->args[2] > 0)
		{
			// carry objects on the floor
			dx = FixedMul(dx, CARRYFACTOR);
			dy = FixedMul(dy, CARRYFACTOR);
			for (s = -1; (s = P_FindSectorFromTag(l->args[0], s)) >= 0;)
				new DScroller(DScroller::sc_carry, dx, dy, control, s, accel);
		}
		break;

	// killough 3/1/98: scroll wall according to linedef
	// (same direction and speed as scrolling floors)
	case Scroll_Texture_Model:
		if (IgnoreSpecial)
			break;

		for (s = -1; (s = P_FindLineFromID(l->args[0], s)) >= 0;)
			if (s != i)
				new DScroller(dx, dy, lines + s, control, accel);
		break;

	case Scroll_Texture_Offsets:
		if (IgnoreSpecial)
			break;

		if (l->args[0] == 0) // [Blair] Hack in generalized scrollers.
		{
			// killough 3/2/98: scroll according to sidedef offsets
			s = lines[i].sidenum[0];
			new DScroller(DScroller::sc_side, -sides[s].textureoffset, sides[s].rowoffset,
			              -1, s, accel);
		}
		else
		{
			if (l->id == 0)
				Printf(PRINT_HIGH, "Line %d is missing a tag!", i);

			if (l->args[0] > 1024)
				control = sides[*l->sidenum].sector - sectors;

			if (l->args[0] == 1026)
				accel = 1;

			s = lines[i].sidenum[0];
			dx = -sides[s].textureoffset / 8;
			dy = sides[s].rowoffset / 8;
			for (s = -1; (s = P_FindLineFromLineTag(l, s)) >= 0;)
				if (s != i)
					new DScroller(DScroller::sc_side, dx, dy, control,
					              lines[s].sidenum[0], accel);
		}
		break;

	case Scroll_Texture_Left:
		if (IgnoreSpecial)
			break;

		new DScroller(DScroller::sc_side, l->args[0] * (FRACUNIT / 64), 0, -1,
		              lines[i].sidenum[0], accel);
		break;

	case Scroll_Texture_Right:
		if (IgnoreSpecial)
			break;

		new DScroller(DScroller::sc_side, l->args[0] * (-FRACUNIT / 64), 0, -1,
		              lines[i].sidenum[0], accel);
		break;

	case Scroll_Texture_Up:
		if (IgnoreSpecial)
			break;

		new DScroller(DScroller::sc_side, 0, l->args[0] * (FRACUNIT / 64), -1,
		              lines[i].sidenum[0], accel);
		break;

	case Scroll_Texture_Down:
		if (IgnoreSpecial)
			break;

		new DScroller(DScroller::sc_side, 0, l->args[0] * (-FRACUNIT / 64), -1,
		              lines[i].sidenum[0], accel);
		break;

	case Scroll_Texture_Both:
		if (IgnoreSpecial)
			break;

		if (l->args[0] == 0)
		{
			dx = (l->args[1] - l->args[2]) * (FRACUNIT / 64);
			dy = (l->args[4] - l->args[3]) * (FRACUNIT / 64);
			new DScroller(DScroller::sc_side, dx, dy, -1, lines[i].sidenum[0], accel);
		}
		break;
	default:
		l->special = special;
		break;
	}
}

void P_SpawnZDoomFriction(line_t* l)
{
	if (l->special == Sector_SetFriction)
	{
		int value;

		if (l->args[1])
			value = l->args[1] <= 200 ? l->args[1] : 200;
		else
			value = P_AproxDistance(l->dx, l->dy) >> FRACBITS;

		P_ApplySectorFriction(l->args[0], value, false);

		l->special = 0;
	}
}

void P_SpawnZDoomPusher(line_t* l)
{
	register int s;

	switch (l->special)
	{
	case Sector_SetWind: // wind
		for (s = -1; (s = P_FindSectorFromTag(l->args[0], s)) >= 0;)
			new DPusher(DPusher::p_wind, l->args[3] ? l : NULL, l->args[1], l->args[2],
			            NULL, s);
		break;
	case Sector_SetCurrent: // current
		for (s = -1; (s = P_FindSectorFromTag(l->args[0], s)) >= 0;)
			new DPusher(DPusher::p_current, l->args[3] ? l : NULL, l->args[1], l->args[2],
			            NULL, s);
		break;
	case PointPush_SetForce: // push/pull
		if (l->args[0])
		{ // [RH] Find thing by sector
			for (s = -1; (s = P_FindSectorFromTag(l->args[0], s)) >= 0;)
			{
				AActor* thing = P_GetPushThing(s);
				if (thing)
				{ // No MT_P* means no effect
					// [RH] Allow narrowing it down by tid
					if (!l->args[1] || l->args[1] == thing->tid)
						new DPusher(DPusher::p_push, l->args[3] ? l : NULL, l->args[2], 0,
						            thing, s);
				}
			}
		}
		else
		{ // [RH] Find thing by tid
			AActor* thing = NULL;

			while ((thing = AActor::FindByTID(thing, l->args[1])))
			{
				if (thing->type == MT_PUSH || thing->type == MT_PULL)
					new DPusher(DPusher::p_push, l->args[3] ? l : NULL, l->args[2], 0,
					            thing, thing->subsector->sector - sectors);
			}
		}
		break;
	}
}

void P_PostProcessZDoomSidedefSpecial(side_t* sd, mapsidedef_t* msd, sector_t* sec,
                                   int i)
{
	switch (sd->special)
	{
	case Transfer_Heights: // variable colormap via 242 linedef
	                       // [RH] The colormap num we get here isn't really a colormap,
	                       //	  but a packed ARGB word for blending, so we also allow
	                       //	  the blend to be specified directly by the texture names
	                       //	  instead of figuring something out from the colormap.
		P_SetTransferHeightBlends(sd, msd);
		break;

	case Static_Init:
		// [RH] Set sector color and fog
		// upper "texture" is light color
		// lower "texture" is fog color
		{
			unsigned int color = 0xffffff, fog = 0x000000;

			SetTextureNoErr(&sd->bottomtexture, &fog, msd->bottomtexture);
			SetTextureNoErr(&sd->toptexture, &color, msd->toptexture);
			sd->midtexture = R_TextureNumForName(msd->midtexture);

			if (fog != 0x000000 || color != 0xffffff)
			{
				dyncolormap_t* colormap =
				    GetSpecialLights(((argb_t)color).getr(), ((argb_t)color).getg(),
				                     ((argb_t)color).getb(), ((argb_t)fog).getr(),
				                     ((argb_t)fog).getg(), ((argb_t)fog).getb());

				for (int s = 0; s < numsectors; s++)
				{
					if (sectors[s].tag == sd->tag)
						sectors[s].colormap = colormap;
				}
			}
		}
		break;

		/*
		          case TranslucentLine:	// killough 4/11/98: apply translucency to 2s
		   normal texture sd->midtexture = strncasecmp("TRANMAP", msd->midtexture, 8) ?
		                (sd->special = W_CheckNumForName(msd->midtexture)) < 0 ||
		                W_LumpLength(sd->special) != 65536 ?
		                sd->special=0, R_TextureNumForName(msd->midtexture) :
		                    (sd->special++, 0) : (sd->special=0);
		            sd->toptexture = R_TextureNumForName(msd->toptexture);
		            sd->bottomtexture = R_TextureNumForName(msd->bottomtexture);
		            break;
		*/
	default: // normal cases
		sd->midtexture = R_TextureNumForName(msd->midtexture);
		sd->toptexture = R_TextureNumForName(msd->toptexture);
		sd->bottomtexture = R_TextureNumForName(msd->bottomtexture);
		break;
	}
}

bool P_ExecuteZDoomLineSpecial(int special, short* args, line_t* line, int side,
                                   AActor* mo)
{
	return LineSpecials[special](line, mo, args[0], args[1], args[2], args[3],
		                                args[4]);
}

unsigned int P_TranslateZDoomLineFlags(const unsigned int flags)
{
	unsigned int result = flags & 0x1ff;

	const unsigned int spac_to_flags[8] = {ML_SPAC_CROSS,
	                                        ML_SPAC_USE,
	                                        ML_SPAC_MCROSS,
	                                        ML_SPAC_IMPACT,
	                                        ML_SPAC_PUSH,
	                                        ML_SPAC_PCROSS,
	                                        ML_SPAC_USE | ML_PASSUSE,
	                                        ML_SPAC_IMPACT | ML_SPAC_PCROSS};

	// from zdoom-in-hexen to Odamex

	result |= spac_to_flags[GET_HSPAC(flags)];

	if (flags & HML_REPEATSPECIAL)
		result |= ML_REPEATSPECIAL;

	if (flags & ZML_BLOCKPLAYERS)
		result |= ML_BLOCKPLAYERS;

	if (flags & ZML_MONSTERSCANACTIVATE)
		result |= ML_MONSTERSCANACTIVATE;

	if (flags & ZML_BLOCKEVERYTHING)
		result |= ML_BLOCKING | ML_BLOCKEVERYTHING;

	return result;
}

void P_PostProcessZDoomLinedefSpecial(line_t* line)
{
	switch (line->special)
	{ // killough 4/11/98: handle special types
		int j;
	case TranslucentLine: // killough 4/11/98: translucent 2s textures
#if 0
				lump = sides[*line->sidenum].special;		// translucency from sidedef
				if (!line->tag)							// if tag==0,
					line->tranlump = lump;				// affect this linedef only
				else
					for (j=0;j<numlines;j++)			// if tag!=0,
						if (lines[j].tag == ld->tag)	// affect all matching linedefs
							lines[j].tranlump = lump;
#else
	                      // [RH] Second arg controls how opaque it is.
		if (!line->args[0])
			line->lucency = (byte)line->args[1];
		else
			for (j = 0; j < numlines; j++)
				if (lines[j].id == line->args[0])
					lines[j].lucency = (byte)line->args[1];
#endif
		line->special = 0;
		break;
	}
}
