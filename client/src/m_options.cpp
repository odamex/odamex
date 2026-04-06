// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1998-2006 by Randy Heit (ZDoom 1.22).
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
//	New options menu code.
//
//	Sorry this got so convoluted. It was originally much cleaner until
//	I started adding all sorts of gadgets to the menus. I might someday
//	make a project of rewriting the entire menu system using Amiga-style
//	taglists to describe each menu item. We'll see...
//
//-----------------------------------------------------------------------------


#include "odamex.h"

#include "gstrings.h"
BEGIN_DISABLE_WARNING_GNU("-Wold-style-cast")
#include "minilzo.h"
END_DISABLE_WARNING_GNU

#include "c_console.h"
#include "c_dispatch.h"
#include "c_bind.h"
#include "cl_responderkeys.h"
#include "cmdlib.h"

#include "i_system.h"
#include "i_time.h"
#include "i_video.h"
#include "i_input.h"
#include "z_zone.h"
#include "v_palette.h"
#include "v_video.h"
#include "v_text.h"
#include "w_wad.h"

#include "hu_stuff.h"

#include "m_memio.h"

#include "m_misc.h"
#include "cl_demo.h"
#include "gi.h"
#include "m_menuconf.h"
#include "m_options_valuesets.h"
#include "m_widgets.h"

#include <unordered_map>

// Data.
#include "m_menu.h"

#define MENUBOXWIDTH	236
#define MENUBOXHEIGHT	200
#define TEAMPLAYBORDER	4

//
// defaulted values
//
// [ML] 09/4/06: Show secret revealed message, 0 = off, 1 = on
EXTERN_CVAR (hud_revealsecrets)

// Show messages has default, 0 = off, 1 = on
EXTERN_CVAR (show_messages)

namespace
{
	struct generatedoptionsmenu_t;
	generatedoptionsmenu_t* GeneratedOptionsMenuByMenu(menu_t* menu);
	menu_t* OptionsMenuSlotById(const std::string& menuId);
	std::string_view CurrentOptionsMenuId();
	const std::string* CurrentOptionsItemSound();
	void PlayCurrentOptionsSound(std::string_view role);
}

extern bool				OptionsActive;

extern int				screenSize;
extern short			indicatorAnimCounter;

extern NetDemo netdemo;

EXTERN_CVAR(con_notifytime)
EXTERN_CVAR(con_midtime)

EXTERN_CVAR (i_skipbootwin)
EXTERN_CVAR (cl_run)
EXTERN_CVAR (invertmouse)
EXTERN_CVAR (lookspring)
EXTERN_CVAR (lookstrafe)
EXTERN_CVAR (hud_crosshair)
EXTERN_CVAR (hud_crosshairhealth)
EXTERN_CVAR (hud_crosshairscale)
EXTERN_CVAR (hud_crosshaircolor)
EXTERN_CVAR (r_forceteamcolor)
EXTERN_CVAR (r_teamcolor)
EXTERN_CVAR (r_forceenemycolor)
EXTERN_CVAR (r_enemycolor)
EXTERN_CVAR (r_softinvulneffect)
EXTERN_CVAR (cl_mouselook)
EXTERN_CVAR (in_autosr50)
EXTERN_CVAR (gammalevel)
EXTERN_CVAR (language)
EXTERN_CVAR (mute_spectators)
EXTERN_CVAR (mute_enemies)
EXTERN_CVAR (wi_oldintermission)

// [electricbrass - Menu] HUD Menu
EXTERN_CVAR (hud_targetnames)
EXTERN_CVAR (hud_gamemsgtype)
EXTERN_CVAR (hud_scale)
EXTERN_CVAR (hud_scalescoreboard)
EXTERN_CVAR (hud_timer)
EXTERN_CVAR (hud_speedometer)
EXTERN_CVAR (hud_bigfont)
EXTERN_CVAR (hud_heldflag)
EXTERN_CVAR (hud_heldflag_flash)
EXTERN_CVAR (hud_transparency)
EXTERN_CVAR (hud_anchoring)
EXTERN_CVAR (hud_revealsecrets)
EXTERN_CVAR(hud_feedobits)
EXTERN_CVAR(hud_feedtime)
EXTERN_CVAR(hud_extendedinfo)

// [Ralphis - Menu] Compatibility Menu
EXTERN_CVAR (co_allowdropoff)
EXTERN_CVAR (co_pursuit)
EXTERN_CVAR (co_realactorheight)
EXTERN_CVAR (co_zdoomphys)
EXTERN_CVAR (co_zdoomsound)
EXTERN_CVAR (co_zdoomammo)
EXTERN_CVAR (co_fixweaponimpacts)
EXTERN_CVAR (cl_deathcam)
EXTERN_CVAR (co_fineautoaim)
EXTERN_CVAR (co_nosilentspawns)
EXTERN_CVAR (co_boomphys)			// [ML] Roll-up of various compat options
EXTERN_CVAR (co_mbfphys)
EXTERN_CVAR (co_helpfriends)
EXTERN_CVAR (co_monsterbacking)
EXTERN_CVAR (co_monsterfriction)
EXTERN_CVAR (co_avoidhazards)
EXTERN_CVAR (co_monstersclimbsteep)
EXTERN_CVAR (co_staylift)
EXTERN_CVAR (co_friend_ledgejumping)
EXTERN_CVAR (co_friend_distance)
EXTERN_CVAR (co_removesoullimit)
EXTERN_CVAR (co_blockmapfix)
EXTERN_CVAR (co_globalsound)
EXTERN_CVAR (co_novileghosts)

// [Toke - Menu] New Menu Stuff.
EXTERN_CVAR (mouse_sensitivity)
EXTERN_CVAR (m_pitch)
EXTERN_CVAR (novert)
EXTERN_CVAR (m_side)
EXTERN_CVAR (m_forward)

// [Ralphis - Menu] Sound Menu
EXTERN_CVAR (snd_musicvolume)
EXTERN_CVAR (snd_musicsystem)
EXTERN_CVAR (snd_nomusic)
EXTERN_CVAR (snd_midireset)
EXTERN_CVAR (snd_midifallback)
EXTERN_CVAR (snd_mididelay)
EXTERN_CVAR (snd_midisysex)
EXTERN_CVAR (snd_oplcore)
EXTERN_CVAR (snd_oplpan)
EXTERN_CVAR (snd_oplchips)
EXTERN_CVAR (snd_oplbank)
EXTERN_CVAR (snd_announcervolume)
EXTERN_CVAR (snd_sfxvolume)
EXTERN_CVAR (snd_crossover)
EXTERN_CVAR (snd_gamesfx)
EXTERN_CVAR (snd_voxtype)
EXTERN_CVAR (cl_connectalert)
EXTERN_CVAR (cl_disconnectalert)
EXTERN_CVAR (snd_votesfx)

// Joystick menu -- Hyper_Eye
EXTERN_CVAR (use_joystick)
EXTERN_CVAR (joy_active)
EXTERN_CVAR (joy_forwardaxis)
EXTERN_CVAR (joy_strafeaxis)
EXTERN_CVAR (joy_turnaxis)
EXTERN_CVAR (joy_lookaxis)
EXTERN_CVAR (joy_sensitivity)
EXTERN_CVAR (joy_fastsensitivity)
EXTERN_CVAR (joy_invert)
EXTERN_CVAR (joy_freelook)
EXTERN_CVAR (joy_deadzone)

