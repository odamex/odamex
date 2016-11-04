// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2006-2015 by The Odamex Team.
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
//  Error handling
//
// AUTHORS:
//  Russell Rice (russell at odamex dot net)
//  Michael Wood (mwoodj at knology dot net)
//
//-----------------------------------------------------------------------------

#ifndef __NET_ERROR_H__
#define __NET_ERROR_H__

namespace odalpapi
{

void _ReportError(const char* file, int line, const char* func,
                  const char* fmt, ...);
#if (_MSC_VER < 1400) // __VA_ARGS__ not supported by older ANSI C++
	void NET_ReportError(const char* fmt, ...);
#else
	#define NET_ReportError(...) \
		_ReportError(__FILE__, __LINE__, __FUNCTION__, __VA_ARGS__)
#endif

#define REPERR_NO_ARGS ""

} // namespace

#endif // __NET_ERROR_H__
