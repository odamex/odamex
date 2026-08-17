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

#include <array>

#include <algorithm>
#include <cmath>

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
#include "v_video.h"
#include "v_text.h"
#include "w_wad.h"

#include "hu_stuff.h"

#include "m_memio.h"

#include "s_sound.h"
#include "i_music.h"
#include "i_musicsystem.h"


#include "m_misc.h"
#include "cl_demo.h"

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

extern bool				OptionsActive;

extern int				screenSize;
extern short			skullAnimCounter;

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
EXTERN_CVAR (co_zdoomfriendtargeting)

// [Toke - Menu] New Menu Stuff.
void MouseSetup();
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
void JoystickSetup();
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
EXTERN_CVAR(cl_showofflinesprees)
EXTERN_CVAR(cl_showofflinemultikills)

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

void M_ChangeMessages();
void M_SizeDisplay(float diff);

int  M_StringHeight(char *string);
void M_ClearMenus();
namespace
{


bool CanScrollUp;
bool CanScrollDown;
int VisBottom;
} // namespace


EXTERN_CVAR(ui_mouse)

constexpr int MAX_OPT_MOUSE_ROWS = 32;

// The menu lays itself out in the 320x200 "clean" coordinate space.
constexpr int MENU_CENTER_X = 160;
constexpr int MENU_TITLE_Y = 10;

// Video modes are listed in three columns of resolution strings.
constexpr int RESCOLUMN_WIDTH = 104;
constexpr int RESCOLUMN_TEXT_X = 20;
constexpr int RESCOLUMN_CURSOR_X = 8;

// Menu item text indents.
constexpr int MENU_HALFPASTINDENT = 177;
constexpr int MENU_LONGTEXTINDENT = 240;

struct optmouserow_t
{
	int		item;		// index into CurrentMenu->items
	int		y1, y2;		// surface pixel bounds of the row
};
namespace
{


std::array<optmouserow_t, MAX_OPT_MOUSE_ROWS>	OptMouseRows;
int				OptMouseRowCount = 0;

const int		OPT_WHEEL_LINES = 3;

int				OptDragItem = -1;
menu_t*			OptDragMenu = nullptr;
} // namespace


// NOLINTBEGIN(readability-magic-numbers) - the numbers are the data
std::array<value_t, 2> YesNo = {{
	{ .value = 0.0, .name = "No"},
	{ .value = 1.0, .name = "Yes"}
}};

std::array<value_t, 2> NoYes = {{
	{ .value = 0.0, .name = "Yes"},
	{ .value = 1.0, .name = "No"}
}};

std::array<value_t, 2> OnOff = {{
	{ .value = 0.0, .name = "Off"},
	{ .value = 1.0, .name = "On"}
}};

std::array<value_t, 2> HideShow = {{
	{ .value = 0.0, .name = "Hide"},
	{ .value = 1.0, .name = "Show"}
}};

std::array<value_t, 2> OffOn = {{
	{ .value = 0.0, .name = "On"},
	{ .value = 1.0, .name = "Off"}
}};

std::array<value_t, 3> OnOffAuto = {{
	{ .value = 0.0, .name = "Off"},
	{ .value = 1.0, .name = "On"},
	{ .value = 2.0, .name = "Auto"}
}};

std::array<value_t, 2> DemoRestrictions = {{
	{ .value = 0.0, .name = "Restrict"},
	{ .value = 1.0, .name = "Allow"}
}};
namespace
{


std::array<value_t, 2> DoomOrOdamex = {{
	{ .value = 0.0, .name = "Odamex"},
	{ .value = 1.0, .name = "Doom"}
}};
// NOLINTEND(readability-magic-numbers)
} // namespace


menu_t  *CurrentMenu;
int		CurrentItem;
bool configuring_controls = false;
namespace
{

bool	WaitingForKey;
bool	WaitingForAxis;
const char	   *OldContMessage;
itemtype OldContType;
const char	   *OldAxisMessage;
itemtype OldAxisType;

/*=======================================
 *
 * Options Menu
 *
 *=======================================*/

void PlayerSetup();
void CustomizeControls();
void VideoOptions();
void SoundOptions();
void CompatOptions();
void NetworkOptions();
void WeaponOptions();
void GoToConsole();
} // namespace

void Reset2Defaults();
void Reset2Saved();
namespace
{


void SetVidMode();

// NOLINTBEGIN(readability-magic-numbers) - the numbers are the data
std::array<menuitem_t, 19> OptionItems = {{
	{ .type = more, 	.label = "Player Setup",     	.a = {.cvar = nullptr},					.b = {.leftval = 0.0}, .c = {.rightval = 0.0},	.d = {.step = 0.0}, .e = {.mfunc = PlayerSetup}},
	{ .type = more,		.label = "Weapon Preferences",	.a = {.cvar = nullptr},					.b = {.leftval = 0.0}, .c = {.rightval = 0.0},	.d = {.step = 0.0}, .e = {.mfunc = WeaponOptions}},
	{ .type = more,		.label = "Customize Controls",	.a = {.cvar = nullptr},					.b = {.leftval = 0.0}, .c = {.rightval = 0.0},	.d = {.step = 0.0}, .e = {.mfunc = CustomizeControls}},
	{ .type = more,		.label = "Mouse Options",	    .a = {.cvar = nullptr},					.b = {.leftval = 0.0}, .c = {.rightval = 0.0},	.d = {.step = 0.0}, .e = {.mfunc = MouseSetup}},
	{ .type = more,		.label = "Joystick Setup",	    .a = {.cvar = nullptr},					.b = {.leftval = 0.0}, .c = {.rightval = 0.0},	.d = {.step = 0.0}, .e = {.mfunc = JoystickSetup}},
	{ .type = redtext,	.label = " ",					.a = {.cvar = nullptr},					.b = {.leftval = 0.0}, .c = {.rightval = 0.0},	.d = {.step = 0.0}, .e = {.values = nullptr}},
	{ .type = more,		.label = "Compatibility Options",.a = {.cvar = nullptr},					.b = {.leftval = 0.0}, .c = {.rightval = 0.0},	.d = {.step = 0.0}, .e = {.mfunc = CompatOptions}},
	{ .type = more,		.label = "Network Options",		.a = {.cvar = nullptr},					.b = {.leftval = 0.0}, .c = {.rightval = 0.0},	.d = {.step = 0.0}, .e = {.mfunc = NetworkOptions}},
	{ .type = more,		.label = "Sound Options",		.a = {.cvar = nullptr},					.b = {.leftval = 0.0}, .c = {.rightval = 0.0},	.d = {.step = 0.0}, .e = {.mfunc = SoundOptions}},
	{ .type = more,		.label = "Display Options",		.a = {.cvar = nullptr},					.b = {.leftval = 0.0}, .c = {.rightval = 0.0},	.d = {.step = 0.0}, .e = {.mfunc = VideoOptions}},
	{ .type = more,		.label = "Set Video Mode",		.a = {.cvar = nullptr},					.b = {.leftval = 0.0}, .c = {.rightval = 0.0},	.d = {.step = 0.0}, .e = {.mfunc = SetVidMode}},
	{ .type = redtext,	.label = " ",					.a = {.cvar = nullptr},					.b = {.leftval = 0.0}, .c = {.rightval = 0.0},	.d = {.step = 0.0}, .e = {.values = nullptr}},
	{ .type = more,		.label = "Go To Console",		.a = {.cvar = nullptr},					.b = {.leftval = 0.0}, .c = {.rightval = 0.0},	.d = {.step = 0.0}, .e = {.mfunc = GoToConsole}},
	{ .type = redtext,	.label = " ",					.a = {.cvar = nullptr},					.b = {.leftval = 0.0}, .c = {.rightval = 0.0},	.d = {.step = 0.0}, .e = {.values = nullptr}},
	{ .type = discrete,	.label = "Always Run",			.a = {.cvar = &cl_run},				.b = {.leftval = ARRAY_LENGTH(OnOff)}, .c = {.rightval = 0.0},	.d = {.step = 0.0}, .e = {.values = OnOff.data()}},
	{ .type = discrete, .label = "Skip Boot Window",		.a = {.cvar = &i_skipbootwin},		.b = {.leftval = ARRAY_LENGTH(OnOff)}, .c = {.rightval = 0.0},	.d = {.step = 0.0}, .e = {.values = OnOff.data()}},
	{ .type = redtext,	.label = " ",					.a = {.cvar = nullptr},					.b = {.leftval = 0.0}, .c = {.rightval = 0.0},	.d = {.step = 0.0}, .e = {.values = nullptr}},
	{ .type = more,		.label = "Reset to defaults",	.a = {.cvar = nullptr},					.b = {.leftval = 0.0}, .c = {.rightval = 0.0},	.d = {.step = 0.0}, .e = {.mfunc = Reset2Defaults}},
	{ .type = more,		.label = "Reset to last saved",	.a = {.cvar = nullptr},					.b = {.leftval = 0.0}, .c = {.rightval = 0.0},	.d = {.step = 0.0}, .e = {.mfunc = Reset2Saved}}
}};
// NOLINTEND(readability-magic-numbers)
} // namespace