// Network Options
EXTERN_CVAR (cl_serverdownload)

// Demo Options
EXTERN_CVAR(cl_splitnetdemos)
EXTERN_CVAR(cl_autorecord)
EXTERN_CVAR(cl_autorecord_coop)
EXTERN_CVAR(cl_autorecord_deathmatch)
EXTERN_CVAR(cl_autorecord_duel)
EXTERN_CVAR(cl_autorecord_teamdm)
EXTERN_CVAR(cl_autorecord_ctf)
EXTERN_CVAR(cl_autorecord_horde)

// Spree options
EXTERN_CVAR(cl_showsprees)
EXTERN_CVAR(cl_showmultikills)

// Weapon Preferences
EXTERN_CVAR (cl_switchweapon)
EXTERN_CVAR (cl_weaponpref_fst)
EXTERN_CVAR (cl_weaponpref_csw)
EXTERN_CVAR (cl_weaponpref_pis)
EXTERN_CVAR (cl_weaponpref_sg)
EXTERN_CVAR (cl_weaponpref_ssg)
EXTERN_CVAR (cl_weaponpref_cg)
EXTERN_CVAR (cl_weaponpref_rl)
EXTERN_CVAR (cl_weaponpref_pls)
EXTERN_CVAR (cl_weaponpref_bfg)

void M_ChangeMessages(void);
void M_IncrementDisplaySize(float diff);
void M_StartControlPanel(void);

void M_ClearMenus (void);

bool configuring_controls = false;
static bool	WaitingForKey;
static bool	WaitingForAxis;
static const char	   *OldContMessage;
static itemtype OldContType;
static const char	   *OldAxisMessage;
static itemtype OldAxisType;

static void GoToConsole();
void M_ResetToDefaults();
void M_ResetToSaved();

static void SetVidMode();
static void M_UpdateDisplayOptions();

void M_ResetMouseValues()
{
	mouse_sensitivity.RestoreDefault();
	m_pitch.RestoreDefault();
	cl_mouselook.RestoreDefault();
	invertmouse.RestoreDefault();
	lookstrafe.RestoreDefault();
	novert.RestoreDefault();
	m_side.RestoreDefault();
	m_forward.RestoreDefault();
}


EXTERN_CVAR(cl_chatsounds)

extern const char *weaponnames[];

void M_ResetCustomColors (void);

EXTERN_CVAR (am_rotate)
EXTERN_CVAR (am_overlay)
EXTERN_CVAR (am_thickness)
EXTERN_CVAR (am_showmonsters)
EXTERN_CVAR (am_showitems)
EXTERN_CVAR (am_showsecrets)
EXTERN_CVAR (am_showtime)
EXTERN_CVAR (am_classicmapstring)
EXTERN_CVAR (am_usecustomcolors)
EXTERN_CVAR (am_showlocked)
EXTERN_CVAR (st_scale)
EXTERN_CVAR (r_stretchsky)
EXTERN_CVAR (r_linearsky)
EXTERN_CVAR (r_skypalette)
EXTERN_CVAR (r_wipetype)
EXTERN_CVAR (r_drawplayersprites)
EXTERN_CVAR (screenblocks)
EXTERN_CVAR (ui_dimamount)
EXTERN_CVAR (r_loadicon)
EXTERN_CVAR (r_showendoom)
EXTERN_CVAR (r_painintensity)
EXTERN_CVAR (cl_movebob)
EXTERN_CVAR (cl_centerbobonfire)
EXTERN_CVAR (cl_showspawns)
EXTERN_CVAR (cl_showfriends)
EXTERN_CVAR (hud_show_scoreboard_ondeath)
EXTERN_CVAR (hud_demobar)
EXTERN_CVAR(hud_targetnames)
EXTERN_CVAR(am_ovminimap)
EXTERN_CVAR(am_ovlocation)
EXTERN_CVAR(am_ovscalewidth)
EXTERN_CVAR(am_ovscaleheight)

static void M_SendUINewColor (int red, int green, int blue);
static void M_SlideUIRed (int);
static void M_SlideUIGreen (int);
static void M_SlideUIBlue (int);
static cvar_t *flagsvar;

void M_ResetOptionsBuiltinState()
{
	CanScrollUp = false;
	CanScrollDown = false;
	flagsvar = NULL;
}

CVAR_FUNC_IMPL (ui_transred)
{
    M_SlideUIRed(var.asInt());
}

CVAR_FUNC_IMPL (ui_transgreen)
{
    M_SlideUIGreen(var.asInt());
}

CVAR_FUNC_IMPL (ui_transblue)
{
    M_SlideUIBlue(var.asInt());
}

EXTERN_CVAR (message_showpickups)
EXTERN_CVAR (message_showobituaries)
EXTERN_CVAR (con_coloredmessages)
EXTERN_CVAR (con_scaletext)
EXTERN_CVAR (hud_scaletext)
EXTERN_CVAR (msg0color)
EXTERN_CVAR (msg1color)
EXTERN_CVAR (msg2color)
EXTERN_CVAR (msg3color)
EXTERN_CVAR (msg4color)
EXTERN_CVAR (msgmidcolor)
EXTERN_CVAR (ui_dimcolor)

namespace
{
	struct generatedoptionsmenu_t
	{
		std::string menuId;
		menu_t menu = {"", 0, 0, 0, nullptr, 0, 0, NULL};
		menuconfheader_t header;
		std::vector<menuconfitem_t> sourceItems;
		std::vector<menuitem_t> generatedItems;
		std::vector<std::string> labels;
		std::vector<int> labelWidths;
		std::vector<std::string> commands;
	};

	std::vector<generatedoptionsmenu_t> gGeneratedOptionsMenus;

	generatedoptionsmenu_t* GeneratedOptionsMenuByMenu(menu_t* menu)
	{
		for (generatedoptionsmenu_t& generatedMenu : gGeneratedOptionsMenus)
		{
			if (&generatedMenu.menu == menu)
			{
				return &generatedMenu;
			}
		}

		return nullptr;
	}

	const generatedoptionsmenu_t* GeneratedOptionsMenuByMenu(const menu_t* menu)
	{
		for (const generatedoptionsmenu_t& generatedMenu : gGeneratedOptionsMenus)
		{
			if (&generatedMenu.menu == menu)
			{
				return &generatedMenu;
			}
		}

		return nullptr;
	}

	menu_t* OptionsMenuSlotById(const std::string& menuId)
	{
		for (generatedoptionsmenu_t& generatedMenu : gGeneratedOptionsMenus)
		{
			if (generatedMenu.menuId == menuId)
			{
				return &generatedMenu.menu;
			}
		}

		return nullptr;
	}

	std::string_view CurrentOptionsMenuId()
	{
		const generatedoptionsmenu_t* generatedMenu = GeneratedOptionsMenuByMenu(CurrentMenu);
		return generatedMenu != nullptr ? std::string_view(generatedMenu->menuId) : std::string_view();
	}

