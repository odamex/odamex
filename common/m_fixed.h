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
	return ((int32_t)FRAC64FILL(x >> FRACBITS64, x));
}

[[nodiscard]]
inline constexpr fixed64_t INT2FIXED64(int64_t x)
{
	return x << FRACBITS64;
}

[[nodiscard]]
inline constexpr fixed_t FIXED642FIXED(fixed64_t x)
{
	return (fixed_t)FRAC64FILLFIXED(x >> (FRACBITS64 - FRACBITS), x);
}

[[nodiscard]]
inline constexpr fixed64_t FIXED2FIXED64(fixed_t x)
{
	return (fixed64_t)x << (FRACBITS64 - FRACBITS);
}

//
// Fixed Point Multiplication for 16.16 precision
//
[[nodiscard]]
inline constexpr fixed_t FixedMul(fixed_t a, fixed_t b)
{
	return (fixed_t)(((int64_t)a * b) >> FRACBITS);
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
	constexpr auto absce = [](fixed_t x) -> fixed_t {
		return (x < 0) ? -x : x;
	};
	return (absce(a) >> 14) >= absce(b) ? ((a ^ b) >> 31) ^ MAXINT :
		(fixed_t)(((int64_t)a << FRACBITS) / b);
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
		return (a ^ b) < 0 ? MINLONG : MAXLONG;
	}
	return (a << FRACBITS64) / b;
}

//
// Fixed-point muliplication for non 16.16 precision
//
[[nodiscard]]
inline constexpr int32_t FixedMul1(int32_t a, int32_t b)
{	return (int32_t)(((int64_t)a * b) >> 1);	}

[[nodiscard]]
inline constexpr int32_t FixedMul2(int32_t a, int32_t b)
{	return (int32_t)(((int64_t)a * b) >> 2);	}

[[nodiscard]]
inline constexpr int32_t FixedMul3(int32_t a, int32_t b)
{	return (int32_t)(((int64_t)a * b) >> 3);	}

[[nodiscard]]
inline constexpr int32_t FixedMul4(int32_t a, int32_t b)
{	return (int32_t)(((int64_t)a * b) >> 4);	}

[[nodiscard]]
inline constexpr int32_t FixedMul5(int32_t a, int32_t b)
{	return (int32_t)(((int64_t)a * b) >> 5);	}

[[nodiscard]]
inline constexpr int32_t FixedMul6(int32_t a, int32_t b)
{	return (int32_t)(((int64_t)a * b) >> 6);	}

[[nodiscard]]
inline constexpr int32_t FixedMul7(int32_t a, int32_t b)
{	return (int32_t)(((int64_t)a * b) >> 7);	}

[[nodiscard]]
inline constexpr int32_t FixedMul8(int32_t a, int32_t b)
{	return (int32_t)(((int64_t)a * b) >> 8);	}

[[nodiscard]]
inline constexpr int32_t FixedMul9(int32_t a, int32_t b)
{	return (int32_t)(((int64_t)a * b) >> 9);	}

[[nodiscard]]
inline constexpr int32_t FixedMul10(int32_t a, int32_t b)
{	return (int32_t)(((int64_t)a * b) >> 10);	}

[[nodiscard]]
inline constexpr int32_t FixedMul11(int32_t a, int32_t b)
{	return (int32_t)(((int64_t)a * b) >> 11);	}

[[nodiscard]]
inline constexpr int32_t FixedMul12(int32_t a, int32_t b)
{	return (int32_t)(((int64_t)a * b) >> 12);	}

[[nodiscard]]
inline constexpr int32_t FixedMul13(int32_t a, int32_t b)
{	return (int32_t)(((int64_t)a * b) >> 13);	}

[[nodiscard]]
inline constexpr int32_t FixedMul14(int32_t a, int32_t b)
{	return (int32_t)(((int64_t)a * b) >> 14);	}

[[nodiscard]]
inline constexpr int32_t FixedMul15(int32_t a, int32_t b)
{	return (int32_t)(((int64_t)a * b) >> 15);	}

[[nodiscard]]
inline constexpr int32_t FixedMul16(int32_t a, int32_t b)
{	return (int32_t)(((int64_t)a * b) >> 16);	}

