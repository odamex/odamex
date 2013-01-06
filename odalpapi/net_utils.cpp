// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
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
//	Utility functions
//
// AUTHORS: 
//  Russell Rice (russell at odamex dot net)
//  Michael Wood (mwoodj at knology dot net)
//
//-----------------------------------------------------------------------------

#include <iostream>
#include <sys/time.h>

#include <limits>

#include "net_utils.h"

namespace odalpapi {

// GetMillisNow()

// denis - use this unless you want your program
// to get confused every 49 days due to DWORD limit
uint64_t _UnwrapTime(uint32_t now32)
{
	static uint64_t last = 0;
	uint64_t now = now32;
	static uint64_t max = std::numeric_limits<uint32_t>::max();

	if(now < last%max)
		last += (max-(last%max)) + now;
	else
		last = now;

	return last;
}

int32_t _Millis()
{
    struct timeval tp;
    
    gettimeofday(&tp, (struct timezone *)NULL);

    return (tp.tv_sec * 1000 + (tp.tv_usec / 1000));
}

uint64_t GetMillisNow()
{
    return _UnwrapTime(_Millis());
}

} // namespace

// ???
// ---
