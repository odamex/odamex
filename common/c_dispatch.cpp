// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1998-2006 by Randy Heit (ZDoom).
// Copyright (C) 2006-2025 by The Odamex Team.
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
//	Argument processing (?)
//
//-----------------------------------------------------------------------------


#include "odamex.h"

#include <sstream>
#include <algorithm>
#include <ctime>
#include <map>

#include "cmdlib.h"
#include "c_console.h"
#include "c_dispatch.h"
#include "m_argv.h"
#include "m_fileio.h"
#include "m_alloc.h"
#include "d_player.h"
#include "r_defs.h"
#include "i_system.h"

#include "hashtable.h"
#include "m_ostring.h"
#include "oscanner.h"

IMPLEMENT_CLASS (DConsoleCommand, DObject)
IMPLEMENT_CLASS (DConsoleAlias, DConsoleCommand)

EXTERN_CVAR (lookspring)

typedef std::map<std::string, DConsoleCommand *> command_map_t;
command_map_t &Commands()
{
	static command_map_t _Commands;
	return _Commands;
}

struct ActionBits actionbits[NUM_ACTIONS] =
{
	{ 0x00409, ACTION_USE,				"use" },
	{ 0x0074d, ACTION_BACK,				"back" },
	{ 0x007e4, ACTION_LEFT,				"left" },
	{ 0x00816, ACTION_JUMP,				"jump" },
	{ 0x0106d, ACTION_KLOOK,			"klook" },
	{ 0x0109d, ACTION_MLOOK,			"mlook" },
	{ 0x010d8, ACTION_RIGHT,			"right" },
	{ 0x0110a, ACTION_SPEED,			"speed" },
	{ 0x01fc5, ACTION_ATTACK,			"attack" },
	{ 0x021ae, ACTION_LOOKUP,			"lookup" },
	{ 0x021fe, ACTION_MOVEUP,			"moveup" },
	{ 0x02315, ACTION_STRAFE,			"strafe" },
	{ 0x041c4, ACTION_FORWARD,			"forward" },
	{ 0x07cfa, ACTION_AUTOMAP_PANUP,	"am_panup" },
	{ 0x08126, ACTION_FASTTURN,   		"fastturn"},
    { 0x08788, ACTION_LOOKDOWN,			"lookdown"},
	{ 0x088c4, ACTION_MOVELEFT,			"moveleft" },
	{ 0x088c8, ACTION_MOVEDOWN,			"movedown" },
	{ 0x0fc5c, ACTION_AUTOMAP_ZOOMIN,	"am_zoomin" },
	{ 0x11268, ACTION_MOVERIGHT,		"moveright" },
	{ 0x1f4b4, ACTION_AUTOMAP_PANLEFT,	"am_panleft" },
	{ 0x1f4b8, ACTION_AUTOMAP_PANDOWN,	"am_pandown" },
	{ 0x1f952, ACTION_AUTOMAP_ZOOMOUT,	"am_zoomout" },
	{ 0x2314d, ACTION_SHOWSCORES,		"showscores" },
	{ 0x3ea48, ACTION_AUTOMAP_PANRIGHT, "am_panright" },
};
byte Actions[NUM_ACTIONS];


class ActionKeyTracker
{
public:

	ActionKeyTracker() :
		mTable(NUM_ACTIONS)
	{ }

	void clear()
	{
		mTable.clear();
	}

	bool isActionActivated(const OString action)
	{
		ActionKeyListTable::iterator it = mTable.find(action);
		if (it == mTable.end())
			return false;
		ActionKeyList* action_key_list = &it->second;
		return !action_key_list->empty();
	}

	bool pressKey(uint32_t key, const OString action)
	{
		ActionKeyListTable::iterator it = mTable.find(action);
		if (it == mTable.end())
			it = mTable.emplace(action, ActionKeyList()).first;
		ActionKeyList* action_key_list = &it->second;

		if (std::find(action_key_list->begin(), action_key_list->end(), key) != action_key_list->end())
			return false;

		action_key_list->push_back(key);
		return true;
	}

