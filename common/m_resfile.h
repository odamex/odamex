// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2021 by Alex Mayfield.
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
//  A handle that wraps a resource file on disk.
//
//-----------------------------------------------------------------------------

#ifndef __M_RESFILE_H__
#define __M_RESFILE_H__

#include <string>
#include <vector>

class OResFile
{
	std::string m_fullpath;
	std::string m_hash;
	std::string m_basename;

  public:
	const std::string& GetFullpath()
	{
		return m_fullpath;
	}
	const std::string& GetHash()
	{
		return m_hash;
	}
	const std::string& GetBasename()
	{
		return m_basename;
	}
	static bool Make(OResFile& out, const std::string& file);
	static bool MakeWithHash(OResFile& out, const std::string& file,
	                         const std::string& hash);
};

bool M_ResolveResFile(OResFile& out, std::string filename, const char* ext);

#endif // __ORESFILE_H__
