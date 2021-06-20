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

static const char* SINGLE_CHAR_TOKENS = "$(),;=[]{}";

bool OScanner::checkPair(char a, char b)
{
	return m_position[0] == a && m_position + 1 < m_scriptEnd && m_position[1] == b;
}

void OScanner::skipWhitespace()
{
	while (m_position < m_scriptEnd && m_position[0] <= ' ')
	{
		if (m_position[0] == '\n')
			m_lineNumber += 1;

		m_position += 1;
	}
}

void OScanner::skipToNextLine()
{
	while (m_position < m_scriptEnd && m_position[0] != '\n')
		m_position += 1;

	m_position += 1;
	m_lineNumber += 1;
}

void OScanner::skipPastPair(char a, char b)
{
	while (m_position < m_scriptEnd)
	{
		if (checkPair(a, b))
		{
			m_position += 2;
			return;
		}

		if (m_position[0] == '\n')
			m_lineNumber += 1;

		m_position += 1;
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
	while (m_position < m_scriptEnd)
	{
		// Found an escape character quotation mark in string.
		if (m_position[0] == '\\' && m_position + 1 < m_scriptEnd && m_position[1] == '"')
		{
			m_removeEscapeCharacter = true;
			m_position += 2;
		}

		// Found ending quote.
		if (m_position[0] == '"')
			return true;

		m_position += 1;
	}

	// Ran off the end of the script, this is also a problem.
	return false;
}

void OScanner::munchString()
{
	while (m_position < m_scriptEnd)
	{
		// Munch until whitespace.
		if (m_position[0] <= ' ')
			return;

		// There are some tokens that can end the string without whitespace.
		if (m_position[0] == '"')
			return;

		if (m_config.semiComments && m_position[0] == ';')
			return;

		if (m_config.cComments && checkPair('/', '/'))
			return;

		if (m_config.cComments && checkPair('/', '*'))
			return;

		if (strchr(SINGLE_CHAR_TOKENS, m_position[0]) != NULL)
			return;

		m_position += 1;
	}
}

//
// Set our token to a pointer/length combo.
//
// Note that this string is assumed NOT to be null-terminated.
//
void OScanner::pushToken(const char* string, size_t length)
{
	m_token.assign(string, length);

	if (m_removeEscapeCharacter)
	{
		size_t pos = m_token.find("\\\"", 0);

		while (pos != std::string::npos)
		{
			m_token.replace(pos, 2, "\"");
			pos += 2;

			pos = m_token.find("\\\"", pos);
		}

		m_removeEscapeCharacter = false;
	}
}

void OScanner::pushToken(const std::string& string)
{
	m_token = string;

	if (m_removeEscapeCharacter)
	{
		size_t pos = m_token.find("\\\"", 0);

		while (pos != std::string::npos)
		{
			m_token.replace(pos, 2, "\"");
			pos += 2;

			pos = m_token.find("\\\"", pos);
		}

		m_removeEscapeCharacter = false;
	}
}

//
// Initialize the scanner on a memory buffer.
//
OScanner OScanner::openBuffer(const OScannerConfig& config, const char* start,
                              const char* end)
{
	OScanner os = OScanner(config);
	os.m_scriptStart = start;
	os.m_scriptEnd = end;
	os.m_position = start;
	return os;
}

//
// Scan the text until we either find a token or we reach the end.
//
bool OScanner::scan()
{
	if (m_unScan)
	{
		m_unScan = false;
		return true;
	}

	m_isQuotedString = false;

	while (m_position < m_scriptEnd)
	{
		// What are we looking at?
		if (m_position[0] <= ' ')
		{
			skipWhitespace();
			continue;
		}
		if (m_config.semiComments && m_position[0] == ';')
		{
			skipToNextLine();
			continue;
		}
		else if (m_config.cComments && checkPair('/', '/'))
		{
			skipToNextLine();
			continue;
		}
		else if (m_config.cComments && checkPair('/', '*'))
		{
			skipPastPair('*', '/');
			continue;
		}

		// We found an interesting token.  What is it?
		const char* single = strchr(SINGLE_CHAR_TOKENS, m_position[0]);
		if (single != NULL)
		{
			// Found a single char token.
			pushToken(single, 1);

			m_position += 1;
			return true;
		}
		else if (m_position[0] == '"')
		{
			// Found a quoted string.
			m_isQuotedString = true;

			m_position += 1;
			const char* begin = m_position;
			if (munchQuotedString() == false)
				return false;
			const char* end = m_position;
			pushToken(begin, end - begin);

			m_position += 1;
			return true;
		}

		// Found a bare string.
		const char* begin = m_position;
		munchString();
		const char* end = m_position;
		pushToken(begin, end - begin);
		return true;
	}

	return false;
}

//
// Ensure next token is a string.
//
void OScanner::mustScan()
{
	if (!scan())
	{
		error("Missing string (unexpected end of file).");
	}
}

//
// Ensure next token is an int.
//
void OScanner::mustScanInt()
{
	if (!scan())
	{
		error("Missing integer (unexpected end of file).");
	}

	if (IsNum(m_token.c_str()) == false && m_token != "MAXINT")
	{
		std::string err;
		StrFormat(err, "Expected integer, got \"%s\".", m_token.c_str());
		error(err.c_str());
	}
}

//
// Ensure next token is a float.
//
void OScanner::mustScanFloat()
{
	if (!scan())
	{
		error("Missing float (unexpected end of file).");
	}

	if (IsRealNum(m_token.c_str()) == false)
	{
		std::string err;
		StrFormat(err, "Expected float, got \"%s\".", m_token.c_str());
		error(err.c_str());
	}
}

//
// Ensure next token is a bool.
//
void OScanner::mustScanBool()
{
	if (!scan())
	{
		error("Missing boolean (unexpected end of file).");
	}

	if (!iequals(m_token, "true") && !iequals(m_token, "false"))
	{
		std::string err;
		StrFormat(err, "Expected boolean, got \"%s\".", m_token.c_str());
		error(err.c_str());
	}
}

//
// Rewind to the previous token.
//
// FIXME: Currently this just causes the scanner to return early once - you
//        can't actually access the previous token.
//
void OScanner::unScan()
{
	if (m_unScan == true)
	{
		error("Tried to unScan twice in a row.");
	}

	m_unScan = true;
}

//
// Get the most recent token.
//
std::string OScanner::getToken() const
{
	return m_token;
}

//
// Get token as an int.
//
int OScanner::getTokenInt() const
{
	char* stopper;

	if (m_token == "MAXINT")
	{
		return MAXINT; // INT32_MAX;
	}

	const int num = strtol(m_token.c_str(), &stopper, 0);

	if (*stopper != 0)
	{
		std::string err;
		StrFormat(err, "Bad integer constant \"%s\".", m_token.c_str());
		error(err.c_str());
	}

	return num;
}

//
// Get token as a float.
//
float OScanner::getTokenFloat() const
{
	char* stopper;

	const double num = strtod(m_token.c_str(), &stopper);

	if (*stopper != 0)
	{
		std::string err;
		StrFormat(err, "Bad float constant \"%s\".", m_token.c_str());
		error(err.c_str());
	}

	return static_cast<float>(num);
}

//
// Get token as a bool.
//
bool OScanner::getTokenBool() const
{
	return iequals(m_token, "true");
}

//
// Check if last token read in was a quoted string.
//
bool OScanner::isQuotedString() const
{
	return m_isQuotedString;
}

//
// Assert token is equal to the passed string, or error.
//
void OScanner::assertTokenIs(const char* string) const
{
	if (m_token.compare(string) != 0)
	{
		std::string err;
		StrFormat(err, "Unexpected Token (expected \"%s\" actual \"%s\").", string,
		          m_token.c_str());
		error(err.c_str());
	}
}

//
// Compare the most recent token with the passed string.
//
bool OScanner::compareToken(const char* string) const
{
	return m_token.compare(string) == 0;
}

//
// Print given error message.
//
void OScanner::error(const char* message) const
{
	I_Error("Script Error: %s:%d: %s", m_config.lumpName, m_lineNumber, message);
}

VERSION_CONTROL(sc_oman_cpp, "$Id$")
