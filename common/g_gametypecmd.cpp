// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2006-2020 by The Odamex Team.
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
//   Built-in gametype commands.
//
//-----------------------------------------------------------------------------

#include "c_dispatch.h"
#include "cmdlib.h"
#include "hashtable.h"

typedef std::vector<std::string> StringList;

struct GametypeParam
{
	const char* cvar;
	const int def;
	const char* flag;
	const char* flagparam;
	const char* help;
};

/**
 * @brief Generate flag help for gametype functions.
 *
 * @param params Array of parameters to examine.
 */
template <size_t SIZE> static void GametypeHelp(const GametypeParam (&params)[SIZE])
{
	Printf("Flags:\n");
	Printf("  default\n");
	Printf("    Use default settings.\n");
	for (size_t i = 0; i < SIZE; i++)
	{
		Printf("  %s <%s>\n", params[i].flag, params[i].flagparam);
		Printf("    %s\n", params[i].help);
	}
}

/**
 * @brief Parse arguments for a gametype.
 *
 * @param params Gametype arguments.
 * @param argc Argument count.
 * @param argv Arguments pointer.
 * @return List of console commands to execute.
 */
template <size_t SIZE>
static StringList GametypeArgs(const GametypeParam (&params)[SIZE], size_t argc,
                               char** argv)
{
	StringList ret;

	// Insert defaults into a hashtable.
	typedef OHashTable<std::string, int> CvarTable;
	CvarTable cvars;
	for (size_t i = 0; i < SIZE; i++)
	{
		cvars.insert(std::make_pair(params[i].cvar, params[i].def));
	}

	StringList args = VectorArgs(argc, argv);
	if (!(!args.empty() && iequals(args.at(0).c_str(), "default")))
	{
		for (;;)
		{
			if (args.empty())
			{
				// Ran out of parameters to munch.
				break;
			}
			else if (args.size() == 1)
			{
				// All params take a second one.
				Printf(PRINT_HIGH, "Missing argument for \"%s\"\n", args.front().c_str());
				return ret;
			}

			const std::string& cmd = args.at(0);
			const int val = atoi(args.at(1).c_str());

			bool next = false;
			for (size_t i = 0; i < SIZE; i++)
			{
				if (iequals(cmd.c_str(), params[i].flag))
				{
					// Set a new value.
					cvars.insert(std::make_pair(std::string(params[i].cvar), val));

					// Shift param and argument off the front.
					args.erase(args.begin(), args.begin() + 2);

					// Next flag.
					next = true;
					break;
				}
			}

			if (next)
			{
				// Next flag.
				continue;
			}

			// Unknown flag.
			Printf(PRINT_HIGH, "Unknown flag \"%s\"\n", cmd.c_str());
			return ret;
		}
	}

	// Apply all the flags
	std::string buffer;
	for (CvarTable::const_iterator it = cvars.begin(); it != cvars.end(); ++it)
	{
		StrFormat(buffer, "%s %d", it->first.c_str(), it->second);
		ret.push_back(buffer);
	}

	// We're done!
	return ret;
}

/*
 * Actual commands start below.
 */

static GametypeParam coopParams[] = {
    {"sv_skill", 4, "skill", "SKILL",
     "Set the skill of the game to SKILL.  Defaults to 4."}};

static void CoopHelp()
{
	Printf("game_coop - Configures some settings for a basic Cooperative game\n");
	GametypeHelp(::coopParams);
}

BEGIN_COMMAND(game_coop)
{
	if (argc < 2)
	{
		CoopHelp();
		return;
	}

	std::string buffer;
	StringList params = GametypeArgs(::coopParams, argc, argv);
	if (params.empty())
	{
		CoopHelp();
		return;
	}

	params.push_back("g_lives 0");
	params.push_back("g_lives_jointimer 30");
	params.push_back("g_rounds 0");
	params.push_back("sv_forcerespawn 0");
	params.push_back("sv_gametype 0");
	params.push_back("sv_nomonsters 0");

	std::string config = JoinStrings(params, "; ");
	Printf("Configuring Cooperative...\n%s\n", config.c_str());
	AddCommandString(config.c_str());
}
END_COMMAND(game_coop)

static GametypeParam survivalParams[] = {
    {"g_lives", 3, "lives", "LIVES",
     "Set number of player lives to LIVES.  Defaults to 3."},
    {"sv_skill", 4, "skill", "SKILL",
     "Set the skill of the game to SKILL.  Defaults to 4."}};

