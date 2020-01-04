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
//	SDL music handler
//
//-----------------------------------------------------------------------------

#include <stdio.h>
#include <stdlib.h>

#include "win32inc.h"
#if defined(_WIN32) && !defined(_XBOX)
	#include <mmsystem.h>
#endif

#ifndef OSX
	#ifdef UNIX
		#include <sys/stat.h>
	#endif
#endif

#include "doomtype.h"
#include "m_argv.h"
#include "i_music.h"
#include "w_wad.h"
#include "c_console.h"
#include "c_dispatch.h"
#include "z_zone.h"
#include "i_system.h"
#include "resources/res_identifier.h"

#include <SDL_mixer.h>
#include "mus2midi.h"
#include "i_musicsystem.h"

MusicSystem* musicsystem = NULL;
MusicSystemType current_musicsystem_type = MS_NONE;

void S_StopMusic();
void S_ChangeMusic (std::string musicname, int looping);

EXTERN_CVAR (snd_musicvolume)
EXTERN_CVAR (snd_musicsystem)


std::string currentmusic;

//
// I_ResetMidiVolume()
//
// [SL] 2011-12-31 - Set all midi devices' output volume to maximum in the OS.
// This function is used to work around shortcomings of the SDL_Mixer library
// on the Windows Vista/7 platform, where PCM and MIDI volumes are linked
// together in the OS's audio mixer.  Because SDL_Mixer sets the volume of
// midi output devices to 0 when not playing music, all sound
// output (PCM & MIDI) becomes muted in Odamex (see Odamex bug 443).
//
void I_ResetMidiVolume()
{
	#if defined(_WIN32) && !defined(_XBOX)
	SDL_LockAudio();

	for (UINT device = MIDI_MAPPER; device != midiOutGetNumDevs(); device++)
	{
		MIDIOUTCAPS caps;
		// Can this midi device change volume?
		MMRESULT result = midiOutGetDevCaps(device, &caps, sizeof(caps));

		// Set the midi device's volume
		static const DWORD volume = 0xFFFFFFFF;		// maximum volume
		if (result == MMSYSERR_NOERROR && (caps.dwSupport & MIDICAPS_VOLUME))
			midiOutSetVolume((HMIDIOUT)device, volume);
	}

	SDL_UnlockAudio();
	#endif	// _WIN32
}

//
// I_UpdateMusic()
//
// Play the next chunk of music for the current gametic
//
void I_UpdateMusic()
{
	if (musicsystem)
		musicsystem->playChunk();
}

// [Russell] - A better name, since we support multiple formats now
void I_SetMusicVolume (float volume)
{
	if (musicsystem)
		musicsystem->setVolume(volume);
}

void I_InitMusic(MusicSystemType musicsystem_type)
{
	I_ShutdownMusic();
	I_ResetMidiVolume();

	if (I_IsHeadless() || Args.CheckParm("-nosound") || Args.CheckParm("-nomusic") || snd_musicsystem == MS_NONE)
	{
		// User has chosen to disable music
		musicsystem = new SilentMusicSystem();
		current_musicsystem_type = MS_NONE;
		return;
	}

	switch ((int)musicsystem_type)
	{
		#ifdef OSX
		case MS_AUDIOUNIT:
			musicsystem = new AuMusicSystem();
			break;
		#endif	// OSX

		#ifdef PORTMIDI
		case MS_PORTMIDI:
			musicsystem = new PortMidiMusicSystem();
			break;
		#endif	// PORTMIDI

		case MS_SDLMIXER:	// fall through
		default:
			musicsystem = new SdlMixerMusicSystem();
			break;
	}

	current_musicsystem_type = musicsystem_type;
}

void STACK_ARGS I_ShutdownMusic(void)
{
	if (musicsystem)
	{
		delete musicsystem;
		musicsystem = NULL;
	}
}

CVAR_FUNC_IMPL (snd_musicsystem)
{
	if ((int)current_musicsystem_type == snd_musicsystem.asInt())
		return;

	if (musicsystem)
	{
		I_ShutdownMusic();
		S_StopMusic();
	}
	I_InitMusic();
	
	if (!level.music || level.music[0] == 0)
		S_ChangeMusic(currentmusic.c_str(), true);	
	else
		S_ChangeMusic(std::string(level.music, 8), true);
}

//
// I_SelectMusicSystem
//
// Takes the data and length of a song and determines which music system
// should be used to play the song, based on user preference and the song
// type.
//
static MusicSystemType I_SelectMusicSystem(byte *data, size_t length)
{
	// Always honor the no-music preference
	if (snd_musicsystem == MS_NONE)
		return MS_NONE;

	bool ismidi = (Res_MusicIsMus(data, length) || Res_MusicIsMidi(data, length));

	if (ismidi)
		return static_cast<MusicSystemType>(snd_musicsystem.asInt());

	// Non-midi music always uses SDL_Mixer (for now at least)
	return MS_SDLMIXER;
}

void I_PlaySong(byte* data, size_t length, bool loop)
{
	if (!musicsystem)
		return;

	MusicSystemType newtype = I_SelectMusicSystem(data, length);
	if (newtype != current_musicsystem_type)
	{
		if (musicsystem)
		{
			I_ShutdownMusic();
			S_StopMusic();
		}
		I_InitMusic(newtype);
	}

	musicsystem->startSong(data, length, loop);

	// Hack for problems with Windows Vista/7 & SDL_Mixer
	// See comment for I_ResetMidiVolume().
	I_ResetMidiVolume();

	I_SetMusicVolume(snd_musicvolume);
}

void I_PauseSong()
{
	if (musicsystem)
		musicsystem->pauseSong();
}

void I_ResumeSong()
{
	if (musicsystem)
		musicsystem->resumeSong();
}

void I_StopSong()
{
	if (musicsystem)
		musicsystem->stopSong();
}

bool I_QrySongPlaying (int handle)
{
	if (musicsystem)
		return musicsystem->isPlaying();

	return false;
}

VERSION_CONTROL (i_music_cpp, "$Id$")

