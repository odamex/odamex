// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1998-2006 by Randy Heit (ZDoom).
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
//	String Abstraction Layer (FStringTable)
//
//-----------------------------------------------------------------------------
//


#include <string.h>
#include <stddef.h>

#include "stringtable.h"
#include "cmdlib.h"
#include "m_swap.h"
#include "w_wad.h"
#include "z_zone.h"
#include "i_system.h"

struct FStringTable::Header
{
	DWORD FileSize;
	WORD NameCount;
	WORD NameLen;
};

void FStringTable::FreeData ()
{
	if (Strings != NULL)
	{
		FreeStrings ();
	}

	if (StringStatus)	delete[] StringStatus;
	if (Strings)		delete[] Strings;
	if (Names)			delete[] Names;

	StringStatus = NULL;
	Strings = NULL;
	Names = NULL;
	NumStrings = 0;
}

void FStringTable::FreeStrings ()
{
	for (int i = 0; i < NumStrings; ++i)
	{
		if (Strings[i] < CompactBase ||
			Strings[i] >= CompactBase + CompactSize)
		{
			delete[] Strings[i];
		}
	}
	if (CompactBase)
	{
		delete[] CompactBase;
		CompactBase = NULL;
		CompactSize = 0;
	}
}

void FStringTable::FreeStandardStrings ()
{
	if (Strings != NULL)
	{
		for (int i = 0; i < NumStrings; ++i)
		{
			if ((StringStatus[i/8] & (1<<(i&7))) == 0)
			{
				if (Strings[i] < CompactBase ||
					Strings[i] >= CompactBase + CompactSize)
				{
					delete[] Strings[i];
				}
				Strings[i] = NULL;
			}
		}
	}
}

#include "errors.h"
void FStringTable::LoadStrings (int lump, int expectedSize, bool enuOnly)
{
	BYTE *strData = (BYTE *)W_CacheLumpNum (lump, PU_CACHE);
	int lumpLen = LELONG(((Header *)strData)->FileSize);
	int nameCount = LESHORT(((Header *)strData)->NameCount);
	int nameLen = LESHORT(((Header *)strData)->NameLen);

	int languageStart = sizeof(Header) + nameCount*4 + nameLen;
	languageStart += (4 - languageStart) & 3;

	if (expectedSize >= 0 && nameCount != expectedSize)
	{
		char name[9];

		W_GetLumpName (name, lump);
		name[8] = 0;
		I_FatalError ("%s had %d strings.\nThis version of ZDoom expects it to have %d.",
			name, nameCount, expectedSize);
	}

	FreeStandardStrings ();

	NumStrings = nameCount;
	LumpNum = lump;
	if (Strings == NULL)
	{
		Strings = new char *[nameCount];
		StringStatus = new BYTE[(nameCount+7)/8];
		memset (StringStatus, 0, (nameCount+7)/8);	// 0 means: from wad (standard)
		memset (Strings, 0, sizeof(char *)*nameCount);
	}

	BYTE *const start = strData + languageStart;
	BYTE *const end = strData + lumpLen;
	int loadedCount, i;

	for (loadedCount = i = 0; i < NumStrings; ++i)
	{
		if (Strings[i] != NULL)
		{
			++loadedCount;
		}
	}

	if (!enuOnly)
	{
		for (i = 0; i < 4 && loadedCount != nameCount; ++i)
		{
			loadedCount += LoadLanguage (LanguageIDs[i], true, start, end);
			loadedCount += LoadLanguage (LanguageIDs[i] & MAKE_ID(0xff,0xff,0,0), true, start, end);
			loadedCount += LoadLanguage (LanguageIDs[i], false, start, end);
		}
	}

	// Fill in any missing strings with the default language (enu)
	if (loadedCount != nameCount)
	{
		loadedCount += LoadLanguage (MAKE_ID('e','n','u',0), true, start, end);
	}

	DoneLoading (start, end);

	if (loadedCount != nameCount)
	{
		I_FatalError ("Loaded %d strings (expected %d)", loadedCount, nameCount);
	}
}

void FStringTable::ReloadStrings ()
{
	LoadStrings (LumpNum, -1, false);
}

