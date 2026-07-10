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

#include "srv_utils.h"

#include <algorithm>
#include <cmath>
#include <vector>

#include <wx/cursor.h>
#include <wx/settings.h>
#include <wx/sizer.h>
#include <wx/stattext.h>

#include "str_utils.h"

using namespace odalpapi;

wxString OdaGetGameTypeString(const Server& s)
{
	if(s.Info.GameType == GT_Cooperative && s.Info.Lives)
		return "Survival";
	else if (s.Info.GameType == GT_Cooperative && s.Info.MaxPlayers <= 1)
		return "Single-player";
	else if(s.Info.GameType == GT_Cooperative)
		return "Cooperative";
	else if(s.Info.GameType == GT_Deathmatch && s.Info.Lives)
		return "Last Marine Standing";
	else if(s.Info.GameType == GT_Deathmatch && s.Info.MaxPlayers <= 2)
		return "Duel";
	else if(s.Info.GameType == GT_Deathmatch)
		return "Deathmatch";
	else if(s.Info.GameType == GT_TeamDeathmatch && s.Info.Lives)
		return "Team Last Marine Standing";
	else if(s.Info.GameType == GT_TeamDeathmatch)
		return "Team Deathmatch";
	else if(s.Info.GameType == GT_CaptureTheFlag && s.Info.Sides)
		return "Attack & Defend CTF";
	else if(s.Info.GameType == GT_CaptureTheFlag && s.Info.Lives)
		return "LMS Capture The Flag";
	else if(s.Info.GameType == GT_CaptureTheFlag)
		return "Capture The Flag";
	else if(s.Info.GameType == GT_Horde && s.Info.Lives)
		return "Survival Horde";
	else if(s.Info.GameType == GT_Horde)
		return "Horde";

	return "Unknown";
}

const Cvar_t* OdaFindCvar(const Server& s, const std::string& Name)
{
	for(size_t i = 0; i < s.Info.Cvars.size(); ++i)
	{
		if(s.Info.Cvars[i].Name == Name)
			return &s.Info.Cvars[i];
	}

	return NULL;
}

bool OdaGetCvarValue(const Server& s, const std::string& Name, wxString& Out)
{
	const Cvar_t* Cvar = OdaFindCvar(s, Name);

	if(!Cvar)
		return false;

	switch(Cvar->Type)
	{
	case CVARTYPE_BYTE:
		Out = wxString::Format("%d", Cvar->i8);
		break;
	case CVARTYPE_WORD:
		Out = wxString::Format("%d", Cvar->i16);
		break;
	case CVARTYPE_INT:
		Out = wxString::Format("%d", Cvar->i32);
		break;
	case CVARTYPE_FLOAT:
	case CVARTYPE_STRING:
	default:
		Out = stdstr_towxstr(Cvar->Value);
		break;
	}

	return true;
}

int OdaGetCvarInt(const Server& s, const std::string& Name, int Default)
{
	wxString Value;
	long Result = 0;

	if(OdaGetCvarValue(s, Name, Value) && Value.ToLong(&Result))
		return (int)Result;

	return Default;
}

wxString OdaGetSkillString(const Server& s)
{
	wxString Value;

	if(!OdaGetCvarValue(s, "sv_skill", Value))
		return "Unknown";

	// sv_skill is 1-indexed (1-5) and mirrors the classic Doom skill levels.
	long Skill = 0;

	if(Value.ToLong(&Skill))
	{
		switch(Skill)
		{
		case 1:
			return "I'm Too Young To Die";
		case 2:
			return "Hey, Not Too Rough";
		case 3:
			return "Hurt Me Plenty";
		case 4:
			return "Ultra-Violence";
		case 5:
			return "Nightmare!";
		default:
			break;
		}
	}

	// custom skill defined in the WAD
	// just show what the server reported
	return Value;
}

wxString OdaGetTimeString(int Seconds)
{
	if(Seconds <= 0)
		return wxEmptyString;

	const int Hours = Seconds / 3600;
	const int Minutes = (Seconds % 3600) / 60;
	const int Secs = Seconds % 60;

	wxString Result;

	// Appends "N unit(s)" for a non-zero component, comma-separating as needed
	auto Append = [&Result](int Value, const char* Unit)
	{
		if(Value <= 0)
			return;

		if(!Result.IsEmpty())
			Result += ", ";

		Result += wxString::Format("%d %s%s", Value, Unit,
		                           Value == 1 ? "" : "s");
	};

	Append(Hours, "hour");
	Append(Minutes, "minute");
	Append(Secs, "second");

	return Result;
}

wxString OdaGetVersionString(const Server& s)
{
	wxString Revision;

	if(!s.Info.VersionRevStr.empty())
		Revision = wxString::Format(" (%s)", s.Info.VersionRevStr);
	else if(s.Info.VersionRevision != 0)
		Revision = wxString::Format(" (r%u)", s.Info.VersionRevision);

	return wxString::Format("%u.%u.%u%s", s.Info.VersionMajor,
	                        s.Info.VersionMinor, s.Info.VersionPatch, Revision);
}