[[nodiscard]]
inline constexpr int32_t FixedMul17(int32_t a, int32_t b)
{	return (int32_t)(((int64_t)a * b) >> 17);	}

[[nodiscard]]
inline constexpr int32_t FixedMul18(int32_t a, int32_t b)
{	return (int32_t)(((int64_t)a * b) >> 18);	}

[[nodiscard]]
inline constexpr int32_t FixedMul19(int32_t a, int32_t b)
{	return (int32_t)(((int64_t)a * b) >> 19);	}

[[nodiscard]]
inline constexpr int32_t FixedMul20(int32_t a, int32_t b)
{	return (int32_t)(((int64_t)a * b) >> 20);	}

[[nodiscard]]
inline constexpr int32_t FixedMul21(int32_t a, int32_t b)
{	return (int32_t)(((int64_t)a * b) >> 21);	}

[[nodiscard]]
inline constexpr int32_t FixedMul22(int32_t a, int32_t b)
{	return (int32_t)(((int64_t)a * b) >> 22);	}

[[nodiscard]]
inline constexpr int32_t FixedMul23(int32_t a, int32_t b)
{	return (int32_t)(((int64_t)a * b) >> 23);	}

[[nodiscard]]
inline constexpr int32_t FixedMul24(int32_t a, int32_t b)
{	return (int32_t)(((int64_t)a * b) >> 24);	}

[[nodiscard]]
inline constexpr int32_t FixedMul25(int32_t a, int32_t b)
{	return (int32_t)(((int64_t)a * b) >> 25);	}

[[nodiscard]]
inline constexpr int32_t FixedMul26(int32_t a, int32_t b)
{	return (int32_t)(((int64_t)a * b) >> 26);	}

[[nodiscard]]
inline constexpr int32_t FixedMul27(int32_t a, int32_t b)
{	return (int32_t)(((int64_t)a * b) >> 27);	}

[[nodiscard]]
inline constexpr int32_t FixedMul28(int32_t a, int32_t b)
{	return (int32_t)(((int64_t)a * b) >> 28);	}

[[nodiscard]]
inline constexpr int32_t FixedMul29(int32_t a, int32_t b)
{	return (int32_t)(((int64_t)a * b) >> 29);	}

[[nodiscard]]
inline constexpr int32_t FixedMul30(int32_t a, int32_t b)
{	return (int32_t)(((int64_t)a * b) >> 30);	}

[[nodiscard]]
inline constexpr int32_t FixedMul31(int32_t a, int32_t b)
{	return (int32_t)(((int64_t)a * b) >> 31);	}

[[nodiscard]]
inline constexpr int32_t FixedMul32(int32_t a, int32_t b)
{	return (int32_t)(((int64_t)a * b) >> 32);	}

// Fixed-point division for non 16.16 precision
[[nodiscard]]
inline constexpr int32_t FixedDiv1(int32_t a, int32_t b)
{	return (int32_t)(((int64_t)a << 1) / b);	}

[[nodiscard]]
inline constexpr int32_t FixedDiv2(int32_t a, int32_t b)
{	return (int32_t)(((int64_t)a << 2) / b);	}

[[nodiscard]]
inline constexpr int32_t FixedDiv3(int32_t a, int32_t b)
{	return (int32_t)(((int64_t)a << 3) / b);	}

[[nodiscard]]
inline constexpr int32_t FixedDiv4(int32_t a, int32_t b)
{	return (int32_t)(((int64_t)a << 4) / b);	}

[[nodiscard]]
inline constexpr int32_t FixedDiv5(int32_t a, int32_t b)
{	return (int32_t)(((int64_t)a << 5) / b);	}

[[nodiscard]]
inline constexpr int32_t FixedDiv6(int32_t a, int32_t b)
{	return (int32_t)(((int64_t)a << 6) / b);	}

[[nodiscard]]
inline constexpr int32_t FixedDiv7(int32_t a, int32_t b)
{	return (int32_t)(((int64_t)a << 7) / b);	}

[[nodiscard]]
inline constexpr int32_t FixedDiv8(int32_t a, int32_t b)
{	return (int32_t)(((int64_t)a << 8) / b);	}

