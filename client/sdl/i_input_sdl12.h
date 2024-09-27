// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2006-2024 by The Odamex Team.
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

#ifdef SDL12

// ============================================================================
//
// ISDL12KeyboardInputDevice class interface
//
// ============================================================================

class ISDL12KeyboardInputDevice : public IKeyboardInputDevice
{
public:
	ISDL12KeyboardInputDevice(int id);
	virtual ~ISDL12KeyboardInputDevice();

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
	int translateKey(SDL_keysym keysym);

	bool					mActive;

	typedef std::queue<event_t> EventQueue;
	EventQueue				mEvents;
};


// ============================================================================
//
// ISDL12MouseInputDevice class interface
//
// ============================================================================

class ISDL12MouseInputDevice : public IInputDevice
{
public:
	ISDL12MouseInputDevice(int id);
	virtual ~ISDL12MouseInputDevice();

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
	void center();

	bool			mActive;

	typedef std::queue<event_t> EventQueue;
	EventQueue		mEvents;
};


// ============================================================================
//
// ISDL12JoystickInputDevice class interface
//
// ============================================================================

class ISDL12JoystickInputDevice : public IInputDevice
{
public:
	ISDL12JoystickInputDevice(int id);
	virtual ~ISDL12JoystickInputDevice();

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
	static const int JOY_DEADZONE = 6000;

	bool			mActive;

	typedef std::queue<event_t> EventQueue;
	EventQueue		mEvents;

	int				mJoystickId;
	SDL_Joystick*	mJoystick;

	int				mNumHats;
	int*			mHatStates;
};


// ============================================================================
//
// ISDL12InputSubsystem class interface
//
// ============================================================================

class ISDL12InputSubsystem : public IInputSubsystem
{
public:
	ISDL12InputSubsystem();
	virtual ~ISDL12InputSubsystem();

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

#endif	// SDL12