wxString OdaGetPlayerCountString(const Server& s)
{
	const int Clients = (int)s.Info.Players.size();
	const int PlayerCount = (int)std::count_if(
	    s.Info.Players.begin(), s.Info.Players.end(),
	    [](const Player_t& p) { return !p.Spectator; });
	const int CanJoin = s.Info.MaxPlayers > 0
	                        ? (int)s.Info.MaxPlayers - PlayerCount
	                        : (int)s.Info.MaxClients - PlayerCount;

	return wxString::Format("%d / %d clients, %d player%s can join", Clients,
	                        (int)s.Info.MaxClients, CanJoin,
	                        CanJoin == 1 ? "" : "s");
}

wxString OdaGetFriendlyFireString(const Server& s)
{
	wxString Raw;

	if(OdaGetCvarValue(s, "sv_friendlyfire", Raw) &&
	   (s.Info.GameType == GT_TeamDeathmatch ||
	    s.Info.GameType == GT_CaptureTheFlag ||
	    s.Info.GameType == GT_Horde || s.Info.GameType == GT_Cooperative))
		return "Yes";

	return wxEmptyString;
}

OdaModifier_t OdaGetDamagePercent(const Server& s, const std::string& Cvar)
{
	OdaModifier_t DmgPercent;

	wxString Value;
	double Factor = 1.0;

	if(OdaGetCvarValue(s, Cvar, Value) && Value.ToDouble(&Factor) &&
	   Factor != 1.0)
	{
		DmgPercent.Value = wxString::Format("%g%%", Factor * 100.0);
		DmgPercent.IsPositive = (Factor > 1.0);
	}

	return DmgPercent;
}

wxString OdaGetRoundsString(const Server& s)
{
	wxString Enabled;

	if(!OdaGetCvarValue(s, "g_rounds", Enabled))
		return wxEmptyString;

	const int RoundLimit = OdaGetCvarInt(s, "g_roundlimit", 0);
	const int WinLimit = OdaGetCvarInt(s, "g_winlimit", 0);

	if(RoundLimit == 0 && WinLimit == 0)
		return "Unlimited";

	wxString Rounds;

	if(RoundLimit > 0)
	{
		Rounds = wxString::Format("%d round limit", RoundLimit);

		if(WinLimit > 0 && RoundLimit > WinLimit)
			Rounds += wxString::Format(", first to %d win%s", WinLimit,
			                           WinLimit == 1 ? "" : "s");
	}

	return Rounds;
}

wxString OdaGetTimeLeftString(const Server& s)
{
	if(s.Info.TimeLimit)
		return OdaGetTimeString(s.Info.TimeLeft * 60);

	return wxEmptyString;
}

wxString OdaGetScoreString(const Server& s)
{
	int HighestScore = 0;

	if(s.Info.GameType == GT_TeamDeathmatch ||
	   s.Info.GameType == GT_CaptureTheFlag)
	{
		if(!s.Info.ScoreLimit)
			return wxEmptyString;

		auto WinningTeam = std::max_element(
		    s.Info.Teams.begin(), s.Info.Teams.end(),
		    [](const Team_t& a, const Team_t& b) { return a.Score < b.Score; });

		if(WinningTeam != s.Info.Teams.end())
			HighestScore = WinningTeam->Score;

		return wxString::Format("%d / %u", HighestScore, s.Info.ScoreLimit);
	}
	else if(s.Info.GameType == GT_Deathmatch)
	{
		if(!s.Info.FragLimit)
			return wxEmptyString;

		auto WinningPlayer = std::max_element(
		    s.Info.Players.begin(), s.Info.Players.end(),
		    [](const Player_t& a, const Player_t& b) { return a.Frags < b.Frags; });

		if(WinningPlayer != s.Info.Players.end())
			HighestScore = WinningPlayer->Frags;

		return wxString::Format("%d / %u", HighestScore, s.Info.FragLimit);
	}

	return wxEmptyString;
}

OdaModifier_t OdaGetGravity(const Server& s)
{
	constexpr long Default = 800;

	OdaModifier_t Gravity;
	Gravity.Default = "800";

	wxString Value;

	if(!OdaGetCvarValue(s, "sv_gravity", Value))
		return Gravity;

	long GravityValue = 0;
	if (!Value.ToLong(&GravityValue))
	{
		Gravity.Value = Value;
		return Gravity;
	}

	// The default isn't worth showing.
	if(GravityValue == Default)
		return Gravity;

	const double Diff = (double)(GravityValue - Default) / (double)Default;

	Gravity.Value = Value;
	Gravity.Delta = wxString::Format("(%+.4g%%)", Diff);
	Gravity.IsPositive = (Diff > 0.0);

	return Gravity;
}

