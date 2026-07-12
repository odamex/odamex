// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2006-2015 by The Odamex Team.
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
//
// Resource file locating functions
//
//-----------------------------------------------------------------------------

#include "odamex.h"
#include "m_fileio.h"
#include "md5.h"
#include "cmdlib.h"

#include <string>
#include <vector>
#include <sstream>
#include <fstream>
#include <algorithm>
#include <iomanip>

#ifdef _WIN32
	#include "win32inc.h"
#endif
#include "crc32.h"


// denis - Standard MD5SUM
OMD5Hash Res_MD5(const std::string& filename)
{
	OMD5Hash rvo;

	const int file_chunk_size = 8192;
	auto fp = uqFile(fopen(filename.c_str(), "rb"));

	if (!fp)
		return rvo;

	md5_state_t state;
	md5_init(&state);

	size_t n = 0;
	unsigned char buf[file_chunk_size];

	while ((n = fread(buf, 1, sizeof(buf), fp.get())))
		md5_append(&state, static_cast<byte*>(buf), n);

	md5_byte_t digest[16];
	md5_finish(&state, digest);

	std::stringstream hashStr;

	for (int i = 0; i < 16; i++)
		hashStr << std::setw(2) << std::setfill('0') << std::hex << std::uppercase << static_cast<short>(digest[i]);

	OMD5Hash::makeFromHexStr(rvo, hashStr.str());
	return rvo;
}

/**
 * @brief Calculate a CRC32 hash from a file.
 *
 * @param filename Filename of file to hash.
 * @return Output hash, or blank if file could not be found.
 */
OCRC32Sum Res_CRC32(const std::string& filename)
{
	OCRC32Sum rvo;

	const int file_chunk_size = 8192;
	auto fp = uqFile(fopen(filename.c_str(), "rb"));

	if (!fp)
		return rvo;

	size_t n = 0;
	unsigned char buf[file_chunk_size];
	uint32_t crc = 0;

	while ((n = fread(buf, 1, sizeof(buf), fp.get())))
	{
		crc = crc32_fast(buf, n, crc);
	}

	std::string hashStr;

	hashStr = fmt::sprintf("%08X", crc);

	OCRC32Sum::makeFromHexStr(rvo, hashStr);
	return rvo; // bubble up failure
}

//
// Res_CleanseFilename
//
// Strips a file name of path information and transforms it into uppercase.
//
std::string Res_CleanseFilename(const std::string& filename)
{
	std::string newname(filename);
	FixPathSeparator(newname);

	size_t slash = newname.find_last_of(PATHSEPCHAR);
	if (slash != std::string::npos)
		newname = newname.substr(slash + 1, newname.length() - slash);

	std::transform(newname.begin(), newname.end(), newname.begin(), toupper);

	return newname;
}


//
// Res_AddSearchDir
//
// denis - Split a new directory string using the separator and append results to the output
//
void Res_AddSearchDir(std::vector<std::string>& search_dirs, const char* dir, const char separator)
{
	if (!dir || dir[0] == '\0')
		return;

	// search through dwd
	std::stringstream ss(dir);
	std::string segment;

	while (!ss.eof())
	{
		std::getline(ss, segment, separator);

		if (segment.empty())
			continue;

		FixPathSeparator(segment);
		M_ExpandHomeDir(segment);

		if (segment[segment.length() - 1] != PATHSEPCHAR)
			segment += PATHSEP;

		search_dirs.push_back(segment);
	}
}


#if defined(_WIN32) && !defined(_XBOX)

typedef struct
{
	HKEY root;
	const char* path;
	const char* value;
} registry_value_t;

static const char* uninstaller_string = "\\uninstl.exe /S ";

// Keys installed by the various CD editions.  These are actually the
// commands to invoke the uninstaller and look like this:
//
// C:\Program Files\Path\uninstl.exe /S C:\Program Files\Path
//
// With some munging we can find where Doom was installed.

