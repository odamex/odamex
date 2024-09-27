// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
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
//	The not so system specific sound interface.
//
//-----------------------------------------------------------------------------

#pragma once

#include "m_fixed.h"

#define MAX_SNDNAME 63

class AActor;
class player_s;
typedef player_s player_t;

//
// SoundFX struct.
//
typedef struct sfxinfo_struct sfxinfo_t;

struct sfxinfo_struct
{
	char name[MAX_SNDNAME + 1]; // [RH] Sound name defined in SNDINFO
	unsigned normal;            // Normal sample handle
	unsigned looping;           // Looping sample handle
	void* data;

	int link;
	enum { NO_LINK = 0xffffffff };

	int lumpnum;              // lump number of sfx
	unsigned int ms;          // [RH] length of sfx in milliseconds
	unsigned int next, index; // [RH] For hashing
	unsigned int frequency;   // [RH] Preferred playback rate
	unsigned int length;      // [RH] Length of the sound in bytes
	bool israndom;            // [DE] Whether or not this is an alias for a set of random sounds
};

// the complete set of sound effects
extern std::vector<sfxinfo_t> S_sfx;

// map of every sound id for sounds that have randomized variants
extern std::map<int, std::vector<int> > S_rnd;

// Initializes sound stuff, including volume
// Sets channels, SFX and music volume,
//	allocates channel buffer, sets S_sfx lookup.
//
void S_Init(float sfxVolume, float musicVolume);
void S_Deinit();

// Per level startup code.
// Kills playing sounds at start of level,
//	determines music if any, changes music.
//
void S_Stop();
void S_Start();

// Start sound for thing at <ent>
void S_Sound(int channel, const char* name, float volume, int attenuation);
void S_Sound(AActor* ent, int channel, const char* name, float volume, int attenuation);
void S_Sound(fixed_t* pt, int channel, const char* name, float volume, int attenuation);
void S_Sound(fixed_t x, fixed_t y, int channel, const char* name, float volume,
             int attenuation);
void S_PlatSound(fixed_t* pt, int channel, const char* name, float volume,
                 int attenuation); // [Russell] - Hack to stop multiple plat stop sounds
void S_LoopedSound(AActor* ent, int channel, const char* name, float volume,
                   int attenuation);
void S_LoopedSound(fixed_t* pt, int channel, const char* name, float volume,
                   int attenuation);
void S_SoundID(int channel, int sfxid, float volume, int attenuation);
void S_SoundID(fixed_t x, fixed_t y, int channel, int sound_id, float volume,
               int attenuation);
void S_SoundID(AActor* ent, int channel, int sfxid, float volume, int attenuation);
void S_SoundID(fixed_t* pt, int channel, int sfxid, float volume, int attenuation);
void S_LoopedSoundID(AActor* ent, int channel, int sfxid, float volume, int attenuation);
void S_LoopedSoundID(fixed_t* pt, int channel, int sfxid, float volume, int attenuation);

// sound channels
// channel 0 never willingly overrides
// other channels (1-8) always override a playing sound on that channel
#define CHAN_AUTO 0
#define CHAN_WEAPON 1
#define CHAN_VOICE 2
#define CHAN_ITEM 3
#define CHAN_BODY 4
#define CHAN_ANNOUNCER 5
#define CHAN_GAMEINFO 6
#define CHAN_INTERFACE 7

// modifier flags
//#define CHAN_NO_PHS_ADD		8	// send to all clients, not just ones in PHS (ATTN 0 will
//also do this) #define CHAN_RELIABLE			16	// send by reliable message, not
//datagram

// sound attenuation values
#define ATTN_NONE 0 // full volume the entire level
#define ATTN_NORM 1
#define ATTN_IDLE 2
#define ATTN_STATIC 3 // diminish very rapidly with distance

// Stops a sound emanating from one of an entity's channels
void S_StopSound(AActor* ent, int channel);
void S_StopSound(fixed_t* pt, int channel);
void S_StopSound(fixed_t* pt);

// Stop sound for all channels
void S_StopAllChannels();

// Is the sound playing on one of the entity's channels?
bool S_GetSoundPlayingInfo(AActor* ent, int sound_id);
bool S_GetSoundPlayingInfo(fixed_t* pt, int sound_id);

// Moves all sounds from one mobj to another
void S_RelinkSound(AActor* from, AActor* to);

// Start music using <music_name>
void S_StartMusic(const char* music_name);

// Start music using <music_name>, and set whether looping
void S_ChangeMusic(std::string music_name, int looping);

// Stops the music fer sure.
void S_StopMusic();

// Stop and resume music, during game PAUSE.
void S_PauseSound();
void S_ResumeSound();

//
// Updates music & sounds
//
void S_UpdateSounds(void* listener);
void S_UpdateMusic();

void S_SetMusicVolume(float volume);
void S_SetSfxVolume(float volume);

// [RH] Activates an ambient sound. Called when the thing is added to the map.
//		(0-biased)
void S_ActivateAmbient(AActor* mobj, int ambient);

// [RH] S_sfx "maintenance" routines
void S_ParseSndInfo();

void S_HashSounds();
int S_FindSound(const char* logicalname);
int S_FindSoundByLump(int lump);
int S_AddSound(const char* logicalname, const char* lumpname); // Add sound by lumpname
int S_AddSoundLump(char* logicalname, int lump);         // Add sound by lump index
void S_AddRandomSound(int owner, std::vector<int>& list);
void S_ClearSoundLumps();

void UV_SoundAvoidPlayer(AActor* mo, byte channel, const char* name, byte attenuation);

// [RH] Prints sound debug info to the screen.
//		Modelled after Hexen's noise cheat.
void S_NoiseDebug();

// The following functions work seamlessly on local clients and networked games.

#if SERVER_APP
#include "sv_main.h"
#endif

inline static void S_NetSound(AActor* mo, byte channel, const char* name, const byte attenuation)
{
#if SERVER_APP
	SV_Sound(mo, channel, name, attenuation);
#else
	S_Sound(mo, channel, name, 1, attenuation);
#endif
}

inline static void S_PlayerSound(player_t* pl, AActor* mo, const byte channel, const char* name,
                          const byte attenuation)
{
#if SERVER_APP
	SV_Sound(*pl, mo, channel, name, attenuation);
#else
	S_Sound(mo, channel, name, 1, attenuation);
#endif
}
