// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
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
//
// Input event handler
//
//-----------------------------------------------------------------------------


#include "odamex.h"

// SoM 12-24-05: yeah... I'm programming on christmas eve.
// Removed all the DirectX crap.

#include <stdlib.h>
#include <list>

#include "i_sdl.h"
#include "win32inc.h"

#include "m_argv.h"
#include "i_input.h"
#include "i_video.h"
#include "d_main.h"
#include "c_bind.h"
#include "c_console.h"
#include "i_system.h"
#include "hu_stuff.h"

#ifdef _XBOX
	#include "i_xbox.h"
#elif __SWITCH__
	#include "nx_io.h"
#endif

#if defined(SDL12)
#include "i_input_sdl12.h"
#elif defined(SDL20)
#include "i_input_sdl20.h"
#endif

#ifdef _WIN32
bool tab_keydown = false;	// [ML] Actual status of tab key
#endif

static IInputSubsystem* input_subsystem = NULL;

static bool nomouse = false;

KeyNameTable key_names;

//
// I_InitializeKeyNameTable
//
// Builds a table mapping keycodes to string representations of key names.
//
static void I_InitializeKeyNameTable()
{
	key_names.clear();
	key_names[OKEY_BACKSPACE] = "backspace";
	key_names[OKEY_TAB] = "tab";
	key_names[OKEY_ENTER] = "enter";
	key_names[OKEY_PAUSE] = "pause";
	key_names[OKEY_ESCAPE] = "escape";
	key_names[OKEY_SPACE] = "space";
	key_names['!'] = "!";
	key_names['\"'] = "\"";
	key_names['#'] = "#";
	key_names['$'] = "$";
	key_names['&'] = "&";
	key_names['\''] = "\'";
	key_names['('] = "(";
	key_names[')'] = ")";
	key_names['*'] = "*";
	key_names['+'] = "+";
	key_names[','] = ",";
	key_names['-'] = "-";
	key_names['.'] = ".";
	key_names['/'] = "/";
	key_names['0'] = "0";
	key_names['1'] = "1";
	key_names['2'] = "2";
	key_names['3'] = "3";
	key_names['4'] = "4";
	key_names['5'] = "5";
	key_names['6'] = "6";
	key_names['7'] = "7";
	key_names['8'] = "8";
	key_names['9'] = "9";
	key_names[':'] = ":";
	key_names[';'] = ":";
	key_names['<'] = "<";
	key_names['='] = "=";
	key_names['>'] = ">";
	key_names['?'] = "?";
	key_names['@'] = "@";
	key_names['['] = "[";
	key_names['\\'] = "\\";
	key_names[']'] = "]";
	key_names['^'] = "^";
	key_names['_'] = "_";
	key_names['`'] = "grave";
	key_names[OKEY_TILDE] = "tilde";
	key_names['a'] = "a";
	key_names['b'] = "b";
	key_names['c'] = "c";
	key_names['d'] = "d";
	key_names['e'] = "e";
	key_names['f'] = "f";
	key_names['g'] = "g";
	key_names['h'] = "h";
	key_names['i'] = "i";
	key_names['j'] = "j";
	key_names['k'] = "k";
	key_names['l'] = "l";
	key_names['m'] = "m";
	key_names['n'] = "n";
	key_names['o'] = "o";
	key_names['p'] = "p";
	key_names['q'] = "q";
	key_names['r'] = "r";
	key_names['s'] = "s";
	key_names['t'] = "t";
	key_names['u'] = "u";
	key_names['v'] = "v";
	key_names['w'] = "w";
	key_names['x'] = "x";
	key_names['y'] = "y";
	key_names['z'] = "z";
	key_names[OKEY_DEL] = "del";
	key_names[OKEYP_0] = "kp0";
	key_names[OKEYP_1] = "kp1";
	key_names[OKEYP_2] = "kp2";
	key_names[OKEYP_3] = "kp3";
	key_names[OKEYP_4] = "kp4";
	key_names[OKEYP_5] = "kp5";
	key_names[OKEYP_6] = "kp6";
	key_names[OKEYP_7] = "kp7";
	key_names[OKEYP_8] = "kp8";
	key_names[OKEYP_9] = "kp9";
	key_names[OKEYP_PERIOD] = "kp.";
	key_names[OKEYP_DIVIDE] = "kp/";
	key_names[OKEYP_MULTIPLY] = "kp*";
	key_names[OKEYP_MINUS] = "kp-";
	key_names[OKEYP_PLUS] = "kp+";
	key_names[OKEYP_ENTER] = "kpenter";
	key_names[OKEYP_EQUALS] = "kp=";
	key_names[OKEY_UPARROW] = "uparrow";
	key_names[OKEY_DOWNARROW] = "downarrow";
	key_names[OKEY_LEFTARROW] = "leftarrow";
	key_names[OKEY_RIGHTARROW] = "rightarrow";
	key_names[OKEY_INS] = "ins";
	key_names[OKEY_HOME] = "home";
	key_names[OKEY_END] = "end";
	key_names[OKEY_PGUP] = "pgup";
	key_names[OKEY_PGDN] = "pgdn";
	key_names[OKEY_F1] = "f1";
	key_names[OKEY_F2] = "f2";
	key_names[OKEY_F3] = "f3";
	key_names[OKEY_F4] = "f4";
	key_names[OKEY_F5] = "f5";
	key_names[OKEY_F6] = "f6";
	key_names[OKEY_F7] = "f7";
	key_names[OKEY_F8] = "f8";
	key_names[OKEY_F9] = "f9";
	key_names[OKEY_F10] = "f10";
	key_names[OKEY_F11] = "f11";
	key_names[OKEY_F12] = "f12";
	key_names[OKEY_F13] = "f13";
	key_names[OKEY_F14] = "f14";
	key_names[OKEY_F15] = "f15";
	key_names[OKEY_NUMLOCK] = "numlock";
	key_names[OKEY_CAPSLOCK] = "capslock";
	key_names[OKEY_SCRLCK] = "scroll";
	key_names[OKEY_RSHIFT] = "rightshift";
	key_names[OKEY_LSHIFT] = "leftshift";
	key_names[OKEY_RCTRL] = "rightctrl";
	key_names[OKEY_LCTRL] = "leftctrl";
	key_names[OKEY_RALT] = "rightalt";
	key_names[OKEY_LALT] = "leftalt";
	key_names[OKEY_LWIN] = "lwin";
	key_names[OKEY_RWIN] = "rwin";
	key_names[OKEY_HELP] = "help";
	key_names[OKEY_PRINT] = "print";
	key_names[OKEY_SYSRQ] = "sysrq";
	key_names[OKEY_MOUSE1] = "mouse1";
	key_names[OKEY_MOUSE2] = "mouse2";
	key_names[OKEY_MOUSE3] = "mouse3";
	key_names[OKEY_MOUSE4] = "mouse4";
	key_names[OKEY_MOUSE5] = "mouse5";
	key_names[OKEY_MWHEELDOWN] = "mwheeldown";
	key_names[OKEY_MWHEELUP] = "mwheelup";
	key_names[OKEY_JOY1] = "joy1";
	key_names[OKEY_JOY2] = "joy2";
	key_names[OKEY_JOY3] = "joy3";
	key_names[OKEY_JOY4] = "joy4";
	key_names[OKEY_JOY5] = "joy5";
	key_names[OKEY_JOY6] = "joy6";
	key_names[OKEY_JOY7] = "joy7";
	key_names[OKEY_JOY8] = "joy8";
	key_names[OKEY_JOY9] = "joy9";
	key_names[OKEY_JOY10] = "joy10";
	key_names[OKEY_JOY11] = "joy11";
	key_names[OKEY_JOY12] = "joy12";
	key_names[OKEY_JOY13] = "joy13";
	key_names[OKEY_JOY14] = "joy14";
	key_names[OKEY_JOY15] = "joy15";
	key_names[OKEY_JOY16] = "joy16";
	key_names[OKEY_JOY17] = "joy17";
	key_names[OKEY_JOY18] = "joy18";
	key_names[OKEY_JOY19] = "joy19";
	key_names[OKEY_JOY20] = "joy20";
	key_names[OKEY_JOY21] = "joy21";
	key_names[OKEY_JOY22] = "joy22";
	key_names[OKEY_JOY23] = "joy23";
	key_names[OKEY_JOY24] = "joy24";
	key_names[OKEY_JOY25] = "joy25";
	key_names[OKEY_JOY26] = "joy26";
	key_names[OKEY_JOY27] = "joy27";
	key_names[OKEY_JOY28] = "joy28";
	key_names[OKEY_JOY29] = "joy29";
	key_names[OKEY_JOY30] = "joy30";
	key_names[OKEY_JOY31] = "joy31";
	key_names[OKEY_JOY32] = "joy32";
	key_names[OKEY_HAT1] = "hat1up";
	key_names[OKEY_HAT2] = "hat1right";
	key_names[OKEY_HAT3] = "hat1down";
	key_names[OKEY_HAT4] = "hat1left";
	key_names[OKEY_HAT5] = "hat2up";
	key_names[OKEY_HAT6] = "hat2right";
	key_names[OKEY_HAT7] = "hat2down";
	key_names[OKEY_HAT8] = "hat2left";

#ifdef __SWITCH__
	NX_InitializeKeyNameTable();
#endif

}