	EColorRange OptionsMenuColor(std::string_view role,
	                             const std::string* overrideColor = nullptr)
	{
		return M_MenuTextColor(role, CurrentOptionsMenuId(), overrideColor);
	}

	const std::string* CurrentOptionsItemSound()
	{
		const generatedoptionsmenu_t* generatedMenu = GeneratedOptionsMenuByMenu(CurrentMenu);
		if (generatedMenu == nullptr || CurrentItem < 0 ||
		    static_cast<size_t>(CurrentItem) >= generatedMenu->sourceItems.size())
		{
			return nullptr;
		}

		const std::string& sound = generatedMenu->sourceItems[CurrentItem].sound;
		return sound.empty() ? nullptr : &sound;
	}

	void PlayCurrentOptionsSound(std::string_view role)
	{
		M_PlayMenuSound(role, CurrentOptionsItemSound(), CurrentOptionsMenuId());
	}

	float CurrentDiscreteValue(const menuitem_t& item)
	{
		if (item.a.cvar == nullptr)
		{
			return 0.0f;
		}

		if ((item.a.cvar->flags() & CVAR_LATCH) && (item.a.cvar->flags() & CVAR_MODIFIED) &&
		    item.a.cvar->latched()[0] != '\0')
		{
			return static_cast<float>(atof(item.a.cvar->latched()));
		}

		return item.a.cvar->value();
	}

	const char* ResolveOptionLabel(const menuconfitem_t& item)
	{
		if (!item.languageKey.empty())
		{
			return M_LocalizedMenuString(item.languageKey.c_str());
		}

		if (!item.text.empty())
		{
			return item.text.c_str();
		}

		if (item.textProvider.rfind("weaponName:", 0) == 0)
		{
			const int index = atoi(item.textProvider.c_str() + 11);
			return index >= 0 && index < 9 ? weaponnames[index] : "";
		}

		return "";
	}

	itemtype ResolveGeneratedLabelType(const menuconfitem_t& item)
	{
		if (item.kind == menuconfitemkind_t::separator)
		{
			return redtext;
		}

		if (item.kind == menuconfitemkind_t::label)
		{
			if (item.style == "heading") return yellowtext;
			if (item.style == "warning") return orangetext;
			return whitetext;
		}

		if (item.kind == menuconfitemkind_t::submenu || item.kind == menuconfitemkind_t::action)
		{
			return more;
		}

		if (item.kind == menuconfitemkind_t::controlBinding)
		{
			if (item.bindingSet == "automap") return mapcontrol;
			if (item.bindingSet == "netdemo") return netdemocontrol;
			return control;
		}

		if (item.kind == menuconfitemkind_t::dynamic)
		{
			if (item.provider == "activeJoystick") return joyactive;
			if (item.provider == "gammaSlider") return slider;
			if (item.provider == "overlayMode") return discrete;
		}

		if (item.kind == menuconfitemkind_t::cvarSlider)
		{
			if (item.cvar == "ui_transred") return redslider;
			if (item.cvar == "ui_transgreen") return greenslider;
			if (item.cvar == "ui_transblue") return blueslider;

			if (item.widget == "colorChannel")
			{
				if (item.channel == "red") return redslider;
				if (item.channel == "green") return greenslider;
				if (item.channel == "blue") return blueslider;
			}

			return slider;
		}

		if (item.kind == menuconfitemkind_t::cvarDiscrete)
		{
			if (item.widget == "colorChoice")
			{
				return cdiscrete;
			}

			cvar_t* dummy = nullptr;
			if (cvar_t* cvar = cvar_t::FindCVar(item.cvar, &dummy))
			{
				if (cvar->flags() & CVAR_SERVERINFO)
				{
					return svdiscrete;
				}
			}

			return discrete;
		}

		return nochoice;
	}

	cvar_t* ResolveOptionCVar(const std::string& name)
	{
		if (name.empty())
		{
			return nullptr;
		}

		cvar_t* dummy = nullptr;
		return cvar_t::FindCVar(name, &dummy);
	}

	bool HasBindingItems(const generatedoptionsmenu_t& generatedMenu)
	{
		for (const menuitem_t& item : generatedMenu.generatedItems)
		{
			if (item.type == control || item.type == mapcontrol || item.type == netdemocontrol)
			{
				return true;
			}
		}

		return false;
	}

	bool IsUIChannelSlider(const cvar_t* cvar)
	{
		if (cvar == nullptr)
		{
			return false;
		}

		return iequals(cvar->name(), "ui_transred") || iequals(cvar->name(), "ui_transgreen") ||
		       iequals(cvar->name(), "ui_transblue");
	}

	struct colorsliderstate_t
	{
		argb_t tint;
		int value = 0;
	};

	argb_t CachedMenuColor(const cvar_t* cvar)
	{
		struct cachedmenucolor_t
		{
			std::string text;
			argb_t color;
		};

		static std::unordered_map<const cvar_t*, cachedmenucolor_t> cache;

		if (cvar == nullptr)
		{
			return argb_t();
		}

		const char* current = cvar->cstring();
		cachedmenucolor_t& entry = cache[cvar];
		if (entry.text != current)
		{
			entry.text = current;
			entry.color = V_GetColorFromString(entry.text);
		}

		return entry.color;
	}

	colorsliderstate_t MenuColorSliderState(const menuitem_t& item)
	{
		colorsliderstate_t state;

		if (IsUIChannelSlider(item.a.cvar))
		{
			state.value = item.a.cvar->asInt();
			switch (item.type)
			{
			case redslider:
				state.tint = argb_t(state.value, 0, 0);
				break;
			case greenslider:
				state.tint = argb_t(0, state.value, 0);
				break;
			case blueslider:
				state.tint = argb_t(0, 0, state.value);
				break;
			default:
				break;
			}
			return state;
		}

		const argb_t color = CachedMenuColor(item.a.cvar);
		switch (item.type)
		{
		case redslider:
			state.value = color.getr();
			state.tint = argb_t(state.value, 0, 0);
			break;
		case greenslider:
			state.value = color.getg();
			state.tint = argb_t(0, state.value, 0);
			break;
		case blueslider:
			state.value = color.getb();
			state.tint = argb_t(0, 0, state.value);
			break;
		default:
			break;
		}

		return state;
	}

	int MenuSliderChannelValue(const menuitem_t& item)
	{
		return MenuColorSliderState(item).value;
	}

	void SetMenuSliderChannelValue(menuitem_t& item, int part)
	{
		part = clamp(part, 0, 255);

		if (IsUIChannelSlider(item.a.cvar))
		{
			item.a.cvar->Set(static_cast<float>(part));
			return;
		}

		const char* oldcolor = item.a.cvar->cstring();
		char newcolor[9];

		if (strlen(oldcolor) == 8)
			memcpy(newcolor, oldcolor, 9);
		else
			memcpy(newcolor, "00 00 00", 9);

		char singlecolor[3];
		snprintf(singlecolor, 3, "%02x", part);

		if (item.type == redslider)
			memcpy(newcolor, singlecolor, 2);
		else if (item.type == greenslider)
			memcpy(newcolor + 3, singlecolor, 2);
		else if (item.type == blueslider)
			memcpy(newcolor + 6, singlecolor, 2);

		item.a.cvar->Set(newcolor);
	}

