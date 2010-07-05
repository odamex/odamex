// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1998-2006 by Randy Heit (ZDoom 1.22).
// Copyright (C) 2006-2009 by The Odamex Team.
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
#include "dstrings.h"

#include "c_console.h"
#include "c_dispatch.h"
#include "c_bind.h"

#include "d_main.h"

#include "i_system.h"
#include "i_video.h"
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

#include "doomstat.h"

#include "m_misc.h"

// Data.
#include "m_menu.h"

//
// defaulted values
//
//CVAR (mouse_sensitivity, "1.0", CVAR_ARCHIVE)
//CVAR (cont_preset,			"0",	CVAR_ARCHIVE)
EXTERN_CVAR (dynres_state)
EXTERN_CVAR (dynresval)
//CVAR (mouse_preset,			"0",	CVAR_ARCHIVE)
EXTERN_CVAR (mouse_sensitivity)
EXTERN_CVAR (mouse_type)
EXTERN_CVAR (novert)

// [ML] 09/4/06: Show secret revealed message, 0 = off, 1 = on
EXTERN_CVAR (revealsecrets)

// Show messages has default, 0 = off, 1 = on
EXTERN_CVAR (show_messages)

extern bool				OptionsActive;

extern int				screenSize;
extern short			skullAnimCounter;

EXTERN_CVAR (cl_run)
EXTERN_CVAR (invertmouse)
EXTERN_CVAR (lookspring)
EXTERN_CVAR (lookstrafe)
EXTERN_CVAR (crosshair)
EXTERN_CVAR (cl_mouselook)
EXTERN_CVAR (cl_autoaim)

// [Ralphis - Menu] Compatibility Menu
EXTERN_CVAR (co_level8soundfeature)
EXTERN_CVAR (hud_targetcount)
EXTERN_CVAR (revealsecrets)
EXTERN_CVAR (show_endoom)

// [Toke - Menu] New Menu Stuff.
void MouseSetup (void);
EXTERN_CVAR (m_pitch)
EXTERN_CVAR (m_side)
EXTERN_CVAR (m_forward)
EXTERN_CVAR (displaymouse)
EXTERN_CVAR (mouse_acceleration)
EXTERN_CVAR (mouse_threshold)

// [Ralphis - Menu] Sound Menu
EXTERN_CVAR (snd_musicvolume)
EXTERN_CVAR (snd_sfxvolume)
EXTERN_CVAR (cl_connectalert)

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

menu_t  *CurrentMenu;
int		CurrentItem;
static BOOL	WaitingForKey;
static const char	   *OldMessage;
static itemtype OldType;

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
static void GoToConsole (void);
void Reset2Defaults (void);
void Reset2Saved (void);

static void SetVidMode (void);

static menuitem_t OptionItems[] =
{
    { more, 	"Player Setup",     	{NULL},					{0.0}, {0.0},	{0.0}, {(value_t *)PlayerSetup} },
 	{ more,		"Customize Controls",	{NULL},					{0.0}, {0.0},	{0.0}, {(value_t *)CustomizeControls} },
	{ more,		"Mouse Options" ,	    {NULL},					{0.0}, {0.0},	{0.0}, {(value_t *)MouseSetup} },
//	{ more,		"Joystick Setup" ,	    {NULL},					{0.0}, {0.0},	{0.0}, {(value_t *)JoystickSetup} },
 	{ redtext,	" ",					{NULL},					{0.0}, {0.0},	{0.0}, {NULL} },
 	{ more,		"Compatibility Options",{NULL},					{0.0}, {0.0},	{0.0}, {(value_t *)CompatOptions} },
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
};

/*=======================================
 *
 * Controls Menu
 *
 *=======================================*/

