// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 2006-2025 by The Odamex Team.
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
#include <filesystem>

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

namespace fs = std::filesystem;

std::string M_GetBinaryDir()
{
#if defined(__SWITCH__)
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
	fs::path path = M_GetHomeDir();
	path /= ".odamex";
	return path.string();
}

std::string M_GetWriteDir()
{
	// Our path is relative to the home directory
	std::string path = M_GetUserDir();

	// Create the directory.
	struct stat info;
	if (stat(path.c_str(), &info) == -1)
	{
		if (mkdir(path.c_str(), S_IRUSR | S_IWUSR | S_IXUSR) == -1)
		{
			I_FatalError("Failed to create {} directory:\n{}", path,
			             strerror(errno));
		}
	}
	else
	{
		if (!S_ISDIR(info.st_mode))
		{
			I_FatalError("{} must be a directory", path);
		}
	}

	return path;
}

#endif
