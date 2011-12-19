// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: i_musicsystem.cpp 2541 2011-10-27 02:36:31Z dr_sean $
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

#include "SDL_mixer.h"
#include "mus2midi.h"

#include "i_music.h"
#include "i_musicsystem.h"
#include "m_fileio.h"

#ifdef OSX
// denis - midi via SDL+timidity on OSX crashes miserably after a while
// this is not our fault, but we have to live with it until someone
// bothers to fix it, therefore use native midi on OSX for now
#include <AudioToolbox/AudioToolbox.h>
#endif

// [Russell] - define a temporary midi file, for consistency
// SDL < 1.2.7
#ifdef _XBOX
	// Use the cache partition
	#define TEMP_MIDI "Z:\\temp_music"
#elif MIX_MAJOR_VERSION < 1 || (MIX_MAJOR_VERSION == 1 && MIX_MINOR_VERSION < 2) || (MIX_MAJOR_VERSION == 1 && MIX_MINOR_VERSION == 2 && MIX_PATCHLEVEL < 7)
    #define TEMP_MIDI "temp_music"
#endif

EXTERN_CVAR(snd_musicvolume)

// ============================================================================
//
// General functions
//
// ============================================================================

//
// S_ClampVolume
//
// Returns the volume parameter fitted within the inclusive range of 0.0 to 1.0
//
float S_ClampVolume(float volume)
{
	if (volume < 0.0f)
		return 0.0f;
	if (volume > 1.0f)
		return 1.0f;
		
	return volume;
}

bool S_IsMus(byte* data, size_t length)
{
	if (length > 4 && data[0] == 'M' && data[1] == 'U' &&
		data[2] == 'S' && data[3] == 0x1A)
		return true;

	return false;
}

bool S_IsMidi(byte* data, size_t length)
{
	if (length > 4 && data[0] == 'M' && data[1] == 'T' &&
		data[2] == 'h' && data[3] == 'd')
		return true;
		
	return false;
}


// ============================================================================
//
// MusicSystem base class functions
//
// ============================================================================

void MusicSystem::playSong(byte* data, size_t length, bool loop)
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

void SdlMixerMusicSystem::playSong(byte* data, size_t length, bool loop)
{
	if (!isInitialized())
		return;
		
	stopSong();
	
	if (!data || !length)
		return;
	
	_RegisterSong(data, length);
		
	if (!mRegisteredSong.Track)
		return;

	if (Mix_PlayMusic(mRegisteredSong.Track, loop ? -1 : 1) == -1)
	{
		Printf(PRINT_HIGH, "Mix_PlayMusic: %s\n", Mix_GetError());
		return;
	}

    MusicSystem::playSong(data, length, loop);
    
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
	
	//setVolume(0.0f);
	Mix_PauseMusic();
}

void SdlMixerMusicSystem::resumeSong()
{
	MusicSystem::resumeSong();
	
	setVolume(snd_musicvolume);
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
	volume = S_ClampVolume(volume);
	
    Mix_VolumeMusic(int(volume * MIX_MAX_VOLUME));
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
// AuMusicSystem::_RegisterSong
//
// Determines the format of music data and allocates the memory for the music
// data if appropriate.  Note that _UnregisterSong should be called after
// playing to free the allocated memory.
void SdlMixerMusicSystem::_RegisterSong(byte* data, size_t length)
{
	_UnregisterSong();
	
	if (S_IsMus(data, length))
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

	#else
	// We can read the midi data directly from memory
	mRegisteredSong.Track = Mix_LoadMUS_RW(mRegisteredSong.Data);
	
	#endif	// TEMP_MIDI

	if (mRegisteredSong.Data)
	{
		SDL_FreeRW(mRegisteredSong.Data);
		mRegisteredSong.Data = NULL;
	}
	
	if (!mRegisteredSong.Track)
	{
		#ifdef TEMP_MIDI
		Printf(PRINT_HIGH, "Mix_LoadMUS: %s\n", Mix_GetError());
		#else
		Printf(PRINT_HIGH, "Mix_LoadMUS_RW: %s\n", Mix_GetError());
		#endif	// TEMP_MIDI

		return;
	}
}




// ============================================================================
//
// AuMusicSystem
//
// ============================================================================

AuMusicSystem::AuMusicSystem() :
	mIsInitialized(false)
{
	#ifdef OSX
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

	if (AUGraphGetNodeInfo(mGraph, mOutput, NULL, NULL, NULL, &mUnit) != noErr)
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
	
	#else	// !OSX
	Printf(PRINT_HIGH, "I_InitMusic: Music player creation failed using AudioToolbox\n");
	return;
	#endif
}

AuMusicSystem::~AuMusicSystem()
{
	_StopSong();
	MusicSystem::stopSong();
	
	#ifdef OSX
	DisposeMusicPlayer(mPlayer);
	AUGraphClose(mgraph);
	#endif	// OSX
}

void AuMusicSystem::playSong(byte* data, size_t length, bool loop)
{
	if (!isInitialized())
		return;
		
	stopSong();
	
	if (!data || !length)
		return;
	
	_RegisterSong(data, length);
	
	#ifdef OSX	
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

		LoopInfo.loops = _looping ? 0 : 1;

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
	#endif	// OSX
	
	MusicSystem::playSong(data, length, loop);
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
		
	#ifdef OSX
	MusicPlayerStop(mPlayer);
	#endif	// OSX
	
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
	volume = S_ClampVolume(volume);
	
	#ifdef OSX
	if (AudioUnitSetParameter(mUnit, kAudioUnitParameterUnit_LinearGain, kAudioUnitScope_Output, 0, volume, 0) != noErr)
	{
		Printf(PRINT_HIGH, "I_InitMusic: AudioUnitSetParameter failed\n");
		return;
	}
	#endif	// OSX
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

	#ifdef OSX
    DisposeMusicSequence(mSequence);
	#endif	// OSX
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
	
	if (S_IsMus(data, length))
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
	else if (!S_IsMidi(data, length))
	{
		Printf(PRINT_HIGH, "I_PlaySong: AudioUnit does not support this music format\n");
		return;
	}
	
	#ifdef OSX
	if (NewMusicSequence(&mSequence) != noErr)
	{
		Printf(PRINT_HIGH, "I_PlaySong: Unable to create AudioUnit sequence\n");
		return;
	}

	mCfd = CFDataCreate(NULL, (const Uint8 *)regdata, reglength);

	if(!mCfd)
	{
		DisposeMusicSequence(mSequence);
		return;
	}

	if (MusicSequenceLoadSMFData(mSequence, (CFDataRef)mCfd) != noErr)
	{
		DisposeMusicSequence(mSequence);
		CFRelease(mCfd);
		return;
	}
	#endif	// OSX

	if (mus)
		mem_fclose(mus);
	if (midi)
		mem_fclose(midi);
}
	
