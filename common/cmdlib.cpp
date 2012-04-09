// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1997-2000 by id Software Inc.
// Copyright (C) 1998-2006 by Randy Heit (ZDoom).
// Copyright (C) 2006-2010 by The Odamex Team.
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
#include <sstream>

#ifdef WIN32
#define WIN32_LEAN_AND_MEAN
#ifndef _XBOX
#include <windows.h>
#endif // _XBOX
#endif // WIN32

#include "doomtype.h"
#include "cmdlib.h"
#include "i_system.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <map>

#include "m_alloc.h"

char		com_token[8192];
BOOL		com_eof;

std::string progdir, startdir; // denis - todo - maybe move this into Args

void FixPathSeparator (std::string &path)
{
	// Use the platform appropriate path separator
	for(size_t i = 0; i < path.length(); i++)
		if(path[i] == '\\' || path[i] == '/')
			path[i] = PATHSEPCHAR;
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

// [Russell] Returns 0 if strings are the same, optional parameter for case
// sensitivity
int StdStringCompare(const std::string &s1, const std::string &s2,
    bool CIS = false)
{
	// Convert to upper case
	if (CIS)
	{
		return StdStringToUpper(s1).compare(StdStringToUpper(s2));
	}

    return s1.compare(s2);
}

size_t StdStringFind(const std::string& haystack, const std::string& needle,
    size_t pos, size_t n, bool CIS, bool reverse)
{
    if (CIS)
    {
        if(reverse)
        {
            return StdStringToUpper(haystack).rfind(StdStringToUpper(needle).c_str(), pos, n);
        }

        return StdStringToUpper(haystack).find(StdStringToUpper(needle).c_str(), pos, n);
    }

    if(reverse)
    {
        return haystack.rfind(needle.c_str(), pos, n);
    }

    return haystack.find(needle.c_str(), pos, n);
}

size_t StdStringFind(const std::string& haystack, const std::string& needle,
    size_t pos = 0, size_t n = std::string::npos, bool CIS = false)
{
    return StdStringFind(haystack, needle, pos, n, CIS, false);
}

size_t StdStringRFind(const std::string& haystack, const std::string& needle,
    size_t pos = 0, size_t n = std::string::npos, bool CIS = false)
{
    return StdStringFind(haystack, needle, pos, n, CIS, true);
}

static std::string& StdStringToLowerBase(std::string& lower)
{
	std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
	return lower;
}

std::string StdStringToLower(const std::string& str)
{
	std::string lower(str);
	return StdStringToLowerBase(lower);
}

std::string StdStringToLower(const char* str)
{
	std::string lower(str);
	return StdStringToLowerBase(lower);
}

static std::string& StdStringToUpperBase(std::string& upper)
{
	std::transform(upper.begin(), upper.end(), upper.begin(), ::toupper);
    return upper;
}

std::string StdStringToUpper(const std::string& str)
{
	std::string upper(str);
	return StdStringToUpperBase(upper);
}

std::string StdStringToUpper(const char* str)
{
	std::string upper(str);
	return StdStringToUpperBase(upper);
}

// [AM] Convert an argc/argv into a vector of strings.
std::vector<std::string> VectorArgs(size_t argc, char **argv) {
	std::vector<std::string> arguments(argc - 1);
	for (unsigned i = 1;i < argc;i++) {
		arguments[i - 1] = argv[i];
	}
	return arguments;
}

// [AM] Return a joined string based on a vector of strings
std::string JoinStrings(const std::vector<std::string> &pieces, const std::string &glue) {
	std::ostringstream result;
	for (std::vector<std::string>::const_iterator it = pieces.begin();
		 it != pieces.end();++it) {
		result << *it;
		if (it != (pieces.end() - 1)) {
			result << glue;
		}
	}
	return result.str();
}

// [SL] Reimplement std::isspace 
static int _isspace(int c)
{
	return (c == ' ' || c == '\n' || c == '\t' || c == '\v' || c == '\f' || c == '\r');
}

// Trim whitespace from the start and end of a string
std::string &TrimString(std::string &s)
{
	s.erase(remove_if(s.begin(), s.end(), _isspace), s.end());
	return s;
}


//==========================================================================
//
// CheckWildcards
//
// [RH] Checks if text matches the wildcard pattern using ? or *
//
//==========================================================================
bool CheckWildcards (const char *pattern, const char *text)
{
	if (pattern == NULL || text == NULL)
		return true;

	while (*pattern)
	{
		if (*pattern == '*')
		{
			char stop = tolower (*++pattern);
			while (*text && tolower(*text) != stop)
			{
				text++;
			}
			if (*text && tolower(*text) == stop)
			{
				if (CheckWildcards (pattern, text++))
				{
					return true;
				}
				pattern--;
			}
		}
		else if (*pattern == '?' || tolower(*pattern) == tolower(*text))
		{
			pattern++;
			text++;
		}
		else
		{
			return false;
		}
	}
	return (*pattern | *text) == 0;
}

class ReplacedStringTracker
{
	typedef std::map<const char *, bool> replacedStrings_t;
	typedef replacedStrings_t:: iterator iterator;
	replacedStrings_t rs;

public:

	void erase(const char *p)
	{
		iterator i = rs.find(p);
		if(i != rs.end())
		{
			delete [] const_cast<char*>(i->first);
			rs.erase(i);
		}
	}
	void add(const char *p)
	{
		rs[p] = 1;
	}

	ReplacedStringTracker() : rs() {}
	~ReplacedStringTracker()
	{
		for(iterator i = rs.begin(); i != rs.end(); ++i)
			delete[] const_cast<char*>(i->first);
	}
}rst;

void ReplaceString (const char **ptr, const char *str)
{
	if (*ptr)
	{
		if (*ptr == str)
			return;
		rst.erase(*ptr);
	}
	*ptr = copystring (str);
	rst.add(*ptr);
}

VERSION_CONTROL (cmdlib_cpp, "$Id$")
