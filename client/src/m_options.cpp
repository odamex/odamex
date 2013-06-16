// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1998-2006 by Randy Heit (ZDoom 1.22).
// Copyright (C) 2006-2013 by The Odamex Team.
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


#include "doomdef.h"
#include "gstrings.h"
#include "minilzo.h"

#include "c_console.h"
#include "c_dispatch.h"
#include "c_bind.h"
#include "cmdlib.h"

#include "d_main.h"

#include "i_system.h"
#include "i_video.h"
#include "i_input.h"
#include "z_zone.h"
#include "v_video.h"
#include "v_text.h"
#include "w_wad.h"

#include "r_local.h"


#include "hu_stuff.h"

#include "g_game.h"

#include "m_argv.h"
#include "m_swap.h"
#include "m_memio.h"

#include "s_sound.h"
#include "i_musicsystem.h"

#include "doomstat.h"

#include "m_misc.h"

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

EXTERN_CVAR (cl_run)
EXTERN_CVAR (invertmouse)
EXTERN_CVAR (lookspring)
EXTERN_CVAR (lookstrafe)
EXTERN_CVAR (hud_crosshair)
EXTERN_CVAR (hud_crosshairhealth)
EXTERN_CVAR (hud_crosshairscale)
EXTERN_CVAR (cl_mouselook)
EXTERN_CVAR (gammalevel)
EXTERN_CVAR (r_detail)
EXTERN_CVAR (language)

// [Ralphis - Menu] Compatibility Menu
EXTERN_CVAR (co_level8soundfeature)
EXTERN_CVAR (hud_targetnames)
EXTERN_CVAR (hud_gamemsgtype)
EXTERN_CVAR (hud_scale)
EXTERN_CVAR (hud_scalescoreboard)
EXTERN_CVAR (hud_timer)
EXTERN_CVAR (hud_heldflag)
EXTERN_CVAR (hud_transparency)
EXTERN_CVAR (hud_revealsecrets)
EXTERN_CVAR (r_showendoom)
EXTERN_CVAR (co_allowdropoff)
EXTERN_CVAR (co_realactorheight)
EXTERN_CVAR (co_boomlinecheck)
EXTERN_CVAR (wi_newintermission)
EXTERN_CVAR (co_zdoomphys)
EXTERN_CVAR (co_zdoomswitches)
EXTERN_CVAR (co_fixweaponimpacts)
EXTERN_CVAR (co_zdoomsoundcurve)
EXTERN_CVAR (cl_deathcam)
EXTERN_CVAR (co_fineautoaim)
EXTERN_CVAR (co_nosilentspawns)
EXTERN_CVAR (co_boomsectortouch)
EXTERN_CVAR (co_blockmapfix)

// [Toke - Menu] New Menu Stuff.
void MouseSetup (void);
EXTERN_CVAR (mouse_driver)
EXTERN_CVAR (mouse_type)
EXTERN_CVAR (mouse_sensitivity)
EXTERN_CVAR (m_pitch)
EXTERN_CVAR (novert)
EXTERN_CVAR (m_side)
EXTERN_CVAR (m_forward)
EXTERN_CVAR (mouse_acceleration)
EXTERN_CVAR (mouse_threshold)

// [Ralphis - Menu] Sound Menu
EXTERN_CVAR (snd_musicsystem)
EXTERN_CVAR (snd_musicvolume)
EXTERN_CVAR (snd_announcervolume)
EXTERN_CVAR (snd_sfxvolume)
EXTERN_CVAR (snd_crossover)
EXTERN_CVAR (snd_gamesfx)
EXTERN_CVAR (snd_voxtype)
EXTERN_CVAR (cl_connectalert)
EXTERN_CVAR (cl_disconnectalert)

// Joystick menu -- Hyper_Eye
void JoystickSetup (void);
EXTERN_CVAR (use_joystick)
EXTERN_CVAR (joy_active)
EXTERN_CVAR (joy_forwardaxis)
EXTERN_CVAR (joy_strafeaxis)
EXTERN_CVAR (joy_turnaxis)
EXTERN_CVAR (joy_lookaxis)
EXTERN_CVAR (joy_sensitivity)
EXTERN_CVAR (joy_invert)
EXTERN_CVAR (joy_freelook)

// Network Options
EXTERN_CVAR (rate)
EXTERN_CVAR (cl_unlag)
EXTERN_CVAR (cl_updaterate)
EXTERN_CVAR (cl_interp)
EXTERN_CVAR (cl_prednudge)
EXTERN_CVAR (cl_predictpickup)
EXTERN_CVAR (cl_predictsectors)
EXTERN_CVAR (cl_predictweapons)

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
void M_SizeDisplay(float diff);
void M_StartControlPanel(void);

int  M_StringHeight(char *string);
void M_ClearMenus (void);

static bool CanScrollUp;
static bool CanScrollDown;
static int VisBottom;

value_t YesNo[2] = {
	{ 0.0, "No" },
	{ 1.0, "Yes" }
};

value_t NoYes[2] = {
	{ 0.0, "Yes" },
	{ 1.0, "No" }
};

value_t OnOff[2] = {
	{ 0.0, "Off" },
	{ 1.0, "On" }
};

value_t OffOn[2] = {
	{ 0.0, "On" },
	{ 1.0, "Off" }
};

value_t OnOffAuto[3] = {
	{ 0.0, "Off" },
	{ 1.0, "On" },
	{ 2.0, "Auto" }
};

static value_t DoomOrOdamex[2] =
{
	{ 0.0, "Doom" },
	{ 1.0, "Odamex" }
};

menu_t  *CurrentMenu;
int		CurrentItem;
static BOOL	WaitingForKey;
static BOOL	WaitingForAxis;
static const char	   *OldContMessage;
static itemtype OldContType;
static const char	   *OldAxisMessage;
static itemtype OldAxisType;

/*=======================================
 *
 * Options Menu
 *
 *=======================================*/

static void PlayerSetup (void);
static void CustomizeControls (void);
static void VideoOptions (void);
static void SoundOptions (void);
static void CompatOptions (void);
static void NetworkOptions (void);
static void WeaponOptions (void);
static void GoToConsole (void);
static void GoToConsole (void);
void Reset2Defaults (void);
void Reset2Saved (void);

static void SetVidMode (void);

static menuitem_t OptionItems[] =
{
    { more, 	"Player Setup",     	{NULL},					{0.0}, {0.0},	{0.0}, {(value_t *)PlayerSetup} },
	{ more,		"Weapon Preferences",	{NULL},					{0.0}, {0.0},	{0.0}, {(value_t *)WeaponOptions} },
 	{ more,		"Customize Controls",	{NULL},					{0.0}, {0.0},	{0.0}, {(value_t *)CustomizeControls} },
	{ more,		"Mouse Options" ,	    {NULL},					{0.0}, {0.0},	{0.0}, {(value_t *)MouseSetup} },
	{ more,		"Joystick Setup" ,	    {NULL},					{0.0}, {0.0},	{0.0}, {(value_t *)JoystickSetup} },
 	{ redtext,	" ",					{NULL},					{0.0}, {0.0},	{0.0}, {NULL} },
 	{ more,		"Compatibility Options",{NULL},					{0.0}, {0.0},	{0.0}, {(value_t *)CompatOptions} },
	{ more,		"Network Options",		{NULL},					{0.0}, {0.0},	{0.0}, {(value_t *)NetworkOptions} },
	{ more,		"Sound Options",		{NULL},					{0.0}, {0.0},	{0.0}, {(value_t *)SoundOptions} },
 	{ more,		"Display Options",		{NULL},					{0.0}, {0.0},	{0.0}, {(value_t *)VideoOptions} },
	{ more,		"Set Video Mode",		{NULL},					{0.0}, {0.0},	{0.0}, {(value_t *)SetVidMode} },
    { redtext,	" ",					{NULL},					{0.0}, {0.0},	{0.0}, {NULL} },
	{ more,		"Go To Console",		{NULL},					{0.0}, {0.0},	{0.0}, {(value_t *)GoToConsole} },
    { redtext,	" ",					{NULL},					{0.0}, {0.0},	{0.0}, {NULL} },
	{ discrete,	"Always Run",			{&cl_run},				{2.0}, {0.0},	{0.0}, {OnOff} },
 	{ discrete, "Lookspring",			{&lookspring},			{2.0}, {0.0},	{0.0}, {OnOff} },
 	{ redtext,	" ",					{NULL},					{0.0}, {0.0},	{0.0}, {NULL} },
 	{ more,		"Reset to defaults",	{NULL},					{0.0}, {0.0},	{0.0}, {(value_t *)Reset2Defaults} },
 	{ more,		"Reset to last saved",	{NULL},					{0.0}, {0.0},	{0.0}, {(value_t *)Reset2Saved} }
};

menu_t OptionMenu = {
	"M_OPTTTL",
	0,
	STACKARRAY_LENGTH(OptionItems),
	177,
	OptionItems,
	0,
	0,
	NULL
};

/*=======================================
 *
 * Controls Menu
 *
 *=======================================*/
 
