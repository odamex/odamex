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
//	 Type Definitions
//
// AUTHORS:
//	 Michael Wood (mwoodj at huntsvegas dot org)
//
//-----------------------------------------------------------------------------

#ifndef _TYPEDEFS_H
#define _TYPEDEFS_H

#include <memory>

#include <agar/gui.h>

#ifdef _MSC_VER
	typedef signed   __int8   int8_t;
	typedef signed   __int16  int16_t;
	typedef signed   __int32  int32_t;
	typedef unsigned __int8   uint8_t;
	typedef unsigned __int16  uint16_t;
	typedef unsigned __int32  uint32_t;
	typedef signed   __int64  int64_t;
	typedef unsigned __int64  uint64_t;
#else
	#include <stdint.h>
#endif

namespace agOdalaunch {

typedef std::auto_ptr<AG_Surface> AG_SurfacePtr;

} // namespace

#endif
