// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
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
//	Common functions to determine game mode (shareware, registered),
//	parse command line parameters, and handle wad changes.
//
//-----------------------------------------------------------------------------

#include "version.h"

#include <sstream>
#include <string>
#include <vector>
#include <algorithm>

#include "win32inc.h"
#ifndef _WIN32
    #include <sys/stat.h>
#endif

#ifdef UNIX
#include <unistd.h>
#include <dirent.h>
#endif

#include <stdlib.h>

#include "errors.h"

#include "m_alloc.h"
#include "doomdef.h"
#include "gstrings.h"
#include "z_zone.h"
#include "w_wad.h"
#include "m_argv.h"
#include "m_fileio.h"
#include "c_console.h"
#include "i_system.h"
#include "g_game.h"
#include "p_setup.h"
#include "r_local.h"
#include "r_main.h"
#include "d_main.h"
#include "d_dehacked.h"
#include "s_sound.h"
#include "gi.h"
#include "w_ident.h"

#ifdef GEKKO
#include "i_wii.h"
#endif

#ifdef _XBOX
#include "i_xbox.h"
#endif

EXTERN_CVAR (waddirs)
EXTERN_CVAR (cl_waddownloaddir)

std::vector<std::string> wadfiles, wadhashes;		// [RH] remove limit on # of loaded wads
std::vector<std::string> patchfiles, patchhashes;	// [RH] remove limit on # of loaded wads
std::vector<std::string> missingfiles, missinghashes;

bool lastWadRebootSuccess = true;
extern bool step_mode;

bool capfps = true;
float maxfps = 35.0f;


#if defined(_WIN32) && !defined(_XBOX)

#define arrlen(array) (sizeof(array) / sizeof(*array))

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
#endif

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


static char *GetRegistryString(registry_value_t *reg_val)
{
	HKEY key = 0;
	DWORD len = 0;
	DWORD valtype = 0;
	char* result = 0;

	// Open the key (directory where the value is stored)

	if (RegOpenKeyEx(reg_val->root, reg_val->path,
	    0, KEY_READ, &key) != ERROR_SUCCESS)
	{
		return NULL;
	}

	result = NULL;

	// Find the type and length of the string, and only accept strings.

	if (RegQueryValueEx(key, reg_val->value,
	    NULL, &valtype, NULL, &len) == ERROR_SUCCESS && valtype == REG_SZ)
	{
		// Allocate a buffer for the value and read the value
		result = static_cast<char*>(malloc(len));

		if (RegQueryValueEx(key, reg_val->value, NULL, &valtype,
		    (unsigned char *)result, &len) != ERROR_SUCCESS)
		{
			free(result);
			result = NULL;
		}
	}

	// Close the key

	RegCloseKey(key);

	return result;
}

#endif