[[nodiscard]]
inline constexpr int32_t FixedDiv9(int32_t a, int32_t b)
{	return (int32_t)(((int64_t)a << 9) / b);	}

[[nodiscard]]
inline constexpr int32_t FixedDiv10(int32_t a, int32_t b)
{	return (int32_t)(((int64_t)a << 10) / b);	}

[[nodiscard]]
inline constexpr int32_t FixedDiv11(int32_t a, int32_t b)
{	return (int32_t)(((int64_t)a << 11) / b);	}

[[nodiscard]]
inline constexpr int32_t FixedDiv12(int32_t a, int32_t b)
{	return (int32_t)(((int64_t)a << 12) / b);	}

[[nodiscard]]
inline constexpr int32_t FixedDiv13(int32_t a, int32_t b)
{	return (int32_t)(((int64_t)a << 13) / b);	}

[[nodiscard]]
inline constexpr int32_t FixedDiv14(int32_t a, int32_t b)
{	return (int32_t)(((int64_t)a << 14) / b);	}

[[nodiscard]]
inline constexpr int32_t FixedDiv15(int32_t a, int32_t b)
{	return (int32_t)(((int64_t)a << 15) / b);	}

[[nodiscard]]
inline constexpr int32_t FixedDiv16(int32_t a, int32_t b)
{	return (int32_t)(((int64_t)a << 16) / b);	}

[[nodiscard]]
inline constexpr int32_t FixedDiv17(int32_t a, int32_t b)
{	return (int32_t)(((int64_t)a << 17) / b);	}

[[nodiscard]]
inline constexpr int32_t FixedDiv18(int32_t a, int32_t b)
{	return (int32_t)(((int64_t)a << 18) / b);	}

[[nodiscard]]
inline constexpr int32_t FixedDiv19(int32_t a, int32_t b)
{	return (int32_t)(((int64_t)a << 19) / b);	}

[[nodiscard]]
inline constexpr int32_t FixedDiv20(int32_t a, int32_t b)
{	return (int32_t)(((int64_t)a << 20) / b);	}

[[nodiscard]]
inline constexpr int32_t FixedDiv21(int32_t a, int32_t b)
{	return (int32_t)(((int64_t)a << 21) / b);	}

[[nodiscard]]
inline constexpr int32_t FixedDiv22(int32_t a, int32_t b)
{	return (int32_t)(((int64_t)a << 22) / b);	}

[[nodiscard]]
inline constexpr int32_t FixedDiv23(int32_t a, int32_t b)
{	return (int32_t)(((int64_t)a << 23) / b);	}

[[nodiscard]]
inline constexpr int32_t FixedDiv24(int32_t a, int32_t b)
{	return (int32_t)(((int64_t)a << 24) / b);	}

[[nodiscard]]
inline constexpr int32_t FixedDiv25(int32_t a, int32_t b)
{	return (int32_t)(((int64_t)a << 25) / b);	}

[[nodiscard]]
inline constexpr int32_t FixedDiv26(int32_t a, int32_t b)
{	return (int32_t)(((int64_t)a << 26) / b);	}

[[nodiscard]]
inline constexpr int32_t FixedDiv27(int32_t a, int32_t b)
{	return (int32_t)(((int64_t)a << 27) / b);	}

[[nodiscard]]
inline constexpr int32_t FixedDiv28(int32_t a, int32_t b)
{	return (int32_t)(((int64_t)a << 28) / b);	}

[[nodiscard]]
inline constexpr int32_t FixedDiv29(int32_t a, int32_t b)
{	return (int32_t)(((int64_t)a << 29) / b);	}

[[nodiscard]]
inline constexpr int32_t FixedDiv30(int32_t a, int32_t b)
{	return (int32_t)(((int64_t)a << 30) / b);	}

[[nodiscard]]
inline constexpr int32_t FixedDiv31(int32_t a, int32_t b)
{	return (int32_t)(((int64_t)a << 31) / b);	}

[[nodiscard]]
inline constexpr int32_t FixedDiv32(int32_t a, int32_t b)
{	return (int32_t)(((int64_t)a << 32) / b);	}
