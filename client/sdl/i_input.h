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

#define MOUSE_DOOM 0
#define MOUSE_ZDOOM_DI 1

void I_InitMouseDriver();
void I_ShutdownMouseDriver();

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

void I_GetEvent (void);

void I_EnableKeyRepeat();
void I_DisableKeyRepeat();


// ============================================================================
//
// IInputDevice abstract base class interface
//
// ============================================================================

class IInputDevice
{
public:
	virtual ~IInputDevice() { }

	virtual const std::string& getDeviceName() const = 0;

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


// ============================================================================
//
// IInputSubsystem abstract base class interface
//
// ============================================================================

class IInputSubsystem
{
public:
	virtual ~IInputSubsystem() { }

	virtual void grabInput() = 0;
	virtual void releaseInput() = 0;

	virtual void enableKeyRepeat() { }
	virtual void disableKeyRepeat() { }

	virtual void flushInput();

	virtual void aggregateMouseMovement() = 0;

private:


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
	IInputDevice*	(*create)();
} MouseDriverInfo_t;

MouseDriverInfo_t* I_FindMouseDriverInfo(int id);
extern MouseDriverInfo_t MouseDriverInfo[];

#endif  // __I_INPUT_H__
