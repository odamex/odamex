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

#ifndef __OTRANSFER_H__
#define __OTRANSFER_H__

#include <stddef.h>
#include <string>

#define CURL_STATICLIB
#include "curl/curl.h"

struct OTransferProgress
{
	ptrdiff_t dltotal;
	ptrdiff_t dlnow;

	OTransferProgress() : dltotal(0), dlnow(0)
	{
	}
};

struct OTransferInfo
{
	int code;
	curl_off_t speed;
	const char* url;

	OTransferInfo() : code(0), speed(0)
	{
	}
	bool hydrate(CURL* curl);
};

typedef void (*OTransferDoneProc)(const OTransferInfo& info);
typedef void (*OTransferErrorProc)(const char* msg);

/**
 * @brief Encapsulates an HTTP check to see if a specific remote file exists.
 */
class OTransferCheck
{
	OTransferDoneProc _doneproc;
	OTransferErrorProc _errproc;
	CURLM* _curlm;
	CURL* _curl;

	OTransferCheck(const OTransferCheck&);
	static size_t curlWrite(void* data, size_t size, size_t nmemb, void* userp);
	static size_t curlHeader(char* buffer, size_t size, size_t nitems, void* userdata);

  public:
	OTransferCheck(OTransferDoneProc done, OTransferErrorProc err)
	    : _doneproc(done), _errproc(err), _curlm(curl_multi_init()),
	      _curl(curl_easy_init())
	{
	}

	~OTransferCheck()
	{
		curl_multi_remove_handle(_curlm, _curl);
		curl_easy_cleanup(_curl);
		curl_multi_cleanup(_curlm);
	}

	void setURL(const std::string& src);
	bool start();
	void stop();
	bool tick();
};

/**
 * @brief Encapsulates an HTTP transfer of a remote file to a local file.
 */
class OTransfer
{
	OTransferDoneProc _doneproc;
	OTransferErrorProc _errproc;
	CURLM* _curlm;
	CURL* _curl;
	FILE* _file;
	OTransferProgress _progress;
	std::string _filename;
	std::string _filepart;

	OTransfer(const OTransfer&);
	static int curlProgress(void* thisp, curl_off_t dltotal, curl_off_t dlnow,
	                        curl_off_t ultotal, curl_off_t ulnow);
	static size_t curlHeader(char* buffer, size_t size, size_t nitems, void* userdata);
	static int curlDebug(CURL* handle, curl_infotype type, char* data, size_t size,
	                     void* userptr);

  public:
	OTransfer(OTransferDoneProc done, OTransferErrorProc err)
	    : _doneproc(done), _errproc(err), _curlm(curl_multi_init()),
	      _curl(curl_easy_init()), _file(NULL), _progress(OTransferProgress()),
	      _filename(""), _filepart("")
	{
	}

	~OTransfer()
	{
		if (_file != NULL)
			fclose(_file);
		curl_multi_remove_handle(_curlm, _curl);
		curl_easy_cleanup(_curl);
		curl_multi_cleanup(_curlm);

		// Delete partial file if it exists.
		if (_filepart.length() > 0)
			remove(_filepart.c_str());
	}

	void setURL(const std::string& src);
	bool setOutputFile(const std::string& dest);
	bool start();
	void stop();
	bool tick();
	OTransferProgress getProgress() const;
};

#endif
