// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
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
//
// DESCRIPTION:
//	System interface, sound.
//	[Odamex] Fitted to work with SDL
//
//-----------------------------------------------------------------------------


#include "i_sdl.h" 
#include <SDL_mixer.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "z_zone.h"

#include "i_system.h"
#include "i_sound.h"
#include "i_music.h"
#include "m_argv.h"
#include "m_misc.h"
#include "w_wad.h"
#include "v_palette.h"

#include "doomdef.h"

#ifdef _XBOX
#include "i_xbox.h"
#endif

#define NUM_CHANNELS 16

static int mixer_freq;
static Uint16 mixer_format;
static int mixer_channels;

static bool sound_initialized = false;
static bool channel_in_use[NUM_CHANNELS];
static int nextchannel = 0;

EXTERN_CVAR (snd_sfxvolume)
EXTERN_CVAR (snd_musicvolume)
EXTERN_CVAR (snd_crossover)

void I_ClearSoundCache(void)
{
	// clear cached sfx:
	for (int id = 0; id < numsfx; id++)
	{
		sfxinfo_t *sfx = &S_sfx[id];
		if (!sfx->data) continue;

		// free Mix_Chunk memory:
		Mix_Chunk *chunk = (Mix_Chunk *) sfx->data;
		Z_Free(chunk->abuf);
		Z_Free(sfx->data);
		sfx->data = NULL;
	}
}

CVAR_FUNC_IMPL(snd_samplerate)
{
	S_Stop();
	S_Init(snd_sfxvolume, snd_musicvolume);
}

CVAR_FUNC_IMPL(snd_cubic)
{
	S_Stop();
	I_ClearSoundCache();
}

// [Russell] - Chocolate Doom's sound converter code, how awesome!
static bool ConvertibleRatio(int freq1, int freq2)
{
    int ratio;

    if (freq1 > freq2)
    {
        return ConvertibleRatio(freq2, freq1);
    }
    else if ((freq2 % freq1) != 0)
    {
        // Not in a direct ratio

        return false;
    }
    else
    {
        // Check the ratio is a power of 2

        ratio = freq2 / freq1;

        while ((ratio & 1) == 0)
        {
            ratio = ratio >> 1;
        }

        return ratio == 1;
    }
}

// Generic sound expansion function for any sample rate

static Mix_Chunk *ExpandSoundData(byte *data, int samplerate, int length)
{
	Mix_Chunk *chunk;
	Uint32 expanded_length;
	Uint32 expanded_samples;

	expanded_samples = (uint32_t) ((((uint64_t) length) * mixer_freq) / samplerate);

	// Double up twice: 8 -> 16 bit and mono -> stereo
	expanded_length = expanded_samples * 4;

	chunk = (Mix_Chunk *)Z_Malloc(sizeof(Mix_Chunk), PU_STATIC, NULL);
	chunk->allocated = 1;
	chunk->alen = expanded_length;
	chunk->abuf = (Uint8 *)Z_Malloc(expanded_length, PU_STATIC, &chunk->abuf);
	chunk->volume = MIX_MAX_VOLUME;

    SDL_AudioCVT convertor;
    if (samplerate <= mixer_freq
     && ConvertibleRatio(samplerate, mixer_freq)
     && SDL_BuildAudioCVT(&convertor,
                          AUDIO_U8, 1, samplerate,
                          mixer_format, mixer_channels, mixer_freq))
    {
        convertor.len = length;
        convertor.buf = new Uint8[convertor.len * convertor.len_mult];
        memcpy(convertor.buf, data, length);

        SDL_ConvertAudio(&convertor);

        memcpy(chunk->abuf, convertor.buf, chunk->alen);
        delete[] convertor.buf;
    }
    else
    {
        Sint16 *expanded = (Sint16 *) chunk->abuf;
        int expand_ratio;

        // Generic expansion if conversion does not work:
        //
        // SDL's audio conversion only works for rate conversions that are
        // powers of 2; if the two formats are not in a direct power of 2
        // ratio, do this naive conversion instead.

        // number of samples in the converted sound

        expand_ratio = (length << 8) / expanded_samples;

        for (size_t i = 0; i < expanded_samples; ++i)
        {
            Sint16 sample;
            int src;

            src = (i * expand_ratio) >> 8;

            sample = data[src] | (data[src] << 8);
            sample -= 32768;

            // expand 8->16 bits, mono->stereo

            expanded[i * 2] = expanded[i * 2 + 1] = sample;
        }
    }

    return chunk;
}

