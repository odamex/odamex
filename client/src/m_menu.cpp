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

#include <array>
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
#include "m_help.h"
#include "m_loadsave.h"
#include "m_videomodes.h"
#include "m_widgets.h"
#include "m_playersetup.h"

#include "gi.h"
#include "g_skill.h"

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

int                 repeatKey;
int                 repeatCount;

extern bool			sendpause;

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

enum class BuiltInScreen
{
	none,
	help,
	saveload,
	playersetup,
	videomodes,
	count
};

static BuiltInScreen CurrentBuiltinScreen = BuiltInScreen::none;
static int CurrentBuiltinItem = 0;

struct builtinscreendef_t
{
	void (*init)() = nullptr;
	void (*restore)(int&) = nullptr;
	void (*draw)(int) = nullptr;
	bool (*indicatorPosition)(int, int&, int&) = nullptr;
	void (*respond)(int, int, bool, int&) = nullptr;
	void (*ticker)() = nullptr;
	void (*shutdown)() = nullptr;
};

struct generatedmenu_t
{
	std::string menuId;
	std::vector<menuconfitem_t> items;
	int x = 0;
	int y = 0;
	int lastOn = 0;
};

static generatedmenu_t* CurrentGeneratedMenu = nullptr;
static int CurrentGeneratedItem = 0;
static std::unordered_map<std::string, generatedmenu_t> generatedMenus;
static std::string selectedEpisodeId;

//
// PROTOTYPES
//
void M_ChooseSkill(int choice);
void M_ActivateGeneratedMenuItem(int choice);

void M_StartGame(int choice);

void M_QuickSave();
void M_QuickLoad();

void M_StartControlPanel();
void M_StartMessage(const char *string,void (*routine)(int),bool input);
void M_StopMessage();
void M_ClearMenus();
void M_OpenGeneratedMenu(generatedmenu_t& menu, bool newDrawIndicator = true);
void M_DrawGeneratedMenu();
void M_GeneratedMenuResponder(int ch, int ch2, bool numlock);
static bool GeneratedMenuItemSelectable(const menuconfitem_t& item);

static void M_OpenNewGameMenu();
static void M_OpenLoadGameScreen();
static bool M_OpenSaveGameScreen();
static void M_OpenOptionsMenu();
static void M_OpenHelpScreen();
static void M_BeginEndGamePrompt();
static void M_BeginQuitGamePrompt();
static void M_PushBuiltinScreen(BuiltInScreen screen, int initialItem, bool newDrawIndicator);
static void M_BuiltinResponder(int ch, int ch2, bool numlock);
namespace
{
	struct menudestination_t;
	bool M_OpenMenuTargetImpl(const std::string& target);
	bool M_OpenMenuEntrypointImpl(const std::string& name);
}
bool M_DemoNoPlay;
static int M_BigFontLineHeight();

namespace
{
	const builtinscreendef_t* BuiltInScreenDef(BuiltInScreen screen);
	bool BuildGeneratedMenu(generatedmenu_t& generatedMenu, const char* menuId, int defaultLastOn);

	bool GeneratedMenuIndicatorPosition(int& x, int& y)
	{
		if (CurrentGeneratedMenu == nullptr || !drawIndicator)
		{
			return false;
		}

		x = CurrentGeneratedMenu->x + M_MenuIndicatorOffsetX();
		y = CurrentGeneratedMenu->y + M_MenuIndicatorOffsetY() +
		    CurrentGeneratedItem * M_BigFontLineHeight();
		return true;
	}

	int DefaultGeneratedMenuLastOn(const std::string& menuId)
	{
		return iequals(menuId, "skills") ? defaultskillmenu : 0;
	}

