// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1998-2006 by Randy Heit (ZDoom).
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
//   Handles parsing all SPREEDEF lumps.
//
//-----------------------------------------------------------------------------

#pragma once

#include <cstddef>

/// <summary>
/// Parses all SPREEDEF lumps for consumption by
/// the spree and multi kill managers.
/// </summary>
void G_ParseSpreeDef();

/// <summary>
/// Parses a single SPREEDEF from a raw buffer. Errors out through I_Error on anything
/// malformed, as lump parsing does.
/// </summary>
/// <param name="buffer">Start of the SPREEDEF text.</param>
/// <param name="length">Length of the SPREEDEF text.</param>
void G_ParseSpreeDefBuffer(const char* buffer, const size_t length);
