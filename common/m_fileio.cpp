// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id: m_fileio.cpp 5 2007-01-16 19:13:59Z russellrice $
//
// Copyright (C) 1993-1996 by id Software, Inc.
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
#include "z_zone.h"

//
// M_FileLength
//
// Returns the length of a file using an open descriptor
int M_FileLength (FILE *f)
{
	int		pos;
	int		end;

	pos = ftell (f);
	fseek (f, 0, SEEK_END);
	end = ftell (f);
	fseek (f, pos, SEEK_SET);

	return end;
}

//
// M_FileExists
//
// Checks to see whether a file exists or not
BOOL M_FileExists (const char *filename)
{
	FILE *f;

	// [RH] Empty filenames are never there
	if (*filename == 0)
		return false;

	f = fopen (filename, "r");
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
BOOL M_WriteFile(char const *name, void *source, QWORD length)
{
    FILE *handle;
    int	count;
	
    handle = fopen(name, "wb");

    if (handle == NULL)
	{
		Printf(PRINT_HIGH, "Could not open file %s for writing\n", name);
		return false;
	}

    count = fwrite(source, 1, length, handle);
    fclose(handle);
	
	if (count != length)
	{
		Printf(PRINT_HIGH, "Failed while writing to file %s\n", name);
		return false;
	}
		
    return true;
}


//
// M_ReadFile
//
// Reads a file, it will allocate storage via Z_Malloc for it and return
// the buffer and the size.
QWORD M_ReadFile(char const *name, BYTE **buffer)
{
    FILE *handle;
    QWORD count, length;
    BYTE *buf;
	
    handle = fopen(name, "rb");
    
	if (handle == NULL)
	{
		Printf(PRINT_HIGH, "Could not open file %s for reading\n", name);
		return false;
	}

    length = M_FileLength(handle);
    
    buf = (BYTE *)Z_Malloc (length, PU_STATIC, NULL);
    count = fread(buf, 1, length, handle);
    fclose (handle);
	
    if (count != length)
	{
		Printf(PRINT_HIGH, "Failed while reading from file %s\n", name);
		return false;
	}
		
    *buffer = buf;
    return length;
}

//
// M_DefaultExtension
//
// TODO: document
void M_DefaultExtension (std::string &path, const char *extension)
{
	size_t src = path.length() - 1;
//
// if path doesn't have a .EXT, append extension
// (extension should include the .)
//

	while (path[src] != '/' && src)
	{
		if (path[src] == '.')
			return; // assume it has an extension
		src--;
	}

	path += extension;
}

//
// M_ExtractFilePath
//
// TODO: document
// FIXME: should include the slash, otherwise ... should use strings only
// backing to an empty path will be wrong when appending a slash
void M_ExtractFilePath (const char *path, char *dest)
{
	const char *src;

	src = path + strlen(path) - 1;

    // back up until a \ or the start
    while (src != path
		   && *(src-1) != '\\'
		   && *(src-1) != '/')
    {
		src--;
    }

	memcpy (dest, path, src-path);
	dest[src-path] = 0;
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
    dest = filename.substr(last_dot + 1, filename.length());
    
    // convert to uppercase
    std::transform(dest.begin(), dest.end(), dest.begin(), toupper);
    
    // fun in the sun
    return true;
}

//
// M_ExtractFileBase
//
// TODO: document
void M_ExtractFileBase (std::string path, std::string &dest)
{
	//
	// back up until a / or the start
	//	

	size_t l = path.find_last_of('/');
	if(l == std::string::npos)
		l = 0;

	size_t e = path.find_first_of('.');
	if(e == std::string::npos)
		e = path.length();

	if(++l < path.length())
		dest = path.substr(l, e - l);
}

VERSION_CONTROL (m_fileio_cpp, "$Id: m_fileio.cpp 5 2007-01-16 19:13:59Z russellrice $")
