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

#ifndef WIN32
#include <sys/types.h>
#include <sys/wait.h>
#endif

#include <agar/core.h>
#include <agar/gui.h>

#include "game_command.h"
#include "gui_config.h"

#define MAX_ARGS 100

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
	string cmd;
	list<string>::iterator i;

#ifdef _XBOX
	cout << "Xbox: Stub!" << endl;
#elif WIN32
	cout << "Windows: Stub!" << endl;
#else
	char  *argv[MAX_ARGS];
	int    argc = 1;
	pid_t  pid;

	// Get the odamex bin path
	if(GuiConfig::Read("OdamexPath", cmd))
	{
		char cwd[PATH_MAX];

		// No path is configured so use the CWD
		if(!AG_GetCWD(cwd, PATH_MAX))
			cmd = string(cwd) + "/";
	}
	else
		cmd += "/";

	// Add the bin to the path
	cmd += "odamex";

	// The path with bin tacked on is the first arg
	argv[0] = strdup(cmd.c_str());
	
	// Add each param as an arg
	for(i = Parameters.begin(); i != Parameters.end(); ++i, ++argc)
		argv[argc] = strdup((*i).c_str());

	// Mark the end
	argv[argc] = NULL;

	// Fork off a new process
	if((pid = fork()) < 0)
		cerr << "Fork Failed: " << strerror(errno) << endl;
	else if(pid == 0) // child
	{
		// Execute the command
		if(execvp(*argv, argv) < 0)
		{
			cerr << "Failed to execute: " << strerror(errno) << endl;
			exit(1); // Exit child process
		}
	}
	else // parent
	{
#if 0
		int status;

		waitpid(pid, &status, 0);
		
		if(status)
			AG_TextErrorS("Failed to launch Odamex.\n"
					"Please verify that your Odamex path is set correctly.");
#endif
	}

	for(int j = 0; j < argc; j++)
		free(argv[j]);
#endif

	return 0;
}
