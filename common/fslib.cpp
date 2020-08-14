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
//	Filesystem functions.
//
//-----------------------------------------------------------------------------
//
// This file contains significant code from the Go programming language.
//
// Copyright 2009 The Go Authors. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the 3RD-PARTY-LICENSES file.
//
//-----------------------------------------------------------------------------

#include "fslib.h"

#include <algorithm>

#include "doomtype.h"

namespace fs
{

#ifdef _WIN32

static bool isSlash(char c)
{
	return c == '\\' || c == '/';
}

// volumeNameLen returns length of the leading volume name on Windows.
// It returns 0 elsewhere.
static size_t volumeNameLen(std::string path)
{
	if (path.size() < 2)
		return 0;

	// with drive letter
	char c = path.at(0);
	if (path.at(1) == ':' && ('a' <= c && c <= 'z' || 'A' <= c && c <= 'Z'))
		return 2;

	// is it UNC?
	// https://msdn.microsoft.com/en-us/library/windows/desktop/aa365247(v=vs.85).aspx
	size_t l = path.length();
	if (l >= 5 && isSlash(path.at(0)) && isSlash(path.at(1)) && !isSlash(path.at(2)) &&
	    path.at(2) != '.')
	{
		// first, leading `\\` and next shouldn't be `\`. its server name.
		for (size_t n = 3; n < l - 1; n++)
		{
			// second, next '\' shouldn't be repeated.
			if (isSlash(path.at(n)))
			{
				n++;

				// third, following something characters. its share name.
				if (!isSlash(path.at(n)))
				{
					if (path.at(n) == '.')
						break;

					for (; n < l; n++)
					{
						if (isSlash(path.at(n)))
							break;
					}
					return n;
				}
				break;
			}
		}
	}
	return 0;
}

#else

// volumeNameLen returns length of the leading volume name on Windows.
// It returns 0 elsewhere.
static int volumeNameLen(std::string path)
{
	return 0;
}

#endif

#ifdef _WIN32

// IsPathSeparator reports whether c is a directory separator character.
bool IsPathSeparator(char c)
{
	// NOTE: Windows accept / as path separator.
	return c == '\\' || c == '/';
}

#else

// IsPathSeparator reports whether c is a directory separator character.
bool IsPathSeparator(char c)
{
	return PATHSEPCHAR == c;
}

#endif

// FromSlash returns the result of replacing each slash ('/') character
// in path with a separator character. Multiple slashes are replaced
// by multiple separators.
std::string FromSlash(std::string path)
{
	if (PATHSEPCHAR == '/')
		return path;

	std::replace(path.begin(), path.end(), '/', PATHSEPCHAR);
	return path;
}

/**
 * @brief Clean returns the shortest path name equivalent to path by purely
 *        lexical processing.
 *
 * @param path Path to clean.
 * @return Cleaned-up path.
 */
std::string Clean(std::string path)
{
	std::string originalPath = path;
	size_t volLen = volumeNameLen(path);
	path = path.substr(volLen, std::string::npos);
	if (path == "")
	{
		if (volLen > 1 && originalPath.at(1) != ':')
		{
			// should be UNC
			return FromSlash(originalPath);
		}
		return originalPath + ".";
	}
	bool rooted = IsPathSeparator(path.at(0));

	// Invariants:
	//	reading from path; r is index of next byte to process.
	//	writing to buf; w is index of next byte to write.
	//	dotdot is index in buf where .. must stop, either because
	//		it is the leading slash or it is a leading ../../.. prefix.
	size_t n = path.length();
	std::string out = "";
	size_t r = 0;
	size_t dotdot = 0;
	if (rooted)
	{
		out.push_back(PATHSEPCHAR);
		r = 1;
		dotdot = 1;
	}

	while (r < n)
	{
		if (IsPathSeparator(path.at(r)))
		{
			// empty path element
			r += 1;
		}
		else if (path.at(r) == '.' && (r + 1 == n || IsPathSeparator(path.at(r + 1))))
		{
			// . element
			r += 1;
		}
		else if (path.at(r) == '.' && path.at(r + 1) == '.' &&
		         (r + 2 == n || IsPathSeparator(path.at(r + 2))))
		{
			// .. element: remove to last separator
			r += 2;
			if (out.length() > dotdot)
			{
				// can backtrack
				size_t w = out.length() - 1;
				while (w > dotdot && !IsPathSeparator(out.at(w)))
					w -= 1;

				out.resize(w);
			}
			else if (!rooted)
			{
				// cannot backtrack, but not rooted, so append .. element.
				if (out.length() > 0)
					out.push_back(PATHSEPCHAR);

				out.append("..");
				dotdot = out.length();
			}
		}
		else
		{
			// real path element.
			// add slash if needed
			if ((rooted && out.length() != 1) || (!rooted && out.length() != 0))
				out.push_back(PATHSEPCHAR);

			// copy element
			for (; r < n && !IsPathSeparator(path.at(r)); r++)
				out.push_back(path.at(r));
		}
	}

	// Turn empty string into "."
	if (out.length() == 0)
		out.push_back('.');

	return FromSlash(out);
}
} // namespace fs
