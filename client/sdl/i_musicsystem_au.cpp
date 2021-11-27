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
//  Plays music utilizing OSX's Audio Unit system, which is the default for
//  OSX.  On non-OSX systems, the AuMusicSystem will not output any sound
//  and should not be selected.
//
//-----------------------------------------------------------------------------

#ifdef OSX

#include "odamex.h"

#include "i_musicsystem_au.h"

#include <AudioToolbox/AudioToolbox.h>

// ============================================================================
//
// denis - midi via SDL+timidity on OSX crashes miserably after a while
// this is not our fault, but we have to live with it until someone
// bothers to fix it, therefore use native midi on OSX for now
//
// ============================================================================

AuMusicSystem::AuMusicSystem() :
	mIsInitialized(false)
{
	NewAUGraph(&mGraph);

	#if MAC_OS_X_VERSION_MIN_REQUIRED < 1050
	ComponentDescription d;
	#else
	AudioComponentDescription d;
	#endif

	d.componentType = kAudioUnitType_MusicDevice;
	d.componentSubType = kAudioUnitSubType_DLSSynth;
	d.componentManufacturer = kAudioUnitManufacturer_Apple;
	d.componentFlags = 0;
	d.componentFlagsMask = 0;
	#if MAC_OS_X_VERSION_MIN_REQUIRED < 1050
	AUGraphNewNode(mGraph, &d, 0, NULL, &mSynth);
	#else
	AUGraphAddNode(mGraph, &d, &mSynth);
	#endif


	d.componentType = kAudioUnitType_Output;
	d.componentSubType = kAudioUnitSubType_DefaultOutput;
	d.componentManufacturer = kAudioUnitManufacturer_Apple;
	d.componentFlags = 0;
	d.componentFlagsMask = 0;
	#if MAC_OS_X_VERSION_MIN_REQUIRED < 1050
	AUGraphNewNode(mGraph, &d, 0, NULL, &mOutput);
	#else
	AUGraphAddNode(mGraph, &d, &mOutput);
	#endif

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