	bool IsSelectableOptionItem(const menuitem_t& item)
	{
		if (item.type == redtext || item.type == whitetext || item.type == yellowtext ||
		    item.type == orangetext)
		{
			return false;
		}

		if (item.type == screenres && !item.b.res1)
		{
			return false;
		}

		return true;
	}

	int FirstSelectableOptionIndex(const generatedoptionsmenu_t& generatedMenu)
	{
		for (size_t i = 0; i < generatedMenu.generatedItems.size(); ++i)
		{
			if (IsSelectableOptionItem(generatedMenu.generatedItems[i]))
			{
				return static_cast<int>(i);
			}
		}

		return 0;
	}

	void ActivateGeneratedOptionsItem()
	{
		generatedoptionsmenu_t* generatedMenu = GeneratedOptionsMenuByMenu(CurrentMenu);
		if (generatedMenu == nullptr || CurrentItem < 0 ||
		    static_cast<size_t>(CurrentItem) >= generatedMenu->sourceItems.size())
		{
			return;
		}

		const menuconfitem_t& item = generatedMenu->sourceItems[CurrentItem];

		if (item.action == "openConsole")
		{
			GoToConsole();
			return;
		}
		if (item.action == "resetDefaults")
		{
			M_ResetToDefaults();
			return;
		}
		if (item.action == "resetSaved")
		{
			M_ResetToSaved();
			return;
		}
		if (item.action == "resetMouseDefaults")
		{
			M_ResetMouseValues();
			return;
		}
		if (item.action == "resetCustomMapColors")
		{
			M_ResetCustomColors();
			return;
		}

		if (!item.target.empty())
		{
			M_OpenMenuTarget(item.target);
			return;
		}

		if (!item.action.empty())
		{
			M_WarnMenuConf(fmt::sprintf("options action \"%s\" is not wired yet",
			                            item.action.c_str()));
		}
	}

	bool BuildGeneratedOptionsMenu(generatedoptionsmenu_t& generatedMenu, const std::string& menuId)
	{
		const auto it = M_MenuConf().menus.find(menuId);
		if (it == M_MenuConf().menus.end())
		{
			generatedMenu.menu.numitems = 0;
			generatedMenu.menu.items = nullptr;
			return false;
		}

		const menuconfmenu_t& authored = it->second;
		generatedMenu.menuId = menuId;
		generatedMenu.header = authored.header;
		generatedMenu.sourceItems = authored.items;
		generatedMenu.generatedItems.clear();
		generatedMenu.labels.clear();
		generatedMenu.labelWidths.clear();
		generatedMenu.commands.clear();
		generatedMenu.generatedItems.reserve(authored.items.size());
		generatedMenu.labels.reserve(authored.items.size());
		generatedMenu.labelWidths.reserve(authored.items.size());
		generatedMenu.commands.reserve(authored.items.size());
		const OFont* smallFont = OFonts.small();

		for (const menuconfitem_t& item : generatedMenu.sourceItems)
		{
			menuitem_t generated = {};
			generated.type = ResolveGeneratedLabelType(item);

			std::string label = ResolveOptionLabel(item);
			if (generated.type == redtext && item.kind == menuconfitemkind_t::separator)
			{
				label = " ";
			}
			generatedMenu.labels.push_back(std::move(label));
			generatedMenu.labelWidths.push_back(
			    smallFont != nullptr ? V_StringWidth(smallFont, generatedMenu.labels.back().c_str()) : 0);
			generated.label = generatedMenu.labels.back().c_str();

			if (generated.type == more)
			{
				generated.e.mfunc = ActivateGeneratedOptionsItem;
			}
			else if (generated.type == control || generated.type == mapcontrol ||
			         generated.type == netdemocontrol)
			{
				generatedMenu.commands.push_back(item.command);
				generated.e.command = generatedMenu.commands.back().c_str();
			}
			else if (generated.type == joyactive)
			{
				generated.a.cvar = ResolveOptionCVar(item.cvar);
			}
			else if (generated.type == slider || generated.type == redslider ||
			         generated.type == greenslider || generated.type == blueslider)
			{
				generated.a.cvar = ResolveOptionCVar(item.cvar);
				generated.b.leftval = static_cast<float>(item.min);
				generated.c.rightval = static_cast<float>(item.max);
				generated.d.step = static_cast<float>(item.step);
			}
			else if (generated.type == discrete || generated.type == cdiscrete ||
			         generated.type == svdiscrete)
			{
				generated.a.cvar = ResolveOptionCVar(item.cvar);
				if (item.provider == "overlayMode")
				{
					int count = 0;
					generated.e.values = M_OptionValueSet("Overlays", count);
					generated.b.leftval = static_cast<float>(count);
				}
				else
				{
					int count = 0;
					generated.e.values = M_OptionValueSet(item.values, count);
					generated.b.leftval = static_cast<float>(count);
				}
			}

			if ((generated.type == slider || generated.type == redslider ||
			     generated.type == greenslider || generated.type == blueslider ||
			     generated.type == joyactive || generated.type == discrete ||
			     generated.type == cdiscrete || generated.type == svdiscrete) &&
			    generated.a.cvar == nullptr)
			{
				M_WarnMenuConf(fmt::sprintf("options item \"%s\" references unknown cvar \"%s\"",
				                            generated.label ? generated.label : "",
				                            item.cvar.c_str()));
				continue;
			}

			if ((generated.type == discrete || generated.type == cdiscrete ||
			     generated.type == svdiscrete) &&
			    generated.e.values == nullptr)
			{
				M_WarnMenuConf(fmt::sprintf("options item \"%s\" references unknown value set \"%s\"",
				                            generated.label ? generated.label : "",
				                            item.values.c_str()));
				continue;
			}

			generatedMenu.generatedItems.push_back(generated);
		}

		generatedMenu.menu.title = authored.header.patch.empty() ? "" : authored.header.patch.c_str();
		generatedMenu.menu.lastOn = FirstSelectableOptionIndex(generatedMenu);
		generatedMenu.menu.numitems = static_cast<int>(generatedMenu.generatedItems.size());
		generatedMenu.menu.indent = authored.layout.indent;
		generatedMenu.menu.items = generatedMenu.generatedItems.data();
		generatedMenu.menu.scrolltop = authored.layout.scrollTop;
		generatedMenu.menu.scrollpos = 0;
		generatedMenu.menu.refreshfunc = menuId == "options.display" ? &M_UpdateDisplayOptions : NULL;
		return true;
	}

	const char* GeneratedOptionsHeaderText(const generatedoptionsmenu_t* generatedMenu)
	{
		if (generatedMenu == nullptr)
		{
			return "OPTIONS";
		}

		if (!generatedMenu->header.languageKey.empty())
		{
			return M_LocalizedMenuString(generatedMenu->header.languageKey.c_str());
		}

		if (!generatedMenu->header.text.empty())
		{
			return generatedMenu->header.text.c_str();
		}

		return "OPTIONS";
	}
}

