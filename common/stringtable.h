// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1998-2011 by Randy Heit (ZDoom 1.23).
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
// FStringTable
//
// This class manages a list of localizable strings stored in a wad file.
// It does not support adding new strings, although existing ones can
// be changed.
//
//-----------------------------------------------------------------------------

#ifndef __STRINGTABLE_H__
#define __STRINGTABLE_H__

#ifdef _MSC_VER
#pragma once
#endif


#include <stdlib.h>
#include "doomtype.h"

class FStringTable
{
public:
	FStringTable () :
		StringStatus(NULL),
		NumStrings (0),
		Names(NULL),
		Strings(NULL),
		CompactBase(NULL),
		CompactSize(0),
		LumpNum(-1),
		LumpData(NULL) {}

	~FStringTable () { FreeData (); }

	void LoadStrings(int lumpnum, int expectedSize, bool enuOnly);
	void ReloadStrings();
	void ResetStrings();

	void LoadNames() const;
	void FlushNames() const;
	int FindString(const char* stringName) const;
	int MatchString(const char* string) const;

	void SetString(int index, const char* newString);
	void Compact();

	void FreeData();

	const char *operator() (int index)
	{
		// [SL] ensure index is sane
		if (index >= 0 && index < NumStrings)
			return Strings[index];

		// invalid index, return an empty cstring
		static const char emptystr = 0;
		return &emptystr;
	}

private:
	struct Header;

	byte* StringStatus;
	int NumStrings;
	mutable byte* Names;
	char** Strings;
	char* CompactBase;
	size_t CompactSize;
	int LumpNum;
	byte* LumpData;

	void FreeStrings();
	void FreeStandardStrings();
	int SumStringSizes() const;
	int LoadLanguage(uint32_t code, bool exactMatch, byte* startPos, byte* endPos);
	void DoneLoading(byte* startPos, byte* endPos);
};

#endif //__STRINGTABLE_H__
