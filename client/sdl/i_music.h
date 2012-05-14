// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
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
//	SDL music handler
//
//-----------------------------------------------------------------------------

#ifndef __I_MUSIC_H__
#define __I_MUSIC_H__

#include "SDL_mixer.h"
#include "doomstat.h"

typedef struct
{
    Mix_Music *Track;
    SDL_RWops *Data;
} MusicHandler_t;

typedef enum
{
	MS_NONE			= 0,
	MS_SDLMIXER		= 1,
	MS_AUDIOUNIT	= 2,
	MS_PORTMIDI		= 3
} MusicSystemType;

bool S_MusicIsMus(byte* data, size_t length);
bool S_MusicIsMidi(byte* data, size_t length);
bool S_MusicIsOgg(byte* data, size_t length);
bool S_MusicIsMp3(byte* data, size_t length);
bool S_MusicIsWave(byte* data, size_t length);

//
//	MUSIC I/O
//
EXTERN_CVAR(snd_musicsystem)

void I_InitMusic(MusicSystemType musicsystem_type = (MusicSystemType)snd_musicsystem.asInt());
void STACK_ARGS I_ShutdownMusic(void);
// Volume.
void I_SetMusicVolume (float volume);
// PAUSE game handling.
void I_PauseSong();
void I_ResumeSong();
// Called by anything that wishes to start music.
//  plays a song, and when the song is done,
//  starts playing it again in an endless loop.
// Horrible thing to do, considering.
void I_PlaySong(byte* data, size_t length, bool loop);
// Stops a song over 3 seconds.
void I_StopSong();
void I_UpdateMusic();
void I_ResetMidiVolume();

#endif //__I_MUSIC_H__

