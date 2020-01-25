// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1998-2006 by Randy Heit (ZDoom).
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
//	Particle effect thinkers
//
//-----------------------------------------------------------------------------


#include "doomtype.h"
#include "c_cvars.h"
#include "c_effect.h"

EXTERN_CVAR (cl_rockettrails)

//#define FADEFROMTTL(a)	(255/(a))



VERSION_CONTROL (p_effect_cpp, "$Id$")
