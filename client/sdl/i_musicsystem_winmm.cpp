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
//  Plays MIDI music on Windows with winmm with volume adjustment.
//
//-----------------------------------------------------------------------------

#if defined(WIN32) && !defined(_XBOX)

// [AM] This is a new strategy for allowing control over MIDI volume that
//      originated in Woof!  The mechanism is similar to PortMidi in that
//      MIDI events have their volume changed, but this code feeds WinMM
//      directly in a job thread instead of roundtripping events through
//      PortMidi first.

#include "odamex.h"

#include "i_musicsystem_winmm.h"

// Macros for use with the Windows MIDIEVENT dwEvent field.

#define MIDIEVENT_CHANNEL(x) (x & 0x0000000F)
#define MIDIEVENT_TYPE(x) (x & 0x000000F0)
#define MIDIEVENT_DATA1(x) ((x & 0x0000FF00) >> 8)
#define MIDIEVENT_VOLUME(x) ((x & 0x007F0000) >> 16)

static const int g_volumeCorrection[] = {
    0,   4,   7,   11,  13,  14,  16,  18,  21,  22,  23,  24,  24,  24,  25,  25,
    25,  26,  26,  27,  27,  27,  28,  28,  29,  29,  29,  30,  30,  31,  31,  32,
    32,  32,  33,  33,  34,  34,  35,  35,  36,  37,  37,  38,  38,  39,  39,  40,
    40,  41,  42,  42,  43,  43,  44,  45,  45,  46,  47,  47,  48,  49,  49,  50,
    51,  52,  52,  53,  54,  55,  56,  56,  57,  58,  59,  60,  61,  62,  62,  63,
    64,  65,  66,  67,  68,  69,  70,  71,  72,  73,  74,  75,  77,  78,  79,  80,
    81,  82,  84,  85,  86,  87,  89,  90,  91,  92,  94,  95,  96,  98,  99,  101,
    102, 104, 105, 107, 108, 110, 112, 113, 115, 117, 118, 120, 122, 123, 125, 127};

// Message box for midiStream errors.

static void MidiErrorMessage(const DWORD error)
{
	char errorBuf[MAXERRORLENGTH];

	MMRESULT mmr = midiOutGetErrorText(error, (LPSTR)errorBuf, MAXERRORLENGTH);
	if (mmr == MMSYSERR_NOERROR)
	{
		Printf(PRINT_WARNING, "midiStream: %s\n", errorBuf);
	}
	else
	{
		Printf(PRINT_WARNING, "midiStream: Unknown error 0x%x\n", error);
	}
}

/**
 * @brief Initialize music.
 */
WinMMMusicSystem::WinMMMusicSystem()
    : m_initialized(false), m_midiStream(NULL), m_bufferReturnEvent(NULL),
      m_exitEvent(NULL), m_playerThread(NULL)
{
	// [AM] Update this if m_buffer gets any non-POD members.
	memset(m_buffer.events, 0x0, sizeof(m_buffer.events));

	UINT MidiDevice = MIDI_MAPPER;
	MIDIHDR* hdr = &m_buffer.MidiStreamHdr;
	MMRESULT mmr;

	mmr = midiStreamOpen(&m_midiStream, &MidiDevice, static_cast<DWORD>(1),
	                     reinterpret_cast<DWORD_PTR>(WinMMMusicSystem::midiStreamProc),
	                     reinterpret_cast<DWORD_PTR>(this), CALLBACK_FUNCTION);
	if (mmr != MMSYSERR_NOERROR)
	{
		MidiErrorMessage(mmr);
		return;
	}

	hdr->lpData = (LPSTR)m_buffer.events;
	hdr->dwBytesRecorded = 0;
	hdr->dwBufferLength = STREAM_MAX_EVENTS * sizeof(native_event_t);
	hdr->dwFlags = 0;
	hdr->dwOffset = 0;

	mmr = midiOutPrepareHeader(reinterpret_cast<HMIDIOUT>(m_midiStream), hdr,
	                           sizeof(MIDIHDR));
	if (mmr != MMSYSERR_NOERROR)
	{
		MidiErrorMessage(mmr);
		return;
	}

	m_bufferReturnEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	m_exitEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	m_initialized = true;
}

/**
 * @brief Shutdown music system.
 */
WinMMMusicSystem::~WinMMMusicSystem()
{
	MIDIHDR* hdr = &m_buffer.MidiStreamHdr;
	MMRESULT mmr;

	stopSong();

	mmr = midiOutUnprepareHeader((HMIDIOUT)m_midiStream, hdr, sizeof(MIDIHDR));
	if (mmr != MMSYSERR_NOERROR)
	{
		MidiErrorMessage(mmr);
	}

	mmr = midiStreamClose(m_midiStream);
	if (mmr != MMSYSERR_NOERROR)
	{
		MidiErrorMessage(mmr);
	}

	m_midiStream = NULL;

	CloseHandle(m_bufferReturnEvent);
	CloseHandle(m_exitEvent);
}

