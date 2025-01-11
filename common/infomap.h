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

#pragma once

#include "doomdef.h"
#include "info.h"

mobjtype_t P_NameToMobj(const std::string& name);
mobjtype_t P_INameToMobj(const std::string& name);
weapontype_t P_NameToWeapon(const std::string& name);
std::string P_MobjToName(const mobjtype_t name);
