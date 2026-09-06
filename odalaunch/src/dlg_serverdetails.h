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
//  Modal single-server details dialog.
//
//-----------------------------------------------------------------------------

#pragma once

#include "odalaunch.h"

#include <wx/dialog.h>
#include <wx/timer.h>
#include <wx/collpane.h>

#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "net_packet.h"

class dlgMain;
class LstOdaPlayerList;

class wxStaticText;
class wxStaticBitmap;
class wxTextCtrl;
class wxScrolledWindow;
class wxGauge;
class wxButton;
class wxToggleButton;
class wxHyperlinkCtrl;
class wxFlexGridSizer;
class wxBoxSizer;
class wxSizer;

class dlgServerDetails : public wxDialog
{
public:
	dlgServerDetails(dlgMain* parent);
	virtual ~dlgServerDetails();

	// Seeds the dialog from an already-queried server, repaints, and shows it
	// modally. This is less taxing than loading the XRC on demand.
	int ShowForServer(const odalpapi::Server& Source);

private:
	// Builds the static label/value grids once at details dialog construction time.
	void BuildMetadataGrid();
	void BuildGameplayGrid();

	// Re-queries the server then repaints every panel.
	void DoQuery();
	void Populate();
	void PopulateMetadata();
	void PopulateGameplay();
	void PopulateServerVars();
	void PopulatePlayerList();

	// Creates the cvar rows for a Server Variables pane the first time it is expanded.
	void BuildPaneContent(wxCollapsiblePane* Pane);

	void OnRefresh(wxCommandEvent& event);
	void OnJoin(wxCommandEvent& event);
	void OnTimer(wxTimerEvent& event);
	void OnClose(wxCloseEvent& event);

	// Connects to the server and closes the dialog.
	void DoJoin();

	// Refreshes the Join controls for the current fullness/refresh state.
	// Returns true if an armed "join when slot free" wait should fire now.
	bool UpdateJoinControls();

	// Creates a bold label + value row in a 2-column grid, returning the value
	// widget. The created label is written to *LabelOut when non-null (so the
	// row can later be hidden).
	wxStaticText* AddRow(wxWindow* Parent, wxFlexGridSizer* Grid,
	                     const wxString& Label, wxStaticText** LabelOut = NULL);

	// Shows/hides an optional grid row, setting the value when shown.
	void SetOptionalRow(wxFlexGridSizer* Grid, wxStaticText* Label,
	                    wxStaticText* Value, const wxString& Text);

	// As SetOptionalRow, but colours the whole value green when IsPositive, red
	// otherwise (for above/below-default percentage values).
	void SetColouredRow(wxFlexGridSizer* Grid, wxStaticText* Label,
	                    wxStaticText* Value, const wxString& Text,
	                    bool IsPositive);

	// Like AddRow but the value cell also holds a trailing delta control (for a
	// coloured "(+/-N%)" suffix). Returns the value widget; the label and delta
	// widgets are written to *LabelOut / *DeltaOut.
	wxStaticText* AddDeltaRow(wxFlexGridSizer* Grid, const wxString& Label,
	                          wxStaticText** LabelOut, wxStaticText** DeltaOut);

	// Shows/hides a value+delta row, setting the value and colouring the delta
	// (green when IsPositive, red otherwise) when shown. DefaultText, when set,
	// adds a "Default is X" hover hint to the value and delta.
	void SetDeltaRow(wxFlexGridSizer* Grid, wxStaticText* Label,
	                 wxStaticText* Value, wxStaticText* Delta,
	                 const wxString& ValueText, const wxString& DeltaText,
	                 bool IsPositive, const wxString& DefaultText);

	dlgMain* m_Parent;
	odalpapi::Server m_Server;

	wxTimer m_Timer;
	int m_GaugeTicks;

