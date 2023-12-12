// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1998-2006 by Randy Heit (ZDoom 1.22).
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
#include "minilzo.h"

#include "c_console.h"
#include "c_dispatch.h"
#include "c_bind.h"
#include "cl_responderkeys.h"
#include "cmdlib.h"

#include "i_system.h"
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


// [Ralphis - Menu] Compatibility Menu
EXTERN_CVAR (hud_targetnames)
EXTERN_CVAR (hud_hidespyname)
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
EXTERN_CVAR (co_allowdropoff)
EXTERN_CVAR (co_realactorheight)
EXTERN_CVAR (wi_oldintermission)
EXTERN_CVAR (co_zdoomphys)
EXTERN_CVAR (co_zdoomsound)
EXTERN_CVAR (co_fixweaponimpacts)
EXTERN_CVAR (cl_deathcam)
EXTERN_CVAR (co_fineautoaim)
EXTERN_CVAR (co_nosilentspawns)
EXTERN_CVAR (co_boomphys)			// [ML] Roll-up of various compat options
EXTERN_CVAR (co_removesoullimit)
EXTERN_CVAR (co_blockmapfix)
EXTERN_CVAR (co_globalsound)
EXTERN_CVAR(hud_feedobits)
EXTERN_CVAR(hud_feedtime)

// [Toke - Menu] New Menu Stuff.
void MouseSetup (void);
EXTERN_CVAR (mouse_sensitivity)
EXTERN_CVAR (m_pitch)
EXTERN_CVAR (novert)
EXTERN_CVAR (m_side)
EXTERN_CVAR (m_forward)

// [Ralphis - Menu] Sound Menu
EXTERN_CVAR (snd_midireset)
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
EXTERN_CVAR (joy_fastsensitivity)
EXTERN_CVAR (joy_invert)
EXTERN_CVAR (joy_freelook)
EXTERN_CVAR (joy_deadzone)

// Network Options
EXTERN_CVAR (cl_interp)
EXTERN_CVAR (cl_prednudge)
EXTERN_CVAR (cl_predictpickup)
EXTERN_CVAR (cl_predictsectors)
EXTERN_CVAR (cl_predictweapons)
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

value_t HideShow[2] = {
	{ 0.0, "Hide" },
	{ 1.0, "Show" }
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

value_t DemoRestrictions[2] = {
	{ 0.0, "Restrict" },
	{ 1.0, "Allow" }
};

static value_t DoomOrOdamex[2] =
{
	{ 0.0, "Odamex" },
	{ 1.0, "Doom" }
};

menu_t  *CurrentMenu;
int		CurrentItem;
bool configuring_controls = false;
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
 	{ discrete, "Skip Boot Window",		{&i_skipbootwin},		{2.0}, {0.0},	{0.0}, {OnOff} },
 	{ redtext,	" ",					{NULL},					{0.0}, {0.0},	{0.0}, {NULL} },
 	{ more,		"Reset to defaults",	{NULL},					{0.0}, {0.0},	{0.0}, {(value_t *)Reset2Defaults} },
 	{ more,		"Reset to last saved",	{NULL},					{0.0}, {0.0},	{0.0}, {(value_t *)Reset2Saved} }
};

