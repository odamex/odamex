// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 2006-2026 by The Odamex Team.
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


#include "odamex.h"


#include <sstream>
#include <algorithm>
#include <filesystem>
#include <fstream>

#include "win32inc.h"
#ifndef _WIN32
    #include <sys/stat.h>
#endif

#ifdef UNIX
#include <unistd.h>
#include <dirent.h>
#endif

#include <stdlib.h>

#include "m_alloc.h"
#include "gstrings.h"
#include "z_zone.h"
#include "w_wad.h"
#include "m_argv.h"
#include "m_fileio.h"
#include "m_menuconf.h"
#include "c_console.h"
#include "i_system.h"
#include "i_time.h"
#include "g_game.h"
#include "g_episode.h"
#include "g_spawninv.h"
#include "r_main.h"
#include "d_main.h"
#include "d_dehacked.h"
#include "s_sound.h"
#include "gi.h"
#include "w_ident.h"
#include "m_resfile.h"
#include "odainfo.h"
#include "infomap.h"

OResFiles wadfiles;
OResFiles patchfiles;
OWantFiles missingfiles;
bool missingCommercialIWAD = false;

bool lastWadRebootSuccess = true;
extern bool step_mode;

bool capfps = true;
float maxfps = 35.0f;

// Subdirs of a Steam library where IWADs are found.
static const char* steam_install_subdirs[] =
{
	"steamapps/common/Doom 2/base",
	"steamapps/common/Doom 2/masterbase/master/wads",
	"steamapps/common/Final Doom/base",
	"steamapps/common/Doom 2/finaldoombase",
	"steamapps/common/Ultimate Doom/base",
	"steamapps/common/DOOM 3 BFG Edition/base/wads",
	"steamapps/common/Master Levels of Doom/master/wads", // Let Odamex find the Master Levels pwads too
	"steamapps/common/Ultimate Doom/base/doom2", // 2024 Steam re-release additions here and below
	"steamapps/common/Ultimate Doom/base/master/wads",
	"steamapps/common/Ultimate Doom/base/plutonia",
	"steamapps/common/Ultimate Doom/base/tnt",
	"steamapps/common/Ultimate Doom/rerelease",
	"steamapps/common/Heretic + Hexen",
};

static void D_AddUniquePath(std::vector<std::string>& paths, const std::string& path)
{
	std::string cleanPath = M_CleanPath(path);
	if (std::find(paths.begin(), paths.end(), cleanPath) == paths.end())
		paths.push_back(cleanPath);
}

static bool D_IsExistingDir(const std::string& path)
{
	std::error_code ec;
	return std::filesystem::is_directory(std::filesystem::path(path), ec);
}

static std::vector<std::string> GetSteamLibraryPaths(const std::string& install_path)
{
	std::vector<std::string> paths;

	if (install_path.empty())
		return paths;

	D_AddUniquePath(paths, install_path);

	const std::filesystem::path vdfpath =
		std::filesystem::path(install_path) / "steamapps" / "libraryfolders.vdf";
	std::ifstream file(vdfpath);
	if (!file.is_open())
		return paths;

	std::string line;
	while (std::getline(file, line))
	{
		if (line.find("\"path\"") == std::string::npos)
			continue;

		const size_t value_start = line.find('"', line.find("\"path\"") + 6);
		if (value_start == std::string::npos)
			continue;
		const size_t value_end = line.find('"', value_start + 1);
		if (value_end == std::string::npos)
			continue;

		std::string path = line.substr(value_start + 1, value_end - value_start - 1);
		size_t pos = 0;
		while ((pos = path.find("\\\\", pos)) != std::string::npos)
		{
			path.replace(pos, 2, "\\");
			pos += 1;
		}

		D_AddUniquePath(paths, path);
	}

	return paths;
}

static void D_AddSteamSearchDirs(std::vector<std::string>& dirs,
                                 const std::vector<std::string>& steam_library_paths,
                                 const char separator)
{
	for (const auto& library : steam_library_paths)
	{
		for (const auto& dir : steam_install_subdirs)
		{
			const std::string subpath =
				(std::filesystem::path(library) / std::filesystem::path(dir)).string();

			D_AddSearchDir(dirs, subpath.c_str(), separator, missing_dir_policy::silent);
		}
	}
}

