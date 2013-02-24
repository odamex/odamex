// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id: i_musicsystem.h 1788 2010-08-24 04:42:57Z russellrice $
//
// Copyright (C) 2006-2012 by The Odamex Team.
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

#ifndef __I_MUSICSYSTEM_H__
#define __I_MUSICSYSTEM_H__

#include "SDL_mixer.h"
#include "i_music.h"
#include "i_midi.h"

#ifdef OSX
#include <AudioToolbox/AudioToolbox.h>
#endif	// OSX

#ifdef PORTMIDI
#include "portmidi.h"
#endif	// PORTMIDI

// ============================================================================
//
// MusicSystem abstract base class
//
// Abstract base class that provides an interface for inheriting classes as
// well as default implementations for several functions.
//
// ============================================================================

class MusicSystem
{
public:
	MusicSystem() : mIsPlaying(false), mIsPaused(false), mTempo(120.0f), mVolume(1.0f) {}
	virtual ~MusicSystem() {}
	
	virtual void startSong(byte* data, size_t length, bool loop);
	virtual void stopSong();
	virtual void pauseSong();
	virtual void resumeSong();
	virtual void playChunk() = 0;
	
	virtual void setVolume(float volume);
	float getVolume() const { return mVolume; }
	virtual void setTempo(float tempo);
	float getTempo() const { return mTempo; }
	
	virtual bool isInitialized() const = 0;
	bool isPlaying() const { return mIsPlaying; }
	bool isPaused() const { return mIsPaused; }
	
	// Can this MusicSystem object play a particular type of music file?
	virtual bool isMusCapable() const { return false; }
	virtual bool isMidiCapable() const { return false; }
	virtual bool isOggCapable() const { return false; }
	virtual bool isMp3Capable() const { return false; }
	virtual bool isModCapable() const { return false; }
	virtual bool isWaveCapable() const { return false; }
	
private:
	bool	mIsPlaying;
	bool	mIsPaused;
	
	float	mTempo;
	float	mVolume;
};


// ============================================================================
//
// SilentMusicSystem class
//
// This music system does not play any music.  It can be selected when the user
// wishes to disable music output.
//
// ============================================================================

class SilentMusicSystem : public MusicSystem
{
public:
	SilentMusicSystem() { Printf(PRINT_HIGH, "I_InitMusic: Music playback disabled.\n"); }
	
	virtual void startSong(byte* data, size_t length, bool loop) {}
	virtual void stopSong() {}
	virtual void pauseSong() {}
	virtual void resumeSong() {}
	virtual void playChunk() {}
	virtual void setVolume(float volume) const {}
	
	virtual bool isInitialized() const { return true; }
	
	// SilentMusicSystem can handle any type of music by doing nothing
	virtual bool isMusCapable() const { return true; }
	virtual bool isMidiCapable() const { return true; }
	virtual bool isOggCapable() const { return true; }
	virtual bool isMp3Capable() const { return true; }
	virtual bool isModCapable() const { return true; }
	virtual bool isWaveCapable() const { return true; }
};


// ============================================================================
//
// SdlMixerMusicSystem class
//
// Plays music utilizing the SDL_Mixer library and can handle a wide range of
// music formats.
//
// ============================================================================

class SdlMixerMusicSystem : public MusicSystem
{
public:
	SdlMixerMusicSystem();
	virtual ~SdlMixerMusicSystem();
	
	virtual void startSong(byte* data, size_t length, bool loop);
	virtual void stopSong();
	virtual void pauseSong();
	virtual void resumeSong();
	virtual void playChunk() {}
	virtual void setVolume(float volume);
	
	virtual bool isInitialized() const { return mIsInitialized; }

	virtual bool isMusCapable() const { return true; }
	virtual bool isMidiCapable() const { return true; }
	virtual bool isOggCapable() const { return true; }
	virtual bool isMp3Capable() const { return true; }
	virtual bool isModCapable() const { return true; }
	virtual bool isWaveCapable() const { return true; }

private:
	bool					mIsInitialized;
	MusicHandler_t			mRegisteredSong;

