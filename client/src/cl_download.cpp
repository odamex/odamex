// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1998-2006 by Randy Heit (ZDoom).
// Copyright (C) 2000-2006 by Sergey Makovkin (CSDoom .62).
// Copyright (C) 2006-2012 by The Odamex Team.
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
//	File downloads
//
//-----------------------------------------------------------------------------

#include <sstream>
#include <iomanip>

#include "c_dispatch.h"
#include "cmdlib.h"
#include "cl_main.h"
#include "d_main.h"
#include "i_net.h"
#include "md5.h"
#include "m_argv.h"
#include "m_fileio.h"

#ifdef _XBOX
#include "i_xbox.h"
#endif

EXTERN_CVAR (waddirs)

void CL_Reconnect(void);

// GhostlyDeath <October 26, 2008> -- VC6 Compiler Error
// C2552: 'identifier' : non-aggregates cannot be initialized with initializer list
// What does this mean? VC6 considers std::string non-static (that it changes every time?)
// So it complains because std::string has a constructor!

/*struct download_s
{
	public:
		std::string filename;
		std::string md5;
		buf_t *buf;
		unsigned int got_bytes;
} download = { "", "", NULL, 0 };*/

// this works though!
struct download_s
{
	public:
		std::string filename;
		std::string md5;
		buf_t *buf;
		unsigned int got_bytes;

		download_s()
		{
			buf = NULL;
			this->clear();
		}

		~download_s()
		{
		}

		void clear()
		{	
			filename = "";
			md5 = "";
			got_bytes = 0;
        	
			if (buf != NULL)
			{
				delete buf;
				buf = NULL;
			}
		}
} download;


extern std::string DownloadStr;

// Pretty prints supplied byte amount into a string
std::string FormatNBytes(float b)
{
    std::ostringstream osstr;
    float amt;

    if (b >= 1048576)
    {
        amt = b / 1048576;

        osstr << std::setprecision(4) << amt << 'M';
    }
    else
    if (b >= 1024)
    {
        amt = b / 1024;

        osstr << std::setprecision(4) << amt << 'K';
    }
    else
    {
       osstr << std::setprecision(4) << b << 'B';
    }

    return osstr.str();
}

void SetDownloadPercentage(size_t pc)
{
    std::stringstream ss;

    ss << "Downloading " << download.filename << ": " << pc << "%";

    DownloadStr = ss.str();
}

void ClearDownloadProgressBar()
{
    DownloadStr = "";
}


void IntDownloadComplete(void)
{
    std::string actual_md5 = MD5SUM(download.buf->ptr(), download.buf->maxsize());

	Printf(PRINT_HIGH, "\nDownload complete, got %u bytes\n", download.buf->maxsize());
	Printf(PRINT_HIGH, "%s\n %s\n", download.filename.c_str(), actual_md5.c_str());

	if(download.md5 == "")
	{
		Printf(PRINT_HIGH, "Server gave no checksum, assuming valid\n", (int)download.buf->maxsize());
	}
	else if(actual_md5 != download.md5)
	{
		Printf(PRINT_HIGH, " %s on server\n", download.md5.c_str());
		Printf(PRINT_HIGH, "Download failed: bad checksum\n");

		download.clear();
        CL_QuitNetGame();

        ClearDownloadProgressBar();

        return;
    }

    // got the wad! save it!
    std::vector<std::string> dirs;
    std::string filename;
    size_t i;
#ifdef WIN32
    const char separator = ';';
#else
    const char separator = ':';
#endif

    // Try to save to the wad paths in this order -- Hyper_Eye
    D_AddSearchDir(dirs, Args.CheckValue("-waddir"), separator);
    D_AddSearchDir(dirs, getenv("DOOMWADDIR"), separator);
    D_AddSearchDir(dirs, getenv("DOOMWADPATH"), separator);
    D_AddSearchDir(dirs, waddirs.cstring(), separator);
    dirs.push_back(startdir);
    dirs.push_back(progdir);

    dirs.erase(std::unique(dirs.begin(), dirs.end()), dirs.end());

    for(i = 0; i < dirs.size(); i++)
    {
        filename.clear();
        filename = dirs[i];
        if(filename[filename.length() - 1] != PATHSEPCHAR)
            filename += PATHSEP;
        filename += download.filename;

        // check for existing file
   	    if(M_FileExists(filename.c_str()))
   	    {
   	        // there is an existing file, so use a new file whose name includes the checksum
   	        filename += ".";
   	        filename += actual_md5;
   	    }

        if (M_WriteFile(filename, download.buf->ptr(), download.buf->maxsize()))
            break;
    }

    // Unable to write
    if(i == dirs.size())
    {
		download.clear();
        CL_QuitNetGame();
        return;            
    }

    Printf(PRINT_HIGH, "Saved download as \"%s\"\n", filename.c_str());

	download.clear();
    CL_QuitNetGame();
    CL_Reconnect();

    ClearDownloadProgressBar();
}

