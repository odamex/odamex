// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2006-2021 by The Odamex Team.
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
//  UMAPINFO/ZDoom name lookups.
//
//-----------------------------------------------------------------------------

#include "odamex.h"

#include "infomap.h"

#include "hashtable.h"

typedef OHashTable<std::string, mobjtype_t> MobjMap;

/**
 * "Class" of mobj, used to determine how to "give" them to players.
 */
enum mobjclass_e
{
	MC_NONE,
	MC_AMMO,
	MC_WEAPON,
	MC_KEY,
	MC_POWER,
	MC_BACKPACK,
};

static MobjMap g_MonsterMap;

static void MapMobj(const mobjtype_t type, const std::string& name, const uint32_t flags)
{
	::g_MonsterMap.insert(MobjMap::value_type(name, type));
}

static void InitMap()
{
	MapMobj(MT_PLAYER, "DoomPlayer", MC_NONE);
	MapMobj(MT_POSSESSED, "ZombieMan", MC_NONE);
	MapMobj(MT_SHOTGUY, "ShotgunGuy", MC_NONE);
	MapMobj(MT_VILE, "Archvile", MC_NONE);
	MapMobj(MT_FIRE, "ArchvileFire", MC_NONE);
	MapMobj(MT_UNDEAD, "Revenant", MC_NONE);
	MapMobj(MT_TRACER, "RevenantTracer", MC_NONE);
	MapMobj(MT_SMOKE, "RevenantTracerSmoke", MC_NONE);
	MapMobj(MT_FATSO, "Fatso", MC_NONE);
	MapMobj(MT_FATSHOT, "FatShot", MC_NONE);
	MapMobj(MT_CHAINGUY, "ChaingunGuy", MC_NONE);
	MapMobj(MT_TROOP, "DoomImp", MC_NONE);
	MapMobj(MT_SERGEANT, "Demon", MC_NONE);
	MapMobj(MT_SHADOWS, "Spectre", MC_NONE);
	MapMobj(MT_HEAD, "Cacodemon", MC_NONE);
	MapMobj(MT_BRUISER, "BaronOfHell", MC_NONE);
	MapMobj(MT_BRUISERSHOT, "BaronBall", MC_NONE);
	MapMobj(MT_KNIGHT, "HellKnight", MC_NONE);
	MapMobj(MT_SKULL, "LostSoul", MC_NONE);
	MapMobj(MT_SPIDER, "SpiderMastermind", MC_NONE);
	MapMobj(MT_BABY, "Arachnotron", MC_NONE);
	MapMobj(MT_CYBORG, "Cyberdemon", MC_NONE);
	MapMobj(MT_PAIN, "PainElemental", MC_NONE);
	MapMobj(MT_WOLFSS, "WolfensteinSS", MC_NONE);
	MapMobj(MT_KEEN, "CommanderKeen", MC_NONE);
	MapMobj(MT_BOSSBRAIN, "BossBrain", MC_NONE);
	MapMobj(MT_BOSSSPIT, "BossEye", MC_NONE);
	MapMobj(MT_BOSSTARGET, "BossTarget", MC_NONE);
	MapMobj(MT_SPAWNSHOT, "SpawnShot", MC_NONE);
	MapMobj(MT_SPAWNFIRE, "SpawnFire", MC_NONE);
	MapMobj(MT_BARREL, "ExplosiveBarrel", MC_NONE);
	MapMobj(MT_TROOPSHOT, "DoomImpBall", MC_NONE);
	MapMobj(MT_HEADSHOT, "CacodemonBall", MC_NONE);
	MapMobj(MT_ROCKET, "Rocket", MC_NONE);
	MapMobj(MT_PLASMA, "PlasmaBall", MC_NONE);
	MapMobj(MT_BFG, "BFGBall", MC_NONE);
	MapMobj(MT_ARACHPLAZ, "ArachnotronPlasma", MC_NONE);
	MapMobj(MT_PUFF, "BulletPuff", MC_NONE);
	MapMobj(MT_BLOOD, "Blood", MC_NONE);
	MapMobj(MT_TFOG, "TeleportFog", MC_NONE);
	MapMobj(MT_IFOG, "ItemFog", MC_NONE);
	MapMobj(MT_TELEPORTMAN, "TeleportDest", MC_NONE);
	MapMobj(MT_EXTRABFG, "BFGExtra", MC_NONE);
	MapMobj(MT_MISC0, "GreenArmor", MC_NONE);
	MapMobj(MT_MISC1, "BlueArmor", MC_NONE);
	MapMobj(MT_MISC2, "HealthBonus", MC_NONE);
	MapMobj(MT_MISC3, "ArmorBonus", MC_NONE);
	MapMobj(MT_MISC4, "BlueCard", MC_KEY);
	MapMobj(MT_MISC5, "RedCard", MC_KEY);
	MapMobj(MT_MISC6, "YellowCard", MC_KEY);
	MapMobj(MT_MISC7, "YellowSkull", MC_KEY);
	MapMobj(MT_MISC8, "RedSkull", MC_KEY);
	MapMobj(MT_MISC9, "BlueSkull", MC_KEY);
	MapMobj(MT_MISC10, "Stimpack", MC_NONE);
	MapMobj(MT_MISC11, "Medikit", MC_NONE);
	MapMobj(MT_MISC12, "Soulsphere", MC_NONE);
	MapMobj(MT_INV, "InvulnerabilitySphere", MC_POWER);
	MapMobj(MT_MISC13, "Berserk", MC_POWER);
	MapMobj(MT_INS, "BlurSphere", MC_POWER);
	MapMobj(MT_MISC14, "RadSuit", MC_POWER);
	MapMobj(MT_MISC15, "Allmap", MC_POWER);
	MapMobj(MT_MISC16, "Infrared", MC_POWER);
	MapMobj(MT_MEGA, "Megasphere", MC_NONE);
	MapMobj(MT_CLIP, "Clip", MC_AMMO);
	MapMobj(MT_MISC17, "ClipBox", MC_AMMO);
	MapMobj(MT_MISC18, "RocketAmmo", MC_AMMO);
	MapMobj(MT_MISC19, "RocketBox", MC_AMMO);
	MapMobj(MT_MISC20, "Cell", MC_AMMO);
	MapMobj(MT_MISC21, "CellPack", MC_AMMO);
	MapMobj(MT_MISC22, "Shell", MC_AMMO);
	MapMobj(MT_MISC23, "ShellBox", MC_AMMO);
	MapMobj(MT_MISC24, "Backpack", MC_BACKPACK);
	MapMobj(MT_MISC25, "BFG9000", MC_WEAPON);
	MapMobj(MT_CHAINGUN, "Chaingun", MC_WEAPON);
	MapMobj(MT_MISC26, "Chainsaw", MC_WEAPON);
	MapMobj(MT_MISC27, "RocketLauncher", MC_WEAPON);
	MapMobj(MT_MISC28, "PlasmaRifle", MC_WEAPON);
	MapMobj(MT_SHOTGUN, "Shotgun", MC_WEAPON);
	MapMobj(MT_SUPERSHOTGUN, "SuperShotgun", MC_WEAPON);
	MapMobj(MT_MISC29, "TechLamp", MC_NONE);
	MapMobj(MT_MISC30, "TechLamp2", MC_NONE);
	MapMobj(MT_MISC31, "Column", MC_NONE);
	MapMobj(MT_MISC32, "TallGreenColumn", MC_NONE);
	MapMobj(MT_MISC33, "ShortGreenColumn", MC_NONE);
	MapMobj(MT_MISC34, "TallRedColumn", MC_NONE);
	MapMobj(MT_MISC35, "ShortRedColumn", MC_NONE);
	MapMobj(MT_MISC36, "SkullColumn", MC_NONE);
	MapMobj(MT_MISC37, "HeartColumn", MC_NONE);
	MapMobj(MT_MISC38, "EvilEye", MC_NONE);
	MapMobj(MT_MISC39, "FloatingSkull", MC_NONE);
	MapMobj(MT_MISC40, "TorchTree", MC_NONE);
	MapMobj(MT_MISC41, "BlueTorch", MC_NONE);
	MapMobj(MT_MISC42, "GreenTorch", MC_NONE);
	MapMobj(MT_MISC43, "RedTorch", MC_NONE);
	MapMobj(MT_MISC44, "ShortBlueTorch", MC_NONE);
	MapMobj(MT_MISC45, "ShortGreenTorch", MC_NONE);
	MapMobj(MT_MISC46, "ShortRedTorch", MC_NONE);
	MapMobj(MT_MISC47, "Stalagtite", MC_NONE);
	MapMobj(MT_MISC48, "TechPillar", MC_NONE);
	MapMobj(MT_MISC49, "CandleStick", MC_NONE);
	MapMobj(MT_MISC50, "Candelabra", MC_NONE);
	MapMobj(MT_MISC51, "BloodyTwitch", MC_NONE);
	MapMobj(MT_MISC52, "Meat2", MC_NONE);
	MapMobj(MT_MISC53, "Meat3", MC_NONE);
	MapMobj(MT_MISC54, "Meat4", MC_NONE);
	MapMobj(MT_MISC55, "Meat5", MC_NONE);
	MapMobj(MT_MISC56, "NonsolidMeat2", MC_NONE);
	MapMobj(MT_MISC57, "NonsolidMeat4", MC_NONE);
	MapMobj(MT_MISC58, "NonsolidMeat3", MC_NONE);
	MapMobj(MT_MISC59, "NonsolidMeat5", MC_NONE);
	MapMobj(MT_MISC60, "NonsolidTwitch", MC_NONE);
	MapMobj(MT_MISC61, "DeadCacodemon", MC_NONE);
	MapMobj(MT_MISC62, "DeadMarine", MC_NONE);
	MapMobj(MT_MISC63, "DeadZombieMan", MC_NONE);
	MapMobj(MT_MISC64, "DeadDemon", MC_NONE);
	MapMobj(MT_MISC65, "DeadLostSoul", MC_NONE);
	MapMobj(MT_MISC66, "DeadDoomImp", MC_NONE);
	MapMobj(MT_MISC67, "DeadShotgunGuy", MC_NONE);
	MapMobj(MT_MISC68, "GibbedMarine", MC_NONE);
	MapMobj(MT_MISC69, "GibbedMarineExtra", MC_NONE);
	MapMobj(MT_MISC70, "HeadsOnAStick", MC_NONE);
	MapMobj(MT_MISC71, "Gibs", MC_NONE);
	MapMobj(MT_MISC72, "HeadOnAStick", MC_NONE);
	MapMobj(MT_MISC73, "HeadCandles", MC_NONE);
	MapMobj(MT_MISC74, "DeadStick", MC_NONE);
	MapMobj(MT_MISC75, "LiveStick", MC_NONE);
	MapMobj(MT_MISC76, "BigTree", MC_NONE);
	MapMobj(MT_MISC77, "BurningBarrel", MC_NONE);
	MapMobj(MT_MISC78, "HangNoGuts", MC_NONE);
	MapMobj(MT_MISC79, "HangBNoBrain", MC_NONE);
	MapMobj(MT_MISC80, "HangTLookingDown", MC_NONE);
	MapMobj(MT_MISC81, "HangTSkull", MC_NONE);
	MapMobj(MT_MISC82, "HangTLookingUp", MC_NONE);
	MapMobj(MT_MISC83, "HangTNoBrain", MC_NONE);
	MapMobj(MT_MISC84, "ColonGibs", MC_NONE);
	MapMobj(MT_MISC85, "SmallBloodPool", MC_NONE);
	MapMobj(MT_MISC86, "BrainStem", MC_NONE);
	MapMobj(MT_PUSH, "PointPusher", MC_NONE);
	MapMobj(MT_PULL, "PointPuller", MC_NONE);
	MapMobj(MT_DOGS, "MBFHelperDog", MC_NONE);
	MapMobj(MT_PLASMA1, "PlasmaBall1", MC_NONE);
	MapMobj(MT_PLASMA2, "PlasmaBall2", MC_NONE);
	MapMobj(MT_SCEPTRE, "EvilSceptre", MC_NONE);
	MapMobj(MT_BIBLE, "UnholyBible", MC_NONE);
	// MapMobj(MT_MUSICSOURCE, "MusicChanger", MC_NONE);
}

/**
 * @brief Convert a UMAPINFO/ZDoom class name to a MT Mobj index.
 */
mobjtype_t P_NameToMobj(const std::string& name)
{
	if (::g_MonsterMap.empty())
	{
		InitMap();
	}

	MobjMap::iterator it = ::g_MonsterMap.find(name);
	if (it == ::g_MonsterMap.end())
	{
		return MT_NULL;
	}
	return it->second;
}

weapontype_t P_NameToWeapon(const std::string& name)
{
	if (name == "Fist")
	{
		return wp_fist;
	}
	else if (name == "Pistol")
	{
		return wp_pistol;
	}
	else if (name == "Shotgun")
	{
		return wp_shotgun;
	}
	else if (name == "Chaingun")
	{
		return wp_chaingun;
	}
	else if (name == "RocketLauncher")
	{
		return wp_missile;
	}
	else if (name == "PlasmaRifle")
	{
		return wp_plasma;
	}
	else if (name == "BFG9000")
	{
		return wp_bfg;
	}
	else if (name == "Chainsaw")
	{
		return wp_chainsaw;
	}
	else if (name == "SuperShotgun")
	{
		return wp_supershotgun;
	}

	return wp_none;
}
