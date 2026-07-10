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

class wxWindow;
class wxSizer;
class wxStaticText;

// A modifier value ready for display: the value text (empty when the modifier
// is absent or at its default), an optional parenthesized delta from default,
// whether the value is above default (used to color it green vs red), and the
// modifier's default value as text (for a "Default is X" tooltip).
struct OdaModifier_t
{
	wxString Value;
	wxString Delta;
	bool     IsPositive;
	wxString Default;

	OdaModifier_t() : IsPositive(false) {}
};

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

// Returns the server's "major.minor.patch" version, with a revision suffix
// (" (rN)" or " (revstr)") when the server reports one.
wxString OdaGetVersionString(const odalpapi::Server& s);

// Returns the "X / Y clients, Z player(s) can join" summary line.
wxString OdaGetPlayerCountString(const odalpapi::Server& s);

// Returns "Yes" when friendly fire applies (a team/coop/horde mode with
// sv_friendlyfire transmitted), otherwise an empty string.
wxString OdaGetFriendlyFireString(const odalpapi::Server& s);

// Returns a damage/health modifier cvar as a percentage (e.g. "150%"),
// or empty when the cvar is absent or at its neutral 1.0. IsPositive is
// true when the modifier is above 1.0.
OdaModifier_t OdaGetDamagePercent(const odalpapi::Server& s,
                                  const std::string& Cvar);

// Returns the rounds summary ("Unlimited", "N round limit[, first to M wins]"),
// or an empty string when g_rounds is not in play.
wxString OdaGetRoundsString(const odalpapi::Server& s);

// Returns the remaining time (via OdaGetTimeString), or an empty string when
// the game is not timed.
wxString OdaGetTimeLeftString(const odalpapi::Server& s);

// Returns the "leader / limit" score line for the mode's win condition (team
// score for team modes, frags for deathmatch), or empty when none applies.
wxString OdaGetScoreString(const odalpapi::Server& s);

// Returns the sv_gravity value, empty when absent or at its
// default of 800. Delta is the signed difference from default as a
// parenthesized string (e.g. "(-0.25%)") and IsPositive is true when the value
// is above default.
OdaModifier_t OdaGetGravity(const odalpapi::Server& s);

// Returns the sv_aircontrol value, empty when absent or at
// its default of 0.00390625. Delta / IsPositive behaves the same
// as OdaGetGravity.
OdaModifier_t OdaGetAirControl(const odalpapi::Server& s);

// Colours a delta control green when IsPositive, red otherwise, choosing shades
// that stay legible on the control's background (light or dark).
void OdaApplyDeltaColour(wxStaticText* Ctrl, bool IsPositive);

// Underlines Ctrl, gives it a "Default is X" tooltip and a help cursor to hint
// that hovering reveals its default. No-op when DefaultText is empty.
void OdaApplyDefaultHint(wxStaticText* Ctrl, const wxString& DefaultText);

// Returns "Yes" when fast monsters are enabled (sv_fastmonsters) below
// Nightmare skill, or an empty string otherwise.
wxString OdaGetFastMonstersString(const odalpapi::Server& s);

// Returns a comma-separated summary of the non-default CTF rules in effect
// ("Flag always scores", "manual return", "flag timeout returns only"), with
// only the first word capitalised, or an empty string when none apply / the
// server isn't running CTF.
wxString OdaGetCtfRulesString(const odalpapi::Server& s);

// Fills Target with a coloured "Name: Score" box per team (white text on the
// team colour, separated by " - "), clearing it first. Only team modes with
// teams produce boxes; returns true when any were added.
bool OdaBuildTeamScoreBoxes(wxWindow* Parent, wxSizer* Target,
                            const odalpapi::Server& s);
