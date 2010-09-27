// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2006-2010 by The Odamex Team.
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
//	Launch a Game Instance
//
// AUTHORS:
//	 Michael Wood (mwoodj at huntsvegas dot org)
//
//-----------------------------------------------------------------------------

#include <iostream>
#include <string>
#include <cstdlib>
#include <errno.h>

#ifdef _XBOX
#include <xtl.h>
#endif

#ifndef _WIN32
#include <sys/types.h>
#include <sys/wait.h>
#endif

#include <agar/core.h>
#include <agar/gui.h>

#include "game_command.h"
#include "gui_config.h"

#ifdef _WIN32
#define usleep(t) Sleep(t / 1000)
#endif

using namespace std;

void GameCommand::AddParameter(string parameter)
{
	Parameters.push_back(parameter);
}

void GameCommand::AddParameter(string parameter, string value)
{
	Parameters.push_back(parameter);
	Parameters.push_back(value);
}

int GameCommand::Launch()
{
	list<string>::iterator i;
	string        cmd;
	AG_ProcessID  pid;
	char         *argv[AG_ARG_MAX];
	int           argc = 1;

	// Get the odamex bin path
	if(GuiConfig::Read("OdamexPath", cmd))
	{
		char cwd[AG_PATHNAME_MAX];

		// No path is configured so use the CWD
		if(!AG_GetCWD(cwd, AG_PATHNAME_MAX))
			cmd = string(cwd) + AG_PATHSEP;
	}
	else
		cmd += AG_PATHSEP;

	// Add the bin to the path
#ifdef _XBOX
	cmd += "odamex.xbe";
#elif _WIN32
	cmd += "odamex.exe";
#else
	cmd += "odamex";
#endif

	// The path with bin tacked on is the first arg
	argv[0] = strdup(cmd.c_str());
	
	// Add each param as an arg
	for(i = Parameters.begin(); i != Parameters.end(); ++i, ++argc)
		argv[argc] = strdup((*i).c_str());

	// Mark the end
	argv[argc] = NULL;

	// Launch Odamex
	if((pid = AG_Execute(*argv, argv)) == -1)
	{
		AG_TextErrorS("Failed to launch Odamex.\n"
			"Please verify that your Odamex path is set correctly.");
		return -1;
	}

	for(int j = 0; j < argc; j++)
		free(argv[j]);

	// Give the new process a very short time to start or fail.
	usleep(5000);

	// Perform a non-blocking wait to find out if the launch failed.
	if(AG_WaitOnProcess(pid, AG_EXEC_WAIT_IMMEDIATE) == -1)
	{
		AG_TextErrorS("Failed to launch Odamex.\n"
			"Please verify that your Odamex path is set correctly.");
		return -1;
	}

	return 0;
}
