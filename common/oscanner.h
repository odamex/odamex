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

#ifndef __OSCANNER_H__
#define __OSCANNER_H__

#include <stddef.h>
#include <string>

struct OScannerConfig
{
	const char* lumpName;
	bool semiComments;
	bool cComments;
};

class OScanner
{
	OScannerConfig _config;
	const char* _scriptStart;
	const char* _scriptEnd;
	const char* _position;
	int _lineNumber;
	std::string _token;
	bool _unScan;

	bool checkPair(char a, char b);
	void skipWhitespace();
	void skipToNextLine();
	void skipPastPair(char a, char b);
	bool munchQuotedString();
	void munchString();
	void pushToken(const char* string, size_t length);
	void pushToken(const std::string& string);

  public:
	OScanner(const OScannerConfig& config)
	    : _config(config), _scriptStart(NULL), _scriptEnd(NULL), _position(NULL),
	      _lineNumber(0), _token(""), _unScan(false){};

	static OScanner openBuffer(const OScannerConfig& config, const char* start,
	                           const char* end);
	bool scan();
	void unScan();
	std::string getToken() const;
	void assertTokenIs(const char* string) const;
	bool compareToken(const char* string) const;
	void error(const char* message);
};

#endif // __OSCANNER_H__
