// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2006-2015 by The Odamex Team.
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

/*
native_midi_macosx: Native Midi support on Mac OS X for the SDL_mixer library
Copyright (C) 2009 Ryan C. Gordon <icculus@icculus.org>

This software is provided 'as-is', without any express or implied
warranty. In no event will the authors be held liable for any damages
arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must not
claim that you wrote the original software. If you use this software
in a product, an acknowledgment in the product documentation would be
appreciated but is not required.
2. Altered source versions must be plainly marked as such, and must not be
misrepresented as being the original software.
3. This notice may not be removed or altered from any source distribution.
*/

#include <string>
#include <math.h>
#include "i_system.h"
#include "m_fileio.h"
#include "cmdlib.h"

#include "i_sdl.h"
#include "i_music.h"
#include "i_midi.h"
#include "mus2midi.h"
#include "i_musicsystem.h"

#include <SDL_mixer.h>

#ifdef OSX
#include <AudioToolbox/AudioToolbox.h>
#include <CoreServices/CoreServices.h>
#endif	// OSX

#ifdef PORTMIDI
#include "portmidi.h"
#endif	// PORTMIDI

// [Russell] - define a temporary midi file, for consistency
// SDL < 1.2.7
#ifdef _XBOX
	// Use the cache partition
	#define TEMP_MIDI "Z:\\temp_music"
#elif MIX_MAJOR_VERSION < 1 || (MIX_MAJOR_VERSION == 1 && MIX_MINOR_VERSION < 2) || (MIX_MAJOR_VERSION == 1 && MIX_MINOR_VERSION == 2 && MIX_PATCHLEVEL < 7)
    #define TEMP_MIDI "temp_music"
#endif

EXTERN_CVAR(snd_musicvolume)
EXTERN_CVAR(snd_musicdevice)

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
// SdlMixerMusicSystem
//
// ============================================================================


SdlMixerMusicSystem::SdlMixerMusicSystem() :
	mIsInitialized(false), mRegisteredSong()
{
	Printf(PRINT_HIGH, "I_InitMusic: Music playback enabled using SDL_Mixer.\n");
	mIsInitialized = true;
}

SdlMixerMusicSystem::~SdlMixerMusicSystem()
{
	if (!isInitialized())
		return;
		
	Mix_HaltMusic();

	_StopSong();
	mIsInitialized = false;
}

void SdlMixerMusicSystem::startSong(byte* data, size_t length, bool loop)
{
	if (!isInitialized())
		return;
		
	stopSong();
	
	if (!data || !length)
		return;
	
	_RegisterSong(data, length);
		
	if (!mRegisteredSong.Track || !mRegisteredSong.Data)
		return;

	if (Mix_PlayMusic(mRegisteredSong.Track, loop ? -1 : 1) == -1)
	{
		Printf(PRINT_HIGH, "Mix_PlayMusic: %s\n", Mix_GetError());
		return;
	}

	Mix_HookMusicFinished(I_ResetMidiVolume);

	MusicSystem::startSong(data, length, loop);
    
	// [Russell] - Hack for setting the volume on windows vista, since it gets
	// reset on every music change
	setVolume(snd_musicvolume);
}

//
// SdlMixerMusicSystem::_StopSong()
//
// Fades the current music out and frees the data structures used for the
// current song with _UnregisterSong().
//
void SdlMixerMusicSystem::_StopSong()
{
	if (!isInitialized() || !isPlaying())
		return;

	if (isPaused())
		resumeSong();
	
	Mix_FadeOutMusic(100);
	
	_UnregisterSong();
}

//
// SdlMixerMusicSystem::stopSong()
//
// [SL] 2011-12-16 - This function simply calls _StopSong().  Since stopSong()
// is a virtual function, calls to it should be avoided in ctors & dtors.  Our
// dtor calls the non-virtual function _StopSong() instead and the virtual
// function stopSong() might as well reuse the code in _StopSong().
//
void SdlMixerMusicSystem::stopSong()
{
	_StopSong();
	MusicSystem::stopSong();
}