menu_t OptionMenu = {
	"M_OPTTTL",
	0,
	OptionItems.size(),
	MENU_HALFPASTINDENT,
	OptionItems.data(),
	0,
	0,
	nullptr
};
namespace
{


/*=======================================
 *
 * Controls Menu
 *
 *=======================================*/

// NOLINTBEGIN(readability-magic-numbers) - the numbers are the data
// Sized by the initializer: entries here are conditionally compiled,
// so a fixed std::array size would be wrong on some builds.
// NOLINTNEXTLINE(modernize-avoid-c-arrays)
menuitem_t ControlsItems[] = {
#ifdef GCONSOLE
	{ .type = whitetext,.label = "A to change, START to clear", .a = {.cvar = nullptr}, .b = {.leftval = 0.0}, .c = {.rightval = 0.0}, .d = {.step = 0.0}, .e = {.values = nullptr}},
#else
	{ .type = whitetext,.label = "ENTER to change, BACKSPACE to clear", .a = {.cvar = nullptr}, .b = {.leftval = 0.0}, .c = {.rightval = 0.0}, .d = {.step = 0.0}, .e = {.values = nullptr}},
#endif
	{ .type = redtext,	.label = " ",					.a = {.cvar = nullptr},	.b = {.leftval = 0.0}, .c = {.rightval = 0.0}, .d = {.step = 0.0}, .e = {.values = nullptr}},
	{ .type = yellowtext,.label = "Basic Movement",		.a = {.cvar = nullptr},	.b = {.leftval = 0.0}, .c = {.rightval = 0.0}, .d = {.step = 0.0}, .e = {.values = nullptr}},
	{ .type = control,	.label = "Move forward",			.a = {.cvar = nullptr}, .b = {.leftval = 0.0}, .c = {.rightval = 0.0}, .d = {.step = 0.0}, .e = {.command = "+forward"}},
	{ .type = control,	.label = "Move backward",		.a = {.cvar = nullptr}, .b = {.leftval = 0.0}, .c = {.rightval = 0.0}, .d = {.step = 0.0}, .e = {.command = "+back"}},
	{ .type = control,	.label = "Strafe left",			.a = {.cvar = nullptr}, .b = {.leftval = 0.0}, .c = {.rightval = 0.0}, .d = {.step = 0.0}, .e = {.command = "+moveleft"}},
	{ .type = control,	.label = "Strafe right",			.a = {.cvar = nullptr}, .b = {.leftval = 0.0}, .c = {.rightval = 0.0}, .d = {.step = 0.0}, .e = {.command = "+moveright"}},
	{ .type = control,	.label = "Turn left",			.a = {.cvar = nullptr}, .b = {.leftval = 0.0}, .c = {.rightval = 0.0}, .d = {.step = 0.0}, .e = {.command = "+left"}},
	{ .type = control,	.label = "Turn right",			.a = {.cvar = nullptr}, .b = {.leftval = 0.0}, .c = {.rightval = 0.0}, .d = {.step = 0.0}, .e = {.command = "+right"}},
	{ .type = control,	.label = "Run",					.a = {.cvar = nullptr}, .b = {.leftval = 0.0}, .c = {.rightval = 0.0}, .d = {.step = 0.0}, .e = {.command = "+speed"}},
	{ .type = control,	.label = "Always Run",			.a = {.cvar = nullptr}, .b = {.leftval = 0.0}, .c = {.rightval = 0.0}, .d = {.step = 0.0}, .e = {.command = "togglerun"}},
	{ .type = control,	.label = "Strafe",				.a = {.cvar = nullptr}, .b = {.leftval = 0.0}, .c = {.rightval = 0.0}, .d = {.step = 0.0}, .e = {.command = "+strafe"}},
	{ .type = control,	.label = "Jump",					.a = {.cvar = nullptr}, .b = {.leftval = 0.0}, .c = {.rightval = 0.0}, .d = {.step = 0.0}, .e = {.command = "+jump"}},
	{ .type = control,	.label = "Turn 180",				.a = {.cvar = nullptr}, .b = {.leftval = 0.0}, .c = {.rightval = 0.0}, .d = {.step = 0.0}, .e = {.command = "turn180"}},
	{ .type = control,	.label = "Alternate Turn",		.a = {.cvar = nullptr}, .b = {.leftval = 0.0}, .c = {.rightval = 0.0}, .d = {.step = 0.0}, .e = {.command = "+fastturn"}},
	{ .type = redtext,	.label = " ",					.a = {.cvar = nullptr},	.b = {.leftval = 0.0}, .c = {.rightval = 0.0}, .d = {.step = 0.0}, .e = {.values = nullptr}},
	{ .type = yellowtext,.label = "Actions",		        .a = {.cvar = nullptr},	.b = {.leftval = 0.0}, .c = {.rightval = 0.0}, .d = {.step = 0.0}, .e = {.values = nullptr}},
	{ .type = control,	.label = "Fire",					.a = {.cvar = nullptr}, .b = {.leftval = 0.0}, .c = {.rightval = 0.0}, .d = {.step = 0.0}, .e = {.command = "+attack"}},
	{ .type = control,	.label = "Use / Open",			.a = {.cvar = nullptr}, .b = {.leftval = 0.0}, .c = {.rightval = 0.0}, .d = {.step = 0.0}, .e = {.command = "+use"}},
	{ .type = control,	.label = "Next weapon",			.a = {.cvar = nullptr}, .b = {.leftval = 0.0}, .c = {.rightval = 0.0}, .d = {.step = 0.0}, .e = {.command = "weapnext"}},
	{ .type = control,	.label = "Previous weapon",		.a = {.cvar = nullptr}, .b = {.leftval = 0.0}, .c = {.rightval = 0.0}, .d = {.step = 0.0}, .e = {.command = "weapprev"}},
	{ .type = redtext,	.label = " ",					.a = {.cvar = nullptr},	.b = {.leftval = 0.0}, .c = {.rightval = 0.0}, .d = {.step = 0.0}, .e = {.values = nullptr}},
	{ .type = yellowtext,.label = "Weapons",		        .a = {.cvar = nullptr},	.b = {.leftval = 0.0}, .c = {.rightval = 0.0}, .d = {.step = 0.0}, .e = {.values = nullptr}},
	{ .type = control,	.label = "Fist/Chainsaw",		.a = {.cvar = nullptr}, .b = {.leftval = 0.0}, .c = {.rightval = 0.0}, .d = {.step = 0.0}, .e = {.command = "impulse 1"}},
	{ .type = control,	.label = "Pistol",       		.a = {.cvar = nullptr}, .b = {.leftval = 0.0}, .c = {.rightval = 0.0}, .d = {.step = 0.0}, .e = {.command = "impulse 2"}},
	{ .type = control,	.label = "Shotgun/SSG",  		.a = {.cvar = nullptr}, .b = {.leftval = 0.0}, .c = {.rightval = 0.0}, .d = {.step = 0.0}, .e = {.command = "impulse 3"}},
	{ .type = control,	.label = "Chaingun",     		.a = {.cvar = nullptr}, .b = {.leftval = 0.0}, .c = {.rightval = 0.0}, .d = {.step = 0.0}, .e = {.command = "impulse 4"}},
	{ .type = control,	.label = "Rocket Launcher",		.a = {.cvar = nullptr}, .b = {.leftval = 0.0}, .c = {.rightval = 0.0}, .d = {.step = 0.0}, .e = {.command = "impulse 5"}},
	{ .type = control,	.label = "Plasma Rifle",   		.a = {.cvar = nullptr}, .b = {.leftval = 0.0}, .c = {.rightval = 0.0}, .d = {.step = 0.0}, .e = {.command = "impulse 6"}},
	{ .type = control,	.label = "BFG",          		.a = {.cvar = nullptr}, .b = {.leftval = 0.0}, .c = {.rightval = 0.0}, .d = {.step = 0.0}, .e = {.command = "impulse 7"}},
	{ .type = control,	.label = "Chainsaw",     		.a = {.cvar = nullptr}, .b = {.leftval = 0.0}, .c = {.rightval = 0.0}, .d = {.step = 0.0}, .e = {.command = "impulse 8"}},
	{ .type = redtext,	.label = " ",					.a = {.cvar = nullptr},	.b = {.leftval = 0.0}, .c = {.rightval = 0.0}, .d = {.step = 0.0}, .e = {.values = nullptr}},
	{ .type = yellowtext,	.label = "Automap Controls",	.a = {.cvar = nullptr},	.b = {.leftval = 0.0}, .c = {.rightval = 0.0}, .d = {.step = 0.0}, .e = {.values = nullptr}},
	{ .type = control,		.label = "Toggle Automap",	.a = {.cvar = nullptr}, .b = {.leftval = 0.0}, .c = {.rightval = 0.0}, .d = {.step = 0.0}, .e = {.command = "togglemap"}},
	{ .type = mapcontrol,	.label = "Follow Player",	.a = {.cvar = nullptr}, .b = {.leftval = 0.0}, .c = {.rightval = 0.0}, .d = {.step = 0.0}, .e = {.command = "am_togglefollow"}},
	{ .type = mapcontrol,	.label = "Toggle Grid",		.a = {.cvar = nullptr}, .b = {.leftval = 0.0}, .c = {.rightval = 0.0}, .d = {.step = 0.0}, .e = {.command = "am_grid"}},
	{ .type = mapcontrol,	.label = "Add Marker",		.a = {.cvar = nullptr}, .b = {.leftval = 0.0}, .c = {.rightval = 0.0}, .d = {.step = 0.0}, .e = {.command = "am_setmark"}},
	{ .type = mapcontrol,	.label = "Clear Markers",	.a = {.cvar = nullptr}, .b = {.leftval = 0.0}, .c = {.rightval = 0.0}, .d = {.step = 0.0}, .e = {.command = "am_clearmarks"}},
	{ .type = mapcontrol,	.label = "Big Automap",		.a = {.cvar = nullptr}, .b = {.leftval = 0.0}, .c = {.rightval = 0.0}, .d = {.step = 0.0}, .e = {.command = "am_big"}},
	{ .type = mapcontrol,	.label = "Zoom In",			.a = {.cvar = nullptr}, .b = {.leftval = 0.0}, .c = {.rightval = 0.0}, .d = {.step = 0.0}, .e = {.command = "+am_zoomin"}},
	{ .type = mapcontrol,	.label = "Zoom Out",			.a = {.cvar = nullptr}, .b = {.leftval = 0.0}, .c = {.rightval = 0.0}, .d = {.step = 0.0}, .e = {.command = "+am_zoomout"}},
	{ .type = mapcontrol,	.label = "Pan Up",			.a = {.cvar = nullptr}, .b = {.leftval = 0.0}, .c = {.rightval = 0.0}, .d = {.step = 0.0}, .e = {.command = "+am_panup"}},
	{ .type = mapcontrol,	.label = "Pan Down",			.a = {.cvar = nullptr}, .b = {.leftval = 0.0}, .c = {.rightval = 0.0}, .d = {.step = 0.0}, .e = {.command = "+am_pandown"}},
	{ .type = mapcontrol,	.label = "Pan Left",			.a = {.cvar = nullptr}, .b = {.leftval = 0.0}, .c = {.rightval = 0.0}, .d = {.step = 0.0}, .e = {.command = "+am_panleft"}},
	{ .type = mapcontrol,	.label = "Pan Right",		.a = {.cvar = nullptr}, .b = {.leftval = 0.0}, .c = {.rightval = 0.0}, .d = {.step = 0.0}, .e = {.command = "+am_panright"}},
	{ .type = redtext,	.label = " ",					.a = {.cvar = nullptr},	.b = {.leftval = 0.0}, .c = {.rightval = 0.0}, .d = {.step = 0.0}, .e = {.values = nullptr}},
	{ .type = yellowtext,.label = "Advanced Movement",    .a = {.cvar = nullptr},	.b = {.leftval = 0.0}, .c = {.rightval = 0.0}, .d = {.step = 0.0}, .e = {.values = nullptr}},
	{ .type = control,	.label = "Fly / Swim up",		.a = {.cvar = nullptr},	.b = {.leftval = 0.0}, .c = {.rightval = 0.0}, .d = {.step = 0.0}, .e = {.command = "+moveup"}},
	{ .type = control,	.label = "Fly / Swim down",		.a = {.cvar = nullptr},	.b = {.leftval = 0.0}, .c = {.rightval = 0.0}, .d = {.step = 0.0}, .e = {.command = "+movedown"}},
	{ .type = control,	.label = "Toggle flying",		.a = {.cvar = nullptr},	.b = {.leftval = 0.0}, .c = {.rightval = 0.0}, .d = {.step = 0.0}, .e = {.command = "fly"}},
	{ .type = control,	.label = "Look up",				.a = {.cvar = nullptr}, .b = {.leftval = 0.0}, .c = {.rightval = 0.0}, .d = {.step = 0.0}, .e = {.command = "+lookup"}},
	{ .type = control,	.label = "Look down",			.a = {.cvar = nullptr}, .b = {.leftval = 0.0}, .c = {.rightval = 0.0}, .d = {.step = 0.0}, .e = {.command = "+lookdown"}},
	{ .type = control,	.label = "Center view",			.a = {.cvar = nullptr}, .b = {.leftval = 0.0}, .c = {.rightval = 0.0}, .d = {.step = 0.0}, .e = {.command = "centerview"}},
	{ .type = control,	.label = "Mouse look",			.a = {.cvar = nullptr}, .b = {.leftval = 0.0}, .c = {.rightval = 0.0}, .d = {.step = 0.0}, .e = {.command = "+mlook"}},
	{ .type = control,	.label = "Keyboard look",		.a = {.cvar = nullptr}, .b = {.leftval = 0.0}, .c = {.rightval = 0.0}, .d = {.step = 0.0}, .e = {.command = "+klook"}},
	{ .type = redtext,	.label = " ",					.a = {.cvar = nullptr},	.b = {.leftval = 0.0}, .c = {.rightval = 0.0}, .d = {.step = 0.0}, .e = {.values = nullptr}},
	{ .type = yellowtext,.label = "Multiplayer",		    .a = {.cvar = nullptr},	.b = {.leftval = 0.0}, .c = {.rightval = 0.0}, .d = {.step = 0.0}, .e = {.values = nullptr}},
	{ .type = control,	.label = "Say",					.a = {.cvar = nullptr}, .b = {.leftval = 0.0}, .c = {.rightval = 0.0}, .d = {.step = 0.0}, .e = {.command = "messagemode"}},
	{ .type = control,	.label = "Team say",				.a = {.cvar = nullptr}, .b = {.leftval = 0.0}, .c = {.rightval = 0.0}, .d = {.step = 0.0}, .e = {.command = "messagemode2"}},
	{ .type = control,	.label = "Ready",				.a = {.cvar = nullptr}, .b = {.leftval = 0.0}, .c = {.rightval = 0.0}, .d = {.step = 0.0}, .e = {.command = "ready"}},
	{ .type = control,	.label = "Change teams",			.a = {.cvar = nullptr}, .b = {.leftval = 0.0}, .c = {.rightval = 0.0}, .d = {.step = 0.0}, .e = {.command = "changeteams"}},
	{ .type = control,	.label = "Spectate",				.a = {.cvar = nullptr}, .b = {.leftval = 0.0}, .c = {.rightval = 0.0}, .d = {.step = 0.0}, .e = {.command = "spectate"}},
	{ .type = control,	.label = "Coop Spy",				.a = {.cvar = nullptr}, .b = {.leftval = 0.0}, .c = {.rightval = 0.0}, .d = {.step = 0.0}, .e = {.command = "spynext"}},
	{ .type = control,	.label = "Show Scoreboard",		.a = {.cvar = nullptr}, .b = {.leftval = 0.0}, .c = {.rightval = 0.0}, .d = {.step = 0.0}, .e = {.command = "+showscores"}},
	{ .type = control,	.label = "Vote Yes", .a = {.cvar = nullptr}, .b = {.leftval = 0.0}, .c = {.rightval = 0.0}, .d = {.step = 0.0}, .e = {.command = "vote_yes"}},
	{ .type = control,	.label = "Vote No", .a = {.cvar = nullptr}, .b = {.leftval = 0.0}, .c = {.rightval = 0.0}, .d = {.step = 0.0}, .e = {.command = "vote_no"}},
	{ .type = redtext,	.label = " ",					.a = {.cvar = nullptr},	.b = {.leftval = 0.0}, .c = {.rightval = 0.0}, .d = {.step = 0.0}, .e = {.values = nullptr}},
	{ .type = yellowtext,.label = "Menus",				.a = {.cvar = nullptr},	.b = {.leftval = 0.0}, .c = {.rightval = 0.0}, .d = {.step = 0.0}, .e = {.values = nullptr}},
	{ .type = control,  .label = "Main menu",			.a = {.cvar = nullptr}, .b = {.leftval = 0.0}, .c = {.rightval = 0.0}, .d = {.step = 0.0}, .e = {.command = "menu_main"}},
	{ .type = control,	.label = "Help menu",			.a = {.cvar = nullptr}, .b = {.leftval = 0.0}, .c = {.rightval = 0.0}, .d = {.step = 0.0}, .e = {.command = "menu_help"}},
	{ .type = control,	.label = "Save menu",			.a = {.cvar = nullptr}, .b = {.leftval = 0.0}, .c = {.rightval = 0.0}, .d = {.step = 0.0}, .e = {.command = "menu_save"}},
	{ .type = control,	.label = "Load menu",			.a = {.cvar = nullptr}, .b = {.leftval = 0.0}, .c = {.rightval = 0.0}, .d = {.step = 0.0}, .e = {.command = "menu_load"}},
	{ .type = control,	.label = "Options menu",			.a = {.cvar = nullptr}, .b = {.leftval = 0.0}, .c = {.rightval = 0.0}, .d = {.step = 0.0}, .e = {.command = "menu_options"}},
	{ .type = control,	.label = "Display options",	    .a = {.cvar = nullptr}, .b = {.leftval = 0.0}, .c = {.rightval = 0.0}, .d = {.step = 0.0}, .e = {.command = "menu_display"}},
	{ .type = control,	.label = "Player setup menu",	.a = {.cvar = nullptr}, .b = {.leftval = 0.0}, .c = {.rightval = 0.0}, .d = {.step = 0.0}, .e = {.command = "menu_player"}},
	{ .type = control,	.label = "Configure controls",	.a = {.cvar = nullptr}, .b = {.leftval = 0.0}, .c = {.rightval = 0.0}, .d = {.step = 0.0}, .e = {.command = "menu_keys"}},
	{ .type = control,	.label = "Change resolution",	.a = {.cvar = nullptr}, .b = {.leftval = 0.0}, .c = {.rightval = 0.0}, .d = {.step = 0.0}, .e = {.command = "menu_video"}},
	{ .type = redtext,	.label = " ",					.a = {.cvar = nullptr},	.b = {.leftval = 0.0}, .c = {.rightval = 0.0}, .d = {.step = 0.0}, .e = {.values = nullptr}},
	{ .type = yellowtext,	.label = "Netdemo Controls",	.a = {.cvar = nullptr},	.b = {.leftval = 0.0}, .c = {.rightval = 0.0}, .d = {.step = 0.0}, .e = {.values = nullptr}},
	{ .type = netdemocontrol,.label = "Pause Netdemo",	.a = {.cvar = nullptr}, .b = {.leftval = 0.0}, .c = {.rightval = 0.0}, .d = {.step = 0.0}, .e = {.command = "netpause"}},
	{ .type = netdemocontrol, .label = "Fast Forward", .a = {.cvar = nullptr}, .b = {.leftval = 0.0}, .c = {.rightval = 0.0}, .d = {.step = 0.0}, .e = {.command = "netff"}},
	{ .type = netdemocontrol, .label = "Rewind", .a = {.cvar = nullptr}, .b = {.leftval = 0.0}, .c = {.rightval = 0.0}, .d = {.step = 0.0}, .e = {.command = "netrew"}},
	{ .type = netdemocontrol, .label = "Next map", .a = {.cvar = nullptr}, .b = {.leftval = 0.0}, .c = {.rightval = 0.0}, .d = {.step = 0.0}, .e = {.command = "netnextmap"}},
	{ .type = netdemocontrol,	.label = "Previous map",	.a = {.cvar = nullptr}, .b = {.leftval = 0.0}, .c = {.rightval = 0.0}, .d = {.step = 0.0}, .e = {.command = "netprevmap"}},
	{ .type = redtext,	.label = " ",					.a = {.cvar = nullptr},	.b = {.leftval = 0.0}, .c = {.rightval = 0.0}, .d = {.step = 0.0}, .e = {.values = nullptr}},
	{ .type = yellowtext,.label = "Other",				.a = {.cvar = nullptr},	.b = {.leftval = 0.0}, .c = {.rightval = 0.0}, .d = {.step = 0.0}, .e = {.values = nullptr}},
	{ .type = control,	.label = "Increase screen size",	.a = {.cvar = nullptr}, .b = {.leftval = 0.0}, .c = {.rightval = 0.0}, .d = {.step = 0.0}, .e = {.command = "sizeup"}},
	{ .type = control,	.label = "Reduce screen size",	.a = {.cvar = nullptr}, .b = {.leftval = 0.0}, .c = {.rightval = 0.0}, .d = {.step = 0.0}, .e = {.command = "sizedown"}},
	{ .type = control,	.label = "Chasecam",				.a = {.cvar = nullptr}, .b = {.leftval = 0.0}, .c = {.rightval = 0.0}, .d = {.step = 0.0}, .e = {.command = "chase"}},
	{ .type = control,	.label = "Screenshot",			.a = {.cvar = nullptr}, .b = {.leftval = 0.0}, .c = {.rightval = 0.0}, .d = {.step = 0.0}, .e = {.command = "screenshot"}},
	{ .type = control,  .label = "Open console",			.a = {.cvar = nullptr}, .b = {.leftval = 0.0}, .c = {.rightval = 0.0}, .d = {.step = 0.0}, .e = {.command = "toggleconsole"}},
	{ .type = control,  .label = "End current game",     .a = {.cvar = nullptr}, .b = {.leftval = 0.0}, .c = {.rightval = 0.0}, .d = {.step = 0.0}, .e = {.command = "menu_endgame"}},
	{ .type = control,  .label = "Quit Odamex",	        .a = {.cvar = nullptr}, .b = {.leftval = 0.0}, .c = {.rightval = 0.0}, .d = {.step = 0.0}, .e = {.command = "menu_quit"}}

};
// NOLINTEND(readability-magic-numbers)
} // namespace


menu_t ControlsMenu = {
	"M_CONTRO",
	3,
	ARRAY_LENGTH(ControlsItems),
	0,
	ControlsItems,
	2,
	0,
	nullptr
};

// -------------------------------------------------------
//
//	[Toke] New [ Mouse Menu ]
//
// -------------------------------------------------------

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
namespace
{



// NOLINTBEGIN(readability-magic-numbers) - the numbers are the data
std::array<menuitem_t, 14> MouseItems = {{
	{ .type = slider,	.label = "Overall Sensitivity", .a = {.cvar = &mouse_sensitivity},	.b = {.leftval = 0.05},	.c = {.rightval = 2.5},		.d = {.step = 0.05},		.e = {.values = nullptr}},
	{ .type = slider,	.label = "Freelook Sensitivity", .a = {.cvar = &m_pitch},			.b = {.leftval = 0.05},	.c = {.rightval = 2.5},		.d = {.step = 0.05},		.e = {.values = nullptr}},

	{ .type = redtext,	.label = " ", .a = {.cvar = nullptr},				.b = {.leftval = 0.0},	.c = {.rightval = 0.0},		.d = {.step = 0.0},		.e = {.values = nullptr}},
	{ .type = discrete,	.label = "Always FreeLook", .a = {.cvar = &cl_mouselook},		.b = {.leftval = ARRAY_LENGTH(OnOff)},	.c = {.rightval = 0.0},		.d = {.step = 0.0},		.e = {.values = OnOff.data()}},
	{ .type = discrete,	.label = "Invert Mouse", .a = {.cvar = &invertmouse},		.b = {.leftval = ARRAY_LENGTH(OnOff)},	.c = {.rightval = 0.0},		.d = {.step = 0.0},		.e = {.values = OnOff.data()}},
	{ .type = discrete, .label = "Auto SR50 on Strafe", .a = {.cvar = &in_autosr50},		.b = {.leftval = ARRAY_LENGTH(OnOff)},	.c = {.rightval = 0.0},		.d = {.step = 0.0},		.e = {.values = OnOff.data()}}, // [AM] Does not belong here
	{ .type = discrete, .label = "Lookspring", .a = {.cvar = &lookspring},		.b = {.leftval = ARRAY_LENGTH(OnOff)},	.c = {.rightval = 0.0},		.d = {.step = 0.0},		.e = {.values = OnOff.data()}},
	{ .type = redtext,	.label = " ", .a = {.cvar = nullptr},				.b = {.leftval = 0.0},	.c = {.rightval = 0.0},		.d = {.step = 0.0},		.e = {.values = nullptr}},
	{ .type = discrete,	.label = "Horizontal Movement", .a = {.cvar = &lookstrafe},		.b = {.leftval = ARRAY_LENGTH(OnOff)},	.c = {.rightval = 0.0},		.d = {.step = 0.0},		.e = {.values = OnOff.data()}},
	{ .type = discrete,	.label = "Vertical Movement", .a = {.cvar = &novert},			.b = {.leftval = ARRAY_LENGTH(OffOn)},	.c = {.rightval = 0.0},		.d = {.step = 0.0},		.e = {.values = OffOn.data()}},
	{ .type = slider,	.label = "Horizontal Movement Speed", .a = {.cvar = &m_side},			.b = {.leftval = 0.0},	.c = {.rightval = 15},		.d = {.step = 0.5},		.e = {.values = nullptr}},
	{ .type = slider,	.label = "Vertical Movement Speed", .a = {.cvar = &m_forward},			.b = {.leftval = 0.0},	.c = {.rightval = 15},		.d = {.step = 0.5},		.e = {.values = nullptr}},
	{ .type = redtext,	.label = " ", .a = {.cvar = nullptr},				.b = {.leftval = 0.0},	.c = {.rightval = 0.0},		.d = {.step = 0.0},		.e = {.values = nullptr}},
	{ .type = more,		.label = "Reset mouse to defaults", .a = {.cvar = nullptr},				.b = {.leftval = 0.0},	.c = {.rightval = 0.0},		.d = {.step = 0.0},		.e = {.mfunc = M_ResetMouseValues}},
}};
// NOLINTEND(readability-magic-numbers)
} // namespace



menu_t MouseMenu = {
	"M_MOUSET",
	0,
	MouseItems.size(),
	MENU_HALFPASTINDENT,
	MouseItems.data(),
	0,
	0,
	nullptr
};
namespace
{



/*=======================================
 *
 * Joystick Menu
 *
 *=======================================*/

// NOLINTBEGIN(readability-magic-numbers) - the numbers are the data
std::array<menuitem_t, 11> JoystickItems = {{
	{ .type = discrete,	.label = "Use Joystick", .a = {.cvar = &use_joystick},		.b = {.leftval = ARRAY_LENGTH(OnOff)},		.c = {.rightval = 0.0},		.d = {.step = 0.0},		.e = {.values = OnOff.data()}},
	{ .type = redtext,	.label = " ", .a = {.cvar = nullptr},				.b = {.leftval = 0.0},		.c = {.rightval = 0.0},		.d = {.step = 0.0},		.e = {.values = nullptr}},
	{ .type = joyactive,	.label = "Active Joystick", .a = {.cvar = &joy_active},		.b = {.leftval = 0.0},		.c = {.rightval = 0.0},		.d = {.step = 0.0},		.e = {.values = nullptr}},
	{ .type = redtext,	.label = " ", .a = {.cvar = nullptr},				.b = {.leftval = 0.0},		.c = {.rightval = 0.0},		.d = {.step = 0.0},		.e = {.values = nullptr}},
	{ .type = discrete,	.label = "Always FreeLook", .a = {.cvar = &joy_freelook},		.b = {.leftval = ARRAY_LENGTH(OnOff)},		.c = {.rightval = 0.0},		.d = {.step = 0.0},		.e = {.values = OnOff.data()}},
	{ .type = discrete,	.label = "Invert Look Axis", .a = {.cvar = &joy_invert},		.b = {.leftval = ARRAY_LENGTH(OnOff)},		.c = {.rightval = 0.0},		.d = {.step = 0.0},		.e = {.values = OnOff.data()}},
	{ .type = redtext,	.label = " ", .a = {.cvar = nullptr},				.b = {.leftval = 0.0},		.c = {.rightval = 0.0},		.d = {.step = 0.0},		.e = {.values = nullptr}},
	{ .type = whitetext,	.label = "Sensitivity Settings", .a = {.cvar = nullptr}, 				.b = {.leftval = 0.0}, 		.c = {.rightval = 0.0}, 		.d = {.step = 0.0}, 		.e = {.values = nullptr}},
	{ .type = slider,	.label = "Turn Sensitivity", .a = {.cvar = &joy_sensitivity},	.b = {.leftval = 1.0},		.c = {.rightval = 30.0},		.d = {.step = 1.0},		.e = {.values = nullptr}},
	{ .type = slider,	.label = "Alt. Turn Sensitivity", .a = {.cvar = &joy_fastsensitivity},	.b = {.leftval = 1.0},		.c = {.rightval = 30.0},		.d = {.step = 1.0},		.e = {.values = nullptr}},
	{ .type = slider,	.label = "Joystick Deadzone", .a = {.cvar = &joy_deadzone},		.b = {.leftval = 0.0},		.c = {.rightval = 0.75},		.d = {.step = 0.05},		.e = {.values = nullptr}},
}};
// NOLINTEND(readability-magic-numbers)
} // namespace


menu_t JoystickMenu = {
	"M_JOYSTK",
	0,
	JoystickItems.size(),
	MENU_HALFPASTINDENT,
	JoystickItems.data(),
	0,
	0,
	nullptr
};
namespace
{


