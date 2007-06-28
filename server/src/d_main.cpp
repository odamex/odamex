// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
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
//	DOOM main program (D_DoomMain) and game loop (D_DoomLoop),
//	plus functions to determine game mode (shareware, registered),
//	parse command line parameters, configure game parameters (turbo),
//	and call the startup functions.
//
//-----------------------------------------------------------------------------

#include "version.h"

#include <sstream>
#include <string>
#include <vector>
#include <algorithm>

#ifdef WIN32
#include <windows.h>
#else
#include <sys/stat.h>
#endif

#ifdef UNIX
#include <unistd.h>
#endif

#include <time.h>
#include <math.h>

#include "errors.h"

#include "m_alloc.h"
#include "m_random.h"
#include "minilzo.h"
#include "doomdef.h"
#include "doomstat.h"
#include "dstrings.h"
#include "z_zone.h"
#include "w_wad.h"
#include "s_sound.h"
#include "v_video.h"
#include "m_argv.h"
#include "m_misc.h"
#include "c_console.h"
#include "c_dispatch.h"
#include "i_system.h"
#include "g_game.h"
#include "p_setup.h"
#include "r_local.h"
#include "r_sky.h"
#include "d_main.h"
#include "d_dehacked.h"
#include "cmdlib.h"
#include "s_sound.h"
#include "m_swap.h"
#include "v_text.h"
#include "gi.h"
#include "sv_main.h"

EXTERN_CVAR (timelimit)

extern void M_RestoreMode (void);
extern void R_ExecuteSetViewSize (void);

void D_CheckNetGame (void);
void D_ProcessEvents (void);
void G_BuildTiccmd (ticcmd_t* cmd);
void D_DoAdvanceDemo (void);

#ifdef UNIX
void daemon_init();
#endif

void D_DoomLoop (void);

extern gameinfo_t SharewareGameInfo;
extern gameinfo_t RegisteredGameInfo;
extern gameinfo_t RetailGameInfo;
extern gameinfo_t CommercialGameInfo;

extern int testingmode;
extern BOOL setsizeneeded;
extern BOOL setmodeneeded;
extern BOOL netdemo;
extern int NewWidth, NewHeight, NewBits, DisplayBits;
EXTERN_CVAR (st_scale) // removeme
extern BOOL gameisdead;
extern BOOL demorecording;
extern DThinker ThinkerCap;

CVAR (def_patch, "", CVAR_NULL) // removeme ??


std::vector<std::string> wadfiles;		// [RH] remove limit on # of loaded wads
BOOL devparm;				// started game with -devparm
char *D_DrawIcon;			// [RH] Patch name of icon to draw on next refresh
int NoWipe;					// [RH] Allow wipe? (Needs to be set each time)
char startmap[8];
BOOL autostart;
BOOL advancedemo;
FILE *debugfile;
event_t events[MAXEVENTS];
int eventhead;
int eventtail;
gamestate_t wipegamestate = GS_DEMOSCREEN;	// can be -1 to force a wipe
DCanvas *page;

static int demosequence;
static int pagetic;

//
// D_ProcessEvents
// Send all the events of the given timestamp down the responder chain
//
void D_ProcessEvents (void)
{
}

//
// D_PostEvent
// Called by the I/O functions when input is detected
//
void D_PostEvent (const event_t* ev)
{
	events[eventhead] = *ev;

	if(++eventhead >= MAXEVENTS)
		eventhead = 0;
}

//
// D_Display
//  draw current display, possibly wiping it from the previous
//
void D_Display (void)
{
}

//
// D_DoomLoop
//
void D_DoomLoop (void)
{
	while (1)
	{
		try
		{
			SV_RunTics (); // will run at least one tic
		}
		catch (CRecoverableError &error)
		{
			Printf (PRINT_HIGH, "ERROR: %s\n", error.GetMessage().c_str());
			Printf (PRINT_HIGH, "sleeping for 10 seconds before map reload...");

			// denis - drop clients
			SV_SendDisconnectSignal();

			// denis - sleep to conserve server resources (in case of recurring problem)
			I_WaitForTic(I_GetTime() + 1000*10/TICRATE);

			// denis - reload with current settings
			G_ChangeMap ();

			// denis - todo - throw I_FatalError if this keeps happening
		}
	}
}

