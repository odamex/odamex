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
	std::string url;
	std::string contentType;

	OTransferInfo() : code(0), speed(0), url(""), contentType("")
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
	OTransferDoneProc m_doneProc;
	OTransferErrorProc m_errorProc;
	CURLM* m_curlm;
	CURL* m_curl;

	OTransferCheck(const OTransferCheck&);
	static size_t curlWrite(void* data, size_t size, size_t nmemb, void* userp);

  public:
	OTransferCheck(OTransferDoneProc done, OTransferErrorProc err)
	    : m_doneProc(done), m_errorProc(err), m_curlm(curl_multi_init()),
	      m_curl(curl_easy_init())
	{
	}

	~OTransferCheck()
	{
		curl_multi_remove_handle(m_curlm, m_curl);
		curl_easy_cleanup(m_curl);
		curl_multi_cleanup(m_curlm);
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
	OTransferDoneProc m_doneProc;
	OTransferErrorProc m_errorProc;
	CURLM* m_curlm;
	CURL* m_curl;
	FILE* m_file;
	OTransferProgress m_progress;
	std::string m_filename;
	std::string m_filePart;
	std::string m_expectHash;
	bool m_shouldCheckAgain;

	OTransfer(const OTransfer&);
	static int curlProgress(void* clientp, double dltotal, double dlnow, double ultotal,
	                        double ulnow);

  public:
	OTransfer(OTransferDoneProc done, OTransferErrorProc err)
	    : m_doneProc(done), m_errorProc(err), m_curlm(curl_multi_init()),
	      m_curl(curl_easy_init()), m_file(NULL), m_progress(OTransferProgress()),
	      m_filename(""), m_filePart(""), m_expectHash(""), m_shouldCheckAgain(true)
	{
	}

	~OTransfer()
	{
		if (m_file != NULL)
			fclose(m_file);
		curl_multi_remove_handle(m_curlm, m_curl);
		curl_easy_cleanup(m_curl);
		curl_multi_cleanup(m_curlm);

		// Delete partial file if it exists.
		if (m_filePart.length() > 0)
			remove(m_filePart.c_str());
	}

	void setURL(const std::string& src);
	int setOutputFile(const std::string& dest);
	void setHash(const std::string& hash);
	bool start();
	void stop();
	bool tick();
	bool shouldCheckAgain() const;
	std::string getFilename() const;
	OTransferProgress getProgress() const;
};

#endif
