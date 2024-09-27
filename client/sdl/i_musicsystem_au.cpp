// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2006-2024 by The Odamex Team.
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
//  OSX.
//
//-----------------------------------------------------------------------------

#ifdef OSX

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

#include "odamex.h"

#include "i_musicsystem_au.h"

#include "i_music.h"
#include "mus2midi.h"

EXTERN_CVAR(snd_musicvolume);

// ============================================================================
//
// denis - midi via SDL+timidity on OSX crashes miserably after a while
// this is not our fault, but we have to live with it until someone
// bothers to fix it, therefore use native midi on OSX for now
//
// ============================================================================

AuMusicSystem::AuMusicSystem() : m_isInitialized(false)
{
	NewAUGraph(&m_graph);

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
	AUGraphNewNode(m_graph, &d, 0, NULL, &m_synth);
#else
	AUGraphAddNode(m_graph, &d, &m_synth);
#endif

	d.componentType = kAudioUnitType_Output;
	d.componentSubType = kAudioUnitSubType_DefaultOutput;
	d.componentManufacturer = kAudioUnitManufacturer_Apple;
	d.componentFlags = 0;
	d.componentFlagsMask = 0;
#if MAC_OS_X_VERSION_MIN_REQUIRED < 1050
	AUGraphNewNode(m_graph, &d, 0, NULL, &m_output);
#else
	AUGraphAddNode(m_graph, &d, &m_output);
#endif

	if (AUGraphConnectNodeInput(m_graph, m_synth, 0, m_output, 0) != noErr)
	{
		Printf(PRINT_HIGH, "I_InitMusic: AUGraphConnectNodeInput failed\n");
		return;
	}

	if (AUGraphOpen(m_graph) != noErr)
	{
		Printf(PRINT_HIGH, "I_InitMusic: AUGraphOpen failed\n");
		return;
	}

	if (AUGraphInitialize(m_graph) != noErr)
	{
		Printf(PRINT_HIGH, "I_InitMusic: AUGraphInitialize failed\n");
		return;
	}

#if MAC_OS_X_VERSION_MIN_REQUIRED < 1050 /* this is deprecated, but works back to 10.0 \
                                          */
	if (AUGraphGetNodeInfo(m_graph, m_output, NULL, NULL, NULL, &m_unit) != noErr)
#else /* not deprecated, but requires 10.5 or later */
	if (AUGraphNodeInfo(m_graph, m_output, NULL, &m_unit) != noErr)
#endif
	{
		Printf(PRINT_HIGH, "I_InitMusic: AUGraphGetNodeInfo failed\n");
		return;
	}

	if (NewMusicPlayer(&m_player) != noErr)
	{
		Printf(PRINT_HIGH,
		       "I_InitMusic: Music player creation failed using AudioToolbox\n");
		return;
	}

	Printf(PRINT_HIGH, "I_InitMusic: Music playback enabled using AudioToolbox\n");
	m_isInitialized = true;
	return;
}

AuMusicSystem::~AuMusicSystem()
{
	_StopSong();
	MusicSystem::stopSong();

	DisposeMusicPlayer(m_player);
	AUGraphClose(m_graph);
}

