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
//	Encapsulates an HTTP transfer.
//
//-----------------------------------------------------------------------------

#include "otransfer.h"

#include "i_system.h"

// PRIVATE //

//
// https://curl.haxx.se/libcurl/c/CURLOPT_PROGRESSFUNCTION.html
//
void OTransfer::curlSetProgress(void* thisp, curl_off_t dltotal, curl_off_t dlnow,
                                curl_off_t ultotal, curl_off_t ulnow)
{
	Printf(PRINT_HIGH, "%s\n", __FUNCTION__);
	static_cast<OTransfer*>(thisp)->_progress.dltotal = dltotal;
	static_cast<OTransfer*>(thisp)->_progress.dlnow = dlnow;
}

//
// https://curl.haxx.se/libcurl/c/CURLOPT_DEBUGFUNCTION.html
//
void OTransfer::curlDebug(CURL* handle, curl_infotype type, char* data, size_t size,
                          void* userptr)
{
	Printf(PRINT_HIGH, "%s\n", __FUNCTION__);
}

// PUBLIC //

OTransfer::OTransfer()
    : _curlm(curl_multi_init()), _curl(curl_easy_init()), _file(NULL),
      _progress(OTransferProgress())
{
}

OTransfer::~OTransfer()
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
void OTransfer::setURL(const char* src)
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
bool OTransfer::setOutputFile(const char* dest)
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
bool OTransfer::start()
{
	curl_easy_setopt(_curl, CURLOPT_XFERINFOFUNCTION, OTransfer::curlSetProgress);
	curl_easy_setopt(_curl, CURLOPT_XFERINFODATA, this);
	curl_easy_setopt(_curl, CURLOPT_DEBUGFUNCTION, OTransfer::curlDebug);
	curl_easy_setopt(_curl, CURLOPT_DEBUGDATA, this);
	curl_multi_add_handle(_curlm, _curl);

	int running;
	curl_multi_perform(_curlm, &running);
	return running > 0;
}

/**
 * @brief Cancel the transfer.
 */
void OTransfer::stop()
{
	curl_multi_remove_handle(_curlm, _curl);
}

/**
 * @brief Run one tic of the transfer.
 *
 * @return
 */
int OTransfer::tick()
{
	int running;
	curl_multi_perform(_curlm, &running);
	return running;
}

void OTransfer::getInfo()
{
	long resCode;
	curl_easy_getinfo(_curl, CURLINFO_RESPONSE_CODE, &resCode);
}

int OTransfer::getMessage(CURLcode* code)
{
	int queuelen;
	CURLMsg* msg = curl_multi_info_read(_curlm, &queuelen);
	if (msg == NULL)
		return 0;

	*code = msg->data.result;
	return queuelen;
}

OTransferProgress OTransfer::getProgress() const
{
	return _progress;
}
