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

#pragma once

#include <string_view>
#include <variant>
#include <optional>

#include "doomdef.h"
#include "info.h"

namespace infomap
{

using backpack_t = std::monostate;
constexpr backpack_t backpack{};
using givetype_t = std::variant<card_t, weapontype_t, ammotype_t, powertype_t, backpack_t>;

struct mobjclass_t
{
    mobjtype_t type = MT_NULL;
    std::optional<givetype_t> givetype = std::nullopt;
};

}

mobjtype_t P_NameToMobj(const std::string& name);
mobjtype_t P_INameToMobj(const std::string& name);
infomap::mobjclass_t P_NameToMobjFull(const std::string& name);
infomap::mobjclass_t P_INameToMobjFull(const std::string& name);
weapontype_t P_NameToWeapon(std::string_view name);
weapontype_t P_INameToWeapon(std::string_view name);
std::string P_MobjToName(const mobjtype_t type);
void P_MapDehThing(const mobjtype_t type, const std::string& name);
void P_InitMobjNameMap();
