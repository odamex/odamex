// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
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
//  UMAPINFO/ZDoom name lookups.
//
//-----------------------------------------------------------------------------

#include "odamex.h"

#include "infomap.h"

#include "hashtable.h"

#include "c_dispatch.h"

#include "cmdlib.h"

using infomap::givetype_t, infomap::mobjclass_t;

namespace
{

using MobjNameToTypeMap = OHashTable<std::string, mobjclass_t>;
using MobjTypeToNameMap = OHashTable<mobjtype_t, std::string>;

MobjNameToTypeMap g_MonsterMap;
MobjNameToTypeMap g_MonsterMapNoCase;
MobjTypeToNameMap g_InverseMonsterMap;

void MapMobj(const mobjtype_t type, const std::string& name, const std::optional<givetype_t> mclass = std::nullopt)
{
	::g_MonsterMap.emplace(name, mobjclass_t { type, mclass });
	::g_MonsterMapNoCase.emplace(StdStringToLower(name), mobjclass_t { type, mclass });
	::g_InverseMonsterMap.emplace(type, name);
}

} // namespace

void P_InitMobjNameMap()
{
	::g_MonsterMap.clear();
	::g_MonsterMapNoCase.clear();
	::g_InverseMonsterMap.clear();
	MapMobj(MT_PLAYER, "DoomPlayer");
	MapMobj(MT_POSSESSED, "ZombieMan");
	MapMobj(MT_SHOTGUY, "ShotgunGuy");
	MapMobj(MT_VILE, "Archvile");
	MapMobj(MT_FIRE, "ArchvileFire");
	MapMobj(MT_UNDEAD, "Revenant");
	MapMobj(MT_TRACER, "RevenantTracer");
	MapMobj(MT_SMOKE, "RevenantTracerSmoke");
	MapMobj(MT_FATSO, "Fatso");
	MapMobj(MT_FATSHOT, "FatShot");
	MapMobj(MT_CHAINGUY, "ChaingunGuy");
	MapMobj(MT_TROOP, "DoomImp");
	MapMobj(MT_SERGEANT, "Demon");
	MapMobj(MT_SHADOWS, "Spectre");
	MapMobj(MT_HEAD, "Cacodemon");
	MapMobj(MT_BRUISER, "BaronOfHell");
	MapMobj(MT_BRUISERSHOT, "BaronBall");
	MapMobj(MT_KNIGHT, "HellKnight");
	MapMobj(MT_SKULL, "LostSoul");
	MapMobj(MT_SPIDER, "SpiderMastermind");
	MapMobj(MT_BABY, "Arachnotron");
	MapMobj(MT_CYBORG, "Cyberdemon");
	MapMobj(MT_PAIN, "PainElemental");
	MapMobj(MT_WOLFSS, "WolfensteinSS");
	MapMobj(MT_KEEN, "CommanderKeen");
	MapMobj(MT_BOSSBRAIN, "BossBrain");
	MapMobj(MT_BOSSSPIT, "BossEye");
	MapMobj(MT_BOSSTARGET, "BossTarget");
	MapMobj(MT_SPAWNSHOT, "SpawnShot");
	MapMobj(MT_SPAWNFIRE, "SpawnFire");
	MapMobj(MT_BARREL, "ExplosiveBarrel");
	MapMobj(MT_TROOPSHOT, "DoomImpBall");
	MapMobj(MT_HEADSHOT, "CacodemonBall");
	MapMobj(MT_ROCKET, "Rocket");
	MapMobj(MT_PLASMA, "PlasmaBall");
	MapMobj(MT_BFG, "BFGBall");
	MapMobj(MT_ARACHPLAZ, "ArachnotronPlasma");
	MapMobj(MT_PUFF, "BulletPuff");
	MapMobj(MT_BLOOD, "Blood");
	MapMobj(MT_TFOG, "TeleportFog");
	MapMobj(MT_IFOG, "ItemFog");
	MapMobj(MT_TELEPORTMAN, "TeleportDest");
	MapMobj(MT_EXTRABFG, "BFGExtra");
	MapMobj(MT_MISC0, "GreenArmor");
	MapMobj(MT_MISC1, "BlueArmor");
	MapMobj(MT_MISC2, "HealthBonus");
	MapMobj(MT_MISC3, "ArmorBonus");
	MapMobj(MT_MISC4, "BlueCard", it_bluecard);
	MapMobj(MT_MISC5, "RedCard", it_redcard);
	MapMobj(MT_MISC6, "YellowCard", it_yellowcard);
	MapMobj(MT_MISC7, "YellowSkull", it_yellowskull);
	MapMobj(MT_MISC8, "RedSkull", it_redskull);
	MapMobj(MT_MISC9, "BlueSkull", it_blueskull);
	MapMobj(MT_MISC10, "Stimpack");
	MapMobj(MT_MISC11, "Medikit");
	MapMobj(MT_MISC12, "Soulsphere");
	MapMobj(MT_INV, "InvulnerabilitySphere", pw_invulnerability);
	MapMobj(MT_MISC13, "Berserk", pw_strength);
	MapMobj(MT_INS, "BlurSphere", pw_invisibility);
	MapMobj(MT_MISC14, "RadSuit", pw_ironfeet);
	MapMobj(MT_MISC15, "Allmap", pw_allmap);
	MapMobj(MT_MISC16, "Infrared", pw_infrared);
	MapMobj(MT_MEGA, "Megasphere");
	MapMobj(MT_CLIP, "Clip", am_clip);
	MapMobj(MT_MISC17, "ClipBox", am_clip);
	MapMobj(MT_MISC18, "RocketAmmo", am_misl);
	MapMobj(MT_MISC19, "RocketBox", am_misl);
	MapMobj(MT_MISC20, "Cell", am_cell);
	MapMobj(MT_MISC21, "CellPack", am_cell);
	MapMobj(MT_MISC22, "Shell", am_shell);
	MapMobj(MT_MISC23, "ShellBox", am_shell);
	MapMobj(MT_MISC24, "Backpack", infomap::backpack);
	MapMobj(MT_MISC25, "BFG9000", wp_bfg);
	MapMobj(MT_CHAINGUN, "Chaingun", wp_bfg);
	MapMobj(MT_MISC26, "Chainsaw", wp_chainsaw);
	MapMobj(MT_MISC27, "RocketLauncher", wp_missile);
	MapMobj(MT_MISC28, "PlasmaRifle", wp_plasma);
	MapMobj(MT_SHOTGUN, "Shotgun", wp_shotgun);
	MapMobj(MT_SUPERSHOTGUN, "SuperShotgun", wp_supershotgun);
	MapMobj(MT_MISC29, "TechLamp");
	MapMobj(MT_MISC30, "TechLamp2");
	MapMobj(MT_MISC31, "Column");
	MapMobj(MT_MISC32, "TallGreenColumn");
	MapMobj(MT_MISC33, "ShortGreenColumn");
	MapMobj(MT_MISC34, "TallRedColumn");
	MapMobj(MT_MISC35, "ShortRedColumn");
	MapMobj(MT_MISC36, "SkullColumn");
	MapMobj(MT_MISC37, "HeartColumn");
	MapMobj(MT_MISC38, "EvilEye");
	MapMobj(MT_MISC39, "FloatingSkull");
	MapMobj(MT_MISC40, "TorchTree");
	MapMobj(MT_MISC41, "BlueTorch");
	MapMobj(MT_MISC42, "GreenTorch");
	MapMobj(MT_MISC43, "RedTorch");
	MapMobj(MT_MISC44, "ShortBlueTorch");
	MapMobj(MT_MISC45, "ShortGreenTorch");
	MapMobj(MT_MISC46, "ShortRedTorch");
	MapMobj(MT_MISC47, "Stalagtite");
	MapMobj(MT_MISC48, "TechPillar");
	MapMobj(MT_MISC49, "CandleStick");
	MapMobj(MT_MISC50, "Candelabra");
	MapMobj(MT_MISC51, "BloodyTwitch");
	MapMobj(MT_MISC52, "Meat2");
	MapMobj(MT_MISC53, "Meat3");
	MapMobj(MT_MISC54, "Meat4");
	MapMobj(MT_MISC55, "Meat5");
	MapMobj(MT_MISC56, "NonsolidMeat2");
	MapMobj(MT_MISC57, "NonsolidMeat4");
	MapMobj(MT_MISC58, "NonsolidMeat3");
	MapMobj(MT_MISC59, "NonsolidMeat5");
	MapMobj(MT_MISC60, "NonsolidTwitch");
	MapMobj(MT_MISC61, "DeadCacodemon");
	MapMobj(MT_MISC62, "DeadMarine");
	MapMobj(MT_MISC63, "DeadZombieMan");
	MapMobj(MT_MISC64, "DeadDemon");
	MapMobj(MT_MISC65, "DeadLostSoul");
	MapMobj(MT_MISC66, "DeadDoomImp");
	MapMobj(MT_MISC67, "DeadShotgunGuy");
	MapMobj(MT_MISC68, "GibbedMarine");
	MapMobj(MT_MISC69, "GibbedMarineExtra");
	MapMobj(MT_MISC70, "HeadsOnAStick");
	MapMobj(MT_MISC71, "Gibs");
	MapMobj(MT_MISC72, "HeadOnAStick");
	MapMobj(MT_MISC73, "HeadCandles");
	MapMobj(MT_MISC74, "DeadStick");
	MapMobj(MT_MISC75, "LiveStick");
	MapMobj(MT_MISC76, "BigTree");
	MapMobj(MT_MISC77, "BurningBarrel");
	MapMobj(MT_MISC78, "HangNoGuts");
	MapMobj(MT_MISC79, "HangBNoBrain");
	MapMobj(MT_MISC80, "HangTLookingDown");
	MapMobj(MT_MISC81, "HangTSkull");
	MapMobj(MT_MISC82, "HangTLookingUp");
	MapMobj(MT_MISC83, "HangTNoBrain");
	MapMobj(MT_MISC84, "ColonGibs");
	MapMobj(MT_MISC85, "SmallBloodPool");
	MapMobj(MT_MISC86, "BrainStem");
	MapMobj(MT_PUSH, "PointPusher");
	MapMobj(MT_PULL, "PointPuller");
	MapMobj(MT_DOGS, "MBFHelperDog");
	MapMobj(MT_PLASMA1, "PlasmaBall1");
	MapMobj(MT_PLASMA2, "PlasmaBall2");
	MapMobj(MT_SCEPTRE, "EvilSceptre");
	MapMobj(MT_BIBLE, "UnholyBible");
	MapMobj(MT_MUSICSOURCE, "MusicChanger");
	MapMobj(MT_AVATAR, "PlayerAvatar");
	MapMobj(MT_HORDESPAWN, "HordeSpawn");
	MapMobj(MT_CAREPACK, "CarePackage");
	MapMobj(MT_EXTRALIFE, "ExtraLifePowerUp");
	MapMobj(MT_RESTEAMMATE, "ResurrectTeammatePowerUp");
	MapMobj(MT_BRIDGE, "Bridge");
	MapMobj(MT_ZDOOMBRIDGE, "ZBridge");
	MapMobj(MT_BRIDGE8, "InvisibleBridge8");
	MapMobj(MT_BRIDGE16, "InvisibleBridge16");
	MapMobj(MT_BRIDGE32, "InvisibleBridge32");
	// [AM] Deh_Actor_145-149 are reserved.
	MapMobj(MT_EXTRA00, "Deh_Actor_150");
	MapMobj(MT_EXTRA01, "Deh_Actor_151");
	MapMobj(MT_EXTRA02, "Deh_Actor_152");
	MapMobj(MT_EXTRA03, "Deh_Actor_153");
	MapMobj(MT_EXTRA04, "Deh_Actor_154");
	MapMobj(MT_EXTRA05, "Deh_Actor_155");
	MapMobj(MT_EXTRA06, "Deh_Actor_156");
	MapMobj(MT_EXTRA07, "Deh_Actor_157");
	MapMobj(MT_EXTRA08, "Deh_Actor_158");
	MapMobj(MT_EXTRA09, "Deh_Actor_159");
	MapMobj(MT_EXTRA10, "Deh_Actor_160");
	MapMobj(MT_EXTRA11, "Deh_Actor_161");
	MapMobj(MT_EXTRA12, "Deh_Actor_162");
	MapMobj(MT_EXTRA13, "Deh_Actor_163");
	MapMobj(MT_EXTRA14, "Deh_Actor_164");
	MapMobj(MT_EXTRA15, "Deh_Actor_165");
	MapMobj(MT_EXTRA16, "Deh_Actor_166");
	MapMobj(MT_EXTRA17, "Deh_Actor_167");
	MapMobj(MT_EXTRA18, "Deh_Actor_168");
	MapMobj(MT_EXTRA19, "Deh_Actor_169");
	MapMobj(MT_EXTRA20, "Deh_Actor_170");
	MapMobj(MT_EXTRA21, "Deh_Actor_171");
	MapMobj(MT_EXTRA22, "Deh_Actor_172");
	MapMobj(MT_EXTRA23, "Deh_Actor_173");
	MapMobj(MT_EXTRA24, "Deh_Actor_174");
	MapMobj(MT_EXTRA25, "Deh_Actor_175");
	MapMobj(MT_EXTRA26, "Deh_Actor_176");
	MapMobj(MT_EXTRA27, "Deh_Actor_177");
	MapMobj(MT_EXTRA28, "Deh_Actor_178");
	MapMobj(MT_EXTRA29, "Deh_Actor_179");
	MapMobj(MT_EXTRA30, "Deh_Actor_180");
	MapMobj(MT_EXTRA31, "Deh_Actor_181");
	MapMobj(MT_EXTRA32, "Deh_Actor_182");
	MapMobj(MT_EXTRA33, "Deh_Actor_183");
	MapMobj(MT_EXTRA34, "Deh_Actor_184");
	MapMobj(MT_EXTRA35, "Deh_Actor_185");
	MapMobj(MT_EXTRA36, "Deh_Actor_186");
	MapMobj(MT_EXTRA37, "Deh_Actor_187");
	MapMobj(MT_EXTRA38, "Deh_Actor_188");
	MapMobj(MT_EXTRA39, "Deh_Actor_189");
	MapMobj(MT_EXTRA40, "Deh_Actor_190");
	MapMobj(MT_EXTRA41, "Deh_Actor_191");
	MapMobj(MT_EXTRA42, "Deh_Actor_192");
	MapMobj(MT_EXTRA43, "Deh_Actor_193");
	MapMobj(MT_EXTRA44, "Deh_Actor_194");
	MapMobj(MT_EXTRA45, "Deh_Actor_195");
	MapMobj(MT_EXTRA46, "Deh_Actor_196");
	MapMobj(MT_EXTRA47, "Deh_Actor_197");
	MapMobj(MT_EXTRA48, "Deh_Actor_198");
	MapMobj(MT_EXTRA49, "Deh_Actor_199");
	MapMobj(MT_EXTRA50, "Deh_Actor_200");
	MapMobj(MT_EXTRA51, "Deh_Actor_201");
	MapMobj(MT_EXTRA52, "Deh_Actor_202");
	MapMobj(MT_EXTRA53, "Deh_Actor_203");
	MapMobj(MT_EXTRA54, "Deh_Actor_204");
	MapMobj(MT_EXTRA55, "Deh_Actor_205");
	MapMobj(MT_EXTRA56, "Deh_Actor_206");
	MapMobj(MT_EXTRA57, "Deh_Actor_207");
	MapMobj(MT_EXTRA58, "Deh_Actor_208");
	MapMobj(MT_EXTRA59, "Deh_Actor_209");
	MapMobj(MT_EXTRA60, "Deh_Actor_210");
	MapMobj(MT_EXTRA61, "Deh_Actor_211");
	MapMobj(MT_EXTRA62, "Deh_Actor_212");
	MapMobj(MT_EXTRA63, "Deh_Actor_213");
	MapMobj(MT_EXTRA64, "Deh_Actor_214");
	MapMobj(MT_EXTRA65, "Deh_Actor_215");
	MapMobj(MT_EXTRA66, "Deh_Actor_216");
	MapMobj(MT_EXTRA67, "Deh_Actor_217");
	MapMobj(MT_EXTRA68, "Deh_Actor_218");
	MapMobj(MT_EXTRA69, "Deh_Actor_219");
	MapMobj(MT_EXTRA70, "Deh_Actor_220");
	MapMobj(MT_EXTRA71, "Deh_Actor_221");
	MapMobj(MT_EXTRA72, "Deh_Actor_222");
	MapMobj(MT_EXTRA73, "Deh_Actor_223");
	MapMobj(MT_EXTRA74, "Deh_Actor_224");
	MapMobj(MT_EXTRA75, "Deh_Actor_225");
	MapMobj(MT_EXTRA76, "Deh_Actor_226");
	MapMobj(MT_EXTRA77, "Deh_Actor_227");
	MapMobj(MT_EXTRA78, "Deh_Actor_228");
	MapMobj(MT_EXTRA79, "Deh_Actor_229");
	MapMobj(MT_EXTRA80, "Deh_Actor_230");
	MapMobj(MT_EXTRA81, "Deh_Actor_231");
	MapMobj(MT_EXTRA82, "Deh_Actor_232");
	MapMobj(MT_EXTRA83, "Deh_Actor_233");
	MapMobj(MT_EXTRA84, "Deh_Actor_234");
	MapMobj(MT_EXTRA85, "Deh_Actor_235");
	MapMobj(MT_EXTRA86, "Deh_Actor_236");
	MapMobj(MT_EXTRA87, "Deh_Actor_237");
	MapMobj(MT_EXTRA88, "Deh_Actor_238");
	MapMobj(MT_EXTRA89, "Deh_Actor_239");
	MapMobj(MT_EXTRA90, "Deh_Actor_240");
	MapMobj(MT_EXTRA91, "Deh_Actor_241");
	MapMobj(MT_EXTRA92, "Deh_Actor_242");
	MapMobj(MT_EXTRA93, "Deh_Actor_243");
	MapMobj(MT_EXTRA94, "Deh_Actor_244");
	MapMobj(MT_EXTRA95, "Deh_Actor_245");
	MapMobj(MT_EXTRA96, "Deh_Actor_246");
	MapMobj(MT_EXTRA97, "Deh_Actor_247");
	MapMobj(MT_EXTRA98, "Deh_Actor_248");
	MapMobj(MT_EXTRA99, "Deh_Actor_249");
}

