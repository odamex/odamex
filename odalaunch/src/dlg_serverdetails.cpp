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

#include "dlg_serverdetails.h"

#include <algorithm>
#include <map>
#include <set>
#include <utility>
#include <vector>

#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/statbmp.h>
#include <wx/textctrl.h>
#include <wx/scrolwin.h>
#include <wx/gauge.h>
#include <wx/button.h>
#include <wx/tglbtn.h>
#include <wx/hyperlink.h>
#include <wx/gbsizer.h>
#include <wx/collpane.h>
#include <wx/settings.h>
#include <wx/fileconf.h>
#include <wx/arrstr.h>
#include <wx/cursor.h>
#include <wx/app.h>
#include <wx/xrc/xmlres.h>

#ifdef UNIX
#undef UNIX
#include "dlg_main.h"
#define UNIX
#else
#include "dlg_main.h"
#endif

#include "lst_players.h"
#include "srv_utils.h"
#include "str_utils.h"
#include "cvardoc_db.h"
#include "net_io.h"
#include "oda_defs.h"

using namespace odalpapi;

// Refresh cadence: a tick every 250ms, auto-refreshing after this many ticks
// (40 * 250ms = 10 seconds).
static const int REFRESH_TICK_MS = 250;
static const int REFRESH_TICKS = 40;

wxBEGIN_EVENT_TABLE(dlgServerDetails, wxDialog)
	EVT_TOGGLEBUTTON(XRCID("Id_BtnRefresh"), dlgServerDetails::OnRefresh)
	EVT_BUTTON(XRCID("Id_BtnJoin"), dlgServerDetails::OnJoin)
	EVT_TIMER(wxID_ANY, dlgServerDetails::OnTimer)
	EVT_CLOSE(dlgServerDetails::OnClose)
wxEND_EVENT_TABLE()

// Category -> cvar-name groups for the "Server Variables" accordion, in the
// order they should appear.
typedef std::vector<std::pair<wxString, std::vector<std::string> > > CvarCategories;

static const CvarCategories& GetCvarCategories()
{
	static CvarCategories Categories;

	if(!Categories.empty())
		return Categories;

	auto Add = [](const wxString& Name, std::vector<std::string> Cvars)
	{
		Categories.push_back(std::make_pair(Name, std::move(Cvars)));
	};

	Add("Gameplay Options",
	    {"sv_gametype", "sv_teamsinplay", "sv_teamspawns", "sv_maxcorpses", "g_sides"});
	Add("Gameplay Modifiers",
	    {"sv_aircontrol", "sv_doubleammo", "sv_fastmonsters", "sv_forcerespawn",
	     "sv_forcerespawntime", "sv_forcewater", "sv_friendlyfire",
	     "sv_friendlymonsterfire", "sv_gravity", "sv_infiniteammo",
	     "sv_itemrespawntime", "sv_itemsrespawn", "sv_keepkeys",
	     "sv_monsterdamage", "sv_monstershealth", "sv_monstersrespawn",
	     "sv_nomonsters", "sv_showplayerpowerups", "sv_skill",
	     "sv_spawndelaytime", "sv_splashfactor", "sv_weapondamage",
	     "sv_weapondrop", "sv_weaponstay", "sv_dmfarspawn", "g_spawninv",
	     "g_thingfilter"});
	Add("Optional Gameplay Functionality",
	    {"sv_allowcheats", "sv_allowjump", "sv_allowfov", "sv_allowmovebob",
	     "sv_allowpwo", "sv_allowredscreen", "sv_allowshowspawns",
	     "sv_allowtargetnames", "sv_allowwidescreen", "sv_freelook",
	     "sv_maxunlagtime", "sv_playerbeacons", "sv_respawnbarrels",
	     "sv_respawnsuper", "sv_sharekeys", "sv_unblockfriendly",
	     "sv_unblockplayers"});
	Add("Game Flow",
	    {"sv_countdown", "sv_intermissionlimit", "sv_warmup",
	     "sv_warmup_autostart", "sv_emptyfreeze", "sv_emptyreset"});
	Add("Win Conditions",
	    {"sv_allowexit", "sv_fragexitswitch", "sv_fraglimit", "sv_scorelimit",
	     "sv_timelimit"});
	Add("WAD Downloads", {"sv_downloadsites"});
	Add("Metadata", {"sv_hostname", "sv_email", "sv_motd", "g_gametypename"});
	Add("Players", {"sv_maxclients", "sv_maxplayers", "sv_maxplayersperteam"});
	Add("Sprees", {"sv_showsprees"});
	Add("Multi Kills", {"sv_showmultikills"});
	Add("Voting",
	    {"sv_callvote_coinflip", "sv_callvote_forcespec", "sv_callvote_forcestart",
	     "sv_callvote_fraglimit", "sv_callvote_kick", "sv_callvote_lives",
	     "sv_callvote_map", "sv_callvote_nextmap", "sv_callvote_randcaps",
	     "sv_callvote_randmap", "sv_callvote_randpickup", "sv_callvote_restart",
	     "sv_callvote_scorelimit", "sv_callvote_timelimit", "sv_vote_countabs",
	     "sv_vote_majority", "sv_vote_speccall", "sv_vote_specvote",
	     "sv_vote_timelimit", "sv_vote_timeout"});
	Add("Security", {"join_password", "rcon_password", "sv_flooddelay"});
	Add("Bans", {"sv_banfile", "sv_banfile_reload"});
	Add("Script Variables",
	    {"sv_clientcount", "sv_curmap", "sv_curpwad", "sv_endmapscript",
	     "sv_nextmap", "sv_startmapscript", "sv_startwadscript"});
	Add("Networking",
	    {"sv_maxrate", "sv_natport", "sv_ticbuffer", "sv_upnp",
	     "sv_upnp_description", "sv_upnp_discovertimeout", "sv_upnp_externalip",
	     "sv_upnp_internalip", "sv_usemasters", "net_rcvbuf", "net_sndbuf",
	     "port"});
	Add("Console Log", {"log_color", "log_fulltimestamps", "log_packetdebug"});
	Add("Chat", {"sv_globalspectatorchat"});
	Add("Maplist", {"sv_shufflemaplist"});
	Add("Diagnostics", {"configver", "developer"});
	Add("Compatibility Options",
	    {"co_allowdropoff", "co_avoidhazards", "co_blockmapfix", "co_boomphys",
	     "co_fineautoaim", "co_fixweaponimpacts", "co_friend_distance",
	     "co_friend_helpertype", "co_friend_ledgejumping",
	     "co_friend_playerhelpers", "co_globalsound", "co_helpfriends",
	     "co_mbfphys", "co_monsterbacking", "co_monsterfriction",
	     "co_monstersclimbsteep", "co_nosilentspawns", "co_novileghosts",
	     "co_pursuit", "co_realactorheight", "co_removesoullimit", "co_staylift",
	     "co_zdoomammo", "co_zdoomphys", "co_zdoomsound"});
	Add("CTF", {"g_ctf_notouchreturn", "ctf_flagtimeout", "ctf_flagathometoscore", "ctf_manualreturn"});
	Add("Horde",
	    {"g_horde_extralife", "g_horde_goalhp", "g_horde_maxtotalhp",
	     "g_horde_mintotalhp", "g_horde_resurrect", "g_horde_spawnempty_max",
	     "g_horde_spawnempty_min", "g_horde_spawnfull_max",
	     "g_horde_spawnfull_min", "g_horde_waves"});
	Add("Survival", {"g_lives", "g_lives_jointimer"});
	Add("Rounds",
	    {"g_rounds", "g_preroundtime", "g_postroundtime", "g_preroundreset",
	     "g_roundlimit", "g_winlimit"});
	Add("Progression", {"g_resetinvonexit", "g_winnerstays"});

	return Categories;
}