	bool releaseKey(uint32_t key, const OString action)
	{
		ActionKeyListTable::iterator it = mTable.find(action);
		if (it == mTable.end())
			return false;

		ActionKeyList* action_key_list = &it->second;
		action_key_list->remove(key);
		return action_key_list->empty();
	}

private:
	typedef std::list<uint32_t> ActionKeyList;

	typedef OHashTable<OString, ActionKeyList> ActionKeyListTable;
	ActionKeyListTable mTable;
};

static ActionKeyTracker action_key_tracker;


static int ListActionCommands (void)
{
	int i;

	for (i = 0; i < NUM_ACTIONS; i++)
	{
		Printf (PRINT_HIGH, "+%s\n", actionbits[i].name);
		Printf (PRINT_HIGH, "-%s\n", actionbits[i].name);
	}
	return NUM_ACTIONS * 2;
}

unsigned int MakeKey (const char *s)
{
	unsigned int v = 0;

	if (*s)
		v = tolower(*s++);
	if (*s)
		v = (v*3) + tolower(*s++);
	while (*s)
		v = (v << 1) + tolower(*s++);

	return v;
}

// GetActionBit scans through the actionbits[] array
// for a matching key and returns an index or -1 if
// the key could not be found. This uses binary search,
// actionbits[] must be sorted in ascending order.

int GetActionBit (unsigned int key)
{
	int min = 0;
	int max = NUM_ACTIONS - 1;

	while (min <= max)
	{
		int mid = (min + max) / 2;
		unsigned int seekey = actionbits[mid].key;

		if (seekey == key)
			return actionbits[mid].index;
		else if (seekey < key)
			min = mid + 1;
		else
			max = mid - 1;
	}

	return -1;
}

bool safemode = false;


void C_DoCommand(std::string_view cmd, uint32_t key)
{
	std::string_view data = cmd;

	std::optional<std::string> token = ParseString(data);
	if (!token)
		return;

	// Check if this is an action
	if (token->at(0) == '+' || token->at(0) == '-')
	{
		OString action(token->substr(1));
		int check = GetActionBit(MakeKey(action.c_str()));

		if (token->at(0) == '+')
		{
			if (action_key_tracker.pressKey(key, action))
			{
				if (check != -1)
					Actions[check] = 1;
			}
		}
		else if (token->at(0) == '-')
		{
			if (action_key_tracker.releaseKey(key, action))
			{
				if (check != -1)
					Actions[check] = 0;

				if ((check == ACTION_LOOKDOWN || check == ACTION_LOOKUP || check == ACTION_MLOOK) && lookspring)
					AddCommandString("centerview");
			}
		}
		if (check != -1)
			return;
	}

	size_t argc = 0;
	std::vector<char*> argv;
	StringTokens args;
	const char *realargs = data.data();
	data = cmd;

	while (token = ParseString(data))
	{
		args.push_back(*token);
		argc++;
	}

	for (std::string& arg : args)
		argv.push_back(arg.data());

	// Checking for matching commands follows this search order:
	//	1. Check the Commands map
	//	2. Check the CVars list
	command_map_t::iterator c = Commands().find(StdStringToLower(argv[0]));
	DConsoleCommand* com;

	if (c != Commands().end())
	{
		com = c->second;

		if (!safemode || stricmp(argv[0], "if") == 0 || stricmp(argv[0], "exec") == 0)
		{
			com->argc = argc;
			com->argv = argv.data();
			com->args = realargs;
			com->m_Instigator = consoleplayer().mo;
			com->Run(key);
		}
		else
		{
			PrintFmt(PRINT_HIGH, "Not a cvar command \"{}\"\n", argv[0]);
		}
	}
	else
	{
		// Check for any CVars that match the command
		cvar_t *var, *dummy;

		if ((var = cvar_t::FindCVar(argv[0], &dummy)))
		{
			if (argc >= 2)
			{
				c = Commands().find("set");
				if (c != Commands().end())
				{
					com = c->second;
					com->argc = argc + 1;
					com->argv = argv.data() - 1; // Hack
					com->m_Instigator = consoleplayer().mo;
					com->Run(key);
				}
				else
					PrintFmt(PRINT_HIGH, "set command not found\n");
			}
			else
			{
				c = Commands().find("get");
				if (c != Commands().end())
				{
					com = c->second;
					com->argc = argc + 1;
					com->argv = argv.data() - 1; // Hack
					com->m_Instigator = consoleplayer().mo;
					com->Run();
				}
				else
					PrintFmt(PRINT_WARNING, "get command not found\n");
			}
		}
		else
		{
			// We don't know how to handle this command
			PrintFmt(PRINT_WARNING, "Unknown command \"{}\"\n", argv[0]);
		}
	}
}

