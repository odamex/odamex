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
//	Music player classes for the supported music libraries
//
//-----------------------------------------------------------------------------

#pragma once

#include "i_midi.h"

/**
 * @brief Abstract base class that provides an interface for inheriting
 *        classes as well as default implementations for several functions.
 */
class MusicSystem
{
  public:
	MusicSystem() : m_isPlaying(false), m_isPaused(false), m_tempo(120.0f), m_volume(1.0f)
	{
	}
	virtual ~MusicSystem() { }

	virtual void startSong(byte* data, size_t length, bool loop);
	virtual void stopSong();
	virtual void pauseSong();
	virtual void resumeSong();
	virtual void playChunk() = 0;

	virtual void setVolume(float volume);
	float getVolume() const { return m_volume; }
	virtual void setTempo(float tempo);
	float getTempo() const { return m_tempo; }

	virtual bool isInitialized() const = 0;
	bool isPlaying() const { return m_isPlaying; }
	bool isPaused() const { return m_isPaused; }

	// Can this MusicSystem object play a particular type of music file?
	virtual bool isMusCapable() const { return false; }
	virtual bool isMidiCapable() const { return false; }
	virtual bool isOggCapable() const { return false; }
	virtual bool isMp3Capable() const { return false; }
	virtual bool isModCapable() const { return false; }
	virtual bool isWaveCapable() const { return false; }

  private:
	bool m_isPlaying;
	bool m_isPaused;

	float m_tempo;
	float m_volume;
};

/**
 * @brief This music system does not play any music.  It can be selected
 *        when the user wishes to disable music output.
 */
class SilentMusicSystem : public MusicSystem
{
  public:
	SilentMusicSystem()
	{
		Printf(PRINT_WARNING, "I_InitMusic: Music playback disabled.\n");
	}

	virtual void startSong(byte* data, size_t length, bool loop) { }
	virtual void stopSong() { }
	virtual void pauseSong() { }
	virtual void resumeSong() { }
	virtual void playChunk() { }
	virtual void setVolume(float volume) { }

	virtual bool isInitialized() const { return true; }

	// SilentMusicSystem can handle any type of music by doing nothing
	virtual bool isMusCapable() const { return true; }
	virtual bool isMidiCapable() const { return true; }
	virtual bool isOggCapable() const { return true; }
	virtual bool isMp3Capable() const { return true; }
	virtual bool isModCapable() const { return true; }
	virtual bool isWaveCapable() const { return true; }
};

/**
 * @brief Abstract base class that provides an interface for cross-platform
 *        midi libraries.
 *
 * @detail MidiMusicSystem handles parsing a lump containing a MUS or MIDI
 *         file and feeding each midi event to the library.  MidiMusicSystem
 *         does the heavy lifting for the subclasses that are based on it.
 */
class MidiMusicSystem : public MusicSystem
{
  public:
	MidiMusicSystem();
	virtual ~MidiMusicSystem();

	virtual void startSong(byte* data, size_t length, bool loop);
	virtual void stopSong();
	virtual void pauseSong();
	virtual void resumeSong();
	virtual void restartSong();

	virtual void playChunk();
	virtual void playEvent(int time, MidiEvent *event);
	virtual void setVolume(float volume);

	// Only plays midi-type music
	virtual bool isMusCapable() const { return true; }
	virtual bool isMidiCapable() const { return true; }

	virtual void writeVolume(int time, byte channel, byte volume) = 0;
	virtual void writeControl(int time, byte channel, byte control, byte value) = 0;
	virtual void writeChannel(int time, byte channel, byte status, byte param1, byte param2 = 0) = 0;
	virtual void writeSysEx(int time, const byte *data, size_t length) = 0;
	virtual void allNotesOff() = 0;
	virtual void allSoundOff() = 0;

  protected:
	static const int NUM_CHANNELS = 16;
	bool m_useResetDelay;

	void _InitFallback();
	void _EnableFallback();
	void _DisableFallback();

	unsigned int _GetLastEventTime() const { return m_lastEventTime; }

  private:
	MidiSong* m_midiSong;
	MidiSong::const_iterator m_songItr;
	bool m_loop;
	double msperclock;
	bool m_useFallback;
	midi_fallback_t m_fallback;

	unsigned int m_lastEventTime;
	int m_prevClockTime;

	void _ResetFallback();
};