// [AM] From the persepctive of a 64-bit executable, 32-bit registry keys are
//      located in a different spot.
#if _WIN64
#define SOFTWARE_KEY "Software\\Wow6432Node"
#else
#define SOFTWARE_KEY "Software"
#endif	// _WIN64

static registry_value_t uninstall_values[] =
{
	// Ultimate Doom, CD version (Depths of Doom trilogy)
	{
		HKEY_LOCAL_MACHINE,
		SOFTWARE_KEY "\\Microsoft\\Windows\\CurrentVersion\\"
			"Uninstall\\Ultimate Doom for Windows 95",
		"UninstallString",
	},

	// Doom II, CD version (Depths of Doom trilogy)
	{
		HKEY_LOCAL_MACHINE,
		SOFTWARE_KEY "\\Microsoft\\Windows\\CurrentVersion\\"
			"Uninstall\\Doom II for Windows 95",
		"UninstallString",
	},

	// Final Doom
	{
		HKEY_LOCAL_MACHINE,
		SOFTWARE_KEY "\\Microsoft\\Windows\\CurrentVersion\\"
			"Uninstall\\Final Doom for Windows 95",
		"UninstallString",
	},

	// Shareware version
	{
		HKEY_LOCAL_MACHINE,
		SOFTWARE_KEY "\\Microsoft\\Windows\\CurrentVersion\\"
			"Uninstall\\Doom Shareware for Windows 95",
		"UninstallString",
	},
};

// Value installed by the Collector's Edition when it is installed
static registry_value_t collectors_edition_value =
{
	HKEY_LOCAL_MACHINE,
	SOFTWARE_KEY "\\Activision\\DOOM Collector's Edition\\v1.0",
	"INSTALLPATH",
};

// Subdirectories of the above install path, where IWADs are installed.
static const char* collectors_edition_subdirs[] =
{
	"Doom2",
	"Final Doom",
	"Ultimate Doom",
};

// Location where Steam is installed
static registry_value_t steam_install_location =
{
	HKEY_LOCAL_MACHINE,
	SOFTWARE_KEY "\\Valve\\Steam",
	"InstallPath",
};

// Subdirs of the steam install directory where IWADs are found
static const char* steam_install_subdirs[] =
{
	"steamapps\\common\\doom 2\\base",
	"steamapps\\common\\Doom 2\\masterbase",
	"steamapps\\common\\final doom\\base",
	"steamapps\\common\\Doom 2\\finaldoombase",
	"steamapps\\common\\ultimate doom\\base",
	"steamapps\\common\\DOOM 3 BFG Edition\\base\\wads",
	"steamapps\\common\\master levels of doom\\master\\wads", //Let Odamex find the Master Levels pwads too
	"steamapps\\common\\ultimate doom\\base\\doom2", //2024 Steam re-release additions here and below
	"steamapps\\common\\ultimate doom\\base\\master\\wads",
	"steamapps\\common\\ultimate doom\\base\\plutonia",
	"steamapps\\common\\ultimate doom\\base\\tnt",
	"steamapps\\common\\ultimate doom\\rerelease",
};


static char* GetRegistryString(registry_value_t *reg_val)
{
	char* result = NULL;
	HKEY key = 0;
	DWORD len = 0;
	DWORD valtype = 0;

	// Open the key (directory where the value is stored)
	if (RegOpenKeyEx(reg_val->root, reg_val->path, 0, KEY_READ, &key) != ERROR_SUCCESS)
		return NULL;

	// Find the type and length of the string, and only accept strings.
	if (RegQueryValueEx(key, reg_val->value, NULL, &valtype, NULL, &len) == ERROR_SUCCESS && valtype == REG_SZ)
	{
		// Allocate a buffer for the value and read the value
		result = static_cast<char*>(malloc(len));

		if (RegQueryValueEx(key, reg_val->value, NULL, &valtype, reinterpret_cast<unsigned char*>(result), &len) != ERROR_SUCCESS)
		{
			free(result);
			result = NULL;
		}
	}

	RegCloseKey(key);	// Close the key
	return result;
}

#endif	// _WIN32 && !_XBOX


