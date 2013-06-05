// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2006-2013 by The Odamex Team.
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

#define MOUSE_DOOM 0
#define MOUSE_ODAMEX 1
#define MOUSE_ZDOOM_DI 2

void I_InitMouseDriver();
void I_ShutdownMouseDriver();

bool I_InitInput (void);
void STACK_ARGS I_ShutdownInput (void);
void I_PauseMouse();
void I_ResumeMouse();

int I_GetJoystickCount();
std::string I_GetJoystickNameFromIndex (int index);
bool I_OpenJoystick();
void I_CloseJoystick();

void I_GetEvent (void);

void I_EnableKeyRepeat();
void I_DisableKeyRepeat();

// ============================================================================
//
// Mouse Drivers
//
// ============================================================================

class MouseInput;

enum
{
	SDL_MOUSE_DRIVER = 0,
	RAW_WIN32_MOUSE_DRIVER = 1,
	NUM_MOUSE_DRIVERS = 2
};

typedef struct
{
	int				id;
	const char*	name;
	bool 			(*avail_test)();
	MouseInput*		(*create)();
} MouseDriverInfo_t;

MouseDriverInfo_t* I_FindMouseDriverInfo(int id);
extern MouseDriverInfo_t MouseDriverInfo[];

class MouseInput
{
public:
	virtual ~MouseInput() { }

	virtual void processEvents() = 0;
	virtual void flushEvents() = 0;
	virtual void center() = 0;

	virtual bool paused() const = 0;
	virtual void pause() = 0;
	virtual void resume() = 0;

	virtual void debug() const { }
};

#if defined WIN32 && !defined _XBOX
	#define USE_RAW_WIN32_MOUSE
#else
	#undef USE_RAW_WIN32_MOUSE
#endif

#ifdef USE_RAW_WIN32_MOUSE
class RawWin32Mouse : public MouseInput
{
public:
	static MouseInput* create();
	virtual ~RawWin32Mouse();

	void processEvents();
	void flushEvents();
	void center();

	bool paused() const;
	void pause();
	void resume();

	void debug() const;

private:
	RawWin32Mouse();
	RawWin32Mouse(const RawWin32Mouse& other) { }
	RawWin32Mouse& operator=(const RawWin32Mouse& other) { return *this; }

	void registerWindowProc();
	void unregisterWindowProc();
	LRESULT CALLBACK windowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
	static LRESULT CALLBACK windowProcWrapper(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

	bool registerMouseDevice();
	bool unregisterMouseDevice();

	void backupMouseDevice(const RAWINPUTDEVICE& device);
	void restoreMouseDevice(RAWINPUTDEVICE& device) const;
	
	static RawWin32Mouse*	mInstance;

	bool					mActive;
	bool					mInitialized;

	RAWINPUTDEVICE			mBackupDevice;
	bool					mHasBackupDevice;
	bool					mRegisteredMouseDevice;	

	HWND					mWindow;
	WNDPROC					mBaseWindowProc;
	bool					mRegisteredWindowProc;

	static const size_t	QUEUE_CAPACITY = 256;
	RAWMOUSE				mInputQueue[QUEUE_CAPACITY];
	size_t					mQueueFront;
	size_t					mQueueBack;

	inline size_t queueSize() const
	{
		return (mQueueBack + QUEUE_CAPACITY - mQueueFront) % QUEUE_CAPACITY;
	}

	inline void pushBack(const RAWMOUSE* input)
	{
		if (queueSize() < QUEUE_CAPACITY)
		{
			memcpy(&mInputQueue[mQueueBack], input, sizeof(*input));
			mQueueBack = (mQueueBack + 1) % QUEUE_CAPACITY;
		}
	}

	inline void popFront()
	{
		if (queueSize() > 0)
			mQueueFront = (mQueueFront + 1) % QUEUE_CAPACITY;
	}

	inline const RAWMOUSE* front() const
	{
		if (queueSize() > 0)
			return &mInputQueue[mQueueFront];
		return NULL;
	}

	inline void clear()
	{
		mQueueFront = mQueueBack = 0;
	}
};
#endif  // USE_RAW_WIN32_MOUSE

class SDLMouse : public MouseInput
{
public:
	static MouseInput* create();
	virtual ~SDLMouse();

	void processEvents();
	void flushEvents();
	void center();

	bool paused() const;
	void pause();
	void resume();

	void debug() const;

private:
	SDLMouse();
	SDLMouse(const SDLMouse& other) { }
	SDLMouse& operator=(const SDLMouse& other) { return *this; }

	bool	mActive;

	static const int MAX_EVENTS = 256;
	SDL_Event	mEvents[MAX_EVENTS];
};

#endif  // __I_INPUT_H__
