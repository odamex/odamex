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

#include "ctrl_popover.h"

#if wxUSE_POPUPWIN

#include <algorithm>

#include <wx/sizer.h>
#include <wx/wrapsizer.h>
#include <wx/settings.h>

#include "str_utils.h"
#include "srv_utils.h"

using namespace odalpapi;

// Maximum height we let the player popover grow to before relying on the
// list's own scrollbar.
static const int PLAYER_POPOVER_MAX_HEIGHT = 360;
static const int PLAYER_POPOVER_WIDTH = 720;

//
// ServerInfoPopover
//

ServerInfoPopover::ServerInfoPopover(wxWindow* parent)
	: wxPopupWindow(parent, wxBORDER_SIMPLE)
{
	m_Address = new wxStaticText(this, wxID_ANY, "");
	m_Name = new wxStaticText(this, wxID_ANY, "");
	m_Skill = new wxStaticText(this, wxID_ANY, "");
	m_Version = new wxStaticText(this, wxID_ANY, "");
	m_AdminEmail = new wxStaticText(this, wxID_ANY, "");
	m_DownloadURI = new wxStaticText(this, wxID_ANY, "");
	m_Password = new wxStaticText(this, wxID_ANY, "");

	// These labels are allowed to disappear if empty
	m_AdminEmailLabel = nullptr;
	m_DownloadURILabel = nullptr;

	wxPanel* Panel = new wxPanel(this);
	Panel->SetBackgroundColour(
	    wxSystemSettings::GetColour(wxSYS_COLOUR_INFOBK));

	wxFlexGridSizer* Grid = new wxFlexGridSizer(0, 2, 4, 12);

	// Helper to add a bold label + value row
	const wxFont LabelFont =
	    wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT).Bold();

	struct
	{
		const char*    Label;
		wxStaticText** Value;
		wxStaticText** LabelOut; // optional: where to store the label widget
	} Rows[] =
	{
		{ "Server Name:",      &m_Name,        nullptr },
		{ "Version:",          &m_Version,     nullptr },
		{ "Address:",          &m_Address,     nullptr },
		{ "Skill:",            &m_Skill,       nullptr },
		{ "Wad Download URI:", &m_DownloadURI, &m_DownloadURILabel },
		{ "Admin Email:",      &m_AdminEmail,  &m_AdminEmailLabel },
		{ "Password:",         &m_Password,    nullptr },
	};

	for(size_t i = 0; i < sizeof(Rows) / sizeof(Rows[0]); ++i)
	{
		wxStaticText* Label = new wxStaticText(Panel, wxID_ANY, Rows[i].Label);
		Label->SetFont(LabelFont);

		*Rows[i].Value = new wxStaticText(Panel, wxID_ANY, "");

		if(Rows[i].LabelOut)
			*Rows[i].LabelOut = Label;

		Grid->Add(Label, 0, wxALIGN_CENTER_VERTICAL);
		Grid->Add(*Rows[i].Value, 0, wxALIGN_CENTER_VERTICAL);
	}

	m_Grid = Grid;

	wxBoxSizer* Border = new wxBoxSizer(wxVERTICAL);
	Border->Add(Grid, 1, wxEXPAND | wxALL, 8);
	Panel->SetSizer(Border);

	wxBoxSizer* Outer = new wxBoxSizer(wxVERTICAL);
	Outer->Add(Panel, 1, wxEXPAND);
	SetSizer(Outer);

	Hide();
}

void ServerInfoPopover::Populate(const Server& s)
{
	m_Name->SetLabel(stdstr_towxstr(s.Info.Name));
	m_Version->SetLabel(OdaGetVersionString(s));
	m_Address->SetLabel(stdstr_towxstr(s.GetAddress()));
	m_Skill->SetLabel(OdaGetSkillString(s));

	// sv_downloadsites is a space-separated list of URIs; show one per line.
	// Empty/absent optional cvars collapse their row entirely.
	wxString DownloadSites;

	if(!OdaGetCvarValue(s, "sv_downloadsites", DownloadSites))
		DownloadSites.Clear();

	DownloadSites.Trim(true).Trim(false);
	DownloadSites.Replace(" ", "\n");

	SetOptionalRow(m_DownloadURILabel, m_DownloadURI, DownloadSites);

	wxString AdminEmail;

	if(!OdaGetCvarValue(s, "sv_email", AdminEmail))
		AdminEmail.Clear();

	SetOptionalRow(m_AdminEmailLabel, m_AdminEmail, AdminEmail);

	m_Password->SetLabel(s.Info.PasswordHash.empty() ? "No" : "Yes");

	// Recompute size now that some rows may have collapsed
	m_Grid->Layout();
	GetSizer()->Fit(this);
	Layout();
}

