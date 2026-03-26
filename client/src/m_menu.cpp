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
//		DOOM selection menu, options, episode etc.
//		Sliders and icons. Kinda widget stuff.
//
//-----------------------------------------------------------------------------


#include "odamex.h"

#include <ctime>

#include "gstrings.h"
#include "c_console.h"
#include "c_dispatch.h"
#include "d_pages.h"
#include "d_main.h"
#include "i_music.h"
#include "i_time.h"
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
#include "v_palette.h"
#include "v_text.h"
#include "st_stuff.h"
#include "p_ctf.h"
#include "r_sky.h"
#include "cl_main.h"
#include "c_bind.h"
#include "cl_responderkeys.h"
#include "m_menuconf.h"

#include "gi.h"
#include "g_skill.h"
#include "m_fileio.h"

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

static page_image_t help_page;

enum class oldmenustring_t
{
	NONE,
	SAVEGAME,
	PLAYERNAME
};

// we are going to be entering a savegame string
oldmenustring_t		genStringEnter;
size_t				genStringLen;	// [RH] Max # of chars that can be entered
void	(*genStringEnd)(int slot);
int 				saveSlot;		// which slot to save in
size_t 				saveCharIndex;	// which char we're editing
// old save description before edit
char				saveOldString[SAVESTRINGSIZE];

int                 repeatKey;
int                 repeatCount;

extern bool			sendpause;
char				savegamestrings[10][SAVESTRINGSIZE];

menustack_t			MenuStack[16];
int					MenuStackDepth;

short				itemOn; 			// menu item indicator is on
static int			SkullBaseLump;		// lump number of first large skull in animation
static int			MenuTime;			// Ticker for Heretic skulls
short				indicatorAnimCounter;	// indicator animation counter
short				whichIndicator; 		// which indicator to draw
bool				drawIndicator;			// [RH] don't always draw indicator

// hack for PlayerSetup
int					PSetupDepth;

// current menudef
oldmenu_t *currentMenu;
struct configuredoldmenubridge_t
{
	const char* menuId = nullptr;
	oldmenu_t* menu = nullptr;
	std::vector<menuconfitem_t> items;
	std::vector<oldmenuitem_t> legacyItems;
};

static configuredoldmenubridge_t gConfiguredMainBridge;
static configuredoldmenubridge_t gConfiguredEpisodeBridge;
static configuredoldmenubridge_t gConfiguredExpansionBridge;
static configuredoldmenubridge_t gConfiguredSkillBridge;
static configuredoldmenubridge_t gConfiguredGameFilesBridge;
static std::string gSelectedEpisodeId;

//
// PROTOTYPES
//
void M_NewGame(int choice);
void M_ChooseSkill(int choice);
void M_LoadGame(int choice);
void M_SaveGame(int choice);
void M_Options(int choice);
void M_EndGame(int choice);
void M_ReadThis(int choice);
void M_ReadThis2(int choice);
void M_ReadThis3(int choice);
void M_QuitGame(int choice);
void M_ActivateConfiguredMenuItem(int choice);

void M_ChangeDetail(int choice);
void M_StartGame(int choice);
void M_Sound(int choice);

void M_FinishReadThis(int choice);
void M_LoadSelect(int choice);
void M_SaveSelect(int choice);
void M_ReadSaveStrings();
void M_QuickSave();
void M_QuickLoad();

void M_DrawMainMenu();
void M_DrawReadThis1();
void M_DrawReadThis2();
void M_DrawReadThis3();
void M_DrawOptions();
void M_DrawSound();
void M_DrawLoad();
void M_DrawSave();

void M_DrawSaveLoadBorder(int x,int y, int len);
void M_SetupNextMenu(oldmenu_t *menudef);
void M_DrawEmptyCell(oldmenu_t *menu,int item);
void M_DrawSelCell(oldmenu_t *menu,int item);
void M_StartControlPanel();
void M_StartMessage(const char *string,void (*routine)(int),bool input);
void M_StopMessage();
void M_ClearMenus();

// [RH] For player setup menu.
static void M_PlayerSetupTicker();
static void M_PlayerSetupDrawer();
static void M_EditPlayerName (int choice);
static void M_PlayerNameChanged (int choice);
static void M_ChangeGender (int choice);
static void M_ChangeAutoAim (int choice);
static void M_ChangeColorPreset (int choice);
static void SendNewColor (int red, int green, int blue);
static void M_SlidePlayerRed (int choice);
static void M_SlidePlayerGreen (int choice);
static void M_SlidePlayerBlue (int choice);
namespace
{
	struct menudestination_t;
	bool M_OpenMenuTarget(const std::string& target);
	bool M_OpenMenuEntrypoint(const std::string& name);
}
bool M_DemoNoPlay;

static IWindowSurface* fire_surface;
static constexpr int fire_surface_width = 72;
static constexpr int fire_surface_height = 77;

static int M_BigFontLineHeight()
{
	const OFont* font = OFonts.big();
	return font != nullptr ? font->lineHeight() : 0;
}

static int M_SmallFontLineHeight()
{
	const OFont* font = OFonts.small();
	return font != nullptr ? font->lineHeight() : 0;
}

static void M_PauseSound(void)
{
	if (paused || gamestate != GS_LEVEL || multiplayer || demoplayback ||
	    netdemo.isPlaying())
	{
		return;
	}

	S_PauseSound();
}

static void M_ResumeSound(void)
{
	if (paused || gamestate != GS_LEVEL || multiplayer || demoplayback ||
	    netdemo.isPlaying())
	{
		return;
	}

	S_ResumeSound();
}

oldmenu_t MainDef =
{
	0,
	nullptr,
	M_DrawMainMenu,
	97,64,
	0
};

oldmenu_t EpiDef =
{
	0,
	nullptr,
	nullptr,
	48,63,				// x,y
	0	 				// lastOn
};

oldmenu_t ExpDef =
{
	0,
	nullptr,
	nullptr,
	48,63,				// x,y
	0
};


//
// NEW GAME
//

oldmenu_t NewDef =
{
	0,
	nullptr,
	nullptr,
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
	{ 1,"","\0", M_EditPlayerName, 'N' },
	{ 2,"","\0", M_ChangeTeam, 'T' },
	{ 2,"","\0", M_ChangeGender, 'E' },
	{ 2,"","\0", M_ChangeAutoAim, 'A' },
    { 2,"","\0", M_ChangeColorPreset, 'C' },
	{ 2,"","\0", M_SlidePlayerRed, 'R' },
	{ 2,"","\0", M_SlidePlayerGreen, 'G' },
	{ 2,"","\0", M_SlidePlayerBlue, 'B' }
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
	{1,"","\0",M_ReadThis2,0}
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
	{1,"","\0",M_ReadThis3,0}
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
	{1,"","\0",M_FinishReadThis,0}
};

oldmenu_t ReadDef3 =
{
	read3_end,
	ReadMenu3,
	M_DrawReadThis3,
	330,175,
	0
};

oldmenu_t GameFilesDef =
{
	0,
	nullptr,
	nullptr,
	110,60,
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
	{1,"","\0", M_LoadSelect,'1'},
	{1,"","\0", M_LoadSelect,'2'},
	{1,"","\0", M_LoadSelect,'3'},
	{1,"","\0", M_LoadSelect,'4'},
	{1,"","\0", M_LoadSelect,'5'},
	{1,"","\0", M_LoadSelect,'6'},
	{1,"","\0", M_LoadSelect,'7'},
	{1,"","\0", M_LoadSelect,'8'},
};

oldmenu_t LoadDef =
{
	load_end,
	LoadSavegameMenu,
	M_DrawLoad,
	76,54,
	0
};

oldmenu_t HticLoadDef =
{
	load_end,
	LoadSavegameMenu,
	M_DrawLoad,
	62,20,
	0
};