#if defined(_WIN32)

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

static registry_value_t gog_doom_plus_doom2 =
{
	HKEY_LOCAL_MACHINE,
	SOFTWARE_KEY "\\GOG.com\\Games\\1413291984",
	"path",
};

static registry_value_t gog_doom =
{
	HKEY_LOCAL_MACHINE,
	SOFTWARE_KEY "\\GOG.com\\Games\\1435827232",
	"path",
};

static registry_value_t gog_doom2 =
{
	HKEY_LOCAL_MACHINE,
	SOFTWARE_KEY "\\GOG.com\\Games\\1435848814",
	"path",
};

static registry_value_t gog_final_doom =
{
	HKEY_LOCAL_MACHINE,
	SOFTWARE_KEY "\\GOG.com\\Games\\1435848742",
	"path",
};

static registry_value_t gog_heretic_hexen =
{
	HKEY_LOCAL_MACHINE,
	SOFTWARE_KEY "\\GOG.com\\Games\\1776058590",
	"path",
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
			M_Free(result);
		}
	}

	// Close the key

	RegCloseKey(key);

	return result;
}

#endif

//
// D_InitializeDoomObjectTables()
// [CMB] Initialize all the doom objects: MobjInfo, SprNames, SoundMap, etc.
//
void D_InitializeDoomObjectTables()
{
	// [RH] Initialize items. Still only used for the give command. :-(
	InitItems();
	// Initialize states
	states.clear();
	states.insert({boomstates, ::NUMSTATES}, S_NULL);
	states.insert(getOdaStates(), S_GIB0);
	// Initialize mobjinfo
	mobjinfo.clear();
	mobjinfo.insert({doom_mobjinfo, ::NUMMOBJTYPES}, MT_PLAYER);
	mobjinfo.insert(getOdaMobjinfo(), MT_GIB0);
	// Initialize sprnames
	sprnames.clear();
	sprnames.insert({doom_sprnames, ::NUMSPRITES}, SPR_TROO);
	sprnames.insert(getOdaSprNames(), SPR_GIB0);
	// Initialize soundmap
	SoundMap.clear();
	SoundMap.insert({doom_SoundMap, ARRAY_LENGTH(doom_SoundMap)}, 0);
	SoundMap.insert({odamex_SoundMap, ARRAY_LENGTH(odamex_SoundMap)}, 0x80000000);
	// Initialize spawn map
	D_BuildSpawnMap();

	states.rebuildMap(
		[](const state_t& lhs, const state_t& rhs){ return lhs.statenum < rhs.statenum; },
		[](const state_t& s){ return s.statenum; }
	);
	mobjinfo.rebuildMap(
		[](const mobjinfo_t& lhs, const mobjinfo_t& rhs){
			return lhs.type < rhs.type || (lhs.type == rhs.type && lhs.doomednum < rhs.doomednum);
		},
		[](const mobjinfo_t& m){ return m.type; }
	);
}

//
// D_AddSearchDir
// denis - Split a new directory string using the separator and append results to the output
//
void D_AddSearchDir(std::vector<std::string> &dirs, const char *dir, const char separator,
                    const missing_dir_policy policy)
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

		M_ExpandHomeDir(segment);
		segment = M_CleanPath(segment);
		if (D_IsExistingDir(segment))
		{
			dirs.push_back(segment);
		}
		else if (policy == missing_dir_policy::warn ||
		         (policy == missing_dir_policy::developer_warn && (::developer || ::devparm)))
		{
			PrintFmt(PRINT_HIGH, "{}: search dir not found: {}\n", __FUNCTION__, segment);
		}
	}
}