dlgServerDetails::dlgServerDetails(dlgMain* parent)
	: m_Parent(parent), m_Timer(this), m_GaugeTicks(0)
{
	wxXmlResource::Get()->LoadDialog(this, parent, "dlgServerDetails");

	m_TxtMotd = XRCCTRL(*this, "Id_TxtMotd", wxTextCtrl);
	m_PnlMetadata = XRCCTRL(*this, "Id_PnlMetadata", wxPanel);
	m_PnlServerVars = XRCCTRL(*this, "Id_PnlServerVars", wxScrolledWindow);
	m_PnlGameplayVars = XRCCTRL(*this, "Id_PnlGameplayVars", wxPanel);
	m_PnlPlayerList = XRCCTRL(*this, "Id_PnlPlayerList", wxPanel);
	m_Gauge = XRCCTRL(*this, "Id_GaugeRefresh", wxGauge);
	m_BtnRefresh = XRCCTRL(*this, "Id_BtnRefresh", wxToggleButton);
	m_BtnJoin = XRCCTRL(*this, "Id_BtnJoin", wxButton);
	m_BtnJoinWhenFree = XRCCTRL(*this, "Id_BtnJoinWhenFree", wxToggleButton);

	m_Gauge->SetRange(REFRESH_TICKS);
	m_Gauge->SetValue(0);

	// Show the MOTD in a bold, monospace font.
	// Also centered but that's in the actual XRC file.
	wxFont MotdFont(wxNORMAL_FONT->GetPointSize(), wxFONTFAMILY_TELETYPE,
	                wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD);
	m_TxtMotd->SetFont(MotdFont);

	// How can we make this look disabled without actually being disabled?
	m_TxtMotd->SetBackgroundColour(
	    wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE));
	m_TxtMotd->SetForegroundColour(
	    wxSystemSettings::GetColour(wxSYS_COLOUR_BTNTEXT));
	// Show the normal arrow cursor instead of the I-beam edit cursor.
	m_TxtMotd->SetCursor(wxCursor(wxCURSOR_ARROW));
	m_TxtMotd->Bind(wxEVT_SET_FOCUS, [this](wxFocusEvent& evt)
	{
		// Bounce focus to the Refresh button so the MOTD never shows a caret
		// or holds a selection.
		if(m_BtnRefresh)
			m_BtnRefresh->SetFocus();
	});

	// Remember the MOTD's static-box sizer so the whole section can be hidden
	// when the server reports no MOTD.
	m_MotdSizer = m_TxtMotd->GetContainingSizer();

	BuildMetadataGrid();
	BuildGameplayGrid();

	// Player list lives inside its placeholder panel.
	m_PlayerList = new LstOdaPlayerList();
	m_PlayerList->Create(m_PnlPlayerList, wxID_ANY, wxDefaultPosition,
	                     wxDefaultSize, wxLC_REPORT | wxLC_SINGLE_SEL);

	wxBoxSizer* PlayerSizer = new wxBoxSizer(wxVERTICAL);
	PlayerSizer->Add(m_PlayerList, 1, wxEXPAND);
	m_PnlPlayerList->SetSizer(PlayerSizer);

	// Server variables get a vertical sizer of collapsible panes.
	m_PnlServerVars->SetSizer(new wxBoxSizer(wxVERTICAL));

	// Reflow the scrolled area whenever a category pane expands/collapses, and
	// remember the expansion state so a refresh can restore it.
	Bind(wxEVT_COLLAPSIBLEPANE_CHANGED,
	     [this](wxCollapsiblePaneEvent& evt)
	     {
		     wxCollapsiblePane* Pane =
		         wxDynamicCast(evt.GetEventObject(), wxCollapsiblePane);

		     Freeze();

		     if(Pane)
		     {
			     if(evt.GetCollapsed())
			     {
				     m_ExpandedCategories.erase(Pane->GetLabel());
			     }
			     else
			     {
				     // Build the pane's rows the first time it opens.
				     Pane->Freeze();
				     BuildPaneContent(Pane);
				     m_ExpandedCategories.insert(Pane->GetLabel());
				     Pane->Thaw();
			     }
		     }

		     m_PnlServerVars->FitInside();
		     m_PnlServerVars->Layout();
		     Thaw();
	     });

	// On idle, build the panes one at a time so the dialog stays snappy and responsive.
	Bind(wxEVT_IDLE, [this](wxIdleEvent& evt)
	{
		for(std::map<wxCollapsiblePane*,
		             std::vector<std::pair<std::string, wxString> > >::iterator
		        it = m_PaneRows.begin(); it != m_PaneRows.end(); ++it)
		{
			if(!m_BuiltPanes.count(it->first))
			{
				it->first->Freeze();
				BuildPaneContent(it->first);
				it->first->Thaw();
				evt.RequestMore(); // more panes may remain; keep idling
				return;
			}
		}
	});

	SetMinSize(wxSize(960, 680));
	SetSize(wxSize(1120, 820));
}