void SdlMixerMusicSystem::pauseSong()
{
	MusicSystem::pauseSong();
	
	setVolume(0.0f);
	Mix_PauseMusic();
}

void SdlMixerMusicSystem::resumeSong()
{
	MusicSystem::resumeSong();
	
	setVolume(getVolume());
	Mix_ResumeMusic();
}

//
// SdlMixerMusicSystem::setVolume
//
// Sanity checks the volume parameter and then sets the volume for the midi
// output mixer channel.  Note that Windows Vista/7 combines the volume control
// for the audio and midi channels and changing the midi volume also changes
// the SFX volume.
void SdlMixerMusicSystem::setVolume(float volume)
{
	MusicSystem::setVolume(volume);
	Mix_VolumeMusic(int(getVolume() * MIX_MAX_VOLUME));
}

//
// SdlMixerMusicSystem::_UnregisterSong
//
// Frees the data structures that store the song.  Called when stopping song.
//
void SdlMixerMusicSystem::_UnregisterSong()
{
	if (!isInitialized())
		return;

	if (mRegisteredSong.Track)
		Mix_FreeMusic(mRegisteredSong.Track);
		
	mRegisteredSong.Track = NULL;
	mRegisteredSong.Data = NULL;
}

//
// SdlMixerMusicSystem::_RegisterSong
//
// Determines the format of music data and allocates the memory for the music
// data if appropriate.  Note that _UnregisterSong should be called after
// playing to free the allocated memory.
void SdlMixerMusicSystem::_RegisterSong(byte* data, size_t length)
{
	_UnregisterSong();
	
	if (S_MusicIsMus(data, length))
	{
		MEMFILE *mus = mem_fopen_read(data, length);
		MEMFILE *midi = mem_fopen_write();
	
		int result = mus2mid(mus, midi);
		if (result == 0)
			mRegisteredSong.Data = SDL_RWFromMem(mem_fgetbuf(midi), mem_fsize(midi));
		else
			Printf(PRINT_HIGH, "MUS is not valid\n");

		mem_fclose(mus);
		mem_fclose(midi);		
	}
	else
	{
		mRegisteredSong.Data = SDL_RWFromMem(data, length);
	}

	if (!mRegisteredSong.Data)
	{
		Printf(PRINT_HIGH, "SDL_RWFromMem: %s\n", SDL_GetError());
		return;
	}

	#ifdef TEMP_MIDI
	// We're using an older version of SDL and must save the midi data
	// to a temporary file first
	FILE *fp = fopen(TEMP_MIDI, "wb+");
	if (!fp)
	{
		Printf(PRINT_HIGH, "Could not open temporary music file %s, not playing track\n", TEMP_MIDI);
		return;
	}
    
	// Get the size of the music data
	SDL_RWseek(mRegisteredSong.Data, 0, SEEK_END);
	size_t reglength = SDL_RWtell(mRegisteredSong.Data);
	
	// Write the music data to the temporary file
	SDL_RWseek(mRegisteredSong.Data, 0, SEEK_SET);
	char buf[1024];
	while (reglength)
	{
		size_t chunksize = reglength > sizeof(buf) ? sizeof(buf) : reglength;
		
		SDL_RWread(mRegisteredSong.Data, buf, chunksize, 1);
		fwrite(buf, chunksize, 1, fp);
		reglength -= chunksize;
	}
	
	fclose(fp);
	// Read the midi data from the temporary file
	mRegisteredSong.Track = Mix_LoadMUS(TEMP_MIDI);
	unlink(TEMP_MIDI);	// remove the temporary file

	if (!mRegisteredSong.Track)
	{
		Printf(PRINT_HIGH, "Mix_LoadMUSW: %s\n", Mix_GetError());
		return;
	}

	#else

	// We can read the midi data directly from memory
	#ifdef SDL20
	mRegisteredSong.Track = Mix_LoadMUS_RW(mRegisteredSong.Data, 0);
	#elif defined SDL12
	mRegisteredSong.Track = Mix_LoadMUS_RW(mRegisteredSong.Data);
	#endif	// SDL12
	
	if (!mRegisteredSong.Track)
	{
		Printf(PRINT_HIGH, "Mix_LoadMUS_RW: %s\n", Mix_GetError());
		return;
	}

	#endif	// TEMP_MIDI
}


