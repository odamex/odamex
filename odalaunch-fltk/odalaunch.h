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

// stdint.h defines

#if defined(_MSC_VER)
#if _MSC_VER >= 1600 // 2010
#define USE_STDINT_H
#endif
#else
#define USE_STDINT_H
#endif

#if defined(USE_STDINT_H)
#include <stdint.h>
#undef USE_STDINT_H
#else
#include "pstdint.h"
#endif

// snprintf/vsnprintf
#include <stdio.h>
#include <stdarg.h>

#if defined(_MSC_VER)
#if _MSC_VER < 1900 // 2015
#define USE_MS_SNPRINTF
#endif
#endif

#if defined(USE_MS_SNPRINTF)
int __cdecl ms_snprintf(char* buffer, size_t n, const char* format, ...);
int __cdecl ms_vsnprintf(char* s, size_t n, const char* format, va_list arg);
#define snprintf ms_snprintf
#define vsnprintf ms_vsnprintf
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

// Utility functions.

#include <string>

std::string AddressCombine(const char* address, uint16_t port);
std::string AddressCombine(const char* address, const char* port);
void AddressSplit(const char* address, std::string& outIp, uint16_t& outPort);

// Global variables.

class MainWindow;

struct globals_t
{
	MainWindow* mainWindow;
	globals_t() : mainWindow(NULL) { }
};

extern globals_t g;
