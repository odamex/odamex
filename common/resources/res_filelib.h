// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2006-2015 by The Odamex Team.
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
//
// Resource file locating functions
//
//-----------------------------------------------------------------------------

#pragma once

#include <string>
#include <vector>

#include "ohash.h"

void Res_AddPlatformSearchDirs(std::vector<std::string>& search_dirs);
OMD5Hash Res_MD5(const std::string& filename);
OCRC32Sum Res_CRC32(const std::string& filename);

std::string Res_CleanseFilename(const std::string& filename);

//
// What to do about a search directory that does not exist. A path the user
// asked for is worth a warning; the speculative platform paths are not.
//
enum struct missing_dir_policy_t
{
	SILENT,
	WARN,
	DEV_WARN,
};

// Appends one directory.
void Res_AddSearchDir(std::vector<std::string>& search_dirs, const char* dir,
                      missing_dir_policy_t policy = missing_dir_policy_t::SILENT);
void Res_AddSearchDir(std::vector<std::string>& search_dirs, std::string dir,
                      missing_dir_policy_t policy = missing_dir_policy_t::SILENT);

// Appends every directory in a SEARCHPATHSEPCHAR-separated list.
void Res_AddSearchDirList(std::vector<std::string>& search_dirs, const char* dirs,
                          missing_dir_policy_t policy = missing_dir_policy_t::SILENT);
void Res_AddSearchDirList(std::vector<std::string>& search_dirs, std::string dirs,
                          missing_dir_policy_t policy = missing_dir_policy_t::SILENT);