/**
 * @brief Add a new DeHackEd thing to the map (for ID24 DEHTHING_x strings)
 */
void P_MapDehThing(const mobjtype_t type, const std::string& name)
{
	if (::g_MonsterMap.empty())
	{
		P_InitMobjNameMap();
	}

	MapMobj(type, name);
}

/**
 * @brief Convert a UMAPINFO/ZDoom class name to a MT Mobj index.
 */
mobjclass_t P_NameToMobjFull(const std::string& name)
{
	if (::g_MonsterMap.empty())
	{
		P_InitMobjNameMap();
	}

	const auto it = ::g_MonsterMap.find(name);

	if (it == ::g_MonsterMap.end() || !mobjinfo.contains(it->second.type))
	{
		return {};
	}

	return it->second;
}

/**
 * @brief Convert a UMAPINFO/ZDoom class name to a MT Mobj index.
 */
mobjtype_t P_NameToMobj(const std::string& name)
{
	return P_NameToMobjFull(name).type;
}

/**
 * @brief Convert a UMAPINFO/ZDoom class name to a MT Mobj index. Case insensitive for
 * UMAPINFO
 */
mobjclass_t P_INameToMobjFull(const std::string& name)
{
	if (::g_MonsterMap.empty())
	{
		P_InitMobjNameMap();
	}

	const auto it = ::g_MonsterMapNoCase.find(StdStringToLower(name));

	if (it == ::g_MonsterMapNoCase.end() || !mobjinfo.contains(it->second.type))
	{
		return {};
	}

	return it->second;
}

