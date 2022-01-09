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
//  Worker thread
//
//-----------------------------------------------------------------------------

#include "work_thread.h"

#include "thread.h"
#include <stddef.h>

#include "db.h"
#include "net_packet.h"

// Default list of master servers, usually official ones
static const char* DEFAULT_MASTERS[] = {"master1.odamex.net:15000",
                                        "voxelsoft.com:15000"};

static const int MASTER_TIMEOUT = 500;
static const int SERVER_TIMEOUT = 1000;
static const int USE_BROADCAST = false;
static const int QUERY_RETRIES = 2;

static void RefreshMaster()
{
	odalpapi::MasterServer master;

	for (size_t i = 0; i < ARRAY_LENGTH(::DEFAULT_MASTERS); i++)
	{
		master.AddMaster(::DEFAULT_MASTERS[i]);
	}

	odalpapi::BufferedSocket socket;
	master.SetSocket(&socket);

	// Query the masters with the timeout
	master.QueryMasters(::MASTER_TIMEOUT, ::USE_BROADCAST, ::QUERY_RETRIES);

	// Get the amount of servers found
	for (size_t i = 0; i < master.GetServerCount(); i++)
	{
		std::string address;
		uint16_t port;
		master.GetServerAddress(i, address, port);
		DB_AddServer(address, port);
	}
}

static int WorkerMain(void*)
{
	RefreshMaster();
	return 0;
}

void Work_Init()
{
	thread_ptr_t worker = thread_create(WorkerMain, NULL, "WorkerMain", 8192);
}
