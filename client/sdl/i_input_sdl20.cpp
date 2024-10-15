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

#include "i_input_sdl20.h"

#include "i_sdl.h" 
#include "i_input.h"

#include "i_video.h"
#include "d_event.h"
#include "doomkeys.h"
#include <queue>
#include <cassert>

static const int MAX_SDL_EVENTS_PER_TIC = 8192;

EXTERN_CVAR(joy_deadzone)
EXTERN_CVAR(joy_lefttrigger_deadzone)
EXTERN_CVAR(joy_righttrigger_deadzone)

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

			// Add the mod in
			ev.mod = mod;

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
	SDL_SetRelativeMouseMode(SDL_FALSE);
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
	reset();
	SDL_SetRelativeMouseMode(SDL_TRUE);
	SDL_EventState(SDL_MOUSEMOTION, SDL_ENABLE);
	SDL_EventState(SDL_MOUSEBUTTONDOWN, SDL_ENABLE);
	SDL_EventState(SDL_MOUSEBUTTONUP, SDL_ENABLE);
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
					ev.data1 = OKEY_MWHEELUP;
				else if (direction * sdl_ev.wheel.y < 0)
					ev.data1 = OKEY_MWHEELDOWN;
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
				else if (sdl_ev.button.button == SDL_BUTTON_X1)
					ev.data1 = OKEY_MOUSE4;	// [Xyltol 07/21/2011] - Add support for MOUSE4
				else if (sdl_ev.button.button == SDL_BUTTON_X2)
					ev.data1 = OKEY_MOUSE5;	// [Xyltol 07/21/2011] - Add support for MOUSE5
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
	mActive(false), mJoystickId(id), mJoystick(NULL)
{
	assert(SDL_WasInit(SDL_INIT_GAMECONTROLLER));
	assert(mJoystickId >= 0 && mJoystickId < SDL_NumJoysticks());

	mJoystick = SDL_GameControllerOpen(mJoystickId);
	if (mJoystick == NULL)
		return;

	// This turns on automatic event polling for joysticks so that the state
	// of each button and axis doesn't need to be manually queried each tick. -- Hyper_Eye
	SDL_GameControllerEventState(SDL_ENABLE);
	
	resume();
}


//
// ISDL20JoystickInputDevice::~ISDL20JoystickInputDevice
//
ISDL20JoystickInputDevice::~ISDL20JoystickInputDevice()
{
	pause();

	SDL_GameControllerEventState(SDL_IGNORE);

	if (mJoystick != NULL)
		SDL_GameControllerClose(mJoystick);
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
	SDL_EventState(SDL_CONTROLLERAXISMOTION, SDL_IGNORE);
	SDL_EventState(SDL_CONTROLLERBUTTONDOWN, SDL_IGNORE);
	SDL_EventState(SDL_CONTROLLERBUTTONUP, SDL_IGNORE);
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
	SDL_EventState(SDL_CONTROLLERAXISMOTION, SDL_ENABLE);
	SDL_EventState(SDL_CONTROLLERBUTTONDOWN, SDL_ENABLE);
	SDL_EventState(SDL_CONTROLLERBUTTONUP, SDL_ENABLE);
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

	while ((num_events = SDL_PeepEvents(sdl_events, MAX_SDL_EVENTS_PER_TIC, SDL_GETEVENT,
	                                 SDL_CONTROLLERAXISMOTION, SDL_CONTROLLERBUTTONUP)))
	{
		for (int i = 0; i < num_events; i++)
		{
			const SDL_Event& sdl_ev = sdl_events[i];

			assert(sdl_ev.type == SDL_CONTROLLERBUTTONDOWN ||
			       sdl_ev.type == SDL_CONTROLLERBUTTONUP ||
			       sdl_ev.type == SDL_CONTROLLERAXISMOTION);

			if ((sdl_ev.type == SDL_CONTROLLERBUTTONDOWN ||
			     sdl_ev.type == SDL_CONTROLLERBUTTONUP) &&
				sdl_ev.jbutton.which == mJoystickId)
			{
				event_t button_event;
				button_event.type =
				    (sdl_ev.type == SDL_CONTROLLERBUTTONDOWN) ? ev_keydown : ev_keyup;
				button_event.data1 = sdl_ev.cbutton.button + OKEY_JOY1;
				mEvents.push(button_event);
			}
			else if (sdl_ev.type == SDL_CONTROLLERAXISMOTION &&
			         sdl_ev.caxis.which == mJoystickId)
			{

				float deadzone;
				bool bPressed = false;

				if (sdl_ev.caxis.axis == SDL_CONTROLLER_AXIS_TRIGGERLEFT)
				{
					event_t button_event;
					
					deadzone = (joy_lefttrigger_deadzone * 32767);
					
					if ((sdl_ev.caxis.value >= deadzone) ||
					    (sdl_ev.caxis.value <= -deadzone)){
						bPressed = true;	
					}

					button_event.type = bPressed ? ev_keydown : ev_keyup;
					button_event.data1 = OKEY_JOY20; // LEFT TRIGGER
					mEvents.push(button_event);
				}

				else if (sdl_ev.caxis.axis == SDL_CONTROLLER_AXIS_TRIGGERRIGHT)
				{
					event_t button_event;

					deadzone = (joy_righttrigger_deadzone * 32767);

					if ((sdl_ev.caxis.value >= deadzone) ||
					    (sdl_ev.caxis.value <= -deadzone)){
						bPressed = true;
					}

					button_event.type = bPressed ? ev_keydown : ev_keyup;
					button_event.data1 = OKEY_JOY21;	// RIGHT TRIGGER
					mEvents.push(button_event);
				}
				else
				{
					event_t motion_event(ev_joystick);
					motion_event.type = ev_joystick;
					motion_event.data2 = sdl_ev.caxis.axis;
					motion_event.data3 = calcAxisValue(sdl_ev.caxis.value);
					mEvents.push(motion_event);
				}

			}
		}
	}

	// Flush all remaining joystick and game controller events.
	SDL_FlushEvents(SDL_JOYAXISMOTION, SDL_CONTROLLERDEVICEREMAPPED);
}

