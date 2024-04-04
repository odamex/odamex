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

#pragma once

#include "i_sdl.h"
#include "i_input.h"

#include "d_event.h"
#include <queue>
#include <list>
#include "hashtable.h"

typedef OHashTable<int, int> KeyTranslationTable;

#ifdef SDL20

// ============================================================================
//
// ISDL20KeyboardInputDevice class interface
//
// ============================================================================

class ISDL20KeyboardInputDevice : public IKeyboardInputDevice
{
public:
	ISDL20KeyboardInputDevice(int id);
	virtual ~ISDL20KeyboardInputDevice();

	virtual bool active() const;

	virtual void pause();
	virtual void resume();
	virtual void reset();

	virtual void gatherEvents();

	virtual bool hasEvent() const
	{	return !mEvents.empty();	}

	virtual void getEvent(event_t* ev);

	virtual void flushEvents();

	virtual void enableTextEntry();
	virtual void disableTextEntry();

private:
	int translateKey(SDL_Keysym keysym);
	int getTextEventValue();

	bool					mActive;
	bool					mTextEntry;

	typedef std::queue<event_t> EventQueue;
	EventQueue				mEvents;
};


// ============================================================================
//
// ISDL20MouseInputDevice class interface
//
// ============================================================================

class ISDL20MouseInputDevice : public IInputDevice
{
public:
	ISDL20MouseInputDevice(int id);
	virtual ~ISDL20MouseInputDevice();

	virtual bool active() const;

	virtual void pause();
	virtual void resume();
	virtual void reset();

	virtual void gatherEvents();

	virtual bool hasEvent() const
	{	return !mEvents.empty();	}

	virtual void getEvent(event_t* ev);

	virtual void flushEvents();

private:
	bool			mActive;

	typedef std::queue<event_t> EventQueue;
	EventQueue		mEvents;
};


// ============================================================================
//
// ISDL20JoystickInputDevice class interface
//
// ============================================================================

class ISDL20JoystickInputDevice : public IInputDevice
{
public:
	ISDL20JoystickInputDevice(int id);
	virtual ~ISDL20JoystickInputDevice();

	virtual bool active() const;

	virtual void pause();
	virtual void resume();
	virtual void reset();

	virtual void gatherEvents();

	virtual bool hasEvent() const
	{	return !mEvents.empty();	}

	virtual void getEvent(event_t* ev);

	virtual void flushEvents();

private:
	int calcAxisValue(int raw_value);

	static const int JOY_DEADZONE = 6000;

	bool			mActive;

	typedef std::queue<event_t> EventQueue;
	EventQueue		mEvents;

	int				mJoystickId;
	SDL_GameController*	mJoystick;
};


// ============================================================================
//
// ISDL20InputSubsystem class interface
//
// ============================================================================

class ISDL20InputSubsystem : public IInputSubsystem
{
public:
	ISDL20InputSubsystem();
	virtual ~ISDL20InputSubsystem();

	virtual void grabInput();
	virtual void releaseInput();

	virtual bool isInputGrabbed() const
	{	return mInputGrabbed;	}

	virtual std::vector<IInputDeviceInfo> getKeyboardDevices() const;
	virtual void initKeyboard(int id);
	virtual void shutdownKeyboard(int id);

	virtual std::vector<IInputDeviceInfo> getMouseDevices() const;
	virtual void initMouse(int id);
	virtual void shutdownMouse(int id);

	virtual std::vector<IInputDeviceInfo> getJoystickDevices() const;
	virtual void initJoystick(int id);
	virtual void shutdownJoystick(int id);

private:
	bool				mInputGrabbed;
};

#endif	// SDL20