//
// Res_AddPlatformSearchDirs
//
// [AM] Add platform-sepcific search directories
//
void Res_AddPlatformSearchDirs(std::vector<std::string>& search_dirs)
{
#if defined(_WIN32) && !defined(_XBOX)
	#define arrlen(array) (sizeof(array) / sizeof(*array))

	// Doom 95
	{
		for (unsigned int i = 0; i < arrlen(uninstall_values); ++i)
		{
			char* val = GetRegistryString(&uninstall_values[i]);
			if (val == NULL)
				continue;

			char* unstr = strstr(val, uninstaller_string);
			if (unstr == NULL)
			{
				free(val);
			}
			else
			{
				char* path = unstr + strlen(uninstaller_string);
				Res_AddSearchDir(search_dirs, path, SEARCHPATHSEPCHAR);
			}
		}
	}

	// Doom Collectors Edition
	{
		char* install_path = GetRegistryString(&collectors_edition_value);
		if (install_path != NULL)
		{
			for (unsigned int i = 0; i < arrlen(collectors_edition_subdirs); ++i)
			{
				char* subpath = static_cast<char*>(malloc(strlen(install_path)
				                             + strlen(collectors_edition_subdirs[i])
				                             + 5));
				sprintf(subpath, "%s\\%s", install_path, collectors_edition_subdirs[i]);
				Res_AddSearchDir(search_dirs, subpath, SEARCHPATHSEPCHAR);
				free(subpath);
			}

			free(install_path);
		}
	}

	// Doom on Steam
	{
		char* install_path = GetRegistryString(&steam_install_location);
		if (install_path != NULL)
		{
			for (size_t i = 0; i < arrlen(steam_install_subdirs); ++i)
			{
				char* subpath = static_cast<char*>(malloc(strlen(install_path)
				                             + strlen(steam_install_subdirs[i]) + 5));
				sprintf(subpath, "%s\\%s", install_path, steam_install_subdirs[i]);
				Res_AddSearchDir(search_dirs, subpath, SEARCHPATHSEPCHAR);
				free(subpath);
			}

			free(install_path);
		}
	}

	// DOS Doom via DEICE
	Res_AddSearchDir(search_dirs, "\\doom2", SEARCHPATHSEPCHAR);    // Doom II
	Res_AddSearchDir(search_dirs, "\\plutonia", SEARCHPATHSEPCHAR); // Final Doom
	Res_AddSearchDir(search_dirs, "\\tnt", SEARCHPATHSEPCHAR);
	Res_AddSearchDir(search_dirs, "\\doom_se", SEARCHPATHSEPCHAR);  // Ultimate Doom
	Res_AddSearchDir(search_dirs, "\\doom", SEARCHPATHSEPCHAR);     // Shareware / Registered Doom
	Res_AddSearchDir(search_dirs, "\\dooms", SEARCHPATHSEPCHAR);    // Shareware versions
	Res_AddSearchDir(search_dirs, "\\doomsw", SEARCHPATHSEPCHAR);
#endif	// _WIN32 && !_XBOX

#ifdef UNIX
	#if defined(INSTALL_PREFIX) && defined(INSTALL_DATADIR)
	Res_AddSearchDir(search_dirs, INSTALL_PREFIX "/" INSTALL_DATADIR "/odamex", SEARCHPATHSEPCHAR);
	Res_AddSearchDir(search_dirs, INSTALL_PREFIX "/" INSTALL_DATADIR "/games/odamex", SEARCHPATHSEPCHAR);
	#endif

	Res_AddSearchDir(search_dirs, "/usr/share/doom", SEARCHPATHSEPCHAR);
	Res_AddSearchDir(search_dirs, "/usr/share/games/doom", SEARCHPATHSEPCHAR);
	Res_AddSearchDir(search_dirs, "/usr/local/share/games/doom", SEARCHPATHSEPCHAR);
	Res_AddSearchDir(search_dirs, "/usr/local/share/doom", SEARCHPATHSEPCHAR);
#endif	// UNIX
}


VERSION_CONTROL (res_filelib_cpp, "$Id$")
