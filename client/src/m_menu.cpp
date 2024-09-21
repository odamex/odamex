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
//		DOOM selection menu, options, episode etc.
//		Sliders and icons. Kinda widget stuff.
//
//-----------------------------------------------------------------------------


#include "odamex.h"

#include <ctime>

#include "gstrings.h"
#include "c_console.h"
#include "c_dispatch.h"
#include "d_main.h"
#include "i_music.h"
#include "i_video.h"
#include "z_zone.h"
#include "v_video.h"
#include "w_wad.h"
#include "r_local.h"
#include "hu_stuff.h"
#include "g_episode.h"
#include "g_game.h"
#include "m_random.h"
#include "s_sound.h"
#include "m_menu.h"
#include "v_text.h"
#include "st_stuff.h"
#include "p_ctf.h"
#include "r_sky.h"
#include "cl_main.h"
#include "c_bind.h"
#include "cl_responderkeys.h"

#include "gi.h"
#include "g_skill.h"
#include "m_fileio.h"

#ifdef _XBOX
#include "i_xbox.h"
#endif

EXTERN_CVAR(g_resetinvonexit)

// temp for screenblocks (0-9)
int 				screenSize;

// -1 = no quicksave slot picked!
int 				quickSaveSlot;

 // 1 = message to be printed
int 				messageToPrint;
// ...and here is the message string!
const char*				messageString;

// message x & y
int 				messx;
int 				messy;
int 				messageLastMenuActive;

// timed message = no input from user
bool				messageNeedsInput;

void	(*messageRoutine)(int response);
void	CL_SendUserInfo();
void	M_ChangeTeam (int choice);
team_t D_TeamByName (const char *team);
gender_t D_GenderByName (const char *gender);
colorpreset_t D_ColorPreset (const char *colorpreset);

#define SAVESTRINGSIZE	24

// we are going to be entering a savegame string
int 				genStringEnter;
int					genStringLen;	// [RH] Max # of chars that can be entered
void	(*genStringEnd)(int slot);
int 				saveSlot;		// which slot to save in
int 				saveCharIndex;	// which char we're editing
// old save description before edit
char				saveOldString[SAVESTRINGSIZE];

BOOL 				menuactive;

int                 repeatKey;
int                 repeatCount;

extern bool			sendpause;
char				savegamestrings[10][SAVESTRINGSIZE];

menustack_t			MenuStack[16];
int					MenuStackDepth;

short				itemOn; 			// menu item skull is on
short				skullAnimCounter;	// skull animation counter
short				whichSkull; 		// which skull to draw
bool				drawSkull;			// [RH] don't always draw skull

// hack for PlayerSetup
int					PSetupDepth;

// graphic name of skulls
char				skullName[2][9] = {"M_SKULL1", "M_SKULL2"};

// current menudef
oldmenu_t *currentMenu;

//
// PROTOTYPES
//
void M_NewGame(int choice);
void M_Episode(int choice);
void M_Expansion(int choice);
void M_ChooseSkill(int choice);
void M_LoadGame(int choice);
void M_SaveGame(int choice);
void M_Options(int choice);
void M_EndGame(int choice);
void M_ReadThis(int choice);
void M_ReadThis2(int choice);
void M_ReadThis3(int choice);
void M_QuitDOOM(int choice);

void M_ChangeDetail(int choice);
void M_StartGame(int choice);
void M_Sound(int choice);

void M_FinishReadThis(int choice);
void M_LoadSelect(int choice);
void M_SaveSelect(int choice);
void M_ReadSaveStrings(void);
void M_QuickSave(void);
void M_QuickLoad(void);

void M_DrawMainMenu(void);
void M_DrawReadThis1(void);
void M_DrawReadThis2(void);
void M_DrawReadThis3(void);
void M_DrawNewGame(void);
void M_DrawEpisode(void);
void M_DrawOptions(void);
void M_DrawSound(void);
void M_DrawLoad(void);
void M_DrawSave(void);

void M_DrawSaveLoadBorder(int x,int y, int len);
void M_SetupNextMenu(oldmenu_t *menudef);
void M_DrawEmptyCell(oldmenu_t *menu,int item);
void M_DrawSelCell(oldmenu_t *menu,int item);
int  M_StringHeight(char *string);
void M_StartControlPanel(void);
void M_StartMessage(const char *string,void (*routine)(int),bool input);
void M_StopMessage(void);
void M_ClearMenus (void);

// [RH] For player setup menu.
static void M_PlayerSetupTicker (void);
static void M_PlayerSetupDrawer (void);
static void M_EditPlayerName (int choice);
//static void M_EditPlayerTeam (int choice);
//static void M_PlayerTeamChanged (int choice);
static void M_PlayerNameChanged (int choice);
static void M_ChangeGender (int choice);
static void M_ChangeAutoAim (int choice);
static void M_ChangeColorPreset (int choice);
static void SendNewColor (int red, int green, int blue);
static void M_SlidePlayerRed (int choice);
static void M_SlidePlayerGreen (int choice);
static void M_SlidePlayerBlue (int choice);
bool M_DemoNoPlay;

static IWindowSurface* fire_surface;
static const int fire_surface_width = 72;
static const int fire_surface_height = 77;

//
// DOOM MENU
//
enum d1_main_t
{
	d1_newgame = 0,
	d1_options,					// [RH] Moved
	d1_loadgame,
	d1_savegame,
	d1_readthis,
	d1_quitdoom,
	d1_main_end
}d1_main_e;

oldmenuitem_t DoomMainMenu[]=
{
	{1,"M_NGAME",M_NewGame,'N'},
	{1,"M_OPTION",M_Options,'O'},	// [RH] Moved
    {1,"M_LOADG",M_LoadGame,'L'},
    {1,"M_SAVEG",M_SaveGame,'S'},
    {1,"M_RDTHIS",M_ReadThis,'R'},
	{1,"M_QUITG",M_QuitDOOM,'Q'}
};

//
// DOOM 2 MENU
//

enum d2_main_t
{
	d2_newgame = 0,
	d2_options,					// [RH] Moved
	d2_loadgame,
	d2_savegame,
	d2_quitdoom,
	d2_main_end
}d2_main_e;

oldmenuitem_t Doom2MainMenu[]=
{
	{1,"M_NGAME",M_NewGame,'N'},
	{1,"M_OPTION",M_Options,'O'},	// [RH] Moved
    {1,"M_LOADG",M_LoadGame,'L'},
    {1,"M_SAVEG",M_SaveGame,'S'},
	{1,"M_QUITG",M_QuitDOOM,'Q'}
};


// Default used is the Doom Menu
oldmenu_t MainDef =
{
	d1_main_end,
	DoomMainMenu,
	M_DrawMainMenu,
	97,64,
	0
};



//
// EPISODE SELECT
//

oldmenuitem_t EpisodeMenu[MAX_EPISODES] =
{
	{1,"\0", M_Episode,0},
	{1,"\0", M_Episode,0},
	{1,"\0", M_Episode,0},
	{1,"\0", M_Episode,0},
	{1,"\0", M_Episode,0},
	{1,"\0", M_Episode,0},
	{1,"\0", M_Episode,0},
	{1,"\0", M_Episode,0}
};

oldmenu_t EpiDef =
{
	0,
	EpisodeMenu,		// oldmenuitem_t ->
	M_DrawEpisode,		// drawing routine ->
	48,63,				// x,y
	0	 				// lastOn
};

int epi;

//
// EXPANSION SELECT (DOOM2 BFG)
//
enum expansions_t
{
	hoe,
	nrftl,
	exp_end
} expansions_e;

oldmenuitem_t ExpansionMenu[]=
{
	{1,"M_EPI1", M_Expansion,'h'},
	{1,"M_EPI2", M_Expansion,'n'},
};

oldmenu_t ExpDef =
{
	exp_end,	 		// # of menu items
	ExpansionMenu,		// oldmenuitem_t ->
	M_DrawEpisode,		// drawing routine ->
	48,63,				// x,y
	hoe 				// lastOn
};


//
// NEW GAME
//

oldmenuitem_t NewGameMenu[MAX_SKILLS + 1]=
{
	{1,"\0", M_ChooseSkill,0},
	{1,"\0", M_ChooseSkill,0},
	{1,"\0", M_ChooseSkill,0},
	{1,"\0", M_ChooseSkill,0},
	{1,"\0", M_ChooseSkill,0},
	{1,"\0", M_ChooseSkill,0},
	{1,"\0", M_ChooseSkill,0},
	{1,"\0", M_ChooseSkill,0},
	//{1,"\0", M_ChooseSkill,0}
};

