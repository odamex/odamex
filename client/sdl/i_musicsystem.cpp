// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2006-2020 by The Odamex Team.
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
// Music player classes for the supported music libraries
//
//-----------------------------------------------------------------------------

#include "odamex.h"

#include <math.h>

#include "i_midi.h"
#include "i_music.h"
#include "i_musicsystem.h"
#include "i_sdl.h"
#include "i_system.h"
#include "mus2midi.h"

extern MusicSystem* musicsystem;
EXTERN_CVAR(snd_mididelay)
EXTERN_CVAR(snd_midifallback)

//
// I_CalculateMsPerMidiClock()
//
// Returns milliseconds per midi clock based on the current tempo and
// the time division value in the midi file's header.

static double I_CalculateMsPerMidiClock(int timeDivision, double tempo = 120.0)
{
	if (timeDivision & 0x8000)
	{
		// timeDivision is in SMPTE frames per second format
		double framespersecond = double((timeDivision & 0x7F00) >> 8);
		double ticksperframe = double((timeDivision & 0xFF));

		// [SL] 2011-12-23 - An fps value of 29 in timeDivision really implies
		// 29.97 fps.
		if (framespersecond == 29.0)
			framespersecond = 29.97;

		return 1000.0 / framespersecond / ticksperframe;
	}
	else
	{
		// timeDivision is in ticks per beat format
		double ticsperbeat = double(timeDivision & 0x7FFF);
		static double millisecondsperminute = 60.0 * 1000.0;
		double millisecondsperbeat = millisecondsperminute / tempo;

		return millisecondsperbeat / ticsperbeat;
	}
}

// ============================================================================
//
// MusicSystem base class functions
//
// ============================================================================

void MusicSystem::startSong(byte* data, size_t length, bool loop)
{
	m_isPlaying = true;
	m_isPaused = false;
}

void MusicSystem::stopSong()
{
	m_isPlaying = false;
	m_isPaused = false;
}

void MusicSystem::pauseSong()
{
	m_isPaused = m_isPlaying;
}

void MusicSystem::resumeSong()
{
	m_isPaused = false;
}

void MusicSystem::setTempo(float tempo)
{
	if (tempo > 0.0f)
		m_tempo = tempo;
}

void MusicSystem::setVolume(float volume)
{
	m_volume = clamp(volume, 0.0f, 1.0f);
}

// ============================================================================
//
// MidiMusicSystem non-member helper functions
//
// ============================================================================

//
// I_RegisterMidiSong()
//
// Returns a new MidiSong object, parsing the MUS or MIDI lump stored
// in data.
//
static MidiSong* I_RegisterMidiSong(byte* data, size_t length)
{
	byte* regdata = data;
	size_t reglength = length;
	MEMFILE *mus = NULL, *midi = NULL;

	// Convert from MUS format to MIDI format
	if (S_MusicIsMus(data, length))
	{
		mus = mem_fopen_read(data, length);
		midi = mem_fopen_write();

		int result = mus2mid(mus, midi);
		if (result == 0)
		{
			regdata = (byte*)mem_fgetbuf(midi);
			reglength = mem_fsize(midi);
		}
		else
		{
			Printf(PRINT_WARNING, "I_RegisterMidiSong: MUS is not valid\n");
			regdata = NULL;
			reglength = 0;
		}
	}
	else if (!S_MusicIsMidi(data, length))
	{
		Printf(PRINT_WARNING, "I_RegisterMidiSong: Only midi music formats are supported "
		                      "with the selected music system.\n");
		return NULL;
	}

	MidiSong* midisong = new MidiSong(regdata, reglength);

	if (mus)
		mem_fclose(mus);
	if (midi)
		mem_fclose(midi);

	return midisong;
}

//
// I_UnregisterMidiSong()
//
// Frees the memory allocated for a MidiSong object
//
static void I_UnregisterMidiSong(MidiSong* midisong)
{
	if (midisong)
		delete midisong;
}

// ============================================================================
//
// MidiMusicSystem
//
// Partially based on an implementation from prboom-plus by Nicholai Main (Natt).
// ============================================================================

MidiMusicSystem::MidiMusicSystem()
	: MusicSystem(), m_useResetDelay(false), m_midiSong(NULL), m_songItr(),
	  m_loop(false), msperclock(0.0), m_useFallback(false),
	  m_fallback(), m_lastEventTime(0), m_prevClockTime(0)
{
}

MidiMusicSystem::~MidiMusicSystem() { }

void MidiMusicSystem::_InitFallback()
{
	I_MidiInitFallback();
	m_fallback.type = MIDI_FALLBACK_NONE;
	m_fallback.value = 0;
}

void MidiMusicSystem::_ResetFallback()
{
	I_MidiResetFallback();
	m_fallback.type = MIDI_FALLBACK_NONE;
	m_fallback.value = 0;
}

void MidiMusicSystem::_EnableFallback()
{
	_ResetFallback();
	m_useFallback = (snd_midifallback.asInt() == 1);
}

void MidiMusicSystem::_DisableFallback()
{
	_ResetFallback();
	m_useFallback = false;
}

