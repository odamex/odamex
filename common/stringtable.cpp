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
#include "cmdlib.h"
#include "m_swap.h"
#include "w_wad.h"
#include "i_system.h"
#include "errors.h"
#include "oscanner.h"

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
	delete [] LumpData;

	StringStatus = NULL;
	Strings = NULL;
	Names = NULL;
	NumStrings = 0;
	LumpData = NULL;
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
					delete[] Strings[i];
				Strings[i] = NULL;
			}
		}
	}
}

void FStringTable::LoadStrings()
{
	int lump = -1;

	lump = -1;
	while ((lump = W_FindLump("LANGUAGE", lump)) != -1)
	{
		LoadStringsLump(lump, "LANGUAGE");
	}
/*




	if (lumpnum < 0)
		return;

	char lumpname[9];
	W_GetLumpName(lumpname, lumpnum);
	lumpname[8] = 0;

	// lump is not long enough for the expected header
	if (W_LumpLength(lumpnum) < 8)
	{
		Printf(PRINT_HIGH, "Warning: unsupported string table %s.\n", lumpname);
		return;
	}

	// [SL] read and store the lump data into LumpData
	delete[] LumpData;
	LumpData = new byte[W_LumpLength(lumpnum)];
	W_ReadLump(lumpnum, LumpData);

	int lumpLen = LELONG(*(uint32_t*)(LumpData + 0));
	int nameCount = LESHORT(*(uint16_t*)(LumpData + 4));
	int nameLen = LESHORT(*(uint16_t*)(LumpData + 6));

	// invalid language lump
	if (W_LumpLength(lumpnum) != (unsigned)lumpLen)
	{
		Printf(PRINT_HIGH, "Warning: unsupported string table %s.\n", lumpname);
		return;
	}

	int languageStart = 8 + nameCount*4 + nameLen;
	languageStart += (4 - languageStart) & 3;

	if (expectedSize >= 0 && nameCount != expectedSize)
	{
		I_FatalError("%s had %d strings.\nThis version of Odamex expects it to have %d.",
			lumpname, nameCount, expectedSize);
	}

	FreeStandardStrings();

	LumpNum = lumpnum;
	NumStrings = nameCount;

	if (Strings == NULL)
	{
		Strings = new char*[nameCount];
		StringStatus = new byte[(nameCount+7)/8];
		memset(StringStatus, 0, (nameCount+7)/8);	// 0 means: from wad (standard)
		memset(Strings, 0, sizeof(char*)*nameCount);
	}

	byte* const start = LumpData + languageStart;
	byte* const end = LumpData + lumpLen;
	int loadedCount, i;

	for (loadedCount = i = 0; i < NumStrings; ++i)
	{
		if (Strings[i] != NULL)
			++loadedCount;
	}

	if (!enuOnly)
	{
		for (i = 0; i < 4 && loadedCount != nameCount; ++i)
		{
			loadedCount += LoadLanguage(LanguageIDs[i], true, start, end);
			loadedCount += LoadLanguage(LanguageIDs[i] & MAKE_ID(0xff,0xff,0,0), true, start, end);
			loadedCount += LoadLanguage(LanguageIDs[i], false, start, end);
		}
	}

	// Fill in any missing strings with the default language (enu)
	if (loadedCount != nameCount)
		loadedCount += LoadLanguage(MAKE_ID('e','n','u',0), true, start, end);

	DoneLoading(start, end);

	if (loadedCount != nameCount)
		I_FatalError("Loaded %d strings (expected %d)", loadedCount, nameCount);*/
	I_FatalError("not done");
}

void FStringTable::LoadStringsLump(int lump, const char* lumpname)
{
	// Can't use Z_Malloc this early, so we use raw new/delete.
	size_t len = W_LumpLength(lump);
	char* languageLump = new char[len + 1];
	W_ReadLump(lump, languageLump);
	languageLump[len] = '\0';

	// Load string defaults.
	LoadLanguage(MAKE_ID('*','*',0,0), true, languageLump, len);

	// Load language-specific strings.
	for (size_t i = 0; i < ARRAY_LENGTH(LanguageIDs); i++)
	{
		LoadLanguage(LanguageIDs[i], true, languageLump, len);
		LoadLanguage(LanguageIDs[i] & MAKE_ID(0xff,0xff,0,0), true, languageLump, len);
		LoadLanguage(LanguageIDs[i], false, languageLump, len);
	}

	delete[] languageLump;
}

