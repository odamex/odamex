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
//
//-----------------------------------------------------------------------------


#include "odamex.h"

#include "i_input_sdl12.h"

#include "i_sdl.h" 
#include "i_input.h"

#include "i_video.h"
#include "d_event.h"
#include "doomkeys.h"
#include <queue>
#include <cassert>

EXTERN_CVAR(joy_deadzone)
EXTERN_CVAR(joy_lefttrigger_deadzone)
EXTERN_CVAR(joy_righttrigger_deadzone)

// ============================================================================
//
// SDL 1.x Implementation
//
// ============================================================================

#ifdef SDL12

static const int MAX_SDL_EVENTS_PER_TIC = 8192;

// ============================================================================
//
// ISDL12KeyboardInputDevice implementation
//
// ============================================================================

// Map SDL 1.2 keycodes to SDL 2.0 keycodes (Odamex's internal keycode representation)
static int sdl12_keycode_map[] = {
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,	/* 000-004 */ \
	0x00000000, 0x00000000, 0x00000000, 0x00000008, 0x00000009,	/* 005-009 */ \
	0x00000000, 0x00000000, 0x4000009C, 0x0000000D, 0x00000000,	/* 010-014 */ \
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x40000048,	/* 015-019 */ \
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,	/* 020-024 */ \
	0x00000000, 0x00000000, 0x0000001B, 0x00000000, 0x00000000,	/* 025-029 */ \
	0x00000000, 0x00000000, 0x00000020, 0x00000021, 0x00000022,	/* 030-034 */ \
	0x00000023, 0x00000024, 0x00000000, 0x00000026, 0x00000027,	/* 035-039 */ \
	0x00000028, 0x00000029, 0x0000002A, 0x0000002B, 0x0000002C,	/* 040-044 */ \
	0x0000002D, 0x0000002E, 0x0000002F, 0x00000030, 0x00000031,	/* 045-049 */ \
	0x00000032, 0x00000033, 0x00000034, 0x00000035, 0x00000036,	/* 050-054 */ \
	0x00000037, 0x00000038, 0x00000039, 0x0000003A, 0x0000003B,	/* 055-059 */ \
	0x0000003C, 0x0000003D, 0x0000003E, 0x0000003F, 0x00000040,	/* 060-064 */ \
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,	/* 065-069 */ \
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,	/* 070-074 */ \
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,	/* 075-079 */ \
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,	/* 080-084 */ \
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,	/* 085-089 */ \
	0x00000000, 0x0000005B, 0x0000005C, 0x0000005D, 0x0000005E,	/* 090-094 */ \
	0x0000005F, 0x00000060, 0x00000061, 0x00000062, 0x00000063,	/* 095-099 */ \
	0x00000064, 0x00000065, 0x00000066, 0x00000067, 0x00000068,	/* 100-104 */ \
	0x00000069, 0x0000006A, 0x0000006B, 0x0000006C, 0x0000006D,	/* 105-109 */ \
	0x0000006E, 0x0000006F, 0x00000070, 0x00000071, 0x00000072,	/* 110-114 */ \
	0x00000073, 0x00000074, 0x00000075, 0x00000076, 0x00000077,	/* 115-119 */ \
	0x00000078, 0x00000079, 0x0000007A, 0x00000000, 0x00000000,	/* 120-124 */ \
	0x00000000, 0x00000000, 0x0000007F, 0x00000000, 0x00000000,	/* 125-129 */ \
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,	/* 130-134 */ \
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,	/* 135-139 */ \
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,	/* 140-144 */ \
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,	/* 145-149 */ \
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,	/* 150-154 */ \
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,	/* 155-159 */ \
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,	/* 160-164 */ \
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,	/* 165-169 */ \
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,	/* 170-174 */ \
	0x00000000, 0x00000000, 0x00000000, 0x000000B2, 0x00000000,	/* 175-179 */ \
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,	/* 180-184 */ \
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,	/* 185-189 */ \
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,	/* 190-194 */ \
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,	/* 195-199 */ \
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,	/* 200-204 */ \
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,	/* 205-209 */ \
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,	/* 210-214 */ \
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,	/* 215-219 */ \
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,	/* 220-224 */ \
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,	/* 225-229 */ \
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,	/* 230-234 */ \
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,	/* 235-239 */ \
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,	/* 240-244 */ \
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,	/* 245-249 */ \
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,	/* 250-254 */ \
	0x00000000, 0x40000062, 0x40000059, 0x4000005A, 0x4000005B,	/* 255-259 */ \
	0x4000005C, 0x4000005D, 0x4000005E, 0x4000005F, 0x40000060,	/* 260-264 */ \
	0x40000061, 0x40000063, 0x40000054, 0x40000055, 0x40000056,	/* 265-269 */ \
	0x40000057, 0x40000058, 0x40000067, 0x40000052, 0x40000051,	/* 270-274 */ \
	0x4000004F, 0x40000050, 0x40000049, 0x4000004A, 0x4000004D,	/* 275-279 */ \
	0x4000004B, 0x4000004E, 0x4000003A, 0x4000003B, 0x4000003C,	/* 280-284 */ \
	0x4000003D, 0x4000003E, 0x4000003F, 0x40000040, 0x40000041,	/* 285-289 */ \
	0x40000042, 0x40000043, 0x40000044, 0x40000045, 0x40000068,	/* 290-294 */ \
	0x40000069, 0x4000006A, 0x00000000, 0x00000000, 0x00000000,	/* 295-299 */ \
	0x40000053, 0x40000039, 0x40000047, 0x400000E5, 0x400000E1,	/* 300-304 */ \
	0x400000E4, 0x400000E0, 0x400000E6, 0x400000E2, 0x00000000,	/* 305-309 */ \
	0x00000000, 0x400000E3, 0x400000E7, 0x40000101, 0x00000000,	/* 310-314 */ \
	0x40000075, 0x40000046, 0x4000009A, 0x00000000, 0x40000076,	/* 315-319 */ \
	0x40000066, 0x00000000, 0x4000007A							/* 320-322 */ \
};


