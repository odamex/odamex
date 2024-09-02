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
//  Global defines.
//
//-----------------------------------------------------------------------------

#pragma once

#if defined(_MSC_VER)
#include <sal.h> // Useful correctness macros.
#endif

#include <cstdint>

#include <fmt/core.h>
#include <string>

// Utility functions and classes.

#if defined(_MSC_VER)
#define NODISCARD _Check_return_
#else
#define NODISCARD __attribute__((__warn_unused_result__))
#endif

//
// ARRAY_LENGTH
//
// Safely counts the number of items in an C array.
//
// https://www.drdobbs.com/cpp/counting-array-elements-at-compile-time/197800525?pgno=1
//
#define ARRAY_LENGTH(arr)                                                  \
	(0 * sizeof(reinterpret_cast<const ::Bad_arg_to_ARRAY_LENGTH*>(arr)) + \
	 0 * sizeof(::Bad_arg_to_ARRAY_LENGTH::check_type((arr), &(arr))) +    \
	 sizeof(arr) / sizeof((arr)[0]))

struct Bad_arg_to_ARRAY_LENGTH
{
	class Is_pointer; // incomplete
	class Is_array
	{
	};
	template <typename T>
	static Is_pointer check_type(const T*, const T* const*);
	static Is_array check_type(const void*, const void*);
};

// ScopeExit
//
// Run a function on scope exit.
//
// https://stackoverflow.com/a/42506763/91642

template <typename F>
struct ScopeExit
{
	ScopeExit(F&& f) : m_f(std::forward<F>(f)) { }
	~ScopeExit() { m_f(); }
	F m_f;
};

template <typename F>
ScopeExit<F> makeScopeExit(F&& f)
{
	return ScopeExit<F>(std::forward<F>(f));
};

/**
 * @brief Combine an address and port number into a string addr:port.
 */
std::string AddressCombine(const std::string& address, const uint16_t port);

/**
 * @brief Split a string containing an address and port into its constituant
 *        parts.
 */
void AddressSplit(std::string& outIp, uint16_t& outPort, const std::string& address);

// Global variables.

class MainWindow;

struct globals_t
{
	MainWindow* mainWindow = nullptr;
};

extern globals_t g;
