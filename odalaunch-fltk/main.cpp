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

#include "main_window.h"

#include <string>
#include <vector>

#include "Fl/Fl.H"
#include "thread.h"

#include "net_io.h"
#include "net_packet.h"

// Default list of master servers, usually official ones
static const char* DEFAULT_MASTERS[] = {"master1.odamex.net:15000",
                                        "voxelsoft.com:15000"};

struct globals_t
{
	odalpapi::MasterServer master;
	std::vector<odalpapi::Server> servers;
	int masterTimeout;
	int serverTimeout;
	bool broadcast;
	int retries;
	globals_t()
	    : masterTimeout(500), serverTimeout(1000), broadcast(false), retries(2) { }
} g;

static void RefreshServers()
{
	for (size_t i = 0; i < ARRAY_LENGTH(::DEFAULT_MASTERS); i++)
	{
		::g.master.AddMaster(::DEFAULT_MASTERS[i]);
	}

	odalpapi::BufferedSocket socket;
	::g.master.SetSocket(&socket);

	// Query the masters with the timeout
	::g.master.QueryMasters(::g.masterTimeout, ::g.broadcast, ::g.retries);

	// Get the amount of servers found
	const size_t count = ::g.master.GetServerCount();
	if (count > 0)
	{
		::g.servers.clear();
		::g.servers.resize(count);
	}

	for (size_t i = 0; i < ::g.servers.size(); i++)
	{
		std::string address;
		uint16_t port;
		::g.master.GetServerAddress(i, address, port);
		::g.servers[i].SetAddress(address, port);

		::g.servers[i].SetSocket(&socket);
		::g.servers[i].Query(::g.serverTimeout);
	}
}

int main(int argc, char** argv)
{
	if (!odalpapi::BufferedSocket::InitializeSocketAPI())
	{
		Fl::error("Could not initialize sockets.");
		return 1;
	}
	atexit(odalpapi::BufferedSocket::ShutdownSocketAPI);

	//RefreshServers();

	Fl_Window* w = new MainWindow(640, 480);
	w->show(argc, argv);

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