//
// SAVE GAME MENU
//
oldmenuitem_t SaveMenu[]=
{
	{1,"","\0", M_SaveSelect,'1'},
	{1,"","\0", M_SaveSelect,'2'},
	{1,"","\0", M_SaveSelect,'3'},
	{1,"","\0", M_SaveSelect,'4'},
	{1,"","\0", M_SaveSelect,'5'},
	{1,"","\0", M_SaveSelect,'6'},
	{1,"","\0", M_SaveSelect,'7'},
	{1,"","\0", M_SaveSelect,'8'}
};

oldmenu_t SaveDef =
{
	load_end,
	SaveMenu,
	M_DrawSave,
	76,54,
	0
};
oldmenu_t HticSaveDef =
{
	load_end,
	SaveMenu,
	M_DrawSave,
	62,20,
	0
};

// [RH] Most menus can now be accessed directly
// through console commands.
BEGIN_COMMAND (menu_main)
{
	M_StartControlPanel ();
	if (M_OpenMenuEntrypoint("mainMenu"))
	{
		PSetupDepth = 2;
	}
}
END_COMMAND (menu_main)

BEGIN_COMMAND (menu_help)
{
    // F1
	M_StartControlPanel ();
	M_ReadThis(0);
}
END_COMMAND (menu_help)

BEGIN_COMMAND (menu_save)
{
    // F2
	M_StartControlPanel ();
	M_SaveGame (0);
	//Printf (PRINT_WARNING, "Saving is not available at this time.\n");
}
END_COMMAND (menu_save)

BEGIN_COMMAND (menu_load)
{
    // F3
	M_StartControlPanel ();
	M_LoadGame (0);
	//Printf (PRINT_WARNING, "Loading is not available at this time.\n");
}
END_COMMAND (menu_load)

BEGIN_COMMAND (menu_options)
{
    // F4
    M_StartControlPanel ();
	if (M_OpenMenuEntrypoint("optionsMenu"))
	{
		PSetupDepth = 1;
	}
}
END_COMMAND (menu_options)

BEGIN_COMMAND (quicksave)
{
    // F6
	M_StartControlPanel ();
	M_QuickSave ();
	//Printf (PRINT_WARNING, "Saving is not available at this time.\n");
}
END_COMMAND (quicksave)

BEGIN_COMMAND (menu_endgame)
{	// F7
	M_StartControlPanel ();
	M_EndGame(0);
}
END_COMMAND (menu_endgame)

BEGIN_COMMAND (quickload)
{
    // F9
	M_StartControlPanel ();
	M_QuickLoad ();
	//Printf (PRINT_WARNING, "Loading is not available at this time.\n");
}
END_COMMAND (quickload)

BEGIN_COMMAND (menu_quit)
{	// F10
	M_StartControlPanel ();
	M_QuitGame(0);
}
END_COMMAND (menu_quit)

BEGIN_COMMAND (menu_player)
{
	M_StartControlPanel ();
	M_PlayerSetup(0);
	PSetupDepth = 0;
}
END_COMMAND (menu_player)

static const char* LocalizedString(const char* key)
{
	if (GStrings.hasString(key))
	{
		const char* s = GStrings(key);
		if (s && s[0])
			return s;
	}
	return key;
}

namespace
{
	enum class menudestinationkind_t
	{
		invalid,
		menu,
		builtin
	};

	struct menudestination_t
	{
		menudestinationkind_t kind = menudestinationkind_t::invalid;
		std::string id;
	};

	configuredoldmenubridge_t* MenuBridgeByMenu(oldmenu_t* menu)
	{
		configuredoldmenubridge_t* bridges[] = {
			&gConfiguredMainBridge,
			&gConfiguredEpisodeBridge,
			&gConfiguredExpansionBridge,
			&gConfiguredSkillBridge,
			&gConfiguredGameFilesBridge
		};

		for (configuredoldmenubridge_t* bridge : bridges)
		{
			if (bridge->menu == menu)
			{
				return bridge;
			}
		}

		return nullptr;
	}

	const menuconftheme_t& MenuConfTheme()
	{
		return M_MenuConf().theme;
	}

	const menuconfmenu_t* MenuConfMenu(const char* id)
	{
		const auto it = M_MenuConf().menus.find(id);
		return it != M_MenuConf().menus.end() ? &it->second : nullptr;
	}

	const patch_t* MenuConfPatch(const std::string& name)
	{
		return !name.empty() && W_CheckNumForName(name.c_str()) >= 0 ? W_CachePatch(name.c_str()) : nullptr;
	}

	const patch_t* MenuIndicatorPatch(int which)
	{
		const auto& patches = MenuConfTheme().indicator.patches;
		if (patches.empty())
		{
			static constexpr const char* kFallback[] = { "M_SKULL1", "M_SKULL2" };
			return MenuConfPatch(kFallback[which & 1]);
		}

		const std::string& name = patches[which % patches.size()];
		return MenuConfPatch(name);
	}

	const patch_t* MenuInputBoxFullPatch()
	{
		const patch_t* patch = MenuConfPatch(MenuConfTheme().inputBox.fullPatch);
		return patch != nullptr ? patch : MenuConfPatch("M_FSLOT");
	}

	const patch_t* MenuInputBoxLeftPatch()
	{
		const patch_t* patch = MenuConfPatch(MenuConfTheme().inputBox.leftPatch);
		return patch != nullptr ? patch : MenuConfPatch("M_LSLEFT");
	}

	const patch_t* MenuInputBoxMiddlePatch()
	{
		const patch_t* patch = MenuConfPatch(MenuConfTheme().inputBox.middlePatch);
		return patch != nullptr ? patch : MenuConfPatch("M_LSCNTR");
	}

	const patch_t* MenuInputBoxRightPatch()
	{
		const patch_t* patch = MenuConfPatch(MenuConfTheme().inputBox.rightPatch);
		return patch != nullptr ? patch : MenuConfPatch("M_LSRGHT");
	}

	void WarnMenuConf(const std::string& message)
	{
		PrintFmt(PRINT_WARNING, "MENUCONF: {}\n", message);
	}

	int SkillIndexForId(const std::string& id)
	{
		if (id == "baby") return 0;
		if (id == "easy") return 1;
		if (id == "normal") return 2;
		if (id == "hard") return 3;
		if (id == "nightmare") return 4;
		return -1;
	}

	void DrawConfiguredMenu()
	{
		configuredoldmenubridge_t* bridge = MenuBridgeByMenu(currentMenu);
		if (bridge == nullptr || bridge->menuId == nullptr)
		{
			return;
		}

		const menuconfmenu_t* menu = MenuConfMenu(bridge->menuId);
		if (menu == nullptr)
		{
			return;
		}

		const patch_t* headerPatch = MenuConfPatch(menu->header.patch);
		if (headerPatch != nullptr)
		{
			int x = (320 - headerPatch->width()) / 2;
			if (iequals(menu->header.align, "absolute"))
			{
				x = menu->header.x;
			}
			else
			{
				x += menu->header.x;
			}

			screen->DrawPatchClean(headerPatch, x, menu->header.y);
			return;
		}

		const char* headerText = nullptr;
		if (!menu->header.languageKey.empty())
		{
			headerText = LocalizedString(menu->header.languageKey.c_str());
		}
		else if (!menu->header.text.empty())
		{
			headerText = menu->header.text.c_str();
		}

		if (headerText != nullptr)
		{
			const OFont* bigFont = OFonts.big();
			int x = 160 - V_StringWidth(bigFont, headerText) / 2;
			if (iequals(menu->header.align, "absolute"))
			{
				x = menu->header.x;
			}
			else
			{
				x += menu->header.x;
			}

			screen->DrawTextCleanMove(bigFont, CR_GRAY, x, menu->header.y, headerText);
		}
	}