static menuitem_t ControlsItems[] = {
#ifdef _XBOX
	{ whitetext,"A to change, START to clear", {NULL}, {0.0}, {0.0}, {0.0}, {NULL} },
#else
	{ whitetext,"ENTER to change, BACKSPACE to clear", {NULL}, {0.0}, {0.0}, {0.0}, {NULL} },
#endif
	{ redtext,	" ",					{NULL},	{0.0}, {0.0}, {0.0}, {NULL} },
	{ bricktext,"Basic Movement",		{NULL},	{0.0}, {0.0}, {0.0}, {NULL} },
	{ control,	"Move forward",			{NULL}, {0.0}, {0.0}, {0.0}, {(value_t *)"+forward"} },
	{ control,	"Move backward",		{NULL}, {0.0}, {0.0}, {0.0}, {(value_t *)"+back"} },
	{ control,	"Strafe left",			{NULL}, {0.0}, {0.0}, {0.0}, {(value_t *)"+moveleft"} },
	{ control,	"Strafe right",			{NULL}, {0.0}, {0.0}, {0.0}, {(value_t *)"+moveright"} },
	{ control,	"Turn left",			{NULL}, {0.0}, {0.0}, {0.0}, {(value_t *)"+left"} },
	{ control,	"Turn right",			{NULL}, {0.0}, {0.0}, {0.0}, {(value_t *)"+right"} },
	{ control,	"Run",					{NULL}, {0.0}, {0.0}, {0.0}, {(value_t *)"+speed"} },
	{ control,	"Strafe",				{NULL}, {0.0}, {0.0}, {0.0}, {(value_t *)"+strafe"} },
	{ control,	"Jump",					{NULL}, {0.0}, {0.0}, {0.0}, {(value_t *)"+jump"} },
	{ control,	"Turn 180",				{NULL}, {0.0}, {0.0}, {0.0}, {(value_t *)"turn180"} },
	{ redtext,	" ",					{NULL},	{0.0}, {0.0}, {0.0}, {NULL} },
	{ bricktext,"Actions",		        {NULL},	{0.0}, {0.0}, {0.0}, {NULL} },
	{ control,	"Fire",					{NULL}, {0.0}, {0.0}, {0.0}, {(value_t *)"+attack"} },
	{ control,	"Use / Open",			{NULL}, {0.0}, {0.0}, {0.0}, {(value_t *)"+use"} },
	{ control,	"Toggle Automap",		{NULL}, {0.0}, {0.0}, {0.0}, {(value_t *)"togglemap"} },
	{ control,	"Next weapon",			{NULL}, {0.0}, {0.0}, {0.0}, {(value_t *)"weapnext"} },
	{ control,	"Previous weapon",		{NULL}, {0.0}, {0.0}, {0.0}, {(value_t *)"weapprev"} },
	{ redtext,	" ",					{NULL},	{0.0}, {0.0}, {0.0}, {NULL} },
	{ bricktext,"Weapons",		        {NULL},	{0.0}, {0.0}, {0.0}, {NULL} },
	{ control,	"Fist/Chainsaw",		{NULL}, {0.0}, {0.0}, {0.0}, {(value_t *)"impulse 1"} },
	{ control,	"Pistol",       		{NULL}, {0.0}, {0.0}, {0.0}, {(value_t *)"impulse 2"} },
	{ control,	"Shotgun/SSG",  		{NULL}, {0.0}, {0.0}, {0.0}, {(value_t *)"impulse 3"} },
	{ control,	"Chaingun",     		{NULL}, {0.0}, {0.0}, {0.0}, {(value_t *)"impulse 4"} },
	{ control,	"Rocket Launcher",		{NULL}, {0.0}, {0.0}, {0.0}, {(value_t *)"impulse 5"} },
	{ control,	"Plasma Rifle",   		{NULL}, {0.0}, {0.0}, {0.0}, {(value_t *)"impulse 6"} },
	{ control,	"BFG",          		{NULL}, {0.0}, {0.0}, {0.0}, {(value_t *)"impulse 7"} },
	{ control,	"Chainsaw",     		{NULL}, {0.0}, {0.0}, {0.0}, {(value_t *)"impulse 8"} },
	{ redtext,	" ",					{NULL},	{0.0}, {0.0}, {0.0}, {NULL} },
	{ bricktext,"Advanced Movement",    {NULL},	{0.0}, {0.0}, {0.0}, {NULL} },
	{ control,	"Fly / Swim up",		{NULL},	{0.0}, {0.0}, {0.0}, {(value_t *)"+moveup"} },
	{ control,	"Fly / Swim down",		{NULL},	{0.0}, {0.0}, {0.0}, {(value_t *)"+movedown"} },
	{ control,	"Toggle flying",		{NULL},	{0.0}, {0.0}, {0.0}, {(value_t *)"fly"} },
	{ control,	"Look up",				{NULL}, {0.0}, {0.0}, {0.0}, {(value_t *)"+lookup"} },
	{ control,	"Look down",			{NULL}, {0.0}, {0.0}, {0.0}, {(value_t *)"+lookdown"} },
	{ control,	"Center view",			{NULL}, {0.0}, {0.0}, {0.0}, {(value_t *)"centerview"} },
	{ control,	"Mouse look",			{NULL}, {0.0}, {0.0}, {0.0}, {(value_t *)"+mlook"} },
	{ control,	"Keyboard look",		{NULL}, {0.0}, {0.0}, {0.0}, {(value_t *)"+klook"} },
	{ redtext,	" ",					{NULL},	{0.0}, {0.0}, {0.0}, {NULL} },
	{ bricktext,"Multiplayer",		    {NULL},	{0.0}, {0.0}, {0.0}, {NULL} },
	{ control,	"Say",					{NULL}, {0.0}, {0.0}, {0.0}, {(value_t *)"messagemode"} },
	{ control,	"Team say",				{NULL}, {0.0}, {0.0}, {0.0}, {(value_t *)"messagemode2"} },
	{ control,	"Ready",				{NULL}, {0.0}, {0.0}, {0.0}, {(value_t *)"ready"} },
	{ control,	"Change teams",			{NULL}, {0.0}, {0.0}, {0.0}, {(value_t *)"changeteams"} },
	{ control,	"Spectate",				{NULL}, {0.0}, {0.0}, {0.0}, {(value_t *)"spectate"} },
	{ control,	"Coop Spy",				{NULL}, {0.0}, {0.0}, {0.0}, {(value_t *)"spynext"} },
	{ control,	"Show Scoreboard",		{NULL}, {0.0}, {0.0}, {0.0}, {(value_t *)"+showscores"} },
	{ control,	"Vote Yes", {NULL}, {0.0}, {0.0}, {0.0}, {(value_t *)"vote_yes"} },
	{ control,	"Vote No", {NULL}, {0.0}, {0.0}, {0.0}, {(value_t *)"vote_no"} },
	{ redtext,	" ",					{NULL},	{0.0}, {0.0}, {0.0}, {NULL} },
	{ bricktext,"Menus",				{NULL},	{0.0}, {0.0}, {0.0}, {NULL} },
	{ control,  "Main menu",			{NULL}, {0.0}, {0.0}, {0.0}, {(value_t *)"menu_main"} },
	{ control,	"Help menu",			{NULL}, {0.0}, {0.0}, {0.0}, {(value_t *)"menu_help"} },
	{ control,	"Save menu",			{NULL}, {0.0}, {0.0}, {0.0}, {(value_t *)"menu_save"} },
	{ control,	"Load menu",			{NULL}, {0.0}, {0.0}, {0.0}, {(value_t *)"menu_load"} },
	{ control,	"Options menu",			{NULL}, {0.0}, {0.0}, {0.0}, {(value_t *)"menu_options"} },
	{ control,	"Display options",	    {NULL}, {0.0}, {0.0}, {0.0}, {(value_t *)"menu_display"} },	
	{ control,	"Player setup menu",	{NULL}, {0.0}, {0.0}, {0.0}, {(value_t *)"menu_player"} },
	{ control,	"Configure controls",	{NULL}, {0.0}, {0.0}, {0.0}, {(value_t *)"menu_keys"} },
	{ control,	"Change resolution",	{NULL}, {0.0}, {0.0}, {0.0}, {(value_t *)"menu_video"} },
	{ redtext,	" ",					{NULL},	{0.0}, {0.0}, {0.0}, {NULL} },
	{ bricktext,"Other",				{NULL},	{0.0}, {0.0}, {0.0}, {NULL} },
    { control,	"Increase screen size",	{NULL}, {0.0}, {0.0}, {0.0}, {(value_t *)"sizeup"} },		
	{ control,	"Reduce screen size",	{NULL}, {0.0}, {0.0}, {0.0}, {(value_t *)"sizedown"} },	
	{ control,	"Chasecam",				{NULL}, {0.0}, {0.0}, {0.0}, {(value_t *)"chase"} },
	{ control,	"Screenshot",			{NULL}, {0.0}, {0.0}, {0.0}, {(value_t *)"screenshot"} },
	{ control,  "Open console",			{NULL}, {0.0}, {0.0}, {0.0}, {(value_t *)"toggleconsole"} },
	{ control,  "End current game",     {NULL}, {0.0}, {0.0}, {0.0}, {(value_t *)"menu_endgame"} },
	{ control,  "Quit Odamex",	        {NULL}, {0.0}, {0.0}, {0.0}, {(value_t *)"menu_quit"} }
};
 
menu_t ControlsMenu = {
	"M_CONTRO",
	3,
	STACKARRAY_LENGTH(ControlsItems),
	0,
	ControlsItems,
	2,
	0,
	NULL
};

// -------------------------------------------------------
//
//	[Toke] New [ Mouse Menu ]
//
// -------------------------------------------------------

static value_t MouseDrivers[NUM_MOUSE_DRIVERS];

static value_t MouseType[] = {
	{ MOUSE_DOOM,		"Doom"},
	{ MOUSE_ZDOOM_DI,	"ZDoom"}
};

static int previous_mouse_type;
void M_ResetMouseValues();

static menuitem_t MouseItems[] =
{
	{ discrete, "Mouse Driver"					, {&mouse_driver},		{0.0},	{0.0},		{0.0},		{MouseDrivers}},
	{ discrete,	"Mouse Config Type"				, {&mouse_type},		{2.0},	{0.0},		{0.0},		{MouseType}},
	{ redtext,	" "								, {NULL},				{0.0},	{0.0},		{0.0},		{NULL}},
	{ slider,	"Overall Sensitivity" 			, {&mouse_sensitivity},	{0.0},	{77.0},		{1.0},		{NULL}},
	{ slider,	"Freelook Sensitivity"			, {&m_pitch},			{0.0},	{1.0},		{0.025},	{NULL}},
	{ redtext,	" "								, {NULL},				{0.0},	{0.0},		{0.0},		{NULL}},
	{ discrete,	"Always FreeLook"				, {&cl_mouselook},		{2.0},	{0.0},		{0.0},		{OnOff}},
	{ discrete,	"Invert Mouse"					, {&invertmouse},		{2.0},	{0.0},		{0.0},		{OnOff}},
	{ redtext,	" "								, {NULL},				{0.0},	{0.0},		{0.0},		{NULL}},
	{ discrete,	"Horizontal Movement"			, {&lookstrafe},		{2.0},	{0.0},		{0.0},		{OnOff}},
	{ discrete,	"Vertical Movement"				, {&novert},			{2.0},	{0.0},		{0.0},		{OffOn}},
	{ slider,	"Horizontal Movement Speed"		, {&m_side},			{0.0},	{15},		{0.5},		{NULL}},
	{ slider,	"Vertical Movement Speed"		, {&m_forward},			{0.0},	{15},		{0.5},		{NULL}},
	{ redtext,	" "								, {NULL},				{0.0},	{0.0},		{0.0},		{NULL}},
	{ slider,	"Mouse Acceleration"			, {&mouse_acceleration},{0.0},	{10.0},		{0.5},		{NULL}},
	{ slider,	"Mouse Threshold"				, {&mouse_threshold},	{0.0},	{20.0},		{1.0},		{NULL}},
	{ redtext,	" "								, {NULL},				{0.0},	{0.0},		{0.0},		{NULL}},
	{ more,		"Reset mouse to defaults"		, {NULL},				{0.0},	{0.0},		{0.0},		{(value_t *)M_ResetMouseValues}},
};