/**
 * @brief Convert a UMAPINFO/ZDoom class name to a MT Mobj index. Case insensitive for
 * UMAPINFO
 */
mobjtype_t P_INameToMobj(const std::string& name)
{
	return P_INameToMobjFull(name).type;
}

std::string P_MobjToName(const mobjtype_t type)
{
	if (::g_MonsterMap.empty())
	{
		P_InitMobjNameMap();
	}

	const auto it = ::g_InverseMonsterMap.find(type);

	if (it == ::g_InverseMonsterMap.end())
	{
		return "";
	}

	return it->second;
}

weapontype_t P_NameToWeapon(std::string_view name)
{
	using OUtil::CONST_HASH;
	switch (CONST_HASH(name))
	{
		case CONST_HASH("Fist"):
			return wp_fist;
		case CONST_HASH("Pistol"):
			return wp_pistol;
		case CONST_HASH("Shotgun"):
			return wp_shotgun;
		case CONST_HASH("Chaingun"):
			return wp_chaingun;
		case CONST_HASH("RocketLauncher"):
			return wp_missile;
		case CONST_HASH("PlasmaRifle"):
			return wp_plasma;
		case CONST_HASH("BFG9000"):
			return wp_bfg;
		case CONST_HASH("Chainsaw"):
			return wp_chainsaw;
		case CONST_HASH("SuperShotgun"):
			return wp_supershotgun;
		default:
			return wp_none;
	}
}

