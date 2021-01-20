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
 * @brief Populate an OResFile.
 *
 * @param out OResFile to populate.
 * @param file File to populate.
 * @return True if the OResFile was populated successfully.
 */
bool OResFile::Make(OResFile& out, const std::string& file)
{
	if (!M_FileExists(file))
	{
		return false;
	}

	std::string fullpath;
	if (!M_GetAbsPath(file, fullpath))
	{
		return false;
	}

	std::string basename = M_ExtractFileName(fullpath);
	if (basename.empty())
	{
		return false;
	}

	std::string hash = W_MD5(file);
	if (hash.empty())
	{
		return false;
	}

	out.m_fullpath = fullpath;
	out.m_hash = hash;
	out.m_basename = basename;
	return true;
}

/**
 * @brief Populate an OResFile with an already calculated hash.
 *
 * @param out OResFile to populate.
 * @param file File to populate.
 * @param hash Hash to populate with.
 * @return True if the OResFile was populated successfully.
 */
bool OResFile::MakeWithHash(OResFile& out, const std::string& file,
                            const std::string& hash)
{
	if (!M_FileExists(file))
	{
		return false;
	}

	std::string fullpath;
	if (!M_GetAbsPath(file, fullpath))
	{
		return false;
	}

	std::string basename = M_ExtractFileName(fullpath);
	if (basename.empty())
	{
		return false;
	}

	out.m_fullpath = fullpath;
	out.m_hash = hash;
	out.m_basename = basename;
	return true;
}

/**
 * @brief Resolve an OResFile given a filename.
 *
 * @param path Path to search for.
 * @param ext Extension to search for if missing, including the dot.
 */
bool M_ResolveResFile(OResFile& out, std::string filename, const char* ext)
{
	// If someone goes throught the effort of pointing directly to a file
	// correctly, believe them.
	if (M_FileExists(filename))
	{
		return OResFile::Make(out, filename);
	}

	std::string path, basename, strext;
	filename = M_CleanPath(filename);
	M_ExtractFilePath(filename, path);
	M_ExtractFileBase(filename, basename);
	if (!M_ExtractFileExtension(filename, strext))
	{
		strext = ext;
	}
	else
	{
		strext.insert(strext.begin(), '.');
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

	// And now...we resolve.
	for (std::vector<std::string>::const_iterator it = dirs.begin(); it != dirs.end();
	     ++it)
	{
		const std::string result = M_BaseFileSearchDir(*it, basename, strext);
		if (!result.empty())
		{
			// Found a file.
			const std::string fullpath = *it + PATHSEP + result;
			return OResFile::Make(out, fullpath);
		}
	}

	// Couldn't find anything.
	return false;
}

BEGIN_COMMAND(whereis)
{
	if (argc < 2)
		return;

	OResFile file;
	if (M_ResolveResFile(file, argv[1], ".wad"))
	{
		Printf("basename: %s\nfullpath: %s\nhash: %s\n", file.GetBasename().c_str(),
		       file.GetFullpath().c_str(), file.GetHash().c_str());
		return;
	}

	Printf("Could not find location of \"%s\".\n", argv[1]);
}
END_COMMAND(whereis)
