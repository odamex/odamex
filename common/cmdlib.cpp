// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id:$
//
// Copyright (C) 1997-2000 by id Software Inc.
// Copyright (C) 1998-2006 by Randy Heit (ZDoom).
// Copyright (C) 2006-2007 by The Odamex Team.
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
//	Command library (mostly borrowed from the Q2 source)
//
//-----------------------------------------------------------------------------


#include <stdio.h>
#include <stdlib.h>

#ifdef WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

#include "doomtype.h"
#include "cmdlib.h"
#include "i_system.h"
#include <sys/types.h>
#include <sys/stat.h>

#include "m_alloc.h"

char		com_token[8192];
BOOL		com_eof;

std::string progdir, startdir; // denis - todo - maybe move this into Args

void FixPathSeparator (std::string &path)
{
	for(size_t i = 0; i < path.length(); i++)
		if(path[i] == '\\')
			path[i] = '/';
}

char *copystring (const char *s)
{
	char *b;
	if (s)
	{
		b = new char[strlen(s)+1];
		strcpy (b, s);
	}
	else
	{
		b = new char[1];
		b[0] = '\0';
	}
	return b;
}


//
// COM_Parse
//
// Parse a token out of a string
//
char *COM_Parse (char *data) // denis - todo - security com_token overrun needs expert check, i have just put simple bounds on len
{
	int			c;
	size_t		len;
	
	len = 0;
	com_token[0] = 0;
	
	if (!data)
		return NULL;
		
// skip whitespace
skipwhite:
	while ( (c = *data) <= ' ')
	{
		if (c == 0)
		{
			com_eof = true;
			return NULL;			// end of file;
		}
		data++;
	}
	
// skip // comments
	if (c=='/' && data[1] == '/')
	{
		while (*data && *data != '\n')
			data++;
		goto skipwhite;
	}
	

// handle quoted strings specially
	if (c == '\"')
	{
		data++;
		do
		{
			c = *data++;
			if (c=='\"')
			{
				com_token[len] = 0;
				return data;
			}
			com_token[len] = c;
			len++;
		} while (len < sizeof(com_token) + 2);
	}

// parse single characters
	if (c=='{' || c=='}'|| c==')'|| c=='(' || c=='\'' || c==':' || /*[RH]*/c=='=')
	{
		com_token[len] = c;
		len++;
		com_token[len] = 0;
		return data+1;
	}

// parse a regular word
	do
	{
		com_token[len] = c;
		data++;
		len++;
		c = *data;
	if (c=='{' || c=='}'|| c==')'|| c=='(' || c=='\'' || c==':' || c=='=')
			break;
	} while (c>32 && len < sizeof(com_token) + 2);
	
	com_token[len] = 0;
	return data;
}


//
// Misc
//

//
// Q_filelength
//
int Q_filelength (FILE *f)
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
// FileExists
//
BOOL FileExists (const char *filename)
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
// DefaultExtension
//
void DefaultExtension (std::string &path, const char *extension)
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
// Extract file parts
//
// FIXME: should include the slash, otherwise ... should use strings only
// backing to an empty path will be wrong when appending a slash
void ExtractFilePath (const char *path, char *dest)
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
// ExtractFileBase
//
void ExtractFileBase (std::string path, std::string &dest)
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


//
// ParseNum / ParseHex
//
int ParseHex (char *hex)
{
	char *str;
	int num;

	num = 0;
	str = hex;

	while (*str)
	{
		num <<= 4;
		if (*str >= '0' && *str <= '9')
			num += *str-'0';
		else if (*str >= 'a' && *str <= 'f')
			num += 10 + *str-'a';
		else if (*str >= 'A' && *str <= 'F')
			num += 10 + *str-'A';
		else {
			Printf (PRINT_HIGH, "Bad hex number: %s\n",hex);
			return 0;
		}
		str++;
	}

	return num;
}

//
// ParseNum
//
int ParseNum (char *str)
{
	if (str[0] == '$')
		return ParseHex (str+1);
	if (str[0] == '0' && str[1] == 'x')
		return ParseHex (str+2);
	return atol (str);
}


// [RH] Returns true if the specified string is a valid decimal number

BOOL IsNum (char *str)
{
	BOOL result = true;

	while (*str) {
		if (((*str < '0') || (*str > '9')) && (*str != '-')) {
			result = false;
			break;
		}
		str++;
	}
	return result;
}

