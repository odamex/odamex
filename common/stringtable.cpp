// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1998-2006 by Randy Heit (ZDoom).
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
//	String Abstraction Layer (FStringTable)
//
//-----------------------------------------------------------------------------
//

#include <cstring>
#include <stddef.h>

#include "stringtable.h"
#include "doomdef.h"
#include "cmdlib.h"
#include "m_swap.h"
#include "i_system.h"
#include "errors.h"

struct FStringTable::Header
{
	uint32_t FileSize;
	uint16_t NameCount;
	uint16_t NameLen;
};

void FStringTable::FreeData()
{
	if (Strings != NULL)
		FreeStrings();

	delete [] StringStatus;
	delete [] Strings;
	delete [] Names;

	StringStatus = NULL;
	Strings = NULL;
	Names = NULL;
	NumStrings = 0;
}

void FStringTable::FreeStrings()
{
	for (int i = 0; i < NumStrings; ++i)
	{
		if (Strings[i] < CompactBase || Strings[i] >= CompactBase + CompactSize)
			delete[] Strings[i];
	}
	if (CompactBase)
	{
		delete[] CompactBase;
		CompactBase = NULL;
		CompactSize = 0;
	}
}

void FStringTable::FreeStandardStrings()
{
	if (Strings != NULL)
	{
		for (int i = 0; i < NumStrings; ++i)
		{
			if ((StringStatus[i/8] & (1<<(i&7))) == 0)
			{
				if (Strings[i] < CompactBase || Strings[i] >= CompactBase + CompactSize)
					delete [] Strings[i];
				Strings[i] = NULL;
			}
		}
	}
}

void FStringTable::LoadStrings(byte* data, size_t length, int expectedSize, bool enuOnly)
{
	const char lumpname[] = "LANGUAGE";

	// data is not long enough for the expected header
	if (length < 8)
	{
		Printf(PRINT_HIGH, "Warning: unsupported string table %s.\n", lumpname);
		return;
	}

	// or the given length doesn't match the lump's header
	uint32_t header_length = LELONG(*(uint32_t*)(data + 0));
	if (length != header_length) 
	{
		Printf(PRINT_HIGH, "Warning: unsupported string table %s.\n", lumpname);
		return;
	}

	int name_count = LESHORT(*(uint16_t*)(data + 4));
	int name_length = LESHORT(*(uint16_t*)(data + 6));

	int languageStart = 8 + name_count*4 + name_length;
	languageStart += (4 - languageStart) & 3;

	if (expectedSize >= 0 && name_count != expectedSize)
	{
		I_FatalError("%s had %d strings.\nThis version of Odamex expects it to have %d.",
			lumpname, name_count, expectedSize);
	}

	FreeStandardStrings();

	NumStrings = name_count;

	if (Strings == NULL)
	{
		Strings = new char*[name_count];
		StringStatus = new byte[(name_count+7)/8];
		memset(StringStatus, 0, (name_count+7)/8);	// 0 means: from wad (standard)
		memset(Strings, 0, sizeof(char*)*name_count);
	}

	byte* start = data + languageStart;
	byte* end = data + length;

	int loaded_count = 0;

	for (int i = 0; i < NumStrings; ++i)
	{
		if (Strings[i] != NULL)
			++loaded_count;
	}

	if (!enuOnly)
	{
		for (int i = 0; i < 4 && loaded_count != name_count; ++i)
		{
			loaded_count += LoadLanguage(LanguageIDs[i], true, start, end);
			loaded_count += LoadLanguage(LanguageIDs[i] & MAKE_ID(0xff,0xff,0,0), true, start, end);
			loaded_count += LoadLanguage(LanguageIDs[i], false, start, end);
		}
	}

	// Fill in any missing strings with the default language (enu)
	if (loaded_count != name_count)
		loaded_count += LoadLanguage(MAKE_ID('e','n','u',0), true, start, end);

	DoneLoading(start, end);

	if (loaded_count != name_count)
		I_FatalError("Loaded %d strings (expected %d)", loaded_count, name_count);

	delete[] Names;
	Names = NULL;

	Names = new byte[name_length + 4 * NumStrings];
	memcpy(Names, data + 8, name_length + 4 * NumStrings);
}

int FStringTable::LoadLanguage(uint32_t code, bool exactMatch, byte* start, byte* end)
{
	const uint32_t orMask = exactMatch ? 0 : MAKE_ID(0,0,0xff,0);
	int count = 0;

	code |= orMask;

	while (start < end)
	{
		const uint32_t langLen = LELONG(*(uint32_t*)&start[4]);

		if (((*(uint32_t*)start) | orMask) == code)
		{
			start[3] = 1;

			const byte* probe = start + 8;

			while (probe < start + langLen)
			{
				int index = probe[0] | (probe[1]<<8);

				if (Strings[index] == NULL)
				{
					Strings[index] = copystring((const char*)(probe + 2));
					++count;
				}
				probe += 3 + strlen((const char*)(probe + 2));
			}
		}

		start += langLen + 8;
		start += (4 - (ptrdiff_t)start) & 3;
	}
	return count;
}

void FStringTable::DoneLoading(byte* start, byte* end)
{
	while (start < end)
	{
		start[3] = 0;
		start += LELONG(*(uint32_t*)&start[4]) + 8;
		start += (4 - (ptrdiff_t)start) & 3;
	}
}

void FStringTable::SetString(int index, const char* newString)
{
	if (index >= 0 && index < NumStrings)
	{
		if (Strings[index] < CompactBase || Strings[index] >= CompactBase + CompactSize)
			delete[] Strings[index];

		Strings[index] = copystring(newString);
		StringStatus[index/8] |= 1<<(index&7);
	}
}

// Compact all strings into a single block of memory
void FStringTable::Compact()
{
	if (NumStrings == 0)
		return;

	int len = SumStringSizes();
	char* newspace = new char[len];
	char* pos = newspace;
	int i;

	for (i = 0; i < NumStrings; ++i)
	{
		strcpy(pos, Strings[i]);
		pos += strlen(pos) + 1;
	}

	FreeStrings();

	pos = newspace;
	for (i = 0; i < NumStrings; ++i)
	{
		Strings[i] = pos;
		pos += strlen(pos) + 1;
	}

	CompactBase = newspace;
	CompactSize = len;
}

int FStringTable::SumStringSizes() const
{
	int len;
	int i;

	for (i = len = 0; i < NumStrings; ++i)
	{
		len += strlen(Strings[i]) + 1;
	}
	return len;
}


// Find a string by name
int FStringTable::FindString(const char* name) const
{
	if (NumStrings == 0)
		return -1;

	const uint16_t* nameOfs = (uint16_t*)Names;
	const char* nameBase = (char*)Names + NumStrings*4;

	int min = 0;
	int max = NumStrings-1;

	while (min <= max)
	{
		const int mid = (min + max) / 2;
		const char* const tablename = LESHORT(nameOfs[mid*2]) + nameBase;
		const int lex = stricmp(name, tablename);
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
int FStringTable::MatchString(const char* string) const
{
	for (int i = 0; i < NumStrings; i++)
	{
		if (strcmp(Strings[i], string) == 0)
			return i;
	}
	return -1;
}

VERSION_CONTROL (stringtable_cpp, "$Id$")
