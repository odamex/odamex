// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: i_sdlinput.h 5302 2015-04-23 01:53:13Z dr_sean $
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

#include "doomtype.h"
#include <SDL.h>
#include "i_input.h"

#include "d_event.h"
#include <queue>
#include "hashtable.h"


class ISDL12KeyboardInputDevice : public IInputDevice
{
public:
	ISDL12KeyboardInputDevice();
	virtual ~ISDL12KeyboardInputDevice() { }

	virtual const std::string& getDeviceName() const
	{	static const std::string name("Keyboard"); return name;	}

	virtual bool paused() const
	{	return mActive == false;		}

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

	bool					mActive;

	typedef std::queue<SDL_Event> EventQueue;
	EventQueue				mEvents;

	typedef OHashTable<int, int> KeyTranslationTable;
	KeyTranslationTable		mSDLKeyTransTable;
	KeyTranslationTable		mShiftTransTable;
};


class ISDL12MouseInputDevice : public IInputDevice
{
public:
	ISDL12MouseInputDevice();
	virtual ~ISDL12MouseInputDevice() { }

	virtual const std::string& getDeviceName() const
	{	static const std::string name("Mouse"); return name;	}

	virtual bool paused() const
	{	return mActive == false;		}

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

	typedef std::queue<SDL_Event> EventQueue;
	EventQueue		mEvents;
};


class ISDL12JoystickInputDevice : public IInputDevice
{
public:
	ISDL12JoystickInputDevice(int id);
	virtual ~ISDL12JoystickInputDevice();

	virtual const std::string& getDeviceName() const
	{	return mDeviceName;	}

	virtual bool paused() const
	{	return mActive == false;		}

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

	std::string		mDeviceName;
	bool			mActive;

	typedef std::queue<SDL_Event> EventQueue;
	EventQueue		mEvents;

	int				mJoystickId;
	SDL_Joystick*	mJoystick;
};


#endif	// __I_SDLINPUT_H__
