// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1997-2000 by id Software Inc.
// Copyright (C) 1998-2006 by Randy Heit (ZDoom).
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
//	Command library (mostly borrowed from the Q2 source)
//
//-----------------------------------------------------------------------------

#include <ctime>
#include <stdarg.h>
#include <stdlib.h>
#include <sstream>
#include <functional>

#include "win32inc.h"

#include "i_system.h"
#include "doomtype.h"
#include "cmdlib.h"
#include <map>

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
bool iequals(const std::string &s1, const std::string &s2)
{
	return StdStringToUpper(s1).compare(StdStringToUpper(s2)) == 0;
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
		if (str.c_str()[pos] == '\\' && str.c_str()[pos + 1] == 'c' && str.c_str()[pos + 2] != '\0')
			str.erase(pos, 3);
		else
			pos++;
	}
}



//
// CRC32
//
// Calculates a 32-bit checksum from a data buffer.
//
// Based on public domain code by Michael Barr
// http://www.barrgroup.com/Embedded-Systems/How-To/CRC-Calculation-C-Code
//

static inline uint8_t reflect_data(uint32_t data)
{
	uint32_t reflection = 0;

	for (uint8_t bit = 0; bit < 8; ++bit, data >>= 1)
	{
		if (data & 0x01)
			reflection |= (1 << (7 - bit));
	}

	return reflection & 0xFF;
}

static inline uint32_t reflect_result(uint32_t data)
{
	uint32_t reflection = 0;

	for (uint8_t bit = 0; bit < 32; ++bit, data >>= 1)
	{
		if (data & 0x01)
			reflection |= (1 << (31 - bit));
	}

	return reflection;
}