void G_ConvertMouseSettings(int old_type, int new_type);

static void M_UpdateMouseOptions()
{
	const static size_t menu_length = STACKARRAY_LENGTH(MouseItems);
	const static size_t mouse_sens_index = M_FindCvarInMenu(mouse_sensitivity, MouseItems, menu_length); 
	const static size_t mouse_pitch_index = M_FindCvarInMenu(m_pitch, MouseItems, menu_length); 
	const static size_t mouse_accel_index = M_FindCvarInMenu(mouse_acceleration, MouseItems, menu_length);
	const static size_t mouse_thresh_index = M_FindCvarInMenu(mouse_threshold, MouseItems, menu_length);
	const static size_t mouse_driver_index = M_FindCvarInMenu(mouse_driver, MouseItems, menu_length);

	static menuitem_t doom_sens_menuitem = MouseItems[mouse_sens_index];
	static menuitem_t doom_pitch_menuitem = MouseItems[mouse_pitch_index];
	static menuitem_t doom_accel_menuitem = MouseItems[mouse_accel_index];
	static menuitem_t doom_thresh_menuitem = MouseItems[mouse_thresh_index];

	static menuitem_t zdoom_sens_menuitem =
		{ slider	,	"Overall Sensitivity"			, {&mouse_sensitivity},	{0.25},		{2.5},		{0.1},		{NULL}};
	static menuitem_t zdoom_pitch_menuitem =
		{ slider	,	"Freelook Sensitivity"			, {&m_pitch},			{0.25},		{2.5},		{0.1},		{NULL}};
	static menuitem_t zdoom_accel_menuitem =
		{ redtext	,	" "								, {NULL},				{0.0},		{0.0},		{0.0},		{NULL}};
	static menuitem_t zdoom_thresh_menuitem =
		{ redtext	,	" "								, {NULL},				{0.0},		{0.0},		{0.0},		{NULL}};

	if (mouse_type == MOUSE_ZDOOM_DI)
	{
		if (mouse_sens_index < menu_length)
			memcpy(&MouseItems[mouse_sens_index], &zdoom_sens_menuitem, sizeof(menuitem_t));
		if (mouse_pitch_index < menu_length)
			memcpy(&MouseItems[mouse_pitch_index], &zdoom_pitch_menuitem, sizeof(menuitem_t));
		if (mouse_accel_index < menu_length)
			memcpy(&MouseItems[mouse_accel_index], &zdoom_accel_menuitem, sizeof(menuitem_t));
		if (mouse_thresh_index < menu_length)
			memcpy(&MouseItems[mouse_thresh_index], &zdoom_thresh_menuitem, sizeof(menuitem_t));
	}
	else
	{
		if (mouse_sens_index < menu_length)
			memcpy(&MouseItems[mouse_sens_index], &doom_sens_menuitem, sizeof(menuitem_t));
		if (mouse_pitch_index < menu_length)
			memcpy(&MouseItems[mouse_pitch_index], &doom_pitch_menuitem, sizeof(menuitem_t));
		if (mouse_accel_index < menu_length)
			memcpy(&MouseItems[mouse_accel_index], &doom_accel_menuitem, sizeof(menuitem_t));
		if (mouse_thresh_index < menu_length)
			memcpy(&MouseItems[mouse_thresh_index], &doom_thresh_menuitem, sizeof(menuitem_t));
	}

	// refresh the list of availible mouse drivers
	if (mouse_driver_index < menu_length)
	{
		// check each potential driver's availibility
		int num_avail_drivers = 0;
		for (int i = 0; i < NUM_MOUSE_DRIVERS; i++)
		{
			if (MouseDriverInfo[i].avail_test() == true)
			{			
				// update the menu with the pair of {value,name} for this driver
				MouseDrivers[num_avail_drivers].value = float(MouseDriverInfo[i].id);
				MouseDrivers[num_avail_drivers].name = MouseDriverInfo[i].name; 
				num_avail_drivers++;
			}
		}

		// update the number of driver options in the menu
		MouseItems[mouse_driver_index].b.leftval = (float)num_avail_drivers;
	}

	G_ConvertMouseSettings(previous_mouse_type, mouse_type);
	previous_mouse_type = mouse_type;
}

void M_ResetMouseValues()
{
	mouse_type.RestoreDefault();
	mouse_sensitivity.RestoreDefault();
	m_pitch.RestoreDefault();
	cl_mouselook.RestoreDefault();
	invertmouse.RestoreDefault();	
	lookstrafe.RestoreDefault();
	novert.RestoreDefault();
	m_side.RestoreDefault();
	m_forward.RestoreDefault();
	mouse_acceleration.RestoreDefault();
	mouse_threshold.RestoreDefault();

	previous_mouse_type = mouse_type;
	M_UpdateMouseOptions();
}

menu_t MouseMenu = {
    "M_MOUSET",
    0,
    STACKARRAY_LENGTH(MouseItems),
    177,
    MouseItems,
	0,
	0,
	&M_UpdateMouseOptions
};


/*=======================================
 *
 * Joystick Menu
 *
 *=======================================*/

static menuitem_t JoystickItems[] =
{
	{ discrete	,	"Use Joystick"							, {&use_joystick},		{2.0},		{0.0},		{0.0},		{OnOff}						},
	{ redtext	,	" "										, {NULL},				{0.0},		{0.0},		{0.0},		{NULL}						},
	{ joyactive	,	"Active Joystick"						, {&joy_active},		{0.0},		{0.0},		{0.0},		{NULL}						},
	{ redtext	,	" "										, {NULL},				{0.0},		{0.0},		{0.0},		{NULL}						},
	{ discrete	,	"Always FreeLook"						, {&joy_freelook},		{2.0},		{0.0},		{0.0},		{OnOff}						},
	{ discrete	,	"Invert Look Axis"						, {&joy_invert},		{2.0},		{0.0},		{0.0},		{OnOff}						},
	{ slider	,	"Turn Sensitivity"						, {&joy_sensitivity},	{1.0},		{30.0},		{1.0},		{NULL}						},
	{ redtext	,	" "										, {NULL},				{0.0},		{0.0},		{0.0},		{NULL}						},
	{ whitetext	,	"Press ENTER to change"					, {NULL}, 				{0.0}, 		{0.0}, 		{0.0}, 		{NULL} 						},
	{ joyaxis	,	"Walk Analog Axis"						, {&joy_forwardaxis},	{0.0},		{0.0},		{0.0},		{NULL}						},
	{ joyaxis	,	"Strafe Analog Axis"					, {&joy_strafeaxis},	{0.0},		{0.0},		{0.0},		{NULL}						},
	{ joyaxis	,	"Turn Analog Axis"						, {&joy_turnaxis},		{0.0},		{0.0},		{0.0},		{NULL}						},
	{ joyaxis	,	"Look Analog Axis"						, {&joy_lookaxis},		{0.0},		{0.0},		{0.0},		{NULL}						},
};

menu_t JoystickMenu = {
    "M_JOYSTK",
    0,
    STACKARRAY_LENGTH(JoystickItems),
    177,
    JoystickItems,
	0,
	0,
	NULL
};

 /*=======================================
  *
  * Sound Menu [Ralphis]
  *
  *=======================================*/

static value_t MusSys[] = {
	{ MS_SDLMIXER,	"SDL Mixer"},
	#ifdef OSX
	{ MS_AUDIOUNIT,	"AudioUnit"},
	#endif	// OSX
	#ifdef PORTMIDI
	{ MS_PORTMIDI,	"PortMidi"},
	#endif	// PORTMIDI
	{ MS_NONE,		"No Music"}
};

static value_t VoxType[] = {
	{ 0.0,			"Off" },
	{ 1.0,			"Team Colors" },
	{ 2.0,			"Possessive" }
};

static float num_mussys = static_cast<float>(STACKARRAY_LENGTH(MusSys));

static menuitem_t SoundItems[] = {
    { redtext,	" ",					{NULL},	{0.0}, {0.0}, {0.0}, {NULL} },    
	{ bricktext ,   "Sound Levels"                      , {NULL},	            {0.0},      	{0.0},      {0.0},      {NULL} },
	{ slider    ,	"Music Volume"                      , {&snd_musicvolume},	{0.0},      	{1.0},	    {0.1},      {NULL} },
	{ slider    ,	"Sound Volume"                      , {&snd_sfxvolume},		{0.0},      	{1.0},	    {0.1},      {NULL} },
	{ slider    ,	"Announcer Volume"             		, {&snd_announcervolume},	{0.0},      {1.0},	    {0.1},      {NULL} },
	{ discrete  ,   "Stereo Switch"                     , {&snd_crossover},	    {2.0},			{0.0},		{0.0},		{OnOff} },	
	{ redtext   ,	" "                                 , {NULL},	            {0.0},      	{0.0},      {0.0},      {NULL} },
	{ discrete	,	"Music System Backend"				, {&snd_musicsystem},	{num_mussys},	{0.0},		{0.0},		{MusSys} },
	{ redtext   ,	" "                                 , {NULL},	            {0.0},      	{0.0},      {0.0},      {NULL} },
	{ bricktext ,   "Sound Options"                     , {NULL},	            {0.0},      	{0.0},      {0.0},      {NULL} },
	{ discrete  ,   "Game SFX"                          , {&snd_gamesfx},		{2.0},			{0.0},		{0.0},		{OnOff} },
	{ discrete  ,   "Announcer Type"                    , {&snd_voxtype},		{3.0},			{0.0},		{0.0},		{VoxType} },
	{ discrete  ,   "Player Connect Alert"              , {&cl_connectalert},	{2.0},			{0.0},		{0.0},		{OnOff} },
	{ discrete  ,   "Player Disconnect Alert"           , {&cl_disconnectalert},{2.0},			{0.0},		{0.0},		{OnOff} }	
 };
 
