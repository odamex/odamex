// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 2006-2020 by The Odamex Team.
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

using fixed_t = int;			// fixed 16.16
using dsfixed_t = unsigned int;	// fixedpt used by span drawer

//
// Fixed Point / Floating Point Conversion
//
constexpr float FIXED2FLOAT(fixed_t x)
{
	return x * (1.0f / float(FRACUNIT));
}

constexpr double FIXED2DOUBLE(fixed_t x)
{
	return x * (1.0 / double(FRACUNIT));
}

constexpr fixed_t FLOAT2FIXED(float x)
{
	return fixed_t(x * float(FRACUNIT));
}

constexpr fixed_t DOUBLE2FIXED(double x)
{
	return fixed_t(x * double(FRACUNIT));
}

constexpr int FIXED2INT(fixed_t x)
{
	return (x + FRACUNIT / 2) / FRACUNIT;
}

constexpr fixed_t INT2FIXED(int x)
{
	return x << FRACBITS;
}


//
// Fixed Point Multiplication for 16.16 precision
//
constexpr static fixed_t FixedMul(fixed_t a, fixed_t b)
{
	return (fixed_t)(((int64_t)a * b) >> FRACBITS);
}

//
// Fixed Point Division for 16.16 precision
//
constexpr static fixed_t FixedDiv(fixed_t a, fixed_t b)
{
	return (abs(a) >> 14) >= abs(b) ? ((a ^ b) >> 31) ^ MAXINT :
		(fixed_t)(((int64_t)a << FRACBITS) / b);
}

//
// Fixed-point muliplication for non 16.16 precision
//
static constexpr int32_t FixedMul1(int32_t a, int32_t b)
{	return (int32_t)(((int64_t)a * b) >> 1);	}

static constexpr int32_t FixedMul2(int32_t a, int32_t b)
{	return (int32_t)(((int64_t)a * b) >> 2);	}

static constexpr int32_t FixedMul3(int32_t a, int32_t b)
{	return (int32_t)(((int64_t)a * b) >> 3);	}

static constexpr int32_t FixedMul4(int32_t a, int32_t b)
{	return (int32_t)(((int64_t)a * b) >> 4);	}

static constexpr int32_t FixedMul5(int32_t a, int32_t b)
{	return (int32_t)(((int64_t)a * b) >> 5);	}

static constexpr int32_t FixedMul6(int32_t a, int32_t b)
{	return (int32_t)(((int64_t)a * b) >> 6);	}

static constexpr int32_t FixedMul7(int32_t a, int32_t b)
{	return (int32_t)(((int64_t)a * b) >> 7);	}

static constexpr int32_t FixedMul8(int32_t a, int32_t b)
{	return (int32_t)(((int64_t)a * b) >> 8);	}

static constexpr int32_t FixedMul9(int32_t a, int32_t b)
{	return (int32_t)(((int64_t)a * b) >> 9);	}

static constexpr int32_t FixedMul10(int32_t a, int32_t b)
{	return (int32_t)(((int64_t)a * b) >> 10);	}

static constexpr int32_t FixedMul11(int32_t a, int32_t b)
{	return (int32_t)(((int64_t)a * b) >> 11);	}

static constexpr int32_t FixedMul12(int32_t a, int32_t b)
{	return (int32_t)(((int64_t)a * b) >> 12);	}

static constexpr int32_t FixedMul13(int32_t a, int32_t b)
{	return (int32_t)(((int64_t)a * b) >> 13);	}

static constexpr int32_t FixedMul14(int32_t a, int32_t b)
{	return (int32_t)(((int64_t)a * b) >> 14);	}

static constexpr int32_t FixedMul15(int32_t a, int32_t b)
{	return (int32_t)(((int64_t)a * b) >> 15);	}

static constexpr int32_t FixedMul16(int32_t a, int32_t b)
{	return (int32_t)(((int64_t)a * b) >> 16);	}

static constexpr int32_t FixedMul17(int32_t a, int32_t b)
{	return (int32_t)(((int64_t)a * b) >> 17);	}

static constexpr int32_t FixedMul18(int32_t a, int32_t b)
{	return (int32_t)(((int64_t)a * b) >> 18);	}

static constexpr int32_t FixedMul19(int32_t a, int32_t b)
{	return (int32_t)(((int64_t)a * b) >> 19);	}

static constexpr int32_t FixedMul20(int32_t a, int32_t b)
{	return (int32_t)(((int64_t)a * b) >> 20);	}

static constexpr int32_t FixedMul21(int32_t a, int32_t b)
{	return (int32_t)(((int64_t)a * b) >> 21);	}

static constexpr int32_t FixedMul22(int32_t a, int32_t b)
{	return (int32_t)(((int64_t)a * b) >> 22);	}

static constexpr int32_t FixedMul23(int32_t a, int32_t b)
{	return (int32_t)(((int64_t)a * b) >> 23);	}

static constexpr int32_t FixedMul24(int32_t a, int32_t b)
{	return (int32_t)(((int64_t)a * b) >> 24);	}

static constexpr int32_t FixedMul25(int32_t a, int32_t b)
{	return (int32_t)(((int64_t)a * b) >> 25);	}

static constexpr int32_t FixedMul26(int32_t a, int32_t b)
{	return (int32_t)(((int64_t)a * b) >> 26);	}

static constexpr int32_t FixedMul27(int32_t a, int32_t b)
{	return (int32_t)(((int64_t)a * b) >> 27);	}