void AddCommandString(const std::string &str, uint32_t key)
{
	size_t totallen = str.length();
	if (!totallen)
		return;

	// pointers to the start and end of the current substring in str.c_str()
	const char* cstart = str.c_str();
	const char* cend;

	// stores a copy of the current substring
	char* command = new char[totallen + 1];

	// scan for a command ending
	while (*cstart)
	{
		const char* cp = cstart;

		// read until the next command (separated by semicolon) or until comment (two slashes)
		while (*cp != ';' && !(cp[0] == '/' && cp[1] == '/') && *cp != 0)
		{
			if (cp[0] == '\\' && cp[1] != 0)
			{
				// [AM] Skip two chars if escaped.
				cp += 2;
			}
			else if (*cp == '"')
			{
				// Ignore ';' if it is inside a pair of quotes.
				while (1)
				{
					cp++;
					if (*cp == 0)
					{
						// End of string.
						break;
					}
					if (cp[0] == '\\' && cp[1] == '"')
					{
						// [AM] Skip over escaped quote.
						cp++;
					}
					else if (*cp == '"')
					{
						// End of quote.  Skip over ending quote.
						cp++;
						break;
					}
				}
			}
			else
			{
				// Advance to next char.
				cp++;
			}
		}

		cend = cp - 1;

		// remove leading and trailing whitespace
		while (cstart < cend && *cstart == ' ')
			cstart++;
		while (cend > cstart && *cend == ' ')
			cend--;

		size_t clength = cend - cstart + 1;
		memcpy(command, cstart, clength);
		command[clength] = '\0';

		C_DoCommand(command, key);

		// don't parse anymore if there's a comment
		if (cp[0] == '/' && cp[1] == '/')
			break;

		// are there more commands following this one?
		if (*cp == ';')
			cstart = cp + 1;
		else
			cstart = cp;
	}

	delete[] command;
}

#define MAX_EXEC_DEPTH 32

static bool if_command_result;

