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

#include "m_resfile.h"

#include <algorithm>

#include "c_dispatch.h"
#include "m_argv.h"
#include "m_fileio.h"
#include "w_wad.h"

EXTERN_CVAR(cl_waddownloaddir)
EXTERN_CVAR(waddirs)

/**
 * @brief Create a OResFile from either a known-good filename or an empty
 *        string to create a "missing" file.
 *
 * @param filename Correct filename.
 * @return Resolved OResFile.
 */
static OResFile ResFileFactory(const std::string& filename)
{
	if (!M_FileExists(filename))
	{
		// Whoops, missing file.
		return OResFile();
	}

	OResFile file;
	file.hash = W_MD5(filename);
	return file;
}

/**
 * @brief Resolve an OResFile given a filename.
 *
 * @param filename Filename to search for.
 * @param ext Extension to search for if missing.
 */
OResFile M_ResolveResFile(const std::string& filename, const char* ext)
{
	// If someone goes throught the effort of pointing directly to a file
	// correctly, believe them.
	if (M_FileExists(filename))
	{
		return ResFileFactory(filename);
	}

	// [cSc] Add cl_waddownloaddir as default path
	std::vector<std::string> dirs;
	D_AddSearchDir(dirs, ::cl_waddownloaddir.cstring(), PATHLISTSEPCHAR);
	D_AddSearchDir(dirs, ::Args.CheckValue("-waddir"), PATHLISTSEPCHAR);
	D_AddSearchDir(dirs, getenv("DOOMWADDIR"), PATHLISTSEPCHAR);
	D_AddSearchDir(dirs, getenv("DOOMWADPATH"), PATHLISTSEPCHAR);
	D_AddSearchDir(dirs, ::waddirs.cstring(), PATHLISTSEPCHAR);
	dirs.push_back(M_GetUserDir());
	dirs.push_back(M_GetCWD());

	// [AM] Search additional paths based on platform
	D_AddPlatformSearchDirs(dirs);

	// Get rid of any dupes.
	dirs.erase(std::unique(dirs.begin(), dirs.end()), dirs.end());

	OResFile file;
	return file;
}

BEGIN_COMMAND(resolve)
{
	if (argc < 2)
		return;

	OResFile file = M_ResolveResFile(argv[1], "wad");
	Printf("\nbasename: %s\nfullpath: %s\nhash: %s\n", file.basename.c_str(),
	       file.fullpath.c_str(), file.hash.c_str());
}
END_COMMAND(resolve)