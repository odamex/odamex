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
//	SDL music handler
//
//-----------------------------------------------------------------------------

#pragma once

#include <SDL_mixer.h>

#include "m_memio.h"

struct MusicHandler_t
{
	Mix_Music* Track;
	SDL_RWops* Data;
	MEMFILE* Mem;
	MusicHandler_t() : Track(NULL), Data(NULL), Mem(NULL) { }
};

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

// [ML] Keep track of the currently loaded music lump name
extern std::string currentmusic;

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