int dlgServerDetails::ShowForServer(const Server& Source)
{
	// Seed our working copy from the already-queried list entry so the dialog
	// opens instantly.
	std::string Host;
	uint16_t Port = 0;
	Source.GetAddress(Host, Port);
	m_Server.SetAddress(Host, Port);
	m_Server.Info = Source.Info;
	m_Server.SetPing(Source.GetPing());
	m_Server.SetValidResponse(Source.GotResponse());

	// Start each showing with auto-refresh off and the gauge reset.
	m_Timer.Stop();
	m_GaugeTicks = 0;
	m_BtnRefresh->SetValue(false);
	m_Gauge->SetValue(0);

	Populate();
	CentreOnParent();

	// Kick off the background pane building (see the wxEVT_IDLE handler).
	wxWakeUpIdle();

	return ShowModal();
}

dlgServerDetails::~dlgServerDetails()
{
	m_Timer.Stop();
}

wxStaticText* dlgServerDetails::AddRow(wxWindow* Parent, wxFlexGridSizer* Grid,
                                       const wxString& Label,
                                       wxStaticText** LabelOut)
{
	const wxFont LabelFont =
	    wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT).Bold();

	wxStaticText* LabelCtrl = new wxStaticText(Parent, wxID_ANY, Label);
	LabelCtrl->SetFont(LabelFont);

	wxStaticText* Value = new wxStaticText(Parent, wxID_ANY, "");

	Grid->Add(LabelCtrl, 0, wxALIGN_CENTER_VERTICAL);
	Grid->Add(Value, 0, wxALIGN_CENTER_VERTICAL);

	if(LabelOut)
		*LabelOut = LabelCtrl;

	return Value;
}

void dlgServerDetails::SetOptionalRow(wxFlexGridSizer* Grid,
                                      wxStaticText* Label, wxStaticText* Value,
                                      const wxString& Text)
{
	const bool Show = !Text.IsEmpty();

	if(Show)
		Value->SetLabel(Text);

	Grid->Show(Label, Show);
	Grid->Show(Value, Show);
}

void dlgServerDetails::SetColouredRow(wxFlexGridSizer* Grid, wxStaticText* Label,
                                      wxStaticText* Value, const wxString& Text,
                                      bool IsPositive)
{
	SetOptionalRow(Grid, Label, Value, Text);

	if(!Text.IsEmpty())
		OdaApplyDeltaColour(Value, IsPositive);
}

wxStaticText* dlgServerDetails::AddDeltaRow(wxFlexGridSizer* Grid,
                                            const wxString& Label,
                                            wxStaticText** LabelOut,
                                            wxStaticText** DeltaOut)
{
	const wxFont LabelFont =
	    wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT).Bold();

	wxStaticText* LabelCtrl =
	    new wxStaticText(m_PnlGameplayVars, wxID_ANY, Label);
	LabelCtrl->SetFont(LabelFont);

	wxStaticText* Value = new wxStaticText(m_PnlGameplayVars, wxID_ANY, "");
	wxStaticText* Delta = new wxStaticText(m_PnlGameplayVars, wxID_ANY, "");

	// The value cell holds the plain value plus a trailing coloured delta.
	wxBoxSizer* Cell = new wxBoxSizer(wxHORIZONTAL);
	Cell->Add(Value, 0, wxALIGN_CENTER_VERTICAL);
	Cell->Add(Delta, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 4);

	Grid->Add(LabelCtrl, 0, wxALIGN_CENTER_VERTICAL);
	Grid->Add(Cell, 0, wxALIGN_CENTER_VERTICAL);

	if(LabelOut)
		*LabelOut = LabelCtrl;
	if(DeltaOut)
		*DeltaOut = Delta;

	return Value;
}

void dlgServerDetails::SetDeltaRow(wxFlexGridSizer* Grid, wxStaticText* Label,
                                   wxStaticText* Value, wxStaticText* Delta,
                                   const wxString& ValueText,
                                   const wxString& DeltaText, bool IsPositive,
                                   const wxString& DefaultText)
{
	const bool Show = !ValueText.IsEmpty();

	if(Show)
	{
		Value->SetLabel(ValueText);
		Delta->SetLabel(DeltaText);
		OdaApplyDeltaColour(Delta, IsPositive);
		OdaApplyDefaultHint(Delta, DefaultText);
	}

	// The value and delta live in a nested cell sizer, so hide them recursively.
	Grid->Show(Label, Show);
	Grid->Show(Value, Show, true);
	Grid->Show(Delta, Show, true);
}

