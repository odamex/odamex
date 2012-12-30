// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 2006-2012 by The Odamex Team.
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
//		Fixed point implementation.
//
//-----------------------------------------------------------------------------


#include <stdio.h>
#include <stdlib.h>

#include "doomtype.h"
#include "i_system.h"

#include "m_fixed.h"


// C routines

fixed_t FixedMul_C (fixed_t a, fixed_t b)
{
	return (fixed_t)(((__int64) a * (__int64) b) >> FRACBITS);
}

fixed_t FixedDiv_C (fixed_t a, fixed_t b)
{
	if ((abs (a) >> 14) >= abs (b))
		return (a^b)<0 ? MININT : MAXINT;

	{
#if 0
		long long c;
		c = ((long long)a<<16) / ((long long)b);
		return (fixed_t) c;
#endif

		double c;

		c = ((double)a) / ((double)b) * FRACUNIT;
/*
	    if (c >= 2147483648.0 || c < -2147483648.0)
			I_FatalError("FixedDiv: divide by zero");
*/
		return (fixed_t) c;
	}
}


VERSION_CONTROL (m_fixed_cpp, "$Id$")