uint32_t CRC32(const uint8_t* buf, uint32_t len)
{
	static const uint32_t crctab[256] = {
		0x00000000, 0x04C11DB7, 0x09823B6E, 0x0D4326D9, 0x130476DC, 0x17C56B6B,
		0x1A864DB2, 0x1E475005, 0x2608EDB8, 0x22C9F00F, 0x2F8AD6D6, 0x2B4BCB61,
		0x350C9B64, 0x31CD86D3, 0x3C8EA00A, 0x384FBDBD, 0x4C11DB70, 0x48D0C6C7,
		0x4593E01E, 0x4152FDA9, 0x5F15ADAC, 0x5BD4B01B, 0x569796C2, 0x52568B75,
		0x6A1936C8, 0x6ED82B7F, 0x639B0DA6, 0x675A1011, 0x791D4014, 0x7DDC5DA3,
		0x709F7B7A, 0x745E66CD, 0x9823B6E0, 0x9CE2AB57, 0x91A18D8E, 0x95609039,
		0x8B27C03C, 0x8FE6DD8B, 0x82A5FB52, 0x8664E6E5, 0xBE2B5B58, 0xBAEA46EF,
		0xB7A96036, 0xB3687D81, 0xAD2F2D84, 0xA9EE3033, 0xA4AD16EA, 0xA06C0B5D,
		0xD4326D90, 0xD0F37027, 0xDDB056FE, 0xD9714B49, 0xC7361B4C, 0xC3F706FB,
		0xCEB42022, 0xCA753D95, 0xF23A8028, 0xF6FB9D9F, 0xFBB8BB46, 0xFF79A6F1,
		0xE13EF6F4, 0xE5FFEB43, 0xE8BCCD9A, 0xEC7DD02D, 0x34867077, 0x30476DC0,
		0x3D044B19, 0x39C556AE, 0x278206AB, 0x23431B1C, 0x2E003DC5, 0x2AC12072,
		0x128E9DCF, 0x164F8078, 0x1B0CA6A1, 0x1FCDBB16, 0x018AEB13, 0x054BF6A4,
		0x0808D07D, 0x0CC9CDCA, 0x7897AB07, 0x7C56B6B0, 0x71159069, 0x75D48DDE,
		0x6B93DDDB, 0x6F52C06C, 0x6211E6B5, 0x66D0FB02, 0x5E9F46BF, 0x5A5E5B08,
		0x571D7DD1, 0x53DC6066, 0x4D9B3063, 0x495A2DD4, 0x44190B0D, 0x40D816BA,
		0xACA5C697, 0xA864DB20, 0xA527FDF9, 0xA1E6E04E, 0xBFA1B04B, 0xBB60ADFC,
		0xB6238B25, 0xB2E29692, 0x8AAD2B2F, 0x8E6C3698, 0x832F1041, 0x87EE0DF6,
		0x99A95DF3, 0x9D684044, 0x902B669D, 0x94EA7B2A, 0xE0B41DE7, 0xE4750050,
		0xE9362689, 0xEDF73B3E, 0xF3B06B3B, 0xF771768C, 0xFA325055, 0xFEF34DE2,
		0xC6BCF05F, 0xC27DEDE8, 0xCF3ECB31, 0xCBFFD686, 0xD5B88683, 0xD1799B34,
		0xDC3ABDED, 0xD8FBA05A, 0x690CE0EE, 0x6DCDFD59, 0x608EDB80, 0x644FC637,
		0x7A089632, 0x7EC98B85, 0x738AAD5C, 0x774BB0EB, 0x4F040D56, 0x4BC510E1,
		0x46863638, 0x42472B8F, 0x5C007B8A, 0x58C1663D, 0x558240E4, 0x51435D53,
		0x251D3B9E, 0x21DC2629, 0x2C9F00F0, 0x285E1D47, 0x36194D42, 0x32D850F5,
		0x3F9B762C, 0x3B5A6B9B, 0x0315D626, 0x07D4CB91, 0x0A97ED48, 0x0E56F0FF,
		0x1011A0FA, 0x14D0BD4D, 0x19939B94, 0x1D528623, 0xF12F560E, 0xF5EE4BB9,
		0xF8AD6D60, 0xFC6C70D7, 0xE22B20D2, 0xE6EA3D65, 0xEBA91BBC, 0xEF68060B,
		0xD727BBB6, 0xD3E6A601, 0xDEA580D8, 0xDA649D6F, 0xC423CD6A, 0xC0E2D0DD,
		0xCDA1F604, 0xC960EBB3, 0xBD3E8D7E, 0xB9FF90C9, 0xB4BCB610, 0xB07DABA7,
		0xAE3AFBA2, 0xAAFBE615, 0xA7B8C0CC, 0xA379DD7B, 0x9B3660C6, 0x9FF77D71,
		0x92B45BA8, 0x9675461F, 0x8832161A, 0x8CF30BAD, 0x81B02D74, 0x857130C3,
		0x5D8A9099, 0x594B8D2E, 0x5408ABF7, 0x50C9B640, 0x4E8EE645, 0x4A4FFBF2,
		0x470CDD2B, 0x43CDC09C, 0x7B827D21, 0x7F436096, 0x7200464F, 0x76C15BF8,
		0x68860BFD, 0x6C47164A, 0x61043093, 0x65C52D24, 0x119B4BE9, 0x155A565E,
		0x18197087, 0x1CD86D30, 0x029F3D35, 0x065E2082, 0x0B1D065B, 0x0FDC1BEC,
		0x3793A651, 0x3352BBE6, 0x3E119D3F, 0x3AD08088, 0x2497D08D, 0x2056CD3A,
		0x2D15EBE3, 0x29D4F654, 0xC5A92679, 0xC1683BCE, 0xCC2B1D17, 0xC8EA00A0,
		0xD6AD50A5, 0xD26C4D12, 0xDF2F6BCB, 0xDBEE767C, 0xE3A1CBC1, 0xE760D676,
		0xEA23F0AF, 0xEEE2ED18, 0xF0A5BD1D, 0xF464A0AA, 0xF9278673, 0xFDE69BC4,
		0x89B8FD09, 0x8D79E0BE, 0x803AC667, 0x84FBDBD0, 0x9ABC8BD5, 0x9E7D9662,
		0x933EB0BB, 0x97FFAD0C, 0xAFB010B1, 0xAB710D06, 0xA6322BDF, 0xA2F33668,
		0xBCB4666D, 0xB8757BDA, 0xB5365D03, 0xB1F740B4
	};

	uint32_t remainder = 0xFFFFFFFF, index;

	for (; len; len--, buf++)
	{
		index = reflect_data(*buf) ^ (remainder >> 24);
		remainder = crctab[index] ^ (remainder << 8);
	}

	return reflect_result(remainder) ^ 0xFFFFFFFF;
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


VERSION_CONTROL (cmdlib_cpp, "$Id$")
