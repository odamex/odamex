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
//	File Input/Output Operations for windows-like platforms.
//
//-----------------------------------------------------------------------------

#if defined(_WIN32)

#if defined(UNIX)
#error "_WIN32 is mutually exclusive with UNIX"
#endif

#include "m_fileio.h"

#include <string>

#include "win32inc.h"
#include <shlobj.h>
#include <shlwapi.h>

#include "cmdlib.h"
#include "i_system.h"

#ifdef UNIX
std::string I_GetHomeDir(std::string user = "")
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
#endif

std::string M_GetBinaryDir()
{
	std::string ret;

	char tmp[MAX_PATH]; // denis - todo - make separate function
	GetModuleFileName(NULL, tmp, sizeof(tmp));
	ret = tmp;

	M_FixPathSep(ret);

	size_t slash = ret.find_last_of(PATHSEPCHAR);
	if (slash == std::string::npos)
	{
		return "";
	}

	return ret.substr(0, slash);
}

std::string M_GetHomeDir(const std::string& user)
{
	// [AM] Use SHGetKnownFolderPath when we don't feel like supporting
	//      Windows XP anymore.
	TCHAR folderPath[MAX_PATH];
	if (!SUCCEEDED(SHGetFolderPath(NULL, CSIDL_PERSONAL, NULL, 0, folderPath)))
	{
		I_FatalError("Could not get user's personal folder.\n");
	}

	// Now that we have the Documents folder, just go up one.
	std::string path;
	StrFormat(path, "%s\\..", folderPath);
	return M_CleanPath(path);
}

std::string M_GetUserDir()
{
#if defined(_XBOX)
	return "T:" PATHSEP;
#else
	// [AM] Use SHGetKnownFolderPath when we don't feel like supporting
	//      Windows XP anymore.
	TCHAR folderPath[MAX_PATH];
	if (!SUCCEEDED(SHGetFolderPath(NULL, CSIDL_PERSONAL, NULL, 0, folderPath)))
	{
		I_FatalError("Could not get user's personal folder.\n");
	}

	std::string path;
	StrFormat(path, "%s\\My Games\\Odamex", folderPath);
	return path;
#endif
}

std::string M_GetUserFileName(const std::string& file)
{
#if defined(_XBOX)
	std::string path = "T:";

	path += PATHSEP;
	path += file;

	return M_CleanPath(path);
#else
	// Is absolute path?  If so, stop here.
	if (!PathIsRelative(file.c_str()))
	{
		return file;
	}

	// Is this an explicitly relative path?  If so, stop here.
	size_t fileLen = file.length();
	if (fileLen >= 2 && file[0] == '.' && M_IsPathSep(file[1]))
	{
		return file;
	}
	else if (fileLen >= 3 && file[0] == '.' && file[1] == '.' && M_IsPathSep(file[2]))
	{
		return file;
	}

	// Has Odamex been installed?
	std::string installed = M_GetBinaryDir() + PATHSEP "odamex-installed.txt";
	if (M_FileExists(installed))
	{
		// Does the user folder exist?
		std::string userPath = M_GetUserDir();
		int ok = SHCreateDirectoryEx(NULL, userPath.c_str(), NULL);
		if (ok == ERROR_SUCCESS || ok == ERROR_ALREADY_EXISTS)
		{
			// User directory exists - point to it.
			userPath += PATHSEP;
			userPath += file;

			return M_CleanPath(userPath);
		}
		else
		{
			I_FatalError("Failed to create %s directory.\n", userPath.c_str());
		}
	}

	// Our path is relative to the current working directory.
	std::string path = M_GetCWD();
	if (!M_IsPathSep(*(path.end() - 1)))
	{
		path += PATHSEP;
	}
	path += file;

	return M_CleanPath(path);
#endif
}

#endif
