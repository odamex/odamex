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

#ifndef __CL_DOWNLOAD_H__
#define __CL_DOWNLOAD_H__

#include <string>

void CL_DownloadInit();
void CL_DownloadShutdown();
bool CL_IsDownloading();
bool CL_StartDownload(const std::string& website, const std::string& filename,
                      const std::string& hash);
void CL_DownloadTick();
std::string CL_DownloadProgress();

#endif
