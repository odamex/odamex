// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
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
//	File Input/Output Operations
//
//-----------------------------------------------------------------------------

#include "m_fileio.h"
#include "c_dispatch.h"
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
BOOL M_FileExists (std::string filename)
{
	FILE *f;

	f = fopen (filename.c_str(), "r");
	if (!f)
		return false;
	fclose (f);
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
void M_ExtractFilePath (std::string filename, std::string &dest)
{
    FixPathSeparator(filename);

	size_t l = filename.find_last_of(PATHSEPCHAR);
	if(l == std::string::npos)
		l = filename.length();

    if(l < filename.length())
        dest = filename.substr(0, l);
}

//
// M_ExtractFileExtension
//
// Extract the extension of a file, returns false if it can't find
// extension seperator, true if succeeded, the extension is returned in
// dest
BOOL M_ExtractFileExtension (std::string filename, std::string &dest)
{
    QWORD last_dot = 0;

    if (&dest == NULL || !filename.length())
        return false;

    // find the last dot, iterating backwards
    last_dot = filename.find_last_of('.', filename.length());

    if (last_dot == std::string::npos)
        return false;

    // extract extension without leading dot
    dest = filename.substr(last_dot + 1);

    // fun in the sun
    return true;
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

std::string M_ExtractFileName(const std::string filename) {
	std::string result;
	M_ExtractFileName(filename, result);
	return result;
}

VERSION_CONTROL (m_fileio_cpp, "$Id$")
