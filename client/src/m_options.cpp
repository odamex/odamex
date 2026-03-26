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

#include "s_sound.h"
#include "i_music.h"
#include "i_musicsystem.h"


#include "m_misc.h"
#include "cl_demo.h"
#include "gi.h"
#include "m_menuconf.h"
#include "m_options_valuesets.h"

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
	const char* LocalizedMenuString(const char* key)
	{
		if (GStrings.hasString(key))
		{
			const char* s = GStrings(key);
			if (s && s[0])
			{
				return s;
			}
		}

		return key;
	}

	const menuconftheme_t& MenuConfTheme()
	{
		return M_MenuConf().theme;
	}

	struct configuredoptionsbridge_t;
	configuredoptionsbridge_t* ConfiguredOptionsBridgeByMenu(menu_t* menu);
	menu_t* OptionsMenuSlotById(const std::string& menuId);

	const patch_t* MenuConfPatch(const std::string& name)
	{
		return !name.empty() && W_CheckNumForName(name.c_str()) >= 0 ? W_CachePatch(name.c_str()) : nullptr;
	}

	int MenuCursorOffsetY()
	{
		return MenuConfTheme().cursorOffsetY;
	}

	const patch_t* MenuCursorPatch()
	{
		const patch_t* patch = MenuConfPatch(MenuConfTheme().cursorPatch);
		return patch != nullptr ? patch : MenuConfPatch("LITLCURS");
	}

	const patch_t* MenuSliderLeftPatch()
	{
		const patch_t* patch = MenuConfPatch(MenuConfTheme().slider.leftPatch);
		return patch != nullptr ? patch : MenuConfPatch("LSLIDE");
	}

	const patch_t* MenuSliderMiddlePatch()
	{
		const patch_t* patch = MenuConfPatch(MenuConfTheme().slider.middlePatch);
		return patch != nullptr ? patch : MenuConfPatch("MSLIDE");
	}

	const patch_t* MenuSliderRightPatch()
	{
		const patch_t* patch = MenuConfPatch(MenuConfTheme().slider.rightPatch);
		return patch != nullptr ? patch : MenuConfPatch("RSLIDE");
	}

	const patch_t* MenuSliderKnobPatch()
	{
		const patch_t* patch = MenuConfPatch(MenuConfTheme().slider.knobPatch);
		return patch != nullptr ? patch : MenuConfPatch("CSLIDE");
	}

	const patch_t* MenuSliderGreenKnobPatch()
	{
		const patch_t* patch = MenuConfPatch(MenuConfTheme().slider.greenKnobPatch);
		return patch != nullptr ? patch : MenuConfPatch("GSLIDE");
	}

	const patch_t* MenuSliderOverlayPatch()
	{
		const patch_t* patch = MenuConfPatch(MenuConfTheme().slider.overlayPatch);
		return patch != nullptr ? patch : MenuConfPatch("OSLIDE");
	}
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
void M_SizeDisplay(float diff);
void M_StartControlPanel(void);

void M_ClearMenus (void);

static bool CanScrollUp;
static bool CanScrollDown;
static int VisBottom;

menu_t  *CurrentMenu;
int		CurrentItem;
bool configuring_controls = false;
static bool	WaitingForKey;
static bool	WaitingForAxis;
static const char	   *OldContMessage;
static itemtype OldContType;
static const char	   *OldAxisMessage;
static itemtype OldAxisType;

/*=======================================
 *
 * Options Menu
 *
 *=======================================*/

static void GoToConsole();
void Reset2Defaults();
void Reset2Saved();

static void SetVidMode();
static void M_UpdateDisplayOptions();


menu_t OptionMenu = {"", 0, 0, 0, nullptr, 0, 0, NULL};

/*=======================================
 *
 * Controls Menu
 *
 *=======================================*/


menu_t ControlsMenu = {"", 0, 0, 0, nullptr, 0, 0, NULL};

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



menu_t MouseMenu = {"", 0, 0, 0, nullptr, 0, 0, NULL};


/*=======================================
 *
 * Joystick Menu
 *
 *=======================================*/


menu_t JoystickMenu = {"", 0, 0, 0, nullptr, 0, 0, NULL};

 /*=======================================
  *
  * Sound Menu [Ralphis]
  *
  *=======================================*/

