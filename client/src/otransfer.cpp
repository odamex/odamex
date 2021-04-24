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
#include "m_fileio.h"
#include "w_ident.h"
#include "w_wad.h"

// // Common callbacks // //

//
// https://curl.haxx.se/libcurl/c/CURLOPT_HEADERFUNCTION.html
//
static size_t curlHeader(char* buffer, size_t size, size_t nitems, void* userdata)
{
	static const std::string CONTENT_TYPE = "content-type: ";
	static const std::string WANTED_TYPES[] = {
	    "content-type: application/octet-stream", // Generic binary file.
	    "content-type: application/x-doom",       // Doom WAD MIME type.
	    "content-type: text/html"                 // Used for redirect pages.
	};

	if (nitems < 2)
		return nitems;

	// Ensure we're only grabbing binary content types.
	std::string str = StdStringToLower(std::string(buffer, nitems - 2));
	size_t pos = str.find(CONTENT_TYPE);
	if (pos == 0)
	{
		// Found Content-Type, see if it's the correct one.
		for (size_t i = 0; i < ARRAY_LENGTH(WANTED_TYPES); i++)
		{
			size_t pos2 = str.find(WANTED_TYPES[i]);
			if (pos2 == 0)
			{
				// Ding, right answer.
				return nitems;
			}
		}

		// Bzzt, wrong answer.
		return 0;
	}

	return nitems;
}

//
// https://curl.haxx.se/libcurl/c/CURLOPT_DEBUGFUNCTION.html
//
static int curlDebug(CURL* handle, curl_infotype type, char* data, size_t size,
                     void* userptr)
{
	std::string str = std::string(data, size);

	switch (type)
	{
	case CURLINFO_TEXT:
		Printf("curl | %s\n", str.c_str());
		break;
	case CURLINFO_HEADER_IN:
		Printf("curl < %s\n", str.c_str());
		break;
	case CURLINFO_HEADER_OUT:
		Printf("curl > %s\n", str.c_str());
		break;
	default:
		// Don't print data/binary SSL stuff.
		break;
	}

	return 0;
}

// // OTransferInfo // //

bool OTransferInfo::hydrate(CURL* curl)
{
	long resCode;
	if (curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &resCode) != CURLE_OK)
		return false;

	double speed;
	if (curl_easy_getinfo(curl, CURLINFO_SPEED_DOWNLOAD, &speed) != CURLE_OK)
		return false;

	const char* url;
	if (curl_easy_getinfo(curl, CURLINFO_EFFECTIVE_URL, &url) != CURLE_OK)
		return false;

	const char* contentType;
	if (curl_easy_getinfo(curl, CURLINFO_CONTENT_TYPE, &contentType) != CURLE_OK)
		return false;

	this->code = resCode;
	this->speed = speed;
	this->url = (url != NULL) ? url : "";
	this->contentType = (contentType != NULL) ? contentType : "";

	return true;
}

// // OTransferExists // //

// PRIVATE //

//
// https://curl.haxx.se/libcurl/c/CURLOPT_WRITEFUNCTION.html
//
size_t OTransferCheck::curlWrite(void* data, size_t size, size_t nmemb, void* userp)
{
	// A HEAD request should never write anything.
	return size * nmemb;
}

// PUBLIC //

/**
 * @brief Set the source URL of the check.
 *
 * @param src Source URL, complete with protocol.
 */
void OTransferCheck::setURL(const std::string& src)
{
	curl_easy_setopt(m_curl, CURLOPT_URL, src.c_str());
}

/**
 * @brief Start the checking transfer.
 *
 * @return True if the transfer started properly.
 */
