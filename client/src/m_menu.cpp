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
//   Menu widget stuff, episode selection and such.
//
//-----------------------------------------------------------------------------


#include "odamex.h"

#include <ctime>
#include <unordered_map>
#include <unordered_set>

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
#include "m_options_valuesets.h"
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
menu_t*             CurrentMenu;
int                 CurrentItem;
bool                CanScrollUp;
bool                CanScrollDown;
int                 VisBottom;

static int			MenuTime;			// Ticker for Heretic skulls
short				indicatorAnimCounter;	// indicator animation counter
short				whichIndicator; 		// which indicator to draw
bool				drawIndicator;			// [RH] don't always draw indicator

enum class builtinscreenkind_t
{
	none,
	help,
	saveload,
	playersetup
};

static builtinscreenkind_t CurrentBuiltinScreen = builtinscreenkind_t::none;
static int CurrentBuiltinItem = 0;
static int gLoadLastOn = 0;
static int gSaveLastOn = 0;

struct generatedmenu_t
{
	const char* menuId = nullptr;
	std::vector<menuconfitem_t> items;
	int x = 0;
	int y = 0;
	int lastOn = 0;
};

static generatedmenu_t* CurrentGeneratedMenu = nullptr;
static int CurrentGeneratedItem = 0;
static generatedmenu_t gGeneratedMainMenu;
static generatedmenu_t gGeneratedEpisodeMenu;
static generatedmenu_t gGeneratedExpansionMenu;
static generatedmenu_t gGeneratedSkillMenu;
static generatedmenu_t gGeneratedGameFilesMenu;
static std::string gSelectedEpisodeId;

//
// PROTOTYPES
//
void M_ChooseSkill(int choice);
void M_ActivateGeneratedMenuItem(int choice);

void M_StartGame(int choice);

void M_LoadSelect(int choice);
void M_SaveSelect(int choice);
void M_ReadSaveStrings();
void M_QuickSave();
void M_QuickLoad();

void M_DrawSaveLoadScreen();

void M_DrawSaveLoadBorder(int x,int y, int len);
void M_StartControlPanel();
void M_StartMessage(const char *string,void (*routine)(int),bool input);
void M_StopMessage();
void M_ClearMenus();
void M_OpenGeneratedMenu(generatedmenu_t& menu, bool newDrawIndicator = true);
void M_DrawGeneratedMenu();
void M_GeneratedMenuResponder(int ch, int ch2, bool numlock);
static bool GeneratedMenuItemSelectable(const menuconfitem_t& item);

// [RH] For player setup menu.
static void M_PlayerSetupTicker();
static void M_PlayerSetupDrawer();
static void M_EditPlayerName();
static void M_PlayerNameChanged();
static void M_PlayerNameChangedSlot(int);
static void M_ChangeGender (int choice);
static void M_ChangeAutoAim (int choice);
static void M_ChangeColorPreset (int choice);
static void SendNewColor (int red, int green, int blue);
static void M_SlidePlayerRed (int choice);
static void M_SlidePlayerGreen (int choice);
static void M_SlidePlayerBlue (int choice);