static value_t MusSys[] = {
	#ifndef _WIN32
	{ MS_SDLMIXER,	"SDL Mixer"},
	#endif
	{ MS_LIBADLMIDI,"libADLMIDI (OPL3 FM)"},
	#ifdef OSX
	{ MS_AUDIOUNIT,	"AudioUnit"},
	#endif	// OSX
	#ifdef PORTMIDI
	{ MS_PORTMIDI,	"PortMidi"},
	#endif	// PORTMIDI
};

static value_t MidiReset[] = {
	{ 0.0,			"None" },
	{ 1.0,			"GM" },
	{ 2.0,			"GS" },
	{ 3.0,			"XG" }
};

static value_t OplCore[] = {
	{ 0.0,			"Fast (Dosbox)"},
	{ 1.0,			"Balanced (Nuked 1.74)"},
	{ 2.0,			"Accurate (Nuked 1.8)"}
};

static value_t OplBank[] = {
	{ 0.0,			"Doom"},
	{ 1.0,			"Doom II"},
	{ 2.0,			"DMXOPL3"}
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

static void AdvMidiOptions (void);
static void LibAdlMidiOptions (void);

static constexpr float num_mussys = static_cast<float>(ARRAY_LENGTH(MusSys));

EXTERN_CVAR(cl_chatsounds)


menu_t AdvMidiMenu = {"", 0, 0, 0, nullptr, 0, 0, NULL};

menu_t LibAdlMidiMenu = {"", 0, 0, 0, nullptr, 0, 0, NULL};

menu_t SoundMenu = {"", 0, 0, 0, nullptr, 0, 0, NULL};


/*=======================================
 *
 * Compatibility Options Menu
 *
 *=======================================*/

menu_t CompatMenu = {"", 0, 0, 0, nullptr, 0, 0, NULL};


/*=======================================
 *
 * Network Options Menu
 *
 *=======================================*/


menu_t NetworkMenu = {"", 0, 0, 0, nullptr, 0, 0, NULL};


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


menu_t WeaponMenu = {"", 0, 0, 0, nullptr, 0, 0, NULL};


/*=======================================
 *
 * Display Options Menu
 *
 *=======================================*/
void ResetCustomColors (void);

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

static value_t Wipes[] = {
	{ 0.0, "None" },
	{ 1.0, "Melt" },
	{ 2.0, "Burn" },
	{ 3.0, "Crossfade" },
	{ 4.0, "Auto" }
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

static value_t Endoom[] = {{0.0, "Off"}, {1.0, "On"}, {2.0, "PWAD Only"}};

menu_t VideoMenu = {"", 0, 0, 0, nullptr, 0, 0, &M_UpdateDisplayOptions};

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

static value_t ExtendedHudStyles[] = {{0.0, "Off"}, {1.0, "Horizontal 1"}, {2.0, "Horizontal 2"},
								 {3.0, "Vertical 1"}, {4.0, "Vertical 2"},};


menu_t HUDMenu = {"", 0, 0, 0, nullptr, 0, 0, NULL};

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
//static value_t Languages[] = { // unused
//	{ 0.0, "Auto" },
//	{ 1.0, "English" },
//	{ 2.0, "French" },
//	{ 3.0, "Italian" }
//};

static value_t ScaleFactors[] = {{0.0, "Auto"}, {1.0, "1X"}, {2.0, "2X"},
                                 {3.0, "3X"},   {4.0, "4X"}, {5.0, "5X"}};


menu_t MessagesMenu = {"", 0, 0, 0, nullptr, 0, 0, NULL};

/*=======================================
 *
 * Automap Menu
 *
 *=======================================*/

static value_t ClassicMapStringTypes[] = {
	{ 0.0, "Odamex" },
	{ 1.0, "Classic" }
};

static value_t AutomapScales[] = {
	{ 0.0, "Auto" },
	{ 1.0, "1X" },
	{ 2.0, "2X" },
	{ 3.0, "3X" },
	{ 4.0, "4X" },
	{ 5.0, "5X" },
	{ 6.0, "6X" },
};

static value_t MinimapLocations[] = {
	{ 0.0, "Left Top" },
	{ 1.0, "Left Middle" },
	{ 2.0, "Left Bottom" },
	{ 3.0, "Right Top" },
	{ 4.0, "Right Middle" },
	{ 5.0, "Right Bottom" },
};


menu_t AutomapMenu = {"", 0, 0, 0, nullptr, 0, 0, NULL};


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

	AddCommandString(fmt::format("vid_setmode {} {}", width, height));

	SetModesMenu(width, height);
}


void M_RestoreVideoMode()
{
	testingmode = 0;
	M_SetVideoMode(old_width, old_height);
}


static value_t Depths[22];

#ifdef GCONSOLE
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
	static_cast<int>(ARRAY_LENGTH(ModesItems)),
	130,
	ModesItems,
	0,
	0,
	NULL
};

