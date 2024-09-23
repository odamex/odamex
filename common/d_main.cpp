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
//	Common functions to determine game mode (shareware, registered),
//	parse command line parameters, and handle wad changes.
//
//-----------------------------------------------------------------------------


#include "odamex.h"


#include <sstream>
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

#include "m_alloc.h"
#include "gstrings.h"
#include "z_zone.h"
#include "w_wad.h"
#include "m_argv.h"
#include "m_fileio.h"
#include "c_console.h"
#include "i_system.h"
#include "g_game.h"
#include "g_spawninv.h"
#include "r_main.h"
#include "d_main.h"
#include "d_dehacked.h"
#include "s_sound.h"
#include "gi.h"
#include "w_ident.h"
#include "m_resfile.h"

#ifdef GEKKO
#include "i_wii.h"
#endif

#ifdef _XBOX
#include "i_xbox.h"
#endif

OResFiles wadfiles;
OResFiles patchfiles;
OWantFiles missingfiles;
bool missingCommercialIWAD = false;

bool lastWadRebootSuccess = true;
extern bool step_mode;

bool capfps = true;
float maxfps = 35.0f;


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

		M_ExpandHomeDir(segment);
		segment = M_CleanPath(segment);

		dirs.push_back(segment);
	}
}

// [AM] Add platform-sepcific search directories
void D_AddPlatformSearchDirs(std::vector<std::string> &dirs)
{
	#if defined(_WIN32) && !defined(_XBOX)

	const char separator = ';';

	// Doom 95
	{
		unsigned int i;

		for (i = 0; i < ARRAY_LENGTH(uninstall_values); ++i)
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
			for (i = 0; i < ARRAY_LENGTH(collectors_edition_subdirs); ++i)
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
			for (i = 0; i < ARRAY_LENGTH(steam_install_subdirs); ++i)
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

	D_AddSearchDir(dirs, "/usr/share/doom", separator);
	D_AddSearchDir(dirs, "/usr/share/games/doom", separator);
	D_AddSearchDir(dirs, "/usr/local/share/games/doom", separator);
	D_AddSearchDir(dirs, "/usr/local/share/doom", separator);

	#endif
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


/**
 * @brief Load all found DEH patches, as well as all found DEHACKED lumps.
 */
void D_LoadResolvedPatches()
{
	// Load external patch files first.
	bool chexLoaded = false;
	for (OResFiles::const_iterator it = ::patchfiles.begin(); it != ::patchfiles.end();
	     ++it)
	{
		if (StdStringToUpper(it->getBasename()) == "CHEX.DEH")
		{
			chexLoaded = true;
		}
		D_DoDehPatch(&*it, -1);
	}

	// Check WAD files for lumps.
	int lump = -1;
	while ((lump = W_FindLump("DEHACKED", lump)) != -1)
	{
		D_DoDehPatch(NULL, lump);
	}

	if (::gamemode == retail_chex && !::multiplayer && !chexLoaded)
	{
		Printf(
		    PRINT_WARNING,
		    "Warning: chex.deh not loaded, experience may differ from the original!\n");
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
	for (std::vector<OString>::const_iterator it = filenames.begin();
	     it != filenames.end(); ++it)
	{
		// Construct a file.
		OWantFile wantfile;
		std::string filename = it->c_str();
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

	Printf("Odamex attempted to load\n> %s.\n\n", info->mIdName.c_str());

	// Try to find an IWAD file with a matching name in the user's directories.
	OWantFile sameNameWant;
	OWantFile::make(sameNameWant, wanted.getBasename(), OFILE_WAD);
	OResFile sameNameRes;
	const bool resolved = M_ResolveWantedFile(sameNameRes, sameNameWant);
	if (!resolved)
	{
		Printf(
		    "Odamex could not find the data file for this game in any of the locations "
		    "it searches for WAD files.  If you know you have %s on your hard drive, you "
		    "can add that path to the 'waddirs' cvar so Odamex can find it.\n\n",
		    wanted.getBasename().c_str());
	}
	else
	{
		const fileIdentifier_t* curInfo = W_GameInfo(sameNameRes.getMD5());
		if (curInfo)
		{
			// Found a file, but it's the wrong version.
			Printf("Odamex found a possible data file, but it's the wrong version.\n> "
			       "%s\n> %s\n\n",
			       curInfo->mIdName.c_str(), sameNameRes.getFullpath().c_str());
		}
		else
		{
			// Found a file, but it's not recognized at all.
			Printf("Odamex found a possible data file, but Odamex does not recognize "
			       "it.\n> %s\n\n",
			       sameNameRes.getFullpath().c_str());
		}

#ifdef _WIN32
		Printf("You can use a tool such as Omniscient "
		       "<https://drinkybird.net/doom/omniscient> to patch your way to the "
		       "correct version of the data file.\n");
#else
		Printf("You can use a tool such as xdelta3 <http://xdelta.org/> paried with IWAD "
		       "patches located on Github <https://github.com/Doom-Utils/iwad-patches> "
		       "to patch your way to the correct version of the data file.\n");
#endif
	}

	Printf("If you do not own this game, consider purchasing it on Steam, GOG, or other "
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
	for (OWantFiles::const_iterator it = newwadfiles.begin(); it != newwadfiles.end();
	     ++it)
	{
		OResFile file;
		if (!M_ResolveWantedFile(file, *it))
		{
			// Give more useful information when trying to load an IWAD.
			const bool isCommercial = CommercialIWADWarning(*it);
			if (isCommercial && !::missingCommercialIWAD)
			{
				::missingCommercialIWAD = true;
			}

			::missingfiles.push_back(*it);
			Printf(PRINT_WARNING, "Could not resolve resource file \"%s\".",
			       it->getWantedPath().c_str());
			continue;
		}
		resolved_wads.push_back(file);
	}

	// Resolve wanted patches.
	OResFiles resolved_patches;
	resolved_patches.reserve(newpatchfiles.size());
	for (OWantFiles::const_iterator it = newpatchfiles.begin(); it != newpatchfiles.end();
	     ++it)
	{
		OResFile file;
		if (!M_ResolveWantedFile(file, *it))
		{
			::missingfiles.push_back(*it);
			Printf(PRINT_WARNING, "Could not resolve patch file \"%s\".",
			       it->getWantedPath().c_str());
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
			I_FatalError("Could not resolve \"%s\".  Please ensure this file is "
			             "someplace where Odamex can find it.\n",
			             want_odamex.getBasename().c_str());
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
				Printf_Bold("WARNING: IWAD %s is outdated. Please update it to the "
				            "latest version.\n",
				            next_iwad.getBasename().c_str());
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
		Printf("Currently loaded resources match server checksums.\n\n");
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

		// get skill / episode / map from parms
		startmap = (gameinfo.flags & GI_MAPxx) ? "MAP01" : "E1M1";

		D_Init();
	}
	catch (CRecoverableError& error)
	{
		failmsg = error.GetMsg();
	}

	if (!failmsg.empty())
	{
		// Uh oh, loading the new resource set failed for some reason.
		Printf(PRINT_WARNING,
		       "Could not load new resource files.\n%s\nReloading previous resource "
		       "set...\n",
		       failmsg.c_str());

		D_Shutdown();

		std::string fatalmsg;
		try
		{
			LoadResolvedFiles(oldwadfiles, oldpatchfiles);

			// get skill / episode / map from parms
			startmap = (gameinfo.flags & GI_MAPxx) ? "MAP01" : "E1M1";

			D_Init();
		}
		catch (CRecoverableError& error)
		{
			// Something is seriously wrong.
			fatalmsg = error.GetMsg();
		}
		if (!fatalmsg.empty())
		{
			I_FatalError("Failed to load new resource files, then ran into error when "
			             "loading original resource files:\n%s\n",
			             fatalmsg.c_str());
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
		OWantFile::make(file, files.GetArg(i), type);
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
		|| timingdemo || step_mode)
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
	dtime_t wake_time = std::min<dtime_t>(simulation_wake_time, display_wake_time);

	const dtime_t max_sleep_amount = 1000LL * 1000LL;	// 1ms

	// Sleep in 1ms increments until the next scheduled task
	for (dtime_t now = I_GetTime(); wake_time > now; now = I_GetTime())
	{
		dtime_t sleep_amount = std::min<dtime_t>(max_sleep_amount, wake_time - now);
		I_Sleep(sleep_amount);
	}
}

VERSION_CONTROL (d_main_cpp, "$Id$")