// [AM] Add platform-sepcific search directories
void D_AddPlatformSearchDirs(std::vector<std::string> &dirs)
{
	#if defined(_WIN32)

	const char separator = ';';

	// Doom 95
	{
		for (auto& uninstallval : uninstall_values)
		{
			char* val;
			char* path;
			char* unstr;

			val = GetRegistryString(&uninstallval);

			if (val == nullptr)
				continue;

			unstr = strstr(val, uninstaller_string);

			if (unstr == nullptr)
			{
				M_Free(val);
			}
			else
			{
				path = unstr + strlen(uninstaller_string);

				const char* cpath = path;
				D_AddSearchDir(dirs, cpath, separator, missing_dir_policy::silent);
			}
		}
	}

	// Doom Collectors Edition
	{
		char* install_path = GetRegistryString(&collectors_edition_value);

		if (install_path != nullptr)
		{
			for (const auto& dir : collectors_edition_subdirs)
			{
				const std::string subpath = fmt::format("{}\\{}", install_path, dir);

				D_AddSearchDir(dirs, subpath.c_str(), separator, missing_dir_policy::silent);
			}

			M_Free(install_path);
		}
	}

	// Doom on Steam
	{
		char* install_path = GetRegistryString(&steam_install_location);

		if (install_path != nullptr)
		{
			const auto steam_library_paths = GetSteamLibraryPaths(install_path);
			D_AddSteamSearchDirs(dirs, steam_library_paths, separator);

			M_Free(install_path);
		}
	}

	// Doom on GOG
	{
		char* doom_plus_doom2_path = GetRegistryString(&gog_doom_plus_doom2);

		if (doom_plus_doom2_path != nullptr)
		{
			D_AddSearchDir(dirs, doom_plus_doom2_path, separator, missing_dir_policy::silent);
			M_Free(doom_plus_doom2_path);
		}

		char* doom_path = GetRegistryString(&gog_doom);

		if (doom_path != nullptr)
		{
			D_AddSearchDir(dirs, doom_path, separator, missing_dir_policy::silent);
			M_Free(doom_path);
		}

		char* doom2_path = GetRegistryString(&gog_doom2);

		if (doom2_path != nullptr)
		{
			const std::string full_doom2_path = fmt::format("{}\\{}", doom2_path, "doom2");
			const std::string master_levels_path = fmt::format("{}\\{}", doom2_path, "master\\wads");
			D_AddSearchDir(dirs, full_doom2_path.c_str(), separator, missing_dir_policy::silent);
			D_AddSearchDir(dirs, master_levels_path.c_str(), separator, missing_dir_policy::silent);
			M_Free(doom2_path);
		}

		char* final_doom_path = GetRegistryString(&gog_final_doom);

		if (final_doom_path != nullptr)
		{
			const std::string plutonia_path = fmt::format("{}\\{}", final_doom_path, "Plutonia");
			const std::string tnt_path = fmt::format("{}\\{}", final_doom_path, "TNT");
			D_AddSearchDir(dirs, plutonia_path.c_str(), separator, missing_dir_policy::silent);
			D_AddSearchDir(dirs, tnt_path.c_str(), separator, missing_dir_policy::silent);
			M_Free(final_doom_path);
		}

		char* heretic_plus_hexen_path = GetRegistryString(&gog_heretic_hexen);

		if (heretic_plus_hexen_path != nullptr)
		{
			D_AddSearchDir(dirs, heretic_plus_hexen_path, separator, missing_dir_policy::silent);
			M_Free(heretic_plus_hexen_path);
		}
	}

	// DOS Doom via DEICE
	D_AddSearchDir(dirs, "\\doom2", separator, missing_dir_policy::developer_warn);    // Doom II
	D_AddSearchDir(dirs, "\\plutonia", separator, missing_dir_policy::developer_warn); // Final Doom
	D_AddSearchDir(dirs, "\\tnt", separator, missing_dir_policy::developer_warn);
	D_AddSearchDir(dirs, "\\doom_se", separator, missing_dir_policy::developer_warn);  // Ultimate Doom
	D_AddSearchDir(dirs, "\\doom", separator, missing_dir_policy::developer_warn);     // Shareware / Registered Doom
	D_AddSearchDir(dirs, "\\dooms", separator, missing_dir_policy::developer_warn);    // Shareware versions
	D_AddSearchDir(dirs, "\\doomsw", separator, missing_dir_policy::developer_warn);

	#elif defined(UNIX)

	const char separator = ':';

	// Doom on Steam
	{
		std::vector<std::string> steam_install_paths;

	#if defined(__APPLE__)
		steam_install_paths.emplace_back("~/Library/Application Support/Steam");
	#else
		if (const char* xdg_data_home = std::getenv("XDG_DATA_HOME"))
			steam_install_paths.emplace_back(std::string(xdg_data_home) + PATHSEP + "Steam");
		steam_install_paths.emplace_back("~/.steam/steam");
		steam_install_paths.emplace_back("~/.local/share/Steam");
		steam_install_paths.emplace_back("~/.var/app/com.valvesoftware.Steam/.local/share/Steam");
	#endif

		for (auto& install_path : steam_install_paths)
		{
			M_ExpandHomeDir(install_path);
			const auto steam_library_paths = GetSteamLibraryPaths(install_path);
			D_AddSteamSearchDirs(dirs, steam_library_paths, separator);
		}
	}

	#if defined(INSTALL_PREFIX) && defined(INSTALL_DATADIR)
	D_AddSearchDir(dirs, INSTALL_PREFIX "/" INSTALL_DATADIR "/odamex", separator, missing_dir_policy::developer_warn);
	D_AddSearchDir(dirs, INSTALL_PREFIX "/" INSTALL_DATADIR "/games/odamex", separator, missing_dir_policy::developer_warn);
	#endif
	// Search the maintainer-directed data directory for WADs
	#if defined(ODAMEX_INSTALL_DATADIR)
	D_AddSearchDir(dirs, ODAMEX_INSTALL_DATADIR, separator, missing_dir_policy::developer_warn);
	#endif

	D_AddSearchDir(dirs, "/usr/share/doom", separator, missing_dir_policy::developer_warn);
	D_AddSearchDir(dirs, "/usr/share/games/doom", separator, missing_dir_policy::developer_warn);
	D_AddSearchDir(dirs, "/usr/local/share/games/doom", separator, missing_dir_policy::developer_warn);
	D_AddSearchDir(dirs, "/usr/local/share/doom", separator, missing_dir_policy::developer_warn);
	// Flatpak sandbox default directories
	// (Since you need to pass envvars to a Flatpak)
	D_AddSearchDir(dirs, "/run/host/usr/share/doom", separator, missing_dir_policy::developer_warn);
	D_AddSearchDir(dirs, "/run/host/usr/share/games/doom", separator, missing_dir_policy::developer_warn);
	D_AddSearchDir(dirs, "/run/host/usr/local/share/games/doom", separator, missing_dir_policy::developer_warn);
	D_AddSearchDir(dirs, "/run/host/usr/local/share/doom", separator, missing_dir_policy::developer_warn);

	#endif
}