	void M_BuildGeneratedMenus()
	{
		generatedMenus.clear();
		generatedMenus.reserve(M_MenuConf().menus.size());

		for (const auto& [menuId, authoredMenu] : M_MenuConf().menus)
		{
			if (authoredMenu.items.empty())
			{
				continue;
			}

			generatedmenu_t& generatedMenu = generatedMenus[menuId];
			BuildGeneratedMenu(generatedMenu, menuId.c_str(), DefaultGeneratedMenuLastOn(menuId));
		}
	}

	void M_DrawMenuIndicatorOverlay()
	{
		if (!drawIndicator)
		{
			return;
		}

		int x = 0;
		int y = 0;
		bool havePosition = false;
		const builtinscreendef_t* builtin = BuiltInScreenDef(CurrentBuiltinScreen);
		const patch_t* indicator = M_MenuIndicatorPatch(whichIndicator);

		if (CurrentGeneratedMenu != nullptr)
		{
			havePosition = GeneratedMenuIndicatorPosition(x, y);
		}
		else if (builtin != nullptr && builtin->indicatorPosition != nullptr)
		{
			havePosition = builtin->indicatorPosition(CurrentBuiltinItem, x, y);
		}

		if (!havePosition)
		{
			return;
		}

		screen->DrawPatchClean(indicator, x, y);
	}

	constexpr auto BuiltInScreenDefs = std::array{
	    builtinscreendef_t{},
	    builtinscreendef_t{nullptr, M_HelpRestore, M_HelpDrawer, nullptr,
	                      M_HelpResponder,
	                      nullptr, M_HelpShutdown},
	    builtinscreendef_t{M_LoadSaveInit, M_LoadSaveRestore, M_LoadSaveDrawer,
	                      M_LoadSaveIndicatorPosition,
	                      M_LoadSaveResponder, nullptr, nullptr},
	    builtinscreendef_t{M_PlayerSetupInit, M_PlayerSetupOpen,
	                      M_PlayerSetupDrawer, M_PlayerSetupIndicatorPosition,
	                      M_PlayerSetupResponder,
	                      M_PlayerSetupTicker, M_PlayerSetupShutdown},
	    builtinscreendef_t{M_VideoModesInit, M_VideoModesRestore, M_VideoModesDrawer,
	                      nullptr,
	                      M_VideoModesResponder, nullptr, nullptr},
	};
	static_assert(BuiltInScreenDefs.size() == static_cast<size_t>(BuiltInScreen::count));

	const builtinscreendef_t* BuiltInScreenDef(BuiltInScreen screen)
	{
		const size_t index = static_cast<size_t>(screen);
		return index < BuiltInScreenDefs.size() ? &BuiltInScreenDefs[index] : nullptr;
	}
}

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

//
// OPTIONS MENU
//
// [RH] This menu is now handled in m_options.c
//
bool OptionsActive;

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
}
END_COMMAND (menu_save)

BEGIN_COMMAND (menu_load)
{
    // F3
	M_OpenMenuTarget("builtin:loadGame");
}
END_COMMAND (menu_load)

BEGIN_COMMAND (menu_options)
{
    // F4
	M_OpenMenuEntrypoint("optionsMenu");
}
END_COMMAND (menu_options)

BEGIN_COMMAND (menu_display)
{
	// F5
	M_OpenMenuTarget("options.display");
}
END_COMMAND (menu_display)

BEGIN_COMMAND (quicksave)
{
    // F6
	M_QuickSave ();
}
END_COMMAND (quicksave)

BEGIN_COMMAND (menu_endgame)
{
	// F7
	M_BeginEndGamePrompt();
}
END_COMMAND (menu_endgame)

BEGIN_COMMAND (quickload)
{
	// F9
	M_QuickLoad ();
}
END_COMMAND (quickload)