weapontype_t P_INameToWeapon(std::string_view name)
{
	using OUtil::CONST_HASH_NO_CASE;
	switch (CONST_HASH_NO_CASE(name))
	{
		case CONST_HASH_NO_CASE("Fist"):
			return wp_fist;
		case CONST_HASH_NO_CASE("Pistol"):
			return wp_pistol;
		case CONST_HASH_NO_CASE("Shotgun"):
			return wp_shotgun;
		case CONST_HASH_NO_CASE("Chaingun"):
			return wp_chaingun;
		case CONST_HASH_NO_CASE("RocketLauncher"):
			return wp_missile;
		case CONST_HASH_NO_CASE("PlasmaRifle"):
			return wp_plasma;
		case CONST_HASH_NO_CASE("BFG9000"):
			return wp_bfg;
		case CONST_HASH_NO_CASE("Chainsaw"):
			return wp_chainsaw;
		case CONST_HASH_NO_CASE("SuperShotgun"):
			return wp_supershotgun;
		default:
			return wp_none;
	}
}

namespace
{

using MobjPair = std::pair<std::string, mobjclass_t>;

// Hashtables don't work with std::sort
// This is a half measure to sort it without
// Messing with the internals of hashtable.
// I wonder if we just want std::map instead of OHashTable?
// We don't need the speed too much
std::vector<MobjPair> OrderedMobjMap()
{
	std::vector<MobjPair> orderedVector;

	for (const auto& pair : ::g_MonsterMap)
	{
		orderedVector.push_back(pair);
	}

	std::sort(orderedVector.begin(), orderedVector.end(),
			[](const auto& left, const auto& right) {
		    return left.second.type < right.second.type;
			});

	return orderedVector;
}

}

BEGIN_COMMAND(dumpactors)
{
	if (::g_MonsterMap.empty())
	{
		P_InitMobjNameMap();
	}

	std::vector<MobjPair> infomap = OrderedMobjMap();

	PrintFmt(PRINT_HIGH, "Total amount of actors: {}\n", infomap.size());

	for (const auto& [name, _] : infomap)
	{
		PrintFmt(PRINT_HIGH, "{}\n", name);
	}
}
END_COMMAND(dumpactors)
