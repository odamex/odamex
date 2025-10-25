// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 2006-2025 by The Odamex Team.
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
//
// This file contains significant code from the Go programming language.
//
// Copyright 2009 The Go Authors. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the 3RD-PARTY-LICENSES file.
//
//-----------------------------------------------------------------------------


#include "odamex.h"

#include "m_fileio.h"

#include "w_wad.h"
#include "z_zone.h"
#include "i_system.h"

#include <filesystem>
namespace fs = std::filesystem;

// Simple logging
std::ofstream LOG;

// Simple file based console input
std::ifstream CON;

/**
 * @brief Joins two paths together
*/
std::string M_JoinPath(std::string_view path1, std::string_view path2)
{
	return (fs::path(path1) / path2).string();
}

/**
 * @brief Expand "~" into the user's home directory.
*/
void M_ExpandHomeDir(std::string& path)
{
	if (!path.length())
		return;

	if (path[0] != '~')
		return;

	std::string user;

	size_t slash_pos = path.find_first_of(PATHSEPCHAR);
	size_t end_pos = path.length();

	if (slash_pos == std::string::npos)
		slash_pos = end_pos;

	if (path.length() != 1 && slash_pos != 1)
		user = path.substr(1, slash_pos - 1);

	if (slash_pos != end_pos)
		slash_pos++;

	path = M_GetHomeDir(user) + path.substr(slash_pos, end_pos - slash_pos);
}

/**
 * @brief Check for the existence of a file in a user directory that might
 *       or might not have an extension.
 *
 * @param file Filename to find, which might or might not have an extension.
 * @param ext Extension to append, including the initial period.
 * @return std::string Found path or an empty string if not found.
 */
std::string M_FindUserFileName(const std::string& file, const char* ext)
{
	std::string found = M_GetUserFileName(file);
	if (M_FileExists(found))
	{
		return found;
	}
	else if (ext != NULL)
	{
		found = M_GetUserFileName(std::string(file) + ext);
		if (M_FileExists(found))
		{
			return found;
		}
	}
	return "";
}

/**
 * @brief Convert all path separators into the platform-specific path
 *        separator.
 *
 * @detail Technically, POSIX directories can have back-slashes, but this
 *         function assumes that the path is user input and backslashes
 *         are incredibly uncommon in directory names.
 *
 * @param path Path to mutate.
 */
void M_FixPathSep(std::string& path)
{
	// Use the platform appropriate path separator
	for (auto& c : path)
	{
		if (c == '\\' || c == '/')
		{
			c = PATHSEPCHAR;
		}
	}
}

/**
 * @brief Get the current working directory.
 */
std::string M_GetCWD()
{
#ifdef __SWITCH__
	return "./";
#else
	return fs::current_path().string();
#endif
}


//
// M_FileLength
//
// Returns the length of a file using an open descriptor
SDWORD M_FileLength (FILE *f)
{
	SDWORD CurrentPosition = -1;
	SDWORD FileSize = -1;

    if (f != NULL)
    {
        CurrentPosition = ftell (f);
        fseek (f, 0, SEEK_END);
        FileSize = ftell (f);
        fseek (f, CurrentPosition, SEEK_SET);
    }

	return FileSize;
}


/**
 * @brief Checks to see whether a file exists or not
 *
 * @param filename Filename to check.
 */
bool M_FileExists(const std::string& filename)
{
	return fs::exists(filename);
}

/**
 * @brief Checks to see whether a file exists.  If the exact name does not
 *        exist, try again with the extension.
 *
 * @param filename Filename to check.
 * @param ext Extension to check as a second try, with the initial period.
 */
bool M_FileExistsExt(const std::string& filename, const char* ext)
{
	if (M_FileExists(filename))
	{
		return true;
	}

	if (M_FileExists(filename + ext))
	{
		return true;
	}

	return false;
}

