// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id:
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 2006-2025 by The Odamex Team.
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
//   Skill data for defining new skills.
//
//-----------------------------------------------------------------------------

#pragma once

#define MAX_SKILLS 7

inline constexpr uint32_t SKILL_NOINFIGHTING = BIT(0);
inline constexpr uint32_t SKILL_TOTALINFIGHTING = BIT(1);

struct SkillInfo
{
	std::string name;
	uint32_t flags                = 0;

	float ammo_factor             = 1.0f;
	float double_ammo_factor      = 2.0f;
	float drop_ammo_factor        = -1.0f;	// not implemented
	float damage_factor           = 1.0f;
	float armor_factor            = 1.0f;
	float health_factor           = 1.0f;
	float kickback_factor         = 1.0f;	// not implemented

	bool fast_monsters            = false;
	bool slow_monsters            = false;	// not implemented
	bool disable_cheats           = false;
	bool auto_use_health          = false;	// not implemented

	bool easy_boss_brain          = false;
	bool easy_key                 = false;	// not implemented
	bool no_menu                  = 0;		// not implemented
	int respawn_counter           = 0;
	int respawn_limit             = 0;		// not implemented
	float aggressiveness          = 1.0f;	// not implemented
	int spawn_filter              = 0;
	bool spawn_multi              = false;
	bool instant_reaction         = false;
	int ACS_return                = 0;		// not implemented
	std::string menu_name;
	OLumpName pic_name;
	//SkillMenuNames menu_names_for_player_class;	// not implemented
	bool must_confirm             = false;
	std::string must_confirm_text = "$NIGHTMARE";
	char shortcut                 = 0;
	byte text_color[4]            = { 0 };	// not implemented
	//SkillActorReplacement replace;	// not implemented
	//SkillActorReplacement replaced;	// not implemented
	float monster_health          = 1.0f;	// not implemented
	float friendly_health         = 1.0f;	// not implemented
	bool no_pain                  = false;	// not implemented
	bool player_respawn           = false;	// not implemented

	SkillInfo()	{}
};

inline SkillInfo SkillInfos[MAX_SKILLS];
inline byte skillnum;
inline byte defaultskillmenu;

inline const SkillInfo& G_GetCurrentSkill()
{
	return SkillInfos[sv_skill.asInt() - 1];
}