menu_t OptionMenu = {
	"M_OPTTTL",
	0,
	ARRAY_LENGTH(OptionItems),
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
	{ yellowtext,"Basic Movement",		{NULL},	{0.0}, {0.0}, {0.0}, {NULL} },
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
	{ control,	"Alternate Turn",		{NULL}, {0.0}, {0.0}, {0.0}, {(value_t *)"+fastturn"} },
	{ redtext,	" ",					{NULL},	{0.0}, {0.0}, {0.0}, {NULL} },
	{ yellowtext,"Actions",		        {NULL},	{0.0}, {0.0}, {0.0}, {NULL} },
	{ control,	"Fire",					{NULL}, {0.0}, {0.0}, {0.0}, {(value_t *)"+attack"} },
	{ control,	"Use / Open",			{NULL}, {0.0}, {0.0}, {0.0}, {(value_t *)"+use"} },
	{ control,	"Next weapon",			{NULL}, {0.0}, {0.0}, {0.0}, {(value_t *)"weapnext"} },
	{ control,	"Previous weapon",		{NULL}, {0.0}, {0.0}, {0.0}, {(value_t *)"weapprev"} },
	{ redtext,	" ",					{NULL},	{0.0}, {0.0}, {0.0}, {NULL} },
	{ yellowtext,"Weapons",		        {NULL},	{0.0}, {0.0}, {0.0}, {NULL} },
	{ control,	"Fist/Chainsaw",		{NULL}, {0.0}, {0.0}, {0.0}, {(value_t *)"impulse 1"} },
	{ control,	"Pistol",       		{NULL}, {0.0}, {0.0}, {0.0}, {(value_t *)"impulse 2"} },
	{ control,	"Shotgun/SSG",  		{NULL}, {0.0}, {0.0}, {0.0}, {(value_t *)"impulse 3"} },
	{ control,	"Chaingun",     		{NULL}, {0.0}, {0.0}, {0.0}, {(value_t *)"impulse 4"} },
	{ control,	"Rocket Launcher",		{NULL}, {0.0}, {0.0}, {0.0}, {(value_t *)"impulse 5"} },
	{ control,	"Plasma Rifle",   		{NULL}, {0.0}, {0.0}, {0.0}, {(value_t *)"impulse 6"} },
	{ control,	"BFG",          		{NULL}, {0.0}, {0.0}, {0.0}, {(value_t *)"impulse 7"} },
	{ control,	"Chainsaw",     		{NULL}, {0.0}, {0.0}, {0.0}, {(value_t *)"impulse 8"} },
	{ redtext,	" ",					{NULL},	{0.0}, {0.0}, {0.0}, {NULL} },
	{ yellowtext,	"Automap Controls",	{NULL},	{0.0}, {0.0}, {0.0}, {NULL} },
	{ control,		"Toggle Automap",	{NULL}, {0.0}, {0.0}, {0.0}, {(value_t*)"togglemap"} },
	{ mapcontrol,	"Follow Player",	{NULL}, {0.0}, {0.0}, {0.0}, {(value_t*)"am_togglefollow"} },
	{ mapcontrol,	"Toggle Grid",		{NULL}, {0.0}, {0.0}, {0.0}, {(value_t*)"am_grid"} },
	{ mapcontrol,	"Add Marker",		{NULL}, {0.0}, {0.0}, {0.0}, {(value_t*)"am_setmark"} },
	{ mapcontrol,	"Clear Markers",	{NULL}, {0.0}, {0.0}, {0.0}, {(value_t*)"am_clearmarks"} },
	{ mapcontrol,	"Big Automap",		{NULL}, {0.0}, {0.0}, {0.0}, {(value_t*)"am_big"} },
	{ mapcontrol,	"Zoom In",			{NULL}, {0.0}, {0.0}, {0.0}, {(value_t*)"+am_zoomin"} },
	{ mapcontrol,	"Zoom Out",			{NULL}, {0.0}, {0.0}, {0.0}, {(value_t*)"+am_zoomout"} },
	{ mapcontrol,	"Pan Up",			{NULL}, {0.0}, {0.0}, {0.0}, {(value_t*)"+am_panup"} },
	{ mapcontrol,	"Pan Down",			{NULL}, {0.0}, {0.0}, {0.0}, {(value_t*)"+am_pandown"} },
	{ mapcontrol,	"Pan Left",			{NULL}, {0.0}, {0.0}, {0.0}, {(value_t*)"+am_panleft"} },
	{ mapcontrol,	"Pan Right",		{NULL}, {0.0}, {0.0}, {0.0}, {(value_t*)"+am_panright"} },
	{ redtext,	" ",					{NULL},	{0.0}, {0.0}, {0.0}, {NULL} },
	{ yellowtext,"Advanced Movement",    {NULL},	{0.0}, {0.0}, {0.0}, {NULL} },
	{ control,	"Fly / Swim up",		{NULL},	{0.0}, {0.0}, {0.0}, {(value_t *)"+moveup"} },
	{ control,	"Fly / Swim down",		{NULL},	{0.0}, {0.0}, {0.0}, {(value_t *)"+movedown"} },
	{ control,	"Toggle flying",		{NULL},	{0.0}, {0.0}, {0.0}, {(value_t *)"fly"} },
	{ control,	"Look up",				{NULL}, {0.0}, {0.0}, {0.0}, {(value_t *)"+lookup"} },
	{ control,	"Look down",			{NULL}, {0.0}, {0.0}, {0.0}, {(value_t *)"+lookdown"} },
	{ control,	"Center view",			{NULL}, {0.0}, {0.0}, {0.0}, {(value_t *)"centerview"} },
	{ control,	"Mouse look",			{NULL}, {0.0}, {0.0}, {0.0}, {(value_t *)"+mlook"} },
	{ control,	"Keyboard look",		{NULL}, {0.0}, {0.0}, {0.0}, {(value_t *)"+klook"} },
	{ redtext,	" ",					{NULL},	{0.0}, {0.0}, {0.0}, {NULL} },
	{ yellowtext,"Multiplayer",		    {NULL},	{0.0}, {0.0}, {0.0}, {NULL} },
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
	{ yellowtext,"Menus",				{NULL},	{0.0}, {0.0}, {0.0}, {NULL} },
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
	{ yellowtext,	"Netdemo Controls",	{NULL},	{0.0}, {0.0}, {0.0}, {NULL} },
	{ netdemocontrol,"Pause Netdemo",	{NULL}, {0.0}, {0.0}, {0.0}, {(value_t*)"netpause"} },
    { netdemocontrol, "Fast Forward", {NULL}, {0.0}, {0.0}, {0.0}, {(value_t*)"netff"}},
    { netdemocontrol, "Rewind", {NULL}, {0.0}, {0.0}, {0.0}, {(value_t*)"netrew"}},
    { netdemocontrol, "Next map", {NULL}, {0.0}, {0.0}, {0.0}, {(value_t*)"netnextmap"}},
	{ netdemocontrol,	"Previous map",	{NULL}, {0.0}, {0.0}, {0.0}, {(value_t*)"netprevmap"} },
	{ redtext,	" ",					{NULL},	{0.0}, {0.0}, {0.0}, {NULL} },
	{ yellowtext,"Other",				{NULL},	{0.0}, {0.0}, {0.0}, {NULL} },
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
	ARRAY_LENGTH(ControlsItems),
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


static menuitem_t MouseItems[] =
{
	{ slider,	"Overall Sensitivity"			, {&mouse_sensitivity},	{0.05},	{2.5},		{0.05},		{NULL}},
	{ slider,	"Freelook Sensitivity"			, {&m_pitch},			{0.05},	{2.5},		{0.05},		{NULL}},

	{ redtext,	" "								, {NULL},				{0.0},	{0.0},		{0.0},		{NULL}},
	{ discrete,	"Always FreeLook"				, {&cl_mouselook},		{2.0},	{0.0},		{0.0},		{OnOff}},
	{ discrete,	"Invert Mouse"					, {&invertmouse},		{2.0},	{0.0},		{0.0},		{OnOff}},
	{ discrete, "Auto SR50 on Strafe"			, {&in_autosr50},		{2.0},	{0.0},		{0.0},		{OnOff}}, // [AM] Does not belong here
	{ discrete, "Lookspring"					, {&lookspring},		{2.0},	{0.0},		{0.0},		{OnOff}},
	{ redtext,	" "								, {NULL},				{0.0},	{0.0},		{0.0},		{NULL}},
	{ discrete,	"Horizontal Movement"			, {&lookstrafe},		{2.0},	{0.0},		{0.0},		{OnOff}},
	{ discrete,	"Vertical Movement"				, {&novert},			{2.0},	{0.0},		{0.0},		{OffOn}},
	{ slider,	"Horizontal Movement Speed"		, {&m_side},			{0.0},	{15},		{0.5},		{NULL}},
	{ slider,	"Vertical Movement Speed"		, {&m_forward},			{0.0},	{15},		{0.5},		{NULL}},
	{ redtext,	" "								, {NULL},				{0.0},	{0.0},		{0.0},		{NULL}},
	{ more,		"Reset mouse to defaults"		, {NULL},				{0.0},	{0.0},		{0.0},		{(value_t *)M_ResetMouseValues}},
};


menu_t MouseMenu = {
    "M_MOUSET",
    0,
    ARRAY_LENGTH(MouseItems),
    177,
    MouseItems,
	0,
	0,
	NULL
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
	{ redtext	,	" "										, {NULL},				{0.0},		{0.0},		{0.0},		{NULL}						},
	{ whitetext	,	"Sensitivity Settings"					, {NULL}, 				{0.0}, 		{0.0}, 		{0.0}, 		{NULL} 						},
	{ slider	,	"Turn Sensitivity"						, {&joy_sensitivity},	{1.0},		{30.0},		{1.0},		{NULL}						},
	{ slider	,	"Alt. Turn Sensitivity"					, {&joy_fastsensitivity},	{1.0},		{30.0},		{1.0},		{NULL}						},
	{ slider	,	"Joystick Deadzone"						, {&joy_deadzone},		{0.0},		{0.75},		{0.05},		{NULL}						},
#ifdef SDL12
	{ redtext	,	" "										, {NULL},				{0.0},		{0.0},		{0.0},		{NULL}						},
	{ whitetext	,	"Press ENTER to change"					, {NULL}, 				{0.0}, 		{0.0}, 		{0.0}, 		{NULL} 						},
	{ joyaxis	,	"Walk Analog Axis"						, {&joy_forwardaxis},	{0.0},		{0.0},		{0.0},		{NULL}						},
	{ joyaxis	,	"Strafe Analog Axis"					, {&joy_strafeaxis},	{0.0},		{0.0},		{0.0},		{NULL}						},
	{ joyaxis	,	"Turn Analog Axis"						, {&joy_turnaxis},		{0.0},		{0.0},		{0.0},		{NULL}						},
	{ joyaxis	,	"Look Analog Axis"						, {&joy_lookaxis},		{0.0},		{0.0},		{0.0},		{NULL}						},
#endif
};

menu_t JoystickMenu = {
    "M_JOYSTK",
    0,
    ARRAY_LENGTH(JoystickItems),
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

static value_t MidiReset[] = {
	{ 0.0,			"None" },
	{ 1.0,			"GM" },
	{ 2.0,			"GS" },
	{ 3.0,			"XG" }
};

static value_t VoxType[] = {
	{ 0.0,			"Off" },
	{ 1.0,			"Team Colors" },
	{ 2.0,			"Possessive" }
};

static value_t ChatSndType[] = {
	{ 0.0,			"Disabled" },
	{ 1.0,			"Enabled" },
	{ 2.0,			"Teamchat only" }
};

static float num_mussys = static_cast<float>(ARRAY_LENGTH(MusSys));

EXTERN_CVAR(cl_chatsounds)

static menuitem_t SoundItems[] = {
    { redtext,	" ",					{NULL},	{0.0}, {0.0}, {0.0}, {NULL} },
	{ yellowtext ,   "Sound Levels"                      , {NULL},	            {0.0},      	{0.0},      {0.0},      {NULL} },
	{ slider    ,	"Music Volume"                      , {&snd_musicvolume},	{0.0},      	{1.0},	    {0.015625},      {NULL} },
	{ slider    ,	"Sound Volume"                      , {&snd_sfxvolume},		{0.0},      	{1.0},	    {0.015625},      {NULL} },
	{ slider    ,	"Announcer Volume"             		, {&snd_announcervolume},	{0.0},      {1.0},	    {0.015625},      {NULL} },
	{ discrete  ,   "Stereo Switch"                     , {&snd_crossover},	    {2.0},			{0.0},		{0.0},		{OnOff} },
	{ redtext   ,	" "                                 , {NULL},	            {0.0},      	{0.0},      {0.0},      {NULL} },
	{ yellowtext ,   "Music Options"                     , {NULL},	            {0.0},      	{0.0},      {0.0},      {NULL} },
	{ discrete	,	"Music System Backend"				, {&snd_musicsystem},	{num_mussys},	{0.0},		{0.0},		{MusSys} },
	{ discrete	,	"MIDI Reset"						, {&snd_midireset},		{4.0},			{0.0},		{0.0},		{MidiReset} },
	{ redtext   ,	" "                                 , {NULL},	            {0.0},      	{0.0},      {0.0},      {NULL} },
	{ yellowtext ,   "Sound Options"                     , {NULL},	            {0.0},      	{0.0},      {0.0},      {NULL} },
	{ discrete  ,   "Game SFX"                          , {&snd_gamesfx},		{2.0},			{0.0},		{0.0},		{OnOff} },
	{ discrete  ,   "Announcer Type"                    , {&snd_voxtype},		{3.0},			{0.0},		{0.0},		{VoxType} },
	{ discrete  ,   "Player Connect Alert"              , {&cl_connectalert},	{2.0},			{0.0},		{0.0},		{OnOff} },
	{ discrete  ,   "Player Disconnect Alert"           , {&cl_disconnectalert},{2.0},			{0.0},		{0.0},		{OnOff} },
    { discrete  ,	"Chat sounds"						, {&cl_chatsounds},		{3.0},			{0.0},		{0.0},		{ChatSndType}},

 };

menu_t SoundMenu = {
	"M_SOUND",
	2,
	ARRAY_LENGTH(SoundItems),
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
	{yellowtext, "Gameplay",							{NULL},                  {0.0}, {0.0}, {0.0}, {NULL}},
	{svdiscrete, "Finer-precision Autoaim",        {&co_fineautoaim},       {2.0}, {0.0}, {0.0}, {OnOff}},
	{svdiscrete, "Fix hit detection at grid edges",{&co_blockmapfix},       {2.0}, {0.0}, {0.0}, {OnOff}},
	{svdiscrete, "Remove pain elemental spawn limit",{&co_removesoullimit}, {2.0}, {0.0}, {0.0}, {OnOff}},
	{redtext,   " ",								{NULL},                  {0.0}, {0.0}, {0.0}, {NULL}},
	{yellowtext, "Items and Decoration",				{NULL},                  {0.0}, {0.0}, {0.0}, {NULL}},
	{svdiscrete, "Fix invisible puffs under skies",{&co_fixweaponimpacts},  {2.0}, {0.0}, {0.0}, {OnOff}},
	{svdiscrete, "Items can be walked over/under", {&co_realactorheight},   {2.0}, {0.0}, {0.0}, {OnOff}},
	{svdiscrete, "Items can drop off ledges",      {&co_allowdropoff},      {2.0}, {0.0}, {0.0}, {OnOff}},
	{redtext,   " ",								{NULL},                  {0.0}, {0.0}, {0.0}, {NULL}},
	{yellowtext, "Engine Compatibility",				{NULL},                  {0.0}, {0.0}, {0.0}, {NULL}},
	{svdiscrete, "BOOM actor/sector/line checks",  {&co_boomphys},			 {2.0}, {0.0}, {0.0}, {OnOff}},
	{svdiscrete, "ZDOOM 1.23 physics",             {&co_zdoomphys},         {2.0}, {0.0}, {0.0}, {OnOff}},
	{redtext,   " ",								{NULL},                  {0.0}, {0.0}, {0.0}, {NULL}},
	{yellowtext, "Sound",							{NULL},                  {0.0}, {0.0}, {0.0}, {NULL}},
	{svdiscrete, "Fix silent west spawns",         {&co_nosilentspawns},    {2.0}, {0.0}, {0.0}, {OnOff}},
	{svdiscrete, "ZDoom Sound Response",			{&co_zdoomsound},		 {2.0}, {0.0}, {0.0}, {OnOff}},
    {svdiscrete, "Global Pickup Sounds",			{&co_globalsound},		 {2.0}, {0.0}, {0.0}, {OnOff}},
};

menu_t CompatMenu = {
	"M_COMPAT",
	1,
	ARRAY_LENGTH(CompatItems),
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

static value_t PredictSectors[] = {
	{ 0.0, "None" },
	{ 1.0, "All" },
	{ 2.0, "Only Mine" }
};

static menuitem_t NetworkItems[] = {
    { redtext,	" ",					{NULL},	{0.0}, {0.0}, {0.0}, {NULL} },
	{ yellowtext,	"Adjust Network Settings",		{NULL},				{0.0},		{0.0},		{0.0},		{NULL} },
	{ slider,		"Interpolation time",			{&cl_interp},		{0.0},		{4.0},		{1.0},		{NULL} },
	{ slider,		"Smooth collisions",			{&cl_prednudge},	{1.0},		{0.1},		{-0.1},		{NULL} },
	{ discrete,		"Predict weapon pickups",		{&cl_predictpickup},{2.0},		{0.0},		{0.0},		{OnOff} },
	{ discrete,		"Predict sector actions",		{&cl_predictsectors},{3.0},		{0.0},		{0.0},		{PredictSectors} },
	{ discrete,		"Predict weapon effects",		{&cl_predictweapons},{2.0},		{0.0},		{0.0},		{OnOff} },
	{ redtext,		" ",							{NULL},				{0.0}, 		{0.0}, 		{0.0}, 		{NULL} },
	{ discrete, 	"Download From Server", 		{&cl_serverdownload}, {2.0}, 		{0.0}, 		{0.0}, 		{OnOff} },

	{ redtext,		" ",							{NULL},				{0.0},		{0.0},		{0.0},		{NULL} },
	{ yellowtext,	"Netdemo Settings",				{NULL},				{0.0},		{0.0},		{0.0},		{NULL} },
	{ discrete,		"Autorecord demos",				{&cl_autorecord},	{2.0},		{0.0},		{0.0},		{OnOff} },
	{ discrete,		"Split every map",				{&cl_splitnetdemos},	{2.0},		{0.0},		{0.0},		{OnOff} },

	{ redtext,		" ",							{NULL},	{0.0}, {0.0}, {0.0}, {NULL} },
	{ yellowtext,	"Autorecord filters",			{NULL},				{0.0},		{0.0},		{0.0},		{NULL} },
	{ discrete,		"Cooperation",					{&cl_autorecord_coop},{2.0},		{0.0},		{0.0},		{DemoRestrictions} },
	{ discrete,		"Deathmatch",					{&cl_autorecord_deathmatch},{2.0},		{0.0},		{0.0},		{DemoRestrictions} },
	{ discrete,		"Duel",							{&cl_autorecord_duel},{2.0},		{0.0},		{0.0},		{DemoRestrictions} },
	{ discrete,		"Team Deathmatch",				{&cl_autorecord_teamdm},{2.0},		{0.0},		{0.0},		{DemoRestrictions} },
	{ discrete,		"Capture the Flag",				{&cl_autorecord_ctf},{2.0},		{0.0},		{0.0},		{DemoRestrictions} },
	{ discrete,		"Horde",						{&cl_autorecord_horde},{2.0},		{0.0},		{0.0},		{DemoRestrictions} },
};

menu_t NetworkMenu = {
	"M_NETWRK",
	2,
	ARRAY_LENGTH(NetworkItems),
	177,
	NetworkItems,
	1,
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
	{ 2.0,			"By Preference" },
    { 3.0,			"Attack Cancels PWO"}
};

extern const char *weaponnames[];

static menuitem_t WeaponItems[] = {
	{yellowtext, "Weapon Preferences",  {NULL},               {0.0}, {0.0}, {0.0}, {NULL}},
	{discrete,  "Switch on pickup",    {&cl_switchweapon},   {4.0}, {0.0}, {0.0}, {WeapSwitch}},
	{redtext,   " ",                   {NULL},               {0.0}, {0.0}, {0.0}, {NULL}},
	{yellowtext, "Weapon Switch Order", {NULL},               {0.0}, {0.0}, {0.0}, {NULL}},
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
    {redtext,	" ",				   {NULL},				 {0.0}, {0.0}, {0.0}, {NULL}},
    {yellowtext, "! ! ! NOTICE ! ! !", {NULL}, {0.0}, {0.0}, {0.0}, {NULL}},
    {orangetext, "While playing online, this feature", {NULL},{0.0}, {0.0}, {0.0}, {NULL}},
    {orangetext, "only works when the server allows it!", {NULL}, {0.0}, {0.0}, {0.0}, {NULL}},
};

menu_t WeaponMenu = {
	"M_WEAPON",
	1,
	ARRAY_LENGTH(WeaponItems),
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
static void StartHUDMenu();
static void StartMessagesMenu (void);
static void StartAutomapMenu (void);
void ResetCustomColors (void);

EXTERN_CVAR (am_rotate)
EXTERN_CVAR (am_overlay)
EXTERN_CVAR (am_showmonsters)
EXTERN_CVAR (am_showitems)
EXTERN_CVAR (am_showsecrets)
EXTERN_CVAR (am_showtime)
EXTERN_CVAR (am_classicmapstring)
EXTERN_CVAR (am_usecustomcolors)
EXTERN_CVAR (st_scale)
EXTERN_CVAR (r_stretchsky)
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
EXTERN_CVAR (hud_show_scoreboard_ondeath)
EXTERN_CVAR (hud_demobar)
EXTERN_CVAR(hud_targetnames)
EXTERN_CVAR(hud_hidespyname)

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
    M_SlideUIRed((int)var);
}

CVAR_FUNC_IMPL (ui_transgreen)
{
    M_SlideUIGreen((int)var);
}

CVAR_FUNC_IMPL (ui_transblue)
{
    M_SlideUIBlue((int)var);
}

static menuitem_t VideoItems[] = {
    {more, "Heads-up display", {NULL}, {0.0}, {0.0}, {0.0}, {(value_t*)StartHUDMenu}},
	{ more,		"Messages",				    {NULL},					{0.0}, {0.0},	{0.0},  {(value_t *)StartMessagesMenu} },
	{ more,		"Automap",				    {NULL},					{0.0}, {0.0},	{0.0},  {(value_t *)StartAutomapMenu} },
	{ redtext,	" ",					    {NULL},					{0.0}, {0.0},	{0.0},  {NULL} },
	{ slider,	"Screen size",			    {&screenblocks},	   	{3.0}, {12.0},	{1.0},  {NULL} },
	{ slider,	"Brightness",			    {&gammalevel},			{1.0}, {8.0},	{1.0},  {NULL} },
	{ slider,	"Red Pain Intensity",		{&r_painintensity},		{0.0}, {1.0},	{0.1},  {NULL} },
	{ slider,	"Movement bobbing",			{&cl_movebob},			{0.0}, {1.0},	{0.1},	{NULL} },
	{ slider,   "Weapon Visibility",        {&r_drawplayersprites}, {0.0}, {1.0},   {0.1},  {NULL} },
	{ discrete,	"Visible Spawn Points",		{&cl_showspawns},		{2.0}, {0.0},	{0.0},	{OnOff} },
	{ discrete, "Center weapon when firing",{&cl_centerbobonfire},	{2.0}, {0.0},	{0.0},	{OnOff} },
	{ redtext,	" ",					    {NULL},				    {0.0}, {0.0},	{0.0},  {NULL} },
	{ discrete, "Force Team Color",			{&r_forceteamcolor},	{2.0}, {0.0},	{0.0},	{OnOff} },
	{ redslider,   "Team Color Red",        {&r_teamcolor},  {0.0}, {0.0},   {0.0},  {NULL} },
	{ greenslider, "Team Color Green",      {&r_teamcolor},  {0.0}, {0.0},   {0.0},  {NULL} },
	{ blueslider,  "Team Color Blue",       {&r_teamcolor},  {0.0}, {0.0},   {0.0},  {NULL} },
	{ redtext,	" ",					    {NULL},				    {0.0}, {0.0},	{0.0},  {NULL} },
	{ discrete, "Force Enemy Color",        {&r_forceenemycolor},	{2.0}, {0.0},	{0.0},	{OnOff} },
	{ redslider,   "Enemy Color Red",       {&r_enemycolor},  {0.0}, {0.0},   {0.0},  {NULL} },
	{ greenslider, "Enemy Color Green",     {&r_enemycolor},  {0.0}, {0.0},   {0.0},  {NULL} },
	{ blueslider,  "Enemy Color Blue",      {&r_enemycolor},  {0.0}, {0.0},   {0.0},  {NULL} },
	{ redtext,	" ",					    {NULL},				    {0.0}, {0.0},	{0.0},  {NULL} },
	{ slider,   "UI Background Red",        {&ui_transred},         {0.0}, {255.0}, {16.0}, {NULL} },
	{ slider,   "UI Background Green",      {&ui_transgreen},       {0.0}, {255.0}, {16.0}, {NULL} },
	{ slider,   "UI Background Blue",       {&ui_transblue},        {0.0}, {255.0}, {16.0}, {NULL} },
	{ slider,   "UI Background Visibility", {&ui_dimamount},        {0.0}, {1.0},   {0.1},  {NULL} },
	{ redtext,	" ",					    {NULL},					{0.0}, {0.0},	{0.0},  {NULL} },
	{ discrete, "See killer on Death",			{&cl_deathcam},   {2.0}, {0.0}, {0.0}, {OnOff}},
	{ discrete, "Stretch short skies",	    {&r_stretchsky},	   	{3.0}, {0.0},	{0.0},  {OnOffAuto} },
	{ discrete, "Invuln changes skies",		{&r_skypalette},		{2.0}, {0.0},	{0.0},	{OnOff} },
	{ discrete, "Use softer invuln effect", {&r_softinvulneffect},	{2.0}, {0.0},	{0.0},	{OnOff} },
	{ discrete, "Screen wipe style",	    {&r_wipetype},			{4.0}, {0.0},	{0.0},  {Wipes} },
	{ discrete, "Multiplayer Intermissions",{&wi_oldintermission},	{2.0}, {0.0},	{0.0},  {DoomOrOdamex} },
	{ discrete, "Show loading disk icon",	{&r_loadicon},			{2.0}, {0.0},	{0.0},	{OnOff} },
    { discrete,	"Show DOS ending screen" ,  {&r_showendoom},		{2.0}, {0.0},	{0.0},  {OnOff} },


};

static void M_UpdateDisplayOptions()
{
	const static size_t menu_length = ARRAY_LENGTH(VideoItems);
	const static size_t gamma_index = M_FindCvarInMenu(gammalevel, VideoItems, menu_length);

	// update the parameters for gammalevel based on vid_gammatype (doom or zdoom gamma)
	VideoItems[gamma_index].b.leftval = V_GetMinimumGammaLevel();
	VideoItems[gamma_index].c.rightval = V_GetMaximumGammaLevel();
	VideoItems[gamma_index].d.step = 0.1f;
}

menu_t VideoMenu = {
	"M_VIDEO",
	0,
	ARRAY_LENGTH(VideoItems),
	0,
	VideoItems,
	4,
	0,
	&M_UpdateDisplayOptions
};

/*=======================================
 *
 * HUD Menu
 *
 *=======================================*/

static value_t SecretOptions[] = {
    {0.0, "Off"},
    {1.0, "On (with sounds)"},
    {2.0, "On (w/o sounds)"},
    {3.0, "Own only"},
};

static value_t TimerStyles[] = {
    {0.0, "No Timer"}, {1.0, "Count Down"}, {2.0, "Count Up"}};

static value_t FlagHelds[] = {{0.0, "Off"}, {1.0, "Complete"}, {2.0, "Simple"}};

static value_t Crosshairs[] = {{0.0, "None"}, {1.0, "Cross 1"}, {2.0, "Cross 2"},
                               {3.0, "X"},    {4.0, "Diamond"}, {5.0, "Dot"},
                               {6.0, "Box"},  {7.0, "Angle"},   {8.0, "Big Thing"}};

static menuitem_t HUDItems[] = {
    {yellowtext, "Status Bar", {NULL}, {0.0}, {0.0}, {0.0}, {NULL}},
    {discrete, "Scale status bar", {&st_scale}, {2.0}, {0.0}, {0.0}, {OnOff}},
    {redtext, " ", {NULL}, {0.0}, {0.0}, {0.0}, {NULL}},
    {yellowtext, "Floating HUD elements", {NULL}, {0.0}, {0.0}, {0.0}, {NULL}},
    {discrete, "Scale HUD elements", {&hud_scale}, {2.0}, {0.0}, {0.0}, {OnOff}},
    {slider, "HUD Transparency", {&hud_transparency}, {0.0}, {1.0}, {0.1}, {NULL}},
    {slider, "HUD Anchoring", {&hud_anchoring}, {0.0}, {1.0}, {0.1}, {NULL}},
    {discrete, "Bigger font in HUD", {&hud_bigfont}, {2.0}, {0.0}, {0.0}, {OnOff}},
    // clang-format off
    {discrete, "Show Secret Messages", {&hud_revealsecrets}, {4.0}, {0.0}, {0.0}, {SecretOptions}},
    {discrete, "Player target names", {&hud_targetnames}, {2.0}, {0.0}, {0.0}, {HideShow}},
    // clang-format on
    {discrete, "Timer Type", {&hud_timer}, {3.0}, {0.0}, {0.0}, {TimerStyles}},
    {discrete, "Speedometer", {&hud_speedometer}, {2.0}, {0.0}, {0.0}, {OnOff}},
    {slider, "Feed Timeout", {&hud_feedtime}, {1.0}, {10.0}, {0.25}, {NULL}},
    {discrete, "Show Kills in Feed", {&hud_feedobits}, {2.0}, {0.0}, {0.0}, {OnOff}},
    {discrete, "Netdemo infos", {&hud_demobar}, {2.0}, {0.0}, {0.0}, {OnOff}},
    {redtext, " ", {NULL}, {0.0}, {0.0}, {0.0}, {NULL}},

    {yellowtext, "Scoreboard", {NULL}, {0.0}, {0.0}, {0.0}, {NULL}},
    {slider, "Scale scoreboard", {&hud_scalescoreboard}, {0.0}, {1.0}, {0.125}, {NULL}},
    // clang-format off
	{discrete, "Scores on Death", {&hud_show_scoreboard_ondeath}, {2.0}, {0.0}, {0.0}, {OnOff}},
    // clang-format on
    {redtext, " ", {NULL}, {0.0}, {0.0}, {0.0}, {NULL}},

    {yellowtext, "Capture the Flag", {NULL}, {0.0}, {0.0}, {0.0}, {NULL}},
    {discrete, "Event Message Type", {&hud_gamemsgtype}, {3.0}, {0.0}, {0.0}, {VoxType}},
    {discrete, "Held Flag Border", {&hud_heldflag}, {3.0}, {0.0}, {0.0}, {FlagHelds}},
    {discrete, "Held Flag Flashes", {&hud_heldflag_flash}, {2.0}, {0.0}, {0.0}, {OnOff}},
    {redtext, " ", {NULL}, {0.0}, {0.0}, {0.0}, {NULL}},

    {yellowtext, "Crosshair", {NULL}, {0.0}, {0.0}, {0.0}, {NULL}},
    {discrete, "Crosshair type", {&hud_crosshair}, {9.0}, {0.0}, {0.0}, {Crosshairs}},
    {discrete, "Scale crosshair", {&hud_crosshairscale}, {2.0}, {0.0}, {0.0}, {OnOff}},
    {discrete, "Crosshair health", {&hud_crosshairhealth}, {2.0}, {0.0}, {0.0}, {OnOff}},
    {redslider, "Crosshair Red", {&hud_crosshaircolor}, {0.0}, {0.0}, {0.0}, {NULL}},
    {greenslider, "Crosshair Green", {&hud_crosshaircolor}, {0.0}, {0.0}, {0.0}, {NULL}},
    {blueslider, "Crosshair Blue", {&hud_crosshaircolor}, {0.0}, {0.0}, {0.0}, {NULL}},
    {redtext, " ", {NULL}, {0.0}, {0.0}, {0.0}, {NULL}},
};

menu_t HUDMenu = {
    "M_VIDEO",              // title
    1,                      // lastOn
    ARRAY_LENGTH(HUDItems), // numitems
    0,                      // indent
    HUDItems,               // items
    0,                      // scrolltop
    0,                      // scrollpos
    NULL,                   // refreshfunc
};

/*=======================================
 *
 * Messages Menu
 *
 *=======================================*/
EXTERN_CVAR(message_showpickups)
EXTERN_CVAR(message_showobituaries)
EXTERN_CVAR (con_coloredmessages)
EXTERN_CVAR (hud_scaletext)
EXTERN_CVAR (msg0color)
EXTERN_CVAR (msg1color)
EXTERN_CVAR (msg2color)
EXTERN_CVAR (msg3color)
EXTERN_CVAR (msg4color)
EXTERN_CVAR (msgmidcolor)

static value_t TextColors[] =
{
	{ CR_BRICK,		"brick" },
	{ CR_TAN,		"tan" },
	{ CR_GRAY,		"gray" },
	{ CR_GREEN,		"green" },
	{ CR_BROWN,		"brown" },
	{ CR_GOLD, 		"gold" },
	{ CR_RED,		"red" },
	{ CR_BLUE,		"blue" },
	{ CR_ORANGE,	"orange" },
	{ CR_WHITE,		"white" },
	{ CR_YELLOW,	"yellow" },
	{ CR_BLACK,		"black" },
	{ CR_LIGHTBLUE,	"light blue" },
	{ CR_CREAM,		"cream" },
	{ CR_OLIVE,		"olive" },
	{ CR_DARKGREEN,	"dark green" },
	{ CR_DARKRED,	"dark red" },
	{ CR_DARKBROWN,	"dark brown" },
	{ CR_PURPLE,	"purple" },
	{ CR_DARKGRAY,	"dark gray" },
	{ CR_CYAN,		"cyan" }
};

// TODO: Put all language info in one array, auto detect what's in the lump?
static value_t Languages[] = {
	{ 0.0, "Auto" },
	{ 1.0, "English" },
	{ 2.0, "French" },
	{ 3.0, "Italian" }
};

static menuitem_t MessagesItems[] = {
#if 0
	{ discrete, "Language", 			 {&language},		   	{4.0}, {0.0},   {0.0}, {Languages} },
#endif
	{ slider,	"Message Timeout",		 {&con_notifytime},		{1.0}, {10.0},	{0.25}, {NULL} },
	{ slider,	"Center Message Timeout",{&con_midtime},		{1.0}, {10.0},	{0.25}, {NULL} },
	{ slider,	"Scale message text",    {&hud_scaletext},		{1.0}, {4.0}, 	{1.0}, {NULL} },
	{ discrete,	"Colorize messages",	{&con_coloredmessages},	{2.0}, {0.0},   {0.0},	{OnOff} },
	{ redtext,	" ",					{NULL},					{0.0}, {0.0},	{0.0}, {NULL} },
	{ yellowtext,"Display settings",		{NULL},					{0.0}, {0.0},	{0.0}, {NULL} },
	{ discrete,	"Show pickup messages",	{&message_showpickups},	{2.0}, {0.0},   {0.0},	{OnOff} },
	{ discrete,	"Show death messages",	{&message_showobituaries},	{2.0}, {0.0},   {0.0},	{OnOff} },
	{ discrete,	"Hide spectator messages",	{&mute_spectators},	{2.0}, {0.0},   {0.0},	{OnOff} },
	{ discrete,	"Hide enemies messages",	{&mute_enemies},	{2.0}, {0.0},   {0.0},	{OnOff} },

	{ redtext,	" ",					{NULL},					{0.0}, {0.0},	{0.0}, {NULL} },
	{ yellowtext, "Message Colors",		{NULL},					{0.0}, {0.0},	{0.0}, {NULL} },
	{ cdiscrete, "Item Pickup",			{&msg0color},		   	{21.0}, {0.0},	{0.0}, {TextColors} },
	{ cdiscrete, "Obituaries",			{&msg1color},		   	{21.0}, {0.0},	{0.0}, {TextColors} },
	{ cdiscrete, "Critical Messages",	{&msg2color},		   	{21.0}, {0.0},	{0.0}, {TextColors} },
	{ cdiscrete, "Chat Messages",		{&msg3color},		   	{21.0}, {0.0},	{0.0}, {TextColors} },
	{ cdiscrete, "Team Messages",		{&msg4color},		   	{21.0}, {0.0},	{0.0}, {TextColors} },
	{ cdiscrete, "Centered Messages",	{&msgmidcolor},			{21.0}, {0.0},	{0.0}, {TextColors} }
};

menu_t MessagesMenu = {
	"M_MESS",
	0,
	ARRAY_LENGTH(MessagesItems),
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
    { discrete, "Show item count",		{&am_showitems},		{2.0}, {0.0},	{0.0},  {OnOff} },
    { discrete, "Show monster count",	{&am_showmonsters},		{2.0}, {0.0},	{0.0},	{OnOff} },
    { discrete, "Show secrets count",	{&am_showsecrets},	   	{2.0}, {0.0},	{0.0},  {OnOff} },
    { discrete, "Show map timer", 	    {&am_showtime}, 	   	{2.0}, {0.0},	{0.0},  {OnOff} },
    { discrete, "Map name style",       {&am_classicmapstring},	{2.0}, {0.0},	{0.0},  {ClassicMapStringTypes} },

	{ redtext,	" ",					{NULL},					{0.0}, {0.0},	{0.0}, {NULL} },
	{ yellowtext, "Automap Colors",		{NULL},					{0.0}, {0.0},	{0.0}, {NULL} },
	{ discrete, "Custom map colors",	{&am_usecustomcolors},	{2.0}, {0.0},	{0.0},  {OnOff} },
	{ more,     "Reset custom map colors",  {NULL},             {0.0}, {0.0},   {0.0},  {(value_t *)ResetCustomColors} },
};

menu_t AutomapMenu = {
	"M_AUTOMP",
	0,
	ARRAY_LENGTH(AutomapItems),
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

int testingmode;		// Holds time to revert to old mode

static bool GetSelectedSize(int line, int *width, int *height);

EXTERN_CVAR (vid_widescreen)
EXTERN_CVAR (vid_maxfps)

EXTERN_CVAR (vid_overscan)
EXTERN_CVAR (vid_fullscreen)
EXTERN_CVAR (vid_32bpp)
EXTERN_CVAR(vid_vsync)

static uint16_t old_width, old_height;

static void SetModesMenu(int w, int h);

static void M_SetVideoMode(uint16_t width, uint16_t height)
{
	old_width = I_GetVideoWidth();
	old_height = I_GetVideoHeight();

	char command[30];
	sprintf(command, "vid_setmode %d %d", width, height);
	AddCommandString(command);

	SetModesMenu(width, height);
}


void M_RestoreVideoMode()
{
	testingmode = 0;
	M_SetVideoMode(old_width, old_height);
}


static value_t Depths[22];

#ifdef _XBOX
static const char VMEnterText[] = "Press A to set mode";
static const char VMTestText[] = "Press X to test mode for 5 seconds";
#else
static const char VMEnterText[] = "Press ENTER to set mode";
static const char VMTestText[] = "Press T to test mode for 5 seconds";
#endif

static const char VMTestWaitText[] = "Please wait 5 seconds...";

static value_t VidFPSCaps[] = {
	{ 35.0,		"35fps" },
	{ 60.0,		"60fps" },
	{ 70.0,		"70fps" },
   	{ 105.0,	"105fps"},
	{ 120.0,	"120fps" }, 
	{ 140.0,	"140fps"},
    	{ 144.0,	"144fps"},
    	{ 240.0,	"240fps"},
	{ 0.0,		"Unlimited" }
};

static value_t FullScreenOptions[] = {
	{ WINDOW_Windowed,			"Window" },
	{ WINDOW_Fullscreen,		"Full Screen Exclusive" },
	{ WINDOW_DesktopFullscreen,	"Full Screen Window" }
};

static value_t WidescreenMode[] = {
	{ 0.0,			"Off" },
	{ 1.0,			"Auto" },
	{ 2.0,			"16:10" },
	{ 3.0,			"16:9" },
	{ 4.0,			"21:9" },
	{ 5.0,			"32:9" }
};

static menuitem_t ModesItems[] = {
#ifdef GCONSOLE
	{ slider, "Overscan",				{&vid_overscan},		{0.84375}, {1.0}, {0.03125}, {NULL} },
#else
	{ discrete, "Fullscreen",			{&vid_fullscreen},		{3.0}, {0.0},	{0.0}, {FullScreenOptions} },
#endif
	{ discrete,	"Widescreen",			{&vid_widescreen},		{6.0}, {0.0},	{0.0}, {WidescreenMode} } ,
	{ discrete,	"VSync",				{&vid_vsync},			{2.0}, {0.0},	{0.0}, {YesNo} },
	{ discrete, "Framerate",			{&vid_maxfps},			{9.0}, {0.0},	{0.0}, {VidFPSCaps} },
	{ discrete, "32-bit color",			{&vid_32bpp},			{2.0}, {0.0},	{0.0}, {YesNo} },
	{ redtext,	"",						{NULL},					{0.0}, {0.0},	{0.0}, {NULL} },
	{ screenres, NULL,					{NULL},					{0.0}, {0.0},	{0.0}, {NULL} },
	{ screenres, NULL,					{NULL},					{0.0}, {0.0},	{0.0}, {NULL} },
	{ screenres, NULL,					{NULL},					{0.0}, {0.0},	{0.0}, {NULL} },
	{ screenres, NULL,					{NULL},					{0.0}, {0.0},	{0.0}, {NULL} },
	{ screenres, NULL,					{NULL},					{0.0}, {0.0},	{0.0}, {NULL} },
	{ screenres, NULL,					{NULL},					{0.0}, {0.0},	{0.0}, {NULL} },
	{ screenres, NULL,					{NULL},					{0.0}, {0.0},	{0.0}, {NULL} },
	{ screenres, NULL,					{NULL},					{0.0}, {0.0},	{0.0}, {NULL} },
	{ redtext,	" ",					{NULL},					{0.0}, {0.0},	{0.0}, {NULL} },
	{ whitetext, " ",					{NULL},					{0.0}, {0.0},	{0.0}, {NULL} },
	{ redtext,	" ",					{NULL},					{0.0}, {0.0},	{0.0}, {NULL} },
	{ yellowtext, " ",					{NULL},					{0.0}, {0.0},	{0.0}, {NULL} },
};

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
	NULL
};

static void BuildModesList(int hiwidth, int hiheight)
{
	// gathers a list of unique resolutions availible for the current
	// screen mode (windowed or fullscreen)
	bool fullscreen = I_GetWindow()->getVideoMode().isFullScreen();

	typedef std::vector< std::pair<uint16_t, uint16_t> > MenuModeList;
	MenuModeList menumodelist;

	const IVideoModeList* videomodelist = I_GetVideoCapabilities()->getSupportedVideoModes();
	for (IVideoModeList::const_iterator it = videomodelist->begin(); it != videomodelist->end(); ++it)
		if (it->isFullScreen() == fullscreen)
			menumodelist.push_back(std::make_pair(it->width, it->height));
	menumodelist.erase(std::unique(menumodelist.begin(), menumodelist.end()), menumodelist.end());

	MenuModeList::const_iterator mode_it = menumodelist.begin();

	char** str = NULL;

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
				int width = mode_it->first;
				int height = mode_it->second;
				++mode_it;

				if (width == hiwidth && height == hiheight)
					ModesItems[i].e.highlight = ModesItems[i].a.selmode = col;

				char strtemp[32];
				sprintf(strtemp, "%dx%d", width, height);
				ReplaceString(str, strtemp);
			}
			else
			{
				str = NULL;
			}
		}
	}
}