// ============================================================================
//
// AuMusicSystem
//
// denis - midi via SDL+timidity on OSX crashes miserably after a while
// this is not our fault, but we have to live with it until someone
// bothers to fix it, therefore use native midi on OSX for now
//
// ============================================================================

#ifdef OSX

AuMusicSystem::AuMusicSystem() :
	mIsInitialized(false)
{
	NewAUGraph(&mGraph);

	ComponentDescription d;

	d.componentType = kAudioUnitType_MusicDevice;
	d.componentSubType = kAudioUnitSubType_DLSSynth;
	d.componentManufacturer = kAudioUnitManufacturer_Apple;
	d.componentFlags = 0;
	d.componentFlagsMask = 0;
	AUGraphNewNode(mGraph, &d, 0, NULL, &mSynth);

	d.componentType = kAudioUnitType_Output;
	d.componentSubType = kAudioUnitSubType_DefaultOutput;
	d.componentManufacturer = kAudioUnitManufacturer_Apple;
	d.componentFlags = 0;
	d.componentFlagsMask = 0;
	AUGraphNewNode(mGraph, &d, 0, NULL, &mOutput);

	if (AUGraphConnectNodeInput(mGraph, mSynth, 0, mOutput, 0) != noErr)
	{
		Printf(PRINT_HIGH, "I_InitMusic: AUGraphConnectNodeInput failed\n");
		return;
	}

	if (AUGraphOpen(mGraph) != noErr)
	{
		Printf(PRINT_HIGH, "I_InitMusic: AUGraphOpen failed\n");
		return;
	}

	if (AUGraphInitialize(mGraph) != noErr)
	{
		Printf(PRINT_HIGH, "I_InitMusic: AUGraphInitialize failed\n");
		return;
	}

	#if MAC_OS_X_VERSION_MIN_REQUIRED < 1050	/* this is deprecated, but works back to 10.0 */
	if (AUGraphGetNodeInfo(mGraph, mOutput, NULL, NULL, NULL, &mUnit) != noErr)
	#else		/* not deprecated, but requires 10.5 or later */
	if (AUGraphNodeInfo(mGraph, mOutput, NULL, &mUnit) != noErr)
	#endif
	{
		Printf(PRINT_HIGH, "I_InitMusic: AUGraphGetNodeInfo failed\n");
		return;
	}

	if (NewMusicPlayer(&mPlayer) != noErr)
	{
		Printf(PRINT_HIGH, "I_InitMusic: Music player creation failed using AudioToolbox\n");
		return;
	}

	Printf(PRINT_HIGH, "I_InitMusic: Music playback enabled using AudioToolbox\n");
	mIsInitialized = true;
	return;
}

AuMusicSystem::~AuMusicSystem()
{
	_StopSong();
	MusicSystem::stopSong();
	
	DisposeMusicPlayer(mPlayer);
	AUGraphClose(mGraph);
}