void AuMusicSystem::startSong(byte* data, size_t length, bool loop)
{
	if (!isInitialized())
		return;

	stopSong();

	if (!data || !length)
		return;

	_RegisterSong(data, length);

	if (MusicSequenceSetAUGraph(m_sequence, m_graph) != noErr)
	{
		Printf(PRINT_HIGH, "I_PlaySong: MusicSequenceSetAUGraph failed\n");
		return;
	}

	if (MusicPlayerSetSequence(m_player, m_sequence) != noErr)
	{
		Printf(PRINT_HIGH, "I_PlaySong: MusicPlayerSetSequence failed\n");
		return;
	}

	if (MusicPlayerPreroll(m_player) != noErr)
	{
		Printf(PRINT_HIGH, "I_PlaySong: MusicPlayerPreroll failed\n");
		return;
	}

	UInt32 outNumberOfTracks = 0;
	if (MusicSequenceGetTrackCount(m_sequence, &outNumberOfTracks) != noErr)
	{
		Printf(PRINT_HIGH, "I_PlaySong: MusicSequenceGetTrackCount failed\n");
		return;
	}

	MusicTimeStamp maxtime = 0;
	for (UInt32 i = 0; i < outNumberOfTracks; i++)
	{
		MusicTrack track;
		MusicTimeStamp time;
		UInt32 size = sizeof(time);

		if (MusicSequenceGetIndTrack(m_sequence, i, &track) != noErr)
		{
			Printf(PRINT_HIGH, "I_PlaySong: MusicSequenceGetIndTrack failed\n");
			return;
		}

		if (MusicTrackGetProperty(track, kSequenceTrackProperty_TrackLength, &time,
		                          &size) != noErr)
		{
			Printf(PRINT_HIGH, "I_PlaySong: MusicTrackGetProperty failed\n");
			return;
		}

		if (time > maxtime)
		{
			maxtime = time;
		}
	}

	for (UInt32 i = 0; i < outNumberOfTracks; i++)
	{
		MusicTrack track;
#if MAC_OS_X_VERSION_MIN_REQUIRED < 1050
		typedef struct
		{
			MusicTimeStamp loopDuration;
			SInt32 numberOfLoops;
		} MusicTrackLoopInfo;
#endif
		MusicTrackLoopInfo LoopInfo;

		if (MusicSequenceGetIndTrack(m_sequence, i, &track) != noErr)
		{
			Printf(PRINT_HIGH, "I_PlaySong: MusicSequenceGetIndTrack failed\n");
			return;
		}

		LoopInfo.loopDuration = maxtime;
		LoopInfo.numberOfLoops = (loop ? 0 : 1);

		if (MusicTrackSetProperty(track, kSequenceTrackProperty_LoopInfo, &LoopInfo,
		                          sizeof(LoopInfo)) != noErr)
		{
			Printf(PRINT_HIGH, "I_PlaySong: MusicTrackSetProperty failed\n");
			return;
		}
	}

	if (MusicPlayerStart(m_player) != noErr)
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

	MusicPlayerStop(m_player);
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

	if (AudioUnitSetParameter(m_unit, kAudioUnitParameterUnit_LinearGain,
	                          kAudioUnitScope_Output, 0, getVolume(), 0) != noErr)
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

	DisposeMusicSequence(m_sequence);
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

	if (NewMusicSequence(&m_sequence) != noErr)
	{
		Printf(PRINT_HIGH, "I_PlaySong: Unable to create AudioUnit sequence\n");
		return;
	}

	m_cfd = CFDataCreate(NULL, (const Uint8*)regdata, reglength);

	if (!m_cfd)
	{
		DisposeMusicSequence(m_sequence);
		return;
	}

#if MAC_OS_X_VERSION_MIN_REQUIRED < 1050
/* MusicSequenceLoadSMFData() (avail. in 10.2, no 64 bit) is
 * equivalent to calling MusicSequenceLoadSMFDataWithFlags()
 * with a flags value of 0 (avail. in 10.3, avail. 64 bit).
 * So, we use MusicSequenceLoadSMFData() for powerpc versions
 * but the *WithFlags() on intel which require 10.4 anyway. */
#if defined(__ppc__) || defined(__POWERPC__)
	if (MusicSequenceLoadSMFData(m_sequence, (CFDataRef)m_cfd) != noErr)
#else
	if (MusicSequenceLoadSMFDataWithFlags(m_sequence, (CFDataRef)m_cfd, 0) != noErr)
#endif
#else /* MusicSequenceFileLoadData() requires 10.5 or later. */
	if (MusicSequenceFileLoadData(m_sequence, (CFDataRef)m_cfd, 0, 0) != noErr)
#endif
	{
		DisposeMusicSequence(m_sequence);
		CFRelease(m_cfd);
		return;
	}

	if (mus)
		mem_fclose(mus);
	if (midi)
		mem_fclose(midi);
}

#endif // OSX
