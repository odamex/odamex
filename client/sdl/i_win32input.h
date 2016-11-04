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

#ifndef __I_WIN32INPUT_H__
#define __I_WIN32INPUT_H__

#include "doomtype.h"
#include "win32inc.h"
#include "i_input.h"

#include "d_event.h"
#include <queue>

bool I_RawWin32MouseAvailible();

#if defined _WIN32 && !defined _XBOX
	#define USE_RAW_WIN32_MOUSE
#else
	#undef USE_RAW_WIN32_MOUSE
#endif

#ifdef USE_RAW_WIN32_MOUSE

// ============================================================================
//
// IRawWin32MouseInputDevice class interface
//
// ============================================================================

class IRawWin32MouseInputDevice : public IInputDevice
{
public:
	IRawWin32MouseInputDevice(int id);
	virtual ~IRawWin32MouseInputDevice();

	virtual const std::string& getDeviceName() const
	{	static const std::string name("Mouse"); return name;	}

	virtual bool active() const
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
	void debug() const;

	void registerWindowProc();
	void unregisterWindowProc();
	LRESULT CALLBACK windowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
	static LRESULT CALLBACK windowProcWrapper(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

	bool registerMouseDevice();
	bool unregisterMouseDevice();

	void backupMouseDevice(const RAWINPUTDEVICE& device);
	void restoreMouseDevice(RAWINPUTDEVICE& device) const;

	static IRawWin32MouseInputDevice*	mInstance;

	bool					mActive;

	typedef std::queue<event_t> EventQueue;
	EventQueue				mEvents;

	bool					mInitialized;

	RAWINPUTDEVICE			mBackupDevice;
	bool					mHasBackupDevice;
	bool					mRegisteredMouseDevice;

	HWND					mWindow;
	WNDPROC					mBaseWindowProc;
	bool					mRegisteredWindowProc;

	int						mAggregateX;
	int						mAggregateY;
};
#endif	// USE_RAW_WIN32_MOUSE

#endif	// __I_WIN32INPUT_H__
