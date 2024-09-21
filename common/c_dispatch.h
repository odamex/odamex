// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1998-2006 by Randy Heit (ZDoom).
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
//	Argument processing (?)
//
//-----------------------------------------------------------------------------

#pragma once

#include "dobject.h"


void C_ExecCmdLineParams (bool onlyset, bool onlylogfile);

// add commands to the console as if they were typed in
// for map changing, etc
void AddCommandString(const std::string &cmd, uint32_t key = 0);

// parse a command string
const char *ParseString (const char *data);

// combine many arguments into one valid argument.
std::string C_ArgCombine(size_t argc, const char **argv);
std::string BuildString (size_t argc, std::vector<std::string> args);

// quote a string
std::string C_QuoteString(const std::string &argstr);

// escape a list of wads
std::string C_EscapeWadList(const std::vector<std::string> wadlist);

class DConsoleCommand : public DObject
{
	DECLARE_CLASS (DConsoleCommand, DObject)
public:
	DConsoleCommand (const char *name);
	virtual ~DConsoleCommand ();
	virtual void Run (uint32_t key = 0) = 0;
	virtual bool IsAlias () { return false; }
	void PrintCommand () { Printf (PRINT_HIGH, "%s\n", m_Name.c_str()); }

	std::string m_Name;

protected:
	DConsoleCommand ();

	AActor *m_Instigator;
	size_t argc;
	char **argv;
	char *args;

	friend void C_DoCommand(const char *cmd, uint32_t key);
};

#define BEGIN_COMMAND(n) \
	class Cmd_##n : public DConsoleCommand { \
		public: \
			Cmd_##n () : DConsoleCommand (#n) {} \
			Cmd_##n (const char *name) : DConsoleCommand (name) {} \
			void Run (uint32_t key = 0)

#define END_COMMAND(n)		}; \
	static Cmd_##n Cmd_instance##n;

class DConsoleAlias : public DConsoleCommand
{
	DECLARE_CLASS (DConsoleAlias, DConsoleCommand)
	bool state_lock;
public:
	DConsoleAlias (const char *name, const char *command);
	virtual ~DConsoleAlias ();
	virtual void Run (uint32_t key = 0);
	virtual bool IsAlias () { return true; }
	void PrintAlias () { Printf (PRINT_HIGH, "%s : %s\n", m_Name.c_str(), m_Command.c_str()); }
	void Archive (FILE *f);

	// Write out alias commands to a file for all current aliases.
	static void C_ArchiveAliases (FILE *f);

	// Destroy all aliases (used on shutdown)
	static void DestroyAll();
protected:
	std::string m_Command;
	std::string m_CommandParam;
};

// Actions
enum {
	ACTION_MLOOK = 0,
	ACTION_KLOOK,
	ACTION_USE,
	ACTION_ATTACK,
	ACTION_SPEED,
	ACTION_MOVERIGHT,
	ACTION_MOVELEFT,
	ACTION_STRAFE,
	ACTION_LOOKDOWN,
	ACTION_LOOKUP,
	ACTION_BACK,
	ACTION_FORWARD,
	ACTION_RIGHT,
	ACTION_LEFT,
	ACTION_MOVEDOWN,
	ACTION_MOVEUP,
	ACTION_JUMP,
	ACTION_SHOWSCORES,

	// Joystick actions
	ACTION_FASTTURN,

	// Automap actions
	ACTION_AUTOMAP_PANUP,
	ACTION_AUTOMAP_PANLEFT,
	ACTION_AUTOMAP_PANDOWN,
	ACTION_AUTOMAP_PANRIGHT,
	ACTION_AUTOMAP_ZOOMIN,
	ACTION_AUTOMAP_ZOOMOUT,
	// NUM
	NUM_ACTIONS
};

extern byte Actions[NUM_ACTIONS];

struct ActionBits
{
	unsigned int	key;
	int				index;
	char			name[12];
};

extern unsigned int MakeKey (const char *s);
