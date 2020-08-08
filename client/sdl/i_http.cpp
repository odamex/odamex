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

#include "i_http.h"

#define CURL_STATICLIB
#include "curl/curl.h"

#include "c_dispatch.h"
#include "i_system.h"
#include "otransfer.h"

namespace http
{

static OTransfer* transfer = NULL;

enum States
{
	STATE_SHUTDOWN,
	STATE_READY,
	STATE_DOWNLOADING
};

static States state = STATE_SHUTDOWN;

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

/**
 * @brief Gets the latest curl error message.
 *
 * @return Returns a pointer to a static string with an error message, or
 *         NULL if there was no error.
 */
const char* GetCURLMessage()
{
	CURLcode msg = CURLE_OK;
	::http::transfer->getMessage(&msg);
	if (msg == CURLE_OK)
		return NULL;
	return curl_easy_strerror(msg);
}

/**
 * Start a transfer.
 */
bool Download()
{
	if (::http::state != STATE_READY)
		return false;

	::http::state = STATE_DOWNLOADING;

	::http::transfer = new OTransfer();
	::http::transfer->setURL("http://doomshack.org/wads/udm3.wad");
	::http::transfer->setOutputFile("udm3.wad");

	bool started = ::http::transfer->start();
}

void Tick()
{
	if (::http::state != STATE_DOWNLOADING)
		return;

	if (::http::transfer == NULL)
		return;

	int running = ::http::transfer->tick();
	if (running < 1)
	{
		// Was the download successful?
		const char* msg = GetCURLMessage();
		if (msg)
			Printf(PRINT_HIGH, "Download finished (%s).\n", msg);
		else
			Printf(PRINT_HIGH, "Download successful.\n");

		// Clean up and prepare for the next one.
		delete ::http::transfer;
		::http::transfer = NULL;
		::http::state = STATE_READY;

		return;
	}

	OTransferProgress prog = ::http::transfer->getProgress();
	Printf(PRINT_HIGH, "Transfer is active (%d, %zd, %zd) ", running, prog.dlnow,
	       prog.dltotal);

	CURLcode code;
	int messages = ::http::transfer->getMessage(&code);
	if (messages > 0)
		Printf(PRINT_HIGH, "%d, %d\n", messages, code);
}

} // namespace http

BEGIN_COMMAND(download)
{
	if (::http::Download())
		Printf(PRINT_HIGH, "Download starting...\n");
	else
		Printf(PRINT_HIGH, "Download did not start (%s).\n", ::http::GetCURLMessage());
}
END_COMMAND(download)
