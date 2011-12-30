// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id: i_musicsystem.h 1788 2010-08-24 04:42:57Z russellrice $
//
// Copyright (C) 2006-2010 by The Odamex Team.
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

#ifndef _I_MUSICSYSTEM_H_
#define _I_MUSICSYSTEM_H_

#include "SDL_mixer.h"
#include "i_music.h"

#ifdef OSX
// denis - midi via SDL+timidity on OSX crashes miserably after a while
// this is not our fault, but we have to live with it until someone
// bothers to fix it, therefore use native midi on OSX for now
#include <AudioToolbox/AudioToolbox.h>
#endif

class MusicSystem
{
public:
	MusicSystem() : mIsPlaying(false), mIsPaused(false) {}
	virtual ~MusicSystem() {}
	
	virtual void playSong(byte* data, size_t length, bool loop);
	virtual void stopSong();
	virtual void pauseSong();
	virtual void resumeSong();
	virtual void setVolume(float volume) = 0;
	
	virtual bool isInitialized() = 0;
	bool isPlaying() { return mIsPlaying; }
	bool isPaused() { return mIsPaused; }
	
	// Can this MusicSystem object play a particular type of music file?
	virtual bool isMusCapable() { return false; }
	virtual bool isMidiCapable() { return false; }
	virtual bool isOggCapable() { return false; }
	virtual bool isMp3Capable() { return false; }
	virtual bool isModCapable() { return false; }
	virtual bool isWaveCapable() { return false; }
	
private:
	bool	mIsPlaying;
	bool	mIsPaused;
};


class SilentMusicSystem : public MusicSystem
{
public:
	SilentMusicSystem() { Printf(PRINT_HIGH, "I_InitMusic: Music playback disabled\n"); }
	virtual void playSong(byte* data, size_t length, bool loop) {}
	virtual void stopSong() {}
	virtual void pauseSong() {}
	virtual void resumeSong() {}
	virtual void setVolume(float volume) {}
	
	virtual bool isInitialized() { return true; }
	
	// SilentMusicSystem can play any type of music, albeit silently
	virtual bool isMusCapable() { return true; }
	virtual bool isMidiCapable() { return true; }
	virtual bool isOggCapable() { return true; }
	virtual bool isMp3Capable() { return true; }
	virtual bool isModCapable() { return true; }
	virtual bool isWaveCapable() { return true; }
	
};


class SdlMixerMusicSystem : public MusicSystem
{
public:
	SdlMixerMusicSystem();
	virtual ~SdlMixerMusicSystem();
	
	virtual void playSong(byte* data, size_t length, bool loop);
	virtual void stopSong();
	virtual void pauseSong();
	virtual void resumeSong();
	virtual void setVolume(float volume);
	
	virtual bool isInitialized() { return mIsInitialized; }

	virtual bool isMusCapable() { return true; }
	virtual bool isMidiCapable() { return true; }
	virtual bool isOggCapable() { return true; }
	virtual bool isMp3Capable() { return true; }
	virtual bool isModCapable() { return true; }
	virtual bool isWaveCapable() { return true; }
	
private:
	bool					mIsInitialized;
	MusicHandler_t			mRegisteredSong;

	void _StopSong();
	void _RegisterSong(byte* data, size_t length);
	void _UnregisterSong();
};


class AuMusicSystem : public MusicSystem
{
public:
	AuMusicSystem();
	~AuMusicSystem();
	
	virtual void playSong(byte* data, size_t length, bool loop);
	virtual void stopSong();
	virtual void pauseSong();
	virtual void resumeSong();
	virtual void setVolume(float volume);
	
	virtual bool isInitialized() { return mIsInitialized; }
	
	// Only plays midi-type music
	virtual bool isMusCapable() { return true; }
	virtual bool isMidiCapable() { return true; }
	
private:
	bool			mIsInitialized;
	
	#ifdef OSX
	MusicPlayer		mPlayer;
	MusicSequence	mSequence;
	AUGraph			mGraph;
	AUNode			mSynth;
	AUNode			mOutput;
	AudioUnit		mUnit;
	CFDataRef		mCfd;
	#endif	// OSX
		
	void _StopSong();
	void _RegisterSong(byte* data, size_t length);
	void _UnregisterSong();
};

#endif