//
// ISDL12KeyboardInputDevice::ISDL12KeyboardInputDevice
//
ISDL12KeyboardInputDevice::ISDL12KeyboardInputDevice(int id) :
	mActive(false)
{
	// enable keyboard input
	resume();
}


//
// ISDL12KeyboardInputDevice::~ISDL12KeyboardInputDevice
//
ISDL12KeyboardInputDevice::~ISDL12KeyboardInputDevice()
{
	pause();
}


//
// ISDL12KeyboardInputDevice::active
//
bool ISDL12KeyboardInputDevice::active() const
{
	return mActive && I_GetWindow()->isFocused();
}


//
// ISDL12KeyboardInputDevice::flushEvents
//
void ISDL12KeyboardInputDevice::flushEvents()
{
	gatherEvents();
	while (!mEvents.empty())
		mEvents.pop();
}


//
// ISDL12KeyboardInputDevice::reset
//
void ISDL12KeyboardInputDevice::reset()
{
	flushEvents();
}


//
// ISDL12KeyboardInputDevice::pause
//
// Sets the internal state to ignore all input for this device.
//
// NOTE: SDL_EventState clears the SDL event queue so only call this after all
// SDL events have been processed in all SDL input device instances.
//
void ISDL12KeyboardInputDevice::pause()
{
	mActive = false;
	SDL_EventState(SDL_KEYDOWN, SDL_IGNORE);
	SDL_EventState(SDL_KEYUP, SDL_IGNORE);
	SDL_EnableUNICODE(SDL_DISABLE);
}


//
// ISDL12KeyboardInputDevice::resume
//
// Sets the internal state to enable all input for this device.
//
// NOTE: SDL_EventState clears the SDL event queue so only call this after all
// SDL events have been processed in all SDL input device instances.
//
void ISDL12KeyboardInputDevice::resume()
{
	mActive = true;
	reset();
	SDL_EventState(SDL_KEYDOWN, SDL_ENABLE);
	SDL_EventState(SDL_KEYUP, SDL_ENABLE);
	SDL_EnableUNICODE(SDL_ENABLE);
}


//
// ISDL12KeyboardInputDevice::translateKey
//
// Performs translation of an SDL_Keysym event to 
// to Odamex's internal key representation.
//
int ISDL12KeyboardInputDevice::translateKey(SDL_keysym keysym)
{
	int sdl12_keycode = keysym.sym;
	if (sdl12_keycode < (int)ARRAY_LENGTH(sdl12_keycode_map))
		return sdl12_keycode_map[sdl12_keycode];
	return 0;
}


