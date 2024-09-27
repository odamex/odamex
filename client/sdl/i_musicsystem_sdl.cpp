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
//  Plays music utilizing the SDL_Mixer library and can handle a wide range
//  of music formats.
//
//-----------------------------------------------------------------------------

#include "odamex.h"

#include "i_musicsystem_sdl.h"

#include "i_sdl.h"
#include <SDL_mixer.h>

#include "i_music.h"
#include "mus2midi.h"

// [Russell] - define a temporary midi file, for consistency
// SDL < 1.2.7
#ifdef _XBOX
// Use the cache partition
#define TEMP_MIDI "Z:\\temp_music"
#elif MIX_MAJOR_VERSION < 1 || (MIX_MAJOR_VERSION == 1 && MIX_MINOR_VERSION < 2) || \
    (MIX_MAJOR_VERSION == 1 && MIX_MINOR_VERSION == 2 && MIX_PATCHLEVEL < 7)
#define TEMP_MIDI "temp_music"
#endif

EXTERN_CVAR(snd_musicvolume)

SdlMixerMusicSystem::SdlMixerMusicSystem() : m_isInitialized(false), m_registeredSong()
{
	Printf("I_InitMusic: Music playback enabled using SDL_Mixer.\n");
	m_isInitialized = true;
}

SdlMixerMusicSystem::~SdlMixerMusicSystem()
{
	if (!isInitialized())
		return;

	Mix_HaltMusic();

	_StopSong();
	m_isInitialized = false;
}

void SdlMixerMusicSystem::startSong(byte* data, size_t length, bool loop)
{
	if (!isInitialized())
		return;

	stopSong();

	if (!data || !length)
		return;

	_RegisterSong(data, length);

	if (!m_registeredSong.Track || !m_registeredSong.Data)
		return;

	if (Mix_PlayMusic(m_registeredSong.Track, loop ? -1 : 1) == -1)
	{
		Printf(PRINT_WARNING, "Mix_PlayMusic: %s\n", Mix_GetError());
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

	if (m_registeredSong.Track)
		Mix_FreeMusic(m_registeredSong.Track);

	m_registeredSong.Track = NULL;
	m_registeredSong.Data = NULL;
	if (m_registeredSong.Mem != NULL)
	{
		mem_fclose(m_registeredSong.Mem);
		m_registeredSong.Mem = NULL;
	}
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
		MEMFILE* mus = mem_fopen_read(data, length);
		m_registeredSong.Mem = mem_fopen_write();

		int result = mus2mid(mus, m_registeredSong.Mem);
		if (result == 0)
		{
			m_registeredSong.Data = SDL_RWFromMem(mem_fgetbuf(m_registeredSong.Mem),
			                                      mem_fsize(m_registeredSong.Mem));
		}
		else
		{
			Printf(PRINT_WARNING, "MUS is not valid\n");
		}

		mem_fclose(mus);
	}
	else
	{
		m_registeredSong.Data = SDL_RWFromMem(data, length);
	}

	if (!m_registeredSong.Data)
	{
		Printf(PRINT_WARNING, "SDL_RWFromMem: %s\n", SDL_GetError());
		return;
	}

#ifdef TEMP_MIDI
	// We're using an older version of SDL and must save the midi data
	// to a temporary file first
	FILE* fp = fopen(TEMP_MIDI, "wb+");
	if (!fp)
	{
		Printf(PRINT_WARNING,
		       "Could not open temporary music file %s, not playing track\n", TEMP_MIDI);
		return;
	}

	// Get the size of the music data
	SDL_RWseek(m_registeredSong.Data, 0, SEEK_END);
	size_t reglength = SDL_RWtell(m_registeredSong.Data);

	// Write the music data to the temporary file
	SDL_RWseek(m_registeredSong.Data, 0, SEEK_SET);
	char buf[1024];
	while (reglength)
	{
		size_t chunksize = reglength > sizeof(buf) ? sizeof(buf) : reglength;

		SDL_RWread(m_registeredSong.Data, buf, chunksize, 1);
		fwrite(buf, chunksize, 1, fp);
		reglength -= chunksize;
	}

	fclose(fp);
	// Read the midi data from the temporary file
	m_registeredSong.Track = Mix_LoadMUS(TEMP_MIDI);
	unlink(TEMP_MIDI); // remove the temporary file

	if (!m_registeredSong.Track)
	{
		Printf(PRINT_WARNING, "Mix_LoadMUSW: %s\n", Mix_GetError());
		return;
	}

#else

// We can read the midi data directly from memory
#ifdef SDL20
	m_registeredSong.Track = Mix_LoadMUS_RW(m_registeredSong.Data, 0);
#elif defined SDL12
	m_registeredSong.Track = Mix_LoadMUS_RW(m_registeredSong.Data);
#endif // SDL12

	if (!m_registeredSong.Track)
	{
		Printf(PRINT_WARNING, "Mix_LoadMUS_RW: %s\n", Mix_GetError());
		return;
	}

#endif // TEMP_MIDI
}