static Uint8 *perform_sdlmix_conv(Uint8 *data, Uint32 size, Uint32 *newsize)
{
    Mix_Chunk *chunk;
    SDL_RWops *mem_op;
    Uint8 *ret_data;

    // load, allocate and convert the format from memory
    mem_op = SDL_RWFromMem(data, size);

    if (!mem_op)
    {
        Printf(PRINT_HIGH,
                "perform_sdlmix_conv - SDL_RWFromMem: %s\n", SDL_GetError());

        return NULL;
    }

    chunk = Mix_LoadWAV_RW(mem_op, 1);

    if (!chunk)
    {
        Printf(PRINT_HIGH,
                "perform_sdlmix_conv - Mix_LoadWAV_RW: %s\n", Mix_GetError());

        return NULL;
    }

    // return the size
    *newsize = chunk->alen;

    // allocate some space in the zone heap
    ret_data = (Uint8 *)Z_Malloc(chunk->alen, PU_STATIC, NULL);

    // copy the converted data to the return buffer
    memcpy(ret_data, chunk->abuf, chunk->alen);

    // clean up
    Mix_FreeChunk(chunk);
    chunk = NULL;

    return ret_data;
}

static double butterworth_q(int order, int phase) {
	return -0.5 / cos(M_PI * (phase + order + 0.5) / order);
}

static Mix_Chunk *CubicResample(byte *data, int samplerate, int length)
{
	//static double highest = 0.0;

	double inputFrequency = samplerate;
	double outputFrequency = mixer_freq;

	double ratio = inputFrequency / outputFrequency;
	double mu = 0.0;
	double s[4] = {0, 0, 0, 0};

	const Uint32 extra_samples = 16;

	Mix_Chunk *chunk;
	Uint32 expanded_length = (Uint32)((((uint64_t) length) * mixer_freq) / samplerate + extra_samples);

	// Double up twice: 8 -> 16 bit and mono -> stereo
	expanded_length *= 4;

	chunk = (Mix_Chunk *)Z_Malloc(sizeof(Mix_Chunk), PU_STATIC, NULL);
	chunk->allocated = 1;
	chunk->alen = expanded_length;
	chunk->abuf = (Uint8 *)Z_Malloc(expanded_length, PU_STATIC, &chunk->abuf);
	chunk->volume = MIX_MAX_VOLUME;

	Sint16 *abuf = (Sint16 *)chunk->abuf;

	// setup lowpass filters:
	const int passes = 3;
	double a0[passes], a1[passes], a2[passes], b1[passes], b2[passes];
	double z1[passes], z2[passes];

	double cutoff = inputFrequency / 2.0 - 2000.0;
	for (int pass = 0; pass < passes; pass++) {
		double q = butterworth_q(passes * 2, pass);
		double k = tan(M_PI * cutoff / inputFrequency);

		double n = 1.0 / (1.0 + k / q + k * k);
		a0[pass] = k * k * n;
		a1[pass] = 2.0 * a0[pass];
		a2[pass] = a0[pass];
		b1[pass] = 2.0 * (k * k - 1.0) * n;
		b2[pass] = (1.0 - k / q + k * k) * n;

		z1[pass] = 0.0;
		z2[pass] = 0.0;
	}

	// for all samples in sfx: lowpass, resample, and convert to 16-bit:
	for (Uint32 i = 0; i < length + extra_samples; i++) {
		double sample;
		if (i < length) {
			sample = (data[i] - 128) / 128.0;
		} else {
			sample = 0;
		}

		// cubic resampler:
		s[0] = s[1];
		s[1] = s[2];
		s[2] = s[3];
		s[3] = sample;

		while (mu <= 1.0 && ((Uint8*)abuf - chunk->abuf < chunk->alen)) {
			double a = s[3] - s[2] - s[0] + s[1];
			double b = s[0] - s[1] - a;
			double c = s[2] - s[0];
			double d = s[1];

			// cubic resample:
			double n = a * mu * mu * mu + b * mu * mu + c * mu + d;

			// run nyquist lowpass filters:
			for (int p = 0; p < passes; p++) {
				double out = n * a0[p] + z1[p];
				z1[p] = n * a1[p] + z2[p] - b1[p] * out;
				z2[p] = n * a2[p] - b2[p] * out;
				n = out;
			}

			// [jsd]: when using a 32767.0 multiplier to convert to 16-bit, the max sample will be 46740 which is out
			// of range, so we use 22970.0 multiplier instead to come in just shy of 32753 as our max 16-bit sample.

			// convert to signed 16-bit sample:
			//highest = MAX(highest, abs(n * 22970.0));
			Sint16 newsample = (Sint16)(n * 22970.0);

			// expand mono to stereo:
			abuf[0] = abuf[1] = newsample;
			abuf += 2;

			mu += ratio;
		}

		mu -= 1.0;
	}

	//Printf(PRINT_LOW, "max 16-bit sample: %d\n", Uint32(highest));
	return chunk;
}

