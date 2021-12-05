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

#include "p_zdoomhexspec.h"

void OnChangedSwitchTexture(line_t* line, int useAgain);
void SV_OnActivatedLine(line_t* line, AActor* mo, const int side,
                        const LineActivationType activationType, const bool bossaction);
bool EV_DoZDoomDoor(DDoor::EVlDoor type, line_t* line, AActor* mo, byte tag,
                    byte speed_byte, int topwait, zdoom_lock_t lock, byte lightTag,
                    bool boomgen, int topcountdown);
bool EV_DoZDoomPillar(DPillar::EPillar type, line_t* line, int tag, fixed_t speed,
                      fixed_t floordist, fixed_t ceilingdist, int crush, bool hexencrush);
int EV_DoZDoomElevator(line_t* line, DElevator::EElevator type, fixed_t speed,
                       fixed_t height, int tag);
int EV_ZDoomFloorCrushStop(int tag);
bool EV_DoZDoomDonut(int tag, line_t* line, fixed_t pillarspeed, fixed_t slimespeed);
bool EV_ZDoomCeilingCrushStop(int tag, bool remove);
void P_HealMobj(AActor* mo, int num);
bool EV_CompatibleTeleport(int tag, line_t* line, int side, AActor* thing, int flags);
void EV_LightSet(int tag, short level);
void EV_LightSetMinNeighbor(int tag);
void EV_LightSetMaxNeighbor(int tag);
void P_ApplySectorFriction(int tag, int value, bool use_thinker);
void P_SetupSectorDamage(sector_t* sector, short amount, byte interval, byte leakrate,
                         unsigned int flags);
void P_AddSectorSecret(sector_t* sector);

EXTERN_CVAR(sv_allowexit)
EXTERN_CVAR(sv_fragexitswitch)

void P_CrossZDoomSpecialLine(line_t* line, int side, AActor* thing, bool bossaction)
{
	if (thing->player)
	{
		P_ActivateZDoomLine(line, thing, side, ML_SPAC_CROSS);
	}
	else if (thing->flags2 & MF2_MCROSS)
	{
		P_ActivateZDoomLine(line, thing, side, ML_SPAC_MCROSS);
	}
	else if (thing->flags2 & MF2_PCROSS)
	{
		P_ActivateZDoomLine(line, thing, side, ML_SPAC_PCROSS);
	}
	else if (line->special == Teleport || line->special == Teleport_NoFog ||
	         line->special == Teleport_Line)
	{ // [RH] Just a little hack for BOOM compatibility
		P_ActivateZDoomLine(line, thing, side, ML_SPAC_MCROSS);
	}
}

