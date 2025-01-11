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
	MapMobj(MT_MUSICSOURCE, "MusicChanger", MC_NONE);
	MapMobj(MT_AVATAR, "PlayerAvatar", MC_NONE);
	MapMobj(MT_HORDESPAWN, "HordeSpawn", MC_NONE);
	MapMobj(MT_CAREPACK, "CarePackage", MC_NONE);
	// [AM] Deh_Actor_145-149 are reserved.
	MapMobj(MT_EXTRA00, "Deh_Actor_150", MC_NONE);
	MapMobj(MT_EXTRA01, "Deh_Actor_151", MC_NONE);
	MapMobj(MT_EXTRA02, "Deh_Actor_152", MC_NONE);
	MapMobj(MT_EXTRA03, "Deh_Actor_153", MC_NONE);
	MapMobj(MT_EXTRA04, "Deh_Actor_154", MC_NONE);
	MapMobj(MT_EXTRA05, "Deh_Actor_155", MC_NONE);
	MapMobj(MT_EXTRA06, "Deh_Actor_156", MC_NONE);
	MapMobj(MT_EXTRA07, "Deh_Actor_157", MC_NONE);
	MapMobj(MT_EXTRA08, "Deh_Actor_158", MC_NONE);
	MapMobj(MT_EXTRA09, "Deh_Actor_159", MC_NONE);
	MapMobj(MT_EXTRA10, "Deh_Actor_160", MC_NONE);
	MapMobj(MT_EXTRA11, "Deh_Actor_161", MC_NONE);
	MapMobj(MT_EXTRA12, "Deh_Actor_162", MC_NONE);
	MapMobj(MT_EXTRA13, "Deh_Actor_163", MC_NONE);
	MapMobj(MT_EXTRA14, "Deh_Actor_164", MC_NONE);
	MapMobj(MT_EXTRA15, "Deh_Actor_165", MC_NONE);
	MapMobj(MT_EXTRA16, "Deh_Actor_166", MC_NONE);
	MapMobj(MT_EXTRA17, "Deh_Actor_167", MC_NONE);
	MapMobj(MT_EXTRA18, "Deh_Actor_168", MC_NONE);
	MapMobj(MT_EXTRA19, "Deh_Actor_169", MC_NONE);
	MapMobj(MT_EXTRA20, "Deh_Actor_170", MC_NONE);
	MapMobj(MT_EXTRA21, "Deh_Actor_171", MC_NONE);
	MapMobj(MT_EXTRA22, "Deh_Actor_172", MC_NONE);
	MapMobj(MT_EXTRA23, "Deh_Actor_173", MC_NONE);
	MapMobj(MT_EXTRA24, "Deh_Actor_174", MC_NONE);
	MapMobj(MT_EXTRA25, "Deh_Actor_175", MC_NONE);
	MapMobj(MT_EXTRA26, "Deh_Actor_176", MC_NONE);
	MapMobj(MT_EXTRA27, "Deh_Actor_177", MC_NONE);
	MapMobj(MT_EXTRA28, "Deh_Actor_178", MC_NONE);
	MapMobj(MT_EXTRA29, "Deh_Actor_179", MC_NONE);
	MapMobj(MT_EXTRA30, "Deh_Actor_180", MC_NONE);
	MapMobj(MT_EXTRA31, "Deh_Actor_181", MC_NONE);
	MapMobj(MT_EXTRA32, "Deh_Actor_182", MC_NONE);
	MapMobj(MT_EXTRA33, "Deh_Actor_183", MC_NONE);
	MapMobj(MT_EXTRA34, "Deh_Actor_184", MC_NONE);
	MapMobj(MT_EXTRA35, "Deh_Actor_185", MC_NONE);
	MapMobj(MT_EXTRA36, "Deh_Actor_186", MC_NONE);
	MapMobj(MT_EXTRA37, "Deh_Actor_187", MC_NONE);
	MapMobj(MT_EXTRA38, "Deh_Actor_188", MC_NONE);
	MapMobj(MT_EXTRA39, "Deh_Actor_189", MC_NONE);
	MapMobj(MT_EXTRA40, "Deh_Actor_190", MC_NONE);
	MapMobj(MT_EXTRA41, "Deh_Actor_191", MC_NONE);
	MapMobj(MT_EXTRA42, "Deh_Actor_192", MC_NONE);
	MapMobj(MT_EXTRA43, "Deh_Actor_193", MC_NONE);
	MapMobj(MT_EXTRA44, "Deh_Actor_194", MC_NONE);
	MapMobj(MT_EXTRA45, "Deh_Actor_195", MC_NONE);
	MapMobj(MT_EXTRA46, "Deh_Actor_196", MC_NONE);
	MapMobj(MT_EXTRA47, "Deh_Actor_197", MC_NONE);
	MapMobj(MT_EXTRA48, "Deh_Actor_198", MC_NONE);
	MapMobj(MT_EXTRA49, "Deh_Actor_199", MC_NONE);
	MapMobj(MT_EXTRA50, "Deh_Actor_200", MC_NONE);
	MapMobj(MT_EXTRA51, "Deh_Actor_201", MC_NONE);
	MapMobj(MT_EXTRA52, "Deh_Actor_202", MC_NONE);
	MapMobj(MT_EXTRA53, "Deh_Actor_203", MC_NONE);
	MapMobj(MT_EXTRA54, "Deh_Actor_204", MC_NONE);
	MapMobj(MT_EXTRA55, "Deh_Actor_205", MC_NONE);
	MapMobj(MT_EXTRA56, "Deh_Actor_206", MC_NONE);
	MapMobj(MT_EXTRA57, "Deh_Actor_207", MC_NONE);
	MapMobj(MT_EXTRA58, "Deh_Actor_208", MC_NONE);
	MapMobj(MT_EXTRA59, "Deh_Actor_209", MC_NONE);
	MapMobj(MT_EXTRA60, "Deh_Actor_210", MC_NONE);
	MapMobj(MT_EXTRA61, "Deh_Actor_211", MC_NONE);
	MapMobj(MT_EXTRA62, "Deh_Actor_212", MC_NONE);
	MapMobj(MT_EXTRA63, "Deh_Actor_213", MC_NONE);
	MapMobj(MT_EXTRA64, "Deh_Actor_214", MC_NONE);
	MapMobj(MT_EXTRA65, "Deh_Actor_215", MC_NONE);
	MapMobj(MT_EXTRA66, "Deh_Actor_216", MC_NONE);
	MapMobj(MT_EXTRA67, "Deh_Actor_217", MC_NONE);
	MapMobj(MT_EXTRA68, "Deh_Actor_218", MC_NONE);
	MapMobj(MT_EXTRA69, "Deh_Actor_219", MC_NONE);
	MapMobj(MT_EXTRA70, "Deh_Actor_220", MC_NONE);
	MapMobj(MT_EXTRA71, "Deh_Actor_221", MC_NONE);
	MapMobj(MT_EXTRA72, "Deh_Actor_222", MC_NONE);
	MapMobj(MT_EXTRA73, "Deh_Actor_223", MC_NONE);
	MapMobj(MT_EXTRA74, "Deh_Actor_224", MC_NONE);
	MapMobj(MT_EXTRA75, "Deh_Actor_225", MC_NONE);
	MapMobj(MT_EXTRA76, "Deh_Actor_226", MC_NONE);
	MapMobj(MT_EXTRA77, "Deh_Actor_227", MC_NONE);
	MapMobj(MT_EXTRA78, "Deh_Actor_228", MC_NONE);
	MapMobj(MT_EXTRA79, "Deh_Actor_229", MC_NONE);
	MapMobj(MT_EXTRA80, "Deh_Actor_230", MC_NONE);
	MapMobj(MT_EXTRA81, "Deh_Actor_231", MC_NONE);
	MapMobj(MT_EXTRA82, "Deh_Actor_232", MC_NONE);
	MapMobj(MT_EXTRA83, "Deh_Actor_233", MC_NONE);
	MapMobj(MT_EXTRA84, "Deh_Actor_234", MC_NONE);
	MapMobj(MT_EXTRA85, "Deh_Actor_235", MC_NONE);
	MapMobj(MT_EXTRA86, "Deh_Actor_236", MC_NONE);
	MapMobj(MT_EXTRA87, "Deh_Actor_237", MC_NONE);
	MapMobj(MT_EXTRA88, "Deh_Actor_238", MC_NONE);
	MapMobj(MT_EXTRA89, "Deh_Actor_239", MC_NONE);
	MapMobj(MT_EXTRA90, "Deh_Actor_240", MC_NONE);
	MapMobj(MT_EXTRA91, "Deh_Actor_241", MC_NONE);
	MapMobj(MT_EXTRA92, "Deh_Actor_242", MC_NONE);
	MapMobj(MT_EXTRA93, "Deh_Actor_243", MC_NONE);
	MapMobj(MT_EXTRA94, "Deh_Actor_244", MC_NONE);
	MapMobj(MT_EXTRA95, "Deh_Actor_245", MC_NONE);
	MapMobj(MT_EXTRA96, "Deh_Actor_246", MC_NONE);
	MapMobj(MT_EXTRA97, "Deh_Actor_247", MC_NONE);
	MapMobj(MT_EXTRA98, "Deh_Actor_248", MC_NONE);
	MapMobj(MT_EXTRA99, "Deh_Actor_249", MC_NONE);
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

/**
 * @brief Convert a UMAPINFO/ZDoom class name to a MT Mobj index. Case insensitive for UMAPINFO
 */
mobjtype_t P_INameToMobj(const std::string& name)
{
	if (::g_MonsterMap.empty())
	{
		InitMap();
	}

	for (MobjMap::iterator it = ::g_MonsterMap.begin(); it != ::g_MonsterMap.end(); ++it)
	{
		if (iequals(it->first, name))
		{
			return it->second;
		}
	}
	return MT_NULL;
}

std::string P_MobjToName(const mobjtype_t name)
{
	if (::g_MonsterMap.empty())
	{
		InitMap();
	}

	for (MobjMap::iterator it = ::g_MonsterMap.begin(); it != ::g_MonsterMap.end(); ++it)
	{
		if (it->second == name)
		{
			return it->first.c_str();
		}
	}

	return "";
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
