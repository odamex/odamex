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

#include "v_pnm.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

//
// Check for a comment and munch the rest of the line if so.
//
void PNMScanner::munchComment()
{
	// We're already off the end.
	if (_pos >= _len)
		return;

	// No comment, no problem.
	if (_data[_pos] != '#')
		return;

	for (;;)
	{
		// Ensure we're not off the end.
		if (_pos >= _len)
			break;

		// Checking for CR.
		if (_data[_pos] == '\r')
		{
			_pos += 1;
			break;
		}


		// Checking for LF.
		if (_data[_pos] == '\n')
		{
			_pos += 1;
			break;
		}

		_pos += 1;
	}
}

PNMScanner::PNMScanner(const uint8_t* data, size_t len) :
	_data(data), _len(len), _pos(0) { }

//
// Get a pointer to the current byte position.
//
const uint8_t* PNMScanner::getRawBytes()
{
	return _data + _pos;
}

//
// Return the number of bytes left in the file.
//
size_t PNMScanner::getBytesLeft()
{
	return _len - _pos;
}

//
// Read the opening magic number of the file.
//
bool PNMScanner::readMagic(int* m)
{
	// Must be enough room for P#
	if (_pos + 2 > _len)
		return false;

	// First character must be 'P'
	if (_data[_pos] != 'P')
		return false;

	_pos += 1;
	// Second character must be a number.
	if (_data[_pos] < '0' || _data[_pos] > '9')
		return false;

	// ASCII is laid out to allow this chicanery.
	*m = _data[_pos] - '0';

	_pos += 1;
	munchComment();
	return true;
}

//
// Read an integer dimension.
//
bool PNMScanner::readInt(int* i)
{
	char buf[16];
	size_t idx = 0;
	memset(&buf[0], 0x00, 16);

	for (;;)
	{
		// We're done here.
		if (idx >= 16 - sizeof('\0'))
			break;

		// Ran off the end of the file.
		if (_pos >= _len)
			break;

		// Non-numeric character.
		if (_data[_pos] < '0' || _data[_pos] > '9')
			break;

		buf[idx] = _data[_pos];

		_pos += 1;
		idx += 1;
	}

	// We must have found at least one number.
	if (idx == 0)
		return false;

	*i = atoi(&buf[0]);
	munchComment();
	return true;
}

//
// Skip past whitespace - if there is none, that's a fail.
//
bool PNMScanner::munchWhitespace()
{
	for (;;)
	{
		// Ran off the end of the file.
		if (_pos >= _len)
			break;

		// Found a non-space.
		if (!isspace(_data[_pos]))
			break;

		_pos += 1;
	}

	munchComment();
	return true;
}

bool V_ReadPBM(PNMScanner& sc, PBMData& pbm)
{
	int m = 0, w = 0, h = 0;
	if (!sc.readMagic(&m) || m != 4)
		return false;

	if (!sc.munchWhitespace())
		return false;

	if (!sc.readInt(&w))
		return false;

	if (!sc.munchWhitespace())
		return false;

	if (!sc.readInt(&h))
		return false;

	if (!sc.munchWhitespace())
		return false;

	pbm.m = m;
	pbm.w = w;
	pbm.h = h;
	pbm.data = sc.getRawBytes();
	pbm.len = sc.getBytesLeft();

	return true;
}