BEGIN_COMMAND (exec)
{
	if (argc < 2)
		return;

	static std::vector<std::string> exec_stack;
	static std::vector<bool>	tag_stack;

	std::string found = M_FindUserFileName(argv[1], ".cfg");
	if (found.empty())
	{
		const char* cfgdir = Args.CheckValue("-cfgdir");
		if (!cfgdir)
		{
			Printf(PRINT_WARNING, "Could not find \"%s\"\n", argv[1]);
			return;
		}

		found = M_CleanPath(M_JoinPath(cfgdir, argv[1]));
		if (!M_FileExists(found))
		{
			found += ".cfg";
			if (!M_FileExists(found))
			{
				Printf(PRINT_WARNING, "Could not find \"%s\"\n", argv[1]);
				return;
			}
		}
	}

	if(std::find(exec_stack.begin(), exec_stack.end(), found) != exec_stack.end())
	{
		Printf (PRINT_HIGH, "Ignoring recursive exec \"%s\"\n", found);
		return;
	}

	if(exec_stack.size() >= MAX_EXEC_DEPTH)
	{
		Printf (PRINT_HIGH, "Ignoring recursive exec \"%s\"\n", found);
		return;
	}

	std::ifstream ifs(found);
	if(ifs.fail())
	{
		Printf(PRINT_WARNING, "Could not open \"%s\"\n", found);
		return;
	}

	exec_stack.push_back(found);

	while(ifs)
	{
		std::string line;
		std::getline(ifs, line);
		line = TrimString(line);

		if (line.empty())
			continue;

		// start tag
		if(line.substr(0, 3) == "#if")
		{
			AddCommandString(line.c_str() + 1);
			tag_stack.push_back(if_command_result);

			continue;
		}

		// else tag
		if(line.substr(0, 5) == "#else")
		{
			if(tag_stack.empty())
				Printf(PRINT_HIGH, "Ignoring stray #else\n");
			else
				tag_stack.back() = !tag_stack.back();

			continue;
		}

		// end tag
		if(line.substr(0, 6) == "#endif")
		{
			if(tag_stack.empty())
				Printf(PRINT_HIGH, "Ignoring stray #endif\n");
			else
				tag_stack.pop_back();

			continue;
		}

		// inside tag that evaluated false?
		if(!tag_stack.empty() && !tag_stack.back())
			continue;

		AddCommandString(line);
	}

	exec_stack.pop_back();
}
END_COMMAND (exec)

// denis
// if cvar eq blah "command";
BEGIN_COMMAND (if)
{
	if_command_result = false;

	if (argc < 4)
		return;

	cvar_t *var, *dummy;
	var = cvar_t::FindCVar (argv[1], &dummy);

	if (!var)
	{
		Printf(PRINT_HIGH, "if: no cvar named %s\n", argv[1]);
		return;
	}

	std::string op = argv[2];

	if(op == "eq")
	{
		if_command_result = !strcmp(var->cstring(), argv[3]);
	}
	else if(op == "ne")
	{
		if_command_result = ((strcmp(var->cstring(), argv[3])) != 0);
	}
	else
	{
		Printf(PRINT_HIGH, "if: no operator %s\n", argv[2]);
		Printf(PRINT_HIGH, "if: operators are eq, ne\n");
		return;
	}

	if(if_command_result && argc > 4)
	{
		std::string param = C_ArgCombine(argc - 4, (const char **)&argv[4]);
		AddCommandString(param);
	}
}
END_COMMAND (if)

// Returns true if the character is a valid escape char, false otherwise.
bool ValidEscape(char data)
{
	return (data == '"' || data == ';' || data == '\\');
}

// ParseString2 is adapted from COM_Parse
// found in the Quake2 source distribution
std::optional<std::string> ParseString2(std::string_view& data)
{
	std::string token;

	const auto white = data.find_first_not_of(' ');
	if (white != std::string_view::npos)
		data.remove_prefix(white);
	else
		return std::nullopt;

	// Ch0wW : If having a comment, break immediately the line!
	if (data.length() >= 2 && data[0] == '/' && data[1] == '/') {
		return std::nullopt;
	}

	if (data.length() >= 2 && data[0] == '\\' && ValidEscape(data[1]))
	{
		// [AM] Handle escaped chars.
		token += data[1];
		data.remove_prefix(2);
	}
	else if (data[0] == '"')
	{
		// Quoted strings count as one large token.
		while (true) {
			data.remove_prefix(1);
			if (data.empty())
			{
				// [AM] Unclosed quote, show no mercy.
				return std::nullopt;
			}
			if (data.length() >= 2 && data[0] == '\\' && ValidEscape(data[1]))
			{
				// [AM] Handle escaped chars.
				token += data[1];
				data.remove_prefix(1); // Skip one _additional_ char.
				continue;
			}
			else if (data[0] == '"')
			{
				// Closing quote, that's the entire token.
				data.remove_prefix(1); // Skip the closing quote.
				return std::optional(token);
			}
			// None of the above, copy the char and continue.
			token += data[0];
		}
	}

	while (true) {
		// Parse a regular word.
		if (data.empty() || data[0] <= ' ')
		{
			// End of word.
			break;
		}
		if (data.length() >= 2 && data[0] == '\\' && ValidEscape(data[1]))
		{
			// [AM] Handle escaped chars.
			token += data[1];
			data.remove_prefix(2); // Skip two chars.
			continue;
		}
		else if (data[0] == '"')
		{
			// End of word.
			break;
		}
		// None of the above, copy the char and continue.
		token += data[0];
		data.remove_prefix(1);
	}
	// We're done
	// return the remaining data to parse.
	return std::optional(token);
}

