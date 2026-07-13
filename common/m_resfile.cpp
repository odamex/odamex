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
//
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
#include "resources/res_filelib.h"
#include "resources/res_main.h"

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

	OMD5Hash hash = Res_MD5(file);
	if (hash.empty())
	{
		return false;
	}

	out.m_fullpath = fullpath;
	out.m_md5 = hash;
	out.m_contentDigest = Res_ContentDigest(fullpath);
	out.m_basename = basename;
	return true;
}

/**
 * @brief Populate an OResFile from a directory used as a resource container.
 *
 * @detail Directories cannot be hashed the way flat files can, so a
 *         deterministic marker hash derived from the directory name is used.
 *         Clients that want this resource will resolve a ZIP/PK3 archive
 *         with the same base name instead.
 *
 * @param out OResFile to populate.
 * @param dir Complete and working path to a directory.
 * @return True if the OResFile was populated successfully.
 */
bool OResFile::makeDirectory(OResFile& out, const std::string& dir)
{
	if (!M_IsDirectory(dir))
	{
		return false;
	}

	std::string fullpath;
	if (!M_GetAbsPath(dir, fullpath))
	{
		return false;
	}

	// Trim any trailing path separator so the basename resolves properly.
	while (!fullpath.empty() && M_IsPathSep(fullpath.back()))
		fullpath.pop_back();

	std::string basename = M_ExtractFileName(fullpath);
	if (basename.empty())
	{
		return false;
	}

	out.m_fullpath = fullpath;
	out.m_md5 = M_DirectoryMarkerHash(basename);
	out.m_contentDigest = Res_ContentDigest(fullpath);
	out.m_basename = basename;
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
	out.m_contentDigest = Res_ContentDigest(fullpath);
	out.m_basename = basename;
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
	out.m_basename = basename;
	out.m_extension = extension;
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
	out.m_basename = basename;
	out.m_extension = extension;
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
	for (const auto& file : files)
	{
		strings.push_back(file.getBasename());
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
	Res_AddSearchDir(dirs, ::cl_waddownloaddir.cstring(), SEARCHPATHSEPCHAR);
	Res_AddSearchDir(dirs, ::Args.CheckValue("-waddir"), SEARCHPATHSEPCHAR);
	Res_AddSearchDir(dirs, getenv("DOOMWADDIR"), SEARCHPATHSEPCHAR);
	Res_AddSearchDir(dirs, getenv("DOOMWADPATH"), SEARCHPATHSEPCHAR);
	Res_AddSearchDir(dirs, ::waddirs.cstring(), SEARCHPATHSEPCHAR);
	dirs.push_back(M_CleanPath(M_GetUserDir() + PATHSEP "downloads"));
	dirs.push_back(M_CleanPath(M_GetBinaryDir() + PATHSEP "downloads"));
	dirs.push_back(M_GetUserDir());
	dirs.push_back(M_GetCWD());
	dirs.push_back(M_GetBinaryDir());

#ifdef __SWITCH__
	dirs.push_back("./wads");
#endif

	// [AM] Search additional paths based on platform
	Res_AddPlatformSearchDirs(dirs);

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
static bool IsArchiveExt(const std::string& ext)
{
	return iequals(ext, ".ZIP") || iequals(ext, ".PK3");
}

/**
 * @brief Compute the deterministic marker hash used to identify a directory
 *        resource of the given base name.
 *
 * @detail Directories cannot be content-hashed the way flat files can, so
 *         they are identified by an MD5 of "DIR:" plus the uppercased
 *         directory name. Both sides of a connection derive the same marker
 *         for a directory of the same name.
 */
OMD5Hash M_DirectoryMarkerHash(const std::string& basename)
{
	const std::string marker = "DIR:" + StdStringToUpper(basename);
	md5_state_t state;
	md5_init(&state);
	md5_append(&state, reinterpret_cast<const md5_byte_t*>(marker.data()), marker.size());
	md5_byte_t digest[16];
	md5_finish(&state, digest);

	std::string hashStr;
	for (int i = 0; i < 16; i++)
		hashStr += fmt::sprintf("%02X", digest[i]);

	OMD5Hash hash;
	OMD5Hash::makeFromHexStr(hash, hashStr);
	return hash;
}

/**
 * @brief Check whether the given hash is the directory resource marker for
 *        the given base name, meaning the want refers to a directory (or its
 *        interchangeable archive form) rather than a concrete file.
 */
bool M_IsDirectoryMarkerHash(const OMD5Hash& hash, const std::string& basename)
{
	return !hash.empty() && hash == M_DirectoryMarkerHash(basename);
}

bool M_ResolveWantedFile(OResFile& out, const OWantFile& wanted)
{
	const OMD5Hash& want_digest = wanted.getWantedContentDigest();

	// A directory can be loaded as a resource container directly.  If the
	// want carries a content digest, the directory's contents must match it.
	if (M_IsDirectory(wanted.getWantedPath()))
	{
		if (want_digest.empty() || Res_ContentDigest(wanted.getWantedPath()) == want_digest)
		{
			return OResFile::makeDirectory(out, wanted.getWantedPath());
		}

		// Wrong contents, keep trying.
	}

	// If someone goes throught the effort of pointing directly to a file
	// correctly, believe them.
	if (M_FileExists(wanted.getWantedPath()))
	{
		if (wanted.getWantedMD5().empty() && want_digest.empty())
		{
			// No hash preference.
			return OResFile::make(out, wanted.getWantedPath());
		}

		OMD5Hash hash = Res_MD5(wanted.getWantedPath());
		if (wanted.getWantedMD5() == hash)
		{
			// File matches our hash.
			return OResFile::makeWithHash(out, wanted.getWantedPath(), hash);
		}

		if (!want_digest.empty() &&
		    Res_ContentDigest(wanted.getWantedPath()) == want_digest)
		{
			// Not the same bytes, but the same logical contents.
			return OResFile::make(out, wanted.getWantedPath());
		}

		// Not a match, keep trying.
	}

	std::string subdir, basename, strext;
	std::vector<std::string> exts;
	std::string path = M_CleanPath(wanted.getWantedPath());
	M_ExtractFilePath(path, subdir);
	M_ExtractFileBase(path, basename);
	if (M_ExtractFileExtension(path, strext))
	{
		if (!strext.empty() && strext[0] != '.')
			strext = "." + strext;
		exts.push_back(StdStringToUpper(strext));

		// Archive resources are interchangeable between the ZIP and
		// PK3 extensions, so search for both spellings.
		if (IsArchiveExt(strext))
		{
			exts.push_back(".ZIP");
			exts.push_back(".PK3");
		}
	}
	else if (M_IsDirectoryMarkerHash(wanted.getWantedMD5(), basename))
	{
		// The want refers to a directory resource on the server.
		// Directories cannot be downloaded or hash-matched, so only look
		// for the archive forms of the resource - never bare WAD files.
		exts.push_back(".ZIP");
		exts.push_back(".PK3");
	}
	else
	{
		const std::vector<std::string>& ftexts = M_FileTypeExts(wanted.getWantedType());
		exts.insert(exts.end(), ftexts.begin(), ftexts.end());

		// Extension-less WAD wants may also resolve to the archive
		// forms of the resource.
		if (wanted.getWantedType() == OFILE_WAD || wanted.getWantedType() == OFILE_UNKNOWN)
		{
			exts.push_back(".ZIP");
			exts.push_back(".PK3");
		}
	}
	exts.erase(std::unique(exts.begin(), exts.end()), exts.end());

	// A want carrying the directory marker hash refers to a directory
	// resource. No real file can match the marker, so archives are matched
	// by name alone.
	OMD5Hash search_md5 = wanted.getWantedMD5();
	if (M_IsDirectoryMarkerHash(search_md5, basename))
		search_md5 = OMD5Hash();

	// And now...we resolve.
	const std::vector<std::string> dirs = M_FileSearchDirs();
	for (const auto& dir : dirs)
	{
		const std::string searchpath = M_JoinPath(dir, subdir);
		const std::string result =
		    M_BaseFileSearchDir(searchpath, basename, exts, search_md5);
		if (!result.empty())
		{
			// Found a file.
			const std::string fullpath = M_JoinPath(searchpath, result);
			return OResFile::make(out, fullpath);
		}
	}

	// The want carries a content digest: accept any same-named archive or
	// directory whose logical contents match it, regardless of container
	// form or how the archive was compressed.
	if (!want_digest.empty())
	{
		static const std::vector<std::string> archive_exts = {".ZIP", ".PK3"};
		for (const auto& dir : dirs)
		{
			const std::string searchpath = M_JoinPath(dir, subdir);
			const std::string result =
			    M_BaseFileSearchDir(searchpath, basename, archive_exts, OMD5Hash());
			if (!result.empty())
			{
				const std::string fullpath = M_JoinPath(searchpath, result);
				if (Res_ContentDigest(fullpath) == want_digest)
				{
					return OResFile::make(out, fullpath);
				}
			}

			const std::string dirpath = M_JoinPath(searchpath, basename);
			if (M_IsDirectory(dirpath) && Res_ContentDigest(dirpath) == want_digest)
			{
				return OResFile::makeDirectory(out, dirpath);
			}
		}
	}

	// Last resort for extension-less wants: a directory with the
	// wanted name inside one of the search directories.
	if (strext.empty())
	{
		for (const auto& dir : dirs)
		{
			const std::string dirpath = M_JoinPath(M_JoinPath(dir, subdir), basename);
			if (M_IsDirectory(dirpath))
			{
				// A known content digest must match; a same-named directory
				// with different contents is not the wanted resource.
				if (!want_digest.empty() && Res_ContentDigest(dirpath) != want_digest)
				{
					continue;
				}
				return OResFile::makeDirectory(out, dirpath);
			}
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
	return StdStringToLower(a.filename) < StdStringToLower(b.filename);
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

	for (const auto& dir : dirs)
	{
		std::vector<std::string> files = M_BaseFilesScanDir(dir, iwads);
		for (const auto& file : files)
		{
			const std::string fullpath = dir + PATHSEP + file;

			// Check to see if we got a real IWAD.
			const OCRC32Sum crc32 = Res_CRC32(fullpath);
			if (crc32.empty())
				continue;

			// Found a dupe?
			if (found.find(crc32) != found.end())
				continue;

			// Does the gameinfo exist?
			const FileIdentifier* id = W_GameInfo(crc32);
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
	const StringTokens dirs = M_FileSearchDirs();

	// possibly change this
	std::vector<scannedPWAD_t> rvo;
	OHashTable<std::string, bool> found;

	for (const auto& dir : dirs)
	{
		const StringTokens files = M_PWADFilesScanDir(dir);
		for (const auto& filename : files)
		{
			// [AM] Don't include odamex.wad or IWADs.
			if (iequals(filename, "odamex.wad"))
				continue;

			OWantFile file;
			if (iequals(filename, "d.WAD"))
			{
				OMD5Hash hash = Res_MD5(dir + PATHSEP + filename);
				OWantFile::makeWithHash(file, filename, OFILE_WAD, hash);
			}
			else
			{
				OWantFile::make(file, filename, OFILE_WAD);
			}
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

	for (const auto& file : ::wadfiles)
	{
		std::string base = file.getBasename();
		std::string hash = file.getMD5().getHexStr();
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
		PrintFmt("basename: {}\nfullpath: {}\nCRC32: {}\nMD5: {}\nDIGEST: {}\n",
		         res.getBasename(), res.getFullpath(),
		         Res_CRC32(res.getFullpath()).getHexStr(), res.getMD5().getHexStr(),
		         res.getContentDigest().getHexStr());
		return;
	}

	PrintFmt("Could not find location of \"{}\".\n", argv[1]);
}
END_COMMAND(whereis)

BEGIN_COMMAND(loaded)
{
	for (const auto& file : ::wadfiles)
	{
		PrintFmt("{}\n", file.getBasename());
		PrintFmt("  PATH: {}\n", file.getFullpath());
		PrintFmt("  MD5:  {}\n", file.getMD5().getHexStr());
		if (!file.getContentDigest().empty())
			PrintFmt("  DIGEST: {}\n", file.getContentDigest().getHexStr());
	}

	for (const auto& file : ::patchfiles)
	{
		PrintFmt("{}\n", file.getBasename());
		PrintFmt("  PATH: {}\n", file.getFullpath());
		PrintFmt("  MD5:  {}\n", file.getMD5().getHexStr());
	}
}
END_COMMAND(loaded)

BEGIN_COMMAND(searchdirs)
{
	PrintFmt("Search Directories:\n");
	std::vector<std::string> dirs = M_FileSearchDirs();
	for (const auto& dir : dirs)
	{
		PrintFmt("  {}\n", dir);
	}
}
END_COMMAND(searchdirs)