void dlgServerDetails::BuildMetadataGrid()
{
	const wxFont LabelFont =
	    wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT).Bold();

	m_MetaGrid = new wxFlexGridSizer(0, 2, 4, 12);

	m_MdName = AddRow(m_PnlMetadata, m_MetaGrid, "Server Name:");
	m_MdVersion = AddRow(m_PnlMetadata, m_MetaGrid, "Version:");
	m_MdAddress = AddRow(m_PnlMetadata, m_MetaGrid, "Address:");

	// Ping row: coloured bullet + number.
	wxStaticText* PingLabel = new wxStaticText(m_PnlMetadata, wxID_ANY, "Ping:");
	PingLabel->SetFont(LabelFont);

	wxBoxSizer* PingSizer = new wxBoxSizer(wxHORIZONTAL);
	m_PingIcon = new wxStaticBitmap(m_PnlMetadata, wxID_ANY, wxNullBitmap);
	m_MdPing = new wxStaticText(m_PnlMetadata, wxID_ANY, "");
	PingSizer->Add(m_PingIcon, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 4);
	PingSizer->Add(m_MdPing, 0, wxALIGN_CENTER_VERTICAL);

	m_MetaGrid->Add(PingLabel, 0, wxALIGN_CENTER_VERTICAL);
	m_MetaGrid->Add(PingSizer, 0, wxALIGN_CENTER_VERTICAL);

	m_MdSkill = AddRow(m_PnlMetadata, m_MetaGrid, "Skill:");
	m_MdMap = AddRow(m_PnlMetadata, m_MetaGrid, "Map:");
	m_MdIwad = AddRow(m_PnlMetadata, m_MetaGrid, "IWAD:");
	m_MdPwad = AddRow(m_PnlMetadata, m_MetaGrid, "PWAD:", &m_MdPwadLabel);
	// Wad Download URI: bold label + a vertical list of hyperlinks (one per
	// site), filled in on populate.
	m_MdDownloadURILabel =
	    new wxStaticText(m_PnlMetadata, wxID_ANY, "Wad Download URI:");
	m_MdDownloadURILabel->SetFont(LabelFont);
	m_MdDownloadSizer = new wxBoxSizer(wxVERTICAL);
	m_MetaGrid->Add(m_MdDownloadURILabel, 0, wxALIGN_TOP);
	m_MetaGrid->Add(m_MdDownloadSizer, 0, wxALIGN_TOP);

	m_MdAdminEmailLabel =
	    new wxStaticText(m_PnlMetadata, wxID_ANY, "Admin Email:");
	m_MdAdminEmailLabel->SetFont(LabelFont);
	m_MdAdminEmail = new wxHyperlinkCtrl(m_PnlMetadata, wxID_ANY,
	                                     wxEmptyString, wxEmptyString);
	m_MetaGrid->Add(m_MdAdminEmailLabel, 0, wxALIGN_CENTER_VERTICAL);
	m_MetaGrid->Add(m_MdAdminEmail, 0, wxALIGN_CENTER_VERTICAL);

	// Password row: a padlock icon (shown only when passworded) + Y/N.
	wxStaticText* PasswordLabel =
	    new wxStaticText(m_PnlMetadata, wxID_ANY, "Password:");
	PasswordLabel->SetFont(LabelFont);

	wxBoxSizer* PasswordSizer = new wxBoxSizer(wxHORIZONTAL);
	m_PasswordIcon =
	    new wxStaticBitmap(m_PnlMetadata, wxID_ANY, wxNullBitmap);
	m_MdPassword = new wxStaticText(m_PnlMetadata, wxID_ANY, "");
	PasswordSizer->Add(m_PasswordIcon, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 4);
	PasswordSizer->Add(m_MdPassword, 0, wxALIGN_CENTER_VERTICAL);

	m_MetaGrid->Add(PasswordLabel, 0, wxALIGN_CENTER_VERTICAL);
	m_MetaGrid->Add(PasswordSizer, 0, wxALIGN_CENTER_VERTICAL);

	wxBoxSizer* Border = new wxBoxSizer(wxVERTICAL);
	Border->Add(m_MetaGrid, 1, wxEXPAND | wxALL, 4);
	m_PnlMetadata->SetSizer(Border);
}