// ParseString calls ParseString2 to remove the first
// token from an input string. If this token is of
// the form $<cvar>, it will be replaced by the
// contents of <cvar>.
std::optional<std::string> ParseString (std::string_view& data)
{
	cvar_t *var, *dummy;
	std::optional<std::string> token = ParseString2(data);

	if (token && !token->empty() && token->at(0) == '$')
	{
		if ( (var = cvar_t::FindCVar (std::string_view(*token).substr(1).data(), &dummy)) )
		{
			return std::optional(var->str());
		}
	}

	return token;
}

DConsoleCommand::DConsoleCommand (const char *name)
{
	static bool firstTime = true;

	if (firstTime)
	{
		firstTime = false;

		// Add all the action commands for tab completion
		for (const auto& bit : actionbits)
		{
			std::string tname = fmt::format("+{}", bit.name);
			C_AddTabCommand(tname.c_str());
			tname[0] = '-';
			C_AddTabCommand(tname.c_str());
		}
	}

	m_Name = name;

	Commands()[name] = this;
	C_AddTabCommand(name);
}

DConsoleCommand::~DConsoleCommand ()
{
	C_RemoveTabCommand (m_Name.c_str());
}

DConsoleAlias::DConsoleAlias (const char *name, const char *command)
	:	DConsoleCommand(StdStringToLower(name).c_str()),  state_lock(false),
		m_Command(command)
{
}

DConsoleAlias::~DConsoleAlias ()
{
}

void DConsoleAlias::Run(uint32_t key)
{
	if(!state_lock)
	{
		state_lock = true;

        m_CommandParam = m_Command;

		// [Russell] - Allows for aliases with parameters
		if (argc > 1)
        {
            for (size_t i = 1; i < argc; i++)
            {
                m_CommandParam += " ";
                m_CommandParam += argv[i];
            }
        }

        AddCommandString(m_CommandParam, key);

		state_lock = false;
	}
	else
	{
		Printf(PRINT_HIGH, "warning: ignored recursive alias");
	}
}

// [AM] Combine many arguments into one valid argument.  Since this
//      function is called after we parse arguments, we don't need to
//      escape the output.
std::string C_ArgCombine(size_t argc, const char **argv)
{
	std::ostringstream buffer;
	for (size_t i = 0;i < argc;i++)
	{
		buffer << argv[i];
		if (i + 1 < argc)
			buffer << " ";
	}
	return buffer.str();
}

std::string BuildString (size_t argc, std::vector<std::string> args)
{
	std::string out;

	for(size_t i = 0; i < argc; i++)
	{
		if(args[i].find_first_of(' ') != std::string::npos)
		{
			out += "\"";
			out += args[i];
			out += "\"";
		}
		else
		{
			out += args[i];
		}

		if(i + 1 < argc)
		{
			out += " ";
		}
	}

	return out;
}