oldmenu_t NewDef =
{
	0,			// # of menu items
	NewGameMenu,		// oldmenuitem_t ->
	M_DrawNewGame,		// drawing routine ->
	48,63,				// x,y
	0				// lastOn
};

//
// [RH] Player Setup Menu
//
byte FireRemap[256];

enum psetup_t
{
	playername,
	playerteam,
	playersex,
	playeraim,
	playercolorpreset,
	playerred,
	playergreen,
	playerblue,
	psetup_end
} psetup_e;

oldmenuitem_t PlayerSetupMenu[] =
{
	{ 1,"", M_EditPlayerName, 'N' },
	{ 2,"", M_ChangeTeam, 'T' },
	{ 2,"", M_ChangeGender, 'E' },
	{ 2,"", M_ChangeAutoAim, 'A' },
    { 2,"", M_ChangeColorPreset, 'C' },
	{ 2,"", M_SlidePlayerRed, 'R' },
	{ 2,"", M_SlidePlayerGreen, 'G' },
	{ 2,"", M_SlidePlayerBlue, 'B' }
};

oldmenu_t PSetupDef = {
	psetup_end,
	PlayerSetupMenu,
	M_PlayerSetupDrawer,
	48,	47,
	playername
};

//
// OPTIONS MENU
//
// [RH] This menu is now handled in m_options.c
//
bool OptionsActive;

oldmenu_t OptionsDef =
{
	0,
	NULL,
	NULL,
	0,0,
	0
};


//
// Read This!
//
enum read_t
{
	rdthsempty1,
	read1_end
} read_e;

oldmenuitem_t ReadMenu1[] =
{
	{1,"",M_ReadThis2,0}
};

oldmenu_t	ReadDef1 =
{
	read1_end,
	ReadMenu1,
	M_DrawReadThis1,
	280,185,
	0
};

enum read_t2
{
	rdthsempty2,
	read2_end
} read_e2;

oldmenuitem_t ReadMenu2[]=
{
	{1,"",M_ReadThis3,0}
};

oldmenu_t ReadDef2 =
{
	read2_end,
	ReadMenu2,
	M_DrawReadThis2,
	330,175,
	0
};

enum read_t3
{
	rdthsempty3,
	read3_end
} read_e3;


oldmenuitem_t ReadMenu3[]=
{
	{1,"",M_FinishReadThis,0}
};

oldmenu_t ReadDef3 =
{
	read3_end,
	ReadMenu3,
	M_DrawReadThis3,
	330,175,
	0
};

//
// LOAD GAME MENU
//
enum load_t
{
	load1,
	load2,
	load3,
	load4,
	load5,
	load6,
	load7,
	load8,
	load_end
} load_e;

static oldmenuitem_t LoadSavegameMenu[]=
{
	{1,"", M_LoadSelect,'1'},
	{1,"", M_LoadSelect,'2'},
	{1,"", M_LoadSelect,'3'},
	{1,"", M_LoadSelect,'4'},
	{1,"", M_LoadSelect,'5'},
	{1,"", M_LoadSelect,'6'},
	{1,"", M_LoadSelect,'7'},
	{1,"", M_LoadSelect,'8'},
};

oldmenu_t LoadDef =
{
	load_end,
	LoadSavegameMenu,
	M_DrawLoad,
	80,54,
	0
};

//
// SAVE GAME MENU
//
oldmenuitem_t SaveMenu[]=
{
	{1,"", M_SaveSelect,'1'},
	{1,"", M_SaveSelect,'2'},
	{1,"", M_SaveSelect,'3'},
	{1,"", M_SaveSelect,'4'},
	{1,"", M_SaveSelect,'5'},
	{1,"", M_SaveSelect,'6'},
	{1,"", M_SaveSelect,'7'},
	{1,"", M_SaveSelect,'8'}
};

oldmenu_t SaveDef =
{
	load_end,
	SaveMenu,
	M_DrawSave,
	80,54,
	0
};

// [RH] Most menus can now be accessed directly
// through console commands.
BEGIN_COMMAND (menu_main)
{
    S_Sound (CHAN_INTERFACE, "switches/normbutn", 1, ATTN_NONE);
	M_StartControlPanel ();
	M_SetupNextMenu (&MainDef);
	PSetupDepth = 2;
}
END_COMMAND (menu_main)

BEGIN_COMMAND (menu_help)
{
    // F1
    S_Sound (CHAN_INTERFACE, "switches/normbutn", 1, ATTN_NONE);
	M_StartControlPanel ();
	M_ReadThis(0);
}
END_COMMAND (menu_help)

BEGIN_COMMAND (menu_save)
{
    // F2
	S_Sound (CHAN_INTERFACE, "switches/normbutn", 1, ATTN_NONE);
	M_StartControlPanel ();
	M_SaveGame (0);
	//Printf (PRINT_WARNING, "Saving is not available at this time.\n");
}
END_COMMAND (menu_save)

BEGIN_COMMAND (menu_load)
{
    // F3
	S_Sound (CHAN_INTERFACE, "switches/normbutn", 1, ATTN_NONE);
	M_StartControlPanel ();
	M_LoadGame (0);
	//Printf (PRINT_WARNING, "Loading is not available at this time.\n");
}
END_COMMAND (menu_load)

BEGIN_COMMAND (menu_options)
{
    // F4
    S_Sound (CHAN_INTERFACE, "switches/normbutn", 1, ATTN_NONE);
    M_StartControlPanel ();
	M_Options(0);
	PSetupDepth = 1;
}
END_COMMAND (menu_options)

BEGIN_COMMAND (quicksave)
{
    // F6
	S_Sound (CHAN_INTERFACE, "switches/normbutn", 1, ATTN_NONE);
	M_StartControlPanel ();
	M_QuickSave ();
	//Printf (PRINT_WARNING, "Saving is not available at this time.\n");
}
END_COMMAND (quicksave)

BEGIN_COMMAND (menu_endgame)
{	// F7
    S_Sound (CHAN_INTERFACE, "switches/normbutn", 1, ATTN_NONE);
	M_StartControlPanel ();
	M_EndGame(0);
}
END_COMMAND (menu_endgame)

BEGIN_COMMAND (quickload)
{
    // F9
	S_Sound (CHAN_INTERFACE, "switches/normbutn", 1, ATTN_NONE);
	M_StartControlPanel ();
	M_QuickLoad ();
	//Printf (PRINT_WARNING, "Loading is not available at this time.\n");
}
END_COMMAND (quickload)

BEGIN_COMMAND (menu_quit)
{	// F10
	S_Sound (CHAN_INTERFACE, "switches/normbutn", 1, ATTN_NONE);
	M_StartControlPanel ();
	M_QuitDOOM(0);
}
END_COMMAND (menu_quit)

BEGIN_COMMAND (menu_player)
{
    S_Sound (CHAN_INTERFACE, "switches/normbutn", 1, ATTN_NONE);
	M_StartControlPanel ();
	M_PlayerSetup(0);
	PSetupDepth = 0;
}
END_COMMAND (menu_player)

/*
void M_LoadSaveResponse(int choice)
{
    // dummy
}


void M_LoadGame (int choice)
{
    M_StartMessage("Loading/saving is not supported\n\n(Press any key to "
                   "continue)\n", M_LoadSaveResponse, false);
}
*/

//
// M_ReadSaveStrings
//	read the strings from the savegame files
//
void M_ReadSaveStrings(void)
{
	FILE *handle;
	int i;

	for (i = 0; i < load_end; i++)
	{
		std::string name;

		G_BuildSaveName (name, i);

		handle = fopen (name.c_str(), "rb");
		if (handle == NULL)
		{
			strcpy (&savegamestrings[i][0], GStrings(EMPTYSTRING));
			LoadSavegameMenu[i].status = 0;
		}
		else
		{
			const size_t readlen = fread (&savegamestrings[i], SAVESTRINGSIZE, 1, handle);
			if (readlen < 1)
			{
				printf("M_Read_SaveStrings(): Failed to read handle.\n");
				fclose(handle);
				return;
			}
			fclose (handle);
			LoadSavegameMenu[i].status = 1;
		}
	}
}


