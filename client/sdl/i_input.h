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

#ifndef __I_INPUT_H__
#define __I_INPUT_H__

#include "doomtype.h"
#include <SDL.h>
#include "win32inc.h"

#include "d_event.h"
#include "hashtable.h"
#include <queue>
#include <list>

#define MOUSE_DOOM 0
#define MOUSE_ZDOOM_DI 1

bool I_InitInput (void);
void STACK_ARGS I_ShutdownInput (void);
void I_PauseMouse();
void I_ResumeMouse();
void I_ForceUpdateGrab();
void I_FlushInput();

int I_GetJoystickCount();
std::string I_GetJoystickNameFromIndex (int index);
bool I_OpenJoystick();
void I_CloseJoystick();


// ============================================================================
//
// IInputDevice abstract base class interface
//
// ============================================================================

class IInputDevice
{
public:
	virtual ~IInputDevice() { }

	virtual bool active() const = 0;
	virtual void pause() = 0; 
	virtual void resume() = 0;
	virtual void reset() = 0;

	virtual void gatherEvents() = 0;
	virtual bool hasEvent() const = 0;
	virtual void getEvent(event_t* ev) = 0;

	virtual void flushEvents()
	{
		event_t ev;
		gatherEvents();
		while (hasEvent())
			getEvent(&ev);
	}
};


struct IInputDeviceInfo
{
	std::string		mDeviceName;
	int				mId;
};


// ============================================================================
//
// IInputSubsystem abstract base class interface
//
// ============================================================================

class IInputSubsystem
{
public:
	IInputSubsystem();
	virtual ~IInputSubsystem();

	virtual void grabInput() = 0;
	virtual void releaseInput() = 0;

	virtual void enableKeyRepeat();
	virtual void disableKeyRepeat();

	virtual void flushInput()
	{
		event_t dummy_event;
		gatherEvents();
		while (hasEvent())
			getEvent(&dummy_event);
	}

	virtual bool hasEvent() const
	{	return mEvents.empty() == false;	}

	virtual void gatherEvents();
	virtual void getEvent(event_t* ev);

	virtual std::vector<IInputDeviceInfo> getKeyboardDevices() const = 0; 
	virtual void initKeyboard(int id) = 0;
	virtual void shutdownKeyboard(int id) = 0;
	virtual void pauseKeyboard() = 0;
	virtual void resumeKeyboard() = 0;

	virtual std::vector<IInputDeviceInfo> getMouseDevices() const = 0; 
	virtual void initMouse(int id) = 0;
	virtual void shutdownMouse(int id) = 0;
	virtual void pauseMouse() = 0;
	virtual void resumeMouse() = 0;

	virtual std::vector<IInputDeviceInfo> getJoystickDevices() const = 0; 
	virtual void initJoystick(int id) = 0;
	virtual void shutdownJoystick(int id) = 0;
	virtual void pauseJoystick() = 0;
	virtual void resumeJoystick() = 0;

protected:
	void registerInputDevice(IInputDevice* device);
	void unregisterInputDevice(IInputDevice* device);

	void setKeyboardInputDevice(IInputDevice* device)
	{
		mKeyboardInputDevice = device;
	}

	IInputDevice* getKeyboardInputDevice()
	{
		return mKeyboardInputDevice;
	}

	void setMouseInputDevice(IInputDevice* device)
	{
		mMouseInputDevice = device;
	}

	IInputDevice* getMouseInputDevice()
	{
		return mMouseInputDevice;
	}

	void setJoystickInputDevice(IInputDevice* device)
	{
		mJoystickInputDevice = device;
	}

	IInputDevice* getJoystickInputDevice()
	{
		return mJoystickInputDevice;
	}

	static const uint64_t	mRepeatDelay;
	static const uint64_t	mRepeatInterval;

private:
	void addToEventRepeaters(event_t& ev);
	void repeatEvents();

	// Data for key repeating
	struct EventRepeater
	{
		uint64_t	last_time;
		bool		repeating;
		event_t		event;
	};

	// the EventRepeaterTable hashtable typedef uses
	// event_t::data1 as its key as there should only be
	// a single instance with that value in the table.
	typedef OHashTable<int, EventRepeater> EventRepeaterTable;
	EventRepeaterTable	mEventRepeaters;

	bool				mRepeating;

	typedef std::queue<event_t> EventQueue;
	EventQueue			mEvents;

	// Input device management
	typedef std::list<IInputDevice*> InputDeviceList;
	InputDeviceList		mInputDevices;

	IInputDevice*		mKeyboardInputDevice;
	IInputDevice*		mMouseInputDevice;
	IInputDevice*		mJoystickInputDevice;
};


// ============================================================================
//
// Mouse Driver selection declarations
//
// ============================================================================
enum
{
	SDL_MOUSE_DRIVER = 0,
	RAW_WIN32_MOUSE_DRIVER = 1,
	NUM_MOUSE_DRIVERS = 2
};

typedef struct
{
	int				id;
	const char*		name;
	bool 			(*avail_test)();
} MouseDriverInfo_t;

MouseDriverInfo_t* I_FindMouseDriverInfo(int id);
extern MouseDriverInfo_t MouseDriverInfo[];

#endif  // __I_INPUT_H__
