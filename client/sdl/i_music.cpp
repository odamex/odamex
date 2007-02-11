// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2006-2007 by The Odamex Team.
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

// [Russell] - define a temporary midi file, for consistency
// SDL < 1.2.7
#if MIX_MAJOR_VERSION < 1 || (MIX_MAJOR_VERSION == 1 && MIX_MINOR_VERSION < 2) || (MIX_MAJOR_VERSION == 1 && MIX_MINOR_VERSION == 2 && MIX_PATCHLEVEL < 7)
    #define TEMP_MIDI "temp_music"
#endif

Mix_Music *registered_tracks[MUSIC_TRACKS];
int current_track;
static bool music_initialized = false;

// [Russell] - A better name, since we support multiple formats now
void I_SetMusicVolume (float volume)
{
	if(!music_initialized)
		return;

	Mix_VolumeMusic((int)(volume * MIX_MAX_VOLUME));
}

void I_InitMusic (void)
{
	if(Args.CheckParm("-nomusic"))
	{
		Printf (PRINT_HIGH, "I_InitMusic: Music playback disabled\n");
		return;    
	}

	Printf (PRINT_HIGH, "I_InitMusic: Music playback enabled\n");
	
	music_initialized = true;
}


void STACK_ARGS I_ShutdownMusic(void)
{
	if(!music_initialized)
		return;
	
	Mix_HaltMusic();
	I_UnRegisterSong(0);

	music_initialized = false;
}

void I_PlaySong (int handle, int _looping)
{
	if(!music_initialized)
		return;

	_looping = _looping ? -1 : 1;

	if(handle < 0 || handle >= MUSIC_TRACKS)
		return;

	if(!registered_tracks[handle])
		return;

	if(Mix_PlayMusic(registered_tracks[handle], _looping) == -1)
	{
		Printf(PRINT_HIGH, "Mix_PlayMusic: %s\n", Mix_GetError());
		current_track = 0;
		return;
	}
	else
		current_track = handle;
}

void I_PauseSong (int handle)
{
	if(!music_initialized)
		return;

	Mix_PauseMusic();
}

void I_ResumeSong (int handle)
{
	if(!music_initialized)
		return;
	
	Mix_ResumeMusic();
}

void I_StopSong (int handle)
{
	if(!music_initialized)
		return;
	
	Mix_FadeOutMusic(100);
	current_track = 0;
}

void I_UnRegisterSong (int handle)
{
	if(!music_initialized)
		return;
	
	if(handle < 0 || handle >= MUSIC_TRACKS)
		return;

	if(!registered_tracks[handle])
		return;

	if(handle == current_track)
		I_StopSong(current_track);

	Mix_FreeMusic(registered_tracks[handle]);
	registered_tracks[handle] = 0;
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

	Mix_Music *music = 0;
	
	switch(result)
    {
        case 1:
            Printf(PRINT_HIGH, "MUS is not valid\n");
            break;
        case 0:
        case 2:
            
		// older versions of sdl-mixer require a physical midi file to be read, 1.2.7+ can read from memory
#ifndef TEMP_MIDI // SDL >= 1.2.7

            SDL_RWops *rw = SDL_RWFromMem(mem_fgetbuf(midi), mem_fsize(midi));
            
            if (!rw)
            {
                Printf(PRINT_HIGH, "SDL_RWFromMem: %s\n", Mix_GetError());
                break;
            }
            
            music = Mix_LoadMUS_RW(rw);

			if(!music)
            {
                Printf(PRINT_HIGH, "Mix_LoadMUS_RW: %s\n", Mix_GetError());
                SDL_FreeRW(rw);
                
                break;
            }

			SDL_FreeRW(rw);

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

            registered_tracks[0] = music;
            
            break;
    }
        
	mem_fclose(mus);
	mem_fclose(midi);

	return 0;
}

bool I_QrySongPlaying (int handle)
{
	if(!music_initialized)
		return false;
	
	return Mix_PlayingMusic() ? true : false;
}

VERSION_CONTROL (i_music_cpp, "$Id$")

