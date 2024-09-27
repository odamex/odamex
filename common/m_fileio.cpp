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

#include "z_zone.h"

// unfortunately, we still need you
#include "cmdlib.h"

#if defined(_WIN32)
#include <direct.h> // getcwd
#else
#include <unistd.h> // getcwd
#endif

// Simple logging
std::ofstream LOG;

// Simple file based console input
std::ifstream CON;

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
	for (size_t i = 0; i < path.length(); i++)
	{
		if (path[i] == '\\' || path[i] == '/')
		{
			path[i] = PATHSEPCHAR;
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
#endif

	char tmp[4096] = {0};
	std::string ret = "./";

	const char* cwd = getcwd(tmp, sizeof(tmp));
	if (cwd)
		ret = cwd;

	M_FixPathSep(ret);

	return ret;
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
	FILE* f = fopen(filename.c_str(), "r");
	if (f == NULL)
	{
		return false;
	}

	fclose(f);
	return true;
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
BOOL M_WriteFile(std::string filename, void *source, QWORD length)
{
    FILE *handle;
    QWORD count;

    handle = fopen(filename.c_str(), "wb");

    if (handle == NULL)
	{
		Printf(PRINT_HIGH, "Could not open file %s for writing\n", filename.c_str());
		return false;
	}

    count = fwrite(source, 1, length, handle);
    fclose(handle);

	if (count != length)
	{
		Printf(PRINT_HIGH, "Failed while writing to file %s\n", filename.c_str());
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
		Printf(PRINT_HIGH, "Could not open file %s for reading\n", filename.c_str());
		return false;
	}

    length = M_FileLength(handle);

    buf = (BYTE *)Z_Malloc (length, PU_STATIC, NULL);
    count = fread(buf, 1, length, handle);
    fclose (handle);

    if (count != length)
	{
		Printf(PRINT_HIGH, "Failed while reading from file %s\n", filename.c_str());
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
BOOL M_AppendExtension (std::string &filename, std::string extension, bool if_needed)
{
    M_FixPathSep(filename);

    size_t l = filename.find_last_of(PATHSEPCHAR);
	if(l == filename.length())
		return false;

    size_t dot = extension.find_first_of('.');
    if (dot == std::string::npos)
        return false;

    if (if_needed)
    {
        size_t dot = filename.find_last_of('.');

        if (dot == std::string::npos)
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

	size_t l = dest.find_last_of(PATHSEPCHAR);
	if (l == std::string::npos)
		dest.clear();
	else if (l < dest.length())
		dest = dest.substr(0, l);
}

//
// M_ExtractFileExtension
//
// Extract the extension of a file, returns false if it can't find
// extension seperator, true if succeeded, the extension is returned in
// dest
bool M_ExtractFileExtension(const std::string& filename, std::string &dest)
{
	if (!filename.length())
		return false;

	// find the last dot, iterating backwards
	size_t last_dot = filename.find_last_of('.', filename.length());
	if (last_dot == std::string::npos)
		dest.clear();
	else
		dest = filename.substr(last_dot + 1);

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

	size_t l = filename.find_last_of(PATHSEPCHAR);
	if(l == std::string::npos)
		l = 0;
	else
		l++;

	size_t e = filename.find_last_of('.');
	if(e == std::string::npos)
		e = filename.length();

	if(l < filename.length())
		dest = filename.substr(l, e - l);
}

//
// M_ExtractFileName
//
// Extract the name of a file from a path (name = filename with extension)
void M_ExtractFileName (std::string filename, std::string &dest)
{
    M_FixPathSep(filename);

	size_t l = filename.find_last_of(PATHSEPCHAR);
	if(l == std::string::npos)
		l = 0;
	else
		l++;

    if(l < filename.length())
        dest = filename.substr(l);
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

#if defined(_WIN32) && !defined(_XBOX)
	// This is not the canonical path separator, but it is valid.
	if (ch == '/')
	{
		return true;
	}
#endif

	return false;
}

// VolumeNameLen returns length of the leading volume name on Windows.
// It returns 0 elsewhere.
static size_t VolumeNameLen(std::string path)
{
#ifdef _WIN32
	if (path.size() < 2)
		return 0;

	// with drive letter
	char c = path[0];
	if (path[1] == ':' && ('a' <= c && c <= 'z' || 'A' <= c && c <= 'Z'))
		return 2;

	// is it UNC?
	// https://msdn.microsoft.com/en-us/library/windows/desktop/aa365247(v=vs.85).aspx
	size_t l = path.length();
	if (l >= 5 && M_IsPathSep(path[0]) && M_IsPathSep(path[1]) &&
	    !M_IsPathSep(path[2]) && path[2] != '.')
	{
		// first, leading `\\` and next shouldn't be `\`. its server name.
		for (size_t n = 3; n < l - 1; n++)
		{
			// second, next '\' shouldn't be repeated.
			if (M_IsPathSep(path[n]))
			{
				n++;

				// third, following something characters. its share name.
				if (!M_IsPathSep(path[n]))
				{
					if (path[n] == '.')
						break;

					for (; n < l; n++)
					{
						if (M_IsPathSep(path[n]))
							break;
					}
					return n;
				}
				break;
			}
		}
	}
	return 0;
#else
	return 0;
#endif
}

// FromSlash returns the result of replacing each slash ('/') character
// in path with a separator character. Multiple slashes are replaced
// by multiple separators.
static std::string FromSlash(std::string path)
{
	if (PATHSEPCHAR == '/')
		return path;

	std::replace(path.begin(), path.end(), '/', PATHSEPCHAR);
	return path;
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
	std::string originalPath = path;
	size_t volLen = VolumeNameLen(path);
	std::string vol = path.substr(0, volLen);
	path = path.substr(volLen, std::string::npos);
	if (path == "")
	{
		if (volLen > 1 && originalPath[1] != ':')
		{
			// should be UNC
			return FromSlash(originalPath);
		}
		return originalPath + ".";
	}
	bool rooted = M_IsPathSep(path[0]);

	// Invariants:
	//	reading from path; r is index of next byte to process.
	//	writing to buf; w is index of next byte to write.
	//	dotdot is index in buf where .. must stop, either because
	//		it is the leading slash or it is a leading ../../.. prefix.
	size_t n = path.length();
	std::string out = "";
	size_t r = 0;
	size_t dotdot = 0;
	if (rooted)
	{
		out.push_back(PATHSEPCHAR);
		r = 1;
		dotdot = 1;
	}

	while (r < n)
	{
		if (M_IsPathSep(path[r]))
		{
			// empty path element
			r += 1;
		}
		else if (path[r] == '.' && (r + 1 == n || M_IsPathSep(path[r + 1])))
		{
			// . element
			r += 1;
		}
		else if (path[r] == '.' && path[r + 1] == '.' &&
		         (r + 2 == n || M_IsPathSep(path[r + 2])))
		{
			// .. element: remove to last separator
			r += 2;
			if (out.length() > dotdot)
			{
				// can backtrack
				size_t w = out.length() - 1;
				while (w > dotdot && !M_IsPathSep(out[w]))
					w -= 1;

				out.resize(w);
			}
			else if (!rooted)
			{
				// cannot backtrack, but not rooted, so append .. element.
				if (out.length() > 0)
					out.push_back(PATHSEPCHAR);

				out.append("..");
				dotdot = out.length();
			}
		}
		else
		{
			// real path element.
			// add slash if needed
			if ((rooted && out.length() != 1) || (!rooted && out.length() != 0))
				out.push_back(PATHSEPCHAR);

			// copy element
			for (; r < n && !M_IsPathSep(path[r]); r++)
				out.push_back(path[r]);
		}
	}

	// Turn empty string into "."
	if (out.length() == 0)
		out.push_back('.');

	return FromSlash(vol + out);
}

VERSION_CONTROL(m_fileio_cpp, "$Id$")
