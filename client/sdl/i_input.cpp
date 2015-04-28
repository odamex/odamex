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
static ISDL12KeyboardInputDevice* sdl_keyboard_input_device = NULL;

static bool window_focused = false;
static bool input_grabbed = false;
static bool nomouse = false;
extern bool configuring_controls;

EXTERN_CVAR (use_joystick)
EXTERN_CVAR (joy_active)

typedef struct
{
	SDL_Event	Event;
	unsigned int RegTick;
	unsigned int LastTick;
} JoystickEvent_t;

static SDL_Joystick *openedjoy = NULL;
static std::list<JoystickEvent_t*> JoyEventList;

// denis - from chocolate doom
//
// Mouse acceleration
//
// This emulates some of the behavior of DOS mouse drivers by increasing
// the speed when the mouse is moved fast.
//
// The mouse input values are input directly to the game, but when
// the values exceed the value of mouse_threshold, they are multiplied
// by mouse_acceleration to increase the speed.
EXTERN_CVAR (mouse_acceleration)
EXTERN_CVAR (mouse_threshold)

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

	if (mouse_input)
		mouse_input->flushEvents();
}

void I_EnableKeyRepeat()
{
	SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY / 2, SDL_DEFAULT_REPEAT_INTERVAL);
}

void I_DisableKeyRepeat()
{
	SDL_EnableKeyRepeat(0, 0);
}

void I_ResetKeyRepeat()
{
	SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY, SDL_DEFAULT_REPEAT_INTERVAL);
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

		if (mouse_input)
			mouse_input->flushEvents();
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


// Add any joystick event to a list if it will require manual polling
// to detect release. This includes hat events (mostly due to d-pads not
// triggering the centered event when released) and analog axis bound
// as a key/button -- HyperEye
//
// RegisterJoystickEvent
//
static int RegisterJoystickEvent(SDL_Event *ev, int value)
{
	JoystickEvent_t *evc = NULL;
	event_t		  event;

	if(!ev)
		return -1;

	if(ev->type == SDL_JOYHATMOTION)
	{
		if(!JoyEventList.empty())
		{
			std::list<JoystickEvent_t*>::iterator i;

			for(i = JoyEventList.begin(); i != JoyEventList.end(); ++i)
			{
				if(((*i)->Event.type == ev->type) && ((*i)->Event.jhat.which == ev->jhat.which)
							&& ((*i)->Event.jhat.hat == ev->jhat.hat) && ((*i)->Event.jhat.value == value))
					return 0;
			}
		}

		evc = new JoystickEvent_t;

		memcpy(&evc->Event, ev, sizeof(SDL_Event));
		evc->Event.jhat.value = value;
		evc->LastTick = evc->RegTick = SDL_GetTicks();

		event.data1 = event.data2 = event.data3 = 0;

		event.type = ev_keydown;
		if(value == SDL_HAT_UP)
			event.data1 = (ev->jhat.hat * 4) + KEY_HAT1;
		else if(value == SDL_HAT_RIGHT)
			event.data1 = (ev->jhat.hat * 4) + KEY_HAT2;
		else if(value == SDL_HAT_DOWN)
			event.data1 = (ev->jhat.hat * 4) + KEY_HAT3;
		else if(value == SDL_HAT_LEFT)
			event.data1 = (ev->jhat.hat * 4) + KEY_HAT4;

		event.data2 = event.data1;
	}

	if(evc)
	{
		JoyEventList.push_back(evc);
		D_PostEvent(&event);
		return 1;
	}

	return 0;
}

void UpdateJoystickEvents()
{
	std::list<JoystickEvent_t*>::iterator i;
	event_t	event;

	if(JoyEventList.empty())
		return;

	i = JoyEventList.begin();
	while(i != JoyEventList.end())
	{
		if((*i)->Event.type == SDL_JOYHATMOTION)
		{
			// Hat position released
			if(!(SDL_JoystickGetHat(openedjoy, (*i)->Event.jhat.hat) & (*i)->Event.jhat.value))
				event.type = ev_keyup;
			// Hat button still held - Repeat at key repeat interval
			else if((SDL_GetTicks() - (*i)->RegTick >= SDL_DEFAULT_REPEAT_DELAY) &&
					(SDL_GetTicks() - (*i)->LastTick >= SDL_DEFAULT_REPEAT_INTERVAL*2))
			{
				(*i)->LastTick = SDL_GetTicks();
				event.type = ev_keydown;
			}
			else
			{
				++i;
				continue;
			}

			event.data1 = event.data2 = event.data3 = 0;

			if((*i)->Event.jhat.value == SDL_HAT_UP)
				event.data1 = ((*i)->Event.jhat.hat * 4) + KEY_HAT1;
			else if((*i)->Event.jhat.value == SDL_HAT_RIGHT)
				event.data1 = ((*i)->Event.jhat.hat * 4) + KEY_HAT2;
			else if((*i)->Event.jhat.value == SDL_HAT_DOWN)
				event.data1 = ((*i)->Event.jhat.hat * 4) + KEY_HAT3;
			else if((*i)->Event.jhat.value == SDL_HAT_LEFT)
				event.data1 = ((*i)->Event.jhat.hat * 4) + KEY_HAT4;

			D_PostEvent(&event);

			if(event.type == ev_keyup)
			{
				// Delete the released event
				delete *i;
				i = JoyEventList.erase(i);
				continue;
			}
		}
		++i;
	}
}

