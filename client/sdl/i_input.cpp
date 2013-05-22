// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
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
//	SDL input handler
//
//-----------------------------------------------------------------------------


// SoM 12-24-05: yeah... I'm programming on christmas eve.
// Removed all the DirectX crap.

#include <stdlib.h>
#include <list>
#include <sstream>

#ifdef WIN32
#define _WIN32_WINNT 0x0400
#define WIN32_LEAN_AND_MEAN
#ifndef _XBOX
#include <windows.h>
#endif // !_XBOX
#endif // WIN32

#include <SDL.h>

#include "doomstat.h"
#include "m_argv.h"
#include "i_input.h"
#include "v_video.h"
#include "d_main.h"
#include "c_console.h"
#include "c_cvars.h"
#include "i_system.h"
#include "c_dispatch.h"

#ifdef _XBOX
#include "i_xbox.h"
#endif

#define JOY_DEADZONE 6000

EXTERN_CVAR (vid_fullscreen)
EXTERN_CVAR (vid_defwidth)
EXTERN_CVAR (vid_defheight)

static bool window_focused = false;
static bool nomouse = false;

EXTERN_CVAR (use_joystick)
EXTERN_CVAR (joy_active)

typedef struct
{
	SDL_Event    Event;
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

EXTERN_CVAR (mouse_driver)
static MouseInput* mouse_input = NULL;

//
// I_ShutdownMouseDriver
//
// Frees the memory used by mouse_input
//
static void I_ShutdownMouseDriver()
{
	delete mouse_input;
	mouse_input = NULL;
}

//
// I_InitMouseDriver
//
// Instantiates the proper concrete MouseInput object based on the
// mouse_driver cvar and stores a pointer to the object in mouse_input.
//
static void I_InitMouseDriver()
{
	I_ShutdownMouseDriver();

	if (!nomouse && screen)
	{
#ifdef WIN32
		if (mouse_input == NULL && mouse_driver == DI_MOUSE_DRIVER)
			mouse_input = DirectInputMouse::create();
#endif
	
		if (mouse_input == NULL)
			mouse_input = SDLMouse::create();
	}
}

//
// I_FlushInput
//
// Eat all pending input from outside the game
//
void I_FlushInput()
{
	SDL_Event ev;
	while (SDL_PollEvent(&ev));
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
// I_CheckMouseGrab
//
// Determines if SDL should grab the mouse based on the game window having
// focus and the status of the menu and console.
//
static bool I_CheckMouseGrab()
{
	// if the window doesn't have focus, never grab it
	if (!window_focused)
		return false;

	// always grab the mouse when full screen (dont want to
	// see the mouse pointer)
	if (vid_fullscreen)
		return true;

	// Don't grab the mouse if mouse input is disabled
	if (nomouse)
		return false;

    // when menu is active, console is down or game is paused, release the mouse
    if (menuactive || ConsoleState == c_down || paused)
        return false;

    // only grab mouse when playing levels (but not demos)
    return (gamestate == GS_LEVEL || gamestate == GS_INTERMISSION) && !demoplayback;
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
	return (state & SDL_APPINPUTFOCUS) && (state & SDL_APPACTIVE);
}

//
// I_InitFocus
//
// Sets the initial value of window_focused.
//
static void I_InitFocus()
{
	window_focused = I_CheckFocusState();

	if (window_focused)
	{
		SDL_WM_GrabInput(SDL_GRAB_ON);
		I_ResumeMouse();
	}
	else
	{
		SDL_WM_GrabInput(SDL_GRAB_OFF);
		I_PauseMouse();
	}
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
//
static void I_UpdateFocus()
{
	bool new_window_focused = I_CheckFocusState();

	if (new_window_focused && !window_focused)
	{
		I_FlushInput();
	}
	else if (!new_window_focused && window_focused)
	{
	}

	window_focused = new_window_focused;

	bool mouse_grabbed = mouse_input && !mouse_input->paused();
	bool can_grab_mouse = I_CheckMouseGrab();

	if (can_grab_mouse && !mouse_grabbed)
	{
		SDL_WM_GrabInput(SDL_GRAB_ON);
		I_FlushInput();
		I_ResumeMouse();
	}
	else if (mouse_grabbed && !can_grab_mouse)
	{
		SDL_WM_GrabInput(SDL_GRAB_OFF);
		I_PauseMouse();
	}
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
	event_t          event;

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
	event_t    event;

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
	int        ndx;

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

	atterm (I_ShutdownInput);

	SDL_EnableUNICODE(true);

	I_DisableKeyRepeat();

	// Initialize the joystick subsystem and open a joystick if use_joystick is enabled. -- Hyper_Eye
	Printf(PRINT_HIGH, "I_InitInput: Initializing SDL's joystick subsystem.\n");
	SDL_InitSubSystem(SDL_INIT_JOYSTICK);

	if((int)use_joystick && I_GetJoystickCount())
	{
		I_OpenJoystick();
		EnableJoystickPolling();
	}

#ifdef WIN32
	// denis - in fullscreen, prevent exit on accidental windows key press
	// [Russell] - Disabled because it screws with the mouse
	//g_hKeyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL,  LowLevelKeyboardProc, GetModuleHandle(NULL), 0);
#endif

	I_InitMouseDriver();
	I_InitFocus();

	return true;
}

//
// I_ShutdownInput
//
void STACK_ARGS I_ShutdownInput (void)
{
	I_ShutdownMouseDriver();

	SDL_WM_GrabInput(SDL_GRAB_OFF);
	I_ResetKeyRepeat();

#ifdef WIN32
	// denis - in fullscreen, prevent exit on accidental windows key press
	// [Russell] - Disabled because it screws with the mouse
	//UnhookWindowsHookEx(g_hKeyboardHook);
#endif
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
void I_GetEvent()
{
	const int MAX_EVENTS = 256;
	static SDL_Event sdl_events[MAX_EVENTS];
	event_t event;

	// Process mouse movement and button events
	if (mouse_input)
		mouse_input->processEvents();

	// Force SDL to gather events from input devices. This is called
	// implicitly from SDL_PollEvent but since we're using SDL_PeepEvents to
	// process only mouse events, SDL_PumpEvents is necessary.
	SDL_PumpEvents();
	const unsigned int mask = 
		SDL_KEYEVENTMASK | SDL_JOYEVENTMASK | SDL_VIDEORESIZEMASK |
		SDL_VIDEOEXPOSEMASK | SDL_QUITMASK | SDL_SYSWMEVENTMASK;

	int num_events = SDL_PeepEvents(sdl_events, MAX_EVENTS, SDL_GETEVENT, mask);

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
			break;

		case SDL_KEYDOWN:
			event.type = ev_keydown;
			event.data1 = sdl_ev->key.keysym.sym;

			if (event.data1 >= SDLK_KP0 && event.data1 <= SDLK_KP9)
				event.data2 = event.data3 = '0' + (event.data1 - SDLK_KP0);
			else if (event.data1 == SDLK_KP_PERIOD)
				event.data2 = event.data3 = '.';
			else if (event.data1 == SDLK_KP_DIVIDE)
				event.data2 = event.data3 = '/';
			else if (event.data1 == SDLK_KP_ENTER)
				event.data2 = event.data3 = '\r';
			else if ((sdl_ev->key.keysym.unicode & 0xFF80) == 0)
				event.data2 = event.data3 = sdl_ev->key.keysym.unicode;
			else
				event.data2 = event.data3 = 0;

#ifdef _XBOX
			// Fix for ENTER key on Xbox
            if (event.data1 == SDLK_RETURN)
				event.data2 = event.data3 = '\r';
#endif

#ifdef WIN32
			//HeX9109: Alt+F4 for cheats! Thanks Spleen
			if (event.data1 == SDLK_F4 && SDL_GetModState() & (KMOD_LALT | KMOD_RALT))
				AddCommandString("quit");
			// SoM: Ignore the tab portion of alt-tab presses
			// [AM] Windows 7 seems to preempt this check.
			if (event.data1 == SDLK_TAB && SDL_GetModState() & (KMOD_LALT | KMOD_RALT))
				event.data1 = event.data2 = event.data3 = 0;
			else
#endif
			D_PostEvent(&event);
			break;

		case SDL_KEYUP:
			event.type = ev_keyup;
			event.data1 = sdl_ev->key.keysym.sym;
			if ((sdl_ev->key.keysym.unicode & 0xFF80) == 0)
				event.data2 = event.data3 = sdl_ev->key.keysym.unicode;
			else
				event.data2 = event.data3 = 0;
			D_PostEvent(&event);
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
	I_UpdateFocus();
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
// DirectInputMouse
//
// ============================================================================

#ifdef WIN32

DirectInputMouse::DirectInputMouse() :
	mActive(false),
	mInitialized(false)
{
}

DirectInputMouse::~DirectInputMouse()
{
}

//
// DirectInputMouse::create
//
// Instantiates and returns a new DirectInputMouse object or returns NULL
// if DirectInput could not be initialized.
//
MouseInput* DirectInputMouse::create()
{
}

void DirectInputMouse::flushEvents()
{
}

void DirectInputMouse::processEvents()
{
}

void DirectInputMouse::center()
{
}

bool DirectInputMouse::paused() const
{
	return mActive == false;
}

void DirectInputMouse::pause()
{
}

void DirectInputMouse::resume()
{
}

#endif	// WIN32

// ============================================================================
//
// SDLMouse
//
// ============================================================================

SDLMouse::SDLMouse() :
	mActive(false)
{
}

SDLMouse::~SDLMouse()
{
	SDL_ShowCursor(true);
}

MouseInput* SDLMouse::create()
{
	return new SDLMouse();
}

void SDLMouse::flushEvents()
{
	SDL_PumpEvents();
	SDL_PeepEvents(mEvents, MAX_EVENTS, SDL_GETEVENT, SDL_MOUSEEVENTMASK);
}

void SDLMouse::processEvents()
{
	if (!mActive)
		return;

	// [SL] accumulate the total mouse movement over all events polled
	// and post one aggregate mouse movement event after all are polled.
	event_t movement_event;
	movement_event.type = ev_mouse;
	movement_event.data1 = movement_event.data2 = movement_event.data3 = 0;

	// Force SDL to gather events from input devices. This is called
	// implicitly from SDL_PollEvent but since we're using SDL_PeepEvents to
	// process only mouse events, SDL_PumpEvents is necessary.
	SDL_PumpEvents();
	int num_events = SDL_PeepEvents(mEvents, MAX_EVENTS, SDL_GETEVENT, SDL_MOUSEEVENTMASK);

	for (int i = 0; i < num_events; i++)
	{
		SDL_Event* sdl_ev = &mEvents[i];
		switch (sdl_ev->type)
		{
		case SDL_MOUSEMOTION:
		{
			movement_event.data2 += sdl_ev->motion.xrel;
			movement_event.data3 -= sdl_ev->motion.yrel;
			break;
		}

		case SDL_MOUSEBUTTONDOWN:
		{
			event_t button_event;
			button_event.type = ev_keydown;
			button_event.data1 = button_event.data2 = button_event.data3 = 0;

			if (sdl_ev->button.button == SDL_BUTTON_LEFT)
				button_event.data1 = KEY_MOUSE1;
			else if (sdl_ev->button.button == SDL_BUTTON_RIGHT)
				button_event.data1 = KEY_MOUSE2;
			else if (sdl_ev->button.button == SDL_BUTTON_MIDDLE)
				button_event.data1 = KEY_MOUSE3;
			else if (sdl_ev->button.button == SDL_BUTTON_X1)
				button_event.data1 = KEY_MOUSE4;	// [Xyltol 07/21/2011] - Add support for MOUSE4
			else if (sdl_ev->button.button == SDL_BUTTON_X2)
				button_event.data1 = KEY_MOUSE5;	// [Xyltol 07/21/2011] - Add support for MOUSE5
			else if (sdl_ev->button.button == SDL_BUTTON_WHEELUP)
				button_event.data1 = KEY_MWHEELUP;
			else if (sdl_ev->button.button == SDL_BUTTON_WHEELDOWN)
				button_event.data1 = KEY_MWHEELDOWN;

			if (button_event.data1 != 0)
				D_PostEvent(&button_event);
			break;
		}
		case SDL_MOUSEBUTTONUP:
		{
			event_t button_event;
			button_event.type = ev_keyup;
			button_event.data1 = button_event.data2 = button_event.data3 = 0;

			if (sdl_ev->button.button == SDL_BUTTON_LEFT)
				button_event.data1 = KEY_MOUSE1;
			else if (sdl_ev->button.button == SDL_BUTTON_RIGHT)
				button_event.data1 = KEY_MOUSE2;
			else if (sdl_ev->button.button == SDL_BUTTON_MIDDLE)
				button_event.data1 = KEY_MOUSE3;
			else if (sdl_ev->button.button == SDL_BUTTON_X1)
				button_event.data1 = KEY_MOUSE4;	// [Xyltol 07/21/2011] - Add support for MOUSE4
			else if (sdl_ev->button.button == SDL_BUTTON_X2)
				button_event.data1 = KEY_MOUSE5;	// [Xyltol 07/21/2011] - Add support for MOUSE5

			if (button_event.data1 != 0)
				D_PostEvent(&button_event);
			break;
		}
		default:
			// do nothing
			break;
		}
	}

	if (movement_event.data2 != 0 || movement_event.data3 != 0)
		D_PostEvent(&movement_event);

	center();
}

void SDLMouse::center()
{
	if (screen)
	{
		// warp the mouse to the center of the screen
		SDL_WarpMouse(screen->width / 2, screen->height / 2);
		// SDL_WarpMouse creates a new event in the queue and needs to be thrown out
		flushEvents();
	}
}


bool SDLMouse::paused() const
{
	return mActive == false;
}

void SDLMouse::pause()
{
	mActive = false;
	SDL_ShowCursor(true);
}

void SDLMouse::resume()
{
	mActive = true;
	SDL_ShowCursor(false);
	center();
}

VERSION_CONTROL (i_input_cpp, "$Id$")


