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
//	String Abstraction Layer (StringTable)
//
//-----------------------------------------------------------------------------
//

#include <cstring>
#include <stddef.h>

#include "cmdlib.h"
#include "errors.h"
#include "i_system.h"
#include "m_swap.h"
#include "oscanner.h"
#include "stringtable.h"
#include "w_wad.h"

void StringTable::clearStrings()
{
	_stringHash.empty();
	_indexes.empty();
}

void StringTable::loadLanguage(uint32_t code, bool exactMatch, char* lump, size_t lumpLen)
{
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
				if (code == (uint32_t)MAKE_ID('*', '*', 0, 0))
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
					if (code == (uint32_t)MAKE_ID(lang[0], lang[1], '\0', '\0'))
					{
						shouldParseSection = true;
					}
				}
				else if (lang.length() == 3)
				{
					if (code == (uint32_t)MAKE_ID(lang[0], lang[1], lang[2], '\0'))
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

				// If we can find the token, skip past the string.
				StringHash::iterator it = _stringHash.find(ident);
				if (it != _stringHash.end())
				{
					while (os.scan())
					{
						if (os.compareToken(";"))
							break;
					}
					break;
				}

				os.scan();
				os.assertTokenIs("=");

				// Grab the string value.
				std::string value;
				while (os.scan())
				{
					const std::string piece = os.getToken();
					if (piece.compare(";") == 0)
					{
						// Found the end of the string, next batter up.
						break;
					}

					value += piece;
				}

				fprintf(stderr, "%s = %s\n", ident.c_str(), value.c_str());
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
}

void StringTable::loadStringsLump(int lump, const char* lumpname)
{
	// Can't use Z_Malloc this early, so we use raw new/delete.
	size_t len = W_LumpLength(lump);
	char* languageLump = new char[len + 1];
	W_ReadLump(lump, languageLump);
	languageLump[len] = '\0';

	// Load language-specific strings.
	for (size_t i = 0; i < ARRAY_LENGTH(LanguageIDs); i++)
	{
		loadLanguage(LanguageIDs[i], true, languageLump, len);
		loadLanguage(LanguageIDs[i] & MAKE_ID(0xff, 0xff, 0, 0), true, languageLump, len);
		loadLanguage(LanguageIDs[i], false, languageLump, len);
	}

	// Load string defaults.
	loadLanguage(MAKE_ID('*', '*', 0, 0), true, languageLump, len);

	delete[] languageLump;
}

//
// See if a string exists in the table.
//
bool StringTable::hasString(const OString& name) const
{
	StringHash::const_iterator it = _stringHash.find(name);
	if (it == _stringHash.end())
		return false;

	return true;
}

//
// Load strings from all LANGUAGE lumps in all loaded WAD files.
//
void StringTable::loadStrings()
{
	clearStrings();

	int lump = -1;

	lump = -1;
	while ((lump = W_FindLump("LANGUAGE", lump)) != -1)
	{
		loadStringsLump(lump, "LANGUAGE");
	}

	I_FatalError("not done");
}

//
// Find a string with the same text.
//
const OString& StringTable::matchString(const OString& string) const
{
	for (StringHash::const_iterator it = _stringHash.begin(); it != _stringHash.end();
	     ++it)
	{
		if ((*it).second.string == string)
			return (*it).first;
	}

	static OString empty = "";
	return empty;
}

//
// Set a string to something specific by name.
//
void StringTable::setString(const OString& name, const OString& string)
{
	StringHash::iterator it = _stringHash.find(name);
	if (it == _stringHash.end())
		return;

	(*it).second.string = string;
}

VERSION_CONTROL(stringtable_cpp, "$Id$")