//
// ISDL12KeyboardInputDevice::gatherEvents
//
// Pumps the SDL Event queue and retrieves any mouse events and puts them into
// this instance's event queue.
//
void ISDL12KeyboardInputDevice::gatherEvents()
{
	if (!active())
		return;

	// Force SDL to gather events from input devices. This is called
	// implicitly from SDL_PollEvent but since we're using SDL_PeepEvents to
	// process only mouse events, SDL_PumpEvents is necessary.
	SDL_PumpEvents();

	// Retrieve events from SDL
	int num_events = 0;
	SDL_Event sdl_events[MAX_SDL_EVENTS_PER_TIC];

	bool quit_event = false;

	while ((num_events = SDL_PeepEvents(sdl_events, MAX_SDL_EVENTS_PER_TIC, SDL_GETEVENT, SDL_KEYEVENTMASK)))
	{
		for (int i = 0; i < num_events; i++)
		{
			const SDL_Event& sdl_ev = sdl_events[i];
			assert(sdl_ev.type == SDL_KEYDOWN || sdl_ev.type == SDL_KEYUP);

			if (sdl_ev.key.keysym.sym == SDLK_F4 && sdl_ev.key.keysym.mod & (KMOD_LALT | KMOD_RALT))
			{
				// HeX9109: Alt+F4 for cheats! Thanks Spleen
				// [SL] Don't handle it here but make sure we indicate there was an ALT+F4 event.
				quit_event = true;
			}
			else if (sdl_ev.key.keysym.sym == SDLK_TAB && sdl_ev.key.keysym.mod & (KMOD_LALT | KMOD_RALT))
			{
				// do nothing - the event is dropped
			}
			else
			{
				// Normal game keyboard event - insert it into our internal queue
				event_t ev;
				ev.type = (sdl_ev.type == SDL_KEYDOWN) ? ev_keydown : ev_keyup;
				ev.data1 = translateKey(sdl_ev.key.keysym);

				if (ev.data1 >= SDLK_KP0 && ev.data1 <= SDLK_KP9)
					ev.data2 = ev.data3 = '0' + (ev.data1 - SDLK_KP0);
				else if (ev.data1 == SDLK_KP_PERIOD)
					ev.data2 = ev.data3 = '.';
				else if (ev.data1 == SDLK_KP_DIVIDE)
					ev.data2 = ev.data3 = '/';
				else if (ev.data1 == SDLK_KP_ENTER)
					ev.data2 = ev.data3 = '\r';
				else if ((sdl_ev.key.keysym.unicode & 0xFF80) == 0)
					ev.data2 = ev.data3 = sdl_ev.key.keysym.unicode;

				ev.mod = sdl_ev.key.keysym.mod;
				
				if (ev.data1)
					mEvents.push(ev);
			}
		}
	}

	// Translate the ALT+F4 key combo event into a SDL_QUIT event and push
	// it back into SDL's event queue so that it can be handled elsewhere.
	if (quit_event)
	{
		SDL_Event sdl_ev;
		sdl_ev.type = SDL_QUIT;
		SDL_PushEvent(&sdl_ev);
	}
}


//
// ISDL12KeyboardInputDevice::getEvent
//
// Removes the first event from the queue and returns it.
// This makes no checks to ensure there actually is an event in the queue and
// if there is not, the behavior is undefined.
//
void ISDL12KeyboardInputDevice::getEvent(event_t* ev)
{
	assert(hasEvent());
	*ev = mEvents.front();
	mEvents.pop();
}



// ============================================================================
//
// ISDL12MouseInputDevice implementation
//
// ============================================================================

//
// ISDL12MouseInputDevice::ISDL12MouseInputDevice
//
ISDL12MouseInputDevice::ISDL12MouseInputDevice(int id) :
	mActive(false)
{
	reset();
}


//
// ISDL12MouseInputDevice::~ISDL12MouseInputDevice
ISDL12MouseInputDevice::~ISDL12MouseInputDevice()
{
	pause();
}


//
// ISDL12MouseInputDevice::active
//
bool ISDL12MouseInputDevice::active() const
{
	return mActive && I_GetWindow()->isFocused();
}


