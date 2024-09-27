// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 2006-2024 by The Odamex Team.
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
//	File Input/Output Operations
//
//-----------------------------------------------------------------------------


#if defined(UNIX)

#if defined(_WIN32)
#error "UNIX is mutually exclusive with _WIN32"
#endif

#include "odamex.h"

#include "m_fileio.h"

#include <sstream>

#include <errno.h>
#include <pwd.h>
#include <sys/stat.h>
#include <unistd.h>
#if defined(__linux__)
#include <linux/limits.h>
#else
#include <sys/syslimits.h>
#endif
#include <dirent.h>

#include "cmdlib.h"
#include "i_system.h"
#include "m_argv.h"
#include "m_ostring.h"
#include "w_wad.h"

std::string M_GetBinaryDir()
{
#if defined(GEKKO)
	ret = "sd:/";
#elif defined(__SWITCH__)
	return "./";
#else
	std::string ret;

	if (!Args[0])
		return "./";

	char realp[PATH_MAX];
	if (realpath(Args[0], realp))
		ret = realp;
	else
	{
		// search through $PATH
		const char* path = getenv("PATH");
		if (path)
		{
			std::stringstream ss(path);
			std::string segment;

			while (ss)
			{
				std::getline(ss, segment, ':');

				if (!segment.length())
					continue;

				if (segment[segment.length() - 1] != PATHSEPCHAR)
					segment += PATHSEP;
				segment += Args[0];

				if (realpath(segment.c_str(), realp))
				{
					ret = realp;
					break;
				}
			}
		}
	}

	M_FixPathSep(ret);

	size_t slash = ret.find_last_of(PATHSEPCHAR);
	if (slash == std::string::npos)
		return "";
	else
		return ret.substr(0, slash);
#endif
}

std::string M_GetHomeDir(const std::string& user)
{
	const char* envhome = getenv("HOME");
	std::string home = (envhome != NULL) ? envhome : "";

#ifndef __SWITCH__
	if (!home.length())
	{
		// try the uid way
		passwd* p = user.length() ? getpwnam(user.c_str()) : getpwuid(getuid());
		if (p && p->pw_dir)
		{
			home = p->pw_dir;
		}

		if (!home.length())
		{
			I_FatalError("Please set your HOME variable");
		}
	}

	if (home[home.length() - 1] != PATHSEPCHAR)
	{
		home += PATHSEP;
	}
#endif

	return home;
}

std::string M_GetUserDir()
{
	std::string path = M_GetHomeDir();

	if (path[path.length() - 1] != PATHSEPCHAR)
		path += PATHSEP;

	path += ".odamex";
	return path;
}

std::string M_GetWriteDir()
{
	// Our path is relative to the home directory.
	std::string path = M_GetHomeDir();
	if (!M_IsPathSep(*(path.end() - 1)))
	{
		path += PATHSEP;
	}
	path += ".odamex";

	// Create the directory.
	struct stat info;
	if (stat(path.c_str(), &info) == -1)
	{
		if (mkdir(path.c_str(), S_IRUSR | S_IWUSR | S_IXUSR) == -1)
		{
			I_FatalError("Failed to create %s directory:\n%s", path.c_str(),
			             strerror(errno));
		}
	}
	else
	{
		if (!S_ISDIR(info.st_mode))
		{
			I_FatalError("%s must be a directory", path.c_str());
		}
	}

	return path;
}

std::string M_GetUserFileName(const std::string& file)
{

#ifdef __SWITCH__
		std::string path = file;
#else
	// Is absolute path?  If so, stop here.
	size_t fileLen = file.length();
	if (fileLen >= 1 && M_IsPathSep(file[0]))
	{
		return file;
	}

	// Is this an explicitly relative path?  If so, stop here.
	if (fileLen >= 2 && file[0] == '.' && M_IsPathSep(file[1]))
	{
		return file;
	}
	else if (fileLen >= 3 && file[0] == '.' && file[1] == '.' && M_IsPathSep(file[2]))
	{
		return file;
	}

	// Our path is relative to the write directory.
	std::string path = M_GetWriteDir();
	path += PATHSEP;
	path += file;
#endif
	return M_CleanPath(path);
}

