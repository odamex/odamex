// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2006-2026 by The Odamex Team.
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
#include "hashtable.h"

using KeyTranslationTable = OHashTable<int, int>;

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
	~ISDL20KeyboardInputDevice() override;

	[[nodiscard]] bool active() const override;

	void pause() override;
	void resume() override;
	void reset() override;

	void gatherEvents() override;

	[[nodiscard]] bool hasEvent() const override
	{	return !mEvents.empty();	}

	void getEvent(event_t* ev) override;

	void flushEvents() override;

	void enableTextEntry() override;
	void disableTextEntry() override;

private:
	int translateKey(SDL_Keysym keysym);
	int getTextEventValue();

	bool					mActive;
	bool					mTextEntry;

	using EventQueue = std::queue<event_t>;
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
	~ISDL20MouseInputDevice() override;

	[[nodiscard]] bool active() const override;

	void pause() override;
	void resume() override;
	void resumeUI() override;
	void reset() override;

	void gatherEvents() override;

	[[nodiscard]] bool hasEvent() const override
	{	return !mEvents.empty();	}

	void getEvent(event_t* ev) override;

	void flushEvents() override;

private:
	void enableEvents(bool relative);

	bool			mActive;

	bool			mUIMode;

	using EventQueue = std::queue<event_t>;
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
	~ISDL20JoystickInputDevice() override;

	[[nodiscard]] bool active() const override;

	void pause() override;
	void resume() override;
	void reset() override;

	void gatherEvents() override;

	[[nodiscard]] bool hasEvent() const override
	{	return !mEvents.empty();	}

	void getEvent(event_t* ev) override;

	void flushEvents() override;

private:
	int calcAxisValue(int raw_value);

	static constexpr int JOY_DEADZONE = 6000;

	bool			mActive;

	using EventQueue = std::queue<event_t>;
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
	~ISDL20InputSubsystem() override;

	void grabInput() override;
	void releaseInput() override;
	void grabInputForUI() override;

	[[nodiscard]] bool isInputGrabbed() const override
	{	return mInputGrabbed;	}

	[[nodiscard]] std::vector<IInputDeviceInfo> getKeyboardDevices() const override;
	void initKeyboard(int id) override;
	void shutdownKeyboard(int id) override;

	[[nodiscard]] std::vector<IInputDeviceInfo> getMouseDevices() const override;
	void initMouse(int id) override;
	void shutdownMouse(int id) override;

	[[nodiscard]] std::vector<IInputDeviceInfo> getJoystickDevices() const override;
	void initJoystick(int id) override;
	void shutdownJoystick(int id) override;

private:
	bool				mInputGrabbed;
};

#endif	// SDL20
