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
	OTransferInfo() : code(0), speed(0)
	{
	}
	bool hydrate(CURL* curl);
};

typedef void (*OTransferDoneProc)(const OTransferInfo& info);
typedef void (*OTransferErrorProc)(const char* msg);

class OTransfer
{
	OTransferDoneProc _doneproc;
	OTransferErrorProc _errproc;
	CURLM* _curlm;
	CURL* _curl;
	FILE* _file;
	OTransferProgress _progress;

	OTransfer(const OTransfer&);

	static void curlSetProgress(void* thisp, curl_off_t dltotal, curl_off_t dlnow,
	                            curl_off_t ultotal, curl_off_t ulnow);
	static void curlDebug(CURL* handle, curl_infotype type, char* data, size_t size,
	                      void* userptr);

  public:
	OTransfer(OTransferDoneProc done, OTransferErrorProc err);
	~OTransfer();
	void setURL(const char* src);
	bool setOutputFile(const char* dest);
	void start();
	void stop();
	bool tick();
	OTransferProgress getProgress() const;
};

#endif
