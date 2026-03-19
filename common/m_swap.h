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
//	Endianess handling, swapping 16bit and 32bit.
//
//-----------------------------------------------------------------------------

#pragma once

#include <cstdint> // bit.hpp is supposed to include this itself, but a bug in 2.0.0 prevents it
#include <nonstd/bit.hpp>

#if TARGET_CPU_X86 || TARGET_CPU_X86_64
#ifdef __BIG_ENDIAN__
#undef __BIG_ENDIAN__
#endif
#ifndef __LITTLE_ENDIAN__
#define __LITTLE_ENDIAN__
#endif
#endif

#if TARGET_CPU_PPC
#ifndef __BIG_ENDIAN__
#define __BIG_ENDIAN__
#endif
#ifdef __LITTLE_ENDIAN__
#undef __LITTLE_ENDIAN__
#endif
#endif

inline static uint16_t LESHORT(const uint16_t x) noexcept
{
	return nonstd::bit::as_little_endian(x);
}

inline static int16_t LESHORT(const int16_t x) noexcept
{
	return nonstd::bit::as_little_endian(x);
}

inline static uint32_t LELONG(const uint32_t x) noexcept
{
	return nonstd::bit::as_little_endian(x);
}

inline static int32_t LELONG(const int32_t x) noexcept
{
	return nonstd::bit::as_little_endian(x);
}

inline static uint64_t LELONGLONG(const uint64_t x) noexcept
{
	return nonstd::bit::as_little_endian(x);
}

inline static int64_t LELONGLONG(const int64_t x) noexcept
{
	return nonstd::bit::as_little_endian(x);
}

template <typename T, typename = std::enable_if_t<std::is_integral_v<T>>>
inline static T LESWAP(const T x) noexcept
{
	return nonstd::bit::as_little_endian(x);
}

inline static uint16_t BESHORT(const uint16_t x) noexcept
{
	return nonstd::bit::as_big_endian(x);
}

inline static int16_t BESHORT(const int16_t x) noexcept
{
	return nonstd::bit::as_big_endian(x);
}

inline static uint32_t BELONG(const uint32_t x) noexcept
{
	return nonstd::bit::as_big_endian(x);
}

inline static int32_t BELONG(const int32_t x) noexcept
{
	return nonstd::bit::as_big_endian(x);
}

inline static uint64_t BELONGLONG(const uint64_t x) noexcept
{
	return nonstd::bit::as_big_endian(x);
}

inline static int64_t BELONGLONG(const int64_t x) noexcept
{
	return nonstd::bit::as_big_endian(x);
}

template <typename T, typename = std::enable_if_t<std::is_integral_v<T>>>
inline static T BESWAP(const T x) noexcept
{
	return nonstd::bit::as_big_endian(x);
}
