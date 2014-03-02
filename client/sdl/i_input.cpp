// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2006-2014 by The Odamex Team.
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

#include "i_sdl.h" 
#include "win32inc.h"

#include "doomstat.h"
#include "m_argv.h"
#include "i_input.h"
#include "i_video.h"
#include "d_main.h"
#include "c_console.h"
#include "c_cvars.h"
#include "i_system.h"
#include "c_dispatch.h"

#ifdef _XBOX
#include "i_xbox.h"
#endif

#ifdef _WIN32
#include <SDL_syswm.h>
#endif

#define JOY_DEADZONE 6000


// TODO: remove this!
extern SDL_Window* I_GetSDLWindow();

EXTERN_CVAR (vid_fullscreen)
EXTERN_CVAR (vid_defwidth)
EXTERN_CVAR (vid_defheight)

static bool window_focused = false;
static bool nomouse = false;
extern bool configuring_controls;

static bool key_repeat_enabled;

static unsigned int key_repeat_delay = 250;		// in milliseconds
static unsigned int key_repeat_interval = 30;	// in milliseconds

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
	while (SDL_PollEvent(&ev));
}


//
// I_EnableKeyRepeat
//
void I_EnableKeyRepeat()
{
	key_repeat_enabled = true;

	#ifdef SDL12
	SDL_EnableKeyRepeat(key_repeat_delay, key_repeat_interval);
	#endif	// SDL12
}


//
// I_DisableKeyRepeat
//
void I_DisableKeyRepeat()
{
	key_repeat_enabled = false;

	#ifdef SDL12
	SDL_EnableKeyRepeat(0, 0);
	#endif	// SDL12
}


//
// I_ResetKeyRepeat
//
void I_ResetKeyRepeat()
{
	key_repeat_enabled = true;

	#ifdef SDL12
	SDL_EnableKeyRepeat(key_repeat_delay, key_repeat_interval);
	#endif	// SDL12
}


//
// I_CheckFocusState
//
// Determines if the Odamex window currently has the window manager focus.
// We should have input (keyboard) focus and be visible (not minimized).
//
static bool I_CheckFocusState()
{
	#ifdef SDL20
	SDL_Window* window = I_GetSDLWindow();
	return (SDL_GetWindowFlags(window) & (SDL_WINDOW_SHOWN | SDL_WINDOW_MINIMIZED)) == SDL_WINDOW_SHOWN;
	#elif defined SDL12
	SDL_PumpEvents();
	Uint8 state = SDL_GetAppState();
	return (state & SDL_APPACTIVE) && (state & SDL_APPINPUTFOCUS);
	#endif	// SDL12
}


//
// I_GrabInput
//
static void I_GrabInput()
{
	#ifdef SDL20
	SDL_Window* window = I_GetSDLWindow();
	SDL_SetWindowGrab(window, SDL_TRUE);
	#elif defined SDL12
	SDL_WM_GrabInput(SDL_GRAB_ON);
	#endif	// SDL12
}


//
// I_UngrabInput
//
static void I_UngrabInput()
{
	#ifdef SDL20
	SDL_Window* window = I_GetSDLWindow();
	SDL_SetWindowGrab(window, SDL_FALSE);
	#elif defined SDL12
	SDL_WM_GrabInput(SDL_GRAB_OFF);
	#endif	// SDL12
}


//
// I_IsInputGrabbed
//
static bool I_IsInputGrabbed()
{
	#ifdef SDL20
	SDL_Window* window = I_GetSDLWindow();
	return SDL_GetWindowGrab(window);
	#elif defined SDL12
	return SDL_WM_GrabInput(SDL_GRAB_QUERY);
	#endif	// SDL12
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
		I_GrabInput();
		I_ResumeMouse();
	}
	else
	{
		I_UngrabInput();
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
}


