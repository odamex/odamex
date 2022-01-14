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

#include <stddef.h>
#include <thread>

#include "concurrentqueue.h"
#include <FL/Fl.H>

#include "db.h"
#include "log.h"
#include "main_window.h"
#include "net_packet.h"
#include "plat.h"

enum class jobSignal_e
{
	NONE,
	QUIT,
	REFRESH_ALL,
};

enum awakeMessage_e
{
	AWAKE_NONE,
	AWAKE_REDRAW_SERVERS,
};

struct workerMessage_t
{
	enum class message_e
	{
		NONE,
		REFRESH_MASTER,
		REFRESH_SERVER,
	};

	message_e msg;
	std::string server;
};

static std::atomic<jobSignal_e> kJobSignal;
static moodycamel::ConcurrentQueue<workerMessage_t> kJobQueue;
static std::vector<std::thread> kThreads;

// Default list of master servers, usually official ones
static const char* DEFAULT_MASTERS[] = {"master1.odamex.net:15000",
                                        "voxelsoft.com:15000"};

static const int MASTER_TIMEOUT = 500;
static const int SERVER_TIMEOUT = 1000;
static const int USE_BROADCAST = false;
static const int QUERY_RETRIES = 2;
static const int WORKER_SLEEP = 50;

/**
 * @brief [WORKER] Get a server list from the master server.
 */
static void WorkerRefreshMaster()
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
		std::string ip;
		uint16_t port;
		master.GetServerAddress(i, ip, port);
		DB_AddServer(AddressCombine(ip, port));
		Log_Debug("Added server %s:%u.\n", ip.c_str(), port);
	}
}

/**
 * @brief Update server info for a particular address.
 */
static void WorkerRefreshServer(const std::string& address)
{
	odalpapi::Server server;
	odalpapi::BufferedSocket socket;

	std::string ip;
	uint16_t port;
	AddressSplit(address, ip, port);

	server.SetSocket(&socket);
	server.SetAddress(ip, port);
	int ok = server.Query(::SERVER_TIMEOUT);
	if (ok)
	{
		DB_AddServerInfo(server);
		Log_Debug("Added server info %s.\n", address, port);
	}
	else
	{
		Log_Debug("Could not update server info for %s.\n", address, port);
	}
}

/**
 * @brief [MAIN] Proc run on Fl::awake.
 */
static void AwakeProc(void* data)
{
	switch ((ptrdiff_t)data)
	{
	case AWAKE_REDRAW_SERVERS:
		::g.mainWindow->redrawServers();
		return;
	default:
		return;
	}
}

/**
 * @brief [WORKER] Main worker thread proc.
 */
static void WorkerProc()
{
	for (;;)
	{
		workerMessage_t work;
		if (kJobQueue.try_dequeue(work))
		{
			switch (work.msg)
			{
			case workerMessage_t::message_e::REFRESH_MASTER:
				WorkerRefreshMaster();
				kJobSignal.store(jobSignal_e::REFRESH_ALL);
				Fl::awake(AwakeProc, (void*)AWAKE_REDRAW_SERVERS);
				break;
			case workerMessage_t::message_e::REFRESH_SERVER:
				WorkerRefreshServer(work.server.c_str());
				Fl::awake(AwakeProc, (void*)AWAKE_REDRAW_SERVERS);
				break;
			default:
				break;
			}
		}

		switch (kJobSignal.load())
		{
		case jobSignal_e::QUIT: {
			return;
		}
		case jobSignal_e::REFRESH_ALL: {
			const uint64_t id = std::hash<std::thread::id>{}(std::this_thread::get_id());
			std::string address;
			if (DB_LockAddressForServerInfo(id, address))
			{
				// Locked an address - let's update it.
				WorkerRefreshServer(address);
				Fl::awake(AwakeProc, (void*)AWAKE_REDRAW_SERVERS);
			}
			else
			{
				// Could not lock a server - we're done.
				kJobSignal.store(jobSignal_e::NONE);
			}
		}
		default:
			break;
		}

		std::this_thread::sleep_for(std::chrono::milliseconds(50));
	}
}

/**
 * @brief [MAIN] Initializes the workers.
 */
void Work_Init()
{
	kJobSignal.store(jobSignal_e::NONE);
	kJobQueue.try_enqueue({workerMessage_t::message_e::REFRESH_MASTER, ""});

	const unsigned threads = std::thread::hardware_concurrency();
	for (uint32_t i = 0; i < threads; i++)
	{
		kThreads.emplace_back(WorkerProc);
	}
}

/**
 * @brief [MAIN] Destroys the workers.
 */
void Work_Deinit()
{
	kJobSignal.store(jobSignal_e::QUIT);
	for (uint32_t i = 0; i < kThreads.size(); i++)
	{
		kThreads[i].join();
	}
}

void Work_RefreshMaster()
{
	kJobQueue.try_enqueue({workerMessage_t::message_e::REFRESH_MASTER, ""});
}

void Work_RefreshServer(const std::string& address)
{
	kJobQueue.try_enqueue({workerMessage_t::message_e::REFRESH_SERVER, address});
}

void Work_RefreshAll()
{
	kJobSignal.store(jobSignal_e::REFRESH_ALL);
}