//
// D_GetTitleString
//
// Returns the proper name of the game currently loaded into gameinfo & gamemission
//
std::string D_GetTitleString()
{
	return gameinfo.titleString;
}


//
// D_PrintIWADIdentity
//
static void D_PrintIWADIdentity()
{
	if (clientside)
	{
		PrintFmt(PRINT_HIGH, "\n\35\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36"
    	                     "\36\36\36\36\36\36\36\36\36\36\36\36\37\n");

		if (gamemode == undetermined)
			PrintFmt_Bold("Game mode indeterminate, no standard wad found.\n\n");
		else
			PrintFmt_Bold("{}\n\n", D_GetTitleString());
	}
	else
	{
		if (gamemode == undetermined)
			PrintFmt(PRINT_HIGH, "Game mode indeterminate, no standard wad found.\n");
		else
			PrintFmt(PRINT_HIGH, "{}\n", D_GetTitleString());
	}
}


/**
 * @brief Load all found DEH patches, as well as all found DEHACKED lumps.
 */
void D_LoadResolvedPatches(bool reloadStrings)
{
	// Load internal chex.deh if necessary
	if (::gamemode == retail_chex)
	{
		D_DoDehPatch(nullptr, W_GetNumForName("_CHXHACK"), reloadStrings);
	}

	// Load external patch files first.
	for (const auto& file : ::patchfiles)
	{
		D_DoDehPatch(&file, -1, reloadStrings);
	}

	// Check WAD files for lumps.
	int lump = -1;
	while ((lump = W_FindLump("DEHACKED", lump)) != -1)
	{
		D_DoDehPatch(NULL, lump, reloadStrings);
	}

	// Re-apply spawninv settings with our new DEH settings.
	G_SetupSpawnInventory();
}


