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
#include "cl_main.h"
#include "cmdlib.h"
#include "doomstat.h"
#include "i_system.h"
#include "m_argv.h"
#include "m_fileio.h"
#include "otransfer.h"

EXTERN_CVAR(cl_waddownloaddir)
EXTERN_CVAR(waddirs)

namespace http
{

enum States
{
	STATE_SHUTDOWN,
	STATE_READY,
	STATE_CHECKING,
	STATE_DOWNLOADING
};

static struct DownloadState
{
  private:
	DownloadState(const DownloadState&);

  public:
	States state;
	OTransferCheck* check;
	OTransfer* transfer;
	std::string url;
	std::string filename;
	std::string hash;
	std::string checkfilename;
	int checkfails;
	DownloadState()
	    : state(STATE_SHUTDOWN), check(NULL), transfer(NULL), url(""), filename(""),
	      hash(""), checkfilename(""), checkfails(0)
	{
	}
	void Ready()
	{
		this->state = STATE_READY;
		delete this->check;
		this->check = NULL;
		delete this->transfer;
		this->transfer = NULL;
		this->url = "";
		this->filename = "";
		this->hash = "";
		this->checkfilename = "";
		this->checkfails = 0;
	}
} g_state;

/**
 * @brief Init the HTTP download system.
 */
void Init()
{
	Printf(PRINT_HIGH, "http::Init: Init HTTP subsystem (libcurl %d.%d.%d)\n",
	       LIBCURL_VERSION_MAJOR, LIBCURL_VERSION_MINOR, LIBCURL_VERSION_PATCH);

	curl_global_init(CURL_GLOBAL_ALL);

	g_state.state = STATE_READY;
}

/**
 * @brief Shutdown the HTTP download system completely.
 */
void Shutdown()
{
	if (g_state.state == STATE_SHUTDOWN)
		return;

	delete g_state.check;
	g_state.check = NULL;
	delete g_state.transfer;
	g_state.transfer = NULL;

	curl_global_cleanup();
	g_state.state = STATE_SHUTDOWN;
}

/**
 * @brief Get the current state of any file transfer.
 *
 * @return true if there is a download in progress.
 */
bool IsDownloading()
{
	return g_state.state == STATE_CHECKING || g_state.state == STATE_DOWNLOADING;
}

/**
 * @brief Start a transfer.
 *
 * @param website Website to download from, without the WAD at the end.
 * @param filename Filename of the WAD to download.
 * @param hash Hash of the file to download.
 */
void StartDownload(const std::string& website, const std::string& filename,
                   const std::string& hash)
{
	if (g_state.state != STATE_READY)
		return;

	std::string url = website;

	// Add a slash to the end of the base website.
	if (*(url.rbegin()) != '/')
		url.push_back('/');

	g_state.url = url;
	g_state.filename = filename;
	g_state.hash = hash;

	// Start the checking bit on the next tick.
	g_state.state = STATE_CHECKING;
}

/**
 * @brief Called after a check is done.
 *
 * @param info Completed check info.
 */
static void CheckDone(const OTransferInfo& info)
{
	// Found the file, download it next tick.
	g_state.state = STATE_DOWNLOADING;
	g_state.url = info.url;

	Printf(PRINT_HIGH, "Found file at %s.\n", info.url);
}

/**
 * @brief Called after a check bails out.
 *
 * @param msg Error message.
 */
static void CheckError(const char* msg)
{
	// That's a strike.
	g_state.checkfails += 1;

	delete g_state.check;
	g_state.check = NULL;

	// Has our luck run out?
	if (g_state.checkfails >= 3)
	{
		g_state.Ready();
		Printf(PRINT_HIGH, "Could not find %s (%s)...\n", g_state.checkfilename.c_str(),
		       msg);

		if (::gamestate == GS_DOWNLOAD)
			CL_QuitNetGame();
	}
}

static void TickCheck()
{
	if (g_state.check == NULL)
	{
		// Try three different variants of the file.
		g_state.checkfilename = g_state.filename;
		if (g_state.checkfails >= 2)
		{
			// Second strike, try all uppercase.
			g_state.checkfilename = StdStringToUpper(g_state.checkfilename);
		}
		else if (g_state.checkfails == 1)
		{
			// First stirke, try all lowercase.
			g_state.checkfilename = StdStringToLower(g_state.checkfilename);
		}

		// Now we have the full URL.
		std::string fullurl = g_state.url + g_state.checkfilename;

		// Create the check transfer.
		g_state.check = new OTransferCheck(::http::CheckDone, ::http::CheckError);
		g_state.check->setURL(fullurl.c_str());
		if (!g_state.check->start())
		{
			// Failed to start, bail out.
			g_state.Ready();
			return;
		}

		g_state.state = STATE_CHECKING;
		Printf(PRINT_HIGH, "Checking for file at %s...\n", fullurl.c_str());
	}

	// Tick the checker - the done/error callbacks mutate the state appropriately,
	// so we don't need to bother with the return value here.
	g_state.check->tick();
}

/**
 * @brief Construct a list of download directories.
 */
static StringTokens GetDownloadDirs()
{
	StringTokens dirs;

	// Add all of the sources.
	D_AddSearchDir(dirs, cl_waddownloaddir.cstring(), PATHLISTSEPCHAR);
	D_AddSearchDir(dirs, Args.CheckValue("-waddir"), PATHLISTSEPCHAR);
	D_AddSearchDir(dirs, getenv("DOOMWADDIR"), PATHLISTSEPCHAR);
	D_AddSearchDir(dirs, getenv("DOOMWADPATH"), PATHLISTSEPCHAR);
	D_AddSearchDir(dirs, waddirs.cstring(), PATHLISTSEPCHAR);
	dirs.push_back(startdir);
	dirs.push_back(progdir);

	// Clean up all of the directories before deduping them.
	StringTokens::iterator it = dirs.begin();
	for (; it != dirs.end(); ++it)
		*it = M_CleanPath(*it);

	// Dedupe directories.
	dirs.erase(std::unique(dirs.begin(), dirs.end()), dirs.end());
	return dirs;
}

static void TransferDone(const OTransferInfo& info)
{
	Printf(PRINT_HIGH, "Download completed at %ld bytes per second.\n", info.speed);

	if (::gamestate == GS_DOWNLOAD)
	{
		CL_QuitNetGame();
		CL_Reconnect();
	}
}

static void TransferError(const char* msg)
{
	Printf(PRINT_HIGH, "Download error (%s).\n", msg);

	if (::gamestate == GS_DOWNLOAD)
		CL_QuitNetGame();
}

static void TickDownload()
{
	if (g_state.transfer == NULL)
	{
		// Create the transfer.
		g_state.transfer = new OTransfer(::http::TransferDone, ::http::TransferError);
		g_state.transfer->setURL(g_state.url.c_str());

		// Figure out where our destination should be.
		std::string dest;
		StringTokens dirs = GetDownloadDirs();
		for (StringTokens::iterator it = dirs.begin(); it != dirs.end(); ++it)
		{
			// Ensure no path-traversal shenanegins are going on.
			dest = *it + PATHSEP + g_state.filename;
			M_CleanPath(dest);
			if (dest.find(*it) != 0)
			{
				// Something about the filename is trying to escape the
				// download directory.  This is almost certainly malicious.
				::http::TransferError("Saved file tried to escape download directory.\n");
				g_state.Ready();
				return;
			}

			// If the output file was set successfully, escape the loop.
			int err = g_state.transfer->setOutputFile(g_state.filename.c_str());
			if (err == 0)
				break;

			// Otherwise, set the destination to the empty string and try again.
			Printf(PRINT_HIGH, "Could not save to %s (%s)\n", dest.c_str(),
			       strerror(err));
			dest = "";
		}

		if (dest.empty())
		{
			// Found no safe place to write, bail out.
			::http::TransferError("No safe place to save file.\n");
			g_state.Ready();
			return;
		}

		if (!g_state.transfer->start())
		{
			// Failed to start, bail out.
			g_state.Ready();
			return;
		}

		g_state.state = STATE_DOWNLOADING;
		Printf(PRINT_HIGH, "Downloading %s...\n", g_state.url.c_str());
	}

	if (!g_state.transfer->tick())
	{
		// Transfer is done ticking - clean it up.
		g_state.Ready();
	}
}

/**
 * @brief Service the download per-tick.
 */
void Tick()
{
	switch (g_state.state)
	{
	case STATE_CHECKING:
		delete g_state.transfer;
		g_state.transfer = NULL;
		TickCheck();
		break;
	case STATE_DOWNLOADING:
		delete g_state.check;
		g_state.check = NULL;
		TickDownload();
		break;
	default:
		delete g_state.check;
		g_state.check = NULL;
		delete g_state.transfer;
		g_state.check = NULL;
		return;
	}
}

/**
 * @brief Returns a progress string for the console.
 *
 * @return Progress string, or empty string if there is no progress.
 */
std::string Progress()
{
	std::string buffer;

	switch (g_state.state)
	{
	case STATE_CHECKING:
		StrFormat(buffer, "Checking possible locations: %d of 3...",
		          g_state.checkfails + 1);
		break;
	case STATE_DOWNLOADING: {
		if (g_state.transfer == NULL)
		{
			buffer = "Downloading...";
		}
		else
		{
			OTransferProgress progress = g_state.transfer->getProgress();
			StrFormat(buffer, "Downloaded %ld of %ld...", progress.dlnow,
			          progress.dltotal);
		}
		break;
	}
	default:
		break;
	}

	return buffer;
}

} // namespace http

BEGIN_COMMAND(download)
{
	if (argc < 2)
		return;

	std::string outfile = argv[1];
	std::string url = "http://doomshack.org/wads/";
	http::StartDownload(url, outfile, "");
}
END_COMMAND(download)