//
// M_LoadGame & Cie.
//
void M_DrawLoad (void)
{
	int i;

	screen->DrawPatchClean ((patch_t *)W_CachePatch("M_LOADG"), 72, 28);
	for (i = 0; i < load_end; i++)
	{
		M_DrawSaveLoadBorder (LoadDef.x, LoadDef.y+LINEHEIGHT*i, 24);
		screen->DrawTextCleanMove (CR_RED, LoadDef.x, LoadDef.y+LINEHEIGHT*i, savegamestrings[i]);
	}
}

//
// User wants to load this game
//
void M_LoadSelect (int choice)
{
	std::string name;

	G_BuildSaveName (name, choice);
	G_LoadGame ((char *)name.c_str());
	gamestate = gamestate == GS_FULLCONSOLE ? GS_HIDECONSOLE : gamestate;
	M_ClearMenus ();
	if (quickSaveSlot == -2)
	{
		quickSaveSlot = choice;
	}
}

//
// Selected from DOOM menu
// [ML] 7 Sept 08: Bringing game saving/loading in from
//                 zdoom 1.22 source, see MAINTAINERS
//
void M_LoadGame (int choice)
{
	M_SetupNextMenu (&LoadDef);
	M_ReadSaveStrings ();
}

//
//	M_SaveGame & Cie.
// [ML] 7 Sept 08: Bringing game saving/loading in from
//                 zdoom 1.22 source, see MAINTAINERS
//
void M_DrawSave(void)
{
	int i;

	screen->DrawPatchClean ((patch_t *)W_CachePatch("M_SAVEG"), 72, 28);
	for (i = 0; i < load_end; i++)
	{
		M_DrawSaveLoadBorder(LoadDef.x,LoadDef.y+LINEHEIGHT*i,24);
		screen->DrawTextCleanMove (CR_RED, LoadDef.x, LoadDef.y+LINEHEIGHT*i, savegamestrings[i]);
	}

	if (genStringEnter)
	{
		i = V_StringWidth(savegamestrings[saveSlot]);
		screen->DrawTextCleanMove (CR_RED, LoadDef.x + i, LoadDef.y+LINEHEIGHT*saveSlot, "_");
	}
}


//
// M_Responder calls this when user is finished
// [ML] 7 Sept 08: Bringing game saving/loading in from
//                 zdoom 1.22 source, see MAINTAINERS
//
void M_DoSave (int slot)
{
	G_SaveGame (slot,savegamestrings[slot]);
	M_ClearMenus ();
		// PICK QUICKSAVE SLOT YET?
	if (quickSaveSlot == -2)
		quickSaveSlot = slot;
}

//
// User wants to save. Start string input for M_Responder
// [ML] 7 Sept 08: Bringing game saving/loading in from
//                 zdoom 1.22 source, see MAINTAINERS
//
void M_SaveSelect (int choice)
{
	const time_t     ti = time(NULL);
	const struct tm *lt = localtime(&ti);

	// we are going to be intercepting all chars
	genStringEnter = 1;
	genStringEnd = M_DoSave;
	genStringLen = SAVESTRINGSIZE-1;

	saveSlot = choice;
	strcpy(saveOldString,savegamestrings[choice]);

	// If on a game console, auto-fill with date and time to save name

#ifndef GCONSOLE
	if (!LoadSavegameMenu[choice].status)
#endif
	{
		strncpy(savegamestrings[choice], asctime(lt) + 4, 20);
	}

	saveCharIndex = strlen(savegamestrings[choice]);
}

//
// Selected from DOOM menu
// [ML] 7 Sept 08: Bringing game saving/loading in from
//                 zdoom 1.22 source, see MAINTAINERS
//
void M_SaveGame (int choice)
{
	if (multiplayer && !demoplayback)
	{
		M_StartMessage("you can't save while in a net game!\n\npress a key.",
			NULL,false);
		M_ClearMenus ();
		return;
	}

	if (!usergame)
	{
		M_StartMessage(GStrings(SAVEDEAD),NULL,false);
		M_ClearMenus ();
		return;
	}

	if (gamestate != GS_LEVEL)
		return;

	M_SetupNextMenu(&SaveDef);
	M_ReadSaveStrings();
}


//
//		M_QuickSave
// [ML] 7 Sept 08: Bringing game saving/loading in from
//                 zdoom 1.22 source, see MAINTAINERS
//
char	tempstring[80];

void M_QuickSaveResponse(int ch)
{
	if (ch == 'y' || Key_IsYesKey(ch))
	{
		M_DoSave (quickSaveSlot);
		S_Sound (CHAN_INTERFACE, "switches/exitbutn", 1, ATTN_NONE);
	}
}

void M_QuickSave(void)
{
	if (multiplayer)
	{
		S_Sound (CHAN_INTERFACE, "player/male/grunt1", 1, ATTN_NONE);
		M_ClearMenus ();
		return;
	}

	if (!usergame)
	{
		S_Sound (CHAN_INTERFACE, "player/male/grunt1", 1, ATTN_NONE);
		M_ClearMenus ();
		return;
	}

	if (gamestate != GS_LEVEL)
		return;

	if (quickSaveSlot < 0)
	{
		M_StartControlPanel();
		M_ReadSaveStrings();
		M_SetupNextMenu(&SaveDef);
		quickSaveSlot = -2; 	// means to pick a slot now
		return;
	}
	sprintf (tempstring, GStrings(QSPROMPT), savegamestrings[quickSaveSlot]);
	M_StartMessage (tempstring, M_QuickSaveResponse, true);
}



//
// M_QuickLoad
// [ML] 7 Sept 08: Bringing game saving/loading in from
//                 zdoom 1.22 source, see MAINTAINERS
//
void M_QuickLoadResponse(int ch)
{
	if (ch == 'y' || Key_IsYesKey(ch))
	{
		M_LoadSelect(quickSaveSlot);
		S_Sound (CHAN_INTERFACE, "switches/exitbutn", 1, ATTN_NONE);
	}
}


void M_QuickLoad(void)
{
	if (quickSaveSlot < 0)
	{
		M_StartControlPanel();
		M_LoadGame (0);
		return;
	}
	sprintf(tempstring,GStrings(QLPROMPT),savegamestrings[quickSaveSlot]);
	M_StartMessage(tempstring,M_QuickLoadResponse,true);
}


//
// M_ReadThis
//
void M_ReadThis(int choice)
{
	choice = 0;
	drawSkull = false;
	M_SetupNextMenu(&ReadDef1);
}

void M_ReadThis2(int choice)
{
	choice = 0;
	drawSkull = false;
	M_SetupNextMenu(&ReadDef2);
}

void M_ReadThis3(int choice)
{
    if (gameinfo.flags & GI_SHAREWARE) {
        choice = 0;
        drawSkull = false;
        M_SetupNextMenu(&ReadDef3);
    } else {
        M_FinishReadThis(0);
    }
}

void M_FinishReadThis(int choice)
{
	choice = 0;
	drawSkull = true;
	MenuStackDepth = 0;
	M_SetupNextMenu(&MainDef);
}

//
// Draw border for the savegame description
// [RH] Width of the border is variable
//
void M_DrawSaveLoadBorder (int x, int y, int len)
{
	int i;

	screen->DrawPatchClean (W_CachePatch ("M_LSLEFT"), x-8, y+7);

	for (i = 0; i < len; i++)
	{
		screen->DrawPatchClean (W_CachePatch ("M_LSCNTR"), x, y+7);
		x += 8;
	}

	screen->DrawPatchClean (W_CachePatch ("M_LSRGHT"), x, y+7);
}

//
// M_DrawMainMenu
//
void M_DrawMainMenu()
{
	screen->DrawPatchClean (W_CachePatch("M_DOOM"), 94, 2);
}

void M_DrawNewGame()
{
	screen->DrawPatchClean((patch_t*)W_CachePatch("M_NEWG"), 96, 14);
	screen->DrawPatchClean((patch_t*)W_CachePatch("M_SKILL"), 54, 38);

	const int SMALLFONT_OFFSET = 8; // Line up with the skull

	const char* pslabel = "Pistol Start Each Level ";
	const int psy = NewDef.y + (LINEHEIGHT * skillnum) + SMALLFONT_OFFSET;

	screen->DrawTextCleanMove(CR_RED, NewDef.x, psy, pslabel);
	screen->DrawTextCleanMove(CR_GREY, NewDef.x + V_StringWidth(pslabel), psy,
	                          g_resetinvonexit ? "ON" : "OFF");
}