static menuitem_t ControlsItems[] = {
	{ whitetext,"ENTER to change, BACKSPACE to clear", {NULL}, {0.0}, {0.0}, {0.0}, {NULL} },
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
	{ redtext,	" ",					{NULL},	{0.0}, {0.0}, {0.0}, {NULL} },
	{ bricktext,"Actions",		        {NULL},	{0.0}, {0.0}, {0.0}, {NULL} },
	{ control,	"Fire",					{NULL}, {0.0}, {0.0}, {0.0}, {(value_t *)"+attack"} },
	{ control,	"Use / Open",			{NULL}, {0.0}, {0.0}, {0.0}, {(value_t *)"+use"} },
	{ control,	"Next weapon",			{NULL}, {0.0}, {0.0}, {0.0}, {(value_t *)"weapnext"} },
	{ control,	"Previous weapon",		{NULL}, {0.0}, {0.0}, {0.0}, {(value_t *)"weapprev"} },
	{ control,	"Toggle Automap",		{NULL}, {0.0}, {0.0}, {0.0}, {(value_t *)"togglemap"} },
	{ redtext,	" ",					{NULL},	{0.0}, {0.0}, {0.0}, {NULL} },
	{ bricktext,"Advanced Movement",    {NULL},	{0.0}, {0.0}, {0.0}, {NULL} },
	{ control,	"Fly / Swim up",		{NULL},	{0.0}, {0.0}, {0.0}, {(value_t *)"+moveup"} },
	{ control,	"Fly / Swim down",		{NULL},	{0.0}, {0.0}, {0.0}, {(value_t *)"+movedown"} },
	{ control,	"Stop flying",			{NULL},	{0.0}, {0.0}, {0.0}, {(value_t *)"land"} },
	{ control,	"Look up",				{NULL}, {0.0}, {0.0}, {0.0}, {(value_t *)"+lookup"} },
	{ control,	"Look down",			{NULL}, {0.0}, {0.0}, {0.0}, {(value_t *)"+lookdown"} },
	{ control,	"Center view",			{NULL}, {0.0}, {0.0}, {0.0}, {(value_t *)"centerview"} },
	{ control,	"Mouse look",			{NULL}, {0.0}, {0.0}, {0.0}, {(value_t *)"+mlook"} },
	{ control,	"Keyboard look",		{NULL}, {0.0}, {0.0}, {0.0}, {(value_t *)"+klook"} },
	{ redtext,	" ",					{NULL},	{0.0}, {0.0}, {0.0}, {NULL} },
	{ bricktext,"Multiplayer",		    {NULL},	{0.0}, {0.0}, {0.0}, {NULL} },
	{ control,	"Say",					{NULL}, {0.0}, {0.0}, {0.0}, {(value_t *)"messagemode"} },
	{ control,	"Team say",				{NULL}, {0.0}, {0.0}, {0.0}, {(value_t *)"messagemode2"} },
	{ control,	"Spectate",				{NULL}, {0.0}, {0.0}, {0.0}, {(value_t *)"spectate"} },
	{ control,	"Coop Spy",				{NULL}, {0.0}, {0.0}, {0.0}, {(value_t *)"spynext"} },
	{ control,	"Show Scoreboard",			{NULL}, {0.0}, {0.0}, {0.0}, {(value_t *)"+showscores"} },
	{ redtext,	" ",					{NULL},	{0.0}, {0.0}, {0.0}, {NULL} },
	{ bricktext,"Inventory",			{NULL},	{0.0}, {0.0}, {0.0}, {NULL} },
	{ control,	"Activate item",		{NULL}, {0.0}, {0.0}, {0.0}, {(value_t *)"invuse"} },
	{ control,	"Activate all items",	{NULL}, {0.0}, {0.0}, {0.0}, {(value_t *)"invuseall"} },
	{ control,	"Next item",			{NULL}, {0.0}, {0.0}, {0.0}, {(value_t *)"invnext"} },
	{ control,	"Previous item",		{NULL}, {0.0}, {0.0}, {0.0}, {(value_t *)"invprev"} },
	{ redtext,	" ",					{NULL},	{0.0}, {0.0}, {0.0}, {NULL} },
	{ bricktext,"Other",				{NULL},	{0.0}, {0.0}, {0.0}, {NULL} },
	{ control,	"Chasecam",				{NULL}, {0.0}, {0.0}, {0.0}, {(value_t *)"chase"} },
	{ control,	"Screenshot",			{NULL}, {0.0}, {0.0}, {0.0}, {(value_t *)"screenshot"} },
	{ control,  "Open console",			{NULL}, {0.0}, {0.0}, {0.0}, {(value_t *)"toggleconsole"} },
	{ control,  "Quit",			        {NULL}, {0.0}, {0.0}, {0.0}, {(value_t *)"quit"} }
};

menu_t ControlsMenu = {
	"M_CONTRO",
	3,
	STACKARRAY_LENGTH(ControlsItems),
	0,
	ControlsItems,
	2,
};