void MidiMusicSystem::startSong(byte* data, size_t length, bool loop)
{
	if (!isInitialized())
		return;

	stopSong();

	if (!data || !length)
		return;

	m_midiSong = I_RegisterMidiSong(data, length);
	if (!m_midiSong)
	{
		stopSong();
		return;
	}

	setTempo(120.0f);
	msperclock = I_CalculateMsPerMidiClock(m_midiSong->getTimeDivision(), getTempo());

	m_loop = loop;
	m_songItr = m_midiSong->begin();
	m_prevClockTime = 0;
	m_lastEventTime = I_MSTime();

	MusicSystem::startSong(data, length, loop);
}

void MidiMusicSystem::stopSong()
{
	I_UnregisterMidiSong(m_midiSong);
	m_midiSong = NULL;
	MusicSystem::stopSong();
}

void MidiMusicSystem::pauseSong()
{
	MusicSystem::pauseSong();
}

void MidiMusicSystem::resumeSong()
{
	MusicSystem::resumeSong();
	m_lastEventTime = I_MSTime();
}

void MidiMusicSystem::setVolume(float volume)
{
	MusicSystem::setVolume(volume);
}

void MidiMusicSystem::restartSong()
{
	m_songItr = m_midiSong->begin();
	m_prevClockTime = 0;
	m_lastEventTime = I_MSTime();
}

void MidiMusicSystem::playEvent(int time, MidiEvent *event)
{
	if (m_useFallback)
		I_MidiCheckFallback(event, &m_fallback);

	if (I_IsMidiMetaEvent(event))
	{
		MidiMetaEvent *metaevent = static_cast<MidiMetaEvent*>(event);

		if (metaevent->getMetaType() == MIDI_META_SET_TEMPO)
		{
			double tempo = I_GetTempoChange(metaevent);
			setTempo(tempo);
			msperclock = I_CalculateMsPerMidiClock(m_midiSong->getTimeDivision(), getTempo());
		}
	}
	else if (I_IsMidiSysexEvent(event) && event->getEventType() == MIDI_EVENT_SYSEX)
	{
		MidiSysexEvent *sysexevent = static_cast<MidiSysexEvent*>(event);
		const byte *data = sysexevent->getData();
		size_t length = sysexevent->getDataLength();

		if (length > 0 && data[length - 1] == MIDI_EVENT_SYSEX_SPLIT)
			writeSysEx(time, data, length);
	}
	else if (I_IsMidiControllerEvent(event))
	{
		MidiControllerEvent *ctrlevent = static_cast<MidiControllerEvent*>(event);
		byte channel = ctrlevent->getChannel();
		byte control = ctrlevent->getControllerType();
		byte param1 = ctrlevent->getParam1();

		switch (control)
		{
			case MIDI_CONTROLLER_VOLUME_MSB:
				writeVolume(time, channel, param1);
				break;

			case MIDI_CONTROLLER_VOLUME_LSB:
				// Volume controlled by MSB value only
				break;

			case MIDI_CONTROLLER_BANK_SELECT_LSB:
				param1 = (m_fallback.type == MIDI_FALLBACK_BANK_LSB) ? m_fallback.value : param1;
				writeControl(time, channel, control, param1);
				break;

			default:
				writeControl(time, channel, control, param1);
				break;
		}
	}
	else if (I_IsMidiChannelEvent(event))
	{
		MidiChannelEvent *chanevent = static_cast<MidiChannelEvent*>(event);
		byte status = event->getEventType();
		byte channel = chanevent->getChannel();
		byte param1 = chanevent->getParam1();
		byte param2 = chanevent->getParam2();

		if (status == MIDI_EVENT_PROGRAM_CHANGE)
		{
			switch (m_fallback.type)
			{
				case MIDI_FALLBACK_BANK_MSB:
					writeControl(time, channel, MIDI_CONTROLLER_BANK_SELECT_MSB, m_fallback.value);
					writeChannel(time, channel, status, param1, 0);
					break;

				case MIDI_FALLBACK_DRUMS:
					writeChannel(time, channel, status, m_fallback.value, 0);
					break;

				default: // No fallback
					writeChannel(time, channel, status, param1, 0);
					break;
			}
		}
		else
		{
			writeChannel(time, channel, status, param1, param2);
		}
	}
}

void MidiMusicSystem::playChunk()
{
	if (!isInitialized() || !m_midiSong || !isPlaying() || isPaused())
		return;

	unsigned int endtime = I_MSTime() + 1000 / TICRATE;

	while (m_songItr != m_midiSong->end())
	{
		MidiEvent* event = *m_songItr;
		if (!event)
			break;

		unsigned int deltatime = (event->getMidiClockTime() - m_prevClockTime) * msperclock;
		unsigned int eventplaytime = m_lastEventTime + deltatime;

		if (m_useResetDelay)
			eventplaytime += snd_mididelay;

		if (eventplaytime > endtime)
			break;

		playEvent(eventplaytime, event);

		m_useResetDelay = false;
		m_prevClockTime = event->getMidiClockTime();
		m_lastEventTime = eventplaytime;
		++m_songItr;
	}

	// At the end of the song.  Either stop or loop back to the begining
	if (m_songItr == m_midiSong->end())
	{
		if (m_loop)
			restartSong();
		else
			stopSong();
	}
}

VERSION_CONTROL(i_musicsystem_cpp, "$Id$")