void WinMMMusicSystem::startSong(byte* data, size_t length, bool loop)
{
	MidiMusicSystem::startSong(data, length, loop);

	m_playerThread = CreateThread(
	    NULL, 0, reinterpret_cast<LPTHREAD_START_ROUTINE>(playerProc), this, 0, 0);
	if (m_playerThread == NULL)
	{
		Printf(PRINT_WARNING, "WinMMMusicSystem: Could not create thread for music.\n");
		return;
	}
	SetThreadPriority(m_playerThread, THREAD_PRIORITY_TIME_CRITICAL);

	MMRESULT mmr = midiStreamRestart(m_midiStream);
	if (mmr != MMSYSERR_NOERROR)
	{
		MidiErrorMessage(mmr);
	}
}

void WinMMMusicSystem::stopSong()
{
	MMRESULT mmr;

	if (m_playerThread)
	{
		SetEvent(m_exitEvent);
		WaitForSingleObject(m_playerThread, INFINITE);

		CloseHandle(m_playerThread);
		m_playerThread = NULL;
	}

	mmr = midiStreamStop(m_midiStream);
	if (mmr != MMSYSERR_NOERROR)
	{
		MidiErrorMessage(mmr);
	}
	mmr = midiOutReset((HMIDIOUT)m_midiStream);
	if (mmr != MMSYSERR_NOERROR)
	{
		MidiErrorMessage(mmr);
	}

	MidiMusicSystem::stopSong();
}

/**
 * @brief Fill the buffer with MIDI events, adjusting the volume as needed.
 */
void WinMMMusicSystem::fillBuffer()
{
	int i;

	for (i = 0; i < STREAM_MAX_EVENTS; ++i)
	{
		native_event_t* event = &m_buffer.events[i];

		if (mSongItr == mMidiSong->end())
		{
			if (mLoop)
			{
				mSongItr = mMidiSong->begin();
			}
			else
			{
				break;
			}
		}

		MidiEvent* odaEvent = *mSongItr;
		event->dwDeltaTime = odaEvent->getMidiClockTime();
		event->dwStreamID = 0;
		event->dwEvent = odaEvent->getEventType();

		if (MIDIEVENT_TYPE(event->dwEvent) == MIDI_EVENT_CONTROLLER &&
		    MIDIEVENT_DATA1(event->dwEvent) == MIDI_CONTROLLER_MAIN_VOLUME)
		{
			int volume = MIDIEVENT_VOLUME(event->dwEvent);

			_SetChannelVolume(MIDIEVENT_CHANNEL(event->dwEvent), volume);

			// volume *= volume_factor;

			volume = ::g_volumeCorrection[volume];

			event->dwEvent = (event->dwEvent & 0xFF00FFFF) | ((volume & 0x7F) << 16);
		}

		++mSongItr;
	}

	m_buffer.num_events = i;
}

/**
 * @brief Queue MIDI events.
 */
void WinMMMusicSystem::streamOut()
{
	MIDIHDR* hdr = &m_buffer.MidiStreamHdr;
	int num_events = m_buffer.num_events;

	if (num_events == 0)
	{
		return;
	}

	hdr->lpData = reinterpret_cast<LPSTR>(m_buffer.events);
	hdr->dwBytesRecorded = num_events * sizeof(native_event_t);

	MMRESULT mmr = midiStreamOut(m_midiStream, hdr, sizeof(MIDIHDR));
	if (mmr != MMSYSERR_NOERROR)
	{
		MidiErrorMessage(mmr);
	}
}

/**
 * @brief Stub for WinAPI callback.
 */
void CALLBACK WinMMMusicSystem::midiStreamProc(HMIDIIN hMidi, UINT uMsg,
                                               DWORD_PTR dwInstance, DWORD_PTR dwParam1,
                                               DWORD_PTR dwParam2)
{
	if (dwInstance)
	{
		reinterpret_cast<WinMMMusicSystem*>(dwInstance)
		    ->_midiStreamProc(hMidi, uMsg, dwParam1, dwParam2);
	}
}

/**
 * @brief Handle done messages in MIDI stream.
 */
void WinMMMusicSystem::_midiStreamProc(const HMIDIIN hMidi, const UINT uMsg,
                                       const DWORD_PTR dwParam1, const DWORD_PTR dwParam2)
{
	if (uMsg == MOM_DONE)
	{
		SetEvent(m_bufferReturnEvent);
	}
}

/**
 * @brief Stub for WinAPI thead entry point.
 */
DWORD WINAPI WinMMMusicSystem::playerProc(LPVOID lpParameter)
{
	if (lpParameter)
	{
		return reinterpret_cast<WinMMMusicSystem*>(lpParameter)->_playerProc();
	}
	else
	{
		return 0;
	}
}

/**
 * @brief The Windows API documentation states: "Applications should not
 *        call any multimedia functions from inside the callback function,
 *        as doing so can cause a deadlock." We use thread to avoid possible
 *        deadlocks.
 */
DWORD WinMMMusicSystem::_playerProc()
{
	HANDLE events[2] = {m_bufferReturnEvent, m_exitEvent};

	while (1)
	{
		switch (WaitForMultipleObjects(2, events, FALSE, INFINITE))
		{
		case WAIT_OBJECT_0:
			fillBuffer();
			streamOut();
			break;

		case WAIT_OBJECT_0 + 1:
			return 0;
		}
	}
	return 0;
}

#endif
