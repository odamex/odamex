// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2006-2024 by The Odamex Team.
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
//   Non-ingame horde functionality.
//
//-----------------------------------------------------------------------------

#pragma once

#include "p_hordedefine.h"

void G_ParseHordeDefs();
const hordeDefine_t& G_HordeDefine(size_t id);
bool CheckIfDehActorDefined(const mobjtype_t mobjtype);