 /*=======================================
  *
  * Sound Menu [Ralphis]
  *
  *=======================================*/

// NOLINTBEGIN(readability-magic-numbers) - the numbers are the data
// Sized by the initializer: entries here are conditionally compiled,
// so a fixed std::array size would be wrong on some builds.
// NOLINTNEXTLINE(modernize-avoid-c-arrays)
value_t MusSys[] = {
	{ .value = MS_AUTO,		.name = "Auto"},
	#ifndef _WIN32
	{ .value = MS_SDLMIXER,	.name = "SDL Mixer"},
	#endif
	{ .value = MS_LIBADLMIDI,.name = "libADLMIDI (OPL3 FM)"},
	#ifdef OSX
	{ .value = MS_AUDIOUNIT,	.name = "AudioUnit"},
	#endif	// OSX
	#ifdef PORTMIDI
	{ .value = MS_PORTMIDI,	.name = "PortMidi"},
	#endif	// PORTMIDI
};

std::array<value_t, 4> MidiReset = {{
	{ .value = 0.0,			.name = "None"},
	{ .value = 1.0,			.name = "GM"},
	{ .value = 2.0,			.name = "GS"},
	{ .value = 3.0,			.name = "XG"}
}};

std::array<value_t, 3> OplCore = {{
	{ .value = 0.0,			.name = "Fast (Dosbox)"},
	{ .value = 1.0,			.name = "Balanced (Nuked-Fast 1.8)"},
	{ .value = 2.0,			.name = "Accurate (Nuked 1.8)"}
}};

std::array<value_t, 3> OplBank = {{
	{ .value = 0.0,			.name = "Doom"},
	{ .value = 1.0,			.name = "Doom II"},
	{ .value = 2.0,			.name = "DMXOPL3"}
}};

std::array<value_t, 3> VoxType = {{
	{ .value = 0.0,			.name = "Off"},
	{ .value = 1.0,			.name = "Team Colors"},
	{ .value = 2.0,			.name = "Possessive"}
}};

std::array<value_t, 3> ChatSndType = {{
	{ .value = 0.0,			.name = "Disabled"},
	{ .value = 1.0,			.name = "Enabled"},
	{ .value = 2.0,			.name = "Teamchat only"}
}};
// NOLINTEND(readability-magic-numbers)

void AdvMidiOptions();
void LibAdlMidiOptions();
} // namespace


EXTERN_CVAR(cl_chatsounds)

// NOLINTBEGIN(readability-magic-numbers) - the numbers are the data
// Sized by the initializer: entries here are conditionally compiled,
// so a fixed std::array size would be wrong on some builds.
// NOLINTNEXTLINE(modernize-avoid-c-arrays)
static menuitem_t AdvMidiItems[] = {
	{ .type = redtext, .label = " ", .a = {.cvar = nullptr}, .b = {.leftval = 0.0}, .c = {.rightval = 0.0}, .d = {.step = 0.0}, .e = {.values = nullptr}},
	{ .type = yellowtext, .label = "Advanced MIDI Options", .a = {.cvar = nullptr}, .b = {.leftval = 0.0}, .c = {.rightval = 0.0}, .d = {.step = 0.0}, .e = {.values = nullptr}},
	{ .type = redtext, .label = " ", .a = {.cvar = nullptr}, .b = {.leftval = 0.0}, .c = {.rightval = 0.0}, .d = {.step = 0.0}, .e = {.values = nullptr}},
	{ .type = discrete, .label = "MIDI Instrument Fallback", .a = {.cvar = &snd_midifallback}, .b = {.leftval = ARRAY_LENGTH(OnOff)}, .c = {.rightval = 0.0}, .d = {.step = 0.0}, .e = {.values = OnOff.data()}},
	{ .type = slider, .label = "MIDI Reset Delay (ms)", .a = {.cvar = &snd_mididelay}, .b = {.leftval = 0.0}, .c = {.rightval = 2000.0}, .d = {.step = 50.0}, .e = {.values = nullptr}},
	#ifdef PORTMIDI
	{ .type = redtext, .label = " ", .a = {.cvar = nullptr}, .b = {.leftval = 0.0}, .c = {.rightval = 0.0}, .d = {.step = 0.0}, .e = {.values = nullptr}},
	{ .type = yellowtext, .label = "PortMidi Options", .a = {.cvar = nullptr}, .b = {.leftval = 0.0}, .c = {.rightval = 0.0}, .d = {.step = 0.0}, .e = {.values = nullptr}},
	{ .type = redtext, .label = " ", .a = {.cvar = nullptr}, .b = {.leftval = 0.0}, .c = {.rightval = 0.0}, .d = {.step = 0.0}, .e = {.values = nullptr}},
	{ .type = discrete, .label = "MIDI Reset", .a = {.cvar = &snd_midireset}, .b = {.leftval = ARRAY_LENGTH(MidiReset)}, .c = {.rightval = 0.0}, .d = {.step = 0.0}, .e = {.values = MidiReset.data()}},
	{ .type = discrete, .label = "Read MIDI SysEx", .a = {.cvar = &snd_midisysex}, .b = {.leftval = ARRAY_LENGTH(OnOff)}, .c = {.rightval = 0.0}, .d = {.step = 0.0}, .e = {.values = OnOff.data()}},
	#endif
	{ .type = redtext, .label = " ", .a = {.cvar = nullptr}, .b = {.leftval = 0.0}, .c = {.rightval = 0.0}, .d = {.step = 0.0}, .e = {.values = nullptr}},
	{ .type = redtext, .label = " ", .a = {.cvar = nullptr}, .b = {.leftval = 0.0}, .c = {.rightval = 0.0}, .d = {.step = 0.0}, .e = {.values = nullptr}},
	{ .type = redtext, .label = " ", .a = {.cvar = nullptr}, .b = {.leftval = 0.0}, .c = {.rightval = 0.0}, .d = {.step = 0.0}, .e = {.values = nullptr}},
	{ .type = yellowtext, .label = "! ! ! NOTICE ! ! !", .a = {.cvar = nullptr}, .b = {.leftval = 0.0}, .c = {.rightval = 0.0}, .d = {.step = 0.0}, .e = {.values = nullptr}},
	{ .type = orangetext, .label = "Modifying these settings may cause", .a = {.cvar = nullptr},.b = {.leftval = 0.0}, .c = {.rightval = 0.0}, .d = {.step = 0.0}, .e = {.values = nullptr}},
	{ .type = orangetext, .label = "unwanted behavior during MIDI playback!", .a = {.cvar = nullptr}, .b = {.leftval = 0.0}, .c = {.rightval = 0.0}, .d = {.step = 0.0}, .e = {.values = nullptr}},
};
namespace
{


 std::array<menuitem_t, 7> LibAdlMidiItems = {{
	{ .type = redtext, .label = " ", .a = {.cvar = nullptr}, .b = {.leftval = 0.0}, .c = {.rightval = 0.0}, .d = {.step = 0.0}, .e = {.values = nullptr}},
	{ .type = yellowtext, .label = "OPL FM Synth Options", .a = {.cvar = nullptr}, .b = {.leftval = 0.0}, .c = {.rightval = 0.0}, .d = {.step = 0.0}, .e = {.values = nullptr}},
	{ .type = redtext, .label = " ", .a = {.cvar = nullptr}, .b = {.leftval = 0.0}, .c = {.rightval = 0.0}, .d = {.step = 0.0}, .e = {.values = nullptr}},
	{ .type = discrete, .label = "OPL quality", .a = {.cvar = &snd_oplcore}, .b = {.leftval = ARRAY_LENGTH(OplCore)}, .c = {.rightval = 0.0}, .d = {.step = 0.0}, .e = {.values = OplCore.data()}},
	{ .type = discrete, .label = "Full OPL panning", .a = {.cvar = &snd_oplpan}, .b = {.leftval = ARRAY_LENGTH(OnOff)}, .c = {.rightval = 0.0}, .d = {.step = 0.0}, .e = {.values = OnOff.data()}},
	{ .type = slider, .label = "# of OPL chips", .a = {.cvar = &snd_oplchips}, .b = {.leftval = 1.0}, .c = {.rightval = 8.0}, .d = {.step = 1.0}, .e = {.values = nullptr}},
	{ .type = discrete, .label = "OPL instruments", .a = {.cvar = &snd_oplbank}, .b = {.leftval = ARRAY_LENGTH(OplBank)}, .c = {.rightval = 0.0}, .d = {.step = 0.0}, .e = {.values = OplBank.data()}},
}};

std::array<menuitem_t, 21> SoundItems = {{
	{ .type = redtext,   .label = " ", .a = {.cvar = nullptr},                .b = {.leftval = 0.0},        .c = {.rightval = 0.0}, .d = {.step = 0.0},      .e = {.values = nullptr}},
	{ .type = yellowtext,   .label = "Sound Levels", .a = {.cvar = nullptr},                .b = {.leftval = 0.0},        .c = {.rightval = 0.0}, .d = {.step = 0.0},      .e = {.values = nullptr}},
	{ .type = slider,	.label = "Music Volume", .a = {.cvar = &snd_musicvolume},    .b = {.leftval = 0.0},        .c = {.rightval = 1.0}, .d = {.step = 0.015625}, .e = {.values = nullptr}},
	{ .type = slider,	.label = "Sound Volume", .a = {.cvar = &snd_sfxvolume},      .b = {.leftval = 0.0},        .c = {.rightval = 1.0}, .d = {.step = 0.015625}, .e = {.values = nullptr}},
	{ .type = slider,	.label = "Announcer Volume", .a = {.cvar = &snd_announcervolume},.b = {.leftval = 0.0},        .c = {.rightval = 1.0}, .d = {.step = 0.015625}, .e = {.values = nullptr}},
	{ .type = discrete,   .label = "Stereo Switch", .a = {.cvar = &snd_crossover},      .b = {.leftval = ARRAY_LENGTH(OnOff)},        .c = {.rightval = 0.0}, .d = {.step = 0.0},      .e = {.values = OnOff.data()}},
	{ .type = redtext,	.label = " ", .a = {.cvar = nullptr},                .b = {.leftval = 0.0},        .c = {.rightval = 0.0}, .d = {.step = 0.0},      .e = {.values = nullptr}},
	{ .type = yellowtext,   .label = "Music Options", .a = {.cvar = nullptr},                .b = {.leftval = 0.0},        .c = {.rightval = 0.0}, .d = {.step = 0.0},      .e = {.values = nullptr}},
	{ .type = discrete,   .label = "Midi Synth", .a = {.cvar = &snd_musicsystem},    .b = {.leftval = ARRAY_LENGTH(MusSys)}, .c = {.rightval = 0.0}, .d = {.step = 0.0},      .e = {.values = MusSys}},
	{ .type = discrete,   .label = "Disable Music", .a = {.cvar = &snd_nomusic},        .b = {.leftval = ARRAY_LENGTH(YesNo)},        .c = {.rightval = 0.0}, .d = {.step = 0.0},      .e = {.values = YesNo.data()}},
	{ .type = redtext,	.label = " ", .a = {.cvar = nullptr},                .b = {.leftval = 0.0},        .c = {.rightval = 0.0}, .d = {.step = 0.0},      .e = {.values = nullptr}},
	{ .type = more,   .label = "OPL FM Synth Options", .a = {.cvar = nullptr},                .b = {.leftval = 0.0},        .c = {.rightval = 0.0}, .d = {.step = 0.0},      .e = {.mfunc = LibAdlMidiOptions}},
	{ .type = more,   .label = "Advanced MIDI Options", .a = {.cvar = nullptr},                .b = {.leftval = 0.0},        .c = {.rightval = 0.0}, .d = {.step = 0.0},      .e = {.mfunc = AdvMidiOptions}},
	{ .type = redtext,   .label = " ", .a = {.cvar = nullptr},                .b = {.leftval = 0.0},        .c = {.rightval = 0.0}, .d = {.step = 0.0},      .e = {.values = nullptr}},
	{ .type = yellowtext,   .label = "Sound Options", .a = {.cvar = nullptr},                .b = {.leftval = 0.0},        .c = {.rightval = 0.0}, .d = {.step = 0.0},      .e = {.values = nullptr}},
	{ .type = discrete,   .label = "Game SFX", .a = {.cvar = &snd_gamesfx},        .b = {.leftval = ARRAY_LENGTH(OnOff)},        .c = {.rightval = 0.0}, .d = {.step = 0.0},      .e = {.values = OnOff.data()}},
	{ .type = discrete,   .label = "Announcer Type", .a = {.cvar = &snd_voxtype},        .b = {.leftval = ARRAY_LENGTH(VoxType)},        .c = {.rightval = 0.0}, .d = {.step = 0.0},      .e = {.values = VoxType.data()}},
	{ .type = discrete,   .label = "Player Connect Alert", .a = {.cvar = &cl_connectalert},    .b = {.leftval = ARRAY_LENGTH(OnOff)},        .c = {.rightval = 0.0}, .d = {.step = 0.0},      .e = {.values = OnOff.data()}},
	{ .type = discrete,   .label = "Player Disconnect Alert", .a = {.cvar = &cl_disconnectalert}, .b = {.leftval = ARRAY_LENGTH(OnOff)},        .c = {.rightval = 0.0}, .d = {.step = 0.0},      .e = {.values = OnOff.data()}},
	{ .type = discrete,   .label = "Chat sounds", .a = {.cvar = &cl_chatsounds},      .b = {.leftval = ARRAY_LENGTH(ChatSndType)},        .c = {.rightval = 0.0}, .d = {.step = 0.0},      .e = {.values = ChatSndType.data()}},
	{ .type = discrete,   .label = "Voting Sounds", .a = {.cvar = &snd_votesfx},		.b = {.leftval = ARRAY_LENGTH(OnOff)},        .c = {.rightval = 0.0}, .d = {.step = 0.0},	     .e = {.values = OnOff.data()}},
 }};
// NOLINTEND(readability-magic-numbers)
} // namespace


menu_t AdvMidiMenu = {
	"M_SOUND",
	3,
	ARRAY_LENGTH(AdvMidiItems),
	MENU_HALFPASTINDENT,
	AdvMidiItems,
	0,
	0,
	nullptr
};

menu_t LibAdlMidiMenu = {
	"M_SOUND",
	3,
	LibAdlMidiItems.size(),
	MENU_HALFPASTINDENT,
	LibAdlMidiItems.data(),
	0,
	0,
	nullptr
};

menu_t SoundMenu = {
	"M_SOUND",
	2,
	SoundItems.size(),
	MENU_HALFPASTINDENT,
	SoundItems.data(),
	0,
	0,
	nullptr
};
namespace
{



/*=======================================
 *
 * Compatibility Options Menu
 *
 *=======================================*/
// NOLINTBEGIN(readability-magic-numbers) - the numbers are the data
std::array<menuitem_t, 31> CompatItems = {{
	{.type = yellowtext, .label = "Gameplay",							.a = {.cvar = nullptr},                  .b = {.leftval = 0.0}, .c = {.rightval = 0.0}, .d = {.step = 0.0}, .e = {.values = nullptr}},
	{.type = svdiscrete, .label = "Finer-precision Autoaim",        .a = {.cvar = &co_fineautoaim},       .b = {.leftval = ARRAY_LENGTH(OnOff)}, .c = {.rightval = 0.0}, .d = {.step = 0.0}, .e = {.values = OnOff.data()}},
	{.type = svdiscrete, .label = "Fix hit detection at grid edges",.a = {.cvar = &co_blockmapfix},       .b = {.leftval = ARRAY_LENGTH(OnOff)}, .c = {.rightval = 0.0}, .d = {.step = 0.0}, .e = {.values = OnOff.data()}},
	{.type = svdiscrete, .label = "Remove pain elemental spawn limit",.a = {.cvar = &co_removesoullimit}, .b = {.leftval = ARRAY_LENGTH(OnOff)}, .c = {.rightval = 0.0}, .d = {.step = 0.0}, .e = {.values = OnOff.data()}},
	{.type = svdiscrete, .label = "Fix arch-vile ghost bug",			.a = {.cvar = &co_novileghosts}, .b = {.leftval = ARRAY_LENGTH(OnOff)}, .c = {.rightval = 0.0}, .d = {.step = 0.0}, .e = {.values = OnOff.data()}},
	{.type = redtext,   .label = " ",								.a = {.cvar = nullptr},                  .b = {.leftval = 0.0}, .c = {.rightval = 0.0}, .d = {.step = 0.0}, .e = {.values = nullptr}},
	{.type = yellowtext, .label = "Items and Decoration",				.a = {.cvar = nullptr},                  .b = {.leftval = 0.0}, .c = {.rightval = 0.0}, .d = {.step = 0.0}, .e = {.values = nullptr}},
	{.type = svdiscrete, .label = "Fix invisible puffs under skies",.a = {.cvar = &co_fixweaponimpacts},  .b = {.leftval = ARRAY_LENGTH(OnOff)}, .c = {.rightval = 0.0}, .d = {.step = 0.0}, .e = {.values = OnOff.data()}},
	{.type = svdiscrete, .label = "Items can be walked over/under", .a = {.cvar = &co_realactorheight},   .b = {.leftval = ARRAY_LENGTH(OnOff)}, .c = {.rightval = 0.0}, .d = {.step = 0.0}, .e = {.values = OnOff.data()}},
	{.type = svdiscrete, .label = "Items can drop off ledges",      .a = {.cvar = &co_allowdropoff},      .b = {.leftval = ARRAY_LENGTH(OnOff)}, .c = {.rightval = 0.0}, .d = {.step = 0.0}, .e = {.values = OnOff.data()}},
	{.type = redtext,   .label = " ",								.a = {.cvar = nullptr},                  .b = {.leftval = 0.0}, .c = {.rightval = 0.0}, .d = {.step = 0.0}, .e = {.values = nullptr}},
	{.type = yellowtext, .label = "Engine Compatibility",				.a = {.cvar = nullptr},                  .b = {.leftval = 0.0}, .c = {.rightval = 0.0}, .d = {.step = 0.0}, .e = {.values = nullptr}},
	{.type = svdiscrete, .label = "BOOM actor/sector/line checks",  .a = {.cvar = &co_boomphys},			 .b = {.leftval = ARRAY_LENGTH(OnOff)}, .c = {.rightval = 0.0}, .d = {.step = 0.0}, .e = {.values = OnOff.data()}},
	{.type = svdiscrete, .label = "MBF movement and collision",  .a = {.cvar = &co_mbfphys},			 .b = {.leftval = ARRAY_LENGTH(OnOff)}, .c = {.rightval = 0.0}, .d = {.step = 0.0}, .e = {.values = OnOff.data()}},
	{.type = svdiscrete, .label = "ZDOOM 1.23 physics",             .a = {.cvar = &co_zdoomphys},         .b = {.leftval = ARRAY_LENGTH(OnOff)}, .c = {.rightval = 0.0}, .d = {.step = 0.0}, .e = {.values = OnOff.data()}},
	{.type = svdiscrete, .label = "ZDOOM 1.23 ammo checks",         .a = {.cvar = &co_zdoomammo},         .b = {.leftval = ARRAY_LENGTH(OnOff)}, .c = {.rightval = 0.0}, .d = {.step = 0.0}, .e = {.values = OnOff.data()}},
	{.type = svdiscrete, .label = "ZDOOM Friendly Targeting",       .a = {.cvar = &co_zdoomfriendtargeting},   .b = {.leftval = ARRAY_LENGTH(OnOff)}, .c = {.rightval = 0.0}, .d = {.step = 0.0}, .e = {.values = OnOff.data()}},
	{.type = svdiscrete, .label = "MBF Monster target selection",.a = {.cvar = &co_pursuit},      .b = {.leftval = ARRAY_LENGTH(OnOff)}, .c = {.rightval = 0.0}, .d = {.step = 0.0}, .e = {.values = OnOff.data()}},
	{.type = svdiscrete, .label = "Monsters help friends (MBF)",.a = {.cvar = &co_helpfriends},      .b = {.leftval = ARRAY_LENGTH(OnOff)}, .c = {.rightval = 0.0}, .d = {.step = 0.0}, .e = {.values = OnOff.data()}},
	{.type = svdiscrete, .label = "Monsters strafe (MBF)",.a = {.cvar = &co_monsterbacking},      .b = {.leftval = ARRAY_LENGTH(OnOff)}, .c = {.rightval = 0.0}, .d = {.step = 0.0}, .e = {.values = OnOff.data()}},
	{.type = svdiscrete, .label = "Monster wind/friction (MBF)",.a = {.cvar = &co_monsterfriction},      .b = {.leftval = ARRAY_LENGTH(OnOff)}, .c = {.rightval = 0.0}, .d = {.step = 0.0}, .e = {.values = OnOff.data()}},
	{.type = svdiscrete, .label = "Monsters avoid crushers (MBF)",.a = {.cvar = &co_avoidhazards},      .b = {.leftval = ARRAY_LENGTH(OnOff)}, .c = {.rightval = 0.0}, .d = {.step = 0.0}, .e = {.values = OnOff.data()}},
	{.type = svdiscrete, .label = "Monsters climb (MBF)",.a = {.cvar = &co_monstersclimbsteep},      .b = {.leftval = ARRAY_LENGTH(OnOff)}, .c = {.rightval = 0.0}, .d = {.step = 0.0}, .e = {.values = OnOff.data()}},
	{.type = svdiscrete, .label = "Monsters stay on lifts (MBF)",.a = {.cvar = &co_staylift},      .b = {.leftval = ARRAY_LENGTH(OnOff)}, .c = {.rightval = 0.0}, .d = {.step = 0.0}, .e = {.values = OnOff.data()}},
	{.type = svdiscrete, .label = "Friends can drop off (MBF)",.a = {.cvar = &co_friend_ledgejumping},      .b = {.leftval = ARRAY_LENGTH(OnOff)}, .c = {.rightval = 0.0}, .d = {.step = 0.0}, .e = {.values = OnOff.data()}},
	{.type = slider,		 .label = "Friend distance (MBF)", .a = {.cvar = &co_friend_distance}, .b = {.leftval = 0.0}, .c = {.rightval = 2048.0}, .d = {.step = 64.0}, .e = {.values = nullptr}},
	{.type = redtext,   .label = " ",								.a = {.cvar = nullptr},                  .b = {.leftval = 0.0}, .c = {.rightval = 0.0}, .d = {.step = 0.0}, .e = {.values = nullptr}},
	{.type = yellowtext, .label = "Sound",							.a = {.cvar = nullptr},                  .b = {.leftval = 0.0}, .c = {.rightval = 0.0}, .d = {.step = 0.0}, .e = {.values = nullptr}},
	{.type = svdiscrete, .label = "Fix silent west spawns",         .a = {.cvar = &co_nosilentspawns},    .b = {.leftval = ARRAY_LENGTH(OnOff)}, .c = {.rightval = 0.0}, .d = {.step = 0.0}, .e = {.values = OnOff.data()}},
	{.type = svdiscrete, .label = "ZDoom Sound Response",			.a = {.cvar = &co_zdoomsound},		 .b = {.leftval = ARRAY_LENGTH(OnOff)}, .c = {.rightval = 0.0}, .d = {.step = 0.0}, .e = {.values = OnOff.data()}},
	{.type = svdiscrete, .label = "Global Pickup Sounds",			.a = {.cvar = &co_globalsound},		 .b = {.leftval = ARRAY_LENGTH(OnOff)}, .c = {.rightval = 0.0}, .d = {.step = 0.0}, .e = {.values = OnOff.data()}},
}};
// NOLINTEND(readability-magic-numbers)
} // namespace


