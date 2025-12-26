// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2000-2006 by Sergey Makovkin (CSDoom .62).
// Copyright (C) 2006-2025 by The Odamex Team.
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
//	none
//
//-----------------------------------------------------------------------------


#include "odamex.h"

#include "m_alloc.h"
#include "i_system.h"
#include "s_sound.h"
#include "c_dispatch.h"
#include "z_zone.h"
#include "m_random.h"
#include "w_wad.h"
#include "cmdlib.h"
#include "oscanner.h"
#include "v_video.h"

#include <algorithm>

#define NORM_PITCH				128
#define NORM_PRIORITY			64
#define NORM_SEP				128

#define S_PITCH_PERTURB 		1
#define S_STEREO_SWING			(96<<FRACBITS)

//
// [RH] Print sound debug info. Called from D_Display()
//
void S_NoiseDebug()
{
}


//
// Internals.
//

//
// Initializes sound stuff, including volume
// Sets channels, SFX and music volume,
// allocates channel buffer, sets S_sfx lookup.
//
void S_Init(float sfxVolume, float musicVolume)
{
	// [RH] Read in sound sequences
	//NumSequences = 0;
}

void S_Start()
{
}

void S_Stop()
{
}

void S_SoundID(int channel, int sound_id, float volume, int attenuation)
{
}

void S_SoundID(AActor *ent, int channel, int sound_id, float volume, int attenuation)
{
}

void S_SoundID(fixed_t *pt, int channel, int sound_id, float volume, int attenuation)
{
}

void S_LoopedSoundID(AActor *ent, int channel, int sound_id, float volume, int attenuation)
{
}

void S_LoopedSoundID(fixed_t *pt, int channel, int sound_id, float volume, int attenuation)
{
}

// [Russell] - Hack to stop multiple plat stop sounds
void S_PlatSound(fixed_t *pt, int channel, const char *name, float volume, int attenuation)
{
}

void S_Sound(int channel, const char *name, float volume, int attenuation)
{
}

void S_Sound(AActor *ent, int channel, const char *name, float volume, int attenuation)
{
}

void S_Sound(fixed_t *pt, int channel, const char *name, float volume, int attenuation)
{
}

void S_LoopedSound(AActor *ent, int channel, const char *name, float volume, int attenuation)
{
}

void S_LoopedSound(fixed_t *pt, int channel, const char *name, float volume, int attenuation)
{
}

void S_Sound(fixed_t x, fixed_t y, int channel, const char *name, float volume, int attenuation)
{
}

void S_StopSound(fixed_t *pt)
{
}

void S_StopSound(fixed_t *pt, int channel)
{
}

void S_StopSound(AActor *ent, int channel)
{
}

void S_StopAllChannels()
{
}

void S_StopAmbientSound()
{
}

void S_PauseSound()
{
}

void S_ResumeSound()
{
}

// Moves all the sounds from one thing to another. If the destination is
// NULL, then the sound becomes a positioned sound.
void S_RelinkSound(AActor *from, AActor *to)
{
}

bool S_GetSoundPlayingInfo(fixed_t *pt, int sound_id)
{
	return false;
}

bool S_GetSoundPlayingInfo(AActor *ent, int sound_id)
{
	return S_GetSoundPlayingInfo (ent ? &ent->x : NULL, sound_id);
}

//
// Stop and resume music, during game PAUSE.
//
void S_PauseMusic()
{
}

void S_ResumeMusic()
{
}

//
// Updates music & sounds
//
void S_UpdateSounds(void *listener_p)
{
}

void S_UpdateMusic()
{
}

void S_SetMusicVolume(float volume)
{
}

void S_SetSfxVolume(float volume)
{
}

//
// Starts some music with the music id found in sounds.h.
//
void S_StartMusic(const char *m_id)
{
}

// [RH] S_ChangeMusic() now accepts the name of the music lump.
// It's up to the caller to figure out what that name is.
void S_ChangeMusic(std::string musicname, bool looping, int order)
{
}

void S_StopMusic()
{
}


// [RH] ===============================
//
//	Ambient sound and SNDINFO routines
//
// =============================== [RH]

void A_Ambient(AActor *actor)
{
}

void S_ActivateAmbient(AActor *origin, int ambient)
{
}

VERSION_CONTROL (s_sound_cpp, "$Id$")