static void M_UpdateDisplayOptions()
{
	menu_t* menu = OptionsMenuSlotById("options.display");
	if (menu == nullptr || menu->items == nullptr)
	{
		return;
	}

	generatedoptionsmenu_t* generatedMenu = GeneratedOptionsMenuByMenu(menu);
	if (generatedMenu == nullptr)
	{
		return;
	}

	for (menuitem_t& item : generatedMenu->generatedItems)
	{
		if (item.a.cvar == &gammalevel)
		{
			item.b.leftval = V_GetMinimumGammaLevel();
			item.c.rightval = V_GetMaximumGammaLevel();
			item.d.step = 0.1f;
			break;
		}
	}
}

void M_BuildGeneratedOptionsMenus()
{
	static const char* const kMenuIds[] = {
		"options",
		"options.controls",
		"options.mouse",
		"options.joystick",
		"options.sound",
		"options.sound.advancedMidi",
		"options.sound.opl",
		"options.compat",
		"options.network",
		"options.weapons",
		"options.display",
		"options.display.hud",
		"options.display.messages",
		"options.display.automap",
	};

	gGeneratedOptionsMenus.clear();
	gGeneratedOptionsMenus.reserve(ARRAY_LENGTH(kMenuIds));

	for (const char* menuId : kMenuIds)
	{
		gGeneratedOptionsMenus.emplace_back();
		if (!BuildGeneratedOptionsMenu(gGeneratedOptionsMenus.back(), menuId))
		{
			gGeneratedOptionsMenus.pop_back();
		}
	}
}

// [Russell] - Modified to send new colours
static void M_SendUINewColor (int red, int green, int blue)
{
	AddCommandString(fmt::format("ui_dimcolor \"{:02} {:02x} {:02}\"", red, green, blue));
}

static void M_SlideUIRed(int val)
{
	argb_t color = V_GetColorFromString(ui_dimcolor);
	color.setr(val);
	M_SendUINewColor(color.getr(), color.getg(), color.getb());
}

static void M_SlideUIGreen (int val)
{
	argb_t color = V_GetColorFromString(ui_dimcolor);
	color.setg(val);
	M_SendUINewColor(color.getr(), color.getg(), color.getb());
}

static void M_SlideUIBlue (int val)
{
	argb_t color = V_GetColorFromString(ui_dimcolor);
	color.setb(val);
	M_SendUINewColor(color.getr(), color.getg(), color.getb());
}

bool M_PrepareGeneratedOptionsMenu(const std::string& menuId, menu_t*& menu)
{
	menu = OptionsMenuSlotById(menuId);
	if (menu == nullptr)
	{
		M_WarnMenuConf(fmt::sprintf("options target \"%s\" is not mapped to a runtime menu",
		                            menuId.c_str()));
		return false;
	}

	const generatedoptionsmenu_t* generatedMenu = GeneratedOptionsMenuByMenu(menu);
	if (generatedMenu == nullptr)
	{
		M_WarnMenuConf(fmt::sprintf("options menu \"%s\" is not available", menuId.c_str()));
		return false;
	}

	if (HasBindingItems(*generatedMenu))
	{
		M_BuildKeyList(menu->items, menu->numitems);
	}

	if (menu->numitems > 0 &&
	    (menu->lastOn < 0 || menu->lastOn >= menu->numitems ||
	     !IsSelectableOptionItem(menu->items[menu->lastOn])))
	{
		menu->lastOn = FirstSelectableOptionIndex(*generatedMenu);
	}

	M_ResetOptionsBuiltinState();
	return true;
}