namespace
{
	void SetupEpisodeList()
	{
		for (int i = 0; i < episodenum; ++i)
		{
			if (EpisodeInfos[i].fulltext)
			{
				// Not implemented
			}
			else
			{
				strncpy(EpisodeMenu[i].name, EpisodeInfos[i].name.c_str(), 8);
			}

			EpisodeMenu[i].alphaKey = EpisodeInfos[i].key;
		}
	}

	void SetupSkillList()
	{
	    NewDef.lastOn = defaultskillmenu;

	    int i = 0;
	    for (; i < skillnum; ++i)
	    {
		    if (SkillInfos[i].pic_name.empty())
		    {
			    strncpy(NewGameMenu[i].name, SkillInfos[i].menu_name.c_str(), 8);
		    }
		    else
		    {
			    strncpy(NewGameMenu[i].name, SkillInfos[i].pic_name.c_str(), 8);
		    }

			NewGameMenu[i].alphaKey = SkillInfos[i].shortcut;
	    }

		strncpy(NewGameMenu[i].name, "\0", 1);
	    NewGameMenu[i].alphaKey = 'p';
	}
}

void M_NewGame(int choice)
{
	if (gamemode == commercial_bfg)
    {
        M_SetupNextMenu(&ExpDef);
    }
	else
	{
		EpiDef.numitems = episodenum;

		// Set up episode menu positioning
		EpiDef.x = 48;
		EpiDef.y = 63;

		if (episodenum > 4)
		{
			EpiDef.y -= LINEHEIGHT;
		}

		epi = 0;

		NewDef.numitems = skillnum + 1;

		if (episodenum > 1)
		{
			SetupEpisodeList();
			M_SetupNextMenu(&EpiDef);
		}
		else
		{
			SetupSkillList();
			M_SetupNextMenu(&NewDef);
		}
	}
}


//
//		M_Episode
//

void M_DrawEpisode()
{
	int y = 38;

	if (episodenum > 4)
	{
		y -= (LINEHEIGHT * (episodenum - 4));
	}
		
	screen->DrawPatchClean(W_CachePatch("M_EPISOD"), 54, y);
}

static int skillchoice = 0;

void M_VerifyNightmare(int ch)
{
	if (ch != 'y' && !Key_IsYesKey(ch))
	{
	    M_ClearMenus();
		return;
	}

	M_StartGame(skillchoice);
}

void M_StartGame(int choice)
{
	sv_skill.Set (static_cast<float>(choice + 1));
	sv_gametype = GM_COOP;

    if (gamemode == commercial_bfg)     // Funky external loading madness fun time (DOOM 2 BFG)
    {
	    const std::string str = "nerve.wad";

        if (epi)
        {
            // Load No Rest for The Living Externally
            epi = 0;
            G_LoadWadString(str);
        }
        else
        {
            // Check for nerve.wad, if it's loaded re-load with just iwad (DOOM 2 BFG)
            for (unsigned int i = 2; i < wadfiles.size(); i++)
            {
                if (iequals(str, wadfiles[i].getBasename()))
                {
                    G_LoadWadString(wadfiles[1].getFullpath());
                }
            }

            G_DeferedInitNew (CalcMapName (epi+1, 1));
        }
    }
    else
    {
        G_DeferedInitNew (EpisodeMaps[epi].c_str());
    }

    M_ClearMenus ();
}

void M_ChooseSkill(int choice)
{
	if (choice == skillnum)
	{
		g_resetinvonexit = !g_resetinvonexit;
		return;
	}
	else if (SkillInfos[choice].must_confirm)
	{
		const char* must_confirm_text = SkillInfos[choice].must_confirm_text.c_str();

		if (must_confirm_text[0] == '$')
			M_StartMessage(GStrings(StdStringToUpper(must_confirm_text + 1)),
		               M_VerifyNightmare, true);
		else
			M_StartMessage(must_confirm_text, M_VerifyNightmare, true);

		skillchoice = choice;

		return;
	}

	M_StartGame(choice);
}

void M_Episode(int choice)
{
	if ((gameinfo.flags & GI_SHAREWARE) && choice)
	{
		M_StartMessage(GStrings(SWSTRING),NULL,false);
		//M_SetupNextMenu(&ReadDef1);
		M_ClearMenus();
		return;
	}

	epi = choice;

	if (EpisodeInfos[epi].noskillmenu)
		M_StartGame(defaultskillmenu);
	else
	{
		SetupSkillList();
		M_SetupNextMenu(&NewDef);
	}
}

void M_Expansion(int choice)
{
	epi = choice;
	SetupSkillList();
	M_SetupNextMenu(&NewDef);
}

//
// Read This Menus
// Had a "quick hack to fix romero bug"
//
void M_DrawReadThis1()
{
	const patch_t *p = W_CachePatch(gameinfo.info.infoPage[0]);
	screen->DrawPatchFullScreen(p, false);
}

//
// Read This Menus - optional second page.
//
void M_DrawReadThis2()
{
	const patch_t *p = W_CachePatch(gameinfo.info.infoPage[1]);
	screen->DrawPatchFullScreen(p, false);
}

//
// Read This Menus - shareware third page.
//
void M_DrawReadThis3()
{
	const patch_t *p = W_CachePatch(gameinfo.info.infoPage[2]);
	screen->DrawPatchFullScreen(p, false);
}

//
// M_Options
//
void M_DrawOptions()
{
	screen->DrawPatchClean (W_CachePatch("M_OPTTTL"), 108, 15);
}

void M_Options(int choice)
{
	//M_SetupNextMenu(&OptionsDef);
	OptionsActive = M_StartOptionsMenu();
}

//
// M_EndGame
//
void M_EndGameResponse(int ch)
{
	if ((!isascii(ch) || toupper(ch) != 'Y') && !Key_IsYesKey(ch))
	{
	    M_ClearMenus ();
		return;
	}

	currentMenu->lastOn = itemOn;
	M_ClearMenus ();
	D_StartTitle ();
	CL_QuitNetGame(NQ_SILENT);
}

void M_EndGame(int choice)
{
	choice = 0;
	if (!usergame)
	{
		S_Sound (CHAN_INTERFACE, "player/male/grunt1", 1, ATTN_NONE);
		return;
	}

	M_StartMessage(GStrings(multiplayer ? NETEND : ENDGAME), M_EndGameResponse, true);
}

//
// M_QuitDOOM
//

void STACK_ARGS call_terms();

void M_QuitResponse(int ch)
{
	if ((!isascii(ch) || toupper(ch) != 'Y') && !Key_IsYesKey(ch))
	{
	    M_ClearMenus ();
		return;
	}

	// Stop the music so we do not get stuck notes
	I_StopSong();
	if (snd_musicsystem.asInt() == MS_PORTMIDI)
		I_ShutdownMusic();

	if (!multiplayer)
	{
		if (gameinfo.quitSound[0])
		{
			S_Sound(CHAN_INTERFACE, gameinfo.quitSound, 1, ATTN_NONE);
			I_WaitVBL (105);
		}
	}

    call_terms();

	exit(EXIT_SUCCESS);
}

void M_QuitDOOM(int choice)
{
	static std::string endstring;

	// We pick index 0 which is language sensitive,
	//  or one at random, between 1 and maximum number.
	StrFormat(endstring, "%s\n\n%s",
	          GStrings.getIndex(GStrings.toIndex(QUITMSG) + (gametic % NUM_QUITMESSAGES)),
	          GStrings(DOSY));

	M_StartMessage(endstring.c_str(), M_QuitResponse, true);
}

// -----------------------------------------------------
//		Player Setup Menu code
// -----------------------------------------------------

void M_DrawSlider(int x, int y, float leftval, float rightval, float cur, float step);

static const char *genders[3] = { "male", "female", "cyborg" };
// Acts 19 quiz the order must match d_netinf.h
static const char *colorpresets[11] = { "custom", "blue", "indigo", "green", "brown", "red", "gold", "jungle green", "purple", "white", "black" };
static state_t *PlayerState;
static int PlayerTics;
argb_t CL_GetPlayerColor(player_t*);


EXTERN_CVAR (cl_name)
EXTERN_CVAR (cl_team)
EXTERN_CVAR (cl_colorpreset)
EXTERN_CVAR (cl_customcolor)
EXTERN_CVAR (cl_color)
EXTERN_CVAR (cl_gender)
EXTERN_CVAR (cl_autoaim)

