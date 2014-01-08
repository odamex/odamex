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
//	Main
//
// AUTHORS:
//  Michael Wood (mwoodj at huntsvegas dot org)
//
//-----------------------------------------------------------------------------

#include <iostream>
#include <sstream>

#include <agar/core.h>
#include <agar/gui.h>

#ifdef AG_DEBUG
#include <agar/dev.h>
#endif

#include <SDL/SDL.h>

#include "agol_main.h"
#include "net_io.h"
#include "gui_config.h"

#ifdef _XBOX
#include "xbox_main.h"
#endif

using namespace std;

namespace agOdalaunch {

#ifdef AG_DEBUG
static void AGOL_StartDebugger(void)
{
	AG_Window *win;

	if((win = (AG_Window*)AG_GuiDebugger(agWindowFocused)) != NULL)
	{
		AG_WindowShow(win);
	}
}
#endif

int AGOL_InitVideo(const string& drivers, const int width, const int height, const short depth)
{
	ostringstream spec;

	cout << "Initializing with resolution (" << width << "x" << height << ")..." << endl;

	/* Initialize Agar-GUI. */
	if(drivers.size())
	{
		spec << drivers;
	}
	else
	{
		spec << "<OpenGL>";
	}

	spec << "(width=" << width << ":height=" << height << ":depth=" << depth << ")";

	if (AG_InitGraphics(spec.str().c_str()) == -1) 
	{
		cerr << AG_GetError() << endl;
		return -1;
	}

#ifdef _XBOX
	// Software cursor only updates at the refresh rate so make it respectable
	if(agDriverSw)
	{
		AG_SetRefreshRate(60);
	}
#endif

	// Pick up GUI subsystem options
	GuiConfig::Load();

	return 0;
}

} // namespace

using namespace agOdalaunch;

#ifdef GCONSOLE
int agol_main(int argc, char *argv[])
#else
int main(int argc, char *argv[])
#endif
{
	AGOL_MainWindow *mainWindow;
	string           drivers;
	char            *optArg;
	int              c;
	int              width, height;

	/* Initialize Agar-Core. */
	if (AG_InitCore("ag-odalaunch", AG_VERBOSE | AG_CREATE_DATADIR) == -1) 
	{
		cerr << AG_GetError() << endl;
		return (-1);
	}

	// Initial config load
	GuiConfig::Load();

	while ((c = AG_Getopt(argc, argv, "?d:f", &optArg, NULL)) != -1) 
	{
		switch (c) 
		{
			case 'd':
				drivers = optArg;
				break;
			case 'f':
				/* Force full screen */
				GuiConfig::Write("view.full-screen", true);
				break;
			case '?':
			default:
				cout << agProgName << "[-df] [-d driver] " << endl;
				exit(0);
		}
	}

	// Set the default font size
	if(!GuiConfig::IsDefined("font.size"))
	{
		GuiConfig::Write("font.size", 10);
	}

#ifdef GCONSOLE
	// For now just use a resolution that compensates for overscan on most televisions
	width = 600;
	height = 450;
#else
	// Get the dimensions for initialization
	if(GuiConfig::Read("MainWindow-Width", width) || width <= 0)
	{
		width = 640;
	}
	if(GuiConfig::Read("MainWindow-Height", height) || height <= 0)
	{
		height = 480;
	}
#endif

	// Check if a video driver is specified in the config file
	if(!drivers.size())
	{
		GuiConfig::Read("VideoDriver", drivers);

#ifdef _XBOX
		if(!drivers.size())
		{
			drivers = "sdlfb";
		}
#endif
	}

	if(AGOL_InitVideo(drivers, width, height, 32))
	{
		return (-1);
	}

	// Initialize socket API
	BufferedSocket::InitializeSocketAPI();

	// Create the main window
	mainWindow = new AGOL_MainWindow(width, height);

	// Set key bindings
	AG_BindGlobalKey(AG_KEY_ESCAPE, AG_KEYMOD_ANY, AG_QuitGUI);
	AG_BindGlobalKey(AG_KEY_F8, AG_KEYMOD_ANY, AG_ViewCapture);
#ifdef AG_DEBUG
	AG_BindGlobalKey(AG_KEY_F12,    AG_KEYMOD_ANY,  AGOL_StartDebugger);
#endif

#ifdef _XBOX
	// Initialize the Xbox controller
	Xbox::InitializeJoystick();
#endif

	// Event (main) Loop
	AG_EventLoop();

	delete mainWindow;

	GuiConfig::Save();

	AG_Destroy();

	// Shutdown socket API
	BufferedSocket::ShutdownSocketAPI();

	cout << "Exiting..." << endl;

	return 0;
}