// This turns on automatic event polling for joysticks so that the state
// of each button and axis doesn't need to be manually queried each tick. -- Hyper_Eye
//
// EnableJoystickPolling
//
static int EnableJoystickPolling()
{
	return SDL_JoystickEventState(SDL_ENABLE);
}

static int DisableJoystickPolling()
{
	return SDL_JoystickEventState(SDL_IGNORE);
}

CVAR_FUNC_IMPL (use_joystick)
{
	if(var <= 0.0)
	{
		// Don't let console users disable joystick support because
		// they won't have any way to reenable through the menu.
#ifdef GCONSOLE
		use_joystick = 1.0;
#else
		I_CloseJoystick();
		DisableJoystickPolling();
#endif
	}
	else
	{
		I_OpenJoystick();
		EnableJoystickPolling();
	}
}


CVAR_FUNC_IMPL (joy_active)
{
	if( (var < 0.0) || ((int)var > I_GetJoystickCount()) )
		var = 0.0;

	I_CloseJoystick();

	I_OpenJoystick();
}

//
// I_GetJoystickCount
//
int I_GetJoystickCount()
{
	return SDL_NumJoysticks();
}

//
// I_GetJoystickNameFromIndex
//
std::string I_GetJoystickNameFromIndex (int index)
{
	const char  *joyname = NULL;
	std::string  ret;

	joyname = SDL_JoystickName(index);

	if(!joyname)
		return "";

	ret = joyname;

	return ret;
}

//
// I_OpenJoystick
//
bool I_OpenJoystick()
{
	int numjoy;

	numjoy = I_GetJoystickCount();

	if(!numjoy || !use_joystick)
		return false;

	if((int)joy_active > numjoy)
		joy_active.Set(0.0);

	if(!SDL_JoystickOpened(joy_active))
		openedjoy = SDL_JoystickOpen(joy_active);

	if(!SDL_JoystickOpened(joy_active))
		return false;

	return true;
}

//
// I_CloseJoystick
//
void I_CloseJoystick()
{
	extern int joyforward, joystrafe, joyturn, joylook;
	int		ndx;

#ifndef _XBOX // This is to avoid a bug in SDLx
	if(!I_GetJoystickCount() || !openedjoy)
		return;

	ndx = SDL_JoystickIndex(openedjoy);

	if(SDL_JoystickOpened(ndx))
		SDL_JoystickClose(openedjoy);

	openedjoy = NULL;
#endif

	// Reset joy position values. Wouldn't want to get stuck in a turn or something. -- Hyper_Eye
	joyforward = joystrafe = joyturn = joylook = 0;
}

//
// I_InitInput
//
bool I_InitInput (void)
{
	if (Args.CheckParm("-nomouse"))
		nomouse = true;

	if (sdl_keyboard_input_device == NULL)
		sdl_keyboard_input_device = new ISDL12KeyboardInputDevice(0);

	atterm(I_ShutdownInput);

	I_DisableKeyRepeat();

	// Initialize the joystick subsystem and open a joystick if use_joystick is enabled. -- Hyper_Eye
	Printf(PRINT_HIGH, "I_InitInput: Initializing SDL's joystick subsystem.\n");
	SDL_InitSubSystem(SDL_INIT_JOYSTICK);

	if((int)use_joystick && I_GetJoystickCount())
	{
		I_OpenJoystick();
		EnableJoystickPolling();
	}

#ifdef _WIN32
	// denis - in fullscreen, prevent exit on accidental windows key press
	// [Russell] - Disabled because it screws with the mouse
	//g_hKeyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL,  LowLevelKeyboardProc, GetModuleHandle(NULL), 0);
#endif

	I_InitFocus();

	// [SL] do not intialize mouse driver here since it will be called from
	// the mouse_driver CVAR callback

	return true;
}