void M_PlayerSetup(int choice)
{
	strcpy(savegamestrings[0], cl_name.cstring());
	M_SetupNextMenu (&PSetupDef);
	PlayerState = &states[mobjinfo[MT_PLAYER].seestate];
	PlayerTics = PlayerState->tics;

	if (fire_surface == NULL)
		fire_surface = I_AllocateSurface(fire_surface_width, fire_surface_height, 8);

	// [Nes] Intialize the player preview color.
	const argb_t player_color = CL_GetPlayerColor(&consoleplayer());
	R_BuildPlayerTranslation(0, player_color);
}

static void M_PlayerSetupTicker()
{
	// Based on code in f_finale.c
	if (--PlayerTics > 0)
		return;

	if (PlayerState->tics == -1 || PlayerState->nextstate == S_NULL)
		PlayerState = &states[mobjinfo[MT_PLAYER].seestate];
	else
		PlayerState = &states[PlayerState->nextstate];
	PlayerTics = PlayerState->tics;
}

template<typename PIXEL_T>
static forceinline PIXEL_T R_FirePixel(const byte c);

template<>
forceinline byte R_FirePixel<byte>(const byte c)
{
	return FireRemap[c];
}

template<>
forceinline argb_t R_FirePixel<argb_t>(const byte c)
{
	return V_GammaCorrect(argb_t(c, 0, 0));
}

template<int xscale, typename PIXEL_T>
static forceinline void R_RenderFire(int x, int y)
{
	IWindowSurface* surface = I_GetPrimarySurface();
	int surface_pitch = surface->getPitchInPixels();

	fire_surface->lock();

	for (int b = 0; b < fire_surface_height; b++)
	{
		PIXEL_T* to = (PIXEL_T*)surface->getBuffer() + y * surface_pitch + x;
		const palindex_t* from = (palindex_t*)fire_surface->getBuffer() + b * fire_surface->getPitch();
		y += CleanYfac;

		for (int a = 0; a < fire_surface_width; a++, to += xscale, from++)
		{
			for (int c = CleanYfac; c; c--)
			{
				for (int i = 0; i < xscale; ++i)
					*(to + surface_pitch * c + i) = R_FirePixel<PIXEL_T>(*from);
			}
		}
	}

	fire_surface->unlock();
}

static void M_PlayerSetupDrawer()
{
	const int x1 = (I_GetSurfaceWidth() / 2) - (160 * CleanXfac);
	const int y1 = (I_GetSurfaceHeight() / 2) - (100 * CleanYfac);

	const int x2 = (I_GetSurfaceWidth() / 2) + (160 * CleanXfac);
	const int y2 = (I_GetSurfaceHeight() / 2) + (100 * CleanYfac);

	int colorpreset = D_ColorPreset(cl_colorpreset.cstring());

	// Background effect
	OdamexEffect(x1,y1,x2,y2);

	// Draw title
	{
		const patch_t *patch = W_CachePatch ("M_PSTTL");
        screen->DrawPatchClean (patch, 160-patch->width()/2, 10);

		/*screen->DrawPatchClean (patch,
			160 - (patch->width() >> 1),
			PSetupDef.y - (patch->height() * 3));*/
	}

	// Draw player name box
	screen->DrawTextCleanMove (CR_RED, PSetupDef.x, PSetupDef.y, "Name");
	M_DrawSaveLoadBorder (PSetupDef.x + 56, PSetupDef.y, MAXPLAYERNAME+1);
	screen->DrawTextCleanMove (CR_RED, PSetupDef.x + 56, PSetupDef.y, savegamestrings[0]);

	// Draw cursor for either of the above
	if (genStringEnter)
		screen->DrawTextCleanMove(CR_RED, PSetupDef.x + V_StringWidth(savegamestrings[saveSlot]) + 56,
							PSetupDef.y + ((saveSlot == 0) ? 0 : LINEHEIGHT), "_");

	// Draw player character
	{
		int x = 320 - 88 - 32, y = PSetupDef.y + LINEHEIGHT*3 - 14;

		x = (x-160)*CleanXfac+(I_GetSurfaceWidth() / 2);
		y = (y-100)*CleanYfac+(I_GetSurfaceHeight() / 2);
		if (!fire_surface)
		{
			const argb_t color = V_GetDefaultPalette()->basecolors[34];
			screen->Clear(x, y, x + fire_surface_width * CleanXfac, y + fire_surface_height * CleanYfac, color);
		}
		else
		{
			// [RH] The following fire code is based on the PTC fire demo
			int a, b;

			fire_surface->lock();
			const int pitch = fire_surface->getPitch();

			palindex_t* from = (palindex_t*)fire_surface->getBuffer() + (fire_surface_height - 3) * pitch;
			for (a = 0; a < fire_surface_width; a++, from++)
				*from = *(from + (pitch << 1)) = M_Random();

			from = (palindex_t*)fire_surface->getBuffer();
			for (b = 0; b < fire_surface_height - 4; b += 2)
			{
				palindex_t* pixel = from;

				// special case: first pixel on line
				palindex_t* p = pixel + (pitch << 1);

				unsigned int top = *p + *(p + fire_surface_width - 1) + *(p + 1);
				unsigned int bottom = *(pixel + (pitch << 2));
				unsigned int c1 = (top + bottom) >> 2;
				if (c1 > 1)
					c1--;
				*pixel = c1;
				*(pixel + pitch) = (c1 + bottom) >> 1;
				pixel++;

				// main line loop
				for (a = 1; a < fire_surface_width - 1; a++)
				{
					// sum top pixels
					p = pixel + (pitch << 1);
					top = *p + *(p - 1) + *(p + 1);

					// bottom pixel
					bottom = *(pixel + (pitch << 2));

					// combine pixels
					c1 = (top + bottom) >> 2;
					if (c1 > 1)
						c1--;

					// store pixels
					*pixel = c1;
					*(pixel + pitch) = (c1 + bottom) >> 1;		// interpolate

					// next pixel
					pixel++;
				}

				// special case: last pixel on line
				p = pixel + (pitch << 1);
				top = *p + *(p - 1) + *(p - fire_surface_width + 1);
				bottom = *(pixel + (pitch << 2));
				c1 = (top + bottom) >> 2;
				if (c1 > 1)
					c1--;
				*pixel = c1;
				*(pixel + pitch) = (c1 + bottom) >> 1;

				// next line
				from += pitch << 1;
			}

			y--;
			if (I_GetPrimarySurface()->getBitsPerPixel() == 8)
			{
				// 8bpp rendering:
				     if (CleanXfac == 1) R_RenderFire<1, palindex_t>(x, y);
				else if (CleanXfac == 2) R_RenderFire<2, palindex_t>(x, y);
				else if (CleanXfac == 3) R_RenderFire<3, palindex_t>(x, y);
				else if (CleanXfac == 4) R_RenderFire<4, palindex_t>(x, y);
				else if (CleanXfac == 5) R_RenderFire<5, palindex_t>(x, y);
			}
			else
			{
				// 32bpp rendering:
				     if (CleanXfac == 1) R_RenderFire<1, argb_t>(x, y);
				else if (CleanXfac == 2) R_RenderFire<2, argb_t>(x, y);
				else if (CleanXfac == 3) R_RenderFire<3, argb_t>(x, y);
				else if (CleanXfac == 4) R_RenderFire<4, argb_t>(x, y);
				else if (CleanXfac == 5) R_RenderFire<5, argb_t>(x, y);
			}

			fire_surface->unlock();
		}
	}
	{
		const int spritenum = states[mobjinfo[MT_PLAYER].spawnstate].sprite;
		const spriteframe_t* sprframe = &sprites[spritenum].spriteframes[PlayerState->frame & FF_FRAMEMASK];

		// [Nes] Color of player preview uses the unused translation table (player 0), instead
		// of the table of the current player color. (Which is different in single, demo, and team)
		const argb_t player_color = CL_GetPlayerColor(&consoleplayer());
		R_BuildPlayerTranslation(0, player_color);
		V_ColorMap = translationref_t(translationtables, 0);

		// Draw box surrounding fire and player:
		screen->DrawPatchClean(W_CachePatch("M_PBOX"), 320 - 88 - 32 + 36,
			PSetupDef.y + LINEHEIGHT * 3 + 22);

		screen->DrawTranslatedPatchClean (W_CachePatch (sprframe->lump[0]),
			320 - 52 - 32, PSetupDef.y + LINEHEIGHT*3 + 46);
	}

	// Draw team setting
	{
		const team_t team = D_TeamByName(cl_team.cstring());
		const int x = V_StringWidth ("Prefered Team") + 8 + PSetupDef.x;
		screen->DrawTextCleanMove (CR_RED, PSetupDef.x, PSetupDef.y + LINEHEIGHT, "Prefered Team");
		screen->DrawTextCleanMove (CR_GREY, x, PSetupDef.y + LINEHEIGHT, team == TEAM_NONE ? "NONE" : GetTeamInfo(team)->ColorStringUpper.c_str());
	}

	// Draw gender setting
	{
		const gender_t gender = D_GenderByName(cl_gender.cstring());
		const int x = V_StringWidth ("Gender") + 8 + PSetupDef.x;
		screen->DrawTextCleanMove (CR_RED, PSetupDef.x, PSetupDef.y + LINEHEIGHT*2, "Gender");
		screen->DrawTextCleanMove (CR_GREY, x, PSetupDef.y + LINEHEIGHT*2, genders[gender]);
	}

	// Draw autoaim setting
	{
		const int x = V_StringWidth ("Autoaim") + 8 + PSetupDef.x;
		const float aim = cl_autoaim;

		screen->DrawTextCleanMove (CR_RED, PSetupDef.x, PSetupDef.y + LINEHEIGHT*3, "Autoaim");
		screen->DrawTextCleanMove (CR_GREY, x, PSetupDef.y + LINEHEIGHT*3,
			aim == 0 ? "Never" :
			aim <= 0.25 ? "Very Low" :
			aim <= 0.5 ? "Low" :
			aim <= 1 ? "Medium" :
			aim <= 2 ? "High" :
			aim <= 3 ? "Very High" : "Always");
	}

	// Draw color setting
	{
		const int x = V_StringWidth ("Color") + 8 + PSetupDef.x;
		screen->DrawTextCleanMove (CR_RED, PSetupDef.x, PSetupDef.y + LINEHEIGHT*4, "Color");
		screen->DrawTextCleanMove (CR_GREY, x, PSetupDef.y + LINEHEIGHT*4, colorpresets[colorpreset]);
	}

	int PSetupSize = sizeof(PlayerSetupMenu) / sizeof(PlayerSetupMenu[0]);
	if (colorpreset == COLOR_CUSTOM && PSetupDef.numitems < PSetupSize)
		PSetupDef.numitems = PSetupDef.numitems + 3;
	else if (colorpreset != COLOR_CUSTOM && PSetupDef.numitems > PSetupSize - 3)
		PSetupDef.numitems = PSetupDef.numitems - 3;

	// Draw player color sliders
	//V_DrawTextCleanMove (CR_GREY, PSetupDef.x, PSetupDef.y + LINEHEIGHT, "Color");

	if (colorpreset == COLOR_CUSTOM)
	{
		screen->DrawTextCleanMove (CR_RED, PSetupDef.x, PSetupDef.y + LINEHEIGHT*5, "Red");
		screen->DrawTextCleanMove (CR_RED, PSetupDef.x, PSetupDef.y + LINEHEIGHT*6, "Green");
		screen->DrawTextCleanMove (CR_RED, PSetupDef.x, PSetupDef.y + LINEHEIGHT*7, "Blue");

		{
			const int x = V_StringWidth("Green") + 8 + PSetupDef.x;
			const argb_t playercolor = V_GetColorFromString(cl_color);

			M_DrawSlider(x, PSetupDef.y + LINEHEIGHT*5, 0.0f, 255.0f, playercolor.getr(), 0.0f);
			M_DrawSlider(x, PSetupDef.y + LINEHEIGHT*6, 0.0f, 255.0f, playercolor.getg(), 0.0f);
			M_DrawSlider(x, PSetupDef.y + LINEHEIGHT*7, 0.0f, 255.0f, playercolor.getb(), 0.0f);
		}
	}
}