//
// M_WriteFile
//
// Writes a buffer to a new file, if it already exists, the file will be
// erased and recreated with the new contents
bool M_WriteFile(std::string filename, void *source, QWORD length)
{
    FILE *handle;
    QWORD count;

    handle = fopen(filename.c_str(), "wb");

    if (handle == NULL)
	{
		PrintFmt(PRINT_HIGH, "Could not open file {} for writing\n", filename);
		return false;
	}

    count = fwrite(source, 1, length, handle);
    fclose(handle);

	if (count != length)
	{
		PrintFmt(PRINT_HIGH, "Failed while writing to file {}\n", filename);
		return false;
	}

    return true;
}


//
// M_ReadFile
//
// Reads a file, it will allocate storage via Z_Malloc for it and return
// the buffer and the size.
QWORD M_ReadFile(std::string filename, BYTE **buffer)
{
    FILE *handle;
    QWORD count, length;
    BYTE *buf;

    handle = fopen(filename.c_str(), "rb");

	if (handle == NULL)
	{
		PrintFmt(PRINT_HIGH, "Could not open file {} for reading\n", filename);
		return false;
	}

    length = M_FileLength(handle);

    buf = (BYTE *)Z_Malloc (length, PU_STATIC, NULL);
    count = fread(buf, 1, length, handle);
    fclose (handle);

    if (count != length)
	{
		PrintFmt(PRINT_HIGH, "Failed while reading from file {}\n", filename);
		return false;
	}

    *buffer = buf;
    return length;
}

//
// M_AppendExtension
//
// Add an extension onto the end of a filename, returns false if it failed.
// if_needed detects if an extension is not present in path, if it isn't, it is
// added.
// The extension must contain a . at the beginning
bool M_AppendExtension (std::string &filename, std::string extension, bool if_needed)
{
    M_FixPathSep(filename);

	fs::path path(filename);

	if (!path.has_filename())
		return false;

	if (if_needed)
	{
		if (!path.has_extension())
			filename.append(extension);

		return true;
	}

    filename.append(extension);

    return true;
}

//
// M_ExtractFilePath
//
// Extract the path from a filename that includes one
void M_ExtractFilePath(const std::string& filename, std::string &dest)
{
	dest = filename;
	M_FixPathSep(dest);
	dest = fs::path(dest).parent_path().string();
}

//
// M_ExtractFileExtension
//
// Extract the extension of a file, returns false if it can't find
// extension seperator, true if succeeded, the extension is returned in
// dest
// Extension includes the .
bool M_ExtractFileExtension(const std::string& filename, std::string &dest)
{
	if (filename.empty())
		return false;

	fs::path path(filename);

	if (!path.has_extension())
	{
		dest.clear();
		return false;
	}

	dest = fs::path(filename).extension().string();

	return (!dest.empty());
}

//
// M_ExtractFileBase
//
// Extract the base file name from a path string (basefile = filename with no extension)
//
// e.g. /asdf/qwerty.zxc -> qwerty
// 	iuyt.wad -> iuyt
//      hgfd -> hgfd
//
// Note: On windows, text after the last . is considered the extension, so any preceding
// .'s won't be removed
void M_ExtractFileBase (std::string filename, std::string &dest)
{
    M_FixPathSep(filename);

	dest = fs::path(filename).stem().string();
}

//
// M_ExtractFileName
//
// Extract the name of a file from a path (name = filename with extension)
void M_ExtractFileName (std::string filename, std::string &dest)
{
    M_FixPathSep(filename);

	dest = fs::path(filename).filename().string();
}

std::string M_ExtractFileName(const std::string &filename) {
	std::string result;
	M_ExtractFileName(filename, result);
	return result;
}

/**
 * @brief Check to see if a character is a valid path separator.
 *
 * @param ch Character to check.
 *
 * @return True if the character is a path separator, otherwise false.
 */
bool M_IsPathSep(const char ch)
{
	if (ch == PATHSEPCHAR)
	{
		return true;
	}

#if defined(_WIN32)
	// This is not the canonical path separator, but it is valid.
	if (ch == '/')
	{
		return true;
	}
#endif

	return false;
}