void M_OptDrawer (void)
{
	const OFont* smallFont = OFonts.small();
	const OFont* bigFont = OFonts.big();
	int color;
	int y, width, i, x, ytop;
	int ystart = 15;
	menuitem_t *item;
	patch_t *title;
	const int lineHeight = smallFont->lineHeight();
	const palette_t* palette = V_GetPaletteFromLump("ODAPAL");
	const generatedoptionsmenu_t* generatedMenu = GeneratedOptionsMenuByMenu(CurrentMenu);

	if (W_CheckNumForName(CurrentMenu->title) >= 0)
	{
		title = W_CachePatch (CurrentMenu->title);
		screen->DrawPatchCleanWithPalette(title, 160-title->width()/2, 10, palette);
		y = ystart + title->height();
	}
	else
	{	
		const char* titleText = GeneratedOptionsHeaderText(generatedMenu);
		int titlewidth = V_StringWidth(bigFont, titleText) * CleanXfac;
		int titleX = (I_GetSurfaceWidth() / 2) - (titlewidth / 2);
		int titleY = 20*CleanYfac;
		screen->DrawTextClean(bigFont, OptionsMenuColor("title"), titleX, titleY,
		                      titleText);
		y = ystart;
	}
	ytop = y + CurrentMenu->scrolltop * lineHeight;

	for (i = 0; i < CurrentMenu->numitems && y <= 200 - lineHeight; i++, y += lineHeight)	// TIJ
	{
		if (i == CurrentMenu->scrolltop)
			i += CurrentMenu->scrollpos;

		item = CurrentMenu->items + i;

		if (item->type == screenres)
		{
			const char *str = NULL;

			for (x = 0; x < 3; x++)
			{
				switch (x)
				{
				case 0:
					str = item->b.res1;
					break;
				case 1:
					str = item->c.res2;
					break;
				case 2:
					str = item->d.res3;
					break;
				}
				if (str)
				{
					if (x == item->e.highlight)
						color = OptionsMenuColor("itemHighlight");
					else
						color = OptionsMenuColor("item");
					screen->DrawTextCleanMove(smallFont, color, 104 * x + 20, y, str);
				}
			}

			if (i == CurrentItem &&
			    ((item->a.selmode != -1 && (indicatorAnimCounter < 6 || WaitingForKey)) ||
			     WaitingForAxis))
			{
				const patch_t* cursor = menu::cursor::Patch();
				screen->DrawPatchCleanWithPalette(cursor, item->a.selmode * 104 + 8,
						y + menu::cursor::OffsetY(), palette);
			}
		}
		else
		{
			width = generatedMenu != nullptr && static_cast<size_t>(i) < generatedMenu->labelWidths.size() ?
			            generatedMenu->labelWidths[i] :
			            V_StringWidth(smallFont, item->label);
			switch (item->type)
			{
			case more:
				x = CurrentMenu->indent - width;
				color = OptionsMenuColor("itemHighlight");
				break;

			case redtext:
				x = 160 - width / 2;
				color = OptionsMenuColor("item");
				break;

			case whitetext:
				x = 160 - width / 2;
				color = OptionsMenuColor("value");
				break;

			case yellowtext:
				x = 160 - width / 2;
				color = OptionsMenuColor("label");
				break;

			case orangetext:
				x = 160 - width / 2;
				color = OptionsMenuColor("warning");
				break;

			case listelement:
				x = CurrentMenu->indent + 14;
				color = OptionsMenuColor("item");
				break;

			default:
				x = CurrentMenu->indent - width;
				color = OptionsMenuColor("item");
				break;
			}
			screen->DrawTextCleanMove(smallFont, color, x, y, item->label);

			switch (item->type)
			{
			case discrete:
			case cdiscrete:
			case svdiscrete:
			{
				int v, vals;

				vals = static_cast<int>(item->b.leftval);
				v = M_FindCurVal(CurrentDiscreteValue(*item), item->e.values, vals);

				if (v == vals)
				{
					screen->DrawTextCleanMove(smallFont, OptionsMenuColor("value"),
					                          CurrentMenu->indent + 14, y, "Unknown");
				}
				else
				{
					int color_num = OptionsMenuColor("value");
					if (item->type == cdiscrete)
						color_num = item->a.cvar->asInt();
					screen->DrawTextCleanMove(smallFont, color_num, CurrentMenu->indent + 14, y, item->e.values[v].name);
				}

			}
			break;

			case nochoice:
				screen->DrawTextCleanMove(smallFont, OptionsMenuColor("label"),
				                          CurrentMenu->indent + 14, y,
										   (item->e.values[static_cast<int>(item->b.leftval)]).name);
				break;

			case slider:
				menu::slider::Draw(CurrentMenu->indent + 8, y, item->b.leftval,
				                   item->c.rightval, item->a.cvar->value(),
				                   item->d.step);
				break;

			case redslider:
			{
				const colorsliderstate_t state = MenuColorSliderState(*item);
				menu::slider::Draw(CurrentMenu->indent + 8, y, 0, 255,
				                   static_cast<float>(state.value), 0.0f,
				                   {.color = state.tint, .showValue = false});
			}
			break;
			case greenslider:
			{
				const colorsliderstate_t state = MenuColorSliderState(*item);
				menu::slider::Draw(CurrentMenu->indent + 8, y, 0, 255,
				                   static_cast<float>(state.value), 0.0f,
				                   {.color = state.tint, .showValue = false});
			}
			break;
			case blueslider:
			{
				const colorsliderstate_t state = MenuColorSliderState(*item);
				menu::slider::Draw(CurrentMenu->indent + 8, y, 0, 255,
				                   static_cast<float>(state.value), 0.0f,
				                   {.color = state.tint, .showValue = false});
			}
			break;

			case control:
			{
				std::string desc = Bindings.GetNameKeys(item->b.key1, item->c.key2);
				screen->DrawTextCleanMove(smallFont, OptionsMenuColor("value"),
				                          CurrentMenu->indent + 14, y, desc.c_str());
			}
			break;

			case mapcontrol:
			{
				std::string desc = AutomapBindings.GetNameKeys(item->b.key1, item->c.key2);
				screen->DrawTextCleanMove(smallFont, OptionsMenuColor("value"),
				                          CurrentMenu->indent + 14, y, desc.c_str());
			}
			break;

			case netdemocontrol:
			{
				std::string desc = NetDemoBindings.GetNameKeys(item->b.key1, item->c.key2);
				screen->DrawTextCleanMove(smallFont, OptionsMenuColor("value"),
				                          CurrentMenu->indent + 14, y, desc.c_str());
			}
			break;

			case bitflag:
			{
				value_t *value;
				const char *str;
				int count = 0;

				if (item->b.leftval)
					value = M_OptionValueSet("NoYes", count);
				else
					value = M_OptionValueSet("YesNo", count);

				if (*item->e.flagint & item->a.flagmask)
					str = value[1].name;
				else
					str = value[0].name;
				screen->DrawTextCleanMove(smallFont, OptionsMenuColor("value"),
				                          CurrentMenu->indent + 14, y, str);
			}
			break;

			case joyactive:
			{
				std::string joyname;

				size_t numjoy = I_GetJoystickCount();

				if(static_cast<size_t>(item->a.cvar->value()) > numjoy)
					item->a.cvar->Set(0.0);

				if(!numjoy)
					joyname = "No device detected";
				else
				{
					joyname = item->a.cvar->str();
					joyname += ": " + I_GetJoystickNameFromIndex(item->a.cvar->asInt());
				}
				screen->DrawTextCleanMove(smallFont, OptionsMenuColor("value"),
				                          CurrentMenu->indent + 14, y, joyname.c_str());
			}
			break;

			case joyaxis:
			{
				screen->DrawTextCleanMove(smallFont, OptionsMenuColor("value"),
				                          CurrentMenu->indent + 14, y, item->a.cvar->cstring());
			}
			break;

			default:
				break;
			}

			if (i == CurrentItem && (indicatorAnimCounter < 6 || WaitingForKey || WaitingForAxis))
			{
				const patch_t* cursor = menu::cursor::Patch();
				screen->DrawPatchCleanWithPalette(cursor, CurrentMenu->indent + 3,
					                                  y + menu::cursor::OffsetY(), palette);
			}
		}
	}

	VisBottom = i - 1;
	CanScrollUp = (CurrentMenu->scrollpos != 0);
	CanScrollDown = (i < CurrentMenu->numitems);

	if (CanScrollUp)
	{
		const patch_t* upPatch = M_MenuConfConfiguredPatch(M_MenuConf().theme.upPatch, "theme.upPatch");
		screen->DrawPatchCleanWithPalette(upPatch, 3, ytop, palette);
	}

	if (CanScrollDown)
	{
		const patch_t* downPatch = M_MenuConfConfiguredPatch(M_MenuConf().theme.downPatch, "theme.downPatch");
		screen->DrawPatchCleanWithPalette(downPatch, 3, 190, palette);
	}
}