	bool M_ResolveMenuTarget(const std::string& target, menudestination_t& out)
	{
		out = menudestination_t();
		if (target.empty())
		{
			return false;
		}

		if (target.rfind("builtin:", 0) == 0)
		{
			const std::string builtinId = target.substr(8);
			if (builtinId == "help" || builtinId == "loadGame" || builtinId == "saveGame" ||
			    builtinId == "playerSetup" || builtinId == "videoMode")
			{
				out.kind = menudestinationkind_t::builtin;
				out.id = builtinId;
				return true;
			}

			WarnMenuConf(fmt::sprintf("unsupported builtin target \"%s\"", target.c_str()));
			return false;
		}

		if (M_MenuConf().menus.find(target) == M_MenuConf().menus.end())
		{
			WarnMenuConf(fmt::sprintf("unknown menu target \"%s\"", target.c_str()));
			return false;
		}

		out.kind = menudestinationkind_t::menu;
		out.id = target;
		return true;
	}

	bool M_ResolveMenuEntrypoint(const std::string& name, menudestination_t& out)
	{
		const auto it = M_MenuConf().entrypoints.find(name);
		if (it == M_MenuConf().entrypoints.end())
		{
			WarnMenuConf(fmt::sprintf("missing entrypoint \"%s\"", name.c_str()));
			out = menudestination_t();
			return false;
		}

		return M_ResolveMenuTarget(it->second, out);
	}

	bool M_OpenBuiltinTarget(const std::string& builtinId)
	{
		if (builtinId == "help")
		{
			M_ReadThis(0);
			return true;
		}
		if (builtinId == "loadGame")
		{
			M_LoadGame(0);
			return true;
		}
		if (builtinId == "saveGame")
		{
			M_SaveGame(0);
			return true;
		}
		if (builtinId == "playerSetup")
		{
			M_PlayerSetup(0);
			return true;
		}
		if (builtinId == "videoMode")
		{
			AddCommandString("menu_video");
			return true;
		}

		WarnMenuConf(fmt::sprintf("unsupported builtin target \"builtin:%s\"", builtinId.c_str()));
		return false;
	}

	bool M_OpenResolvedDestination(const menudestination_t& destination)
	{
		switch (destination.kind)
		{
		case menudestinationkind_t::builtin:
			return M_OpenBuiltinTarget(destination.id);

		case menudestinationkind_t::menu:
			if (destination.id == "main")
			{
				M_SetupNextMenu(&MainDef);
				return true;
			}
			if (destination.id == "options")
			{
				OptionsActive = M_StartOptionsMenu();
				return true;
			}
			if (destination.id == "episodes")
			{
				M_SetupNextMenu(&EpiDef);
				return true;
			}
			if (destination.id == "skills")
			{
				M_SetupNextMenu(&NewDef);
				return true;
			}
			if (destination.id == "expansions")
			{
				M_SetupNextMenu(&ExpDef);
				return true;
			}
			if (destination.id == "gamefiles")
			{
				M_SetupNextMenu(&GameFilesDef);
				return true;
			}

			WarnMenuConf(fmt::sprintf("menu target \"%s\" is not wired to the legacy runtime yet",
			                          destination.id.c_str()));
			return false;

		case menudestinationkind_t::invalid:
		default:
			return false;
		}
	}

	bool M_OpenMenuTarget(const std::string& target)
	{
		menudestination_t destination;
		return M_ResolveMenuTarget(target, destination) && M_OpenResolvedDestination(destination);
	}

	bool M_OpenMenuEntrypoint(const std::string& name)
	{
		menudestination_t destination;
		return M_ResolveMenuEntrypoint(name, destination) && M_OpenResolvedDestination(destination);
	}

	bool BuildConfiguredOldMenu(configuredoldmenubridge_t& bridge, const char* menuId, oldmenu_t& menu,
	                            void (*routine)())
	{
		const menuconfmenu_t* configuredMenu = MenuConfMenu(menuId);
		if (configuredMenu == nullptr || configuredMenu->items.empty())
		{
			bridge.items.clear();
			bridge.legacyItems.clear();
			bridge.menuId = menuId;
			bridge.menu = &menu;
			menu.menuitems = nullptr;
			menu.numitems = 0;
			return false;
		}

		bridge.menuId = menuId;
		bridge.menu = &menu;
		bridge.items = configuredMenu->items;
		if (iequals(menuId, "episodes") &&
		    episodenum > 0 && static_cast<int>(bridge.items.size()) > episodenum)
		{
			bridge.items.resize(episodenum);
		}
		bridge.legacyItems.clear();
		bridge.legacyItems.reserve(bridge.items.size());

		for (const menuconfitem_t& item : bridge.items)
		{
			oldmenuitem_t legacy = {};
			legacy.status = item.kind == menuconfitemkind_t::separator ? -1 : 1;
			legacy.routine = legacy.status == -1 ? nullptr : M_ActivateConfiguredMenuItem;
			legacy.alphaKey = item.hotkey.empty() ? 0 : item.hotkey[0];

			if (!item.patch.empty())
			{
				legacy.name = item.patch.c_str();
			}

			const char* text = nullptr;
			if (!item.languageKey.empty())
			{
				text = item.languageKey.c_str();
			}
			else if (!item.text.empty())
			{
				text = item.text.c_str();
			}

			if (text != nullptr)
			{
				M_StringCopy(legacy.textname, text, sizeof(legacy.textname));
			}

			bridge.legacyItems.push_back(legacy);
		}

		menu.menuitems = bridge.legacyItems.data();
		menu.numitems = static_cast<short>(bridge.legacyItems.size());
		menu.routine = routine;
		menu.lastOn = iequals(menuId, "skills") ? defaultskillmenu : 0;
		if (configuredMenu->layout.x != 0) menu.x = configuredMenu->layout.x;
		if (configuredMenu->layout.y != 0) menu.y = configuredMenu->layout.y;
		return true;
	}

	void DrawMainMenuHeaderDecorations()
	{
		const menuconfmenu_t* mainMenu = MenuConfMenu("main");
		if (mainMenu == nullptr || !mainMenu->header.decorations.defined)
		{
			return;
		}

		const auto& decorations = mainMenu->header.decorations;
		const int frameCount = std::max(1, decorations.left.frameCount > 0 ?
			decorations.left.frameCount : decorations.right.frameCount);
		const int frameTics = std::max(1, decorations.frameTics);
		const int frame = (MenuTime / frameTics) % frameCount;

		auto drawSide = [frame, frameCount](const menuconfheadertside_t& side)
		{
			if (side.basePatch.empty())
			{
				return;
			}

			const int baseLump = W_CheckNumForName(side.basePatch.c_str());
			if (baseLump < 0)
			{
				return;
			}

			int drawFrame = frame;
			if (iequals(side.animateDirection, "reverse"))
			{
				drawFrame = frameCount - 1 - frame;
			}

			screen->DrawPatchClean(W_CachePatch(baseLump + drawFrame, PU_CACHE), side.x, side.y);
		};

		drawSide(decorations.left);
		drawSide(decorations.right);
	}
}