menu_t CompatMenu = {
	"M_COMPAT",
	1,
	CompatItems.size(),
	MENU_LONGTEXTINDENT,
	CompatItems.data(),
	0,
	0,
	nullptr,
};
namespace
{



/*=======================================
 *
 * Network Options Menu
 *
 *=======================================*/

// NOLINTBEGIN(readability-magic-numbers) - the numbers are the data
std::array<menuitem_t, 15> NetworkItems = {{
	{ .type = redtext,	.label = " ",					.a = {.cvar = nullptr},	.b = {.leftval = 0.0}, .c = {.rightval = 0.0}, .d = {.step = 0.0}, .e = {.values = nullptr}},
	{ .type = yellowtext,	.label = "Wad Download Settings",		.a = {.cvar = nullptr},				.b = {.leftval = 0.0},		.c = {.rightval = 0.0},		.d = {.step = 0.0},		.e = {.values = nullptr}},
	{ .type = discrete, 	.label = "Download From Internet", 		.a = {.cvar = &cl_serverdownload}, .b = {.leftval = ARRAY_LENGTH(OnOff)}, 		.c = {.rightval = 0.0}, 		.d = {.step = 0.0}, 		.e = {.values = OnOff.data()}},

	{ .type = redtext,		.label = " ",							.a = {.cvar = nullptr},				.b = {.leftval = 0.0},		.c = {.rightval = 0.0},		.d = {.step = 0.0},		.e = {.values = nullptr}},
	{ .type = yellowtext,	.label = "Netdemo Settings",				.a = {.cvar = nullptr},				.b = {.leftval = 0.0},		.c = {.rightval = 0.0},		.d = {.step = 0.0},		.e = {.values = nullptr}},
	{ .type = discrete,		.label = "Autorecord demos",				.a = {.cvar = &cl_autorecord},	.b = {.leftval = ARRAY_LENGTH(OnOff)},		.c = {.rightval = 0.0},		.d = {.step = 0.0},		.e = {.values = OnOff.data()}},
	{ .type = discrete,		.label = "Split every map",				.a = {.cvar = &cl_splitnetdemos},	.b = {.leftval = ARRAY_LENGTH(OnOff)},		.c = {.rightval = 0.0},		.d = {.step = 0.0},		.e = {.values = OnOff.data()}},

	{ .type = redtext,		.label = " ",							.a = {.cvar = nullptr},	.b = {.leftval = 0.0}, .c = {.rightval = 0.0}, .d = {.step = 0.0}, .e = {.values = nullptr}},
	{ .type = yellowtext,	.label = "Autorecord filters",			.a = {.cvar = nullptr},				.b = {.leftval = 0.0},		.c = {.rightval = 0.0},		.d = {.step = 0.0},		.e = {.values = nullptr}},
	{ .type = discrete,		.label = "Cooperation",					.a = {.cvar = &cl_autorecord_coop},.b = {.leftval = ARRAY_LENGTH(DemoRestrictions)},		.c = {.rightval = 0.0},		.d = {.step = 0.0},		.e = {.values = DemoRestrictions.data()}},
	{ .type = discrete,		.label = "Deathmatch",					.a = {.cvar = &cl_autorecord_deathmatch},.b = {.leftval = ARRAY_LENGTH(DemoRestrictions)},		.c = {.rightval = 0.0},		.d = {.step = 0.0},		.e = {.values = DemoRestrictions.data()}},
	{ .type = discrete,		.label = "Duel",							.a = {.cvar = &cl_autorecord_duel},.b = {.leftval = ARRAY_LENGTH(DemoRestrictions)},		.c = {.rightval = 0.0},		.d = {.step = 0.0},		.e = {.values = DemoRestrictions.data()}},
	{ .type = discrete,		.label = "Team Deathmatch",				.a = {.cvar = &cl_autorecord_teamdm},.b = {.leftval = ARRAY_LENGTH(DemoRestrictions)},		.c = {.rightval = 0.0},		.d = {.step = 0.0},		.e = {.values = DemoRestrictions.data()}},
	{ .type = discrete,		.label = "Capture the Flag",				.a = {.cvar = &cl_autorecord_ctf},.b = {.leftval = ARRAY_LENGTH(DemoRestrictions)},		.c = {.rightval = 0.0},		.d = {.step = 0.0},		.e = {.values = DemoRestrictions.data()}},
	{ .type = discrete,		.label = "Horde",						.a = {.cvar = &cl_autorecord_horde},.b = {.leftval = ARRAY_LENGTH(DemoRestrictions)},		.c = {.rightval = 0.0},		.d = {.step = 0.0},		.e = {.values = DemoRestrictions.data()}},
}};
// NOLINTEND(readability-magic-numbers)
} // namespace


menu_t NetworkMenu = {
	"M_NETWRK",
	2,
	NetworkItems.size(),
	MENU_HALFPASTINDENT,
	NetworkItems.data(),
	1,
	0,
	nullptr
};
namespace
{



/*=======================================
 *
 * Weapon Preferences Menu
 *
 *=======================================*/

// NOLINTBEGIN(readability-magic-numbers) - the numbers are the data
std::array<value_t, 4> WeapSwitch = {{
	{ .value = 0.0,			.name = "Never"},
	{ .value = 1.0,			.name = "Always"},
	{ .value = 2.0,			.name = "By Preference"},
	{ .value = 3.0,			.name = "Attack Cancels PWO"}
}};
// NOLINTEND(readability-magic-numbers)
} // namespace


extern const char *weaponnames[];
namespace
{


// NOLINTBEGIN(readability-magic-numbers) - the numbers are the data
std::array<menuitem_t, 20> WeaponItems = {{
	{.type = yellowtext, .label = "Weapon Preferences",  .a = {.cvar = nullptr},               .b = {.leftval = 0.0}, .c = {.rightval = 0.0}, .d = {.step = 0.0}, .e = {.values = nullptr}},
	{.type = discrete,  .label = "Switch on pickup",    .a = {.cvar = &cl_switchweapon},   .b = {.leftval = ARRAY_LENGTH(WeapSwitch)}, .c = {.rightval = 0.0}, .d = {.step = 0.0}, .e = {.values = WeapSwitch.data()}},
	{.type = redtext,   .label = " ",                   .a = {.cvar = nullptr},               .b = {.leftval = 0.0}, .c = {.rightval = 0.0}, .d = {.step = 0.0}, .e = {.values = nullptr}},
	{.type = yellowtext, .label = "Weapon Switch Order", .a = {.cvar = nullptr},               .b = {.leftval = 0.0}, .c = {.rightval = 0.0}, .d = {.step = 0.0}, .e = {.values = nullptr}},
	{.type = slider,    .label = weaponnames[0],        .a = {.cvar = &cl_weaponpref_fst}, .b = {.leftval = 0.0}, .c = {.rightval = 8.0}, .d = {.step = 1.0}, .e = {.values = nullptr}},
	{.type = slider,    .label = weaponnames[7],        .a = {.cvar = &cl_weaponpref_csw}, .b = {.leftval = 0.0}, .c = {.rightval = 8.0}, .d = {.step = 1.0}, .e = {.values = nullptr}},
	{.type = slider,    .label = weaponnames[1],        .a = {.cvar = &cl_weaponpref_pis}, .b = {.leftval = 0.0}, .c = {.rightval = 8.0}, .d = {.step = 1.0}, .e = {.values = nullptr}},
	{.type = slider,    .label = weaponnames[2],        .a = {.cvar = &cl_weaponpref_sg},  .b = {.leftval = 0.0}, .c = {.rightval = 8.0}, .d = {.step = 1.0}, .e = {.values = nullptr}},
	{.type = slider,    .label = weaponnames[8],        .a = {.cvar = &cl_weaponpref_ssg}, .b = {.leftval = 0.0}, .c = {.rightval = 8.0}, .d = {.step = 1.0}, .e = {.values = nullptr}},
	{.type = slider,    .label = weaponnames[3],        .a = {.cvar = &cl_weaponpref_cg},  .b = {.leftval = 0.0}, .c = {.rightval = 8.0}, .d = {.step = 1.0}, .e = {.values = nullptr}},
	{.type = slider,    .label = weaponnames[4],        .a = {.cvar = &cl_weaponpref_rl},  .b = {.leftval = 0.0}, .c = {.rightval = 8.0}, .d = {.step = 1.0}, .e = {.values = nullptr}},
	{.type = slider,    .label = weaponnames[5],        .a = {.cvar = &cl_weaponpref_pls}, .b = {.leftval = 0.0}, .c = {.rightval = 8.0}, .d = {.step = 1.0}, .e = {.values = nullptr}},
	{.type = slider,    .label = weaponnames[6],        .a = {.cvar = &cl_weaponpref_bfg}, .b = {.leftval = 0.0}, .c = {.rightval = 8.0}, .d = {.step = 1.0}, .e = {.values = nullptr}},
	{.type = redtext,   .label = " ",                   .a = {.cvar = nullptr},               .b = {.leftval = 0.0}, .c = {.rightval = 0.0}, .d = {.step = 0.0}, .e = {.values = nullptr}},
	{.type = whitetext, .label = "Weapons with higher", .a = {.cvar = nullptr},               .b = {.leftval = 0.0}, .c = {.rightval = 0.0}, .d = {.step = 0.0}, .e = {.values = nullptr}},
	{.type = whitetext, .label = "preference are selected first", .a = {.cvar = nullptr},     .b = {.leftval = 0.0}, .c = {.rightval = 0.0}, .d = {.step = 0.0}, .e = {.values = nullptr}},
	{.type = redtext,	.label = " ",				   .a = {.cvar = nullptr},				 .b = {.leftval = 0.0}, .c = {.rightval = 0.0}, .d = {.step = 0.0}, .e = {.values = nullptr}},
	{.type = yellowtext, .label = "! ! ! NOTICE ! ! !", .a = {.cvar = nullptr}, .b = {.leftval = 0.0}, .c = {.rightval = 0.0}, .d = {.step = 0.0}, .e = {.values = nullptr}},
	{.type = orangetext, .label = "While playing online, this feature", .a = {.cvar = nullptr},.b = {.leftval = 0.0}, .c = {.rightval = 0.0}, .d = {.step = 0.0}, .e = {.values = nullptr}},
	{.type = orangetext, .label = "only works when the server allows it!", .a = {.cvar = nullptr}, .b = {.leftval = 0.0}, .c = {.rightval = 0.0}, .d = {.step = 0.0}, .e = {.values = nullptr}},
}};
// NOLINTEND(readability-magic-numbers)
} // namespace


menu_t WeaponMenu = {
	"M_WEAPON",
	1,
	WeaponItems.size(),
	MENU_HALFPASTINDENT,
	WeaponItems.data(),
	0,
	0,
	nullptr
};
namespace
{



/*=======================================
 *
 * Display Options Menu
 *
 *=======================================*/
void StartHUDMenu();
void StartMessagesMenu();
void StartAutomapMenu();
} // namespace

void ResetCustomColors();

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

namespace
{

// NOLINTBEGIN(readability-magic-numbers) - the numbers are the data
std::array<value_t, 4> Wipes = {{
	{ .value = 0.0, .name = "None"},
	{ .value = 1.0, .name = "Melt"},
	{ .value = 2.0, .name = "Burn"},
	{ .value = 3.0, .name = "Crossfade"}
}};


std::array<value_t, 4> Overlays = {{
	{ .value = 0.0, .name = "Off"},
	{ .value = 1.0, .name = "Standard"},
	{ .value = 2.0, .name = "Full"},
	{ .value = 3.0, .name = "Full Only"}
}};
// NOLINTEND(readability-magic-numbers)

void M_SendUINewColor (int red, int green, int blue);
void M_SlideUIRed (int);
void M_SlideUIGreen (int);
void M_SlideUIBlue (int);
} // namespace