void dlgServerDetails::BuildGameplayGrid()
{
	m_GpGrid = new wxFlexGridSizer(0, 2, 4, 12);

	m_GpGameType = AddRow(m_PnlGameplayVars, m_GpGrid, "Game Type:");
	m_GpTimeLeft =
	    AddRow(m_PnlGameplayVars, m_GpGrid, "Time Left:", &m_GpTimeLeftLabel);
	m_GpFriendlyFire = AddRow(m_PnlGameplayVars, m_GpGrid, "Friendly Fire:",
	                          &m_GpFriendlyFireLabel);
	m_GpPlayerDmg =
	    AddRow(m_PnlGameplayVars, m_GpGrid, "Player Dmg:", &m_GpPlayerDmgLabel);
	m_GpMonsterDmg =
	    AddRow(m_PnlGameplayVars, m_GpGrid, "Monster Dmg:", &m_GpMonsterDmgLabel);
	m_GpMonsterHealth = AddRow(m_PnlGameplayVars, m_GpGrid, "Monster Health:",
	                           &m_GpMonsterHealthLabel);
	m_GpGravity = AddDeltaRow(m_GpGrid, "Gravity:", &m_GpGravityLabel,
	                          &m_GpGravityDelta);
	m_GpAirControl = AddDeltaRow(m_GpGrid, "Air Control:", &m_GpAirControlLabel,
	                             &m_GpAirControlDelta);
	m_GpWaves = AddRow(m_PnlGameplayVars, m_GpGrid, "Waves:", &m_GpWavesLabel);
	m_GpCtfRules =
	    AddRow(m_PnlGameplayVars, m_GpGrid, "CTF Rules:", &m_GpCtfRulesLabel);
	m_GpPlayers = AddRow(m_PnlGameplayVars, m_GpGrid, "Players:");
	m_GpScore = AddRow(m_PnlGameplayVars, m_GpGrid, "Score:", &m_GpScoreLabel);
	m_GpRounds = AddRow(m_PnlGameplayVars, m_GpGrid, "Rounds:", &m_GpRoundsLabel);
	m_GpLives = AddRow(m_PnlGameplayVars, m_GpGrid, "Lives:", &m_GpLivesLabel);
	m_GpFastMonsters = AddRow(m_PnlGameplayVars, m_GpGrid, "Fast Monsters:",
	                          &m_GpFastMonstersLabel);

	m_GpTeamsSizer = new wxBoxSizer(wxHORIZONTAL);

	wxBoxSizer* Border = new wxBoxSizer(wxVERTICAL);
	Border->Add(m_GpGrid, 0, wxEXPAND | wxALL, 4);
	Border->Add(m_GpTeamsSizer, 0,
	            wxALIGN_CENTER_HORIZONTAL | wxLEFT | wxRIGHT | wxBOTTOM, 4);
	m_PnlGameplayVars->SetSizer(Border);
}

void dlgServerDetails::DoQuery()
{
	wxFileConfig ConfigInfo;
	wxInt32 ServerTimeout, RetryCount;

	ConfigInfo.Read(SERVERTIMEOUT, &ServerTimeout, ODA_QRYSERVERTIMEOUT);
	ConfigInfo.Read(RETRYCOUNT, &RetryCount, ODA_QRYGSRETRYCOUNT);

	BufferedSocket Socket;
	m_Server.SetSocket(&Socket);
	m_Server.SetRetries(RetryCount);
	m_Server.Query(ServerTimeout);

	Populate();

	if(m_Server.GotResponse())
		m_Parent->ApplyServerRefresh(m_Server);
}

void dlgServerDetails::Populate()
{
	SetTitle(wxString::Format("Server Details - %s",
	                          stdstr_towxstr(m_Server.Info.Name)));

	Freeze();

	PopulateMetadata();
	PopulateGameplay();
	PopulateServerVars();
	PopulatePlayerList();

	m_PnlMetadata->Layout();
	m_PnlGameplayVars->Layout();
	Layout();

	Thaw();

	// Refresh the Join controls last; if an armed "join when slot free" wait is
	// now satisfied, act on it (which closes the dialog).
	if(UpdateJoinControls())
		DoJoin();
}

void dlgServerDetails::PopulateMetadata()
{
	const Server& s = m_Server;

	// MOTD: literal "\n" sequences become real newlines.
	wxString Motd;
	OdaGetCvarValue(s, "sv_motd", Motd);
	Motd.Replace("\\n", "\n");
	m_TxtMotd->SetValue(Motd);

	// Hide the whole "Message of the day" section when there's no MOTD.
	if(m_MotdSizer)
		GetSizer()->Show(m_MotdSizer, !Motd.IsEmpty(), true);

	m_MdName->SetLabel(stdstr_towxstr(s.Info.Name));
	m_MdVersion->SetLabel(OdaGetVersionString(s));
	m_MdAddress->SetLabel(stdstr_towxstr(s.GetAddress()));
	m_MdSkill->SetLabel(OdaGetSkillString(s));

	// Ping number + coloured bullet, using the same thresholds as the list.
	const wxUint64 Ping = s.GetPing();
	m_MdPing->SetLabel(wxString::Format("%llu", Ping));

	wxFileConfig ConfigInfo;
	wxInt32 PQGood, PQPlayable, PQLaggy;
	ConfigInfo.Read(ICONPINGQGOOD, &PQGood, ODA_UIPINGQUALITYGOOD);
	ConfigInfo.Read(ICONPINGQPLAYABLE, &PQPlayable, ODA_UIPINGQUALITYPLAYABLE);
	ConfigInfo.Read(ICONPINGQLAGGY, &PQLaggy, ODA_UIPINGQUALITYLAGGY);

	const char* Bullet = "bullet_gray";
	if(Ping < (wxUint64)PQGood)
		Bullet = "bullet_green";
	else if(Ping < (wxUint64)PQPlayable)
		Bullet = "bullet_orange";
	else if(Ping < (wxUint64)PQLaggy)
		Bullet = "bullet_red";

	m_PingIcon->SetBitmap(wxXmlResource::Get()->LoadBitmap(Bullet));

	m_MdMap->SetLabel(stdstr_towxstr(s.Info.CurrentMap).Upper());

	// IWAD is Wads[1]; PWADs are Wads[2..].
	const size_t WadCount = s.Info.Wads.size();
	wxString Iwad;
	if(WadCount > 1)
	{
		const std::string& Name = s.Info.Wads[1].Name;
		Iwad = stdstr_towxstr(Name.substr(0, Name.find('.')));
	}
	m_MdIwad->SetLabel(Iwad);

	wxString Pwads;
	for(size_t i = 2; i < WadCount; ++i)
	{
		const std::string& Name = s.Info.Wads[i].Name;
		if(!Pwads.IsEmpty())
			Pwads += "\n";
		Pwads += stdstr_towxstr(Name.substr(0, Name.find('.')));
	}
	SetOptionalRow(m_MetaGrid, m_MdPwadLabel, m_MdPwad, Pwads);

	// Download sites: a space-separated list, shown as one hyperlink per URL.
	wxString DownloadSites;
	OdaGetCvarValue(s, "sv_downloadsites", DownloadSites);
	DownloadSites.Trim(true).Trim(false);

	m_MdDownloadSizer->Clear(true); // destroy the previous link controls
	wxArrayString Sites = wxSplit(DownloadSites, ' ');
	for(size_t i = 0; i < Sites.GetCount(); ++i)
	{
		if(Sites[i].IsEmpty())
			continue;

		wxHyperlinkCtrl* Link = new wxHyperlinkCtrl(
		    m_PnlMetadata, wxID_ANY, Sites[i], Sites[i]);
		m_MdDownloadSizer->Add(Link, 0);
	}

	const bool ShowDownloads = m_MdDownloadSizer->GetItemCount() > 0;
	m_MetaGrid->Show(m_MdDownloadURILabel, ShowDownloads);
	m_MetaGrid->Show(m_MdDownloadSizer, ShowDownloads, true);

	wxString AdminEmail;
	OdaGetCvarValue(s, "sv_email", AdminEmail);
	const bool ShowEmail = !AdminEmail.IsEmpty();
	if(ShowEmail)
	{
		m_MdAdminEmail->SetLabel(AdminEmail);
		m_MdAdminEmail->SetURL("mailto:" + AdminEmail);
	}
	m_MetaGrid->Show(m_MdAdminEmailLabel, ShowEmail);
	m_MetaGrid->Show(m_MdAdminEmail, ShowEmail);

	const bool HasPassword = !s.Info.PasswordHash.empty();
	m_MdPassword->SetLabel(HasPassword ? "Yes" : "No");
	// Show the padlock icon (as used in the server list) next to a "Yes".
	m_PasswordIcon->SetBitmap(
	    HasPassword ? wxXmlResource::Get()->LoadBitmap("locked_server")
	                : wxNullBitmap);
	m_PasswordIcon->Show(HasPassword);
}