static void SurvivalHelp()
{
	Printf("game_survival - Configures some settings for a basic game of Survival\n");
	GametypeHelp(::survivalParams);
}

BEGIN_COMMAND(game_survival)
{
	if (argc < 2)
	{
		SurvivalHelp();
		return;
	}

	std::string buffer;
	StringList params = GametypeArgs(::survivalParams, argc, argv);
	if (params.empty())
	{
		SurvivalHelp();
		return;
	}

	params.push_back("g_lives_jointimer 30");
	params.push_back("g_rounds 0");
	params.push_back("sv_forcerespawn 1");
	params.push_back("sv_gametype 0");
	params.push_back("sv_nomonsters 0");

	std::string config = JoinStrings(params, "; ");
	Printf("Configuring Survival...\n%s\n", config.c_str());
	AddCommandString(config.c_str());
}
END_COMMAND(game_survival)

static GametypeParam dmParams[] = {
    {"sv_fraglimit", 30, "frags", "FRAGLIMIT",
     "Set number of frags needed to win to FRAGLIMIT.  Defaults to 30."},
    {"sv_timelimit", 10, "time", "TIMELIMIT",
     "Set number of minutes until the match ends to TIMELIMIT.  Defaults to 10."}};

static void DMHelp()
{
	Printf("game_dm - Configures some settings for a basic game of Deathmatch\n");
	GametypeHelp(::dmParams);
}

BEGIN_COMMAND(game_dm)
{
	if (argc < 2)
	{
		DMHelp();
		return;
	}

	std::string buffer;
	StringList params = GametypeArgs(::dmParams, argc, argv);
	if (params.empty())
	{
		DMHelp();
		return;
	}

	params.push_back("g_lives 0");
	params.push_back("g_rounds 0");
	params.push_back("sv_forcerespawn 0");
	params.push_back("sv_gametype 1");
	params.push_back("sv_nomonsters 1");
	params.push_back("sv_skill 5");

	std::string config = JoinStrings(params, "; ");
	Printf("Configuring Deathmatch...\n%s\n", config.c_str());
	AddCommandString(config.c_str());
}
END_COMMAND(game_dm)

static GametypeParam duelParams[] = {
    {"sv_fraglimit", 50, "frags", "FRAGLIMIT",
     "Set number of frags needed to win to FRAGLIMIT.  Defaults to 50."}};

static void DuelHelp()
{
	Printf("game_duel - Configures some settings for a basic Duel\n");
	GametypeHelp(::duelParams);
}

BEGIN_COMMAND(game_duel)
{
	if (argc < 2)
	{
		DuelHelp();
		return;
	}

	std::string buffer;
	StringList params = GametypeArgs(::dmParams, argc, argv);
	if (params.empty())
	{
		DuelHelp();
		return;
	}

	params.push_back("g_lives 0");
	params.push_back("g_rounds 0");
	params.push_back("g_winnerstays 1");
	params.push_back("sv_forcerespawn 1");
	params.push_back("sv_forcerespawntime 10");
	params.push_back("sv_gametype 1");
	params.push_back("sv_maxplayers 2");
	params.push_back("sv_nomonsters 1");
	params.push_back("sv_skill 5");
	params.push_back("sv_warmup 1");
	params.push_back("sv_warmup_autostart 1.0");

	std::string config = JoinStrings(params, "; ");
	Printf("Configuring Duel...\n%s\n", config.c_str());
	AddCommandString(config.c_str());
}
END_COMMAND(game_duel)

static GametypeParam lmsParams[] = {
    {"g_lives", 1, "lives", "LIVES",
     "Set number of player lives per round to LIVES.  Defaults to 1."},
    {"g_winlimit", 5, "wins", "WINS",
     "Set number of round wins needed to win the game to WINS.  Defaults to 5."}};

static void LMSHelp()
{
	Printf(
	    "game_lms - Configures some settings for a basic game of Last Marine Standing\n");
	GametypeHelp(::lmsParams);
}

BEGIN_COMMAND(game_lms)
{
	if (argc < 2)
	{
		LMSHelp();
		return;
	}

	std::string buffer;
	StringList params = GametypeArgs(::lmsParams, argc, argv);
	if (params.empty())
	{
		LMSHelp();
		return;
	}

	params.push_back("g_lives_jointimer 0");
	params.push_back("g_rounds 1");
	params.push_back("sv_forcerespawn 1");
	params.push_back("sv_gametype 1");
	params.push_back("sv_nomonsters 1");
	params.push_back("sv_skill 5");

	std::string config = JoinStrings(params, "; ");
	Printf("Configuring Last Marine Standing...\n%s\n", config.c_str());
	AddCommandString(config.c_str());
}
END_COMMAND(game_lms)

