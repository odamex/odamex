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

#include "cl_download.h"

#define CURL_STATICLIB
#include "curl/curl.h"

#include "c_dispatch.h"
#include "cl_main.h"
#include "cmdlib.h"
#include "doomstat.h"
#include "i_system.h"
#include "m_argv.h"
#include "m_fileio.h"
#include "w_ident.h"

EXTERN_CVAR(cl_waddownloaddir)
EXTERN_CVAR(waddirs)

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
	unsigned flags;
	Websites checkurls;
	size_t checkurlidx;
	std::string checkfilename;
	int checkfails;
	DownloadState()
	    : state(STATE_SHUTDOWN), check(NULL), transfer(NULL), url(""), filename(""),
	      hash(""), flags(0), checkurls(), checkurlidx(0), checkfilename(""),
	      checkfails(0)
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
		this->flags = 0;
		this->checkurls.clear();
		this->checkurlidx = 0;
		this->checkfilename = "";
		this->checkfails = 0;
	}
} dlstate;

/**
 * @brief Init the HTTP download system.
 */
void CL_DownloadInit()
{
	Printf("CL_DownloadInit: Init HTTP subsystem (libcurl %d.%d.%d)\n",
	       LIBCURL_VERSION_MAJOR, LIBCURL_VERSION_MINOR, LIBCURL_VERSION_PATCH);

	curl_global_init(CURL_GLOBAL_ALL);

	::dlstate.state = STATE_READY;
}

/**
 * @brief Shutdown the HTTP download system completely.
 */
void CL_DownloadShutdown()
{
	if (::dlstate.state == STATE_SHUTDOWN)
		return;

	delete ::dlstate.check;
	::dlstate.check = NULL;
	delete ::dlstate.transfer;
	::dlstate.transfer = NULL;

	curl_global_cleanup();
	::dlstate.state = STATE_SHUTDOWN;
}

/**
 * @brief Get the current state of any file transfer.
 *
 * @return true if there is a download in progress.
 */
bool CL_IsDownloading()
{
	return ::dlstate.state == STATE_CHECKING || ::dlstate.state == STATE_DOWNLOADING;
}

/**
 * @brief Start a transfer.
 *
 * @param urls Website to download from, without the WAD at the end.
 * @param filename Filename of the WAD to download.
 * @param flags DL_* flags to set for the download process.
 */
bool CL_StartDownload(const Websites& urls, const OWantFile& filename, unsigned flags)
{
	if (::dlstate.state != STATE_READY)
	{
		Printf(PRINT_WARNING, "Can't start download when download state is not ready.\n");
		return false;
	}

	// Remove all URL's that don't look like URL's.
	Websites checkurls;
	Websites::const_iterator wit = urls.begin();
	for (; wit != urls.end(); ++wit)
	{
		// Ensure the URL exists.
		if (wit->empty())
			continue;

		// Ensure that the URL begins with the proper protocol.
		if (wit->find("http://", 0) != 0 && wit->find("https://", 0) != 0)
			continue;

		// Ensure the URL ends with a slash.
		std::string url = *wit;
		if (*(url.rbegin()) != '/')
			url += '/';

		checkurls.push_back(url);
	}

	if (checkurls.empty())
	{
		Printf(PRINT_WARNING, "No sites were provided for download.\n");
		return false;
	}

	if (W_IsFilenameCommercialIWAD(filename.getBasename()))
	{
		Printf(PRINT_WARNING, "Refusing to download commercial IWAD file.\n");
		return false;
	}

	if (W_IsFilehashCommercialIWAD(filename.getWantedHash()))
	{
		Printf(PRINT_WARNING, "Refusing to download renamed commercial IWAD file.\n");
		return false;
	}

	// Add a slash to the end of the base sites.
	::dlstate.checkurls = checkurls;

	// Assign the other params to the download state.
	::dlstate.filename = filename.getBasename();
	::dlstate.hash = filename.getWantedHash();
	::dlstate.flags = flags;

	// Start the checking bit on the next tick.
	::dlstate.state = STATE_CHECKING;
	return true;
}

/**
 * @brief Cancel an in-progress download.
 *
 * @return True if a download was cancelled, otherwise false.
 */
bool CL_StopDownload()
{
	if (!CL_IsDownloading())
		return false;

	::dlstate.Ready();
	return true;
}

/**
 * @brief Called after a check is done.
 *
 * @param info Completed check info.
 */