void dlgServerDetails::PopulateGameplay()
{
	const Server& s = m_Server;

	m_GpPlayers->SetLabel(OdaGetPlayerCountString(s));
	m_GpGameType->SetLabel(OdaGetGameTypeString(s));

	SetOptionalRow(m_GpGrid, m_GpFriendlyFireLabel, m_GpFriendlyFire,
	               OdaGetFriendlyFireString(s));

	const OdaModifier_t PlayerDmg =
	    OdaGetDamagePercent(s, "sv_weapondamage");
	SetColouredRow(m_GpGrid, m_GpPlayerDmgLabel, m_GpPlayerDmg, PlayerDmg.Value,
	               PlayerDmg.IsPositive);

	OdaModifier_t MonsterDmg;
	if(s.Info.GameType == GT_Horde || s.Info.GameType == GT_Cooperative)
		MonsterDmg = OdaGetDamagePercent(s, "sv_monsterdamage");
	SetColouredRow(m_GpGrid, m_GpMonsterDmgLabel, m_GpMonsterDmg,
	               MonsterDmg.Value, !MonsterDmg.IsPositive);

	const OdaModifier_t MonsterHealth =
	    OdaGetDamagePercent(s, "sv_monstershealth");
	SetColouredRow(m_GpGrid, m_GpMonsterHealthLabel, m_GpMonsterHealth,
	               MonsterHealth.Value, !MonsterHealth.IsPositive);

	const OdaModifier_t Gravity = OdaGetGravity(s);
	SetDeltaRow(m_GpGrid, m_GpGravityLabel, m_GpGravity, m_GpGravityDelta,
	            Gravity.Value, Gravity.Delta, Gravity.IsPositive,
	            Gravity.Default);

	const OdaModifier_t AirControl = OdaGetAirControl(s);
	SetDeltaRow(m_GpGrid, m_GpAirControlLabel, m_GpAirControl,
	            m_GpAirControlDelta, AirControl.Value, AirControl.Delta,
	            AirControl.IsPositive, AirControl.Default);

	wxString Waves;
	if(s.Info.GameType == GT_Horde)
		OdaGetCvarValue(s, "g_horde_waves", Waves);
	SetOptionalRow(m_GpGrid, m_GpWavesLabel, m_GpWaves, Waves);

	SetOptionalRow(m_GpGrid, m_GpCtfRulesLabel, m_GpCtfRules,
	               OdaGetCtfRulesString(s));

	wxString Lives;
	OdaGetCvarValue(s, "g_lives", Lives);
	SetOptionalRow(m_GpGrid, m_GpLivesLabel, m_GpLives, Lives);

	SetOptionalRow(m_GpGrid, m_GpFastMonstersLabel, m_GpFastMonsters,
	               OdaGetFastMonstersString(s));

	SetOptionalRow(m_GpGrid, m_GpRoundsLabel, m_GpRounds, OdaGetRoundsString(s));
	SetOptionalRow(m_GpGrid, m_GpTimeLeftLabel, m_GpTimeLeft,
	               OdaGetTimeLeftString(s));
	SetOptionalRow(m_GpGrid, m_GpScoreLabel, m_GpScore, OdaGetScoreString(s));


	const bool ShowTeams =
	    OdaBuildTeamScoreBoxes(m_PnlGameplayVars, m_GpTeamsSizer, s);
	m_PnlGameplayVars->GetSizer()->Show(m_GpTeamsSizer, ShowTeams, true);
}