menu_t SoundMenu = {
	"M_SOUND",
	2,
	STACKARRAY_LENGTH(SoundItems),
	177,
	SoundItems,
	0,
	0,
	NULL
};


/*=======================================
 *
 * Compatibility Options Menu
 *
 *=======================================*/
static menuitem_t CompatItems[] ={
	{bricktext, "Gameplay",                        {NULL},                  {0.0}, {0.0}, {0.0}, {NULL}},
	{discrete,  "Camera follows killer on death",  {&cl_deathcam},          {2.0}, {0.0}, {0.0}, {OnOff}},
	{discrete,  "Finer-precision Autoaim",         {&co_fineautoaim},       {2.0}, {0.0}, {0.0}, {OnOff}},
	{discrete,  "Fix hit detection at grid edges", {&co_blockmapfix},       {2.0}, {0.0}, {0.0}, {OnOff}},
	{discrete,  "ZDOOM 1.23 physics",              {&co_zdoomphys},         {2.0}, {0.0}, {0.0}, {OnOff}},
	{redtext,   " ",                               {NULL},                  {0.0}, {0.0}, {0.0}, {NULL}},
	{bricktext, "Items and Decoration",            {NULL},                  {0.0}, {0.0}, {0.0}, {NULL}},
	{discrete,  "Fix invisible puffs under skies", {&co_fixweaponimpacts},  {2.0}, {0.0}, {0.0}, {OnOff}},
	{discrete,  "Items can be walked over/under",  {&co_realactorheight},   {2.0}, {0.0}, {0.0}, {OnOff}},
	{discrete,  "Items can drop off ledges",       {&co_allowdropoff},      {2.0}, {0.0}, {0.0}, {OnOff}},
	{redtext,   " ",                               {NULL},                  {0.0}, {0.0}, {0.0}, {NULL}},
	{bricktext, "Map Compatibility",               {NULL},                  {0.0}, {0.0}, {0.0}, {NULL}},
	{discrete,  "BOOM actor-in-sector check",      {&co_boomsectortouch},   {2.0}, {0.0}, {0.0}, {OnOff}},
	{discrete,  "BOOM extra line checks on use",   {&co_boomlinecheck},     {2.0}, {0.0}, {0.0}, {OnOff}},
	{redtext,   " ",                               {NULL},                  {0.0}, {0.0}, {0.0}, {NULL}},
	{bricktext, "Sound",                           {NULL},                  {0.0}, {0.0}, {0.0}, {NULL}},
	{discrete,  "Fix silent west spawns",          {&co_nosilentspawns},    {2.0}, {0.0}, {0.0}, {OnOff}},
	{discrete,  "Louder sounds on map 8",          {&co_level8soundfeature},{2.0}, {0.0}, {0.0}, {OnOff}},
	{discrete,  "Positional switch sounds",        {&co_zdoomswitches},     {2.0}, {0.0}, {0.0}, {OnOff}},
	{discrete,  "ZDOOM 1.23 extended sound curve", {&co_zdoomsoundcurve},   {2.0}, {0.0}, {0.0}, {OnOff}},
};

menu_t CompatMenu = {
	"M_COMPAT",
	1,
	STACKARRAY_LENGTH(CompatItems),
	240,
	CompatItems,
	0,
	0,
	NULL,
};


/*=======================================
 *
 * Network Options Menu
 *
 *=======================================*/

static value_t BandwidthLevels[] = {
	{ 7.0,			"56kbps" },
	{ 200.0,		"1.5Mbps" },
	{ 375.0,		"3.0Mbps" },
	{ 750.0,		"6.0Mbps" }
};

static value_t UpdateRate[] = {
	{ 1.0,			"Every tic" },
	{ 2.0,			"Every 2nd tic" },
	{ 3.0,			"Every 3rd tic" }
};

static menuitem_t NetworkItems[] = {
    { redtext,	" ",					{NULL},	{0.0}, {0.0}, {0.0}, {NULL} },    
	{ bricktext,	"Adjust Network Settings",		{NULL},				{0.0},		{0.0},		{0.0},		{NULL} },
	{ discrete,		"Bandwidth",					{&rate},			{4.0},		{0.0},		{0.0},		{BandwidthLevels} },
	{ discrete,		"Position update freq",			{&cl_updaterate},	{3.0},		{0.0},		{0.0},		{UpdateRate} },
	{ slider,		"Interpolation time",			{&cl_interp},		{0.0},		{4.0},		{1.0},		{NULL} },
	{ slider,		"Smooth collisions",			{&cl_prednudge},	{1.0},		{0.1},		{-0.1},		{NULL} },
	{ discrete,		"Adjust weapons for lag",		{&cl_unlag},		{2.0},		{0.0},		{0.0},		{OnOff} },
	{ discrete,		"Predict weapon pickups",		{&cl_predictpickup},{2.0},		{0.0},		{0.0},		{OnOff} },
	{ discrete,		"Predict sectors",				{&cl_predictsectors},{2.0},		{0.0},		{0.0},		{OnOff} },
	{ discrete,		"Predict weapon effects",		{&cl_predictweapons},{2.0},		{0.0},		{0.0},		{OnOff} },
};

menu_t NetworkMenu = {
	"M_NETWRK",
	2,
	STACKARRAY_LENGTH(NetworkItems),
	177,
	NetworkItems,
	0,
	0,
	NULL
};


/*=======================================
 *
 * Weapon Preferences Menu
 *
 *=======================================*/

static value_t WeapSwitch[] = {
	{ 0.0,			"Never" },
	{ 1.0,			"Always" },
	{ 2.0,			"By Preference" }
};

extern const char *weaponnames[];

static menuitem_t WeaponItems[] = {
	{bricktext, "Weapon Preferences",  {NULL},               {0.0}, {0.0}, {0.0}, {NULL}},
	{discrete,  "Switch on pickup",    {&cl_switchweapon},   {3.0}, {0.0}, {0.0}, {WeapSwitch}},
	{redtext,   " ",                   {NULL},               {0.0}, {0.0}, {0.0}, {NULL}},
	{bricktext, "Weapon Switch Order", {NULL},               {0.0}, {0.0}, {0.0}, {NULL}},
	{slider,    weaponnames[0],        {&cl_weaponpref_fst}, {0.0}, {8.0}, {1.0}, {NULL}},
	{slider,    weaponnames[7],        {&cl_weaponpref_csw}, {0.0}, {8.0}, {1.0}, {NULL}},
	{slider,    weaponnames[1],        {&cl_weaponpref_pis}, {0.0}, {8.0}, {1.0}, {NULL}},
	{slider,    weaponnames[2],        {&cl_weaponpref_sg},  {0.0}, {8.0}, {1.0}, {NULL}},
	{slider,    weaponnames[8],        {&cl_weaponpref_ssg}, {0.0}, {8.0}, {1.0}, {NULL}},
	{slider,    weaponnames[3],        {&cl_weaponpref_cg},  {0.0}, {8.0}, {1.0}, {NULL}},
	{slider,    weaponnames[4],        {&cl_weaponpref_rl},  {0.0}, {8.0}, {1.0}, {NULL}},
	{slider,    weaponnames[5],        {&cl_weaponpref_pls}, {0.0}, {8.0}, {1.0}, {NULL}},
	{slider,    weaponnames[6],        {&cl_weaponpref_bfg}, {0.0}, {8.0}, {1.0}, {NULL}},
	{redtext,   " ",                   {NULL},               {0.0}, {0.0}, {0.0}, {NULL}},
	{whitetext, "Weapons with higher", {NULL},               {0.0}, {0.0}, {0.0}, {NULL}},
	{whitetext, "preference are selected first", {NULL},     {0.0}, {0.0}, {0.0}, {NULL}},
};

menu_t WeaponMenu = {
	"M_WEAPON",
	1,
	STACKARRAY_LENGTH(WeaponItems),
	177,
	WeaponItems,
	0,
	0,
	NULL
};


/*=======================================
 *
 * Display Options Menu
 *
 *=======================================*/
static void StartMessagesMenu (void);
static void StartAutomapMenu (void);
void ResetCustomColors (void);

EXTERN_CVAR (am_rotate)
EXTERN_CVAR (am_overlay)
EXTERN_CVAR (am_ovshare)
EXTERN_CVAR (am_showmonsters)
EXTERN_CVAR (am_showsecrets)
EXTERN_CVAR (am_showtime)
EXTERN_CVAR (am_classicmapstring)
EXTERN_CVAR (am_usecustomcolors)
EXTERN_CVAR (r_widescreen)
EXTERN_CVAR (st_scale)
EXTERN_CVAR (r_stretchsky)
EXTERN_CVAR (r_skypalette)
EXTERN_CVAR (r_wipetype)
EXTERN_CVAR (screenblocks)
EXTERN_CVAR (ui_dimamount)
EXTERN_CVAR (r_showendoom)
EXTERN_CVAR (r_painintensity)
EXTERN_CVAR (cl_movebob)
EXTERN_CVAR (cl_showspawns)

static value_t Crosshairs[] =
{
	{ 0.0, "None" },
	{ 1.0, "Cross 1" },
	{ 2.0, "Cross 2" },
	{ 3.0, "X" },
	{ 4.0, "Diamond" },
	{ 5.0, "Dot" },
	{ 6.0, "Box" },
	{ 7.0, "Angle" },
	{ 8.0, "Big Thing" }
};

static value_t Wipes[] = {
	{ 0.0, "None" },
	{ 1.0, "Melt" },
	{ 2.0, "Burn" },
	{ 3.0, "Crossfade" }
};