bool ServerInfoPopover::SetOptionalRow(wxStaticText* Label,
                                       wxStaticText* Field,
                                       const wxString& Value)
{
	const bool Show = !Value.IsEmpty();

	if(Show)
		Field->SetLabel(Value);

	m_Grid->Show(Label, Show);
	m_Grid->Show(Field, Show);

	return Show;
}

//
// PlayerListPopover
//

PlayerListPopover::PlayerListPopover(wxWindow* parent)
	: wxPopupWindow(parent, wxBORDER_SIMPLE)
{
	wxPanel* Panel = new wxPanel(this);
	Panel->SetBackgroundColour(
	    wxSystemSettings::GetColour(wxSYS_COLOUR_INFOBK));

	const wxFont LabelFont =
	    wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT).Bold();

	// Builds a single inline "Label: value" item and returns its sub-sizer.
	auto MakeItem = [&](const wxString& LabelText,
	                    wxStaticText** ValueOut) -> wxSizer*
	{
		wxBoxSizer* Item = new wxBoxSizer(wxHORIZONTAL);

		wxStaticText* Label = new wxStaticText(Panel, wxID_ANY, LabelText);
		Label->SetFont(LabelFont);

		*ValueOut = new wxStaticText(Panel, wxID_ANY, "");

		Item->Add(Label, 0, wxALIGN_CENTER_VERTICAL);
		Item->Add(*ValueOut, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 4);

		return Item;
	};

	// The same as MakeItem, but with a trailing delta control for a colored "(+/-X%)" control.
	auto MakeDeltaItem = [&](const wxString& LabelText, wxStaticText** ValueOut,
	                         wxStaticText** DeltaOut) -> wxSizer*
	{
		wxBoxSizer* Item = new wxBoxSizer(wxHORIZONTAL);

		wxStaticText* Label = new wxStaticText(Panel, wxID_ANY, LabelText);
		Label->SetFont(LabelFont);

		*ValueOut = new wxStaticText(Panel, wxID_ANY, "");
		*DeltaOut = new wxStaticText(Panel, wxID_ANY, "");

		Item->Add(Label, 0, wxALIGN_CENTER_VERTICAL);
		Item->Add(*ValueOut, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 4);
		Item->Add(*DeltaOut, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 4);

		return Item;
	};

	// Gap between consecutive items in the wrapping "rest" row
	const int ItemGap = 18;
	// Vertical gap between items stacked within a column
	const int RowGap = 4;
	// Gap between the two columns
	const int ColGap = 28;

	// Master vertical sizer: a two-column block on top, then a wrapping row of
	// the remaining items underneath. SetOptionalItem() hides items via a
	// recursive search from this sizer, so the nesting doesn't matter.
	wxBoxSizer* HeaderBox = new wxBoxSizer(wxVERTICAL);
	m_HeaderSizer = HeaderBox;

	wxBoxSizer* Columns = new wxBoxSizer(wxHORIZONTAL);
	wxBoxSizer* LeftCol = new wxBoxSizer(wxVERTICAL);
	wxBoxSizer* RightCol = new wxBoxSizer(wxVERTICAL);

	// Left column, top to bottom
	LeftCol->Add(MakeItem("Game Type:", &m_GameType), 0, wxBOTTOM, RowGap);

	m_TimeLeftItem = MakeItem("Time Left:", &m_TimeLeft);
	LeftCol->Add(m_TimeLeftItem, 0, wxBOTTOM, RowGap);

	m_FriendlyFireItem = MakeItem("Friendly Fire:", &m_FriendlyFire);
	LeftCol->Add(m_FriendlyFireItem, 0, wxBOTTOM, RowGap);

	m_PlayerDmgItem = MakeItem("Player Dmg:", &m_PlayerDmg);
	LeftCol->Add(m_PlayerDmgItem, 0, wxBOTTOM, RowGap);

	m_MonsterDmgItem = MakeItem("Monster Dmg:", &m_MonsterDmg);
	LeftCol->Add(m_MonsterDmgItem, 0, wxBOTTOM, RowGap);

	m_MonsterHealthItem = MakeItem("Monster Health:", &m_MonsterHealth);
	LeftCol->Add(m_MonsterHealthItem, 0, wxBOTTOM, RowGap);

	m_GravityItem = MakeDeltaItem("Gravity:", &m_Gravity, &m_GravityDelta);
	LeftCol->Add(m_GravityItem, 0, wxBOTTOM, RowGap);

	m_AirControlItem =
	    MakeDeltaItem("Air Control:", &m_AirControl, &m_AirControlDelta);
	LeftCol->Add(m_AirControlItem, 0, wxBOTTOM, RowGap);

	m_WavesItem = MakeItem("Waves:", &m_Waves);
	LeftCol->Add(m_WavesItem, 0, wxBOTTOM, RowGap);

	m_CtfRulesItem = MakeItem("CTF Rules:", &m_CtfRules);
	LeftCol->Add(m_CtfRulesItem, 0, wxBOTTOM, RowGap);

	// Right column, top to bottom
	RightCol->Add(MakeItem("Players:", &m_ClientPlayerCount), 0, wxBOTTOM, RowGap);

	m_ScoreLimitItem = MakeItem("Score:", &m_ScoreLimit);
	RightCol->Add(m_ScoreLimitItem, 0, wxBOTTOM, RowGap);

	m_RoundsItem = MakeItem("Rounds:", &m_Rounds);
	RightCol->Add(m_RoundsItem, 0, wxBOTTOM, RowGap);

	m_FastMonstersItem = MakeItem("Fast Monsters:", &m_FastMonsters);
	RightCol->Add(m_FastMonstersItem, 0, wxBOTTOM, RowGap);

	// Remaining items flow in a wrapping row at the bottom of the right column.
	wxWrapSizer* RestRow = new wxWrapSizer(wxHORIZONTAL);

	m_LivesItem = MakeItem("Lives:", &m_Lives);
	RestRow->Add(m_LivesItem, 0, wxRIGHT, ItemGap);

	RightCol->Add(RestRow, 0);

	Columns->Add(LeftCol, 0, wxRIGHT, ColGap);
	Columns->Add(RightCol, 0);

	HeaderBox->Add(Columns, 0);

	// Team labels are created on demand in Populate(), so remember their parent.
	m_ContentPanel = Panel;

	// Centered row of team score boxes, populated for team modes only.
	m_TeamSizer = new wxBoxSizer(wxHORIZONTAL);

	m_PlayerList = new LstOdaPlayerList();
	m_PlayerList->Create(Panel, wxID_ANY, wxDefaultPosition,
	                     wxSize(PLAYER_POPOVER_WIDTH, PLAYER_POPOVER_MAX_HEIGHT),
	                     wxLC_REPORT | wxLC_SINGLE_SEL);

	wxBoxSizer* Inner = new wxBoxSizer(wxVERTICAL);
	Inner->Add(m_HeaderSizer, 0, wxEXPAND | wxALL, 8);
	Inner->Add(m_TeamSizer, 0, wxALIGN_CENTER_HORIZONTAL);
	Inner->Add(m_PlayerList, 1, wxEXPAND | wxALL, 6);
	Panel->SetSizer(Inner);

	wxBoxSizer* Outer = new wxBoxSizer(wxVERTICAL);
	Outer->Add(Panel, 1, wxEXPAND);
	SetSizer(Outer);

	Hide();
}

