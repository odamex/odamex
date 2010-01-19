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
//		DOOM selection menu, options, episode etc.
//		Sliders and icons. Kinda widget stuff.
//
//-----------------------------------------------------------------------------

#include "doomdef.h"
#include "dstrings.h"
#include "c_console.h"
#include "c_dispatch.h"
#include "d_main.h"
#include "i_system.h"
#include "i_video.h"
#include "z_zone.h"
#include "v_video.h"
#include "w_wad.h"
#include "r_local.h"
#include "hu_stuff.h"
#include "g_game.h"
#include "m_argv.h"
#include "m_swap.h"
#include "m_random.h"
#include "s_sound.h"
#include "doomstat.h"
#include "m_menu.h"
#include "v_text.h"
#include "st_stuff.h"
#include "cl_ctf.h"
#include "r_sky.h"
#include "cl_main.h"

#include "gi.h"

extern patch_t* 	hu_font[HU_FONTSIZE];

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
void	CL_SendUserInfo(void);
void	M_ChangeTeam (int choice);
team_t D_TeamByName (const char *team);
gender_t D_GenderByName (const char *gender);

#define SAVESTRINGSIZE	24

// we are going to be entering a savegame string
int 				genStringEnter;
int					genStringLen;	// [RH] Max # of chars that can be entered
void	(*genStringEnd)(int slot);
int 				saveSlot;		// which slot to save in
int 				saveCharIndex;	// which char we're editing
// old save description before edit
char				saveOldString[SAVESTRINGSIZE];

bool 				menuactive;

#define SKULLXOFF	-32
#define ARROWXOFF	-28
#define ARROWYOFF	-1
#define LINEHEIGHT	16
#define HTCLINEHEIGHT 20

extern bool st_firsttime;

extern bool			sendpause;
char				savegamestrings[10][SAVESTRINGSIZE];

char				endstring[160];

menustack_t			MenuStack[16];
int					MenuStackDepth;

short				itemOn; 			// menu item skull is on
short				skullAnimCounter;	// skull animation counter
static int			SkullBaseLump;
static int			MenuTime;			// Ticker for Heretic skulls
short				whichSkull; 		// which skull to draw
bool				drawSkull;			// [RH] don't always draw skull

// graphic name of skulls
char				skullName[2][9] = {"M_SKULL1", "M_SKULL2"};
char				arrowName[2][9] = {"M_SLCTR1", "M_SLCTR2"};

// current menudef
oldmenu_t *currentMenu;

//
// PROTOTYPES
//
void M_NewGame(int choice);
void M_Episode(int choice);
void M_ChooseSkill(int choice);
void M_LoadGame(int choice);
void M_SaveGame(int choice);
void M_HtcGameFiles(int choice);	//[ML] Heretic puts save/load in "game files"
void M_Options(int choice);
void M_EndGame(int choice);
void M_ReadThis(int choice);
void M_ReadThis2(int choice);
void M_ReadThis3(int choice);
void M_QuitDOOM(int choice);
void M_QuitHERETIC(int choice);

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
static void M_SlidePlayerRed (int choice);
static void M_SlidePlayerGreen (int choice);
static void M_SlidePlayerBlue (int choice);
static void M_ChangeGender (int choice);
static void M_ChangeSkin (int choice);
static void M_ChangeAutoAim (int choice);
bool M_DemoNoPlay;

static DCanvas *FireScreen;

EXTERN_CVAR (hud_targetnames)
EXTERN_CVAR(cl_connectalert)

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

//
// HERETIC MENU
//
enum htc_main_t
{
	htc_newgame = 0,
	htc_options,					// [RH] Moved
	htc_gamefiles,
	htc_info,
	htc_quitheretic,
	htc_main_end
}htc_main_e;

oldmenuitem_t HereticMainMenu[]=
{
	{1,"New Game",M_NewGame,'N'},
	{1,"Options",M_Options,'O'},	// [RH] Moved
    {1,"Game Files",M_HtcGameFiles,'G'},
    {1,"Info",M_ReadThis,'I'},
	{1,"Quit Game",M_QuitHERETIC,'Q'}
};