namespace
{
	struct configuredoptionsbridge_t
	{
		std::string menuId;
		menu_t* menu = nullptr;
		menuconfheader_t header;
		std::vector<menuconfitem_t> sourceItems;
		std::vector<menuitem_t> generatedItems;
		std::vector<std::string> labels;
		std::vector<std::string> commands;
	};

	std::vector<configuredoptionsbridge_t> gConfiguredOptionsMenus;

	void WarnMenuConfOption(const std::string& message)
	{
		PrintFmt(PRINT_WARNING, "MENUCONF: {}\n", message);
	}

	configuredoptionsbridge_t* ConfiguredOptionsBridgeByMenu(menu_t* menu)
	{
		for (configuredoptionsbridge_t& bridge : gConfiguredOptionsMenus)
		{
			if (bridge.menu == menu)
			{
				return &bridge;
			}
		}

		return nullptr;
	}

	const configuredoptionsbridge_t* ConfiguredOptionsBridgeByMenu(const menu_t* menu)
	{
		for (const configuredoptionsbridge_t& bridge : gConfiguredOptionsMenus)
		{
			if (bridge.menu == menu)
			{
				return &bridge;
			}
		}

		return nullptr;
	}

	menu_t* OptionsMenuSlotById(const std::string& menuId)
	{
		if (menuId == "options") return &OptionMenu;
		if (menuId == "options.controls") return &ControlsMenu;
		if (menuId == "options.mouse") return &MouseMenu;
		if (menuId == "options.joystick") return &JoystickMenu;
		if (menuId == "options.sound") return &SoundMenu;
		if (menuId == "options.sound.advancedMidi") return &AdvMidiMenu;
		if (menuId == "options.sound.opl") return &LibAdlMidiMenu;
		if (menuId == "options.compat") return &CompatMenu;
		if (menuId == "options.network") return &NetworkMenu;
		if (menuId == "options.weapons") return &WeaponMenu;
		if (menuId == "options.display") return &VideoMenu;
		if (menuId == "options.display.hud") return &HUDMenu;
		if (menuId == "options.display.messages") return &MessagesMenu;
		if (menuId == "options.display.automap") return &AutomapMenu;
		return nullptr;
	}

