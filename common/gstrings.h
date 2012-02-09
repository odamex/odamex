// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1998-2011 by Randy Heit (ZDoom 1.22).
// Copyright (C) 2011-2012 by The Odamex Team.
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
//	GSTRINGS Define
//
//-----------------------------------------------------------------------------


#ifndef __GSTRINGS_H__
#define __GSTRINGS_H__

#ifdef _MSC_VER
#pragma once
#endif

#include "stringtable.h"
#include "stringenums.h"

extern FStringTable GStrings;

// QuitDOOM messages
#define NUM_QUITMESSAGES   15

extern const char *endmsg[];


#endif //__GSTRINGS_H__