oldmenuitem_t HereticGameFilesMenu[]=
{
	{1,"Load Game",M_LoadGame,'l'},
	{1,"Save Game",M_SaveGame,'2'}
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
enum doomepisodes_t
{
	d1_ep1,
	d1_ep2,
	d1_ep3,
	d1_ep4,
	d1_ep_end
} doomepisodes_e;

oldmenuitem_t DoomEpisodeMenu[]=
{
	{1,"M_EPI1", M_Episode,'k'},
	{1,"M_EPI2", M_Episode,'t'},
	{1,"M_EPI3", M_Episode,'i'},
	{1,"M_EPI4", M_Episode,'t'}
};

enum hereticepisodes_t
{
	htc_ep1,
	htc_ep2,
	htc_ep3,
	htc_ep4,
	htc_ep5,
	htc_ep_end
} hereticepisodes_e;

oldmenuitem_t HereticEpisodeMenu[]=
{
	{1,"CITY OF THE DAMNED",	M_Episode,'k'},
	{1,"HELL'S MAW",			M_Episode,'t'},
	{1,"THE DOME OF D'SPARIL",	M_Episode,'i'},
	{1,"THE OSSUARY",			M_Episode,'t'},
	{1,"THE STAGNANT DEMESNE",	M_Episode,'t'}
};

oldmenu_t EpiDef =
{
	d1_ep4,	 			// # of menu items
	DoomEpisodeMenu,	// oldmenuitem_t ->
	M_DrawEpisode,		// drawing routine ->
	48,63,				// x,y
	d1_ep1 				// lastOn
};

//
// NEW GAME
//
enum newgame_t
{
	killthings,
	toorough,
	hurtme,
	violence,
	nightmare,
	newg_end
} newgame_e;

oldmenuitem_t DoomNewGameMenu[]=
{
	{1,"M_JKILL",	M_ChooseSkill, 'i'},
	{1,"M_ROUGH",	M_ChooseSkill, 'h'},
	{1,"M_HURT",	M_ChooseSkill, 'h'},
	{1,"M_ULTRA",	M_ChooseSkill, 'u'},
	{1,"M_NMARE",	M_ChooseSkill, 'n'}
};

oldmenuitem_t HereticNewGameMenu[]=
{
    {1, "THOU NEEDETH A WET-NURSE",	M_ChooseSkill, 't'},
    {1, "YELLOWBELLIES-R-US",	M_ChooseSkill, 'y'},
    {1, "BRINGEST THEM ONETH",	M_ChooseSkill, 'b'},
    {1, "THOU ART A SMITE-MEISTER",	M_ChooseSkill, 'h'},
    {1, "BLACK PLAGUE POSSESSES THEE",	M_ChooseSkill, 'b'}
};

oldmenu_t NewDef =
{
	newg_end,			// # of menu items
	DoomNewGameMenu,		// oldmenuitem_t ->
	M_DrawNewGame,		// drawing routine ->
	48,63,				// x,y
	hurtme				// lastOn
};


//
// [RH] Player Setup Menu
//
byte FireRemap[256];

enum psetup_t
{
	playername,
	playerteam,
	playerred,
	playergreen,
	playerblue,
	playersex,
	playerskin,
	playeraim,
	psetup_end
} psetup_e;

oldmenuitem_t PlayerSetupMenu[] =
{
	{ 1,"", M_EditPlayerName, 'N' },
	{ 2,"", M_ChangeTeam, 'T' },
	{ 2,"", M_SlidePlayerRed, 'R' },
	{ 2,"", M_SlidePlayerGreen, 'G' },
	{ 2,"", M_SlidePlayerBlue, 'B' },
	{ 2,"", M_ChangeGender, 'E' },
	{ 2,"", M_ChangeSkin, 'S' },
	{ 2,"", M_ChangeAutoAim, 'A' }
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
enum
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

oldmenuitem_t LoadMenu[]=
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
	LoadMenu,
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
    S_Sound (CHAN_VOICE, "switches/normbutn", 1, ATTN_NONE);
	M_StartControlPanel ();
	M_SetupNextMenu (&MainDef);
}
END_COMMAND (menu_main)

BEGIN_COMMAND (menu_help)
{
    // F1
    S_Sound (CHAN_VOICE, "switches/normbutn", 1, ATTN_NONE);
	M_StartControlPanel ();
	M_ReadThis(0);
}
END_COMMAND (menu_help)

BEGIN_COMMAND (menu_save)
{
    // F2
	S_Sound (CHAN_VOICE, "switches/normbutn", 1, ATTN_NONE);
	M_StartControlPanel ();
	M_SaveGame (0);
	//Printf (PRINT_HIGH, "Saving is not available at this time.\n");
}
END_COMMAND (menu_save)

BEGIN_COMMAND (menu_load)
{
    // F3
	S_Sound (CHAN_VOICE, "switches/normbutn", 1, ATTN_NONE);
	M_StartControlPanel ();
	M_LoadGame (0);
	//Printf (PRINT_HIGH, "Loading is not available at this time.\n");
}
END_COMMAND (menu_load)

BEGIN_COMMAND (menu_options)
{
    // F4
    S_Sound (CHAN_VOICE, "switches/normbutn", 1, ATTN_NONE);
    M_StartControlPanel ();
	M_Options(0);
}
END_COMMAND (menu_options)

BEGIN_COMMAND (quicksave)
{
    // F6
	S_Sound (CHAN_VOICE, "switches/normbutn", 1, ATTN_NONE);
	M_StartControlPanel ();
	M_QuickSave ();
	//Printf (PRINT_HIGH, "Saving is not available at this time.\n");
}
END_COMMAND (quicksave)

BEGIN_COMMAND (menu_endgame)
{	// F7
    S_Sound (CHAN_VOICE, "switches/normbutn", 1, ATTN_NONE);
	M_StartControlPanel ();
	M_EndGame(0);
}
END_COMMAND (menu_endgame)

BEGIN_COMMAND (quickload)
{
    // F9
	S_Sound (CHAN_VOICE, "switches/normbutn", 1, ATTN_NONE);
	M_StartControlPanel ();
	M_QuickLoad ();
	//Printf (PRINT_HIGH, "Loading is not available at this time.\n");
}
END_COMMAND (quickload)

BEGIN_COMMAND (menu_quit)
{	// F10
	S_Sound (CHAN_VOICE, "switches/normbutn", 1, ATTN_NONE);
	M_StartControlPanel ();
	M_QuitDOOM(0);
}
END_COMMAND (menu_quit)

BEGIN_COMMAND (menu_player)
{
    S_Sound (CHAN_VOICE, "switches/normbutn", 1, ATTN_NONE);
	M_StartControlPanel ();
	M_PlayerSetup(0);
}
END_COMMAND (menu_player)

BEGIN_COMMAND (bumpgamma)
{
    // F11
	// [RH] Gamma correction tables are now generated
	// on the fly for *any* gamma level.
	// Q: What are reasonable limits to use here?

	float newgamma = gammalevel + 1;

	if (newgamma > 5.0)
		newgamma = 1.0;

	gammalevel.Set (newgamma);

	if (gammalevel.value() == 1.0)
        Printf (PRINT_HIGH, "Gamma correction off\n");
    else
        Printf (PRINT_HIGH, "Gamma correction level %g\n", gammalevel.value() - 1);
}
END_COMMAND (bumpgamma)

void M_HtcGameFilesResponse(int ch)
{
	return;
}

void M_HtcGameFiles(int choice)
{
     M_StartMessage("Loading/saving is not supported\n\n(Press any key to "
                   "continue)\n", M_HtcGameFilesResponse, false);
}

//
// M_ReadSaveStrings
//	read the strings from the savegame files
//
void M_ReadSaveStrings(void)
{
	FILE *handle;
	int count;
	int i;

	for (i = 0; i < load_end; i++)
	{
		std::string name;

		G_BuildSaveName (name, i);

		handle = fopen (name.c_str(), "rb");
		if (handle == NULL)
		{
			strcpy (&savegamestrings[i][0], EMPTYSTRING);
			LoadMenu[i].status = 0;
		}
		else
		{
			count = fread (&savegamestrings[i], SAVESTRINGSIZE, 1, handle);
			fclose (handle);
			LoadMenu[i].status = 1;
		}
	}
}


//
// M_LoadGame & Cie.
//
void M_DrawLoad (void)
{
	int i;

	screen->DrawPatchClean ((patch_t *)W_CacheLumpName ("M_LOADG",PU_CACHE), 72, 28);
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
	/*if (netgame)
	{
		M_StartMessage (LOADNET,NULL,false);
		return;
	}*/

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

	screen->DrawPatchClean ((patch_t *)W_CacheLumpName("M_SAVEG",PU_CACHE), 72, 28);
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
	// we are going to be intercepting all chars
	genStringEnter = 1;
	genStringEnd = M_DoSave;
	genStringLen = SAVESTRINGSIZE-1;

	saveSlot = choice;
	strcpy(saveOldString,savegamestrings[choice]);
	if (!strcmp(savegamestrings[choice],EMPTYSTRING))
		savegamestrings[choice][0] = 0;
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
		M_StartMessage(SAVEDEAD,NULL,false);
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
	if (ch == 'y')
	{
		M_DoSave (quickSaveSlot);
		S_Sound (CHAN_VOICE, "switches/exitbutn", 1, ATTN_NONE);
	}
}

void M_QuickSave(void)
{
	if (multiplayer)
	{
		S_Sound (CHAN_VOICE, "player/male/grunt1", 1, ATTN_NONE);
		M_ClearMenus ();
		return;
	}

	if (!usergame)
	{
		S_Sound (CHAN_VOICE, "player/male/grunt1", 1, ATTN_NONE);
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
	sprintf (tempstring, QSPROMPT, savegamestrings[quickSaveSlot]);
	M_StartMessage (tempstring, M_QuickSaveResponse, true);
}


//
// M_QuickLoad
// [ML] 7 Sept 08: Bringing game saving/loading in from
//                 zdoom 1.22 source, see MAINTAINERS
//
void M_QuickLoadResponse(int ch)
{
	if (ch == 'y')
	{
		M_LoadSelect(quickSaveSlot);
		S_Sound (CHAN_VOICE, "switches/exitbutn", 1, ATTN_NONE);
	}
}


void M_QuickLoad(void)
{
	/*if (netgame)
	{
		M_StartMessage(QLOADNET,NULL,false);
		return;
	}*/

	if (quickSaveSlot < 0)
	{
		M_StartControlPanel();
		M_LoadGame (0);
		return;
	}
	sprintf(tempstring,QLPROMPT,savegamestrings[quickSaveSlot]);
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
void M_DrawMainMenu (void)
{
	int frame;

	if (gamemode == registered_heretic) {
		frame = (MenuTime / 3) % 18;
		screen->DrawPatchClean (W_CachePatch("M_HTIC"), 88, 0);
		screen->DrawPatchClean ((patch_t *)W_CacheLumpNum(SkullBaseLump + (17 - frame), PU_CACHE), 40, 10);
		screen->DrawPatchClean ((patch_t *)W_CacheLumpNum(SkullBaseLump + frame, PU_CACHE), 232, 10);
	}
	else
		screen->DrawPatchClean (W_CachePatch("M_DOOM"), 94, 2);
}

void M_DrawNewGame(void)
{
	screen->DrawPatchClean ((patch_t *)W_CacheLumpName("M_NEWG",PU_CACHE), 96, 14);
	screen->DrawPatchClean ((patch_t *)W_CacheLumpName("M_SKILL",PU_CACHE), 54, 38);
}

void M_NewGame(int choice)
{
/*	if (netgame && !demoplayback)
	{
		M_StartMessage(NEWGAME,NULL,false);
		return;
	}
*/
	if (gameinfo.flags & GI_MAPxx || gamemode == retail_chex)	// [ML] Don't show the episode selection in chex mode
		M_SetupNextMenu(&NewDef);
	else
	{
		if (gamemode == registered_heretic) {
			EpiDef.numitems = htc_ep_end;
			EpiDef.menuitems = HereticEpisodeMenu;
			EpiDef.routine = NULL;
			EpiDef.x = 80;
			EpiDef.y = 30;
		}
		else if (gameinfo.flags & GI_MENUHACK_RETAIL)
			EpiDef.numitems = d1_ep_end;
		else
			EpiDef.numitems = d1_ep4;

		M_SetupNextMenu(&EpiDef);
	}
}


//
//		M_Episode
//
int 	epi;

void M_DrawEpisode(void)
{
	screen->DrawPatchClean ((patch_t *)W_CacheLumpName("M_EPISOD",PU_CACHE), 54, 38);
}

void M_VerifyNightmare(int ch)
{
	if (ch != 'y') {
	    M_ClearMenus ();
		return;
	}

	M_StartGame(nightmare);
}

void M_StartGame(int choice)
{
	skill.Set ((float)choice+1);

	G_DeferedInitNew (CalcMapName (epi+1, 1));
	M_ClearMenus ();
}

void M_ChooseSkill(int choice)
{
	if (choice == nightmare)
	{
		M_StartMessage(NIGHTMARE,M_VerifyNightmare,true);
		return;
	}

	M_StartGame(choice);
}

void M_Episode (int choice)
{
	if ((gameinfo.flags & GI_SHAREWARE) && choice)
	{
		M_StartMessage(SWSTRING,NULL,false);
		//M_SetupNextMenu(&ReadDef1);
		M_ClearMenus ();
		return;
	}

	epi = choice;
	if (gamemode == registered_heretic) {
		NewDef.menuitems = HereticNewGameMenu;
		NewDef.routine = NULL;
		NewDef.x = 38;
		NewDef.y = 30;
	}

	M_SetupNextMenu(&NewDef);
}

//
// Read This Menus
// Had a "quick hack to fix romero bug"
//
void M_DrawReadThis1 (void)
{
	screen->DrawPatchIndirect ((patch_t *)W_CacheLumpName (gameinfo.info.infoPage[0], PU_CACHE), 0, 0);
}



//
// Read This Menus - optional second page.
//
void M_DrawReadThis2 (void)
{
	screen->DrawPatchIndirect ((patch_t *)W_CacheLumpName (gameinfo.info.infoPage[1], PU_CACHE), 0, 0);
}

//
// Read This Menus - shareware third page.
//
void M_DrawReadThis3 (void)
{
	screen->DrawPatchIndirect ((patch_t *)W_CacheLumpName (gameinfo.info.infoPage[2], PU_CACHE), 0, 0);
}

//
// M_Options
//
void M_DrawOptions(void)
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
	if (toupper(ch) != 'Y') {
	    M_ClearMenus ();
		return;
	}

	currentMenu->lastOn = itemOn;
	M_ClearMenus ();
	D_StartTitle ();
	CL_QuitNetGame();
}

void M_EndGame(int choice)
{
	choice = 0;
	if (!usergame)
	{
		S_Sound (CHAN_VOICE, "player/male/grunt1", 1, ATTN_NONE);
		return;
	}

	M_StartMessage((multiplayer ? NETEND : ENDGAME), M_EndGameResponse, true);
}

//
// M_QuitDOOM
//

void M_QuitResponse(int ch)
{
	if (toupper(ch) != 'Y') {
	    M_ClearMenus ();
		return;
	}

	if (!multiplayer)
	{
		if (gameinfo.quitSounds)
		{
			S_Sound (CHAN_VOICE, gameinfo.quitSounds[(gametic>>2)&7],
				1, ATTN_SURROUND);
			I_WaitVBL (105);
		}
	}
	exit (0);
}

void M_QuitDOOM (int choice)
{
	// We pick index 0 which is language sensitive,
	//  or one at random, between 1 and maximum number.
	if (language != english )
		sprintf(endstring,"%s\n\n%s", endmsg[0], DOSY );
	else
		sprintf(endstring,"%s\n\n%s", endmsg[ (gametic%(NUM_QUITMESSAGES-2))+1 ], DOSY);

	M_StartMessage(endstring,M_QuitResponse,true);
}

void M_QuitHERETIC (int choice)
{
	sprintf(endstring,"%s\n\n", endmsg[0] );
	M_StartMessage(endstring,M_QuitResponse,true);
}


// -----------------------------------------------------
//		Player Setup Menu code
// -----------------------------------------------------

void M_DrawSlider (int x, int y, float min, float max, float cur);

static const char *genders[3] = { "male", "female", "cyborg" };
static state_t *PlayerState;
static int PlayerTics;

EXTERN_CVAR (cl_name)
EXTERN_CVAR (cl_team)
EXTERN_CVAR (cl_color)
EXTERN_CVAR (cl_skin)
EXTERN_CVAR (cl_autoaim)
EXTERN_CVAR (cl_gender)

void M_PlayerSetup (int choice)
{
	choice = 0;
	strcpy (savegamestrings[0], cl_name.cstring());
//	strcpy (savegamestrings[1], team.cstring());	if (t = 1) // [Toke - Teams]
	//M_DemoNoPlay = true;
	//if (demoplayback)
	//	G_CheckDemoStatus ();
	M_SetupNextMenu (&PSetupDef);
	PlayerState = &states[mobjinfo[MT_PLAYER].seestate];
	PlayerTics = PlayerState->tics;
	if (FireScreen == NULL)
		FireScreen = I_AllocateScreen (72, 72+5, 8);

	// [Nes] Intialize the player preview color.
	R_BuildPlayerTranslation (0, V_GetColorFromString (NULL, cl_color.cstring()));
}

static void M_PlayerSetupTicker (void)
{
	// Based on code in f_finale.c
	if (--PlayerTics > 0)
		return;

	if (PlayerState->tics == -1 || PlayerState->nextstate == S_NULL) {
		PlayerState = &states[mobjinfo[MT_PLAYER].seestate];
	} else {
		PlayerState = &states[PlayerState->nextstate];
	}
	PlayerTics = PlayerState->tics;
}

static void M_PlayerSetupDrawer (void)
{
	// Draw title
	{
		patch_t *patch = W_CachePatch ("M_PSTTL");

		screen->DrawPatchClean (patch,
			160 - (patch->width() >> 1),
			PSetupDef.y - (patch->height() * 3));
	}

	// Draw player name box
	screen->DrawTextCleanMove (CR_RED, PSetupDef.x, PSetupDef.y, "Name");
	M_DrawSaveLoadBorder (PSetupDef.x + 56, PSetupDef.y, MAXPLAYERNAME+1);
	screen->DrawTextCleanMove (CR_RED, PSetupDef.x + 56, PSetupDef.y, savegamestrings[0]);

	// Draw player team box
//	screen->DrawTextCleanMove (CR_RED, PSetupDef.x, PSetupDef.y + LINEHEIGHT, "Team");	if (t = 1) // [Toke - Teams]
//	screen->DrawTextCleanMove (CR_RED, PSetupDef.x + 56, PSetupDef.y + LINEHEIGHT, savegamestrings[1]);


	// Draw cursor for either of the above
	if (genStringEnter)
		screen->DrawTextCleanMove (CR_RED, PSetupDef.x + V_StringWidth(savegamestrings[saveSlot]) + 56, PSetupDef.y + ((saveSlot == 0) ? 0 : LINEHEIGHT), "_");

	// Draw player character
	{
		int x = 320 - 88 - 32, y = PSetupDef.y + LINEHEIGHT*3 - 14;

		x = (x-160)*CleanXfac+(screen->width>>1);
		y = (y-100)*CleanYfac+(screen->height>>1);
		if (!FireScreen)
		{
			screen->Clear (x, y, x + 72 * CleanXfac, y + 72 * CleanYfac, 34);
		}
		else
		{
			// [RH] The following fire code is based on the PTC fire demo
			int a, b;
			byte *from;
			int width, height, pitch;

			FireScreen->Lock ();

			width = FireScreen->width;
			height = FireScreen->height;
			pitch = FireScreen->pitch;

			from = FireScreen->buffer + (height - 3) * pitch;
			for (a = 0; a < width; a++, from++)
			{
				*from = *(from + (pitch << 1)) = M_Random();
			}

			from = FireScreen->buffer;
			for (b = 0; b < FireScreen->height - 4; b += 2)
			{
				byte *pixel = from;

				// special case: first pixel on line
				byte *p = pixel + (pitch << 1);
				unsigned int top = *p + *(p + width - 1) + *(p + 1);
				unsigned int bottom = *(pixel + (pitch << 2));
				unsigned int c1 = (top + bottom) >> 2;
				if (c1 > 1) c1--;
				*pixel = c1;
				*(pixel + pitch) = (c1 + bottom) >> 1;
				pixel++;

				// main line loop
				for (a = 1; a < width-1; a++)
				{
					// sum top pixels
					p = pixel + (pitch << 1);
					top = *p + *(p - 1) + *(p + 1);

					// bottom pixel
					bottom = *(pixel + (pitch << 2));

					// combine pixels
					c1 = (top + bottom) >> 2;
					if (c1 > 1) c1--;

					// store pixels
					*pixel = c1;
					*(pixel + pitch) = (c1 + bottom) >> 1;		// interpolate

					// next pixel
					pixel++;
				}

				// special case: last pixel on line
				p = pixel + (pitch << 1);
				top = *p + *(p - 1) + *(p - width + 1);
				bottom = *(pixel + (pitch << 2));
				c1 = (top + bottom) >> 2;
				if (c1 > 1) c1--;
				*pixel = c1;
				*(pixel + pitch) = (c1 + bottom) >> 1;

				// next line
				from += pitch << 1;
			}

			y--;
			pitch = screen->pitch;
			switch (CleanXfac)
			{
			case 1:
				for (b = 0; b < FireScreen->height; b++)
				{
					byte *to = screen->buffer + y * screen->pitch + x;
					from = FireScreen->buffer + b * FireScreen->pitch;
					y += CleanYfac;

					for (a = 0; a < FireScreen->width; a++, to++, from++)
					{
						int c;
						for (c = CleanYfac; c; c--)
							*(to + pitch*c) = FireRemap[*from];
					}
				}
				break;

			case 2:
				for (b = 0; b < FireScreen->height; b++)
				{
					byte *to = screen->buffer + y * screen->pitch + x;
					from = FireScreen->buffer + b * FireScreen->pitch;
					y += CleanYfac;

					for (a = 0; a < FireScreen->width; a++, to += 2, from++)
					{
						int c;
						for (c = CleanYfac; c; c--)
						{
							*(to + pitch*c) = FireRemap[*from];
							*(to + pitch*c + 1) = FireRemap[*from];
						}
					}
				}
				break;

			case 3:
				for (b = 0; b < FireScreen->height; b++)
				{
					byte *to = screen->buffer + y * screen->pitch + x;
					from = FireScreen->buffer + b * FireScreen->pitch;
					y += CleanYfac;

					for (a = 0; a < FireScreen->width; a++, to += 3, from++)
					{
						int c;
						for (c = CleanYfac; c; c--)
						{
							*(to + pitch*c) = FireRemap[*from];
							*(to + pitch*c + 1) = FireRemap[*from];
							*(to + pitch*c + 2) = FireRemap[*from];
						}
					}
				}
				break;

			case 4:
			default:
				for (b = 0; b < FireScreen->height; b++)
				{
					byte *to = screen->buffer + y * screen->pitch + x;
					from = FireScreen->buffer + b * FireScreen->pitch;
					y += CleanYfac;

					for (a = 0; a < FireScreen->width; a++, to += 4, from++)
					{
						int c;
						for (c = CleanYfac; c; c--)
						{
							*(to + pitch*c) = FireRemap[*from];
							*(to + pitch*c + 1) = FireRemap[*from];
							*(to + pitch*c + 2) = FireRemap[*from];
							*(to + pitch*c + 3) = FireRemap[*from];
						}
					}
				}
				break;
			}
			FireScreen->Unlock ();
		}
	}
	{
		unsigned skin = R_FindSkin(cl_skin.cstring());
		spriteframe_t *sprframe =
			&sprites[skins[skin].sprite].spriteframes[PlayerState->frame & FF_FRAMEMASK];

		// [Nes] Color of player preview uses the unused translation table (player 0), instead
		// of the table of the current player color. (Which is different in single, demo, and team)
		V_ColorMap = translationtables; // + 0 * 256
		//V_ColorMap = translationtables + consoleplayer().id * 256;

		screen->DrawTranslatedPatchClean (W_CachePatch (sprframe->lump[0]),
			320 - 52 - 32, PSetupDef.y + LINEHEIGHT*3 + 46);
	}
	screen->DrawPatchClean (W_CachePatch ("M_PBOX"),
		320 - 88 - 32 + 36, PSetupDef.y + LINEHEIGHT*3 + 22);

	// Draw player color sliders
	//V_DrawTextCleanMove (CR_GREY, PSetupDef.x, PSetupDef.y + LINEHEIGHT, "Color");

	screen->DrawTextCleanMove (CR_RED, PSetupDef.x, PSetupDef.y + LINEHEIGHT*2, "Red");
	screen->DrawTextCleanMove (CR_RED, PSetupDef.x, PSetupDef.y + LINEHEIGHT*3, "Green");
	screen->DrawTextCleanMove (CR_RED, PSetupDef.x, PSetupDef.y + LINEHEIGHT*4, "Blue");

	{
		int x = V_StringWidth ("Green") + 8 + PSetupDef.x;
		int color = V_GetColorFromString(NULL, cl_color.cstring());

		M_DrawSlider (x, PSetupDef.y + LINEHEIGHT*2, 0.0f, 255.0f, RPART(color));
		M_DrawSlider (x, PSetupDef.y + LINEHEIGHT*3, 0.0f, 255.0f, GPART(color));
		M_DrawSlider (x, PSetupDef.y + LINEHEIGHT*4, 0.0f, 255.0f, BPART(color));
	}

	// Draw team setting
	{
		team_t team = D_TeamByName(cl_team.cstring());
		int x = V_StringWidth ("Prefered Team") + 8 + PSetupDef.x;
		screen->DrawTextCleanMove (CR_RED, PSetupDef.x, PSetupDef.y + LINEHEIGHT, "Prefered Team");
		screen->DrawTextCleanMove (CR_GREY, x, PSetupDef.y + LINEHEIGHT, team == TEAM_NONE ? "NONE" : team_names[team]);
	}

	// Draw gender setting
	{
		gender_t gender = D_GenderByName(cl_gender.cstring());
		int x = V_StringWidth ("Gender") + 8 + PSetupDef.x;
		screen->DrawTextCleanMove (CR_RED, PSetupDef.x, PSetupDef.y + LINEHEIGHT*5, "Gender");
		screen->DrawTextCleanMove (CR_GREY, x, PSetupDef.y + LINEHEIGHT*5, genders[gender]);
	}

	// Draw skin setting
	{
		if (gametype != GM_CTF) // [Toke - CTF] Dont allow skin selection if in CTF or Teamplay mode
		{
			int x = V_StringWidth ("Skin") + 8 + PSetupDef.x;
			screen->DrawTextCleanMove (CR_RED, PSetupDef.x, PSetupDef.y + LINEHEIGHT*6, "Skin");
			screen->DrawTextCleanMove (CR_GREY, x, PSetupDef.y + LINEHEIGHT*6, cl_skin.cstring());
		}
	}

	// Draw autoaim setting
	{
		int x = V_StringWidth ("Autoaim") + 8 + PSetupDef.x;
		float aim = cl_autoaim;

		screen->DrawTextCleanMove (CR_RED, PSetupDef.x, PSetupDef.y + LINEHEIGHT*7, "Autoaim");
		screen->DrawTextCleanMove (CR_GREY, x, PSetupDef.y + LINEHEIGHT*7,
			aim == 0 ? "Never" :
			aim <= 0.25 ? "Very Low" :
			aim <= 0.5 ? "Low" :
			aim <= 1 ? "Medium" :
			aim <= 2 ? "High" :
			aim <= 3 ? "Very High" : "Always");
	}
}

void M_ChangeTeam (int choice) // [Toke - Teams]
{
	team_t team = D_TeamByName(cl_team.cstring());

	if(choice)
	{
		switch(team)
		{
			case TEAM_NONE: team = TEAM_BLUE; break;
			case TEAM_BLUE: team = TEAM_RED; break;
			case TEAM_RED: team = TEAM_GOLD; break;
			default:
			case TEAM_GOLD: team = TEAM_NONE; break;
		}
	}
	else
	{
		switch(team)
		{
			case TEAM_NONE: team = TEAM_GOLD; break;
			case TEAM_RED: team = TEAM_BLUE; break;
			case TEAM_GOLD: team = TEAM_RED; break;
			default:
			case TEAM_BLUE: team = TEAM_NONE; break;
		}
	}

	cl_team = (team == TEAM_NONE) ? "" : team_names[team];
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

static void M_ChangeSkin (int choice) // [Toke - Skins]
{
	unsigned skin = R_FindSkin(cl_skin.cstring());

	if (!choice)
		skin = (skin == 0) ? numskins - 1 : skin - 1;
	else
		skin = (skin + 1 < numskins) ? skin + 1 : 0;

	cl_skin = skins[skin].name;
}

static void M_ChangeAutoAim (int choice)
{
	static const float ranges[] = { 0, 0.25, 0.5, 1, 2, 3, 5000 };
	float aim = cl_autoaim;
	int i;

	if (!choice) {
		// Select a lower autoaim

		for (i = 6; i >= 1; i--) {
			if (aim >= ranges[i]) {
				aim = ranges[i - 1];
				break;
			}
		}
	} else {
		// Select a higher autoaim

		for (i = 5; i >= 0; i--) {
			if (aim >= ranges[i]) {
				aim = ranges[i + 1];
				break;
			}
		}
	}

	cl_autoaim.Set (aim);
}

static void M_EditPlayerName (int choice)
{
	// we are going to be intercepting all chars
	genStringEnter = 1;
	genStringEnd = M_PlayerNameChanged;
	genStringLen = MAXPLAYERNAME;

	saveSlot = 0;
	strcpy(saveOldString,savegamestrings[0]);
	if (!strcmp(savegamestrings[0],EMPTYSTRING))
		savegamestrings[0][0] = 0;
	saveCharIndex = strlen(savegamestrings[0]);
}

static void M_PlayerNameChanged (int choice)
{
	char command[SAVESTRINGSIZE+8];

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

static void SendNewColor (int red, int green, int blue)
{
	char command[24];

	sprintf (command, "cl_color \"%02x %02x %02x\"", red, green, blue);
	AddCommandString (command);

	// [Nes] Change the player preview color.
	R_BuildPlayerTranslation (0, V_GetColorFromString (NULL, cl_color.cstring()));
}

static void M_SlidePlayerRed (int choice)
{
	int color = V_GetColorFromString(NULL, cl_color.cstring());
	int red = RPART(color);

	if (choice == 0) {
		red -= 1;
		if (red < 0)
			red = 0;
	} else {
		red += 1;
		if (red > 255)
			red = 255;
	}

	SendNewColor (red, GPART(color), BPART(color));
}

static void M_SlidePlayerGreen (int choice)
{
	int color = V_GetColorFromString(NULL, cl_color.cstring());
	int green = GPART(color);

	if (choice == 0) {
		green -= 1;
		if (green < 0)
			green = 0;
	} else {
		green += 1;
		if (green > 255)
			green = 255;
	}

	SendNewColor (RPART(color), green, BPART(color));
}

static void M_SlidePlayerBlue (int choice)
{
	int color = V_GetColorFromString(NULL, cl_color.cstring());
	int blue = BPART(color);

	if (choice == 0) {
		blue -= 1;
		if (blue < 0)
			blue = 0;
	} else {
		blue += 1;
		if (blue > 255)
			blue = 255;
	}

	SendNewColor (RPART(color), GPART(color), blue);
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
	return;
}



void M_StopMessage (void)
{
	menuactive = messageLastMenuActive;
	messageToPrint = 0;
}


//
//		Find string height from hu_font chars
//
int M_StringHeight(char* string)
{
	int h;
	int height = hu_font[0]->height();

	h = height;
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
	int ch, ch2;
	int i;

	ch = ch2 = -1;

	// eat mouse events
	if(menuactive)
		if(ev->type == ev_mouse)
			return true;

	if (ev->type == ev_keydown)
	{
		ch = ev->data1; 		// scancode
		ch2 = ev->data2;		// ASCII
	}

	if (ch == -1 || headsupactive)
		return false;

	if (menuactive && OptionsActive) {
		M_OptResponder (ev);
		return true;
	}

	// Save Game string input
	// [RH] and Player Name string input
	if (genStringEnter)
	{
		switch(ch)
		{
		  case KEY_BACKSPACE:
			if (saveCharIndex > 0)
			{
				saveCharIndex--;
				savegamestrings[saveSlot][saveCharIndex] = 0;
			}
			break;

		  case KEY_ESCAPE:
			genStringEnter = 0;
			M_ClearMenus ();
			strcpy(&savegamestrings[saveSlot][0],saveOldString);
			break;

		  case KEY_ENTER:
			genStringEnter = 0;
			M_ClearMenus ();
			if (savegamestrings[saveSlot][0])
				genStringEnd(saveSlot);	// [RH] Function to call when enter is pressed
			break;

		  default:
			ch = toupper(ev->data3);	// [RH] Use user keymap
			if (ch != 32)
				if (ch-HU_FONTSTART < 0 || ch-HU_FONTSTART >= HU_FONTSIZE)
					break;
			if (ch >= 32 && ch <= 127 &&
				saveCharIndex < genStringLen &&
				V_StringWidth(savegamestrings[saveSlot]) <
				(genStringLen-1)*8)
			{
				savegamestrings[saveSlot][saveCharIndex++] = ch;
				savegamestrings[saveSlot][saveCharIndex] = 0;
			}
			break;
		}
		return true;
	}

	// Take care of any messages that need input
	if (messageToPrint)
	{
		if (messageNeedsInput &&
			!(ch2 == ' ' || toupper(ch2) == 'N' || toupper(ch2) == 'Y' || ch == KEY_ESCAPE))
			return true;

		menuactive = messageLastMenuActive;
		messageToPrint = 0;
		if (messageRoutine)
			messageRoutine(ch2);

		menuactive = false;
		S_Sound (CHAN_VOICE, "switches/exitbutn", 1, ATTN_NONE);
		return true;
	}

	// [RH] F-Keys are now just normal keys that can be bound,
	//		so they aren't checked here anymore.

	// If devparm is set, pressing F1 always takes a screenshot no matter
	// what it's bound to. (for those who don't bother to read the docs)
	if (devparm && ch == KEY_F1) {
		G_ScreenShot (NULL);
		return true;
	}

	// Pop-up menu?
	if (!menuactive)
	{
		if (ch == KEY_ESCAPE)
		{
			M_StartControlPanel ();
			M_SetupNextMenu (&MainDef);
			S_Sound (CHAN_VOICE, "switches/normbutn", 1, ATTN_NONE);
			return true;
		}
		return false;
	}


	// Keys usable within menu
	switch (ch)
	{
	  case KEY_DOWNARROW:
		do
		{
			if (itemOn+1 > currentMenu->numitems-1)
				itemOn = 0;
			else
			{
				// [Toke - CTF]  Skip the skins item in CTF or Teamplay mode
				if (gametype == GM_CTF && currentMenu == &PSetupDef && itemOn == 5)
					itemOn = itemOn + 2;
				else	itemOn++;
			}
			S_Sound (CHAN_VOICE, "plats/pt1_stop", 1, ATTN_NONE);
		} while(currentMenu->menuitems[itemOn].status==-1);
		return true;

	  case KEY_UPARROW:
		do
		{
			if (!itemOn)
				itemOn = currentMenu->numitems-1;
			else
			{
				// [Toke - CTF]  Skip the skins item in CTF or Teamplay mode
				if (gametype == GM_CTF && currentMenu == &PSetupDef && itemOn == 7)
					itemOn = itemOn - 2;
				else itemOn--;
			}
			S_Sound (CHAN_VOICE, "plats/pt1_stop", 1, ATTN_NONE);
		} while(currentMenu->menuitems[itemOn].status==-1);
		return true;

	  case KEY_LEFTARROW:
		if (currentMenu->menuitems[itemOn].routine &&
			currentMenu->menuitems[itemOn].status == 2)
		{
			S_Sound (CHAN_VOICE, "plats/pt1_mid", 1, ATTN_NONE);
			currentMenu->menuitems[itemOn].routine(0);
		}
		return true;

	  case KEY_RIGHTARROW:
		if (currentMenu->menuitems[itemOn].routine &&
			currentMenu->menuitems[itemOn].status == 2)
		{
			S_Sound (CHAN_VOICE, "plats/pt1_mid", 1, ATTN_NONE);
			currentMenu->menuitems[itemOn].routine(1);
		}
		return true;

	  case KEY_ENTER:
		if (currentMenu->menuitems[itemOn].routine &&
			currentMenu->menuitems[itemOn].status)
		{
			currentMenu->lastOn = itemOn;
			if (currentMenu->menuitems[itemOn].status == 2)
			{
				currentMenu->menuitems[itemOn].routine(1);		// right arrow
				S_Sound (CHAN_VOICE, "plats/pt1_mid", 1, ATTN_NONE);
			}
			else
			{
				currentMenu->menuitems[itemOn].routine(itemOn);
				S_Sound (CHAN_VOICE, "weapons/pistol", 1, ATTN_NONE);
			}
		}
		return true;

	  // [RH] Escape now moves back one menu instead of
	  //	  quitting the menu system. Thus, backspace
	  //	  is now ignored.
	  case KEY_ESCAPE:
		currentMenu->lastOn = itemOn;
		M_PopMenuStack ();
		return true;

	  default:
		if (ch2) {
			for (i = itemOn+1;i < currentMenu->numitems;i++)
				if (currentMenu->menuitems[i].alphaKey == toupper(ch2))
				{
					itemOn = i;
					S_Sound (CHAN_VOICE, "plats/pt1_stop", 1, ATTN_NONE);
					return true;
				}
			for (i = 0;i <= itemOn;i++)
				if (currentMenu->menuitems[i].alphaKey == toupper(ch2))
				{
					itemOn = i;
					S_Sound (CHAN_VOICE, "plats/pt1_stop", 1, ATTN_NONE);
					return true;
				}
		}
		break;

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
	MenuTime = 0;
	currentMenu = &MainDef;
	itemOn = currentMenu->lastOn;
	OptionsActive = false;			// [RH] Make sure none of the options menus appear.
	I_PauseMouse ();				// [RH] Give the mouse back in windowed modes.
}


//
// M_Drawer
// Called after the view has been rendered,
// but before it has been blitted.
//
void M_Drawer (void)
{
	int i, x, y, max;

	st_firsttime = true;
	//screen->Dim (); // denis - removed, see bug 388

	// Horiz. & Vertically center string and print it.
	if (messageToPrint)
	{
		brokenlines_t *lines = V_BreakLines (320, messageString);
		y = 100;

		for (i = 0; lines[i].width != -1; i++)
			y -= hu_font[0]->height() / 2;

		for (i = 0; lines[i].width != -1; i++)
		{
			screen->DrawTextCleanMove (CR_RED, 160 - lines[i].width/2, y, lines[i].string);
			y += hu_font[0]->height();
		}

		V_FreeBrokenLines (lines);
	}
	else if (menuactive)
	{
		if (OptionsActive)
		{
			M_OptDrawer ();
		}
		else
		{
			if (currentMenu->routine)
				currentMenu->routine(); 		// call Draw routine

			// DRAW MENU
			x = currentMenu->x;
			y = currentMenu->y;
			max = currentMenu->numitems;

			for (i = 0; i < max; i++)
			{
				if (currentMenu->menuitems[i].name[0]) {
					if (gamemode == registered_heretic)
					{
						screen->DrawTextLargeClean(0, x*2, y*2, currentMenu->menuitems[i].name);
						y += HTCLINEHEIGHT;
					}
					else
					{
						screen->DrawPatchClean (W_CachePatch(currentMenu->menuitems[i].name), x, y);
						y += LINEHEIGHT;
					}
				}
			}

			// DRAW SKULL
			if (drawSkull)
			{
				if (gamemode == registered_heretic)
					screen->DrawPatchClean (W_CachePatch(arrowName[whichSkull]),
						x + ARROWXOFF, (currentMenu->y + itemOn*HTCLINEHEIGHT + ARROWYOFF));
				else
					screen->DrawPatchClean (W_CachePatch(skullName[whichSkull]),
						x + SKULLXOFF, currentMenu->y - 5 + itemOn*LINEHEIGHT);
			}
		}
	}
}


//
// M_ClearMenus
//
void M_ClearMenus (void)
{
	if (FireScreen)
	{
		I_FreeScreen(FireScreen);
		FireScreen = NULL;
	}
	MenuStackDepth = 0;
	menuactive = false;
	drawSkull = true;
	M_DemoNoPlay = false;
	if (gamestate != GS_FULLCONSOLE)
		I_ResumeMouse ();	// [RH] Recapture the mouse in windowed modes.
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
		I_PauseMouse ();
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
		S_Sound (CHAN_VOICE, "switches/normbutn", 1, ATTN_NONE);
	} else {
		M_ClearMenus ();
		S_Sound (CHAN_VOICE, "switches/exitbutn", 1, ATTN_NONE);
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
	{
		// [Toke - CTF] skip skins selection
		if (gametype == GM_CTF)
			if (itemOn == 6)
				itemOn = 5;

		M_PlayerSetupTicker ();
	}
	MenuTime++;
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
	MenuTime = 0;
	SkullBaseLump = (gamemode == registered_heretic ? W_GetNumForName("M_SKL00") : 0);
	itemOn = currentMenu->lastOn;
	whichSkull = 0;
	skullAnimCounter = 10;
	drawSkull = true;
	screenSize = (int)screenblocks - 3;
	messageToPrint = 0;
	messageString = NULL;
	messageLastMenuActive = menuactive;

    if (gamemode == commercial)
    {
        // Commercial has no "read this" entry.
        MainDef.numitems = d2_main_end;
        MainDef.menuitems = Doom2MainMenu;

        MainDef.y += 8;
    }
    else if (gamemode == registered_heretic)
    {
    	// Heretic changes stuff
		MainDef.numitems = htc_main_end;
        MainDef.menuitems = HereticMainMenu;
        MainDef.x = 110;
        MainDef.y = 56;
    }

	M_OptInit ();

	// [RH] Build a palette translation table for the fire
	for (i = 0; i < 255; i++)
		FireRemap[i] = BestColor (DefaultPalette->basecolors, i, 0, 0, DefaultPalette->numcolors);
}

VERSION_CONTROL (m_menu_cpp, "$Id$")


