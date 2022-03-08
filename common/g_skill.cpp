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


#include "odamex.h"

#include "g_skill.h"

SkillInfo SkillInfos[MAX_SKILLS];
byte skillnum = 0;
byte defaultskillmenu = 2;

const SkillInfo& G_GetCurrentSkill()
{
	return SkillInfos[sv_skill.asInt() - 1];
}