// Like ReloadStrings, but clears all the strings before reloading
void FStringTable::ResetStrings ()
{
	FreeData ();
	LoadStrings (LumpNum, -1, false);
}

int FStringTable::LoadLanguage (DWORD code, bool exactMatch, BYTE *start, BYTE *end)
{
	const DWORD orMask = exactMatch ? 0 : MAKE_ID(0,0,0xff,0);
	int count = 0;

	code |= orMask;

	while (start < end)
	{
		const DWORD langLen = LELONG(*(DWORD *)&start[4]);

		if (((*(DWORD *)start) | orMask) == code)
		{
			start[3] = 1;

			const BYTE *probe = start + 8;

			while (probe < start + langLen)
			{
				int index = probe[0] | (probe[1]<<8);

				if (Strings[index] == NULL)
				{
					Strings[index] = copystring ((const char *)(probe + 2));
					++count;
				}
				probe += 3 + strlen ((const char *)(probe + 2));
			}
		}

		start += langLen + 8;
		start += (4 - (ptrdiff_t)start) & 3;
	}
	return count;
}

void FStringTable::DoneLoading (BYTE *start, BYTE *end)
{
	while (start < end)
	{
		start[3] = 0;
		start += LELONG(*(DWORD *)&start[4]) + 8;
		start += (4 - (ptrdiff_t)start) & 3;
	}
}

void FStringTable::SetString (int index, const char *newString)
{
	if ((unsigned)index >= (unsigned)NumStrings)
		return;

	if (Strings[index] < CompactBase ||
		Strings[index] >= CompactBase + CompactSize)
	{
		delete[] Strings[index];
	}
	Strings[index] = copystring (newString);
	StringStatus[index/8] |= 1<<(index&7);
}

// Compact all strings into a single block of memory
void FStringTable::Compact ()
{
	if (NumStrings == 0)
		return;

	int len = SumStringSizes ();
	char *newspace = new char[len];
	char *pos = newspace;
	int i;

	for (i = 0; i < NumStrings; ++i)
	{
		strcpy (pos, Strings[i]);
		pos += strlen (pos) + 1;
	}

	FreeStrings ();

	pos = newspace;
	for (i = 0; i < NumStrings; ++i)
	{
		Strings[i] = pos;
		pos += strlen (pos) + 1;
	}

	CompactBase = newspace;
	CompactSize = len;
}

int FStringTable::SumStringSizes () const
{
	int len;
	int i;

	for (i = len = 0; i < NumStrings; ++i)
	{
		len += strlen (Strings[i]) + 1;
	}
	return len;
}


void FStringTable::LoadNames () const
{
	BYTE *lump = (BYTE *)W_CacheLumpNum (LumpNum, PU_CACHE);
	int nameLen = LESHORT(((Header *)lump)->NameLen);

	FlushNames ();
	Names = new BYTE[nameLen + 4*NumStrings];
	memcpy (Names, lump + sizeof(Header), nameLen + 4*NumStrings);
}

void FStringTable::FlushNames () const
{
	if (Names != NULL)
	{
		delete[] Names;
		Names = NULL;
	}
}

// Find a string by name
int FStringTable::FindString (const char *name) const
{
	if (Names == NULL)
	{
		LoadNames ();
	}

	const WORD *nameOfs = (WORD *)Names;
	const char *nameBase = (char *)Names + NumStrings*4;

	int min = 0;
	int max = NumStrings-1;

	while (min <= max)
	{
		const int mid = (min + max) / 2;
		const char *const tablename = LESHORT(nameOfs[mid*2]) + nameBase;
		const int lex = stricmp (name, tablename);
		if (lex == 0)
			return nameOfs[mid*2+1];
		else if (lex < 0)
			max = mid - 1;
		else
			min = mid + 1;
	}
	return -1;
}

// Find a string with the same text
int FStringTable::MatchString (const char *string) const
{
	for (int i = 0; i < NumStrings; i++)
	{
		if (strcmp (Strings[i], string) == 0)
			return i;
	}
	return -1;
}

VERSION_CONTROL (stringtable_cpp, "$Id$")