//
// ISDL12MouseInputDevice::center
//
// Moves the mouse to the center of the screen to prevent absolute position
// methods from causing problems when the mouse is near the screen edges.
//
void ISDL12MouseInputDevice::center()
{
	if (active())
	{
		const int centerx = I_GetVideoWidth() / 2, centery = I_GetVideoHeight() / 2;
		int prevx, prevy;

		// get the x and y mouse position prior to centering it
		SDL_GetMouseState(&prevx, &prevy);

		// warp the mouse to the center of the screen
		SDL_WarpMouse(centerx, centery);

		// SDL_WarpMouse inserts a mouse event to warp the cursor to the center of the screen.
		// Filter out this mouse event by reading all of the mouse events in SDL'S queue and
		// re-insert any mouse events that were not inserted by SDL_WarpMouse.
		SDL_PumpEvents();

		// Retrieve events from SDL
		int num_events = 0;
		SDL_Event sdl_events[MAX_SDL_EVENTS_PER_TIC];

		num_events = SDL_PeepEvents(sdl_events, MAX_SDL_EVENTS_PER_TIC, SDL_GETEVENT, SDL_MOUSEMOTIONMASK);
		for (int i = 0; i < num_events; i++)
		{
			SDL_Event& sdl_ev = sdl_events[i];
			assert(sdl_ev.type == SDL_MOUSEMOTION);

			// drop the events caused by SDL_WarpMouse
			if (sdl_ev.motion.x == centerx && sdl_ev.motion.y == centery && 
				sdl_ev.motion.xrel == centerx - prevx && sdl_ev.motion.yrel == centery - prevy)
				continue;

			// this event is not the event caused by SDL_WarpMouse so add it back
			// to the event queue
			SDL_PushEvent(&sdl_ev);
		}
	}
}


//
// ISDL12MouseInputDevice::flushEvents
//
void ISDL12MouseInputDevice::flushEvents()
{
	gatherEvents();
	while (!mEvents.empty())
		mEvents.pop();
}


//
// ISDL12MouseInputDevice::reset
//
void ISDL12MouseInputDevice::reset()
{
	flushEvents();
	center();
}


//
// ISDL12MouseInputDevice::pause
//
// Sets the internal state to ignore all input for this device.
//
// NOTE: SDL_EventState clears the SDL event queue so only call this after all
// SDL events have been processed in all SDL input device instances.
//
void ISDL12MouseInputDevice::pause()
{
	mActive = false;
	SDL_EventState(SDL_MOUSEMOTION, SDL_IGNORE);
	SDL_EventState(SDL_MOUSEBUTTONDOWN, SDL_IGNORE);
	SDL_EventState(SDL_MOUSEBUTTONUP, SDL_IGNORE);
	SDL_WM_GrabInput(SDL_GRAB_OFF);
	SDL_ShowCursor(true);
}


//
// ISDL12MouseInputDevice::resume
//
// Sets the internal state to enable all input for this device.
//
// NOTE: SDL_EventState clears the SDL event queue so only call this after all
// SDL events have been processed in all SDL input device instances.
//
void ISDL12MouseInputDevice::resume()
{
	mActive = true;
	reset();
	SDL_WM_GrabInput(SDL_GRAB_ON);
	SDL_ShowCursor(false);
	SDL_EventState(SDL_MOUSEMOTION, SDL_ENABLE);
	SDL_EventState(SDL_MOUSEBUTTONDOWN, SDL_ENABLE);
	SDL_EventState(SDL_MOUSEBUTTONUP, SDL_ENABLE);
}