// -------------------------------------------------------
//
//	[Toke] New [ Mouse Menu ]
//
// -------------------------------------------------------

static value_t MouseBases[] =
{
	{ 0.0, "Doom" },
	{ 1.0, "Odamex" },
};

static menuitem_t MouseItems[] =
{
	{ discrete	,	"Mouse Type"							, {&mouse_type},		{2.0},		{0.0},		{0.0},		{MouseBases}				},
	{ redtext	,	" "										, {NULL},				{0.0},		{0.0},		{0.0},		{NULL}						},
	{ discrete	,	"Always FreeLook"						, {&cl_mouselook},		{2.0},		{0.0},		{0.0},		{OnOff}						},
	{ discrete	,	"Invert Mouse"							, {&invertmouse},		{2.0},		{0.0},		{0.0},		{OnOff}						},
	{ slider	,	"Horizontal Sensitivity" 				, {&mouse_sensitivity},	{0.0},		{77.0},		{1.0},		{NULL}						},
	{ slider	,	"Vertical Sensitivity"					, {&m_pitch},			{0.0},		{1.85},		{0.025},	{NULL}						},
	{ redtext	,	" "										, {NULL},				{0.0},		{0.0},		{0.0},		{NULL}						},
	/*{ discrete,	"Use Dynamic Resolution"				, {&dynres_state},		{2.0},		{0.0},		{0.0},		{OnOff}						},*/
	/*{ slider	,	"Dynamic Resolution"					, {&dynresval},			{1.001},	{1.232},	{0.003},	{NULL}						},*/
	{ discrete	,	"Horizontal Movement"					, {&lookstrafe},		{2.0},		{0.0},		{0.0},		{OnOff}						},
	{ discrete	,	"Vertical Movement"						, {&novert},			{2.0},		{0.0},		{0.0},		{OffOn}						},
	{ slider	,	"Horizontal Movement Speed"				, {&m_side},			{0.0},		{15},		{0.5},		{NULL}						},
	{ slider	,	"Vertical Movement Speed"				, {&m_forward},			{0.0},		{15},		{0.5},		{NULL}						},
	{ redtext	,	" "										, {NULL},				{0.0},		{0.0},		{0.0},		{NULL}						},
	{ slider    ,   "Mouse Acceleration"					, {&mouse_acceleration},{0.0},      {10.0},     {0.5},      {NULL}                      },
	{ slider    ,   "Mouse Threshold"						, {&mouse_threshold},   {0.0},      {20.0},     {1.0},      {NULL}                      },
	{ redtext	,	" "										, {NULL},				{0.0},		{0.0},		{0.0},		{NULL}						},
	{ discrete	,	"Show Mouse Values"						, {&displaymouse},		{2.0},		{0.0},		{0.0},		{OnOff}						},
};

menu_t MouseMenu = {
    "M_MOUSET",
    0,
    STACKARRAY_LENGTH(MouseItems),
    177,
    MouseItems,
};

 /*=======================================
  *
  * Sound Menu [Ralphis]
  *
  *=======================================*/

static menuitem_t SoundItems[] = {
	{ whitetext ,   "Sound Levels"                          , {NULL},	            {0.0},      {0.0},      {0.0},      {NULL} },
	{ slider    ,	"Music Volume"                          , {&snd_musicvolume},	{0.0},      {1.0},	    {0.1},      {NULL} },
	{ slider    ,	"Sound Volume"                          , {&snd_sfxvolume},		{0.0},      {1.0},	    {0.1},      {NULL} },
	{ redtext   ,	" "                                     , {NULL},	            {0.0},      {0.0},      {0.0},      {NULL} },
	{ whitetext ,   "Other Options"                         , {NULL},	            {0.0},      {0.0},      {0.0},      {NULL} },
	{ discrete  ,   "Player Connect Alert"                  , {&cl_connectalert},	{2.0},		{0.0},		{0.0},		{OnOff} },
 };

menu_t SoundMenu = {
	"M_SOUND",
	1,
	STACKARRAY_LENGTH(SoundItems),
	177,
	SoundItems,
};

 /*=======================================
  *
  * Compatibility Menu [Ralphis]
  *
  *=======================================*/

