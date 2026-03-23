// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
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
//   Functions that should be used everywhere.
//
//-----------------------------------------------------------------------------

#pragma once

#include "fmt/format.h"

#include "doomstat.h"

#include "v_textcolors.h"

#ifdef SERVER_APP
void SV_BasePrintAllPlayers(const int printlevel, const std::string& str);
void SV_BasePrintButPlayer(const int printlevel, const int player_id, const std::string& str);
#endif

size_t C_BasePrint(const int printlevel, const char* color_code, const std::string& str);

template <typename... ARGS>
size_t PrintFmt(fmt::format_string<ARGS...> format, ARGS&&... args)
{
	return C_BasePrint(PRINT_HIGH, TEXTCOLOR_NORMAL, fmt::format(format, std::forward<ARGS>(args)...));
}

template <typename... ARGS>
size_t PrintFmt(const int printlevel, fmt::format_string<ARGS...> format, ARGS&&... args)
{
	return C_BasePrint(printlevel, TEXTCOLOR_NORMAL, fmt::format(format, std::forward<ARGS>(args)...));
}

template <typename... ARGS>
size_t PrintFmt_Bold(fmt::format_string<ARGS...> format, ARGS&&... args)
{
	return C_BasePrint(PRINT_HIGH, TEXTCOLOR_BOLD, fmt::format(format, std::forward<ARGS>(args)...));
}

template <typename... ARGS>
size_t DPrintFmt(fmt::format_string<ARGS...> format, ARGS&&... args)
{
	if (::developer || ::devparm)
	{
		return C_BasePrint(PRINT_WARNING, TEXTCOLOR_NORMAL, fmt::format(format, std::forward<ARGS>(args)...));
	}

	return 0;
}

/**
 * @brief Print to all clients in a server, or to the local player offline.
 *
 * @note This could really use a new name, like "ServerPrintf".
 *
 * @param printlevel PRINT_* constant designating what kind of print this is.
 * @param format printf-style format string.
 * @param args printf-style arguments.
 */
template <typename... ARGS>
void SV_BroadcastPrintFmt(int printlevel, fmt::format_string<ARGS...> format, ARGS&&... args)
{
	if (!serverside)
		return;

	std::string string = fmt::format(format, std::forward<ARGS>(args)...);
	C_BasePrint(printlevel, TEXTCOLOR_NORMAL, string);

	#ifdef SERVER_APP
	// Hacky code to display messages as normal ones to clients
	if (printlevel == PRINT_NORCON)
		printlevel = PRINT_HIGH;

	SV_BasePrintAllPlayers(printlevel, string);
	#endif
}

/**
 * @brief Print to all clients in a server, or to the local player offline.
 *
 * @note This could really use a new name, like "ServerPrintf".
 *
 * @param format printf-style format string.
 * @param args printf-style arguments.
 */
template <typename... ARGS>
void SV_BroadcastPrintFmt(fmt::format_string<ARGS...> format, ARGS&&... args)
{
	SV_BroadcastPrintFmt(PRINT_NORCON, format, std::forward<ARGS>(args)...);
}

#ifdef SERVER_APP
template <typename... ARGS>
void SV_BroadcastPrintFmtButPlayer(int printlevel, int player_id, fmt::format_string<ARGS...> format, ARGS&&... args)
{
	std::string string = fmt::format(format, std::forward<ARGS>(args)...);
	C_BasePrint(printlevel, TEXTCOLOR_NORMAL, string); // print to the console

	// Hacky code to display messages as normal ones to clients
	if (printlevel == PRINT_NORCON)
		printlevel = PRINT_HIGH;

	SV_BasePrintButPlayer(printlevel, player_id, string);
}
#endif

namespace OUtil
{

// Wrapper for easy iteration over containers in reverse with ranged for loops
template <typename T>
struct reverse_wrapper
{
    T& iterable;
    inline auto begin() { return std::rbegin(iterable); }
    inline auto end() { return std::rend(iterable); }
};

/**
 * @brief Reverse the iteration in a range-based for loop
 */
template <typename T>
inline reverse_wrapper<T> reverse(T&& iterable) { return { iterable }; }

// Wrapper for skipping the first N elements in a range-based for loop
template <typename T>
struct drop_wrapper
{
    T& iterable;
    size_t count;

    auto begin() {
        auto it = std::begin(iterable);
        auto end_it = std::end(iterable);
        for (size_t i = 0; i < count && it != end_it; ++i)
            ++it;
        return it;
    }

    inline auto end() { return std::end(iterable); }
};

/**
 * @brief Skip the first `count` elements in a range-based for loop
 */
template <typename T>
inline drop_wrapper<T> drop(T&& iterable, std::size_t count) { return { iterable, count }; }

// Helper for use of std::visit with lambdas
template<class... Ts>
struct visitor : Ts... { using Ts::operator()...; };
// This shouldn't be needed in C++20, but for some reason macOS builds fail without it
template<class... Ts>
visitor(Ts...) -> visitor<Ts...>;

// C++23 std::unreachable
[[noreturn]] inline void unreachable()
{
#if defined(_MSC_VER) && !defined(__clang__)
	__assume(false);
#else
	__builtin_unreachable();
#endif
}

}

// Literals for stdint types

[[nodiscard]]
consteval int8_t operator ""_i8(unsigned long long x)
{
	if (x > std::numeric_limits<int8_t>::max())
		throw "Literal out of range for type int8_t";
	return static_cast<int8_t>(x);
}

[[nodiscard]]
consteval uint8_t operator ""_u8(unsigned long long x)
{
	if (x > std::numeric_limits<uint8_t>::max())
		throw "Literal out of range for type uint8_t";
	return static_cast<uint8_t>(x);
}

[[nodiscard]]
consteval int16_t operator ""_i16(unsigned long long x)
{
	if (x > std::numeric_limits<int16_t>::max())
		throw "Literal out of range for type int16_t";
	return static_cast<int16_t>(x);
}

[[nodiscard]]
consteval uint16_t operator ""_u16(unsigned long long x)
{
	if (x > std::numeric_limits<uint16_t>::max())
		throw "Literal out of range for type uint16_t";
	return static_cast<uint16_t>(x);
}

[[nodiscard]]
consteval int32_t operator ""_i32(unsigned long long x)
{
	if (x > std::numeric_limits<int32_t>::max())
		throw "Literal out of range for type int32_t";
	return static_cast<int32_t>(x);
}

[[nodiscard]]
consteval uint32_t operator ""_u32(unsigned long long x)
{
	if (x > std::numeric_limits<uint32_t>::max())
		throw "Literal out of range for type uint32_t";
	return static_cast<uint32_t>(x);
}

[[nodiscard]]
consteval int64_t operator ""_i64(unsigned long long x)
{
	if (x > std::numeric_limits<int64_t>::max())
		throw "Literal out of range for type int64_t";
	return static_cast<int64_t>(x);
}

[[nodiscard]]
consteval uint64_t operator ""_u64(unsigned long long x)
{
	if (x > std::numeric_limits<uint64_t>::max())
		throw "Literal out of range for type uint65_t";
	return static_cast<uint64_t>(x);
}