//
// D_PageTicker
// Handles timing for warped projection
//
void D_PageTicker (void)
{
	if (--pagetic < 0)
		D_AdvanceDemo ();
}

//
// D_PageDrawer
//
void D_PageDrawer (void)
{
}

//
// D_AdvanceDemo
// Called after each demo or intro demosequence finishes
//
void D_AdvanceDemo (void)
{
	advancedemo = true;
}

//
// D_DoAdvanceDemo
//
void D_DoAdvanceDemo (void)
{
}

//
// D_StartTitle
//
void D_StartTitle (void)
{
	gameaction = ga_nothing;
	demosequence = -1;
	D_AdvanceDemo ();
}

//
// CheckIWAD
//
// Tries to find an IWAD from a set of know IWAD names, and checks the first
// one found's contents to determine whether registered/commercial features
// should be executed (notably loading PWAD's).
//
static bool CheckIWAD (std::string suggestion, std::string &titlestring)
{
	static const char *doomwadnames[] =
	{
		"doom2f.wad",
		"doom2.wad",
		"plutonia.wad",
		"tnt.wad",
		"doomu.wad", // Hack from original Linux version. Not necessary, but I threw it in anyway.
		"doom.wad",
		"doom1.wad",
		"freedoom.wad",
		NULL
	};

	std::string iwad;
	int i;

	if(suggestion.length())
	{
		std::string found = BaseFileSearch(suggestion, ".WAD");

		if(found.length())
			iwad = found;
		else
		{
			if(FileExists(suggestion.c_str()))
				iwad = suggestion;
		}

		if(iwad.length())
		{
			FILE *f;

			if ( (f = fopen (iwad.c_str(), "rb")) )
			{
				wadinfo_t header;
				fread (&header, sizeof(header), 1, f);
				header.identification = LONG(header.identification);
				if (header.identification != IWAD_ID)
					iwad = "";
				fclose(f);
			}
		}
	}

	if(!iwad.length())
	{
		// Search for a pre-defined IWAD from the list above
		for (i = 0; doomwadnames[i]; i++)
		{
			std::string found = BaseFileSearch(doomwadnames[i]);

			if(found.length())
			{
				iwad = found;
				break;
			}
		}
	}

	// Now scan the contents of the IWAD to determine which one it is
	if (iwad.length())
	{
#define NUM_CHECKLUMPS 9
		static const char checklumps[NUM_CHECKLUMPS][8] = {
			"E1M1", "E2M1", "E4M1", "MAP01",
			{ 'A','N','I','M','D','E','F','S'},
			"FINAL2", "REDTNT2", "CAMO1",
			{ 'E','X','T','E','N','D','E','D'}
		};
		int lumpsfound[NUM_CHECKLUMPS];
		wadinfo_t header;
		FILE *f;

		memset (lumpsfound, 0, sizeof(lumpsfound));
		if ( (f = fopen (iwad.c_str(), "rb")) )
		{
			fread (&header, sizeof(header), 1, f);
			header.identification = LONG(header.identification);
			if (header.identification == IWAD_ID ||
				header.identification == PWAD_ID)
			{
				header.numlumps = LONG(header.numlumps);
				if (0 == fseek (f, LONG(header.infotableofs), SEEK_SET))
				{
					for (i = 0; i < header.numlumps; i++)
					{
						filelump_t lump;
						int j;

						if (0 == fread (&lump, sizeof(lump), 1, f))
							break;
						for (j = 0; j < NUM_CHECKLUMPS; j++)
							if (!strnicmp (lump.name, checklumps[j], 8))
								lumpsfound[j]++;
					}
				}
			}
			fclose (f);
		}

		gamemode = undetermined;

		if (lumpsfound[3])
		{
			gamemode = commercial;
			gameinfo = CommercialGameInfo;
			if (lumpsfound[6])
			{
				gamemission = pack_tnt;
				titlestring = "DOOM 2: TNT - Evilution";
			}
			else if (lumpsfound[7])
			{
				gamemission = pack_plut;
				titlestring = "DOOM 2: Plutonia Experiment";
			}
			else
			{
				gamemission = doom2;
				titlestring = "DOOM 2: Hell on Earth";
			}
		}
		else if (lumpsfound[0])
		{
			gamemission = doom;
			if (lumpsfound[1])
			{
				if (lumpsfound[2])
				{
					gamemode = retail;
					gameinfo = RetailGameInfo;
					titlestring = "The Ultimate DOOM";
				}
				else
				{
					gamemode = registered;
					gameinfo = RegisteredGameInfo;
					titlestring = "DOOM Registered";
				}
			}
			else
			{
				gamemode = shareware;
				gameinfo = SharewareGameInfo;
				titlestring = "DOOM Shareware";
			}
		}
	}

	if (gamemode == undetermined)
	{
		gameinfo = SharewareGameInfo;
	}

	if (iwad.length())
		wadfiles.push_back(iwad);

	return iwad.length() ? true : false;
}