static void CheckDone(const OTransferInfo& info)
{
	// Found the file, download it next tick.
	::dlstate.state = STATE_DOWNLOADING;
	::dlstate.url = info.url;

	Printf("Found file at %s.\n", info.url.c_str());
}

/**
 * @brief Called after a check bails out.
 *
 * @param msg Error message.
 */
static void CheckError(const char* msg)
{
	// That's a strike.
	::dlstate.checkfails += 1;

	delete ::dlstate.check;
	::dlstate.check = NULL;

	// Three strikes and you're out.
	if (::dlstate.checkfails >= 3)
	{
		Printf(PRINT_WARNING, "Could not find %s at %s (%s)...\n",
		       ::dlstate.checkfilename.c_str(),
		       ::dlstate.checkurls.at(::dlstate.checkurlidx).c_str(), msg);

		// Check the next base URL.
		::dlstate.checkfails = 0;
		::dlstate.checkurlidx += 1;
		if (::dlstate.checkurlidx >= ::dlstate.checkurls.size())
		{
			// No more base URL's to check - our luck has run out.
			Printf(PRINT_WARNING, "Download failed, no sites have %s for download.\n",
			       ::dlstate.checkfilename.c_str());
			::dlstate.Ready();
		}
	}
}

static void TickCheck()
{
	if (::dlstate.check == NULL)
	{
		// Start with our base URL.
		std::string fullurl = ::dlstate.checkurls.at(::dlstate.checkurlidx);

		// Try three different variants of the file.
		::dlstate.checkfilename = ::dlstate.filename;
		if (::dlstate.checkfails >= 2)
		{
			// Second strike, try all uppercase.
			::dlstate.checkfilename = StdStringToUpper(::dlstate.checkfilename);
		}
		else if (::dlstate.checkfails == 1)
		{
			// First stirke, try all lowercase.
			::dlstate.checkfilename = StdStringToLower(::dlstate.checkfilename);
		}

		// Now we have the full URL.
		fullurl += ::dlstate.checkfilename;

		// Create the check transfer.
		::dlstate.check = new OTransferCheck(CheckDone, CheckError);
		::dlstate.check->setURL(fullurl.c_str());
		if (!::dlstate.check->start())
		{
			// Failed to start, bail out.
			::dlstate.Ready();
			return;
		}

		::dlstate.state = STATE_CHECKING;
		Printf("Checking for file at %s...\n", fullurl.c_str());
	}

	// Tick the checker - the done/error callbacks mutate the state appropriately,
	// so we don't need to bother with the return value here.
	::dlstate.check->tick();
}

/**
 * @brief Construct a list of download directories.
 */
static StringTokens GetDownloadDirs()
{
	StringTokens dirs;

	// Add all of the sources.
	D_AddSearchDir(dirs, cl_waddownloaddir.cstring(), PATHLISTSEPCHAR);

		// These folders should only work on PC versions
#ifndef GCONSOLE
	D_AddSearchDir(dirs, Args.CheckValue("-waddir"), PATHLISTSEPCHAR);
	D_AddSearchDir(dirs, getenv("DOOMWADDIR"), PATHLISTSEPCHAR);
	D_AddSearchDir(dirs, getenv("DOOMWADPATH"), PATHLISTSEPCHAR);
#endif

	D_AddSearchDir(dirs, waddirs.cstring(), PATHLISTSEPCHAR);
	dirs.push_back(M_GetUserDir());

#ifdef __SWITCH__
	dirs.push_back("./wads");
#endif

	dirs.push_back(M_GetCWD());

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
	std::string bytes;
	StrFormatBytes(bytes, info.speed);
	Printf("Download completed at %s/s.\n", bytes.c_str());

	if (::dlstate.flags & DL_RECONNECT)
		CL_Reconnect();
}

static void TransferError(const char* msg)
{
	Printf(PRINT_WARNING, "Download error (%s).\n", msg);
}