//
// ISDL12MouseInputDevice::gatherEvents
//
// Pumps the SDL Event queue and retrieves any mouse events and puts them into
// this instance's event queue.
//
void ISDL12MouseInputDevice::gatherEvents()
{
	if (active())
	{
		// Force SDL to gather events from input devices. This is called
		// implicitly from SDL_PollEvent but since we're using SDL_PeepEvents to
		// process only mouse events, SDL_PumpEvents is necessary.
		SDL_PumpEvents();

		// Retrieve events from SDL
		int num_events = 0;
		SDL_Event sdl_events[MAX_SDL_EVENTS_PER_TIC];

		while ((num_events = SDL_PeepEvents(sdl_events, MAX_SDL_EVENTS_PER_TIC, SDL_GETEVENT, SDL_MOUSEEVENTMASK)))
		{
			// insert the SDL_Events into our queue
			for (int i = 0; i < num_events; i++)
			{
				const SDL_Event& sdl_ev = sdl_events[i];
				assert(sdl_ev.type == SDL_MOUSEMOTION ||
						sdl_ev.type == SDL_MOUSEBUTTONDOWN || sdl_ev.type == SDL_MOUSEBUTTONUP);

				event_t ev;

				if (sdl_ev.type == SDL_MOUSEMOTION)
				{
					ev.type = ev_mouse;
					ev.data2 = sdl_ev.motion.xrel;
					ev.data3 = -sdl_ev.motion.yrel;
				}
				else if (sdl_ev.type == SDL_MOUSEBUTTONDOWN || sdl_ev.type == SDL_MOUSEBUTTONUP)
				{
					ev.type = (sdl_ev.type == SDL_MOUSEBUTTONDOWN) ? ev_keydown : ev_keyup;
					if (sdl_ev.button.button == SDL_BUTTON_LEFT)
						ev.data1 = OKEY_MOUSE1;
					else if (sdl_ev.button.button == SDL_BUTTON_RIGHT)
						ev.data1 = OKEY_MOUSE2;
					else if (sdl_ev.button.button == SDL_BUTTON_MIDDLE)
						ev.data1 = OKEY_MOUSE3;
					#if SDL_VERSION_ATLEAST(1, 2, 14)
					else if (sdl_ev.button.button == SDL_BUTTON_X1)
						ev.data1 = OKEY_MOUSE4;	// [Xyltol 07/21/2011] - Add support for MOUSE4
					else if (sdl_ev.button.button == SDL_BUTTON_X2)
						ev.data1 = OKEY_MOUSE5;	// [Xyltol 07/21/2011] - Add support for MOUSE5
					#endif
					else if (sdl_ev.button.button == SDL_BUTTON_WHEELUP)
						ev.data1 = OKEY_MWHEELUP;
					else if (sdl_ev.button.button == SDL_BUTTON_WHEELDOWN)
						ev.data1 = OKEY_MWHEELDOWN;
				}

				mEvents.push(ev);
			}
		}

		center();
	}
}


//
// ISDL12MouseInputDevice::getEvent
//
// Removes the first event from the queue and returns it.
// This makes no checks to ensure there actually is an event in the queue and
// if there is not, the behavior is undefined.
//
void ISDL12MouseInputDevice::getEvent(event_t* ev)
{
	assert(hasEvent());
	*ev = mEvents.front();
	mEvents.pop();
}



// ============================================================================
//
// ISDL12JoystickInputDevice implementation
//
// ============================================================================

//
// ISDL12JoystickInputDevice::ISDL12JoystickInputDevice
//
ISDL12JoystickInputDevice::ISDL12JoystickInputDevice(int id) :
	mActive(false), mJoystickId(id), mJoystick(NULL),
	mNumHats(0), mHatStates(NULL)
{
	assert(SDL_WasInit(SDL_INIT_JOYSTICK));
	assert(mJoystickId >= 0 && mJoystickId < SDL_NumJoysticks());

	mJoystick = SDL_JoystickOpen(mJoystickId);
	if (mJoystick == NULL)
		return;

	mNumHats = SDL_JoystickNumHats(mJoystick);
	mHatStates = new int[mNumHats];

	// This turns on automatic event polling for joysticks so that the state
	// of each button and axis doesn't need to be manually queried each tick. -- Hyper_Eye
	SDL_JoystickEventState(SDL_ENABLE);
	
	resume();
}


//
// ISDL12JoystickInputDevice::~ISDL12JoystickInputDevice
//
ISDL12JoystickInputDevice::~ISDL12JoystickInputDevice()
{
	pause();

	SDL_JoystickEventState(SDL_IGNORE);

	#ifndef _XBOX // This is to avoid a bug in SDLx
	if (mJoystick != NULL)
		SDL_JoystickClose(mJoystick);
	#endif

	delete [] mHatStates;
}


//
// ISDL12JoystickInputDevice::active
//
bool ISDL12JoystickInputDevice::active() const
{
	return mJoystick != NULL && mActive && I_GetWindow()->isFocused();
}


//
// ISDL12JoystickInputDevice::flushEvents
//
void ISDL12JoystickInputDevice::flushEvents()
{
	gatherEvents();
	while (!mEvents.empty())
		mEvents.pop();
	for (int i = 0; i < mNumHats; i++)
		mHatStates[i] = SDL_HAT_CENTERED;
}


//
// ISDL12JoystickInputDevice::reset
//
void ISDL12JoystickInputDevice::reset()
{
	flushEvents();
}