//
// I_ShutdownInput
//
void STACK_ARGS I_ShutdownInput (void)
{
	I_PauseMouse();

	I_ShutdownMouseDriver();

	I_UngrabInput();
	I_ResetKeyRepeat();

	delete sdl_keyboard_input_device;
	sdl_keyboard_input_device = NULL;
}

//
// I_PauseMouse
//
// Enables the mouse cursor and prevents the game from processing mouse movement
// or button events
//
void I_PauseMouse()
{
	if (mouse_input)
		mouse_input->pause();
}

//
// I_ResumeMouse
//
// Disables the mouse cursor and allows the game to process mouse movement
// or button events
//
void I_ResumeMouse()
{
	if (mouse_input)
		mouse_input->resume();
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
	const int MAX_EVENTS = 256;
	static SDL_Event sdl_events[MAX_EVENTS];
	event_t event;

	I_UpdateFocus();
	I_UpdateGrab();

	// Process mouse movement and button events
	if (mouse_input)
	{
		mouse_input->gatherEvents();
		while (mouse_input->hasEvent())
		{
			event_t event;
			mouse_input->getEvent(&event);
			D_PostEvent(&event);
		}
	}

	sdl_keyboard_input_device->gatherEvents();
	while (sdl_keyboard_input_device->hasEvent())
	{
		event_t event;
		sdl_keyboard_input_device->getEvent(&event);
		D_PostEvent(&event);
	}


	// set mask to get all events except keyboard, mouse, and joystick events
	const int event_mask = SDL_ALLEVENTS & ~SDL_KEYEVENTMASK & ~SDL_MOUSEEVENTMASK & ~SDL_JOYEVENTMASK;

	// Force SDL to gather events from input devices. This is called
	// implicitly from SDL_PollEvent but since we're using SDL_PeepEvents to
	// process only non-mouse events, SDL_PumpEvents is necessary.
	SDL_PumpEvents();
	int num_events = SDL_PeepEvents(sdl_events, MAX_EVENTS, SDL_GETEVENT, event_mask);

	for (int i = 0; i < num_events; i++)
	{
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

		case SDL_JOYBUTTONDOWN:
			if (sdl_ev->jbutton.which == joy_active)
			{
				event.type = ev_keydown;
				event.data1 = sdl_ev->jbutton.button + KEY_JOY1;
				event.data2 = event.data1;

				D_PostEvent(&event);
				break;
			}

		case SDL_JOYBUTTONUP:
			if (sdl_ev->jbutton.which == joy_active)
			{
				event.type = ev_keyup;
				event.data1 = sdl_ev->jbutton.button + KEY_JOY1;
				event.data2 = event.data1;

				D_PostEvent(&event);
				break;
			}

		case SDL_JOYAXISMOTION:
			if (sdl_ev->jaxis.which == joy_active)
			{
				event.type = ev_joystick;
				event.data1 = 0;
				event.data2 = sdl_ev->jaxis.axis;
				if ((sdl_ev->jaxis.value < JOY_DEADZONE) && (sdl_ev->jaxis.value > -JOY_DEADZONE))
					event.data3 = 0;
				else
					event.data3 = sdl_ev->jaxis.value;

				D_PostEvent(&event);
				break;
			}

		case SDL_JOYHATMOTION:
			if (sdl_ev->jhat.which == joy_active)
			{
				// Each of these need to be tested because more than one can be pressed and a
				// unique event is needed for each
				if (sdl_ev->jhat.value & SDL_HAT_UP)
					RegisterJoystickEvent(sdl_ev, SDL_HAT_UP);
				if (sdl_ev->jhat.value & SDL_HAT_RIGHT)
					RegisterJoystickEvent(sdl_ev, SDL_HAT_RIGHT);
				if (sdl_ev->jhat.value & SDL_HAT_DOWN)
					RegisterJoystickEvent(sdl_ev, SDL_HAT_DOWN);
				if (sdl_ev->jhat.value & SDL_HAT_LEFT)
					RegisterJoystickEvent(sdl_ev, SDL_HAT_LEFT);

				break;
			}
		};
	}

	if (use_joystick)
		UpdateJoystickEvents();
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

BEGIN_COMMAND(debugmouse)
{
//	if (mouse_input)
//		mouse_input->debug();
}
END_COMMAND(debugmouse)


VERSION_CONTROL (i_input_cpp, "$Id$")


