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


#include "odamex.h"

#include "stringtable.h"


#include "cmdlib.h"
#include "i_system.h"
#include "oscanner.h"
#include "stringenums.h"
#include "w_wad.h"

/**
 * @brief Map a ZDoom game name to Odamex's internals and returns true if
 *        the current game is the passed string.
 * 
 * @param str String to check against.
 * @return True if the game matches the passed string, otherwise false.
 */
static bool IfGameZDoom(const std::string& str)
{
	if (!stricmp(str.c_str(), "doom") && ::gamemode != retail_chex &&
	    ::gamemode != undetermined)
	{
		return true;
	}

	if (!stricmp(str.c_str(), "chex") && ::gamemode == retail_chex)
	{
		return true;
	}

	// We don't support anything else.
	return false;
}

bool StringTable::canSetPassString(int pass, const std::string& name) const
{
	StringHash::const_iterator it = _stringHash.find(name);

	// New string?
	if (it == _stringHash.end())
		return true;

	// Found an entry, does the string exist?
	if ((*it).second.string.first == false)
		return true;

	// Was the string set with a less exact pass?
	if ((*it).second.pass >= pass)
		return true;

	return false;
}

void StringTable::clearStrings()
{
	_stringHash.clear();
}

//
// Loads a language
//
void StringTable::loadLanguage(const char* code, bool exactMatch, int pass, char* lump,
                               size_t lumpLen)
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
			// Code to check against.
			char checkCode[4] = {'\0', '\0', '\0', '\0'};

			if (os.compareToken("]"))
			{
				break;
			}
			else if (os.compareToken("default"))
			{
				// Default has a speical ID.
				strncpy(checkCode, "**", 3);
			}
			else
			{
				// Turn the language into an ID.
				const std::string& lang = os.getToken();

				if (lang.length() == 2 || lang.length() == 3)
				{
					strncpy(checkCode, lang.c_str(), lang.length());
				}
				else
				{
					os.error("Language identifier must be 2 or 3 characters");
				}
			}

			if (exactMatch && strncmp(code, checkCode, 3) == 0)
			{
				shouldParseSection = true;
			}
			else if (!exactMatch && strncmp(code, checkCode, 2) == 0)
			{
				shouldParseSection = true;
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

				// $ifgame() does not appear to be documented in the wiki,
				// but it causes the next string to only be set it the game
				// matches up.
				bool skip = false;
				if (os.compareToken("$"))
				{
					os.scan();
					os.assertTokenIs("ifgame");
					os.scan();
					os.assertTokenIs("(");
					os.scan();
					skip = !IfGameZDoom(os.getToken());
					os.scan();
					os.assertTokenIs(")");
					os.scan();
				}

				// String name
				const std::string& name = os.getToken();

				// If we can find the token, skip past the string
				if (!canSetPassString(pass, name))
				{
					while (os.scan())
					{
						if (os.compareToken(";"))
							break;
					}
					continue;
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

				replaceEscapes(value);
				if (skip)
				{
					continue;
				}
				setPassString(pass, name, value);
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

void StringTable::loadStringsLump(const int lump, const char* lumpname, const bool engOnly)
{
	// Can't use Z_Malloc this early, so we use raw new/delete.
	size_t len = W_LumpLength(lump);
	char* languageLump = new char[len + 1];
	W_ReadLump(lump, languageLump);
	languageLump[len] = '\0';

	// String replacement pass.  Strings in an later pass can be replaced
	// by a string in an earlier pass from another lump.
	int pass = 1;

	if (!engOnly)
	{
		// Load language-specific strings.
		for (size_t i = 0; i < ARRAY_LENGTH(::LanguageIDs); i++)
		{
			// Deconstruct code into something less confusing.
			char code[4];
			UNMAKE_ID(code, ::LanguageIDs[i]);

			// Language codes are up to three letters long.
			code[3] = '\0';

			// Try the full language code (enu).
			loadLanguage(code, true, pass++, languageLump, len);

			// Try the partial language code (en).
			code[2] = '\0';
			loadLanguage(code, true, pass++, languageLump, len);

			// Try an inexact match for all languages in the same family (en_).
			loadLanguage(code, false, pass++, languageLump, len);
		}
	}

	// Load string defaults.
	loadLanguage("**", true, pass++, languageLump, len);

	delete[] languageLump;
}

void StringTable::prepareIndexes()
{
	// All of the default strings have index numbers that represent their
	// position in the now-removed enumeration.  This function simply sets
	// them all up.
	for (size_t i = 0; i < ARRAY_LENGTH(::stringIndexes); i++)
	{
		OString name = *(::stringIndexes[i]);
		StringHash::iterator it = _stringHash.find(name);
		if (it == _stringHash.end())
		{
			TableEntry entry = {std::make_pair(false, ""), 0xFF, i};
			_stringHash.insert(std::make_pair(name, entry));
		}
	}
}

void StringTable::replaceEscapes(std::string& str)
{
	size_t index = 0;

	for (;;)
	{
		// Find the initial slash.
		index = str.find("\\", index);
		if (index == std::string::npos || index == str.length() - 1)
			break;

		// Substitute the escape string.
		switch (str.at(index + 1))
		{
		case 'n':
			str.replace(index, 2, "\n");
			break;
		case '\\':
			str.replace(index, 2, "\\");
			break;
		}
		index += 1;
	}
}

//
// Dump all strings to the console.
//
// Sometimes a blunt instrument is what is necessary.
//
void StringTable::dumpStrings()
{
	StringHash::const_iterator it = _stringHash.begin();
	for (; it != _stringHash.end(); ++it)
	{
		Printf(PRINT_HIGH, "%s (pass: %d, index: %d) = %s\n", (*it).first.c_str(),
		       (*it).second.pass, (*it).second.index, (*it).second.string.second.c_str());
	}
}

//
// See if a string exists in the table.
//
bool StringTable::hasString(const OString& name) const
{
	StringHash::const_iterator it = _stringHash.find(name);
	if (it == _stringHash.end())
		return false;
	if ((*it).second.string.first == false)
		return false;

	return true;
}

//
// Load strings from all LANGUAGE lumps in all loaded WAD files.
//
void StringTable::loadStrings(const bool engOnly)
{
	clearStrings();
	prepareIndexes();

	int lump = -1;

	lump = -1;
	while ((lump = W_FindLump("LANGUAGE", lump)) != -1)
	{
		loadStringsLump(lump, "LANGUAGE", engOnly);
	}
}

//
// Find a string with the same text.
//
const OString& StringTable::matchString(const OString& string) const
{
	for (StringHash::const_iterator it = _stringHash.begin(); it != _stringHash.end();
	     ++it)
	{
		if ((*it).second.string.first == false)
			continue;
		if ((*it).second.string.second == string)
			return (*it).first;
	}

	static OString empty = "";
	return empty;
}

//
// Set a string to something specific by name.
//
// Overrides the existing string, if it exists.
//
void StringTable::setString(const OString& name, const OString& string)
{
	StringHash::iterator it = _stringHash.find(name);
	if (it == _stringHash.end())
	{
		// Stringtable entry does not exist, insert it.
		TableEntry entry = {std::make_pair(true, string), 0, -1};
		_stringHash.insert(std::make_pair(name, entry));
	}
	else
	{
		// Stringtable entry exists, update it.
		(*it).second.string.first = true;
		(*it).second.string.second = string;
	}
}

//
// Set a string to something specific by name.
//
// Does not set the string if it already exists.
//
void StringTable::setPassString(int pass, const OString& name, const OString& string)
{
	StringHash::iterator it = _stringHash.find(name);
	if (it == _stringHash.end())
	{
		// Stringtable entry does not exist.
		TableEntry entry = {std::make_pair(true, string), pass, -1};
		_stringHash.insert(std::make_pair(name, entry));
	}
	else
	{
		// Stringtable entry exists, but has not been set yet.
		(*it).second.string.first = true;
		(*it).second.string.second = string;
		(*it).second.pass = pass;
	}
}

//
// Number of entries in the stringtable.
//
size_t StringTable::size() const
{
	return _stringHash.size();
}

VERSION_CONTROL(stringtable_cpp, "$Id$")
