// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2006-2009 by The Odamex Team.
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
//	SDL music handler
//
//-----------------------------------------------------------------------------


#include <stdio.h>
#include <stdlib.h>

#include "doomtype.h"
#include "m_argv.h"
#include "i_music.h"
#include "w_wad.h"
#include "c_console.h"
#include "c_dispatch.h"
#include "z_zone.h"
#include "i_system.h"

#include "SDL_mixer.h"
#include "mus2midi.h"

#define MUSIC_TRACKS 1

// denis - midi via SDL+timidity on OSX crashes miserably after a while
// this is not our fault, but we have to live with it until someone
// bothers to fix it, therefore use native midi on OSX for now
#ifdef OSX
#include <AudioToolbox/AudioToolbox.h>
static MusicPlayer player;
static MusicSequence sequence;
static AUGraph graph;
static AUNode synth, output;
static AudioUnit unit;
static CFDataRef cfd;
#endif

// [Russell] - define a temporary midi file, for consistency
// SDL < 1.2.7
#if MIX_MAJOR_VERSION < 1 || (MIX_MAJOR_VERSION == 1 && MIX_MINOR_VERSION < 2) || (MIX_MAJOR_VERSION == 1 && MIX_MINOR_VERSION == 2 && MIX_PATCHLEVEL < 7)
    #define TEMP_MIDI "temp_music"
#endif

typedef struct 
{
    Mix_Music *Track;
    SDL_RWops *Data;
} MusicHandler_t;

MusicHandler_t registered_tracks[MUSIC_TRACKS];
int current_track;
static bool music_initialized = false;

// [Nes] - For pausing the music during... pause.
int curpause = 0;
EXTERN_CVAR (snd_musicvolume)

// [Russell] - A better name, since we support multiple formats now
void I_SetMusicVolume (float volume)
{
	if(!music_initialized)
		return;

    if (curpause && volume != 0.0)
        return;

#ifdef OSX

	if(AudioUnitSetParameter(unit, kAudioUnitParameterUnit_LinearGain, kAudioUnitScope_Output, 0, volume, 0) != noErr)
	{
		Printf (PRINT_HIGH, "I_InitMusic: AudioUnitSetParameter failed\n");
		return;
	}

#else

	Mix_VolumeMusic((int)(volume * MIX_MAX_VOLUME));

#endif
}

void I_InitMusic (void)
{
	if(Args.CheckParm("-nomusic"))
	{
		Printf (PRINT_HIGH, "I_InitMusic: Music playback disabled\n");
		return;
	}

#ifdef OSX

	NewAUGraph(&graph);

	ComponentDescription d;

	d.componentType = kAudioUnitType_MusicDevice;
	d.componentSubType = kAudioUnitSubType_DLSSynth;
	d.componentManufacturer = kAudioUnitManufacturer_Apple;
	d.componentFlags = 0;
	d.componentFlagsMask = 0;
	AUGraphNewNode(graph, &d, 0, NULL, &synth);

	d.componentType = kAudioUnitType_Output;
	d.componentSubType = kAudioUnitSubType_DefaultOutput;
	d.componentManufacturer = kAudioUnitManufacturer_Apple;
	d.componentFlags = 0;
	d.componentFlagsMask = 0;
	AUGraphNewNode(graph, &d, 0, NULL, &output);

	if(AUGraphConnectNodeInput(graph, synth, 0, output, 0) != noErr)
	{
		Printf (PRINT_HIGH, "I_InitMusic: AUGraphConnectNodeInput failed\n");
		return;
	}

	if(AUGraphOpen(graph) != noErr)
	{
		Printf (PRINT_HIGH, "I_InitMusic: AUGraphOpen failed\n");
		return;
	}

	if(AUGraphInitialize(graph) != noErr)
	{
		Printf (PRINT_HIGH, "I_InitMusic: AUGraphInitialize failed\n");
		return;
	}

	if(AUGraphGetNodeInfo(graph, output, NULL, NULL, NULL, &unit) != noErr)
	{
		Printf (PRINT_HIGH, "I_InitMusic: AUGraphGetNodeInfo failed\n");
		return;
	}

	if(NewMusicPlayer(&player) != noErr)
	{
		Printf (PRINT_HIGH, "I_InitMusic: Music player creation failed using AudioToolbox\n");
		return;
	}

	Printf (PRINT_HIGH, "I_InitMusic: Music playback enabled using AudioToolbox\n");

#else

	Printf (PRINT_HIGH, "I_InitMusic: Music playback enabled\n");

#endif

	music_initialized = true;
}