static menuitem_t CompatItems[] = {
	{ discrete  ,   "Full Volume on MAP08"                  , {&co_level8soundfeature},	{2.0},	{0.0},		{0.0},		{OnOff} },
	{ discrete  ,	"Reveal Secrets Alert"                  , {&revealsecrets},	    {2.0},      {0.0},	    {0.0},      {OnOff} },
	{ discrete  ,	"Show DOS Ending Screen"                , {&show_endoom},		{2.0},      {0.0},	    {0.0},      {OnOff} },
 };

menu_t CompatMenu = {
	"M_COMPAT",
	0,
	STACKARRAY_LENGTH(CompatItems),
	177,
	CompatItems,
};

/*=======================================
 *
 * Display Options Menu
 *
 *=======================================*/
static void StartMessagesMenu (void);
void ResetCustomColors (void);

EXTERN_CVAR (am_rotate)
EXTERN_CVAR (am_overlay)
EXTERN_CVAR (st_scale)
EXTERN_CVAR (am_usecustomcolors)
EXTERN_CVAR (r_stretchsky)
EXTERN_CVAR (wipetype)
EXTERN_CVAR (screenblocks)
EXTERN_CVAR (dimamount)
EXTERN_CVAR (usehighresboard)
EXTERN_CVAR (show_endoom)

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
	{ more,		"Messages",				{NULL},					{0.0}, {0.0},	{0.0}, {(value_t *)StartMessagesMenu} },
	{ redtext,	" ",					{NULL},					{0.0}, {0.0},	{0.0}, {NULL} },
	{ slider,	"Screen size",			{&screenblocks},	   	{3.0}, {12.0},	{1.0}, {NULL} },
	{ slider,	"Brightness",			{&gammalevel},			{1.0}, {5.0},	{1.0}, {NULL} },
	{ redtext,	" ",					{NULL},					{0.0}, {0.0},	{0.0}, {NULL} },
	{ slider,   "UI Transparency",      {&dimamount},           {0.0}, {1.0},   {0.1}, {NULL} },
	{ slider,   "UI Trans Red",         {&ui_transred},         {0.0}, {255.0}, {16.0}, {NULL} },
	{ slider,   "UI Trans Green",       {&ui_transgreen},       {0.0}, {255.0}, {16.0}, {NULL} },
	{ slider,   "UI Trans Blue",        {&ui_transblue},        {0.0}, {255.0}, {16.0}, {NULL} },
	{ redtext,	" ",					{NULL},					{0.0}, {0.0},	{0.0}, {NULL} },
	{ discrete,	"Crosshair",			{&crosshair},		   	{9.0}, {0.0},	{0.0}, {Crosshairs} },
	{ discrete, "Stretch short skies",	{&r_stretchsky},	   	{3.0}, {0.0},	{0.0}, {OnOffAuto} },
	{ discrete, "Stretch status bar",	{&st_scale},			{2.0}, {0.0},	{0.0}, {OnOff} },
	{ discrete, "Screen wipe style",	{&wipetype},			{4.0}, {0.0},	{0.0}, {Wipes} },
	{ discrete, "Use high-res scoreboard",	{&usehighresboard},			{2.0}, {0.0},	{0.0}, {OnOff} },
	{ discrete,	"Show player target names",	{&hud_targetcount},		{2.0}, {0.0}, {0.0},	{OnOff} },
	{ redtext,	" ",					{NULL},					{0.0}, {0.0},	{0.0}, {NULL} },
	{ discrete, "Rotate automap",		{&am_rotate},		   	{2.0}, {0.0},	{0.0}, {OnOff} },
	{ discrete, "Overlay automap",		{&am_overlay},			{4.0}, {0.0},	{0.0}, {Overlays} },
	{ discrete, "Standard map colors",	{&am_usecustomcolors},	{2.0}, {0.0},	{0.0}, {NoYes} },
	{ more,     "Reset custom map colors", {NULL},              {0.0}, {0.0},   {0.0}, {(value_t *)ResetCustomColors} }
};

menu_t VideoMenu = {
	"M_VIDEO",
	0,
	STACKARRAY_LENGTH(VideoItems),
	0,
	VideoItems,
};

/*=======================================
 *
 * Messages Menu
 *
 *=======================================*/
EXTERN_CVAR (con_scaletext)
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
	{ 9.0, "yellow" },
	{ 10.0, "white" }
};

static value_t MessageLevels[] = {
	{ 0.0, "Item Pickup" },
	{ 1.0, "Obituaries" },
	{ 2.0, "Critical Messages" }
};