// [AM] Take a string, quote it, and escape it, making it suitable for parsing
//      as an argument.
std::string C_QuoteString(const std::string &argstr)
{
	std::ostringstream buffer;
	buffer << "\"";
	for (const auto c : argstr)
	{
		if (ValidEscape(c))
		{
			// Escape this char.
			buffer << '\\' << c;
		}
		else
		{
			buffer << c;
		}
	}
	buffer << "\"";
	return buffer.str();
}

// Take a string of inputted WADs and escape them indvidually
// and add a space before loading them into the system.
std::string C_EscapeWadList(const std::vector<std::string> wadlist)
{
	std::string wadstr;
	for (size_t i = 0; i < wadlist.size(); i++)
	{
		if (i != 0)
		{
			wadstr += " ";
		}
		wadstr += C_QuoteString(wadlist.at(i));
	}
	return wadstr;
}

static int DumpHash (bool aliases)
{
	int count = 0;

	for (const auto& [_, cmd] : Commands())
	{
		count++;
		if (cmd->IsAlias())
		{
			if (aliases)
				static_cast<DConsoleAlias *>(cmd)->PrintAlias ();
		}
		else if (!aliases)
			cmd->PrintCommand ();
	}

	return count;
}

void DConsoleAlias::Archive(FILE *f)
{
	fmt::print(f, "alias {} {}\n", C_QuoteString(m_Name), C_QuoteString(m_Command));
}

void DConsoleAlias::C_ArchiveAliases (FILE *f)
{
	for (const auto& [_, alias] : Commands())
	{
		if (alias->IsAlias())
			static_cast<DConsoleAlias *>(alias)->Archive (f);
	}
}

void DConsoleAlias::DestroyAll()
{
	for (const auto& [_, alias] : Commands())
	{
		if (alias->IsAlias())
			delete alias;
	}
}

BEGIN_COMMAND (alias)
{
	if (argc == 1)
	{
		Printf (PRINT_HIGH, "Current alias commands:\n");
		DumpHash (true);
	}
	else
	{
		command_map_t::iterator i = Commands().find(StdStringToLower(argv[1]));

		if(i != Commands().end())
		{
			if(i->second->IsAlias())
			{
				// Remove the old alias
				delete i->second;
				Commands().erase(i);
			}
			else
			{
				Printf(PRINT_HIGH, "%s: is a command, can not become an alias\n", argv[1]);
				return;
			}
		}
		else if(argc == 2)
		{
			Printf(PRINT_HIGH, "%s: not an alias\n", argv[1]);
			return;
		}

		if(argc > 2)
		{
			// Build the new alias
			std::string param = C_ArgCombine(argc - 2, (const char **)&argv[2]);
			new DConsoleAlias (argv[1], param.c_str());
		}
	}
}
END_COMMAND (alias)

BEGIN_COMMAND (cmdlist)
{
	int count;

	count = ListActionCommands ();
	count += DumpHash (false);
	Printf (PRINT_HIGH, "%d commands\n", count);
}
END_COMMAND (cmdlist)


// Execute any console commands specified on the command line.
// These all begin with '+' as opposed to '-'.
// If onlyset is true, only "set" commands will be executed,
// otherwise only non-"set" commands are executed.
// If onlylogfile is true... well, you get the point.
void C_ExecCmdLineParams (bool onlyset, bool onlylogfile)
{
	size_t cmdlen, argstart;
	int didlogfile = 0;

	for (size_t currArg = 1; currArg < Args.NumArgs(); )
	{
		if (*Args.GetArg (currArg++) == '+')
		{
			int setComp = stricmp (Args.GetArg (currArg - 1) + 1, "set");
			int logfileComp = stricmp (Args.GetArg (currArg - 1) + 1, "logfile");
			if ((onlyset && setComp) || (onlylogfile && logfileComp) ||
                (!onlyset && !setComp) || (!onlylogfile && !logfileComp))
			{
				continue;
			}

			cmdlen = 1;
			argstart = currArg - 1;

			while (currArg < Args.NumArgs())
			{
				if (*Args.GetArg (currArg) == '-' || *Args.GetArg (currArg) == '+')
					break;
				currArg++;
				cmdlen++;
			}

			std::string cmdString = BuildString (cmdlen, Args.GetArgList(argstart));
			if (cmdString.length()) {
				C_DoCommand(std::string_view(cmdString).substr(1), 0);
				if (onlylogfile) didlogfile = 1;
			}
		}
	}

    // [Nes] - Calls version at startup if no logfile.
	if (onlylogfile && !didlogfile) AddCommandString("version");
}