void M_ActivateConfiguredMenuItem(int choice)
{
	configuredoldmenubridge_t* bridge = MenuBridgeByMenu(currentMenu);
	if (bridge == nullptr || choice < 0 || static_cast<size_t>(choice) >= bridge->items.size())
	{
		return;
	}

	const menuconfitem_t& item = bridge->items[choice];
	bool actionSucceeded = true;

	if (!item.action.empty())
	{
		if (item.action == "quitGame")
		{
			M_QuitGame(choice);
		}
		else if (item.action == "chooseEpisode")
		{
			if ((gameinfo.flags & GI_SHAREWARE) && choice > 0)
			{
				const char* sharewareMessage =
				    gameinfo.sharewareMessage.empty() ? GStrings(SWSTRING) :
				                                       LocalizedString(gameinfo.sharewareMessage.c_str());
				M_StartMessage(sharewareMessage, NULL, false);
				M_ClearMenus();
				return;
			}

			gSelectedEpisodeId.clear();
			if (item.params.isObject() && item.params["episode"].isString())
			{
				gSelectedEpisodeId = item.params["episode"].asString();
			}

			if (choice >= 0 && choice < episodenum && EpisodeInfos[choice].noskillmenu)
			{
				M_StartGame(defaultskillmenu);
				return;
			}
		}
		else if (item.action == "startGame")
		{
			if (!(item.params.isObject() && item.params["skill"].isString()))
			{
				WarnMenuConf("startGame item is missing a string skill param");
				actionSucceeded = false;
			}
			else
			{
				const int skill = SkillIndexForId(item.params["skill"].asString());
				if (skill < 0)
				{
					WarnMenuConf(fmt::sprintf("unknown skill id \"%s\"",
					                          item.params["skill"].asCString()));
					actionSucceeded = false;
				}
				else
				{
					M_ChooseSkill(skill);
				}
			}
		}
		else
		{
			WarnMenuConf(fmt::sprintf("menu action \"%s\" is not wired yet", item.action.c_str()));
			actionSucceeded = false;
		}
	}

	if (actionSucceeded && !item.target.empty())
	{
		M_OpenMenuTarget(item.target);
	}
}

//
// M_ReadSaveStrings
//	read the strings from the savegame files
//
void M_ReadSaveStrings()
{
	for (int i = 0; i < load_end; i++)
	{
		std::string name;

		G_BuildSaveName (name, i);

		auto handle = uqFile(fopen(name.c_str(), "rb"));
		if (handle == nullptr)
		{
			M_StringCopy(&savegamestrings[i][0], GStrings(EMPTYSTRING), SAVESTRINGSIZE);
			LoadSavegameMenu[i].status = 0;
		}
		else
		{
			const size_t readlen = fread (&savegamestrings[i], SAVESTRINGSIZE, 1, handle.get());
			if (readlen < 1)
			{
				fmt::print("M_Read_SaveStrings(): Failed to read handle.\n");
				return;
			}
			LoadSavegameMenu[i].status = 1;
		}
	}
}


//
// M_LoadGame & Cie.
//
void M_DrawInputBox (char *text, int x, int y, int width) 
{
	const OFont* smallFont = OFonts.small();
	const int text_y =  y+(M_BigFontLineHeight()/2 - M_SmallFontLineHeight()/2);

	M_DrawSaveLoadBorder(x, y, width);
	screen->DrawTextCleanMove(smallFont, CR_RED, x + (M_SmallFontLineHeight() / 2), text_y, text);
}

void M_DrawLoad ()
{
	const OFont* bigFont = OFonts.big();
	int i, list_y;
	const int slot_width = 24;
	const int slot_padding = 2;
	const int slot_height = M_BigFontLineHeight() - slot_padding;

	if (W_CheckNumForName("M_LOADG") >= 0)
	{
		screen->DrawPatchClean(W_CachePatch("M_LOADG"), 72, 28);
	}
	else
	{
		const char* title = LocalizedString("MNU_LOADGAME");
		screen->DrawTextCleanMove(bigFont, CR_GRAY, 160 - V_StringWidth(bigFont, title) / 2, 0,
		                          title);
	}

	list_y = LoadDef.y;
	for (i = 0; i < load_end; i++)
	{
		M_DrawInputBox(savegamestrings[i], LoadDef.x, list_y, slot_width);
		list_y += slot_height + slot_padding;
	}
}