//
// D_CleanseFileName
//
// Strips a file name of path information and transforms it into uppercase
//
std::string D_CleanseFileName(const std::string &filename, const std::string &ext)
{
	std::string newname(filename);

	M_FixPathSep(newname);
	if (ext.length())
		M_AppendExtension(newname, "." + ext);

	size_t slash = newname.find_last_of(PATHSEPCHAR);

	if (slash != std::string::npos)
		newname = newname.substr(slash + 1, newname.length() - slash);

	std::transform(newname.begin(), newname.end(), newname.begin(), toupper);

	return newname;
}


//
// D_FindIWAD
//
// Tries to find an IWAD from a set of known IWAD file names.
//
static bool FindIWAD(OResFile& out)
{
	// Search for a pre-defined IWAD from the list above
	std::vector<OString> filenames = W_GetIWADFilenames();
	for (const auto& filename : filenames)
	{
		// Construct a file.
		OWantFile wantfile;
		if (!OWantFile::make(wantfile, filename, OFILE_WAD))
		{
			continue;
		}

		// Resolve the file.
		if (!M_ResolveWantedFile(out, wantfile))
		{
			continue;
		}

		return W_IsIWAD(out);
	}

	return false;
}

/**
 * @brief Load files that are assumed to be resolved, in the correct order,
 *        and complete.
 *
 * @param newwadfiles New set of WAD files.
 * @param newpatchfiles New set of patch files.
*/
static void LoadResolvedFiles(const OResFiles& newwadfiles,
                              const OResFiles& newpatchfiles)
{
	if (newwadfiles.size() < 2)
	{
		I_FatalError("Tried to load resources without an ODAMEX.WAD or an IWAD.");
	}

	::wadfiles = newwadfiles;
	::patchfiles = newpatchfiles;

	// Now scan the contents of the IWAD to determine which one it is
	W_ConfigureGameInfo(::wadfiles.at(1));

	// print info about the IWAD to the console
	D_PrintIWADIdentity();

	// set the window title based on which IWAD we're using
	I_SetTitleString(D_GetTitleString().c_str());

	::modifiedgame = (::wadfiles.size() > 2) ||
	                 !::patchfiles.empty(); // more than odamex.wad and IWAD?

	if (::modifiedgame && (::gameinfo.flags & GI_SHAREWARE))
	{
		I_FatalError(
		    "\nYou cannot load additional WADs with the shareware version. Register!");
	}

	W_InitMultipleFiles(::wadfiles);

	// [RH] Initialize localizable strings.
	// [SL] It is necessary to load the strings here since a dehacked patch
	// might change the strings
	::GStrings.loadStrings(false);

	P_InitMobjNameMap();

	// Apply DEH patches.
	D_LoadResolvedPatches();
}

/**
 * @brief Print a warning that occurrs when the user has an IWAD that's a
 *        different version than the one we want.
 *
 * @param wanted The IWAD that we wanted.
 * @return True if we emitted an commercial IWAD warning.
 */
static bool CommercialIWADWarning(const OWantFile& wanted)
{
	const OMD5Hash& hash = wanted.getWantedMD5();
	if (hash.empty())
	{
		// No MD5 means there is no error we can reasonably display.
		return false;
	}

	const fileIdentifier_t* info = W_GameInfo(wanted.getWantedMD5());
	if (!info)
	{
		// No GameInfo means that we're not dealing with a WAD we recognize.
		return false;
	}

	if (!info->mIsCommercial)
	{
		// Not commercial means that we should treat the IWAD like any other
		// WAD, with no special callout.
		return false;
	}

	PrintFmt("Odamex attempted to load\n> {}.\n\n", info->mIdName);

	// Try to find an IWAD file with a matching name in the user's directories.
	OWantFile sameNameWant;
	OWantFile::make(sameNameWant, wanted.getBasename(), OFILE_WAD);
	OResFile sameNameRes;
	const bool resolved = M_ResolveWantedFile(sameNameRes, sameNameWant);
	if (!resolved)
	{
		PrintFmt(
		    "Odamex could not find the data file for this game in any of the locations "
		    "it searches for WAD files.  If you know you have {} on your hard drive, you "
		    "can add that path to the 'waddirs' cvar so Odamex can find it.\n\n",
		    wanted.getBasename());
	}
	else
	{
		const fileIdentifier_t* curInfo = W_GameInfo(sameNameRes.getMD5());
		if (curInfo)
		{
			// Found a file, but it's the wrong version.
			PrintFmt("Odamex found a possible data file, but it's the wrong version.\n> "
			         "{}\n> {}\n\n",
			         curInfo->mIdName, sameNameRes.getFullpath());
		}
		else
		{
			// Found a file, but it's not recognized at all.
			PrintFmt("Odamex found a possible data file, but Odamex does not recognize "
			         "it.\n> {}\n\n",
			         sameNameRes.getFullpath());
		}

#ifdef _WIN32
		PrintFmt("You can use a tool such as Omniscient "
		         "<https://drinkybird.net/doom/omniscient> to patch your way to the "
		         "correct version of the data file.\n");
#else
		PrintFmt("You can use a tool such as xdelta3 <http://xdelta.org/> paried with IWAD "
		         "patches located on Github <https://github.com/Doom-Utils/iwad-patches> "
		         "to patch your way to the correct version of the data file.\n");
#endif
	}

	PrintFmt("If you do not own this game, consider purchasing it on Steam, GOG, or other "
	         "digital storefront.\n\n");
	return true;
}

