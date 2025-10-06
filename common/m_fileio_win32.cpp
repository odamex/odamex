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
//	File Input/Output Operations for windows-like platforms.
//
//-----------------------------------------------------------------------------


#if defined(_WIN32)

#if defined(UNIX)
#error "_WIN32 is mutually exclusive with UNIX"
#endif

#include "odamex.h"

#include "m_fileio.h"

#include "win32inc.h"
#include <shlobj.h>
#include <shlwapi.h>

#include "i_system.h"

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
	std::string path = fmt::sprintf("%s\\..", folderPath);
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

	return fmt::sprintf("%s\\My Games\\Odamex", folderPath);
#endif
}

std::string M_GetWriteDir()
{
#if defined(_XBOX)
	return "T:" PATHSEP;
#else
	// Has Odamex been installed?
	std::string installed = M_GetBinaryDir() + PATHSEP "odamex-installed.txt";
	if (M_FileExists(installed))
	{
		// Does the user folder exist?
		std::string userPath = M_GetUserDir();
		int ok = SHCreateDirectoryEx(NULL, userPath.c_str(), NULL);
		if (ok == ERROR_SUCCESS || ok == ERROR_ALREADY_EXISTS)
		{
			return M_CleanPath(userPath);
		}
		else
		{
			I_FatalError("Failed to create {} directory.\n", userPath);
		}
	}

	// Our path is relative to the binary directory.
	// [AM] Don't change this back to CWD because this means your write dir
	//      depends on where you launch it from, which is not great.
	return M_CleanPath(M_GetBinaryDir());
#endif
}

#endif
