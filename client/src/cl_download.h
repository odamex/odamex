// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2006-2024 by The Odamex Team.
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

#pragma once


#include "otransfer.h"
#include "m_resfile.h"

/**
 * @brief Set if the client should reconnect to the last server upon completion
 *        of the download.
 */
#define DL_RECONNECT (1 << 0)

typedef std::vector<std::string> Websites;

void CL_DownloadInit();
void CL_DownloadShutdown();
bool CL_IsDownloading();
bool CL_StartDownload(const Websites& urls, const OWantFile& filename, unsigned flags);
bool CL_StopDownload();
void CL_DownloadTick();
std::string CL_DownloadFilename();
OTransferProgress CL_DownloadProgress();
