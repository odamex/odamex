// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
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
//   A better text lump parser, with no global state.
//
//-----------------------------------------------------------------------------

#include "doomtype.h"

#include "oscanner.h"

#include "cmdlib.h"
#include "version.h"

#include <algorithm>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "i_system.h"


#define SINGLE_CHAR_TOKENS "$();=[]{}"

bool OScanner::checkPair(char a, char b)
{
	return _position[0] == a && _position + 1 < _scriptEnd && _position[1] == b;
}

void OScanner::skipWhitespace()
{
	while (_position < _scriptEnd && _position[0] <= ' ')
	{
		if (_position[0] == '\n')
			_lineNumber += 1;

		_position += 1;
	}
}

void OScanner::skipToNextLine()
{
	while (_position < _scriptEnd && _position[0] != '\n')
		_position += 1;

	_position += 1;
	_lineNumber += 1;
}

void OScanner::skipPastPair(char a, char b)
{
	while (_position < _scriptEnd)
	{
		if (checkPair(a, b))
		{
			_position += 2;
			return;
		}

		if (_position[0] == '\n')
			_lineNumber += 1;

		_position += 1;
	}
}

//
// Find the end of a quoted string.  Returns true if the string was
// successfully munched, or false if the string was not terminated.
//
// Assumes position is currently one past the opening quote.
//
bool OScanner::munchQuotedString()
{
	while (_position < _scriptEnd)
	{
		// Found an escape character quotation mark in string.
		if (_position[0] == '\\' && _position + 1 < _scriptEnd && _position[1] == '"')
		{
			_removeEscapeCharacter = true;
			_position += 2;
		}
		
		// Found ending quote.
		if (_position[0] == '"')
			return true;

		// Ran off the end of the line, this is a problem.
		if (_position[0] == '\n')
			return false;

		_position += 1;
	}

	// Ran off the end of the script, this is also a problem.
	return false;
}

void OScanner::munchString()
{
	while (_position < _scriptEnd)
	{
		// Munch until whitespace.
		if (_position[0] <= ' ')
			return;

		// There are some tokens that can end the string without whitespace.
		if (_position[0] == '"')
			return;
		
		if (_config.semiComments && _position[0] == ';')
			return;

		if (_config.cComments && checkPair('/', '/'))
			return;

		if (_config.cComments && checkPair('/', '*'))
			return;

		if (strchr(SINGLE_CHAR_TOKENS, _position[0]) != NULL)
			return;

		_position += 1;
	}
}

//
// Set our token to a pointer/length combo.
//
// Note that this string is assumed NOT to be null-terminated.
//
void OScanner::pushToken(const char* string, size_t length)
{
	_token.assign(string, length);

	if (_removeEscapeCharacter)
	{
		size_t pos = _token.find("\\\"", 0);
		
		while (pos != std::string::npos)
		{
			_token.replace(pos, 2, "\"");
			pos += 2;
			
			pos = _token.find("\\\"", pos);
		}

		_removeEscapeCharacter = false;
	}
}

void OScanner::pushToken(const std::string& string)
{
	_token = string;

	if (_removeEscapeCharacter)
	{
		size_t pos = _token.find("\\\"", 0);

		while (pos != std::string::npos)
		{
			_token.replace(pos, 2, "\"");
			pos += 2;

			pos = _token.find("\\\"", pos);
		}

		_removeEscapeCharacter = false;
	}
}

//
// Initialize the scanner on a memory buffer.
//
OScanner OScanner::openBuffer(const OScannerConfig& config, const char* start,
                              const char* end)
{
	OScanner os = OScanner(config);
	os._scriptStart = start;
	os._scriptEnd = end;
	os._position = start;
	return os;
}

//
// Scan the text until we either find a token or we reach the end.
//
bool OScanner::scan()
{
	if (_unScan)
	{
		_unScan = false;
		return true;
	}

	_isQuotedString = false;

	while (_position < _scriptEnd)
	{
		// What are we looking at?
		if (_position[0] <= ' ')
		{
			skipWhitespace();
			continue;
		}
		if (_config.semiComments && _position[0] == ';')
		{
			skipToNextLine();
			continue;
		}
		else if (_config.cComments && checkPair('/', '/'))
		{
			skipToNextLine();
			continue;
		}
		else if (_config.cComments && checkPair('/', '*'))
		{
			skipPastPair('*', '/');
			continue;
		}

		// We found an interesting token.  What is it?
		const char* single = strchr(SINGLE_CHAR_TOKENS, _position[0]);
		if (single != NULL)
		{
			// Found a single char token.
			pushToken(single, 1);

			_position += 1;
			return true;
		}
		else if (_position[0] == '"')
		{
			// Found a quoted string.
			_isQuotedString = true;
			
			_position += 1;
			const char* begin = _position;
			if (munchQuotedString() == false)
				return false;
			const char* end = _position;
			pushToken(begin, end - begin);

			_position += 1;
			return true;
		}

		// Found a bare string.
		const char* begin = _position;
		munchString();
		const char* end = _position;
		pushToken(begin, end - begin);
		return true;
	}

	return false;
}

