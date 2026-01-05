// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2006-2025 by The Odamex Team.
// Copyright (C) 2024-2025 by Christian Bernard.
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
//  info.h for odamex/zdoom/ctf stuff
//
//-----------------------------------------------------------------------------

#pragma once

#include <span>

// forward declarations
struct mobjinfo_t;
struct state_t;

std::span<mobjinfo_t> getOdaMobjinfo();
std::span<state_t> getOdaStates();
std::span<const char*> getOdaSprNames();
