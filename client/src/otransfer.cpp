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

#include "cmdlib.h"
#include "i_system.h"

bool OTransferInfo::hydrate(CURL* curl)
{
	long resCode;
	if (curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &resCode) != CURLE_OK)
		return false;

	curl_off_t speed;
	if (curl_easy_getinfo(curl, CURLINFO_SPEED_DOWNLOAD_T, &speed) != CURLE_OK)
		return false;

	const char* url;
	if (curl_easy_getinfo(curl, CURLINFO_EFFECTIVE_URL, &url) != CURLE_OK)
		return false;

	this->code = resCode;
	this->speed = speed;
	this->url = url;

	return true;
}

// // OTransfer // //

// PRIVATE //

//
// https://curl.haxx.se/libcurl/c/CURLOPT_PROGRESSFUNCTION.html
//
int OTransfer::curlSetProgress(void* thisp, curl_off_t dltotal, curl_off_t dlnow,
                               curl_off_t ultotal, curl_off_t ulnow)
{
	static_cast<OTransfer*>(thisp)->_progress.dltotal = dltotal;
	static_cast<OTransfer*>(thisp)->_progress.dlnow = dlnow;
	return 0;
}

//
// https://curl.haxx.se/libcurl/c/CURLOPT_HEADERFUNCTION.html
//
int OTransfer::curlHeader(char* buffer, size_t size, size_t nitems, void* userdata)
{
	static std::string CONTENT_TYPE = "content-type: ";
	static std::string WANTED_TYPE = "content-type: application/octet-stream";

	if (nitems < 2)
		return nitems;

	// Ensure we're only grabbing binary content types.
	std::string str = StdStringToLower(std::string(buffer, nitems - 2));
	size_t pos = str.find(CONTENT_TYPE);
	if (pos == 0)
	{
		// Found Content-Type, see if it's the correct one.
		size_t pos2 = str.find(WANTED_TYPE);
		if (pos2 != 0)
		{
			// Bzzt, wrong answer.
			return 0;
		}
	}
	return nitems;
}

//
// https://curl.haxx.se/libcurl/c/CURLOPT_DEBUGFUNCTION.html
//
int OTransfer::curlDebug(CURL* handle, curl_infotype type, char* data, size_t size,
                         void* userptr)
{
	return 0;
}

// PUBLIC //

OTransfer::OTransfer(OTransferDoneProc done, OTransferErrorProc err)
    : _doneproc(done), _errproc(err), _curlm(curl_multi_init()), _curl(curl_easy_init()),
      _file(NULL), _progress(OTransferProgress())
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
	curl_easy_setopt(_curl, CURLOPT_URL, src);
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
 * @return True if the transfer started properly.
 */
bool OTransfer::start()
{
	curl_easy_setopt(_curl, CURLOPT_FOLLOWLOCATION, 1L);
	curl_easy_setopt(_curl, CURLOPT_NOPROGRESS, 0L); // turns on xferinfo
	curl_easy_setopt(_curl, CURLOPT_XFERINFOFUNCTION, OTransfer::curlSetProgress);
	curl_easy_setopt(_curl, CURLOPT_XFERINFODATA, this);
	curl_easy_setopt(_curl, CURLOPT_HEADERFUNCTION, OTransfer::curlHeader);
	// curl_easy_setopt(_curl, CURLOPT_VERBOSE, 1L); // turns on debug
	// curl_easy_setopt(_curl, CURLOPT_DEBUGFUNCTION, OTransfer::curlDebug);
	// curl_easy_setopt(_curl, CURLOPT_DEBUGDATA, this);
	curl_multi_add_handle(_curlm, _curl);

	int running;
	curl_multi_perform(_curlm, &running);
	if (running < 1)
	{
		// Transfer isn't running right after we enabled it, error out.
		int queuelen;
		CURLMsg* msg = curl_multi_info_read(_curlm, &queuelen);
		if (msg == NULL)
		{
			_errproc("CURL reports no info");
			return false;
		}

		CURLcode code = msg->data.result;
		if (code != CURLE_OK)
		{
			_errproc(curl_easy_strerror(code));
			return false;
		}

		_errproc("CURL isn't running the transfer with no error");
		return false;
	}

	return true;
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
 * @return True if the transfer is ongoing, false if it's done and the object
 *         should be deleted.
 */
bool OTransfer::tick()
{
	int running;
	curl_multi_perform(_curlm, &running);
	if (running > 0)
		return true;

	// We're done.  Was the transfer successful, or an error?
	int queuelen;
	CURLMsg* msg = curl_multi_info_read(_curlm, &queuelen);
	if (msg == NULL)
	{
		_errproc("CURL reports no info");
		return false;
	}

	CURLcode code = msg->data.result;
	if (code != CURLE_OK)
	{
		_errproc(curl_easy_strerror(code));
		return false;
	}

	// A successful transfer.  Populate the info struct.
	OTransferInfo info = OTransferInfo();
	if (!info.hydrate(_curl))
	{
		_errproc("Info struct could not be populated");
		return false;
	}

	_doneproc(info);
	return false;
}

OTransferProgress OTransfer::getProgress() const
{
	return _progress;
}
