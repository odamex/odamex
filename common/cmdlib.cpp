// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1997-2000 by id Software Inc.
// Copyright (C) 1998-2006 by Randy Heit (ZDoom).
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
//	Command library (mostly borrowed from the Q2 source)
//
//-----------------------------------------------------------------------------


#include "odamex.h"

#include <math.h>
#include <stdarg.h>
#include <stdlib.h>

#include <ctime>
#include <functional>
#include <map>
#include <sstream>

#include "win32inc.h"

#include "i_system.h"
#include "cmdlib.h"

#ifdef GEKKO
#include "i_wii.h"
#endif

#ifdef __SWITCH__
#include "nx_system.h"
#endif


char		com_token[8192];
BOOL		com_eof;

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
int ParseHex(const char* hex)
{
	int num = 0;
	const char* str = hex;

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
			DPrintf("Bad hex number: %s\n",hex);
			return 0;
		}
		str++;
	}

	return num;
}

//
// ParseNum
//
int ParseNum(const char* str)
{
	if (str[0] == '$')
		return ParseHex(str+1);
	if (str[0] == '0' && str[1] == 'x')
		return ParseHex(str+2);
	return atol(str);
}


// [RH] Returns true if the specified string is a valid decimal number

bool IsNum(const char* str)
{
	bool result = true;

	while (*str)
	{
		if (((*str < '0') || (*str > '9')) && (*str != '-'))
		{
			result = false;
			break;
		}
		str++;
	}
	return result;
}


//
// IsRealNum
//
// [SL] Returns true if the specified string is a valid real number
//
bool IsRealNum(const char* str)
{
	bool seen_decimal = false;

	if (str == NULL || *str == 0)
		return false;

	if (str[0] == '+' || str[0] == '-')
		str++;

	while (*str)
	{
		if (*str == '.')
		{
			if (seen_decimal)
				return false;		// second decimal point
			else
				seen_decimal = true;
		}
		else if (*str < '0' || *str > '9')
			return false;

		str++;
	}

	return true;
}

