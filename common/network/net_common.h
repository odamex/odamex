// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2006-2015 by The Odamex Team.
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
//  
// Miscellaneous functions common to the networking subsystem 
//
//-----------------------------------------------------------------------------

#ifndef __NET_COMMON_H__
#define __NET_COMMON_H__

#include "doomtype.h"

uint32_t Net_CRC32(const uint8_t* buf, uint32_t len);
uint32_t Net_BitsNeeded(uint32_t n);

const dtime_t ONE_SECOND = 1000000000uLL;
dtime_t Net_CurrentTime();

static inline uint32_t BitsToBytes(uint32_t bits)
{
	return (bits + 7) >> 3;
}

static inline uint32_t BytesToBits(uint32_t bytes)
{
	return bytes << 3;
}


#endif	// __NET_COMMON_H__