//
// ISDL12JoystickInputDevice::pause
//
// Sets the internal state to ignore all input for this device.
//
// NOTE: SDL_EventState clears the SDL event queue so only call this after all
// SDL events have been processed in all SDL input device instances.
//
void ISDL12JoystickInputDevice::pause()
{
	mActive = false;
	SDL_EventState(SDL_JOYAXISMOTION, SDL_IGNORE);
	SDL_EventState(SDL_JOYBALLMOTION, SDL_IGNORE);
	SDL_EventState(SDL_JOYHATMOTION, SDL_IGNORE);
	SDL_EventState(SDL_JOYBUTTONDOWN, SDL_IGNORE);
	SDL_EventState(SDL_JOYBUTTONUP, SDL_IGNORE);
}


//
// ISDL12JoystickInputDevice::resume
//
// Sets the internal state to enable all input for this device.
//
// NOTE: SDL_EventState clears the SDL event queue so only call this after all
// SDL events have been processed in all SDL input device instances.
//
void ISDL12JoystickInputDevice::resume()
{
	mActive = true;
	reset();
	SDL_EventState(SDL_JOYAXISMOTION, SDL_ENABLE);
	SDL_EventState(SDL_JOYBALLMOTION, SDL_ENABLE);
	SDL_EventState(SDL_JOYHATMOTION, SDL_ENABLE);
	SDL_EventState(SDL_JOYBUTTONDOWN, SDL_ENABLE);
	SDL_EventState(SDL_JOYBUTTONUP, SDL_ENABLE);
}


//
// ISDL12JoystickInputDevice::gatherEvents
//
// Pumps the SDL Event queue and retrieves any joystick events and translates
// them to an event_t instances before putting them into this instance's
// event queue.
//
void ISDL12JoystickInputDevice::gatherEvents()
{
	if (!active())
		return;

	// Force SDL to gather events from input devices. This is called
	// implicitly from SDL_PollEvent but since we're using SDL_PeepEvents to
	// process only mouse events, SDL_PumpEvents is necessary.
	SDL_PumpEvents();

	// Retrieve events from SDL
	int num_events = 0;
	SDL_Event sdl_events[MAX_SDL_EVENTS_PER_TIC];

	while ((num_events = SDL_PeepEvents(sdl_events, MAX_SDL_EVENTS_PER_TIC, SDL_GETEVENT, SDL_JOYEVENTMASK)))
	{
		for (int i = 0; i < num_events; i++)
		{
			const SDL_Event& sdl_ev = sdl_events[i];

			assert(sdl_ev.type == SDL_JOYBUTTONDOWN || sdl_ev.type == SDL_JOYBUTTONUP ||
					sdl_ev.type == SDL_JOYAXISMOTION || sdl_ev.type == SDL_JOYHATMOTION ||
					sdl_ev.type == SDL_JOYBALLMOTION);

			if ((sdl_ev.type == SDL_JOYBUTTONDOWN || sdl_ev.type == SDL_JOYBUTTONUP) &&
				sdl_ev.jbutton.which == mJoystickId)
			{
				event_t button_event;
				button_event.data1 = sdl_ev.jbutton.button + OKEY_JOY1;
				button_event.type = (sdl_ev.type == SDL_JOYBUTTONDOWN) ? ev_keydown : ev_keyup;
				mEvents.push(button_event);
			}
			else if (sdl_ev.type == SDL_JOYAXISMOTION && sdl_ev.jaxis.which == mJoystickId)
			{
				float deadzone = (joy_deadzone * 32767);
				event_t motion_event(ev_joystick);
				motion_event.data2 = sdl_ev.jaxis.axis;
				if ((sdl_ev.jaxis.value >= deadzone || (sdl_ev.jaxis.value <= -deadzone)))
					motion_event.data3 = sdl_ev.jaxis.value;
				mEvents.push(motion_event);
			}
			else if (sdl_ev.type == SDL_JOYHATMOTION && sdl_ev.jhat.which == mJoystickId)
			{
				// [SL] A single SDL joystick hat event indicates on/off for each of the
				// directional triggers for that hat. We need to create a separate 
				// ev_keydown or ev_keyup event_t instance for each directional trigger
				// indicated in this SDL joystick event.
				assert(sdl_ev.jhat.hat < mNumHats);
				int new_state = sdl_ev.jhat.value;
				int old_state = mHatStates[sdl_ev.jhat.hat];

				static const int flags[4] = { SDL_HAT_UP, SDL_HAT_RIGHT, SDL_HAT_DOWN, SDL_HAT_LEFT };
				for (int i = 0; i < 4; i++)
				{
					event_t hat_event;
					hat_event.data1 = (sdl_ev.jhat.hat * 4) + OKEY_HAT1 + i;

					// determine if the flag's state has changed (ignore it if it hasn't)
					if (!(old_state & flags[i]) && (new_state & flags[i]))
					{
						hat_event.type = ev_keydown;
						mEvents.push(hat_event);
					}
					else if ((old_state & flags[i]) && !(new_state & flags[i]))
					{
						hat_event.type = ev_keyup;
						mEvents.push(hat_event);
					}
				}

				mHatStates[sdl_ev.jhat.hat] = new_state;
			}
		}
	}
}


