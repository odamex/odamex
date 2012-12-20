// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1997-2000 by id Software Inc.
// Copyright (C) 1998-2006 by Randy Heit (ZDoom).
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
//	Command library (mostly borrowed from the Q2 source)
//
//-----------------------------------------------------------------------------

#include <ctime>
#include <stdio.h>
#include <stdlib.h>
#include <sstream>
#include <functional>

#ifdef WIN32
#define WIN32_LEAN_AND_MEAN
#ifndef _XBOX
#include <windows.h>
#endif // !_XBOX
#include "win32time.h"
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

// Tokenize a string
StringTokens TokenizeString(const std::string& str, const std::string& delim) {
	StringTokens tokens;
	size_t delimPos = 0;
	size_t prevDelim = 0;

	while (delimPos != std::string::npos) {
		delimPos = str.find(delim, prevDelim);
		tokens.push_back(str.substr(prevDelim, delimPos - prevDelim));
		prevDelim = delimPos + 1;
	}

	return tokens;
}

// [AM] Format a tm struct as an ISO8601-compliant extended format string.
//      Assume that the input time is in UTC.
bool StrFormatISOTime(std::string& s, const tm* utc_tm) {
	char buffer[21];
	if (!strftime(buffer, 21, "%Y-%m-%dT%H:%M:%SZ", utc_tm)) {
		return false;
	}
	s = buffer;
	return true;
}

// [AM] Parse an ISO8601-formatted string time into a tm* struct.
bool StrParseISOTime(const std::string& s, tm* utc_tm) {
	if (!strptime(s.c_str(), "%Y-%m-%dT%H:%M:%SZ", utc_tm)) {
		return false;
	}
	return true;
}

// [AM] Turn a string representation of a length of time into a time_t
//      relative to the current time.
bool StrToTime(std::string str, time_t &tim) {
	tim = time(NULL);
	str = TrimString(str);
	str = StdStringToLower(str);

	if (str.empty()) {
		return false;
	}

	// We use 0 as a synonym for forever.
	if (str.compare(std::string("eternity").substr(0, str.size())) == 0 ||
		str.compare(std::string("forever").substr(0, str.size())) == 0 ||
		str.compare(std::string("permanent").substr(0, str.size())) == 0) {
		tim = 0;
		return true;
	}

	// Gather tokens from string representation.
	typedef std::pair<unsigned short, std::string> token_t;
	typedef std::vector<token_t> tokens_t;
	tokens_t tokens;

	size_t i, j;
	size_t size = str.size();
	i = j = 0;

	while (i < size) {
		unsigned short num = 0;
		std::string timeword;

		// Grab a number.
		j = i;
		while (str[j] >= '0' && str[j] <= '9' && j < size) {
			j++;
		}

		if (i == j) {
			// There is no number.
			return false;
		}

		if (!(j < size)) {
			// We were expecting a number but ran off the end of the string.
			return false;
		}

		std::istringstream num_buffer(str.substr(i, j - i));
		num_buffer >> num;

		i = j;

		// Skip whitespace
		while ((str[i] == ' ') && i < size) {
			i++; j++;
		}

		// Grab a time word
		while (str[j] >= 'a' && str[j] <= 'z' && j < size) {
			j++;
		}

		if (i == j) {
			// There is no time word.
			return false;
		}

		timeword = str.substr(i, j - i);
		i = j;

		// Push to tokens vector
		token_t token;
		token.first = num;
		token.second = timeword;
		tokens.push_back(token);

		// Skip whitespace and commas.
		while ((str[i] == ' ' || str[i] == ',') && i < size) {
			i++;
		}
	}

	for (tokens_t::iterator it = tokens.begin();it != tokens.end();++it) {
		if (it->second.compare(std::string("seconds").substr(0, it->second.size())) == 0) {
			tim += it->first;
		} else if (it->second.compare("secs") == 0) {
			tim += it->first;
		} else if (it->second.compare(std::string("minutes").substr(0, it->second.size())) == 0) {
			tim += it->first * 60;
		} else if (it->second.compare("mins") == 0) {
			tim += it->first * 60;
		} else if (it->second.compare(std::string("hours").substr(0, it->second.size())) == 0) {
			tim += it->first * 3600;
		} else if (it->second.compare(std::string("days").substr(0, it->second.size())) == 0) {
			tim += it->first * 86400;
		} else if (it->second.compare(std::string("weeks").substr(0, it->second.size())) == 0) {
			tim += it->first * 604800;
		} else if (it->second.compare(std::string("months").substr(0, it->second.size())) == 0) {
			tim += it->first * 2592000;
		} else if (it->second.compare(std::string("years").substr(0, it->second.size())) == 0) {
			tim += it->first * 31536000;
		} else {
			// Unrecognized timeword
			return false;
		}
	}

	return true;
}

// [SL] Reimplement std::isspace 
static int _isspace(int c)
{
	return (c == ' ' || c == '\n' || c == '\t' || c == '\v' || c == '\f' || c == '\r');
}

// Trim whitespace from the start of a string
std::string &TrimStringStart(std::string &s)
{
	s.erase(s.begin(), std::find_if(s.begin(), s.end(), std::not1(std::ptr_fun<int, int>(_isspace))));
	return s;
}
 
// Trim whitespace from the end of a string
std::string &TrimStringEnd(std::string &s)
{
	s.erase(std::find_if(s.rbegin(), s.rend(), std::not1(std::ptr_fun<int, int>(_isspace))).base(), s.end());
	return s;
}
 
// Trim whitespace from the start and end of a string
std::string &TrimString(std::string &s)
{
	return TrimStringStart(TrimStringEnd(s));
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
