// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: i_sdlinput.cpp 5302 2015-04-23 01:53:13Z dr_sean $
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
//
//-----------------------------------------------------------------------------

#include "doomtype.h"
#include <SDL.h>
#include "i_input.h"
#include "i_sdlinput.h"

#include "i_video.h"
#include "d_event.h"
#include "doomkeys.h"
#include <queue>


//
// ISDL12MouseInputDevice::ISDL12MouseInputDevice
//
ISDL12MouseInputDevice::ISDL12MouseInputDevice() :
	mActive(false)
{
	reset();
}

//
// ISDL12MouseInputDevice::center
//
// Moves the mouse to the center of the screen to prevent absolute position
// methods from causing problems when the mouse is near the screen edges.
//
void ISDL12MouseInputDevice::center()
{
	const int centerx = I_GetVideoWidth() / 2;
	const int centery = I_GetVideoHeight() / 2;

	// warp the mouse to the center of the screen
	SDL_WarpMouse(centerx, centery);

	// SDL_WarpMouse inserts a mouse event to warp the cursor to the center of the screen.
	// Filter out this mouse event by reading all of the mouse events in SDL'S queue and
	// re-insert any mouse events that were not inserted by SDL_WarpMouse.
	SDL_PumpEvents();

	// Retrieve chunks of up to 1024 events from SDL
	const int max_events = 1024;
	int num_events = 0;
	SDL_Event sdl_events[max_events];

	// TODO: if there are more than 1024 events, they won't all be examined...
	num_events = SDL_PeepEvents(sdl_events, max_events, SDL_GETEVENT, SDL_MOUSEMOTION);
	for (int i = 0; i < num_events; i++)
	{
		SDL_Event* sdl_ev = &sdl_events[i];
		if (sdl_ev->type != SDL_MOUSEMOTION || sdl_ev->motion.x != centerx || sdl_ev->motion.y != centery)
		{
			// this event is not the event caused by SDL_WarpMouse so add it back
			// to the event queue
			SDL_PushEvent(sdl_ev);
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
	SDL_ShowCursor(false);
	SDL_EventState(SDL_MOUSEMOTION, SDL_ENABLE);
	SDL_EventState(SDL_MOUSEBUTTONDOWN, SDL_ENABLE);
	SDL_EventState(SDL_MOUSEBUTTONUP, SDL_ENABLE);
	reset();
}


//
// ISDL12MouseInputDevice::gatherEvents
//
// Pumps the SDL Event queue and retrieves any mouse events and puts them into
// this instance's event queue.
//
void ISDL12MouseInputDevice::gatherEvents()
{
	if (!mActive)
		return;

	// Force SDL to gather events from input devices. This is called
	// implicitly from SDL_PollEvent but since we're using SDL_PeepEvents to
	// process only mouse events, SDL_PumpEvents is necessary.
	SDL_PumpEvents();

	// Retrieve chunks of up to 1024 events from SDL
	int num_events = 0;
	const int max_events = 1024;
	SDL_Event sdl_events[max_events];

	while ((num_events = SDL_PeepEvents(sdl_events, max_events, SDL_GETEVENT, SDL_MOUSEEVENTMASK)))
	{
		// insert the SDL_Events into our queue
		for (int i = 0; i < num_events; i++)
			mEvents.push(sdl_events[i]);
	}
}


//
// ISDL12MouseInputDevice::getEvent
//
// Removes the first event from the queue and translates it to a Doom event_t.
// This makes no checks to ensure there actually is an event in the queue and
// if there is not, the behavior is undefined.
//
void ISDL12MouseInputDevice::getEvent(event_t* ev)
{
	// clear the destination struct
	ev->type = ev_keydown;
	ev->data1 = ev->data2 = ev->data3 = 0;

	const SDL_Event& sdl_ev = mEvents.front();

	if (sdl_ev.type == SDL_MOUSEMOTION)
	{
		ev->type = ev_mouse;
		ev->data2 += sdl_ev.motion.xrel;
		ev->data3 -= sdl_ev.motion.yrel;
	}
	else if (sdl_ev.type == SDL_MOUSEBUTTONDOWN)
	{
		ev->type = ev_keydown;

		if (sdl_ev.button.button == SDL_BUTTON_LEFT)
			ev->data1 = KEY_MOUSE1;
		else if (sdl_ev.button.button == SDL_BUTTON_RIGHT)
			ev->data1 = KEY_MOUSE2;
		else if (sdl_ev.button.button == SDL_BUTTON_MIDDLE)
			ev->data1 = KEY_MOUSE3;
		else if (sdl_ev.button.button == SDL_BUTTON_X1)
			ev->data1 = KEY_MOUSE4;	// [Xyltol 07/21/2011] - Add support for MOUSE4
		else if (sdl_ev.button.button == SDL_BUTTON_X2)
			ev->data1 = KEY_MOUSE5;	// [Xyltol 07/21/2011] - Add support for MOUSE5
		else if (sdl_ev.button.button == SDL_BUTTON_WHEELUP)
			ev->data1 = KEY_MWHEELUP;
		else if (sdl_ev.button.button == SDL_BUTTON_WHEELDOWN)
			ev->data1 = KEY_MWHEELDOWN;
	}
	else if (sdl_ev.type == SDL_MOUSEBUTTONUP)
	{
		ev->type = ev_keyup;

		if (sdl_ev.button.button == SDL_BUTTON_LEFT)
			ev->data1 = KEY_MOUSE1;
		else if (sdl_ev.button.button == SDL_BUTTON_RIGHT)
			ev->data1 = KEY_MOUSE2;
		else if (sdl_ev.button.button == SDL_BUTTON_MIDDLE)
			ev->data1 = KEY_MOUSE3;
		else if (sdl_ev.button.button == SDL_BUTTON_X1)
			ev->data1 = KEY_MOUSE4;	// [Xyltol 07/21/2011] - Add support for MOUSE4
		else if (sdl_ev.button.button == SDL_BUTTON_X2)
			ev->data1 = KEY_MOUSE5;	// [Xyltol 07/21/2011] - Add support for MOUSE5
	}

	mEvents.pop();
}


VERSION_CONTROL (i_sdlinput_cpp, "$Id: i_sdlinput.cpp 5315 2015-04-24 05:01:36Z dr_sean $")