static menuitem_t MessagesItems[] = {
	{ discrete,	"Scale text in high res", {&con_scaletext},		{2.0}, {0.0}, 	{0.0}, {OnOff} },
	{ discrete, "Minimum message level", {&msglevel},		   	{3.0}, {0.0},   {0.0}, {MessageLevels} },
	{ discrete, "Reveal Secrets",       {&revealsecrets},       {2.0}, {0.0},   {0.0}, {OnOff} },
	{ redtext,	" ",					{NULL},					{0.0}, {0.0},	{0.0}, {NULL} },
	{ whitetext, "Message Colors",		{NULL},					{0.0}, {0.0},	{0.0}, {NULL} },
	{ redtext,	" ",					{NULL},					{0.0}, {0.0},	{0.0}, {NULL} },
	{ cdiscrete, "Item Pickup",			{&msg0color},		   	{8.0}, {0.0},	{0.0}, {TextColors} },
	{ cdiscrete, "Obituaries",			{&msg1color},		   	{8.0}, {0.0},	{0.0}, {TextColors} },
	{ cdiscrete, "Critical Messages",	{&msg2color},		   	{8.0}, {0.0},	{0.0}, {TextColors} },
	{ cdiscrete, "Chat Messages",		{&msg3color},		   	{8.0}, {0.0},	{0.0}, {TextColors} },
	{ cdiscrete, "Team Messages",		{&msg4color},		   	{8.0}, {0.0},	{0.0}, {TextColors} },
	{ cdiscrete, "Centered Messages",	{&msgmidcolor},			{8.0}, {0.0},	{0.0}, {TextColors} }
};

