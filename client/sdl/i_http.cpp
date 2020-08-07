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

namespace http
{

struct TransferProgress
{
	ptrdiff_t dltotal;
	ptrdiff_t dlnow;
	TransferProgress() : dltotal(0), dlnow(0)
	{
	}
};

class Transfer
{
	CURLM* _curlm;
	CURL* _curl;
	FILE* _file;
	TransferProgress _progress;

	Transfer(const Transfer&);

	//
	// https://curl.haxx.se/libcurl/c/CURLOPT_PROGRESSFUNCTION.html
	//
	static void curlSetProgress(void* thisp, curl_off_t dltotal, curl_off_t dlnow,
	                            curl_off_t ultotal, curl_off_t ulnow)
	{
		Printf(PRINT_HIGH, "%s\n", __FUNCTION__);
		static_cast<Transfer*>(thisp)->_progress.dltotal = dltotal;
		static_cast<Transfer*>(thisp)->_progress.dlnow = dlnow;
	}

	//
	// https://curl.haxx.se/libcurl/c/CURLOPT_DEBUGFUNCTION.html
	//
	static void curlDebug(CURL* handle, curl_infotype type, char* data, size_t size,
	                      void* userptr)
	{
		Printf(PRINT_HIGH, "%s\n", __FUNCTION__);
	}

  public:
	Transfer()
	    : _curlm(curl_multi_init()), _curl(curl_easy_init()), _file(NULL),
	      _progress(TransferProgress())
	{
	}

	~Transfer()
	{
		if (_file != NULL)
			fclose(_file);
		curl_multi_remove_handle(_curlm, _curl);
		curl_easy_cleanup(_curl);
		curl_multi_cleanup(_curlm);
	}

	void setSource(const char* src)
	{
		curl_easy_setopt(_curl, CURLOPT_URL, src);
		curl_easy_setopt(_curl, CURLOPT_WRITEDATA, _file);
	}

	bool setDestination(const char* dest)
	{
		_file = fopen("udm3.wad", "wb+");
		if (_file == NULL)
			return false;
	}

	void start()
	{
		curl_easy_setopt(_curl, CURLOPT_XFERINFOFUNCTION, curlSetProgress);
		curl_easy_setopt(_curl, CURLOPT_XFERINFODATA, this);
		curl_easy_setopt(_curl, CURLOPT_DEBUGFUNCTION, curlDebug);
		curl_easy_setopt(_curl, CURLOPT_DEBUGDATA, this);
		curl_multi_add_handle(_curlm, _curl);
	}

	void stop()
	{
		curl_multi_remove_handle(_curlm, _curl);
	}

	int tick()
	{
		int running;
		curl_multi_perform(_curlm, &running);
		return running;
	}

	int getMessage(CURLcode* code)
	{
		int queuelen;
		CURLMsg* msg = curl_multi_info_read(_curlm, &queuelen);
		*code = msg->data.result;
		return queuelen;
	}

	TransferProgress getProgress() const
	{
		return _progress;
	}
};

static Transfer* transfer = NULL;

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

void Tick()
{
	if (::http::transfer)
	{
		int running = ::http::transfer->tick();
		TransferProgress prog = ::http::transfer->getProgress();
		Printf(PRINT_HIGH, "Transfer is active (%d, %zd, %zd) ", running, prog.dlnow,
		       prog.dltotal);

		CURLcode code;
		int messages = ::http::transfer->getMessage(&code);
		Printf(PRINT_HIGH, "%d, %d\n", messages, code);
	}
}

/**
 * Start a transfer.
 */
void Download()
{
	if (::http::state != STATE_READY)
		return;

	::http::state = STATE_DOWNLOADING;

	::http::transfer = new Transfer();
	::http::transfer->setSource("https://doomshack.org/wads/udm3.wad");
	::http::transfer->setDestination("udm3.wad");
	::http::transfer->start();
}

void Cancel()
{
}

} // namespace http

BEGIN_COMMAND(download)
{
	::http::Download();
}
END_COMMAND(download)