//
// I_GetKeyFromName
//
// Returns the key code for the given key name
//
int I_GetKeyFromName(const std::string& name)
{
	if (key_names.empty())
		I_InitializeKeyNameTable();

	// Names of the form #xxx are translated to key xxx automatically
	if (name[0] == '#' && name[1] != 0)
		return atoi(name.c_str() + 1);

	// Otherwise, we scan the KeyNames[] array for a matching name
	for (KeyNameTable::const_iterator it = key_names.begin(); it != key_names.end(); ++it)
	{
		if (iequals(name, it->second))
			return it->first;
	}
	return 0;
}


//
// I_GetKeyName
//
// Returns the key code for the given key name
//
std::string I_GetKeyName(int key)
{
	if (key_names.empty())
		I_InitializeKeyNameTable();

	KeyNameTable::const_iterator it = key_names.find(key);
	if (it != key_names.end() && !it->second.empty())
		return it->second;

	static char name[11];
	snprintf(name, 11, "#%d", key);
	return std::string(name);
}


//
// I_FlushInput
//
// Eat all pending input from outside the game
//
void I_FlushInput()
{
	C_ReleaseKeys();

	input_subsystem->flushInput();
}


//
// I_EnableKeyRepeat
//
// Enables key repeat for text entry (console, menu system, and chat).
//
static void I_EnableKeyRepeat()
{
	input_subsystem->enableKeyRepeat();
}