void M_ChangeTeam (int choice) // [Toke - Teams]
{
	team_t team = D_TeamByName(cl_team.cstring());

	int iTeam = (int)team;
	if (choice)
	{
		iTeam = ++iTeam % NUMTEAMS;
	}
	else
	{
		iTeam--;
		if (iTeam < 0)
			iTeam = NUMTEAMS - 1;
	}
	team = (team_t)iTeam;

	cl_team = GetTeamInfo(team)->ColorStringUpper.c_str();
}

static void M_ChangeGender (int choice)
{
	int gender = D_GenderByName(cl_gender.cstring());

	if (!choice)
		gender = (gender == 0) ? 2 : gender - 1;
	else
		gender = (gender == 2) ? 0 : gender + 1;

	cl_gender = genders[gender];
}

static void M_ChangeAutoAim (int choice)
{
	static const float ranges[] = { 0, 0.25, 0.5, 1, 2, 3, 5000 };
	float aim = cl_autoaim;

	if (!choice) {
		// Select a lower autoaim

		for (int i = 6; i >= 1; i--) {
			if (aim >= ranges[i]) {
				aim = ranges[i - 1];
				break;
			}
		}
	} else {
		// Select a higher autoaim

		for (int i = 5; i >= 0; i--) {
			if (aim >= ranges[i]) {
				aim = ranges[i + 1];
				break;
			}
		}
	}

	cl_autoaim.Set (aim);
}

static void M_ChangeColorPreset (int choice)
{
	int colorpreset = D_ColorPreset(cl_colorpreset.cstring());
	argb_t customcolor = V_GetColorFromString(cl_customcolor);

	if (!choice)
		colorpreset = (colorpreset == 0) ? 10 : colorpreset - 1;
	else
		colorpreset = (colorpreset == 10) ? 0 : colorpreset + 1;

	cl_colorpreset = colorpresets[colorpreset];

	if (colorpreset == COLOR_BLUE)
		// the Corn Chex jump suit; it should be brighter, but that introduces gray pixels on 8-bit
		SendNewColor(57, 57, 255);
	else if (colorpreset == COLOR_INDIGO)
		// the Wheat Chex jump suit; a little darker than the blue
		SendNewColor(134, 134, 134);
	else if (colorpreset == COLOR_GREEN)
		// the Odamex green default
		SendNewColor(64, 207, 0);
	else if (colorpreset == COLOR_BROWN)
		// my best approximation of the Vanilla brown translation
		SendNewColor(169, 87, 31);
	else if (colorpreset == COLOR_RED)
		// the blue luminosity matched to the Vanilla red hue without looking bad on 8-bit
		SendNewColor(250, 62, 62);
	else if (colorpreset == COLOR_GOLD)
		SendNewColor(255, 206, 43);
	else if (colorpreset == COLOR_JUNGLEGREEN)
		SendNewColor(32, 104, 0);
	else if (colorpreset == COLOR_PURPLE)
		SendNewColor(255, 10, 255);
	else if (colorpreset == COLOR_WHITE)
		SendNewColor(255, 255, 255);
	else if (colorpreset == COLOR_BLACK)
		SendNewColor(0, 0, 0);
	else
		SendNewColor(customcolor.getr(), customcolor.getg(), customcolor.getb());
}

static void M_EditPlayerName (int choice)
{
	// we are going to be intercepting all chars
	genStringEnter = 1;
	genStringEnd = M_PlayerNameChanged;
	genStringLen = MAXPLAYERNAME;

	saveSlot = 0;
	strcpy(saveOldString,savegamestrings[0]);
	if (!strcmp(savegamestrings[0],GStrings(EMPTYSTRING)))
		savegamestrings[0][0] = 0;
	saveCharIndex = strlen(savegamestrings[0]);
}

static void M_PlayerNameChanged (int choice)
{
	char command[SAVESTRINGSIZE+8+2];

	sprintf (command, "cl_name \"%s\"", savegamestrings[0]);
	AddCommandString (command);
}
/*
static void M_PlayerTeamChanged (int choice)
{
	char command[SAVESTRINGSIZE+8];

	sprintf (command, "cl_team \"%s\"", savegamestrings[1]);
	AddCommandString (command);
}
*/

