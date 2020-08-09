// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
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
//	HTTP Downloading.
//
//-----------------------------------------------------------------------------

#include "cl_http.h"

#define CURL_STATICLIB
#include "curl/curl.h"

#include "c_dispatch.h"
#include "i_system.h"
#include "otransfer.h"

namespace http
{

enum States
{
	STATE_SHUTDOWN,
	STATE_READY,
	STATE_DOWNLOADING
};

static OTransfer* transfer = NULL;

static States state = STATE_SHUTDOWN;

static void TransferDone(const OTransferInfo& info)
{
	Printf(PRINT_HIGH, "Download complete (%d from %s @ %d bytes per second).\n", info.code, info.url, info.speed);
}

static void TransferError(const char* msg)
{
	Printf(PRINT_HIGH, "Download error (%s).\n", msg);
}

/**
 * Start a transfer.
 */
static void Download()
{
	if (::http::state != STATE_READY)
		return;

	::http::transfer = new OTransfer(::http::TransferDone, ::http::TransferError);
	::http::transfer->setURL("http://doomshack.org/wads/udm3.wad");
	::http::transfer->setOutputFile("udm3.wad");

	::http::transfer->start();
	::http::state = STATE_DOWNLOADING;
}

void Init()
{
	Printf(PRINT_HIGH, "http::Init: Init HTTP subsystem (libcurl %d.%d.%d)\n",
	       LIBCURL_VERSION_MAJOR, LIBCURL_VERSION_MINOR, LIBCURL_VERSION_PATCH);

	curl_global_init(CURL_GLOBAL_ALL);

	::http::state = STATE_READY;
}

void Shutdown()
{
	if (::http::state == STATE_SHUTDOWN)
		return;

	delete ::http::transfer;
	::http::transfer = NULL;

	curl_global_cleanup();
	::http::state = STATE_SHUTDOWN;
}

void Tick()
{
	if (::http::state != STATE_DOWNLOADING)
		return;

	if (::http::transfer == NULL)
		return;

	if (!::http::transfer->tick())
	{
		// Transfer is done ticking - clean it up and prepare for the next one.
		delete ::http::transfer;
		::http::transfer = NULL;
		::http::state = STATE_READY;
		return;
	}

	// Progress is ongoing, print some information about it.
	OTransferProgress progress = ::http::transfer->getProgress();
	Printf(PRINT_HIGH, "Downloaded %d of %d...\n", progress.dlnow, progress.dltotal);
}

} // namespace http

BEGIN_COMMAND(download)
{
	http::Download();
}
END_COMMAND(download)
