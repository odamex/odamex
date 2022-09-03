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
    : MusicSystem(), m_midiSong(NULL), m_songItr(), m_loop(false), m_timeDivision(96),
      m_lastEventTime(0), m_prevClockTime(0), m_channelVolume()
{
}

MidiMusicSystem::~MidiMusicSystem()
{
	_StopSong();

	I_UnregisterMidiSong(m_midiSong);
}

void MidiMusicSystem::_AllNotesOff()
{
	for (int i = 0; i < _GetNumChannels(); i++)
	{
		MidiControllerEvent event_noteoff(0, MIDI_CONTROLLER_ALL_NOTES_OFF, i);
		playEvent(&event_noteoff);
		MidiControllerEvent event_reset(0, MIDI_CONTROLLER_RESET_ALL, i);
		playEvent(&event_reset);

		// pitch bend range (+/- 2 semitones)
		MidiControllerEvent event_rpn_msb_start(0, MIDI_CONTROLLER_RPN_MSB, i);
		playEvent(&event_rpn_msb_start);
		MidiControllerEvent event_rpn_lsb_start(0, MIDI_CONTROLLER_RPN_LSB, i);
		playEvent(&event_rpn_lsb_start);
		MidiControllerEvent event_data_msb(0, MIDI_CONTROLLER_DATA_ENTRY_MSB, i, 2);
		playEvent(&event_data_msb);
		MidiControllerEvent event_data_lsb(0, MIDI_CONTROLLER_DATA_ENTRY_LSB, i);
		playEvent(&event_data_lsb);
		MidiControllerEvent event_rpn_lsb_end(0, MIDI_CONTROLLER_RPN_LSB, i, 127);
		playEvent(&event_rpn_lsb_end);
		MidiControllerEvent event_rpn_msb_end(0, MIDI_CONTROLLER_RPN_MSB, i, 127);
		playEvent(&event_rpn_msb_end);

		// channel volume and panning
		MidiControllerEvent event_volume(0, MIDI_CONTROLLER_MAIN_VOLUME, i, 100);
		playEvent(&event_volume);
		MidiControllerEvent event_pan(0, MIDI_CONTROLLER_PAN, i, 64);
		playEvent(&event_pan);

		// effect controllers
		MidiControllerEvent event_reverb(0, MIDI_CONTROLLER_REVERB, i, 40);
		playEvent(&event_reverb);
		MidiControllerEvent event_tremolo(0, MIDI_CONTROLLER_TREMOLO, i);
		playEvent(&event_tremolo);
		MidiControllerEvent event_chorus(0, MIDI_CONTROLLER_CHORUS, i);
		playEvent(&event_chorus);
		MidiControllerEvent event_detune(0, MIDI_CONTROLLER_DETUNE, i);
		playEvent(&event_detune);
		MidiControllerEvent event_phaser(0, MIDI_CONTROLLER_PHASER, i);
		playEvent(&event_phaser);
	}
}

void MidiMusicSystem::_StopSong() { }

void MidiMusicSystem::startSong(byte* data, size_t length, bool loop)
{
	if (!isInitialized())
		return;

	stopSong();

	if (!data || !length)
		return;

	m_loop = loop;

	m_midiSong = I_RegisterMidiSong(data, length);
	if (!m_midiSong)
	{
		stopSong();
		return;
	}

	MusicSystem::startSong(data, length, loop);
	_InitializePlayback();
}

void MidiMusicSystem::stopSong()
{
	I_UnregisterMidiSong(m_midiSong);
	m_midiSong = NULL;

	_AllNotesOff();
	MusicSystem::stopSong();
}

void MidiMusicSystem::pauseSong()
{
	_AllNotesOff();

	MusicSystem::pauseSong();
}

void MidiMusicSystem::resumeSong()
{
	MusicSystem::resumeSong();

	m_lastEventTime = I_MSTime();

	MidiEvent* event = *m_songItr;
	if (event)
		m_prevClockTime = event->getMidiClockTime();
}

//
// MidiMusicSystem::setVolume
//
// Sanity checks the volume parameter and then inserts a midi controller
// event to change the volume for all of the channels.
//
void MidiMusicSystem::setVolume(float volume)
{
	MusicSystem::setVolume(volume);
	_RefreshVolume();
}

//
// MidiMusicSystem::_GetScaledVolume
//
// Returns the volume scaled logrithmically so that the for each unit the volume
// increases, the perceived volume increases linearly.
//
float MidiMusicSystem::_GetScaledVolume()
{
	// [SL] mimic the volume curve of midiOutSetVolume, as used by SDL_Mixer
	return pow(MusicSystem::getVolume(), 0.5f);
}

//
// _SetChannelVolume()
//
// Updates the array that tracks midi volume events.  Note that channel
// is 0-indexed (0 - 15).
//
void MidiMusicSystem::_SetChannelVolume(int channel, int volume)
{
	if (channel >= 0 && channel < _GetNumChannels())
		m_channelVolume[channel] = clamp(volume, 0, 127);
}

//
// _RefreshVolume()
//
// Sends out a volume controller event to change the volume to the current
// cached volume for the indicated channel.
//
void MidiMusicSystem::_RefreshVolume()
{
	for (int i = 0; i < _GetNumChannels(); i++)
	{
		MidiControllerEvent event(0, MIDI_CONTROLLER_MAIN_VOLUME, i, m_channelVolume[i]);
		playEvent(&event);
	}
}

//
// _InitializePlayback()
//
// Resets all of the variables used during playChunk() to determine the timing
// of midi events as well as the event iterator.  This should be called at the
// start of playback or when looping back to the beginning of the song.
//
void MidiMusicSystem::_InitializePlayback()
{
	if (!m_midiSong)
		return;

	m_lastEventTime = I_MSTime();

	// seek to the begining of the song
	m_songItr = m_midiSong->begin();
	m_prevClockTime = 0;

	// shut off all notes and reset all controllers
	_AllNotesOff();

	setTempo(120.0);

	// initialize all channel volumes to 100%
	for (int i = 0; i < _GetNumChannels(); i++)
		m_channelVolume[i] = 127;

	_RefreshVolume();
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

		double msperclock =
		    I_CalculateMsPerMidiClock(m_midiSong->getTimeDivision(), getTempo());

		unsigned int deltatime =
		    (event->getMidiClockTime() - m_prevClockTime) * msperclock;

		unsigned int eventplaytime = m_lastEventTime + deltatime;

		if (eventplaytime > endtime)
			break;

		playEvent(event, eventplaytime);

		m_prevClockTime = event->getMidiClockTime();
		m_lastEventTime = eventplaytime;

		++m_songItr;
	}

	// At the end of the song.  Either stop or loop back to the begining
	if (m_songItr == m_midiSong->end())
	{
		if (!m_loop)
		{
			stopSong();
			return;
		}
		else
		{
			_InitializePlayback();
			return;
		}
	}
}

VERSION_CONTROL(i_musicsystem_cpp, "$Id$")
