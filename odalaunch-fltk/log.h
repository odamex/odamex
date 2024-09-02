// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2006-2021 by The Odamex Team.
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
//  A basic debugging logger.
//
//-----------------------------------------------------------------------------

#pragma once

#include "odalaunch.h"

/**
 * @brief Write a log string with varg-style format arguments.
 *
 * @param fmt Format string.
 * @param args Arguments.
 */
void Log_VDebug(fmt::string_view fmt, fmt::format_args args);

/**
 * @brief Write a log string with format arguments.
 *
 * @param fmt Format string.
 * @param ...args Arguments.
 */
template <typename... T>
inline void Log_Debug(fmt::format_string<T...> fmt, T&&... args)
{
	const auto& vargs = fmt::make_format_args(args...);
	Log_VDebug(fmt, vargs);
}