void FStringTable::LoadLanguage(
	uint32_t code, bool exactMatch, char* lump, size_t lumpLen
)
{
	fprintf(stderr, "code: %u\n", code);
	fprintf(stderr, "(%02x, %02x, %02x, %02x)\n",
		code & 0xFF, (code & 0xFF00) >> 8, (code & 0xFF0000) >> 16,
		(code & 0xFF000000) >> 24
	);

	// const uint32_t orMask = exactMatch ? 0 : MAKE_ID(0,0,0xff,0);
	// code |= orMask;

	OScannerConfig config = {
		"LANGUAGE", // lumpName
		false,      // semiComments
		true,       // cComments
	};
	OScanner os = OScanner::openBuffer(config, lump, lump + lumpLen);
	while (os.scan())
	{
		// Parse a language section.
		bool shouldParseSection = false;

		os.assertTokenIs("[");
		while (os.scan())
		{
			if (os.compareToken("]"))
			{
				break;
			}
			else if (os.compareToken("default"))
			{
				// Default has a speical ID.
				if (code == (uint32_t)MAKE_ID('*','*',0,0))
				{
					shouldParseSection = true;
				}
			}
			else
			{
				// Turn the language into an ID and compare it with the passed code.
				const std::string& lang = os.getToken();

				if (lang.length() == 2)
				{
					if (code == (uint32_t)MAKE_ID(lang[0],lang[1],'\0','\0'))
					{
						shouldParseSection = true;
					}
				}
				else if (lang.length() == 3)
				{
					if (code == (uint32_t)MAKE_ID(lang[0],lang[1],lang[2],'\0'))
					{
						shouldParseSection = true;
					}
				}
				else
				{
					os.error("Language identifier must be 2 or 3 characters");
				}
			}
		}

		if (shouldParseSection)
		{
			// Parse all of the strings in this section.
			while (os.scan())
			{
				if (os.compareToken("["))
				{
					// We reached the end of the section.
					os.unScan();
					break;
				}

				// String identifier
				const std::string& ident = os.getToken();

				fprintf(stderr, "begin string: %s\n", ident.c_str());

				os.scan();
				os.assertTokenIs("=");

				// Grab the string value.
				std::string value;
				while (os.scan())
				{
					const std::string piece = os.getToken();
					if (piece.compare(";") == 0)
					{
						fprintf(stderr, "end string: %s\n", value.c_str());
						// Found the end of the string, next batter up.
						break;
					}

					fprintf(stderr, "appending: %s\n", piece.c_str());
					value += piece;
				}
			}
		}
		else
		{
			// Skip past all of the strings in this section.
			while (os.scan())
			{
				if (os.compareToken("["))
				{
					// Found another section, parse it.
					os.unScan();
					break;
				}
			}
		}
	}

	I_Error("done with all loops");

	/*const uint32_t orMask = exactMatch ? 0 : MAKE_ID(0,0,0xff,0);
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
	return count;*/
}

void FStringTable::ReloadStrings()
{
	// if (LumpNum >= 0)
	// 	LoadStrings(LumpNum, -1, false);
}

// Like ReloadStrings, but clears all the strings before reloading
void FStringTable::ResetStrings()
{
	// FreeData();
	// if (LumpNum >= 0)
	// 	LoadStrings(LumpNum, -1, false);
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


void FStringTable::LoadNames() const
{
	if (LumpNum >= 0)
	{
		byte* lumpdata = new byte[W_LumpLength(LumpNum)];
		W_ReadLump(LumpNum, lumpdata);

		int nameLen = LESHORT(*(uint16_t*)(lumpdata + 6));

		FlushNames();
		Names = new byte[nameLen + 4*NumStrings];
		memcpy(Names, lumpdata + 8, nameLen + 4*NumStrings);

		delete [] lumpdata;
	}
}

void FStringTable::FlushNames() const
{
	if (Names != NULL)
	{
		delete[] Names;
		Names = NULL;
	}
}

// Find a string by name
int FStringTable::FindString(const char* name) const
{
	if (Names == NULL)
		LoadNames();
	
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