int dummy = 0;

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
namespace
{


// NOLINTBEGIN(readability-magic-numbers) - the numbers are the data
std::array<value_t, 3> Endoom = {{{.value = 0.0, .name = "Off"}, {.value = 1.0, .name = "On"}, {.value = 2.0, .name = "PWAD Only"}}};

std::array<menuitem_t, 41> VideoItems = {{
	{ .type = more, .label = "Heads-up display", .a = {.cvar = nullptr}, .b = {.leftval = 0.0}, .c = {.rightval = 0.0}, .d = {.step = 0.0}, .e = {.mfunc = StartHUDMenu}},
	{ .type = more,		.label = "Messages",				    .a = {.cvar = nullptr},					.b = {.leftval = 0.0}, .c = {.rightval = 0.0},	.d = {.step = 0.0},  .e = {.mfunc = StartMessagesMenu}},
	{ .type = more,		.label = "Automap",				    .a = {.cvar = nullptr},					.b = {.leftval = 0.0}, .c = {.rightval = 0.0},	.d = {.step = 0.0},  .e = {.mfunc = StartAutomapMenu}},
	{ .type = redtext,	.label = " ",					    .a = {.cvar = nullptr},					.b = {.leftval = 0.0}, .c = {.rightval = 0.0},	.d = {.step = 0.0},  .e = {.values = nullptr}},
	{ .type = slider,	.label = "Screen size",			    .a = {.cvar = &screenblocks},	   	.b = {.leftval = 3.0}, .c = {.rightval = 12.0},	.d = {.step = 1.0},  .e = {.values = nullptr}},
	{ .type = slider,	.label = "Brightness",			    .a = {.cvar = &gammalevel},			.b = {.leftval = 1.0}, .c = {.rightval = 8.0},	.d = {.step = 1.0},  .e = {.values = nullptr}},
	{ .type = slider,	.label = "Red Pain Intensity",		.a = {.cvar = &r_painintensity},		.b = {.leftval = 0.0}, .c = {.rightval = 1.0},	.d = {.step = 0.1},  .e = {.values = nullptr}},
	{ .type = slider,	.label = "Movement bobbing",			.a = {.cvar = &cl_movebob},			.b = {.leftval = 0.0}, .c = {.rightval = 1.0},	.d = {.step = 0.1},	.e = {.values = nullptr}},
	{ .type = slider,   .label = "Weapon Visibility",        .a = {.cvar = &r_drawplayersprites}, .b = {.leftval = 0.0}, .c = {.rightval = 1.0},   .d = {.step = 0.1},  .e = {.values = nullptr}},
	{ .type = discrete,	.label = "Visible Spawn Points",		.a = {.cvar = &cl_showspawns},		.b = {.leftval = ARRAY_LENGTH(OnOff)}, .c = {.rightval = 0.0},	.d = {.step = 0.0},	.e = {.values = OnOff.data()}},
	{ .type = discrete, .label = "Center weapon when firing",.a = {.cvar = &cl_centerbobonfire},	.b = {.leftval = ARRAY_LENGTH(OnOff)}, .c = {.rightval = 0.0},	.d = {.step = 0.0},	.e = {.values = OnOff.data()}},
	{ .type = discrete, .label = "Show Killing Sprees",		.a = {.cvar = &cl_showsprees},	.b = {.leftval = ARRAY_LENGTH(OnOff)}, .c = {.rightval = 0.0},	.d = {.step = 0.0},	.e = {.values = OnOff.data()}},
	{ .type = discrete, .label = "Show Multi Kills",		.a = {.cvar = &cl_showmultikills},	.b = {.leftval = ARRAY_LENGTH(OnOff)}, .c = {.rightval = 0.0},	.d = {.step = 0.0},	.e = {.values = OnOff.data()}},
	{ .type = discrete, .label = "Show Sprees Offline",	.a = {.cvar = &cl_showofflinesprees},.b = {.leftval = ARRAY_LENGTH(OnOff)}, .c = {.rightval = 0.0},	.d = {.step = 0.0},	.e = {.values = OnOff.data()}},
	{ .type = discrete, .label = "Show Multi Kills Offline",	.a = {.cvar = &cl_showofflinemultikills},.b = {.leftval = ARRAY_LENGTH(OnOff)}, .c = {.rightval = 0.0},	.d = {.step = 0.0},	.e = {.values = OnOff.data()}},
	{ .type = redtext,	.label = " ",					    .a = {.cvar = nullptr},				    .b = {.leftval = 0.0}, .c = {.rightval = 0.0},	.d = {.step = 0.0},  .e = {.values = nullptr}},
	{ .type = discrete, .label = "Force Team Color",			.a = {.cvar = &r_forceteamcolor},	.b = {.leftval = ARRAY_LENGTH(OnOff)}, .c = {.rightval = 0.0},	.d = {.step = 0.0},	.e = {.values = OnOff.data()}},
	{ .type = redslider,   .label = "Team Color Red",        .a = {.cvar = &r_teamcolor},  .b = {.leftval = 0.0}, .c = {.rightval = 0.0},   .d = {.step = 0.0},  .e = {.values = nullptr}},
	{ .type = greenslider, .label = "Team Color Green",      .a = {.cvar = &r_teamcolor},  .b = {.leftval = 0.0}, .c = {.rightval = 0.0},   .d = {.step = 0.0},  .e = {.values = nullptr}},
	{ .type = blueslider,  .label = "Team Color Blue",       .a = {.cvar = &r_teamcolor},  .b = {.leftval = 0.0}, .c = {.rightval = 0.0},   .d = {.step = 0.0},  .e = {.values = nullptr}},
	{ .type = redtext,	.label = " ",					    .a = {.cvar = nullptr},				    .b = {.leftval = 0.0}, .c = {.rightval = 0.0},	.d = {.step = 0.0},  .e = {.values = nullptr}},
	{ .type = discrete, .label = "Force Enemy Color",        .a = {.cvar = &r_forceenemycolor},	.b = {.leftval = ARRAY_LENGTH(OnOff)}, .c = {.rightval = 0.0},	.d = {.step = 0.0},	.e = {.values = OnOff.data()}},
	{ .type = redslider,   .label = "Enemy Color Red",       .a = {.cvar = &r_enemycolor},  .b = {.leftval = 0.0}, .c = {.rightval = 0.0},   .d = {.step = 0.0},  .e = {.values = nullptr}},
	{ .type = greenslider, .label = "Enemy Color Green",     .a = {.cvar = &r_enemycolor},  .b = {.leftval = 0.0}, .c = {.rightval = 0.0},   .d = {.step = 0.0},  .e = {.values = nullptr}},
	{ .type = blueslider,  .label = "Enemy Color Blue",      .a = {.cvar = &r_enemycolor},  .b = {.leftval = 0.0}, .c = {.rightval = 0.0},   .d = {.step = 0.0},  .e = {.values = nullptr}},
	{ .type = redtext,	.label = " ",					    .a = {.cvar = nullptr},				    .b = {.leftval = 0.0}, .c = {.rightval = 0.0},	.d = {.step = 0.0},  .e = {.values = nullptr}},
	{ .type = slider,   .label = "UI Background Red",        .a = {.cvar = &ui_transred},         .b = {.leftval = 0.0}, .c = {.rightval = 255.0}, .d = {.step = 16.0}, .e = {.values = nullptr}},
	{ .type = slider,   .label = "UI Background Green",      .a = {.cvar = &ui_transgreen},       .b = {.leftval = 0.0}, .c = {.rightval = 255.0}, .d = {.step = 16.0}, .e = {.values = nullptr}},
	{ .type = slider,   .label = "UI Background Blue",       .a = {.cvar = &ui_transblue},        .b = {.leftval = 0.0}, .c = {.rightval = 255.0}, .d = {.step = 16.0}, .e = {.values = nullptr}},
	{ .type = slider,   .label = "UI Background Visibility", .a = {.cvar = &ui_dimamount},        .b = {.leftval = 0.0}, .c = {.rightval = 1.0},   .d = {.step = 0.1},  .e = {.values = nullptr}},
	{ .type = redtext,	.label = " ",					    .a = {.cvar = nullptr},					.b = {.leftval = 0.0}, .c = {.rightval = 0.0},	.d = {.step = 0.0},  .e = {.values = nullptr}},
	{ .type = discrete, .label = "See killer on Death",			.a = {.cvar = &cl_deathcam},   .b = {.leftval = ARRAY_LENGTH(OnOff)}, .c = {.rightval = 0.0}, .d = {.step = 0.0}, .e = {.values = OnOff.data()}},
	{ .type = discrete, .label = "Stretch short skies",	    .a = {.cvar = &r_stretchsky},	   	.b = {.leftval = ARRAY_LENGTH(OnOffAuto)}, .c = {.rightval = 0.0},	.d = {.step = 0.0},  .e = {.values = OnOffAuto.data()}},
	{ .type = discrete, .label = "Linear Skies",			    .a = {.cvar = &r_linearsky},	   		.b = {.leftval = ARRAY_LENGTH(OnOff)}, .c = {.rightval = 0.0},	.d = {.step = 0.0},  .e = {.values = OnOff.data()}},
	{ .type = discrete, .label = "Invuln changes skies",		.a = {.cvar = &r_skypalette},		.b = {.leftval = ARRAY_LENGTH(OnOff)}, .c = {.rightval = 0.0},	.d = {.step = 0.0},	.e = {.values = OnOff.data()}},
	{ .type = discrete, .label = "Use softer invuln effect", .a = {.cvar = &r_softinvulneffect},	.b = {.leftval = ARRAY_LENGTH(OnOff)}, .c = {.rightval = 0.0},	.d = {.step = 0.0},	.e = {.values = OnOff.data()}},
	{ .type = discrete, .label = "Heart effect on friendlies", .a = {.cvar = &cl_showfriends},	.b = {.leftval = ARRAY_LENGTH(OnOff)}, .c = {.rightval = 0.0},	.d = {.step = 0.0},	.e = {.values = OnOff.data()}},
	{ .type = discrete, .label = "Screen wipe style",	    .a = {.cvar = &r_wipetype},			.b = {.leftval = ARRAY_LENGTH(Wipes)}, .c = {.rightval = 0.0},	.d = {.step = 0.0},  .e = {.values = Wipes.data()}},
	{ .type = discrete, .label = "Multiplayer Intermissions",.a = {.cvar = &wi_oldintermission},	.b = {.leftval = ARRAY_LENGTH(DoomOrOdamex)}, .c = {.rightval = 0.0},	.d = {.step = 0.0},  .e = {.values = DoomOrOdamex.data()}},
	{ .type = discrete, .label = "Show loading disk icon",	.a = {.cvar = &r_loadicon},			.b = {.leftval = ARRAY_LENGTH(OnOff)}, .c = {.rightval = 0.0},	.d = {.step = 0.0},	.e = {.values = OnOff.data()}},
	{ .type = discrete,	.label = "Show DOS ending screen",  .a = {.cvar = &r_showendoom},		.b = {.leftval = ARRAY_LENGTH(Endoom)}, .c = {.rightval = 0.0},	.d = {.step = 0.0},  .e = {.values = Endoom.data()}},


}};
// NOLINTEND(readability-magic-numbers)

void M_UpdateDisplayOptions()
{
	const static size_t menu_length = VideoItems.size();
	const static size_t gamma_index = M_FindCvarInMenu(gammalevel, VideoItems.data(), menu_length);

	// update the parameters for gammalevel based on vid_gammatype (doom or zdoom gamma)
	VideoItems[gamma_index].b.leftval = V_GetMinimumGammaLevel();
	VideoItems[gamma_index].c.rightval = V_GetMaximumGammaLevel();
	VideoItems[gamma_index].d.step = 0.1f;
}
} // namespace


menu_t VideoMenu = {
	"M_VIDEO",
	0,
	VideoItems.size(),
	0,
	VideoItems.data(),
	4,
	0,
	&M_UpdateDisplayOptions
};
namespace
{


/*=======================================
 *
 * HUD Menu
 *
 *=======================================*/

// NOLINTBEGIN(readability-magic-numbers) - the numbers are the data
std::array<value_t, 4> SecretOptions = {{
	{ .value = 0.0, .name = "Off"},
	{ .value = 1.0, .name = "On (with sounds)"},
	{ .value = 2.0, .name = "On (w/o sounds)"},
	{ .value = 3.0, .name = "Own only"},
}};

std::array<value_t, 3> TimerStyles = {{
	{ .value = 0.0, .name = "No Timer" }, { .value = 1.0, .name = "Count Down" }, { .value = 2.0, .name = "Count Up" }
}};

std::array<value_t, 3> FlagHelds = {{
	{ .value = 0.0, .name = "Off" }, { .value = 1.0, .name = "Complete" }, { .value = 2.0, .name = "Simple" }
}};

std::array<value_t, 9> Crosshairs = {{
	{ .value = 0.0, .name = "None" }, { .value = 1.0, .name = "Cross 1" }, { .value = 2.0, .name = "Cross 2" },
	{ .value = 3.0, .name = "X" },    { .value = 4.0, .name = "Diamond" }, { .value = 5.0, .name = "Dot" },
	{ .value = 6.0, .name = "Box"},  {.value = 7.0, .name = "Angle"},   {.value = 8.0, .name = "Big Thing"}}};

std::array<value_t, 5> ExtendedHudStyles = {{
	{ .value = 0.0, .name = "Off" }, { .value = 1.0, .name = "Horizontal 1" }, { .value = 2.0, .name = "Horizontal 2" },
	{ .value = 3.0, .name = "Vertical 1" }, { .value = 4.0, .name = "Vertical 2" }
}};

std::array<menuitem_t, 34> HUDItems = {{
	{ .type = yellowtext, .label = "Status Bar", .a = {.cvar = nullptr}, .b = {.leftval = 0.0}, .c = {.rightval = 0.0}, .d = {.step = 0.0}, .e = {.values = nullptr}},
	{ .type = discrete, .label = "Scale status bar", .a = {.cvar = &st_scale}, .b = {.leftval = ARRAY_LENGTH(OnOff)}, .c = {.rightval = 0.0}, .d = {.step = 0.0}, .e = {.values = OnOff.data()}},
	{ .type = redtext, .label = " ", .a = {.cvar = nullptr}, .b = {.leftval = 0.0}, .c = {.rightval = 0.0}, .d = {.step = 0.0}, .e = {.values = nullptr}},
	{ .type = yellowtext, .label = "Floating HUD elements", .a = {.cvar = nullptr}, .b = {.leftval = 0.0}, .c = {.rightval = 0.0}, .d = {.step = 0.0}, .e = {.values = nullptr}},
	{ .type = discrete, .label = "Scale HUD elements", .a = {.cvar = &hud_scale}, .b = {.leftval = ARRAY_LENGTH(OnOff)}, .c = {.rightval = 0.0}, .d = {.step = 0.0}, .e = {.values = OnOff.data()}},
	{ .type = slider, .label = "HUD Transparency", .a = {.cvar = &hud_transparency}, .b = {.leftval = 0.0}, .c = {.rightval = 1.0}, .d = {.step = 0.1}, .e = {.values = nullptr}},
	{ .type = slider, .label = "HUD Anchoring", .a = {.cvar = &hud_anchoring}, .b = {.leftval = 0.0}, .c = {.rightval = 1.0}, .d = {.step = 0.1}, .e = {.values = nullptr}},
	{.type = discrete, .label = "Bigger font in HUD", .a = {.cvar = &hud_bigfont}, .b = {.leftval = ARRAY_LENGTH(OnOff)}, .c = {.rightval = 0.0}, .d = {.step = 0.0}, .e = {.values = OnOff.data()}},
	// clang-format off
	{ .type = discrete, .label = "Show Secret Messages", .a = {.cvar = &hud_revealsecrets}, .b = {.leftval = ARRAY_LENGTH(SecretOptions)}, .c = {.rightval = 0.0}, .d = {.step = 0.0}, .e = {.values = SecretOptions.data()}},
	{ .type = discrete, .label = "Player target names", .a = {.cvar = &hud_targetnames}, .b = {.leftval = ARRAY_LENGTH(HideShow)}, .c = {.rightval = 0.0}, .d = {.step = 0.0}, .e = {.values = HideShow.data()}},
	// clang-format on
	{ .type = discrete, .label = "Timer Type", .a = {.cvar = &hud_timer}, .b = {.leftval = ARRAY_LENGTH(TimerStyles)}, .c = {.rightval = 0.0}, .d = {.step = 0.0}, .e = {.values = TimerStyles.data()}},
	{ .type = discrete, .label = "Speedometer", .a = {.cvar = &hud_speedometer}, .b = {.leftval = ARRAY_LENGTH(OnOff)}, .c = {.rightval = 0.0}, .d = {.step = 0.0}, .e = {.values = OnOff.data()}},
	{ .type = slider, .label = "Feed Timeout", .a = {.cvar = &hud_feedtime}, .b = {.leftval = 1.0}, .c = {.rightval = 10.0}, .d = {.step = 0.25}, .e = {.values = nullptr}},
	{ .type = discrete, .label = "Show Kills in Feed", .a = {.cvar = &hud_feedobits}, .b = {.leftval = ARRAY_LENGTH(OnOff)}, .c = {.rightval = 0.0}, .d = {.step = 0.0}, .e = {.values = OnOff.data()}},
	{ .type = discrete, .label = "Netdemo infos", .a = {.cvar = &hud_demobar}, .b = {.leftval = ARRAY_LENGTH(OnOff)}, .c = {.rightval = 0.0}, .d = {.step = 0.0}, .e = {.values = OnOff.data()}},
	{ .type = discrete, .label = "Extended hud", .a = {.cvar = &hud_extendedinfo}, .b = {.leftval = ARRAY_LENGTH(ExtendedHudStyles)}, .c = {.rightval = 0.0}, .d = {.step = 0.0}, .e = {.values = ExtendedHudStyles.data()}},
	{ .type = redtext, .label = " ", .a = {.cvar = nullptr}, .b = {.leftval = 0.0}, .c = {.rightval = 0.0}, .d = {.step = 0.0}, .e = {.values = nullptr}},

	{ .type = yellowtext, .label = "Scoreboard", .a = {.cvar = nullptr}, .b = {.leftval = 0.0}, .c = {.rightval = 0.0}, .d = {.step = 0.0}, .e = {.values = nullptr}},
	{ .type = slider, .label = "Scale scoreboard", .a = {.cvar = &hud_scalescoreboard}, .b = {.leftval = 0.0}, .c = {.rightval = 1.0}, .d = {.step = 0.125}, .e = {.values = nullptr}},
	// clang-format off
	{ .type = discrete, .label = "Scores on Death", .a = {.cvar = &hud_show_scoreboard_ondeath}, .b = {.leftval = ARRAY_LENGTH(OnOff)}, .c = {.rightval = 0.0}, .d = {.step = 0.0}, .e = {.values = OnOff.data()}},
	// clang-format on
	{ .type = redtext, .label = " ", .a = {.cvar = nullptr}, .b = {.leftval = 0.0}, .c = {.rightval = 0.0}, .d = {.step = 0.0}, .e = {.values = nullptr}},

	{ .type = yellowtext, .label = "Capture the Flag", .a = {.cvar = nullptr}, .b = {.leftval = 0.0}, .c = {.rightval = 0.0}, .d = {.step = 0.0}, .e = {.values = nullptr}},
	{ .type = discrete, .label = "Event Message Type", .a = {.cvar = &hud_gamemsgtype}, .b = {.leftval = ARRAY_LENGTH(VoxType)}, .c = {.rightval = 0.0}, .d = {.step = 0.0}, .e = {.values = VoxType.data()}},
	{ .type = discrete, .label = "Held Flag Border", .a = {.cvar = &hud_heldflag}, .b = {.leftval = ARRAY_LENGTH(FlagHelds)}, .c = {.rightval = 0.0}, .d = {.step = 0.0}, .e = {.values = FlagHelds.data()}},
	{ .type = discrete, .label = "Held Flag Flashes", .a = {.cvar = &hud_heldflag_flash}, .b = {.leftval = ARRAY_LENGTH(OnOff)}, .c = {.rightval = 0.0}, .d = {.step = 0.0}, .e = {.values = OnOff.data()}},
	{ .type = redtext, .label = " ", .a = {.cvar = nullptr}, .b = {.leftval = 0.0}, .c = {.rightval = 0.0}, .d = {.step = 0.0}, .e = {.values = nullptr}},

	{ .type = yellowtext, .label = "Crosshair", .a = {.cvar = nullptr}, .b = {.leftval = 0.0}, .c = {.rightval = 0.0}, .d = {.step = 0.0}, .e = {.values = nullptr}},
	{ .type = discrete, .label = "Crosshair type", .a = {.cvar = &hud_crosshair}, .b = {.leftval = ARRAY_LENGTH(Crosshairs)}, .c = {.rightval = 0.0}, .d = {.step = 0.0}, .e = {.values = Crosshairs.data()}},
	{ .type = discrete, .label = "Scale crosshair", .a = {.cvar = &hud_crosshairscale}, .b = {.leftval = ARRAY_LENGTH(OnOff)}, .c = {.rightval = 0.0}, .d = {.step = 0.0}, .e = {.values = OnOff.data()}},
	{ .type = discrete, .label = "Crosshair health", .a = {.cvar = &hud_crosshairhealth}, .b = {.leftval = ARRAY_LENGTH(OnOff)}, .c = {.rightval = 0.0}, .d = {.step = 0.0}, .e = {.values = OnOff.data()}},
	{ .type = redslider, .label = "Crosshair Red", .a = {.cvar = &hud_crosshaircolor}, .b = {.leftval = 0.0}, .c = {.rightval = 0.0}, .d = {.step = 0.0}, .e = {.values = nullptr}},
	{ .type = greenslider, .label = "Crosshair Green", .a = {.cvar = &hud_crosshaircolor}, .b = {.leftval = 0.0}, .c = {.rightval = 0.0}, .d = {.step = 0.0}, .e = {.values = nullptr}},
	{ .type = blueslider, .label = "Crosshair Blue", .a = {.cvar = &hud_crosshaircolor}, .b = {.leftval = 0.0}, .c = {.rightval = 0.0}, .d = {.step = 0.0}, .e = {.values = nullptr}},
	{ .type = redtext, .label = " ", .a = {.cvar = nullptr}, .b = {.leftval = 0.0}, .c = {.rightval = 0.0}, .d = {.step = 0.0}, .e = {.values = nullptr}},
}};
// NOLINTEND(readability-magic-numbers)
} // namespace


menu_t HUDMenu = {
	"M_HUD",                // title
	1,                      // lastOn
	HUDItems.size(),        // numitems
	0,                      // indent
	HUDItems.data(),        // items
	0,                      // scrolltop
	0,                      // scrollpos
	nullptr,                // refreshfunc
};

/*=======================================
 *
 * Messages Menu
 *
 *=======================================*/
EXTERN_CVAR(message_showpickups)
EXTERN_CVAR(message_showobituaries)
EXTERN_CVAR (con_coloredmessages)
EXTERN_CVAR (con_scaletext)
EXTERN_CVAR (hud_scaletext)
EXTERN_CVAR (msg0color)
EXTERN_CVAR (msg1color)
EXTERN_CVAR (msg2color)
EXTERN_CVAR (msg3color)
EXTERN_CVAR (msg4color)
EXTERN_CVAR (msgmidcolor)