void dlgServerDetails::PopulateServerVars()
{
	const Server& s = m_Server;

	m_PnlServerVars->Hide();

	wxSizer* Outer = m_PnlServerVars->GetSizer();
	Outer->Clear(true); // destroys the previous panes (and their rows)
	m_PaneRows.clear();
	m_BuiltPanes.clear();

	// Cvar tracker (so we can tell which ones are uncategorized)
	std::set<std::string> Shown;

	// Resolves a cvar name to its display value. Returns false when the server
	// didn't transmit the cvar.
	// IsBool won't show a value (it's existance is enough)
	auto ResolveVar = [&](const std::string& Name, wxString& Value,
	                      bool& IsBool) -> bool
	{
		const Cvar_t* Cvar = OdaFindCvar(s, Name);
		if(!Cvar)
			return false;

		IsBool = (Cvar->Type == CVARTYPE_BOOL);
		Value.Clear();
		if(!IsBool)
			OdaGetCvarValue(s, Name, Value);

		return true;
	};

	// Creates a collapsible pane for the named cvars the server exposes, but
	// defers building the actual row controls until the pane is first expanded
	// (see BuildPaneContent) so the dialog opens quickly. Returns false (and
	// adds nothing) if none are present.
	auto AddPane = [&](const wxString& Title,
	                   const std::vector<std::string>& Names) -> bool
	{
		std::vector<std::pair<std::string, wxString> > Rows;

		for(size_t i = 0; i < Names.size(); ++i)
		{
			if(Shown.count(Names[i]))
				continue;

			wxString Value;
			bool IsBool = false;
			if(ResolveVar(Names[i], Value, IsBool))
			{
				Rows.push_back(std::make_pair(Names[i], Value));
				Shown.insert(Names[i]);
			}
		}

		if(Rows.empty())
			return false;

		// Show the cvars within the pane in alphabetical order.
		std::sort(Rows.begin(), Rows.end(),
		          [](const std::pair<std::string, wxString>& A,
		             const std::pair<std::string, wxString>& B)
		          {
		              return A.first < B.first;
		          });

		wxCollapsiblePane* Pane =
		    new wxCollapsiblePane(m_PnlServerVars, wxID_ANY, Title,
		                          wxDefaultPosition, wxDefaultSize,
		                          wxCP_NO_TLW_RESIZE);

		Pane->SetDoubleBuffered(true);

		m_PaneRows[Pane] = Rows;
		Outer->Add(Pane, 0, wxGROW | wxALL, 2);

		// Restore (and eagerly build) panes the user had expanded before.
		if(m_ExpandedCategories.count(Title))
		{
			Pane->Freeze();
			BuildPaneContent(Pane);
			Pane->Expand();
			Pane->Thaw();
		}

		return true;
	};

	// Display the categories alphabetically ("Uncategorized" is appended last,
	// after this loop, so it always sorts to the bottom regardless).
	CvarCategories Categories = GetCvarCategories();
	std::sort(Categories.begin(), Categories.end(),
	          [](const CvarCategories::value_type& A,
	             const CvarCategories::value_type& B)
	          {
	              return A.first < B.first;
	          });

	for(size_t i = 0; i < Categories.size(); ++i)
	{
		const wxString& Name = Categories[i].first;

		// Mode-specific categories only make sense for their game mode.
		if(Name == "Horde" && s.Info.GameType != GT_Horde)
			continue;
		if(Name == "CTF" && s.Info.GameType != GT_CaptureTheFlag)
			continue;

		AddPane(Name, Categories[i].second);
	}

	// Anything the server sent that belongs to no category at all. We test
	// against every category's cvar list (not just the panes we actually
	// showed) so cvars whose category was hidden by the mode gate above don't
	// wrongly fall through to "Uncategorized".
	std::set<std::string> Categorized;
	for(size_t i = 0; i < Categories.size(); ++i)
		Categorized.insert(Categories[i].second.begin(),
		                   Categories[i].second.end());

	std::vector<std::string> Leftover;
	for(size_t i = 0; i < s.Info.Cvars.size(); ++i)
	{
		const std::string& Name = s.Info.Cvars[i].Name;
		if(!Categorized.count(Name))
			Leftover.push_back(Name);
	}
	AddPane("Uncategorized", Leftover);

	m_PnlServerVars->Show();
	m_PnlServerVars->FitInside();
	m_PnlServerVars->Layout();
}

// Builds the min/max/default cell for a documented cvar: up to three
// underlined values (separated by " / "), each with its own "X value for
// {cvar}" tooltip and a help cursor. Returns an empty sizer when the doc
// carries none of the three.
static wxSizer* BuildCvarBoundsCell(wxWindow* Parent, const std::string& Name,
                                    const CvarDoc_t& Doc)
{
	static const wxCursor HelpCursor(wxCURSOR_QUESTION_ARROW);

	wxFont DocFont = wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT);
	DocFont.SetUnderlined(true);

	const wxString CvarName = stdstr_towxstr(Name);
	wxBoxSizer* Cell = new wxBoxSizer(wxHORIZONTAL);

	auto AddPiece = [&](const wxString& Text, const wxString& Tip)
	{
		if(Cell->GetItemCount() > 0)
			Cell->Add(new wxStaticText(Parent, wxID_ANY, " / "), 0,
			          wxALIGN_CENTER_VERTICAL);

		wxStaticText* Piece = new wxStaticText(Parent, wxID_ANY, Text);
		Piece->SetFont(DocFont);
		Piece->SetToolTip(Tip);
		Piece->SetCursor(HelpCursor);

		Cell->Add(Piece, 0, wxALIGN_CENTER_VERTICAL);
	};

	if(Doc.HasMin)
		AddPiece(wxString::Format("%g", Doc.Min),
		         wxString::Format("Minimum value for %s", CvarName));
	if(Doc.HasMax)
		AddPiece(wxString::Format("%g", Doc.Max),
		         wxString::Format("Maximum value for %s", CvarName));
	if(!Doc.DefaultValue.empty())
		AddPiece(stdstr_towxstr(Doc.DefaultValue),
		         wxString::Format("Default value for %s", CvarName));

	if(Cell->GetItemCount() > 0)
	{
		Cell->Insert(0, new wxStaticText(Parent, wxID_ANY, "("), 0,
		             wxALIGN_CENTER_VERTICAL);
		Cell->Add(new wxStaticText(Parent, wxID_ANY, ")"), 0,
		          wxALIGN_CENTER_VERTICAL);
	}

	return Cell;
}