//
// I_UpdateInputGrabbing
//
// Determines if SDL should grab the mouse based on the game window having
// focus and the status of the menu and console.
//
static void I_UpdateInputGrabbing()
{
#ifndef GCONSOLE
	bool can_grab = false;
	bool grabbed = I_IsInputGrabbed();

	if (!window_focused)
		can_grab = false;
	else if (vid_fullscreen)
		can_grab = true;
	else if (nomouse)
		can_grab = false;
	else if (configuring_controls)
		can_grab = true;
	else if (menuactive || ConsoleState == c_down || paused)
		can_grab = false;
	else if ((gamestate == GS_LEVEL || gamestate == GS_INTERMISSION) && !demoplayback)
		can_grab = true;

	// force I_ResumeMouse or I_PauseMouse if toggling between fullscreen/windowed
	static float prev_vid_fullscreen = vid_fullscreen;
	if (vid_fullscreen != prev_vid_fullscreen)
		grabbed = !can_grab;
	prev_vid_fullscreen = vid_fullscreen;

	// check if the window focus changed (or menu/console status changed)
	if (can_grab && !grabbed)
	{
		I_GrabInput();
		I_ResumeMouse();
		I_FlushInput();
	}
	else if (grabbed && !can_grab)
	{
		I_UngrabInput();
		I_PauseMouse();
	}
#endif
}


//
// I_RegisterJoystickEvent
//
// Add any joystick event to a list if it will require manual polling
// to detect release. This includes hat events (mostly due to d-pads not
// triggering the centered event when released) and analog axis bound
// as a key/button -- HyperEye
//
static int I_RegisterJoystickEvent(SDL_Event *ev, int value)
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
			else if((SDL_GetTicks() - (*i)->RegTick >= key_repeat_delay) &&
					(SDL_GetTicks() - (*i)->LastTick >= key_repeat_interval * 2))
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
std::string I_GetJoystickNameFromIndex(int index)
{
	#ifdef SDL20
	const char* joyname = SDL_JoystickNameForIndex(index);
	#elif defined SDL12
	const char* joyname = SDL_JoystickName(index);
	#endif	// SDL12

	if (!joyname)
		return "";
	return std::string(joyname);
}

//
// I_OpenJoystick
//
bool I_OpenJoystick()
{
	int numjoy = I_GetJoystickCount();

	if (numjoy == 0 || !use_joystick)
		return false;

	if (joy_active.asInt() > numjoy)
		joy_active.Set(0.0);

	#ifdef SDL20
	if (!openedjoy || !SDL_JoystickGetAttached(openedjoy))	
		openedjoy = SDL_JoystickOpen(joy_active.asInt());
	return SDL_JoystickGetAttached(openedjoy);
	#elif defined SDL12
	if (!SDL_JoystickOpened(joy_active.asInt()))
		openedjoy = SDL_JoystickOpen(joy_active.asInt());
	return SDL_JoystickOpened(joy_active.asInt());
	#endif	// SDL12
}

//
// I_CloseJoystick
//
void I_CloseJoystick()
{
#ifndef _XBOX // This is to avoid a bug in SDLx
	if (I_GetJoystickCount() == 0 || !openedjoy)
		return;

	#ifdef SDL20
	if (SDL_JoystickGetAttached(openedjoy))
		SDL_JoystickClose(openedjoy);
	#elif defined SDL12
	if (SDL_JoystickOpened(SDL_JoystickIndex(openedjoy)))
		SDL_JoystickClose(openedjoy);
	#endif	// SDL12

	openedjoy = NULL;
#endif	// _XBOX

	// Reset joy position values. Wouldn't want to get stuck in a turn or something. -- Hyper_Eye
	extern int joyforward, joystrafe, joyturn, joylook;
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

	#ifdef SDL12
	SDL_EnableUNICODE(true);
	#endif	// SDL12

	I_DisableKeyRepeat();

	// Initialize the joystick subsystem and open a joystick if use_joystick is enabled. -- Hyper_Eye
	Printf(PRINT_HIGH, "I_InitInput: Initializing SDL's joystick subsystem.\n");
	SDL_InitSubSystem(SDL_INIT_JOYSTICK);

	if((int)use_joystick && I_GetJoystickCount())
	{
		I_OpenJoystick();
		EnableJoystickPolling();
	}

	I_InitFocus();

	return true;
}


//
// I_ShutdownInput
//
void STACK_ARGS I_ShutdownInput (void)
{
	I_UngrabInput();
	I_ResetKeyRepeat();
}


