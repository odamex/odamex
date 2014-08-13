// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: res_filelib.h 3426 2012-11-19 17:25:28Z dr_sean $
//
// Copyright (C) 2006-2014 by The Odamex Team.
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
//
// Resource file locating functions
//
//-----------------------------------------------------------------------------

#ifndef __RES_FILELIB_H__
#define __RES_FILELIB_H__

#include <string>

std::string Res_CleanseFilename(const std::string& filename);

std::string Res_FindResourceFile(const std::string& filename, const std::string& hash = "");

std::string Res_FindResourceFile(
		const std::string& filename,
		const char* const extlist[], size_t extlist_size,
		const std::string& hash = "");

#endif	// __RES_FILELIB_H__
