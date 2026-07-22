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
//  Frameless hover popovers for server info and the player list
//
//-----------------------------------------------------------------------------

#pragma once

#include "odalaunch.h"

#include <wx/popupwin.h>

#if wxUSE_POPUPWIN

#include <wx/panel.h>
#include <wx/stattext.h>

class wxFlexGridSizer;
class wxSizer;

#include "net_packet.h"
#include "lst_players.h"

// A small popover with a few preset variables to quickly analyze a server configuration.
class ServerInfoPopover : public wxPopupWindow
{
public:
	ServerInfoPopover(wxWindow* parent);

	// Fill in the values from a queried server and resize to fit
	void Populate(const odalpapi::Server& s);

private:
	// Shows/hides an optional label+value row in m_Grid and returns whether it
	// is now visible (true when Value is non-empty).
	bool SetOptionalRow(wxStaticText* Label, wxStaticText* Field,
	                    const wxString& Value);

	wxFlexGridSizer* m_Grid;

	wxStaticText* m_Name;
	wxStaticText* m_Version;
	wxStaticText* m_Address;
	wxStaticText* m_Skill;
	wxStaticText* m_DownloadURI;
	wxStaticText* m_AdminEmail;
	wxStaticText* m_Password;

	// Labels for the optional rows, kept so they can be hidden when empty
	wxStaticText* m_DownloadURILabel;
	wxStaticText* m_AdminEmailLabel;
};

// A popover showing client/player counts, game type and the full player table.
// The detail grid above the table mirrors the styling of ServerInfoPopover and
// shows different rows depending on the game mode.
class PlayerListPopover : public wxPopupWindow
{
public:
	PlayerListPopover(wxWindow* parent);

	// Fill in the details/player table from a queried server and resize to fit
	void Populate(const odalpapi::Server& s);

private:
	// Shows/hides an optional "label: value" item in the header flow and returns
	// whether it is now visible (true when Value is non-empty).
	bool SetOptionalItem(wxSizer* Item, wxStaticText* Field,
	                     const wxString& Value);

	// Same as SetOptionalItem, but colors the value green when IsPositive, red
	// otherwise (for above/below-default percentage values).
	void SetColouredItem(wxSizer* Item, wxStaticText* Field,
	                     const wxString& Value, bool IsPositive);

	// Header items flow horizontally (wrapping as needed) to use the popover's
	// width rather than stacking in a tall column.
	wxSizer* m_HeaderSizer;

	// Always-shown stats
	wxStaticText* m_ClientPlayerCount;
	wxStaticText* m_GameType;

	// Game-mode-dependent stats (items collapse when not applicable)
	wxStaticText* m_FriendlyFire;
	wxStaticText* m_PlayerDmg;
	wxStaticText* m_MonsterDmg;
	wxStaticText* m_MonsterHealth;
	wxStaticText* m_Gravity;
	wxStaticText* m_GravityDelta;
	wxStaticText* m_AirControl;
	wxStaticText* m_AirControlDelta;
	wxStaticText* m_Waves;
	wxStaticText* m_CtfRules;
	wxStaticText* m_Lives;
	wxStaticText* m_Rounds;
	wxStaticText* m_FastMonsters;
	wxStaticText* m_TimeLeft;
	wxStaticText* m_ScoreLimit;

	// The sub-sizers wrapping each optional item, kept so they can be hidden
	wxSizer* m_FriendlyFireItem;
	wxSizer* m_PlayerDmgItem;
	wxSizer* m_MonsterDmgItem;
	wxSizer* m_MonsterHealthItem;
	wxSizer* m_GravityItem;
	wxSizer* m_AirControlItem;
	wxSizer* m_WavesItem;
	wxSizer* m_CtfRulesItem;
	wxSizer* m_LivesItem;
	wxSizer* m_RoundsItem;
	wxSizer* m_FastMonstersItem;
	wxSizer* m_TimeLeftItem;
	wxSizer* m_ScoreLimitItem;

	wxSizer*  m_TeamSizer;
	wxPanel*  m_ContentPanel;

	LstOdaPlayerList* m_PlayerList;
};

#endif // wxUSE_POPUPWIN