bool OTransferCheck::start()
{
	curl_easy_setopt(m_curl, CURLOPT_FAILONERROR, 1L);
	curl_easy_setopt(m_curl, CURLOPT_FOLLOWLOCATION, 1L);
	curl_easy_setopt(m_curl, CURLOPT_CONNECTTIMEOUT, 5L);
	curl_easy_setopt(m_curl, CURLOPT_HEADERFUNCTION, curlHeader);
	// curl_easy_setopt(m_curl, CURLOPT_VERBOSE, 1L);
	// curl_easy_setopt(m_curl, CURLOPT_DEBUGFUNCTION, curlDebug);
	curl_easy_setopt(m_curl, CURLOPT_NOBODY, 1L);
	curl_easy_setopt(m_curl, CURLOPT_WRITEFUNCTION, OTransferCheck::curlWrite);
	curl_multi_add_handle(m_curlm, m_curl);

	int running;
	curl_multi_perform(m_curlm, &running);
	if (running < 1)
	{
		// Transfer isn't running right after we enabled it, error out.
		int queuelen;
		CURLMsg* msg = curl_multi_info_read(m_curlm, &queuelen);
		if (msg == NULL)
		{
			m_errorProc("CURL reports no info");
			return false;
		}

		CURLcode code = msg->data.result;
		if (code != CURLE_OK)
		{
			m_errorProc(curl_easy_strerror(code));
			return false;
		}

		m_errorProc("CURL isn't running the transfer with no error");
		return false;
	}

	return true;
}

/**
 * @brief Cancel the transfer.
 */
void OTransferCheck::stop()
{
	curl_multi_remove_handle(m_curlm, m_curl);
}

/**
 * @brief Run one tic of the transfer.
 *
 * @return True if the transfer is ongoing, false if it's done and the object
 *         should be deleted.
 */
bool OTransferCheck::tick()
{
	int running;
	curl_multi_perform(m_curlm, &running);
	if (running > 0)
		return true;

	// We're done.  Was the transfer successful, or an error?
	int queuelen;
	CURLMsg* msg = curl_multi_info_read(m_curlm, &queuelen);
	if (msg == NULL)
	{
		m_errorProc("CURL reports no info");
		return false;
	}

	CURLcode code = msg->data.result;
	if (code != CURLE_OK)
	{
		m_errorProc(curl_easy_strerror(code));
		return false;
	}

	// A successful transfer.  Populate the info struct.
	OTransferInfo info = OTransferInfo();
	if (!info.hydrate(m_curl))
	{
		m_errorProc("Info struct could not be populated");
		return false;
	}

	// Make sure we didn't find an HTML file - those are only okay on redirects.
	if (stricmp(info.contentType.c_str(), "text/html") == 0)
	{
		m_errorProc("Only found an HTML file");
		return false;
	}

	m_doneProc(info);
	return false;
}

// // OTransfer // //

// PRIVATE //

//
// https://curl.haxx.se/libcurl/c/CURLOPT_PROGRESSFUNCTION.html
//
int OTransfer::curlProgress(void* clientp, double dltotal, double dlnow, double ultotal,
                            double ulnow)
{
	static_cast<OTransfer*>(clientp)->m_progress.dltotal = dltotal;
	static_cast<OTransfer*>(clientp)->m_progress.dlnow = dlnow;
	return 0;
}

// PUBLIC //

/**
 * @brief Set the source URL of the transfer.
 *
 * @param src Source URL, complete with protocol.
 */
void OTransfer::setURL(const std::string& src)
{
	curl_easy_setopt(m_curl, CURLOPT_URL, src.c_str());
}

/**
 * @brief Set the destination file of the transfer.
 *
 * @param dest Destination file path, passed to fopen.
 * @return 0 on success, or the value of errno after the failed fopen on failure.
 */
int OTransfer::setOutputFile(const std::string& dest)
{
	// We download to the partial file and move it later.
	m_filename = dest;
	m_filePart = dest + ".part";

	m_file = fopen(m_filePart.c_str(), "wb+");
	if (m_file == NULL)
		return errno;

	curl_easy_setopt(m_curl, CURLOPT_WRITEDATA, m_file);
	return 0;
}

/**
 * Assert a specific file hash for this file.
 *
 * @param hash Hash string.
 */
void OTransfer::setHash(const std::string& hash)
{
	m_expectHash = hash;
}

/**
 * @brief Start the transfer.
 *
 * @return True if the transfer started properly.
 */
