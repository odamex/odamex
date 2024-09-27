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


#include "odamex.h"


#include "oscanner.h"

#include "cmdlib.h"

#include <algorithm>
#include <stdlib.h>

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
		{
			m_crossed = true;
			m_lineNumber += 1;
		}

		m_position += 1;
	}
}

void OScanner::skipToNextLine()
{
	while (m_position < m_scriptEnd && m_position[0] != '\n')
		m_position += 1;

	m_crossed = true;
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
		{
			m_crossed = true;
			m_lineNumber += 1;
		}

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
	m_crossed = false;

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
// Ensure next token is a string. Optional parameter that ensures the string is equal to
// or under a maximum length. If set to 0, accepts any length.
//
void OScanner::mustScan(size_t max_length)
{
	if (!scan())
	{
		error("Missing string (unexpected end of file).");
	}

	if (max_length)
	{
		if (m_token.length() > max_length)
		{
			error("String \"%s\" is too long. Maximum length is %" PRIuSIZE
			       " characters.", m_token.c_str(), max_length);
		}
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

	std::string str = m_token;
	if (IsNum(str.c_str()) == false && str != "MAXINT")
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

	std::string str = m_token;
	if (IsRealNum(str.c_str()) == false)
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
	std::string str = m_token;
	char* stopper;

	if (str == "MAXINT")
	{
		return MAXINT; // INT32_MAX;
	}

	const int num = strtol(str.c_str(), &stopper, 0);

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
	std::string str = m_token;
	char* stopper;

	const double num = strtod(str.c_str(), &stopper);

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
// Check if the last scan crossed over to a new line.
// It returns a reference so the user can set it to false if need be.
//
bool &OScanner::crossed()
{
	return m_crossed;
}

//
// Check if last token read in was a quoted string.
//
bool OScanner::isQuotedString() const
{
	return m_isQuotedString;
}

//
// Check if the last token read in qualifies as an identifier.
// [A-Za-z_]+[A-Za-z0-9_]*
//
bool OScanner::isIdentifier() const
{
	if (m_token.empty())
		return false;

	for (std::string::const_iterator it = m_token.begin(); it != m_token.end(); ++it)
	{
		const char& ch = *it;
		if (ch == '_')
			continue;

		if (ch >= 'A' && ch <= 'Z')
			continue;

		if (ch >= 'a' && ch <= 'z')
			continue;

		if (it != m_token.begin() && ch >= '0' && ch <= '9')
			continue;

		return false;
	}

	return true;
}

//
// Assert token is equal to the passed string, or error.
//
void OScanner::assertTokenIs(const char* string) const
{
	if (m_token.compare(string) != 0)
	{
		error("Unexpected Token (expected \"%s\" actual \"%s\").", string,
		      m_token.c_str());
	}
}

//
// Assert token is equal to the passed string without regard to case, or error.
//
void OScanner::assertTokenNoCaseIs(const char* string) const
{
	if (!iequals(m_token, string))
	{
		error("Unexpected Token (expected \"%s\" actual \"%s\").", string,
		      m_token.c_str());
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
// Compare the most recent token with the passed string, case-insensitive.
//
bool OScanner::compareTokenNoCase(const char* string) const
{
	return iequals(m_token, string);
}

#define MAX_ERRORTEXT 1024

//
// Print given error message.
//
void STACK_ARGS OScanner::warning(const char* message, ...) const
{
	va_list argptr;
	char errortext[MAX_ERRORTEXT];

	va_start(argptr, message);
	vsnprintf(errortext, 1024, message, argptr);
	Printf(PRINT_WARNING, "Script Warning: %s:%d: %s\n", m_config.lumpName, m_lineNumber,
	       errortext, argptr);
	va_end(argptr);
}

//
// Print given error message.
//
void STACK_ARGS OScanner::error(const char* message, ...) const
{
	va_list argptr;
	char errortext[MAX_ERRORTEXT];

	va_start(argptr, message);
	vsnprintf(errortext, 1024, message, argptr);
	I_Error("Script Error: %s:%d: %s", m_config.lumpName, m_lineNumber, errortext,
	        argptr);
	va_end(argptr);
}

VERSION_CONTROL(sc_oman_cpp, "$Id$")
