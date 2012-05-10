// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
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
//	Launch a Game Instance
//
// AUTHORS:
//	 Michael Wood (mwoodj at huntsvegas dot org)
//
//-----------------------------------------------------------------------------

#include <iostream>
#include <string>
#include <memory>

#ifdef _XBOX
#include <xtl.h>
#endif

#include <agar/core.h>
#include <agar/gui.h>

#include "game_command.h"
#include "gui_config.h"

namespace agOdalaunch {

#ifdef _WIN32
#define usleep(t) Sleep(t / 1000)
#endif

using namespace std;

void GameCommand::AddParameter(const string &parameter)
{
#ifdef _WIN32
	Parameters.push_back(string("\"") + parameter + string("\""));
#else
	Parameters.push_back(parameter);
#endif
}

void GameCommand::AddParameter(const string &parameter, const string &value)
{
#ifdef _WIN32
	Parameters.push_back(string("\"") + parameter + string("\""));
	Parameters.push_back(string("\"") + value + string("\""));
#else
	Parameters.push_back(parameter);
	Parameters.push_back(value);
#endif
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

#ifdef GCONSOLE
	// Game consoles will not return from this method if Odamex is successfully
	// launched as the launcher will terminate. On game consoles the main windows
	// widget states need to be saved before launch or changes will be lost.
	AG_Driver *drv;

	AGOBJECT_FOREACH_CHILD(drv, &agDrivers, ag_driver)
	{
		AG_Window *mainWindow;

		if((mainWindow = static_cast<AG_Window*>(AG_ObjectFindChild(drv, "MainWindow"))) != NULL)
		{
			AG_PostEvent(mainWindow, mainWindow, "save-wstates", NULL);
			break;
		}
	}

	GuiConfig::Save();
#endif

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

	Cleanup(pid);

	return 0;
}

//******************//
// Private Functions //
//******************//
void *GameCommand::Cleanup(void *vpPID)
{
	if(vpPID)
	{
		AG_ProcessID pid = *(AG_ProcessID*)vpPID;

		if((AG_WaitOnProcess(pid, AG_EXEC_WAIT_INFINITE)) == pid)
			cout << "Odamex client (" << pid << ") exited cleanly." << endl;
		else
			cout << "Odamex client (" << pid << ") exited with error: " << AG_GetError() << endl;

		// It is most likely that the instance of the GameCommand class that launched
		// this thread doesn't exist anymore so freeing this here is the only option.
		delete (AG_ProcessID*)vpPID;
	}

	return NULL;
}

void GameCommand::Cleanup(AG_ProcessID pid)
{
	AG_ProcessID *pPID = new AG_ProcessID(pid);
	AG_Thread     thread;

	AG_ThreadCreate(&thread, Cleanup, (void *)pPID);
}

} // namespace
