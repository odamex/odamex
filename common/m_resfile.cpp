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

	OMD5Hash hash = W_MD5(file);
	if (hash.empty())
	{
		return false;
	}

	out.m_fullpath = fullpath;
	out.m_md5 = hash;
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
bool OResFile::makeWithHash(OResFile& out, const std::string& file, const OMD5Hash& hash)
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
	out.m_md5 = hash;
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
                             const OMD5Hash& hash)
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
	out.m_wantedMD5 = hash;
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
		if (wanted.getWantedMD5().empty())
		{
			// No hash preference.
			return OResFile::make(out, wanted.getWantedPath());
		}

		OMD5Hash hash = W_MD5(wanted.getWantedPath());
		if (wanted.getWantedMD5() == hash)
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
		    M_BaseFileSearchDir(*it, basename, exts, wanted.getWantedMD5());
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

static bool ScanIWADCmp(const scannedIWAD_t& a, const scannedIWAD_t& b)
{
	return a.id->weight < b.id->weight;
}

static bool ScanPWADCmp(const scannedPWAD_t& a, const scannedPWAD_t& b)
{
	return a.filename < b.filename;
}

/**
 * @brief Scan all file search directories for IWAD files.
 */
std::vector<scannedIWAD_t> M_ScanIWADs()
{
	const std::vector<OString> iwads = W_GetIWADFilenames();
	const std::vector<std::string> dirs = M_FileSearchDirs();

	std::vector<scannedIWAD_t> rvo;
	OHashTable<OCRC32Sum, bool> found;

	for (size_t i = 0; i < dirs.size(); i++)
	{
		std::vector<std::string> files = M_BaseFilesScanDir(dirs[i], iwads);
		for (size_t j = 0; j < files.size(); j++)
		{
			const std::string fullpath = dirs[i] + PATHSEP + files[j];

			// Check to see if we got a real IWAD.
			const OCRC32Sum crc32 = W_CRC32(fullpath);
			if (crc32.empty())
				continue;

			// Found a dupe?
			if (found.find(crc32) != found.end())
				continue;

			// Does the gameinfo exist?
			const fileIdentifier_t* id = W_GameInfo(crc32);
			if (id == NULL)
				continue;

			scannedIWAD_t iwad = {fullpath, id};
			rvo.push_back(iwad);
			found[crc32] = true;
		}
	}

	// Sort the results by weight.
	std::sort(rvo.begin(), rvo.end(), ScanIWADCmp);

	return rvo;
}

/**
 * @brief Scan all file search directories for PWAD files.
 */
std::vector<scannedPWAD_t> M_ScanPWADs()
{
	const std::vector<std::string> dirs = M_FileSearchDirs();

	// possibly change this
	std::vector<scannedPWAD_t> rvo;
	OHashTable<std::string, bool> found;

	for (size_t i = 0; i < dirs.size(); i++)
	{
		const std::string& dir = dirs[i];

		std::vector<std::string> files = M_PWADFilesScanDir(dir);
		for (size_t j = 0; j < files.size(); j++)
		{
			const std::string filename = files[j];

			// [AM] Don't include odamex.wad or IWADs.
			if (iequals(filename, "odamex.wad"))
				continue;

			OWantFile file;
			OWantFile::make(file, filename, OFILE_WAD);
			if (W_IsKnownIWAD(file))
				continue;

			// Found a dupe?
			if (found.find(file.getBasename()) != found.end())
				continue;

			// Insert our file into the found set.
			const std::string fullpath = dir + PATHSEP + filename;

			scannedPWAD_t pwad = {fullpath, filename};
			rvo.push_back(pwad);
			found[file.getBasename()] = true;
		}
	}

	// Sort the results alphabetically
	std::sort(rvo.begin(), rvo.end(), ScanPWADCmp);

	return rvo;
}

std::string M_GetCurrentWadHashes()
{
	std::string builder = "";

	for (OResFiles::const_iterator it = ::wadfiles.begin(); it != ::wadfiles.end(); ++it)
	{
		std::string base = it->getBasename().c_str();
		std::string hash = it->getMD5().getHexCStr();
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
		Printf("basename: %s\nfullpath: %s\nCRC32: %s\nMD5: %s\n",
		       res.getBasename().c_str(), res.getFullpath().c_str(),
		       W_CRC32(res.getFullpath()).getHexCStr(), res.getMD5().getHexCStr());
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
		Printf("  MD5:  %s\n", it->getMD5().getHexCStr());
	}

	for (OResFiles::const_iterator it = ::patchfiles.begin(); it != ::patchfiles.end();
	     ++it)
	{
		Printf("%s\n", it->getBasename().c_str());
		Printf("  PATH: %s\n", it->getFullpath().c_str());
		Printf("  MD5:  %s\n", it->getMD5().getHexCStr());
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
