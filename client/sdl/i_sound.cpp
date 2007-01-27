// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
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
//
// DESCRIPTION:
//	System interface, sound.
//	[Odamex] Fitted to work with SDL
//
//-----------------------------------------------------------------------------


#include <SDL.h>
#include <SDL_mixer.h>
#include <stdio.h>
#include <stdlib.h>

#include "z_zone.h"

#include "i_system.h"
#include "i_sound.h"
#include "i_music.h"
#include "m_argv.h"
#include "m_misc.h"
#include "w_wad.h"

#include "doomdef.h"

#define NUM_CHANNELS 16

static bool sound_initialized = false;
static int sounds_in_use[256];
static int nextchannel = 0;

CVAR (snd_crossover, "0", CVAR_ARCHIVE);

static Uint8 *expand_sound_data(Uint8 *data, int samplerate, int length)
{
   Uint8 *expanded = NULL;
   int expanded_length;
   int expand_ratio;
   int i;

   if (samplerate == 11025)
   {
      // Most of Doom's sound effects are 11025Hz
      // need to expand to 2 channels, 11025->22050

      expanded = (Uint8 *)Z_Malloc(length * 4, PU_STATIC, NULL);

      for (i = 0; i < length; ++i)
      {
         expanded[(i*4) + 0] = expanded[(i*4) + 2] = data[i]; // chan L sample +0,+1
         expanded[(i*4) + 1] = expanded[(i*4) + 3] = data[i]; // chan R sample +0,+1
      }
   }
   else if (samplerate == 22050)
   {
      expanded = (Uint8 *)Z_Malloc(length * 2, PU_STATIC, NULL);

      for (i = 0; i < length; ++i)
      {
          expanded[i*2 + 0] = data[i]; // chan L
          expanded[i*2 + 1] = data[i]; // chan R
      }
   }
   else
   {
      // Generic expansion function for all other sample rates

      // number of samples in the converted sound

      expanded_length = (length * 22050) / samplerate;
      expand_ratio = (length << 8) / expanded_length;

      expanded = (Uint8 *)Z_Malloc(expanded_length * 2, PU_STATIC, NULL);

      for (i=0; i<expanded_length; ++i)
      {
         int src = (i * expand_ratio) >> 8;

         expanded[i*2 + 0] = data[src]; // chan L
         expanded[i*2 + 1] = data[src]; // chan R
      }
   }

	return expanded;
}


// Expands the 11025Hz, 8bit, mono sound effects in Doom to
// 22050Hz, 16bit stereo

static Uint8 *ExpandSoundData(Uint8 *data, int samplerate, int length)
{
   Uint8 *expanded = NULL;
   int expanded_length;
   int expand_ratio;
   int i;

   if (samplerate == 11025)
   {
        // Most of Doom's sound effects are 11025Hz

        // need to expand to 2 channels, 11025->22050 and 8->16 bit

      expanded = (Uint8 *)Z_Malloc(length * 8, PU_STATIC, NULL);

      for (i=0; i<length; ++i)
      {
         Uint16 sample;

         sample = data[i] | (data[i] << 8);
         sample -= 32768;

         expanded[i * 8] = expanded[i * 8 + 2]
               = expanded[i * 8 + 4] = expanded[i * 8 + 6] = sample & 0xff;
         expanded[i * 8 + 1] = expanded[i * 8 + 3]
               = expanded[i * 8 + 5] = expanded[i * 8 + 7] = (sample >> 8) & 0xff;
      }
   }
   else if (samplerate == 22050)
   {

      expanded = (Uint8 *)Z_Malloc(length * 4, PU_STATIC, NULL);

      for (i=0; i<length; ++i)
      {
         Uint16 sample;

         sample = data[i] | (data[i] << 8);
         sample -= 32768;

         expanded[i * 4] = expanded[i * 4 + 2] = sample & 0xff;
         expanded[i * 4 + 1] = expanded[i * 4 + 3] = (sample >> 8) & 0xff;
      }
   }
   else
   {
        // Generic expansion function for all other sample rates

        // number of samples in the converted sound
      expanded_length = (length * 22050) / samplerate;
      expand_ratio = (length << 8) / expanded_length;

      expanded = (Uint8 *)Z_Malloc(expanded_length * expand_ratio, PU_STATIC, NULL);


      for (i=0; i<expanded_length; ++i)
      {
         Uint16 sample;
         int src;

         src = (i * expand_ratio) >> 8;

         sample = data[src] | (data[src] << 8);
         sample -= 32768;

            // expand 8->16 bits, mono->stereo

         expanded[i * 4] = expanded[i * 4 + 2] = sample & 0xff;
         expanded[i * 4 + 1] = expanded[i * 4 + 3] = (sample >> 8) & 0xff;
      }
   }

   return expanded;
}

static void getsfx (struct sfxinfo_struct *sfx)
{
	int samplerate;
	int length ,expanded_length;
	Uint8 *data;
	Mix_Chunk *chunk;

        data = (Uint8 *)W_CacheLumpNum(sfx->lumpnum, PU_STATIC);

	samplerate = (data[3] << 8) | data[2];
	length = (data[5] << 8) | data[4];

        expanded_length = (length * 22050) / (samplerate / 4);

	chunk = (Mix_Chunk *)Z_Malloc(sizeof(Mix_Chunk), PU_STATIC, NULL);
	chunk->allocated = 1;
        chunk->abuf = (Uint8 *)ExpandSoundData(data + 8, samplerate, length);
	chunk->alen = expanded_length;
	chunk->volume = MIX_MAX_VOLUME;
	sfx->data = chunk;
}