	void _StopSong();
	void _RegisterSong(byte* data, size_t length);
	void _UnregisterSong();
};


// ============================================================================
//
// AuMusicSystem class
//
// Plays music utilizing OSX's Audio Unit system, which is the default for OSX.
// On non-OSX systems, the AuMusicSystem will not output any sound and should
// not be selected.
//
// ============================================================================

#ifdef OSX
class AuMusicSystem : public MusicSystem
{
public:
	AuMusicSystem();
	virtual ~AuMusicSystem();
	
	virtual void startSong(byte* data, size_t length, bool loop);
	virtual void stopSong();
	virtual void pauseSong();
	virtual void resumeSong();
	virtual void playChunk() {}
	virtual void setVolume(float volume);
	
	virtual bool isInitialized() const { return mIsInitialized; }
	
	// Only plays midi-type music
	virtual bool isMusCapable() const { return true; }
	virtual bool isMidiCapable() const { return true; }
	
private:
	bool			mIsInitialized;
	
	MusicPlayer		mPlayer;
	MusicSequence	mSequence;
	AUGraph			mGraph;
	AUNode			mSynth;
	AUNode			mOutput;
	AudioUnit		mUnit;
	CFDataRef		mCfd;
		
	void _StopSong();
	void _RegisterSong(byte* data, size_t length);
	void _UnregisterSong();
};
#endif	// OSX

// ============================================================================
//
// MidiMusicSystem abstract base class
//
// Abstract base class that provides an interface for cross-platform midi
// libraries.  MidiMusicSystem handles parsing a lump containing a MUS or MIDI
// file and feeding each midi event to the library.  MidiMusicSystem does the
// heavy lifting for the subclasses that are based on it.
//
// ============================================================================

class MidiMusicSystem : public MusicSystem
{
public:
	MidiMusicSystem();
	virtual ~MidiMusicSystem();

	virtual void startSong(byte* data, size_t length, bool loop);
	virtual void stopSong();
	virtual void pauseSong();
	virtual void resumeSong();
	
	virtual void playChunk();
	virtual void setVolume(float volume);
	
	// Only plays midi-type music
	virtual bool isMusCapable() const { return true; }
	virtual bool isMidiCapable() const { return true; }
	
	virtual void playEvent(MidiEvent *event, int time = 0) = 0;
	
protected:
	void _StopSong();
	
	virtual void _AllNotesOff();
	
	int _GetNumChannels() const { return cNumChannels; }
	void _SetChannelVolume(int channel, int volume);
	void _RefreshVolume();
	
	unsigned int _GetLastEventTime() const { return mLastEventTime; }
	
	void _InitializePlayback();

	float _GetScaledVolume();
	
private:
	static const int			cNumChannels = 16;
	MidiSong*					mMidiSong;
	MidiSong::const_iterator	mSongItr;
	bool						mLoop;
	int							mTimeDivision;
	
	unsigned int				mLastEventTime;
	int							mPrevClockTime;
	
	byte						mChannelVolume[cNumChannels];
};


// ============================================================================
//
// PortMidiMusicSystem class
//
// Plays music utilizing the PortMidi music library.
//
// ============================================================================

#ifdef PORTMIDI
class PortMidiMusicSystem : public MidiMusicSystem
{
public:
	PortMidiMusicSystem();
	virtual ~PortMidiMusicSystem();
	
	virtual void stopSong();
	virtual bool isInitialized() const { return mIsInitialized; }
		
	virtual void playEvent(MidiEvent *event, int time = 0);

private:
	static const int cLatency = 80;
	
	bool		mIsInitialized;

	PmDeviceID	mOutputDevice;
	PmStream*	mStream;
	
	void _PlayEvent(MidiEvent *event, int time = 0);
	void _StopSong();
};
#endif	// PORTMIDI

#endif	// __I_MUSICSYSTEM_H__

