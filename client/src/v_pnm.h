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
//   PNM image support.
//
//-----------------------------------------------------------------------------

#ifndef __V_PNM_H__
#define __V_PNM_H__

#include "doomdef.h"

class PNMScanner
{
	const unsigned char* _data;
	const size_t _len;
	size_t _pos;
	void munchComment();
public:
	PNMScanner(const uint8_t* data, size_t len);
	const uint8_t* getRawBytes();
	size_t getBytesLeft();
	bool readMagic(int* m);
	bool readInt(int* i);
	bool munchWhitespace();
};

struct PBMData
{
	int m;
	int w;
	int h;
	const uint8_t* data;
	size_t len;
};

bool V_ReadPBM(PNMScanner& sc, PBMData& pbm);

#endif // __V_PNM_H__