void AuMusicSystem::startSong(byte* data, size_t length, bool loop)
{
	if (!isInitialized())
		return;
		
	stopSong();
	
	if (!data || !length)
		return;
	
	_RegisterSong(data, length);
	
	if (MusicSequenceSetAUGraph(mSequence, mGraph) != noErr)
	{
		Printf(PRINT_HIGH, "I_PlaySong: MusicSequenceSetAUGraph failed\n");
		return;
	}

	if (MusicPlayerSetSequence(mPlayer, mSequence) != noErr)
	{
		Printf(PRINT_HIGH, "I_PlaySong: MusicPlayerSetSequence failed\n");
		return;
	}

	if (MusicPlayerPreroll(mPlayer) != noErr)
	{
		Printf(PRINT_HIGH, "I_PlaySong: MusicPlayerPreroll failed\n");
		return;
	}

	UInt32 outNumberOfTracks = 0;
	if (MusicSequenceGetTrackCount(mSequence, &outNumberOfTracks) != noErr)
	{
		Printf(PRINT_HIGH, "I_PlaySong: MusicSequenceGetTrackCount failed\n");
		return;
	}

	for (UInt32 i = 0; i < outNumberOfTracks; i++)
	{
		MusicTrack track;

		if (MusicSequenceGetIndTrack(mSequence, i, &track) != noErr)
		{
			Printf(PRINT_HIGH, "I_PlaySong: MusicSequenceGetIndTrack failed\n");
			return;
		}

		struct s_loopinfo
		{
			MusicTimeStamp time;
			long loops;
		} LoopInfo;

		UInt32 inLength = sizeof(LoopInfo);

		if (MusicTrackGetProperty(track, kSequenceTrackProperty_LoopInfo, &LoopInfo, &inLength) != noErr)
		{
			Printf(PRINT_HIGH, "I_PlaySong: MusicTrackGetProperty failed\n");
			return;
		}

		inLength = sizeof(LoopInfo.time);

		if (MusicTrackGetProperty(track, kSequenceTrackProperty_TrackLength, &LoopInfo.time, &inLength) != noErr)
		{
			Printf(PRINT_HIGH, "I_PlaySong: MusicTrackGetProperty failed\n");
			return;
		}

		LoopInfo.loops = loop ? 0 : 1;

		if (MusicTrackSetProperty(track, kSequenceTrackProperty_LoopInfo, &LoopInfo, sizeof(LoopInfo)) != noErr)
		{
			Printf(PRINT_HIGH, "I_PlaySong: MusicTrackSetProperty failed\n");
			return;
		}
	}

	if (MusicPlayerStart(mPlayer) != noErr)
	{
		Printf(PRINT_HIGH, "I_PlaySong: MusicPlayerStart failed\n");
		return;
	}
	
	MusicSystem::startSong(data, length, loop);
}

//
// AuMusicSystem::_StopSong()
//
// Fades the current music out and frees the data structures used for the
// current song with _UnregisterSong().
//
void AuMusicSystem::_StopSong()
{
	if (!isInitialized() || !isPlaying())
		return;

	if (isPaused())
		resumeSong();
		
	MusicPlayerStop(mPlayer);
	_UnregisterSong();
}

void AuMusicSystem::stopSong()
{
	_StopSong();
	MusicSystem::stopSong();
}

void AuMusicSystem::pauseSong()
{
	setVolume(0.0f);

	MusicSystem::pauseSong();
}

void AuMusicSystem::resumeSong()
{
	setVolume(snd_musicvolume);

	MusicSystem::resumeSong();
}

//
// AuMusicSystem::setVolume
//
// Sanity checks the volume parameter and then sets the volume for the midi
// output mixer channel.  
void AuMusicSystem::setVolume(float volume)
{
	MusicSystem::setVolume(volume);
	
	if (AudioUnitSetParameter(mUnit, kAudioUnitParameterUnit_LinearGain, kAudioUnitScope_Output, 0, getVolume(), 0) != noErr)
	{
		Printf(PRINT_HIGH, "I_InitMusic: AudioUnitSetParameter failed\n");
		return;
	}
}

//
// AuMusicSystem::_UnregisterSong
//
// Frees the data structures that store the song.  Called when stopping song.
//
void AuMusicSystem::_UnregisterSong()
{
	if (!isInitialized())
		return;

    DisposeMusicSequence(mSequence);
}

//
// AuMusicSystem::_RegisterSong
//
// Determines the format of music data and allocates the memory for the music
// data if appropriate.  Note that _UnregisterSong should be called after
// playing to free the allocated memory.
void AuMusicSystem::_RegisterSong(byte* data, size_t length)
{
	byte* regdata = data;
	size_t reglength = length;
	MEMFILE *mus = NULL, *midi = NULL;
	
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
			Printf(PRINT_HIGH, "MUS is not valid\n");
			regdata = NULL;
			reglength = 0;
		}
	}
	else if (!S_MusicIsMidi(data, length))
	{
		Printf(PRINT_HIGH, "I_PlaySong: AudioUnit does not support this music format\n");
		return;
	}
	
	if (NewMusicSequence(&mSequence) != noErr)
	{
		Printf(PRINT_HIGH, "I_PlaySong: Unable to create AudioUnit sequence\n");
		return;
	}

	mCfd = CFDataCreate(NULL, (const Uint8 *)regdata, reglength);

	if (!mCfd)
	{
		DisposeMusicSequence(mSequence);
		return;
	}