menu_t MessagesMenu = {
	"M_MESS",
	0,
	STACKARRAY_LENGTH(MessagesItems),
	0,
	MessagesItems,
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
EXTERN_CVAR (vid_defbits)

static cvar_t DummyDepthCvar (NULL, NULL, 0);

EXTERN_CVAR (vid_fullscreen)

static value_t Depths[22];

static char VMEnterText[] = "Press ENTER to set mode";
static char VMTestText[] = "Press T to test mode for 5 seconds";

static menuitem_t ModesItems[] = {
	{ discrete, "Screen mode",			{&DummyDepthCvar},		{0.0}, {0.0},	{0.0}, {Depths} },
	{ redtext,	" ",					{NULL},					{0.0}, {0.0},	{0.0}, {NULL} },
	{ discrete, "Fullscreen",			{&vid_fullscreen},			{2.0}, {0.0},	{0.0}, {YesNo} },
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
	{ whitetext,"Note: Only 8 bpp modes are supported",{NULL},	{0.0}, {0.0},	{0.0}, {NULL} },
	{ redtext,  VMEnterText,			{NULL},					{0.0}, {0.0},	{0.0}, {NULL} },
	{ redtext,	" ",					{NULL},					{0.0}, {0.0},	{0.0}, {NULL} },
	{ redtext,  VMTestText,				{NULL},					{0.0}, {0.0},	{0.0}, {NULL} },
	{ redtext,	" ",					{NULL},					{0.0}, {0.0},	{0.0}, {NULL} }
};

#define VM_DEPTHITEM	0
#define VM_RESSTART		4
#define VM_ENTERLINE	14
#define VM_TESTLINE		16

menu_t ModesMenu = {
	"M_VIDMOD",
	4,
	STACKARRAY_LENGTH(ModesItems),
	130,
	ModesItems,
};

static cvar_t *flagsvar;

EXTERN_CVAR(dimcolor)

// [Russell] - Modified to send new colours
static void M_SendUINewColor (int red, int green, int blue)
{
	char command[24];

	sprintf (command, "dimcolor \"%02x %02x %02x\"", red, green, blue);
	AddCommandString (command);
}

static void M_SlideUIRed (int val)
{
	int color = V_GetColorFromString(NULL, dimcolor.cstring());
	int red = val;

	M_SendUINewColor (red, GPART(color), BPART(color));
}

static void M_SlideUIGreen (int val)
{
    int color = V_GetColorFromString(NULL, dimcolor.cstring());
	int green = val;

	M_SendUINewColor (RPART(color), green, BPART(color));
}

static void M_SlideUIBlue (int val)
{
    int color = V_GetColorFromString(NULL, dimcolor.cstring());
	int blue = val;

	M_SendUINewColor (RPART(color), GPART(color), blue);
}

//
//		Set some stuff up for the video modes menu
//
static byte BitTranslate[16];

void M_OptInit (void)
{
	int currval = 0, dummy1, dummy2, i;
	char name[24];

	for (i = 1; i < 32; i++)
	{
		I_StartModeIterator (i);
		if (I_NextMode (&dummy1, &dummy2))
		{
			Depths[currval].value = currval;
			sprintf (name, "%d bit", i);
			delete[] Depths[currval].name;
			Depths[currval].name = copystring (name);
			BitTranslate[currval] = i;
			currval++;
		}
	}

	ModesItems[VM_DEPTHITEM].b.min = (float)currval;

	switch (I_DisplayType ())
	{
	case DISPLAY_FullscreenOnly:
		ModesItems[2].type = nochoice;
		ModesItems[2].b.min = 1.f;
		break;
	case DISPLAY_WindowOnly:
		ModesItems[2].type = nochoice;
		ModesItems[2].b.min = 0.f;
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
		Printf (128, "%s\n", MSGOFF);
		show_messages.Set (0.0f);
	}
	else
	{
		Printf (128, "%s\n", MSGON);
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
	S_Sound (CHAN_VOICE, "plats/pt1_mid", 1, ATTN_NONE);
}
END_COMMAND (sizedown)

BEGIN_COMMAND (sizeup)
{
	M_SizeDisplay(1.0);
	S_Sound (CHAN_VOICE, "plats/pt1_mid", 1, ATTN_NONE);
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

	if (menu == &ControlsMenu)
		I_ResumeMouse ();

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

void M_DrawSlider (int x, int y, float min, float max, float cur)
{
	float range;
	int i;

	range = max - min;

	if (cur > max)
		cur = max;
	else if (cur < min)
		cur = min;

	cur -= min;

	screen->DrawPatchClean (W_CachePatch ("LSLIDE"), x, y);
	for (i = 1; i < 11; i++)
		screen->DrawPatchClean (W_CachePatch ("MSLIDE"), x + i*8, y);
	screen->DrawPatchClean (W_CachePatch ("RSLIDE"), x + 88, y);

	screen->DrawPatchClean (W_CachePatch ("CSLIDE"), x + 5 + (int)((cur * 78.0) / range), y);
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
	int valx = 0, valy = 0;
	int theight = 0;
	menuitem_t *item;
	patch_t *title;

	title = W_CachePatch (CurrentMenu->title);
	screen->DrawPatchClean (title, 160-title->width()/2, 10);

	y = 15 + title->height();
	ytop = y + CurrentMenu->scrolltop * 8;

	for (i = 0; i < CurrentMenu->numitems && y <= 200 - theight; i++, y += 8)	// TIJ
	{
		if (i == CurrentMenu->scrolltop)
			i += CurrentMenu->scrollpos;
				
		item = CurrentMenu->items + i;

		if (item->type != screenres)
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

			if (!i)
			{
				valx = x;
				valy = y;
			}

			switch (item->type)
			{
			case discrete:
			case cdiscrete:
			{
				int v, vals;

				vals = (int)item->b.min;
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
										   (item->e.values[(int)item->b.min]).name);
				break;

			case slider:
				M_DrawSlider (CurrentMenu->indent + 8, y, item->b.min, item->c.max, item->a.cvar->value());
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

				if (item->b.min)
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

			default:
				break;
			}

			if (i == CurrentItem && (skullAnimCounter < 6 || WaitingForKey))
			{
				screen->DrawPatchClean (W_CachePatch ("LITLCURS"), CurrentMenu->indent + 3, y);
			}
		}
		else
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

			if (i == CurrentItem && ((item->a.selmode != -1 && (skullAnimCounter < 6 || WaitingForKey)) || testingmode))
			{
				screen->DrawPatchClean (W_CachePatch ("LITLCURS"), item->a.selmode * 104 + 8, y);
			}
		}
	}
	
	VisBottom = i - 1;
	CanScrollUp = (CurrentMenu->scrollpos != 0);
	CanScrollDown = (i < CurrentMenu->numitems);

	if (CanScrollUp)
		screen->DrawPatchClean (W_CachePatch ("LITLUP"), 3, ytop);

	if (CanScrollDown)
		screen->DrawPatchClean (W_CachePatch ("LITLDN"), 3, (CleanYfac < 3 ? 190 : 200));
}

void M_OptResponder (event_t *ev)
{
	menuitem_t *item;
	int ch = ev->data1;

	item = CurrentMenu->items + CurrentItem;

	if (WaitingForKey)
	{
		if (ch != KEY_ESCAPE && ev->type == ev_keydown)
		{
			C_ChangeBinding (item->e.command, ch);
			M_BuildKeyList (CurrentMenu->items, CurrentMenu->numitems);
		}
		WaitingForKey = false;
		CurrentMenu->items[0].label = OldMessage;
		CurrentMenu->items[0].type = OldType;
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

	switch (ch)
	{
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

				S_Sound (CHAN_VOICE, "plats/pt1_stop", 1, ATTN_NONE);
			}
			break;

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

				S_Sound (CHAN_VOICE, "plats/pt1_stop", 1, ATTN_NONE);
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
					S_Sound (CHAN_VOICE, "plats/pt1_stop", 1, ATTN_NONE);
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
					S_Sound (CHAN_VOICE, "plats/pt1_stop", 1, ATTN_NONE);
				}
			}
			break;
		
		case KEY_LEFTARROW:
			switch (item->type)
			{
				case slider:
					{
						float newval = item->a.cvar->value() - item->d.step;

						if (newval < item->b.min)
							newval = item->b.min;

						if (item->e.cfunc)
							item->e.cfunc (item->a.cvar, newval);
						else
							item->a.cvar->Set (newval);
					}
					S_Sound (CHAN_VOICE, "plats/pt1_mid", 1, ATTN_NONE);
					break;

				case discrete:
				case cdiscrete:
					{
						int cur;
						int numvals;

						numvals = (int)item->b.min;
						cur = M_FindCurVal (item->a.cvar->value(), item->e.values, numvals);
						if (--cur < 0)
							cur = numvals - 1;

						item->a.cvar->Set (item->e.values[cur].value);

						// Hack hack. Rebuild list of resolutions
						if (item->e.values == Depths)
							BuildModesList (screen->width, screen->height, DisplayBits);
					}
					S_Sound (CHAN_VOICE, "plats/pt1_mid", 1, ATTN_NONE);
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
					S_Sound (CHAN_VOICE, "plats/pt1_stop", 1, ATTN_NONE);
					break;

				default:
					break;
			}
			break;

		case KEY_RIGHTARROW:
			switch (item->type)
			{
				case slider:
					{
						float newval = item->a.cvar->value() + item->d.step;

						if (newval > item->c.max)
							newval = item->c.max;

						if (item->e.cfunc)
							item->e.cfunc (item->a.cvar, newval);
						else
							item->a.cvar->Set (newval);
					}
					S_Sound (CHAN_VOICE, "plats/pt1_mid", 1, ATTN_NONE);
					break;

				case discrete:
				case cdiscrete:
					{
						int cur;
						int numvals;

						numvals = (int)item->b.min;
						cur = M_FindCurVal (item->a.cvar->value(), item->e.values, numvals);
						if (++cur >= numvals)
							cur = 0;

						item->a.cvar->Set (item->e.values[cur].value);

						// Hack hack. Rebuild list of resolutions
						if (item->e.values == Depths)
							BuildModesList (screen->width, screen->height, DisplayBits);
					}
					S_Sound (CHAN_VOICE, "plats/pt1_mid", 1, ATTN_NONE);
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
					S_Sound (CHAN_VOICE, "plats/pt1_stop", 1, ATTN_NONE);
					break;

				default:
					break;
			}
			break;

		case KEY_BACKSPACE:
			if (item->type == control)
			{
				C_UnbindACommand (item->e.command);
				item->b.key1 = item->c.key2 = 0;
			}
			break;

		case KEY_ENTER:
			if (CurrentMenu == &ModesMenu)
			{
				if (!(item->type == screenres && GetSelectedSize (CurrentItem, &NewWidth, &NewHeight)))
				{
					NewWidth = screen->width;
					NewHeight = screen->height;
				}
				NewBits = BitTranslate[(int)DummyDepthCvar];
				setmodeneeded = true;
				S_Sound (CHAN_VOICE, "weapons/pistol", 1, ATTN_NONE);
				vid_defwidth.Set ((float)NewWidth);
				vid_defheight.Set ((float)NewHeight);
				vid_defbits.Set ((float)NewBits);
				SetModesMenu (NewWidth, NewHeight, NewBits);
			}
			else if (item->type == more && item->e.mfunc)
			{
				CurrentMenu->lastOn = CurrentItem;
				S_Sound (CHAN_VOICE, "weapons/pistol", 1, ATTN_NONE);
				item->e.mfunc();
			}
			else if (item->type == discrete || item->type == cdiscrete)
			{
				int cur;
				int numvals;

				numvals = (int)item->b.min;
				cur = M_FindCurVal (item->a.cvar->value(), item->e.values, numvals);
				if (++cur >= numvals)
					cur = 0;

				item->a.cvar->Set (item->e.values[cur].value);

				// Hack hack. Rebuild list of resolutions
				if (item->e.values == Depths)
					BuildModesList (screen->width, screen->height, DisplayBits);

				S_Sound (CHAN_VOICE, "plats/pt1_mid", 1, ATTN_NONE);
			}
			else if (item->type == control)
			{
				WaitingForKey = true;
				OldMessage = CurrentMenu->items[0].label;
				OldType = CurrentMenu->items[0].type;
				CurrentMenu->items[0].label = "Press new key for control or ESC to cancel";
				CurrentMenu->items[0].type = redtext;
			}
			else if (item->type == listelement)
			{
				CurrentMenu->lastOn = CurrentItem;
				S_Sound (CHAN_VOICE, "weapons/pistol", 1, ATTN_NONE);
				item->e.lfunc (CurrentItem);
			}
			else if (item->type == screenres)
			{
			}
			break;

		case KEY_ESCAPE:
			CurrentMenu->lastOn = CurrentItem;
			M_PopMenuStack ();
			break;

		default:
			if (ev->data2 == 't')
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
					NewBits = BitTranslate[(int)DummyDepthCvar];
					setmodeneeded = true;
					testingmode = I_GetTime() + 5 * TICRATE;
					S_Sound (CHAN_VOICE, "weapons/pistol", 1, ATTN_NONE);
					SetModesMenu (NewWidth, NewHeight, NewBits);
				}
			}
			break;
	}
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
	cvar_t::C_SetCVarsToDefaults ();
	UpdateStuff();
}