//
// BaseFileSearchDir
// denis - Check single paths for a given file with a possible extension
// Case insensitive, but returns actual file name
//
static std::string BaseFileSearchDir(std::string dir, const std::string &file, const std::string &ext, std::string hash = "")
{
	std::string found;

	if (dir[dir.length() - 1] != PATHSEPCHAR)
		dir += PATHSEP;

	hash = StdStringToUpper(hash);
	std::string dothash;
	if (!hash.empty())
		dothash = "." + hash;

	// denis - list files in the directory of interest, case-desensitize
	// then see if wanted wad is listed
#ifdef UNIX
	struct dirent **namelist = 0;
	int n = scandir(dir.c_str(), &namelist, 0, alphasort);

	for (int i = 0; i < n && namelist[i]; i++)
	{
		std::string d_name = namelist[i]->d_name;

		M_Free(namelist[i]);

		if (found.empty())
		{
			if (d_name == "." || d_name == "..")
				continue;

			std::string tmp = StdStringToUpper(d_name);

			if (file == tmp || (file + ext) == tmp || (file + dothash) == tmp || (file + ext + dothash) == tmp)
			{
				std::string local_file(dir + d_name);
				std::string local_hash(wads.GetMD5Hash(local_file));

				if (hash.empty() || hash == local_hash)
				{
					found = d_name;
				}
				else if (!hash.empty())
				{
					Printf (PRINT_HIGH, "WAD at %s does not match required copy\n", local_file.c_str());
					Printf (PRINT_HIGH, "Local MD5: %s\n", local_hash.c_str());
					Printf (PRINT_HIGH, "Required MD5: %s\n\n", hash.c_str());
				}
			}
		}
	}

	M_Free(namelist);
#else
	std::string all_ext = dir + "*";
	//all_ext += ext;

	WIN32_FIND_DATA FindFileData;
	HANDLE hFind = FindFirstFile(all_ext.c_str(), &FindFileData);

	if (hFind == INVALID_HANDLE_VALUE)
		return "";

	do
	{
		if (FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			continue;

		std::string tmp = StdStringToUpper(FindFileData.cFileName);

		if (file == tmp || (file + ext) == tmp || (file + dothash) == tmp || (file + ext + dothash) == tmp)
		{
			std::string local_file(dir + FindFileData.cFileName);
			std::string local_hash(wads.GetMD5Hash(local_file));

			if (hash.empty() || hash == local_hash)
			{
				found = FindFileData.cFileName;
				break;
			}
			else if (!hash.empty())
			{
				Printf (PRINT_HIGH, "WAD at %s does not match required copy\n", local_file.c_str());
				Printf (PRINT_HIGH, "Local MD5: %s\n", local_hash.c_str());
				Printf (PRINT_HIGH, "Required MD5: %s\n\n", hash.c_str());
			}
		}
	} while (FindNextFile(hFind, &FindFileData));

	FindClose(hFind);
#endif

	return found;
}

//
// D_AddSearchDir
// denis - Split a new directory string using the separator and append results to the output
//
void D_AddSearchDir(std::vector<std::string> &dirs, const char *dir, const char separator)
{
	if(!dir)
		return;

	// search through dwd
	std::stringstream ss(dir);
	std::string segment;

	while(!ss.eof())
	{
		std::getline(ss, segment, separator);

		if(!segment.length())
			continue;

		FixPathSeparator(segment);
		I_ExpandHomeDir(segment);

		if(segment[segment.length() - 1] != PATHSEPCHAR)
			segment += PATHSEP;

		dirs.push_back(segment);
	}
}

// [AM] Add platform-sepcific search directories
static void D_AddPlatformSearchDirs(std::vector<std::string> &dirs)
{
	#if defined(_WIN32) && !defined(_XBOX)

	const char separator = ';';

	// Doom 95
	{
		unsigned int i;

		for (i = 0;i < arrlen(uninstall_values);++i)
		{
			char* val;
			char* path;
			char* unstr;

			val = GetRegistryString(&uninstall_values[i]);

			if (val == NULL)
				continue;

			unstr = strstr(val, uninstaller_string);

			if (unstr == NULL)
			{
				free(val);
			}
			else
			{
				path = unstr + strlen(uninstaller_string);

				const char* cpath = path;
				D_AddSearchDir(dirs, cpath, separator);
			}
		}
	}

	// Doom Collectors Edition
	{
		char* install_path;
		char* subpath;
		unsigned int i;

		install_path = GetRegistryString(&collectors_edition_value);

		if (install_path != NULL)
		{
			for (i = 0;i < arrlen(collectors_edition_subdirs);++i)
			{
				subpath = static_cast<char*>(malloc(strlen(install_path)
				                             + strlen(collectors_edition_subdirs[i])
				                             + 5));
				sprintf(subpath, "%s\\%s", install_path, collectors_edition_subdirs[i]);

				const char* csubpath = subpath;
				D_AddSearchDir(dirs, csubpath, separator);
			}

			free(install_path);
		}
	}

	// Doom on Steam
	{
		char* install_path;
		char* subpath;
		size_t i;

		install_path = GetRegistryString(&steam_install_location);

		if (install_path != NULL)
		{
			for (i = 0;i < arrlen(steam_install_subdirs);++i)
			{
				subpath = static_cast<char*>(malloc(strlen(install_path)
				                             + strlen(steam_install_subdirs[i]) + 5));
				sprintf(subpath, "%s\\%s", install_path, steam_install_subdirs[i]);

				const char* csubpath = subpath;
				D_AddSearchDir(dirs, csubpath, separator);
				
				free(subpath);
			}

			free(install_path);
		}
	}

	// DOS Doom via DEICE
	D_AddSearchDir(dirs, "\\doom2", separator);    // Doom II
	D_AddSearchDir(dirs, "\\plutonia", separator); // Final Doom
	D_AddSearchDir(dirs, "\\tnt", separator);
	D_AddSearchDir(dirs, "\\doom_se", separator);  // Ultimate Doom
	D_AddSearchDir(dirs, "\\doom", separator);     // Shareware / Registered Doom
	D_AddSearchDir(dirs, "\\dooms", separator);    // Shareware versions
	D_AddSearchDir(dirs, "\\doomsw", separator);

	#elif defined(UNIX)

	const char separator = ':';

	#if defined(INSTALL_PREFIX) && defined(INSTALL_DATADIR) 
	D_AddSearchDir(dirs, INSTALL_PREFIX "/" INSTALL_DATADIR "/odamex", separator);
	D_AddSearchDir(dirs, INSTALL_PREFIX "/" INSTALL_DATADIR "/games/odamex", separator);
	#endif

	D_AddSearchDir(dirs, "/usr/share/games/doom", separator);
	D_AddSearchDir(dirs, "/usr/local/share/games/doom", separator);
	D_AddSearchDir(dirs, "/usr/local/share/doom", separator);

	#endif
}

//
// BaseFileSearch
// denis - Check all paths of interest for a given file with a possible extension
//
static std::string BaseFileSearch(std::string file, std::string ext = "", std::string hash = "")
{
	#ifdef _WIN32
		// absolute path?
		if (file.find(':') != std::string::npos)
			return file;

		const char separator = ';';
	#else
		// absolute path?
		if (file[0] == PATHSEPCHAR || file[0] == '~')
			return file;

		const char separator = ':';
	#endif

    // [Russell] - Bit of a hack. (since BaseFileSearchDir should handle this)
    // return file if it contains a path already
	if (M_FileExists(file))
		return file;

	file = StdStringToUpper(file);
	ext = StdStringToUpper(ext);
	std::vector<std::string> dirs;

	dirs.push_back(startdir);
	dirs.push_back(progdir);

	D_AddSearchDir(dirs, Args.CheckValue("-waddir"), separator);
	D_AddSearchDir(dirs, getenv("DOOMWADDIR"), separator);
	D_AddSearchDir(dirs, getenv("DOOMWADPATH"), separator);
	D_AddSearchDir(dirs, getenv("HOME"), separator);

	//[cSc] Add cl_waddownloaddir as default path
	D_AddSearchDir(dirs, cl_waddownloaddir.cstring(), separator);

	// [AM] Search additional paths based on platform
	D_AddPlatformSearchDirs(dirs);

	D_AddSearchDir(dirs, waddirs.cstring(), separator);

	dirs.erase(std::unique(dirs.begin(), dirs.end()), dirs.end());

	for (size_t i = 0; i < dirs.size(); i++)
	{
		std::string found = BaseFileSearchDir(dirs[i], file, ext, hash);

		if (!found.empty())
		{
			std::string &dir = dirs[i];

			if (dir[dir.length() - 1] != PATHSEPCHAR)
				dir += PATHSEP;

			return dir + found;
		}
	}

	// Not found
	return "";
}


//
// D_GetTitleString
//
// Returns the proper name of the game currently loaded into gameinfo & gamemission
//
std::string D_GetTitleString()
{
	if (gamemission == pack_tnt)
		return "DOOM 2: TNT - Evilution";
	if (gamemission == pack_plut)
		return "DOOM 2: Plutonia Experiment";
	if (gamemission == chex)
		return "Chex Quest";
	if (gamemission == retail_freedoom)
		return "Ultimate FreeDoom";
	if (gamemission == commercial_freedoom)
		return "FreeDoom";
	if (gamemission == commercial_hacx)
		return "HACX";

	return gameinfo.titleString;
}


//
// D_PrintIWADIdentity
//
static void D_PrintIWADIdentity()
{
	if (clientside)
	{
		Printf(PRINT_HIGH, "\n\35\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36"
    	                   "\36\36\36\36\36\36\36\36\36\36\36\36\37\n");

		if (gamemode == undetermined)
			Printf_Bold("Game mode indeterminate, no standard wad found.\n\n");
		else
			Printf_Bold("%s\n\n", D_GetTitleString().c_str());
	}
	else
	{
		if (gamemode == undetermined)
			Printf(PRINT_HIGH, "Game mode indeterminate, no standard wad found.\n");
		else 
			Printf(PRINT_HIGH, "%s\n", D_GetTitleString().c_str()); 
	}
}


//
// D_DoDefDehackedPatch
//
// [Russell] - Change the meaning, this will load multiple patch files if
//             specified
void D_DoDefDehackedPatch(const std::vector<std::string> &newpatchfiles)
{
	bool use_default = true;

	if (!newpatchfiles.empty())
	{
		for (size_t i = 0; i < newpatchfiles.size(); i++)
		{
			std::string ext;

			if (M_ExtractFileExtension(newpatchfiles[i], ext))
			{
				std::string f = BaseFileSearch(newpatchfiles[i], ext);
				if (f.length())
				{
					if (DoDehPatch(f.c_str(), false))
					{
						std::string Filename;
						M_ExtractFileName(f, Filename);
						patchfiles.push_back(Filename);
					}

					use_default = false;
				}
			}
		}
	}

    // try default patches
    if (use_default)
        DoDehPatch(NULL, true);		// See if there's a patch in a PWAD

	for (size_t i = 0; i < patchfiles.size(); i++)
		patchhashes.push_back(wads.GetMD5Hash(patchfiles[i]));

	// check for ChexQuest
	bool chexLoaded = false;
	for (size_t i = 0; i < patchfiles.size(); i++)
	{
		std::string base_filename;
		M_ExtractFileName(patchfiles[i], base_filename);
		if (iequals(base_filename, "chex.deh"))
			chexLoaded = true;
	}

	if (gamemode == retail_chex && !multiplayer && !chexLoaded)
		Printf(PRINT_HIGH,"Warning: chex.deh not loaded, experience may differ from the original!\n");
}


//
// D_CleanseFileName
//
// Strips a file name of path information and transforms it into uppercase
//
std::string D_CleanseFileName(const std::string &filename, const std::string &ext)
{
	std::string newname(filename);

	FixPathSeparator(newname);
	if (ext.length())
		M_AppendExtension(newname, "." + ext);

	size_t slash = newname.find_last_of(PATHSEPCHAR);

	if (slash != std::string::npos)
		newname = newname.substr(slash + 1, newname.length() - slash);

	std::transform(newname.begin(), newname.end(), newname.begin(), toupper);

	return newname;
}


//
// D_FindResourceFile
//
// Searches for a given file name, and returns the full path to the file if
// found and matches the supplied hash (or the hash was empty). An empty
// string is returned if no matching file is found.
//
static std::string D_FindResourceFile(const std::string& filename, const std::string& hash = "")
{
	// was a path to the file supplied?
	std::string dir;
	M_ExtractFilePath(filename, dir);

	std::string ext;
	M_ExtractFileExtension(filename, ext);
	if (!ext.empty() && ext[0] != '.')
		ext = "." + ext;

	std::string base_filename = D_CleanseFileName(filename);
	if (base_filename.empty())
		return std::string();

	std::string full_filename;

	// is there an exact match for the filename & hash?
	if (dir.empty())
		full_filename = BaseFileSearch(base_filename, ext, hash);
	else
	{
		std::string found = BaseFileSearchDir(dir, base_filename, ext, hash);
		if (!found.empty())
		{
			if (dir[dir.length() - 1] != PATHSEPCHAR)
				dir += PATHSEP;
			full_filename = dir + found;
		}
	}

	if (!full_filename.empty())
		return full_filename;

	return std::string();
}


//
// D_FindIWAD
//
// Tries to find an IWAD from a set of known IWAD file names.
//
static std::string D_FindIWAD(const std::string& suggestion = "")
{
	if (!suggestion.empty())
	{
		std::string full_filename = D_FindResourceFile(suggestion);
		if (!full_filename.empty() && W_IsIWAD(full_filename))
			return full_filename;
	}

	// Search for a pre-defined IWAD from the list above
	std::vector<OString> filenames = W_GetIWADFilenames();
	for (size_t i = 0; i < filenames.size(); i++)
	{
		std::string full_filename = D_FindResourceFile(filenames[i]);
		if (!full_filename.empty() && W_IsIWAD(full_filename))
			return full_filename;
	}

	return std::string();
}


//
// D_AddResourceFile
//
// Searches for a given filename and if found, the full filepath is added
// to the wadfiles vector. If the filename is not found, the filename is
// added to the missingfiles vector instead.
//
static bool D_AddResourceFile(const std::string& filename, const std::string& hash = "")
{
	std::string full_filename = D_FindResourceFile(filename, hash);
	if (!full_filename.empty())
	{
		wadfiles.push_back(full_filename);
		return true;
	}
	else
	{
		Printf(PRINT_HIGH, "could not find resource file: %s\n", filename.c_str());
		missingfiles.push_back(filename);
		if (!hash.empty())
			missinghashes.push_back(hash);
		return false;
	}
}


//
// D_LoadResourceFiles
//
// Performs the grunt work of loading WAD and DEH/BEX files.
// The global wadfiles and patchfiles vectors are filled with the list
// of loaded filenames and the missingfiles vector is also filled if
// applicable.
//
void D_LoadResourceFiles(
	const std::vector<std::string> &newwadfiles,
	const std::vector<std::string> &newpatchfiles,
	const std::vector<std::string> &newwadhashes,
	const std::vector<std::string> &newpatchhashes
)
{
	bool hashcheck = (newwadfiles.size() == newwadhashes.size());
	bool iwad_provided = false;

	// save the iwad filename & hash from the currently loaded resource file list
	std::string iwad_filename = wadfiles.size() >= 2 ? wadfiles[1] : "";
	std::string iwad_hash = wadhashes.size() >= 2 ? wadhashes[1] : "";

	wadfiles.clear();
	patchfiles.clear();
	missingfiles.clear();
	missinghashes.clear();

	// Provided an IWAD filename?
	if (newwadfiles.size() >= 1)
	{
		std::string hash = hashcheck ? newwadhashes[0] : "";
		std::string full_filename = D_FindResourceFile(newwadfiles[0], hash);
		if (W_IsIWAD(full_filename))
		{
			iwad_provided = true;
			iwad_filename = full_filename;
			iwad_hash = hash;
		}
	}

	// Not provided an IWAD filename and an IWAD is not currently loaded?
	// Try to find *any* IWAD using D_FindIWAD.
	if (iwad_filename.empty())
	{
		iwad_filename = D_FindIWAD();
		iwad_hash.clear();
	}

	if (!D_AddResourceFile("odamex.wad"))
		I_FatalError("Cannot find odamex.wad");
	if (iwad_filename.empty() || !D_AddResourceFile(iwad_filename, iwad_hash))
		I_FatalError("Cannot find IWAD (try -waddir)");

	for (size_t i = iwad_provided ? 1 : 0; i < newwadfiles.size(); i++)
		D_AddResourceFile(newwadfiles[i], hashcheck ? newwadhashes[i] : "");

	// Now scan the contents of the IWAD to determine which one it is
	W_ConfigureGameInfo(iwad_filename);

	// print info about the IWAD to the console
	D_PrintIWADIdentity();

	// set the window title based on which IWAD we're using
	I_SetTitleString(D_GetTitleString().c_str());

	modifiedgame = (wadfiles.size() > 2) || !newpatchfiles.empty();	// more than odamex.wad and IWAD?
	if (modifiedgame && (gameinfo.flags & GI_SHAREWARE))
		I_FatalError("\nYou cannot load additional WADs with the shareware version. Register!");

	wadhashes = wads.InitMultipleFiles(wadfiles);

	// [RH] Initialize localizable strings.
	// [SL] It is necessary to load the strings here since a dehacked patch
	// might change the strings
	GStrings.FreeData();
	GStrings.LoadStrings(wads.GetNumForName("LANGUAGE"), STRING_TABLE_SIZE, false);
	GStrings.Compact();

	D_DoDefDehackedPatch(newpatchfiles);
}


//
// D_DoomWadReboot
// [denis] change wads at runtime
// Returns false if there are missing files and fills the missingfiles
// vector
//
// [SL] passing an IWAD as newwadfiles[0] is now optional
// TODO: hash checking for patchfiles
//
bool D_DoomWadReboot(
	const std::vector<std::string> &newwadfiles,
	const std::vector<std::string> &newpatchfiles,
	const std::vector<std::string> &newwadhashes,
	const std::vector<std::string> &newpatchhashes
)
{
	// already loaded these?
	if (lastWadRebootSuccess &&	!wadhashes.empty() &&
		newwadhashes == std::vector<std::string>(wadhashes.begin()+1, wadhashes.end()))
	{
		// fast track if files have not been changed // denis - todo - actually check the file timestamps
		Printf (PRINT_HIGH, "Currently loaded WADs match server checksum\n\n");
		return true;
	}

	lastWadRebootSuccess = false;

	D_Shutdown();

	gamestate_t oldgamestate = gamestate;
	gamestate = GS_STARTUP; // prevent console from trying to use nonexistant font

	// Load all the WAD and DEH/BEX files
	D_LoadResourceFiles(newwadfiles, newpatchfiles, newwadhashes, newpatchhashes); 

	// get skill / episode / map from parms
	strcpy(startmap, (gameinfo.flags & GI_MAPxx) ? "MAP01" : "E1M1");

	D_Init();

	// preserve state
	lastWadRebootSuccess = missingfiles.empty();

	gamestate = oldgamestate; // GS_STARTUP would prevent netcode connecting properly

	return missingfiles.empty();
}


//
// D_AddCommandLineOptionFiles
//
// Adds the full path of all the file names following the given command line
// option parameter (eg, "-file") matching the specified extension to the
// filenames vector.
//
static void D_AddCommandLineOptionFiles(
	std::vector<std::string>& filenames,
	const std::string& option, const std::string& ext)
{
	DArgs files = Args.GatherFiles(option.c_str(), ext.c_str(), true);
	for (size_t i = 0; i < files.NumArgs(); i++)
	{
		std::string filename(files.GetArg(i));
		M_AppendExtension(filename, ext, true);
		filenames.push_back(filename);
	}

	files.FlushArgs();
}

//
// D_AddWadCommandLineFiles
//
// Add the WAD files specified with -file.
// Call this from D_DoomMain
//
void D_AddWadCommandLineFiles(std::vector<std::string>& filenames)
{
	D_AddCommandLineOptionFiles(filenames, "-file", ".WAD");
}

//
// D_AddDehCommandLineFiles
//
// Adds the DEH/BEX files specified with -deh.
// Call this from D_DoomMain
//
void D_AddDehCommandLineFiles(std::vector<std::string>& filenames)
{
	D_AddCommandLineOptionFiles(filenames, "-deh", ".DEH");
	D_AddCommandLineOptionFiles(filenames, "-deh", ".BEX");
}


// ============================================================================
//
// TaskScheduler class
//
// ============================================================================
//
// Attempts to schedule a task (indicated by the function pointer passed to
// the concrete constructor) at a specified interval. For uncapped rates, that
// interval is simply as often as possible. For capped rates, the interval is
// specified by the rate parameter.
//

class TaskScheduler
{
public:
	virtual ~TaskScheduler() { }
	virtual void run() = 0;
	virtual dtime_t getNextTime() const = 0;
	virtual float getRemainder() const = 0;
};

class UncappedTaskScheduler : public TaskScheduler
{
public:
	UncappedTaskScheduler(void (*task)()) :
		mTask(task)
	{ }

	virtual ~UncappedTaskScheduler() { }

	virtual void run()
	{
		mTask();
	}

	virtual dtime_t getNextTime() const
	{
		return I_GetTime();
	}

	virtual float getRemainder() const
	{
		return 0.0f;
	}

private:
	void				(*mTask)();
};

class CappedTaskScheduler : public TaskScheduler
{
public:
	CappedTaskScheduler(void (*task)(), float rate, int max_count) :
		mTask(task), mMaxCount(max_count),
		mFrameDuration(I_ConvertTimeFromMs(1000) / rate),
		mAccumulator(mFrameDuration),
		mPreviousFrameStartTime(I_GetTime())
	{
	}

	virtual ~CappedTaskScheduler() { }

	virtual void run()
	{
		mFrameStartTime = I_GetTime();
		mAccumulator += mFrameStartTime - mPreviousFrameStartTime;
		mPreviousFrameStartTime = mFrameStartTime;

		int count = mMaxCount;

		while (mAccumulator >= mFrameDuration && count--)
		{
			mTask();
			mAccumulator -= mFrameDuration;
		}
	}

	virtual dtime_t getNextTime() const
	{
		return mFrameStartTime + mFrameDuration - mAccumulator;
	}

	virtual float getRemainder() const
	{
		// mAccumulator can be greater than mFrameDuration so only get the
		// time remaining until the next frame
		dtime_t remaining_time = mAccumulator % mFrameDuration;
		return (float)(double(remaining_time) / mFrameDuration);
	}

private:
	void				(*mTask)();
	const int			mMaxCount;
	const dtime_t		mFrameDuration;
	dtime_t				mAccumulator;
	dtime_t				mFrameStartTime;
	dtime_t				mPreviousFrameStartTime;
};

static TaskScheduler* simulation_scheduler;
static TaskScheduler* display_scheduler;

//
// D_InitTaskSchedulers
//
// Checks for external changes to the rate for the simulation and display
// tasks and instantiates the appropriate TaskSchedulers.
//
static void D_InitTaskSchedulers(void (*sim_func)(), void(*display_func)())
{
	bool capped_simulation = !timingdemo;
	bool capped_display = !timingdemo && capfps;

	static bool previous_capped_simulation = !capped_simulation;
	static bool previous_capped_display = !capped_display;
	static float previous_maxfps = -1.0f;

	if (capped_simulation != previous_capped_simulation)
	{
		previous_capped_simulation = capped_simulation;

		delete simulation_scheduler;

		if (capped_simulation)
			simulation_scheduler = new CappedTaskScheduler(sim_func, TICRATE, 4);
		else
			simulation_scheduler = new UncappedTaskScheduler(sim_func);
	}

	if (capped_display != previous_capped_display || maxfps != previous_maxfps)
	{
		previous_capped_display = capped_display;
		previous_maxfps = maxfps;

		delete display_scheduler;

		if (capped_display)
			display_scheduler = new CappedTaskScheduler(display_func, maxfps, 1);
		else
			display_scheduler = new UncappedTaskScheduler(display_func);
	}
}

void STACK_ARGS D_ClearTaskSchedulers()
{
	delete simulation_scheduler;
	delete display_scheduler;
	simulation_scheduler = NULL;
	display_scheduler = NULL;
}

//
// D_RunTics
//
// The core of the main game loop.
// This loop allows the game simulation timing to be decoupled from the display
// timing. If the the user selects a capped framerate and isn't using the
// -timedemo parameter, both the simulation and display functions will be called
// TICRATE times a second. If the framerate is uncapped, the simulation function
// will still be called TICRATE times a second but the display function will
// be called as often as possible. After each iteration through the loop,
// the program yields briefly to the operating system.
//
void D_RunTics(void (*sim_func)(), void(*display_func)())
{
	D_InitTaskSchedulers(sim_func, display_func);

	simulation_scheduler->run();

#ifdef CLIENT_APP
	// Use linear interpolation for rendering entities if the display
	// framerate is not synced with the simulation frequency.
	// Ch0wW : if you experience a spinning effect while trying to pause the frame, 
	// don't forget to add your condition here.
	if ((maxfps == TICRATE && capfps)
		|| timingdemo || paused || step_mode
		|| ((menuactive || ConsoleState == c_down || ConsoleState == c_falling) && !network_game && !demoplayback))
		render_lerp_amount = FRACUNIT;
	else
		render_lerp_amount = simulation_scheduler->getRemainder() * FRACUNIT;
#endif

	display_scheduler->run();

	if (timingdemo)
		return;

	// Sleep until the next scheduled task.
	dtime_t simulation_wake_time = simulation_scheduler->getNextTime();
	dtime_t display_wake_time = display_scheduler->getNextTime();

	do
	{
		I_Yield();
	} while (I_GetTime() < MIN(simulation_wake_time, display_wake_time));			
}

VERSION_CONTROL (d_main_cpp, "$Id$")