namespace
{

// NOLINTBEGIN(readability-magic-numbers) - the numbers are the data
std::array<value_t, 21> TextColors =
{{
	{ .value = CR_BRICK,		.name = "brick"},
	{ .value = CR_TAN,		.name = "tan"},
	{ .value = CR_GRAY,		.name = "gray"},
	{ .value = CR_GREEN,		.name = "green"},
	{ .value = CR_BROWN,		.name = "brown"},
	{ .value = CR_GOLD, 		.name = "gold"},
	{ .value = CR_RED,		.name = "red"},
	{ .value = CR_BLUE,		.name = "blue"},
	{ .value = CR_ORANGE,	.name = "orange"},
	{ .value = CR_WHITE,		.name = "white"},
	{ .value = CR_YELLOW,	.name = "yellow"},
	{ .value = CR_BLACK,		.name = "black"},
	{ .value = CR_LIGHTBLUE,	.name = "light blue"},
	{ .value = CR_CREAM,		.name = "cream"},
	{ .value = CR_OLIVE,		.name = "olive"},
	{ .value = CR_DARKGREEN,	.name = "dark green"},
	{ .value = CR_DARKRED,	.name = "dark red"},
	{ .value = CR_DARKBROWN,	.name = "dark brown"},
	{ .value = CR_PURPLE,	.name = "purple"},
	{ .value = CR_DARKGRAY,	.name = "dark gray"},
	{ .value = CR_CYAN,		.name = "cyan"}
}};


// TODO: Put all language info in one array, auto detect what's in the lump?
//static value_t Languages[] = { // unused
//	{ .value = 0.0, .name = "Auto"},
//	{ .value = 1.0, .name = "English"},
//	{ .value = 2.0, .name = "French"},
//	{ .value = 3.0, .name = "Italian"}
//};

// Stops at 4X: hud_scaletext and con_scaletext are both ranged 0 to 4.
std::array<value_t, 5> ScaleFactors = {{
	{ .value = 0.0, .name = "Auto"},
	{ .value = 1.0, .name = "1X"},
	{ .value = 2.0, .name = "2X"},
	{ .value = 3.0, .name = "3X"},
	{ .value = 4.0, .name = "4X"}
}};

// Sized by the initializer: entries here are conditionally compiled,
// so a fixed std::array size would be wrong on some builds.
// NOLINTNEXTLINE(modernize-avoid-c-arrays)
menuitem_t MessagesItems[] = {
#if 0
	{ .type = discrete, .label = "Language", 			 .a = {.cvar = &language},		   	.b = {.leftval = ARRAY_LENGTH(Languages)}, .c = {.rightval = 0.0},   .d = {.step = 0.0}, .e = {.values = Languages}},
#endif
	{ .type = slider,	.label = "Message Timeout",		 .a = {.cvar = &con_notifytime},		.b = {.leftval = 1.0}, .c = {.rightval = 10.0},	.d = {.step = 0.25}, .e = {.values = nullptr}},
	{ .type = slider,	.label = "Center Message Timeout",.a = {.cvar = &con_midtime},		.b = {.leftval = 1.0}, .c = {.rightval = 10.0},	.d = {.step = 0.25}, .e = {.values = nullptr}},
	{ .type = discrete,	.label = "Scale message text",    .a = {.cvar = &hud_scaletext},		.b = {.leftval = ARRAY_LENGTH(ScaleFactors)}, .c = {.rightval = 0.0}, 	.d = {.step = 0.0}, .e = {.values = ScaleFactors.data()}},
	{ .type = discrete,	.label = "Colorize messages",	.a = {.cvar = &con_coloredmessages},	.b = {.leftval = ARRAY_LENGTH(OnOff)}, .c = {.rightval = 0.0},   .d = {.step = 0.0},	.e = {.values = OnOff.data()}},
	{ .type = discrete,	.label = "Scale console text",   .a = {.cvar = &con_scaletext},		.b = {.leftval = ARRAY_LENGTH(ScaleFactors)}, .c = {.rightval = 0.0}, 	.d = {.step = 0.0}, .e = {.values = ScaleFactors.data()}},
	{ .type = redtext,	.label = " ",					.a = {.cvar = nullptr},					.b = {.leftval = 0.0}, .c = {.rightval = 0.0},	.d = {.step = 0.0}, .e = {.values = nullptr}},
	{ .type = yellowtext,.label = "Display settings",	.a = {.cvar = nullptr},					.b = {.leftval = 0.0}, .c = {.rightval = 0.0},	.d = {.step = 0.0}, .e = {.values = nullptr}},
	{ .type = discrete,	.label = "Pickup messages",		.a = {.cvar = &message_showpickups},	.b = {.leftval = ARRAY_LENGTH(OnOff)}, .c = {.rightval = 0.0},   .d = {.step = 0.0},	.e = {.values = OnOff.data()}},
	{ .type = discrete,	.label = "Death messages",		.a = {.cvar = &message_showobituaries},	.b = {.leftval = ARRAY_LENGTH(OnOff)}, .c = {.rightval = 0.0},   .d = {.step = 0.0},	.e = {.values = OnOff.data()}},
	{ .type = discrete,	.label = "Spectator messages",	.a = {.cvar = &mute_spectators},	.b = {.leftval = ARRAY_LENGTH(OffOn)}, .c = {.rightval = 0.0},   .d = {.step = 0.0},	.e = {.values = OffOn.data()}},
	{ .type = discrete,	.label = "Enemy messages",		.a = {.cvar = &mute_enemies},	.b = {.leftval = ARRAY_LENGTH(OffOn)}, .c = {.rightval = 0.0},   .d = {.step = 0.0},	.e = {.values = OffOn.data()}},

	{ .type = redtext,	.label = " ",					.a = {.cvar = nullptr},					.b = {.leftval = 0.0}, .c = {.rightval = 0.0},	.d = {.step = 0.0}, .e = {.values = nullptr}},
	{ .type = yellowtext, .label = "Message Colors",		.a = {.cvar = nullptr},					.b = {.leftval = 0.0}, .c = {.rightval = 0.0},	.d = {.step = 0.0}, .e = {.values = nullptr}},
	{ .type = cdiscrete, .label = "Item Pickup",			.a = {.cvar = &msg0color},		   	.b = {.leftval = ARRAY_LENGTH(TextColors)}, .c = {.rightval = 0.0},	.d = {.step = 0.0}, .e = {.values = TextColors.data()}},
	{ .type = cdiscrete, .label = "Obituaries",			.a = {.cvar = &msg1color},		   	.b = {.leftval = ARRAY_LENGTH(TextColors)}, .c = {.rightval = 0.0},	.d = {.step = 0.0}, .e = {.values = TextColors.data()}},
	{ .type = cdiscrete, .label = "Critical Messages",	.a = {.cvar = &msg2color},		   	.b = {.leftval = ARRAY_LENGTH(TextColors)}, .c = {.rightval = 0.0},	.d = {.step = 0.0}, .e = {.values = TextColors.data()}},
	{ .type = cdiscrete, .label = "Chat Messages",		.a = {.cvar = &msg3color},		   	.b = {.leftval = ARRAY_LENGTH(TextColors)}, .c = {.rightval = 0.0},	.d = {.step = 0.0}, .e = {.values = TextColors.data()}},
	{ .type = cdiscrete, .label = "Team Messages",		.a = {.cvar = &msg4color},		   	.b = {.leftval = ARRAY_LENGTH(TextColors)}, .c = {.rightval = 0.0},	.d = {.step = 0.0}, .e = {.values = TextColors.data()}},
	{ .type = cdiscrete, .label = "Centered Messages",	.a = {.cvar = &msgmidcolor},			.b = {.leftval = ARRAY_LENGTH(TextColors)}, .c = {.rightval = 0.0},	.d = {.step = 0.0}, .e = {.values = TextColors.data()}}
};
// NOLINTEND(readability-magic-numbers)
} // namespace


menu_t MessagesMenu = {
	"M_MESS",
	0,
	ARRAY_LENGTH(MessagesItems),
	0,
	MessagesItems,
	0,
	0,
	nullptr
};
namespace
{


/*=======================================
 *
 * Automap Menu
 *
 *=======================================*/

// NOLINTBEGIN(readability-magic-numbers) - the numbers are the data
std::array<value_t, 2> ClassicMapStringTypes = {{
	{ .value = 0.0, .name = "Odamex"},
	{ .value = 1.0, .name = "Classic"}
}};

std::array<value_t, 7> AutomapScales = {{
	{ .value = 0.0, .name = "Auto"},
	{ .value = 1.0, .name = "1X"},
	{ .value = 2.0, .name = "2X"},
	{ .value = 3.0, .name = "3X"},
	{ .value = 4.0, .name = "4X"},
	{ .value = 5.0, .name = "5X"},
	{ .value = 6.0, .name = "6X"},
}};

std::array<value_t, 6> MinimapLocations = {{
	{ .value = 0.0, .name = "Left Top"},
	{ .value = 1.0, .name = "Left Middle"},
	{ .value = 2.0, .name = "Left Bottom"},
	{ .value = 3.0, .name = "Right Top"},
	{ .value = 4.0, .name = "Right Middle"},
	{ .value = 5.0, .name = "Right Bottom"},
}};

std::array<menuitem_t, 21> AutomapItems = {{
	{ .type = discrete, .label = "Rotate automap",		.a = {.cvar = &am_rotate},		   	.b = {.leftval = ARRAY_LENGTH(OnOff)}, .c = {.rightval = 0.0},	.d = {.step = 0.0},  .e = {.values = OnOff.data()}},
	{ .type = discrete, .label = "Overlay automap",		.a = {.cvar = &am_overlay},			.b = {.leftval = ARRAY_LENGTH(Overlays)}, .c = {.rightval = 0.0},	.d = {.step = 0.0},  .e = {.values = Overlays.data()}},
	{ .type = redtext,	.label = " ",					.a = {.cvar = nullptr},					.b = {.leftval = 0.0}, .c = {.rightval = 0.0},	.d = {.step = 0.0}, .e = {.values = nullptr}},
	{ .type = discrete, .label = "Line Thickeness",		.a = {.cvar = &am_thickness},		.b = {.leftval = ARRAY_LENGTH(AutomapScales)}, .c = {.rightval = 0.0},	.d = {.step = 0.0},  .e = {.values = AutomapScales.data()}},
	{ .type = redtext,	.label = " ",					.a = {.cvar = nullptr},					.b = {.leftval = 0.0}, .c = {.rightval = 0.0},	.d = {.step = 0.0}, .e = {.values = nullptr}},
	{ .type = discrete, .label = "Show item count",		.a = {.cvar = &am_showitems},		.b = {.leftval = ARRAY_LENGTH(OnOff)}, .c = {.rightval = 0.0},	.d = {.step = 0.0},  .e = {.values = OnOff.data()}},
	{ .type = discrete, .label = "Show monster count",	.a = {.cvar = &am_showmonsters},		.b = {.leftval = ARRAY_LENGTH(OnOff)}, .c = {.rightval = 0.0},	.d = {.step = 0.0},	.e = {.values = OnOff.data()}},
	{ .type = discrete, .label = "Show secrets count",	.a = {.cvar = &am_showsecrets},	   	.b = {.leftval = ARRAY_LENGTH(OnOff)}, .c = {.rightval = 0.0},	.d = {.step = 0.0},  .e = {.values = OnOff.data()}},
	{ .type = discrete, .label = "Show map timer", 	    .a = {.cvar = &am_showtime}, 	   	.b = {.leftval = ARRAY_LENGTH(OnOff)}, .c = {.rightval = 0.0},	.d = {.step = 0.0},  .e = {.values = OnOff.data()}},
	{ .type = discrete, .label = "Map name style",       .a = {.cvar = &am_classicmapstring},	.b = {.leftval = ARRAY_LENGTH(ClassicMapStringTypes)}, .c = {.rightval = 0.0},	.d = {.step = 0.0},  .e = {.values = ClassicMapStringTypes.data()}},

	{ .type = redtext,	.label = " ",					.a = {.cvar = nullptr},					.b = {.leftval = 0.0}, .c = {.rightval = 0.0},	.d = {.step = 0.0}, .e = {.values = nullptr}},
	{ .type = yellowtext, .label = "Automap Colors",		.a = {.cvar = nullptr},					.b = {.leftval = 0.0}, .c = {.rightval = 0.0},	.d = {.step = 0.0}, .e = {.values = nullptr}},
	{ .type = discrete, .label = "Highlight locked doors",.a = {.cvar = &am_showlocked},		.b = {.leftval = ARRAY_LENGTH(OnOff)}, .c = {.rightval = 0.0},	.d = {.step = 0.0},  .e = {.values = OnOff.data()}},
	{ .type = discrete, .label = "Custom map colors",	.a = {.cvar = &am_usecustomcolors},	.b = {.leftval = ARRAY_LENGTH(OnOff)}, .c = {.rightval = 0.0},	.d = {.step = 0.0},  .e = {.values = OnOff.data()}},
	{ .type = more,     .label = "Reset custom map colors",  .a = {.cvar = nullptr},    .b = {.leftval = 0.0}, .c = {.rightval = 0.0},   .d = {.step = 0.0},  .e = {.mfunc = ResetCustomColors}},

	{ .type = redtext,	.label = " ",					.a = {.cvar = nullptr},					.b = {.leftval = 0.0}, .c = {.rightval = 0.0},	.d = {.step = 0.0},  .e = {.values = nullptr}},
	{ .type = yellowtext, .label = "Overlay Minimap Options", .a = {.cvar = nullptr},			.b = {.leftval = 0.0}, .c = {.rightval = 0.0},	.d = {.step = 0.0},  .e = {.values = nullptr}},
	{ .type = discrete, .label = "Enable Minimap",		.a = {.cvar = &am_ovminimap},		.b = {.leftval = ARRAY_LENGTH(OnOff)}, .c = {.rightval = 0.0},	.d = {.step = 0.0},  .e = {.values = OnOff.data()}},
	{ .type = discrete, .label = "Location",				.a = {.cvar = &am_ovlocation},		.b = {.leftval = ARRAY_LENGTH(MinimapLocations)}, .c = {.rightval = 0.0},	.d = {.step = 0.0},  .e = {.values = MinimapLocations.data()}},
	{ .type = slider,	.label = "Scale Width",			.a = {.cvar = &am_ovscalewidth},		.b = {.leftval = 0.0}, .c = {.rightval = 1.0},	.d = {.step = 0.05}, .e = {.values = nullptr}},
	{ .type = slider,	.label = "Scale Height",			.a = {.cvar = &am_ovscaleheight},	.b = {.leftval = 0.0}, .c = {.rightval = 1.0},	.d = {.step = 0.05}, .e = {.values = nullptr}},
}};
// NOLINTEND(readability-magic-numbers)
} // namespace


menu_t AutomapMenu = {
	"M_AUTOMP",
	0,
	AutomapItems.size(),
	0,
	AutomapItems.data(),
	0,
	0,
	nullptr
};


/*=======================================
 *
 * Video Modes Menu
 *
 *=======================================*/

// Tick at which the mode test expires, on the same clock as I_MSTime.
dtime_t testingmode;
namespace
{
		// Holds time to revert to old mode

bool GetSelectedSize(int line, int *width, int *height);
} // namespace


EXTERN_CVAR (vid_widescreen)
EXTERN_CVAR (vid_maxfps)

EXTERN_CVAR (vid_overscan)
EXTERN_CVAR (vid_fullscreen)
EXTERN_CVAR (vid_32bpp)
EXTERN_CVAR(vid_vsync)

static uint16_t old_width, old_height;
namespace
{


void SetModesMenu(int w, int h);

void M_SetVideoMode(uint16_t width, uint16_t height)
{
	old_width = I_GetVideoWidth();
	old_height = I_GetVideoHeight();

	AddCommandString(fmt::format("vid_setmode {} {}", width, height));

	SetModesMenu(width, height);
}
} // namespace



void M_RestoreVideoMode()
{
	testingmode = 0;
	M_SetVideoMode(old_width, old_height);
}

namespace
{

constexpr int MAX_LINES_ONSCREEN = 22;
std::array<value_t, MAX_LINES_ONSCREEN> Depths;

#ifdef GCONSOLE
constexpr const char VMEnterText[] = "Press A to set mode";
constexpr const char VMTestText[] = "Press X to test mode for 5 seconds";
#else
constexpr const char VMEnterText[] = "Press ENTER to set mode";
constexpr const char VMTestText[] = "Press T to test mode for 5 seconds";
#endif

constexpr const char VMTestWaitText[] = "Please wait 5 seconds...";

// NOLINTBEGIN(readability-magic-numbers) - the numbers are the data
std::array<value_t, 10> VidFPSCaps = {{
	{ .value = 35.0,	.name = "35fps"},
	{ .value = 60.0,	.name = "60fps"},
	{ .value = 70.0,	.name = "70fps"},
	{ .value = 90.0,	.name = "90fps"},
	{ .value = 105.0,	.name = "105fps"},
	{ .value = 120.0,	.name = "120fps"},
	{ .value = 140.0,	.name = "140fps"},
	{ .value = 144.0,	.name = "144fps"},
	{ .value = 240.0,	.name = "240fps"},
	{ .value = 0.0,		.name = "Unlimited"}
}};

std::array<value_t, 3> FullScreenOptions = {{
	{ .value = WINDOW_Windowed,			.name = "Window"},
	{ .value = WINDOW_Fullscreen,		.name = "Full Screen Exclusive"},
	{ .value = WINDOW_DesktopFullscreen,	.name = "Full Screen Window"}
}};

std::array<value_t, 6> WidescreenMode = {{
	{ .value = 0.0,			.name = "Off"},
	{ .value = 1.0,			.name = "Auto"},
	{ .value = 2.0,			.name = "16:10"},
	{ .value = 3.0,			.name = "16:9"},
	{ .value = 4.0,			.name = "21:9"},
	{ .value = 5.0,			.name = "32:9"}
}};

// Sized by the initializer: entries here are conditionally compiled,
// so a fixed std::array size would be wrong on some builds.
// NOLINTNEXTLINE(modernize-avoid-c-arrays)
menuitem_t ModesItems[] = {
#ifdef GCONSOLE
	{ .type = slider, .label = "Overscan",				.a = {.cvar = &vid_overscan},		.b = {.leftval = 0.84375}, .c = {.rightval = 1.0}, .d = {.step = 0.03125}, .e = {.values = nullptr}},
#else
	{ .type = discrete, .label = "Fullscreen",			.a = {.cvar = &vid_fullscreen},		.b = {.leftval = ARRAY_LENGTH(FullScreenOptions)}, .c = {.rightval = 0.0},	.d = {.step = 0.0}, .e = {.values = FullScreenOptions.data()}},
#endif
	{ .type = discrete,	.label = "Widescreen",			.a = {.cvar = &vid_widescreen},		.b = {.leftval = ARRAY_LENGTH(WidescreenMode)}, .c = {.rightval = 0.0},	.d = {.step = 0.0}, .e = {.values = WidescreenMode.data()}} ,
	{ .type = discrete,	.label = "VSync",				.a = {.cvar = &vid_vsync},			.b = {.leftval = ARRAY_LENGTH(YesNo)}, .c = {.rightval = 0.0},	.d = {.step = 0.0}, .e = {.values = YesNo.data()}},
	{ .type = discrete, .label = "Framerate",			.a = {.cvar = &vid_maxfps},			.b = {.leftval = ARRAY_LENGTH(VidFPSCaps)}, .c = {.rightval = 0.0},	.d = {.step = 0.0}, .e = {.values = VidFPSCaps.data()}},
	{ .type = discrete, .label = "32-bit color",			.a = {.cvar = &vid_32bpp},			.b = {.leftval = ARRAY_LENGTH(YesNo)}, .c = {.rightval = 0.0},	.d = {.step = 0.0}, .e = {.values = YesNo.data()}},
	{ .type = redtext,	.label = "",						.a = {.cvar = nullptr},					.b = {.leftval = 0.0}, .c = {.rightval = 0.0},	.d = {.step = 0.0}, .e = {.values = nullptr}},
	{ .type = screenres, .label = nullptr,					.a = {.cvar = nullptr},					.b = {.leftval = 0.0}, .c = {.rightval = 0.0},	.d = {.step = 0.0}, .e = {.values = nullptr}},
	{ .type = screenres, .label = nullptr,					.a = {.cvar = nullptr},					.b = {.leftval = 0.0}, .c = {.rightval = 0.0},	.d = {.step = 0.0}, .e = {.values = nullptr}},
	{ .type = screenres, .label = nullptr,					.a = {.cvar = nullptr},					.b = {.leftval = 0.0}, .c = {.rightval = 0.0},	.d = {.step = 0.0}, .e = {.values = nullptr}},
	{ .type = screenres, .label = nullptr,					.a = {.cvar = nullptr},					.b = {.leftval = 0.0}, .c = {.rightval = 0.0},	.d = {.step = 0.0}, .e = {.values = nullptr}},
	{ .type = screenres, .label = nullptr,					.a = {.cvar = nullptr},					.b = {.leftval = 0.0}, .c = {.rightval = 0.0},	.d = {.step = 0.0}, .e = {.values = nullptr}},
	{ .type = screenres, .label = nullptr,					.a = {.cvar = nullptr},					.b = {.leftval = 0.0}, .c = {.rightval = 0.0},	.d = {.step = 0.0}, .e = {.values = nullptr}},
	{ .type = screenres, .label = nullptr,					.a = {.cvar = nullptr},					.b = {.leftval = 0.0}, .c = {.rightval = 0.0},	.d = {.step = 0.0}, .e = {.values = nullptr}},
	{ .type = screenres, .label = nullptr,					.a = {.cvar = nullptr},					.b = {.leftval = 0.0}, .c = {.rightval = 0.0},	.d = {.step = 0.0}, .e = {.values = nullptr}},
	{ .type = redtext,	.label = " ",					.a = {.cvar = nullptr},					.b = {.leftval = 0.0}, .c = {.rightval = 0.0},	.d = {.step = 0.0}, .e = {.values = nullptr}},
	{ .type = whitetext, .label = " ",					.a = {.cvar = nullptr},					.b = {.leftval = 0.0}, .c = {.rightval = 0.0},	.d = {.step = 0.0}, .e = {.values = nullptr}},
	{ .type = redtext,	.label = " ",					.a = {.cvar = nullptr},					.b = {.leftval = 0.0}, .c = {.rightval = 0.0},	.d = {.step = 0.0}, .e = {.values = nullptr}},
	{ .type = yellowtext, .label = " ",					.a = {.cvar = nullptr},					.b = {.leftval = 0.0}, .c = {.rightval = 0.0},	.d = {.step = 0.0}, .e = {.values = nullptr}},
};
// NOLINTEND(readability-magic-numbers)

}

