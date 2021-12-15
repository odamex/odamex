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
//  A handle that wraps a resolved file on disk.
//
//-----------------------------------------------------------------------------

#include "odamex.h"

#include "m_resfile.h"

#include <algorithm>

#include "c_dispatch.h"
#include "cmdlib.h"
#include "m_argv.h"
#include "m_fileio.h"
#include "md5.h"
#include "w_ident.h"
#include "w_wad.h"

EXTERN_CVAR(cl_waddownloaddir)
EXTERN_CVAR(waddirs)

/**
 * @brief Populate an empty OFileHash with just the type.
 *
 * @param out FileHash to populate.
 * @param type Type of hash to populate with.
 */
void OFileHash::makeEmpty(OFileHash& out, const ofilehash_t type)
{
	out.m_type = type;
}

/**
 * @brief Populate an OFileHash with the passed hex string.
 *
 * @param out FileHash to populate.
 * @param hash Hex string to populate with.
 * @return True if the hash was populated, otherwise false.
 */
bool OFileHash::makeFromHexStr(OFileHash& out, const std::string& hash)
{
	if (IsHexString(hash, 8))
	{
		out.m_hash = StdStringToUpper(hash);
		out.m_type = OFHASH_CRC;
		return true;
	}
	else if (IsHexString(hash, 32))
	{
		out.m_hash = StdStringToUpper(hash);
		out.m_type = OFHASH_MD5;
		return true;
	}

	return false;
}

/**
 * @brief Populate an OResFile.
 *
 * @param out OResFile to populate.
 * @param file Complete and working path to file to populate with.
 * @return True if the OResFile was populated successfully.
 */
bool OResFile::make(OResFile& out, const std::string& file)
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

	OFileHash hash = W_MD5(file);
	if (hash.empty())
	{
		return false;
	}

	out.m_fullpath = fullpath;
	out.m_hash = hash;
	out.m_basename = StdStringToUpper(basename);
	return true;
}

/**
 * @brief Populate an OResFile with an already calculated hash.
 *
 * @param out OResFile to populate.
 * @param file Complete and working path to file to populate with.
 * @param hash Correct hash of file to populate with.  This is not checked,
 *             and should only be used if you have already hashed the passed
 *             file.
 * @return True if the OResFile was populated successfully.
 */
bool OResFile::makeWithHash(OResFile& out, const std::string& file, const OFileHash& hash)
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

	if (hash.empty())
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
	out.m_basename = StdStringToUpper(basename);
	return true;
}

/**
 * @brief Populate an OWantFile.
 *
 * @param out OWantFile to populate.
 * @param file Path fragment to file to populate with that may or may not exist.
 * @param type Type of resource we're interested in.
 * @return True if the OWantFile was populated successfully.
 */
bool OWantFile::make(OWantFile& out, const std::string& file, const ofile_t type)
{
	std::string basename = M_ExtractFileName(file);
	if (basename.empty())
	{
		return false;
	}

	std::string extension;
	M_ExtractFileExtension(basename, extension);

	out.m_wantedpath = file;
	out.m_wantedtype = type;
	out.m_basename = StdStringToUpper(basename);
	out.m_extension = std::string(".") + StdStringToUpper(extension);
	return true;
}

/**
 * @brief Populate an OResFile with a suggested hash.
 *
 * @param out OWantFile to populate.
 * @param file Path fragment to file to populate with that may or may not exist.
 * @param type Type of resource we're interested in.
 * @param hash Desired hash to populate with.
 * @return True if the OWantFile was populated successfully.
 */
bool OWantFile::makeWithHash(OWantFile& out, const std::string& file, const ofile_t type,
                             const OFileHash& hash)
{
	std::string basename = M_ExtractFileName(file);
	if (basename.empty())
	{
		return false;
	}

	std::string extension;
	M_ExtractFileExtension(basename, extension);

	out.m_wantedpath = file;
	out.m_wantedtype = type;
	out.m_wantedhash = hash;
	out.m_basename = StdStringToUpper(basename);
	out.m_extension = StdStringToUpper(extension);
	return true;
}

/**
 * @brief Turn passed list of ResFiles to string.
 *
 * @param files Files to stringify.
 */
std::string M_ResFilesToString(const OResFiles& files)
{
	std::vector<std::string> strings;
	strings.reserve(files.size());
	for (OResFiles::const_iterator it = files.begin(); it != files.end(); ++it)
	{
		strings.push_back(it->getBasename());
	}
	return JoinStrings(strings, ", ");
}

/**
 * @brief Return a list of valid extensions for a given file type in order
 *        of priority.
 *
 * @param type Filetype.  Unknown filetypes assumes all valid extensions.
 */
const std::vector<std::string>& M_FileTypeExts(ofile_t type)
{
	static std::vector<std::string> unknown;
	static std::vector<std::string> wad;
	static std::vector<std::string> deh;

	switch (type)
	{
	case OFILE_WAD:
		if (wad.empty())
		{
			wad.push_back(".WAD");
		}
		return wad;
	case OFILE_DEH:
		if (deh.empty())
		{
			deh.push_back(".BEX");
			deh.push_back(".DEH");
		}
		return deh;
	default:
		if (unknown.empty())
		{
			unknown.push_back(".WAD");
			unknown.push_back(".BEX");
			unknown.push_back(".DEH");
		}
		return unknown;
	}
}

/**
 * @brief Construct a list of file search directories from known locations.
 */