void M_RefreshModesList()
{
	BuildModesList(I_GetVideoWidth(), I_GetVideoHeight());
}

static bool GetSelectedSize(int line, int* width, int* height)
{
	if (ModesItems[line].type != screenres)
		return false;

	int mode_num = (line - VM_RESSTART) * 3 + ModesItems[line].a.selmode;

	const char* resolution_str = NULL;

	if (mode_num % 3 == 0)
		resolution_str = ModesItems[line].b.res1;
	else if (mode_num % 3 == 1)
		resolution_str = ModesItems[line].c.res2;
	else if (mode_num % 3 == 2)
		resolution_str = ModesItems[line].d.res3;

	if (!resolution_str)
		return false;

	size_t xpos = 0;
	for (const char* s = resolution_str; s; s++, xpos++)
		if (*s == 'x' || *s == 'X')
			break;

	char width_str[5] = { 0 }, height_str[5] = { 0 };
	strncpy(width_str, resolution_str, xpos);
	strncpy(height_str, resolution_str + xpos + 1, 4);

	*width = atoi(width_str);
	*height = atoi(height_str);

	return true;
}

static void SetModesMenu(int w, int h)
{
	if (!testingmode)
	{
		ModesItems[VM_ENTERLINE].label = VMEnterText;
		ModesItems[VM_TESTLINE].label = VMTestText;
	}
	else
	{
		static char enter_text[64];
		sprintf(enter_text, "TESTING %dx%d", w, h);

		ModesItems[VM_ENTERLINE].label = enter_text;
		ModesItems[VM_TESTLINE].label = VMTestWaitText;
	}

	BuildModesList(w, h);
}

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

