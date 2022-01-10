// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2006-2021 by The Odamex Team.
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
//  Main application sequence
//
//-----------------------------------------------------------------------------

#include "odalaunch.h"

#include "Fl/Fl.H"

#include "db.h"
#include "main_window.h"
#include "net_io.h"
#include "work_thread.h"

globals_t g;

static void Shutdown()
{
	odalpapi::BufferedSocket::ShutdownSocketAPI();
	DB_DeInit();
}

int main(int argc, char** argv)
{
	atexit(Shutdown);

	if (!DB_Init())
	{
		Fl::error("Could not initialize database.");
		return 1;
	}

	if (!odalpapi::BufferedSocket::InitializeSocketAPI())
	{
		Fl::error("Could not initialize sockets.");
		return 1;
	}

	::g.mainWindow = new MainWindow(640, 480);
	::g.mainWindow->show(argc, argv);

	Work_Init();

	Fl::run();
	return 0;
}

#ifdef _WIN32

#include <stdlib.h>

int CALLBACK WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPSTR lpCmdLine, _In_ int nCmdShow)
{
	return main(__argc, __argv);
}

#endif