//
// CL_RequestDownload
// please sir, can i have some more?
//
void CL_RequestDownload(std::string filename, std::string filehash)
{
    // [Russell] - Allow resumeable downloads
	if ((download.filename != filename) ||
        (download.md5 != filehash))
    {
        download.filename = filename;
        download.md5 = filehash;
        download.got_bytes = 0;
    }
	
	// denis todo clear previous downloads
	MSG_WriteMarker(&net_buffer, clc_wantwad);
	MSG_WriteString(&net_buffer, filename.c_str());
	MSG_WriteString(&net_buffer, filehash.c_str());
	MSG_WriteLong(&net_buffer, download.got_bytes);

	NET_SendPacket(net_buffer, serveraddr);

	Printf(PRINT_HIGH, "Requesting download...\n");
	
	// check for completion
	// [Russell] - We go over the boundary, because sometimes the download will
	// pause at 100% if the server disconnected you previously, you can 
	// reconnect a couple of times and this will let the checksum system do its
	// work

	if ((download.buf != NULL) && 
        (download.got_bytes >= download.buf->maxsize()))
	{
        IntDownloadComplete();
	}
}

//
// CL_DownloadStart
// server tells us the size of the file we are about to download
//
void CL_DownloadStart()
{
	DWORD file_len = MSG_ReadLong();

	if(gamestate != GS_DOWNLOAD)
	{
		Printf(PRINT_HIGH, "Server initiated download failed\n");
		return;
	}

	// don't go for more than 100 megs
	if(file_len > 100*1024*1024)
	{
		Printf(PRINT_HIGH, "Download is over 100MiB, aborting!\n");
		CL_QuitNetGame();
		return;
	}
	
    // [Russell] - Allow resumeable downloads
	if (download.got_bytes == 0)
    {
        if (download.buf != NULL)
        {
            delete download.buf;
            download.buf = NULL;
        }
        
        download.buf = new buf_t ((size_t)file_len);
        
        memset(download.buf->ptr(), 0, file_len);
    }
    else
        Printf(PRINT_HIGH, "Resuming download of %s...\n", download.filename.c_str());
    
    
    
	Printf(PRINT_HIGH, "Downloading %s bytes...\n", 
        FormatNBytes(file_len).c_str());

	// Make initial 0% show
	SetDownloadPercentage(0);
}

//
// CL_Download
// denis - get a little chunk of the file and store it, much like a hampster. Well, hamster; but hampsters can dance and sing. Also much like Scraps, the Ice Age squirrel thing, stores his acorn. Only with a bit more success. Actually, quite a bit more success, specifically as in that the world doesn't crack apart when we store our chunk and it does when Scraps stores his (or her?) acorn. But when Scraps does it, it is funnier. The rest of Ice Age mostly sucks.
//
void CL_Download()
{
	DWORD offset = MSG_ReadLong();
	size_t len = MSG_ReadShort();
	size_t left = MSG_BytesLeft();
	void *p = MSG_ReadChunk(len);

	if(gamestate != GS_DOWNLOAD)
		return;

	if (download.buf == NULL)
	{
		// We must have not received the svc_wadinfo message
		Printf(PRINT_HIGH, "Unable to start download, aborting\n");
		download.clear();
		CL_QuitNetGame();
		return;
	}

	// check ranges
	if(offset + len > download.buf->maxsize() || len > left || p == NULL)
	{
		Printf(PRINT_HIGH, "Bad download packet (%d, %d) encountered (%d), aborting\n", (int)offset, (int)left, (int)download.buf->size());
    	
		download.clear();    
		CL_QuitNetGame();
		return;
	}

	// check for missing packet, re-request
	if(offset < download.got_bytes || offset > download.got_bytes)
	{
		DPrintf("Missed a packet after/before %d bytes (got %d), re-requesting\n", download.got_bytes, offset);
		MSG_WriteMarker(&net_buffer, clc_wantwad);
		MSG_WriteString(&net_buffer, download.filename.c_str());
		MSG_WriteString(&net_buffer, download.md5.c_str());
		MSG_WriteLong(&net_buffer, download.got_bytes);
		NET_SendPacket(net_buffer, serveraddr);
		return;
	}

	// send keepalive
	NET_SendPacket(net_buffer, serveraddr);

	// copy into downloaded buffer
	memcpy(download.buf->ptr() + offset, p, len);
	download.got_bytes += len;

	// calculate percentage for the user
	static int old_percent = 0;
	int percent = (download.got_bytes*100)/download.buf->maxsize();
	if(percent != old_percent)
	{
        SetDownloadPercentage(percent);

		old_percent = percent;
	}

	// check for completion
	// [Russell] - We go over the boundary, because sometimes the download will
	// pause at 100% if the server disconnected you previously, you can 
	// reconnect a couple of times and this will let the checksum system do its
	// work
	if(download.got_bytes >= download.buf->maxsize())
	{
        IntDownloadComplete();
	}
}

VERSION_CONTROL (cl_download_cpp, "$Id$")
