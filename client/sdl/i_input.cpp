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
//	SDL input handler
//
//-----------------------------------------------------------------------------

// SoM 12-24-05: yeah... I'm programming on christmas eve.
// Removed all the DirectX crap.

#include <stdlib.h>
#include <list>
#include <sstream>

#include <SDL.h>
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

#ifdef _XBOX
#include "i_xbox.h"
#endif

#ifdef _WIN32
#include <SDL_syswm.h>
#include "i_win32input.h"
bool tab_keydown = false;	// [ML] Actual status of tab key
#endif

#define JOY_DEADZONE 6000

EXTERN_CVAR (vid_fullscreen)
EXTERN_CVAR (vid_defwidth)
EXTERN_CVAR (vid_defheight)

static int mouse_driver_id = -1;
static IInputDevice* mouse_input = NULL;

static IInputSubsystem* input_subsystem = NULL;

static bool window_focused = false;
static bool input_grabbed = false;
static bool nomouse = false;
extern bool configuring_controls;

EXTERN_CVAR (use_joystick)
EXTERN_CVAR (joy_active)

extern constate_e ConsoleState;


//
// I_FlushInput
//
// Eat all pending input from outside the game
//
void I_FlushInput()
{
	SDL_Event ev;

	I_DisableKeyRepeat();

	while (SDL_PollEvent(&ev)) {}

	C_ReleaseKeys();

	I_EnableKeyRepeat();

	input_subsystem->flushInput();
}


//
// I_EnableKeyRepeat
//
// Enables key repeat for text entry (console, menu system, and chat).
//
void I_EnableKeyRepeat()
{
	input_subsystem->enableKeyRepeat();
}


//
// I_DisableKeyRepeat
//
// Disables key repeat for standard game control.
//
void I_DisableKeyRepeat()
{
	input_subsystem->disableKeyRepeat();
}


//
// I_CheckFocusState
//
// Determines if the Odamex window currently has the window manager focus.
// We should have input (keyboard) focus and be visible (not minimized).
//
static bool I_CheckFocusState()
{
	SDL_PumpEvents();
	Uint8 state = SDL_GetAppState();
	return (state & SDL_APPACTIVE) && (state & SDL_APPINPUTFOCUS);
}

//
// I_UpdateFocus
//
// Update the value of window_focused each tic and in response to SDL
// window manager events.
//
// We try to make ourselves be well-behaved: the grab on the mouse
// is removed if we lose focus (such as a popup window appearing),
// and we dont move the mouse around if we aren't focused either.
// [ML] 4-2-14: Make more in line with EE and choco, handle alt+tab focus better
//
static void I_UpdateFocus()
{
	SDL_Event  ev;
	bool new_window_focused = I_CheckFocusState();

	// [CG][EE] Handle focus changes, this is all necessary to avoid repeat events.
	if (window_focused != new_window_focused)
	{
		if(new_window_focused)
		{
			while(SDL_PollEvent(&ev)) {}
			I_EnableKeyRepeat();
		}
		else
		{
			I_DisableKeyRepeat();
		}

#ifdef _WIN32
		tab_keydown = false;
#endif
		C_ReleaseKeys();

		window_focused = new_window_focused;

		input_subsystem->flushInput();
	}
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

	if (!window_focused)
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
	SDL_WM_GrabInput(SDL_GRAB_ON);
	input_grabbed = true;
	I_ResumeMouse();
}

//
// I_UngrabInput
//
static void I_UngrabInput()
{
	SDL_WM_GrabInput(SDL_GRAB_OFF);
	input_grabbed = false;
	I_PauseMouse();
}


//
// I_ForceUpdateGrab
//
// Determines if SDL should grab the mouse based on the game window having
// focus and the status of the menu and console.
//
// Should be called whenever the video mode changes.
//
void I_ForceUpdateGrab()
{
	window_focused = I_CheckFocusState();

	if (I_CanGrab())
		I_GrabInput();
	else
		I_UngrabInput();
}


//
// I_UpdateGrab
//
// Determines if SDL should grab the mouse based on the game window having
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