static value_t Overlays[] = {
    { 0.0, "Off" },
    { 1.0, "Standard" },
    { 2.0, "Full" },
    { 3.0, "Full Only" }
};

static void M_SendUINewColor (int red, int green, int blue);
static void M_SlideUIRed (int);
static void M_SlideUIGreen (int);
static void M_SlideUIBlue (int);

int dummy = 0;

CVAR_FUNC_IMPL (ui_transred)
{
    if (var > 255)
        var.Set(255);

    if (var < 0)
        var.Set(0.0f);

    M_SlideUIRed((int)var);
}

CVAR_FUNC_IMPL (ui_transgreen)
{
    if (var > 255)
        var.Set(255);

    if (var < 0)
        var.Set(0.0f);

    M_SlideUIGreen((int)var);
}

CVAR_FUNC_IMPL (ui_transblue)
{
    if (var > 255)
        var.Set(255);

    if (var < 0)
        var.Set(0.0f);

    M_SlideUIBlue((int)var);
}

static menuitem_t VideoItems[] = {
	{ more,		"Messages",				    {NULL},					{0.0}, {0.0},	{0.0},  {(value_t *)StartMessagesMenu} },
	{ more,		"Automap",				    {NULL},					{0.0}, {0.0},	{0.0},  {(value_t *)StartAutomapMenu} },
	{ redtext,	" ",					    {NULL},					{0.0}, {0.0},	{0.0},  {NULL} },
	{ slider,	"Screen size",			    {&screenblocks},	   	{3.0}, {12.0},	{1.0},  {NULL} },
	{ slider,	"Brightness",			    {&gammalevel},			{1.0}, {8.0},	{1.0},  {NULL} },
	{ slider,	"Red Pain Intensity",		{&r_painintensity},		{0.0}, {1.0},	{0.1},  {NULL} },	
	{ slider,	"Movement bobbing",			{&cl_movebob},			{0.0}, {1.0},	{0.1},	{NULL} },
	{ discrete,	"Visible Spawn Points",		{&cl_showspawns},		{2.0}, {0.0},	{0.0},	{OnOff} },
	{ redtext,	" ",					    {NULL},					{0.0}, {0.0},	{0.0},  {NULL} },	
	{ discrete, "Scale status bar",	        {&st_scale},			{2.0}, {0.0},	{0.0},  {OnOff} },
	{ discrete, "Scale HUD",	            {&hud_scale},			{2.0}, {0.0},	{0.0},  {OnOff} },
	{ slider,   "HUD Visibility",           {&hud_transparency},    {0.0}, {1.0},   {0.1},  {NULL} },	
	{ slider,   "Scale scoreboard",         {&hud_scalescoreboard}, {0.0}, {1.0},   {0.125},  {NULL} },
	{ discrete, "HUD Timer Visibility",     {&hud_timer},           {2.0}, {0.0},   {0.0},  {OnOff} },
	{ discrete, "Held Flag Border",         {&hud_heldflag},        {2.0}, {0.0},   {0.0},  {OnOff} },
	{ discrete,	"Crosshair",			    {&hud_crosshair},		{9.0}, {0.0},	{0.0},  {Crosshairs} },
	{ discrete, "Scale crosshair",			{&hud_crosshairscale},	{2.0}, {0.0},	{0.0},	{OnOff} },
	{ discrete, "Crosshair health",			{&hud_crosshairhealth},	{2.0}, {0.0},	{0.0},	{OnOff} },
	{ discrete, "Multiplayer Intermissions",{&wi_newintermission}, {2.0}, {0.0},	{0.0},  {DoomOrOdamex} },	
	{ redtext,	" ",					    {NULL},				    {0.0}, {0.0},	{0.0},  {NULL} },
	{ slider,   "UI Background Red",        {&ui_transred},         {0.0}, {255.0}, {16.0}, {NULL} },
	{ slider,   "UI Background Green",      {&ui_transgreen},       {0.0}, {255.0}, {16.0}, {NULL} },
	{ slider,   "UI Background Blue",       {&ui_transblue},        {0.0}, {255.0}, {16.0}, {NULL} },
	{ slider,   "UI Background Visibility", {&ui_dimamount},        {0.0}, {1.0},   {0.1},  {NULL} },	
	{ redtext,	" ",					    {NULL},					{0.0}, {0.0},	{0.0},  {NULL} },
	{ discrete, "Stretch short skies",	    {&r_stretchsky},	   	{3.0}, {0.0},	{0.0},  {OnOffAuto} },
	{ discrete, "Invuln changes skies",		{&r_skypalette},		{2.0}, {0.0},	{0.0},	{OnOff} },
	{ discrete, "Screen wipe style",	    {&r_wipetype},			{4.0}, {0.0},	{0.0},  {Wipes} },
    { discrete,	"Show DOS ending screen" ,  {&r_showendoom},		{2.0}, {0.0},	{0.0},  {OnOff} },
};

menu_t VideoMenu = {
	"M_VIDEO",
	0,
	STACKARRAY_LENGTH(VideoItems),
	0,
	VideoItems,
	3,
	0,
	NULL
};

/*=======================================
 *
 * Messages Menu
 *
 *=======================================*/
EXTERN_CVAR (hud_scaletext)
EXTERN_CVAR (msg0color)
EXTERN_CVAR (msg1color)
EXTERN_CVAR (msg2color)
EXTERN_CVAR (msg3color)
EXTERN_CVAR (msg4color)
EXTERN_CVAR (msgmidcolor)
EXTERN_CVAR (msglevel)

static value_t TextColors[] =
{
	{ 0.0, "brick" },
	{ 1.0, "tan" },
	{ 2.0, "gray" },
	{ 3.0, "green" },
	{ 4.0, "brown" },
	{ 5.0, "gold" },
	{ 6.0, "red" },
	{ 7.0, "blue" },
	{ 8.0, "orange" },
	{ 9.0, "white" },
//	[SL] remove yellow until the color translation can be fixed
//	{ 10.0, "yellow" }
};

static value_t MessageLevels[] = {
	{ 0.0, "Item Pickup" },
	{ 1.0, "Obituaries" },
	{ 2.0, "Critical Messages" }
};

// TODO: Put all language info in one array, auto detect what's in the lump?
static value_t Languages[] = {
	{ 0.0, "Auto" },
	{ 1.0, "English" },
	{ 2.0, "French" },
	{ 3.0, "Italian" }
};

static menuitem_t MessagesItems[] = {
	{ discrete, "Language", 			 {&language},		   	{4.0}, {0.0},   {0.0}, {Languages} },
	{ discrete, "Minimum message level", {&msglevel},		   	{3.0}, {0.0},   {0.0}, {MessageLevels} },
	{ slider,	"Scale message text",    {&hud_scaletext},		{1.0}, {4.0}, 	{1.0}, {NULL} },	
    { discrete,	"Show player target names",	{&hud_targetnames},	{2.0}, {0.0},   {0.0},	{OnOff} },
	{ discrete ,"Game Message Type",    {&hud_gamemsgtype},		{3.0}, {0.0},   {0.0}, {VoxType} },
	{ discrete, "Reveal Secrets",       {&hud_revealsecrets},	{2.0}, {0.0},   {0.0}, {OnOff} },
	{ redtext,	" ",					{NULL},					{0.0}, {0.0},	{0.0}, {NULL} },
	{ bricktext, "Message Colors",		{NULL},					{0.0}, {0.0},	{0.0}, {NULL} },
	{ cdiscrete, "Item Pickup",			{&msg0color},		   	{10.0}, {0.0},	{0.0}, {TextColors} },
	{ cdiscrete, "Obituaries",			{&msg1color},		   	{10.0}, {0.0},	{0.0}, {TextColors} },
	{ cdiscrete, "Critical Messages",	{&msg2color},		   	{10.0}, {0.0},	{0.0}, {TextColors} },
	{ cdiscrete, "Chat Messages",		{&msg3color},		   	{10.0}, {0.0},	{0.0}, {TextColors} },
	{ cdiscrete, "Team Messages",		{&msg4color},		   	{10.0}, {0.0},	{0.0}, {TextColors} },
	{ cdiscrete, "Centered Messages",	{&msgmidcolor},			{10.0}, {0.0},	{0.0}, {TextColors} }
};

menu_t MessagesMenu = {
	"M_MESS",
	0,
	STACKARRAY_LENGTH(MessagesItems),
	0,
	MessagesItems,
	0,
	0,
	NULL
};

/*=======================================
 *
 * Automap Menu
 *
 *=======================================*/
 
static value_t ClassicMapStringTypes[] = {
	{ 0.0, "Odamex" },
	{ 1.0, "Classic" }
};

static menuitem_t AutomapItems[] = {
	{ discrete, "Rotate automap",		{&am_rotate},		   	{2.0}, {0.0},	{0.0},  {OnOff} },
	{ discrete, "Overlay automap",		{&am_overlay},			{4.0}, {0.0},	{0.0},  {Overlays} },
	{ redtext,	" ",					{NULL},					{0.0}, {0.0},	{0.0}, {NULL} },
    { discrete, "Show monster count",	{&am_showmonsters},	   	{2.0}, {0.0},	{0.0},  {OnOff} },
    { discrete, "Show secrets count",	{&am_showsecrets},	   	{2.0}, {0.0},	{0.0},  {OnOff} },
    { discrete, "Show map timer", 	    {&am_showtime}, 	   	{2.0}, {0.0},	{0.0},  {OnOff} },
    { discrete, "Map name style",       {&am_classicmapstring},	{2.0}, {0.0},	{0.0},  {ClassicMapStringTypes} },
        
	{ redtext,	" ",					{NULL},					{0.0}, {0.0},	{0.0}, {NULL} },
	{ bricktext, "Automap Colors",		{NULL},					{0.0}, {0.0},	{0.0}, {NULL} },	
	{ discrete, "Custom map colors",	{&am_usecustomcolors},	{2.0}, {0.0},	{0.0},  {OnOff} },
	{ more,     "Reset custom map colors",  {NULL},             {0.0}, {0.0},   {0.0},  {(value_t *)ResetCustomColors} },
};

