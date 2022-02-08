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

#ifndef __G_SKILL__
#define __G_SKILL__


#define MAX_SKILLS 8

struct SkillInfo
{
	std::string name;

	float ammo_factor;
	float double_ammo_factor;
	float drop_ammo_factor;
	float damage_factor;
	float armor_factor;
	float health_factor;
	float kickback_factor;

	bool fast_monsters;
	bool slow_monsters;
	bool disable_cheats;
	bool auto_use_health;

	SkillInfo() : ammo_factor(1) {}
};

extern SkillInfo SkillInfos[MAX_SKILLS];
extern byte skillnum;

#endif