//
// D_LoadResourceFiles
//
// Performs the grunt work of loading WAD and DEH/BEX files.
// The global wadfiles and patchfiles vectors are filled with the list
// of loaded filenames and the missingfiles vector is also filled if
// applicable.
//
void D_LoadResourceFiles(const OWantFiles& newwadfiles, const OWantFiles& newpatchfiles)
{
	OResFile odamex_wad;
	OResFile next_iwad;

	::missingfiles.clear();
	::missingCommercialIWAD = false;

	// Resolve wanted wads.
	OResFiles resolved_wads;
	resolved_wads.reserve(newwadfiles.size());
	for (const auto& wantfile : newwadfiles)
	{
		OResFile file;
		if (!M_ResolveWantedFile(file, wantfile))
		{
			// Give more useful information when trying to load an IWAD.
			const bool isCommercial = CommercialIWADWarning(wantfile);
			if (isCommercial && !::missingCommercialIWAD)
			{
				::missingCommercialIWAD = true;
			}

			::missingfiles.push_back(wantfile);
			PrintFmt(PRINT_WARNING, "Could not resolve resource file \"{}\".",
			         wantfile.getWantedPath());
			continue;
		}
		resolved_wads.push_back(file);
	}

	// Resolve wanted patches.
	OResFiles resolved_patches;
	resolved_patches.reserve(newpatchfiles.size());
	for (const auto& wantfile : newpatchfiles)
	{
		OResFile file;
		if (!M_ResolveWantedFile(file, wantfile))
		{
			::missingfiles.push_back(wantfile);
			PrintFmt(PRINT_WARNING, "Could not resolve patch file \"{}\".",
			         wantfile.getWantedPath());
			continue;
		}
		resolved_patches.push_back(file);
	}

	// ODAMEX.WAD //

	if (::wadfiles.empty())
	{
		// If we don't have odamex.wad, resolve it now.
		OWantFile want_odamex;
		OWantFile::make(want_odamex, "odamex.wad", OFILE_WAD);
		if (!M_ResolveWantedFile(odamex_wad, want_odamex))
		{
			I_FatalError("Could not resolve \"{}\".  Please ensure this file is "
			             "someplace where Odamex can find it.\n",
			             want_odamex.getBasename());
		}
	}
	else
	{
		// We already have odamex.wad, just make a copy of it.
		odamex_wad = ::wadfiles.at(0);
	}

	// IWAD //

	bool got_next_iwad = false;
	if (resolved_wads.size() >= 1)
	{
		// See if the first WAD we passed was an IWAD.
		const OResFile& possible_iwad = resolved_wads.at(0);
		if (W_IsIWAD(possible_iwad))
		{
			next_iwad = possible_iwad;
			got_next_iwad = true;
			resolved_wads.erase(resolved_wads.begin());
			if (W_IsIWADDeprecated(next_iwad))
			{
				PrintFmt_Bold("WARNING: IWAD {} is outdated. Please update it to the "
				              "latest version.\n",
				              next_iwad.getBasename());
			}
		}
	}

	if (!got_next_iwad && ::wadfiles.size() >= 2)
	{
		// Reuse the old IWAD.  As an optimization, assume that the location
		// of the IWAD has not changed on disk.
		next_iwad = ::wadfiles.at(1);
		got_next_iwad = true;
	}

	if (!got_next_iwad)
	{
		// Not provided an IWAD filename and an IWAD is not currently loaded?
		// Try to find *any* IWAD using FindIWAD.
		got_next_iwad = FindIWAD(next_iwad);
	}

	if (!got_next_iwad)
	{
		I_FatalError("Could not resolve an IWAD file.  Please ensure at least "
		             "one IWAD is someplace where Odamex can find it.\n");
	}

	resolved_wads.insert(resolved_wads.begin(), odamex_wad);
	resolved_wads.insert(resolved_wads.begin() + 1, next_iwad);
	LoadResolvedFiles(resolved_wads, resolved_patches);
}