std::string M_BaseFileSearchDir(std::string dir, const std::string& name,
                                const std::vector<std::string>& exts,
                                const OMD5Hash& hash)
{
	dir = M_CleanPath(dir);
	std::vector<OString> cmp_files;
	for (std::vector<std::string>::const_iterator it = exts.begin(); it != exts.end();
	     ++it)
	{
		if (!hash.empty())
		{
			// Filenames with supplied hashes always match first.
			cmp_files.push_back(
			    StdStringToUpper(name + "." + hash.getHexStr().substr(0, 6) + *it));
		}
		cmp_files.push_back(StdStringToUpper(name + *it));
	}

	// denis - list files in the directory of interest, case-desensitize
	// then see if wanted wad is listed
	struct dirent** namelist = 0;
	int n = scandir(dir.c_str(), &namelist, 0, alphasort);

	std::string found;
	std::vector<OString>::iterator found_it = cmp_files.end();
	for (int i = 0; i < n && namelist[i]; i++)
	{
		const std::string d_name = namelist[i]->d_name;
		M_Free(namelist[i]);

		if (found_it == cmp_files.begin())
			continue;

		if (d_name == "." || d_name == "..")
			continue;

		const std::string check = StdStringToUpper(d_name);
		std::vector<OString>::iterator this_it =
		    std::find(cmp_files.begin(), cmp_files.end(), check);
		if (this_it < found_it)
		{
			const std::string local_file(dir + PATHSEP + d_name);
			const OMD5Hash local_hash(W_MD5(local_file));

			if (hash.empty() || hash == local_hash)
			{
				// Found a match.
				found = d_name;
				found_it = this_it;
				continue;
			}
			else if (!hash.empty())
			{
				Printf(PRINT_WARNING, "WAD at %s does not match required copy\n",
				       local_file.c_str());
				Printf(PRINT_WARNING, "Local MD5: %s\n", local_hash.getHexCStr());
				Printf(PRINT_WARNING, "Required MD5: %s\n\n", hash.getHexCStr());
			}
		}
	}

	M_Free(namelist);
	return found;
}

std::vector<std::string> M_BaseFilesScanDir(std::string dir, std::vector<OString> files)
{
	std::vector<std::string> rvo;

	// Fix up parameters.
	dir = M_CleanPath(dir);
	for (size_t i = 0; i < files.size(); i++)
	{
		files[i] = StdStringToUpper(files[i]);
	}

	struct dirent** namelist = 0;
	int n = scandir(dir.c_str(), &namelist, 0, alphasort);

	for (int i = 0; i < n && namelist[i]; i++)
	{
		const std::string d_name = namelist[i]->d_name;
		M_Free(namelist[i]);

		// Find the file.
		std::string check = StdStringToUpper(d_name);
		std::vector<OString>::iterator it = std::find(files.begin(), files.end(), check);

		if (it == files.end())
			continue;

		rvo.push_back(d_name);
	}

	return rvo;
}

// Scan for PWADs and DEH and BEX files
std::vector<std::string> M_PWADFilesScanDir(std::string dir)
{
	std::vector<std::string> rvo;

	// Fix up parameters.
	dir = M_CleanPath(dir);

	struct dirent** namelist = 0;
	int n = scandir(dir.c_str(), &namelist, 0, alphasort);

	for (int i = 0; i < n && namelist[i]; i++)
	{
		const std::string d_name = namelist[i]->d_name;
		M_Free(namelist[i]);

		// Don't care about files with names shorter than the extensions
		if (d_name.length() < 4)
			continue;

		// Only return files with correct extensions
		std::string check = StdStringToUpper(d_name).substr(d_name.length() - 4);
		if (check.compare(".WAD") && check.compare(".DEH") && check.compare(".BEX"))
			continue;

		rvo.push_back(d_name);
	}

	return rvo;
}

bool M_GetAbsPath(const std::string& path, std::string& out)
{

#ifdef __SWITCH__
	std::string res;
	StrFormat(res, "%s", path.c_str());
	out = res;
	return true;
#else
	char buffer[PATH_MAX];
	char* res = realpath(path.c_str(), buffer);
	if (res == NULL)
	{
		return false;
	}
	out = res;
	return true;
#endif
}

#endif
