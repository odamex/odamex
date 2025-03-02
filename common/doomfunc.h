// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 2006-2025 by The Odamex Team.
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

#include "v_textcolors.h"

#ifdef SERVER_APP
void SV_BasePrintAllPlayers(const int printlevel, const std::string& str);
void SV_BasePrintButPlayer(const int printlevel, const int player_id, const std::string& str);
#endif

size_t C_BasePrint(const int printlevel, const char* color_code, const std::string& str);

template <typename... ARGS>
size_t Printf(const fmt::string_view format, const ARGS&... args)
{
	return C_BasePrint(PRINT_HIGH, TEXTCOLOR_NORMAL, fmt::sprintf(format, args...));
}

template <typename... ARGS>
size_t Printf(const int printlevel, const fmt::string_view format, const ARGS&... args)
{
	return C_BasePrint(printlevel, TEXTCOLOR_NORMAL, fmt::sprintf(format, args...));
}

template <typename... ARGS>
size_t PrintFmt_Bold(const fmt::string_view format, const ARGS&... args)
{
	return C_BasePrint(PRINT_HIGH, TEXTCOLOR_BOLD, fmt::format(format, args...));
}

template <typename... ARGS>
size_t DPrintf(const fmt::string_view format, const ARGS&... args)
{
	if (::developer || ::devparm)
	{
		return C_BasePrint(PRINT_WARNING, TEXTCOLOR_NORMAL, fmt::sprintf(format, args...));
	}

	return 0;
}

template <typename... ARGS>
size_t DPrintFmt(const fmt::string_view format, const ARGS&... args)
{
	if (::developer || ::devparm)
	{
		return C_BasePrint(PRINT_WARNING, TEXTCOLOR_NORMAL, fmt::format(format, args...));
	}

	return 0;
}

template <typename... ARGS>
size_t PrintFmt(const fmt::string_view format, const ARGS&... args)
{
	return C_BasePrint(PRINT_HIGH, TEXTCOLOR_NORMAL, fmt::format(format, args...));
}

template <typename... ARGS>
size_t PrintFmt(const int printlevel, const fmt::string_view format, const ARGS&... args)
{
	return C_BasePrint(printlevel, TEXTCOLOR_NORMAL, fmt::format(format, args...));
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
void SV_BroadcastPrintf(int printlevel, const fmt::string_view format, const ARGS&... args)
{
	if (!serverside)
		return;

	std::string string = fmt::sprintf(format, args...);
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
void SV_BroadcastPrintf(const fmt::string_view format, const ARGS&... args)
{
	SV_BroadcastPrintf(PRINT_NORCON, format, args...);
}

#ifdef SERVER_APP
template <typename... ARGS>
void SV_BroadcastPrintfButPlayer(int printlevel, int player_id, const fmt::string_view format, const ARGS&... args)
{
	std::string string = fmt::sprintf(format, args...);
	C_BasePrint(printlevel, TEXTCOLOR_NORMAL, string); // print to the console

	// Hacky code to display messages as normal ones to clients
	if (printlevel == PRINT_NORCON)
		printlevel = PRINT_HIGH;

	SV_BasePrintButPlayer(printlevel, player_id, string);
}
#endif