std::vector<std::string> M_FileSearchDirs()
{
	std::vector<std::string> dirs;

	// [cSc] Add cl_waddownloaddir as default path
	D_AddSearchDir(dirs, ::cl_waddownloaddir.cstring(), PATHLISTSEPCHAR);
	D_AddSearchDir(dirs, ::Args.CheckValue("-waddir"), PATHLISTSEPCHAR);
	D_AddSearchDir(dirs, getenv("DOOMWADDIR"), PATHLISTSEPCHAR);
	D_AddSearchDir(dirs, getenv("DOOMWADPATH"), PATHLISTSEPCHAR);
	D_AddSearchDir(dirs, ::waddirs.cstring(), PATHLISTSEPCHAR);
	dirs.push_back(M_GetUserDir());
	dirs.push_back(M_GetCWD());
	dirs.push_back(M_GetBinaryDir());

#ifdef __SWITCH__
	dirs.push_back("./wads");
#endif

	// [AM] Search additional paths based on platform
	D_AddPlatformSearchDirs(dirs);

	// Get rid of any dupes.
	dirs.erase(std::unique(dirs.begin(), dirs.end()), dirs.end());

	return dirs;
}

/**
 * @brief Resolve an OResFile given a filename.
 *
 * @param out Output OResFile.  On error, this object is not touched.
 * @param wanted Wanted file to resolve.
 * @return True if the file was resolved, otherwise false.
 */
bool M_ResolveWantedFile(OResFile& out, const OWantFile& wanted)
{
	// If someone goes throught the effort of pointing directly to a file
	// correctly, believe them.
	if (M_FileExists(wanted.getWantedPath()))
	{
		if (wanted.getWantedHash().empty())
		{
			// No hash preference.
			return OResFile::make(out, wanted.getWantedPath());
		}

		OFileHash hash = W_MD5(wanted.getWantedPath());
		if (wanted.getWantedHash() == hash)
		{
			// File matches our hash.
			return OResFile::makeWithHash(out, wanted.getWantedPath(), hash);
		}

		// Not a match, keep trying.
	}

	std::string dir, basename, strext;
	std::vector<std::string> exts;
	std::string path = M_CleanPath(wanted.getWantedPath());
	M_ExtractFilePath(path, dir);
	M_ExtractFileBase(path, basename);
	if (M_ExtractFileExtension(path, strext))
	{
		exts.push_back("." + StdStringToUpper(strext));
	}
	else
	{
		const std::vector<std::string>& ftexts = M_FileTypeExts(wanted.getWantedType());
		exts.insert(exts.end(), ftexts.begin(), ftexts.end());
	}
	std::unique(exts.begin(), exts.end());

	// And now...we resolve.
	const std::vector<std::string> dirs = M_FileSearchDirs();
	for (std::vector<std::string>::const_iterator it = dirs.begin(); it != dirs.end();
	     ++it)
	{
		const std::string result =
		    M_BaseFileSearchDir(*it, basename, exts, wanted.getWantedHash());
		if (!result.empty())
		{
			// Found a file.
			const std::string fullpath = *it + PATHSEP + result;
			return OResFile::make(out, fullpath);
		}
	}

	// Couldn't find anything.
	return false;
}

void M_ResolveIWADs()
{
	const std::vector<OString> iwads = W_GetIWADFilenames();
	for (size_t i = 0; i < iwads.size(); i++)
	{
		OWantFile wantfile;
		if (!OWantFile::make(wantfile, iwads[i], OFILE_WAD))
		{
			return;
		}

		OResFile resfile;
		M_ResolveWantedFile(resfile, wantfile);
	}
}

std::string M_GetCurrentWadHashes()
{
	std::string builder = "";

	for (OResFiles::const_iterator it = ::wadfiles.begin(); it != ::wadfiles.end(); ++it)
	{
		std::string base = it->getBasename().c_str();
		std::string hash = it->getHash().getHexStr().c_str();
		std::string line = base + ',' + hash + '\n';

		builder += line;
	}

	return builder;
}

BEGIN_COMMAND(whereis)
{
	if (argc < 2)
		return;

	OWantFile want;
	OWantFile::make(want, argv[1], OFILE_UNKNOWN);

	OResFile res;
	if (M_ResolveWantedFile(res, want))
	{
		Printf("basename: %s\nfullpath: %s\nhash: %s\n", res.getBasename().c_str(),
		       res.getFullpath().c_str(), res.getHash().getHexStr().c_str());
		return;
	}

	Printf("Could not find location of \"%s\".\n", argv[1]);
}
END_COMMAND(whereis)

BEGIN_COMMAND(loaded)
{
	for (OResFiles::const_iterator it = ::wadfiles.begin(); it != ::wadfiles.end(); ++it)
	{
		Printf("%s\n", it->getBasename().c_str());
		Printf("  PATH: %s\n", it->getFullpath().c_str());
		Printf("  MD5:  %s\n", it->getHash().getHexStr().c_str());
	}

	for (OResFiles::const_iterator it = ::patchfiles.begin(); it != ::patchfiles.end();
	     ++it)
	{
		Printf("%s\n", it->getBasename().c_str());
		Printf("  PATH: %s\n", it->getFullpath().c_str());
		Printf("  MD5:  %s\n", it->getHash().getHexStr().c_str());
	}
}
END_COMMAND(loaded)

BEGIN_COMMAND(searchdirs)
{
	Printf("Search Directories:\n");
	std::vector<std::string> dirs = M_FileSearchDirs();
	for (std::vector<std::string>::const_iterator it = dirs.begin(); it != dirs.end();
	     ++it)
	{
		Printf("  %s\n", it->c_str());
	}
}
END_COMMAND(searchdirs)
