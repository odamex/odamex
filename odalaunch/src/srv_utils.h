// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
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
//  Server data presentation helpers (shared formatting routines)
//
//-----------------------------------------------------------------------------

#pragma once

#include "odalaunch.h"

#include <string>

#include <wx/string.h>

#include "net_packet.h"

// Returns an easy-to-read game type label for a server (e.g. "Cooperative",
// "Duel", "Capture The Flag"), taking game modifiers (lives, sides, player
// limits) into account.
// 
// Moved from lst_servers.cpp so it can be used elsewhere
wxString OdaGetGameTypeString(const odalpapi::Server& s);

// Returns a pointer to a server's cvar by name, or nullptr if absent. Useful
// when both the cvar's type and raw value are needed in a single lookup.
const odalpapi::Cvar_t* OdaFindCvar(const odalpapi::Server& s,
                                    const std::string& Name);

// Looks up a cvar by name in a server's cvar list and writes its value (as a
// string). Returns true if the cvar was found.
bool OdaGetCvarValue(const odalpapi::Server& s, const std::string& Name,
                     wxString& Out);

// Returns the integer value of a cvar, or Default if the cvar is absent.
// (The server only transmits cvars whose value is non-zero or non-null,
// so an absent cvar is effectively zero.)
int OdaGetCvarInt(const odalpapi::Server& s, const std::string& Name,
                  int Default = 0);

// Returns an easy-to-read skill label from the server's sv_skill cvar
// (e.g. "Ultra-Violence"). Falls back to the raw value, or "Unknown" if the
// cvar is absent.
wxString OdaGetSkillString(const odalpapi::Server& s);

// Formats a duration (in seconds) as a human-readable string such as
// "1 hour, 5 minutes, 30 seconds". Zero-valued components are omitted, and a
// non-positive duration returns an empty string.
wxString OdaGetTimeString(int Seconds);