static void SetVidMode()
{
	SetModesMenu(I_GetVideoWidth(), I_GetVideoHeight());

	if (ModesMenu.items[ModesMenu.lastOn].type == screenres)
	{
		if (ModesMenu.items[ModesMenu.lastOn].a.selmode == -1)
			ModesMenu.items[ModesMenu.lastOn].a.selmode++;
	}
	M_SwitchMenu(&ModesMenu);
}



static cvar_t *flagsvar;

EXTERN_CVAR(ui_dimcolor)

// [Russell] - Modified to send new colours
static void M_SendUINewColor (int red, int green, int blue)
{
	char command[24];

	sprintf (command, "ui_dimcolor \"%02x %02x %02x\"", red, green, blue);
	AddCommandString (command);
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


//
//		Set some stuff up for the video modes menu
//

void M_OptInit (void)
{
	for (int i = 0; i < 22; i++)
	{
		Depths[i].value = i;
		Depths[i].name = NULL;
	}

	switch (I_GetVideoCapabilities()->getDisplayType())
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
			Bindings.GetKeysForCommand (item->e.command, &item->b.key1, &item->c.key2);
		if (item->type == mapcontrol)
			AutomapBindings.GetKeysForCommand(item->e.command, &item->b.key1, &item->c.key2);
		if (item->type == netdemocontrol)
			NetDemoBindings.GetKeysForCommand(item->e.command, &item->b.key1, &item->c.key2);
	}
}