/**
 * @brief Clean returns the shortest path name equivalent to path by purely
 *        lexical processing.
 *
 * @param path Path to clean.
 * @return Cleaned-up path.
 */
std::string M_CleanPath(std::string path)
{
	return fs::path(path).lexically_normal().string();
}

std::string M_GetUserFileName(const std::string& file)
{
#ifdef __SWITCH__
	std::string path = file;
	return M_CleanPath(path);
#else
	fs::path path(file);
	// Is absolute path?  If so, stop here.
	if (path.is_absolute())
	{
		return file;
	}

	// Is this an explicitly relative path?  If so, stop here.
	size_t fileLen = file.length();
	if (fileLen >= 2 && file[0] == '.' && M_IsPathSep(file[1]))
	{
		return file;
	}
	else if (fileLen >= 3 && file[0] == '.' && file[1] == '.' && M_IsPathSep(file[2]))
	{
		return file;
	}

	// Direct our path to our write directory.
	path = M_GetWriteDir();

	return (path / file).string();
#endif
}

std::string M_GetWriteSubDir(std::string_view folder)
{
	// Does the folder exist?
	fs::path path = M_GetWriteDir();
	path /= folder;
	try
	{
		fs::create_directory(path);
		return M_CleanPath(path.string());
	}
	catch (const fs::filesystem_error& e)
	{
		I_FatalError("Failed to create directory {}: {}\n", path.string(), e.what());
	}
}

std::string M_GetDownloadDir()
{
	return M_GetWriteSubDir("downloads");
}

std::string M_GetScreenshotDir()
{
	return M_GetWriteSubDir("screenshots");
}

std::string M_GetNetDemoDir()
{
	return M_GetWriteSubDir("netdemos");
}

std::string M_GetScreenshotFileName(const std::string& file, const std::string& altpath)
{
#ifdef __SWITCH__
	fs::path path = file;
#else
	fs::path path;
	if (!altpath.empty())
	{
		std::string cleanedpath = M_CleanPath(altpath);
		M_ExpandHomeDir(cleanedpath);
		path = cleanedpath;
		try
		{
			fs::create_directory(path);
		}
		catch (const fs::filesystem_error&)
		{
			PrintFmt(PRINT_WARNING, "M_GetScreenshotFilename: Could not create screenshot directory {}, falling back to default...", altpath);
			path = M_GetScreenshotDir();
		}
	}
	else
	{
		// Direct our path to our screenshot directory.
		path = M_GetScreenshotDir();
	}
	path /= file;
#endif
	return M_CleanPath(path.string());
}

std::string M_GetNetDemoFileName(const std::string& file, const std::string& altpath)
{
#ifdef __SWITCH__
	fs::path path = file;
#else
	fs::path path;
	if (!altpath.empty())
	{
		std::string cleanedpath = M_CleanPath(altpath);
		M_ExpandHomeDir(cleanedpath);
		path = cleanedpath;
		try
		{
			fs::create_directory(path);
		}
		catch (const fs::filesystem_error&)
		{
			PrintFmt(PRINT_WARNING, "M_GetNetDemoFilename: Could not create netdemo directory {}, falling back to default...", altpath);
			path = M_GetNetDemoDir();
		}
	}
	else
	{
		// Direct our path to our netdemo directory.
		path = M_GetNetDemoDir();
	}
	path /= file;
#endif
	return M_CleanPath(path.string());
}

