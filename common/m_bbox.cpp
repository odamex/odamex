// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 2006-2013 by The Odamex Team.
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
//		Main loop menu stuff.
//		Random number LUT.
//		Default Config File.
//		PCX Screenshots.
//    
//-----------------------------------------------------------------------------


#include <stdio.h>

#include "m_bbox.h"

IMPLEMENT_CLASS (DBoundingBox, DObject)

DBoundingBox::DBoundingBox ()
{
	ClearBox ();
}

void DBoundingBox::ClearBox ()
{
	m_Box[BOXTOP] = m_Box[BOXRIGHT] = MININT;
	m_Box[BOXBOTTOM] = m_Box[BOXLEFT] = MAXINT;
}

void DBoundingBox::AddToBox (fixed_t x, fixed_t y)
{
	if (x < m_Box[BOXLEFT])
		m_Box[BOXLEFT] = x;
	else if (x > m_Box[BOXRIGHT])
		m_Box[BOXRIGHT] = x;

	if (y < m_Box[BOXBOTTOM])
		m_Box[BOXBOTTOM] = y;
	else if (y > m_Box[BOXTOP])
		m_Box[BOXTOP] = y;
}






VERSION_CONTROL (m_bbox_cpp, "$Id$")