//
// SFX API
//
void I_SetChannels (int numchannels)
{
}

static float basevolume;

void I_SetSfxVolume (float volume)
{
	basevolume = volume;
}

static int soundtag = 0;

//
// Starting a sound means adding it
//	to the current list of active sounds
//  in the internal channels.
// As the SFX info struct contains
//  e.g. a pointer to the raw data,
//  it is ignored.
// As our sound handling does not handle
//  priority, it is ignored.
// Pitching (that is, increased speed of playback)
//  is set, but currently not used by mixing.
//
int I_StartSound (int id, int vol, int sep, int pitch, bool loop)
{
	if(!sound_initialized)
		return 0;

	Mix_Chunk *chunk = (Mix_Chunk *)S_sfx[id].data;
	int channel;

	// find a free channel, starting from the first after
	// the last channel we used

	channel = nextchannel;

	do
	{
		channel = (channel + 1) % NUM_CHANNELS;

		if (channel == nextchannel)
		{
			fprintf(stderr, "No free sound channels left.\n");
			return -1;
		}
        } while (sounds_in_use[channel] != -1);

	nextchannel = channel;

	// play sound

	Mix_PlayChannelTimed(channel, chunk, loop ? -1 : 0, -1);

	sounds_in_use[channel] = soundtag;
        channel |= (soundtag << 8);
        soundtag++;

	// set seperation, etc.

	I_UpdateSoundParams(channel, vol, sep, pitch);

	return channel;
}


void I_StopSound (int handle)
{
	if(!sound_initialized)
		return;

        int tag = handle >> 8;

	handle &= 0xff;

        // Stopping wrong sound sounds_in_use[handle] != tag (handle)

	if (sounds_in_use[handle] != tag)
            fprintf(stderr, "stopping wrong sound: %i != %i (%i)\n", sounds_in_use[handle], tag, handle);

	sounds_in_use[handle] = -1;

	Mix_HaltChannel(handle);
}



int I_SoundIsPlaying (int handle)
{
	if(!sound_initialized)
		return 0;

	handle &= 0xff;
	return Mix_Playing(handle);
}


void I_UpdateSoundParams (int handle, float vol, int sep, int pitch)
{
	if(!sound_initialized)
		return;

        int tag = handle >> 8;

	handle &= 0xff;

	if (sounds_in_use[handle] != tag)
            fprintf(stderr, "tag is wrong playing sound: tag:%i, handle:%i\n", tag, handle);

	if(sep > 255)
		sep = 255;

	if(snd_crossover)
		sep = 255 - sep;

	int volume = (int)((float)MIX_MAX_VOLUME * basevolume * vol);

	if(volume < 0)
		volume = 0;
	if(volume > MIX_MAX_VOLUME)
		volume = MIX_MAX_VOLUME;

	Mix_Volume(handle, volume);
	Mix_SetPanning(handle, sep, 255-sep);
}

void I_LoadSound (struct sfxinfo_struct *sfx)
{
	if (!sfx->data)
	{
		DPrintf ("loading sound \"%s\" (%d)\n", sfx->name, sfx->lumpnum);
		getsfx (sfx);
	}
}

void I_InitSound (void)
{
	if(Args.CheckParm("-nosound"))
		return;

	Printf(PRINT_HIGH, "I_InitSound: Initializing SDL_mixer\n");

	if (SDL_InitSubSystem(SDL_INIT_AUDIO) < 0)
	{
		Printf(PRINT_HIGH, "Unable to set up sound.\n");
		return;
	}

	const SDL_version *ver = Mix_Linked_Version();

	if(ver->major != MIX_MAJOR_VERSION
		|| ver->minor != MIX_MINOR_VERSION
		|| ver->patch != MIX_PATCHLEVEL)
	{
		Printf(PRINT_HIGH, "I_InitSound: SDL_mixer version conflict (%d.%d.%d vs %d.%d.%d dll)\n",
			MIX_MAJOR_VERSION, MIX_MINOR_VERSION, MIX_PATCHLEVEL,
			ver->major, ver->minor, ver->patch);
		return;
	}


        if (Mix_OpenAudio(22050, AUDIO_S16LSB, 2, 1024) < 0)
	{
		Printf(PRINT_HIGH, "Error initializing SDL_mixer: %s\n", SDL_GetError());
		return;
	}

	Mix_AllocateChannels(NUM_CHANNELS);

	atterm(I_ShutdownSound);

	sound_initialized = true;

	SDL_PauseAudio(0);

	Printf(PRINT_HIGH, "I_InitSound: sound module ready\n");

	I_InitMusic();

        // Half of fix for stopping wrong sound, these need to be -1
        // to be regarded as empty (they'd be initialised to something weird)
        for(int i = 0; i < 256; i++)
            sounds_in_use[i] = -1;
}

void STACK_ARGS I_ShutdownSound (void)
{
	if (!sound_initialized)
		return;

	I_ShutdownMusic();

	Mix_CloseAudio();
	SDL_QuitSubSystem(SDL_INIT_AUDIO);
}


VERSION_CONTROL (i_sound_cpp, "$Id$")