void STACK_ARGS I_ShutdownMusic(void)
{
	if(!music_initialized)
		return;

#ifdef OSX

	DisposeMusicPlayer(player);
	AUGraphClose(graph);

#else

	Mix_HaltMusic();

#endif

	I_UnRegisterSong(0);

	music_initialized = false;
}

void I_PlaySong (int handle, int _looping)
{
	if(!music_initialized)
		return;

	if(--handle < 0 || handle >= MUSIC_TRACKS)
		return;

	if(!registered_tracks[handle].Track)
		return;

#ifdef OSX

	if(MusicSequenceSetAUGraph(sequence, graph) != noErr)
	{
		Printf (PRINT_HIGH, "I_PlaySong: MusicSequenceSetAUGraph failed\n");
		return;
	}

	if(MusicPlayerSetSequence(player, sequence) != noErr)
	{
		Printf (PRINT_HIGH, "I_PlaySong: MusicPlayerSetSequence failed\n");
		return;
	}

	if(MusicPlayerPreroll(player) != noErr)
	{
		Printf (PRINT_HIGH, "I_PlaySong: MusicPlayerPreroll failed\n");
		return;
	}

	UInt32 outNumberOfTracks = 0;
	if(MusicSequenceGetTrackCount(sequence, &outNumberOfTracks) != noErr)
	{
		Printf (PRINT_HIGH, "I_PlaySong: MusicSequenceGetTrackCount failed\n");
		return;
	}

	for(UInt32 i = 0; i < outNumberOfTracks; i++)
	{
		MusicTrack track;

		if(MusicSequenceGetIndTrack(sequence, i, &track) != noErr)
		{
			Printf (PRINT_HIGH, "I_PlaySong: MusicSequenceGetIndTrack failed\n");
			return;
		}

		struct s_loopinfo
		{
			MusicTimeStamp time;
			long loops;
		}LoopInfo;

		UInt32 inLength = sizeof(LoopInfo);

		if(MusicTrackGetProperty(track, kSequenceTrackProperty_LoopInfo, &LoopInfo, &inLength) != noErr)
		{
			Printf (PRINT_HIGH, "I_PlaySong: MusicTrackGetProperty failed\n");
			return;
		}

		inLength = sizeof(LoopInfo.time);

		if(MusicTrackGetProperty(track, kSequenceTrackProperty_TrackLength, &LoopInfo.time, &inLength) != noErr)
		{
			Printf (PRINT_HIGH, "I_PlaySong: MusicTrackGetProperty failed\n");
			return;
		}

		LoopInfo.loops = _looping ? 0 : 1;

		if(MusicTrackSetProperty(track, kSequenceTrackProperty_LoopInfo, &LoopInfo, sizeof(LoopInfo)) != noErr)
		{
			Printf (PRINT_HIGH, "I_PlaySong: MusicTrackSetProperty failed\n");
			return;
		}
	}

	if(MusicPlayerStart(player) != noErr)
	{
		Printf (PRINT_HIGH, "I_PlaySong: MusicPlayerStart failed\n");
		return;
	}

#else

	if(Mix_PlayMusic(registered_tracks[handle].Track, _looping ? -1 : 1) == -1)
	{
		Printf(PRINT_HIGH, "Mix_PlayMusic: %s\n", Mix_GetError());
		current_track = 0;
		return;
	}

    // [Russell] - Hack for setting the volume on windows vista, since it gets
    // reset on every music change
    I_SetMusicVolume(snd_musicvolume);

#endif

	current_track = handle;
}

void I_PauseSong (int handle)
{
	if(!music_initialized)
		return;

    curpause = 1;
    I_SetMusicVolume (0.0);

#ifndef OSX
	Mix_PauseMusic();
#endif
}

void I_ResumeSong (int handle)
{
	if(!music_initialized)
		return;

    curpause = 0;
    I_SetMusicVolume (snd_musicvolume);

#ifndef OSX
	Mix_ResumeMusic();
#endif
}