menu_t AutomapMenu = {
	"M_MESS",
	0,
	STACKARRAY_LENGTH(AutomapItems),
	0,
	AutomapItems,
	0,
	0,
	NULL
};


/*=======================================
 *
 * Video Modes Menu
 *
 *=======================================*/

extern BOOL setmodeneeded;
extern int NewWidth, NewHeight, NewBits;
extern int DisplayBits;

QWORD testingmode;		// Holds time to revert to old mode
int OldWidth, OldHeight, OldBits;

static void BuildModesList (int hiwidth, int hiheight, int hi_id);
static BOOL GetSelectedSize (int line, int *width, int *height);
static void SetModesMenu (int w, int h, int bits);

EXTERN_CVAR (vid_defwidth)
EXTERN_CVAR (vid_defheight)

EXTERN_CVAR (vid_overscan)
EXTERN_CVAR (vid_fullscreen)
EXTERN_CVAR (vid_32bpp)

static value_t Depths[22];

#ifdef _XBOX
static char VMEnterText[] = "Press A to set mode";
static char VMTestText[] = "Press X to test mode for 5 seconds";
#else
static char VMEnterText[] = "Press ENTER to set mode";
static char VMTestText[] = "Press T to test mode for 5 seconds";
#endif

static value_t DetailModes[] = {
	{ 0.0, "Normal" },
	{ 1.0, "Double Horizontal" },
	{ 2.0, "Double Vertical" },
	{ 3.0, "Double Horiz & Vert" }
};

static value_t Widescreen[] =
{
	{ 0.0, "Stretch" },
	{ 1.0, "Zoom" },
	{ 2.0, "Wide or Stretch" },
	{ 3.0, "Wide or Zoom" }
};

static menuitem_t ModesItems[] = {
	{ discrete,	"Detail Mode",			{&r_detail},			{4.0}, {0.0},	{0.0}, {DetailModes} },
#ifdef _XBOX
	{ slider, "Overscan",				{&vid_overscan},		{0.84375}, {1.0}, {0.03125}, {NULL} },
#else
	{ discrete, "Fullscreen",			{&vid_fullscreen},		{2.0}, {0.0},	{0.0}, {YesNo} },
#endif
	{ discrete, "32-bit color",			{&vid_32bpp},			{2.0}, {0.0},	{0.0}, {YesNo} },
	{ discrete,	"Widescreen",			{&r_widescreen},		{4.0}, {0.0},	{0.0},  {Widescreen}} ,
	{ redtext,	" ",					{NULL},					{0.0}, {0.0},	{0.0}, {NULL} },
	{ screenres, NULL,					{NULL},					{0.0}, {0.0},	{0.0}, {NULL} },
	{ screenres, NULL,					{NULL},					{0.0}, {0.0},	{0.0}, {NULL} },
	{ screenres, NULL,					{NULL},					{0.0}, {0.0},	{0.0}, {NULL} },
	{ screenres, NULL,					{NULL},					{0.0}, {0.0},	{0.0}, {NULL} },
	{ screenres, NULL,					{NULL},					{0.0}, {0.0},	{0.0}, {NULL} },
	{ screenres, NULL,					{NULL},					{0.0}, {0.0},	{0.0}, {NULL} },
	{ screenres, NULL,					{NULL},					{0.0}, {0.0},	{0.0}, {NULL} },
	{ screenres, NULL,					{NULL},					{0.0}, {0.0},	{0.0}, {NULL} },
	{ screenres, NULL,					{NULL},					{0.0}, {0.0},	{0.0}, {NULL} },
	{ redtext,  VMEnterText,			{NULL},					{0.0}, {0.0},	{0.0}, {NULL} },
	{ redtext,	" ",					{NULL},					{0.0}, {0.0},	{0.0}, {NULL} },
	{ redtext,  VMTestText,				{NULL},					{0.0}, {0.0},	{0.0}, {NULL} },
	{ redtext,	" ",					{NULL},					{0.0}, {0.0},	{0.0}, {NULL} }
};

#define VM_DEPTHITEM	0
#define VM_RESSTART		5
#define VM_ENTERLINE	15
#define VM_TESTLINE		17

menu_t ModesMenu = {
	"M_VIDMOD",
	5,
	STACKARRAY_LENGTH(ModesItems),
	130,
	ModesItems,
	0,
	0,
	NULL
};

static cvar_t *flagsvar;

EXTERN_CVAR(ui_dimcolor)

// [Russell] - Modified to send new colours
static void M_SendUINewColor (int red, int green, int blue)
{
	char command[24];

	sprintf (command, "ui_dimcolor \"%02x %02x %02x\"", red, green, blue);
	AddCommandString (command);
}

static void M_SlideUIRed (int val)
{
	int color = V_GetColorFromString(NULL, ui_dimcolor.cstring());
	int red = val;

	M_SendUINewColor (red, GPART(color), BPART(color));
}

static void M_SlideUIGreen (int val)
{
    int color = V_GetColorFromString(NULL, ui_dimcolor.cstring());
	int green = val;

	M_SendUINewColor (RPART(color), green, BPART(color));
}

static void M_SlideUIBlue (int val)
{
    int color = V_GetColorFromString(NULL, ui_dimcolor.cstring());
	int blue = val;

	M_SendUINewColor (RPART(color), GPART(color), blue);
}

//
//		Set some stuff up for the video modes menu
//