static void TickDownload()
{
	if (::dlstate.transfer == NULL)
	{
		// Create the transfer.
		::dlstate.transfer = new OTransfer(TransferDone, TransferError);
		::dlstate.transfer->setURL(::dlstate.url.c_str());

		// Figure out where our destination should be.
		std::string dest;
		StringTokens dirs = GetDownloadDirs();
		for (StringTokens::iterator it = dirs.begin(); it != dirs.end(); ++it)
		{
			// Ensure no path-traversal shenanegins are going on.
			dest = *it + PATHSEP + ::dlstate.filename;
			M_CleanPath(dest);
			if (dest.find(*it) != 0)
			{
				// Something about the filename is trying to escape the
				// download directory.  This is almost certainly malicious.
				TransferError("Saved file tried to escape download directory.\n");
				::dlstate.Ready();
				return;
			}

			// If the output file was set successfully, escape the loop.
			int err = ::dlstate.transfer->setOutputFile(dest.c_str());
			if (err == 0)
				break;

			// Otherwise, set the destination to the empty string and try again.
			Printf(PRINT_WARNING, "Could not save to %s (%s)\n", dest.c_str(),
			       strerror(err));
			dest = "";
		}

		if (dest.empty())
		{
			// Found no safe place to write, bail out.
			TransferError("No safe place to save file.\n");
			::dlstate.Ready();
			return;
		}

		// Set our expected hash of the file.
		::dlstate.transfer->setHash(::dlstate.hash);

		if (!::dlstate.transfer->start())
		{
			// Failed to start, bail out.
			::dlstate.Ready();
			return;
		}

		::dlstate.state = STATE_DOWNLOADING;
		Printf("Downloading %s...\n", ::dlstate.url.c_str());
	}

	if (!::dlstate.transfer->tick())
	{
		if (::dlstate.transfer->shouldCheckAgain())
		{
			// Check the next site.
			::dlstate.state = STATE_CHECKING;
			::dlstate.checkfails = 0;
			::dlstate.checkurlidx += 1;
			if (::dlstate.checkurlidx >= ::dlstate.checkurls.size())
			{
				// No more base URL's to check - our luck has run out.
				Printf(PRINT_WARNING, "Download failed, no sites have %s for download.\n",
				       ::dlstate.checkfilename.c_str());
				::dlstate.Ready();
			}
		}
		else
		{
			// Either we are done or encountered an error that is indicitive
			// of an issue that we can't hope to recover from.
			::dlstate.Ready();
		}
	}
}

/**
 * @brief Service the download per-tick.
 */
void CL_DownloadTick()
{
	switch (::dlstate.state)
	{
	case STATE_CHECKING:
		delete ::dlstate.transfer;
		::dlstate.transfer = NULL;
		TickCheck();
		break;
	case STATE_DOWNLOADING:
		delete ::dlstate.check;
		::dlstate.check = NULL;
		TickDownload();
		break;
	default:
		delete ::dlstate.check;
		::dlstate.check = NULL;
		delete ::dlstate.transfer;
		::dlstate.check = NULL;
		return;
	}
}

/**
 * @brief Returns a transfer filename for use by the console.
 *
 * @return Filename being transferred.
 */
std::string CL_DownloadFilename()
{
	if (::dlstate.state != STATE_DOWNLOADING)
		return std::string("");

	if (::dlstate.transfer == NULL)
		return std::string("");

	return ::dlstate.transfer->getFilename();
}

/**
 * @brief Returns a transfer progress object for use by the console.
 *
 * @return Progress object.
 */
OTransferProgress CL_DownloadProgress()
{
	if (::dlstate.state != STATE_DOWNLOADING)
		return OTransferProgress();

	if (::dlstate.transfer == NULL)
		return OTransferProgress();

	return ::dlstate.transfer->getProgress();
}

EXTERN_CVAR(cl_downloadsites)

static void DownloadHelp()
{
	Printf("download - Downloads a WAD file\n\n"
	       "Usage:\n"
	       "  ] download get <FILENAME>\n"
	       "  Downloads the file FILENAME from your configured download sites.\n"
	       "  ] download stop\n"
	       "  Stop an in-progress download.");
}

BEGIN_COMMAND(download)
{
	if (argc < 2)
	{
		DownloadHelp();
		return;
	}

	if (stricmp(argv[1], "get") == 0 && argc >= 3)
	{
		Websites clientsites = TokenizeString(cl_downloadsites.str(), " ");

		// Shuffle the sites so we evenly distribute our requests.
		std::random_shuffle(clientsites.begin(), clientsites.end());

		// Attach the website to the file and download it.
		OWantFile file;
		OWantFile::make(file, argv[2], OFILE_UNKNOWN);
		CL_StartDownload(clientsites, file, 0);
		return;
	}

	if (stricmp(argv[1], "stop") == 0)
	{
		if (CL_StopDownload())
			Printf(PRINT_WARNING, "Download cancelled.\n");

		return;
	}

	DownloadHelp();
}
END_COMMAND(download)

VERSION_CONTROL(cl_download_cpp, "$Id$")
