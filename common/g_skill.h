// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id:
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
//   Skill data for defining new skills.
//
//-----------------------------------------------------------------------------

#pragma once

#define MAX_SKILLS 7

struct SkillInfo
{
	std::string name;

	float ammo_factor;
	float double_ammo_factor;
	float drop_ammo_factor;		// not implemented
	float damage_factor;
	float armor_factor;
	float health_factor;
	float kickback_factor;		// not implemented

	bool fast_monsters;
	bool slow_monsters;			// not implemented
	bool disable_cheats;
	bool auto_use_health;		// not implemented

	bool easy_boss_brain;
	bool easy_key;				// not implemented
	bool no_menu;				// not implemented
	int respawn_counter;
	int respawn_limit;			// not implemented
	float aggressiveness;		// not implemented
	int spawn_filter;
	bool spawn_multi;
	bool instant_reaction;
	int ACS_return;				// not implemented
	std::string menu_name;
	std::string pic_name;
	//SkillMenuNames menu_names_for_player_class;	// not implemented
	bool must_confirm;
	std::string must_confirm_text;
	char shortcut;
	byte text_color[4];					// not implemented
	//SkillActorReplacement replace;	// not implemented
	//SkillActorReplacement replaced;	// not implemented
	float monster_health;	// not implemented
	float friendly_health;	// not implemented
	bool no_pain;			// not implemented
	int infighting;			// not implemented
	bool player_respawn;	// not implemented

	SkillInfo()
		: ammo_factor(1.f)
		, double_ammo_factor(2.f)
		, drop_ammo_factor(-1.f)
		, damage_factor(1.f)
		, armor_factor(1.f)
		, health_factor(1.f)
		, kickback_factor(1.f)

		, fast_monsters(false)
		, slow_monsters(false)
		, disable_cheats(false)
		, auto_use_health(false)

		, easy_boss_brain(false)
		, easy_key(false)
		, respawn_counter(0)
		, respawn_limit(0)
		, aggressiveness(1.f)
		, spawn_filter(0)
		, spawn_multi(false)
		, instant_reaction(false)
		, ACS_return(0)
		, menu_name()
		, pic_name()
		//, menu_names_for_player_class(???)
		, must_confirm(false)
		, must_confirm_text("$NIGHTMARE")
		, shortcut(0)
		, text_color()
		//, replace(???)
		//, replaced(???)
		, monster_health(1.)
		, friendly_health(1.)
		, no_pain(false)
		, infighting(0)
		, player_respawn(false)
	{}
};

extern SkillInfo SkillInfos[MAX_SKILLS];
extern byte skillnum;
extern byte defaultskillmenu;

const SkillInfo& G_GetCurrentSkill();
