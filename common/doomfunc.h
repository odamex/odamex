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
size_t Printf_Bold(const fmt::string_view format, const ARGS&... args)
{
	return C_BasePrint(PRINT_HIGH, TEXTCOLOR_BOLD, fmt::sprintf(format, args...));
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
size_t PrintFmt(const fmt::string_view format, const ARGS&... args)
{
	return C_BasePrint(PRINT_HIGH, TEXTCOLOR_NORMAL, fmt::format(format, args...));
}

template <typename... ARGS>
size_t PrintFmt(const int printlevel, const fmt::string_view format, const ARGS&... args)
{
	return C_BasePrint(printlevel, TEXTCOLOR_NORMAL, fmt::format(format, args...));
}
