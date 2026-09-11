// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1998-2006 by Randy Heit (ZDoom).
// Copyright (C) 2006-2026 by The Odamex Team.
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
#include "v_textcolors.h"
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
	// TODO: should this account for rekkr too? uzdoom seems to just have
	// doom, strife, heretic, hexen, and chex here
	if (!stricmp(str.c_str(), "doom") && !IsChexMission(::gamemission) &&
	    ::gamemode != undetermined && ::gamemission != commercial_hacx)
	{
		return true;
	}

	if (!stricmp(str.c_str(), "chex") && IsChexMission(::gamemission))
	{
		return true;
	}

	if (!stricmp(str.c_str(), "hacx") && ::gamemission == commercial_hacx)
	{
		return true;
	}

	// We don't support anything else.
	return false;
}

bool StringTable::canSetPassString(int pass, const std::string& name) const
{
	StringHash::const_iterator it = _stringHash.find(OString(name));

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
				setPassString(pass, OString(name), OString(value));
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
			TableEntry entry = {std::make_pair(false, ""_os), 0xFF, static_cast<int>(i)};
			_stringHash.emplace(name, entry);
		}
	}
}

namespace
{

//
// Rewrite the ZDoom color escape at the given position into the escape
// character form the text drawing code understands.
//
// Handles both the "\cX" letter form and the "\c[Name]" named form.
//
// Returns false and leaves a malformed escape alone.
//
bool replaceColorEscape(std::string& str, size_t index)
{
	// "\c" needs at least one more character to name a color with.
	if (index + 2 >= str.length())
		return false;

	if (str.at(index + 2) != '[')
	{
		// Letter form. Our color letters are the same as ZDoom's, so only
		// the escape character itself has to change.
		const std::string code = {TEXTCOLOR_ESCAPE, str.at(index + 2)};
		str.replace(index, 3, code);
		return true;
	}

	// Named form. We have no custom translations, so resolve the name to
	// the nearest built-in color range.
	const size_t close = str.find(']', index + 3);
	if (close == std::string::npos)
		return false;

	const std::string name = str.substr(index + 3, close - (index + 3));
	str.replace(index, close - index + 1, TextColorFromRange(TextColorFromString(name)));
	return true;
}

//
// True if the string ends with a color escape code, which makes appending
// another one pointless.
//
bool endsWithColorCode(const std::string& str)
{
	return str.length() >= 2 && str.at(str.length() - 2) == TEXTCOLOR_ESCAPE;
}

} // namespace

void StringTable::replaceEscapes(std::string& str)
{
	size_t index = 0;
	bool colored = false;

	for (;;)
	{
		// Find the initial slash.
		index = str.find('\\', index);
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
		case 'c':
			colored |= replaceColorEscape(str, index);
			break;
		}
		index += 1;
	}

	if (colored && !endsWithColorCode(str))
		str += TEXTCOLOR_NORMAL;
}

//
// Dump all strings to the console.
//
// Sometimes a blunt instrument is what is necessary.
//
void StringTable::dumpStrings()
{
	for (const auto& [first, second] : _stringHash)
	{
		PrintFmt(PRINT_HIGH, "{} (pass: {}, index: {}) = {}\n", first,
		         second.pass, second.index, second.string.second);
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
// Obtain a string by name, retrying with the name uppercased.
//
const char* StringTable::lookup(const std::string& name) const
{
	const char* text = operator()(OString(name));
	if (text[0] == '\0')
		text = operator()(OStringToUpper(name));

	return text;
}

//
// Resolve a "$NAME" token into the string it names.
//
std::string StringTable::maybeLookup(const std::string& token) const
{
	if (token.length() < 2 || token[0] != '$')
		return token;

	const char* text = lookup(token.substr(1));
	return text[0] == '\0' ? token : text;
}

//
// Find a string with the same text.
//
const OString& StringTable::matchString(const OString& string) const
{
	for (const auto& [first, second] : _stringHash)
	{
		if (second.string.first == false)
			continue;
		if (second.string.second == string)
			return first;
	}

	static OString empty = ""_os;
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
		_stringHash.emplace(name, entry);
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
		_stringHash.emplace(name, entry);
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