static void M_OpenNewGameMenu();
static void M_OpenLoadGameScreen();
static void M_OpenSaveGameScreen();
static void M_ActivateSaveLoadSlot(int choice);
static void M_OpenOptionsMenu();
static void M_DrawHelpPage();
static void M_OpenHelpScreen();
static void M_FinishHelpScreen();
static void M_BeginEndGamePrompt();
static void M_BeginQuitGamePrompt();
static void M_AdvanceHelpScreen();
static void M_PushBuiltinScreen(builtinscreenkind_t screen, int initialItem, bool newDrawIndicator);
static void M_BuiltinResponder(int ch, int ch2, bool numlock);
namespace
{
	struct menudestination_t;
	bool M_OpenMenuTargetImpl(const std::string& target);
	bool M_OpenMenuEntrypointImpl(const std::string& name);
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

static constexpr int MAINMENU_DEFAULT_X = 97;
static constexpr int MAINMENU_DEFAULT_Y = 64;
static constexpr int SUBMENU_DEFAULT_X = 48;
static constexpr int SUBMENU_DEFAULT_Y = 63;

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

static int gPlayerSetupLastOn = playername;

//
// OPTIONS MENU
//
// [RH] This menu is now handled in m_options.c
//
bool OptionsActive;

//
// Read This!
//
static int gHelpPageIndex = 0;

static int M_HelpPageCount()
{
	int count = 0;

	for (int i = 0; i < 3; ++i)
	{
		const OLumpName& page = gameinfo.infoPage[i];
		if (page.empty())
		{
			break;
		}

		bool duplicate = false;
		for (int j = 0; j < i; ++j)
		{
			if (iequals(page, gameinfo.infoPage[j]))
			{
				duplicate = true;
				break;
			}
		}

		if (duplicate)
		{
			break;
		}

		++count;
	}

	return count;
}

static constexpr int GAMEFILES_DEFAULT_X = 110;
static constexpr int GAMEFILES_DEFAULT_Y = 60;

//
// LOAD GAME MENU
//
enum class saveloadmode_t
{
	load,
	save
};

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

static saveloadmode_t gSaveLoadMode = saveloadmode_t::load;
static bool gSaveSlotOccupied[load_end] = {};
static int gSaveLoadX = 76;
static int gSaveLoadY = 54;

static void M_ConfigureSaveLoadScreen()
{
	if (gameinfo.enginetype == ENGINE_HERETIC)
	{
		gSaveLoadX = 62;
		gSaveLoadY = 20;
	}
	else
	{
		gSaveLoadX = 76;
		gSaveLoadY = 54;
	}
}

// [RH] Most menus can now be accessed directly
// through console commands.
BEGIN_COMMAND (menu_main)
{
	M_OpenMenuEntrypoint("mainMenu");
}
END_COMMAND (menu_main)

BEGIN_COMMAND (menu_help)
{
    // F1
	M_OpenMenuTarget("builtin:help");
}
END_COMMAND (menu_help)

BEGIN_COMMAND (menu_save)
{
    // F2
	M_OpenMenuTarget("builtin:saveGame");
	//Printf (PRINT_WARNING, "Saving is not available at this time.\n");
}
END_COMMAND (menu_save)

BEGIN_COMMAND (menu_load)
{
    // F3
	M_OpenMenuTarget("builtin:loadGame");
	//Printf (PRINT_WARNING, "Loading is not available at this time.\n");
}
END_COMMAND (menu_load)

BEGIN_COMMAND (menu_options)
{
    // F4
	M_OpenMenuEntrypoint("optionsMenu");
}
END_COMMAND (menu_options)

BEGIN_COMMAND (quicksave)
{
    // F6
	M_QuickSave ();
	//Printf (PRINT_WARNING, "Saving is not available at this time.\n");
}
END_COMMAND (quicksave)

BEGIN_COMMAND (menu_endgame)
{	// F7
	M_BeginEndGamePrompt();
}
END_COMMAND (menu_endgame)

BEGIN_COMMAND (quickload)
{
    // F9
	M_QuickLoad ();
	//Printf (PRINT_WARNING, "Loading is not available at this time.\n");
}
END_COMMAND (quickload)

BEGIN_COMMAND (menu_quit)
{	// F10
	M_BeginQuitGamePrompt();
}
END_COMMAND (menu_quit)

BEGIN_COMMAND (menu_player)
{
	M_OpenMenuTarget("builtin:playerSetup");
}
END_COMMAND (menu_player)

BEGIN_COMMAND (menu_keys)
{
	M_OpenMenuTarget("options.controls");
}
END_COMMAND (menu_keys)

BEGIN_COMMAND (menu_display)
{
	M_OpenMenuTarget("options.display");
}
END_COMMAND (menu_display)

BEGIN_COMMAND (menu_video)
{
	M_OpenMenuTarget("builtin:videoMode");
}
END_COMMAND (menu_video)

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

const char* M_LocalizedMenuString(const char* key)
{
	return LocalizedString(key);
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

	generatedmenu_t* GeneratedMenuById(const char* menuId)
	{
		generatedmenu_t* menus[] = {
			&gGeneratedMainMenu,
			&gGeneratedEpisodeMenu,
			&gGeneratedExpansionMenu,
			&gGeneratedSkillMenu,
			&gGeneratedGameFilesMenu
		};

		for (generatedmenu_t* generatedMenu : menus)
		{
			if (generatedMenu->menuId != nullptr && iequals(generatedMenu->menuId, menuId))
			{
				return generatedMenu;
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

	void WarnMenuConf(const std::string& message);

	void WarnMenuConfOnce(const std::string& message)
	{
		static std::unordered_set<std::string> warnedMessages;
		if (warnedMessages.insert(message).second)
		{
			WarnMenuConf(message);
		}
	}

	const patch_t* MenuConfPatch(const std::string& name)
	{
		return !name.empty() && W_CheckNumForName(name.c_str()) >= 0 ? W_CachePatch(name.c_str()) : nullptr;
	}

	const palette_t* MenuWidgetPalette()
	{
		static const palette_t* palette = V_GetPaletteFromLump("ODAPAL");
		return palette;
	}

	void DrawPatchCleanWithCachedPalette(const patch_t* patch, int x, int y,
	                                     const palette_t* palette)
	{
		if (patch == nullptr)
		{
			return;
		}

		if (palette == nullptr)
		{
			screen->DrawPatchClean(patch, x, y);
			return;
		}

		if (screen->getSurface()->getBitsPerPixel() == 8)
		{
			static palindex_t translation[256];
			static const palette_t* cachedPalette = nullptr;
			static const argb_t* cachedDestPalette = nullptr;

			const argb_t* destPalette = screen->getSurface()->getPalette();
			if (cachedPalette != palette || cachedDestPalette != destPalette)
			{
				cachedPalette = palette;
				cachedDestPalette = destPalette;

				for (int i = 0; i < 256; ++i)
				{
					translation[i] = V_BestColor(destPalette, palette->colors[i]);
				}
			}

			const translationref_t oldColorMap = V_ColorMap;
			V_ColorMap = translationref_t(translation);
			screen->DrawTranslatedPatchClean(patch, x, y);
			V_ColorMap = oldColorMap;
			return;
		}

		screen->DrawPatchCleanWithPalette(patch, x, y, palette);
	}

	template <typename Resolver>
	const patch_t* CachedMenuPatch(const std::string& configuredName, Resolver resolver)
	{
		static std::string cachedName;
		static const patch_t* cachedPatch = nullptr;
		if (cachedName != configuredName)
		{
			cachedName = configuredName;
			cachedPatch = resolver();
		}
		return cachedPatch;
	}

	const patch_t* MenuConfConfiguredPatch(const std::string& name, const char* context)
	{
		if (name.empty())
		{
			return nullptr;
		}

		const patch_t* patch = MenuConfPatch(name);
		if (patch == nullptr)
		{
			WarnMenuConfOnce(
			    fmt::sprintf("%s references missing patch \"%s\"", context, name.c_str()));
		}

		return patch;
	}

	int MenuCursorOffsetY()
	{
		return MenuConfTheme().cursorOffsetY;
	}

	const patch_t* MenuCursorPatch()
	{
		return CachedMenuPatch(MenuConfTheme().cursorPatch, []()
		{
			const patch_t* patch =
			    MenuConfConfiguredPatch(MenuConfTheme().cursorPatch, "theme.cursorPatch");
			return patch != nullptr ? patch : MenuConfPatch("LITLCURS");
		});
	}

	const patch_t* MenuSliderLeftPatch()
	{
		return CachedMenuPatch(MenuConfTheme().slider.leftPatch, []()
		{
			const patch_t* patch = MenuConfConfiguredPatch(MenuConfTheme().slider.leftPatch,
			                                               "theme.slider.leftPatch");
			return patch != nullptr ? patch : MenuConfPatch("LSLIDE");
		});
	}

	const patch_t* MenuSliderMiddlePatch()
	{
		return CachedMenuPatch(MenuConfTheme().slider.middlePatch, []()
		{
			const patch_t* patch = MenuConfConfiguredPatch(MenuConfTheme().slider.middlePatch,
			                                               "theme.slider.middlePatch");
			return patch != nullptr ? patch : MenuConfPatch("MSLIDE");
		});
	}

	const patch_t* MenuSliderRightPatch()
	{
		return CachedMenuPatch(MenuConfTheme().slider.rightPatch, []()
		{
			const patch_t* patch = MenuConfConfiguredPatch(MenuConfTheme().slider.rightPatch,
			                                               "theme.slider.rightPatch");
			return patch != nullptr ? patch : MenuConfPatch("RSLIDE");
		});
	}

	const patch_t* MenuSliderKnobPatch()
	{
		return CachedMenuPatch(MenuConfTheme().slider.knobPatch, []()
		{
			const patch_t* patch = MenuConfConfiguredPatch(MenuConfTheme().slider.knobPatch,
			                                               "theme.slider.knobPatch");
			return patch != nullptr ? patch : MenuConfPatch("CSLIDE");
		});
	}

	const patch_t* MenuSliderGreenKnobPatch()
	{
		return CachedMenuPatch(MenuConfTheme().slider.greenKnobPatch, []()
		{
			const patch_t* patch = MenuConfConfiguredPatch(MenuConfTheme().slider.greenKnobPatch,
			                                               "theme.slider.greenKnobPatch");
			return patch != nullptr ? patch : MenuConfPatch("GSLIDE");
		});
	}

	const patch_t* MenuSliderOverlayPatch()
	{
		return CachedMenuPatch(MenuConfTheme().slider.overlayPatch, []()
		{
			const patch_t* patch = MenuConfConfiguredPatch(MenuConfTheme().slider.overlayPatch,
			                                               "theme.slider.overlayPatch");
			return patch != nullptr ? patch : MenuConfPatch("OSLIDE");
		});
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
		return MenuConfConfiguredPatch(name, "theme.indicator.patches");
	}

	const patch_t* MenuInputBoxFullPatch()
	{
		const patch_t* patch =
		    MenuConfConfiguredPatch(MenuConfTheme().inputBox.fullPatch, "theme.inputBox.fullPatch");
		return patch != nullptr ? patch : MenuConfPatch("M_FSLOT");
	}

	const patch_t* MenuInputBoxLeftPatch()
	{
		const patch_t* patch = MenuConfConfiguredPatch(MenuConfTheme().inputBox.leftPatch,
		                                               "theme.inputBox.leftPatch");
		return patch != nullptr ? patch : MenuConfPatch("M_LSLEFT");
	}

	const patch_t* MenuInputBoxMiddlePatch()
	{
		const patch_t* patch = MenuConfConfiguredPatch(MenuConfTheme().inputBox.middlePatch,
		                                               "theme.inputBox.middlePatch");
		return patch != nullptr ? patch : MenuConfPatch("M_LSCNTR");
	}

	const patch_t* MenuInputBoxRightPatch()
	{
		const patch_t* patch = MenuConfConfiguredPatch(MenuConfTheme().inputBox.rightPatch,
		                                               "theme.inputBox.rightPatch");
		return patch != nullptr ? patch : MenuConfPatch("M_LSRGHT");
	}

	void WarnMenuConf(const std::string& message)
	{
		PrintFmt(PRINT_WARNING, "MENUCONF: {}\n", message);
	}

	const std::string* MenuSoundForRole(std::string_view role,
	                                    const std::string* overrideSound,
	                                    std::string_view menuId)
	{
		if (overrideSound != nullptr && !overrideSound->empty())
		{
			return overrideSound;
		}

		const std::string roleKey(role);
		if (!menuId.empty())
		{
			const menuconfmenu_t* menu = MenuConfMenu(std::string(menuId).c_str());
			if (menu != nullptr)
			{
				const auto menuIt = menu->sounds.find(roleKey);
				if (menuIt != menu->sounds.end() && !menuIt->second.empty())
				{
					return &menuIt->second;
				}
			}
		}

		const auto themeIt = MenuConfTheme().sounds.find(roleKey);
		if (themeIt != MenuConfTheme().sounds.end() && !themeIt->second.empty())
		{
			return &themeIt->second;
		}

		return nullptr;
	}

	void PlayMenuSound(std::string_view role,
	                   const std::string* overrideSound,
	                   std::string_view menuId)
	{
		const std::string* sound = MenuSoundForRole(role, overrideSound, menuId);
		if (sound == nullptr)
		{
			return;
		}

		S_Sound(CHAN_INTERFACE, sound->c_str(), 1, ATTN_NONE);
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

	void DrawMainMenuHeaderDecorations();

	void DrawGeneratedMenuHeader(const generatedmenu_t& generatedMenu)
	{
		if (generatedMenu.menuId == nullptr)
		{
			return;
		}

		const menuconfmenu_t* menu = MenuConfMenu(generatedMenu.menuId);
		if (menu == nullptr)
		{
			return;
		}

		const patch_t* headerPatch =
		    MenuConfConfiguredPatch(menu->header.patch, "menu.header.patch");
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
		}
		else
		{
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

		if (iequals(generatedMenu.menuId, "main"))
		{
			DrawMainMenuHeaderDecorations();
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
			M_OpenHelpScreen();
			return true;
		}
		if (builtinId == "loadGame")
		{
			M_OpenLoadGameScreen();
			return true;
		}
		if (builtinId == "saveGame")
		{
			M_OpenSaveGameScreen();
			return true;
		}
		if (builtinId == "playerSetup")
		{
			M_OpenPlayerSetupScreen();
			return true;
		}
		if (builtinId == "videoMode")
		{
			M_OpenVideoModeScreen();
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
		{
			if (destination.id.rfind("options", 0) == 0)
			{
				OptionsActive = M_OpenGeneratedOptionsMenu(destination.id);
				return OptionsActive;
			}
			generatedmenu_t* generatedMenu = GeneratedMenuById(destination.id.c_str());
			if (generatedMenu == nullptr || generatedMenu->items.empty())
			{
				WarnMenuConf(fmt::sprintf("menu target \"%s\" is not wired to the generated runtime yet",
				                          destination.id.c_str()));
				return false;
			}

			M_OpenGeneratedMenu(*generatedMenu);
			return true;
		}

		case menudestinationkind_t::invalid:
		default:
			return false;
		}
	}

	bool M_OpenMenuTargetImpl(const std::string& target)
	{
		menudestination_t destination;
		return M_ResolveMenuTarget(target, destination) && M_OpenResolvedDestination(destination);
	}

	bool M_OpenMenuEntrypointImpl(const std::string& name)
	{
		menudestination_t destination;
		return M_ResolveMenuEntrypoint(name, destination) && M_OpenResolvedDestination(destination);
	}

	bool BuildGeneratedMenu(generatedmenu_t& generatedMenu, const char* menuId,
	                        int defaultX, int defaultY, int defaultLastOn)
	{
		const menuconfmenu_t* authoredMenu = MenuConfMenu(menuId);
		if (authoredMenu == nullptr || authoredMenu->items.empty())
		{
			generatedMenu.items.clear();
			generatedMenu.menuId = menuId;
			generatedMenu.x = defaultX;
			generatedMenu.y = defaultY;
			generatedMenu.lastOn = defaultLastOn;
			return false;
		}

		generatedMenu.menuId = menuId;
		generatedMenu.items = authoredMenu->items;
		if (iequals(menuId, "episodes") &&
		    episodenum > 0 && static_cast<int>(generatedMenu.items.size()) > episodenum)
		{
			generatedMenu.items.resize(episodenum);
		}
		generatedMenu.lastOn = iequals(menuId, "skills") ? defaultskillmenu : defaultLastOn;
		generatedMenu.x = authoredMenu->layout.x != 0 ? authoredMenu->layout.x : defaultX;
		generatedMenu.y = authoredMenu->layout.y != 0 ? authoredMenu->layout.y : defaultY;
		return true;
	}

	float CurrentGeneratedDiscreteValue(const menuconfitem_t& item)
	{
		cvar_t* dummy = nullptr;
		cvar_t* cvar = cvar_t::FindCVar(item.cvar, &dummy);
		if (cvar == nullptr)
		{
			return 0.0f;
		}

		if ((cvar->flags() & CVAR_LATCH) && (cvar->flags() & CVAR_MODIFIED) &&
		    cvar->latched()[0] != '\0')
		{
			return static_cast<float>(atof(cvar->latched()));
		}

		return cvar->value();
	}

	const char* GeneratedDiscreteValueName(const menuconfitem_t& item)
	{
		int count = 0;
		value_t* values = M_OptionValueSet(item.values, count);
		if (values == nullptr || count <= 0)
		{
			return "";
		}

		const float value = CurrentGeneratedDiscreteValue(item);
		for (int i = 0; i < count; ++i)
		{
			if (values[i].value == value)
			{
				return values[i].name;
			}
		}

		return "";
	}

	std::string GeneratedMenuItemText(const menuconfitem_t& item)
	{
		const char* base = !item.languageKey.empty() ? LocalizedString(item.languageKey.c_str()) :
		                   item.text.c_str();
		if (item.kind != menuconfitemkind_t::cvarDiscrete)
		{
			return base;
		}

		const char* value = GeneratedDiscreteValueName(item);
		return value[0] ? fmt::format("{}: {}", base, value) : std::string(base);
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
				WarnMenuConfOnce(fmt::sprintf("header decoration references missing patch \"%s\"",
				                              side.basePatch.c_str()));
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

bool M_OpenMenuTarget(const std::string& target)
{
	if (!menuactive)
	{
		M_StartControlPanel();
	}

	const bool opened = M_OpenMenuTargetImpl(target);
	if (!opened && !menuactive)
	{
		M_ClearMenus();
	}

	return opened;
}

const patch_t* M_MenuConfConfiguredPatch(const std::string& name, const char* context)
{
	return MenuConfConfiguredPatch(name, context);
}

void M_WarnMenuConf(const std::string& message)
{
	WarnMenuConf(message);
}

void M_PlayMenuSound(std::string_view role,
                     const std::string* overrideSound,
                     std::string_view menuId)
{
	PlayMenuSound(role, overrideSound, menuId);
}

int M_MenuCursorOffsetY()
{
	return MenuCursorOffsetY();
}

const patch_t* M_MenuCursorPatch()
{
	return MenuCursorPatch();
}

bool M_OpenGeneratedOptionsMenu(const std::string& menuId)
{
	menu_t* menu = nullptr;
	if (!M_PrepareGeneratedOptionsMenu(menuId, menu))
	{
		return false;
	}

	M_SwitchMenu(menu);
	return true;
}

bool M_OpenMenuEntrypoint(const std::string& name)
{
	if (!menuactive)
	{
		M_StartControlPanel();
	}

	const bool opened = M_OpenMenuEntrypointImpl(name);
	if (!opened && !menuactive)
	{
		M_ClearMenus();
	}

	return opened;
}

void M_DrawSlider(int x, int y, float leftval, float rightval, float cur, float step)
{
	const OFont* smallFont = OFonts.small();
	const palette_t* palette = MenuWidgetPalette();
	const int drawY = y + MenuCursorOffsetY();
	const patch_t* leftPatch = MenuSliderLeftPatch();
	const patch_t* middlePatch = MenuSliderMiddlePatch();
	const patch_t* rightPatch = MenuSliderRightPatch();
	const patch_t* knobPatch = MenuSliderKnobPatch();

	if (leftval < rightval)
		cur = clamp(cur, leftval, rightval);
	else
		cur = clamp(cur, rightval, leftval);

	const float dist = (cur - leftval) / (rightval - leftval);

	DrawPatchCleanWithCachedPalette(leftPatch, x, drawY, palette);
	for (int i = 1; i < 11; i++)
		DrawPatchCleanWithCachedPalette(middlePatch, x + i * 8, drawY, palette);
	DrawPatchCleanWithCachedPalette(rightPatch, x + 88, drawY, palette);

	DrawPatchCleanWithCachedPalette(knobPatch, x + 5 + static_cast<int>(dist * 78.0), drawY,
	                                palette);

	std::string buf;
	if (step == 0.0f)
	{
		return;
	}
	else if (step >= 1.0f)
		buf = fmt::sprintf("%.0f", cur);
	else if (step >= 0.1f)
		buf = fmt::sprintf("%.1f", cur);
	else
		buf = fmt::sprintf("%.2f", cur);
	screen->DrawTextCleanMove(smallFont, CR_GREEN, x + 96, y, buf.c_str());
}

void M_DrawColoredSlider(int x, int y, float leftval, float rightval, float cur, argb_t color)
{
	const palette_t* palette = MenuWidgetPalette();
	const int drawY = y + MenuCursorOffsetY();
	const patch_t* leftPatch = MenuSliderLeftPatch();
	const patch_t* middlePatch = MenuSliderMiddlePatch();
	const patch_t* rightPatch = MenuSliderRightPatch();
	const patch_t* greenKnobPatch = MenuSliderGreenKnobPatch();
	const patch_t* overlayPatch = MenuSliderOverlayPatch();

	if (leftval < rightval)
		cur = clamp(cur, leftval, rightval);
	else
		cur = clamp(cur, rightval, leftval);

	const float dist = (cur - leftval) / (rightval - leftval);

	DrawPatchCleanWithCachedPalette(leftPatch, x, drawY, palette);

	for (int i = 1; i < 11; i++)
		DrawPatchCleanWithCachedPalette(middlePatch, x + i * 8, drawY, palette);

	DrawPatchCleanWithCachedPalette(rightPatch, x + 88, drawY, palette);

	DrawPatchCleanWithCachedPalette(greenKnobPatch,
	                                x + 5 + static_cast<int>(dist * 78.0), drawY, palette);

	using color_key_t = uint32_t;
	static std::unordered_map<color_key_t, palindex_t> fillCache;
	const color_key_t colorKey =
	    (static_cast<color_key_t>(color.getr()) << 16) |
	    (static_cast<color_key_t>(color.getg()) << 8) |
	    static_cast<color_key_t>(color.getb());
	auto fillIt = fillCache.find(colorKey);
	if (fillIt == fillCache.end())
	{
		fillIt = fillCache.emplace(colorKey,
		                           V_BestColor(V_GetDefaultPalette()->basecolors, color))
		             .first;
	}
	V_ColorFill = fillIt->second;

	screen->DrawColoredPatchClean(overlayPatch, x + 5 + static_cast<int>(dist * 78.0), drawY);
}

void M_ActivateGeneratedMenuItem(int choice)
{
	generatedmenu_t* generatedMenu = CurrentGeneratedMenu;
	if (generatedMenu == nullptr)
	{
		return;
	}

	const int itemIndex = CurrentGeneratedItem >= 0 &&
	                      static_cast<size_t>(CurrentGeneratedItem) < generatedMenu->items.size() &&
	                      generatedMenu->items[CurrentGeneratedItem].kind ==
	                          menuconfitemkind_t::cvarDiscrete
	                          ? CurrentGeneratedItem
	                          : choice;
	if (itemIndex < 0 || static_cast<size_t>(itemIndex) >= generatedMenu->items.size())
	{
		return;
	}

	const menuconfitem_t& item = generatedMenu->items[itemIndex];
	bool actionSucceeded = true;

	if (item.kind == menuconfitemkind_t::cvarDiscrete)
	{
		cvar_t* dummy = nullptr;
		cvar_t* cvar = cvar_t::FindCVar(item.cvar, &dummy);
		int count = 0;
		value_t* values = M_OptionValueSet(item.values, count);
		if (cvar == nullptr || values == nullptr || count <= 0)
		{
			const char* label = item.text.empty() ? item.cvar.c_str() : item.text.c_str();
			WarnMenuConf(fmt::sprintf("discrete menu item \"%s\" is not wired correctly",
			                          label));
			return;
		}

		int current = 0;
		const float value = CurrentGeneratedDiscreteValue(item);
		for (; current < count; ++current)
		{
			if (values[current].value == value)
			{
				break;
			}
		}
		if (current >= count)
		{
			current = 0;
		}

		if (choice == 0)
		{
			current = current > 0 ? current - 1 : count - 1;
		}
		else
		{
			current = (current + 1) % count;
		}

		cvar->Set(values[current].value);
		return;
	}

	if (!item.action.empty())
	{
		if (item.action == "quitGame")
		{
			M_BeginQuitGamePrompt();
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
			gSaveSlotOccupied[i] = false;
		}
		else
		{
			const size_t readlen = fread (&savegamestrings[i], SAVESTRINGSIZE, 1, handle.get());
			if (readlen < 1)
			{
				fmt::print("M_Read_SaveStrings(): Failed to read handle.\n");
				return;
			}
			gSaveSlotOccupied[i] = true;
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

void M_DrawSaveLoadScreen()
{
	const OFont* bigFont = OFonts.big();
	const OFont* smallFont = OFonts.small();
	int i, list_y;
	const int slot_width = 24;
	const int slot_padding = 2;
	const int slot_height = M_BigFontLineHeight() - slot_padding;

	const bool saveMode = gSaveLoadMode == saveloadmode_t::save;
	const char* patchName = saveMode ? "M_SAVEG" : "M_LOADG";
	const char* titleKey = saveMode ? "MNU_SAVEGAME" : "MNU_LOADGAME";

	if (W_CheckNumForName(patchName) >= 0)
	{
		screen->DrawPatchClean(W_CachePatch(patchName), 72, 28);
	}
	else
	{
		const char* title = LocalizedString(titleKey);
		screen->DrawTextCleanMove(bigFont, CR_GRAY, 160 - V_StringWidth(bigFont, title) / 2, 0,
		                          title);
	}

	list_y = gSaveLoadY;
	for (i = 0; i < load_end; i++)
	{
		M_DrawInputBox(savegamestrings[i], gSaveLoadX, list_y, slot_width);
		list_y += slot_height + slot_padding;
	}

	if (genStringEnter != oldmenustring_t::NONE)
	{
		const int string_width = V_StringWidth(smallFont, savegamestrings[saveSlot]);
		screen->DrawTextCleanMove(smallFont, CR_RED, gSaveLoadX + string_width,
		                          gSaveLoadY + M_BigFontLineHeight() * saveSlot, "_");
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

static void M_OpenLoadGameScreen()
{
	if (!menuactive)
	{
		M_StartControlPanel();
	}

	gSaveLoadMode = saveloadmode_t::load;
	M_ConfigureSaveLoadScreen();
	M_PushBuiltinScreen(builtinscreenkind_t::saveload, gLoadLastOn, true);
	M_ReadSaveStrings();
}

static void M_ActivateSaveLoadSlot(int choice)
{
	if (gSaveLoadMode == saveloadmode_t::save)
	{
		M_SaveSelect(choice);
	}
	else
	{
		M_LoadSelect(choice);
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
	if (!gSaveSlotOccupied[choice])
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
static void M_OpenSaveGameScreen()
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

	if (!menuactive)
	{
		M_StartControlPanel();
	}

	gSaveLoadMode = saveloadmode_t::save;
	M_ConfigureSaveLoadScreen();
	M_PushBuiltinScreen(builtinscreenkind_t::saveload, gSaveLastOn, true);
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
		M_OpenSaveGameScreen();
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
		M_OpenLoadGameScreen();
		return;
	}
	snprintf(tempstring, 80, GStrings(QLPROMPT),savegamestrings[quickSaveSlot]);
	M_StartMessage(tempstring,M_QuickLoadResponse,true);
}


//
// M_ReadThis
//
static void M_OpenHelpScreen()
{
	if (!menuactive)
	{
		M_StartControlPanel();
	}

	const int pageCount = M_HelpPageCount();
	if (pageCount <= 0)
	{
		M_ClearMenus();
		return;
	}

	drawIndicator = false;
	gHelpPageIndex = 0;
	D_LoadPageImage(help_page, gameinfo.infoPage[0]);
	M_PushBuiltinScreen(builtinscreenkind_t::help, 0, false);
}

static void M_DrawHelpPage()
{
	D_DrawPageImage(help_page, I_GetPrimarySurface(), true);
}

static void M_AdvanceHelpScreen()
{
	const int pageCount = M_HelpPageCount();
	if (pageCount <= 0 || gHelpPageIndex + 1 >= pageCount)
	{
		M_FinishHelpScreen();
		return;
	}

	++gHelpPageIndex;
	drawIndicator = false;
	D_LoadPageImage(help_page, gameinfo.infoPage[gHelpPageIndex]);
}

static void M_FinishHelpScreen()
{
	drawIndicator = true;
	D_FreePageImage(help_page);
	gHelpPageIndex = 0;
	M_ClearMenus();
	M_OpenMenuEntrypoint("mainMenu");
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

void M_OpenGeneratedMenu(generatedmenu_t& menu, bool newDrawIndicator)
{
	MenuStack[MenuStackDepth].menu.generated = &menu;
	MenuStack[MenuStackDepth].isNewStyle = false;
	MenuStack[MenuStackDepth].isBuiltin = false;
	MenuStack[MenuStackDepth].isGenerated = true;
	MenuStack[MenuStackDepth].drawIndicator = newDrawIndicator;
	MenuStackDepth++;

	OptionsActive = false;
	CurrentBuiltinScreen = builtinscreenkind_t::none;
	CurrentMenu = nullptr;
	CurrentGeneratedMenu = &menu;
	CurrentGeneratedItem = menu.lastOn;
	if (!menu.items.empty())
	{
		CurrentGeneratedItem =
		    clamp(CurrentGeneratedItem, 0, static_cast<int>(menu.items.size()) - 1);
		if (!GeneratedMenuItemSelectable(menu.items[CurrentGeneratedItem]))
		{
			for (size_t i = 0; i < menu.items.size(); ++i)
			{
				if (GeneratedMenuItemSelectable(menu.items[i]))
				{
					CurrentGeneratedItem = static_cast<int>(i);
					break;
				}
			}
		}
		menu.lastOn = CurrentGeneratedItem;
	}
}

void M_DrawGeneratedMenu()
{
	if (CurrentGeneratedMenu == nullptr)
	{
		return;
	}

	const OFont* bigFont = OFonts.big();
	const OFont* smallFont = OFonts.small();
	DrawGeneratedMenuHeader(*CurrentGeneratedMenu);

	const int x = CurrentGeneratedMenu->x;
	int y = CurrentGeneratedMenu->y;
	for (size_t i = 0; i < CurrentGeneratedMenu->items.size(); ++i)
	{
		const menuconfitem_t& item = CurrentGeneratedMenu->items[i];
		if (!item.patch.empty() && W_CheckNumForName(item.patch.c_str()) >= 0)
		{
			screen->DrawPatchClean(W_CachePatch(item.patch.c_str()), x, y);
		}
		else if (!item.languageKey.empty() || !item.text.empty())
		{
			if (item.kind == menuconfitemkind_t::cvarDiscrete)
			{
				const char* base = !item.languageKey.empty() ?
				                       LocalizedString(item.languageKey.c_str()) :
				                       item.text.c_str();
				const char* value = GeneratedDiscreteValueName(item);
				const int smallY = y + (M_BigFontLineHeight() / 2 - M_SmallFontLineHeight() / 2) + 5;

				screen->DrawTextCleanMove(smallFont, CR_RED, x, smallY, base);
				if (value[0])
				{
					screen->DrawTextCleanMove(smallFont, CR_GREY,
					                          x + V_StringWidth(smallFont, base) +
					                              M_SmallFontLineHeight(),
					                          smallY, value);
				}
			}
			else
			{
				const std::string text = GeneratedMenuItemText(item);
				screen->DrawTextCleanMove(bigFont, CR_RED, x, y, text.c_str());
			}
		}

		y += M_BigFontLineHeight();
	}

	if (drawIndicator)
	{
		if (const patch_t* indicator = MenuIndicatorPatch(whichIndicator))
		{
			const int draw_x = x + MenuConfTheme().indicator.offsetX;
			const int draw_y = CurrentGeneratedMenu->y + MenuConfTheme().indicator.offsetY +
			                   CurrentGeneratedItem * M_BigFontLineHeight();
			screen->DrawPatchClean(indicator, draw_x, draw_y);
		}
	}
}

static void M_OpenNewGameMenu()
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

static void M_OpenOptionsMenu()
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

	S_StopAmbientSound();
	M_ClearMenus ();
	D_StartTitle ();
	CL_QuitNetGame(NQ_SILENT);
}

static void M_BeginEndGamePrompt()
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

static void M_BeginQuitGamePrompt()
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
static constexpr int PLAYERSETUP_X = 48;
static constexpr int PLAYERSETUP_Y = 47;


EXTERN_CVAR (cl_name)
EXTERN_CVAR (cl_team)
EXTERN_CVAR (cl_colorpreset)
EXTERN_CVAR (cl_customcolor)
EXTERN_CVAR (cl_color)
EXTERN_CVAR (cl_gender)
EXTERN_CVAR (cl_autoaim)

static int M_PlayerSetupItemCount()
{
	return D_ColorPreset(cl_colorpreset.cstring()) == COLOR_CUSTOM ? psetup_end :
	                                                                playercolorpreset + 1;
}

static void M_ClampPlayerSetupItem()
{
	const int max = M_PlayerSetupItemCount();
	if (CurrentBuiltinItem >= max)
	{
		CurrentBuiltinItem = max - 1;
	}
	if (CurrentBuiltinItem < 0)
	{
		CurrentBuiltinItem = 0;
	}
	gPlayerSetupLastOn = CurrentBuiltinItem;
}

static void M_ActivatePlayerSetupItem(int choice)
{
	switch (CurrentBuiltinItem)
	{
	case playername:
		M_EditPlayerName();
		break;
	case playerteam:
		M_ChangeTeam(choice);
		break;
	case playersex:
		M_ChangeGender(choice);
		break;
	case playeraim:
		M_ChangeAutoAim(choice);
		break;
	case playercolorpreset:
		M_ChangeColorPreset(choice);
		break;
	case playerred:
		M_SlidePlayerRed(choice);
		break;
	case playergreen:
		M_SlidePlayerGreen(choice);
		break;
	case playerblue:
		M_SlidePlayerBlue(choice);
		break;
	default:
		break;
	}

	M_ClampPlayerSetupItem();
}

void M_OpenPlayerSetupScreen(void)
{
	if (!menuactive)
	{
		M_StartControlPanel();
	}

	M_StringCopy(savegamestrings[0], cl_name.cstring(), SAVESTRINGSIZE);
	M_PushBuiltinScreen(builtinscreenkind_t::playersetup, gPlayerSetupLastOn, true);
	PlayerState = &states[mobjinfo[MT_PLAYER].seestate];
	PlayerTics = PlayerState->tics;

	if (fire_surface == NULL)
		fire_surface = I_AllocateSurface(fire_surface_width, fire_surface_height, 8);

	// [Nes] Intialize the player preview color.
	const argb_t player_color = CL_GetPlayerColor(consoleplayer());
	int colorpreset = D_ColorPreset(cl_colorpreset.cstring());
	R_BuildPlayerTranslation(menuplayer_id, player_color, colorpreset);
	M_ClampPlayerSetupItem();
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
	screen->DrawTextCleanMove(smallFont, CR_RED, PLAYERSETUP_X, PLAYERSETUP_Y, "Name");
	M_DrawInputBox(savegamestrings[0], PLAYERSETUP_X + 56, PLAYERSETUP_Y - 4, MAXPLAYERNAME + 1);

	// Draw cursor for either of the above
	if (genStringEnter != oldmenustring_t::NONE)
		screen->DrawTextCleanMove(smallFont, CR_RED,
							PLAYERSETUP_X + V_StringWidth(smallFont, savegamestrings[saveSlot]) + 56,
							PLAYERSETUP_Y + ((saveSlot == 0) ? 0 : M_BigFontLineHeight()), "_");

	// Draw player character
	{
		int x = 320 - 88 - 32, y = PLAYERSETUP_Y + M_BigFontLineHeight() * 3 - 14;

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
			PLAYERSETUP_Y + M_BigFontLineHeight() * 3 + 22, palette);

		screen->DrawTranslatedPatchClean (W_CachePatch (sprframe->lump[0]),
			320 - 52 - 32, PLAYERSETUP_Y + M_BigFontLineHeight() * 3 + 46);
	}

	// Draw team setting
	{
		const OFont* smallFont = OFonts.small();
		const team_t team = D_TeamByName(cl_team.cstring());
		const int x = V_StringWidth(smallFont, "Preferred Team") + 8 + PLAYERSETUP_X;
		screen->DrawTextCleanMove(smallFont, CR_RED, PLAYERSETUP_X, PLAYERSETUP_Y + M_BigFontLineHeight(), "Preferred Team");
		screen->DrawTextCleanMove(smallFont, CR_GREY, x, PLAYERSETUP_Y + M_BigFontLineHeight(), team == TEAM_NONE ? "NONE" : GetTeamInfo(team)->ColorStringUpper.c_str());
	}

	// Draw gender setting
	{
		const OFont* smallFont = OFonts.small();
		const gender_t gender = D_GenderByName(cl_gender.cstring());
		const int x = V_StringWidth(smallFont, "Gender") + 8 + PLAYERSETUP_X;
		screen->DrawTextCleanMove(smallFont, CR_RED, PLAYERSETUP_X, PLAYERSETUP_Y + M_BigFontLineHeight() * 2, "Gender");
		screen->DrawTextCleanMove(smallFont, CR_GREY, x, PLAYERSETUP_Y + M_BigFontLineHeight() * 2, genders[gender]);
	}

	// Draw autoaim setting
	{
		const OFont* smallFont = OFonts.small();
		const int x = V_StringWidth(smallFont, "Autoaim") + 8 + PLAYERSETUP_X;
		const float aim = cl_autoaim;

		screen->DrawTextCleanMove(smallFont, CR_RED, PLAYERSETUP_X, PLAYERSETUP_Y + M_BigFontLineHeight() * 3, "Autoaim");
		screen->DrawTextCleanMove(smallFont, CR_GREY, x, PLAYERSETUP_Y + M_BigFontLineHeight() * 3,
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
		const int x = V_StringWidth(smallFont, "Color") + 8 + PLAYERSETUP_X;
		screen->DrawTextCleanMove(smallFont, CR_RED, PLAYERSETUP_X, PLAYERSETUP_Y + M_BigFontLineHeight() * 4, "Color");
		screen->DrawTextCleanMove(smallFont, CR_GREY, x, PLAYERSETUP_Y + M_BigFontLineHeight() * 4, colorpresets[colorpreset]);
	}

	// Draw player color sliders
	//V_DrawTextCleanMove (CR_GREY, PLAYERSETUP_X, PLAYERSETUP_Y + M_BigFontLineHeight(), "Color");

	if (colorpreset == COLOR_CUSTOM)
	{
		screen->DrawTextCleanMove(smallFont, CR_RED, PLAYERSETUP_X, PLAYERSETUP_Y + M_BigFontLineHeight() * 5, "Red");
		screen->DrawTextCleanMove(smallFont, CR_RED, PLAYERSETUP_X, PLAYERSETUP_Y + M_BigFontLineHeight() * 6, "Green");
		screen->DrawTextCleanMove(smallFont, CR_RED, PLAYERSETUP_X, PLAYERSETUP_Y + M_BigFontLineHeight() * 7, "Blue");

		{
			const int x = V_StringWidth(smallFont, "Green") + 8 + PLAYERSETUP_X;
			const argb_t playercolor = V_GetColorFromString(cl_color);

			M_DrawSlider(x, PLAYERSETUP_Y + M_BigFontLineHeight() * 5, 0.0f, 255.0f, playercolor.getr(), 0.0f);
			M_DrawSlider(x, PLAYERSETUP_Y + M_BigFontLineHeight() * 6, 0.0f, 255.0f, playercolor.getg(), 0.0f);
			M_DrawSlider(x, PLAYERSETUP_Y + M_BigFontLineHeight() * 7, 0.0f, 255.0f, playercolor.getb(), 0.0f);
		}
	}

	M_ClampPlayerSetupItem();
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

static void M_EditPlayerName()
{
	// we are going to be intercepting all chars
	genStringEnter = oldmenustring_t::PLAYERNAME;
	genStringEnd = M_PlayerNameChangedSlot;
	genStringLen = MAXPLAYERNAME;

	saveSlot = 0;
	M_StringCopy(saveOldString, savegamestrings[0], SAVESTRINGSIZE);
	if (!strcmp(savegamestrings[0], GStrings(EMPTYSTRING)))
		savegamestrings[0][0] = 0;
	saveCharIndex = strlen(savegamestrings[0]);
}

static void M_PlayerNameChanged()
{
	AddCommandString (fmt::format("cl_name \"{}\"", savegamestrings[0]));
}

static void M_PlayerNameChangedSlot(int)
{
	M_PlayerNameChanged();
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

	if (CurrentBuiltinScreen != builtinscreenkind_t::none)
	{
		M_BuiltinResponder(ch, ch2, numlock);
		return ev.type == ev_keydown;
	}

	if (CurrentGeneratedMenu != nullptr)
	{
		M_GeneratedMenuResponder(ch, ch2, numlock);
		return ev.type == ev_keydown;
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
	CurrentBuiltinScreen = builtinscreenkind_t::none;
	CurrentMenu = nullptr;
	CurrentGeneratedMenu = nullptr;
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
		else if (CurrentBuiltinScreen == builtinscreenkind_t::help)
		{
			M_DrawHelpPage();
		}
		else if (CurrentBuiltinScreen == builtinscreenkind_t::saveload)
		{
			M_DrawSaveLoadScreen();

			if (drawIndicator)
			{
				if (const patch_t* indicator = MenuIndicatorPatch(whichIndicator))
				{
					const int draw_x = gSaveLoadX + MenuConfTheme().indicator.offsetX;
					const int draw_y =
					    gSaveLoadY + MenuConfTheme().indicator.offsetY +
					    CurrentBuiltinItem * M_BigFontLineHeight();

					screen->DrawPatchClean(indicator, draw_x, draw_y);
				}
			}
		}
		else if (CurrentBuiltinScreen == builtinscreenkind_t::playersetup)
		{
			M_PlayerSetupDrawer();

			if (drawIndicator && genStringEnter == oldmenustring_t::NONE)
			{
				if (const patch_t* indicator = MenuIndicatorPatch(whichIndicator))
				{
					const int draw_x = PLAYERSETUP_X + MenuConfTheme().indicator.offsetX;
					const int draw_y =
					    PLAYERSETUP_Y + MenuConfTheme().indicator.offsetY +
					    CurrentBuiltinItem * M_BigFontLineHeight();

					screen->DrawPatchClean(indicator, draw_x, draw_y);
				}
			}
		}
		else if (CurrentGeneratedMenu != nullptr)
		{
			M_DrawGeneratedMenu();
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
	CurrentBuiltinScreen = builtinscreenkind_t::none;
	CurrentMenu = nullptr;
	CurrentGeneratedMenu = nullptr;
	drawIndicator = true;
	M_DemoNoPlay = false;
    M_ResumeSound();
}




//
// M_SetupNextMenu
//
void M_PushNewMenu(menu_t* menu, bool newDrawIndicator)
{
	MenuStack[MenuStackDepth].menu.newmenu = menu;
	MenuStack[MenuStackDepth].isNewStyle = true;
	MenuStack[MenuStackDepth].isBuiltin = false;
	MenuStack[MenuStackDepth].isGenerated = false;
	MenuStack[MenuStackDepth].drawIndicator = newDrawIndicator;
	MenuStackDepth++;

	CurrentMenu = menu;
	CurrentItem = menu->lastOn;
	CurrentGeneratedMenu = nullptr;
	CurrentBuiltinScreen = builtinscreenkind_t::none;
}

static void M_PushBuiltinScreen(builtinscreenkind_t screen, int initialItem, bool newDrawIndicator)
{
	MenuStack[MenuStackDepth].menu.builtin = static_cast<int>(screen);
	MenuStack[MenuStackDepth].isNewStyle = false;
	MenuStack[MenuStackDepth].isBuiltin = true;
	MenuStack[MenuStackDepth].isGenerated = false;
	MenuStack[MenuStackDepth].drawIndicator = newDrawIndicator;
	MenuStackDepth++;

	OptionsActive = false;
	CurrentBuiltinScreen = screen;
	CurrentMenu = nullptr;
	CurrentBuiltinItem = initialItem;
	CurrentGeneratedMenu = nullptr;
}

void M_BuildKeyList(menuitem_t* item, int numitems)
{
	for (int i = 0; i < numitems; i++, item++)
	{
		if (item->type == control)
			Bindings.GetKeysForCommand(item->e.command, &item->b.key1, &item->c.key2);
		if (item->type == mapcontrol)
			AutomapBindings.GetKeysForCommand(item->e.command, &item->b.key1, &item->c.key2);
		if (item->type == netdemocontrol)
			NetDemoBindings.GetKeysForCommand(item->e.command, &item->b.key1, &item->c.key2);
	}
}

void M_SwitchMenu(menu_t* menu)
{
	int widest = 0;

	M_PushNewMenu(menu, false);

	if (!menu->indent)
	{
		for (int i = 0; i < menu->numitems; i++)
		{
			menuitem_t* item = menu->items + i;
			if (item->type != whitetext && item->type != redtext && item->type != orangetext)
			{
				const int thiswidth = V_StringWidth(OFonts.small(), item->label);
				if (thiswidth > widest)
					widest = thiswidth;
			}
		}
		menu->indent = widest + 6;
	}
}

int M_FindCurVal(float cur, value_t* values, int numvals)
{
	int v;

	for (v = 0; v < numvals; v++)
		if (values[v].value == cur)
			break;

	return v;
}

static void M_BuiltinResponder(int ch, int ch2, bool numlock)
{
	switch (CurrentBuiltinScreen)
	{
	case builtinscreenkind_t::help:
		if (Key_IsAcceptKey(ch))
		{
			M_AdvanceHelpScreen();
		}
		else if (Key_IsCancelKey(ch))
		{
			M_PopMenuStack();
		}
		break;

	case builtinscreenkind_t::saveload:
		if (Key_IsDownKey(ch, numlock))
		{
			CurrentBuiltinItem = (CurrentBuiltinItem + 1) % load_end;
			S_Sound(CHAN_INTERFACE, "plats/pt1_stop", 1, ATTN_NONE);
		}
		else if (Key_IsUpKey(ch, numlock))
		{
			CurrentBuiltinItem = CurrentBuiltinItem > 0 ? CurrentBuiltinItem - 1 : load_end - 1;
			S_Sound(CHAN_INTERFACE, "plats/pt1_stop", 1, ATTN_NONE);
		}
		else if (Key_IsAcceptKey(ch))
		{
			if (gSaveLoadMode == saveloadmode_t::save || gSaveSlotOccupied[CurrentBuiltinItem])
			{
				if (gSaveLoadMode == saveloadmode_t::save)
				{
					gSaveLastOn = CurrentBuiltinItem;
				}
				else
				{
					gLoadLastOn = CurrentBuiltinItem;
				}

				M_ActivateSaveLoadSlot(CurrentBuiltinItem);
				S_Sound(CHAN_INTERFACE, "weapons/pistol", 1, ATTN_NONE);
			}
		}
		else if (Key_IsCancelKey(ch))
		{
			if (gSaveLoadMode == saveloadmode_t::save)
			{
				gSaveLastOn = CurrentBuiltinItem;
			}
			else
			{
				gLoadLastOn = CurrentBuiltinItem;
			}

			M_PopMenuStack();
		}
		else if (ch2 >= '1' && ch2 <= '8')
		{
			CurrentBuiltinItem = ch2 - '1';
			S_Sound(CHAN_INTERFACE, "plats/pt1_stop", 1, ATTN_NONE);
		}
		break;

	case builtinscreenkind_t::playersetup:
		if (Key_IsDownKey(ch, numlock))
		{
			const int count = M_PlayerSetupItemCount();
			CurrentBuiltinItem = (CurrentBuiltinItem + 1) % count;
			gPlayerSetupLastOn = CurrentBuiltinItem;
			S_Sound(CHAN_INTERFACE, "plats/pt1_stop", 1, ATTN_NONE);
		}
		else if (Key_IsUpKey(ch, numlock))
		{
			const int count = M_PlayerSetupItemCount();
			CurrentBuiltinItem = CurrentBuiltinItem > 0 ? CurrentBuiltinItem - 1 : count - 1;
			gPlayerSetupLastOn = CurrentBuiltinItem;
			S_Sound(CHAN_INTERFACE, "plats/pt1_stop", 1, ATTN_NONE);
		}
		else if (Key_IsLeftKey(ch, numlock))
		{
			if (CurrentBuiltinItem != playername)
			{
				M_ActivatePlayerSetupItem(0);
				S_Sound(CHAN_INTERFACE, "plats/pt1_mid", 1, ATTN_NONE);
			}
		}
		else if (Key_IsRightKey(ch, numlock))
		{
			if (CurrentBuiltinItem != playername)
			{
				M_ActivatePlayerSetupItem(1);
				S_Sound(CHAN_INTERFACE, "plats/pt1_mid", 1, ATTN_NONE);
			}
		}
		else if (Key_IsAcceptKey(ch))
		{
			M_ActivatePlayerSetupItem(CurrentBuiltinItem == playername ? CurrentBuiltinItem : 1);
			gPlayerSetupLastOn = CurrentBuiltinItem;
			S_Sound(CHAN_INTERFACE,
			        CurrentBuiltinItem == playername ? "weapons/pistol" : "plats/pt1_mid", 1,
			        ATTN_NONE);
		}
		else if (Key_IsCancelKey(ch))
		{
			gPlayerSetupLastOn = CurrentBuiltinItem;
			M_PopMenuStack();
		}
		else if (ch2 && ch < OKEY_JOY1)
		{
			const char alpha = static_cast<char>(tolower(ch2));
			static constexpr char alphaKeys[psetup_end] = { 'n', 't', 'e', 'a', 'c', 'r', 'g',
				                                            'b' };
			for (int i = CurrentBuiltinItem + 1; i < M_PlayerSetupItemCount(); ++i)
			{
				if (alphaKeys[i] == alpha)
				{
					CurrentBuiltinItem = i;
					gPlayerSetupLastOn = CurrentBuiltinItem;
					S_Sound(CHAN_INTERFACE, "plats/pt1_stop", 1, ATTN_NONE);
					return;
				}
			}
			for (int i = 0; i <= CurrentBuiltinItem && i < M_PlayerSetupItemCount(); ++i)
			{
				if (alphaKeys[i] == alpha)
				{
					CurrentBuiltinItem = i;
					gPlayerSetupLastOn = CurrentBuiltinItem;
					S_Sound(CHAN_INTERFACE, "plats/pt1_stop", 1, ATTN_NONE);
					return;
				}
			}
		}
		break;

	case builtinscreenkind_t::none:
	default:
		break;
	}
}

static bool GeneratedMenuItemSelectable(const menuconfitem_t& item)
{
	return item.kind != menuconfitemkind_t::separator;
}

void M_GeneratedMenuResponder(int ch, int ch2, bool numlock)
{
	if (CurrentGeneratedMenu == nullptr || CurrentGeneratedMenu->items.empty())
	{
		return;
	}

	const int count = static_cast<int>(CurrentGeneratedMenu->items.size());
	auto moveToSelectable = [count](int direction)
	{
		int next = CurrentGeneratedItem;
		do
		{
			next += direction;
			if (next >= count)
			{
				next = 0;
			}
			else if (next < 0)
			{
				next = count - 1;
			}
		} while (!GeneratedMenuItemSelectable(CurrentGeneratedMenu->items[next]));
		CurrentGeneratedItem = next;
		CurrentGeneratedMenu->lastOn = next;
	};

	if (Key_IsDownKey(ch, numlock))
	{
		moveToSelectable(1);
		S_Sound(CHAN_INTERFACE, "plats/pt1_stop", 1, ATTN_NONE);
		return;
	}
	if (Key_IsUpKey(ch, numlock))
	{
		moveToSelectable(-1);
		S_Sound(CHAN_INTERFACE, "plats/pt1_stop", 1, ATTN_NONE);
		return;
	}
	if (Key_IsLeftKey(ch, numlock))
	{
		if (CurrentGeneratedMenu->items[CurrentGeneratedItem].kind == menuconfitemkind_t::cvarDiscrete)
		{
			M_ActivateGeneratedMenuItem(0);
			S_Sound(CHAN_INTERFACE, "plats/pt1_mid", 1, ATTN_NONE);
		}
		return;
	}
	if (Key_IsRightKey(ch, numlock))
	{
		if (CurrentGeneratedMenu->items[CurrentGeneratedItem].kind == menuconfitemkind_t::cvarDiscrete)
		{
			M_ActivateGeneratedMenuItem(1);
			S_Sound(CHAN_INTERFACE, "plats/pt1_mid", 1, ATTN_NONE);
		}
		return;
	}
	if (Key_IsAcceptKey(ch))
	{
		const menuconfitem_t& item = CurrentGeneratedMenu->items[CurrentGeneratedItem];
		if (GeneratedMenuItemSelectable(item))
		{
			M_ActivateGeneratedMenuItem(item.kind == menuconfitemkind_t::cvarDiscrete ?
			                                1 :
			                                CurrentGeneratedItem);
			S_Sound(CHAN_INTERFACE,
			        item.kind == menuconfitemkind_t::cvarDiscrete ? "plats/pt1_mid" :
			                                                         "weapons/pistol",
			        1, ATTN_NONE);
		}
		return;
	}
	if (Key_IsCancelKey(ch))
	{
		M_PopMenuStack();
		return;
	}
	if (ch2 && (ch < OKEY_JOY1))
	{
		const char alpha = static_cast<char>(tolower(ch2));
		for (int i = CurrentGeneratedItem + 1; i < count; ++i)
		{
			if (!CurrentGeneratedMenu->items[i].hotkey.empty() &&
			    tolower(CurrentGeneratedMenu->items[i].hotkey[0]) == alpha)
			{
				CurrentGeneratedItem = i;
				CurrentGeneratedMenu->lastOn = i;
				S_Sound(CHAN_INTERFACE, "plats/pt1_stop", 1, ATTN_NONE);
				return;
			}
		}
		for (int i = 0; i <= CurrentGeneratedItem; ++i)
		{
			if (!CurrentGeneratedMenu->items[i].hotkey.empty() &&
			    tolower(CurrentGeneratedMenu->items[i].hotkey[0]) == alpha)
			{
				CurrentGeneratedItem = i;
				CurrentGeneratedMenu->lastOn = i;
				S_Sound(CHAN_INTERFACE, "plats/pt1_stop", 1, ATTN_NONE);
				return;
			}
		}
	}
}


void M_PopMenuStack()
{
	M_DemoNoPlay = false;
	if (MenuStackDepth > 1) {
		MenuStackDepth -= 2;
		if (MenuStack[MenuStackDepth].isNewStyle) {
			OptionsActive = true;
			CurrentBuiltinScreen = builtinscreenkind_t::none;
			CurrentGeneratedMenu = nullptr;
			CurrentMenu = MenuStack[MenuStackDepth].menu.newmenu;
			CurrentItem = CurrentMenu->lastOn;
		} else if (MenuStack[MenuStackDepth].isBuiltin) {
			OptionsActive = false;
			CurrentGeneratedMenu = nullptr;
			CurrentMenu = nullptr;
			CurrentBuiltinScreen =
			    static_cast<builtinscreenkind_t>(MenuStack[MenuStackDepth].menu.builtin);
			if (CurrentBuiltinScreen == builtinscreenkind_t::help)
			{
				CurrentBuiltinItem = 0;
			}
			else if (CurrentBuiltinScreen == builtinscreenkind_t::saveload)
			{
				CurrentBuiltinItem =
				    gSaveLoadMode == saveloadmode_t::save ? gSaveLastOn : gLoadLastOn;
			}
			else if (CurrentBuiltinScreen == builtinscreenkind_t::playersetup)
			{
				CurrentBuiltinItem = gPlayerSetupLastOn;
			}
		} else if (MenuStack[MenuStackDepth].isGenerated) {
			OptionsActive = false;
			CurrentBuiltinScreen = builtinscreenkind_t::none;
			CurrentMenu = nullptr;
			CurrentGeneratedMenu = MenuStack[MenuStackDepth].menu.generated;
			CurrentGeneratedItem = CurrentGeneratedMenu->lastOn;
		}
		drawIndicator = MenuStack[MenuStackDepth].drawIndicator;
		MenuStackDepth++;
		S_Sound (CHAN_INTERFACE, "switches/normbutn", 1, ATTN_NONE);
	} else {
		M_ClearMenus ();
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

	if (CurrentBuiltinScreen == builtinscreenkind_t::playersetup)
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
	OptionsActive = false;
	menuactive = 0;
	MenuTime = 0;
	CurrentGeneratedMenu = nullptr;
	CurrentGeneratedItem = 0;
	whichIndicator = 0;
	indicatorAnimCounter = 10;
	drawIndicator = true;
	screenSize = screenblocks.asInt() - 3;
	messageToPrint = 0;
	messageString = NULL;
	messageLastMenuActive = menuactive;

	if (gameinfo.flags & GI_MAPxx)
	{
		gGeneratedMainMenu.x = MAINMENU_DEFAULT_X;
		gGeneratedMainMenu.y = MAINMENU_DEFAULT_Y + 8;
	}
	else if (gameinfo.enginetype == ENGINE_HERETIC)
	{
		gGeneratedMainMenu.x = 110;
		gGeneratedMainMenu.y = 56;
	}
	else
	{
		gGeneratedMainMenu.x = MAINMENU_DEFAULT_X;
		gGeneratedMainMenu.y = MAINMENU_DEFAULT_Y;
	}

	M_ConfigureSaveLoadScreen();

	if (!BuildGeneratedMenu(gGeneratedMainMenu, "main", gGeneratedMainMenu.x, gGeneratedMainMenu.y, 0))
	{
		I_Error("M_Init: MENUCONF main menu is missing or empty");
	}
	BuildGeneratedMenu(gGeneratedEpisodeMenu, "episodes", SUBMENU_DEFAULT_X, SUBMENU_DEFAULT_Y, 0);
	BuildGeneratedMenu(gGeneratedExpansionMenu, "expansions", SUBMENU_DEFAULT_X, SUBMENU_DEFAULT_Y, 0);
	BuildGeneratedMenu(gGeneratedSkillMenu, "skills", SUBMENU_DEFAULT_X, SUBMENU_DEFAULT_Y, defaultskillmenu);
	BuildGeneratedMenu(gGeneratedGameFilesMenu, "gamefiles", GAMEFILES_DEFAULT_X, GAMEFILES_DEFAULT_Y, 0);

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
