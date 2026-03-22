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
//	Fixed point arithemtics, implementation.
//
//-----------------------------------------------------------------------------

#pragma once

#include <stdlib.h>

//
// Fixed point, 32bit as 16.16.
//
#define FRACBITS				16
#define FRACUNIT				(1<<FRACBITS)

using fixed_t   = int32_t;  // fixed 16.16
using fixed64_t = int64_t;  // fixed 44.20
using dsfixed_t = uint32_t;	// fixedpt used by span drawer

#define FRACBITS64				20ll
#define FRACUNIT64				(1ll<<FRACBITS64)
#define FRAC64MASK				(FRACUNIT64 - 1ll)
#define FRAC64FILL( x, o )		( ( x ) | ( ( o ) < 0 ? ( FRAC64MASK << ( 64 - FRACBITS64 ) ) : 0 ) )
#define FRAC64FILLFIXED( x, o )	( ( x ) | ( ( o ) < 0 ? ~( (( 1ll << ( 64 - ( FRACBITS64 - FRACBITS ) )) - 1 ) ) : 0 ) )

//
// Fixed Point / Floating Point Conversion
//
[[nodiscard]]
inline constexpr float FIXED2FLOAT(fixed_t x)
{
	return x * (1.0f / float(FRACUNIT));
}

[[nodiscard]]
inline constexpr double FIXED2DOUBLE(fixed_t x)
{
	return x * (1.0 / double(FRACUNIT));
}

[[nodiscard]]
inline constexpr fixed_t FLOAT2FIXED(float x)
{
	return fixed_t(x * float(FRACUNIT));
}

[[nodiscard]]
inline constexpr fixed_t DOUBLE2FIXED(double x)
{
	return fixed_t(x * double(FRACUNIT));
}

[[nodiscard]]
inline constexpr int32_t FIXED2INT(fixed_t x)
{
	return (x + FRACUNIT / 2) >> FRACBITS;
}

[[nodiscard]]
inline constexpr fixed_t INT2FIXED(int x)
{
	return x << FRACBITS;
}

[[nodiscard]]
inline constexpr float FIXED642FLOAT(fixed64_t x)
{
	return x * (1.0f / float(FRACUNIT64));
}

[[nodiscard]]
inline constexpr double FIXED642DOUBLE(fixed64_t x)
{
	return x * (1.0f / double(FRACUNIT64));
}

[[nodiscard]]
inline constexpr fixed64_t FLOAT2FIXED64(float x)
{
	return fixed64_t(x * float(FRACUNIT64));
}

[[nodiscard]]
inline constexpr fixed64_t DOUBLE2FIXED64(double x)
{
	return fixed64_t(x * double(FRACUNIT64));
}

[[nodiscard]]
inline constexpr int32_t FIXED642INT(fixed64_t x)
{
	return static_cast<int32_t>(FRAC64FILL(x >> FRACBITS64, x));
}

[[nodiscard]]
inline constexpr fixed64_t INT2FIXED64(int64_t x)
{
	return x << FRACBITS64;
}

[[nodiscard]]
inline constexpr fixed_t FIXED642FIXED(fixed64_t x)
{
	return static_cast<fixed_t>(FRAC64FILLFIXED(x >> (FRACBITS64 - FRACBITS), x));
}

[[nodiscard]]
inline constexpr fixed64_t FIXED2FIXED64(fixed_t x)
{
	return static_cast<fixed64_t>(x) << (FRACBITS64 - FRACBITS);
}

//
// Fixed Point Multiplication for 16.16 precision
//
[[nodiscard]]
inline constexpr fixed_t FixedMul(fixed_t a, fixed_t b)
{
	return static_cast<fixed_t>((static_cast<int64_t>(a) * b) >> FRACBITS);
}

//
// Fixed Point Multiplication for 44.20 precision
//
[[nodiscard]]
inline constexpr fixed64_t FixedMul64( fixed64_t a, fixed64_t b )
{
	fixed64_t result = (a * b) >> FRACBITS64;
	return FRAC64FILL(result, result);
}

//
// Fixed Point Division for 16.16 precision
//
[[nodiscard]]
inline constexpr fixed_t FixedDiv(fixed_t a, fixed_t b)
{
	// TODO: C++23, just use abs
	constexpr auto absce = [](fixed_t x) -> fixed_t {
		return (x < 0) ? -x : x;
	};
	return (absce(a) >> 14) >= absce(b) ? ((a ^ b) >> 31) ^ limits::MAXINT :
		static_cast<fixed_t>((static_cast<int64_t>(a) << FRACBITS) / b);
}

[[nodiscard]]
inline constexpr fixed64_t FixedAbs64( fixed64_t val )
{
	fixed64_t sign = val >> 63ll;
	return (val ^ sign) - sign;
}

//
// Fixed Point Division for 44.20 precision
//
[[nodiscard]]
inline constexpr fixed64_t FixedDiv64( fixed64_t a, fixed64_t b )
{
	if ((FixedAbs64(a) >> (FRACBITS64 - 2)) >= FixedAbs64(b))
	{
		// fixme: should this really use minlong and maxlong?
		// thats what R&R uses but seems wrong for 64 bit systems
		return (a ^ b) < 0 ? limits::MINLONG : limits::MAXLONG;
	}
	return (a << FRACBITS64) / b;
}

//
// Fixed-point muliplication for non 16.16 precision
//
template <int N>
[[nodiscard]]
inline constexpr int32_t FixedMulN(int32_t a, int32_t b)
{
	static_assert(N >= 1 && N <= 32, "Shift out of range");
	return static_cast<int32_t>((static_cast<int64_t>(a) * b) >> N);
}

//
// Fixed-point division for non 16.16 precision
//
template <int N>
[[nodiscard]]
inline constexpr int32_t FixedDivN(int32_t a, int32_t b)
{
	static_assert(N >= 1 && N <= 32, "Shift out of range");
	return static_cast<int32_t>((static_cast<int64_t>(a) << N) / b);
}