void M_OptResponder(const event_t& ev)
{
	if (CurrentMenu == nullptr)
	{
		return;
	}

	int ch = ev.data1;
	int ch2 = ev.data3;
	int mod = ev.mod;
	const char *cmd = Bindings.GetBind(ch).c_str();

	menuitem_t *item = CurrentMenu->items + CurrentItem;

	bool numlock = mod & OMOD_NUM;

	// Waiting on a key press for control binding
	if (WaitingForKey)
	{
		if (ev.type == ev_keydown)
		{
			if (!Key_IsMenuKey(ch))
			{
				if (item->type == control)
					Bindings.ChangeBinding (item->e.command, ch);
				else if (item->type == mapcontrol)
					AutomapBindings.ChangeBinding(item->e.command, ch);
				else if (item->type == netdemocontrol)
					NetDemoBindings.ChangeBinding(item->e.command, ch);
				M_BuildKeyList (CurrentMenu->items, CurrentMenu->numitems);
			}

			configuring_controls = false;
			WaitingForKey = false;
			// FIXME: magic numbers that could break order of settings changes
			CurrentMenu->items[0].label = OldContMessage;
			CurrentMenu->items[0].type = OldContType;
			return;
		}
	}

	// Waiting on an analog axis motion for setting analog control
	if (WaitingForAxis)
	{
		if(ev.type == ev_keydown)
		{
			if (Key_IsCancelKey(ch))
			{
				WaitingForAxis = false;
				// FIXME: magic numbers that could break order of settings changes
				CurrentMenu->items[8].label = OldAxisMessage;
				CurrentMenu->items[8].type = OldAxisType;
			}
		}
		else if (ev.type == ev_joystick)
		{
			if(ev.data1 == 0) // Analog Motion
			{
				// Require the control to be activated to at least the half-way point
				// to make sure we get the one that is intended -- Hyper_Eye
				if( (ev.data3 > (SHRT_MAX / 2)) || (ev.data3 < (SHRT_MIN / 2)) )
				{
					if ((ev.data2 == joy_forwardaxis.asInt()) &&
					    joy_forwardaxis.name() != item->a.cvar->name())
						joy_forwardaxis.Set(item->a.cvar->value());
					else if ((ev.data2 == joy_strafeaxis.asInt()) &&
					         joy_strafeaxis.name() != item->a.cvar->name())
						joy_strafeaxis.Set(item->a.cvar->value());
					else if ((ev.data2 == joy_turnaxis.asInt()) &&
					         joy_turnaxis.name() != item->a.cvar->name())
						joy_turnaxis.Set(item->a.cvar->value());
					else if ((ev.data2 == joy_lookaxis.asInt()) &&
					         joy_lookaxis.name() != item->a.cvar->name())
						joy_lookaxis.Set(item->a.cvar->value());

					item->a.cvar->Set(ev.data2);
					WaitingForAxis = false;
					// FIXME: magic numbers that could break order of settings changes
					CurrentMenu->items[8].label = OldAxisMessage;
					CurrentMenu->items[8].type = OldAxisType;
				}
			}
		}
		return;
	}

	if (item->type == bitflag && flagsvar &&
	    (Key_IsLeftKey(ch, numlock) || Key_IsRightKey(ch, numlock) || Key_IsAcceptKey(ch))
		&& !demoplayback)
	{
			int newflags = *item->e.flagint ^ item->a.flagmask;
			char val[16];

			snprintf (val, 16, "%d", newflags);
			flagsvar->Set (val);
			return;
	}

	if(cmd)
	{
		// Respond to the main menu binding
		if(!strcmp(cmd, "menu_main"))
		{
			M_ClearMenus();
			return;
		}
	}

	// Handle Keys
	{
		if (Key_IsDownKey(ch, numlock))
		{
			int modecol;

			if (item->type == screenres)
			{
				modecol = item->a.selmode;
				item->a.selmode = -1;
			}
			else
			{
				modecol = 0;
			}

			do
			{
				CurrentItem++;
				if (CanScrollDown && CurrentItem == VisBottom)
				{
					CurrentMenu->scrollpos++;
					VisBottom++;
				}
				if (CurrentItem == CurrentMenu->numitems)
				{
					CurrentMenu->scrollpos = 0;
					CurrentItem = 0;
				}
			} while (CurrentMenu->items[CurrentItem].type == redtext ||
				CurrentMenu->items[CurrentItem].type == whitetext ||
				CurrentMenu->items[CurrentItem].type == yellowtext ||
			  CurrentMenu->items[CurrentItem].type == orangetext ||
				(CurrentMenu->items[CurrentItem].type == screenres &&
					!CurrentMenu->items[CurrentItem].b.res1));

			if (CurrentMenu->items[CurrentItem].type == screenres)
				CurrentMenu->items[CurrentItem].a.selmode = modecol;

			PlayCurrentOptionsSound("navigate");
		}
		else if (Key_IsUpKey(ch, numlock))
		{
			int modecol;

			if (item->type == screenres)
			{
				modecol = item->a.selmode;
				item->a.selmode = -1;
			}
			else
			{
				modecol = 0;
			}

			do
			{
				CurrentItem--;
				if (CanScrollUp &&
					CurrentItem == CurrentMenu->scrolltop + CurrentMenu->scrollpos)
				{
					CurrentMenu->scrollpos--;
					if (CurrentMenu->scrollpos < 0)
						CurrentMenu->scrollpos = 0;
				}
				if (CurrentItem < 0)
				{
					CurrentMenu->scrollpos = MAX(0, CurrentMenu->numitems - 22 + CurrentMenu->scrolltop);
					CurrentItem = CurrentMenu->numitems - 1;
				}
			} while (CurrentMenu->items[CurrentItem].type == redtext ||
				CurrentMenu->items[CurrentItem].type == whitetext ||
				CurrentMenu->items[CurrentItem].type == yellowtext ||
			  CurrentMenu->items[CurrentItem].type == orangetext ||
				(CurrentMenu->items[CurrentItem].type == screenres &&
					!CurrentMenu->items[CurrentItem].b.res1));

			if (CurrentMenu->items[CurrentItem].type == screenres)
				CurrentMenu->items[CurrentItem].a.selmode = modecol;

			PlayCurrentOptionsSound("navigate");
		}
		else if (Key_IsPageUpKey(ch, numlock))
		{
			if (CanScrollUp)
			{
				CurrentMenu->scrollpos -= VisBottom - CurrentMenu->scrollpos - CurrentMenu->scrolltop;
				if (CurrentMenu->scrollpos < 0)
				{
					CurrentMenu->scrollpos = 0;
				}
				CurrentItem = CurrentMenu->scrolltop + CurrentMenu->scrollpos + 1;
				while (CurrentMenu->items[CurrentItem].type == redtext ||
					CurrentMenu->items[CurrentItem].type == whitetext ||
					CurrentMenu->items[CurrentItem].type == yellowtext ||
				  CurrentMenu->items[CurrentItem].type == orangetext ||
					(CurrentMenu->items[CurrentItem].type == screenres &&
						!CurrentMenu->items[CurrentItem].b.res1))
				{
					++CurrentItem;
				}
				PlayCurrentOptionsSound("navigate");
			}
		}
		else if (Key_IsPageDownKey(ch, numlock))
		{
			if (CanScrollDown)
			{
				int pagesize = VisBottom - CurrentMenu->scrollpos - CurrentMenu->scrolltop;
				CurrentMenu->scrollpos += pagesize;
				if (CurrentMenu->scrollpos + CurrentMenu->scrolltop + pagesize > CurrentMenu->numitems)
				{
					CurrentMenu->scrollpos = CurrentMenu->numitems - CurrentMenu->scrolltop - pagesize;
				}
				CurrentItem = CurrentMenu->scrolltop + CurrentMenu->scrollpos + 1;
				while (CurrentMenu->items[CurrentItem].type == redtext ||
					CurrentMenu->items[CurrentItem].type == whitetext ||
					CurrentMenu->items[CurrentItem].type == yellowtext ||
				  CurrentMenu->items[CurrentItem].type == orangetext ||
					(CurrentMenu->items[CurrentItem].type == screenres &&
						!CurrentMenu->items[CurrentItem].b.res1))
				{
					++CurrentItem;
				}
				PlayCurrentOptionsSound("navigate");
			}
		}
		else if (Key_IsLeftKey(ch, numlock))
		{
		switch (item->type)
		{
		case slider:
		{
			float newval = menu::slider::Respond(item->a.cvar->value(), item->b.leftval,
			                                     item->c.rightval, item->d.step, -1);

			if (item->e.cfunc)
				item->e.cfunc(item->a.cvar, newval);
			else
				item->a.cvar->Set(newval);
		}
		PlayCurrentOptionsSound("changeValue");
		break;
		case redslider:
		case greenslider:
		case blueslider:
		{
			int part = MenuSliderChannelValue(*item);
			part -= static_cast<int>(item->d.step > 0.0f ? item->d.step : 0x11);
			SetMenuSliderChannelValue(*item, part);
		}
		PlayCurrentOptionsSound("changeValue");
		break;
		case discrete:
		case cdiscrete:
		case svdiscrete:
		{
			int cur;
			int numvals;

			if (item->type == svdiscrete &&
				(multiplayer || demoplayback || netdemo.isPlaying()))
				break;

			numvals = static_cast<int>(item->b.leftval);
			cur = M_FindCurVal(CurrentDiscreteValue(*item), item->e.values, numvals);
			if (--cur < 0)
				cur = numvals - 1;

			item->a.cvar->Set(item->e.values[cur].value);

		}
		PlayCurrentOptionsSound("changeValue");
		break;

		case joyactive:
		{
			size_t numjoy = I_GetJoystickCount();

			if (static_cast<size_t>(item->a.cvar->value()) > numjoy)
				item->a.cvar->Set(0.0);
			else if (static_cast<size_t>(item->a.cvar->value()) > 0)
				item->a.cvar->Set(item->a.cvar->value() - 1);
		}
		PlayCurrentOptionsSound("changeValue");
		break;

		default:
			break;
		}
		}
		else if (Key_IsRightKey(ch, numlock))
		{
		switch (item->type)
		{
		case slider:
		{
			float newval = menu::slider::Respond(item->a.cvar->value(), item->b.leftval,
			                                     item->c.rightval, item->d.step, 1);

			if (item->e.cfunc)
				item->e.cfunc(item->a.cvar, newval);
			else
				item->a.cvar->Set(newval);
		}
		PlayCurrentOptionsSound("changeValue");
		break;
		case redslider:
		case greenslider:
		case blueslider:
		{
			int part = MenuSliderChannelValue(*item);
			part += static_cast<int>(item->d.step > 0.0f ? item->d.step : 0x11);
			SetMenuSliderChannelValue(*item, part);
		}
		PlayCurrentOptionsSound("changeValue");
		break;
		case discrete:
		case cdiscrete:
		case svdiscrete:
		{
			int cur;
			int numvals;

			if (item->type == svdiscrete &&
				(multiplayer || demoplayback || netdemo.isPlaying()))
				break;

			numvals = static_cast<int>(item->b.leftval);
			cur = M_FindCurVal(CurrentDiscreteValue(*item), item->e.values, numvals);
			if (++cur >= numvals)
				cur = 0;

			item->a.cvar->Set(item->e.values[cur].value);

		}
		PlayCurrentOptionsSound("changeValue");
		break;

		case joyactive:
		{
			size_t numjoy = I_GetJoystickCount();

			if (static_cast<size_t>(item->a.cvar->value()) >= numjoy)
				item->a.cvar->Set(0.0);
			else if (static_cast<size_t>(item->a.cvar->value()) < (numjoy - 1))
				item->a.cvar->Set(item->a.cvar->value() + 1);

		}
		PlayCurrentOptionsSound("changeValue");
		break;

		default:
			break;
		}
		}
		else if (Key_IsUnbindKey(ch))
		{
			if (item->type == control)
			{
				Bindings.UnbindACommand (item->e.command);
				item->b.key1 = item->c.key2 = 0;
			}
			else if (item->type == mapcontrol)
			{
				AutomapBindings.UnbindACommand(item->e.command);
				item->b.key1 = item->c.key2 = 0;
			}
			else if (item->type == netdemocontrol)
			{
				NetDemoBindings.UnbindACommand(item->e.command);
				item->b.key1 = item->c.key2 = 0;
			}
		}
		else if (Key_IsAcceptKey(ch))
		{
			if (item->type == more && item->e.mfunc)
			{
				CurrentMenu->lastOn = CurrentItem;
				PlayCurrentOptionsSound("select");
				item->e.mfunc();
			}
			else if (item->type == discrete || item->type == cdiscrete ||
			         item->type == svdiscrete)
			{
				int cur;
				int numvals;

				if (item->type == svdiscrete &&
				    (multiplayer || demoplayback || netdemo.isPlaying()))
					return;

				numvals = static_cast<int>(item->b.leftval);
				cur = M_FindCurVal(CurrentDiscreteValue(*item), item->e.values, numvals);
				if (++cur >= numvals)
					cur = 0;

				item->a.cvar->Set(item->e.values[cur].value);

				PlayCurrentOptionsSound("changeValue");
			}
			else if (item->type == control || item->type == mapcontrol || item->type == netdemocontrol)
			{
				configuring_controls = true;
				WaitingForKey = true;
				OldContMessage = CurrentMenu->items[0].label;
				OldContType = CurrentMenu->items[0].type;
				CurrentMenu->items[0].label =
				    "Press new key for control or ESC to cancel";
				CurrentMenu->items[0].type = redtext;
			}
			else if (item->type == listelement)
			{
				CurrentMenu->lastOn = CurrentItem;
				PlayCurrentOptionsSound("select");
				item->e.lfunc(CurrentItem);
			}
			else if (item->type == joyaxis)
			{
				WaitingForAxis = true;
				// FIXME: magic numbers that could break order of settings changes
				OldAxisMessage = CurrentMenu->items[8].label;
				OldAxisType = CurrentMenu->items[8].type;
				CurrentMenu->items[8].label =
				    "Activate desired analog axis or ESC to cancel";
				CurrentMenu->items[8].type = redtext;
			}
			else if (item->type == screenres)
			{
			}
		}
		else if (Key_IsCancelKey(ch))
		{
			CurrentMenu->lastOn = CurrentItem;
			M_PopMenuStack();
		}
		else
		{
#ifdef GCONSOLE
		if (ev.data3 == 't' || ev.data1 == OKEY_JOY3)
#else
		if (ev.data3 == 't')
#endif
		{
			// Test selected resolution
		}
		}
	}

	if (OptionsActive && CurrentMenu != nullptr && CurrentMenu->refreshfunc)
		(*CurrentMenu->refreshfunc)();
}