#if MAC_OS_X_VERSION_MIN_REQUIRED < 1050
	/* MusicSequenceLoadSMFData() (avail. in 10.2, no 64 bit) is
	 * equivalent to calling MusicSequenceLoadSMFDataWithFlags()
	 * with a flags value of 0 (avail. in 10.3, avail. 64 bit).
	 * So, we use MusicSequenceLoadSMFData() for powerpc versions
	 * but the *WithFlags() on intel which require 10.4 anyway. */
	# if defined(__ppc__) || defined(__POWERPC__)
	if (MusicSequenceLoadSMFData(mSequence, (CFDataRef)mCfd) != noErr)
	# else
	if (MusicSequenceLoadSMFDataWithFlags(mSequence, (CFDataRef)mCfd, 0) != noErr)
	# endif
#else		/* MusicSequenceFileLoadData() requires 10.5 or later. */
	if (MusicSequenceFileLoadData(mSequence, (CFDataRef)mCfd, 0, 0) != noErr)
#endif
	{
		DisposeMusicSequence(mSequence);
		CFRelease(mCfd);
		return;
	}

	if (mus)
		mem_fclose(mus);
	if (midi)
		mem_fclose(midi);
}
#endif	// OSX


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
static MidiSong* I_RegisterMidiSong(byte *data, size_t length)
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
			Printf(PRINT_HIGH, "I_RegisterMidiSong: MUS is not valid\n");
			regdata = NULL;
			reglength = 0;
		}
	}
	else if (!S_MusicIsMidi(data, length))
	{
		Printf(PRINT_HIGH, "I_RegisterMidiSong: Only midi music formats are supported with the selected music system.\n");
		return NULL;
	}
	
	MidiSong *midisong = new MidiSong(regdata, reglength);
	
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

MidiMusicSystem::MidiMusicSystem() :
	MusicSystem(), mMidiSong(NULL), mSongItr(), mLoop(false), mTimeDivision(96),
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

void MidiMusicSystem::_StopSong()
{
}

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
	
	MidiEvent *event = *mSongItr;
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
		MidiEvent *event = *mSongItr;
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

// ============================================================================
//
// PortMidiMusicSystem
//
// Partially based on an implementation from prboom-plus by Nicholai Main (Natt).
// ============================================================================

#ifdef PORTMIDI

//
// I_PortMidiTime()
//
// A wrapper function for I_MSTime() so that PortMidi can use a function
// pointer to I_MSTime() for its event scheduling needs.
//
static int I_PortMidiTime(void *time_info = NULL)
{
	return I_MSTime();
} 

PortMidiMusicSystem::PortMidiMusicSystem() :
	MidiMusicSystem(), mIsInitialized(false),
	mOutputDevice(-1), mStream(NULL)
{
	const int output_buffer_size = 1024;
	
	if (Pm_Initialize() != pmNoError)
	{
		Printf(PRINT_HIGH, "I_InitMusic: PortMidi initialization failed.\n");
		return;
	}

 	mOutputDevice = Pm_GetDefaultOutputDeviceID();
 	std::string prefdevicename(snd_musicdevice.cstring());
  	
	// List PortMidi devices
	for (int i = 0; i < Pm_CountDevices(); i++)
	{
		const PmDeviceInfo *info = Pm_GetDeviceInfo(i);
		if (!info || !info->output)
			continue;
			
		std::string curdevicename(info->name);
		if (!prefdevicename.empty() && iequals(prefdevicename, curdevicename))
			mOutputDevice = i;

		Printf(PRINT_HIGH, "%d: %s, %s\n", i, info->interf, info->name);
    }
    
    if (mOutputDevice == pmNoDevice)
	{
		Printf(PRINT_HIGH, "I_InitMusic: No PortMidi output devices available.\n");
		Pm_Terminate ();
		return;
	}
	
	if (Pm_OpenOutput(&mStream,	mOutputDevice, NULL, output_buffer_size, I_PortMidiTime, NULL, cLatency) != pmNoError)
	{
		Printf(PRINT_HIGH, "I_InitMusic: Failure opening PortMidi output device %d.\n", mOutputDevice);
		return;
	} 
                  
	if (!mStream)
		return;
		
	Printf(PRINT_HIGH, "I_InitMusic: Music playback enabled using PortMidi.\n");
	mIsInitialized = true;
}