std::string M_BaseFileSearchDir(std::string dir, const std::string& name,
                                const std::vector<std::string>& exts,
                                const OMD5Hash& hash)
{
	fs::path path(M_CleanPath(dir));
	std::vector<OString> cmp_files;
	for (const auto& ext : exts)
	{
		if (!hash.empty())
		{
			// Filenames with supplied hashes always match first.
			cmp_files.push_back(
			    StdStringToUpper(name + "." + hash.getHexStr().substr(0, 6) + ext));
		}
		cmp_files.push_back(StdStringToUpper(name + ext));
	}

	// denis - list files in the directory of interest, case-desensitize
	// then see if wanted wad is listed

	std::string found;
	std::vector<OString>::iterator found_it = cmp_files.end();
	try {
		for (const auto& entry : fs::directory_iterator(path))
		{
			if (entry.is_directory())
				continue;

			// Not only find a match, but check if it is a better match than we
			// found previously.
			fs::path filename = entry.path().filename();
			OString check = StdStringToUpper(filename.string());
			std::vector<OString>::iterator this_it =
			    std::find(cmp_files.begin(), cmp_files.end(), check);
			if (this_it < found_it)
			{
				const std::string local_file = (path / filename).string();
				const OMD5Hash local_hash = W_MD5(local_file);

				if (hash.empty() || hash == local_hash)
				{
					// Found a match.
					found = filename.string();
					found_it = this_it;
					if (found_it == cmp_files.begin())
					{
						// Found the best possible match, we're done.
						break;
					}
				}
				else if (!hash.empty())
				{
					PrintFmt(PRINT_WARNING, "WAD at {} does not match required copy\n", local_file);
					PrintFmt(PRINT_WARNING, "Local MD5: {}\n", local_hash.getHexStr());
					PrintFmt(PRINT_WARNING, "Required MD5: {}\n\n", hash.getHexStr());
				}
			}
		}
	}
	catch (const fs::filesystem_error& e)
	{
		// Probably called on a directory that doesn't exist (e.g. Steam Final Doom directory).
		PrintFmt(PRINT_HIGH, "{}: {}\n", __FUNCTION__, e.what());
	}

	return found;
}

std::vector<std::string> M_BaseFilesScanDir(std::string dir, std::vector<OString> files)
{
	std::vector<std::string> rvo;

	// Fix up parameters.
	fs::path path(M_CleanPath(dir));
	for (auto& file : files)
	{
		file = StdStringToUpper(file);
	}

	try
	{
		for (const auto& entry : fs::directory_iterator(path))
		{
			// Skip directories.
			if (entry.is_directory())
				continue;

			// Find the file.
			std::string filename = entry.path().filename().string();
			std::vector<OString>::iterator it =
		    	std::find(files.begin(), files.end(), StdStringToUpper(filename));

			if (it == files.end())
				continue;

			rvo.push_back(filename);
		}
	}
	catch (const fs::filesystem_error& e)
	{
		// Probably called on a directory that doesn't exist (e.g. Steam Final Doom directory).
		PrintFmt(PRINT_HIGH, "{}: {}\n", __FUNCTION__, e.what());
	}

	return rvo;
}

// Scan for PWADs and DEH and BEX files
std::vector<std::string> M_PWADFilesScanDir(std::string dir)
{
	std::vector<std::string> rvo;

	// Fix up parameters.
	fs::path path(M_CleanPath(dir));

	try
	{
		for (const auto& entry : fs::directory_iterator(path))
		{
			// Skip directories.
			if (entry.is_directory())
				continue;

			// Only return files with correct extensions
			const std::string check = entry.path().extension().string();
			if (iequals(check, ".WAD") || iequals(check, ".DEH") || iequals(check, ".BEX"))
			{
				rvo.push_back(entry.path().filename().string());
			}
		}
	}
	catch (const fs::filesystem_error& e)
	{
		// Probably called on a directory that doesn't exist (e.g. Steam Final Doom directory).
		PrintFmt(PRINT_HIGH, "{}: {}\n", __FUNCTION__, e.what());
	}

	return rvo;
}

bool M_GetAbsPath(const std::string& path, std::string& out)
{
#ifdef __SWITCH__
	out = path;
	return true;
#else
	try
	{
		out = fs::absolute(path).string();
		return true;
	}
	catch (const fs::filesystem_error& e)
	{
		PrintFmt(PRINT_HIGH, "{}: {}\n", __FUNCTION__, e.what());
		return false;
	}
#endif
}

VERSION_CONTROL(m_fileio_cpp, "$Id$")
