// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2006-2021 by The Odamex Team.
// Copyright (C) 2021 Roman Fomin
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
//  Plays MIDI music on Windows with winmm using trickery to adjust volume.
//
//-----------------------------------------------------------------------------

#pragma once

#if defined(WIN32) && !defined(_XBOX)

#include "odamex.h"

#include "win32inc.h"
#include <mmeapi.h>
#include "i_musicsystem.h"

class WinMMMusicSystem : public MidiMusicSystem
{
	WinMMMusicSystem();
	virtual ~WinMMMusicSystem();
	virtual void startSong(byte* data, size_t length, bool loop);

  private:
	static const size_t STREAM_MAX_EVENTS = 4;

	/**
	 * This is a reduced Windows MIDIEVENT structure for MEVT_F_SHORT type
	 * of events.
	 */
	struct native_event_t
	{
		DWORD dwDeltaTime;
		DWORD dwStreamID; // always 0
		DWORD dwEvent;
	};

	struct buffer_t
	{
		native_event_t events[STREAM_MAX_EVENTS];
		int num_events;
		MIDIHDR MidiStreamHdr;
	};

	buffer_t m_buffer;
	bool m_initialized;
	HMIDISTRM m_midiStream;
	HANDLE m_bufferReturnEvent;
	HANDLE m_exitEvent;
	HANDLE m_playerThread;

	void fillBuffer();
	void streamOut();
	static void CALLBACK midiStreamProc(HMIDIIN hMidi, UINT uMsg, DWORD_PTR dwInstance,
	                                    DWORD_PTR dwParam1, DWORD_PTR dwParam2);
	void _midiStreamProc(const HMIDIIN hMidi, const UINT uMsg, const DWORD_PTR dwParam1,
	                     const DWORD_PTR dwParam2);
	static DWORD WINAPI playerProc(LPVOID lpParameter);
	DWORD _playerProc();
};

#endif
