// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2006-2015 by The Odamex Team.
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

// SoM 12-24-05: yeah... I'm programming on christmas eve.
// Removed all the DirectX crap.

#include <stdlib.h>
#include <list>
#include <sstream>

#include "win32inc.h"

#include "doomstat.h"
#include "m_argv.h"
#include "i_input.h"
#include "i_sdlinput.h"
#include "i_video.h"
#include "d_main.h"
#include "c_bind.h"
#include "c_console.h"
#include "c_cvars.h"
#include "i_system.h"
#include "c_dispatch.h"
#include "hu_stuff.h"

#ifdef _XBOX
#include "i_xbox.h"
#endif

#ifdef _WIN32
bool tab_keydown = false;	// [ML] Actual status of tab key
#endif

#define JOY_DEADZONE 6000

static IInputSubsystem* input_subsystem = NULL;

static bool window_focused = false;
static bool input_grabbed = false;
static bool nomouse = false;

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
	return ConsoleState == c_down || chat.GetStatus() != HUDChat::INACTIVE || menuactive;
}


//
// I_UpdateFocus
//
// Update the value of window_focused each tic and in response to
// window manager events.
//
// We try to make ourselves be well-behaved: the grab on the mouse
// is removed if we lose focus (such as a popup window appearing),
// and we dont move the mouse around if we aren't focused either.
// [ML] 4-2-14: Make more in line with EE and choco, handle alt+tab focus better
//
static void I_UpdateFocus()
{
	bool new_window_focused = I_GetWindow()->isFocused();

	// [CG][EE] Handle focus changes, this is all necessary to avoid repeat events.
	if (window_focused != new_window_focused)
		I_FlushInput();

	window_focused = new_window_focused;
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

	if (!I_GetWindow()->isFocused())
		return false;
	else if (I_GetWindow()->isFullScreen())
		return true;
	else if (nomouse)
		return false;
	else if (configuring_controls)
		return true;
	else if (menuactive || ConsoleState == c_down || paused)
		return false;
	else if ((gamestate == GS_LEVEL || gamestate == GS_INTERMISSION) && !demoplayback)
		return true;
	else
		return false;
}


//
// I_GrabInput
//
static void I_GrabInput()
{
	input_subsystem->grabInput();
	input_grabbed = true;
	I_ResumeMouse();
}

//
// I_UngrabInput
//
static void I_UngrabInput()
{
	input_subsystem->releaseInput();
	input_grabbed = false;
	I_PauseMouse();
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
	window_focused = I_GetWindow()->isFocused();

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
	if (!input_grabbed && I_CanGrab())
		I_GrabInput();
	else if (input_grabbed && !I_CanGrab())
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

bool I_OpenMouse();
void I_CloseMouse();

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
// I_PauseMouse
//
// Enables the mouse cursor and prevents the game from processing mouse movement
// or button events
//
void I_PauseMouse()
{
	input_subsystem->pauseMouse();
}


//
// I_ResumeMouse
//
// Disables the mouse cursor and allows the game to process mouse movement
// or button events
//
void I_ResumeMouse()
{
	input_subsystem->resumeMouse();
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

	I_PauseMouse();

	I_UngrabInput();

	delete input_subsystem;
	input_subsystem = NULL;
}


//
// I_GetEvents
//
// Checks for new input events and posts them to the Doom event queue.
//
static void I_GetEvents()
{
	I_UpdateFocus();
	I_UpdateGrab();
	if (I_CanRepeat())
		I_EnableKeyRepeat();
	else
		I_DisableKeyRepeat();

	// Get all of the events from the keboard, mouse, and joystick
	input_subsystem->gatherEvents();
	while (input_subsystem->hasEvent())
	{
		event_t ev;
		input_subsystem->getEvent(&ev);
		D_PostEvent(&ev);
	}
}

//
// I_StartTic
//
void I_StartTic (void)
{
	I_GetEvents();
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
	if (button < KEY_MOUSE1)
	{
		if (button == KEY_CAPSLOCK || button == KEY_SCRLCK ||
			button == KEY_LSHIFT || button == KEY_LCTRL || button == KEY_LALT ||
			button == KEY_RSHIFT || button == KEY_RCTRL || button == KEY_RALT)
			return 0;
		return 1;
	}
	else if (button >= KEY_HAT1 && button <= KEY_HAT8)
	{
		return button;
	}

	return 0;
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
	event_t mouse_motion_event;
	mouse_motion_event.type = ev_mouse;
	mouse_motion_event.data1 = mouse_motion_event.data2 = mouse_motion_event.data3 = 0;

	for (InputDeviceList::iterator it = mInputDevices.begin(); it != mInputDevices.end(); ++it)
	{
		IInputDevice* device = *it;
		device->gatherEvents();
		while (device->hasEvent())
		{
			event_t ev;
			device->getEvent(&ev);

			if (mRepeating)
				addToEventRepeaters(ev);

			if (ev.type == ev_mouse)
			{
				// aggregate all mouse motion into a single event, which is enqueued later
				mouse_motion_event.data2 += ev.data2;
				mouse_motion_event.data3 += ev.data3;
			}
			else
			{
				// default behavior for events: just add it to the queue
				mEvents.push(ev);
			}
		}
	}
	
	// manually add the aggregated mouse motion event to the queue
	if (mouse_motion_event.data2 || mouse_motion_event.data3)
		mEvents.push(mouse_motion_event);

	// Handle repeatable events
	if (mRepeating)
		repeatEvents();
}


//
// IInputSubsystem::getEvent
//
void IInputSubsystem::getEvent(event_t* ev)
{
	assert(hasEvent());

	memcpy(ev, &mEvents.front(), sizeof(event_t));

	mEvents.pop();
}


VERSION_CONTROL (i_input_cpp, "$Id$")
