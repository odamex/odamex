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

#ifdef WIN32
#define DIRECTINPUT_VERSION 0x800
#define _WIN32_WINNT 0x0501	// necessary?
#include <windows.h>
#include <dinput.h>
#endif

#define MOUSE_DOOM 0
#define MOUSE_ODAMEX 1
#define MOUSE_ZDOOM_DI 2

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

#if defined WIN32 && !defined _XBOX
void BackupGDIMouseSettings();
void STACK_ARGS RestoreGDIMouseSettings();
#endif

enum
{
	SDL_MOUSE_DRIVER = 0,
	DI_MOUSE_DRIVER = 1
};

class MouseInput
{
public:
	virtual void processEvents() = 0;
	virtual void flushEvents() = 0;
	virtual void center() = 0;

	virtual bool paused() const = 0;
	virtual void pause() = 0;
	virtual void resume() = 0;
};


#ifdef WIN32
class DirectInputMouse : public MouseInput
{
public:
	DirectInputMouse();
	virtual ~DirectInputMouse();

	static MouseInput* create();

	void processEvents();
	void flushEvents();
	void center();

	bool paused() const;
	void pause();
	void resume();

private:
	bool	mActive;

	static bool				mInitialized;
};
#endif

class SDLMouse : public MouseInput
{
public:
	SDLMouse();
	virtual ~SDLMouse();

	static MouseInput* create();

	void processEvents();
	void flushEvents();
	void center();

	bool paused() const;
	void pause();
	void resume();

private:
	bool	mActive;

	static const int MAX_EVENTS = 256;
	SDL_Event	mEvents[MAX_EVENTS];
};


#endif
