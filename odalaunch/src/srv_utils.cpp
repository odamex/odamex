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