//
// I_DisableKeyRepeat
//
// Disables key repeat for standard game control.
//
static void I_DisableKeyRepeat()
{
	input_subsystem->disableKeyRepeat();
}


//
// I_CanRepeat
//
// Returns true if the input (joystick & keyboard) should have their buttons repeated.
//
static bool I_CanRepeat()
{
	return ConsoleState == c_down || HU_ChatMode() != CHAT_INACTIVE || menuactive;
}


//
// I_CanGrab
//
// Returns true if the input (mouse & keyboard) can be grabbed in
// the current game state.
//
static bool I_CanGrab()
{
	#ifdef GCONSOLE
	return true;
	#endif

	extern bool configuring_controls;
	extern constate_e ConsoleState;

	assert(I_GetWindow() != NULL);

	// If the window doesn't have the focus, don't grab
	if (!I_GetWindow()->isFocused())
		return false;

	// If the window is full screen and has only one monitor, always grab
	if (I_GetWindow()->isFullScreen() && I_GetMonitorCount() <= 1)
		return true;

	if (nomouse)
		return false;

	// Always grab when configuring controllers in the menu
	if (configuring_controls)
		return true;

	// If paused, in the menu or in the console, don't grab
	if (menuactive || ConsoleState == c_down || paused)
		return false;

	// If playing the game, always grab
	if ((gamestate == GS_LEVEL || gamestate == GS_INTERMISSION) && !demoplayback)
		return true;

	return false;
}


//
// I_GrabInput
//
static void I_GrabInput()
{
	input_subsystem->grabInput();
}


//
// I_UngrabInput
//
static void I_UngrabInput()
{
	input_subsystem->releaseInput();
}


//
// I_ForceUpdateGrab
//
// Determines if the input should be grabbed based on the game window having
// focus and the status of the menu and console.
//
// Should be called whenever the video mode changes.
//
void I_ForceUpdateGrab()
{
	if (I_CanGrab())
		I_GrabInput();
	else
		I_UngrabInput();
}


