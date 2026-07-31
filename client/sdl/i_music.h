// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2006-2026 by The Odamex Team.
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

#include <cstddef>
#include <cstdint>
#include <string>
#include <SDL_mixer.h>

#include "c_cvars.h"
#include "doomtype.h"
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
	MS_PORTMIDI		= 3,
	MS_LIBADLMIDI		= 4,

	MS_AUTO			= 255
} MusicSystemType;

//
// I_GetDefaultMusicSystem
//
// The music system to use when the user has expressed no preference, which
// depends on what the platform does best.
//
// TODO: convert this to consteval when merging to protobreak
constexpr MusicSystemType I_GetDefaultMusicSystem()
{
#ifdef _WIN32
	return MS_PORTMIDI;
#elif defined OSX
	return MS_AUDIOUNIT;
#elif defined __linux__
	return MS_LIBADLMIDI;
#else
	return MS_SDLMIXER;
#endif
}

//
// I_ResolveMusicSystem
//
// Turns whatever snd_musicsystem holds into a music system we can actually
// start.
//
constexpr MusicSystemType I_ResolveMusicSystem(int musicsystem_type)
{
	if (musicsystem_type == MS_AUTO)
		return I_GetDefaultMusicSystem();

	return static_cast<MusicSystemType>(musicsystem_type);
}

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

void I_InitMusic(const MusicSystemType musicsystem_type = I_ResolveMusicSystem(snd_musicsystem.asInt()));
void I_ShutdownMusic();
// Volume.
void I_SetMusicVolume (float volume);
// PAUSE game handling.
void I_PauseSong();
void I_ResumeSong();
// Called by anything that wishes to start music.
//  plays a song, and when the song is done,
//  starts playing it again in an endless loop.
// Horrible thing to do, considering.
void I_PlaySong(const OByteSpan data, const bool loop, const int order);
// Stops a song over 3 seconds.
void I_StopSong();
void I_UpdateMusic();
void I_ResetMidiVolume();
