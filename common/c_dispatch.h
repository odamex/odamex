// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1998-2006 by Randy Heit (ZDoom).
// Copyright (C) 2006-2012 by The Odamex Team.
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


#ifndef __C_DISPATCH_H__
#define __C_DISPATCH_H__

#include "dobject.h"

#include <string>
#include <vector>

#define HASH_SIZE	251				// I think this is prime

void C_ExecCmdLineParams (bool onlyset, bool onlylogfile);

// add commands to the console as if they were typed in
// for map changing, etc
void AddCommandString (const std::string &cmd, bool onlycvar = false);

// parse a command string
const char *ParseString (const char *data);

// combine many arguments into one valid argument.
std::string C_ArgCombine(size_t argc, const char **argv);
std::string BuildString (size_t argc, std::vector<std::string> args);

// quote a string
std::string C_QuoteString(const std::string &argstr);

class DConsoleCommand : public DObject
{
	DECLARE_CLASS (DConsoleCommand, DObject)
public:
	DConsoleCommand (const char *name);
	virtual ~DConsoleCommand ();
	virtual void Run () = 0;
	virtual bool IsAlias () { return false; }
	void PrintCommand () { Printf (PRINT_HIGH, "%s\n", m_Name.c_str()); }

	std::string m_Name;

protected:
	DConsoleCommand ();

	AActor *m_Instigator;
	size_t argc;
	char **argv;
	char *args;

	friend void C_DoCommand (const char *cmd);
};

#define BEGIN_COMMAND(n) \
	class Cmd_##n : public DConsoleCommand { \
		public: \
			Cmd_##n () : DConsoleCommand (#n) {} \
			Cmd_##n (const char *name) : DConsoleCommand (name) {} \
			void Run ()

#define END_COMMAND(n)		}; \
	static Cmd_##n Cmd_instance##n;

class DConsoleAlias : public DConsoleCommand
{
	DECLARE_CLASS (DConsoleAlias, DConsoleCommand)
	bool state_lock;
public:
	DConsoleAlias (const char *name, const char *command);
	virtual ~DConsoleAlias ();
	virtual void Run ();
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
#define ACTION_MLOOK		0
#define ACTION_KLOOK		1
#define ACTION_USE			2
#define ACTION_ATTACK		3
#define ACTION_SPEED		4
#define ACTION_MOVERIGHT	5
#define ACTION_MOVELEFT		6
#define ACTION_STRAFE		7
#define ACTION_LOOKDOWN		8
#define ACTION_LOOKUP		9
#define ACTION_BACK			10
#define ACTION_FORWARD		11
#define ACTION_RIGHT		12
#define ACTION_LEFT			13
#define ACTION_MOVEDOWN		14
#define ACTION_MOVEUP		15
#define ACTION_JUMP			16
#define ACTION_SHOWSCORES	17
#define NUM_ACTIONS			18

extern byte Actions[NUM_ACTIONS];

struct ActionBits
{
	unsigned int	key;
	int				index;
	char			name[12];
};

extern unsigned int MakeKey (const char *s);

#endif //__C_DISPATCH_H__
