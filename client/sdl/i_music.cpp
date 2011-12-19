// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
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
//	SDL music handler
//
//-----------------------------------------------------------------------------

#include <stdio.h>
#include <stdlib.h>

#if defined(_WIN32) && !defined(_XBOX)
	#define WIN32_LEAN_AND_MEAN
	#include <windows.h>
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

#include "SDL_mixer.h"
#include "mus2midi.h"
#include "i_musicsystem.h"

MusicSystem* musicsystem = NULL;
MusicSystemType current_musicsystem_type = MS_NONE;

void S_StopMusic();
void S_ChangeMusic (std::string musicname, int looping);

EXTERN_CVAR (snd_musicvolume)
EXTERN_CVAR (snd_musicsystem)

// [Russell] - From prboom+
#if defined(_WIN32) && !defined(_XBOX)
void I_midiOutSetVolumes(int volume)
{
  MMRESULT result;
  int calcVolume;
  MIDIOUTCAPS capabilities;
  unsigned int i;

  if (volume > 128)
    volume = 128;
  if (volume < 0)
    volume = 0;
  calcVolume = (65535 * volume / 128);

  SDL_LockAudio();

  //Device loop
  for (i = 0; i < midiOutGetNumDevs(); i++)
  {
    //Get device capabilities
    result = midiOutGetDevCaps(i, &capabilities, sizeof(capabilities));

    if (result == MMSYSERR_NOERROR)
    {
      //Adjust volume on this candidate
      if ((capabilities.dwSupport & MIDICAPS_VOLUME))
      {
        midiOutSetVolume((HMIDIOUT)i, MAKELONG(calcVolume, calcVolume));
      }
    }
  }

  SDL_UnlockAudio();
}
#endif

// [Russell] - A better name, since we support multiple formats now
void I_SetMusicVolume (float volume)
{
	if (musicsystem)
		musicsystem->setVolume(volume);
}

void I_InitMusic (void)
{
#ifndef OSX
#ifdef UNIX
	struct stat buf;
	if(stat("/etc/timidity.cfg", &buf) && stat("/etc/timidity/timidity.cfg", &buf))
		Args.AppendArg("-nomusic");
#endif
#endif

	I_ShutdownMusic();

	if(Args.CheckParm("-nosound") || Args.CheckParm("-nomusic") || snd_musicsystem == MS_NONE)
	{
		// User has chosen to disable music
		musicsystem = new SilentMusicSystem();
		current_musicsystem_type = MS_NONE;
		return;
	}
	
	switch (snd_musicsystem.asInt())
	{
		#ifdef OSX
		case MS_AU:
			musicsystem = new AuMusicSystem();
			break;
		#endif	// OSX
		
		case MS_SDLMIXER:	// fall through
		default:
			musicsystem = new SdlMixerMusicSystem();
			break;
	}
	
	current_musicsystem_type = static_cast<MusicSystemType>(snd_musicsystem.asInt());
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
	if (current_musicsystem_type == snd_musicsystem)
		return;
		
	S_StopMusic();
	I_ShutdownMusic();
	I_InitMusic();
	S_ChangeMusic(std::string(level.music, 8), true);
}

void I_PlaySong(byte* data, size_t length, bool loop)
{
	if (musicsystem)
		musicsystem->playSong(data, length, loop);
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

