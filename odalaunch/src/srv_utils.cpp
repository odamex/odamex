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
#include <vector>

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

wxString OdaGetDamagePercentString(const Server& s, const std::string& Cvar)
{
	wxString Value;
	double Factor = 1.0;

	if(OdaGetCvarValue(s, Cvar, Value) && Value.ToDouble(&Factor) &&
	   Factor != 1.0)
		return wxString::Format("%g%%", Factor * 100.0);

	return wxEmptyString;
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