//
// ISDL12JoystickInputDevice::getEvent
//
// Removes the first event from the queue and returns it.
// This makes no checks to ensure there actually is an event in the queue and
// if there is not, the behavior is undefined.
//
void ISDL12JoystickInputDevice::getEvent(event_t* ev)
{
	assert(hasEvent());
	*ev = mEvents.front();
	mEvents.pop();
}

// ============================================================================
//
// ISDL12InputSubsystem implementation
//
// ============================================================================

//
// ISDL12InputSubsystem::ISDL12InputSubsystem
//
ISDL12InputSubsystem::ISDL12InputSubsystem() :
	IInputSubsystem(),
	mInputGrabbed(false)
{
	// Initialize the joystick subsystem and open a joystick if use_joystick is enabled. -- Hyper_Eye
	SDL_InitSubSystem(SDL_INIT_JOYSTICK);

	// Tell SDL to ignore events from the input devices
	// IInputDevice constructors will enable these events when they're initialized.
	SDL_EventState(SDL_KEYDOWN, SDL_IGNORE);
	SDL_EventState(SDL_KEYUP, SDL_IGNORE);

	SDL_EventState(SDL_MOUSEMOTION, SDL_IGNORE);
	SDL_EventState(SDL_MOUSEBUTTONDOWN, SDL_IGNORE);
	SDL_EventState(SDL_MOUSEBUTTONUP, SDL_IGNORE);

	SDL_EventState(SDL_JOYAXISMOTION, SDL_IGNORE);
	SDL_EventState(SDL_JOYBALLMOTION, SDL_IGNORE);
	SDL_EventState(SDL_JOYHATMOTION, SDL_IGNORE);
	SDL_EventState(SDL_JOYBUTTONDOWN, SDL_IGNORE);
	SDL_EventState(SDL_JOYBUTTONUP, SDL_IGNORE);

	SDL_ShowCursor(false);
	grabInput();
}


//
// ISDL12InputSubsystem::~ISDL12InputSubsystem
//
ISDL12InputSubsystem::~ISDL12InputSubsystem()
{
	if (getKeyboardInputDevice())
		shutdownKeyboard(0);
	if (getMouseInputDevice())
		shutdownMouse(0);
	if (getJoystickInputDevice())
		shutdownJoystick(0);

	SDL_QuitSubSystem(SDL_INIT_JOYSTICK);
}


//
// ISDL12InputSubsystem::getKeyboardDevices
//
// SDL only allows for one logical keyboard so just return a dummy device
// description.
//
std::vector<IInputDeviceInfo> ISDL12InputSubsystem::getKeyboardDevices() const
{
	std::vector<IInputDeviceInfo> devices;
	devices.push_back(IInputDeviceInfo());
	IInputDeviceInfo& device_info = devices.back();
	device_info.mId = 0;
	device_info.mDeviceName = "SDL 1.2 keyboard";
	return devices;
}


//
// ISDL12InputSubsystem::initKeyboard
//
void ISDL12InputSubsystem::initKeyboard(int id)
{
	shutdownKeyboard(0);

	const std::vector<IInputDeviceInfo> devices = getKeyboardDevices();
	std::string device_name;
	for (std::vector<IInputDeviceInfo>::const_iterator it = devices.begin(); it != devices.end(); ++it)
	{
		if (it->mId == id) 
			device_name = it->mDeviceName;
	}

	Printf(PRINT_HIGH, "I_InitInput: intializing %s\n", device_name.c_str());

	setKeyboardInputDevice(new ISDL12KeyboardInputDevice(id));
	registerInputDevice(getKeyboardInputDevice());
	getKeyboardInputDevice()->resume();
}