//
// I_UpdateGrab
//
// Determines if the input should be grabbed based on the game window having
// focus and the status of the menu and console.
//
static void I_UpdateGrab()
{
#ifndef GCONSOLE
	// force I_ResumeMouse or I_PauseMouse if toggling between fullscreen/windowed
	bool fullscreen = I_GetWindow()->isFullScreen();
	static bool prev_fullscreen = fullscreen;
	if (fullscreen != prev_fullscreen) 
		I_ForceUpdateGrab();
	prev_fullscreen = fullscreen;

	// check if the window focus changed (or menu/console status changed)
	if (!input_subsystem->isInputGrabbed() && I_CanGrab())
		I_GrabInput();
	else if (input_subsystem->isInputGrabbed() && !I_CanGrab())
		I_UngrabInput();
#endif
}


CVAR_FUNC_IMPL(use_joystick)
{
	if (var == 0.0f)
	{
#ifdef GCONSOLE
		// Don't let console users disable joystick support because
		// they won't have any way to reenable through the menu.
		var = 1.0f;
#else
		I_CloseJoystick();
#endif
	}
	else
	{
		I_OpenJoystick();
	}
}


CVAR_FUNC_IMPL(joy_active)
{
	const std::vector<IInputDeviceInfo> devices = input_subsystem->getJoystickDevices();
	for (std::vector<IInputDeviceInfo>::const_iterator it = devices.begin(); it != devices.end(); ++it)
	{
		if (it->mId == (int)var)
		{
			I_OpenJoystick();
			return;
		}
	}

#ifdef GCONSOLE	
	// Don't let console users choose an invalid joystick because
	// they won't have any way to reenable through the menu.
	if (!devices.empty())
		var = devices.front().mId;
#endif
}


//
// I_GetJoystickCount
//
int I_GetJoystickCount()
{
	const std::vector<IInputDeviceInfo> devices = input_subsystem->getJoystickDevices();
	return devices.size();
}

//
// I_GetJoystickNameFromIndex
//
std::string I_GetJoystickNameFromIndex(int index)
{
	const std::vector<IInputDeviceInfo> devices = input_subsystem->getJoystickDevices();
	for (std::vector<IInputDeviceInfo>::const_iterator it = devices.begin(); it != devices.end(); ++it)
	{
		if (it->mId == index)
			return it->mDeviceName;
	}
	return "";
}


//
// I_OpenJoystick
//
bool I_OpenJoystick()
{
	I_CloseJoystick();		// just in case it was left open...
	
	if (use_joystick != 0)
	{
		// Verify that the joystick ID indicated by the joy_active CVAR
		// is valid and if so, initialize that joystick
		const std::vector<IInputDeviceInfo> devices = input_subsystem->getJoystickDevices();
		for (std::vector<IInputDeviceInfo>::const_iterator it = devices.begin(); it != devices.end(); ++it)
		{
			if (it->mId == joy_active.asInt())
			{
				input_subsystem->initJoystick(it->mId);
				return true;
			}
		}
	}
	return false;
}

//
// I_CloseJoystick
//
void I_CloseJoystick()
{
	// Verify that the joystick ID indicated by the joy_active CVAR
	// is valid and if so, shutdown that joystick
	const std::vector<IInputDeviceInfo> devices = input_subsystem->getJoystickDevices();
	for (std::vector<IInputDeviceInfo>::const_iterator it = devices.begin(); it != devices.end(); ++it)
	{
		if (it->mId == joy_active.asInt())
			input_subsystem->shutdownJoystick(it->mId);
	}

	// Reset joy position values. Wouldn't want to get stuck in a turn or something. -- Hyper_Eye
	extern int joyforward, joystrafe, joyturn, joylook;
	joyforward = joystrafe = joyturn = joylook = 0;
}



// ============================================================================
//
// Mouse Drivers
//
// ============================================================================

//
// I_CloseMouse()
//
void I_CloseMouse()
{
	input_subsystem->shutdownMouse(0);
}


//
// I_OpenMouse
//
bool I_OpenMouse()
{
	if (!nomouse)
	{
		I_CloseMouse();
		input_subsystem->initMouse(0);
		return true;
	}
	return false;
}