//
// User wants to load this game
//
void M_LoadSelect (int choice)
{
	std::string name;

	G_BuildSaveName (name, choice);
	G_LoadGame(name);
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
void M_DrawSave()
{
	const OFont* bigFont = OFonts.big();
	const OFont* smallFont = OFonts.small();
	int i, list_y;
	const int slot_width = 24;
	const int slot_padding = 2;
	const int slot_height = M_BigFontLineHeight() - slot_padding;

	if (W_CheckNumForName("M_SAVEG") >= 0)
	{
		screen->DrawPatchClean (W_CachePatch("M_SAVEG"), 72, 28);
	}
	else
	{
		const char* title = LocalizedString("MNU_SAVEGAME");
		screen->DrawTextCleanMove(bigFont, CR_GRAY, 160 - V_StringWidth(bigFont, title) / 2, 0,
		                          title);
	}

	list_y = SaveDef.y;
	for (i = 0; i < load_end; i++)
	{
		M_DrawInputBox(savegamestrings[i], SaveDef.x, list_y, slot_width);
		list_y += slot_height + slot_padding;
	}

	if (genStringEnter != oldmenustring_t::NONE)
	{
		const int string_width = V_StringWidth(smallFont, savegamestrings[saveSlot]);
		screen->DrawTextCleanMove(smallFont, CR_RED, SaveDef.x + string_width,
		                          SaveDef.y + M_BigFontLineHeight() * saveSlot, "_");
	}
}


//
// M_Responder calls this when user is finished
// [ML] 7 Sept 08: Bringing game saving/loading in from
//                 zdoom 1.22 source, see MAINTAINERS
//
void M_DoSave (int slot)
{
	G_SaveGame (slot, { savegamestrings[slot], 24 });
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
	const time_t ti = time(NULL);
	const tm *lt = localtime(&ti);

	// we are going to be intercepting all chars
	genStringEnter = oldmenustring_t::SAVEGAME;
	genStringEnd = M_DoSave;
	genStringLen = SAVESTRINGSIZE-1;

	saveSlot = choice;
	M_StringCopy(saveOldString, savegamestrings[choice], SAVESTRINGSIZE);

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
//	M_QuickSave
//	[ML] 7 Sept 08: Bringing game saving/loading in from
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

void M_QuickSave()
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
	snprintf (tempstring, 80, GStrings(QSPROMPT), savegamestrings[quickSaveSlot]);
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


void M_QuickLoad()
{
	if (quickSaveSlot < 0)
	{
		M_StartControlPanel();
		M_LoadGame (0);
		return;
	}
	snprintf(tempstring, 80, GStrings(QLPROMPT),savegamestrings[quickSaveSlot]);
	M_StartMessage(tempstring,M_QuickLoadResponse,true);
}


//
// M_ReadThis
//
void M_ReadThis(int)
{
	drawIndicator = false;
	D_LoadPageImage(help_page, gameinfo.infoPage[0]);
	M_SetupNextMenu(&ReadDef1);
}

void M_ReadThis2(int)
{
	drawIndicator = false;
	D_LoadPageImage(help_page, gameinfo.infoPage[1]);
	M_SetupNextMenu(&ReadDef2);
}

void M_ReadThis3(int)
{
    if (gameinfo.flags & GI_SHAREWARE) {
        drawIndicator = false;
        D_LoadPageImage(help_page, gameinfo.infoPage[2]);
        M_SetupNextMenu(&ReadDef3);
    } else {
        M_FinishReadThis(0);
    }
}

static void M_DrawHelpPage()
{
	D_DrawPageImage(help_page, I_GetPrimarySurface(), true);
}

void M_FinishReadThis(int)
{
	drawIndicator = true;
	D_FreePageImage(help_page);
	MenuStackDepth = 0;
	M_SetupNextMenu(&MainDef);
}

//
// Draw border for the savegame description
// [RH] Width of the border is variable
//
void M_DrawSaveLoadBorder (int x, int y, int len)
{
	const patch_t* full_slot = MenuInputBoxFullPatch();
	const patch_t* left_slot = MenuInputBoxLeftPatch();
	const patch_t* center_slot = MenuInputBoxMiddlePatch();
	const patch_t* right_slot = MenuInputBoxRightPatch();

	if (full_slot != nullptr)
	{
		screen->DrawPatchClean (full_slot, x, y);
	}
	else
	{
		screen->DrawPatchCleanNoOffsets (left_slot, x, y);

		for (int i = 0; i < len; i++)
		{
			x += M_SmallFontLineHeight();
			screen->DrawPatchCleanNoOffsets (center_slot, x, y);
		}

		screen->DrawPatchCleanNoOffsets (right_slot, x, y);
	}
}

//
// M_DrawMainMenu
//
void M_DrawMainMenu()
{
	const menuconfmenu_t* mainMenu = MenuConfMenu("main");
	if (mainMenu == nullptr)
	{
		return;
	}

	const patch_t* menu_title = MenuConfPatch(mainMenu->header.patch);
	if (menu_title == nullptr)
	{
		return;
	}

	int menu_title_x = (320 - menu_title->width()) / 2;
	int menu_title_y = 1;

	const auto& header = mainMenu->header;
	if (iequals(header.align, "absolute"))
	{
		menu_title_x = header.x;
	}
	else
	{
		menu_title_x += header.x;
	}
	menu_title_y = header.y;

	screen->DrawPatchClean(menu_title, menu_title_x, menu_title_y);
	DrawMainMenuHeaderDecorations();
}

void M_NewGame(int choice)
{
	gSelectedEpisodeId.clear();
	M_OpenMenuEntrypoint("newGameMenu");
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
		if (gSelectedEpisodeId.empty())
		{
			WarnMenuConf("startGame was invoked without a selected expansion");
			return;
		}

		const std::string str = "nerve.wad";
		const bool isNerve = iequals(gSelectedEpisodeId, "doom2.nerve");

		if (isNerve)
		{
			G_LoadWadString(str, "");
		}
		else
		{
			for (unsigned int i = 2; i < wadfiles.size(); i++)
			{
				if (iequals(str, wadfiles[i].getBasename()))
				{
					G_LoadWadString(wadfiles[1].getFullpath(), "");
				}
			}

			G_DeferedInitNew(CalcMapName(1, 1));
		}
	}
	else if (gamemode == commercial)
	{
		G_DeferedInitNew(CalcMapName(1, 1));
	}
	else if (!gSelectedEpisodeId.empty())
	{
		const std::string::size_type end = gSelectedEpisodeId.find_last_not_of("0123456789");
		const std::string::size_type pos = end == std::string::npos ? 0 : end + 1;
		if (pos < gSelectedEpisodeId.size())
		{
			const int episodeIndex =
			    clamp(std::max(1, atoi(gSelectedEpisodeId.c_str() + pos)) - 1, 0, episodenum - 1);
			G_DeferedInitNew(EpisodeMaps[episodeIndex]);
		}
		else
		{
			WarnMenuConf(fmt::sprintf("episode id \"%s\" has no numeric suffix",
			                          gSelectedEpisodeId.c_str()));
			return;
		}
	}
	else
	{
		WarnMenuConf("startGame was invoked without a selected episode");
		return;
	}

	M_ClearMenus ();
	gSelectedEpisodeId.clear();
}

void M_ChooseSkill(int choice)
{
	if (SkillInfos[choice].must_confirm)
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

//
// Read This Menus
// Had a "quick hack to fix romero bug"
//
void M_DrawReadThis1()
{
	M_DrawHelpPage();
}

//
// Read This Menus - optional second page.
//
void M_DrawReadThis2()
{
	M_DrawHelpPage();
}

//
// Read This Menus - shareware third page.
//
void M_DrawReadThis3()
{
	M_DrawHelpPage();
}

//
// M_Options
//
void M_DrawOptions()
{
	if (W_CheckNumForName("M_OPTTTL") >= 0)
	{
		screen->DrawPatchClean(W_CachePatch("M_OPTTTL"), 108, 15);
	}
	else
	{
		screen->DrawTextCleanMove(OFonts.big(), CR_GRAY, 108, 15, LocalizedString("MNU_OPTIONS"));
	}
}

void M_Options(int choice)
{
	M_OpenMenuEntrypoint("optionsMenu");
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
	S_StopAmbientSound();
	M_ClearMenus ();
	D_StartTitle ();
	CL_QuitNetGame(NQ_SILENT);
}

void M_EndGame(int)
{
	if (!usergame)
	{
		S_Sound (CHAN_INTERFACE, "player/male/grunt1", 1, ATTN_NONE);
		return;
	}

	const OString endgame_message = multiplayer ? NETEND : 
		(gameinfo.enginetype == ENGINE_HERETIC ? RAVENENDGAME : ENDGAME);

	M_StartMessage(GStrings(endgame_message), M_EndGameResponse, true);
}

//
// M_QuitGame
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

static const std::string M_QuitMessage()
{
	const int count = gameinfo.quitMessageCount > 0 ? gameinfo.quitMessageCount : 1;
	const int base_index = GStrings.toIndex(StdStringToUpper(gameinfo.quitMessage));

	std::string message;
	if (base_index < 0)
	{
		message = LocalizedString(gameinfo.quitMessage.c_str());
	}
	else
	{
		const int offset = count > 1 ? gametic % count : 0;
		const char* indexed = GStrings.getIndex(base_index + offset);

		message = (indexed == nullptr || indexed[0] == '\0') ? 
			LocalizedString(gameinfo.quitMessage.c_str()) : indexed;
	}

	if (!gameinfo.quitPrompt.empty())
	{
		return fmt::sprintf("%s\n\n%s", message, LocalizedString(gameinfo.quitPrompt.c_str()));
	}

	return fmt::sprintf("%s\n", message);
}

void M_QuitGame(int choice)
{
	static std::string endstring = M_QuitMessage();
	M_StartMessage(endstring.c_str(), M_QuitResponse, true);
}

// -----------------------------------------------------
//		Player Setup Menu code
// -----------------------------------------------------

void M_DrawSlider(int x, int y, float leftval, float rightval, float cur, float step);

static const char *genders[4] = { "male", "female", "cyborg", "other" };
// [Acts 19 quiz] the order must match d_netinf.h
static const char* colorpresets[12] = { "green", "indigo", "brown", "red", "blue", "orange", "gold", "jungle green", "purple", "white", "black", "custom" };
static state_t *PlayerState;
static int PlayerTics;
argb_t CL_GetPlayerColor(const player_t&);


EXTERN_CVAR (cl_name)
EXTERN_CVAR (cl_team)
EXTERN_CVAR (cl_colorpreset)
EXTERN_CVAR (cl_customcolor)
EXTERN_CVAR (cl_color)
EXTERN_CVAR (cl_gender)
EXTERN_CVAR (cl_autoaim)

void M_PlayerSetup(int choice)
{
	M_StringCopy(savegamestrings[0], cl_name.cstring(), SAVESTRINGSIZE);
	M_SetupNextMenu (&PSetupDef);
	PlayerState = &states[mobjinfo[MT_PLAYER].seestate];
	PlayerTics = PlayerState->tics;

	if (fire_surface == NULL)
		fire_surface = I_AllocateSurface(fire_surface_width, fire_surface_height, 8);

	// [Nes] Intialize the player preview color.
	const argb_t player_color = CL_GetPlayerColor(consoleplayer());
	int colorpreset = D_ColorPreset(cl_colorpreset.cstring());
	R_BuildPlayerTranslation(menuplayer_id, player_color, colorpreset);
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
		PIXEL_T* to = reinterpret_cast<PIXEL_T*>(surface->getBuffer()) + y * surface_pitch + x;
		const palindex_t* from = static_cast<palindex_t*>(fire_surface->getBuffer()) + b * fire_surface->getPitch();
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

template<typename PIXEL_T>
static forceinline void R_RenderFire(int x, int y)
{
	IWindowSurface* surface = I_GetPrimarySurface();
	int surface_pitch = surface->getPitchInPixels();

	fire_surface->lock();

	for (int b = 0; b < fire_surface_height; b++)
	{
		PIXEL_T* to = reinterpret_cast<PIXEL_T*>(surface->getBuffer()) + y * surface_pitch + x;
		const palindex_t* from = static_cast<palindex_t*>(fire_surface->getBuffer()) + b * fire_surface->getPitch();
		y += CleanYfac;

		for (int a = 0; a < fire_surface_width; a++, to += CleanXfac, from++)
		{
			for (int c = CleanYfac; c; c--)
			{
				for (int i = 0; i < CleanXfac; ++i)
					*(to + surface_pitch * c + i) = R_FirePixel<PIXEL_T>(*from);
			}
		}
	}

	fire_surface->unlock();
}

static void M_PlayerSetupDrawer()
{
	const OFont* smallFont = OFonts.small();
	const palette_t* palette = V_GetPaletteFromLump("ODAPAL");
	int colorpreset = D_ColorPreset(cl_colorpreset.cstring());

	// Draw title
	{
		if (W_CheckNumForName("M_PSTTL") >= 0)
		{
			const patch_t* patch = W_CachePatch("M_PSTTL");
			screen->DrawPatchCleanWithPalette(patch, 160 - patch->width() / 2, 10, palette);
		}
		else
		{
			screen->DrawTextCleanMove(smallFont, CR_GRAY, 110, 10,
			                         LocalizedString("MNU_PLAYERSETUP"));
		}
	}

	// Draw player name box
	screen->DrawTextCleanMove(smallFont, CR_RED, PSetupDef.x, PSetupDef.y, "Name");
	M_DrawInputBox(savegamestrings[0], PSetupDef.x + 56, PSetupDef.y-4, MAXPLAYERNAME+1);

	// Draw cursor for either of the above
	if (genStringEnter != oldmenustring_t::NONE)
		screen->DrawTextCleanMove(smallFont, CR_RED,
							PSetupDef.x + V_StringWidth(smallFont, savegamestrings[saveSlot]) + 56,
							PSetupDef.y + ((saveSlot == 0) ? 0 : M_BigFontLineHeight()), "_");

	// Draw player character
	{
		int x = 320 - 88 - 32, y = PSetupDef.y + M_BigFontLineHeight()*3 - 14;

		x = (x-160)*CleanXfac+(I_GetSurfaceWidth() / 2);
		y = (y-100)*CleanYfac+(I_GetSurfaceHeight() / 2);
		if (!fire_surface)
		{
			const argb_t color = V_GetDefaultPalette()->basecolors[34];
			screen->Clear(x, y, x + fire_surface_width * CleanXfac, y + fire_surface_height * CleanYfac, color);
		}
		else
		{
			fire_surface->lock();
			const int pitch = fire_surface->getPitch();

			palindex_t* from = static_cast<palindex_t*>(fire_surface->getBuffer()) + (fire_surface_height - 3) * pitch;
			for (int a = 0; a < fire_surface_width; a++, from++)
				*from = *(from + (pitch << 1)) = M_Random();

			from = static_cast<palindex_t*>(fire_surface->getBuffer());
			for (int b = 0; b < fire_surface_height - 4; b += 2)
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
				for (int a = 1; a < fire_surface_width - 1; a++)
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
				else if (CleanXfac == 6) R_RenderFire<6, palindex_t>(x, y);
				else if (CleanXfac == 7) R_RenderFire<7, palindex_t>(x, y);
				else if (CleanXfac == 8) R_RenderFire<8, palindex_t>(x, y);
				else if (CleanXfac == 9) R_RenderFire<9, palindex_t>(x, y);
				else if (CleanXfac == 10) R_RenderFire<10, palindex_t>(x, y);
				else if (CleanXfac == 11) R_RenderFire<11, palindex_t>(x, y);
				else if (CleanXfac == 12) R_RenderFire<12, palindex_t>(x, y);
				else if (CleanXfac == 13) R_RenderFire<13, palindex_t>(x, y);
				else if (CleanXfac == 14) R_RenderFire<14, palindex_t>(x, y);
				else R_RenderFire<palindex_t>(x, y);
			}
			else
			{
				// 32bpp rendering:
				     if (CleanXfac == 1) R_RenderFire<1, argb_t>(x, y);
				else if (CleanXfac == 2) R_RenderFire<2, argb_t>(x, y);
				else if (CleanXfac == 3) R_RenderFire<3, argb_t>(x, y);
				else if (CleanXfac == 4) R_RenderFire<4, argb_t>(x, y);
				else if (CleanXfac == 5) R_RenderFire<5, argb_t>(x, y);
				else if (CleanXfac == 6) R_RenderFire<6, argb_t>(x, y);
				else if (CleanXfac == 7) R_RenderFire<7, argb_t>(x, y);
				else if (CleanXfac == 8) R_RenderFire<8, argb_t>(x, y);
				else if (CleanXfac == 9) R_RenderFire<9, argb_t>(x, y);
				else if (CleanXfac == 10) R_RenderFire<10, argb_t>(x, y);
				else if (CleanXfac == 11) R_RenderFire<11, argb_t>(x, y);
				else if (CleanXfac == 12) R_RenderFire<12, argb_t>(x, y);
				else if (CleanXfac == 13) R_RenderFire<13, argb_t>(x, y);
				else if (CleanXfac == 14) R_RenderFire<14, argb_t>(x, y);
				else R_RenderFire<argb_t>(x, y);
			}

			fire_surface->unlock();
		}
	}
	{
		const int32_t spritenum = states[mobjinfo[MT_PLAYER].spawnstate].sprite;
		const spriteframe_t* sprframe = &sprites[spritenum].spriteframes[PlayerState->frame & FF_FRAMEMASK];

		// [Nes] Color of player preview uses the unused translation table (player 0), instead
		// of the table of the current player color. (Which is different in single, demo, and team)
		const argb_t player_color = CL_GetPlayerColor(consoleplayer());
		R_BuildPlayerTranslation(menuplayer_id, player_color, colorpreset);
		V_ColorMap = translationref_t(translationtables, menuplayer_id);

		// Draw box surrounding fire and player:
		screen->DrawPatchCleanWithPalette(W_CachePatch("M_PBOX"), 320 - 88 - 32 + 36,
			PSetupDef.y + M_BigFontLineHeight() * 3 + 22, palette);

		screen->DrawTranslatedPatchClean (W_CachePatch (sprframe->lump[0]),
			320 - 52 - 32, PSetupDef.y + M_BigFontLineHeight()*3 + 46);
	}

	// Draw team setting
	{
		const OFont* smallFont = OFonts.small();
		const team_t team = D_TeamByName(cl_team.cstring());
		const int x = V_StringWidth(smallFont, "Preferred Team") + 8 + PSetupDef.x;
		screen->DrawTextCleanMove(smallFont, CR_RED, PSetupDef.x, PSetupDef.y + M_BigFontLineHeight(), "Preferred Team");
		screen->DrawTextCleanMove(smallFont, CR_GREY, x, PSetupDef.y + M_BigFontLineHeight(), team == TEAM_NONE ? "NONE" : GetTeamInfo(team)->ColorStringUpper.c_str());
	}

	// Draw gender setting
	{
		const OFont* smallFont = OFonts.small();
		const gender_t gender = D_GenderByName(cl_gender.cstring());
		const int x = V_StringWidth(smallFont, "Gender") + 8 + PSetupDef.x;
		screen->DrawTextCleanMove(smallFont, CR_RED, PSetupDef.x, PSetupDef.y + M_BigFontLineHeight()*2, "Gender");
		screen->DrawTextCleanMove(smallFont, CR_GREY, x, PSetupDef.y + M_BigFontLineHeight()*2, genders[gender]);
	}

	// Draw autoaim setting
	{
		const OFont* smallFont = OFonts.small();
		const int x = V_StringWidth(smallFont, "Autoaim") + 8 + PSetupDef.x;
		const float aim = cl_autoaim;

		screen->DrawTextCleanMove(smallFont, CR_RED, PSetupDef.x, PSetupDef.y + M_BigFontLineHeight()*3, "Autoaim");
		screen->DrawTextCleanMove(smallFont, CR_GREY, x, PSetupDef.y + M_BigFontLineHeight()*3,
			aim == 0 ? "Never" :
			aim <= 0.25 ? "Very Low" :
			aim <= 0.5 ? "Low" :
			aim <= 1 ? "Medium" :
			aim <= 2 ? "High" :
			aim <= 3 ? "Very High" : "Always");
	}

	// Draw color setting
	{
		const OFont* smallFont = OFonts.small();
		const int x = V_StringWidth(smallFont, "Color") + 8 + PSetupDef.x;
		screen->DrawTextCleanMove(smallFont, CR_RED, PSetupDef.x, PSetupDef.y + M_BigFontLineHeight()*4, "Color");
		screen->DrawTextCleanMove(smallFont, CR_GREY, x, PSetupDef.y + M_BigFontLineHeight()*4, colorpresets[colorpreset]);
	}

	int PSetupSize = static_cast<int>(ARRAY_LENGTH(PlayerSetupMenu));
	if (colorpreset == COLOR_CUSTOM && PSetupDef.numitems < PSetupSize)
		PSetupDef.numitems = PSetupDef.numitems + 3;
	else if (colorpreset != COLOR_CUSTOM && PSetupDef.numitems > PSetupSize - 3)
		PSetupDef.numitems = PSetupDef.numitems - 3;

	// Draw player color sliders
	//V_DrawTextCleanMove (CR_GREY, PSetupDef.x, PSetupDef.y + M_BigFontLineHeight(), "Color");

	if (colorpreset == COLOR_CUSTOM)
	{
		screen->DrawTextCleanMove(smallFont, CR_RED, PSetupDef.x, PSetupDef.y + M_BigFontLineHeight()*5, "Red");
		screen->DrawTextCleanMove(smallFont, CR_RED, PSetupDef.x, PSetupDef.y + M_BigFontLineHeight()*6, "Green");
		screen->DrawTextCleanMove(smallFont, CR_RED, PSetupDef.x, PSetupDef.y + M_BigFontLineHeight()*7, "Blue");

		{
			const int x = V_StringWidth(smallFont, "Green") + 8 + PSetupDef.x;
			const argb_t playercolor = V_GetColorFromString(cl_color);

			M_DrawSlider(x, PSetupDef.y + M_BigFontLineHeight()*5, 0.0f, 255.0f, playercolor.getr(), 0.0f);
			M_DrawSlider(x, PSetupDef.y + M_BigFontLineHeight()*6, 0.0f, 255.0f, playercolor.getg(), 0.0f);
			M_DrawSlider(x, PSetupDef.y + M_BigFontLineHeight()*7, 0.0f, 255.0f, playercolor.getb(), 0.0f);
		}
	}
}

void M_ChangeTeam (int choice) // [Toke - Teams]
{
	team_t team = D_TeamByName(cl_team.cstring());

	int iTeam = static_cast<int>(team);
	if (choice)
	{
		iTeam = (iTeam + 1) % NUMTEAMS;
	}
	else
	{
		iTeam--;
		if (iTeam < 0)
			iTeam = NUMTEAMS - 1;
	}
	team = static_cast<team_t>(iTeam);

	cl_team = GetTeamInfo(team)->ColorStringUpper.c_str();
}

static void M_ChangeGender (int choice)
{
	static constexpr int MAX_GENDER = static_cast<int>(ARRAY_LENGTH(genders)) - 1;
	int gender = D_GenderByName(cl_gender.cstring());

	if (!choice)
		gender = (gender == 0) ? MAX_GENDER : gender - 1;
	else
		gender = (gender == MAX_GENDER) ? 0 : gender + 1;

	cl_gender = genders[gender];
}

static void M_ChangeAutoAim (int choice)
{
	static constexpr float ranges[] = { 0, 0.25, 0.5, 1, 2, 3, 5000 };
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
	static constexpr int MAX_PRESET = static_cast<int>(ARRAY_LENGTH(colorpresets)) - 1;
	int colorpreset = D_ColorPreset(cl_colorpreset.cstring());
	argb_t customcolor = V_GetColorFromString(cl_customcolor);

	if (!choice)
		colorpreset = (colorpreset == 0) ? MAX_PRESET : colorpreset - 1;
	else
		colorpreset = (colorpreset == MAX_PRESET) ? 0 : colorpreset + 1;

	cl_colorpreset = colorpresets[colorpreset];

	if (colorpreset == COLOR_GREEN)
		// the Odamex green default
		SendNewColor(64, 207, 0);
	else if (colorpreset == COLOR_INDIGO)
		// the Wheat Chex jump suit; a little darker than the blue
		SendNewColor(134, 134, 134);
	else if (colorpreset == COLOR_BROWN)
		// my best approximation of the Vanilla brown translation
		SendNewColor(169, 87, 31);
	else if (colorpreset == COLOR_RED)
		// the blue luminosity matched to the Vanilla red hue without looking bad on 8-bit
		SendNewColor(250, 62, 62);
	else if (colorpreset == COLOR_BLUE)
		// the Corn Chex jump suit; it should be brighter, but that introduces gray pixels on 8-bit
		SendNewColor(57, 57, 255);
	else if (colorpreset == COLOR_ORANGE)
		SendNewColor(255, 96, 0);
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
	genStringEnter = oldmenustring_t::PLAYERNAME;
	genStringEnd = M_PlayerNameChanged;
	genStringLen = MAXPLAYERNAME;

	saveSlot = 0;
	M_StringCopy(saveOldString, savegamestrings[0], SAVESTRINGSIZE);
	if (!strcmp(savegamestrings[0], GStrings(EMPTYSTRING)))
		savegamestrings[0][0] = 0;
	saveCharIndex = strlen(savegamestrings[0]);
}

static void M_PlayerNameChanged (int choice)
{
	AddCommandString (fmt::format("cl_name \"{}\"", savegamestrings[0]));
}

static void SendNewColor(int red, int green, int blue)
{
	int colorpreset = D_ColorPreset(cl_colorpreset.cstring());

	cl_color.ForceSet(fmt::format("{:02x} {:02x} {:02x}", red, green, blue).c_str());
	if (colorpreset == COLOR_CUSTOM)
	{
		cl_customcolor.ForceSet(fmt::format("{:02x} {:02x} {:02x}", red, green, blue).c_str());
	}

	// [SL] not connected to a server so we don't have to wait for the server
	// to verify the color choice
	if (!connected)
	{
		// [Nes] Change the player preview color.
		R_BuildPlayerTranslation(menuplayer_id, V_GetColorFromString(cl_color), colorpreset);

		if (consoleplayer().ingame())
			R_CopyTranslationRGB(menuplayer_id, consoleplayer_id);
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
		menu->x - 10, menu->y+item*M_BigFontLineHeight() - 1);
}

void M_DrawSelCell (oldmenu_t *menu, int item)
{
	screen->DrawPatchClean (W_CachePatch("M_CELL2"),
		menu->x - 10, menu->y+item*M_BigFontLineHeight() - 1);
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
// CONTROL PANEL
//

//
// M_Responder
//
bool M_Responder(const event_t& ev)
{
	const OFont* smallFont = OFonts.small();
	int ch, ch2, mod;

	ch = ch2 = mod = -1;

	// eat mouse events
	if(menuactive)
	{
		if(ev.type == ev_mouse)
			return true;
		else if(ev.type == ev_joystick)
		{
			if(OptionsActive)
				M_OptResponder (ev);
			// Eat joystick events for now -- Hyper_Eye
			return true;
		}
	}

	if (ev.type == ev_keyup)
	{
		if(repeatKey == ev.data1)
		{
			repeatKey = 0;
			repeatCount = 0;
		}
	}

	if (ev.type == ev_keydown)
	{
		ch = ev.data1; 		// scancode
		ch2 = ev.data3;		// ASCII
		mod = ev.mod;			// key mods
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
	if (genStringEnter != oldmenustring_t::NONE)
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
			if (genStringEnter == oldmenustring_t::SAVEGAME)
				M_ClearMenus();
			genStringEnter = oldmenustring_t::NONE;
			M_StringCopy(&savegamestrings[saveSlot][0], saveOldString, SAVESTRINGSIZE);
		}
		else if (Key_IsAcceptKey(ch))
		{
			if (genStringEnter == oldmenustring_t::SAVEGAME)
				M_ClearMenus();
			genStringEnter = oldmenustring_t::NONE;
			if (savegamestrings[saveSlot][0])
				genStringEnd(saveSlot);	// [RH] Function to call when enter is pressed
		}
		else
		{
			ch = ev.data3;	// [RH] Use user keymap
			if (ch >= 32 && ch <= 127 &&
				saveCharIndex < genStringLen &&
				V_StringWidth(smallFont, savegamestrings[saveSlot]) <
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
			if (ch == '\0' && ch2 != '\0')
				messageRoutine(ch2);
			else
				messageRoutine(ch);
		}

		menuactive = false;
		M_ResumeSound();
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
	if (ev.type == ev_keydown)
		return true;
	else
		return false;
}


//
// M_StartControlPanel
//
void M_StartControlPanel()
{
	// intro might call this repeatedly
	if (menuactive)
		return;

	drawIndicator = true;
	MenuStackDepth = 0;
	menuactive = 1;
	MenuTime = 0;
	currentMenu = &MainDef;
	itemOn = currentMenu->lastOn;
	OptionsActive = false;			// [RH] Make sure none of the options menus appear.
	M_PauseSound();
	S_Sound(CHAN_INTERFACE, "switches/normbutn", 1, ATTN_NONE);
}


//
// [Toke] M_DimBackground
// Draws the 50% reduction in brightness effect
//
void M_DimBackground ()
{
	const int dim_width = screen->getSurface()->getWidth();
	const int dim_height = screen->getSurface()->getHeight();
	const int srcx = 0;
	const int srcy = 0;

	screen->Dim(srcx, srcy, dim_width, dim_height);
}

//
// M_Drawer
// Called after the view has been rendered,
// but before it has been blitted.
//
void M_Drawer()
{
	const OFont* smallFont = OFonts.small();
	const OFont* bigFont = OFonts.big();
	if (messageToPrint && smallFont != nullptr)
	{
		// Horiz. & Vertically center string and print it.
		brokenlines_t *lines = V_BreakLines(smallFont, 320, messageString);
		int y = 100;

		for (int i = 0; lines[i].width != -1; i++)
			y -= smallFont->lineHeight() / 2;

		for (int i = 0; lines[i].width != -1; i++)
		{
			screen->DrawTextCleanMove(smallFont, CR_RED, 160 - lines[i].width/2, y, lines[i].string);
			y += smallFont->lineHeight();
		}

		V_FreeBrokenLines (lines);
	}
	else if (menuactive)
	{
		// Background effect
		M_DimBackground();

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
				if (currentMenu->menuitems[i].name[0] &&
					W_CheckNumForName(currentMenu->menuitems[i].name) >= 0)
				{
					screen->DrawPatchClean(W_CachePatch(currentMenu->menuitems[i].name), x, y);
				}
				else if (currentMenu->menuitems[i].textname[0])
				{
					screen->DrawTextCleanMove(bigFont, CR_RED, x, y,
					                          LocalizedString(currentMenu->menuitems[i].textname));
				}

				y += M_BigFontLineHeight();
			}


			// DRAW SKULL
			if (drawIndicator)
			{
				if (const patch_t* indicator = MenuIndicatorPatch(whichIndicator))
				{
					const int draw_x = x + MenuConfTheme().indicator.offsetX;
					const int draw_y =
						currentMenu->y + MenuConfTheme().indicator.offsetY + itemOn * M_BigFontLineHeight();

					screen->DrawPatchClean(indicator, draw_x, draw_y);
				}
			}
		}
	}

	// [SL] force the status bar to be redrawn in case the menu
	// draws over a portion of the status bar background
	if (R_StatusBarVisible() && (menuactive || messageToPrint))
	{
		ST_ForceRefresh();
	}
}


//
// M_ClearMenus
//
void M_ClearMenus()
{
	I_FreeSurface(fire_surface);
	D_FreePageImage(help_page);
	MenuStackDepth = 0;
	menuactive = false;
	drawIndicator = true;
	M_DemoNoPlay = false;
    M_ResumeSound();
}




//
// M_SetupNextMenu
//
void M_SetupNextMenu (oldmenu_t *menudef)
{
	MenuStack[MenuStackDepth].menu.old = menudef;
	MenuStack[MenuStackDepth].isNewStyle = false;
	MenuStack[MenuStackDepth].drawIndicator = drawIndicator;
	MenuStackDepth++;

	currentMenu = menudef;
	itemOn = currentMenu->lastOn;
}


void M_PopMenuStack()
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
		drawIndicator = MenuStack[MenuStackDepth].drawIndicator;
		MenuStackDepth++;
		S_Sound (CHAN_INTERFACE, "switches/normbutn", 1, ATTN_NONE);
	} else {
		M_ClearMenus ();
		if (currentMenu == &PSetupDef && PSetupDepth > 0)			// hack for PlayerSetup
		{
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
void M_Ticker()
{
	if (--indicatorAnimCounter <= 0)
	{
		whichIndicator ^= 1;
		indicatorAnimCounter = 8;
	}

	if (currentMenu == &PSetupDef)
	{
		M_PlayerSetupTicker ();
	}
	
	MenuTime++;
}


//
// M_Init
//
EXTERN_CVAR (screenblocks)

void M_Init()
{
	MainDef.routine = M_DrawMainMenu;
	MainDef.lastOn = 0;
	MainDef.x = 97;
	MainDef.y = 64;

	currentMenu = &MainDef;
	OptionsActive = false;
	menuactive = 0;
	MenuTime = 0;
	SkullBaseLump = W_CheckNumForName ("M_SKL00");
	itemOn = currentMenu->lastOn;
	whichIndicator = 0;
	indicatorAnimCounter = 10;
	drawIndicator = true;
	screenSize = screenblocks.asInt() - 3;
	messageToPrint = 0;
	messageString = NULL;
	messageLastMenuActive = menuactive;

	if (gameinfo.flags & GI_MAPxx)
	{
		MainDef.y += 8;
	}
	else if (gameinfo.enginetype == ENGINE_HERETIC)
	{
		MainDef.x = 110;
		MainDef.y = 56;
		LoadDef = HticLoadDef;
		SaveDef = HticSaveDef;
	}

	if (!BuildConfiguredOldMenu(gConfiguredMainBridge, "main", MainDef, M_DrawMainMenu))
	{
		I_Error("M_Init: MENUCONF main menu is missing or empty");
	}
	BuildConfiguredOldMenu(gConfiguredEpisodeBridge, "episodes", EpiDef, DrawConfiguredMenu);
	BuildConfiguredOldMenu(gConfiguredExpansionBridge, "expansions", ExpDef, DrawConfiguredMenu);
	BuildConfiguredOldMenu(gConfiguredSkillBridge, "skills", NewDef, DrawConfiguredMenu);
	BuildConfiguredOldMenu(gConfiguredGameFilesBridge, "gamefiles", GameFilesDef, DrawConfiguredMenu);

	if (const menuconfmenu_t* mainMenu = MenuConfMenu("main"))
	{
		if (mainMenu->layout.x != 0)
		{
			MainDef.x = mainMenu->layout.x;
		}
		if (mainMenu->layout.y != 0)
		{
			MainDef.y = mainMenu->layout.y;
		}
	}

	M_OptInit ();

	// [RH] Build a palette translation table for the fire
	for (int i = 0; i < 256; i++)
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

    return limits::MAXINT;    // indicate not found
}


VERSION_CONTROL (m_menu_cpp, "$Id$")