static void getsfx (struct sfxinfo_struct *sfx)
{
    Uint32 samplerate;
	Uint32 length, expanded_length;
	Uint8 *data;
	Uint32 new_size = 0;
	Mix_Chunk *chunk;

    data = (Uint8 *)W_CacheLumpNum(sfx->lumpnum, PU_STATIC);
    // [Russell] - ICKY QUICKY HACKY SPACKY *I HATE THIS SOUND MANAGEMENT SYSTEM!*
    // get the lump size, shouldn't this be filled in elsewhere?
    sfx->length = W_LumpLength(sfx->lumpnum);

    // [Russell] is it not a doom sound lump?
    if (((data[1] << 8) | data[0]) != 3)
    {
        chunk = (Mix_Chunk *)Z_Malloc(sizeof(Mix_Chunk), PU_STATIC, NULL);
        chunk->allocated = 1;
        if (sfx->length < 8) // too short to be anything of interest
        {
            // Let's hope SDL_Mixer checks alen before dereferencing abuf!
            chunk->abuf = NULL;
            chunk->alen = 0;
        }
        else
        {
            chunk->abuf = perform_sdlmix_conv(data, sfx->length, &new_size);
            chunk->alen = new_size;
        }
        chunk->volume = MIX_MAX_VOLUME;

        sfx->data = chunk;

        Z_ChangeTag(data, PU_CACHE);

        return;
    }

	samplerate = (data[3] << 8) | data[2];
    length = (data[5] << 8) | data[4];

    // [Russell] - Ignore doom's sound format length info
    // if the lump is longer than the value, fixes exec.wad's ssg
    length = (sfx->length - 8 > length) ? sfx->length - 8 : length;

	if (snd_cubic) {
		DPrintf("getsfx('%s'): cubic resample\n", sfx->name);
		chunk = CubicResample((unsigned char *) data + 8, samplerate, length);
	} else {
		DPrintf("getsfx('%s'): naive resample\n", sfx->name);
		chunk = ExpandSoundData((unsigned char *) data + 8, samplerate, length);
	}

    sfx->data = chunk;
    
    Z_ChangeTag(data, PU_CACHE);
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


//
// I_StartSound
//
// Starting a sound means adding it to the current list of active sounds
// in the internal channels. As the SFX info struct contains  e.g. a pointer
// to the raw data, it is ignored. As our sound handling does not handle
// priority, it is ignored. Pitching (that is, increased speed of playback)
// is set, but currently not used by mixing.
//
int I_StartSound(int id, float vol, int sep, int pitch, bool loop)
{
	if (!sound_initialized)
		return -1;

	Mix_Chunk *chunk = (Mix_Chunk *)S_sfx[id].data;
	
	// find a free channel, starting from the first after
	// the last channel we used
	int channel = nextchannel;

	do
	{
		channel = (channel + 1) % NUM_CHANNELS;

		if (channel == nextchannel)
		{
			fprintf(stderr, "No free sound channels left.\n");
			return -1;
		}
	} while (channel_in_use[channel]);

	nextchannel = channel;

	// play sound
	Mix_PlayChannelTimed(channel, chunk, loop ? -1 : 0, -1);

	channel_in_use[channel] = true;

	// set seperation, etc.
	I_UpdateSoundParams(channel, vol, sep, pitch);

	return channel;
}


void I_StopSound (int handle)
{
	if(!sound_initialized)
		return;

	channel_in_use[handle] = false;

	Mix_HaltChannel(handle);
}



int I_SoundIsPlaying (int handle)
{
	if(!sound_initialized)
		return 0;

	return Mix_Playing(handle);
}


void I_UpdateSoundParams (int handle, float vol, int sep, int pitch)
{
	if(!sound_initialized)
		return;

	if(sep > 255)
		sep = 255;

	if(!snd_crossover)
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
	if (!sound_initialized)
		return;
	
	if (!sfx->data)
	{
		DPrintf ("loading sound \"%s\" (%d)\n", sfx->name, sfx->lumpnum);
		getsfx (sfx);
	}
}

void I_InitSound()
{
	if (I_IsHeadless() || Args.CheckParm("-nosound"))
		return;
		
    #if defined(SDL12)
    const char *driver = getenv("SDL_AUDIODRIVER");

	if(!driver)
		driver = "default";
		
    Printf(PRINT_HIGH, "I_InitSound: Initializing SDL's sound subsystem (%s)\n", driver);
    #elif defined(SDL20)
    Printf(PRINT_HIGH, "I_InitSound: Initializing SDL's sound subsystem\n");
    #endif

	if (SDL_InitSubSystem(SDL_INIT_AUDIO) < 0)
	{
		Printf(PRINT_HIGH, 
               "I_InitSound: Unable to set up sound: %s\n", 
               SDL_GetError());
               
		return;
	}

    #if defined(SDL20)
	Printf(PRINT_HIGH, "I_InitSound: Using SDL's audio driver (%s)\n", SDL_GetCurrentAudioDriver());
	#endif
	
	const SDL_version *ver = Mix_Linked_Version();

	if(ver->major != MIX_MAJOR_VERSION
		|| ver->minor != MIX_MINOR_VERSION)
	{
		Printf(PRINT_HIGH, "I_InitSound: SDL_mixer version conflict (%d.%d.%d vs %d.%d.%d dll)\n",
			MIX_MAJOR_VERSION, MIX_MINOR_VERSION, MIX_PATCHLEVEL,
			ver->major, ver->minor, ver->patch);
		return;
	}

	if(ver->patch != MIX_PATCHLEVEL)
	{
		Printf_Bold("I_InitSound: SDL_mixer version warning (%d.%d.%d vs %d.%d.%d dll)\n",
			MIX_MAJOR_VERSION, MIX_MINOR_VERSION, MIX_PATCHLEVEL,
			ver->major, ver->minor, ver->patch);
	}

	Printf(PRINT_HIGH, "I_InitSound: Initializing SDL_mixer\n");

    if (Mix_OpenAudio((int)snd_samplerate, AUDIO_S16SYS, 2, 1024) < 0)
	{
		Printf(PRINT_HIGH, 
               "I_InitSound: Error initializing SDL_mixer: %s\n", 
               Mix_GetError());
		return;
	}

    if(!Mix_QuerySpec(&mixer_freq, &mixer_format, &mixer_channels))
	{
		Printf(PRINT_HIGH, 
               "I_InitSound: Error initializing SDL_mixer: %s\n", 
               Mix_GetError());
		return;
	}
	
	Printf(PRINT_HIGH, 
           "I_InitSound: Using %d channels (freq:%d, fmt:%d, chan:%d)\n",
           Mix_AllocateChannels(NUM_CHANNELS),
		   mixer_freq, mixer_format, mixer_channels);

	atterm(I_ShutdownSound);

	sound_initialized = true;

	SDL_PauseAudio(0);

	Printf(PRINT_HIGH, "I_InitSound: sound module ready\n");

	I_InitMusic();

	// Half of fix for stopping wrong sound, these need to be false
	// to be regarded as empty (they'd be initialised to something weird)
	for (int i = 0; i < NUM_CHANNELS; i++)
		channel_in_use[i] = false;
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

