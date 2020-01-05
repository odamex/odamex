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

#include "doomtype.h"
#include "cmdlib.h"
#include "m_fileio.h"
#include "i_system.h"
#include "md5.h"
#include "m_argv.h"
#include "c_cvars.h"
#include "resources/res_main.h"
#include "w_ident.h"

#include <string>
#include <vector>
#include <sstream>
#include <algorithm>
#include <iomanip>

#ifdef UNIX
	#include <unistd.h>
	#include <dirent.h>
	#include <sys/stat.h>
#endif

#ifdef _WIN32
	#include "win32inc.h"
#endif

EXTERN_CVAR(waddirs)

struct HashMismatch
{
	std::string		filename;
	std::string		required_hash;
	std::string		actual_hash;
};

// global list of which filenames failed hash checking
static std::vector<HashMismatch> hash_mismatches;

// global list of filename extensions for use when an extension isn't supplied to Res_FindResourceFile
static const char* const IWAD_EXTLIST[] = { ".WAD" };
static const char* const WAD_EXTLIST[] = { ".WAD", ".ZIP", ".PK3" };
static const char* const DEH_EXTLIST[] = { ".DEH", ".BEX" };
static const char* const ALL_EXTLIST[] = { ".WAD", ".ZIP", ".PK3", ".DEH", ".BEX" };

// extern functions
const char* ParseString2(const char* data);


// denis - Standard MD5SUM
std::string Res_MD5(const std::string& filename)
{
	const int file_chunk_size = 8192;
	FILE *fp = fopen(filename.c_str(), "rb");
	if (!fp)
		return "";

	md5_state_t state;
	md5_init(&state);

	unsigned n = 0;
	unsigned char buf[file_chunk_size];

	while ((n = fread(buf, 1, sizeof(buf), fp)))
		md5_append(&state, (unsigned char *)buf, n);

	md5_byte_t digest[16];
	md5_finish(&state, digest);

	fclose(fp);

	std::stringstream hash;
	for (int i = 0; i < 16; i++)
		hash << std::setw(2) << std::setfill('0') << std::hex << std::uppercase << (short)digest[i];

	return hash.str();
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
		I_ExpandHomeDir(segment);

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
	"steamapps\\common\\final doom\\base",
	"steamapps\\common\\ultimate doom\\base",
	"steamapps\\common\\DOOM 3 BFG Edition\\base\\wads",
	"steamapps\\common\\master levels of doom\\master\\wads", //Let Odamex find the Master Levels pwads too
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

		if (RegQueryValueEx(key, reg_val->value, NULL, &valtype, (unsigned char*)result, &len) != ERROR_SUCCESS)
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
static void Res_AddPlatformSearchDirs(std::vector<std::string>& search_dirs)
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

	Res_AddSearchDir(search_dirs, "/usr/share/games/doom", SEARCHPATHSEPCHAR);
	Res_AddSearchDir(search_dirs, "/usr/local/share/games/doom", SEARCHPATHSEPCHAR);
	Res_AddSearchDir(search_dirs, "/usr/local/share/doom", SEARCHPATHSEPCHAR);
#endif	// UNIX
}