//
// IdentifyVersion
//
static std::string IdentifyVersion (std::string custwad)
{
	std::string titlestring = "Public DOOM - ";

	if(!CheckIWAD (custwad, titlestring))
		Printf (PRINT_HIGH, "Game mode indeterminate, no standard wad found.\n");

	Printf (PRINT_HIGH, "%s\n", titlestring.c_str());

	return titlestring;
}

//
// denis - BaseFileSearchDir
// Check single paths for a given file with a possible extension
// Case insensitive, but returns actual file name
//
std::string BaseFileSearchDir(std::string dir, std::string file, std::string ext, std::string hash = "")
{
	std::string found;

	if(dir[dir.length() - 1] != '/')
		dir += "/";

	std::transform(hash.begin(), hash.end(), hash.begin(), toupper);
	std::string dothash = ".";
	if(hash.length())
		dothash += hash;
	else
		dothash = "";

	// denis - list files in the directory of interest, case-desensitize
	// then see if wanted wad is listed
#ifdef UNIX
	// denis - todo -find a way to deal with dir="./" and file="DIR/DIR/FILE.WAD"
	struct dirent **namelist = 0;
	int n = scandir(dir.c_str(), &namelist, 0, alphasort);

	for(int i = 0; i < n && namelist[i]; i++)
	{
		std::string d_name = namelist[i]->d_name;

		free(namelist[i]);

		if(!found.length())
		{
			if(d_name == "." || d_name == "..")
				continue;

			std::string tmp = d_name;
			std::transform(tmp.begin(), tmp.end(), tmp.begin(), toupper);

			if(file == tmp || (file + ext) == tmp || (file + dothash) == tmp || (file + ext + dothash) == tmp)
			{
				if(!hash.length() || hash == W_MD5((dir + d_name).c_str()))
					found = d_name;
			}
		}
	}

	if(namelist)
		free(namelist);
#else
	if(dir[dir.length() - 1] != '/')
		dir += "/";

	std::string all_ext = dir + "*";
	//all_ext += ext;

	WIN32_FIND_DATA FindFileData;
	HANDLE hFind = FindFirstFile(all_ext.c_str(), &FindFileData);
	DWORD dwError = GetLastError();

	if (hFind == INVALID_HANDLE_VALUE)
	{
		Printf (PRINT_HIGH, "FindFirstFile failed. GetLastError: %d\n", dwError);
		return "";
	}

	while (true)
	{
		if(!FindNextFile(hFind, &FindFileData))
		{
			dwError = GetLastError();

			if(dwError != ERROR_NO_MORE_FILES)
				Printf (PRINT_HIGH, "FindNextFile failed. GetLastError: %d\n", dwError);

			break;
		}

		if(FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			continue;

		std::string tmp = FindFileData.cFileName;
		std::transform(tmp.begin(), tmp.end(), tmp.begin(), toupper);

		if(file == tmp || (file + ext) == tmp || (file + dothash) == tmp || (file + ext + dothash) == tmp)
		{
			if(!hash.length() || hash == W_MD5((dir + FindFileData.cFileName).c_str()))
			{
				found = FindFileData.cFileName;
				break;
			}
		}
	}

	FindClose(hFind);
#endif

	return found;
}

//
// denis - BaseFileSearch
// Check all paths of interest for a given file with a possible extension
//
std::string BaseFileSearch (std::string file, std::string ext, std::string hashd)
{
	std::transform(file.begin(), file.end(), file.begin(), toupper);
	std::transform(ext.begin(), ext.end(), ext.begin(), toupper);
	std::vector<std::string> dirs;

	#ifdef WIN32
		const char separator = ';';
	#else
		const char separator = ':';
	#endif

	const char *awd = Args.CheckValue("-waddir");
	if(awd)
	{
		// search through dwd
		std::stringstream ss(awd);
		std::string segment;

		while(!ss.eof())
		{
			std::getline(ss, segment, separator);

			if(!segment.length())
				continue;

			FixPathSeparator(segment);
			I_ExpandHomeDir(segment);

			if(segment[segment.length() - 1] != '/')
				segment += "/";

			dirs.push_back(segment);
		}
	}

	const char *dwd = getenv("DOOMWADDIR");
	if(dwd)
	{
		std::string segment(dwd);

		FixPathSeparator(segment);
		I_ExpandHomeDir(segment);

		if(segment[segment.length() - 1] != '/')
			segment += "/";

		dirs.push_back(segment);
	}

	const char *dwp = getenv("DOOMWADPATH");
	if(dwp)
	{
		// search through dwd
		std::stringstream ss(dwp);
		std::string segment;

		while(!ss.eof())
		{
			std::getline(ss, segment, separator);

			if(!segment.length())
				continue;

			FixPathSeparator(segment);
			I_ExpandHomeDir(segment);

			if(segment[segment.length() - 1] != '/')
				segment += "/";

			dirs.push_back(segment);
		}
	}

	dirs.push_back(startdir);
	dirs.push_back(progdir);

	dirs.erase(std::unique(dirs.begin(), dirs.end()), dirs.end());

	for(size_t i = 0; i < dirs.size(); i++)
	{
		std::string found = BaseFileSearchDir(dirs[i], file, ext);
		if(found.length())
		{
			std::string &dir = dirs[i];

			if(dir[dir.length() - 1] != '/')
				dir += "/";

			return dir + found;
		}
	}

	// Not found
	return "";
}

//
// D_AddDefWads
//
void D_AddDefWads (std::string iwad)
{
	{
		// [RH] Make sure zdoom.wad is always loaded,
		// as it contains magic stuff we need.
		std::string wad = BaseFileSearch ("odamex.wad");
		if (wad.length())
			wadfiles.push_back(wad);
		else
			I_FatalError ("Cannot find odamex.wad");

		if (wad.length())
			wadfiles.push_back(wad);
	}

	I_SetTitleString (IdentifyVersion(iwad).c_str());

	// [RH] Add any .wad files in the skins directory
/*#ifndef UNIX // denis - fixme - 1) _findnext not implemented on linux or osx, use opendir 2) server does not need skins, does it?
	{
		char curdir[256];

		if (getcwd (curdir, 256))
		{
			char skindir[256];
			findstate_t findstate; // denis - fixme - win32 dependency == BAD!!! this is solved in later csdooms with BaseFileSearch - that could be implemented better with posix opendir stuff
			long handle;
			int stuffstart;

			std::string pd = progdir;
			if(pd[pd.length() - 1] != '/')
				pd += '/';

			stuffstart = sprintf (skindir, "%sskins", pd.c_str());

			if (!chdir (skindir))
			{
				skindir[stuffstart++] = '/';
				if ((handle = I_FindFirst ("*.wad", &findstate)) != -1)
				{
					do
					{
						if (!(I_FindAttr (&findstate) & FA_DIREC))
						{
							strcpy (skindir + stuffstart,
									I_FindName (&findstate));
							wadfiles.push_back(skindir);
						}
					} while (I_FindNext (handle, &findstate) == 0);
					I_FindClose (handle);
				}
			}

			const char *home = getenv ("HOME");
			if (home)
			{
				stuffstart = sprintf (skindir, "%s%s.odamex/skins", home,
									  home[strlen(home)-1] == '/' ? "" : "/");
				if (!chdir (skindir))
				{
					skindir[stuffstart++] = '/';
					if ((handle = I_FindFirst ("*.wad", &findstate)) != -1)
					{
						do
						{
							if (!(I_FindAttr (&findstate) & FA_DIREC))
							{
								strcpy (skindir + stuffstart,
										I_FindName (&findstate));
								wadfiles.push_back(skindir);
							}
						} while (I_FindNext (handle, &findstate) == 0);
						I_FindClose (handle);
					}
				}
			}
			chdir (curdir);
		}
	}
#endif*/

	modifiedgame = false;
}

//
// D_DoDefDehackedPatch
//
void D_DoDefDehackedPatch ()
{
	// [RH] Apply any DeHackEd patch
	{
		bool noDef = false;
		unsigned i;

		// try .deh files on command line
		DArgs files = Args.GatherFiles ("-deh", ".deh", false);
		if (files.NumArgs() > 0)
		{
			for (i = 0; i < files.NumArgs(); i++)
			{
				std::string f = BaseFileSearch (files.GetArg (i), ".DEH");
				if (f.length())
					DoDehPatch (f.c_str(), false);
			}
			noDef = true;
		}

		// try .bex files on command line
		{
			DArgs files = Args.GatherFiles ("-bex", ".bex", false);
			if (files.NumArgs() > 0)
			{
				for (i = 0; i < files.NumArgs(); i++)
				{
					printf (":%s\n", files.GetArg (i));
					std::string f = BaseFileSearch (files.GetArg (i), ".BEX");
					if (f.length())
						printf ("%s\n", f.c_str()), DoDehPatch (f.c_str(), false);
				}
				noDef = true;
			}
		}

		// try default patches
		if (!noDef)
		{
			if (FileExists (def_patch.cstring()))
				// Use patch specified by def_patch.
				DoDehPatch (def_patch.cstring(), true);
			else
				DoDehPatch (NULL, true);	// See if there's a patch in a PWAD
		}
	}
}

void SV_InitMultipleFiles (std::vector<std::string> filenames)
{
	wadnames.clear();

	for(size_t i = 0; i < filenames.size(); i++)
	{
		FixPathSeparator (filenames[i]);
		std::string name = filenames[i];
		DefaultExtension (filenames[i], ".wad");

		size_t slash = name.find_last_of('/');

		if(slash != std::string::npos)
			name = name.substr(slash + 1, name.length() - slash);

		std::transform(name.begin(), name.end(), name.begin(), toupper);

		wadnames.push_back(name);
	}
}

//
// [denis] D_DoomWadReboot
// change wads at runtime
// on 404, returns a vector of bad files
//
std::vector<size_t> D_DoomWadReboot (std::vector<std::string> wadnames)
{
	using namespace std;
	vector<size_t> fails;

	if (modifiedgame && (gameinfo.flags & GI_SHAREWARE))
		I_FatalError ("\nYou cannot switch WAD with the shareware version. Register!");

	SV_SendReconnectSignal();

	G_ExitLevel(0);
	DThinker::DestroyAllThinkers();

	Z_Init();

	gamestate = GS_STARTUP;

	wadfiles.clear();

	string custwad;
	if(wadnames.size())
		custwad = wadnames[0];

	D_AddDefWads(custwad);

	for(size_t i = 0; i < wadnames.size(); i++)
	{
		string file = BaseFileSearch(wadnames[i], ".WAD");

		if(file.length())
			wadfiles.push_back(file);
		else
		{
			Printf (PRINT_HIGH, "could not find WAD: %s\n", wadnames[i].c_str());
			fails.push_back(i);
		}
	}

	if(wadnames.size() > 1)
		modifiedgame = true;

	wadhashes = W_InitMultipleFiles (wadfiles);
	SV_InitMultipleFiles (wadfiles);

	// get skill / episode / map from parms
	strcpy (startmap, (gameinfo.flags & GI_MAPxx) ? "MAP01" : "E1M1");

	D_InitStrings ();

	D_DoDefDehackedPatch();

	G_SetLevelStrings ();
	S_ParseSndInfo();

	R_Init();
	P_Init();

	return fails;
}

//
// D_DoomMain
//
// [NightFang] - Cause I cant call ArgsSet from g_level.cpp
// [ML] 23/1/07 - Add Response file support back in
int teamplayset;

void D_DoomMain (void)
{
	M_ClearRandom();

	gamestate = GS_STARTUP;

	if (lzo_init () != LZO_E_OK)	// [RH] Initialize the minilzo package.
		I_FatalError ("Could not initialize LZO routines");

    AddCommandString("version");

	I_Init ();

	M_LoadDefaults ();			// load before initing other systems
	M_FindResponseFile();		// [ML] 23/1/07 - Add Response file support back in
	C_ExecCmdLineParams (true);	// [RH] do all +set commands on the command line

	D_CheckNetGame ();

	//D_AddDefWads();
	//SV_InitMultipleFiles (wadfiles);
	//wadhashes = W_InitMultipleFiles (wadfiles);

	// Base systems have been inited; enable cvar callbacks
	cvar_t::EnableCallbacks ();

	// [RH] Initialize configurable strings.
	D_InitStrings ();

	// [RH] User-configurable startup strings. Because BOOM does.
	if (STARTUP1[0])	Printf (PRINT_HIGH, "%s\n", STARTUP1);
	if (STARTUP2[0])	Printf (PRINT_HIGH, "%s\n", STARTUP2);
	if (STARTUP3[0])	Printf (PRINT_HIGH, "%s\n", STARTUP3);
	if (STARTUP4[0])	Printf (PRINT_HIGH, "%s\n", STARTUP4);
	if (STARTUP5[0])	Printf (PRINT_HIGH, "%s\n", STARTUP5);

	devparm = Args.CheckParm ("-devparm");

    // get skill / episode / map from parms
	strcpy (startmap, (gameinfo.flags & GI_MAPxx) ? "MAP01" : "E1M1");

	const char *val = Args.CheckValue ("-skill");
	if (val)
	{
		skill.Set (val[0]-'0');
	}

	unsigned p = Args.CheckParm ("-warp");
	if (p && p < Args.NumArgs() - (1+(gameinfo.flags & GI_MAPxx ? 0 : 1)))
	{
		int ep, map;

		if (gameinfo.flags & GI_MAPxx)
		{
			ep = 1;
			map = atoi (Args.GetArg(p+1));
		}
		else
		{
			ep = Args.GetArg(p+1)[0]-'0';
			map = Args.GetArg(p+2)[0]-'0';
		}

		strncpy (startmap, CalcMapName (ep, map), 8);
		autostart = true;
	}

	// [RH] Hack to handle +map
	p = Args.CheckParm ("+map");
	if (p && p < Args.NumArgs()-1)
	{
		strncpy (startmap, Args.GetArg (p+1), 8);
		((char *)Args.GetArg (p))[0] = '-';
		autostart = true;
	}
	if (devparm)
		Printf (PRINT_HIGH, "%s", Strings[0].builtin);	// D_DEVSTR

	const char *v = Args.CheckValue ("-timer");
	if (v)
	{
		double time = atof (v);
		Printf (PRINT_HIGH, "Levels will end after %g minute%s.\n", time, time > 1 ? "s" : "");
		timelimit.Set ((float)time);
	}

	const char *w = Args.CheckValue ("-avg");
	if (w)
	{
		Printf (PRINT_HIGH, "Austin Virtual Gaming: Levels will end after 20 minutes\n");
		timelimit.Set (20);
	}

	// Check for -file in shareware
	if (modifiedgame && (gameinfo.flags & GI_SHAREWARE))
		I_FatalError ("You cannot -file with the shareware version. Register!");

	// [RH] Initialize items. Still only used for the give command. :-(
	InitItems ();

	// [RH] Lock any cvars that should be locked now that we're
	// about to begin the game.
	cvar_t::EnableNoSet ();

	Printf(PRINT_HIGH, "========== Odamex Server Initialized ==========\n");

#ifdef UNIX
	if (Args.CheckParm("-background"))
            daemon_init();
#endif

	// Use wads mentioned on the commandline to start with
	std::vector<std::string> start_wads;

	std::string custwad;
	const char *iwadparm = Args.CheckValue ("-iwad");
	if (iwadparm)
	{
		custwad = iwadparm;
		FixPathSeparator (custwad);
		DefaultExtension (custwad, ".wad");
		start_wads.push_back(custwad);
	}

	DArgs files = Args.GatherFiles ("-file", ".wad", true);
	if (files.NumArgs() > 0)
	{
		modifiedgame = true;
		for (size_t i = 0; i < files.NumArgs(); i++)
		{
			start_wads.push_back(files.GetArg (i));
		}
	}

	D_DoomWadReboot(start_wads);

	// [RH] Now that all game subsystems have been initialized,
	// do all commands on the command line other than +set
	C_ExecCmdLineParams (false);

	strncpy(level.mapname, startmap, sizeof(level.mapname));

	G_ChangeMap ();

	D_DoomLoop (); // never returns
}

VERSION_CONTROL (d_main_cpp, "$Id$")