static GametypeParam tdmParams[] = {
    {"sv_fraglimit", 50, "frags", "FRAGLIMIT",
     "Set number of frags needed to win to FRAGLIMIT.  Defaults to 30."},
    {"sv_timelimit", 10, "time", "TIMELIMIT",
     "Set number of minutes until the match ends to TIMELIMIT.  Defaults to 10."}};

static void TDMHelp()
{
	Printf("game_tdm - Configures some settings for a basic game of Team Deathmatch\n");
	GametypeHelp(::tdmParams);
}

BEGIN_COMMAND(game_tdm)
{
	if (argc < 2)
	{
		TDMHelp();
		return;
	}

	std::string buffer;
	StringList params = GametypeArgs(::tdmParams, argc, argv);
	if (params.empty())
	{
		TDMHelp();
		return;
	}

	params.push_back("g_lives 0");
	params.push_back("g_rounds 0");
	params.push_back("sv_forcerespawn 0");
	params.push_back("sv_friendlyfire 0");
	params.push_back("sv_gametype 2");
	params.push_back("sv_nomonsters 1");
	params.push_back("sv_skill 5");

	std::string config = JoinStrings(params, "; ");
	Printf("Configuring Team Deathmatch...\n%s\n", config.c_str());
	AddCommandString(config.c_str());
}
END_COMMAND(game_tdm)

static GametypeParam tlmsParams[] = {
    {"g_lives", 1, "lives", "LIVES",
     "Set number of player lives per round to LIVES.  Defaults to 1."},
    {"sv_teamsinplay", 2, "teams", "TEAMS",
     "Set number of teams in play to TEAMS.  Defaults to 2."},
    {"g_winlimit", 5, "wins", "WINS",
     "Set number of round wins needed to win the game to WINS.  Defaults to 5."}};

static void TLMSHelp()
{
	Printf("game_tlms - Configures some settings for a basic game of Team Last Marine "
	       "Standing\n");
	GametypeHelp(::tlmsParams);
}

BEGIN_COMMAND(game_tlms)
{
	if (argc < 2)
	{
		TLMSHelp();
		return;
	}

	std::string buffer;
	StringList params = GametypeArgs(::tlmsParams, argc, argv);
	if (params.empty())
	{
		TLMSHelp();
		return;
	}

	params.push_back("g_lives_jointimer 0");
	params.push_back("g_rounds 1");
	params.push_back("sv_forcerespawn 1");
	params.push_back("sv_friendlyfire 0");
	params.push_back("sv_gametype 2");
	params.push_back("sv_nomonsters 1");
	params.push_back("sv_skill 5");

	std::string config = JoinStrings(params, "; ");
	Printf("Configuring Team Last Marine Standing...\n%s\n", config.c_str());
	AddCommandString(config.c_str());
}
END_COMMAND(game_tlms)

static GametypeParam ctfParams[] = {
    {"sv_scorelimit", 5, "score", "SCORELIMIT",
     "Set number of scores needed to win to SCORELIMIT.  Defaults to 5."},
    {"sv_teamsinplay", 2, "teams", "TEAMS",
     "Set number of teams in play to TEAMS.  Defaults to 2."},
    {"sv_timelimit", 10, "time", "TIMELIMIT",
     "Set number of minutes until the match ends to TIMELIMIT.  Defaults to 10."}};

static void CTFHelp()
{
	Printf("game_ctf - Configures some settings for a basic game of Capture the Flag\n");
	GametypeHelp(::ctfParams);
}

BEGIN_COMMAND(game_ctf)
{
	if (argc < 2)
	{
		CTFHelp();
		return;
	}

	std::string buffer;
	StringList params = GametypeArgs(::ctfParams, argc, argv);
	if (params.empty())
	{
		CTFHelp();
		return;
	}

	params.push_back("g_lives 0");
	params.push_back("g_rounds 0");
	params.push_back("sv_forcerespawn 0");
	params.push_back("sv_friendlyfire 0");
	params.push_back("sv_gametype 3");
	params.push_back("sv_nomonsters 1");
	params.push_back("sv_skill 5");

	std::string config = JoinStrings(params, "; ");
	Printf("Configuring Capture the Flag...\n%s\n", config.c_str());
	AddCommandString(config.c_str());
}
END_COMMAND(game_ctf)