int ISDL20JoystickInputDevice::calcAxisValue(int raw_value)
{
	float value;

	// Normalize.
	if (raw_value > 0)
	{
		value = (float)raw_value / (float)SDL_JOYSTICK_AXIS_MAX;
	}
	else if (raw_value < 0)
	{
		value = (float)raw_value / (float)abs(SDL_JOYSTICK_AXIS_MIN);
	}
	else
	{
		value = 0.0f;
	}

	// Apply deadzone.
	if (value > joy_deadzone)
	{
		value = (value - joy_deadzone) / (1.0f - joy_deadzone);
	}
	else if (value < -joy_deadzone)
	{
		value = (value + joy_deadzone) / (1.0f - joy_deadzone);
	}
	else
	{
		value = 0.0f;
	}

	// Scale value to the range used for calculations in G_BuildTiccmd().
	return lroundf(value * 32767.0f);
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
	SDL_InitSubSystem(SDL_INIT_GAMECONTROLLER);

	// Tell SDL to ignore events from the input devices
	// IInputDevice constructors will enable these events when they're initialized.
	SDL_EventState(SDL_KEYDOWN, SDL_IGNORE);
	SDL_EventState(SDL_KEYUP, SDL_IGNORE);
	SDL_EventState(SDL_TEXTINPUT, SDL_IGNORE);

	SDL_EventState(SDL_MOUSEMOTION, SDL_IGNORE);
	SDL_EventState(SDL_MOUSEBUTTONDOWN, SDL_IGNORE);
	SDL_EventState(SDL_MOUSEBUTTONUP, SDL_IGNORE);

	SDL_EventState(SDL_CONTROLLERAXISMOTION, SDL_IGNORE);
	SDL_EventState(SDL_CONTROLLERBUTTONDOWN, SDL_IGNORE);
	SDL_EventState(SDL_CONTROLLERBUTTONUP, SDL_IGNORE);

	// Ignore unsupported game controller events.
#if (SDL_MINOR_VERSION > 0 || SDL_PATCHLEVEL >= 14)
	SDL_EventState(SDL_CONTROLLERTOUCHPADDOWN, SDL_IGNORE);
	SDL_EventState(SDL_CONTROLLERTOUCHPADMOTION, SDL_IGNORE);
	SDL_EventState(SDL_CONTROLLERTOUCHPADUP, SDL_IGNORE);
	SDL_EventState(SDL_CONTROLLERSENSORUPDATE, SDL_IGNORE);
#endif

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

	SDL_QuitSubSystem(SDL_INIT_GAMECONTROLLER);
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
		snprintf(name, 256, "SDL 2.0 joystick (%s)", SDL_GameControllerNameForIndex(i));
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
// ISDL20InputSubsystem::grabInput
//
void ISDL20InputSubsystem::grabInput()
{
	mInputGrabbed = true;
	IInputDevice* device = getMouseInputDevice();
	if (device)
		device->resume();
}


//
// ISDL20InputSubsystem::releaseInput
//
void ISDL20InputSubsystem::releaseInput()
{
	mInputGrabbed = false;
	IInputDevice* device = getMouseInputDevice();
	if (device)
		device->pause();
}

#endif	// SDL20

VERSION_CONTROL(i_input_sdl20_cpp, "$Id$")
