// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2020 by The Odamex Team.
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
//   Platform detection.
//   This file should be safe to use from any project.
//
//-----------------------------------------------------------------------------

#ifndef __OPLATFORM_H__
#define __OPLATFORM_H__

// Compilers

#if defined(_MSC_VER)
/**
 * @brief Compiler is MSVC-like, probably MSVC or Clang-cl.
 */
#define OCOMP_MSVC 1
#endif

#if defined(__GNUG__)
/**
 * @brief Compiler is GCC-like, probably GCC or Clang.
 */
#define OCOMP_GCC 1
#endif

// Architectures

// Mostly cribbed from https://sourceforge.net/p/predef/wiki/Endianness/
#if (defined(__BYTE_ORDER__) && (__BYTE_ORDER__ == __ORDER_BIG_ENDIAN__)) || \
    defined(__BIG_ENDIAN__) || defined(__ARMEB__) || defined(__THUMBEB__) || \
    defined(__AARCH64EB__) || defined(_MIPSEB) || defined(__MIPSEB) ||       \
    defined(__MIPSEB__)
/**
 * @brief Archtecture is big-endian.
 */
#define OARCH_BIG_ENDIAN 1
#endif

// Platforms

#if defined(_WIN32)
/**
 * @brief Platform contains enough Windows functions and conventions to be
 *        considered Windows-like.
 */
#define OPLAT_WINLIKE 1
#endif

#if defined(__APPLE__) && defined(__MACH__)
/**
 * @brief OS is modern macOS.
 */
#define OPLAT_MACOS 1
#endif

#if defined(__unix__) || defined(OPLAT_MACOS)
/**
 * @brief Platform contains enough POSIX functions and conventions to be
 *        considered Unix-like.
 */
#define OPLAT_UNIXLIKE 1
#endif

#if defined(_XBOX)
/**
 * @brief Platform is Xbox.
 */
#define OPLAT_XBOX 1
#endif

#if defined(GEKKO)
/**
 * @brief Platform is Wii.
 */
#define OPLAT_WII 1
#endif

#if defined(OPLAT_WINLIKE) && !defined(OPLAT_XBOX)
/**
 * @brief Platform is proper Windows, not Xbox or anything else.
 */
#define OPLAT_WINDOWS 1
#endif

#if defined(OPLAT_UNIXLIKE) && !defined(OPLAT_WII)
/**
 * @brief Platform is a proper multi-user UNIX.
 */
#define OPLAT_UNIX 1
#endif

#if defined(OPLAT_XBOX) || defined(OPLAT_WII)
/**
 * @brief Platform is a game console where a KB+M should not be assumed.
 */
#define OPLAT_CONSOLE 1
#endif

#endif // __PLATFORM_H__