static void SendNewColor(int red, int green, int blue)
{
	std::string colorcommand;
	std::string customcolorcommand;
	int colorpreset = D_ColorPreset(cl_colorpreset.cstring());

	StrFormat(colorcommand, "cl_color \"%02x %02x %02x\"", red, green, blue);
	AddCommandString(colorcommand);
	if (colorpreset == COLOR_CUSTOM)
	{
		StrFormat(customcolorcommand, "cl_customcolor \"%02x %02x %02x\"", red, green, blue);
		AddCommandString(customcolorcommand);
	}

	// [SL] not connected to a server so we don't have to wait for the server
	// to verify the color choice
	if (!connected)
	{
		// [Nes] Change the player preview color.
		R_BuildPlayerTranslation(0, V_GetColorFromString(cl_color));

		if (consoleplayer().ingame())
			R_CopyTranslationRGB(0, consoleplayer_id);
	}
}

static void M_SlidePlayerRed(int choice)
{
	argb_t color = V_GetColorFromString(cl_color);
	const int accel = repeatCount < 10 ? 0 : 5;

	if (choice == 0)
		color.setr(std::max(0, int(color.getr()) - 1 - accel));
	else
		color.setr(std::min(255, int(color.getr()) + 1 + accel));

	SendNewColor(color.getr(), color.getg(), color.getb());
}

static void M_SlidePlayerGreen (int choice)
{
	argb_t color = V_GetColorFromString(cl_color);
	const int accel = repeatCount < 10 ? 0 : 5;

	if (choice == 0)
		color.setg(std::max(0, int(color.getg()) - 1 - accel));
	else
		color.setg(std::min(255, int(color.getg()) + 1 + accel));

	SendNewColor(color.getr(), color.getg(), color.getb());
}

static void M_SlidePlayerBlue (int choice)
{
	argb_t color = V_GetColorFromString(cl_color);
	const int accel = repeatCount < 10 ? 0 : 5;

	if (choice == 0)
		color.setb(std::max(0, int(color.getb()) - 1 - accel));
	else
		color.setb(std::min(255, int(color.getb()) + 1 + accel));

	SendNewColor(color.getr(), color.getg(), color.getb());
}


//
//		Menu Functions
//
void M_DrawEmptyCell (oldmenu_t *menu, int item)
{
	screen->DrawPatchClean (W_CachePatch("M_CELL1"),
		menu->x - 10, menu->y+item*LINEHEIGHT - 1);
}

void M_DrawSelCell (oldmenu_t *menu, int item)
{
	screen->DrawPatchClean (W_CachePatch("M_CELL2"),
		menu->x - 10, menu->y+item*LINEHEIGHT - 1);
}


void M_StartMessage (const char *string, void (*routine)(int), bool input)
{
	messageLastMenuActive = menuactive;
	messageToPrint = 1;
	messageString = string;
	messageRoutine = routine;
	messageNeedsInput = input;
	menuactive = true;
}



void M_StopMessage()
{
	menuactive = messageLastMenuActive;
	messageToPrint = 0;
}


//
//		Find string height from hu_font chars
//
int M_StringHeight(char* string)
{
	// Default height without a working font is 8.
	if (::hu_font[0].empty())
		return 8;

	const int height = W_ResolvePatchHandle(hu_font[0])->height();

	int h = height;
	while (*string)
		if ((*string++) == '\n')
			h += height;

	return h;
}



//
// CONTROL PANEL
//

//
// M_Responder
//
bool M_Responder (event_t* ev)
{
	int ch, ch2, mod;

	ch = ch2 = mod = -1;

	// eat mouse events
	if(menuactive)
	{
		if(ev->type == ev_mouse)
			return true;
		else if(ev->type == ev_joystick)
		{
			if(OptionsActive)
				M_OptResponder (ev);
			// Eat joystick events for now -- Hyper_Eye
			return true;
		}
	}

	if (ev->type == ev_keyup)
	{
		if(repeatKey == ev->data1)
		{
			repeatKey = 0;
			repeatCount = 0;
		}
	}

	if (ev->type == ev_keydown)
	{
		ch = ev->data1; 		// scancode
		ch2 = ev->data3;		// ASCII
		mod = ev->mod;			// key mods
	}

	if (ch == -1 || HU_ChatMode() != CHAT_INACTIVE)
		return false;

	// Transfer any action to the Options Menu Responder 
	// if we're not on the main menu.
	if (menuactive && OptionsActive) {
		M_OptResponder (ev);
		return true;
	}

	bool numlock = mod & OKEY_NUMLOCK;

	// Handle Repeat
	if (Key_IsLeftKey(ch, numlock) || Key_IsRightKey(ch, numlock))
	{
		if (repeatKey == ch)
			repeatCount++;
		else
		{
			repeatKey = ch;
			repeatCount = 0;
		}
	}

	const char* cmd = Bindings.GetBind(ch).c_str();

	// Save Game string input
	// [RH] and Player Name string input
	if (genStringEnter)
	{
		if (ch == OKEY_BACKSPACE)
		{
			if (saveCharIndex > 0)
			{
				saveCharIndex--;
				savegamestrings[saveSlot][saveCharIndex] = 0;
			}
		} 
		else if (Key_IsCancelKey(ch))
		{
			genStringEnter = 0;
			M_ClearMenus();
			strcpy(&savegamestrings[saveSlot][0], saveOldString);
		}
		else if (Key_IsAcceptKey(ch))
		{
			genStringEnter = 0;
			M_ClearMenus();
			if (savegamestrings[saveSlot][0])
				genStringEnd(saveSlot);	// [RH] Function to call when enter is pressed
		}
		else 
		{ 
			ch = ev->data3;	// [RH] Use user keymap
			if (ch >= 32 && ch <= 127 &&
				saveCharIndex < genStringLen &&
				V_StringWidth(savegamestrings[saveSlot]) <
				(genStringLen - 1) * 8)
			{
				savegamestrings[saveSlot][saveCharIndex++] = ch;
				savegamestrings[saveSlot][saveCharIndex] = 0;
			}
		}

		return true;
	}

	// Take care of any messages that need input
	if (messageToPrint)
	{
		if (messageNeedsInput &&
		    (!(ch2 == ' ' || Key_IsMenuKey(ch) || Key_IsYesKey(ch) || Key_IsNoKey(ch) || 
			(isascii(ch2) && (toupper(ch2) == 'N' || toupper(ch2) == 'Y'))))) 
			return true;

		menuactive = messageLastMenuActive;
		messageToPrint = 0;
		if (messageRoutine)
		{
			if (ch == NULL && ch2 != NULL)
				messageRoutine(ch2);
			else
				messageRoutine(ch);
		}

		menuactive = false;
		S_Sound (CHAN_INTERFACE, "switches/exitbutn", 1, ATTN_NONE);
		return true;
	}

	// If devparm is set, pressing F1 always takes a screenshot no matter
	// what it's bound to. (for those who don't bother to read the docs)
	if (devparm && ch == OKEY_F1) {
		G_ScreenShot (NULL);
		return true;
	}

	// Pop-up menu?
	if (!menuactive)
	{
		if (Key_IsMenuKey(ch))
		{
			AddCommandString("menu_main");
			return true;
		}
		return false;
	}

	if(cmd)
	{
		// Respond to the main menu binding
		if(!strcmp(cmd, "menu_main"))
		{
			M_ClearMenus();
			return true;
		}
	}

	// Keys usable within menu
	{
		if (Key_IsDownKey(ch, numlock))
		{
			do {
				if (itemOn + 1 > currentMenu->numitems - 1)
					itemOn = 0;
				else
					itemOn++;
				S_Sound(CHAN_INTERFACE, "plats/pt1_stop", 1, ATTN_NONE);
			} while (currentMenu->menuitems[itemOn].status == -1);
			return true;
		}
		else if (Key_IsUpKey(ch, numlock))
		{
			do {
				if (!itemOn)
					itemOn = currentMenu->numitems - 1;
				else
					itemOn--;
				S_Sound(CHAN_INTERFACE, "plats/pt1_stop", 1, ATTN_NONE);
			} while (currentMenu->menuitems[itemOn].status == -1);
			return true;
		}
		else if (Key_IsLeftKey(ch, numlock))
		{
			if (currentMenu->menuitems[itemOn].routine &&
				currentMenu->menuitems[itemOn].status == 2)
			{
				S_Sound(CHAN_INTERFACE, "plats/pt1_mid", 1, ATTN_NONE);
				currentMenu->menuitems[itemOn].routine(0);
			}
			return true;
		}
		else if (Key_IsRightKey(ch, numlock))
		{
			if (currentMenu->menuitems[itemOn].routine &&
				currentMenu->menuitems[itemOn].status == 2)
			{
				S_Sound(CHAN_INTERFACE, "plats/pt1_mid", 1, ATTN_NONE);
				currentMenu->menuitems[itemOn].routine(1);
			}
			return true;
		}
		else if (Key_IsAcceptKey(ch))
		{
			if (currentMenu->menuitems[itemOn].routine &&
				currentMenu->menuitems[itemOn].status)
			{
				currentMenu->lastOn = itemOn;
				if (currentMenu->menuitems[itemOn].status == 2)
				{
					currentMenu->menuitems[itemOn].routine(1);		// right arrow
					S_Sound(CHAN_INTERFACE, "plats/pt1_mid", 1, ATTN_NONE);
				}
				else
				{
					currentMenu->menuitems[itemOn].routine(itemOn);
					S_Sound(CHAN_INTERFACE, "weapons/pistol", 1, ATTN_NONE);
				}
			}
			return true;
		}
		else if (Key_IsCancelKey(ch))
		{
			// [RH] Escaping now moves back one menu instead of
			//	  quitting the menu system. Thus, backspace
			//	  is now ignored.
			currentMenu->lastOn = itemOn;
			M_PopMenuStack();
			return true;
		}
		else
		{
			if (ch2 && (ch < OKEY_JOY1))
			{
				for (int i = itemOn + 1; i < currentMenu->numitems; i++)
					if (tolower(currentMenu->menuitems[i].alphaKey) == ch2)
					{
						itemOn = i;
						S_Sound(CHAN_INTERFACE, "plats/pt1_stop", 1, ATTN_NONE);
						return true;
					}
				for (int i = 0; i <= itemOn; i++)
					if (tolower(currentMenu->menuitems[i].alphaKey) == ch2)
					{
						itemOn = i;
						S_Sound(CHAN_INTERFACE, "plats/pt1_stop", 1, ATTN_NONE);
						return true;
					}
			}
		}
	}

	// [RH] Menu now eats all keydown events while active
	if (ev->type == ev_keydown)
		return true;
	else
		return false;
}