#define VM_DEPTHITEM	0
#define VM_RESSTART		6
#define VM_ENTERLINE	15
#define VM_TESTLINE		17

menu_t ModesMenu = {
	"M_VIDMOD",
	0,
	ARRAY_LENGTH(ModesItems),
	130,
	ModesItems,
	0,
	0,
	nullptr
};
namespace
{


void BuildModesList(int hiwidth, int hiheight)
{
	// gathers a list of unique resolutions availible for the current
	// screen mode (windowed or fullscreen)
	const bool fullscreen = I_GetWindow()->getVideoMode().isFullScreen();

	typedef std::vector< std::pair<uint16_t, uint16_t> > MenuModeList;
	MenuModeList menumodelist;

	const IVideoModeList* videomodelist = I_GetVideoCapabilities()->getSupportedVideoModes();
	for (const auto& mode : *videomodelist)
		if (mode.isFullScreen() == fullscreen)
			menumodelist.emplace_back(mode.width, mode.height);
	menumodelist.erase(std::unique(menumodelist.begin(), menumodelist.end()), menumodelist.end());

	MenuModeList::const_iterator mode_it = menumodelist.begin();

	char** str = nullptr;

	for (int i = VM_RESSTART; ModesItems[i].type == screenres; i++)
	{
		ModesItems[i].e.highlight = -1;
		for (int col = 0; col < 3; col++)
		{
			if (col == 0)
				str = &ModesItems[i].b.res1;
			else if (col == 1)
				str = &ModesItems[i].c.res2;
			else if (col == 2)
				str = &ModesItems[i].d.res3;

			if (mode_it != menumodelist.end())
			{
				auto [width, height] = *mode_it;
				++mode_it;

				if (width == hiwidth && height == hiheight)
					ModesItems[i].e.highlight = ModesItems[i].a.selmode = col;

				char strtemp[32];
				snprintf(strtemp, 32, "%dx%d", width, height);
				ReplaceString(str, strtemp);
			}
			else
			{
				str = nullptr;
			}
		}
	}
}
} // namespace


void M_RefreshModesList()
{
	BuildModesList(I_GetVideoWidth(), I_GetVideoHeight());
}
namespace
{


bool GetSelectedSize(int line, int* width, int* height)
{
	if (ModesItems[line].type != screenres)
		return false;

	const int mode_num = ((line - VM_RESSTART) * 3) + ModesItems[line].a.selmode;

	const char* resolution_str = nullptr;

	if (mode_num % 3 == 0)
		resolution_str = ModesItems[line].b.res1;
	else if (mode_num % 3 == 1)
		resolution_str = ModesItems[line].c.res2;
	else if (mode_num % 3 == 2)
		resolution_str = ModesItems[line].d.res3;

	if (!resolution_str)
		return false;

	const std::string_view resolution(resolution_str);
	const size_t xpos = resolution.find_first_of("xX");
	if (xpos == std::string_view::npos)
		return false;

	const std::string_view width_str = resolution.substr(0, xpos);
	const std::string_view height_str = resolution.substr(xpos + 1);
	if (width_str.empty() || height_str.empty())
		return false;

	const std::optional<int> parsed_width = ParseNum<int>(width_str);
	const std::optional<int> parsed_height = ParseNum<int>(height_str);
	if (!parsed_width || !parsed_height)
		return false;

	*width = *parsed_width;
	*height = *parsed_height;

	return true;
}

void SetModesMenu(int w, int h)
{
	if (!testingmode)
	{
		ModesItems[VM_ENTERLINE].label = VMEnterText;
		ModesItems[VM_TESTLINE].label = VMTestText;
	}
	else
	{
		static char enter_text[64];
		snprintf(enter_text, 64, "TESTING %dx%d", w, h);

		ModesItems[VM_ENTERLINE].label = enter_text;
		ModesItems[VM_TESTLINE].label = VMTestWaitText;
	}

	BuildModesList(w, h);
}
} // namespace


//
// M_ModeFlashTestText
//
// Flashes the video mode testing text
//
void M_ModeFlashTestText()
{
	if (I_MSTime() & 256)
		ModesItems[VM_TESTLINE].label = VMTestWaitText;
	else
		ModesItems[VM_TESTLINE].label = "";
}
namespace
{


void SetVidMode()
{
	SetModesMenu(I_GetVideoWidth(), I_GetVideoHeight());

	if (ModesMenu.items[ModesMenu.lastOn].type == screenres)
	{
		if (ModesMenu.items[ModesMenu.lastOn].a.selmode == -1)
			ModesMenu.items[ModesMenu.lastOn].a.selmode++;
	}
	M_SwitchMenu(&ModesMenu);
}



cvar_t *flagsvar;
} // namespace


EXTERN_CVAR(ui_dimcolor)

namespace
{

// [Russell] - Modified to send new colours
void M_SendUINewColor (int red, int green, int blue)
{
	AddCommandString(fmt::format("ui_dimcolor \"{:02} {:02x} {:02}\"", red, green, blue));
}

} // namespace
namespace
{


void M_SlideUIRed(int val)
{
	argb_t color = V_GetColorFromString(ui_dimcolor);
	color.setr(val);
	M_SendUINewColor(color.getr(), color.getg(), color.getb());
}

void M_SlideUIGreen (int val)
{
	argb_t color = V_GetColorFromString(ui_dimcolor);
	color.setg(val);
	M_SendUINewColor(color.getr(), color.getg(), color.getb());
}

void M_SlideUIBlue (int val)
{
	argb_t color = V_GetColorFromString(ui_dimcolor);
	color.setb(val);
	M_SendUINewColor(color.getr(), color.getg(), color.getb());
}
} // namespace



//
//		Set some stuff up for the video modes menu
//

void M_OptInit()
{
	for (size_t i = 0; i < Depths.size(); i++)
	{
		Depths[i].value = i;
		Depths[i].name = nullptr;
	}

	switch (I_GetVideoCapabilities()->getDisplayType())
	{
	// FIXME: this is overriding widescreen even though both fullscreen and windowed
	// should be allowed to toggle it
	case DISPLAY_FullscreenOnly:
		ModesItems[2].type = nochoice;
		ModesItems[2].b.leftval = 1.f;
		break;
	case DISPLAY_WindowOnly:
		ModesItems[2].type = nochoice;
		ModesItems[2].b.leftval = 0.f;
		break;
	default:
		break;
	}
}


//
//		Toggle messages on/off
//
void M_ChangeMessages()
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

void M_SizeDisplay (float diff)
{
	// changing screenblocks automatically resizes the display
	screenblocks.Set (screenblocks + diff);
}

BEGIN_COMMAND (sizedown)
{
	M_SizeDisplay (-1.0);
	S_Sound (CHAN_INTERFACE, "menu/change", 1, ATTN_NONE);
}
END_COMMAND (sizedown)

BEGIN_COMMAND (sizeup)
{
	M_SizeDisplay(1.0);
	S_Sound (CHAN_INTERFACE, "menu/change", 1, ATTN_NONE);
}
END_COMMAND (sizeup)

void M_BuildKeyList (menuitem_t *item, int numitems)
{
	int i;

	for (i = 0; i < numitems; i++, item++)
	{
		if (item->type == control)
			Bindings.GetKeysForCommand (item->e.command, &item->b.key1, &item->c.key2);
		if (item->type == mapcontrol)
			AutomapBindings.GetKeysForCommand(item->e.command, &item->b.key1, &item->c.key2);
		if (item->type == netdemocontrol)
			NetDemoBindings.GetKeysForCommand(item->e.command, &item->b.key1, &item->c.key2);
	}
}

void M_SwitchMenu(menu_t* menu)
{
	MenuStack[MenuStackDepth].menu.newmenu = menu;
	MenuStack[MenuStackDepth].isNewStyle = true;
	MenuStack[MenuStackDepth].drawSkull = false;
	MenuStackDepth++;

	CanScrollUp = false;
	CanScrollDown = false;
	CurrentMenu = menu;
	CurrentItem = menu->lastOn;

	if (!menu->indent)
	{
		int widest = 0;
		for (int i = 0; i < menu->numitems; i++)
		{
			const menuitem_t* item = menu->items + i;
			if (item->type != whitetext && item->type != redtext && item->type != orangetext)
			{
				const int thiswidth = V_StringWidth (item->label);
				widest = std::max(thiswidth, widest);
			}
		}
		menu->indent = widest + 6;
	}

	flagsvar = nullptr;
}

bool M_StartOptionsMenu()
{
	M_SwitchMenu (&OptionMenu);
	return true;
}

void M_DrawSlider (int x, int y, float leftval, float rightval, float cur, float step)
{
	if (leftval < rightval)
		cur = std::clamp(cur, leftval, rightval);
	else
		cur = std::clamp(cur, rightval, leftval);

	const float dist = (cur - leftval) / (rightval - leftval);

	screen->DrawPatchClean (W_CachePatch ("LSLIDE"), x, y);
	for (int i = 1; i < 11; i++)
		screen->DrawPatchClean (W_CachePatch ("MSLIDE"), x + (i*8), y);
	screen->DrawPatchClean (W_CachePatch ("RSLIDE"), x + 88, y);

	screen->DrawPatchClean (W_CachePatch ("CSLIDE"), x + 5 + static_cast<int>(dist * 78.0), y);

	std::string buf;
	if (step == 0.0f)
		return;
	if (step >= 1.0f)
		buf = fmt::sprintf("%.0f", cur);
	else if (step >= 0.1f)
		buf = fmt::sprintf("%.1f", cur);
	else
		buf = fmt::sprintf("%.2f", cur);
	screen->DrawTextCleanMove(CR_GREEN, x + 96, y, buf.c_str());
}

void M_DrawColoredSlider(int x, int y, float leftval, float rightval, float cur, argb_t color)
{
	if (leftval < rightval)
		cur = std::clamp(cur, leftval, rightval);
	else
		cur = std::clamp(cur, rightval, leftval);

	const float dist = (cur - leftval) / (rightval - leftval);

	screen->DrawPatchClean(W_CachePatch ("LSLIDE"), x, y);

	for (int i = 1; i < 11; i++)
		screen->DrawPatchClean (W_CachePatch ("MSLIDE"), x + (i*8), y);

	screen->DrawPatchClean (W_CachePatch ("RSLIDE"), x + 88, y);

	screen->DrawPatchClean (W_CachePatch ("GSLIDE"), x + 5 + static_cast<int>(dist * 78.0), y);

	V_ColorFill = V_BestColor(V_GetDefaultPalette()->basecolors, color);

	screen->DrawColoredPatchClean(W_CachePatch("OSLIDE"), x + 5 + static_cast<int>(dist * 78.0), y);
}

int M_FindCurVal (float cur, value_t *values, int numvals)
{
	int v;

	for (v = 0; v < numvals; v++)
		if (values[v].value == cur)
			break;

	return v;
}

void M_OptDrawer()
{
	int color;
	int y;
	int width;
	int i;
	int x;
	int ytop;
	const int theight = 0;
	menuitem_t *item;
	patch_t *title;

	const int x1 = (I_GetSurfaceWidth() / 2)-(160*CleanXfac);
	const int y1 = (I_GetSurfaceHeight() / 2)-(100*CleanYfac);

	const int x2 = (I_GetSurfaceWidth() / 2)+(160*CleanXfac);
	const int y2 = (I_GetSurfaceHeight() / 2)+(100*CleanYfac);

	// Background effect
	OdamexEffect(x1,y1,x2,y2);

	title = W_CachePatch (CurrentMenu->title);
	screen->DrawPatchClean (title, MENU_CENTER_X - (title->width() / 2), MENU_TITLE_Y);

	y = 15 + title->height();
	ytop = y + (CurrentMenu->scrolltop * 8);

	OptMouseRowCount = 0;

	for (i = 0; i < CurrentMenu->numitems && y <= 192 - theight; i++, y += 8)	// TIJ
	{
		if (i == CurrentMenu->scrolltop)
			i += CurrentMenu->scrollpos;

		item = CurrentMenu->items + i;

		if (OptMouseRowCount < MAX_OPT_MOUSE_ROWS)
		{
			optmouserow_t& row = OptMouseRows[OptMouseRowCount++];
			row.item = i;
			row.y1 = screen->getCleanY(y);
			row.y2 = screen->getCleanY(y + 8);
		}

		if (item->type == screenres)
		{
			const char *str = nullptr;

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
						color = CR_GREY;
					else
						color = CR_RED;

					screen->DrawTextCleanMove (color, (RESCOLUMN_WIDTH * x) + RESCOLUMN_TEXT_X, y, str);
				}
			}

			if (i == CurrentItem && ((item->a.selmode != -1 && (skullAnimCounter < 6 || WaitingForKey))
				|| WaitingForAxis || testingmode))
				screen->DrawPatchClean (W_CachePatch ("LITLCURS"),
				                        (item->a.selmode * RESCOLUMN_WIDTH) + RESCOLUMN_CURSOR_X, y);
		}
		else
		{
			width = V_StringWidth (item->label);
			switch (item->type)
			{
			case more:
				x = CurrentMenu->indent - width;
				color = CR_GREY;
				break;

			case redtext:
				x = MENU_CENTER_X - (width / 2);
				color = CR_RED;
				break;

			case whitetext:
				x = MENU_CENTER_X - (width / 2);
				color = CR_GREY;
				break;

			case yellowtext:
				x = MENU_CENTER_X - (width / 2);
				color = CR_YELLOW;
				break;

			case orangetext:
				x = MENU_CENTER_X - (width / 2);
				color = CR_ORANGE;
				break;

			case listelement:
				x = CurrentMenu->indent + 14;
				color = CR_RED;
				break;

			default:
				x = CurrentMenu->indent - width;
				color = CR_RED;
				break;
			}
			screen->DrawTextCleanMove (color, x, y, item->label);

			switch (item->type)
			{
			case discrete:
			case cdiscrete:
			case svdiscrete:
			{
				int v;
				int vals;

				vals = static_cast<int>(item->b.leftval);
				v = M_FindCurVal(item->a.cvar->value(), item->e.values, vals);

				if (v == vals)
				{
					screen->DrawTextCleanMove(CR_GREY, CurrentMenu->indent + 14, y, "Unknown");
				}
				else
				{
					int color_num = CR_GREY;
					if (item->type == cdiscrete)
						color_num = item->a.cvar->asInt();

					screen->DrawTextCleanMove(color_num, CurrentMenu->indent + 14, y, item->e.values[v].name);
				}

			}
			break;

			case nochoice:
				screen->DrawTextCleanMove (CR_GOLD, CurrentMenu->indent + 14, y,
										   (item->e.values[static_cast<int>(item->b.leftval)]).name);
				break;

			case slider:
				M_DrawSlider (CurrentMenu->indent + 8, y, item->b.leftval, item->c.rightval, item->a.cvar->value(), item->d.step);
				break;

			case redslider:
			{
				const argb_t color = V_GetColorFromString(*item->a.cvar);
				M_DrawColoredSlider(CurrentMenu->indent + 8, y, 0, 255, color.getr(), color);
			}
			break;
			case greenslider:
			{
				const argb_t color = V_GetColorFromString(*item->a.cvar);
				M_DrawColoredSlider(CurrentMenu->indent + 8, y, 0, 255, color.getg(), color);
			}
			break;
			case blueslider:
			{
				const argb_t color = V_GetColorFromString(*item->a.cvar);
				M_DrawColoredSlider(CurrentMenu->indent + 8, y, 0, 255, color.getb(), color);
			}
			break;

			case control:
			{
				const std::string desc = Bindings.GetNameKeys(item->b.key1, item->c.key2);
				screen->DrawTextCleanMove (CR_GREY, CurrentMenu->indent + 14, y, desc.c_str());
			}
			break;

			case mapcontrol:
			{
				const std::string desc = AutomapBindings.GetNameKeys(item->b.key1, item->c.key2);
				screen->DrawTextCleanMove(CR_GREY, CurrentMenu->indent + 14, y, desc.c_str());
			}
			break;

			case netdemocontrol:
			{
				const std::string desc = NetDemoBindings.GetNameKeys(item->b.key1, item->c.key2);
				screen->DrawTextCleanMove(CR_GREY, CurrentMenu->indent + 14, y, desc.c_str());
			}
			break;

			case bitflag:
			{
				value_t *value;
				const char *str;

				if (item->b.leftval)
					value = NoYes.data();
				else
					value = YesNo.data();

				if (*item->e.flagint & item->a.flagmask)
					str = value[1].name;
				else
					str = value[0].name;

				screen->DrawTextCleanMove (CR_GREY, CurrentMenu->indent + 14, y, str);
			}
			break;

			case joyactive:
			{
				std::string joyname;

				const size_t numjoy = I_GetJoystickCount();

				if(static_cast<size_t>(item->a.cvar->value()) > numjoy)
					item->a.cvar->Set(0.0);

				if(!numjoy)
					joyname = "No device detected";
				else
				{
					joyname = item->a.cvar->str();
					joyname += ": " + I_GetJoystickNameFromIndex(item->a.cvar->asInt());
				}

				screen->DrawTextCleanMove (CR_GREY, CurrentMenu->indent + 14, y, joyname.c_str());
			}
			break;

			case joyaxis:
			{
				screen->DrawTextCleanMove (CR_GREY, CurrentMenu->indent + 14, y, item->a.cvar->cstring());
			}
			break;

			default:
				break;
			}

			if (i == CurrentItem && (skullAnimCounter < 6 || WaitingForKey || WaitingForAxis))
			{
				screen->DrawPatchClean (W_CachePatch ("LITLCURS"), CurrentMenu->indent + 3, y);
			}
		}
	}

	VisBottom = i - 1;
	CanScrollUp = (CurrentMenu->scrollpos != 0);
	CanScrollDown = (i < CurrentMenu->numitems);

	if (CanScrollUp)
		screen->DrawPatchClean (W_CachePatch ("LITLUP"), 3, ytop);

	if (CanScrollDown)
		screen->DrawPatchClean (W_CachePatch ("LITLDN"), 3, 190);
}
namespace
{


//
// M_OptItemSelectable
//
// Matches the item types the up/down key handlers skip over.
//
bool M_OptItemSelectable(const menuitem_t* item)
{
	switch (item->type)
	{
	case redtext:
	case whitetext:
	case yellowtext:
	case orangetext:
		return false;
	case screenres:
		return item->b.res1 != nullptr;
	default:
		return true;
	}
}


//
// M_OptRowUnderMouse
//
// Returns an index into OptMouseRows, or -1 if the cursor isn't over a row.
//
int M_OptRowUnderMouse(int mouse_y)
{
	for (int i = 0; i < OptMouseRowCount; i++)
	{
		if (mouse_y >= OptMouseRows[i].y1 && mouse_y < OptMouseRows[i].y2)
			return i;
	}

	return -1;
}


//
// M_OptScreenResColumn
//
// Returns which of the three resolution columns the cursor is over.
//
int M_OptScreenResColumn(int mouse_x)
{
	for (int col = 2; col > 0; col--)
	{
		if (mouse_x >= screen->getCleanX((RESCOLUMN_WIDTH * col) + RESCOLUMN_CURSOR_X))
			return col;
	}

	return 0;
}


//
// M_OptScroll
//
// Scrolls the visible portion of the menu without moving the selection.
//
void M_OptScroll(int lines)
{
	if (lines < 0 && CanScrollUp)
	{
		CurrentMenu->scrollpos += lines;
		if (CurrentMenu->scrollpos < 0)
			CurrentMenu->scrollpos = 0;
	}
	else if (lines > 0 && CanScrollDown)
	{
		const int pagesize = VisBottom - CurrentMenu->scrollpos - CurrentMenu->scrolltop;
		CurrentMenu->scrollpos += lines;
		if (CurrentMenu->scrollpos + CurrentMenu->scrolltop + pagesize > CurrentMenu->numitems)
			CurrentMenu->scrollpos = CurrentMenu->numitems - CurrentMenu->scrolltop - pagesize;
	}
}


//
// M_OptSetSliderFromMouse
//
// Sets a slider to the value the cursor was clicked at.
//
void M_OptSetSliderFromMouse(menuitem_t* item, int mouse_x)
{
	const int x = CurrentMenu->indent + 8;
	const int track_x1 = screen->getCleanX(x + SLIDER_TRACK_X);
	const int track_x2 = screen->getCleanX(x + SLIDER_TRACK_X + SLIDER_TRACK_WIDTH);

	if (track_x2 <= track_x1)
		return;

	float dist = static_cast<float>(mouse_x - track_x1) / static_cast<float>(track_x2 - track_x1);
	dist = std::clamp(dist, 0.0f, 1.0f);

	if (item->type == slider)
	{
		float newval = item->b.leftval + (dist * (item->c.rightval - item->b.leftval));

		// Snap to the item's step so clicking produces the same set of values
		// the arrow keys do.
		if (item->d.step != 0.0f)
		{
			const float step = (item->c.rightval >= item->b.leftval) ? item->d.step : -item->d.step;
			newval = item->b.leftval +
			         (step * std::floor(((newval - item->b.leftval) / step) + 0.5f));
		}

		if (item->b.leftval < item->c.rightval)
			newval = std::clamp(newval, item->b.leftval, item->c.rightval);
		else
			newval = std::clamp(newval, item->c.rightval, item->b.leftval);

		if (item->e.cfunc)
			item->e.cfunc(item->a.cvar, newval);
		else
			item->a.cvar->Set(newval);
	}
	else
	{
		// Color component sliders move in steps of 17
		int part = static_cast<int>(std::lround(dist * 255.0f));
		part = ((part + 0x08) / 0x11) * 0x11;
		part = std::clamp(part, 0, 0xFF);

		const char* oldcolor = item->a.cvar->cstring();
		char newcolor[9];

		if (strlen(oldcolor) == 8)
			memcpy(newcolor, oldcolor, 9);
		else
			memcpy(newcolor, "00 00 00", 9);

		char singlecolor[3];
		snprintf(singlecolor, 3, "%02x", part);

		if (item->type == redslider)
			memcpy(newcolor, singlecolor, 2);
		else if (item->type == greenslider)
			memcpy(newcolor + 3, singlecolor, 2);
		else if (item->type == blueslider)
			memcpy(newcolor + 6, singlecolor, 2);

		item->a.cvar->Set(newcolor);
	}
}


//
// M_OptItemIsSlider
//
bool M_OptItemIsSlider(const menuitem_t* item)
{
	return item->type == slider || item->type == redslider ||
	       item->type == greenslider || item->type == blueslider;
}


//
// M_OptMouseClick
//
void M_OptMouseClick(int mouse_x, int mouse_y)
{
	const int row = M_OptRowUnderMouse(mouse_y);
	if (row == -1)
		return;

	const int index = OptMouseRows[row].item;
	menuitem_t* item = CurrentMenu->items + index;

	if (!M_OptItemSelectable(item))
		return;

	// The resolution list keeps its column in the item being left behind
	if (CurrentMenu->items[CurrentItem].type == screenres && CurrentItem != index)
		CurrentMenu->items[CurrentItem].a.selmode = -1;

	CurrentItem = index;

	if (item->type == screenres)
		item->a.selmode = M_OptScreenResColumn(mouse_x);

	if (M_OptItemIsSlider(item))
	{
		M_OptSetSliderFromMouse(item, mouse_x);
		S_Sound(CHAN_INTERFACE, "menu/change", 1, ATTN_NONE);

		// Keep following the pointer until the button is released
		OptDragItem = index;
		OptDragMenu = CurrentMenu;
		return;
	}

	bool cycles_value = false;
	switch (item->type)
	{
	case discrete:
	case cdiscrete:
	case svdiscrete:
	case bitflag:
	case joyactive:
		cycles_value = true;
		break;
	default:
		break;
	}

	// Everything else behaves exactly as though the accept key was pressed.
	const event_t synth_ev(ev_keydown, cycles_value ? OKEY_RIGHTARROW : OKEY_ENTER, 0, 0, 0);
	M_OptResponder(synth_ev);
}
} // namespace