//
// ISDL12InputSubsystem::shutdownKeyboard
//
void ISDL12InputSubsystem::shutdownKeyboard(int id)
{
	IInputDevice* device = getKeyboardInputDevice();
	if (device)
	{
		unregisterInputDevice(device);
		delete device;
		setKeyboardInputDevice(NULL);
	}
}


//
// ISDL12InputSubsystem::getMouseDevices
//
// SDL only allows for one logical mouse so just return a dummy device
// description.
//
std::vector<IInputDeviceInfo> ISDL12InputSubsystem::getMouseDevices() const
{
	std::vector<IInputDeviceInfo> devices;
	devices.push_back(IInputDeviceInfo());
	IInputDeviceInfo& sdl_device_info = devices.back();
	sdl_device_info.mId = 0;
	sdl_device_info.mDeviceName = "SDL 1.2 mouse";

	return devices;
}


//
// ISDL12InputSubsystem::initMouse
//
void ISDL12InputSubsystem::initMouse(int id)
{
	shutdownMouse(0);

	const std::vector<IInputDeviceInfo> devices = getMouseDevices();
	std::string device_name;
	for (std::vector<IInputDeviceInfo>::const_iterator it = devices.begin(); it != devices.end(); ++it)
	{
		if (it->mId == id) 
			device_name = it->mDeviceName;
	}

	Printf(PRINT_HIGH, "I_InitInput: intializing %s\n", device_name.c_str());

	setMouseInputDevice(new ISDL12MouseInputDevice(id));
	assert(getMouseInputDevice() != NULL);
	registerInputDevice(getMouseInputDevice());
	getMouseInputDevice()->resume();
}


//
// ISDL12InputSubsystem::shutdownMouse
//
void ISDL12InputSubsystem::shutdownMouse(int id)
{
	IInputDevice* device = getMouseInputDevice();
	if (device)
	{
		unregisterInputDevice(device);
		delete device;
		setMouseInputDevice(NULL);
	}
}


//
//
// ISDL12InputSubsystem::getJoystickDevices
//
//
std::vector<IInputDeviceInfo> ISDL12InputSubsystem::getJoystickDevices() const
{
	std::vector<IInputDeviceInfo> devices;
	for (int i = 0; i < SDL_NumJoysticks(); i++)
	{
		devices.push_back(IInputDeviceInfo());
		IInputDeviceInfo& device_info = devices.back();
		device_info.mId = i;
		char name[256];
		sprintf(name, "SDL 1.2 joystick (%s)", SDL_JoystickName(i));
		device_info.mDeviceName = name;
	}

	return devices;
}


// ISDL12InputSubsystem::initJoystick
//
void ISDL12InputSubsystem::initJoystick(int id)
{
	shutdownJoystick(0);

	const std::vector<IInputDeviceInfo> devices = getJoystickDevices();
	std::string device_name;
	for (std::vector<IInputDeviceInfo>::const_iterator it = devices.begin(); it != devices.end(); ++it)
	{
		if (it->mId == id) 
			device_name = it->mDeviceName;
	}

	Printf(PRINT_HIGH, "I_InitInput: intializing %s\n", device_name.c_str());

	setJoystickInputDevice(new ISDL12JoystickInputDevice(id));
	registerInputDevice(getJoystickInputDevice());
	getJoystickInputDevice()->resume();
}


//
// ISDL12InputSubsystem::shutdownJoystick
//
void ISDL12InputSubsystem::shutdownJoystick(int id)
{
	IInputDevice* device = getJoystickInputDevice();
	if (device)
	{
		unregisterInputDevice(device);
		delete device;
		setJoystickInputDevice(NULL);
	}
}


//
// ISDL12InputSubsystem::grabInput
//
void ISDL12InputSubsystem::grabInput()
{
	mInputGrabbed = true;
	IInputDevice* device = getMouseInputDevice();
	if (device)
		device->resume();
}


//
// ISDL12InputSubsystem::releaseInput
//
void ISDL12InputSubsystem::releaseInput()
{
	mInputGrabbed = false;
	IInputDevice* device = getMouseInputDevice();
	if (device)
		device->pause();
}

#endif	// SDL12

VERSION_CONTROL (i_input_sdl12_cpp, "$Id$")
