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

void Res_AddPlatformSearchDirs(std::vector<std::string>& search_dirs);
OMD5Hash Res_MD5(const std::string& filename);
OCRC32Sum Res_CRC32(const std::string& filename);

std::string Res_CleanseFilename(const std::string& filename);

std::string Res_FindResourceFile(const std::string& filename, const std::string& hash = "");

std::string Res_FindResourceFile(
		const std::string& filename,
		const char* const extlist[],
		const std::string& hash = "");

std::vector<std::string> Res_GatherResourceFilesFromArgs();
std::vector<std::string> Res_GatherResourceFilesFromString(const std::string& str);

void Res_ValidateResourceFiles(std::vector<std::string>& resource_filenames,
								const std::vector<std::string>& resource_filehashes,
								std::vector<std::string>& missing_resource_filenames,
								std::vector<std::string>& missing_resource_filehashes);
void Res_ValidateResourceFiles(std::vector<std::string>& resource_filenames,
								std::vector<std::string>& missing_resource_filenames);

void Res_AddSearchDir(std::vector<std::string>& search_dirs, const char* dir, const char separator);