//
// I_InitFocus
//
// Sets the initial value of window_focused.
//
static void I_InitFocus()
{
	I_ForceUpdateGrab();
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

//
// I_InitInput
//
bool I_InitInput()
{
	if (Args.CheckParm("-nomouse"))
		nomouse = true;

	atterm(I_ShutdownInput);

	input_subsystem = new ISDL12InputSubsystem();
	input_subsystem->initKeyboard(0);
	if (!nomouse)
		input_subsystem->initMouse(0);
	I_OpenJoystick();

	I_DisableKeyRepeat();

	I_InitFocus();

	return true;
}

//
// I_ShutdownInput
//
void STACK_ARGS I_ShutdownInput()
{
	I_PauseMouse();

	I_UngrabInput();

	input_subsystem->shutdownJoystick(0);
	input_subsystem->shutdownMouse(0);
	input_subsystem->shutdownKeyboard(0);

	delete input_subsystem;
	input_subsystem = NULL;
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
// I_GetEvent
//
// Pumps SDL for new input events and posts them to the Doom event queue.
// Keyboard and joystick events are retreived directly from SDL while mouse
// movement and buttons are handled by the MouseInput class.
//
void I_GetEvent()
{
	I_UpdateFocus();
	I_UpdateGrab();

	input_subsystem->gatherEvents();
	while (input_subsystem->hasEvent())
	{
		event_t ev;
		input_subsystem->getEvent(&ev);
		D_PostEvent(&ev);
	}

	// [SL] Get application window events
	// TODO: move this to the video subsystem
	//
	const int MAX_EVENTS = 256;
	static SDL_Event sdl_events[MAX_EVENTS];

	// set mask to get all events except keyboard, mouse, and joystick events
	const int event_mask = SDL_ALLEVENTS & ~SDL_KEYEVENTMASK & ~SDL_MOUSEEVENTMASK & ~SDL_JOYEVENTMASK;

	// Force SDL to gather events from input devices. This is called
	// implicitly from SDL_PollEvent but since we're using SDL_PeepEvents to
	// process only non-mouse events, SDL_PumpEvents is necessary.
	SDL_PumpEvents();
	int num_events = SDL_PeepEvents(sdl_events, MAX_EVENTS, SDL_GETEVENT, event_mask);

	for (int i = 0; i < num_events; i++)
	{
		event_t event;
		event.data1 = event.data2 = event.data3 = 0;

		SDL_Event* sdl_ev = &sdl_events[i];
		switch (sdl_ev->type)
		{
		case SDL_QUIT:
			AddCommandString("quit");
			break;

		case SDL_VIDEORESIZE:
		{
			// Resizable window mode resolutions
			if (!vid_fullscreen)
			{
				std::stringstream Command;
				Command << "vid_setmode " << sdl_ev->resize.w << " " << sdl_ev->resize.h;
				AddCommandString(Command.str());

				vid_defwidth.Set((float)sdl_ev->resize.w);
				vid_defheight.Set((float)sdl_ev->resize.h);
			}
			break;
		}

		case SDL_ACTIVEEVENT:
			// need to update our focus state
			I_UpdateFocus();
			I_UpdateGrab();
			// pause the mouse when the focus goes away (eg, alt-tab)
			if (!window_focused)
				I_PauseMouse();
			break;
		};
	}
}

//
// I_StartTic
//
void I_StartTic (void)
{
	I_GetEvent();
}

//
// I_StartFrame
//
void I_StartFrame (void)
{
}

// ============================================================================
//
// Mouse Drivers
//
// ============================================================================

static bool I_SDLMouseAvailible();
static bool I_MouseUnavailible();
#ifdef USE_RAW_WIN32_MOUSE
static bool I_RawWin32MouseAvailible();
#endif	// USE_RAW_WIN32_MOUSE

static IInputDevice* I_CreateSDLMouse()
{
	return new ISDL12MouseInputDevice(0);
}

#ifdef USE_RAW_WIN32_MOUSE
static IInputDevice* I_CreateRawWin32Mouse()
{
	return new IRawWin32MouseInputDevice(0);
}
#endif

MouseDriverInfo_t MouseDriverInfo[] = {
	{ SDL_MOUSE_DRIVER,			"SDL Mouse",	&I_SDLMouseAvailible,		&I_CreateSDLMouse },
#ifdef USE_RAW_WIN32_MOUSE
	{ RAW_WIN32_MOUSE_DRIVER,	"Raw Input",	&I_RawWin32MouseAvailible,	&I_CreateRawWin32Mouse }
#else
	{ RAW_WIN32_MOUSE_DRIVER,	"Raw Input",	&I_MouseUnavailible,	NULL }
#endif	// USE_WIN32_MOUSE
};


//
// I_FindMouseDriverInfo
//
MouseDriverInfo_t* I_FindMouseDriverInfo(int id)
{
	for (int i = 0; i < NUM_MOUSE_DRIVERS; i++)
	{
		if (MouseDriverInfo[i].id == id)
			return &MouseDriverInfo[i];
	}

	return NULL;
}

//
// I_IsMouseDriverValid
//
// Returns whether a mouse driver with the given ID is availible to use.
//
static bool I_IsMouseDriverValid(int id)
{
	MouseDriverInfo_t* info = I_FindMouseDriverInfo(id);
	return (info && info->avail_test() == true);
}

CVAR_FUNC_IMPL(mouse_driver)
{
	if (!I_IsMouseDriverValid(var))
	{
		if (var.asInt() == SDL_MOUSE_DRIVER)
		{
			// can't initialize SDL_MOUSE_DRIVER so don't use a mouse
			I_ShutdownMouseDriver();
			nomouse = true;
		}
		else
		{
			var.Set(SDL_MOUSE_DRIVER);
		}
	}
	else
	{
		if (var.asInt() != mouse_driver_id)
		{
			mouse_driver_id = var.asInt();
			I_InitMouseDriver();
		}
	}
}

//
// I_ShutdownMouseDriver
//
// Frees the memory used by mouse_input
//
void I_ShutdownMouseDriver()
{
	delete mouse_input;
	mouse_input = NULL;
}

static void I_SetSDLIgnoreMouseEvents()
{
	SDL_EventState(SDL_MOUSEMOTION, SDL_IGNORE);
	SDL_EventState(SDL_MOUSEBUTTONDOWN, SDL_IGNORE);
	SDL_EventState(SDL_MOUSEBUTTONUP, SDL_IGNORE);
}

static void I_UnsetSDLIgnoreMouseEvents()
{
	SDL_EventState(SDL_MOUSEMOTION, SDL_ENABLE);
	SDL_EventState(SDL_MOUSEBUTTONDOWN, SDL_ENABLE);
	SDL_EventState(SDL_MOUSEBUTTONUP, SDL_ENABLE);
}

//
// I_InitMouseDriver
//
// Instantiates the proper concrete MouseInput object based on the
// mouse_driver cvar and stores a pointer to the object in mouse_input.
//
void I_InitMouseDriver()
{
	I_ShutdownMouseDriver();

	// ignore SDL mouse input for now... The mouse driver will change this if needed
	I_SetSDLIgnoreMouseEvents();

	if (nomouse)
		return;

	// try to initialize the user's preferred mouse driver
	MouseDriverInfo_t* info = I_FindMouseDriverInfo(mouse_driver_id);
	if (info)
	{
		if (info->create != NULL)
			mouse_input = info->create();
		if (mouse_input != NULL)
			Printf(PRINT_HIGH, "I_InitMouseDriver: Initializing %s input.\n", info->name);
		else
			Printf(PRINT_HIGH, "I_InitMouseDriver: Unable to initalize %s input.\n", info->name);
	}

	// fall back on SDLMouse if the preferred driver failed to initialize
	if (mouse_input == NULL)
	{
		mouse_input = I_CreateSDLMouse();
		if (mouse_input != NULL)
			Printf(PRINT_HIGH, "I_InitMouseDriver: Initializing SDL Mouse input as a fallback.\n");
		else
			Printf(PRINT_HIGH, "I_InitMouseDriver: Unable to initialize SDL Mouse input as a fallback.\n");
	}

	I_FlushInput();
	I_ResumeMouse();
}

//
// I_CheckForProc
//
// Checks if a function with the given name is in the given DLL file.
// This is used to determine if the user's version of Windows has the necessary
// functions availible.
//
#if defined(_WIN32) && !defined(_XBOX)
static bool I_CheckForProc(const char* dllname, const char* procname)
{
	bool avail = false;
	HMODULE dll = LoadLibrary(TEXT(dllname));
	if (dll)
	{
		avail = (GetProcAddress(dll, procname) != NULL);
		FreeLibrary(dll);
	}
	return avail;
}
#endif  // WIN32

//
// I_RawWin32MouseAvailible
//
// Checks if the raw input mouse functions that the RawWin32Mouse
// class calls are availible on the current system. They require
// Windows XP or higher.
//
#ifdef USE_RAW_WIN32_MOUSE
static bool I_RawWin32MouseAvailible()
{
	return	I_CheckForProc("user32.dll", "RegisterRawInputDevices") &&
			I_CheckForProc("user32.dll", "GetRegisteredRawInputDevices") &&
			I_CheckForProc("user32.dll", "GetRawInputData");
}
#endif  // USE_RAW_WIN32_MOUSE

//
// I_SDLMouseAvailible
//
// Checks if SDLMouse can be used. Always true since SDL is used as the
// primary backend for everything.
//
static bool I_SDLMouseAvailible()
{
	return true;
}

//
// I_MouseUnavailible
//
// Generic function to indicate that a particular mouse driver is not availible
// on this platform.
//
static bool I_MouseUnavailible()
{
	return false;
}



// ============================================================================
//
// IInputSubsystem default implementation
//
// ============================================================================

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
// I_IsEventRepeatable
//
// Returns true if an event should be repeated when the key is held down for
// a certain length of time. The list of keys to not repeat was based on
// what SDL_keyboard.c does.
//
static bool I_IsEventRepeatable(const event_t* ev)
{
	if (ev->type != ev_keydown)
		return false;
	
	int button = ev->data1;
	return button != 0 && button != KEY_CAPSLOCK && button != KEY_SCRLCK &&
		button != KEY_LSHIFT && button != KEY_LCTRL && button != KEY_LALT &&
		button != KEY_RSHIFT && button != KEY_RCTRL && button != KEY_RALT &&
		!(button >= KEY_MOUSE1 && button <= KEY_MOUSE5) &&
		!(button >= KEY_JOY1 && button <= KEY_JOY32);
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
	else
	{
		return 0;
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

			// Check if the event needs to be added/removed from the list of repeatable events
			if (mRepeating)
			{
				int key = I_GetEventRepeaterKey(&ev); 
				if (key && ev.type == ev_keydown)
				{
					EventRepeaterTable::iterator it = mEventRepeaters.find(key);
					if (it != mEventRepeaters.end())
					{
						// update existing repeater with this new event
						EventRepeater& repeater = it->second;
						memcpy(&repeater.event, &ev, sizeof(repeater.event));
					}
					else
					{
						// new repeatable event - add to mEventRepeaters
						EventRepeater repeater;
						memcpy(&repeater.event, &ev, sizeof(repeater.event));
						repeater.last_time = I_GetTime();
						repeater.repeating = false;		// start off waiting for mRepeatDelay before repeating
						mEventRepeaters.insert(std::make_pair(key, repeater));
					}
				}
				else if (key && ev.type == ev_keyup)
				{
					EventRepeaterTable::iterator it = mEventRepeaters.find(key);
					if (it != mEventRepeaters.end())
					{
						// remove the repeatable event from mEventRepeaters
						const EventRepeater& repeater = it->second;
						if (repeater.event.data1 == ev.data1)
							mEventRepeaters.erase(it);
					}
				}
			}

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
	{
		for (EventRepeaterTable::iterator it = mEventRepeaters.begin(); it != mEventRepeaters.end(); ++it)
		{
			EventRepeater& repeater = it->second;
			assert(I_IsEventRepeatable(&repeater.event));
			uint64_t current_time = I_GetTime();
			uint64_t delta_time = current_time - repeater.last_time;

			if (!repeater.repeating && current_time - repeater.last_time >= mRepeatDelay)
			{
				repeater.last_time += mRepeatDelay;
				repeater.repeating = true;
			}

			while (repeater.repeating && current_time - repeater.last_time >= mRepeatInterval)
			{
				// repeat the event by adding it  to the queue again
				mEvents.push(repeater.event);
				repeater.last_time += mRepeatInterval;
			}
		}
	}
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