/**
 * @brief Check to see if the list of WAD files and patches matches the
 *        currently loaded files.
 *
 * @detail Note that this relies on the hashes being equal, so if you want
 *         resources to not be reloaded, ensure the hashes are equal by the
 *         time they reach this spot.
 *
 * @param newwadfiles WAD files to check.
 * @param newpatchfiles Patch files to check.
 * @return True if everything checks out.
 */
static bool CheckWantedMatchesLoaded(const OWantFiles& newwadfiles,
                                     const OWantFiles& newpatchfiles)
{
	// Cheking sizes is a good first approximation.

	if (newwadfiles.size() + 1 != ::wadfiles.size())
	{
		return false;
	}

	if (newpatchfiles.size() != ::patchfiles.size())
	{
		return false;
	}

	// Check WAD hashes - with an offset because you can't replace odamex.wad.
	for (OWantFiles::const_iterator it = newwadfiles.begin(); it != newwadfiles.end();
	     ++it)
	{
		size_t idx = it - newwadfiles.begin();
		if (it->getWantedMD5() != ::wadfiles.at(idx + 1).getMD5())
		{
			return false;
		}
	}

	// Check patch hashes.
	for (OWantFiles::const_iterator it = newpatchfiles.begin(); it != newpatchfiles.end();
	     ++it)
	{
		size_t idx = it - newpatchfiles.begin();
		if (it->getWantedMD5() != ::patchfiles.at(idx).getMD5())
		{
			return false;
		}
	}

	return true;
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
bool D_DoomWadReboot(const OWantFiles& newwadfiles, const OWantFiles& newpatchfiles)
{
	// already loaded these?
	if (::lastWadRebootSuccess && CheckWantedMatchesLoaded(newwadfiles, newpatchfiles))
	{
		// fast track if files have not been changed
		PrintFmt("Currently loaded resources match server checksums.\n\n");
		return true;
	}

	::lastWadRebootSuccess = false;

	D_Shutdown();

	gamestate_t oldgamestate = ::gamestate;
	::gamestate = GS_STARTUP; // prevent console from trying to use nonexistant font

	// Load all the WAD and DEH/BEX files
	OResFiles oldwadfiles = ::wadfiles;
	OResFiles oldpatchfiles = ::patchfiles;
	std::string failmsg;
	try
	{
		D_LoadResourceFiles(newwadfiles, newpatchfiles);

		D_Init();

		// get skill / episode / map from parms
		startmap = EpisodeMaps[0];
	}
	catch (CRecoverableError& error)
	{
		failmsg = error.GetMsg();
	}

	if (!failmsg.empty())
	{
		// Uh oh, loading the new resource set failed for some reason.
		PrintFmt(PRINT_WARNING,
		         "Could not load new resource files.\n{}\nReloading previous resource "
		         "set...\n",
		         failmsg);

		D_Shutdown();

		std::string fatalmsg;
		try
		{
			LoadResolvedFiles(oldwadfiles, oldpatchfiles);

			D_Init();

			// get skill / episode / map from parms
			startmap = EpisodeMaps[0];
		}
		catch (CRecoverableError& error)
		{
			// Something is seriously wrong.
			fatalmsg = error.GetMsg();
		}
		if (!fatalmsg.empty())
		{
			I_FatalError("Failed to load new resource files, then ran into error when "
			             "loading original resource files:\n{}\n",
			             fatalmsg);
		}
	}

	// preserve state
	::lastWadRebootSuccess = ::missingfiles.empty();

	::gamestate = oldgamestate; // GS_STARTUP would prevent netcode connecting properly

	return ::missingfiles.empty() && failmsg.empty();
}


//
// AddCommandLineOptionFiles
//
// Adds the full path of all the file names following the given command line
// option parameter (eg, "-file") matching the specified extension to the
// filenames vector.
//
static void AddCommandLineOptionFiles(OWantFiles& out, const std::string& option,
                                      ofile_t type)
{
	DArgs files = Args.GatherFiles(option.c_str());
	for (size_t i = 0; i < files.NumArgs(); i++)
	{
		OWantFile file;
		if (OWantFile::make(file, files.GetArg(i), type))
			out.push_back(file);
	}

	files.FlushArgs();
}

//
// D_AddWadCommandLineFiles
//
// Add the WAD files specified with -file.
// Call this from D_DoomMain
//
void D_AddWadCommandLineFiles(OWantFiles& out)
{
	AddCommandLineOptionFiles(out, "-file", OFILE_WAD);
}

//
// D_AddDehCommandLineFiles
//
// Adds the DEH/BEX files specified with -bex or -deh.
// Call this from D_DoomMain
//
void D_AddDehCommandLineFiles(OWantFiles& out)
{
	AddCommandLineOptionFiles(out, "-bex", OFILE_DEH);
	AddCommandLineOptionFiles(out, "-deh", OFILE_DEH);
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

	~UncappedTaskScheduler() override { }

	void run() override
	{
		mTask();
	}

	dtime_t getNextTime() const override
	{
		return I_GetTime();
	}

	float getRemainder() const override
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

	~CappedTaskScheduler() override { }

	void run() override
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

	dtime_t getNextTime() const override
	{
		return mFrameStartTime + mFrameDuration - mAccumulator;
	}

	float getRemainder() const override
	{
		// mAccumulator can be greater than mFrameDuration so only get the
		// time remaining until the next frame
		dtime_t remaining_time = mAccumulator % mFrameDuration;
		return static_cast<float>(static_cast<double>(remaining_time) / mFrameDuration);
	}

private:
	void				(*mTask)();
	const int			mMaxCount;
	const dtime_t		mFrameDuration;
	dtime_t				mAccumulator;
	dtime_t				mFrameStartTime;
	dtime_t				mPreviousFrameStartTime;
};

static std::unique_ptr<TaskScheduler> simulation_scheduler;
static std::unique_ptr<TaskScheduler> display_scheduler;

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

		if (capped_simulation)
			simulation_scheduler = std::make_unique<CappedTaskScheduler>(sim_func, TICRATE, 4);
		else
			simulation_scheduler = std::make_unique<UncappedTaskScheduler>(sim_func);
	}

	if (capped_display != previous_capped_display || maxfps != previous_maxfps)
	{
		previous_capped_display = capped_display;
		previous_maxfps = maxfps;

		if (capped_display)
			display_scheduler = std::make_unique<CappedTaskScheduler>(display_func, maxfps, 1);
		else
			display_scheduler = std::make_unique<UncappedTaskScheduler>(display_func);
	}
}

void STACK_ARGS D_ClearTaskSchedulers()
{
	simulation_scheduler.reset();
	display_scheduler.reset();
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
		|| timingdemo || step_mode)
		render_lerp_amount = FRACUNIT;
	else
		render_lerp_amount = simulation_scheduler->getRemainder() * FRACUNIT;
#endif

	display_scheduler->run();

	if (timingdemo)
		return;

	// Sleep until the next scheduled task.
	const dtime_t simulation_wake_time = simulation_scheduler->getNextTime();
	const dtime_t display_wake_time = display_scheduler->getNextTime();
	const dtime_t wake_time = std::min<dtime_t>(simulation_wake_time, display_wake_time);

	constexpr dtime_t max_sleep_amount = 1000LL * 1000LL;	// 1ms

	// Sleep in 1ms increments until the next scheduled task
	for (dtime_t now = I_GetTime(); wake_time > now; now = I_GetTime())
	{
		const dtime_t sleep_amount = std::min<dtime_t>(max_sleep_amount, wake_time - now);
		I_Sleep(sleep_amount);
	}
}

VERSION_CONTROL (d_main_cpp, "$Id$")