//
// I_InitInput
//
bool I_InitInput()
{
	if (Args.CheckParm("-nomouse"))
		nomouse = true;

	atterm(I_ShutdownInput);

	#if defined(SDL12)
	input_subsystem = new ISDL12InputSubsystem();
	#elif defined(SDL20)
	input_subsystem = new ISDL20InputSubsystem();
	#endif

	input_subsystem->initKeyboard(0);

	I_OpenMouse();

	I_OpenJoystick();

	I_DisableKeyRepeat();

	I_ForceUpdateGrab();

	input_subsystem->enableTextEntry();

	return true;
}


//
// I_ShutdownInput
//
void STACK_ARGS I_ShutdownInput()
{
	input_subsystem->disableTextEntry();

	I_UngrabInput();

	delete input_subsystem;
	input_subsystem = NULL;
}


//
// I_GetEvents
//
// Checks for new input events and posts them to the Doom event queue.
//
void I_GetEvents(bool mouseOnly)
{
	if (mouseOnly)
	{
		// D_PostEvent will process mouse events immediately
		input_subsystem->gatherMouseEvents();
	}
	else
	{
		static bool previously_focused = false;
		bool currently_focused = I_GetWindow()->isFocused();
		if (currently_focused && !previously_focused)
			I_FlushInput();
		previously_focused = currently_focused;

		I_UpdateGrab();
		if (I_CanRepeat())
			I_EnableKeyRepeat();
		else
			I_DisableKeyRepeat();

		// Get all of the events from the keboard, mouse, and joystick
		input_subsystem->gatherEvents();
	}

	event_t ev;
	while (input_subsystem->hasEvent())
	{
		input_subsystem->getEvent(&ev);
		D_PostEvent(&ev);
	}
}


//
// I_StartTic
//
void I_StartTic (void)
{
	I_GetEvents(false);
}



// ============================================================================
//
// IInputSubsystem default implementation
//
// ============================================================================

// Initialize member constants
// Key repeat delay and interval times are the default values for SDL 1.2.15
const uint64_t IInputSubsystem::mRepeatDelay = I_ConvertTimeFromMs(500);
const uint64_t IInputSubsystem::mRepeatInterval = I_ConvertTimeFromMs(30);


//
// IInputSubsystem::IInputSubsystem
//
IInputSubsystem::IInputSubsystem() :
	mKeyboardInputDevice(NULL), mMouseInputDevice(NULL), mJoystickInputDevice(NULL)
{ }


//
// IInputSubsystem::~IInputSubsystem
//
IInputSubsystem::~IInputSubsystem()
{

}


//
// IInputSubsystem::registerInputDevice
//
void IInputSubsystem::registerInputDevice(IInputDevice* device)
{
	assert(device != NULL);
	InputDeviceList::iterator it = std::find(mInputDevices.begin(), mInputDevices.end(), device);
	assert(it == mInputDevices.end());
	if (it == mInputDevices.end())
		mInputDevices.push_back(device);
}


//
// IInputSubsystem::unregisterInputDevice
//
void IInputSubsystem::unregisterInputDevice(IInputDevice* device)
{
	assert(device != NULL);
	InputDeviceList::iterator it = std::find(mInputDevices.begin(), mInputDevices.end(), device);
	assert(it != mInputDevices.end());
	if (it != mInputDevices.end())
		mInputDevices.erase(it);
}


//
// IInputSubsystem::enableKeyRepeat
//
void IInputSubsystem::enableKeyRepeat()
{
	mRepeating = true;
}


//
// IInputSubsystem::disableKeyRepeat
//
void IInputSubsystem::disableKeyRepeat()
{
	mRepeating = false;
	mEventRepeaters.clear();
}


//
// IInputSubsystem::enableTextEntry
//
void IInputSubsystem::enableTextEntry()
{
	IKeyboardInputDevice* device = static_cast<IKeyboardInputDevice*>(getKeyboardInputDevice());
	if (device)
		device->enableTextEntry();
}


//
// IInputSubsystem::disableTextEntry
//
void IInputSubsystem::disableTextEntry()
{
	IKeyboardInputDevice* device = static_cast<IKeyboardInputDevice*>(getKeyboardInputDevice());
	if (device)
		device->disableTextEntry();
}