// [Russell] Returns 0 if strings are the same, optional parameter for case
// sensitivity
bool iequals(const std::string& s1, const std::string& s2)
{
	return stricmp(s1.c_str(), s2.c_str()) == 0;
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

static std::string& StdStringToLowerBase(std::string& lower, size_t n)
{
	std::string::iterator itend = n >= lower.length() ? lower.end() : lower.begin() + n;
	std::transform(lower.begin(), itend, lower.begin(), ::tolower);
	return lower;
}

std::string StdStringToLower(const std::string& str, size_t n)
{
	std::string lower(str, 0, n);
	return StdStringToLowerBase(lower, n);
}

std::string StdStringToLower(const char* str, size_t n)
{
	std::string lower(str, 0, n);
	return StdStringToLowerBase(lower, n);
}

static std::string& StdStringToUpperBase(std::string& upper, size_t n)
{
	std::string::iterator itend = n >= upper.length() ? upper.end() : upper.begin() + n;
	std::transform(upper.begin(), itend, upper.begin(), ::toupper);
	return upper;
}

std::string StdStringToUpper(const std::string& str, size_t n)
{
	std::string upper(str, 0, n);
	return StdStringToUpperBase(upper, n);
}

std::string StdStringToUpper(const char* str, size_t n)
{
	std::string upper(str, 0, n);
	return StdStringToUpperBase(upper, n);
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

	if (str.empty())
		return tokens;

	size_t delimPos = 0;
	size_t prevDelim = 0;

	while (delimPos != std::string::npos) {
		delimPos = str.find(delim, prevDelim);
		tokens.push_back(str.substr(prevDelim, delimPos - prevDelim));
		prevDelim = delimPos + 1;
	}

	return tokens;
}

//
// A quick and dirty std::string formatting that uses snprintf under the covers.
//
FORMAT_PRINTF(2, 3) void STACK_ARGS StrFormat(std::string& out, const char* fmt, ...)
{
	va_list va;
	va_start(va, fmt);
	VStrFormat(out, fmt, va);
	va_end(va);
}

//
// A quick and dirty std::string formatting that uses snprintf under the covers.
//
void STACK_ARGS VStrFormat(std::string& out, const char* fmt, va_list va)
{
	va_list va2;
	va_copy(va2, va);

	// Get desired length of buffer.
	int chars = vsnprintf(NULL, 0, fmt, va);
	if (chars < 0)
	{
		I_Error("Encoding error detected in StrFormat\n");
	}
	size_t len = (size_t)chars + sizeof('\0');

	// Allocate the buffer.
	char* buf = (char*)malloc(len);
	if (buf == NULL)
	{
		I_Error("Could not allocate StrFormat buffer\n");
	}

	// Actually write to the buffer.
	int ok = vsnprintf(buf, len, fmt, va2);
	if (ok != chars)
	{
		I_Error("Truncation detected in StrFormat\n");
	}

	out = buf;
	free(buf);
}

/**
 * @brief Format passed number of bytes with a byte multiple suffix. 
 * 
 * @param out Output string buffer.
 * @param bytes Number of bytes to format.
 */
void StrFormatBytes(std::string& out, size_t bytes)
{
	static const char* BYTE_MAGS[] = {
	    "B",
	    "kB",
	    "MB",
	    "GB",
	};

	size_t magnitude = 0;
	double checkbytes = bytes;
	while (checkbytes >= 1000.0 && magnitude < ARRAY_LENGTH(BYTE_MAGS))
	{
		magnitude += 1;
		checkbytes /= 1000.0;
	}

	if (magnitude)
		StrFormat(out, "%.2f %s", checkbytes, BYTE_MAGS[magnitude]);
	else
		StrFormat(out, "%.0f %s", checkbytes, BYTE_MAGS[magnitude]);
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

/**
 * @brief Turn the given number of tics into a time.
 * 
 * @param str String buffer to write into.
 * @param time Number of tics to turn into a time.
 * @param ceil Round up to the nearest second.
 */
void TicsToTime(OTimespan& span, int time, bool ceilsec)
{
	if (time < 0)
	{
		// We do not support negative time, so just zero the struct.
		span.hours = 0;
		span.minutes = 0;
		span.seconds = 0;
		span.tics = 0;
		span.csecs = 0;
		return;
	}

	if (ceilsec)
	{
		if (time > 0)
		{
			// This ensures that if two clocks are run side by side and the
			// normal time is exactly 1 second, the ceiling time is also 1
			// second.
			time -= 1;
		}

		time = time + TICRATE - (time % TICRATE);
	}

	span.hours = time / (TICRATE * 3600);
	time -= span.hours * TICRATE * 3600;

	span.minutes = time / (TICRATE * 60);
	time -= span.minutes * TICRATE * 60;

	span.seconds = time / TICRATE;
	span.tics = time % TICRATE;
	span.csecs = (span.tics * 100) / TICRATE;
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

// Ensure that a string only has valid viewable ASCII in it.
bool ValidString(const std::string& s)
{
	for (std::string::const_iterator it = s.begin();it != s.end();++it)
	{
		const char c = *it;
		if (c < ' ' || c > '~')
			return false;
	}
	return true;
}

bool IsHexString(const std::string& str, const size_t len)
{
	if (str.length() != len)
		return false;

	for (std::string::const_iterator it = str.begin(); it != str.end(); ++it)
	{
		if (*it >= '0' && *it <= '9')
			continue;
		if (*it >= 'A' && *it <= 'F')
			continue;
		return false;
	}
	return true;
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

void ReplaceString (char** ptr, const char *str)
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


//
// StripColorCodes
//
// Removes any color code markup from the given string.
//
void StripColorCodes(std::string& str)
{
	size_t pos = 0;
	while (pos < str.length())
	{
		if (str.c_str()[pos] == '\034' && str.c_str()[pos + 1] != '\0')
			str.erase(pos, 2);
		else
			pos++;
	}
}

/**
 * @brief Remap a value from one value range to another.
 *
 * @detail https://stackoverflow.com/q/3451553/91642
 *
 * @param value Value to remap.
 * @param low1 Lower bound on the source range.
 * @param high1 Upper bound on the source range.
 * @param low2 Lower bound on the destination range.
 * @param high2 Upper bound on the destination range.
 */
double Remap(const double value, const double low1, const double high1, const double low2,
             const double high2)
{
	return low2 + (value - low1) * (high2 - low2) / (high1 - low1);
}

//
// Log2
//
// Calculates the log base 2 of a 32-bit number using a lookup table.
//
// Based on public domain code written by Sean Eron Anderson.
// Taken from http://graphics.stanford.edu/~seander/bithacks.html
//
uint32_t Log2(uint32_t n)
{
	#define LT(n) n, n, n, n, n, n, n, n, n, n, n, n, n, n, n, n
	static const signed char LogTable256[256] = 
	{
		-1, 0, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3,
		LT(4), LT(5), LT(5), LT(6), LT(6), LT(6), LT(6),
		LT(7), LT(7), LT(7), LT(7), LT(7), LT(7), LT(7), LT(7)
	};

	register unsigned int t, tt;		// temporaries

	if ((tt = (n >> 16)))
		return (t = (tt >> 8)) ? 24 + LogTable256[t] : 16 + LogTable256[tt];
	else
		return (t = (n >> 8)) ? 8 + LogTable256[t] : LogTable256[n];
}

/**
 * This file has no copyright assigned and is placed in the Public Domain.
 * This file is part of the mingw-w64 runtime package.
 * No warranty is given; refer to the file DISCLAIMER.PD within this package.
 */

/**
 * @brief Returns the next representable value of from in the direction of to.
 */
float NextAfter(const float from, const float to)
{
	const float x = from;
	const float y = to;
	union {
		float f;
		unsigned int i;
	} u;
	if (isnan(y) || isnan(x))
		return x + y;
	if (x == y)
		/* nextafter (0.0, -O.0) should return -0.0.  */
		return y;
	u.f = x;
	if (x == 0.0F)
	{
		u.i = 1;
		return y > 0.0F ? u.f : -u.f;
	}
	if (((x > 0.0F) ^ (y > x)) == 0)
		u.i++;
	else
		u.i--;
	return u.f;
}

VERSION_CONTROL (cmdlib_cpp, "$Id$")
