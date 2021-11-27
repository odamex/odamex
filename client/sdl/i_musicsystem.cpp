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

#include "i_system.h"
#include "i_midi.h"
#include "i_music.h"
#include "i_musicsystem.h"
#include "i_sdl.h"
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
	mIsPlaying = true;
	mIsPaused = false;
}

void MusicSystem::stopSong()
{
	mIsPlaying = false;
	mIsPaused = false;
}

void MusicSystem::pauseSong()
{
	mIsPaused = mIsPlaying;
}

void MusicSystem::resumeSong()
{
	mIsPaused = false;
}

void MusicSystem::setTempo(float tempo)
{
	if (tempo > 0.0f)
		mTempo = tempo;
}

void MusicSystem::setVolume(float volume)
{
	mVolume = clamp(volume, 0.0f, 1.0f);
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
    : MusicSystem(), mMidiSong(NULL), mSongItr(), mLoop(false), mTimeDivision(96),
      mLastEventTime(0), mPrevClockTime(0), mChannelVolume()
{
}

MidiMusicSystem::~MidiMusicSystem()
{
	_StopSong();

	I_UnregisterMidiSong(mMidiSong);
}

void MidiMusicSystem::_AllNotesOff()
{
	for (int i = 0; i < _GetNumChannels(); i++)
	{
		MidiControllerEvent event_noteoff(0, MIDI_CONTROLLER_ALL_NOTES_OFF, i);
		playEvent(&event_noteoff);
		MidiControllerEvent event_reset(0, MIDI_CONTROLLER_RESET_ALL, i);
		playEvent(&event_reset);
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

	mLoop = loop;

	mMidiSong = I_RegisterMidiSong(data, length);
	if (!mMidiSong)
	{
		stopSong();
		return;
	}

	MusicSystem::startSong(data, length, loop);
	_InitializePlayback();
}

void MidiMusicSystem::stopSong()
{
	I_UnregisterMidiSong(mMidiSong);
	mMidiSong = NULL;

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

	mLastEventTime = I_MSTime();

	MidiEvent* event = *mSongItr;
	if (event)
		mPrevClockTime = event->getMidiClockTime();
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
		mChannelVolume[channel] = clamp(volume, 0, 127);
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
		MidiControllerEvent event(0, MIDI_CONTROLLER_MAIN_VOLUME, i, mChannelVolume[i]);
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
	if (!mMidiSong)
		return;

	mLastEventTime = I_MSTime();

	// seek to the begining of the song
	mSongItr = mMidiSong->begin();
	mPrevClockTime = 0;

	// shut off all notes and reset all controllers
	_AllNotesOff();

	setTempo(120.0);

	// initialize all channel volumes to 100%
	for (int i = 0; i < _GetNumChannels(); i++)
		mChannelVolume[i] = 127;

	_RefreshVolume();
}

void MidiMusicSystem::playChunk()
{
	if (!isInitialized() || !mMidiSong || !isPlaying() || isPaused())
		return;

	unsigned int endtime = I_MSTime() + 1000 / TICRATE;

	while (mSongItr != mMidiSong->end())
	{
		MidiEvent* event = *mSongItr;
		if (!event)
			break;

		double msperclock =
		    I_CalculateMsPerMidiClock(mMidiSong->getTimeDivision(), getTempo());

		unsigned int deltatime =
		    (event->getMidiClockTime() - mPrevClockTime) * msperclock;

		unsigned int eventplaytime = mLastEventTime + deltatime;

		if (eventplaytime > endtime)
			break;

		playEvent(event, eventplaytime);

		mPrevClockTime = event->getMidiClockTime();
		mLastEventTime = eventplaytime;

		++mSongItr;
	}

	// At the end of the song.  Either stop or loop back to the begining
	if (mSongItr == mMidiSong->end())
	{
		if (!mLoop)
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