OdaModifier_t OdaGetAirControl(const Server& s)
{
	constexpr double Default = 0.00390625; // 1/256

	OdaModifier_t AirControl;
	AirControl.Default = "0.00390625";

	wxString Value;

	if(!OdaGetCvarValue(s, "sv_aircontrol", Value))
		return AirControl;

	double AirControlValue = 0.0;
	if(!Value.ToDouble(&AirControlValue))
	{
		AirControl.Value = Value;
		return AirControl;
	}

	// The default isn't worth showing.
	if (AirControlValue == Default)
		return AirControl;

	const double Diff = (AirControlValue - Default) / Default;

	AirControl.Value = Value;
	AirControl.Delta = wxString::Format("(%+.4g%%)", Diff);
	AirControl.IsPositive = (Diff > 0.0);

	return AirControl;
}

void OdaApplyDeltaColour(wxStaticText* Ctrl, bool IsPositive)
{
	// Choose shades legible on the control's background: brighter on a dark
	// background, deeper on a light one.
	wxColour Bg = Ctrl->GetParent()
	                  ? Ctrl->GetParent()->GetBackgroundColour()
	                  : wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW);
	const double Luma =
	    Bg.Red() * 0.299 + Bg.Green() * 0.587 + Bg.Blue() * 0.114;
	const bool Dark = Luma < 128.0;

	const wxColour Green = Dark ? wxColour(0x5C, 0xD6, 0x5C)
	                            : wxColour(0x00, 0x80, 0x00);
	const wxColour Red = Dark ? wxColour(0xFF, 0x6B, 0x6B)
	                          : wxColour(0xC0, 0x00, 0x00);

	Ctrl->SetForegroundColour(IsPositive ? Green : Red);
}

void OdaApplyDefaultHint(wxStaticText* Ctrl, const wxString& DefaultText)
{
	if(DefaultText.IsEmpty())
		return;

	wxFont Font = Ctrl->GetFont();
	Font.SetUnderlined(true);
	Ctrl->SetFont(Font);

	Ctrl->SetToolTip(wxString::Format("Default is %s", DefaultText));

	// A help cursor signals that hovering reveals more information
	static const wxCursor HelpCursor(wxCURSOR_QUESTION_ARROW);
	Ctrl->SetCursor(HelpCursor);
}

wxString OdaGetFastMonstersString(const Server& s)
{
	// Nightmare (skill 5) always runs fast monsters, so don't bother showing it.
	if(OdaGetCvarInt(s, "sv_skill", 0) >= 5)
		return wxEmptyString;

	// sv_fastmonsters is transmitted only when enabled.
	if(OdaFindCvar(s, "sv_fastmonsters"))
		return "Yes";

	return wxEmptyString;
}

wxString OdaGetCtfRulesString(const Server& s)
{
	if(s.Info.GameType != GT_CaptureTheFlag)
		return wxEmptyString;

	std::vector<wxString> Rules;

	const bool NoTouchReturn = (OdaGetCvarInt(s, "g_ctf_notouchreturn", 0) == 1);

	if(!OdaFindCvar(s, "ctf_flagathometoscore"))
		Rules.push_back("flag always scores");

	if(!NoTouchReturn && OdaFindCvar(s, "ctf_manualreturn"))
		Rules.push_back("manual return");

	if(NoTouchReturn)
		Rules.push_back("flag timeout returns only");

	if(Rules.empty())
		return wxEmptyString;

	wxString Result;

	for(size_t i = 0; i < Rules.size(); ++i)
	{
		if(i > 0)
			Result += ", ";

		Result += Rules[i];
	}

	// Capitalize only the first letter
	return Result.Left(1).Upper() + Result.Mid(1);
}

bool OdaBuildTeamScoreBoxes(wxWindow* Parent, wxSizer* Target, const Server& s)
{
	Target->Clear(true);

	const bool ShowTeams =
	    (s.Info.GameType == GT_TeamDeathmatch ||
	     s.Info.GameType == GT_CaptureTheFlag) && !s.Info.Teams.empty();

	if(!ShowTeams)
		return false;

	const wxFont TeamFont =
	    wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT).Bold();

	for(size_t i = 0; i < s.Info.Teams.size(); ++i)
	{
		const Team_t& Team = s.Info.Teams[i];

		// " - " separator between teams
		if(i > 0)
			Target->Add(new wxStaticText(Parent, wxID_ANY, " - "), 0,
			            wxALIGN_CENTER_VERTICAL);

		const wxColour TeamColour((Team.Colour >> 16) & 0xFF,
		                          (Team.Colour >> 8) & 0xFF,
		                          Team.Colour & 0xFF);

		wxStaticText* Box = new wxStaticText(
		    Parent, wxID_ANY,
		    wxString::Format(" %s: %d ", stdstr_towxstr(Team.Name),
		                     (int)Team.Score));

		Box->SetForegroundColour(*wxWHITE);
		Box->SetBackgroundColour(TeamColour);
		Box->SetFont(TeamFont);

		Target->Add(Box, 0, wxALIGN_CENTER_VERTICAL);
	}

	return true;
}
