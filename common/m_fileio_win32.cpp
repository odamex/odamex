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
#include <nonstd/scope.hpp>

#include <filesystem>
namespace fs = std::filesystem;

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
	PWSTR folderPath;
	if (!SUCCEEDED(SHGetKnownFolderPath(FOLDERID_Profile, 0, NULL, &folderPath)))
	{
		I_FatalError("Could not get user's personal folder.\n");
	}

	fs::path path(folderPath);

	CoTaskMemFree(folderPath);

	return path.string();
}

std::string M_GetUserDir()
{
	PWSTR folderPath;
	if (!SUCCEEDED(SHGetKnownFolderPath(FOLDERID_Documents, 0, NULL, &folderPath)))
	{
		I_FatalError("Could not get user's personal folder.\n");
	}

	fs::path path(folderPath);

	CoTaskMemFree(folderPath);

	return (path / "My Games" / "Odamex").string();
}

std::string M_GetWriteDir()
{
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
}

#endif