PortMidiMusicSystem::~PortMidiMusicSystem()
{
	if (!isInitialized())
		return;
	
	_StopSong();
	mIsInitialized = false;
	
	if (mStream)
	{
		// Sleep to allow the All-Notes-Off events to be processed
		I_Sleep(I_ConvertTimeFromMs(cLatency * 2));
		
		Pm_Close(mStream);
		Pm_Terminate();
		mStream = NULL;
	}
}

void PortMidiMusicSystem::stopSong()
{
	_StopSong();
	MidiMusicSystem::stopSong();
}

void PortMidiMusicSystem::_StopSong()
{
	// non-virtual version of _AllNotesOff()
	for (int i = 0; i < _GetNumChannels(); i++)
	{
		MidiControllerEvent event_noteoff(0, MIDI_CONTROLLER_ALL_NOTES_OFF, i);
		_PlayEvent(&event_noteoff);
		MidiControllerEvent event_reset(0, MIDI_CONTROLLER_RESET_ALL, i);
		_PlayEvent(&event_reset);
	}
}

//
// PortMidiMusicSystem::playEvent
//
// Virtual wrapper-function for the non-virtual _PlayEvent.  We provide the
// non-virtual version so that it can be safely called by ctors and dtors.
//
void PortMidiMusicSystem::playEvent(MidiEvent *event, int time)
{
	if (event)
		_PlayEvent(event, time);
}

void PortMidiMusicSystem::_PlayEvent(MidiEvent *event, int time)
{
	if (!event)
		return;
		
	// play at the current time if user specifies time 0
	if (time == 0)
		time = _GetLastEventTime();
	
	if (I_IsMidiMetaEvent(event))
	{
		MidiMetaEvent *metaevent = static_cast<MidiMetaEvent*>(event);
		if (metaevent->getMetaType() == MIDI_META_SET_TEMPO)
		{
			double tempo = I_GetTempoChange(metaevent);
			setTempo(tempo);
		}
		
		//	Just ignore other meta events for now
	}
	else if (I_IsMidiSysexEvent(event))
	{
		// Just ignore sysex events for now
	}
	else if (I_IsMidiControllerEvent(event))
	{
		MidiControllerEvent *ctrlevent = static_cast<MidiControllerEvent*>(event);
		byte channel = ctrlevent->getChannel();
		byte controltype = ctrlevent->getControllerType();
		byte param1 = ctrlevent->getParam1();
		
		if (controltype == MIDI_CONTROLLER_MAIN_VOLUME)
		{
			// store the song's volume for the channel
			_SetChannelVolume(channel, param1);
			
			// scale the channel's volume by the master music volume
			param1 *= _GetScaledVolume();
		}
			
		PmMessage msg = Pm_Message(event->getEventType() | channel, controltype, param1);
		Pm_WriteShort(mStream, time, msg);
	}
	else if (I_IsMidiChannelEvent(event))
	{
		MidiChannelEvent *chanevent = static_cast<MidiChannelEvent*>(event);
		byte channel = chanevent->getChannel();
		byte param1 = chanevent->getParam1();
		byte param2 = chanevent->getParam2();
		
		PmMessage msg = Pm_Message(event->getEventType() | channel, param1, param2);
		Pm_WriteShort(mStream, time, msg);
	}
}

#endif	// PORTMIDI

VERSION_CONTROL (i_musicsystem_cpp, "$Id$")
	
