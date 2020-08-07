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

struct TransferInfo
{
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

	/**
	 * @brief Set the source URL of the transfer.
	 *
	 * @param src Source URL, complete with protocol.
	 */
	void setURL(const char* src)
	{
		CURLcode err;
		err = curl_easy_setopt(_curl, CURLOPT_URL, src);
	}

	/**
	 * @brief Set the destination file of the transfer.
	 *
	 * @param dest Destination file path, passed to fopen.
	 * @return True if the output file was set successfully, otherwise false.
	 */
	bool setOutputFile(const char* dest)
	{
		_file = fopen(dest, "wb+");
		if (_file == NULL)
			return false;

		curl_easy_setopt(_curl, CURLOPT_WRITEDATA, _file);
		return true;
	}

	/**
	 * @brief Start the transfer.
	 *
	 * @return True if the transfer was started, or false if the transfer
	 *         stopped after the initial perform.
	 */
	bool start()
	{
		curl_easy_setopt(_curl, CURLOPT_XFERINFOFUNCTION, Transfer::curlSetProgress);
		curl_easy_setopt(_curl, CURLOPT_XFERINFODATA, this);
		curl_easy_setopt(_curl, CURLOPT_DEBUGFUNCTION, Transfer::curlDebug);
		curl_easy_setopt(_curl, CURLOPT_DEBUGDATA, this);
		curl_multi_add_handle(_curlm, _curl);

		int running;
		curl_multi_perform(_curlm, &running);
		return running > 0;
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

	void getInfo()
	{
		long resCode;
		curl_easy_getinfo(_curl, CURLINFO_RESPONSE_CODE, &resCode);
	}

	int getMessage(CURLcode* code)
	{
		int queuelen;
		CURLMsg* msg = curl_multi_info_read(_curlm, &queuelen);
		if (msg == NULL)
			return 0;

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

	::http::transfer = new Transfer();
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

	TransferProgress prog = ::http::transfer->getProgress();
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
