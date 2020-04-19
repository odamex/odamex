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

#include "doomtype.h"
#include "i_sdl.h" 
#include "i_input.h"
#include "i_sdlinput.h"

#include "i_video.h"
#include "d_event.h"
#include "doomkeys.h"
#include <queue>
#include <cassert>

static const int MAX_SDL_EVENTS_PER_TIC = 8192;


// ============================================================================
//
// SDL 1.x Implementation
//
// ============================================================================

#ifdef SDL12

// ============================================================================
//
// ISDL12KeyboardInputDevice implementation
//
// ============================================================================

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
				ev.data1 = sdl_ev.key.keysym.sym;

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
	reset();
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
						ev.data1 = KEY_MOUSE1;
					else if (sdl_ev.button.button == SDL_BUTTON_RIGHT)
						ev.data1 = KEY_MOUSE2;
					else if (sdl_ev.button.button == SDL_BUTTON_MIDDLE)
						ev.data1 = KEY_MOUSE3;
					#if SDL_VERSION_ATLEAST(1, 2, 14)
					else if (sdl_ev.button.button == SDL_BUTTON_X1)
						ev.data1 = KEY_MOUSE4;	// [Xyltol 07/21/2011] - Add support for MOUSE4
					else if (sdl_ev.button.button == SDL_BUTTON_X2)
						ev.data1 = KEY_MOUSE5;	// [Xyltol 07/21/2011] - Add support for MOUSE5
					#endif
					else if (sdl_ev.button.button == SDL_BUTTON_WHEELUP)
						ev.data1 = KEY_MWHEELUP;
					else if (sdl_ev.button.button == SDL_BUTTON_WHEELDOWN)
						ev.data1 = KEY_MWHEELDOWN;
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
				button_event.data1 = sdl_ev.jbutton.button + KEY_JOY1;
				button_event.type = (sdl_ev.type == SDL_JOYBUTTONDOWN) ? ev_keydown : ev_keyup;
				mEvents.push(button_event);
			}
			else if (sdl_ev.type == SDL_JOYAXISMOTION && sdl_ev.jaxis.which == mJoystickId)
			{
				event_t motion_event(ev_joystick);
				motion_event.data2 = sdl_ev.jaxis.axis;
				if ((sdl_ev.jaxis.value >= JOY_DEADZONE) || (sdl_ev.jaxis.value <= -JOY_DEADZONE))
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
					hat_event.data1 = (sdl_ev.jhat.hat * 4) + KEY_HAT1 + i;

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
// ISDL12InputSubsystem::pauseKeyboard
//
void ISDL12InputSubsystem::pauseKeyboard()
{
	IInputDevice* device = getKeyboardInputDevice();
	if (device)
		device->pause();
}


//
// ISDL12InputSubsystem::resumeKeyboard
//
void ISDL12InputSubsystem::resumeKeyboard()
{
	IInputDevice* device = getKeyboardInputDevice();
	if (device)
		device->resume();
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
// ISDL12InputSubsystem::pauseMouse
//
void ISDL12InputSubsystem::pauseMouse()
{
	IInputDevice* device = getMouseInputDevice();
	if (device)
		device->pause();
}


//
// ISDL12InputSubsystem::resumeMouse
//
void ISDL12InputSubsystem::resumeMouse()
{
	IInputDevice* device = getMouseInputDevice();
	if (device)
		device->resume();
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
// ISDL12InputSubsystem::pauseJoystick
//
void ISDL12InputSubsystem::pauseJoystick()
{
	IInputDevice* device = getJoystickInputDevice();
	if (device)
		device->pause();
}


//
// ISDL12InputSubsystem::resumeJoystick
//
void ISDL12InputSubsystem::resumeJoystick()
{
	IInputDevice* device = getJoystickInputDevice();
	if (device)
		device->resume();
}


//
// ISDL12InputSubsystem::grabInput
//
void ISDL12InputSubsystem::grabInput()
{
	SDL_WM_GrabInput(SDL_GRAB_ON);
	mInputGrabbed = true;
}


//
// ISDL12InputSubsystem::releaseInput
//
void ISDL12InputSubsystem::releaseInput()
{
	SDL_WM_GrabInput(SDL_GRAB_OFF);
	mInputGrabbed = false;
}

#endif	// SDL12


// ============================================================================
//
// SDL 2.x Implementation
//
// ============================================================================

#ifdef SDL20

// ============================================================================
//
// ISDL20KeyboardInputDevice implementation
//
// ============================================================================

//
// ISDL20KeyboardInputDevice::ISDL20KeyboardInputDevice
//
ISDL20KeyboardInputDevice::ISDL20KeyboardInputDevice(int id) :
	mActive(false), mTextEntry(false)
{
	// enable keyboard input
	resume();
}


//
// ISDL20KeyboardInputDevice::~ISDL20KeyboardInputDevice
//
ISDL20KeyboardInputDevice::~ISDL20KeyboardInputDevice()
{
	pause();
}


//
// ISDL20KeyboardInputDevice::active
//
bool ISDL20KeyboardInputDevice::active() const
{
	return mActive && I_GetWindow()->isFocused();
}


//
// ISDL20KeyboardInputDevice::flushEvents
//
void ISDL20KeyboardInputDevice::flushEvents()
{
	gatherEvents();
	while (!mEvents.empty())
		mEvents.pop();
}


//
// ISDL20KeyboardInputDevice::reset
//
void ISDL20KeyboardInputDevice::reset()
{
	flushEvents();
}


//
// ISDL20KeyboardInputDevice::pause
//
// Sets the internal state to ignore all input for this device.
//
// NOTE: SDL_EventState clears the SDL event queue so only call this after all
// SDL events have been processed in all SDL input device instances.
//
void ISDL20KeyboardInputDevice::pause()
{
	mActive = false;
	SDL_EventState(SDL_KEYDOWN, SDL_IGNORE);
	SDL_EventState(SDL_KEYUP, SDL_IGNORE);
	SDL_EventState(SDL_TEXTINPUT, SDL_IGNORE);
}


//
// ISDL20KeyboardInputDevice::resume
//
// Sets the internal state to enable all input for this device.
//
// NOTE: SDL_EventState clears the SDL event queue so only call this after all
// SDL events have been processed in all SDL input device instances.
//
void ISDL20KeyboardInputDevice::resume()
{
	mActive = true;
	reset();
	SDL_EventState(SDL_KEYDOWN, SDL_ENABLE);
	SDL_EventState(SDL_KEYUP, SDL_ENABLE);
	SDL_EventState(SDL_TEXTINPUT, SDL_ENABLE);
}


//
// ISDL20KeyboardInputDevice::enableTextEntry
//
// Enables text entry for this device.
//
//
void ISDL20KeyboardInputDevice::enableTextEntry()
{
	mTextEntry = true;
	SDL_StartTextInput();
}


//
// ISDL20KeyboardInputDevice::disableTextEntry
//
// Disables text entry for this device.
//
//
void ISDL20KeyboardInputDevice::disableTextEntry()
{
	mTextEntry = false;
	SDL_StopTextInput();
}


//
// ISDL20KeyboardInputDevice::getTextEventValue
//
// Look for an SDL_TEXTINPUT event following this SDL_KEYDOWN event.
//
// On Windows platforms, SDL receives the translation from a key scancode
// to a localized text representation as a separate event. SDL 1.2 used to
// combine these separate events into a single SDL_Event however SDL 2.0
// creates two separate SDL_Events for this.
//
// When SDL2 receives a Windows event message for a key press from the
// Windows message queue, it calls "TranslateMessage" to generate
// a WM_CHAR event message that contains the localized text representation
// of the keypress. The WM_CHAR event ends up as the next event in the
// Windows message queue in practice (though there is no guarantee of this).
//
// Thus SDL2 inserts the key press event into its own queue as SDL_KEYDOWN
// and follows it with the text event SDL_TEXTINPUT.
//
// [SL] Based on GPL v2 code from ScummVM
//
// ScummVM - Graphic Adventure Engine
//
// ScummVM is the legal property of its developers, whose names
// are too numerous to list here. Please refer to the COPYRIGHT
// file distributed with this source distribution.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
//
int ISDL20KeyboardInputDevice::getTextEventValue()
{
	const size_t max_events = 32;
	SDL_Event sdl_events[max_events];
	
	SDL_PumpEvents();
	const size_t num_events = SDL_PeepEvents(sdl_events, max_events, SDL_PEEKEVENT, SDL_KEYDOWN, SDL_TEXTINPUT);
	for (size_t i = 0; i < num_events; i++)
	{
		// If we found another SDL_KEYDOWN event prior to SDL_TEXTINPUT event,
		// we must assume the next SDL_TEXTINPUT event does not correspond to
		// our original SDL_KEYDOWN event.
		if (sdl_events[i].type == SDL_KEYDOWN)
			return 0;

		// Looks like we found a corresponding SDL_TEXTINPUT event
		if (sdl_events[i].type == SDL_TEXTINPUT)
		{
			#if SDL_BYTEORDER == SDL_BIG_ENDIAN
			const char output_type[] = "UTF-32BE";
			#else
			const char output_type[] = "UTF-32LE";
			#endif 

			const char* src = sdl_events[i].text.text;
			uint32_t utf32 = 0;
			char* dst = SDL_iconv_string(output_type, "UTF-8", src, SDL_strlen(src) + 1);
			if (dst)
			{
				utf32 = *((uint32_t *)dst);
				SDL_free(dst);
			}
			return utf32;
		}
	}

	return 0;
}


//
// ISDL20KeyboardInputDevice::translateKey
//
// Performs translation of an SDL_Keysym event to 
// to Odamex's internal key representation (which is identical
// to SDL 2.0's key representation).
//
int ISDL20KeyboardInputDevice::translateKey(SDL_Keysym keysym)
{
	return keysym.sym;
}


//
// ISDL20KeyboardInputDevice::gatherEvents
//
// Pumps the SDL Event queue and retrieves any mouse events and puts them into
// this instance's event queue.
//
void ISDL20KeyboardInputDevice::gatherEvents()
{
	if (!active())
		return;

	// Force SDL to gather events from input devices. This is called
	// implicitly from SDL_PollEvent but since we're using SDL_PeepEvents to
	// process only keyboard events, SDL_PumpEvents is necessary.
	SDL_PumpEvents();

	SDL_Event sdl_ev;
	while (SDL_PeepEvents(&sdl_ev, 1, SDL_GETEVENT, SDL_KEYDOWN, SDL_TEXTINPUT))
	{
		// Process SDL_KEYDOWN / SDL_KEYUP events. SDL_TEXTINPUT events will
		// be implicitly ignored unless handled below.
		if (sdl_ev.type == SDL_KEYDOWN || sdl_ev.type == SDL_KEYUP)
		{
			const int sym = sdl_ev.key.keysym.sym;
			const int mod = sdl_ev.key.keysym.mod;

			event_t ev;
			ev.type = (sdl_ev.type == SDL_KEYDOWN) ? ev_keydown : ev_keyup;

			// Get the Odamex key code from the scancode
			ev.data1 = translateKey(sdl_ev.key.keysym);

			// From Chocolate Doom:
			// Get the localized version of the key press. This takes into account the
			// keyboard layout, but does not apply any changes due to modifiers, (eg.
			// shift-, alt-, etc.)
			//
			// [SL] not sure if this is actually useful...
			if (sym > 0 && sym < 128)
				ev.data2 = sym;

			// Get the unicode value for the key using the localized keyboard layout
			// and includes modification via shift, etc.
			if (sdl_ev.type == SDL_KEYDOWN)
				ev.data3 = getTextEventValue();

			// Ch0wW : Fixes a problem of ultra-fast repeats.
			if (sdl_ev.key.repeat != 0)
				continue;

			// drop ALT-TAB events - they're handled elsewhere
			if (sym == SDLK_TAB && mod & (KMOD_LALT | KMOD_RALT))
				continue;

			// HeX9109: Alt+F4 for cheats! Thanks Spleen
			// Translate the ALT+F4 key combo event into a SDL_QUIT event and push
			// it back into SDL's event queue so that it can be handled elsewhere.
			if (sym == SDLK_F4 && mod & (KMOD_LALT | KMOD_RALT))
			{
				SDL_Event sdl_quit_ev;
				sdl_quit_ev.type = SDL_QUIT;
				SDL_PushEvent(&sdl_quit_ev);
				continue;
			}

			// Normal game keyboard event - insert it into our internal queue
			if (ev.data1)
				mEvents.push(ev);
		}
	}
}


//
// ISDL20KeyboardInputDevice::getEvent
//
// Removes the first event from the queue and returns it.
// This makes no checks to ensure there actually is an event in the queue and
// if there is not, the behavior is undefined.
//
void ISDL20KeyboardInputDevice::getEvent(event_t* ev)
{
	assert(hasEvent());
	*ev = mEvents.front();
	mEvents.pop();
}



// ============================================================================
//
// ISDL20MouseInputDevice implementation
//
// ============================================================================

//
// ISDL20MouseInputDevice::ISDL20MouseInputDevice
//
ISDL20MouseInputDevice::ISDL20MouseInputDevice(int id) :
	mActive(false)
{
	reset();
}


//
// ISDL20MouseInputDevice::~ISDL20MouseInputDevice
ISDL20MouseInputDevice::~ISDL20MouseInputDevice()
{
	pause();
}


//
// ISDL20MouseInputDevice::active
//
bool ISDL20MouseInputDevice::active() const
{
	return mActive && I_GetWindow()->isFocused();
}


//
// ISDL20MouseInputDevice::flushEvents
//
void ISDL20MouseInputDevice::flushEvents()
{
	gatherEvents();
	while (!mEvents.empty())
		mEvents.pop();
}


//
// ISDL20MouseInputDevice::reset
//
void ISDL20MouseInputDevice::reset()
{
	flushEvents();
}


//
// ISDL20MouseInputDevice::pause
//
// Sets the internal state to ignore all input for this device.
//
// NOTE: SDL_EventState clears the SDL event queue so only call this after all
// SDL events have been processed in all SDL input device instances.
//
void ISDL20MouseInputDevice::pause()
{
	mActive = false;
	SDL_EventState(SDL_MOUSEMOTION, SDL_IGNORE);
	SDL_EventState(SDL_MOUSEBUTTONDOWN, SDL_IGNORE);
	SDL_EventState(SDL_MOUSEBUTTONUP, SDL_IGNORE);
	SDL_ShowCursor(true);
}


//
// ISDL20MouseInputDevice::resume
//
// Sets the internal state to enable all input for this device.
//
// NOTE: SDL_EventState clears the SDL event queue so only call this after all
// SDL events have been processed in all SDL input device instances.
//
void ISDL20MouseInputDevice::resume()
{
	mActive = true;
	SDL_ShowCursor(false);
	reset();
	SDL_EventState(SDL_MOUSEMOTION, SDL_ENABLE);
	SDL_EventState(SDL_MOUSEBUTTONDOWN, SDL_ENABLE);
	SDL_EventState(SDL_MOUSEBUTTONUP, SDL_ENABLE);
	SDL_SetRelativeMouseMode(SDL_TRUE);
}


//
// ISDL20MouseInputDevice::gatherEvents
//
// Pumps the SDL Event queue and retrieves any mouse events and puts them into
// this instance's event queue.
//
void ISDL20MouseInputDevice::gatherEvents()
{
	if (!active())
		return;

	// Force SDL to gather events from input devices. This is called
	// implicitly from SDL_PollEvent but since we're using SDL_PeepEvents to
	// process only mouse events, SDL_PumpEvents is necessary.
	int num_events;
	SDL_Event sdl_events[MAX_SDL_EVENTS_PER_TIC];
	SDL_PumpEvents();

	// Retrieve mouse movement events from SDL
	// [SL] accumulate the total mouse movement over all events polled
	// and post one aggregate mouse movement event to Doom's event queue
	// after all are polled.
	event_t movement_event(ev_mouse);

	while ((num_events = SDL_PeepEvents(sdl_events, MAX_SDL_EVENTS_PER_TIC, SDL_GETEVENT, SDL_MOUSEMOTION, SDL_MOUSEMOTION)))
	{
		for (int i = 0; i < num_events; i++)
		{
			const SDL_Event& sdl_ev = sdl_events[i];
			movement_event.data2 += sdl_ev.motion.xrel;
			movement_event.data3 -= sdl_ev.motion.yrel;
		}
	}

	if (movement_event.data2 || movement_event.data3)
		mEvents.push(movement_event);

	// Retrieve mouse button and wheel events from SDL and post
	// as separate events to Doom's event queue.
	while ((num_events = SDL_PeepEvents(sdl_events, MAX_SDL_EVENTS_PER_TIC, SDL_GETEVENT, SDL_MOUSEBUTTONDOWN, SDL_MOUSEWHEEL)))
	{
		for (int i = 0; i < num_events; i++)
		{
			event_t ev;

			const SDL_Event& sdl_ev = sdl_events[i];

			if (sdl_ev.type == SDL_MOUSEWHEEL)
			{
				ev.type = ev_keydown;
				int direction = 1;
				#if (SDL_VERSION >= SDL_VERSIONNUM(2, 0, 4))
				if (sdl_ev.wheel.direction == SDL_MOUSEWHEEL_FLIPPED)
					direction = -1;
				#endif

				if (direction * sdl_ev.wheel.y > 0)
					ev.data1 = KEY_MWHEELUP;
				else if (direction * sdl_ev.wheel.y < 0)
					ev.data1 = KEY_MWHEELDOWN;
			}
			else if (sdl_ev.type == SDL_MOUSEBUTTONDOWN || sdl_ev.type == SDL_MOUSEBUTTONUP)
			{
				ev.type = (sdl_ev.type == SDL_MOUSEBUTTONDOWN) ? ev_keydown : ev_keyup;
				if (sdl_ev.button.button == SDL_BUTTON_LEFT)
					ev.data1 = KEY_MOUSE1;
				else if (sdl_ev.button.button == SDL_BUTTON_RIGHT)
					ev.data1 = KEY_MOUSE2;
				else if (sdl_ev.button.button == SDL_BUTTON_MIDDLE)
					ev.data1 = KEY_MOUSE3;
				else if (sdl_ev.button.button == SDL_BUTTON_X1)
					ev.data1 = KEY_MOUSE4;	// [Xyltol 07/21/2011] - Add support for MOUSE4
				else if (sdl_ev.button.button == SDL_BUTTON_X2)
					ev.data1 = KEY_MOUSE5;	// [Xyltol 07/21/2011] - Add support for MOUSE5
			}

			if (ev.data1)
				mEvents.push(ev);
		}
	}
}


//
// ISDL20MouseInputDevice::getEvent
//
// Removes the first event from the queue and returns it.
// This makes no checks to ensure there actually is an event in the queue and
// if there is not, the behavior is undefined.
//
void ISDL20MouseInputDevice::getEvent(event_t* ev)
{
	assert(hasEvent());
	*ev = mEvents.front();
	mEvents.pop();
}



// ============================================================================
//
// ISDL20JoystickInputDevice implementation
//
// ============================================================================

//
// ISDL20JoystickInputDevice::ISDL20JoystickInputDevice
//
ISDL20JoystickInputDevice::ISDL20JoystickInputDevice(int id) :
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
// ISDL20JoystickInputDevice::~ISDL20JoystickInputDevice
//
ISDL20JoystickInputDevice::~ISDL20JoystickInputDevice()
{
	pause();

	SDL_JoystickEventState(SDL_IGNORE);

	if (mJoystick != NULL)
		SDL_JoystickClose(mJoystick);

	delete [] mHatStates;
}


//
// ISDL20JoystickInputDevice::active
//
bool ISDL20JoystickInputDevice::active() const
{
	return mJoystick != NULL && mActive && I_GetWindow()->isFocused();
}


//
// ISDL20JoystickInputDevice::flushEvents
//
void ISDL20JoystickInputDevice::flushEvents()
{
	gatherEvents();
	while (!mEvents.empty())
		mEvents.pop();
	for (int i = 0; i < mNumHats; i++)
		mHatStates[i] = SDL_HAT_CENTERED;
}


//
// ISDL20JoystickInputDevice::reset
//
void ISDL20JoystickInputDevice::reset()
{
	flushEvents();
}


//
// ISDL20JoystickInputDevice::pause
//
// Sets the internal state to ignore all input for this device.
//
// NOTE: SDL_EventState clears the SDL event queue so only call this after all
// SDL events have been processed in all SDL input device instances.
//
void ISDL20JoystickInputDevice::pause()
{
	mActive = false;
	SDL_EventState(SDL_JOYAXISMOTION, SDL_IGNORE);
	SDL_EventState(SDL_JOYBALLMOTION, SDL_IGNORE);
	SDL_EventState(SDL_JOYHATMOTION, SDL_IGNORE);
	SDL_EventState(SDL_JOYBUTTONDOWN, SDL_IGNORE);
	SDL_EventState(SDL_JOYBUTTONUP, SDL_IGNORE);
}


//
// ISDL20JoystickInputDevice::resume
//
// Sets the internal state to enable all input for this device.
//
// NOTE: SDL_EventState clears the SDL event queue so only call this after all
// SDL events have been processed in all SDL input device instances.
//
void ISDL20JoystickInputDevice::resume()
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
// ISDL20JoystickInputDevice::gatherEvents
//
// Pumps the SDL Event queue and retrieves any joystick events and translates
// them to an event_t instances before putting them into this instance's
// event queue.
//
void ISDL20JoystickInputDevice::gatherEvents()
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

	while ((num_events = SDL_PeepEvents(sdl_events, MAX_SDL_EVENTS_PER_TIC, SDL_GETEVENT, SDL_JOYAXISMOTION, SDL_JOYBUTTONUP)))
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
				button_event.type = (sdl_ev.type == SDL_JOYBUTTONDOWN) ? ev_keydown : ev_keyup;
				button_event.data1 = sdl_ev.jbutton.button + KEY_JOY1;
				mEvents.push(button_event);
			}
			else if (sdl_ev.type == SDL_JOYAXISMOTION && sdl_ev.jaxis.which == mJoystickId)
			{
				event_t motion_event(ev_joystick);
				motion_event.type = ev_joystick;
				motion_event.data2 = sdl_ev.jaxis.axis;
				if ((sdl_ev.jaxis.value >= JOY_DEADZONE) || (sdl_ev.jaxis.value <= -JOY_DEADZONE))
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
					hat_event.data1 = (sdl_ev.jhat.hat * 4) + KEY_HAT1 + i;

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
// ISDL20JoystickInputDevice::getEvent
//
// Removes the first event from the queue and returns it.
// This makes no checks to ensure there actually is an event in the queue and
// if there is not, the behavior is undefined.
//
void ISDL20JoystickInputDevice::getEvent(event_t* ev)
{
	assert(hasEvent());
	*ev = mEvents.front();
	mEvents.pop();
}

// ============================================================================
//
// ISDL20InputSubsystem implementation
//
// ============================================================================

//
// ISDL20InputSubsystem::ISDL20InputSubsystem
//
ISDL20InputSubsystem::ISDL20InputSubsystem() :
	IInputSubsystem(),
	mInputGrabbed(false)
{
	// Initialize the joystick subsystem and open a joystick if use_joystick is enabled. -- Hyper_Eye
	SDL_InitSubSystem(SDL_INIT_JOYSTICK);

	// Tell SDL to ignore events from the input devices
	// IInputDevice constructors will enable these events when they're initialized.
	SDL_EventState(SDL_KEYDOWN, SDL_IGNORE);
	SDL_EventState(SDL_KEYUP, SDL_IGNORE);
	SDL_EventState(SDL_TEXTINPUT, SDL_IGNORE);

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
// ISDL20InputSubsystem::~ISDL20InputSubsystem
//
ISDL20InputSubsystem::~ISDL20InputSubsystem()
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
// ISDL20InputSubsystem::getKeyboardDevices
//
// SDL only allows for one logical keyboard so just return a dummy device
// description.
//
std::vector<IInputDeviceInfo> ISDL20InputSubsystem::getKeyboardDevices() const
{
	std::vector<IInputDeviceInfo> devices;
	devices.push_back(IInputDeviceInfo());
	IInputDeviceInfo& device_info = devices.back();
	device_info.mId = 0;
	device_info.mDeviceName = "SDL 2.0 keyboard";
	return devices;
}


//
// ISDL20InputSubsystem::initKeyboard
//
void ISDL20InputSubsystem::initKeyboard(int id)
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

	setKeyboardInputDevice(new ISDL20KeyboardInputDevice(id));
	registerInputDevice(getKeyboardInputDevice());
	getKeyboardInputDevice()->resume();
}


//
// ISDL20InputSubsystem::shutdownKeyboard
//
void ISDL20InputSubsystem::shutdownKeyboard(int id)
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
// ISDL20InputSubsystem::pauseKeyboard
//
void ISDL20InputSubsystem::pauseKeyboard()
{
	IInputDevice* device = getKeyboardInputDevice();
	if (device)
		device->pause();
}


//
// ISDL20InputSubsystem::resumeKeyboard
//
void ISDL20InputSubsystem::resumeKeyboard()
{
	IInputDevice* device = getKeyboardInputDevice();
	if (device)
		device->resume();
}


//
// ISDL20InputSubsystem::getMouseDevices
//
// SDL only allows for one logical mouse so just return a dummy device
// description.
//
std::vector<IInputDeviceInfo> ISDL20InputSubsystem::getMouseDevices() const
{
	std::vector<IInputDeviceInfo> devices;
	devices.push_back(IInputDeviceInfo());
	IInputDeviceInfo& sdl_device_info = devices.back();
	sdl_device_info.mId = 0;
	sdl_device_info.mDeviceName = "SDL 2.0 mouse";
	return devices;
}


//
// ISDL20InputSubsystem::initMouse
//
void ISDL20InputSubsystem::initMouse(int id)
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

	setMouseInputDevice(new ISDL20MouseInputDevice(id));
	assert(getMouseInputDevice() != NULL);
	registerInputDevice(getMouseInputDevice());
	getMouseInputDevice()->resume();
}


//
// ISDL20InputSubsystem::shutdownMouse
//
void ISDL20InputSubsystem::shutdownMouse(int id)
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
// ISDL20InputSubsystem::pauseMouse
//
void ISDL20InputSubsystem::pauseMouse()
{
	IInputDevice* device = getMouseInputDevice();
	if (device)
		device->pause();
}


//
// ISDL20InputSubsystem::resumeMouse
//
void ISDL20InputSubsystem::resumeMouse()
{
	IInputDevice* device = getMouseInputDevice();
	if (device)
		device->resume();
}


//
//
// ISDL20InputSubsystem::getJoystickDevices
//
//
std::vector<IInputDeviceInfo> ISDL20InputSubsystem::getJoystickDevices() const
{
	// TODO: does the SDL Joystick subsystem need to be initialized?
	std::vector<IInputDeviceInfo> devices;
	for (int i = 0; i < SDL_NumJoysticks(); i++)
	{
		devices.push_back(IInputDeviceInfo());
		IInputDeviceInfo& device_info = devices.back();
		device_info.mId = i;
		char name[256];
		sprintf(name, "SDL 2.0 joystick (%s)", SDL_JoystickNameForIndex(i));
		device_info.mDeviceName = name;
	}

	return devices;
}


// ISDL20InputSubsystem::initJoystick
//
void ISDL20InputSubsystem::initJoystick(int id)
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

	setJoystickInputDevice(new ISDL20JoystickInputDevice(id));
	registerInputDevice(getJoystickInputDevice());
	getJoystickInputDevice()->resume();
}


//
// ISDL20InputSubsystem::shutdownJoystick
//
void ISDL20InputSubsystem::shutdownJoystick(int id)
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
// ISDL20InputSubsystem::pauseJoystick
//
void ISDL20InputSubsystem::pauseJoystick()
{
	IInputDevice* device = getJoystickInputDevice();
	if (device)
		device->pause();
}


//
// ISDL20InputSubsystem::resumeJoystick
//
void ISDL20InputSubsystem::resumeJoystick()
{
	IInputDevice* device = getJoystickInputDevice();
	if (device)
		device->resume();
}


//
// ISDL20InputSubsystem::grabInput
//
void ISDL20InputSubsystem::grabInput()
{
	SDL_SetRelativeMouseMode(SDL_TRUE);
	mInputGrabbed = true;
}


//
// ISDL20InputSubsystem::releaseInput
//
void ISDL20InputSubsystem::releaseInput()
{
	SDL_SetRelativeMouseMode(SDL_FALSE);
	mInputGrabbed = false;
}

#endif	// SDL20

VERSION_CONTROL (i_sdlinput_cpp, "$Id$")
