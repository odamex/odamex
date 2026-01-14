// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
// Copyright (C) 2025-2026 by The Odamex Team.
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
//  These routines provide console commands streamed in from stdin
//  or a given file.
//
//-----------------------------------------------------------------------------

#pragma once

#include <string>

// Use the given named file to read console commands instead of stdin.
// MUST be called before the first call to M_ConsoleInput().
// If called after the first call M_ConsoleInput, this function has no
// effect.
void M_InitConsoleInputFile (const char* filepath);

// Returns a line from the console input file / stdin.
// If there's no pending command, an empty string is returned.
// Does not block.
std::string M_ConsoleInput (void);