	// Top-level controls (from XRC)
	wxTextCtrl* m_TxtMotd;
	// The "Message of the day" static-box sizer, hidden when the MOTD is empty.
	wxSizer* m_MotdSizer;
	wxWindow* m_PnlMetadata;
	wxScrolledWindow* m_PnlServerVars;
	wxWindow* m_PnlGameplayVars;
	wxWindow* m_PnlPlayerList;
	wxGauge* m_Gauge;
	wxToggleButton* m_BtnRefresh;
	wxButton* m_BtnJoin;
	// Shown in place of m_BtnJoin while the server is full and a refresh cycle
	// is running: an armable "join as soon as a slot opens" toggle.
	wxToggleButton* m_BtnJoinWhenFree;

	LstOdaPlayerList* m_PlayerList;

	// Titles of the "Server Variables" category panes the user has expanded,
	// so the expansion state survives a refresh (which rebuilds the panes).
	std::set<wxString> m_ExpandedCategories;

	// Lazy load state for the Server Variables panes: each pane's (cvar name,
	// value) rows, and the set of panes whose controls have been created.
	std::map<wxCollapsiblePane*, std::vector<std::pair<std::string, wxString> > >
	    m_PaneRows;
	std::set<wxCollapsiblePane*> m_BuiltPanes;

	// Metadata grid
	wxFlexGridSizer* m_MetaGrid;
	wxStaticBitmap* m_PingIcon;
	wxStaticText* m_MdName;
	wxStaticText* m_MdVersion;
	wxStaticText* m_MdAddress;
	wxStaticText* m_MdPing;
	wxStaticText* m_MdSkill;
	wxStaticText* m_MdMap;
	wxStaticText* m_MdIwad;
	wxStaticText* m_MdPwad;
	wxStaticText* m_MdPwadLabel;
	// Download sites are a list, so the value cell holds a vertical sizer of
	// one mailto-style hyperlink per URL.
	wxBoxSizer* m_MdDownloadSizer;
	wxStaticText* m_MdDownloadURILabel;
	wxHyperlinkCtrl* m_MdAdminEmail;
	wxStaticText* m_MdAdminEmailLabel;
	wxStaticBitmap* m_PasswordIcon;
	wxStaticText* m_MdPassword;

	// Gameplay grid
	wxFlexGridSizer* m_GpGrid;
	wxStaticText* m_GpGameType;
	wxStaticText* m_GpTimeLeft;
	wxStaticText* m_GpTimeLeftLabel;
	wxStaticText* m_GpFriendlyFire;
	wxStaticText* m_GpFriendlyFireLabel;
	wxStaticText* m_GpPlayerDmg;
	wxStaticText* m_GpPlayerDmgLabel;
	wxStaticText* m_GpMonsterDmg;
	wxStaticText* m_GpMonsterDmgLabel;
	wxStaticText* m_GpMonsterHealth;
	wxStaticText* m_GpMonsterHealthLabel;
	wxStaticText* m_GpGravity;
	wxStaticText* m_GpGravityLabel;
	wxStaticText* m_GpGravityDelta;
	wxStaticText* m_GpAirControl;
	wxStaticText* m_GpAirControlLabel;
	wxStaticText* m_GpAirControlDelta;
	wxStaticText* m_GpWaves;
	wxStaticText* m_GpWavesLabel;
	wxStaticText* m_GpCtfRules;
	wxStaticText* m_GpCtfRulesLabel;
	wxStaticText* m_GpPlayers;
	wxStaticText* m_GpScore;
	wxStaticText* m_GpScoreLabel;
	wxBoxSizer*   m_GpTeamsSizer;
	wxStaticText* m_GpRounds;
	wxStaticText* m_GpRoundsLabel;
	wxStaticText* m_GpLives;
	wxStaticText* m_GpLivesLabel;
	wxStaticText* m_GpFastMonsters;
	wxStaticText* m_GpFastMonstersLabel;

	wxDECLARE_EVENT_TABLE();
};