BEGIN_COMMAND (menu_quit)
{
	// F10
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

BEGIN_COMMAND (menu_video)
{
	M_OpenMenuTarget("builtin:videoMode");
}
END_COMMAND (menu_video)

const char* M_LocalizedMenuString(const char* key)
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

	generatedmenu_t* GeneratedMenuById(const char* menuId)
	{
		const auto it = generatedMenus.find(menuId);
		return it != generatedMenus.end() ? &it->second : nullptr;
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

	void WarnMenuConfOnce(const std::string& message)
	{
		static std::unordered_set<std::string> warnedMessages;
		if (warnedMessages.insert(message).second)
		{
			M_WarnMenuConf(message);
		}
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
		for (int i = 0; i < skillnum; ++i)
		{
			if (iequals(SkillInfos[i].name, id))
			{
				return i;
			}
		}

		return -1;
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

	void DrawGeneratedMenuHeader(const generatedmenu_t& generatedMenu)
	{
		if (generatedMenu.menuId.empty())
		{
			return;
		}

		const menuconfmenu_t* menu = MenuConfMenu(generatedMenu.menuId.c_str());
		if (menu == nullptr)
		{
			return;
		}

		const patch_t* headerPatch =
		    M_MenuConfConfiguredPatch(menu->header.patch, "menu.header.patch");
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
				headerText = M_LocalizedMenuString(menu->header.languageKey.c_str());
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

			M_WarnMenuConf(fmt::sprintf("unsupported builtin target \"%s\"", target.c_str()));
			return false;
		}

		if (M_MenuConf().menus.find(target) == M_MenuConf().menus.end())
		{
			M_WarnMenuConf(fmt::sprintf("unknown menu target \"%s\"", target.c_str()));
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
			M_WarnMenuConf(fmt::sprintf("missing entrypoint \"%s\"", name.c_str()));
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
			return M_OpenSaveGameScreen();
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

		M_WarnMenuConf(fmt::sprintf("unsupported builtin target \"builtin:%s\"", builtinId.c_str()));
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
				M_WarnMenuConf(fmt::sprintf("menu target \"%s\" is not wired to the generated runtime yet",
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

	bool BuildGeneratedMenu(generatedmenu_t& generatedMenu, const char* menuId, int defaultLastOn)
	{
		const menuconfmenu_t* authoredMenu = MenuConfMenu(menuId);
		if (authoredMenu == nullptr || authoredMenu->items.empty())
		{
			generatedMenu.items.clear();
			generatedMenu.menuId = menuId;
			generatedMenu.x = 0;
			generatedMenu.y = 0;
			generatedMenu.lastOn = defaultLastOn;
			return false;
		}

		generatedMenu.menuId = menuId;
		generatedMenu.items = authoredMenu->items;
		generatedMenu.lastOn = iequals(menuId, "skills") ? defaultskillmenu : defaultLastOn;
		generatedMenu.x = authoredMenu->layout.x;
		generatedMenu.y = authoredMenu->layout.y;
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
		const char* base = !item.languageKey.empty() ? M_LocalizedMenuString(item.languageKey.c_str()) :
		                   item.text.c_str();
		if (item.kind != menuconfitemkind_t::cvarDiscrete)
		{
			return base;
		}

		const char* value = GeneratedDiscreteValueName(item);
		return value[0] ? fmt::format("{}: {}", base, value) : std::string(base);
	}
}

bool M_OpenMenuTarget(const std::string& target)
{
	const bool startedControlPanel = !menuactive;
	if (startedControlPanel)
	{
		M_StartControlPanel();
	}

	const bool opened = M_OpenMenuTargetImpl(target);
	if (!opened && startedControlPanel)
	{
		M_ClearMenus();
	}

	return opened;
}

void M_PlayMenuSound(std::string_view role,
                     const std::string* overrideSound,
                     std::string_view menuId)
{
	PlayMenuSound(role, overrideSound, menuId);
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
	const bool startedControlPanel = !menuactive;
	if (startedControlPanel)
	{
		M_StartControlPanel();
	}

	const bool opened = M_OpenMenuEntrypointImpl(name);
	if (!opened && startedControlPanel)
	{
		M_ClearMenus();
	}

	return opened;
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
			M_WarnMenuConf(fmt::sprintf("discrete menu item \"%s\" is not wired correctly",
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
				                                       M_LocalizedMenuString(gameinfo.sharewareMessage.c_str());
				M_StartMessage(sharewareMessage, NULL, false);
				M_ClearMenus();
				return;
			}

			selectedEpisodeId.clear();
			if (item.params.isObject() && item.params["episode"].isString())
			{
				selectedEpisodeId = item.params["episode"].asString();
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
				M_WarnMenuConf("startGame item is missing a string skill param");
				actionSucceeded = false;
			}
			else
			{
				const int skill = SkillIndexForId(item.params["skill"].asString());
				if (skill < 0)
				{
					M_WarnMenuConf(fmt::sprintf("unknown skill id \"%s\"",
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
			M_WarnMenuConf(fmt::sprintf("menu action \"%s\" is not wired yet", item.action.c_str()));
			actionSucceeded = false;
		}
	}

	if (actionSucceeded && !item.target.empty())
	{
		M_OpenMenuTarget(item.target);
	}
}

static void M_OpenLoadGameScreen()
{
	if (!menuactive)
	{
		M_StartControlPanel();
	}
	M_PushBuiltinScreen(BuiltInScreen::saveload, 0, true);
	M_LoadSaveOpenLoad(CurrentBuiltinItem);
}

//
// Selected from DOOM menu
// [ML] 7 Sept 08: Bringing game saving/loading in from
//                 zdoom 1.22 source, see MAINTAINERS
//
static bool M_OpenSaveGameScreen()
{
	if (multiplayer && !demoplayback)
	{
		M_StartMessage(GStrings(SAVENET),
			NULL,false);
		return true;
	}

	if (demoplayback || netdemo.isPlaying() || !usergame || gamestate != GS_LEVEL)
	{
		M_StartMessage(GStrings(SAVEDEAD),NULL,false);
		return true;
	}

	if (!menuactive)
	{
		M_StartControlPanel();
	}
	M_PushBuiltinScreen(BuiltInScreen::saveload, 0, true);
	M_LoadSaveOpenSave(CurrentBuiltinItem);
	return true;
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
		M_LoadSaveSaveSlot(quickSaveSlot);
		M_PlayMenuSound("close");
	}
}

void M_QuickSave()
{
	if (multiplayer)
	{
		M_PlayMenuSound("invalid");
		M_ClearMenus ();
		return;
	}

	if (!usergame)
	{
		M_PlayMenuSound("invalid");
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
	snprintf (tempstring, 80, GStrings(QSPROMPT), M_LoadSaveSlotName(quickSaveSlot));
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
		M_LoadSaveLoadSlot(quickSaveSlot);
		M_PlayMenuSound("close");
	}
}


void M_QuickLoad()
{
	if (quickSaveSlot < 0)
	{
		M_OpenLoadGameScreen();
		return;
	}
	snprintf(tempstring, 80, GStrings(QLPROMPT), M_LoadSaveSlotName(quickSaveSlot));
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

	if (!M_HelpOpen())
	{
		M_ClearMenus();
		return;
	}

	drawIndicator = false;
	M_PushBuiltinScreen(BuiltInScreen::help, 0, false);
}

//
// Draw border for the savegame description
// [RH] Width of the border is variable
//
void M_OpenGeneratedMenu(generatedmenu_t& menu, bool newDrawIndicator)
{
	MenuStack[MenuStackDepth].menu.generated = &menu;
	MenuStack[MenuStackDepth].isNewStyle = false;
	MenuStack[MenuStackDepth].isBuiltin = false;
	MenuStack[MenuStackDepth].isGenerated = true;
	MenuStack[MenuStackDepth].drawIndicator = newDrawIndicator;
	MenuStackDepth++;

	OptionsActive = false;
	CurrentBuiltinScreen = BuiltInScreen::none;
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
				                       M_LocalizedMenuString(item.languageKey.c_str()) :
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

}

static void M_OpenNewGameMenu()
{
	selectedEpisodeId.clear();
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
		if (selectedEpisodeId.empty())
		{
			M_WarnMenuConf("startGame was invoked without a selected expansion");
			return;
		}

		const std::string str = "nerve.wad";
		const bool isNerve = iequals(selectedEpisodeId, "doom2.nerve");

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
	else if (!selectedEpisodeId.empty())
	{
		const std::string::size_type end = selectedEpisodeId.find_last_not_of("0123456789");
		const std::string::size_type pos = end == std::string::npos ? 0 : end + 1;
		if (pos < selectedEpisodeId.size())
		{
			const int episodeIndex =
			    clamp(std::max(1, atoi(selectedEpisodeId.c_str() + pos)) - 1, 0, episodenum - 1);
			G_DeferedInitNew(EpisodeMaps[episodeIndex]);
		}
		else
		{
			M_WarnMenuConf(fmt::sprintf("episode id \"%s\" has no numeric suffix",
			                          selectedEpisodeId.c_str()));
			return;
		}
	}
	else
	{
		M_WarnMenuConf("startGame was invoked without a selected episode");
		return;
	}

	M_ClearMenus ();
	selectedEpisodeId.clear();
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
		M_PlayMenuSound("invalid");
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
		message = M_LocalizedMenuString(gameinfo.quitMessage.c_str());
	}
	else
	{
		const int offset = count > 1 ? gametic % count : 0;
		const char* indexed = GStrings.getIndex(base_index + offset);

		message = (indexed == nullptr || indexed[0] == '\0') ? 
			M_LocalizedMenuString(gameinfo.quitMessage.c_str()) : indexed;
	}

	if (!gameinfo.quitPrompt.empty())
	{
		return fmt::sprintf("%s\n\n%s", message, M_LocalizedMenuString(gameinfo.quitPrompt.c_str()));
	}

	return fmt::sprintf("%s\n", message);
}

static void M_BeginQuitGamePrompt()
{
	static std::string endstring = M_QuitMessage();
	M_StartMessage(endstring.c_str(), M_QuitResponse, true);
}


void M_OpenPlayerSetupScreen(void)
{
	if (!menuactive)
	{
		M_StartControlPanel();
	}
	M_PushBuiltinScreen(BuiltInScreen::playersetup, 0, true);
	M_PlayerSetupOpen(CurrentBuiltinItem);
}

void M_OpenVideoModeScreen(void)
{
	if (!menuactive)
	{
		M_StartControlPanel();
	}

	M_PushBuiltinScreen(BuiltInScreen::videomodes, 0, true);
	M_VideoModesOpen(CurrentBuiltinItem);
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
		M_PlayMenuSound("close");
		return true;
	}

	// Transfer any action to the Options Menu Responder
	// if we're not on the main menu.
	if (menuactive && OptionsActive) {
		M_OptResponder (ev);
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

	if (CurrentBuiltinScreen != BuiltInScreen::none)
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
	CurrentBuiltinScreen = BuiltInScreen::none;
	CurrentMenu = nullptr;
	CurrentGeneratedMenu = nullptr;
	OptionsActive = false;			// [RH] Make sure none of the options menus appear.
	M_PauseSound();
	M_PlayMenuSound("open");
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
		const builtinscreendef_t* builtin = BuiltInScreenDef(CurrentBuiltinScreen);

		M_DimBackground();

		if (OptionsActive)
		{
			M_OptDrawer();
		}
		else if (builtin != nullptr && builtin->draw != nullptr)
		{
			builtin->draw(CurrentBuiltinItem);
		}
		else if (CurrentGeneratedMenu != nullptr)
		{
			M_DrawGeneratedMenu();
		}

		M_DrawMenuIndicatorOverlay();
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
	for (const builtinscreendef_t& builtin : BuiltInScreenDefs)
	{
		if (builtin.shutdown != nullptr)
		{
			builtin.shutdown();
		}
	}
	MenuStackDepth = 0;
	menuactive = false;
	CurrentBuiltinScreen = BuiltInScreen::none;
	CurrentMenu = nullptr;
	CurrentItem = 0;
	CurrentBuiltinItem = 0;
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
	CurrentBuiltinScreen = BuiltInScreen::none;
}

static void M_PushBuiltinScreen(BuiltInScreen screen, int initialItem, bool newDrawIndicator)
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
	const builtinscreendef_t* builtin = BuiltInScreenDef(CurrentBuiltinScreen);

	if (builtin != nullptr && builtin->respond != nullptr)
	{
		builtin->respond(ch, ch2, numlock, CurrentBuiltinItem);
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
		M_PlayMenuSound("navigate");
		return;
	}
	if (Key_IsUpKey(ch, numlock))
	{
		moveToSelectable(-1);
		M_PlayMenuSound("navigate");
		return;
	}
	if (Key_IsLeftKey(ch, numlock))
	{
		if (CurrentGeneratedMenu->items[CurrentGeneratedItem].kind == menuconfitemkind_t::cvarDiscrete)
		{
			M_ActivateGeneratedMenuItem(0);
			M_PlayMenuSound("changeValue");
		}
		return;
	}
	if (Key_IsRightKey(ch, numlock))
	{
		if (CurrentGeneratedMenu->items[CurrentGeneratedItem].kind == menuconfitemkind_t::cvarDiscrete)
		{
			M_ActivateGeneratedMenuItem(1);
			M_PlayMenuSound("changeValue");
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
			M_PlayMenuSound(item.kind == menuconfitemkind_t::cvarDiscrete ? "changeValue" :
			                                                            "select");
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
				M_PlayMenuSound("navigate");
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
				M_PlayMenuSound("navigate");
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
			CurrentBuiltinScreen = BuiltInScreen::none;
			CurrentGeneratedMenu = nullptr;
			CurrentMenu = MenuStack[MenuStackDepth].menu.newmenu;
			CurrentItem = CurrentMenu->lastOn;
		} else if (MenuStack[MenuStackDepth].isBuiltin) {
			OptionsActive = false;
			CurrentGeneratedMenu = nullptr;
			CurrentMenu = nullptr;
			CurrentBuiltinScreen =
			    static_cast<BuiltInScreen>(MenuStack[MenuStackDepth].menu.builtin);

			const builtinscreendef_t* builtin = BuiltInScreenDef(CurrentBuiltinScreen);
			if (builtin != nullptr && builtin->restore != nullptr)
			{
				builtin->restore(CurrentBuiltinItem);
			}
		} else if (MenuStack[MenuStackDepth].isGenerated) {
			OptionsActive = false;
			CurrentBuiltinScreen = BuiltInScreen::none;
			CurrentMenu = nullptr;
			CurrentGeneratedMenu = MenuStack[MenuStackDepth].menu.generated;
			CurrentGeneratedItem = CurrentGeneratedMenu->lastOn;
		}
		drawIndicator = MenuStack[MenuStackDepth].drawIndicator;
		MenuStackDepth++;
		M_PlayMenuSound("back");
	} else {
		M_ClearMenus ();
		M_PlayMenuSound("close");
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

	const builtinscreendef_t* builtin = BuiltInScreenDef(CurrentBuiltinScreen);
	if (builtin != nullptr && builtin->ticker != nullptr)
	{
		builtin->ticker();
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

	for (const builtinscreendef_t& builtin : BuiltInScreenDefs)
	{
		if (builtin.init != nullptr)
		{
			builtin.init();
		}
	}

	M_BuildGeneratedMenus();

	generatedmenu_t* mainMenu = GeneratedMenuById("main");
	if (mainMenu == nullptr || mainMenu->items.empty())
	{
		I_Error("M_Init: MENUCONF main menu is missing or empty");
	}

	M_BuildGeneratedOptionsMenus();
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
