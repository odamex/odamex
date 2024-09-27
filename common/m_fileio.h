// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
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
//	File Input/Output Operations
//
//-----------------------------------------------------------------------------

#pragma once

#include "d_main.h"

#include <fstream>
#include "m_ostring.h"

extern std::ofstream LOG;
extern std::ifstream CON;

void M_ExpandHomeDir(std::string& path);
std::string M_FindUserFileName(const std::string& file, const char* ext);
void M_FixPathSep(std::string& path);
std::string M_GetCWD();

SDWORD M_FileLength (FILE *f);
bool M_FileExists(const std::string& filename);
bool M_FileExistsExt(const std::string& filename, const char* ext);

BOOL M_WriteFile(std::string filename, void *source, QWORD length);
QWORD M_ReadFile(std::string filename, BYTE **buffer);

BOOL M_AppendExtension (std::string &filename, std::string extension, bool if_needed = true);
void M_ExtractFilePath(const std::string& filename, std::string &dest);
bool M_ExtractFileExtension(const std::string& filename, std::string &dest);
void M_ExtractFileBase (std::string filename, std::string &dest);
void M_ExtractFileName (std::string filename, std::string &dest);
std::string M_ExtractFileName(const std::string &filename);
bool M_IsPathSep(const char ch);
std::string M_CleanPath(std::string path);

/*
 * OS-specific functions are below.
 */

/**
 * @brief Get the directory of the Odamex binary.
 */
std::string M_GetBinaryDir();

/**
 * @brief Get the home directory of the passed user - or the current user.
 * 
 * @param user Name of the user to look up, or blank if no user.
 * @return 
 */
std::string M_GetHomeDir(const std::string& user = "");

/**
 * @brief Obtain a user-specific path under their home directory that might
 *        or might not be used as a path to write files into.
 */
std::string M_GetUserDir();

/**
 * @brief Get the directory that files such as game config and screenshots
 *        shall be written into.  If the directory does not exist, it will
 *        be created.  This function also accounts for portable installations.
 */
std::string M_GetWriteDir();

/**
 * @brief Resolve a file name into a user directory.
 * 
 * @detail This function is OS-specific.
 * 
 *         Aboslute and relative paths that begin with "." and ".." should
 *         be resolved relative to the Odamex binary.  Otherwise, the file
 *         is resolved relative to a platform and user specific directory
 *         in their home directory.
 *
 *         The resolution process will create a user home directory if it
 *         doesn't exist, but otherwise this process is blind and does not
 *         consider the existence of the file in question.
 * 
 * @param file Filename to resolve.
 * @return An absolute path pointing to the resolved file.
 */
std::string M_GetUserFileName(const std::string& file);

/**
 * @brief Attempt to find a file in a directory - case insensitive.
 * 
 * @detail This function is OS-specific.
 * 
 * @param dir Directory to search.
 * @param file File to search, without extension.
 * @param exts Extensions to search, including initial dot - must be capitalized.
 * @param hash Optional hash to match against - must be capitalized.
 * @return Filename of the found file, or empty string if not found.
*/
std::string M_BaseFileSearchDir(std::string dir, const std::string& file,
                                const std::vector<std::string>& exts,
                                const OMD5Hash& hash);

/**
 * @brief Attempt to find multiple files in a directory - case insensitive.
 * 
 * @detail Unlike M_BaseFileSearchDir, this scans the entire directory and
 *         doesn't care about hashes or hashed files.
 * 
 * @param dir Directory to search.
 * @param files Files to search, with extension.
 * @return Filenames of any found files.
 */
std::vector<std::string> M_BaseFilesScanDir(std::string dir, std::vector<OString> files);

/**
 * @brief Attempt to find multiple PWAD files in a directory - case insensitive.
 * 
 * @detail Unlike M_BaseFileSearchDir, this scans the entire directory and
 *         doesn't care about hashes or hashed files.
 * 
 * @param dir Directory to search.
 * @return Filenames of any found files.
 */
std::vector<std::string> M_PWADFilesScanDir(std::string dir);

/**
 * @brief Get absolute path from passed path.
 * 
 * @param path Path to make absolute.
 * @param out Resulting path.
 * @return True if the path was made absolute successfully.
 */
bool M_GetAbsPath(const std::string& path, std::string& out);
