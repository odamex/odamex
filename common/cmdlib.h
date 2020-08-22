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
//	Command library (?)
//
//-----------------------------------------------------------------------------


#ifndef __CMDLIB__
#define __CMDLIB__

#include <algorithm>
#include <string>
#include <vector>

#ifdef _MSC_VER
#pragma warning(disable : 4244)     // MIPS
#pragma warning(disable : 4136)     // X86
#pragma warning(disable : 4051)     // ALPHA

#pragma warning(disable : 4018)     // signed/unsigned mismatch
#pragma warning(disable : 4305)     // truncate from double to float
#endif

#include "doomtype.h"

#include <stdio.h>
#include <cstring>
#include <stdlib.h>
#include <errno.h>
#include <ctype.h>
#include <stdarg.h>

extern std::string progdir, startdir;

void	FixPathSeparator (std::string &path);

int		ParseHex(const char *str);
int 	ParseNum(const char *str);
bool	IsNum(const char* str);		// [RH] added
bool	IsRealNum(const char* str);

// [Russell] Returns 0 if strings are the same, optional parameter for case
// sensitivity
bool iequals(const std::string &, const std::string &);

size_t  StdStringFind(const std::string& haystack, const std::string& needle,
    size_t pos, size_t n, bool CIS);

size_t  StdStringRFind(const std::string& haystack, const std::string& needle,
    size_t pos, size_t n, bool CIS);

std::string StdStringToLower(const std::string&, size_t n = std::string::npos);
std::string StdStringToLower(const char*, size_t n = std::string::npos);
std::string StdStringToUpper(const std::string&, size_t n = std::string::npos);
std::string StdStringToUpper(const char*, size_t n = std::string::npos);

std::string &TrimString(std::string &s);
std::string &TrimStringStart(std::string &s);
std::string &TrimStringEnd(std::string &s);

bool ValidString(const std::string&);

char	*COM_Parse (char *data);

extern	char	com_token[8192];
extern	BOOL	com_eof;

char	*copystring(const char *s);

void	CRC_Init(unsigned short *crcvalue);
void	CRC_ProcessByte(unsigned short *crcvalue, byte data);
unsigned short CRC_Value(unsigned short crcvalue);

std::vector<std::string> VectorArgs(size_t argc, char **argv);
std::string JoinStrings(const std::vector<std::string> &pieces, const std::string &glue = "");

typedef std::vector<std::string> StringTokens;
StringTokens TokenizeString(const std::string& str, const std::string& delim);

FORMAT_PRINTF(2, 3) void STACK_ARGS StrFormat(std::string& out, const char* fmt, ...);
void STACK_ARGS VStrFormat(std::string& out, const char* fmt, va_list va);

bool StrFormatISOTime(std::string& s, const tm* utc_tm);
bool StrParseISOTime(const std::string& s, tm* utc_tm);
bool StrToTime(std::string str, time_t &tim);

bool CheckWildcards (const char *pattern, const char *text);
void ReplaceString (char** ptr, const char* str);

void StripColorCodes(std::string& str);

uint32_t CRC32(const uint8_t* buf, uint32_t len);
uint32_t Log2(uint32_t n);

#endif
