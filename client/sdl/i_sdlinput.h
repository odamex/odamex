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
//	
//
//-----------------------------------------------------------------------------

#ifndef __I_SDLINPUT_H__
#define __I_SDLINPUT_H__

#include "i_sdl.h"
#include "doomtype.h"
#include "i_input.h"

#include "d_event.h"
#include <queue>
#include <list>
#include "hashtable.h"

bool I_SDLMouseAvailible();

#ifdef SDL12

// ============================================================================
//
// ISDL12KeyboardInputDevice class interface
//
// ============================================================================

class ISDL12KeyboardInputDevice : public IInputDevice
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
	void initKeyTranslation();
	void initKeyTextTranslation();
	int translateKey(int sym);
	int translateKeyText(int sym, int mod);
	void center();

	bool					mActive;

	typedef std::queue<event_t> EventQueue;
	EventQueue				mEvents;

	typedef OHashTable<int, int> KeyTranslationTable;
	KeyTranslationTable		mSDLKeyTransTable;
	KeyTranslationTable		mSDLKeyTextTransTable;
	KeyTranslationTable		mShiftTransTable;
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

	virtual std::vector<IInputDeviceInfo> getKeyboardDevices() const;
	virtual void initKeyboard(int id);
	virtual void shutdownKeyboard(int id);
	virtual void pauseKeyboard();
	virtual void resumeKeyboard();

	virtual std::vector<IInputDeviceInfo> getMouseDevices() const;
	virtual void initMouse(int id);
	virtual void shutdownMouse(int id);
	virtual void pauseMouse();
	virtual void resumeMouse();

	virtual std::vector<IInputDeviceInfo> getJoystickDevices() const;
	virtual void initJoystick(int id);
	virtual void shutdownJoystick(int id);
	virtual void pauseJoystick();
	virtual void resumeJoystick();

private:
	bool				mInputGrabbed;
};

#endif	// SDL12

#ifdef SDL20

// ============================================================================
//
// ISDL20KeyboardInputDevice class interface
//
// ============================================================================

class ISDL20KeyboardInputDevice : public IInputDevice
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

private:
	void initKeyTranslation();
	void initKeyTextTranslation();
	int translateKey(int sym);
	int translateKeyText(int sym, int mod);
	void center();

	bool					mActive;

	typedef std::queue<event_t> EventQueue;
	EventQueue				mEvents;

	typedef OHashTable<int, int> KeyTranslationTable;
	KeyTranslationTable		mSDLKeyTransTable;
	KeyTranslationTable		mSDLKeyTextTransTable;
	KeyTranslationTable		mShiftTransTable;
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
	void center();

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

	virtual std::vector<IInputDeviceInfo> getKeyboardDevices() const;
	virtual void initKeyboard(int id);
	virtual void shutdownKeyboard(int id);
	virtual void pauseKeyboard();
	virtual void resumeKeyboard();

	virtual std::vector<IInputDeviceInfo> getMouseDevices() const;
	virtual void initMouse(int id);
	virtual void shutdownMouse(int id);
	virtual void pauseMouse();
	virtual void resumeMouse();

	virtual std::vector<IInputDeviceInfo> getJoystickDevices() const;
	virtual void initJoystick(int id);
	virtual void shutdownJoystick(int id);
	virtual void pauseJoystick();
	virtual void resumeJoystick();

private:
	bool				mInputGrabbed;
};

#endif	// SDL20

#endif	// __I_SDLINPUT_H__