void dlgServerDetails::BuildPaneContent(wxCollapsiblePane* Pane)
{
	if(!Pane || m_BuiltPanes.count(Pane))
		return;

	m_BuiltPanes.insert(Pane);

	std::map<wxCollapsiblePane*,
	         std::vector<std::pair<std::string, wxString> > >::const_iterator it =
	    m_PaneRows.find(Pane);
	if(it == m_PaneRows.end())
		return;

	wxFont DocFont = wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT);
	DocFont.SetUnderlined(true);

	// Constructed once and reused for every documented row.
	static const wxCursor HelpCursor(wxCURSOR_QUESTION_ARROW);

	CvarDocDb& Docs = GetCvarDb();
	wxWindow* PaneWin = Pane->GetPane();

	PaneWin->Freeze();

	wxFlexGridSizer* Grid = new wxFlexGridSizer(0, 3, 2, 12);
	const std::vector<std::pair<std::string, wxString> >& Rows = it->second;

	for(size_t i = 0; i < Rows.size(); ++i)
	{
		const std::string& Name = Rows[i].first;

		wxStaticText* NameCtrl =
		    new wxStaticText(PaneWin, wxID_ANY, stdstr_towxstr(Name));

		const CvarDoc_t* Doc = Docs.Find(Name);
		if(Doc)
		{
			NameCtrl->SetFont(DocFont);
			NameCtrl->SetToolTip(wxString::FromUTF8(Doc->HelpText.c_str()));
			// A help cursor signals that hovering reveals documentation.
			NameCtrl->SetCursor(HelpCursor);
		}

		wxStaticText* ValueCtrl =
		    new wxStaticText(PaneWin, wxID_ANY, Rows[i].second);

		Grid->Add(NameCtrl, 0, wxALIGN_CENTER_VERTICAL);
		Grid->Add(ValueCtrl, 0, wxALIGN_CENTER_VERTICAL);

		// An empty value means a boolean flag (no value/bounds shown).
		// We also don't want to display if it's a string, either.
		const bool IsBool = Rows[i].second.IsEmpty();
		if(Doc && !IsBool && Doc->Type != "string")
			Grid->Add(BuildCvarBoundsCell(PaneWin, Name, *Doc), 0,
			          wxALIGN_CENTER_VERTICAL);
		else
			Grid->AddSpacer(0);
	}

	wxBoxSizer* PaneSizer = new wxBoxSizer(wxVERTICAL);
	PaneSizer->Add(Grid, 0, wxALL, 4);
	PaneWin->SetSizer(PaneSizer);
	PaneWin->Thaw();
}

void dlgServerDetails::PopulatePlayerList()
{
	m_PlayerList->AddPlayersAndSlotsToList(m_Server);
}

void dlgServerDetails::OnRefresh(wxCommandEvent& WXUNUSED(event))
{
	m_GaugeTicks = 0;
	m_Gauge->SetValue(0);

	if(m_BtnRefresh->GetValue())
	{
		// Toggled on: refresh immediately, then run the auto-refresh cycle.
		DoQuery();
		m_Timer.Start(REFRESH_TICK_MS);
	}
	else
	{
		// Toggled off: stop the cycle and revert the Join controls.
		m_Timer.Stop();
		UpdateJoinControls();
	}
}

void dlgServerDetails::OnJoin(wxCommandEvent& WXUNUSED(event))
{
	DoJoin();
}

void dlgServerDetails::DoJoin()
{
	m_Timer.Stop();
	m_Parent->ConnectToServer(m_Server, this);
	Close();
}

bool dlgServerDetails::UpdateJoinControls()
{
	const bool Responded = m_Server.GotResponse();
	const bool Full = (int)m_Server.Info.Players.size() >=
	                  (int)m_Server.Info.MaxClients;
	const bool Refreshing = m_BtnRefresh->GetValue();

	bool AutoJoin = false;
	bool ShowWaitToggle = false;

	if(Responded && !Full)
	{
		// A slot is open: honour an armed wait, otherwise offer a normal Join.
		if(m_BtnJoinWhenFree->IsShown() && m_BtnJoinWhenFree->GetValue())
			AutoJoin = true;
	}
	else if(Responded && Full && Refreshing)
	{
		// Full but actively refreshing: let the user arm a wait-for-slot join.
		ShowWaitToggle = true;
	}

	m_BtnJoin->Show(!ShowWaitToggle);
	m_BtnJoin->Enable(Responded && !Full);

	m_BtnJoinWhenFree->Show(ShowWaitToggle);
	if(!ShowWaitToggle)
		m_BtnJoinWhenFree->SetValue(false);

	Layout();

	return AutoJoin;
}

void dlgServerDetails::OnTimer(wxTimerEvent& WXUNUSED(event))
{
	if(++m_GaugeTicks >= REFRESH_TICKS)
	{
		m_GaugeTicks = 0;
		m_Gauge->SetValue(0);
		DoQuery();
		return;
	}

	m_Gauge->SetValue(m_GaugeTicks);
}

void dlgServerDetails::OnClose(wxCloseEvent& event)
{
	m_Timer.Stop();

	// Forget which panes were expanded so the next open starts fully collapsed
	// (an open pane would otherwise be eagerly rebuilt, slowing the reopen).
	m_ExpandedCategories.clear();

	event.Skip();
}