//
// I_GetEventRepeaterKey
//
// Returns a value for use as a hash table key from a given key-press event.
//
// If the function returns 0, the event should not be repeated. This is the
// case for non-key-press events and for special buttons on the keyboard such
// as Ctrl or CapsLock.
//
// All other keyboard events should return a key value of 1. Typically
// key-repeating only allows one key on the keyboard to be repeating at the
// same time. This can be accomplished by all keyboard events returning
// the same value.
//
// Joystick hat events also repeat but each directional trigger repeats
// concurrently as long as they are held down. Thus a unique value is returned
// for each of them.
// 
static int I_GetEventRepeaterKey(const event_t* ev)
{
	if (ev->type != ev_keydown && ev->type != ev_keyup)
		return 0;

	int button = ev->data1;
	if (button == OKEY_CAPSLOCK || button == OKEY_SCRLCK ||
		button == OKEY_LSHIFT || button == OKEY_LCTRL || button == OKEY_LALT ||
		button == OKEY_RSHIFT || button == OKEY_RCTRL || button == OKEY_RALT ||
		button == OKEY_NUMLOCK)
		return 0;
	else if (button >= OKEY_HAT1 && button <= OKEY_HAT8)
		return button;
	else
		return 1;
}


//
// IInputSubsystem::addToEventRepeaters
//
// NOTE: the caller should check if key-repeating is enabled.
//
void IInputSubsystem::addToEventRepeaters(event_t& ev)
{
	// Check if the event needs to be added/removed from the list of repeatable events
	int key = I_GetEventRepeaterKey(&ev);
	if (ev.type == ev_keydown && key)
	{
		// If there is an existing repeater event for "key",
		// remove it and replace it with a new one.
		EventRepeaterTable::iterator it = mEventRepeaters.find(key);
		if (it != mEventRepeaters.end())
			mEventRepeaters.erase(it);

		// new repeatable event - add to mEventRepeaters
		EventRepeater repeater;
		repeater.event = ev;
		repeater.repeating = false;		// start off waiting for mRepeatDelay before repeating
		repeater.last_time = I_GetTime();
		mEventRepeaters.insert(std::make_pair(key, repeater));
	}
	else if (ev.type == ev_keyup && key)
	{
		// remove the repeatable event from mEventRepeaters
		EventRepeaterTable::iterator it = mEventRepeaters.find(key);
		if (it != mEventRepeaters.end())
			mEventRepeaters.erase(it);
	}
}


//
// IInputSubsystem::repeatEvents
//
// NOTE: the caller should check if key-repeating is enabled.
//
void IInputSubsystem::repeatEvents()
{
	for (EventRepeaterTable::iterator it = mEventRepeaters.begin(); it != mEventRepeaters.end(); ++it)
	{
		EventRepeater& repeater = it->second;
		uint64_t current_time = I_GetTime();

		if (!repeater.repeating && current_time - repeater.last_time >= mRepeatDelay)
		{
			repeater.last_time += mRepeatDelay;
			repeater.repeating = true;
		}

		while (repeater.repeating && current_time - repeater.last_time >= mRepeatInterval)
		{
			// repeat the event by adding it to the queue again
			mEvents.push(repeater.event);
			repeater.last_time += mRepeatInterval;
		}
	}
}


//
// IInputSubsystem::gatherEvents
//
void IInputSubsystem::gatherEvents()
{
	event_t ev;
	for (InputDeviceList::iterator it = mInputDevices.begin(); it != mInputDevices.end(); ++it)
	{
		IInputDevice* device = *it;
		device->gatherEvents();
		while (device->hasEvent())
		{
			device->getEvent(&ev);

			if (mRepeating)
				addToEventRepeaters(ev);

			mEvents.push(ev);
		}
	}

	// Handle repeatable events
	if (mRepeating)
		repeatEvents();
}

void IInputSubsystem::gatherMouseEvents()
{
	event_t mouseEvent;
	if (mMouseInputDevice != NULL)
		mMouseInputDevice->gatherEvents();

	while (mMouseInputDevice->hasEvent())
	{
		mMouseInputDevice->getEvent(&mouseEvent);
		mEvents.push(mouseEvent);
	}
}


//
// IInputSubsystem::getEvent
//
void IInputSubsystem::getEvent(event_t* ev)
{
	assert(hasEvent());
	*ev = mEvents.front();
	mEvents.pop();
}


VERSION_CONTROL (i_input_cpp, "$Id$")