void M_SwitchMenu(menu_t* menu)
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
			if (item->type != whitetext && item->type != redtext && item->type != orangetext)
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

void M_DrawSlider (int x, int y, float leftval, float rightval, float cur, float step)
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

	std::string buf;
	if (step == 0.0f)
		return;
	else if (step >= 1.0f)
		StrFormat(buf, "%.0f", cur);
	else if (step >= 0.1f)
		StrFormat(buf, "%.1f", cur);
	else
		StrFormat(buf, "%.2f", cur);
	screen->DrawTextCleanMove(CR_GREEN, x + 96, y, buf.c_str());
}

void M_DrawColoredSlider(int x, int y, float leftval, float rightval, float cur, argb_t color)
{
	if (leftval < rightval)
		cur = clamp(cur, leftval, rightval);
	else
		cur = clamp(cur, rightval, leftval);

	float dist = (cur - leftval) / (rightval - leftval);

	screen->DrawPatchClean(W_CachePatch ("LSLIDE"), x, y);

	for (int i = 1; i < 11; i++)
		screen->DrawPatchClean (W_CachePatch ("MSLIDE"), x + i*8, y);

	screen->DrawPatchClean (W_CachePatch ("RSLIDE"), x + 88, y);

	screen->DrawPatchClean (W_CachePatch ("GSLIDE"), x + 5 + (int)(dist * 78.0), y);

	V_ColorFill = V_BestColor(V_GetDefaultPalette()->basecolors, color);

	screen->DrawColoredPatchClean(W_CachePatch("OSLIDE"), x + 5 + (int)(dist * 78.0), y);
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

	x1 = (I_GetSurfaceWidth() / 2)-(160*CleanXfac);
	y1 = (I_GetSurfaceHeight() / 2)-(100*CleanYfac);

    x2 = (I_GetSurfaceWidth() / 2)+(160*CleanXfac);
	y2 = (I_GetSurfaceHeight() / 2)+(100*CleanYfac);

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

			if (i == CurrentItem && ((item->a.selmode != -1 && (skullAnimCounter < 6 || WaitingForKey))
				|| WaitingForAxis || testingmode))
				screen->DrawPatchClean (W_CachePatch ("LITLCURS"), item->a.selmode * 104 + 8, y);
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

			case yellowtext:
				x = 160 - width / 2;
				color = CR_YELLOW;
				break;

			case orangetext:
				x = 160 - width / 2;
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
				int v, vals;

				vals = (int)item->b.leftval;
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
										   (item->e.values[(int)item->b.leftval]).name);
				break;

			case slider:
				M_DrawSlider (CurrentMenu->indent + 8, y, item->b.leftval, item->c.rightval, item->a.cvar->value(), item->d.step);
				break;

			case redslider:
			{
				argb_t color = V_GetColorFromString(*item->a.cvar);
				M_DrawColoredSlider(CurrentMenu->indent + 8, y, 0, 255, color.getr(), color);
			}
			break;
			case greenslider:
			{
				argb_t color = V_GetColorFromString(*item->a.cvar);
				M_DrawColoredSlider(CurrentMenu->indent + 8, y, 0, 255, color.getg(), color);
			}
			break;
			case blueslider:
			{
				argb_t color = V_GetColorFromString(*item->a.cvar);
				M_DrawColoredSlider(CurrentMenu->indent + 8, y, 0, 255, color.getb(), color);
			}
			break;

			case control:
			{
				std::string desc = Bindings.GetNameKeys(item->b.key1, item->c.key2);
				screen->DrawTextCleanMove (CR_GREY, CurrentMenu->indent + 14, y, desc.c_str());
			}
			break;

			case mapcontrol:
			{
				std::string desc = AutomapBindings.GetNameKeys(item->b.key1, item->c.key2);
				screen->DrawTextCleanMove(CR_GREY, CurrentMenu->indent + 14, y, desc.c_str());
			}
			break;

			case netdemocontrol:
			{
				std::string desc = NetDemoBindings.GetNameKeys(item->b.key1, item->c.key2);
				screen->DrawTextCleanMove(CR_GREY, CurrentMenu->indent + 14, y, desc.c_str());
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
	int mod = ev->mod;
	const char *cmd = Bindings.GetBind(ch).c_str();

	item = CurrentMenu->items + CurrentItem;

	bool numlock = mod & OMOD_NUM;

	// Waiting on a key press for control binding
	if (WaitingForKey)
	{
		if (ev->type == ev_keydown)
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
			if (Key_IsCancelKey(ch))
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
	    (Key_IsLeftKey(ch, numlock) || Key_IsRightKey(ch, numlock) || Key_IsAcceptKey(ch))
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

			S_Sound(CHAN_INTERFACE, "plats/pt1_stop", 1, ATTN_NONE);
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

			S_Sound(CHAN_INTERFACE, "plats/pt1_stop", 1, ATTN_NONE);
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
				S_Sound(CHAN_INTERFACE, "plats/pt1_stop", 1, ATTN_NONE);
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
				S_Sound(CHAN_INTERFACE, "plats/pt1_stop", 1, ATTN_NONE);
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
				newval = MAX(newval, item->b.leftval);
			else
				newval = MIN(newval, item->b.leftval);

			if (item->e.cfunc)
				item->e.cfunc(item->a.cvar, newval);
			else
				item->a.cvar->Set(newval);
		}
		S_Sound(CHAN_INTERFACE, "plats/pt1_mid", 1, ATTN_NONE);
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

			argb_t color = V_GetColorFromString(oldcolor);
			int part = 0;

			if (item->type == redslider)
				part = color.getr();
			else if (item->type == greenslider)
				part = color.getg();
			else if (item->type == blueslider)
				part = color.getb();

			if (part > 0x00)
				part -= 0x11;
			if (part < 0x00)
				part = 0x00;

			char singlecolor[3];
			sprintf(singlecolor, "%02x", part);

			if (item->type == redslider)
				memcpy(newcolor, singlecolor, 2);
			else if (item->type == greenslider)
				memcpy(newcolor + 3, singlecolor, 2);
			else if (item->type == blueslider)
				memcpy(newcolor + 6, singlecolor, 2);

			item->a.cvar->Set(newcolor);
		}
		S_Sound(CHAN_INTERFACE, "plats/pt1_mid", 1, ATTN_NONE);
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

			numvals = (int)item->b.leftval;
			cur = M_FindCurVal(item->a.cvar->value(), item->e.values, numvals);
			if (--cur < 0)
				cur = numvals - 1;

			item->a.cvar->Set(item->e.values[cur].value);

			// Hack hack. Rebuild list of resolutions
			if (item->e.values == Depths)
				BuildModesList(I_GetVideoWidth(), I_GetVideoHeight());
		}
		S_Sound(CHAN_INTERFACE, "plats/pt1_mid", 1, ATTN_NONE);
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
		S_Sound(CHAN_INTERFACE, "plats/pt1_stop", 1, ATTN_NONE);
		break;

		case joyactive:
		{
			int         numjoy;

			numjoy = I_GetJoystickCount();

			if ((int)item->a.cvar->value() > numjoy)
				item->a.cvar->Set(0.0);
			else if ((int)item->a.cvar->value() > 0)
				item->a.cvar->Set(item->a.cvar->value() - 1);
		}
		S_Sound(CHAN_INTERFACE, "plats/pt1_mid", 1, ATTN_NONE);
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
				newval = MIN(newval, item->c.rightval);
			else
				newval = MAX(newval, item->c.rightval);

			if (item->e.cfunc)
				item->e.cfunc(item->a.cvar, newval);
			else
				item->a.cvar->Set(newval);
		}
		S_Sound(CHAN_INTERFACE, "plats/pt1_mid", 1, ATTN_NONE);
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

			argb_t color = V_GetColorFromString(oldcolor);
			int part = 0;

			if (item->type == redslider)
				part = color.getr();
			else if (item->type == greenslider)
				part = color.getg();
			else if (item->type == blueslider)
				part = color.getb();

			if (part < 0xff)
				part += 0x11;
			if (part > 0xff)
				part = 0xff;

			char singlecolor[3];
			sprintf(singlecolor, "%02x", part);

			if (item->type == redslider)
				memcpy(newcolor, singlecolor, 2);
			else if (item->type == greenslider)
				memcpy(newcolor + 3, singlecolor, 2);
			else if (item->type == blueslider)
				memcpy(newcolor + 6, singlecolor, 2);

			item->a.cvar->Set(newcolor);
		}
		S_Sound(CHAN_INTERFACE, "plats/pt1_mid", 1, ATTN_NONE);
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

			numvals = (int)item->b.leftval;
			cur = M_FindCurVal(item->a.cvar->value(), item->e.values, numvals);
			if (++cur >= numvals)
				cur = 0;

			item->a.cvar->Set(item->e.values[cur].value);

			// Hack hack. Rebuild list of resolutions
			if (item->e.values == Depths)
				BuildModesList(I_GetVideoWidth(), I_GetVideoHeight());
		}
		S_Sound(CHAN_INTERFACE, "plats/pt1_mid", 1, ATTN_NONE);
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
		S_Sound(CHAN_INTERFACE, "plats/pt1_stop", 1, ATTN_NONE);
		break;

		case joyactive:
		{
			int         numjoy;

			numjoy = I_GetJoystickCount();

			if ((int)item->a.cvar->value() >= numjoy)
				item->a.cvar->Set(0.0);
			else if ((int)item->a.cvar->value() < (numjoy - 1))
				item->a.cvar->Set(item->a.cvar->value() + 1);

		}
		S_Sound(CHAN_INTERFACE, "plats/pt1_mid", 1, ATTN_NONE);
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
				int width, height;

				if (!(item->type == screenres &&
				      GetSelectedSize(CurrentItem, &width, &height)))
				{
					width = I_GetVideoWidth();
					height = I_GetVideoHeight();
				}

				M_SetVideoMode(width, height);
				S_Sound(CHAN_INTERFACE, "weapons/pistol", 1, ATTN_NONE);
			}
			else if (item->type == more && item->e.mfunc)
			{
				CurrentMenu->lastOn = CurrentItem;
				S_Sound(CHAN_INTERFACE, "weapons/pistol", 1, ATTN_NONE);
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

				numvals = (int)item->b.leftval;
				cur = M_FindCurVal(item->a.cvar->value(), item->e.values, numvals);
				if (++cur >= numvals)
					cur = 0;

				item->a.cvar->Set(item->e.values[cur].value);

				// Hack hack. Rebuild list of resolutions
				if (item->e.values == Depths)
					BuildModesList(I_GetVideoWidth(), I_GetVideoHeight());
				S_Sound(CHAN_INTERFACE, "plats/pt1_mid", 1, ATTN_NONE);
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
				S_Sound(CHAN_INTERFACE, "weapons/pistol", 1, ATTN_NONE);
				item->e.lfunc(CurrentItem);
			}
			else if (item->type == joyaxis)
			{
				WaitingForAxis = true;
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
#ifdef _XBOX
		if (ev->data3 == 't' || ev->data1 == OKEY_JOY3)
#else
		if (ev->data3 == 't')
#endif
		{
			// Test selected resolution
			if (CurrentMenu == &ModesMenu)
			{
				int width, height;

				if (!(item->type == screenres && GetSelectedSize(CurrentItem, &width, &height)))
				{
					width = I_GetVideoWidth();
					height = I_GetVideoHeight();
				}

				testingmode = I_MSTime() * TICRATE / 1000 + 5 * TICRATE;
				M_SetVideoMode(width, height);

				S_Sound(CHAN_INTERFACE, "weapons/pistol", 1, ATTN_NONE);
			}
		}
		}
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
	cvar_t::C_SetCVarsToDefaults(CVAR_CLIENTARCHIVE);
	UpdateStuff();
}

void Reset2Saved (void)
{
	std::string cmd = "exec " + C_QuoteString(M_GetConfigPath());
	AddCommandString(cmd);
	UpdateStuff();
}

static void StartHUDMenu()
{
	M_SwitchMenu(&HUDMenu);
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


BEGIN_COMMAND (menu_video)
{
	M_StartControlPanel ();
	S_Sound (CHAN_INTERFACE, "switches/normbutn", 1, ATTN_NONE);
	OptionsActive = true;
	SetVidMode ();
}
END_COMMAND (menu_video)

VERSION_CONTROL (m_options_cpp, "$Id$")
