// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: d_main.cpp 3426 2012-11-19 17:25:28Z dr_sean $
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 2006-2013 by The Odamex Team.
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

#ifdef GEKKO
#include "i_wii.h"
#endif

#ifdef _XBOX
#include "i_xbox.h"
#endif

EXTERN_CVAR (waddirs)

std::vector<std::string> wadfiles, wadhashes;		// [RH] remove limit on # of loaded wads
std::vector<std::string> patchfiles, patchhashes;	// [RH] remove limit on # of loaded wads
std::vector<std::string> missingfiles, missinghashes;

extern gameinfo_t SharewareGameInfo;
extern gameinfo_t RegisteredGameInfo;
extern gameinfo_t RetailGameInfo;
extern gameinfo_t CommercialGameInfo;
extern gameinfo_t RetailBFGGameInfo;
extern gameinfo_t CommercialBFGGameInfo;

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
	HKEY key;
	DWORD len;
	DWORD valtype;
	char* result;

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
static std::string BaseFileSearchDir(std::string dir, std::string file, std::string ext, std::string hash = "")
{
	std::string found;

	if(dir[dir.length() - 1] != PATHSEPCHAR)
		dir += PATHSEP;

	std::transform(hash.begin(), hash.end(), hash.begin(), toupper);
	std::string dothash = ".";
	if(hash.length())
		dothash += hash;
	else
		dothash = "";

	// denis - list files in the directory of interest, case-desensitize
	// then see if wanted wad is listed
#ifdef UNIX
	struct dirent **namelist = 0;
	int n = scandir(dir.c_str(), &namelist, 0, alphasort);

	for(int i = 0; i < n && namelist[i]; i++)
	{
		std::string d_name = namelist[i]->d_name;

		M_Free(namelist[i]);

		if(!found.length())
		{
			if(d_name == "." || d_name == "..")
				continue;

			std::string tmp = d_name;
			std::transform(tmp.begin(), tmp.end(), tmp.begin(), toupper);

			if(file == tmp || (file + ext) == tmp || (file + dothash) == tmp || (file + ext + dothash) == tmp)
			{
				std::string local_file = (dir + d_name).c_str();
				std::string local_hash = W_MD5(local_file);

				if(!hash.length() || hash == local_hash)
				{
					found = d_name;
				}
				else if(hash.length())
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
		if(FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			continue;

		std::string tmp = FindFileData.cFileName;
		std::transform(tmp.begin(), tmp.end(), tmp.begin(), toupper);

		if(file == tmp || (file + ext) == tmp || (file + dothash) == tmp || (file + ext + dothash) == tmp)
		{
			std::string local_file = (dir + FindFileData.cFileName).c_str();
			std::string local_hash = W_MD5(local_file);

			if(!hash.length() || hash == local_hash)
			{
				found = FindFileData.cFileName;
				break;
			}
			else if(hash.length())
			{
				Printf (PRINT_HIGH, "WAD at %s does not match required copy\n", local_file.c_str());
				Printf (PRINT_HIGH, "Local MD5: %s\n", local_hash.c_str());
				Printf (PRINT_HIGH, "Required MD5: %s\n\n", hash.c_str());
			}
		}
	} while(FindNextFile(hFind, &FindFileData));

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

	#ifdef INSTALL_PREFIX
	D_AddSearchDir(dirs, INSTALL_PREFIX "/share/odamex", separator);
	D_AddSearchDir(dirs, INSTALL_PREFIX "/share/games/odamex", separator);
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
		if(file.find(':') != std::string::npos)
			return file;

		const char separator = ';';
	#else
		// absolute path?
		if(file[0] == PATHSEPCHAR || file[0] == '~')
			return file;

		const char separator = ':';
	#endif

    // [Russell] - Bit of a hack. (since BaseFileSearchDir should handle this)
    // return file if it contains a path already
	if (M_FileExists(file))
		return file;

	std::transform(file.begin(), file.end(), file.begin(), toupper);
	std::transform(ext.begin(), ext.end(), ext.begin(), toupper);
	std::vector<std::string> dirs;

	dirs.push_back(startdir);
	dirs.push_back(progdir);

	D_AddSearchDir(dirs, Args.CheckValue("-waddir"), separator);
	D_AddSearchDir(dirs, getenv("DOOMWADDIR"), separator);
	D_AddSearchDir(dirs, getenv("DOOMWADPATH"), separator);
	D_AddSearchDir(dirs, getenv("HOME"), separator);

	// [AM] Search additional paths based on platform
	D_AddPlatformSearchDirs(dirs);

	D_AddSearchDir(dirs, waddirs.cstring(), separator);

	dirs.erase(std::unique(dirs.begin(), dirs.end()), dirs.end());

	for(size_t i = 0; i < dirs.size(); i++)
	{
		std::string found = BaseFileSearchDir(dirs[i], file, ext, hash);

		if(found.length())
		{
			std::string &dir = dirs[i];

			if(dir[dir.length() - 1] != PATHSEPCHAR)
				dir += PATHSEP;

			return dir + found;
		}
	}

	// Not found
	return "";
}


//
// D_ConfigureGameInfo
//
// Opens the specified file name and sets gameinfo, gamemission, and gamemode
// to the appropriate values for the IWAD file.
//
// gamemode will be set to undetermined if the file is not a valid IWAD.
//
static void D_ConfigureGameInfo(const std::string& iwad_filename)
{
	gamemode = undetermined;

	static const int NUM_CHECKLUMPS = 10;
	static const char checklumps[NUM_CHECKLUMPS][8] = {
		"E1M1", "E2M1", "E4M1", "MAP01",
		{ 'A','N','I','M','D','E','F','S'},
		"FINAL2", "REDTNT2", "CAMO1",
		{ 'E','X','T','E','N','D','E','D'},
		{ 'D','M','E','N','U','P','I','C'}
	};

	int lumpsfound[NUM_CHECKLUMPS];
	wadinfo_t header;
	FILE *f;

	memset(lumpsfound, 0, sizeof(lumpsfound));
	if ( (f = fopen(iwad_filename.c_str(), "rb")) )
	{
		fread(&header, sizeof(header), 1, f);
		header.identification = LELONG(header.identification);
		if (header.identification == IWAD_ID)
		{
			header.numlumps = LELONG(header.numlumps);
			if (0 == fseek(f, LELONG(header.infotableofs), SEEK_SET))
			{
				for (int i = 0; i < header.numlumps; i++)
				{
					filelump_t lump;

					if (0 == fread(&lump, sizeof(lump), 1, f))
						break;
					for (int j = 0; j < NUM_CHECKLUMPS; j++)
						if (!strnicmp(lump.name, checklumps[j], 8))
							lumpsfound[j]++;
				}
			}
		}
		fclose(f);
	}

	if (lumpsfound[3])
	{
		if (lumpsfound[9])
		{
			gameinfo = CommercialBFGGameInfo;
			gamemode = commercial_bfg;
		}
		else
		{
			gameinfo = CommercialGameInfo;
			gamemode = commercial;
		}

		if (lumpsfound[6])
			gamemission = pack_tnt;
		else if (lumpsfound[7])
			gamemission = pack_plut;
		else
			gamemission = doom2;
	}
	else if (lumpsfound[0])
	{
		gamemission = doom;
		if (lumpsfound[1])
		{
			if (lumpsfound[2])
			{
				// [ML] 1/7/10: HACK - There's no unique lumps in the chex quest
				// iwad.  It's ultimate doom with their stuff replacing most things.
				std::string base_filename;
				M_ExtractFileName(iwad_filename, base_filename);
				if (iequals(base_filename, "chex.wad"))
				{
					gamemission = chex;
					gamemode = retail_chex;
					gameinfo = RetailGameInfo;
				}
				else
				{
					gamemode = retail;

					if (lumpsfound[9])
						gameinfo = RetailBFGGameInfo;
					else
						gameinfo = RetailGameInfo;
				}
			}
			else
			{
				gamemode = registered;
				gameinfo = RegisteredGameInfo;
			}
		}
		else
		{
			gamemode = shareware;
			gameinfo = SharewareGameInfo;
		}
	}

	if (gamemode == undetermined)
		gameinfo = SharewareGameInfo;
}


//
// D_GetTitleString
//
// Returns the proper name of the game currently loaded into gameinfo & gamemission
//
static std::string D_GetTitleString()
{
	if (gamemission == pack_tnt)
		return "DOOM 2: TNT - Evilution";
	if (gamemission == pack_plut)
		return "DOOM 2: Plutonia Experiment";
	if (gamemission == chex)
		return "Chex Quest";

	return gameinfo.titleString;
}


//
// D_CheckIWAD
//
// Tries to find an IWAD from a set of know IWAD names, and checks the first
// one found's contents to determine whether registered/commercial features
// should be executed (notably loading PWAD's).
//
static std::string D_CheckIWAD(const std::string& suggestion)
{
	std::string iwad;
	std::string iwad_file;

	if (!suggestion.empty())
	{
		std::string found = BaseFileSearch(suggestion, ".WAD");

		if (!found.empty())
			iwad = found;
		else
		{
			if (M_FileExists(suggestion))
				iwad = suggestion;
		}
	}

	if (iwad.empty())
	{
		// Search for a pre-defined IWAD from the list above
		for (int i = 0; !doomwadnames[i].name.empty(); i++)
		{
			std::string found = BaseFileSearch(doomwadnames[i].name);

			if (!found.empty())
			{
				iwad = found;
				break;
			}
		}
	}

	// Now scan the contents of the IWAD to determine which one it is
	D_ConfigureGameInfo(iwad);

	return iwad;
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
		patchhashes.push_back(W_MD5(patchfiles[i]));

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
// D_VerifyFile
//
// Searches for a given file name, setting base_filename and full_filename
// to the full path of the file. Returns true if the file was found and it
// matched the supplied hash (or has was empty).
//
static bool D_VerifyFile(
		const std::string &filename,
		std::string &base_filename,
		std::string &full_filename,
		const std::string &hash = "")
{
	base_filename.clear();
	full_filename.clear();

	std::string ext;
	M_ExtractFileExtension(filename, ext);

	base_filename = D_CleanseFileName(filename);
	if (base_filename.empty())
		return false;

	// was a path to the file supplied?
	std::string dir;
	M_ExtractFilePath(filename, dir);
	if (!dir.empty())
	{
		std::string found = BaseFileSearchDir(dir, base_filename, ext, hash);
		if (!found.empty())
		{
			if (dir[dir.length() - 1] != PATHSEPCHAR)
				dir += PATHSEP;
			full_filename = dir + found;
			return true;
		}
	}

	// is there an exact match for the filename and hash in WADDIRS?
	full_filename = BaseFileSearch(base_filename, "." + ext, hash);
	if (!full_filename.empty())
		return true;

	// is there a file with matching name even if the hash is incorrect in WADDIRS?
	full_filename = BaseFileSearch(base_filename, "." + ext);
	if (full_filename.empty())
		return false;

	// if it's an IWAD, check if we have a valid alternative hash
	std::string found_hash = W_MD5(full_filename);
	if (W_IsIWAD(base_filename, found_hash))
		return true;

	return false;
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

	missingfiles.clear();
	missinghashes.clear();

	// [SL] 2012-12-06 - If we weren't provided with a new IWAD filename in
	// newwadfiles, use the previous IWAD.
	std::string iwad_filename, iwad_hash;
	if ((newwadfiles.empty() || !W_IsIWAD(newwadfiles[0])) && (wadfiles.size() >= 2))
	{
		iwad_filename = wadfiles[1];
		iwad_hash = wadhashes[1];
	}
	else if (!newwadfiles.empty())
	{
		iwad_provided = true;
		iwad_filename = newwadfiles[0];
		iwad_hash = hashcheck ? newwadhashes[0] : "";
	}

	wadfiles.clear();
    patchfiles.clear();

	// add ODAMEX.WAD to wadfiles	
	std::string odamex_filename = BaseFileSearch("odamex.wad");
	if (odamex_filename.empty())
		I_FatalError("Cannot find odamex.wad");
	wadfiles.push_back(odamex_filename);

	// add the IWAD to wadfiles
	std::string titlestring;
	iwad_filename = D_CheckIWAD(iwad_filename);
	if (iwad_filename.empty())
		I_Error("Cannot find IWAD (try -waddir)");

	wadfiles.push_back(iwad_filename);

	// print info about the IWAD to the console
	D_PrintIWADIdentity();

	// set the window title based on which IWAD we're using
	I_SetTitleString(D_GetTitleString().c_str());

	// check if the wad files exist and if they match the MD5SUM
	std::string base_filename, full_filename;

	if (!D_VerifyFile(iwad_filename, base_filename, full_filename, iwad_hash))
	{
		Printf(PRINT_HIGH, "could not find WAD: %s\n", base_filename.c_str());
		missingfiles.push_back(base_filename);
		if (hashcheck)
			missinghashes.push_back(iwad_hash);
	}

	for (size_t i = 0; i < newwadfiles.size(); i++)
	{
		std::string hash = hashcheck ? newwadhashes[i] : "";

		// already added the IWAD 
		if (i == 0 && iwad_provided)
			continue;

		if (D_VerifyFile(newwadfiles[i], base_filename, full_filename, hash))
			wadfiles.push_back(full_filename);
		else
		{
			Printf(PRINT_HIGH, "could not find WAD: %s\n", base_filename.c_str());
			missingfiles.push_back(base_filename);
			if (hashcheck)
				missinghashes.push_back(newwadhashes[i]);
		}
	}

	modifiedgame = (wadfiles.size() > 2) || !newpatchfiles.empty();	// more than odamex.wad and IWAD?
	if (modifiedgame && (gameinfo.flags & GI_SHAREWARE))
		I_Error("\nYou cannot load additional WADs with the shareware version. Register!");

	wadhashes = W_InitMultipleFiles(wadfiles);

	D_DoDefDehackedPatch(newpatchfiles);
}

void D_NewWadInit();


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

	if (gamestate == GS_LEVEL)
		G_ExitLevel(0, 0);

	S_Stop();

	DThinker::DestroyAllThinkers();

	// Close all open WAD files
	W_Close();

	// [ML] 9/11/10: Reset custom wad level information from MAPINFO et al.
	for (size_t i = 0; i < wadlevelinfos.size(); i++)
	{
		if (wadlevelinfos[i].snapshot)
		{
			delete wadlevelinfos[i].snapshot;
			wadlevelinfos[i].snapshot = NULL;
		}
	}

	wadlevelinfos.clear();
	wadclusterinfos.clear();

	UndoDehPatch();

	// Restart the memory manager
	Z_Init();

	SetLanguageIDs ();

	gamestate_t oldgamestate = gamestate;
	gamestate = GS_STARTUP; // prevent console from trying to use nonexistant font

	// Load all the WAD and DEH/BEX files
	D_LoadResourceFiles(newwadfiles, newpatchfiles, newwadhashes, newpatchhashes); 

	// get skill / episode / map from parms
	strcpy(startmap, (gameinfo.flags & GI_MAPxx) ? "MAP01" : "E1M1");

	// [RH] Initialize localizable strings.
	GStrings.ResetStrings ();
	GStrings.Compact ();

	D_NewWadInit();

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
		std::string filename = BaseFileSearch(files.GetArg(i), ext);
		if (!filename.empty())
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

//
// D_RunTics
//
// The core of the main game loop.
// This loop allows the game simulation timing to be decoupled from the renderer
// timing. If the the user selects a capped framerate and isn't using the
// -timedemo parameter, both the simulation and render functions will be called
// TICRATE times a second. If the framerate is uncapped, the simulation function
// will still be called TICRATE times a second but the render function will
// be called as often as possible. After each iteration through the loop,
// the program yields briefly to the operating system.
//
void D_RunTics(void (*sim_func)(), void(*render_func)())
{
	static const uint64_t sim_dt = I_ConvertTimeFromMs(1000) / TICRATE;

	static uint64_t previous_time = I_GetTime();
	uint64_t current_time = I_GetTime();

	// reset the rendering interpolation
	render_lerp_amount = FRACUNIT;

	static uint64_t accumulator = sim_dt;

	if (!timingdemo)	// run simulation function at 35Hz?
	{
		// Run upto 4 simulation frames. Limit the number because there's already a
		// slowdown and running more will only make things worse.
		accumulator += MIN(current_time - previous_time, 4 * sim_dt);

		// calculate how late the start of the frame is (for debugging)
		uint64_t late_time_ms = current_time - previous_time > sim_dt ?
			I_ConvertTimeToMs(current_time - previous_time - sim_dt) : 0;

		if (late_time_ms > 2)
			DPrintf("Warning: frame start is %ums late!\n", late_time_ms);

		while (accumulator >= sim_dt)
		{
			sim_func();
			accumulator -= sim_dt;
		}

		// Use linear interpolation for rendering entities if the renderer
		// framerate is not synced with the physics frequency.
		if (maxfps != TICRATE && !(paused || menuactive || step_mode))
			render_lerp_amount = (fixed_t)(accumulator * FRACUNIT / sim_dt);
	}
	else			// run simulation function as fast as possible
	{
		sim_func();
		accumulator = 0;
	}

	render_func();

	static float previous_maxfps = -1;

	if (!timingdemo && capfps)		// render at a capped framerate?
	{
		static uint64_t render_dt, previous_block;
		uint64_t current_block;

		// The capped framerate has changed so recalculate some stuff
		if (maxfps != previous_maxfps)
		{
			render_dt = I_ConvertTimeFromMs(1000) / maxfps;
			previous_block = current_time / render_dt;
		}

		// With capped framerates, frames are rendered within fixed blocks of time
		// and at the end of a frame, sleep until the start of the next block.
		do
			I_Yield();
		while ( (current_block = I_GetTime() / render_dt) <= previous_block);

		previous_block = current_block;
		previous_maxfps = maxfps;
	}
	else if (!timingdemo)			// render at an unlimited framerate (but still yield)
	{
		// sleep for 1ms to allow the operating system some time
		I_Yield();
		previous_maxfps = -1;
	}

	previous_time = current_time;
}

VERSION_CONTROL (d_main_cpp, "$Id: d_main.cpp 3426 2012-11-19 17:25:28Z dr_sean $")