//
//	ADDITONAL MENU ACTIONS
//
void M_ChangeMessages (void)
{
	if (show_messages)
	{
		PrintFmt(128, "{}\n", GStrings(MSGOFF));
		show_messages.Set (0.0f);
	}
	else
	{
		PrintFmt(128, "{}\n", GStrings(MSGON));
		show_messages.Set (1.0f);
	}
}

BEGIN_COMMAND (togglemessages)
{
	M_ChangeMessages ();
}
END_COMMAND (togglemessages)

void M_IncrementDisplaySize (float diff)
{
	// changing screenblocks automatically resizes the display
	screenblocks.Set (screenblocks + diff);
}

BEGIN_COMMAND (sizedown)
{
	M_IncrementDisplaySize (-1.0);
	M_PlayMenuSound("changeValue");
}
END_COMMAND (sizedown)

BEGIN_COMMAND (sizeup)
{
	M_IncrementDisplaySize(1.0);
	M_PlayMenuSound("changeValue");
}
END_COMMAND (sizeup)

static void GoToConsole (void)
{
	M_ClearMenus ();
	C_ToggleConsole ();
}

void M_ResetToDefaults (void)
{
	AddCommandString ("unbindall; binddefaults");
	cvar_t::C_SetCVarsToDefaults(CVAR_CLIENTARCHIVE);
	M_IncrementDisplaySize (0.0);
}

void M_ResetToSaved (void)
{
	std::string cmd = "exec " + C_QuoteString(M_GetConfigPath());
	AddCommandString(cmd);
	M_IncrementDisplaySize (0.0);
}

void M_ResetCustomColors (void)
{
	AddCommandString ("M_ResetCustomColors");
}

VERSION_CONTROL (m_options_cpp, "$Id$")