static constexpr int32_t FixedMul28(int32_t a, int32_t b)
{	return (int32_t)(((int64_t)a * b) >> 28);	}

static constexpr int32_t FixedMul29(int32_t a, int32_t b)
{	return (int32_t)(((int64_t)a * b) >> 29);	}

static constexpr int32_t FixedMul30(int32_t a, int32_t b)
{	return (int32_t)(((int64_t)a * b) >> 30);	}

static constexpr int32_t FixedMul31(int32_t a, int32_t b)
{	return (int32_t)(((int64_t)a * b) >> 31);	}

static constexpr int32_t FixedMul32(int32_t a, int32_t b)
{	return (int32_t)(((int64_t)a * b) >> 32);	}

// Fixed-point division for non 16.16 precision
static constexpr int32_t FixedDiv1(int32_t a, int32_t b)
{	return (int32_t)(((int64_t)a << 1) / b);	}

static constexpr int32_t FixedDiv2(int32_t a, int32_t b)
{	return (int32_t)(((int64_t)a << 2) / b);	}

static constexpr int32_t FixedDiv3(int32_t a, int32_t b)
{	return (int32_t)(((int64_t)a << 3) / b);	}

static constexpr int32_t FixedDiv4(int32_t a, int32_t b)
{	return (int32_t)(((int64_t)a << 4) / b);	}

static constexpr int32_t FixedDiv5(int32_t a, int32_t b)
{	return (int32_t)(((int64_t)a << 5) / b);	}

static constexpr int32_t FixedDiv6(int32_t a, int32_t b)
{	return (int32_t)(((int64_t)a << 6) / b);	}

static constexpr int32_t FixedDiv7(int32_t a, int32_t b)
{	return (int32_t)(((int64_t)a << 7) / b);	}

static constexpr int32_t FixedDiv8(int32_t a, int32_t b)
{	return (int32_t)(((int64_t)a << 8) / b);	}

static constexpr int32_t FixedDiv9(int32_t a, int32_t b)
{	return (int32_t)(((int64_t)a << 9) / b);	}

static constexpr int32_t FixedDiv10(int32_t a, int32_t b)
{	return (int32_t)(((int64_t)a << 10) / b);	}

static constexpr int32_t FixedDiv11(int32_t a, int32_t b)
{	return (int32_t)(((int64_t)a << 11) / b);	}

static constexpr int32_t FixedDiv12(int32_t a, int32_t b)
{	return (int32_t)(((int64_t)a << 12) / b);	}

static constexpr int32_t FixedDiv13(int32_t a, int32_t b)
{	return (int32_t)(((int64_t)a << 13) / b);	}

static constexpr int32_t FixedDiv14(int32_t a, int32_t b)
{	return (int32_t)(((int64_t)a << 14) / b);	}

static constexpr int32_t FixedDiv15(int32_t a, int32_t b)
{	return (int32_t)(((int64_t)a << 15) / b);	}

static constexpr int32_t FixedDiv16(int32_t a, int32_t b)
{	return (int32_t)(((int64_t)a << 16) / b);	}

static constexpr int32_t FixedDiv17(int32_t a, int32_t b)
{	return (int32_t)(((int64_t)a << 17) / b);	}

static constexpr int32_t FixedDiv18(int32_t a, int32_t b)
{	return (int32_t)(((int64_t)a << 18) / b);	}

static constexpr int32_t FixedDiv19(int32_t a, int32_t b)
{	return (int32_t)(((int64_t)a << 19) / b);	}

static constexpr int32_t FixedDiv20(int32_t a, int32_t b)
{	return (int32_t)(((int64_t)a << 20) / b);	}

static constexpr int32_t FixedDiv21(int32_t a, int32_t b)
{	return (int32_t)(((int64_t)a << 21) / b);	}

static constexpr int32_t FixedDiv22(int32_t a, int32_t b)
{	return (int32_t)(((int64_t)a << 22) / b);	}

static constexpr int32_t FixedDiv23(int32_t a, int32_t b)
{	return (int32_t)(((int64_t)a << 23) / b);	}

static constexpr int32_t FixedDiv24(int32_t a, int32_t b)
{	return (int32_t)(((int64_t)a << 24) / b);	}

static constexpr int32_t FixedDiv25(int32_t a, int32_t b)
{	return (int32_t)(((int64_t)a << 25) / b);	}

static constexpr int32_t FixedDiv26(int32_t a, int32_t b)
{	return (int32_t)(((int64_t)a << 26) / b);	}

static constexpr int32_t FixedDiv27(int32_t a, int32_t b)
{	return (int32_t)(((int64_t)a << 27) / b);	}

static constexpr int32_t FixedDiv28(int32_t a, int32_t b)
{	return (int32_t)(((int64_t)a << 28) / b);	}

static constexpr int32_t FixedDiv29(int32_t a, int32_t b)
{	return (int32_t)(((int64_t)a << 29) / b);	}

static constexpr int32_t FixedDiv30(int32_t a, int32_t b)
{	return (int32_t)(((int64_t)a << 30) / b);	}

static constexpr int32_t FixedDiv31(int32_t a, int32_t b)
{	return (int32_t)(((int64_t)a << 31) / b);	}

static constexpr int32_t FixedDiv32(int32_t a, int32_t b)
{	return (int32_t)(((int64_t)a << 32) / b);	}