//
// I_CenterMouse
//
// Moves the mouse to the center of the screen to prevent absolute position
// methods from causing problems when the mouse is near the screen edges.
//
void I_CenterMouse()
{
	int x = I_GetVideoWidth() / 2;
	int y = I_GetVideoHeight() / 2;

	#ifdef SDL20
	SDL_Window* window = I_GetSDLWindow();
	SDL_WarpMouseInWindow(window, x, y);
	#elif defined SDL12
	SDL_WarpMouse(x, y);
	#endif	// SDL12

	// SDL_WarpMouse inserts a mouse event to warp the cursor to the center of the screen
	// we need to filter out this event
	SDL_PumpEvents();

	static const int MAX_EVENTS = 512;
	SDL_Event sdl_events[MAX_EVENTS];

	#ifdef SDL20
	int num_events = SDL_PeepEvents(sdl_events, MAX_EVENTS, SDL_GETEVENT, SDL_MOUSEMOTION, SDL_MOUSEMOTION);
	#elif defined SDL12
	int num_events = SDL_PeepEvents(sdl_events, MAX_EVENTS, SDL_GETEVENT, SDL_MOUSEMOTIONMASK);
	#endif	// SDL12

	for (int i = 0; i < num_events; i++)
	{
		SDL_Event* sdl_ev = &sdl_events[i];
		if (sdl_ev->type != SDL_MOUSEMOTION || sdl_ev->motion.x != x || sdl_ev->motion.y != y)
		{
			// this event is not the event caused by SDL_WarpMouse so add it back
			// to the event queue
			SDL_PushEvent(sdl_ev);
		}
	}
}


//
// I_PauseMouse
//
// Enables the mouse cursor and prevents the game from processing mouse movement
// or button events
//
void I_PauseMouse()
{
	I_CenterMouse();
	SDL_ShowCursor(true);
//	I_SetSDLIgnoreMouseEvents();
}


//
// I_ResumeMouse
//
// Disables the mouse cursor and allows the game to process mouse movement
// or button events
//
void I_ResumeMouse()
{
	I_CenterMouse();
	SDL_ShowCursor(false);
//	I_UnsetSDLIgnoreMouseEvents();
}