void Reset2Saved (void)
{
	std::string cmd = "exec \"";
	cmd += GetConfigPath();
	cmd += "\"";

	AddCommandString (cmd.c_str());
	UpdateStuff();
}

static void StartMessagesMenu (void)
{
	M_SwitchMenu (&MessagesMenu);
}

void ResetCustomColors (void)
{
	AddCommandString ("resetcustomcolors");
}

void MouseSetup (void) // [Toke] for mouse menu
{
	M_SwitchMenu (&MouseMenu);
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
	S_Sound (CHAN_VOICE, "switches/normbutn", 1, ATTN_NONE);
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


BEGIN_COMMAND (menu_display)
{
	M_StartControlPanel ();
	S_Sound (CHAN_VOICE, "switches/normbutn", 1, ATTN_NONE);
	OptionsActive = true;
	M_SwitchMenu (&VideoMenu);
}
END_COMMAND (menu_display)

static void BuildModesList (int hiwidth, int hiheight, int hi_bits)
{
	char strtemp[32];
    const char **str = NULL;
	int	 i, c;
	int	 width, height, showbits;

	showbits = BitTranslate[(int)DummyDepthCvar];

	I_StartModeIterator (showbits);

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
				if (/* hi_bits == showbits && */ width == hiwidth && height == hiheight)
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
		I_StartModeIterator (BitTranslate[(int)DummyDepthCvar]);

		stopat = (line - VM_RESSTART) * 3 + ModesItems[line].a.selmode + 1;

		for (i = 0; i < stopat; i++)
			if (!I_NextMode (width, height))
				return false;
	}

	return true;
}

static int FindBits (int bits)
{
	int i;

	for (i = 0; i < 22; i++)
	{
		if (BitTranslate[i] == bits)
			return i;
	}

	return 0;
}

static void SetModesMenu (int w, int h, int bits)
{
	char strtemp[64];

	DummyDepthCvar.Set ((float)FindBits (bits));

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
	S_Sound (CHAN_VOICE, "switches/normbutn", 1, ATTN_NONE);
	OptionsActive = true;
	SetVidMode ();
}
END_COMMAND (menu_video)


VERSION_CONTROL (m_options_cpp, "$Id$")