//
// M_OptUpdateMouseItem
//
// Moves the selection to whatever the cursor is hovering over. Only acts when
// the pointer actually moves so that a resting cursor doesn't fight the
// keyboard.
//
void M_OptUpdateMouseItem()
{
	static int prev_mouse_x = -1;
	static int prev_mouse_y = -1;

	if (ui_mouse.asInt() == 0 || WaitingForKey || WaitingForAxis)
		return;

	if (OptDragItem != -1 &&
	    (OptDragMenu != CurrentMenu || OptDragItem >= CurrentMenu->numitems ||
	     !M_OptItemIsSlider(CurrentMenu->items + OptDragItem) ||
	     !I_IsUIMouseButtonDown(OKEY_MOUSE1)))
		OptDragItem = -1;

	int mouse_x;
	int mouse_y;
	if (!I_GetUIMousePosition(mouse_x, mouse_y))
		return;

	const bool moved = (mouse_x != prev_mouse_x || mouse_y != prev_mouse_y);
	prev_mouse_x = mouse_x;
	prev_mouse_y = mouse_y;

	if (!moved)
		return;

	if (OptDragItem != -1)
	{
		M_OptSetSliderFromMouse(CurrentMenu->items + OptDragItem, mouse_x);
		return;
	}

	const int row = M_OptRowUnderMouse(mouse_y);
	if (row == -1)
		return;

	const int index = OptMouseRows[row].item;
	if (index == CurrentItem || !M_OptItemSelectable(CurrentMenu->items + index))
		return;

	if (CurrentMenu->items[CurrentItem].type == screenres)
		CurrentMenu->items[CurrentItem].a.selmode = -1;

	CurrentItem = index;

	if (CurrentMenu->items[CurrentItem].type == screenres)
		CurrentMenu->items[CurrentItem].a.selmode = M_OptScreenResColumn(mouse_x);

	S_Sound(CHAN_INTERFACE, "menu/cursor", 1, ATTN_NONE);
}

void M_OptResponder(const event_t& ev)
{
	const int ch = ev.data1;
	const int mod = ev.mod;
	const char *cmd = Bindings.GetBind(ch).c_str();

	menuitem_t *item = CurrentMenu->items + CurrentItem;

	const bool numlock = (mod & OMOD_NUM) != 0;

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

	if (ui_mouse.asBool() && ev.type == ev_keydown &&
	    ch >= OKEY_MOUSE1 && ch <= OKEY_MWHEELRIGHT)
	{
		int mouse_x;
		int mouse_y;

		if (ch == OKEY_MOUSE2)
		{
			CurrentMenu->lastOn = CurrentItem;
			M_PopMenuStack();
			return;
		}
		if (ch == OKEY_MWHEELUP)
			M_OptScroll(-OPT_WHEEL_LINES);
		else if (ch == OKEY_MWHEELDOWN)
			M_OptScroll(OPT_WHEEL_LINES);
		else if (ch == OKEY_MOUSE1 && I_GetUIMousePosition(mouse_x, mouse_y))
			M_OptMouseClick(mouse_x, mouse_y);

		if (CurrentMenu->refreshfunc)
			(*CurrentMenu->refreshfunc)();
		return;
	}

	if (item->type == bitflag && flagsvar &&
	    (Key_IsLeftKey(ch, numlock) || Key_IsRightKey(ch, numlock) || Key_IsAcceptKey(ch))
		&& !demoplayback)
	{
			const int newflags = *item->e.flagint ^ item->a.flagmask;
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

			S_Sound(CHAN_INTERFACE, "menu/cursor", 1, ATTN_NONE);
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
					CurrentMenu->scrollpos = std::max(CurrentMenu->scrollpos, 0);
				}
				if (CurrentItem < 0)
				{
					CurrentMenu->scrollpos = std::max(0, CurrentMenu->numitems - MAX_LINES_ONSCREEN + CurrentMenu->scrolltop);
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

			S_Sound(CHAN_INTERFACE, "menu/cursor", 1, ATTN_NONE);
		}
		else if (Key_IsPageUpKey(ch, numlock))
		{
			if (CanScrollUp)
			{
				CurrentMenu->scrollpos -= VisBottom - CurrentMenu->scrollpos - CurrentMenu->scrolltop;
				CurrentMenu->scrollpos = std::max(CurrentMenu->scrollpos, 0);
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
				S_Sound(CHAN_INTERFACE, "menu/cursor", 1, ATTN_NONE);
			}
		}
		else if (Key_IsPageDownKey(ch, numlock))
		{
			if (CanScrollDown)
			{
				const int pagesize = VisBottom - CurrentMenu->scrollpos - CurrentMenu->scrolltop;
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
				S_Sound(CHAN_INTERFACE, "menu/cursor", 1, ATTN_NONE);
			}
		}
		else if (Key_IsLeftKey(ch, numlock))
		{
		switch (item->type)
		{
		case slider:
		{
			float newval = item->a.cvar->value() - item->d.step;

			if (item->b.leftval < item->c.rightval)
				newval = std::min(newval, item->b.leftval);
			else
				newval = std::max(newval, item->b.leftval);

			if (item->e.cfunc)
				item->e.cfunc(item->a.cvar, newval);
			else
				item->a.cvar->Set(newval);
		}
		S_Sound(CHAN_INTERFACE, "menu/change", 1, ATTN_NONE);
		break;
		case redslider:
		case greenslider:
		case blueslider:
		{
			const char* oldcolor = item->a.cvar->cstring();
			char newcolor[9];

			if (strlen(oldcolor) == 8)
				memcpy(newcolor, oldcolor, 9);
			else
				memcpy(newcolor, "00 00 00", 9);

			const argb_t color = V_GetColorFromString(oldcolor);
			int part = 0;

			if (item->type == redslider)
				part = color.getr();
			else if (item->type == greenslider)
				part = color.getg();
			else if (item->type == blueslider)
				part = color.getb();

			if (part > 0x00)
				part -= 0x11;
			part = std::max(part, 0x00);

			char singlecolor[3];
			snprintf(singlecolor, 3, "%02x", part);

			if (item->type == redslider)
				memcpy(newcolor, singlecolor, 2);
			else if (item->type == greenslider)
				memcpy(newcolor + 3, singlecolor, 2);
			else if (item->type == blueslider)
				memcpy(newcolor + 6, singlecolor, 2);

			item->a.cvar->Set(newcolor);
		}
		S_Sound(CHAN_INTERFACE, "menu/change", 1, ATTN_NONE);
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
			cur = M_FindCurVal(item->a.cvar->value(), item->e.values, numvals);
			if (--cur < 0)
				cur = numvals - 1;

			item->a.cvar->Set(item->e.values[cur].value);

			// Hack hack. Rebuild list of resolutions
			if (item->e.values == Depths.data())
				BuildModesList(I_GetVideoWidth(), I_GetVideoHeight());
		}
		S_Sound(CHAN_INTERFACE, "menu/change", 1, ATTN_NONE);
		break;

		case screenres:
		{
			int col;

			col = item->a.selmode - 1;
			if (col < 0)
			{
				if (CurrentItem > 0)
				{
					if (CurrentMenu->items[CurrentItem - 1].type == screenres)
					{
						item->a.selmode = -1;
						CurrentMenu->items[--CurrentItem].a.selmode = 2;
					}
				}
			}
			else
			{
				item->a.selmode = col;
			}
		}
		S_Sound(CHAN_INTERFACE, "menu/choose", 1, ATTN_NONE);
		break;

		case joyactive:
		{
			const size_t numjoy = I_GetJoystickCount();

			if (static_cast<size_t>(item->a.cvar->value()) > numjoy)
				item->a.cvar->Set(0.0);
			else if (static_cast<size_t>(item->a.cvar->value()) > 0)
				item->a.cvar->Set(item->a.cvar->value() - 1);
		}
		S_Sound(CHAN_INTERFACE, "menu/change", 1, ATTN_NONE);
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
			float newval = item->a.cvar->value() + item->d.step;

			if (item->b.leftval < item->c.rightval)
				newval = std::min(newval, item->c.rightval);
			else
				newval = std::max(newval, item->c.rightval);

			if (item->e.cfunc)
				item->e.cfunc(item->a.cvar, newval);
			else
				item->a.cvar->Set(newval);
		}
		S_Sound(CHAN_INTERFACE, "menu/change", 1, ATTN_NONE);
		break;
		case redslider:
		case greenslider:
		case blueslider:
		{
			const char* oldcolor = item->a.cvar->cstring();
			char newcolor[9];

			if (strlen(oldcolor) == 8)
				memcpy(newcolor, oldcolor, 9);
			else
				memcpy(newcolor, "00 00 00", 9);

			const argb_t color = V_GetColorFromString(oldcolor);
			int part = 0;

			if (item->type == redslider)
				part = color.getr();
			else if (item->type == greenslider)
				part = color.getg();
			else if (item->type == blueslider)
				part = color.getb();

			if (part < 0xff)
				part += 0x11;
			part = std::min(part, 0xff);

			char singlecolor[3];
			snprintf(singlecolor, 3, "%02x", part);

			if (item->type == redslider)
				memcpy(newcolor, singlecolor, 2);
			else if (item->type == greenslider)
				memcpy(newcolor + 3, singlecolor, 2);
			else if (item->type == blueslider)
				memcpy(newcolor + 6, singlecolor, 2);

			item->a.cvar->Set(newcolor);
		}
		S_Sound(CHAN_INTERFACE, "menu/change", 1, ATTN_NONE);
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
			cur = M_FindCurVal(item->a.cvar->value(), item->e.values, numvals);
			if (++cur >= numvals)
				cur = 0;

			item->a.cvar->Set(item->e.values[cur].value);

			// Hack hack. Rebuild list of resolutions
			if (item->e.values == Depths.data())
				BuildModesList(I_GetVideoWidth(), I_GetVideoHeight());
		}
		S_Sound(CHAN_INTERFACE, "menu/change", 1, ATTN_NONE);
		break;

		case screenres:
		{
			int col;

			col = item->a.selmode + 1;
			if ((col > 2) || (col == 2 && !item->d.res3) || (col == 1 && !item->c.res2))
			{
				if (CurrentMenu->numitems - 1 > CurrentItem)
				{
					if (CurrentMenu->items[CurrentItem + 1].type == screenres)
					{
						if (CurrentMenu->items[CurrentItem + 1].b.res1)
						{
							item->a.selmode = -1;
							CurrentMenu->items[++CurrentItem].a.selmode = 0;
						}
					}
				}
			}
			else
			{
				item->a.selmode = col;
			}
		}
		S_Sound(CHAN_INTERFACE, "menu/choose", 1, ATTN_NONE);
		break;

		case joyactive:
		{
			const size_t numjoy = I_GetJoystickCount();

			if (static_cast<size_t>(item->a.cvar->value()) >= numjoy)
				item->a.cvar->Set(0.0);
			else if (static_cast<size_t>(item->a.cvar->value()) < (numjoy - 1))
				item->a.cvar->Set(item->a.cvar->value() + 1);

		}
		S_Sound(CHAN_INTERFACE, "menu/change", 1, ATTN_NONE);
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
			if (CurrentMenu == &ModesMenu)
			{
				int width;
				int height;

				if (!(item->type == screenres &&
				      GetSelectedSize(CurrentItem, &width, &height)))
				{
					width = I_GetVideoWidth();
					height = I_GetVideoHeight();
				}

				M_SetVideoMode(width, height);
				S_Sound(CHAN_INTERFACE, "menu/choose", 1, ATTN_NONE);
			}
			else if (item->type == more && item->e.mfunc)
			{
				CurrentMenu->lastOn = CurrentItem;
				S_Sound(CHAN_INTERFACE, "menu/advance", 1, ATTN_NONE);
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
				cur = M_FindCurVal(item->a.cvar->value(), item->e.values, numvals);
				if (++cur >= numvals)
					cur = 0;

				item->a.cvar->Set(item->e.values[cur].value);

				// Hack hack. Rebuild list of resolutions
				if (item->e.values == Depths.data())
					BuildModesList(I_GetVideoWidth(), I_GetVideoHeight());
				S_Sound(CHAN_INTERFACE, "menu/change", 1, ATTN_NONE);
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
				S_Sound(CHAN_INTERFACE, "menu/choose", 1, ATTN_NONE);
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
			if (CurrentMenu == &ModesMenu)
			{
				int width;
				int height;

				if (!(item->type == screenres && GetSelectedSize(CurrentItem, &width, &height)))
				{
					width = I_GetVideoWidth();
					height = I_GetVideoHeight();
				}

				constexpr dtime_t testduration = dtime_t{5} * TICRATE;
				testingmode = (I_MSTime() * TICRATE / MSECS_PER_SEC) + testduration;
				M_SetVideoMode(width, height);

				S_Sound(CHAN_INTERFACE, "menu/choose", 1, ATTN_NONE);
			}
		}
		}
	}

	if (CurrentMenu->refreshfunc)
		(*CurrentMenu->refreshfunc)();
}
namespace
{


void GoToConsole()
{
	M_ClearMenus ();
	C_ToggleConsole ();
}

void UpdateStuff()
{
	M_SizeDisplay (0.0);
}
} // namespace


void Reset2Defaults()
{
	AddCommandString ("unbindall; binddefaults");
	cvar_t::C_SetCVarsToDefaults(CVAR_CLIENTARCHIVE);
	UpdateStuff();
}

void Reset2Saved()
{
	const std::string cmd = "exec " + C_QuoteString(M_GetConfigPath());
	AddCommandString(cmd);
	UpdateStuff();
}
namespace
{


void StartHUDMenu()
{
	M_SwitchMenu(&HUDMenu);
}

void StartMessagesMenu()
{
	M_SwitchMenu (&MessagesMenu);
}

void StartAutomapMenu()
{
	M_SwitchMenu (&AutomapMenu);
}
} // namespace


void ResetCustomColors()
{
	AddCommandString ("resetcustomcolors");
}

void MouseSetup () // [Toke] for mouse menu
{
	M_SwitchMenu (&MouseMenu);
}

void JoystickSetup()
{
	M_SwitchMenu (&JoystickMenu);
}
namespace
{


void CustomizeControls()
{
	M_BuildKeyList (ControlsMenu.items, ControlsMenu.numitems);
	M_SwitchMenu (&ControlsMenu);
}

// [Russell] - Hack for getting to the player setup menu, doesn't
// record the last menu though, unfortunately
void PlayerSetup()
{
    M_ClearMenus ();
    M_StartControlPanel ();
	M_PlayerSetup(0);
}
} // namespace


BEGIN_COMMAND (menu_keys)
{
	M_StartControlPanel ();
	OptionsActive = true;
	CustomizeControls();
}
END_COMMAND (menu_keys)

namespace
{

void VideoOptions()
{
	M_SwitchMenu (&VideoMenu);
}

void AdvMidiOptions()
{
	M_SwitchMenu (&AdvMidiMenu);
}

void LibAdlMidiOptions()
{
	M_SwitchMenu (&LibAdlMidiMenu);
}

void SoundOptions () // [Ralphis] for sound menu
{
	M_SwitchMenu (&SoundMenu);
}

void CompatOptions () // [Ralphis] for compatibility menu
{
	M_SwitchMenu (&CompatMenu);
}

void NetworkOptions()
{
	M_SwitchMenu (&NetworkMenu);
}

void WeaponOptions()
{
	M_SwitchMenu (&WeaponMenu);
}

} // namespace

BEGIN_COMMAND (menu_display)
{
	M_StartControlPanel ();
	OptionsActive = true;
	M_SwitchMenu (&VideoMenu);
}
END_COMMAND (menu_display)


BEGIN_COMMAND (menu_video)
{
	M_StartControlPanel ();
	OptionsActive = true;
	SetVidMode ();
}
END_COMMAND (menu_video)

VERSION_CONTROL (m_options_cpp, "$Id$")
