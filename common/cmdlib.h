// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1998-2006 by Randy Heit (ZDoom).
// Copyright (C) 2006-2026 by The Odamex Team.
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
//	Command library (?)
//
//-----------------------------------------------------------------------------

#pragma once

#include <algorithm>
#include <ctime>
#include <optional>
#include <charconv>
#include <vector>
#include <string_view>
#include <string>
#include <concepts>

#ifdef _MSC_VER
#pragma warning(disable : 4244)     // MIPS
#pragma warning(disable : 4136)     // X86
#pragma warning(disable : 4051)     // ALPHA

#pragma warning(disable : 4018)     // signed/unsigned mismatch
#pragma warning(disable : 4305)     // truncate from double to float
#endif


#include <stdlib.h>
#include <errno.h>
#include <ctype.h>
#include <stdarg.h>

#include "doomdef.h"

struct OTimespan
{
	int csecs;
	int tics;
	int seconds;
	int minutes;
	int hours;
	OTimespan() : csecs(0), tics(0), seconds(0), minutes(0), hours(0) { }
};

int		ParseHex(const char *str);
int 	ParseNum(const char *str);
bool	IsNum(const char* str);		// [RH] added
bool	IsNum(std::string_view str);
bool	IsRealNum(const char* str);
bool	IsRealNum(std::string_view str);

template<std::integral T>
std::optional<T> ParseNum(std::string_view str, int base = 10)
{
    T out;
	while (!str.empty() && std::isspace(static_cast<unsigned char>(str.front())))
		str.remove_prefix(1);
	if (str[0] == '$')
	{
		str.remove_prefix(1);
		base = 16;
	}
    const std::from_chars_result result = std::from_chars(str.data(), str.data() + str.size(), out, base);
    if (result.ec != std::errc())
    {
        return std::nullopt;
    }
    return out;
}

namespace cmd_detail
{

template<typename T>
concept has_from_chars_float =
requires(const char* f, const char* l, T v)
{
    std::from_chars(f, l, v);
};

template <typename T>
inline constexpr bool always_false = false;

}

template<typename T>
requires std::is_floating_point_v<T>
std::optional<T> ParseNum(std::string_view str)
{
	if constexpr (cmd_detail::has_from_chars_float<T>)
	{
    	T out;
		while (!str.empty() && std::isspace(static_cast<unsigned char>(str.front())))
			str.remove_prefix(1);

    	const std::from_chars_result result = std::from_chars(str.data(), str.data() + str.size(), out);
    	if (result.ec != std::errc())
    	{
    	    return std::nullopt;
    	}
    	return out;
	}
	else
	{
		std::string nulltermstr(str);
		char* endptr = nullptr;
		errno = 0;
		const auto strtof_template = [](const char* str, char** str_end)
		{
			if constexpr (std::is_same_v<T, float>)
				return strtof(str, str_end);
			else if constexpr (std::is_same_v<T, double>)
				return strtod(str, str_end);
			else if constexpr (std::is_same_v<T, long double>)
				return strtold(str, str_end);
			else
			{
				static_assert(cmd_detail::always_false<T>, "Unknown floating point type");
			}
		};

		const auto out = strtof_template(nulltermstr.c_str(), &endptr);
		if (errno == ERANGE || endptr == nulltermstr.c_str())
			return std::nullopt;
		else
			return out;
	}
}


// [Russell] Returns 0 if strings are the same, optional parameter for case
// sensitivity
bool iequals(std::string_view, std::string_view);

size_t  StdStringFind(const std::string& haystack, const std::string& needle,
    size_t pos = 0, size_t n = std::string::npos, bool CIS = false);

size_t  StdStringRFind(const std::string& haystack, const std::string& needle,
    size_t pos = 0, size_t n = std::string::npos, bool CIS = false);

std::string StdStringToLower(const std::string&, size_t n = std::string::npos);
std::string StdStringToLower(const char*, size_t n = std::string::npos);
std::string StdStringToUpper(const std::string&, size_t n = std::string::npos);
std::string StdStringToUpper(const char*, size_t n = std::string::npos);

std::string &TrimString(std::string &s);
std::string &TrimStringStart(std::string &s);
std::string &TrimStringEnd(std::string &s);
std::string_view TrimStringView(std::string_view s);
std::string_view TrimStringViewStart(std::string_view s);
std::string_view TrimStringViewEnd(std::string_view s);

bool ValidString(const std::string&);
bool IsHexString(const std::string& str, const size_t len);

char	*copystring(const char *s);
bool M_StringCopy(char *dest, const char *src, size_t dest_size);