lineresult_s P_ActivateZDoomLine(line_t* line, AActor* mo, int side,
                                 unsigned int activationType)
{
	bool repeat;
	bool buttonSuccess;

	lineresult_s result;

	if (!P_CanActivateSpecials(thing, line))
	{
		result.lineexecuted = false;
		result.switchchanged = false;
		return result;
	}

	if (!P_TestActivateZDoomLine(line, mo, side, activationType))
	{
		result.lineexecuted = false;
		result.switchchanged = false;
		return result;
	}

	repeat = (line->flags & ML_REPEATSPECIAL) != 0 && P_HandleSpecialRepeat(line);

	buttonSuccess = P_ExecuteZDoomLineSpecial(line->special, line->args, line, side, mo);

	result.switchchanged = buttonSuccess;
	result.lineexecuted = true;

	if (!repeat && buttonSuccess)
	{ // clear the special on non-retriggerable lines
		line->special = 0;
	}

	SV_OnActivatedLine(line, thing, side, activationType, false);

	if (buttonSuccess && line->flags & map_format.switch_activation)
	{
		if (serverside)
		{
			P_ChangeSwitchTexture(line, repeat, true);
			OnChangedSwitchTexture(line, repeat);
		}
	}

	return result;
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

	if (line->special == Teleport && lineActivation & ML_SPAC_CROSS &&
	    activationType == ML_SPAC_PCROSS && mo && mo->flags & MF_MISSILE)
	{ // Let missiles use regular player teleports
		lineActivation |= ML_SPAC_PCROSS;
	}

	if (!(lineActivation & activationType))
	{
		if (activationType != ML_SPAC_MCROSS || lineActivation != ML_SPAC_CROSS)
		{
			return false;
		}
	}

	if (mo && !mo->player && thing->type != MT_AVATAR && !(mo->flags & MF_MISSILE) &&
	    !(line->flags & ML_MONSTERSCANACTIVATE) &&
	    (activationType != ML_SPAC_MCROSS || !(lineActivation & ML_SPAC_MCROSS)))
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

	if (activationType == ML_SPAC_MCROSS && !(lineActivation & ML_SPAC_MCROSS) &&
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

	if (sector->damage.amount > 0)
	{
		if (sector->flags & SECF_ENDGODMODE)
		{
			player->cheats &= ~CF_GODMODE;
		}

		if (sector->flags & SECF_DMGUNBLOCKABLE || !player->powers[pw_ironfeet] ||
		    (sector->damage.leakrate && P_Random(player->mo) < sector->damage.leakrate))
		{
			if (sector->flags & SECF_HAZARD)
			{
				player->hazardcount += sector->damage.amount;
				player->hazardinterval = sector->damage.interval;
			}
			else
			{
				if (level.time % sector->damage.interval == 0)
				{
					P_DamageMobj(player->mo, NULL, NULL, sector->damage.amount);

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

bool P_ActorInZDoomSector(AActor* mobj)
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

void P_SpawnZDoomSectorSpecial(sector_t* sector, int i)
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
				sectors[s].damage.amount = damage;
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

bool P_ExecuteZDoomLineSpecial(int special, byte* args, line_t* line, int side,
                                   AActor* mo)
{
	bool buttonSuccess = false;

	switch (special)
	{
	case Polyobj_StartLine:
		break;
	case Polyobj_RotateLeft:
		buttonSuccess = EV_RotatePoly(line, args[0], args[1], args[2], 1, false);
		break;
	case Polyobj_RotateRight:
		buttonSuccess = EV_RotatePoly(line, args[0], args[1], args[2], -1, false);
		break;
	case Polyobj_Move:
		buttonSuccess =
		    EV_MovePoly(line, args[0], SPEED(args[1]), BYTEANGLE(args[2]), args[3] * FRACUNIT, false);
		break;
	case Polyobj_ExplicitLine:
		Printf(PRINT_HIGH, "Polyobj_ExplicitLine currently not supported!");
		break;
	case Polyobj_MoveTimes8:
		buttonSuccess = EV_MovePoly(line, args[0], SPEED(args[1]), BYTEANGLE(args[2]),
		                            args[3] * FRACUNIT * 8, false);
		break;
	case Polyobj_DoorSlide:
		buttonSuccess = EV_OpenPolyDoor(line, args[0], SPEED(args[1]), BYTEANGLE(args[2]), args[4],
		                                args[3] * FRACUNIT,
		                PODOOR_SLIDE);
		break;
	case Polyobj_OR_RotateLeft:
		buttonSuccess = EV_RotatePoly(line, args[0], args[1], args[2], 1, true);
		break;
	case Polyobj_OR_RotateRight:
		buttonSuccess = EV_RotatePoly(line, args[0], args[1], args[2], -1, true);
		break;
	case Polyobj_OR_Move:
		buttonSuccess =
		    EV_MovePoly(line, args[0], SPEED(args[1]), BYTEANGLE(args[2]), args[3] * FRACUNIT, true);
		break;
	case Polyobj_OR_MoveTimes8:
		buttonSuccess = EV_MovePoly(line, args[0], SPEED(args[1]), BYTEANGLE(args[2]),
		                            args[3] * FRACUNIT * 8, true);
		break;
	case Line_Horizon:
		// Handled during map start.
		break;
	case Polyobj_DoorSwing:
		buttonSuccess =
		    EV_OpenPolyDoor(line, args[0], args[1], BYTEANGLE(args[2]), args[3], 0, PODOOR_SWING);
		break;
	case Door_Close:
		buttonSuccess = EV_DoZDoomDoor(DDoor::doorClose, line, mo, args[0], args[1], 0, 0,
		                               args[2], false, 0);
		break;
	case Door_Open:
		buttonSuccess = EV_DoZDoomDoor(DDoor::doorOpen, line, mo, args[0], args[1], 0, 0,
		                               args[2], false, 0);
		break;
	case Door_Raise:
		buttonSuccess = EV_DoZDoomDoor(DDoor::doorRaise, line, mo, args[0], args[1],
		                               args[2], 0, args[3], false, 0);
		break;
	case Door_LockedRaise:
		buttonSuccess =
		    EV_DoZDoomDoor(args[2] ? DDoor::doorRaise : DDoor::doorOpen, line, mo,
		                   args[0], args[1], args[2], args[3], args[4], false, 0);
		break;
	case Door_CloseWaitOpen:
		buttonSuccess = EV_DoZDoomDoor(DDoor::genCdO, line, mo, args[0], args[1],
		                               (int)args[2] * 35 / 8, 0, args[3], false, 0);
		break;
	case Door_WaitRaise:
		buttonSuccess = EV_DoZDoomDoor(DDoor::waitRaiseDoor, line, mo, args[0], args[1], args[2],
		                               0, args[4], false, args[3]);
		break;
	case Door_WaitClose:
		buttonSuccess = EV_DoZDoomDoor(DDoor::waitCloseDoor, line, mo, args[0], args[1],
		                               0, 0, args[3], false, args[2]);
		break;
	case Generic_Door: {
		byte tag, lightTag;
		DDoor::EVlDoor type;
		bool boomgen = false;

		switch (args[2] & 63)
		{
		case 0:
			type = DDoor::doorRaise;
			break;
		case 1:
			type = DDoor::doorOpen;
			break;
		case 2:
			type = DDoor::genCdO;
			break;
		case 3:
			type = DDoor::doorClose;
			break;
		default:
			return 0;
		}

		// Boom doesn't allow manual generalized doors to be activated while they move
		if (args[2] & 64)
			boomgen = true;

		if (args[2] & 128)
		{
			tag = 0;
			lightTag = args[0];
		}
		else
		{
			tag = args[0];
			lightTag = 0;
		}

		buttonSuccess =
		    EV_DoZDoomDoor(type, line, mo, tag, args[1], (int)args[3] * 35 / 8, args[4],
		                   lightTag, boomgen, 0);
	}
	break;
	case Pillar_Build:
		buttonSuccess =
		    EV_DoZDoomPillar(DPillar::EPillar::pillarBuild, line, args[0], P_ArgToSpeed(args[1]),
		                     args[2], 0, NO_CRUSH, false);
		break;
	case Pillar_BuildAndCrush:
		buttonSuccess = EV_DoZDoomPillar(DPillar::EPillar::pillarBuild, line, args[0],
		                                 P_ArgToSpeed(args[1]), args[2], 0, args[3],
		                                 P_ArgToCrushType(args[4]));
		break;
	case Pillar_Open:
		buttonSuccess =
		    EV_DoZDoomPillar(DPillar::EPillar::pillarOpen, line, args[0],
		                     P_ArgToSpeed(args[1]), args[2], args[3], NO_CRUSH, false);
		break;
	case Elevator_MoveToFloor:
		buttonSuccess = EV_DoZDoomElevator(line, DElevator::EElevator::elevateCurrent,
		                                   P_ArgToSpeed(args[1]), 0, args[0]);
		break;
	case Elevator_RaiseToNearest:
		buttonSuccess = EV_DoZDoomElevator(line, DElevator::EElevator::elevateUp,
		                                   P_ArgToSpeed(args[1]), 0, args[0]);
		break;
	case Elevator_LowerToNearest:
		buttonSuccess = EV_DoZDoomElevator(line, DElevator::EElevator::elevateDown,
		                                   P_ArgToSpeed(args[1]), 0, args[0]);
		break;
	case FloorAndCeiling_LowerByValue:
		buttonSuccess = EV_DoZDoomElevator(line, DElevator::EElevator::elevateLower,
		                                   P_ArgToSpeed(args[1]), args[2], args[0]);
		break;
	case FloorAndCeiling_RaiseByValue:
		buttonSuccess = EV_DoZDoomElevator(line, DElevator::EElevator::elevateRaise,
		                                   P_ArgToSpeed(args[1]), args[2], args[0]);
		break;
	case Floor_LowerByValue:
		buttonSuccess =
		    EV_DoZDoomFloor(DFloor::EFloor::floorLowerByValue, line, args[0], args[1], args[2], NO_CRUSH,
		                    P_ArgToChange(args[3]), false, false);
		break;
	case Floor_LowerToLowest:
		buttonSuccess =
		    EV_DoZDoomFloor(DFloor::EFloor::floorLowerToLowest, line, args[0], args[1], 0,
		                                NO_CRUSH, P_ArgToChange(args[2]), false, false);
		break;
	case Floor_LowerToHighest:
		buttonSuccess =
		    EV_DoZDoomFloor(DFloor::EFloor::floorLowerToHighest, line, args[0], args[1],
		                    (int)args[2] - 128, NO_CRUSH, 0, false, args[3] == 1);
		break;
	case Floor_LowerToHighestEE:
		buttonSuccess =
		    EV_DoZDoomFloor(DFloor::EFloor::floorLowerToHighest, line, args[0], args[1],
		                    0, NO_CRUSH, P_ArgToChange(args[2]), false, false);
		break;
	case Floor_LowerToNearest:
		buttonSuccess =
		    EV_DoZDoomFloor(DFloor::EFloor::floorLowerToNearest, line, args[0], args[1],
		                    0, NO_CRUSH, P_ArgToChange(args[2]), false, false);
		break;
	case Floor_RaiseByValue:
		buttonSuccess = EV_DoZDoomFloor(DFloor::EFloor::floorRaiseByValue, line, args[0],
		                                args[1], args[2], P_ArgToCrush(args[4]),
		                                P_ArgToChange(args[3]), true, false);
		break;
	case Floor_RaiseToHighest:
		buttonSuccess =
		    EV_DoZDoomFloor(DFloor::EFloor::floorRaiseToHighest, line, args[0], args[1], 0,
		                    P_ArgToCrush(args[3]), P_ArgToChange(args[2]), true, false);
		break;
	case Floor_RaiseToNearest:
		buttonSuccess =
		    EV_DoZDoomFloor(DFloor::EFloor::floorRaiseToNearest, line, args[0], args[1], 0,
		                    P_ArgToCrush(args[3]), P_ArgToChange(args[2]), true, false);
		break;
	case Floor_RaiseToLowest:
		buttonSuccess =
		    EV_DoZDoomFloor(DFloor::EFloor::floorRaiseToLowest, line, args[0], 2, 0,
		                    P_ArgToCrush(args[3]), P_ArgToChange(args[2]), true, false);
		break;
	case Floor_RaiseAndCrush:
		buttonSuccess =
		    EV_DoZDoomFloor(DFloor::EFloor::floorRaiseAndCrush, line, args[0], args[1], 0,
		                                args[2], 0, P_ArgToCrushType(args[3]), false);
		break;
	case Floor_RaiseAndCrushDoom:
		buttonSuccess =
		    EV_DoZDoomFloor(DFloor::EFloor::floorRaiseAndCrushDoom, line, args[0], args[1], 0,
		                                args[2], 0, P_ArgToCrushType(args[3]), false);
		break;
	case Floor_RaiseByValueTimes8:
		buttonSuccess = EV_DoZDoomFloor(DFloor::EFloor::floorRaiseByValue, line, args[0],
		                                args[1], (int)args[2] * 8, P_ArgToCrush(args[4]),
		                                P_ArgToChange(args[3]), true, false);
		break;
	case Floor_LowerByValueTimes8:
		buttonSuccess = EV_DoZDoomFloor(DFloor::EFloor::floorLowerByValue, line, args[0],
		                                args[1], (int)args[2] * 8, NO_CRUSH,
		                                P_ArgToChange(args[3]), false, false);
		break;
	case Floor_LowerInstant:
		buttonSuccess = EV_DoZDoomFloor(DFloor::EFloor::floorLowerInstant, line, args[0],
		                                0, (int)args[2] * 8, NO_CRUSH,
		                                P_ArgToChange(args[3]), false, false);
		break;
	case Floor_RaiseInstant:
		buttonSuccess = EV_DoZDoomFloor(DFloor::EFloor::floorRaiseInstant, line, args[0],
		                                0, (int)args[2] * 8, P_ArgToCrush(args[4]),
		                                P_ArgToChange(args[3]), true, false);
		break;
	case Floor_ToCeilingInstant:
		buttonSuccess =
		    EV_DoZDoomFloor(DFloor::EFloor::floorLowerToCeiling, line, args[0], 0, args[3],
		                    P_ArgToCrush(args[2]), P_ArgToChange(args[1]), true, false);
		break;
	case Floor_MoveToValue:
		buttonSuccess =
		    EV_DoZDoomFloor(DFloor::EFloor::floorMoveToValue, line, args[0], args[1],
		                                (int)args[2] * (args[3] ? -1 : 1), NO_CRUSH,
		                                P_ArgToChange(args[4]), false, false);
		break;
	case Floor_MoveToValueTimes8:
		buttonSuccess = EV_DoZDoomFloor(DFloor::EFloor::floorMoveToValue, line, args[0],
		                                args[1], (int)args[2] * 8 * (args[3] ? -1 : 1),
		                                NO_CRUSH, P_ArgToChange(args[4]), false, false);
		break;
	case Floor_RaiseToLowestCeiling:
		buttonSuccess = EV_DoZDoomFloor(DFloor::EFloor::floorRaiseToLowestCeiling, line,
		                                args[0], args[1], 0, P_ArgToCrush(args[3]),
		                                P_ArgToChange(args[2]), true, false);
		break;
	case Floor_LowerToLowest:
		buttonSuccess = EV_DoZDoomFloor(DFloor::EFloor::floorLowerToLowestCeiling, line,
		                                args[0], args[1], args[4], NO_CRUSH,
		                                P_ArgToChange(args[2]), true, false);
		break;
	case Floor_RaiseByTexture:
		buttonSuccess = EV_DoZDoomFloor(DFloor::EFloor::floorRaiseByTexture, line,
		                                args[0], args[1], 0, P_ArgToCrush(args[3]),
		                                P_ArgToChange(args[2]), true, false);
		break;
	case Floor_LowerByTexture:
		buttonSuccess =
		    EV_DoZDoomFloor(DFloor::EFloor::floorLowerByTexture, line, args[0], args[1],
		                    0, NO_CRUSH, P_ArgToChange(args[2]), true, false);
		break;
	case Floor_RaiseToCeiling:
		buttonSuccess = EV_DoZDoomFloor(DFloor::EFloor::floorRaiseToCeiling, line,
		                                args[0], args[1], args[4], P_ArgToCrush(args[3]),
		                                P_ArgToChange(args[2]), true, false);
		break;
	case Floor_RaiseByValueTxTy:
		buttonSuccess =
		    EV_DoZDoomFloor(DFloor::EFloor::floorRaiseAndChange, line, args[0], args[1],
		                    args[2], NO_CRUSH, 0, false, false);
		break;
	case Floor_LowerToLowestTxTy:
		buttonSuccess =
		    EV_DoZDoomFloor(DFloor::EFloor::floorLowerAndChange, line, args[0], args[1],
		                    args[2], NO_CRUSH, 0, false, false);
		break;
	case Generic_Floor: {
		DFloor::EFloor type;
		bool raise_or_lower;
		byte index;

		static DFloor::EFloor floor_type[2][7] = {{
		                                       floorLowerByValue,
		                                       floorLowerToHighest,
		                                       floorLowerToLowest,
		                                       floorLowerToNearest,
		                                       floorLowerToLowestCeiling,
		                                       floorLowerToCeiling,
		                                       floorLowerByTexture,
		                                   },
		                                   {
		                                       floorRaiseByValue,
		                                       floorRaiseToHighest,
		                                       floorRaiseToLowest,
		                                       floorRaiseToNearest,
		                                       floorRaiseToLowestCeiling,
		                                       floorRaiseToCeiling,
		                                       floorRaiseByTexture,
		                                   }};

		raise_or_lower = (args[4] & 8) >> 3;
		index = (args[3] < 7) ? args[3] : 0;
		type = floor_type[raise_or_lower][index];

		buttonSuccess =
		    EV_DoZDoomFloor(type, line, args[0], args[1], args[2],
		                    (args[4] & 16) ? 20 : NO_CRUSH, args[4] & 7, false, false);
	}
	break;
	case Floor_CrushStop:
		buttonSuccess = EV_ZDoomFloorCrushStop(args[0]);
		break;
	case Floor_Donut:
		buttonSuccess =
		    EV_DoZDoomDonut(args[0], line, P_ArgToSpeed(args[1]), P_ArgToSpeed(args[2]));
		break;
	case Ceiling_LowerByValue:
		buttonSuccess = EV_DoZDoomCeiling(
		    DCeiling::ECeiling::ceilLowerByValue, line, args[0], P_ArgToSpeed(args[1]), 0, args[2],
		    P_ArgToCrush(args[4]), 0, P_ArgToChange(args[3]), false);
		break;
	case Ceiling_RaiseByValue:
		buttonSuccess = EV_DoZDoomCeiling(
		    DCeiling::ECeiling::ceilRaiseByValue, line, args[0], P_ArgToSpeed(args[1]), 0,
		    args[2], P_ArgToCrush(args[4]), 0, P_ArgToChange(args[3]), false);
		break;
	case Ceiling_LowerByValueTimes8:
		buttonSuccess = EV_DoZDoomCeiling(
		    DCeiling::ECeiling::ceilLowerByValue, line, args[0],
		                                  P_ArgToSpeed(args[1]), 0, (int)args[2] * 8,
		                                  NO_CRUSH, 0, P_ArgToChange(args[3]), false);
		break;
	case Ceiling_RaiseByValueTimes8:
		buttonSuccess = EV_DoZDoomCeiling(
		    DCeiling::ECeiling::ceilRaiseByValue, line, args[0],
		                                  P_ArgToSpeed(args[1]), 0, (int)args[2] * 8,
		                                  NO_CRUSH, 0, P_ArgToChange(args[3]), false);
		break;
	case Ceiling_CrushAndRaise:
		buttonSuccess =
		    EV_DoZDoomCeiling(DCeiling::ECeiling::ceilCrushAndRaise, line, args[0],
		                      P_ArgToSpeed(args[1]), P_ArgToSpeed(args[1]) / 2, 8,
		                      args[2], 0, 0, P_ArgToCrushMode(args[3], false));
		break;
	case Ceiling_LowerAndCrush:
		buttonSuccess =
		    EV_DoZDoomCeiling(DCeiling::ECeiling::ceilLowerAndCrush, line, args[0],
		                      P_ArgToSpeed(args[1]), P_ArgToSpeed(args[1]), 8, args[2], 0,
		                      0, P_ArgToCrushMode(args[3], args[1] == 8));
		break;
	case Ceiling_LowerAndCrushDist:
		buttonSuccess =
		    EV_DoZDoomCeiling(DCeiling::ECeiling::ceilLowerAndCrush, line, args[0],
		                      P_ArgToSpeed(args[1]), P_ArgToSpeed(args[1]), args[3],
		                      args[2], 0, 0, P_ArgToCrushMode(args[4], args[1] == 8));
		break;
	case Ceiling_CrushRaiseAndStay:
		buttonSuccess =
		    EV_DoZDoomCeiling(DCeiling::ECeiling::ceilCrushRaiseAndStay, line, args[0],
		                      P_ArgToSpeed(args[1]), P_ArgToSpeed(args[1]) / 2, 8,
		                      args[2], 0, 0, P_ArgToCrushMode(args[3], false));
		break;
	case Ceiling_MoveToValueTimes8:
		buttonSuccess = EV_DoZDoomCeiling(DCeiling::ECeiling::ceilMoveToValue, line,
		                                  args[0], P_ArgToSpeed(args[1]), 0,
		                                  (int)args[2] * 8 * (args[3] ? -1 : 1), NO_CRUSH,
		                                  0, P_ArgToChange(args[4]), false);
		break;
	case Ceiling_MoveToValue:
		buttonSuccess =
		    EV_DoZDoomCeiling(DCeiling::ECeiling::ceilMoveToValue, line, args[0],
		                      P_ArgToSpeed(args[1]), 0, (int)args[2] * (args[3] ? -1 : 1),
		                      NO_CRUSH, 0, P_ArgToChange(args[4]), false);
		break;
	case Ceiling_LowerToHighestFloor:
		buttonSuccess =
		    EV_DoZDoomCeiling(DCeiling::ECeiling::ceilLowerToHighestFloor, line, args[0],
		                      P_ArgToSpeed(args[1]), 0, args[4], P_ArgToCrush(args[3]), 0,
		                      P_ArgToChange(args[2]), false);
		break;
	case Ceiling_LowerInstant:
		buttonSuccess = EV_DoZDoomCeiling(
		    DCeiling::ECeiling::ceilLowerInstant, line, args[0], 0, 0, (int)args[2] * 8,
		    P_ArgToCrush(args[4]), 0, P_ArgToChange(args[3]), false);
		break;
	case Ceiling_RaiseInstant:
		buttonSuccess = EV_DoZDoomCeiling(DCeiling::ECeiling::ceilRaiseInstant, line,
		                                  args[0], 0, 0, (int)args[2] * 8, NO_CRUSH, 0,
		                                  P_ArgToChange(args[3]), false);
		break;
	case Ceiling_CrushRaiseAndStayA:
		buttonSuccess = EV_DoZDoomCeiling(
		    DCeiling::ECeiling::ceilCrushRaiseAndStay, line, args[0], P_ArgToSpeed(args[1]),
		    P_ArgToSpeed(args[2]), 0, args[3], 0, 0, P_ArgToCrushMode(args[4], false));
		break;
	case Ceiling_CrushRaiseAndStaySilA:
		buttonSuccess =
		    EV_DoZDoomCeiling(DCeiling::ECeiling::ceilCrushRaiseAndStay, line, args[0],
		                      P_ArgToSpeed(args[1]), P_ArgToSpeed(args[2]), 0, args[3], 1,
		                      0, P_ArgToCrushMode(args[4], false));
		break;
	case Ceiling_CrushAndRaiseA:
		buttonSuccess =
		    EV_DoZDoomCeiling(DCeiling::ECeiling::ceilCrushAndRaise, line, args[0],
		                      P_ArgToSpeed(args[1]), P_ArgToSpeed(args[2]), 0, args[3], 0,
		                      0, P_ArgToCrushMode(args[4], args[1] == 8 && args[2] == 8));
		break;
	case Ceiling_CrushAndRaiseDist:
		buttonSuccess =
		    EV_DoZDoomCeiling(DCeiling::ECeiling::ceilCrushAndRaise, line, args[0],
		                      P_ArgToSpeed(args[2]), P_ArgToSpeed(args[2]), args[1],
		                      args[3], 0, 0, P_ArgToCrushMode(args[4], args[2] == 8));
		break;
	case Ceiling_CrushAndRaiseSilentA:
		buttonSuccess =
		    EV_DoZDoomCeiling(DCeiling::ECeiling::ceilCrushAndRaise, line, args[0],
		                      P_ArgToSpeed(args[1]), P_ArgToSpeed(args[2]), 0, args[3], 1,
		                      0, P_ArgToCrushMode(args[4], args[1] == 8 && args[2] == 8));
		break;
	case Ceiling_CrushAndRaiseSilentDist:
		buttonSuccess =
		    EV_DoZDoomCeiling(DCeiling::ECeiling::ceilCrushAndRaise, line, args[0],
		                      P_ArgToSpeed(args[2]),
		                      P_ArgToSpeed(args[2]), args[1], args[3], 1, 0,
		                      P_ArgToCrushMode(args[4], args[2] == 8));
		break;
	case Ceiling_RaiseToNearest:
		buttonSuccess = EV_DoZDoomCeiling(DCeiling::ECeiling::ceilRaiseToNearest, line,
		                                  args[0], P_ArgToSpeed(args[1]), 0, 0, NO_CRUSH,
		                                  P_ArgToChange(args[2]), 0, false);
		break;
	case Ceiling_RaiseToHighest:
		buttonSuccess = EV_DoZDoomCeiling(DCeiling::ECeiling::ceilRaiseToHighest, line,
		                                  args[0], P_ArgToSpeed(args[1]), 0, 0, NO_CRUSH,
		                                  P_ArgToChange(args[2]), 0, false);
		break;
	case Ceiling_RaiseToLowest:
		buttonSuccess = EV_DoZDoomCeiling(DCeiling::ECeiling::ceilRaiseToLowest, line,
		                                  args[0], P_ArgToSpeed(args[1]), 0, 0, NO_CRUSH,
		                                  P_ArgToChange(args[2]), 0, false);
		break;
	case Ceiling_RaiseToHighestFloor:
		buttonSuccess = EV_DoZDoomCeiling(DCeiling::ECeiling::ceilRaiseToHighestFloor,
		                                  line, args[0], P_ArgToSpeed(args[1]), 0, 0,
		                                  NO_CRUSH, P_ArgToChange(args[2]), 0, false);
		break;
	case Ceiling_RaiseByTexture:
		buttonSuccess = EV_DoZDoomCeiling(DCeiling::ECeiling::ceilRaiseByTexture, line,
		                                  args[0], P_ArgToSpeed(args[1]), 0, 0, NO_CRUSH,
		                                  P_ArgToChange(args[2]), 0, false);
		break;
	case Ceiling_LowerToLowest:
		buttonSuccess = EV_DoZDoomCeiling(
		    DCeiling::ECeiling::ceilLowerToLowest, line, args[0], P_ArgToSpeed(args[1]),
		    0, 0, P_ArgToCrush(args[3]), 0, P_ArgToChange(args[2]), false);
		break;
	case Ceiling_LowerToNearest:
		buttonSuccess = EV_DoZDoomCeiling(
		    DCeiling::ECeiling::ceilLowerToNearest, line, args[0], P_ArgToSpeed(args[1]),
		    0, 0, P_ArgToCrush(args[3]), 0, P_ArgToChange(args[2]), false);
		break;
	case Ceiling_ToHighestInstant:
		buttonSuccess = EV_DoZDoomCeiling(
		    DCeiling::ECeiling::ceilLowerToHighest, line, args[0], 2 * FRACUNIT, 0, 0,
		                      P_ArgToCrush(args[2]), 0, P_ArgToChange(args[1]), false);
		break;
	case Ceiling_ToFloorInstant:
		buttonSuccess = EV_DoZDoomCeiling(
		    DCeiling::ECeiling::ceilRaiseToFloor, line, args[0], 2 * FRACUNIT, 0, args[3],
		                      P_ArgToCrush(args[2]), 0, P_ArgToChange(args[1]), false);
		break;
	case Ceiling_LowerToFloor:
		buttonSuccess = EV_DoZDoomCeiling(
		    DCeiling::ECeiling::ceilLowerToFloor, line, args[0], P_ArgToSpeed(args[1]), 0,
		    args[4], P_ArgToCrush(args[3]), 0, P_ArgToChange(args[4]), false);
		break;
	case Ceiling_LowerByTexture:
		buttonSuccess = EV_DoZDoomCeiling(
		    DCeiling::ECeiling::ceilLowerByTexture, line, args[0], P_ArgToSpeed(args[1]),
		    0, 0, P_ArgToCrush(args[3]), 0, P_ArgToChange(args[4]), false);
		break;
	case Generic_Ceiling: {
		DCeiling::ECeiling type;
		bool raise_or_lower;
		byte index;

		static DCeiling::ECeiling ceiling_type[2][7] = {{
		                                         ceilLowerByValue,
		                                         ceilLowerToHighest,
		                                         ceilLowerToLowest,
		                                         ceilLowerToNearest,
		                                         ceilLowerToHighestFloor,
		                                         ceilLowerToFloor,
		                                         ceilLowerByTexture,
		                                     },
		                                     {
		                                         ceilRaiseByValue,
		                                         ceilRaiseToHighest,
		                                         ceilRaiseToLowest,
		                                         ceilRaiseToNearest,
		                                         ceilRaiseToHighestFloor,
		                                         ceilRaiseToFloor,
		                                         ceilRaiseByTexture,
		                                     }};

		raise_or_lower = (args[4] & 8) >> 3;
		index = (args[3] < 7) ? args[3] : 0;
		type = ceiling_type[raise_or_lower][index];

		buttonSuccess = EV_DoZDoomCeiling(
		    type, line, args[0], P_ArgToSpeed(args[1]), P_ArgToSpeed(args[1]), args[2],
		    (args[4] & 16) ? 20 : NO_CRUSH, 0, args[4] & 7, false);
	}
	break;
	case Ceiling_CrushStop: {
		bool remove;

		switch (args[3])
		{
		case 1:
			remove = false;
			break;
		case 2:
			remove = true;
			break;
		default:
			//remove = hexen;
			remove = false;
			break;
		}

		buttonSuccess = EV_ZDoomCeilingCrushStop(args[0], remove);
	}
	break;
	case Generic_Crusher:
		buttonSuccess = EV_DoZDoomCeiling(
		    DCeiling::ECeiling::ceilCrushAndRaise, line, args[0], P_ArgToSpeed(args[1]),
		    P_ArgToSpeed(args[2]), 0, args[4], args[3] ? 2 : 0, 0,
		    (args[1] <= 24 && args[2] <= 24) ? crushSlowdown : crushDoom);
		break;
	case Generic_Crusher2:
		buttonSuccess = EV_DoZDoomCeiling(
		    DCeiling::ECeiling::ceilCrushAndRaise, line, args[0], P_ArgToSpeed(args[1]),
		    P_ArgToSpeed(args[2]), 0, args[4], args[3] ? 2 : 0, 0, crushHexen);
		break;
	case Floor_Waggle:
		buttonSuccess =
		    EV_StartPlaneWaggle(args[0], line, args[1], args[2], args[3], args[4], false);
		break;
	case Ceiling_Waggle:
		buttonSuccess =
		    EV_StartPlaneWaggle(args[0], line, args[1], args[2], args[3], args[4], true);
		break;
	case FloorAndCeiling_LowerRaise:
		buttonSuccess =
		    EV_DoZDoomCeiling(DCeiling::ECeiling::ceilRaiseToHighest, line, args[0],
		                      P_ArgToSpeed(args[2]), 0, 0, 0, 0, 0, false);
		buttonSuccess |= EV_DoZDoomFloor(DFloor::EFloor::floorLowerToLowest, line,
		                                 args[0], args[1], 0, NO_CRUSH, 0, false, false);
		break;
	case Stairs_BuildDown:
		buttonSuccess = EV_BuildStairs(args[0], DFloor::EStair::buildDown, line, args[2],
		                               P_ArgToSpeed(args[1]), args[3], args[4], 0,
		                               STAIR_USE_SPECIALS);
		break;
	case Stairs_BuildUp:
		buttonSuccess = EV_BuildStairs(args[0], DFloor::EStair::buildUp, line, args[2],
		                               P_ArgToSpeed(args[1]), args[3], args[4], 0,
		                               STAIR_USE_SPECIALS);
		break;
	case Stairs_BuildDownSync:
		buttonSuccess = EV_BuildStairs(args[0], DFloor::EStair::buildDown, line, args[2],
		                               P_ArgToSpeed(args[1]), 0, args[3], 0,
		                               STAIR_USE_SPECIALS | STAIR_SYNC);
		break;
	case Stairs_BuildUpSync:
		buttonSuccess = EV_BuildStairs(args[0], DFloor::EStair::buildUp, line, args[2],
		                               P_ArgToSpeed(args[1]), 0, args[3], 0,
		                               STAIR_USE_SPECIALS | STAIR_SYNC);
		break;
	case Stairs_BuildDownDoom:
		buttonSuccess = EV_BuildStairs(args[0], DFloor::EStair::buildDown, line, args[2],
		                        P_ArgToSpeed(args[1]), args[3], args[4], 0, 0);
		break;
	case Stairs_BuildUpDoom:
		buttonSuccess = EV_BuildStairs(args[0], DFloor::EStair::buildUp, line, args[2],
		                        P_ArgToSpeed(args[1]), args[3], args[4], 0, 0);
		break;
	case Stairs_BuildDownDoomSync:
		buttonSuccess = EV_BuildStairs(args[0], DFloor::EStair::buildDown, line, args[2],
		                        P_ArgToSpeed(args[1]), 0, args[3], 0, STAIR_SYNC);
		break;
	case Stairs_BuildUpDoomSync:
		buttonSuccess = EV_BuildStairs(args[0], DFloor::EStair::buildUp, line, args[2],
		                        P_ArgToSpeed(args[1]), 0, args[3], 0, STAIR_SYNC);
		break;
	case Generic_Stairs: {
		DFloor::EStair type;

		type = (args[3] & 1) ? DFloor::EStair::buildUp : DFloor::EStair::buildDown;
		buttonSuccess = EV_BuildStairs(args[0], type, line, args[2],
		                               P_ArgToSpeed(args[1]), 0, args[4], args[3] & 2, 0);

		// Toggle direction of next activation of repeatable stairs
		if (buttonSuccess && line && line->flags & ML_REPEATSPECIAL &&
		    line->special == Generic_Stairs)
		{
			line->arg4 ^= 1; // args[3]
		}
	}
	break;
	case Plat_Stop: {
		bool remove;

		switch (args[3])
		{
		case 1:
			remove = false;
			break;
		case 2:
			remove = true;
			break;
		default:
			remove = false;
			break;
		}

		EV_StopPlat(args[0], remove);
		buttonSuccess = 1;
	}
	break;
	case Plat_PerpetualRaise:
		buttonSuccess = EV_DoPlat(args[0], line, DPlat::EPlatType::platPerpetualRaise, 0,
		                               P_ArgToSpeed(args[1]), args[2], 8, 0);
		break;
	case Plat_PerpetualRaiseLip:
		buttonSuccess = EV_DoPlat(args[0], line, DPlat::EPlatType::platPerpetualRaise, 0,
		                               P_ArgToSpeed(args[1]), args[2], args[3], 0);
		break;
	case Plat_DownWaitUpStay:
		buttonSuccess = EV_DoPlat(args[0], line, DPlat::EPlatType::platDownWaitUpStay, 0,
		                               P_ArgToSpeed(args[1]), args[2], 8, 0);
		break;
	case Plat_DownWaitUpStay:
		buttonSuccess = EV_DoPlat(args[0], line,
		                          args[4] ? DPlat::EPlatType::platDownWaitUpStayStone
		                                  : DPlat::EPlatType::platDownWaitUpStay,
		                          0, P_ArgToSpeed(args[1]), args[2], args[3], 0);
		break;
	case Plat_DownByValue:
		buttonSuccess = EV_DoPlat(args[0], line, DPlat::EPlatType::platDownByValue,
		                          (int)args[3] * 8, P_ArgToSpeed(args[1]), args[2], 0, 0);
		break;
	case Plat_UpByValue:
		buttonSuccess = EV_DoPlat(args[0], line, DPlat::EPlatType::platUpByValue,
		                          (int)args[3] * 8, P_ArgToSpeed(args[1]), args[2], 0, 0);
		break;
	case Plat_UpWaitDownStay:
		buttonSuccess = EV_DoPlat(args[0], line, DPlat::EPlatType::platUpWaitDownStay, 0,
		                               P_ArgToSpeed(args[1]), args[2], 0, 0);
		break;
	case Plat_UpNearestWaitDownStay:
		buttonSuccess =
		    EV_DoPlat(args[0], line, DPlat::EPlatType::platUpNearestWaitDownStay, 0,
		              P_ArgToSpeed(args[1]), args[2], 0, 0);
		break;
	case Plat_RaiseAndStayTx0: {
		DPlat::EPlatType type;

		switch (args[3])
		{
		case 1:
			type = DPlat::EPlatType::platRaiseAndStay;
			break;
		case 2:
			type = DPlat::EPlatType::platRaiseAndStayLockout;
			break;
		default:
			type = DPlat::EPlatType::platRaiseAndStay;
			break;
		}

		buttonSuccess = EV_DoPlat(args[0], line, type, 0, P_ArgToSpeed(args[1]), 0, 0, 1);
	}
	break;
	case Plat_UpByValueStayTx:
		buttonSuccess = EV_DoPlat(args[0], line, DPlat::EPlatType::platUpByValueStay,
		                          (int)args[2] * 8, P_ArgToSpeed(args[1]), 0, 0, 2);
		break;
	case Plat_ToggleCeiling:
		buttonSuccess =
		    EV_DoPlat(args[0], line, DPlat::EPlatType::platToggle, 0, 0, 0, 0, 0);
		break;
	case Generic_Lift: {
		DPlat::EPlatType type;

		switch (args[3])
		{
		case 1:
			type = DPlat::EPlatType::platDownWaitUpStay;
			break;
		case 2:
			type = DPlat::EPlatType::platDownToNearestFloor;
			break;
		case 3:
			type = DPlat::EPlatType::platDownToLowestCeiling;
			break;
		case 4:
			type = DPlat::EPlatType::platPerpetualRaise;
			break;
		default:
			type = DPlat::EPlatType::platUpByValue;
			break;
		}

		buttonSuccess =
		    EV_DoPlat(args[0], line, type, (int)args[4] * 8, P_ArgToSpeed(args[1]),
		                   (int)args[2] * 35 / 8, 0, 0);
	}
	break;
	case Line_SetBlocking:
		if (args[0])
		{
			int i, s;
			static const int flags[] = {ML_BLOCKING,
			                            ML_BLOCKMONSTERS,
			                            ML_BLOCKPLAYERS,
			                            0, // block floaters (not supported)
			                            0, // block projectiles (not supported)
			                            ML_BLOCKEVERYTHING,
			                            0, // railing (not supported)
			                            0, // block use (not supported)
			                            0, // block sight (not supported)
			                            0, // block hitscan (not supported)
			                            ML_SOUNDBLOCK,
			                            -1};

			int setflags = 0;
			int clearflags = 0;

			for (i = 0; flags[i] != -1; i++, args[1] >>= 1, args[2] >>= 1)
			{
				if (args[1] & 1)
					setflags |= flags[i];
				if (args[2] & 1)
					clearflags |= flags[i];
			}

			for (s = -1; (s = P_FindLineFromTag(args[0], s)) >= 0;)
			{
				lines[s].flags = (lines[s].flags & ~clearflags) | setflags;
			}

			buttonSuccess = 1;
		}
		break;
	case Scroll_Wall:
		if (args[0])
		{
			int s;
			int side = !!args[3];

			for (s = -1; (s = P_FindLineFromTag(args[0], s)) >= 0;)
			{
				new DScroller(DScroller::EScrollType::sc_side, args[1], args[2], -1, lines[s].sidenum[side], 0);
			}

			buttonSuccess = 1;
		}
		break;
	case Line_SetTextureOffset:
		if (args[0] && args[3] <= 1)
		{
			int s;
			int sidenum = !!args[3];

			for (s = -1; (s = P_FindLineFromTag(args[0], s)) >= 0;)
			{
				side_t* side = &sides[lines[s].sidenum[sidenum]];
				side->textureoffset = args[1];
				side->rowoffset = args[2];
			}

			buttonSuccess = 1;
		}
		break;
	case Noise_Alert: {
		AActor *target, *emitter;

		if (!args[0])
		{
			target = mo;
		}
		else
		{
			// not supported yet
			target = NULL;
		}

		if (!args[1])
		{
			emitter = mo;
		}
		else
		{
			// not supported yet
			emitter = NULL;
		}

		if (emitter)
		{
			P_NoiseAlert(target, emitter);
		}

		buttonSuccess = 1;
	}
	break;
	case Sector_SetGravity: {
		fixed_t gravity;
		int s = -1;

		if (args[2] > 99)
			args[2] = 99;

		gravity = P_ArgsToFixed(args[1], args[2]);

		while ((s = P_FindSectorFromTag(args[0], s)) >= 0)
			sectors[s].gravity = gravity;
	}
		buttonSuccess = 1;
		break;
	case Sector_SetDamage: {
		int s = -1;
		bool unblockable = false;

		if (args[3] == 0)
		{
			if (args[1] < 20)
			{
				args[4] = 0;
				args[3] = 32;
			}
			else if (args[1] < 50)
			{
				args[4] = 5;
				args[3] = 32;
			}
			else
			{
				unblockable = true;
				args[4] = 0;
				args[3] = 1;
			}
		}

		while ((s = P_FindSectorFromTag(args[0], s)) >= 0)
		{
			sectors[s].damage.amount = args[1];
			sectors[s].damage.interval = args[3];
			sectors[s].damage.leakrate = args[4];
			if (unblockable)
				sectors[s].flags |= SECF_DMGUNBLOCKABLE;
			else
				sectors[s].flags &= ~SECF_DMGUNBLOCKABLE;
		}
	}
		buttonSuccess = 1;
		break;
	case Floor_TransferNumeric:
		buttonSuccess = EV_DoChange(line, numChangeOnly, args[0]);
		break;
	case Floor_TransferTrigger:
		buttonSuccess = EV_DoChange(line, trigChangeOnly, args[0]);
		break;
	case Sector_SetFloorOffset: {
		int s = -1;
		fixed_t xoffs, yoffs;

		xoffs = P_ArgsToFixed(args[1], args[2]);
		yoffs = P_ArgsToFixed(args[3], args[4]);

		while ((s = P_FindSectorFromTag(args[0], s)) >= 0)
		{
			sectors[s].floor_xoffs = xoffs;
			sectors[s].floor_yoffs = yoffs;
		}
	}
		buttonSuccess = 1;
		break;
	case Sector_SetCeilingOffset: {
		int s = -1;
		fixed_t xoffs, yoffs;

		xoffs = P_ArgsToFixed(args[1], args[2]);
		yoffs = P_ArgsToFixed(args[3], args[4]);

		while ((s = P_FindSectorFromTag(args[0], s)) >= 0)
		{
			sectors[s].ceiling_xoffs = xoffs;
			sectors[s].ceiling_yoffs = yoffs;
		}
	}
		buttonSuccess = 1;
		break;
	case HealThing:
		if (mo)
		{
			int max = args[1];

			buttonSuccess = true;

			if (!max || !mo->player)
			{
				P_HealMobj(mo, args[0]);
				break;
			}
			else if (max == 1)
			{
				max = mobjinfo[mo->type].spawnhealth * 2;
			}

			if (mo->health < max)
			{
				mo->health += args[0];
				if (mo->health > max && max > 0)
				{
					mo->health = max;
				}
				mo->player->health = mo->health;
			}
		}
		break;
	case Thing_Stop:
		AActor* target;

		if (args[0] != 0)
		{
			FActorIterator iterator(args[0]);

			while ((target = iterator.Next()))
			{
				target->momx = target->momy = target->momz = 0;
				if (target->player != NULL)
					target->momx = target->momy = 0;

				buttonSuccess = true;
			}
		}
		else if (mo)
		{
			mo->momx = mo->momy = mo->momz = 0;
			if (mo->player != NULL)
				mo->momx = mo->momy = 0;

			buttonSuccess = true;
		}
		break;
	case ThrustThing:
		if (!mo)
			return false;

		angle_t angle = BYTEANGLE(args[0]) >> ANGLETOFINESHIFT;

		mo->momx = args[1] * finecosine[angle];
		mo->momy = args[1] * finesine[angle];

		return true;
		break;
	case DamageThing:
		if (!mo)
			return false;

		if (args[0])
			P_DamageMobj(it, NULL, NULL, args[0], MOD_UNKNOWN);
		else
			P_DamageMobj(it, NULL, NULL, 10000, MOD_UNKNOWN);
		break;
	case ForceField:
		if (mo)
		{
			P_DamageMobj(mo, NULL, NULL, 16);
			P_ThrustMobj(mo, ANG180 + mo->angle, 2048 * 250);
		}
		buttonSuccess = 1;
		break;
	case Clear_ForceField: {
		int s = -1;

		while ((s = P_FindSectorFromTag(args[0], s)) >= 0)
		{
			int i;
			line_t* line;

			buttonSuccess = 1;

			for (i = 0; i < sectors[s].linecount; i++)
			{
				line_t* line = sectors[s].lines[i];

				if (line->backsector && line->special == ForceField)
				{
					line->flags &= ~(ML_BLOCKING | ML_BLOCKEVERYTHING);
					line->special = 0;
					sides[line->sidenum[0]].midtexture = NO_TEXTURE;
					sides[line->sidenum[1]].midtexture = NO_TEXTURE;
				}
			}
		}
	}
	break;
	case Exit_Normal:
		if (!serverside)
		{
			break;
		}
		if (sv_allowexit)
		{

			G_ExitLevel(0, 1); // args[0] is position
		}
		else if (sv_fragexitswitch)
		{
			P_DamageMobj(mo, NULL, NULL, 10000, MOD_EXIT);
		}
		else
		{
			break;
		}
		buttonSuccess = 1;
		break;
	case Exit_Secret:
		if (!serverside)
		{
			break;
		}
		if (sv_allowexit)
		{
			G_SecretExitLevel(0, 1); // args[0] is position
		}
		else if (sv_fragexitswitch)
		{
			P_DamageMobj(mo, NULL, NULL, 10000, MOD_EXIT);
		}
		else
		{
			break;
		}
		buttonSuccess = 1;
		break;
	case ACS_Execute:
		if (!serverside)
			break;

		LevelInfos& levels = getLevelInfos();
		level_pwad_info_t& info = levels.findByNum(arg1);

		if (args[1] == 0 || !info.exists())
		{
			buttonSuccess = P_StartScript(mo, line, args[0], ::level.mapname.c_str(),
			                              side, args[2], args[3], args[4], 0);
		}

		buttonSuccess = P_StartScript(mo, line, args[0], info.mapname.c_str(), side,
		                              args[2], args[3], args[4], 0);
		break;
	case ACS_Suspend:
		if (!serverside)
			break;

		LevelInfos& levels = getLevelInfos();
		level_pwad_info_t& info = levels.findByNum(args[1]);

		if (arg1 == 0 || !info.exists())
			P_SuspendScript(args[0], ::level.mapname.c_str());
		else
			P_SuspendScript(args[0], info.mapname.c_str());

		buttonSuccess = true;
		break;
	case ACS_Terminate:
		if (!serverside)
			break;

		level_pwad_info_t& info = getLevelInfos().findByNum(args[1]);

		if (arg1 == 0 || !info.exists())
			P_TerminateScript(args[0], ::level.mapname.c_str());
		else
			P_TerminateScript(args[0], info.mapname.c_str());

		buttonSuccess = true;
		break;
	case ACS_LockedExecute:
		if (!serverside)
			break;

		if (args[4] && !P_CanUnlockZDoomDoor(mo->player, (zdoom_lock_t)args[4], true))
		{
			buttonSuccess = false;
		}
		else
		{
			LevelInfos& levels = getLevelInfos();
			level_pwad_info_t& info = levels.findByNum(arg1);

			if (args[1] == 0 || !info.exists())
			{
				buttonSuccess = P_StartScript(mo, line, args[0], ::level.mapname.c_str(),
				                              side, args[2], args[3], args[4], 0);
			}

			buttonSuccess = P_StartScript(mo, line, args[0], info.mapname.c_str(), side,
			                              args[2], args[3], args[4], 0);
		}
		break;
	case Radius_Quake:
		buttonSuccess = P_StartQuake(args[4], args[0], args[1], args[2], args[3]);
		break;
	case Teleport: {
		int flags = TELF_DESTFOG;

		if (!args[2])
			flags |= TELF_SOURCEFOG;

		buttonSuccess = EV_CompatibleTeleport(args[1], line, side, mo, flags);
	}
	break;
	case Teleport_NoFog: {
		int flags = 0;

		switch (args[1])
		{
		case 0:
			flags |= TELF_KEEPORIENTATION;
			break;

		case 2:
			if (line)
				flags |= TELF_KEEPORIENTATION | TELF_ROTATEBOOM;
			break;

		case 3:
			if (line)
				flags |= TELF_KEEPORIENTATION | TELF_ROTATEBOOMINVERSE;
			break;

		default:
			break;
		}

		if (args[3])
			flags |= TELF_KEEPHEIGHT;

		buttonSuccess = EV_CompatibleTeleport(args[2], line, side, mo, flags);
	}
	break;
	case Teleport_NoStop: {
		int flags = TELF_DESTFOG | TELF_KEEPVELOCITY;

		if (!args[2])
			flags |= TELF_SOURCEFOG;

		buttonSuccess = EV_CompatibleTeleport(args[1], line, side, mo, flags);
	}
	break;
	case Teleport_Line:
		buttonSuccess = EV_SilentLineTeleport(line, side, mo, args[1], args[2]);
		break;
	case Teleport_NewMap:
		if (!side)
			break;

		LevelInfos& levels = getLevelInfos();
		level_pwad_info_t info = levels.findByNum(arg0);

		if (mo && (info.levelnum != 0 && CheckIfExitIsGood(it)))
		{
			level.nextmap = info.mapname;
			G_ExitLevel(args[0], 1);
			buttonSuccess = true;
		}
		buttonSuccess = false;
		break;
	case Teleport_EndGame:
		if (!side && mo && CheckIfExitIsGood(mo))
		{
			M_CommitWDLLog();
			level.nextmap = "EndGameC";
			G_ExitLevel(0, 1);
			buttonSuccess = true;
		}
		buttonSuccess = false;
		break;
	case Light_RaiseByValue:
		EV_LightChange(args[0], args[1]);
		buttonSuccess = 1;
		break;
	case Light_LowerByValue:
		EV_LightChange(args[0], -(short)args[1]);
		buttonSuccess = 1;
		break;
	case Light_ChangeToValue:
		EV_LightSet(args[0], args[1]);
		buttonSuccess = 1;
		break;
	case Light_MinNeighbor:
		EV_LightSetMinNeighbor(args[0]);
		buttonSuccess = 1;
		break;
	case Light_MaxNeighbor:
		EV_LightSetMaxNeighbor(args[0]);
		buttonSuccess = 1;
		break;
	case Light_Fade:
		EV_StartLightFading(args[0], args[1], args[2]);
		buttonSuccess = 1;
		break;
	case Light_Glow:
		EV_StartLightGlowing(args[0], args[1], args[2], args[3]);
		buttonSuccess = 1;
		break;
	case Light_Flicker:
		EV_StartLightFlickering(args[0], args[1], args[2]);
		buttonSuccess = 1;
		break;
	case Light_Strobe:
		EV_StartLightStrobing(args[0], args[1], args[2], args[3], args[4]);
		buttonSuccess = 1;
		break;
	case Light_StrobeDoom:
		EV_StartLightStrobing(args[0], args[1], args[2]);
		buttonSuccess = 1;
		break;
	case Light_Stop:
		EV_TurnTagLightsOff(args[0]);
		buttonSuccess = 1;
		break;
	case Light_ForceLightning:
		Printf(PRINT_HIGH, "Light_ForceLightning currently not supported!");
		break;
	default:
		break;
	}

	return buttonSuccess;
}