void M_OptInit (void)
{
	int currval = 0, dummy1, dummy2, i;
	char name[24];

	I_StartModeIterator ();
		if (I_NextMode (&dummy1, &dummy2))
		{
			Depths[currval].value = currval;
			delete[] Depths[currval].name;
			Depths[currval].name = copystring (name);
			currval++;
		}

	switch (I_DisplayType ())
	{
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
void M_ChangeMessages (void)
{
	if (show_messages)
	{
		Printf (128, "%s\n", GStrings(MSGOFF));
		show_messages.Set (0.0f);
	}
	else
	{
		Printf (128, "%s\n", GStrings(MSGON));
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
	S_Sound (CHAN_INTERFACE, "plats/pt1_mid", 1, ATTN_NONE);
}
END_COMMAND (sizedown)

BEGIN_COMMAND (sizeup)
{
	M_SizeDisplay(1.0);
	S_Sound (CHAN_INTERFACE, "plats/pt1_mid", 1, ATTN_NONE);
}
END_COMMAND (sizeup)

void M_BuildKeyList (menuitem_t *item, int numitems)
{
	int i;

	for (i = 0; i < numitems; i++, item++)
	{
		if (item->type == control)
			C_GetKeysForCommand (item->e.command, &item->b.key1, &item->c.key2);
	}
}

void M_SwitchMenu (menu_t *menu)
{
	int i, widest = 0, thiswidth;
	menuitem_t *item;

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
		for (i = 0; i < menu->numitems; i++)
		{
			item = menu->items + i;
			if (item->type != whitetext && item->type != redtext)
			{
				thiswidth = V_StringWidth (item->label);
				if (thiswidth > widest)
					widest = thiswidth;
			}
		}
		menu->indent = widest + 6;
	}

	flagsvar = NULL;
}

bool M_StartOptionsMenu (void)
{
	M_SwitchMenu (&OptionMenu);
	return true;
}

void M_DrawSlider (int x, int y, float leftval, float rightval, float cur)
{
	if (leftval < rightval)
		cur = clamp(cur, leftval, rightval);
	else
		cur = clamp(cur, rightval, leftval);

	float dist = (cur - leftval) / (rightval - leftval);

	screen->DrawPatchClean (W_CachePatch ("LSLIDE"), x, y);
	for (int i = 1; i < 11; i++)
		screen->DrawPatchClean (W_CachePatch ("MSLIDE"), x + i*8, y);
	screen->DrawPatchClean (W_CachePatch ("RSLIDE"), x + 88, y);

	screen->DrawPatchClean (W_CachePatch ("CSLIDE"), x + 5 + (int)(dist * 78.0), y);
}

int M_FindCurVal (float cur, value_t *values, int numvals)
{
	int v;

	for (v = 0; v < numvals; v++)
		if (values[v].value == cur)
			break;

	return v;
}

void M_OptDrawer (void)
{
	int color;
	int y, width, i, x, ytop;
	int x1,y1,x2,y2;
	int theight = 0;
	menuitem_t *item;
	patch_t *title;
	
	x1 = (screen->width / 2)-(160*CleanXfac);
	y1 = (screen->height / 2)-(100*CleanYfac);
	
    x2 = (screen->width / 2)+(160*CleanXfac);
	y2 = (screen->height / 2)+(100*CleanYfac);
	
	// Background effect
	OdamexEffect(x1,y1,x2,y2);

	title = W_CachePatch (CurrentMenu->title);
	screen->DrawPatchClean (title, 160-title->width()/2, 10);

	y = 15 + title->height();
	ytop = y + CurrentMenu->scrolltop * 8;

	for (i = 0; i < CurrentMenu->numitems && y <= 192 - theight; i++, y += 8)	// TIJ
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
						color = CR_GREY;
					else
						color = CR_RED;

					screen->DrawTextCleanMove (color, 104 * x + 20, y, str);
				}
			}

			if (i == CurrentItem && ((item->a.selmode != -1 && (skullAnimCounter < 6 || WaitingForKey)) || WaitingForAxis || testingmode))
			{
				screen->DrawPatchClean (W_CachePatch ("LITLCURS"), item->a.selmode * 104 + 8, y);
			}
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
				x = 160 - width / 2;
				color = CR_RED;
				break;

			case whitetext:
				x = 160 - width / 2;
				color = CR_GREY;
				break;

			case bricktext:
				x = 160 - width / 2;
				color = CR_BRICK;
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
			{
				int v, vals;

				vals = (int)item->b.leftval;
				v = M_FindCurVal (item->a.cvar->value(), item->e.values, vals);

				if (v == vals)
				{
					screen->DrawTextCleanMove (CR_GREY, CurrentMenu->indent + 14, y, "Unknown");
				}
				else
				{
					if (item->type == cdiscrete)
						screen->DrawTextCleanMove (v, CurrentMenu->indent + 14, y, item->e.values[v].name);
					else
						screen->DrawTextCleanMove (CR_GREY, CurrentMenu->indent + 14, y, item->e.values[v].name);
				}

			}
			break;

			case nochoice:
				screen->DrawTextCleanMove (CR_GOLD, CurrentMenu->indent + 14, y,
										   (item->e.values[(int)item->b.leftval]).name);
				break;

			case slider:
				M_DrawSlider (CurrentMenu->indent + 8, y, item->b.leftval, item->c.rightval, item->a.cvar->value());
				break;

			case control:
			{
				std::string desc = C_NameKeys (item->b.key1, item->c.key2);
				screen->DrawTextCleanMove (CR_GREY, CurrentMenu->indent + 14, y, desc.c_str());
			}
			break;

			case bitflag:
			{
				value_t *value;
				const char *str;

				if (item->b.leftval)
					value = NoYes;
				else
					value = YesNo;

				if (*item->e.flagint & item->a.flagmask)
					str = value[1].name;
				else
					str = value[0].name;

				screen->DrawTextCleanMove (CR_GREY, CurrentMenu->indent + 14, y, str);
			}
			break;

			case joyactive:
			{
				int         numjoy;
				std::string joyname;

				numjoy = I_GetJoystickCount();

				if((int)item->a.cvar->value() > numjoy)
					item->a.cvar->Set(0.0);

				if(!numjoy)
					joyname = "No device detected";
				else
				{
					joyname = item->a.cvar->cstring();
					joyname += ": " + I_GetJoystickNameFromIndex((int)item->a.cvar->value());
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

void M_OptResponder (event_t *ev)
{
	menuitem_t *item;
	int ch = ev->data1;
	const char *cmd = C_GetBinding(ch);

	item = CurrentMenu->items + CurrentItem;

	// Waiting on a key press for control binding
	if (WaitingForKey)
	{
		if(ev->type == ev_keydown)
		{
#ifdef _XBOX
			if (ch != KEY_ESCAPE && ch != KEY_JOY9)
#else
			if (ch != KEY_ESCAPE)
#endif
			{
				C_ChangeBinding (item->e.command, ch);
				M_BuildKeyList (CurrentMenu->items, CurrentMenu->numitems);
			}
			WaitingForKey = false;
			CurrentMenu->items[0].label = OldContMessage;
			CurrentMenu->items[0].type = OldContType;
			return;
		}
	}

	// Waiting on an analog axis motion for setting analog control
	if (WaitingForAxis)
	{
		if(ev->type == ev_keydown)
		{
			if(ch == KEY_ESCAPE)
			{
				WaitingForAxis = false;
				CurrentMenu->items[8].label = OldAxisMessage;
				CurrentMenu->items[8].type = OldAxisType;
			}
		}
		else if (ev->type == ev_joystick)
		{
			if(ev->data1 == 0) // Analog Motion
			{
				// Require the control to be activated to at least the half-way point
				// to make sure we get the one that is intended -- Hyper_Eye
				if( (ev->data3 > (SHRT_MAX / 2)) || (ev->data3 < (SHRT_MIN / 2)) )
				{
					if( (ev->data2 == (int)joy_forwardaxis) && 
							strcmp(joy_forwardaxis.name(), item->a.cvar->name()) )
						joy_forwardaxis.Set(item->a.cvar->value());
					else if( (ev->data2 == (int)joy_strafeaxis) && 
							strcmp(joy_strafeaxis.name(), item->a.cvar->name()) )
						joy_strafeaxis.Set(item->a.cvar->value());
					else if( (ev->data2 == (int)joy_turnaxis) && 
							strcmp(joy_turnaxis.name(), item->a.cvar->name()) )
						joy_turnaxis.Set(item->a.cvar->value());
					else if( (ev->data2 == (int)joy_lookaxis) && 
							strcmp(joy_lookaxis.name(), item->a.cvar->name()) )
						joy_lookaxis.Set(item->a.cvar->value());

					item->a.cvar->Set(ev->data2);
					WaitingForAxis = false;
					CurrentMenu->items[8].label = OldAxisMessage;
					CurrentMenu->items[8].type = OldAxisType;
				}
			}
		}
		return;
	}

	if (item->type == bitflag && flagsvar &&
		(ch == KEY_LEFTARROW || ch == KEY_RIGHTARROW || ch == KEY_ENTER)
		&& !demoplayback)
	{
			int newflags = *item->e.flagint ^ item->a.flagmask;
			char val[16];

			sprintf (val, "%d", newflags);
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

	switch (ch)
	{
		case KEY_HAT3:
		case KEY_DOWNARROW:
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
						 CurrentMenu->items[CurrentItem].type == bricktext ||
						 (CurrentMenu->items[CurrentItem].type == screenres &&
						  !CurrentMenu->items[CurrentItem].b.res1));

				if (CurrentMenu->items[CurrentItem].type == screenres)
					CurrentMenu->items[CurrentItem].a.selmode = modecol;

				S_Sound (CHAN_INTERFACE, "plats/pt1_stop", 1, ATTN_NONE);
			}
			break;

		case KEY_HAT1:
		case KEY_UPARROW:
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
					}
					if (CurrentItem < 0)
					{
						CurrentMenu->scrollpos = MAX (0,CurrentMenu->numitems - 22 + CurrentMenu->scrolltop);
						CurrentItem = CurrentMenu->numitems - 1;
					}
				} while (CurrentMenu->items[CurrentItem].type == redtext ||
						 CurrentMenu->items[CurrentItem].type == whitetext ||
						 CurrentMenu->items[CurrentItem].type == bricktext ||
						 (CurrentMenu->items[CurrentItem].type == screenres &&
						  !CurrentMenu->items[CurrentItem].b.res1));

				if (CurrentMenu->items[CurrentItem].type == screenres)
					CurrentMenu->items[CurrentItem].a.selmode = modecol;

				S_Sound (CHAN_INTERFACE, "plats/pt1_stop", 1, ATTN_NONE);
			}
			break;
			
		case KEY_PGUP:
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
						   CurrentMenu->items[CurrentItem].type == bricktext ||
						   (CurrentMenu->items[CurrentItem].type == screenres &&
							!CurrentMenu->items[CurrentItem].b.res1))
					{
						++CurrentItem;
					}
					S_Sound (CHAN_INTERFACE, "plats/pt1_stop", 1, ATTN_NONE);
				}
			}
			break;

		case KEY_PGDN:
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
						   CurrentMenu->items[CurrentItem].type == bricktext ||
						   (CurrentMenu->items[CurrentItem].type == screenres &&
							!CurrentMenu->items[CurrentItem].b.res1))
					{
						++CurrentItem;
					}
					S_Sound (CHAN_INTERFACE, "plats/pt1_stop", 1, ATTN_NONE);
				}
			}
			break;
		
		case KEY_HAT4:
		case KEY_LEFTARROW:
			switch (item->type)
			{
				case slider:
					{
						float newval = item->a.cvar->value() - item->d.step;

						if (item->b.leftval < item->c.rightval)
							newval = MAX(newval, item->b.leftval);
						else
							newval = MIN(newval, item->b.leftval);

						if (item->e.cfunc)
							item->e.cfunc (item->a.cvar, newval);
						else
							item->a.cvar->Set (newval);
					}
					S_Sound (CHAN_INTERFACE, "plats/pt1_mid", 1, ATTN_NONE);
					break;

				case discrete:
				case cdiscrete:
					{
						int cur;
						int numvals;

						numvals = (int)item->b.leftval;
						cur = M_FindCurVal (item->a.cvar->value(), item->e.values, numvals);
						if (--cur < 0)
							cur = numvals - 1;

						item->a.cvar->Set (item->e.values[cur].value);

						// Hack hack. Rebuild list of resolutions
						if (item->e.values == Depths)
							BuildModesList (screen->width, screen->height, DisplayBits);
					}
					S_Sound (CHAN_INTERFACE, "plats/pt1_mid", 1, ATTN_NONE);
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
					S_Sound (CHAN_INTERFACE, "plats/pt1_stop", 1, ATTN_NONE);
					break;

				case joyactive:
					{
						int         numjoy;

						numjoy = I_GetJoystickCount();

						if((int)item->a.cvar->value() > numjoy)
							item->a.cvar->Set(0.0);
						else if((int)item->a.cvar->value() > 0)
							item->a.cvar->Set(item->a.cvar->value() - 1);
					}
					S_Sound (CHAN_INTERFACE, "plats/pt1_mid", 1, ATTN_NONE);
					break;

				default:
					break;
			}
			break;

		case KEY_HAT2:
		case KEY_RIGHTARROW:
			switch (item->type)
			{
				case slider:
					{
						float newval = item->a.cvar->value() + item->d.step;

						if (item->b.leftval < item->c.rightval)
							newval = MIN(newval, item->c.rightval);
						else
							newval = MAX(newval, item->c.rightval);

						if (item->e.cfunc)
							item->e.cfunc (item->a.cvar, newval);
						else
							item->a.cvar->Set (newval);
					}
					S_Sound (CHAN_INTERFACE, "plats/pt1_mid", 1, ATTN_NONE);
					break;

				case discrete:
				case cdiscrete:
					{
						int cur;
						int numvals;

						numvals = (int)item->b.leftval;
						cur = M_FindCurVal (item->a.cvar->value(), item->e.values, numvals);
						if (++cur >= numvals)
							cur = 0;

						item->a.cvar->Set (item->e.values[cur].value);

						// Hack hack. Rebuild list of resolutions
						if (item->e.values == Depths)
							BuildModesList (screen->width, screen->height, DisplayBits);
					}
					S_Sound (CHAN_INTERFACE, "plats/pt1_mid", 1, ATTN_NONE);
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
					S_Sound (CHAN_INTERFACE, "plats/pt1_stop", 1, ATTN_NONE);
					break;

				case joyactive:
					{
						int         numjoy;

						numjoy = I_GetJoystickCount();

						if((int)item->a.cvar->value() >= numjoy)
							item->a.cvar->Set(0.0);
						else if((int)item->a.cvar->value() < (numjoy - 1))
							item->a.cvar->Set(item->a.cvar->value() + 1);

					}
					S_Sound (CHAN_INTERFACE, "plats/pt1_mid", 1, ATTN_NONE);
					break;

				default:
					break;
			}
			break;