//
// Rewind to the previous token.
//
// FIXME: Currently this just causes the scanner to return early once - you
//        can't actually access the previous token.
//
void OScanner::unScan()
{
	if (_unScan == true)
	{
		I_Error("Script Error: %d:%s: Tried to unScan twice in a row",
			_lineNumber, _config.lumpName);
	}

	_unScan = true;
}

//
// Get the most recent token.
//
std::string OScanner::getToken() const
{
	return _token;
}

//
// Assert token is equal to the passed string, or error.
//
void OScanner::assertTokenIs(const char* string) const
{
	if (_token.compare(string) != 0)
	{
		I_Error("Script Error: %d:%s: Unexpected Token (expected '%s' actual '%s')",
		        _lineNumber, _config.lumpName, string, _token.c_str());
	}
}

//
// Compare the most recent token with the passed string.
//
bool OScanner::compareToken(const char* string) const
{
	return _token.compare(string) == 0;
}

//
// Print given error message.
//
void OScanner::error(const char* message)
{
	I_Error("%s", message);
}

//
// Check if last token read in was a quoted string.
//
bool OScanner::isQuotedString() const
{
	return _isQuotedString;
}

//
// Get token as an int.
//
int OScanner::getTokenAsInt() const
{
	// fix for parser reading in commas
	std::string str = _token;

	// remove comma if necessary
	if (str[str.length() - 1] == ',')
	{
		str[str.length() - 1] = '\0';
	}

	char* stopper;

	if (str == "MAXINT")
	{
		return MAXINT;//INT32_MAX;
	}

	const int num = strtol(str.c_str(), &stopper, 0);

	if (*stopper != 0)
	{
		I_Error("Bad numeric constant \"%s\".", str.c_str());
	}

	return num;
}

//
// Get token as a float.
//
float OScanner::getTokenAsFloat() const
{
	// fix for parser reading in commas
	std::string str = _token;

	// remove comma if necessary
	if (str[str.length() - 1] == ',')
	{
		str[str.length() - 1] = '\0';
	}

	char* stopper;

	const double num = strtod(str.c_str(), &stopper);

	if (*stopper != 0)
	{
		I_Error("Bad numeric constant \"%s\".", str.c_str());
	}

	return static_cast<float>(num);
}

//
// Get token as a bool.
//
bool OScanner::getTokenAsBool() const
{
	return stricmp(_token.c_str(), "true") == 0;
}

//
// Ensure next token is a string.
//
void OScanner::mustGetString()
{
	if (!scan())
	{
		error("Missing string (unexpected end of file).");
	}
}

//
// Ensure next token is an int.
//
void OScanner::mustGetInt()
{
	if (!scan())
	{
		error("Missing integer (unexpected end of file).");
	}

	// fix for parser reading in commas
	std::string str = _token;

	// remove comma if necessary
	if (str[str.length() - 1] == ',')
	{
		str[str.length() - 1] = '\0';
	}

	if (IsNum(str.c_str()) == false || str != "MAXINT")
	{
		error("Missing integer (unexpected end of file).");
	}
}

//
// Ensure next token is a float.
//
void OScanner::mustGetFloat()
{
	if (!scan())
	{
		error("Missing floating-point number (unexpected end of file).");
	}

	// fix for parser reading in commas
	std::string str = _token;

	// remove comma if necessary
	if (str[str.length() - 1] == ',')
	{
		str[str.length() - 1] = '\0';
	}

	if (IsRealNum(str.c_str()) == false)
	{
		error("Missing floating-point number (unexpected end of file).");
	}
}

//
// Ensure next token is a bool.
//
void OScanner::mustGetBool()
{
	if (!scan())
	{
		error("Missing boolean (unexpected end of file).");
	}
	
	if (stricmp(_token.c_str(), "true") != 0 && stricmp(_token.c_str(), "false") != 0)
	{
		error("Missing boolean (unexpected end of file).");
	}
}

VERSION_CONTROL(sc_oman_cpp, "$Id$")
