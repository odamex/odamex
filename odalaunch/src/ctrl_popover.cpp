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
	wxString Revision;

	if(!s.Info.VersionRevStr.empty())
		Revision = wxString::Format(" (%s)", s.Info.VersionRevStr);
	else if(s.Info.VersionRevision != 0)
		Revision = wxString::Format(" (r%u)", s.Info.VersionRevision);

	m_Name->SetLabel(stdstr_towxstr(s.Info.Name));
	m_Version->SetLabel(wxString::Format("%u.%u.%u%s",
	                                     s.Info.VersionMajor,
	                                     s.Info.VersionMinor,
	                                     s.Info.VersionPatch,
	                                     Revision));
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

	m_WavesItem = MakeItem("Waves:", &m_Waves);
	LeftCol->Add(m_WavesItem, 0, wxBOTTOM, RowGap);

	// Right column, top to bottom
	RightCol->Add(MakeItem("Players:", &m_ClientPlayerCount), 0, wxBOTTOM, RowGap);

	m_ScoreLimitItem = MakeItem("Score:", &m_ScoreLimit);
	RightCol->Add(m_ScoreLimitItem, 0, wxBOTTOM, RowGap);

	m_RoundsItem = MakeItem("Rounds:", &m_Rounds);
	RightCol->Add(m_RoundsItem, 0, wxBOTTOM, RowGap);

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

	const int PlayerCount = (int)std::count_if(std::begin(s.Info.Players), std::end(s.Info.Players),
                           [] (const Player_t& obj) { return obj.Spectator == false; });

	const int NumPlayersCanJoin = (int)s.Info.MaxPlayers > 0 ?
		(int)s.Info.MaxPlayers - PlayerCount :
		(int)s.Info.MaxClients - PlayerCount;

	m_ClientPlayerCount->SetLabel(wxString::Format("%d / %d clients, %d player%s can join", Clients,
	                                                (int)s.Info.MaxClients, NumPlayersCanJoin,
	                                                NumPlayersCanJoin == 1 ? "" : "s"));
	m_GameType->SetLabel(OdaGetGameTypeString(s));

	wxString FriendlyFire;
	wxString FriendlyFireEnabled;

	if (OdaGetCvarValue(s, "sv_friendlyfire", FriendlyFireEnabled) &&
	    (s.Info.GameType == GT_TeamDeathmatch ||
	     s.Info.GameType == GT_CaptureTheFlag ||
	     s.Info.GameType == GT_Horde ||
	     s.Info.GameType == GT_Cooperative))
	{
		FriendlyFire = OdaGetCvarValue(s, "sv_friendlyfire", FriendlyFireEnabled)
		               ? "Yes" : "No";
	}
	
	SetOptionalItem(m_FriendlyFireItem, m_FriendlyFire, FriendlyFire);

	auto DamagePercent = [&s](const char* Cvar) -> wxString
	{
		wxString Value;
		double Factor = 1.0;

		if(OdaGetCvarValue(s, Cvar, Value) && Value.ToDouble(&Factor) &&
		        Factor != 1.0)
			return wxString::Format("%g%%", Factor * 100.0);

		return wxEmptyString;
	};

	SetOptionalItem(m_PlayerDmgItem, m_PlayerDmg, DamagePercent("sv_weapondamage"));

	wxString MonsterDmg;

	if(s.Info.GameType == GT_Horde || s.Info.GameType == GT_Cooperative)
		MonsterDmg = DamagePercent("sv_monsterdamage");

	SetOptionalItem(m_MonsterDmgItem, m_MonsterDmg, MonsterDmg);

	SetOptionalItem(m_MonsterHealthItem, m_MonsterHealth,
	                DamagePercent("sv_monstershealth"));

	// Waves (g_horde_waves) - Horde mode only
	wxString Waves;

	if(s.Info.GameType == GT_Horde)
		OdaGetCvarValue(s, "g_horde_waves", Waves);

	SetOptionalItem(m_WavesItem, m_Waves, Waves);

	// Lives (shown for all game modes)
	wxString Lives;

	OdaGetCvarValue(s, "g_lives", Lives);

	SetOptionalItem(m_LivesItem, m_Lives, Lives);

	// Rounds (shown for all game modes)
	wxString Rounds;
	wxString RoundsEnabled;

	if(OdaGetCvarValue(s, "g_rounds", RoundsEnabled))
	{
		const int RoundLimit = OdaGetCvarInt(s, "g_roundlimit", 0);
		const int WinLimit = OdaGetCvarInt(s, "g_winlimit", 0);

		if(RoundLimit == 0 && WinLimit == 0)
		{
			Rounds = "Unlimited";
		}
		else
		{
			if(RoundLimit > 0)
			{
				Rounds = wxString::Format("%d round limit", RoundLimit);

				if (WinLimit > 0 && RoundLimit > WinLimit)
					Rounds += wxString::Format(", first to %d win%s", WinLimit,
					                          WinLimit == 1 ? "" : "s");
			}
		}
	}

	SetOptionalItem(m_RoundsItem, m_Rounds, Rounds);

	// Time left only applies to timed games. Info.TimeLeft is in minutes; the
	// item collapses once no time remains (intermission) or the game has no timelimit.
	wxString TimeLeft;

	if(s.Info.TimeLimit)
		TimeLeft = OdaGetTimeString(s.Info.TimeLeft * 60);

	SetOptionalItem(m_TimeLeftItem, m_TimeLeft, TimeLeft);

	// Frag/point limit, shown only when the mode enforces one
	wxString ScoreLimit;

	int HighestScore = 0;

	if (s.Info.GameType == GT_TeamDeathmatch || s.Info.GameType == GT_CaptureTheFlag)
	{
		if (s.Info.ScoreLimit)
		{
			auto winningTeam = std::max_element(
			    s.Info.Teams.begin(), s.Info.Teams.end(),
			    [](const Team_t& a, const Team_t& b) { return a.Score < b.Score; });

			if (winningTeam != s.Info.Teams.end())
				HighestScore = winningTeam->Score;

			ScoreLimit = wxString::Format("%d / %u", HighestScore, s.Info.ScoreLimit);
		}
	}
	else if (s.Info.GameType == GT_Deathmatch)
	{
		if (s.Info.FragLimit)
		{
			auto winningPlayer = std::max_element(
			    s.Info.Players.begin(), s.Info.Players.end(),
			    [](const Player_t& a, const Player_t& b) { return a.Frags < b.Frags; });

			if (winningPlayer != s.Info.Players.end())
				HighestScore = winningPlayer->Frags;

			ScoreLimit = wxString::Format("%d / %u", HighestScore, s.Info.FragLimit);
		}
	}

	SetOptionalItem(m_ScoreLimitItem, m_ScoreLimit, ScoreLimit);

	// Team score boxes - a centered row of "Name: Score" boxes (white text on
	// the team colour), separated by " - ". Team modes only.
	m_TeamSizer->Clear(true);

	if((s.Info.GameType == GT_TeamDeathmatch ||
	        s.Info.GameType == GT_CaptureTheFlag) &&
	        !s.Info.Teams.empty())
	{
		const wxFont TeamFont =
		    wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT).Bold();

		for(size_t i = 0; i < s.Info.Teams.size(); ++i)
		{
			const Team_t& Team = s.Info.Teams[i];

			// " - " separator between teams
			if(i > 0)
				m_TeamSizer->Add(
				    new wxStaticText(m_ContentPanel, wxID_ANY, " - "), 0,
				    wxALIGN_CENTER_VERTICAL);

			const wxColour TeamColour((Team.Colour >> 16) & 0xFF,
			                          (Team.Colour >> 8) & 0xFF,
			                          Team.Colour & 0xFF);

			wxStaticText* Box = new wxStaticText(
			    m_ContentPanel, wxID_ANY,
			    wxString::Format(" %s: %d ", stdstr_towxstr(Team.Name),
			                     (int)Team.Score));

			Box->SetForegroundColour(*wxWHITE);
			Box->SetBackgroundColour(TeamColour);
			Box->SetFont(TeamFont);

			m_TeamSizer->Add(Box, 0, wxALIGN_CENTER_VERTICAL);
		}
	}

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

#endif // wxUSE_POPUPWIN
