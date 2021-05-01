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
//   Color-specific functions and utilities.
//
//-----------------------------------------------------------------------------

#pragma once

#include "doomtype.h"

#include <vector>

typedef std::vector<argb_t> OGradient;

double V_ColorDistance(const argb_t& e1, const argb_t& e2);
byte V_ColorToGrey(const argb_t& color);
OGradient V_ColorGradient(const argb_t& start, const argb_t& end, const size_t len);
void V_DebugGradient(const argb_t& a, const argb_t& b, const size_t len, const int y);
