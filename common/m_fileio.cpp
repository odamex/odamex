// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
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

#include "z_zone.h"

// unfortunately, we still need you
#include "cmdlib.h"

// Simple logging
std::ofstream LOG;

// Simple file based console input
std::ifstream CON;

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

//
// M_FileExists
//
// Checks to see whether a file exists or not
//
bool M_FileExists(const std::string& filename)
{
	FILE *f = fopen(filename.c_str(), "r");
	if (!f)
		return false;
	fclose(f);
	return true;
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
    FixPathSeparator(filename);

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
	FixPathSeparator(dest);

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
    FixPathSeparator(filename);

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
    FixPathSeparator(filename);

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

#ifdef _WIN32

static bool IsSlash(char c)
{
	return c == '\\' || c == '/';
}

// VolumeNameLen returns length of the leading volume name on Windows.
// It returns 0 elsewhere.
static size_t VolumeNameLen(std::string path)
{
	if (path.size() < 2)
		return 0;

	// with drive letter
	char c = path.at(0);
	if (path.at(1) == ':' && ('a' <= c && c <= 'z' || 'A' <= c && c <= 'Z'))
		return 2;

	// is it UNC?
	// https://msdn.microsoft.com/en-us/library/windows/desktop/aa365247(v=vs.85).aspx
	size_t l = path.length();
	if (l >= 5 && IsSlash(path.at(0)) && IsSlash(path.at(1)) && !IsSlash(path.at(2)) &&
	    path.at(2) != '.')
	{
		// first, leading `\\` and next shouldn't be `\`. its server name.
		for (size_t n = 3; n < l - 1; n++)
		{
			// second, next '\' shouldn't be repeated.
			if (IsSlash(path.at(n)))
			{
				n++;

				// third, following something characters. its share name.
				if (!IsSlash(path.at(n)))
				{
					if (path.at(n) == '.')
						break;

					for (; n < l; n++)
					{
						if (IsSlash(path.at(n)))
							break;
					}
					return n;
				}
				break;
			}
		}
	}
	return 0;
}

#else

// VolumeNameLen returns length of the leading volume name on Windows.
// It returns 0 elsewhere.
static int VolumeNameLen(std::string path)
{
	return 0;
}

#endif

#ifdef _WIN32

// IsPathSeparator reports whether c is a directory separator character.
static bool IsPathSeparator(char c)
{
	// NOTE: Windows accept / as path separator.
	return c == '\\' || c == '/';
}

#else

// IsPathSeparator reports whether c is a directory separator character.
static bool IsPathSeparator(char c)
{
	return PATHSEPCHAR == c;
}

#endif

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
		if (volLen > 1 && originalPath.at(1) != ':')
		{
			// should be UNC
			return FromSlash(originalPath);
		}
		return originalPath + ".";
	}
	bool rooted = IsPathSeparator(path.at(0));

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
		if (IsPathSeparator(path.at(r)))
		{
			// empty path element
			r += 1;
		}
		else if (path.at(r) == '.' && (r + 1 == n || IsPathSeparator(path.at(r + 1))))
		{
			// . element
			r += 1;
		}
		else if (path.at(r) == '.' && path.at(r + 1) == '.' &&
		         (r + 2 == n || IsPathSeparator(path.at(r + 2))))
		{
			// .. element: remove to last separator
			r += 2;
			if (out.length() > dotdot)
			{
				// can backtrack
				size_t w = out.length() - 1;
				while (w > dotdot && !IsPathSeparator(out.at(w)))
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
			for (; r < n && !IsPathSeparator(path.at(r)); r++)
				out.push_back(path.at(r));
		}
	}

	// Turn empty string into "."
	if (out.length() == 0)
		out.push_back('.');

	return FromSlash(vol + out);
}

VERSION_CONTROL (m_fileio_cpp, "$Id$")
