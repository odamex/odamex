// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1998-2011 by Randy Heit (ZDoom 1.23).
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
		LumpNum (-1) {}
	~FStringTable () { FreeData (); }

	void LoadStrings (int lump, int expectedSize, bool enuOnly);
	void ReloadStrings ();
	void ResetStrings ();

	void LoadNames () const;
	void FlushNames () const;
	int FindString (const char *stringName) const;
	int MatchString (const char *string) const;

	void SetString (int index, const char *newString);
	void Compact ();
	const char *operator() (int index) { return Strings[index]; }

private:
	struct Header;

	BYTE *StringStatus;
	int NumStrings;
	mutable BYTE *Names;
	char **Strings;
	char *CompactBase;
	size_t CompactSize;
	int LumpNum;

	void FreeData ();
	void FreeStrings ();
	void FreeStandardStrings ();
	int SumStringSizes () const;
	int LoadLanguage (DWORD code, bool exactMatch, BYTE *startPos, BYTE *endPos);
	void DoneLoading (BYTE *startPos, BYTE *endPos);
};

#endif //__STRINGTABLE_H__