std::vector<std::string> VectorArgs(size_t argc, char **argv);
std::string JoinStrings(const std::vector<std::string> &pieces, const std::string &glue = "");

typedef std::vector<std::string> StringTokens;
StringTokens TokenizeString(const std::string& str, const std::string& delim);

void StrFormatBytes(std::string& out, size_t bytes);
bool StrFormatISOTime(std::string& s, const tm* utc_tm);
bool StrParseISOTime(const std::string& s, tm* utc_tm);
bool StrToTime(std::string str, time_t &tim);

void TicsToTime(OTimespan& span, int time, bool ceilsec = false);

/**
 * @brief Round a tic count to tenths of a second.
 *
 * Callers that go on to split the result into seconds, minutes and hours must
 * round here first and divide afterwards.
 *
 * @param tics    Tics to round. Negative counts as zero.
 * @param ceilsec Round up rather than down.
 */
inline int TicsToTenths(int tics, const bool ceilsec = false)
{
	if (tics < 0)
		tics = 0;

	return ceilsec ? (tics * 10 + TICRATE - 1) / TICRATE : (tics * 10) / TICRATE;
}

/**
 * @brief Render a tic count as a clock with a tenths place - "mm:ss.t", or
 *        "hh:mm:ss.t" if there is an hour to show. (currently limited to 60s)
 *
 * @param tics    Tics to render. Negative counts as zero.
 * @param ceilsec Round up rather than down. Counting down wants this so the
 *                clock never claims less time than is left, and counting up wants
 *                it off so it never claims more time than has elapsed.
 */
inline std::string TicsToClockTenths(const int tics, const bool ceilsec = false)
{
	const int tenths = TicsToTenths(tics, ceilsec);
	const int secs = tenths / 10;
	const int hours = secs / 3600;

	const auto pad = [](const int n) {
		return (n < 10 ? "0" : "") + std::to_string(n);
	};

	const std::string clock = pad((secs / 60) % 60) + ":" + pad(secs % 60) + "." +
	                          std::to_string(tenths % 10);

	if (hours)
	{
		return pad(hours) + ":" + clock;
	}

	return clock;
}

/**
 * @brief Render a tic count as a compact seconds string, growing a tenths part
 *        once it is small enough to be worth watching closely.
 *
 * Both halves round the same way, chosen by ceilsec. Rounding up suits a
 * countdown: the number never claims less time than is left, and "0.0" is only
 * ever reached at a true zero, because a single tic still to go rounds up to
 * "0.1". Rounding down suits counting up, so it never claims more time than
 * has actually elapsed.
 *
 * @param tics       Tics to render. Negative counts as zero.
 * @param tenthstics Render "Seconds.Tenths" at or below this many tics, and
 *                   whole seconds above it.
 *                   Zero for whole seconds only. Keep this an exact multiple of
 *                   TICRATE when ceilsec is set, or the handover flashes a
 *                   value for a single tic.
 * @param ceilsec    Round up rather than down, as described above.
 */
inline std::string TicsToShortTime(int tics, const int tenthstics,
                                   const bool ceilsec = false)
{
	if (tics < 0)
		tics = 0;

	if (tenthstics > 0 && tics <= tenthstics)
	{
		const int tenths = TicsToTenths(tics, ceilsec);

		return std::to_string(tenths / 10) + "." + std::to_string(tenths % 10);
	}

	if (ceilsec)
		return std::to_string((tics + TICRATE - 1) / TICRATE);

	return std::to_string(tics / TICRATE);
}

bool CheckWildcards (const char *pattern, const char *text);
void ReplaceString (char** ptr, const char* str);

void StripColorCodes(std::string& str);

double Remap(const double value, const double low1, const double high1, const double low2,
             const double high2);
uint32_t Log2(uint32_t n);

/**
 * @brief Initialize an array with a specific value.
 *
 * @tparam A Array type to initialize.
 * @tparam T Value type to initialize with.
 * @param dst Array to initialize.
 * @param val Value to initialize with.
 */
template <typename A, typename T>
static void ArrayInit(A& dst, const T& val)
{
	for (size_t i = 0; i < ARRAY_LENGTH(dst); i++)
		dst[i] = val;
}

/**
 * @brief Copy the complete contents of an array from one to the other.
 *
 * @detail Both params are templated in case the destination's type doesn't
 *         line up 100% with the source.
 *
 * @tparam A1 Destination array type.
 * @tparam A2 Source array type.
 * @param dst Destination array to write to.
 * @param src Source array to write from.
 */
template <typename A1, typename A2>
static void ArrayCopy(A1& dst, const A2& src)
{
	for (size_t i = 0; i < ARRAY_LENGTH(src); i++)
		dst[i] = src[i];
}