void PlayerListPopover::Populate(const Server& s)
{
	const int Clients = (int)s.Info.Players.size();

	m_ClientPlayerCount->SetLabel(OdaGetPlayerCountString(s));
	m_GameType->SetLabel(OdaGetGameTypeString(s));

	SetOptionalItem(m_FriendlyFireItem, m_FriendlyFire,
	                OdaGetFriendlyFireString(s));

	const OdaModifier_t PlayerDmg =
	    OdaGetDamagePercent(s, "sv_weapondamage");
	SetColouredItem(m_PlayerDmgItem, m_PlayerDmg, PlayerDmg.Value,
	                PlayerDmg.IsPositive);

	OdaModifier_t MonsterDmg;

	if(s.Info.GameType == GT_Horde || s.Info.GameType == GT_Cooperative)
		MonsterDmg = OdaGetDamagePercent(s, "sv_monsterdamage");

	SetColouredItem(m_MonsterDmgItem, m_MonsterDmg, MonsterDmg.Value,
	                !MonsterDmg.IsPositive);

	const OdaModifier_t MonsterHealth =
	    OdaGetDamagePercent(s, "sv_monstershealth");
	SetColouredItem(m_MonsterHealthItem, m_MonsterHealth, MonsterHealth.Value,
	                !MonsterHealth.IsPositive);

	const OdaModifier_t Gravity = OdaGetGravity(s);
	m_GravityDelta->SetLabel(Gravity.Delta);
	OdaApplyDeltaColour(m_GravityDelta, Gravity.IsPositive);
	SetOptionalItem(m_GravityItem, m_Gravity, Gravity.Value);

	const OdaModifier_t AirControl = OdaGetAirControl(s);
	m_AirControlDelta->SetLabel(AirControl.Delta);
	OdaApplyDeltaColour(m_AirControlDelta, AirControl.IsPositive);
	SetOptionalItem(m_AirControlItem, m_AirControl, AirControl.Value);

	// Waves (g_horde_waves) - Horde mode only
	wxString Waves;

	if(s.Info.GameType == GT_Horde)
		OdaGetCvarValue(s, "g_horde_waves", Waves);

	SetOptionalItem(m_WavesItem, m_Waves, Waves);

	SetOptionalItem(m_CtfRulesItem, m_CtfRules, OdaGetCtfRulesString(s));

	// Lives (shown for all game modes)
	wxString Lives;

	OdaGetCvarValue(s, "g_lives", Lives);

	SetOptionalItem(m_LivesItem, m_Lives, Lives);

	SetOptionalItem(m_RoundsItem, m_Rounds, OdaGetRoundsString(s));
	SetOptionalItem(m_FastMonstersItem, m_FastMonsters,
	                OdaGetFastMonstersString(s));
	SetOptionalItem(m_TimeLeftItem, m_TimeLeft, OdaGetTimeLeftString(s));
	SetOptionalItem(m_ScoreLimitItem, m_ScoreLimit, OdaGetScoreString(s));

	OdaBuildTeamScoreBoxes(m_ContentPanel, m_TeamSizer, s);

	m_PlayerList->DeleteAllItems();
	m_PlayerList->AddPlayersToList(s);

	// 18 seems like a good maxitem number.
	int RowHeight = 18;

	if(m_PlayerList->GetItemCount() > 0)
	{
		wxRect Rect;

		if(m_PlayerList->GetItemRect(0, Rect) && Rect.height > 0)
			RowHeight = Rect.height;
	}

	// header row + one row per player, plus a little padding
	int ListHeight = (Clients + 2) * RowHeight + 8;

	if(ListHeight > PLAYER_POPOVER_MAX_HEIGHT)
		ListHeight = PLAYER_POPOVER_MAX_HEIGHT;

	m_PlayerList->SetMinSize(wxSize(PLAYER_POPOVER_WIDTH, ListHeight));

	// Recompute size now that some game-mode items may have collapsed
	m_HeaderSizer->Layout();
	GetSizer()->Fit(this);
	Layout();
}

bool PlayerListPopover::SetOptionalItem(wxSizer* Item, wxStaticText* Field,
                                        const wxString& Value)
{
	const bool Show = !Value.IsEmpty();

	if(Show)
		Field->SetLabel(Value);

	// Hiding the item's sub-sizer hides its label and value and lets the
	// surrounding column / wrap sizer reflow. The recursive search is needed
	// because items now live in nested column sizers, not directly in
	// m_HeaderSizer.
	m_HeaderSizer->Show(Item, Show, true);

	return Show;
}

void PlayerListPopover::SetColouredItem(wxSizer* Item, wxStaticText* Field,
                                        const wxString& Value, bool IsPositive)
{
	if(SetOptionalItem(Item, Field, Value))
		OdaApplyDeltaColour(Field, IsPositive);
}

#endif // wxUSE_POPUPWIN
