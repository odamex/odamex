// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 2006-2020 by The Odamex Team.
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

#include "m_fileio.h"

#include <string>
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

#include "m_argv.h"
#include "i_system.h"

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
	std::string home = envhome ? envhome : "";

	if (!home.length())
	{

#ifdef HAVE_PWD_H
		// try the uid way
		passwd* p = user.length() ? getpwnam(user.c_str()) : getpwuid(getuid());
		if (p && p->pw_dir)
			home = p->pw_dir;
#endif

		if (!home.length())
			I_FatalError("Please set your HOME variable");
	}

	if (home[home.length() - 1] != PATHSEPCHAR)
		home += PATHSEP;

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

	// Our path is relative to the home directory.
	std::string path = M_GetHomeDir();
	if (!M_IsPathSep(*(path.end() - 1)))
	{
		path += PATHSEP;
	}
	path += ".odamex";

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

	path += PATHSEP;
	path += file;
#endif
	return M_CleanPath(path);
}

#endif