#ifdef _XBOX
		case KEY_JOY9: // Start button
#endif
		case KEY_BACKSPACE:
			if (item->type == control)
			{
				C_UnbindACommand (item->e.command);
				item->b.key1 = item->c.key2 = 0;
			}
			break;

		case KEY_JOY1:
		case KEY_ENTER:
			if (CurrentMenu == &ModesMenu)
			{
				if (!(item->type == screenres && GetSelectedSize (CurrentItem, &NewWidth, &NewHeight)))
				{
					NewWidth = screen->width;
					NewHeight = screen->height;
				}
				NewBits = (int)vid_32bpp ? 32 : 8;
				setmodeneeded = true;
				S_Sound (CHAN_INTERFACE, "weapons/pistol", 1, ATTN_NONE);
				vid_defwidth.Set ((float)NewWidth);
				vid_defheight.Set ((float)NewHeight);
				SetModesMenu (NewWidth, NewHeight, NewBits);
			}
			else if (item->type == more && item->e.mfunc)
			{
				CurrentMenu->lastOn = CurrentItem;
				S_Sound (CHAN_INTERFACE, "weapons/pistol", 1, ATTN_NONE);
				item->e.mfunc();
			}
			else if (item->type == discrete || item->type == cdiscrete)
			{
				int cur;
				int numvals;

				numvals = (int)item->b.leftval;
				cur = M_FindCurVal (item->a.cvar->value(), item->e.values, numvals);
				if (++cur >= numvals)
					cur = 0;

				item->a.cvar->Set (item->e.values[cur].value);

				// Hack hack. Rebuild list of resolutions
				if (item->e.values == Depths)
					BuildModesList (screen->width, screen->height, DisplayBits);

				S_Sound (CHAN_INTERFACE, "plats/pt1_mid", 1, ATTN_NONE);
			}
			else if (item->type == control)
			{
				WaitingForKey = true;
				OldContMessage = CurrentMenu->items[0].label;
				OldContType = CurrentMenu->items[0].type;
				CurrentMenu->items[0].label = "Press new key for control or ESC to cancel";
				CurrentMenu->items[0].type = redtext;
			}
			else if (item->type == listelement)
			{
				CurrentMenu->lastOn = CurrentItem;
				S_Sound (CHAN_INTERFACE, "weapons/pistol", 1, ATTN_NONE);
				item->e.lfunc (CurrentItem);
			}
			else if (item->type == joyaxis)
			{
				WaitingForAxis = true;
				OldAxisMessage = CurrentMenu->items[8].label;
				OldAxisType = CurrentMenu->items[8].type;
				CurrentMenu->items[8].label = "Activate desired analog axis or ESC to cancel";
				CurrentMenu->items[8].type = redtext;
			}
			else if (item->type == screenres)
			{
			}
			break;

		case KEY_JOY2:
		case KEY_ESCAPE:
			CurrentMenu->lastOn = CurrentItem;
			M_PopMenuStack ();
			break;

		default:
#ifdef _XBOX
			if (ev->data2 == 't' || ev->data2 == KEY_JOY3)
#else
			if (ev->data2 == 't')
#endif
			{
				// Test selected resolution
				if (CurrentMenu == &ModesMenu)
				{
					if (!(item->type == screenres &&
						GetSelectedSize (CurrentItem, &NewWidth, &NewHeight)))
					{
						NewWidth = screen->width;
						NewHeight = screen->height;
					}
					OldWidth = screen->width;
					OldHeight = screen->height;
					OldBits = DisplayBits;
					NewBits = (int)vid_32bpp ? 32 : 8;
					setmodeneeded = true;
					testingmode = I_GetTime() + 5 * TICRATE;
					S_Sound (CHAN_INTERFACE, "weapons/pistol", 1, ATTN_NONE);
					SetModesMenu (NewWidth, NewHeight, NewBits);
				}
			}
			break;
	}

	if (CurrentMenu->refreshfunc)
		(*CurrentMenu->refreshfunc)();
}

static void GoToConsole (void)
{
	M_ClearMenus ();
	C_ToggleConsole ();
}

static void UpdateStuff (void)
{
	M_SizeDisplay (0.0);
}

void Reset2Defaults (void)
{
	AddCommandString ("unbindall; binddefaults");
	cvar_t::C_SetCVarsToDefaults(CVAR_ARCHIVE | CVAR_CLIENTARCHIVE);
	UpdateStuff();
}

void Reset2Saved (void)
{
	std::string cmd = "exec " + C_QuoteString(M_GetConfigPath());
	AddCommandString(cmd);
	UpdateStuff();
}

static void StartMessagesMenu (void)
{
	M_SwitchMenu (&MessagesMenu);
}

static void StartAutomapMenu (void)
{
	M_SwitchMenu (&AutomapMenu);
}

void ResetCustomColors (void)
{
	AddCommandString ("resetcustomcolors");
}

void MouseSetup (void) // [Toke] for mouse menu
{
	previous_mouse_type = mouse_type;
	M_UpdateMouseOptions();
	M_SwitchMenu (&MouseMenu);
}

void JoystickSetup (void)
{
	M_SwitchMenu (&JoystickMenu);
}

static void CustomizeControls (void)
{
	M_BuildKeyList (ControlsMenu.items, ControlsMenu.numitems);
	M_SwitchMenu (&ControlsMenu);
}

// [Russell] - Hack for getting to the player setup menu, doesn't
// record the last menu though, unfortunately
static void PlayerSetup (void)
{
    M_ClearMenus ();
    M_StartControlPanel ();
	M_PlayerSetup(0);
}

BEGIN_COMMAND (menu_keys)
{
	M_StartControlPanel ();
	S_Sound (CHAN_INTERFACE, "switches/normbutn", 1, ATTN_NONE);
	OptionsActive = true;
	CustomizeControls();
}
END_COMMAND (menu_keys)

static void VideoOptions (void)
{
	M_SwitchMenu (&VideoMenu);
}

void SoundOptions (void) // [Ralphis] for sound menu
{
	M_SwitchMenu (&SoundMenu);
}

void CompatOptions (void) // [Ralphis] for compatibility menu
{
	M_SwitchMenu (&CompatMenu);
}

void NetworkOptions (void)
{
	M_SwitchMenu (&NetworkMenu);
}

void WeaponOptions (void)
{
	M_SwitchMenu (&WeaponMenu);
}

BEGIN_COMMAND (menu_display)
{
	M_StartControlPanel ();
	S_Sound (CHAN_INTERFACE, "switches/normbutn", 1, ATTN_NONE);
	OptionsActive = true;
	M_SwitchMenu (&VideoMenu);
}
END_COMMAND (menu_display)

static void BuildModesList (int hiwidth, int hiheight, int hi_bits)
{
	char strtemp[32];
    const char **str = NULL;
	int	 i, c;
	int	 width, height;

	I_StartModeIterator ();

	for (i = VM_RESSTART; ModesItems[i].type == screenres; i++)
	{
		ModesItems[i].e.highlight = -1;
		for (c = 0; c < 3; c++)
		{
			switch (c)
			{
				case 0:
					str = &ModesItems[i].b.res1;
					break;
				case 1:
					str = &ModesItems[i].c.res2;
					break;
				case 2:
					str = &ModesItems[i].d.res3;
					break;
			}
			if (I_NextMode (&width, &height))
			{
				if (width == hiwidth && height == hiheight)
					ModesItems[i].e.highlight = ModesItems[i].a.selmode = c;

				sprintf (strtemp, "%dx%d", width, height);
				ReplaceString (str, strtemp);
			}
			else
			{
				/*if (*str) // denis - ReplaceString is no longer leaky...
				{
					free (*str);
				}*/
				*str = NULL;
			}
		}
	}
}

void M_RefreshModesList ()
{
	BuildModesList (screen->width, screen->height, DisplayBits);
}

static BOOL GetSelectedSize (int line, int *width, int *height)
{
	int i, stopat;

	if (ModesItems[line].type != screenres)
	{
		return false;
	}
	else
	{
		I_StartModeIterator ();

		stopat = (line - VM_RESSTART) * 3 + ModesItems[line].a.selmode + 1;

		for (i = 0; i < stopat; i++)
			if (!I_NextMode (width, height))
				return false;
	}

	return true;
}

static void SetModesMenu (int w, int h, int bits)
{
	char strtemp[64];

	if (!testingmode)
	{
		ModesItems[VM_ENTERLINE].label = VMEnterText;
		ModesItems[VM_TESTLINE].label = VMTestText;
	}
	else
	{
		sprintf (strtemp, "TESTING %dx%dx%d", w, h, bits);
		ModesItems[VM_ENTERLINE].label = copystring (strtemp);
		ModesItems[VM_TESTLINE].label = "Please wait 5 seconds...";
	}

	BuildModesList (w, h, bits);
}

//
// void M_ModeFlashTestText (void)
//
// Flashes the video mode testing text
void M_ModeFlashTestText (void)
{
    if (ModesItems[VM_TESTLINE].label[0] == ' ')
    {
		ModesItems[VM_TESTLINE].label = "Please wait 5 seconds...";
    }
    else
    {
        ModesItems[VM_TESTLINE].label = " ";
    }
}

void M_RestoreMode (void)
{
	NewWidth = OldWidth;
	NewHeight = OldHeight;
	NewBits = OldBits;
	setmodeneeded = true;
	testingmode = 0;
	SetModesMenu (OldWidth, OldHeight, OldBits);
}

static void SetVidMode (void)
{
	SetModesMenu (screen->width, screen->height, DisplayBits);
	if (ModesMenu.items[ModesMenu.lastOn].type == screenres)
	{
		if (ModesMenu.items[ModesMenu.lastOn].a.selmode == -1)
		{
			ModesMenu.items[ModesMenu.lastOn].a.selmode++;
		}
	}
	M_SwitchMenu (&ModesMenu);
}

BEGIN_COMMAND (menu_video)
{
	M_StartControlPanel ();
	S_Sound (CHAN_INTERFACE, "switches/normbutn", 1, ATTN_NONE);
	OptionsActive = true;
	SetVidMode ();
}
END_COMMAND (menu_video)

VERSION_CONTROL (m_options_cpp, "$Id$")