void I_StopSong (int handle)
{
	if(!music_initialized)
		return;

#ifdef OSX

	MusicPlayerStop(player);

#else

	Mix_FadeOutMusic(100);
	current_track = 0;

#endif
}

void I_UnRegisterSong (int handle)
{
	if(!music_initialized)
		return;

	if(handle < 0 || handle >= MUSIC_TRACKS)
		return;

	if(!registered_tracks[handle].Track)
		return;

	if(handle == current_track)
		I_StopSong(current_track);

#ifdef OSX

	DisposeMusicSequence(sequence);

#else

    if (registered_tracks[handle].Track)
        Mix_FreeMusic(registered_tracks[handle].Track);
    
    //if (registered_tracks[handle].Data)
        //SDL_FreeRW(registered_tracks[handle].Data);

#endif

	registered_tracks[handle].Track = NULL;
	registered_tracks[handle].Data = NULL;

}

int I_RegisterSong (char *data, size_t musicLen)
{
	if(!music_initialized)
		return 0;

	// input mus memory file and midi
	MEMFILE *mus = mem_fopen_read(data, musicLen);
	MEMFILE *midi = mem_fopen_write();

	I_UnRegisterSong(0);

	int result = mus2mid(mus, midi);

	switch(result)
    {
        case 1:
            Printf(PRINT_HIGH, "MUS is not valid\n");
            break;
        case 0:
        case 2:
		{
#ifdef OSX

		if (NewMusicSequence(&sequence) != noErr)
			return 0;

		cfd = CFDataCreate(NULL, (const Uint8 *)mem_fgetbuf(midi), mem_fsize(midi));

		if(!cfd)
		{
			DisposeMusicSequence(sequence);
			return 0;
		}

		if (MusicSequenceLoadSMFData(sequence, (CFDataRef)cfd) != noErr)
		{
			DisposeMusicSequence(sequence);
			CFRelease(cfd);
			return 0;
		}

		registered_tracks[0].Track = (Mix_Music*)1;

#else

		Mix_Music *music = 0;

		// older versions of sdl-mixer require a physical midi file to be read, 1.2.7+ can read from memory
#ifndef TEMP_MIDI // SDL >= 1.2.7
            
            if (result == 0) // it is a midi
            {
                registered_tracks[0].Data = SDL_RWFromMem(mem_fgetbuf(midi), mem_fsize(midi));
            }
            else // it is another format
            {
                registered_tracks[0].Data = SDL_RWFromMem(data, musicLen);
            }
            

            if (!registered_tracks[0].Data)
            {
                Printf(PRINT_HIGH, "SDL_RWFromMem: %s\n", SDL_GetError());
                break;
            }

            music = Mix_LoadMUS_RW(registered_tracks[0].Data);

			if(!music)
            {
                Printf(PRINT_HIGH, "Mix_LoadMUS_RW: %s\n", Mix_GetError());
                
                SDL_FreeRW(registered_tracks[0].Data);
                registered_tracks[0].Data = NULL;
                
                break;
            }

#else // SDL <= 1.2.6 - Create a file so it can load the midi

			FILE *fp = fopen(TEMP_MIDI, "wb+");

			if(!fp)
			{
				Printf(PRINT_HIGH, "Could not open temporary music file %s, not playing track\n", TEMP_MIDI);

				break;
			}

			for(int i = 0; i < mem_fsize(midi); i++)
				fputc(mem_fgetbuf(midi)[i], fp);

			fclose(fp);

            music = Mix_LoadMUS(TEMP_MIDI);

            if(!music)
			{
				Printf(PRINT_HIGH, "Mix_LoadMUS: %s\n", Mix_GetError());
				break;
			}

#endif

		registered_tracks[0].Track = music;

#endif // OSX
            break;
		} // case 2
    }

	mem_fclose(mus);
	mem_fclose(midi);

	return 1;
}

bool I_QrySongPlaying (int handle)
{
	if(!music_initialized)
		return false;

#ifdef OSX

	Boolean result;
	MusicPlayerIsPlaying(player, &result);
	return result;

#else

	return Mix_PlayingMusic() ? true : false;

#endif
}

VERSION_CONTROL (i_music_cpp, "$Id$")