BEGIN_COMMAND (actorlist)
{
	AActor *mo;
	TThinkerIterator<AActor> iterator;
	Printf (PRINT_HIGH, "Actors at level.time == %d:\n", level.time);
	while ( (mo = iterator.Next ()) )
	{
		Printf (PRINT_HIGH, "%s (%x, %x, %x | %x) state: %zd tics: %d\n", mobjinfo[mo->type].name, mo->x, mo->y, mo->z, mo->angle, mo->state - states, mo->tics);
	}
}
END_COMMAND(actorlist)

BEGIN_COMMAND(logfile)
{
	time_t rawtime;
	struct tm* timeinfo;
	const std::string default_logname =
	    M_GetUserFileName(::serverside ? "odasrv.log" : "odamex.log");

	if (::LOG.is_open())
	{
		if ((argc == 1 && ::LOG_FILE == default_logname) ||
		    (argc > 1 && ::LOG_FILE == argv[1]))
		{
			Printf("Log file %s already in use\n", ::LOG_FILE.c_str());
			return;
		}

		time(&rawtime);
		timeinfo = localtime(&rawtime);
		Printf("Log file %s closed on %s\n", ::LOG_FILE, asctime(timeinfo));
		::LOG.close();
	}

	::LOG_FILE = (argc > 1 ? argv[1] : default_logname);
	::LOG.open(::LOG_FILE.c_str(), std::ios::app);

	if (!::LOG.is_open())
	{
		Printf(PRINT_HIGH, "Unable to create logfile: %s\n", ::LOG_FILE);
	}
	else
	{
		time(&rawtime);
		timeinfo = localtime(&rawtime);
		::LOG.flush();
		::LOG << std::endl;
		Printf(PRINT_HIGH, "Logging in file %s started %s\n", ::LOG_FILE,
		       asctime(timeinfo));
	}
}
END_COMMAND(logfile)

BEGIN_COMMAND (stoplog)
{
	time_t rawtime;
	struct tm * timeinfo;

	if (LOG.is_open()) {
		time (&rawtime);
    	timeinfo = localtime (&rawtime);
		Printf (PRINT_HIGH, "Logging to file %s stopped %s\n", LOG_FILE, asctime (timeinfo));
		LOG.close();
	}
}
END_COMMAND (stoplog)

bool P_StartScript (AActor *who, line_t *where, int script, const char *map, int lineSide,
					int arg0, int arg1, int arg2, int always);

BEGIN_COMMAND (puke)
{
	if (argc < 2 || argc > 5) {
		Printf (PRINT_HIGH, " puke <script> [arg1] [arg2] [arg3]\n");
	} else {
		int script = atoi (argv[1]);
		int arg0=0, arg1=0, arg2=0;

		if (argc > 2) {
			arg0 = atoi (argv[2]);
			if (argc > 3) {
				arg1 = atoi (argv[3]);
				if (argc > 4) {
					arg2 = atoi (argv[4]);
				}
			}
		}
		P_StartScript (m_Instigator, NULL, script, level.mapname.c_str(), 0, arg0, arg1, arg2, false);
	}
}
END_COMMAND (puke)

BEGIN_COMMAND (error)
{
	std::string text = C_ArgCombine(argc - 1, (const char **)(argv + 1));
	I_Error (text);
}
END_COMMAND (error)

VERSION_CONTROL (c_dispatch_cpp, "$Id$")