//
// I_GetEvent
//
// Pumps SDL for new input events and posts them to the Doom event queue.
//
// [SL] SDL 1.2 only:
// Keyboard and joystick events are retreived directly from SDL while mouse
// movement and buttons are handled by the MouseInput class.
//
void I_GetEvent()
{
	static const int MAX_EVENTS = 512;
	static SDL_Event sdl_events[MAX_EVENTS];

	// accumulator for mouse movement events 
	event_t mouse_movement_event(ev_mouse);

	// Force SDL to gather events from input devices. 
	SDL_PumpEvents();

	#ifdef SDL20
	int num_events = SDL_PeepEvents(sdl_events, MAX_EVENTS, SDL_GETEVENT, SDL_FIRSTEVENT, SDL_LASTEVENT);
	#elif defined SDL12
	int num_events = SDL_PeepEvents(sdl_events, MAX_EVENTS, SDL_GETEVENT, SDL_ALLEVENTS & ~SDL_MOUSEEVENTMASK);
	#endif	// SDL12

	for (int i = 0; i < num_events; i++)
	{
		SDL_Event* sdl_ev = &sdl_events[i];
		switch (sdl_ev->type)
		{
		case SDL_QUIT:
			AddCommandString("quit");
			break;

		#ifdef SDL20
		case SDL_WINDOWEVENT:
		{
			if (sdl_ev->window.event == SDL_WINDOWEVENT_RESIZED)
			{
				// Resizable window mode resolutions
				if (!vid_fullscreen)
				{
					// ignore sdl_ev->window.windowID for now
					int width = sdl_ev->window.data1;
					int height = sdl_ev->window.data2;

					std::stringstream command;
					command << "vid_setmode " << width << " " << height;
					AddCommandString(command.str());

					vid_defwidth.Set(float(width));
					vid_defheight.Set(float(height));
				}
			}
			else if (sdl_ev->window.event == SDL_WINDOWEVENT_FOCUS_GAINED ||
					sdl_ev->window.event == SDL_WINDOWEVENT_FOCUS_LOST)
			{
				// need to update our focus state
				I_UpdateFocus();
				// pause the mouse when the focus goes away (eg, alt-tab)
				if (!window_focused)
					I_PauseMouse();
			}

			break;
		}

		#elif defined SDL12
		case SDL_VIDEORESIZE:
		{
			// Resizable window mode resolutions
			if (!vid_fullscreen)
			{
				int width = sdl_ev->resize.w;
				int height = sdl_ev->resize.h;

				std::stringstream command;
				command << "vid_setmode " << width << " " << height;
				AddCommandString(command.str());

				vid_defwidth.Set(float(width));
				vid_defheight.Set(float(height));
			}
			break;
		}

		case SDL_ACTIVEEVENT:
		{
			// need to update our focus state
			I_UpdateFocus();
			// pause the mouse when the focus goes away (eg, alt-tab)
			if (!window_focused)
				I_PauseMouse();
			break;
		}

		#endif	// SDL12

		case SDL_KEYDOWN:
		{
			#ifdef SDL20
			if (!key_repeat_enabled && sdl_ev->key.repeat)
				break;
			#endif	// SDL20

			event_t event(ev_keydown);
			event.data1 = sdl_ev->key.keysym.sym;

			#ifdef SDL20
			if (event.data1 >= SDLK_KP_0 && event.data1 <= SDLK_KP_9)
				event.data2 = event.data3 = '0' + (event.data1 - SDLK_KP_0);
			#elif defined SDL12
			if (event.data1 >= SDLK_KP0 && event.data1 <= SDLK_KP9)
				event.data2 = event.data3 = '0' + (event.data1 - SDLK_KP0);
			#endif	// SDL12
			else if (event.data1 == SDLK_KP_PERIOD)
				event.data2 = event.data3 = '.';
			else if (event.data1 == SDLK_KP_DIVIDE)
				event.data2 = event.data3 = '/';
			else if (event.data1 == SDLK_KP_ENTER)
				event.data2 = event.data3 = '\r';

			// TODO: handle unicode for SDL20
			#ifdef SDL12
			else if ((sdl_ev->key.keysym.unicode & 0xFF80) == 0)
				event.data2 = event.data3 = sdl_ev->key.keysym.unicode;
			#endif	// SDL12

			#ifdef _XBOX
			// Fix for ENTER key on Xbox
			if (event.data1 == SDLK_RETURN)
				event.data2 = event.data3 = '\r';
			#endif	// _XBOX

			#ifdef _WIN32
			//HeX9109: Alt+F4 for cheats! Thanks Spleen
			if (event.data1 == SDLK_F4 && SDL_GetModState() & (KMOD_LALT | KMOD_RALT))
				AddCommandString("quit");
			// SoM: Ignore the tab portion of alt-tab presses
			// [AM] Windows 7 seems to preempt this check.
			if (event.data1 == SDLK_TAB && SDL_GetModState() & (KMOD_LALT | KMOD_RALT))
				event.data1 = event.data2 = event.data3 = 0;
			#endif	// _WIN32

			if (event.data1)
				D_PostEvent(&event);
			break;
		}

		case SDL_KEYUP:
		{
			#ifdef SDL20
			if (!key_repeat_enabled && sdl_ev->key.repeat)
				break;
			#endif	// SDL20

			event_t event(ev_keyup);
			event.data1 = sdl_ev->key.keysym.sym;

			// TODO: handle unicode for SDL20
			#ifdef SDL12
			if ((sdl_ev->key.keysym.unicode & 0xFF80) == 0)
				event.data2 = event.data3 = sdl_ev->key.keysym.unicode;
			#endif	// SDL12

			if (event.data1)
				D_PostEvent(&event);
			break;
		}

		case SDL_JOYBUTTONDOWN:
			if (sdl_ev->jbutton.which == joy_active)
			{
				event_t event(ev_keydown);
				event.data1 = event.data2 = sdl_ev->jbutton.button + KEY_JOY1;
				D_PostEvent(&event);
				break;
			}

		case SDL_JOYBUTTONUP:
			if (sdl_ev->jbutton.which == joy_active)
			{
				event_t event(ev_keyup);
				event.data1 = event.data2 = sdl_ev->jbutton.button + KEY_JOY1;
				D_PostEvent(&event);
				break;
			}

		case SDL_JOYAXISMOTION:
			if (sdl_ev->jaxis.which == joy_active)
			{
				event_t event(ev_joystick);
				event.data2 = sdl_ev->jaxis.axis;
				if ((sdl_ev->jaxis.value >= JOY_DEADZONE) || (sdl_ev->jaxis.value <= -JOY_DEADZONE))
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
					I_RegisterJoystickEvent(sdl_ev, SDL_HAT_UP);
				if (sdl_ev->jhat.value & SDL_HAT_RIGHT)
					I_RegisterJoystickEvent(sdl_ev, SDL_HAT_RIGHT);
				if (sdl_ev->jhat.value & SDL_HAT_DOWN)
					I_RegisterJoystickEvent(sdl_ev, SDL_HAT_DOWN);
				if (sdl_ev->jhat.value & SDL_HAT_LEFT)
					I_RegisterJoystickEvent(sdl_ev, SDL_HAT_LEFT);

				break;
			}

		case SDL_MOUSEMOTION:
		{
			mouse_movement_event.data2 += sdl_ev->motion.xrel;
			mouse_movement_event.data3 -= sdl_ev->motion.yrel;
			break;
		}

		case SDL_MOUSEBUTTONDOWN:
		{
			event_t event(ev_keydown);

			if (sdl_ev->button.button == SDL_BUTTON_LEFT)
				event.data1 = KEY_MOUSE1;
			else if (sdl_ev->button.button == SDL_BUTTON_RIGHT)
				event.data1 = KEY_MOUSE2;
			else if (sdl_ev->button.button == SDL_BUTTON_MIDDLE)
				event.data1 = KEY_MOUSE3;
			else if (sdl_ev->button.button == SDL_BUTTON_X1)
				event.data1 = KEY_MOUSE4;	// [Xyltol 07/21/2011] - Add support for MOUSE4
			else if (sdl_ev->button.button == SDL_BUTTON_X2)
				event.data1 = KEY_MOUSE5;	// [Xyltol 07/21/2011] - Add support for MOUSE5

			#ifdef SDL12
			else if (sdl_ev->button.button == SDL_BUTTON_WHEELUP)
				event.data1 = KEY_MWHEELUP;
			else if (sdl_ev->button.button == SDL_BUTTON_WHEELDOWN)
				event.data1 = KEY_MWHEELDOWN;
			#endif	// SDL12

			if (event.data1)
				D_PostEvent(&event);
			break;
		}

		case SDL_MOUSEBUTTONUP:
		{
			event_t event(ev_keyup);

			if (sdl_ev->button.button == SDL_BUTTON_LEFT)
				event.data1 = KEY_MOUSE1;
			else if (sdl_ev->button.button == SDL_BUTTON_RIGHT)
				event.data1 = KEY_MOUSE2;
			else if (sdl_ev->button.button == SDL_BUTTON_MIDDLE)
				event.data1 = KEY_MOUSE3;
			else if (sdl_ev->button.button == SDL_BUTTON_X1)
				event.data1 = KEY_MOUSE4;	// [Xyltol 07/21/2011] - Add support for MOUSE4
			else if (sdl_ev->button.button == SDL_BUTTON_X2)
				event.data1 = KEY_MOUSE5;	// [Xyltol 07/21/2011] - Add support for MOUSE5

			if (event.data1)
				D_PostEvent(&event);
			break;
		}

		#ifdef SDL20
		case SDL_MOUSEWHEEL:
		{
			event_t event(ev_keydown);

			if (sdl_ev->wheel.y > 0)
				event.data1 = KEY_MWHEELUP;
			else if (sdl_ev->wheel.y < 0)
				event.data1 = KEY_MWHEELDOWN;

			if (event.data1)
				D_PostEvent(&event);
			break;
		}
		#endif	// SDL20

		default:
			// do nothing
			break;
		}
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
	I_UpdateInputGrabbing();
	I_GetEvent();
}

//
// I_StartFrame
//
void I_StartFrame (void)
{
}



VERSION_CONTROL (i_input_cpp, "$Id$")