//
// M_StartControlPanel
//
void M_StartControlPanel (void)
{
	// intro might call this repeatedly
	if (menuactive)
		return;

	drawSkull = true;
	MenuStackDepth = 0;
	menuactive = 1;
	currentMenu = &MainDef;
	itemOn = currentMenu->lastOn;
	OptionsActive = false;			// [RH] Make sure none of the options menus appear.
}


//
// M_Drawer
// Called after the view has been rendered,
// but before it has been blitted.
//
void M_Drawer()
{
	if (messageToPrint && !::hu_font[0].empty())
	{
		// Horiz. & Vertically center string and print it.
		brokenlines_t *lines = V_BreakLines (320, messageString);
		int y = 100;

		const patch_t* ch = W_ResolvePatchHandle(hu_font[0]);

		for (int i = 0; lines[i].width != -1; i++)
			y -= ch->height() / 2;

		for (int i = 0; lines[i].width != -1; i++)
		{
			screen->DrawTextCleanMove(CR_RED, 160 - lines[i].width/2, y, lines[i].string);
			y += ch->height();
		}

		V_FreeBrokenLines (lines);
	}
	else if (menuactive)
	{
		if (OptionsActive)
		{
			M_OptDrawer();
		}
		else
		{
			if (currentMenu->routine)
				currentMenu->routine(); 		// call Draw routine

			// DRAW MENU
			const int x = currentMenu->x;
			int y = currentMenu->y;
			const int max = currentMenu->numitems;

			for (int i = 0; i < max; i++)
			{
				if (currentMenu->menuitems[i].name[0])
					screen->DrawPatchClean (W_CachePatch(currentMenu->menuitems[i].name), x, y);
				y += LINEHEIGHT;
			}


			// DRAW SKULL
			if (drawSkull)
			{
				screen->DrawPatchClean(W_CachePatch(skullName[whichSkull]),
					x + SKULLXOFF, currentMenu->y - 5 + itemOn*LINEHEIGHT);
			}
		}
	}

	// [SL] force the status bar to be redrawn in case the menu
	// draws over a portion of the status bar background
	if (R_StatusBarVisible() && (menuactive || messageToPrint))
		ST_ForceRefresh();
}


//
// M_ClearMenus
//
void M_ClearMenus (void)
{
	I_FreeSurface(fire_surface);
	MenuStackDepth = 0;
	menuactive = false;
	drawSkull = true;
	M_DemoNoPlay = false;
}




//
// M_SetupNextMenu
//
void M_SetupNextMenu (oldmenu_t *menudef)
{
	MenuStack[MenuStackDepth].menu.old = menudef;
	MenuStack[MenuStackDepth].isNewStyle = false;
	MenuStack[MenuStackDepth].drawSkull = drawSkull;
	MenuStackDepth++;

	currentMenu = menudef;
	itemOn = currentMenu->lastOn;
}


void M_PopMenuStack (void)
{
	M_DemoNoPlay = false;
	if (MenuStackDepth > 1) {
		MenuStackDepth -= 2;
		if (MenuStack[MenuStackDepth].isNewStyle) {
			OptionsActive = true;
			CurrentMenu = MenuStack[MenuStackDepth].menu.newmenu;
			CurrentItem = CurrentMenu->lastOn;
		} else {
			OptionsActive = false;
			currentMenu = MenuStack[MenuStackDepth].menu.old;
			itemOn = currentMenu->lastOn;
		}
		drawSkull = MenuStack[MenuStackDepth].drawSkull;
		MenuStackDepth++;
		S_Sound (CHAN_INTERFACE, "switches/normbutn", 1, ATTN_NONE);
	} else {
		M_ClearMenus ();
		if (currentMenu == &PSetupDef && PSetupDepth > 0)			// hack for PlayerSetup
		{
			S_Sound (CHAN_INTERFACE, "switches/normbutn", 1, ATTN_NONE);
			M_StartControlPanel();
			if (PSetupDepth == 2)
				M_SetupNextMenu(&MainDef);
			M_Options(0);
		}
		else
			S_Sound (CHAN_INTERFACE, "switches/exitbutn", 1, ATTN_NONE);
	}
}


//
// M_Ticker
//
void M_Ticker (void)
{
	if (--skullAnimCounter <= 0)
	{
		whichSkull ^= 1;
		skullAnimCounter = 8;
	}

	if (currentMenu == &PSetupDef)
		M_PlayerSetupTicker ();
}


//
// M_Init
//
EXTERN_CVAR (screenblocks)

void M_Init (void)
{
	int i;

    // [Russell] - Set this beforehand, because when you switch wads
    // (ie from doom to doom2 back to doom), you will have less menu items
    {
        MainDef.numitems = d1_main_end;
        MainDef.menuitems = DoomMainMenu;
        MainDef.routine = M_DrawMainMenu,
        MainDef.lastOn = 0;
        MainDef.x = 97;
        MainDef.y = 64;
    }

	currentMenu = &MainDef;
	OptionsActive = false;
	menuactive = 0;
	itemOn = currentMenu->lastOn;
	whichSkull = 0;
	skullAnimCounter = 10;
	drawSkull = true;
	screenSize = (int)screenblocks - 3;
	messageToPrint = 0;
	messageString = NULL;
	messageLastMenuActive = menuactive;

    if (gameinfo.flags & GI_MAPxx)
    {
        // Commercial has no "read this" entry.
        MainDef.numitems = d2_main_end;
        MainDef.menuitems = Doom2MainMenu;

        MainDef.y += 8;
    }

	M_OptInit ();

	// [RH] Build a palette translation table for the fire
	for (i = 0; i < 256; i++)
		FireRemap[i] = V_BestColor(V_GetDefaultPalette()->basecolors, i, 0, 0);
}

//
// M_FindCvarInMenu
//
// Takes an array of menu items and returns the index in the array of the
// menu item containing that cvar.  Returns MAXINT if not found.
//
size_t M_FindCvarInMenu(cvar_t &cvar, menuitem_t *menu, size_t length)
{
	if (menu)
	{
    	for (size_t i = 0; i < length; i++)
    	{
        	if (menu[i].a.cvar == &cvar)
            	return i;
    	}
	}

    return MAXINT;    // indicate not found
}


VERSION_CONTROL (m_menu_cpp, "$Id$")