	const char* ResolveOptionLabel(const menuconfitem_t& item)
	{
		if (!item.languageKey.empty())
		{
			return LocalizedMenuString(item.languageKey.c_str());
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

	bool HasBindingItems(const configuredoptionsbridge_t& bridge)
	{
		for (const menuitem_t& item : bridge.generatedItems)
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

	argb_t MenuSliderColor(const menuitem_t& item)
	{
		if (IsUIChannelSlider(item.a.cvar))
		{
			return argb_t(ui_transred.asInt(), ui_transgreen.asInt(), ui_transblue.asInt());
		}

		return V_GetColorFromString(*item.a.cvar);
	}

	int MenuSliderChannelValue(const menuitem_t& item)
	{
		if (IsUIChannelSlider(item.a.cvar))
		{
			return item.a.cvar->asInt();
		}

		const argb_t color = V_GetColorFromString(*item.a.cvar);
		switch (item.type)
		{
		case redslider: return color.getr();
		case greenslider: return color.getg();
		case blueslider: return color.getb();
		default: return 0;
		}
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

	int FirstSelectableOptionIndex(const configuredoptionsbridge_t& bridge)
	{
		for (size_t i = 0; i < bridge.generatedItems.size(); ++i)
		{
			if (IsSelectableOptionItem(bridge.generatedItems[i]))
			{
				return static_cast<int>(i);
			}
		}

		return 0;
	}

	void ActivateConfiguredOptionsItem()
	{
		configuredoptionsbridge_t* bridge = ConfiguredOptionsBridgeByMenu(CurrentMenu);
		if (bridge == nullptr || CurrentItem < 0 ||
		    static_cast<size_t>(CurrentItem) >= bridge->sourceItems.size())
		{
			return;
		}

		const menuconfitem_t& item = bridge->sourceItems[CurrentItem];

		if (item.action == "openConsole")
		{
			GoToConsole();
			return;
		}
		if (item.action == "resetDefaults")
		{
			Reset2Defaults();
			return;
		}
		if (item.action == "resetSaved")
		{
			Reset2Saved();
			return;
		}
		if (item.action == "resetMouseDefaults")
		{
			M_ResetMouseValues();
			return;
		}
		if (item.action == "resetCustomMapColors")
		{
			ResetCustomColors();
			return;
		}

		if (!item.target.empty())
		{
			M_OpenMenuTarget(item.target);
			return;
		}

		if (!item.action.empty())
		{
			WarnMenuConfOption(fmt::sprintf("options action \"%s\" is not wired yet",
			                                item.action.c_str()));
		}
	}

	bool BuildConfiguredOptionsMenu(configuredoptionsbridge_t& bridge, const std::string& menuId,
	                                menu_t& slot)
	{
		const auto it = M_MenuConf().menus.find(menuId);
		if (it == M_MenuConf().menus.end())
		{
			slot.numitems = 0;
			slot.items = nullptr;
			return false;
		}

		const menuconfmenu_t& authored = it->second;
		bridge.menuId = menuId;
		bridge.menu = &slot;
		bridge.header = authored.header;
		bridge.sourceItems = authored.items;
		bridge.generatedItems.clear();
		bridge.labels.clear();
		bridge.commands.clear();
		bridge.generatedItems.reserve(authored.items.size());
		bridge.labels.reserve(authored.items.size());
		bridge.commands.reserve(authored.items.size());

		for (const menuconfitem_t& item : bridge.sourceItems)
		{
			menuitem_t generated = {};
			generated.type = ResolveGeneratedLabelType(item);

			std::string label = ResolveOptionLabel(item);
			if (generated.type == redtext && item.kind == menuconfitemkind_t::separator)
			{
				label = " ";
			}
			bridge.labels.push_back(std::move(label));
			generated.label = bridge.labels.back().c_str();

			if (generated.type == more)
			{
				generated.e.mfunc = ActivateConfiguredOptionsItem;
			}
			else if (generated.type == control || generated.type == mapcontrol ||
			         generated.type == netdemocontrol)
			{
				bridge.commands.push_back(item.command);
				generated.e.command = bridge.commands.back().c_str();
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
				WarnMenuConfOption(fmt::sprintf("options item \"%s\" references unknown cvar \"%s\"",
				                                generated.label ? generated.label : "",
				                                item.cvar.c_str()));
				continue;
			}

			if ((generated.type == discrete || generated.type == cdiscrete ||
			     generated.type == svdiscrete) &&
			    generated.e.values == nullptr)
			{
				WarnMenuConfOption(fmt::sprintf("options item \"%s\" references unknown value set \"%s\"",
				                                generated.label ? generated.label : "",
				                                item.values.c_str()));
				continue;
			}

			bridge.generatedItems.push_back(generated);
		}

		slot.title = authored.header.patch.empty() ? "" : authored.header.patch.c_str();
		slot.lastOn = FirstSelectableOptionIndex(bridge);
		slot.numitems = static_cast<int>(bridge.generatedItems.size());
		slot.indent = authored.layout.indent;
		slot.items = bridge.generatedItems.data();
		slot.scrolltop = authored.layout.scroll ? 0 : 0;
		slot.scrollpos = 0;
		slot.refreshfunc = menuId == "options.display" ? &M_UpdateDisplayOptions : NULL;
		return true;
	}

	void BuildConfiguredOptionsMenus()
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

		gConfiguredOptionsMenus.clear();
		gConfiguredOptionsMenus.reserve(ARRAY_LENGTH(kMenuIds));

		for (const char* menuId : kMenuIds)
		{
			menu_t* slot = OptionsMenuSlotById(menuId);
			if (slot == nullptr)
			{
				continue;
			}

			gConfiguredOptionsMenus.emplace_back();
			if (!BuildConfiguredOptionsMenu(gConfiguredOptionsMenus.back(), menuId, *slot))
			{
				gConfiguredOptionsMenus.pop_back();
			}
		}
	}

	const patch_t* MenuArrowPatch(const std::string& name)
	{
		return MenuConfPatch(name);
	}

	const char* ConfiguredOptionsHeaderText(const configuredoptionsbridge_t* bridge)
	{
		if (bridge == nullptr)
		{
			return "OPTIONS";
		}

		if (!bridge->header.languageKey.empty())
		{
			return LocalizedMenuString(bridge->header.languageKey.c_str());
		}

		if (!bridge->header.text.empty())
		{
			return bridge->header.text.c_str();
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

	configuredoptionsbridge_t* bridge = ConfiguredOptionsBridgeByMenu(menu);
	if (bridge == nullptr)
	{
		return;
	}

	for (menuitem_t& item : bridge->generatedItems)
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

static void BuildModesList(int hiwidth, int hiheight)
{
	// gathers a list of unique resolutions availible for the current
	// screen mode (windowed or fullscreen)
	bool fullscreen = I_GetWindow()->getVideoMode().isFullScreen();

	typedef std::vector< std::pair<uint16_t, uint16_t> > MenuModeList;
	MenuModeList menumodelist;

	const IVideoModeList* videomodelist = I_GetVideoCapabilities()->getSupportedVideoModes();
	for (const auto& mode : *videomodelist)
		if (mode.isFullScreen() == fullscreen)
			menumodelist.emplace_back(mode.width, mode.height);
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
		snprintf(enter_text, 64, "TESTING %dx%d", w, h);

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

	BuildConfiguredOptionsMenus();
}


//
//		Toggle messages on/off
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
	MenuStack[MenuStackDepth].drawIndicator = false;
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
				thiswidth = V_StringWidth(OFonts.small(), item->label);
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
	return M_OpenGeneratedOptionsMenu("options");
}

bool M_OpenGeneratedOptionsMenu(const std::string& menuId)
{
	menu_t* menu = OptionsMenuSlotById(menuId);
	if (menu == nullptr)
	{
		WarnMenuConfOption(fmt::sprintf("options target \"%s\" is not mapped to a runtime menu",
		                                menuId.c_str()));
		return false;
	}

	const configuredoptionsbridge_t* bridge = ConfiguredOptionsBridgeByMenu(menu);
	if (bridge == nullptr)
	{
		WarnMenuConfOption(fmt::sprintf("options menu \"%s\" is not available", menuId.c_str()));
		return false;
	}

	if (HasBindingItems(*bridge))
	{
		M_BuildKeyList(menu->items, menu->numitems);
	}

	if (menu->numitems > 0 &&
	    (menu->lastOn < 0 || menu->lastOn >= menu->numitems ||
	     !IsSelectableOptionItem(menu->items[menu->lastOn])))
	{
		menu->lastOn = FirstSelectableOptionIndex(*bridge);
	}

	M_SwitchMenu(menu);
	return true;
}

void M_DrawSlider (int x, int y, float leftval, float rightval, float cur, float step)
{
	const OFont* smallFont = OFonts.small();
	const palette_t* palette = V_GetPaletteFromLump("ODAPAL");
	const int drawY = y + MenuCursorOffsetY();
	const patch_t* leftPatch = MenuSliderLeftPatch();
	const patch_t* middlePatch = MenuSliderMiddlePatch();
	const patch_t* rightPatch = MenuSliderRightPatch();
	const patch_t* knobPatch = MenuSliderKnobPatch();

	if (leftval < rightval)
		cur = clamp(cur, leftval, rightval);
	else
		cur = clamp(cur, rightval, leftval);

	float dist = (cur - leftval) / (rightval - leftval);

	screen->DrawPatchCleanWithPalette(leftPatch, x, drawY, palette);
	for (int i = 1; i < 11; i++)
		screen->DrawPatchCleanWithPalette(middlePatch, x + i * 8, drawY, palette);
	screen->DrawPatchCleanWithPalette(rightPatch, x + 88, drawY, palette);

	screen->DrawPatchCleanWithPalette(knobPatch, x + 5 + static_cast<int>(dist * 78.0), drawY, palette);

	std::string buf;
	if (step == 0.0f)
		return;
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
	const palette_t* palette = V_GetPaletteFromLump("ODAPAL");
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

	float dist = (cur - leftval) / (rightval - leftval);

	screen->DrawPatchCleanWithPalette(leftPatch, x, drawY, palette);

	for (int i = 1; i < 11; i++)
		screen->DrawPatchCleanWithPalette(middlePatch, x + i * 8, drawY, palette);

	screen->DrawPatchCleanWithPalette(rightPatch, x + 88, drawY, palette);

	screen->DrawPatchCleanWithPalette(greenKnobPatch, x + 5 + static_cast<int>(dist * 78.0), drawY, palette);

	V_ColorFill = V_BestColor(V_GetDefaultPalette()->basecolors, color);

	screen->DrawColoredPatchClean(overlayPatch, x + 5 + static_cast<int>(dist * 78.0), drawY);
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
	const OFont* smallFont = OFonts.small();
	const OFont* bigFont = OFonts.big();
	int color;
	int y, width, i, x, ytop;
	int ystart = 15;
	menuitem_t *item;
	patch_t *title;
	const int lineHeight = smallFont->lineHeight();
	const palette_t* palette = V_GetPaletteFromLump("ODAPAL");
	const configuredoptionsbridge_t* bridge = ConfiguredOptionsBridgeByMenu(CurrentMenu);

	if (W_CheckNumForName(CurrentMenu->title) >= 0)
	{
		title = W_CachePatch (CurrentMenu->title);
		screen->DrawPatchCleanWithPalette(title, 160-title->width()/2, 10, palette);
		y = ystart + title->height();
	}
	else
	{	
		const char* titleText = ConfiguredOptionsHeaderText(bridge);
		int titlewidth = V_StringWidth(bigFont, titleText) * CleanXfac;
		int titleX = (I_GetSurfaceWidth() / 2) - (titlewidth / 2);
		int titleY = 20*CleanYfac;
		screen->DrawTextClean(bigFont, CR_GRAY, titleX, titleY, titleText);
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
						color = CR_GREY;
					else
						color = CR_RED;

					screen->DrawTextCleanMove(smallFont, color, 104 * x + 20, y, str);
				}
			}

			if (i == CurrentItem && ((item->a.selmode != -1 && (indicatorAnimCounter < 6 || WaitingForKey))
				|| WaitingForAxis || testingmode))
			{
				if (const patch_t* cursor = MenuCursorPatch())
				{
					screen->DrawPatchCleanWithPalette(cursor, item->a.selmode * 104 + 8,
					                                  y + MenuCursorOffsetY(), palette);
				}
			}
		}
		else
		{
			width = V_StringWidth(smallFont, item->label);
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
			screen->DrawTextCleanMove(smallFont, color, x, y, item->label);

			switch (item->type)
			{
			case discrete:
			case cdiscrete:
			case svdiscrete:
			{
				int v, vals;

				vals = static_cast<int>(item->b.leftval);
				v = M_FindCurVal(item->a.cvar->value(), item->e.values, vals);

				if (v == vals)
				{
					screen->DrawTextCleanMove(smallFont, CR_GREY, CurrentMenu->indent + 14, y, "Unknown");
				}
				else
				{
					int color_num = CR_GREY;
					if (item->type == cdiscrete)
						color_num = item->a.cvar->asInt();

					screen->DrawTextCleanMove(smallFont, color_num, CurrentMenu->indent + 14, y, item->e.values[v].name);
				}

			}
			break;

			case nochoice:
				screen->DrawTextCleanMove(smallFont, CR_GOLD, CurrentMenu->indent + 14, y,
										   (item->e.values[static_cast<int>(item->b.leftval)]).name);
				break;

			case slider:
				M_DrawSlider (CurrentMenu->indent + 8, y, item->b.leftval, item->c.rightval, item->a.cvar->value(), item->d.step);
				break;

			case redslider:
			{
				argb_t color = MenuSliderColor(*item);
				M_DrawColoredSlider(CurrentMenu->indent + 8, y, 0, 255,
				                    static_cast<float>(MenuSliderChannelValue(*item)), color);
			}
			break;
			case greenslider:
			{
				argb_t color = MenuSliderColor(*item);
				M_DrawColoredSlider(CurrentMenu->indent + 8, y, 0, 255,
				                    static_cast<float>(MenuSliderChannelValue(*item)), color);
			}
			break;
			case blueslider:
			{
				argb_t color = MenuSliderColor(*item);
				M_DrawColoredSlider(CurrentMenu->indent + 8, y, 0, 255,
				                    static_cast<float>(MenuSliderChannelValue(*item)), color);
			}
			break;

			case control:
			{
				std::string desc = Bindings.GetNameKeys(item->b.key1, item->c.key2);
				screen->DrawTextCleanMove(smallFont, CR_GREY, CurrentMenu->indent + 14, y, desc.c_str());
			}
			break;

			case mapcontrol:
			{
				std::string desc = AutomapBindings.GetNameKeys(item->b.key1, item->c.key2);
				screen->DrawTextCleanMove(smallFont, CR_GREY, CurrentMenu->indent + 14, y, desc.c_str());
			}
			break;

			case netdemocontrol:
			{
				std::string desc = NetDemoBindings.GetNameKeys(item->b.key1, item->c.key2);
				screen->DrawTextCleanMove(smallFont, CR_GREY, CurrentMenu->indent + 14, y, desc.c_str());
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

				screen->DrawTextCleanMove(smallFont, CR_GREY, CurrentMenu->indent + 14, y, str);
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

				screen->DrawTextCleanMove(smallFont, CR_GREY, CurrentMenu->indent + 14, y, joyname.c_str());
			}
			break;

			case joyaxis:
			{
				screen->DrawTextCleanMove(smallFont, CR_GREY, CurrentMenu->indent + 14, y, item->a.cvar->cstring());
			}
			break;

			default:
				break;
			}

			if (i == CurrentItem && (indicatorAnimCounter < 6 || WaitingForKey || WaitingForAxis))
			{
				if (const patch_t* patch = MenuCursorPatch())
				{
					screen->DrawPatchCleanWithPalette(patch, CurrentMenu->indent + 3,
					                                  y + MenuCursorOffsetY(), palette);
				}
			}
		}
	}

	VisBottom = i - 1;
	CanScrollUp = (CurrentMenu->scrollpos != 0);
	CanScrollDown = (i < CurrentMenu->numitems);

	if (CanScrollUp)
	{
		if (const patch_t* patch = MenuArrowPatch(M_MenuConf().theme.upPatch))
		{
			screen->DrawPatchCleanWithPalette(patch, 3, ytop, palette);
		}
	}

	if (CanScrollDown)
	{
		if (const patch_t* patch = MenuArrowPatch(M_MenuConf().theme.downPatch))
		{
			screen->DrawPatchCleanWithPalette(patch, 3, 190, palette);
		}
	}
}

void M_OptResponder(const event_t& ev)
{
	int ch = ev.data1;
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
			int part = MenuSliderChannelValue(*item);
			part -= static_cast<int>(item->d.step > 0.0f ? item->d.step : 0x11);
			SetMenuSliderChannelValue(*item, part);
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

			numvals = static_cast<int>(item->b.leftval);
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
			size_t numjoy = I_GetJoystickCount();

			if (static_cast<size_t>(item->a.cvar->value()) > numjoy)
				item->a.cvar->Set(0.0);
			else if (static_cast<size_t>(item->a.cvar->value()) > 0)
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
			int part = MenuSliderChannelValue(*item);
			part += static_cast<int>(item->d.step > 0.0f ? item->d.step : 0x11);
			SetMenuSliderChannelValue(*item, part);
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

			numvals = static_cast<int>(item->b.leftval);
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
			size_t numjoy = I_GetJoystickCount();

			if (static_cast<size_t>(item->a.cvar->value()) >= numjoy)
				item->a.cvar->Set(0.0);
			else if (static_cast<size_t>(item->a.cvar->value()) < (numjoy - 1))
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

				numvals = static_cast<int>(item->b.leftval);
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

void ResetCustomColors (void)
{
	AddCommandString ("resetcustomcolors");
}

BEGIN_COMMAND (menu_keys)
{
	M_StartControlPanel ();
	OptionsActive = true;
	M_OpenGeneratedOptionsMenu("options.controls");
}
END_COMMAND (menu_keys)

BEGIN_COMMAND (menu_display)
{
	M_StartControlPanel ();
	OptionsActive = true;
	M_OpenGeneratedOptionsMenu("options.display");
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