bool OTransfer::start()
{
	curl_easy_setopt(m_curl, CURLOPT_FAILONERROR, 1L);
	curl_easy_setopt(m_curl, CURLOPT_FOLLOWLOCATION, 1L);
	curl_easy_setopt(m_curl, CURLOPT_CONNECTTIMEOUT, 5L);
	curl_easy_setopt(m_curl, CURLOPT_NOPROGRESS, 0L); // turns on xferinfo
	curl_easy_setopt(m_curl, CURLOPT_PROGRESSFUNCTION, OTransfer::curlProgress);
	curl_easy_setopt(m_curl, CURLOPT_PROGRESSDATA, this);
	curl_easy_setopt(m_curl, CURLOPT_HEADERFUNCTION, curlHeader);
	// curl_easy_setopt(m_curl, CURLOPT_VERBOSE, 1L);
	// curl_easy_setopt(m_curl, CURLOPT_DEBUGFUNCTION, curlDebug);
	curl_multi_add_handle(m_curlm, m_curl);

	int running;
	curl_multi_perform(m_curlm, &running);
	if (running < 1)
	{
		// Transfer isn't running right after we enabled it, error out.
		int queuelen;
		CURLMsg* msg = curl_multi_info_read(m_curlm, &queuelen);
		if (msg == NULL)
		{
			m_errorProc("CURL reports no info");
			return false;
		}

		CURLcode code = msg->data.result;
		if (code != CURLE_OK)
		{
			m_errorProc(curl_easy_strerror(code));
			return false;
		}

		m_errorProc("CURL isn't running the transfer with no error");
		return false;
	}

	return true;
}

/**
 * @brief Cancel the transfer.
 */
void OTransfer::stop()
{
	curl_multi_remove_handle(m_curlm, m_curl);
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
	curl_multi_perform(m_curlm, &running);
	if (running > 0)
		return true;

	// We're done.  Was the transfer successful, or an error?
	int queuelen;
	CURLMsg* msg = curl_multi_info_read(m_curlm, &queuelen);
	if (msg == NULL)
	{
		m_errorProc("CURL reports no info");
		return false;
	}

	CURLcode code = msg->data.result;
	if (code != CURLE_OK)
	{
		m_errorProc(curl_easy_strerror(code));
		return false;
	}

	// A successful transfer.  Populate the info struct.
	OTransferInfo info = OTransferInfo();
	if (!info.hydrate(m_curl))
	{
		m_errorProc("Info struct could not be populated");
		return false;
	}

	// Make sure we didn't download an HTML file - those are only okay on redirects.
	if (stricmp(info.contentType.c_str(), "text/html") == 0)
	{
		m_errorProc("Accidentally downloaded an HTML file");
		return false;
	}

	// Close the file so we can rename it.
	fclose(m_file);
	m_file = NULL;

	// Verify that the file is what the server wants and is not a renamed
	// commercial IWAD.
	std::string actualHash = W_MD5(m_filePart);
	if (W_IsFilehashCommercialIWAD(actualHash))
	{
		remove(m_filePart.c_str());
		m_errorProc("Accidentally downloaded a commercial IWAD - file removed");
		return false;
	}
	else if (!m_expectHash.empty() && m_expectHash != actualHash)
	{
		remove(m_filePart.c_str());
		m_errorProc(
		    "Downloaded file is not the same as the server's file - file removed");
		return false;
	}

	int ok = rename(m_filePart.c_str(), m_filename.c_str());
	if (ok != 0)
	{
		// See if we can write a file with a partial hash.
		std::string path, base, ext, fallback;
		M_ExtractFilePath(m_filename, path);
		M_ExtractFileBase(m_filename, base);
		if (M_ExtractFileExtension(m_filename, ext))
		{
			ext = std::string(".") + ext;
		}
		StrFormat(fallback, "%s%s%s.%s%s", path.c_str(), PATHSEP, base.c_str(),
		          actualHash.substr(0, 6).c_str(), ext.c_str());

		// Try one more time.
		ok = rename(m_filePart.c_str(), fallback.c_str());
		if (ok != 0)
		{
			// Something is seriously wrong with our writable directory.
			m_shouldCheckAgain = false;

			std::string buf;
			StrFormat(buf, "File %s could not be renamed to %s - %s", m_filePart.c_str(),
			          m_filename.c_str(), strerror(errno));
			m_errorProc(buf.c_str());
			return false;
		}

		Printf("Saved to fallback location \"%s\".\n", fallback.c_str());
	}
	else
	{
		Printf("Saved to location \"%s\".\n", m_filename.c_str());
	}

	m_shouldCheckAgain = false;
	m_doneProc(info);
	return false;
}

bool OTransfer::shouldCheckAgain() const
{
	return m_shouldCheckAgain;
}

std::string OTransfer::getFilename() const
{
	return m_filename;
}

OTransferProgress OTransfer::getProgress() const
{
	return m_progress;
}