//
// Res_BaseFileSearchDir
//
// [SL] Searches the given directory for a case-insensitive match of filename.
// If a hash is supplied, any matches will have their MD5Sum verified against
// the supplied hash and an entry in hash_mismatches will be added if
// verification fails. Upon successful matching, the full path to the file
// will be returned (case-sensitive).
//
static std::string Res_BaseFileSearchDir(
		std::string dir, std::string filename,
		const std::string& hash = "")
{
	std::string found;

	// tack a trailing slash on the end of dir
	if (dir[dir.length() - 1] != PATHSEPCHAR)
		dir += PATHSEP;

	// handle relative directories
	std::string relative_dir;
	M_ExtractFilePath(filename, relative_dir);
	if (!relative_dir.empty())
		dir += relative_dir;
	if (dir[dir.length() - 1] != PATHSEPCHAR)
		dir += PATHSEP;

	filename = Res_CleanseFilename(filename);

	// denis - list files in the directory of interest, case-desensitize
	// then see if wanted wad is listed
#ifdef UNIX
	DIR* dp = opendir(dir.c_str());
	if (!dp)
		return std::string();

	struct dirent* entry;
	while ( (entry = readdir(dp)) )
	{
		std::string local_base_filename(entry->d_name);
		std::string local_filename(dir + local_base_filename);

		// ensure this is a regular file
		struct stat st;
		stat(local_filename.c_str(), &st);
		if (!S_ISREG(st.st_mode))
			continue;

		if (iequals(filename, local_base_filename))
		{
			std::string local_hash(Res_MD5(local_filename));

			if (hash.empty() || iequals(hash, local_hash))
			{
				found = local_filename;
				break;
			}
			else
			{
				hash_mismatches.push_back(HashMismatch());
				hash_mismatches.back().filename = local_filename;
				hash_mismatches.back().required_hash = StdStringToUpper(hash);
				hash_mismatches.back().actual_hash = StdStringToUpper(local_hash);
			}
		}
	}

	closedir(dp);
#endif	// UNIX

#ifdef _WIN32
	WIN32_FIND_DATA FindFileData;
	std::string all_ext(dir + "*");
	HANDLE hFind = FindFirstFile(all_ext.c_str(), &FindFileData);

	if (hFind == INVALID_HANDLE_VALUE)
		return std::string();

	do
	{
		if (FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			continue;

		std::string local_base_filename(FindFileData.cFileName);
		std::string local_filename(dir + local_base_filename);

		if (iequals(filename, local_base_filename))
		{
			std::string local_hash(Res_MD5(local_filename));

			if (hash.empty() || iequals(hash, local_hash))
			{
				found = local_filename; 
				break;
			}
			else
			{
				hash_mismatches.push_back(HashMismatch());
				hash_mismatches.back().filename = local_filename;
				hash_mismatches.back().required_hash = StdStringToUpper(hash);
				hash_mismatches.back().actual_hash = StdStringToUpper(local_hash);
			}
		}
	} while (FindNextFile(hFind, &FindFileData));

	FindClose(hFind);
#endif // _WIN32

	return found;
}


//
// Res_BaseFileSearch
//
// denis - Check all paths of interest for a given file with a possible extension
//
static std::string Res_BaseFileSearch(const std::string& filename, const std::string& hash = "")
{
	std::string absolute_dir;

	#ifdef _WIN32
		if (filename.find(':') != std::string::npos)			// absolute path?
			M_ExtractFilePath(filename, absolute_dir);
	#endif	// _WIN32
	
	#ifdef UNIX
		if (filename[0] == PATHSEPCHAR || filename[0] == '~')	// absolute path?
			M_ExtractFilePath(filename, absolute_dir);
	#endif	// UNIX

	if (!absolute_dir.empty())
	{
		std::string base_filename = Res_CleanseFilename(filename);
		std::string full_filename = Res_BaseFileSearchDir(absolute_dir, base_filename, hash);
		// filename not found? Try filename.hash
		if (full_filename.empty() && !hash.empty())
			full_filename = Res_BaseFileSearchDir(absolute_dir, base_filename + '.' + hash, hash);
		return full_filename;
	}

	std::vector<std::string> search_dirs;
	search_dirs.push_back(startdir);
	search_dirs.push_back(progdir);

	Res_AddSearchDir(search_dirs, Args.CheckValue("-waddir"), SEARCHPATHSEPCHAR);
	Res_AddSearchDir(search_dirs, getenv("DOOMWADDIR"), SEARCHPATHSEPCHAR);
	Res_AddSearchDir(search_dirs, getenv("DOOMWADPATH"), SEARCHPATHSEPCHAR);
	Res_AddSearchDir(search_dirs, getenv("HOME"), SEARCHPATHSEPCHAR);

	// [AM] Search additional paths based on platform
	Res_AddPlatformSearchDirs(search_dirs);

	Res_AddSearchDir(search_dirs, waddirs.cstring(), SEARCHPATHSEPCHAR);

	search_dirs.erase(std::unique(search_dirs.begin(), search_dirs.end()), search_dirs.end());

	for (size_t i = 0; i < search_dirs.size(); i++)
	{
		std::string dir(search_dirs[i]);

		std::string full_filename = Res_BaseFileSearchDir(dir, filename, hash);
		// filename not found? Try filename.hash
		if (full_filename.empty() && !hash.empty())
			full_filename = Res_BaseFileSearchDir(dir, filename + '.' + hash, hash);
		if (!full_filename.empty())
			return full_filename;
	}

	return std::string();
}


//
// Res_FindResourceFile
//
// Searches for a given filename, and returns the full path to the file if
// found and matches the supplied hash (or the hash was empty). An empty
// string is returned if no matching file is found.
//
std::string Res_FindResourceFile(const std::string& filename, const std::string& hash = "")
{
	return Res_BaseFileSearch(filename, hash);
}


//
// Res_FindResourceFile
//
// Searches for a given filename like the standard version of
// Res_FindResourceFile, but will try other filename extensions as well.
// 
std::string Res_FindResourceFile(
		const std::string& filename,
		const char* const extlist[], 
		const std::string& hash = "")
{
	const size_t extlist_size = sizeof(extlist) / sizeof(*extlist);

	// Check for the filename without adding any extensions
	std::string full_filename = Res_FindResourceFile(filename, hash);
	if (!full_filename.empty())
		return full_filename;

	// Try adding each extension in the list until a match is found
	for (size_t i = 0; i < extlist_size; i++)
	{
		std::string ext(extlist[i]);
		if (!ext.empty() && ext[0] != '.')
			ext = '.' + ext;
		std::string new_filename = M_AppendExtension(filename, ext);

		std::string full_filename = Res_FindResourceFile(new_filename, hash);
		if (!full_filename.empty())
			return full_filename;
	}

	return std::string();	// not found with any supplied extension
}


//
// Res_FindIWAD
//
// Tries to find an IWAD from a set of known IWAD file names.
//
std::string Res_FindIWAD(const std::string& suggestion = "")
{
	if (!suggestion.empty())
	{
		std::string full_filename = Res_FindResourceFile(suggestion, IWAD_EXTLIST);
		if (!full_filename.empty() && W_IsIWAD(full_filename))
			return full_filename;
	}

	// Search for a pre-defined IWAD from the list above
	std::vector<OString> filenames = W_GetIWADFilenames();
	for (size_t i = 0; i < filenames.size(); i++)
	{
		std::string full_filename = Res_FindResourceFile(filenames[i]);
		if (!full_filename.empty() && W_IsIWAD(full_filename))
			return full_filename;
	}

	return std::string();
}


//
// Res_GatherResourceFilesFromString
//
// Adds the full path of all of the resource filenames in a given string
// (separated by spaces).
//
std::vector<std::string> Res_GatherResourceFilesFromString(const std::string& str)
{
	std::vector<std::string> resource_filenames;

	const char* data = str.c_str();

	for (size_t argv = 0; (data = ParseString2(data)); argv++)
	{
		std::string filename(com_token);

		if (M_IsDirectory(filename))
		{
			resource_filenames.push_back(filename);
		}
		else
		{
			std::string full_filename(Res_FindResourceFile(filename, ALL_EXTLIST));
			if (!full_filename.empty())
				resource_filenames.push_back(full_filename);
			// [SL] TODO: properly handle missing files
			// else
				// missing_files.push_back(filename);
		}
	}

	return resource_filenames;
}


//
// Res_GetFullPath
static std::string Res_GetFullPath(const std::string& filename)
{
	if (M_IsDirectory(filename))
		return filename;

	return Res_FindResourceFile(filename, ALL_EXTLIST);
}


//
// Res_GatherResourceFileFromArgs
//
// Adds the full path of all the resource filenames given on the command line
// following the "-iwad", "-file", or "-deh" parameters.
//
std::vector<std::string> Res_GatherResourceFilesFromArgs()
{
	std::vector<std::string> resource_filenames;

	// Note: this only adds the first filename specified with the -iwad parameter
	size_t i = Args.CheckParm("-iwad");
	if (i > 0 && i < Args.NumArgs() - 1)
	{
		std::string filename(Args.GetArg(i + 1));
		std::string full_filename(Res_FindResourceFile(filename, IWAD_EXTLIST));

		if (!full_filename.empty())
			resource_filenames.push_back(full_filename);
		// [SL] TODO: properly handle missing files
		// else
			// missing_files.push_back(filename);
	}

	// [SL] the first parameter should be treated as a file name and
	// doesn't need to be preceeded by -file
	bool is_filename = true;
	const char* const* extlist = ALL_EXTLIST;

	for (size_t i = 1; i < Args.NumArgs(); i++)
	{
		const char* arg_value = Args.GetArg(i);
		if (arg_value[0] == '-' || arg_value[0] == '+')
		{
			if (stricmp(arg_value, "-file") == 0)
				is_filename = true, extlist = ALL_EXTLIST;
			else if (stricmp(arg_value, "-deh") == 0)
				is_filename = true, extlist = DEH_EXTLIST;
			else
				is_filename = false;
		}
		else if (is_filename)
		{
			std::string full_filename = Res_GetFullPath(arg_value);
			if (!full_filename.empty())
				resource_filenames.push_back(full_filename);
		}
	}

	return resource_filenames;
}


//
// Res_ValidateResourceFiles
//
// Validates a list of resource filenames.
// Ensures that the list contains a valid ODAMEX.WAD resource file and a valid
// IWAD resource file, adding them if they weren't in the original list.
//
// If the file names do not include full paths, the default search paths will be
// used to find the files.
//
std::vector<std::string> Res_ValidateResourceFiles(const std::vector<std::string>& resource_filenames)
{
	std::vector<std::string> new_resource_filenames;

	const size_t invalid_position = (size_t)-1;
	size_t odamex_wad_position = invalid_position;
	size_t iwad_position = invalid_position;

	const OString& engine_resource_filename(Res_GetEngineResourceFileName());

	if (resource_filenames.size() >= 1 && iequals(Res_CleanseFilename(resource_filenames[0]), engine_resource_filename))
		odamex_wad_position = 0;
	
	if (odamex_wad_position == invalid_position && resource_filenames.size() >= 1 && W_IsIWAD(Res_GetFullPath(resource_filenames[0])))
		iwad_position = 0;

	if (odamex_wad_position == 0 && resource_filenames.size() >= 2 && W_IsIWAD(Res_GetFullPath(resource_filenames[1])))
		iwad_position = 1;

	// locate ODAMEX.WAD and add its full path to the validated filename array
	std::string odamex_wad_filename;
	if (odamex_wad_position == invalid_position)
		odamex_wad_filename = engine_resource_filename;
	else
		odamex_wad_filename = resource_filenames[odamex_wad_position];
	std::string odamex_wad_full_filename = Res_GetFullPath(odamex_wad_filename);
	if (odamex_wad_full_filename.empty())
		I_FatalError("Unable to locate \"%s\" resource file.", engine_resource_filename.c_str());
	else
		new_resource_filenames.push_back(odamex_wad_full_filename);

	// locate the IWAD and add its full path to the validated filename array
	std::string iwad_filename;
	if (iwad_position == invalid_position)
		iwad_filename = Res_FindIWAD();
	else
		iwad_filename = resource_filenames[iwad_position];
	std::string iwad_full_filename = Res_GetFullPath(iwad_filename);
	if (iwad_full_filename.empty())
		I_FatalError("Unable to locate any IWAD resource files.");
	else
		new_resource_filenames.push_back(iwad_full_filename);

	// add any PWADs to the validated filename array
	size_t pwad_start_position = std::max<int>(odamex_wad_position, iwad_position) + 1;
	size_t pwad_end_position = resource_filenames.size();

	if (W_IsIWADShareware(iwad_filename) && pwad_end_position > pwad_start_position)
	{
		Printf(PRINT_HIGH, "You cannot load additional resource files with the shareware version. Register!\n");
		pwad_end_position = pwad_start_position;
	}

	for (size_t i = pwad_start_position; i < pwad_end_position; i++)
	{
		std::string full_filename = Res_GetFullPath(resource_filenames[i]);
		if (!full_filename.empty())
			new_resource_filenames.push_back(full_filename);
		// [SL] TODO: properly handle missing files
	}

	return new_resource_filenames;
}

VERSION_CONTROL (res_filelib_cpp, "$Id$")
