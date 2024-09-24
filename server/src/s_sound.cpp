// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2000-2006 by Sergey Makovkin (CSDoom .62).
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
void S_PauseSound()
{
}

void S_ResumeSound()
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
void S_ChangeMusic(std::string musicname, int looping)
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

std::vector<sfxinfo_t> S_sfx; // [RH] This is no longer defined in sounds.c
std::map<int, std::vector<int> > S_rnd;

static struct AmbientSound {
	unsigned	type;		// type of ambient sound
	int			periodmin;	// # of tics between repeats
	int			periodmax;	// max # of tics for random ambients
	float		volume;		// relative volume of sound
	float		attenuation;
	char		sound[MAX_SNDNAME+1]; // Logical name of sound to play
} Ambients[256];

#define RANDOM		1
#define PERIODIC	2
#define CONTINUOUS	3
#define POSITIONAL	4
#define SURROUND	16

void S_HashSounds()
{
	// Mark all buckets as empty
	for (unsigned i = 0; i < S_sfx.size(); i++)
		S_sfx[i].index = ~0;

	// Now set up the chains
	for (unsigned i = 0; i < S_sfx.size(); i++)
	{
		const unsigned j = MakeKey(S_sfx[i].name) % static_cast<unsigned>(S_sfx.size() - 1);
		S_sfx[i].next = S_sfx[j].index;
		S_sfx[j].index = i;
	}
}

int S_FindSound(const char *logicalname)
{
	if (S_sfx.empty())
		return -1;

	int i = S_sfx[MakeKey(logicalname) % static_cast<unsigned>(S_sfx.size() - 1)].index;

	while ((i != -1) && strnicmp(S_sfx[i].name, logicalname, MAX_SNDNAME))
		i = S_sfx[i].next;

	return i;
}

int S_FindSoundByLump(int lump)
{
	if (lump != -1) 
	{
		for (unsigned i = 0; i < S_sfx.size(); i++)
			if (S_sfx[i].lumpnum == lump)
				return i;
	}
	return -1;
}

int S_AddSoundLump(const char *logicalname, int lump)
{
	S_sfx.push_back(sfxinfo_t());
	sfxinfo_t& new_sfx = S_sfx[S_sfx.size() - 1];

	// logicalname MUST be < MAX_SNDNAME chars long
	strcpy(new_sfx.name, logicalname);
	new_sfx.data = NULL;
	new_sfx.link = sfxinfo_t::NO_LINK;
	new_sfx.lumpnum = lump;
	return S_sfx.size() - 1;
}

void S_ClearSoundLumps()
{
	S_sfx.clear();
	S_rnd.clear();
}

int FindSoundNoHash(const char* logicalname)
{
	for (size_t i = 0; i < S_sfx.size(); i++)
		if (iequals(logicalname, S_sfx[i].name))
			return i;

	return S_sfx.size();
}

int FindSoundTentative(const char* name)
{
	int id = FindSoundNoHash(name);
	if (id == static_cast<int>(S_sfx.size()))
	{
		id = S_AddSoundLump(name, -1);
	}
	return id;
}

int S_AddSound(const char *logicalname, const char *lumpname)
{
	int sfxid = FindSoundNoHash(logicalname);

	const int lump = lumpname ? W_CheckNumForName(lumpname) : -1;

	// Otherwise, prepare a new one.
	if (sfxid != static_cast<int>(S_sfx.size()))
	{
		sfxinfo_t& sfx = S_sfx[sfxid];

		sfx.lumpnum = lump;
		sfx.link = sfxinfo_t::NO_LINK;
		if (sfx.israndom)
		{
			S_rnd.erase(sfxid);
			sfx.israndom = false;
		}
	}
	else
		sfxid = S_AddSoundLump(logicalname, lump);

	return sfxid;
}

void S_AddRandomSound(int owner, std::vector<int>& list)
{
	S_rnd[owner] = list;
	S_sfx[owner].link = owner;
	S_sfx[owner].israndom = true;
}

// S_ParseSndInfo
// Parses all loaded SNDINFO lumps.
void S_ParseSndInfo()
{
	S_ClearSoundLumps();

	int lump = -1;
	while ((lump = W_FindLump("SNDINFO", lump)) != -1)
	{
		char* buffer = static_cast<char*>(W_CacheLumpNum(lump, PU_CACHE));

		const OScannerConfig config = {
		    "SNDINFO", // lumpName
		    true,      // semiComments
		    true,      // cComments
		};
		OScanner os = OScanner::openBuffer(config, buffer, buffer + W_LumpLength(lump));

		while (os.scan())
		{
			std::string tok = os.getToken();

			// check if token is a command
			if (tok[0] == '$')
			{
				os.mustScan();
				if (os.compareTokenNoCase("ambient"))
				{
					// $ambient <num> <logical name> [point [atten]|surround] <type>
					// [secs] <relative volume>
					AmbientSound *ambient, dummy;

					os.mustScanInt();
					const int index = os.getTokenInt();
					if (index < 0 || index > 255)
					{
						os.warning("Bad ambient index (%d)\n", index);
						ambient = &dummy;
					}
					else
					{
						ambient = Ambients + index;
					}

					ambient->type = 0;
					ambient->periodmin = 0;
					ambient->periodmax = 0;
					ambient->volume = 0.0f;

					os.mustScan();
					strncpy(ambient->sound, os.getToken().c_str(), MAX_SNDNAME);
					ambient->sound[MAX_SNDNAME] = 0;
					ambient->attenuation = 0.0f;

					os.mustScan();
					if (os.compareTokenNoCase("point"))
					{
						ambient->type = POSITIONAL;
						os.mustScan();

						if (IsRealNum(os.getToken().c_str()))
						{
							ambient->attenuation =
							    (os.getTokenFloat() > 0) ? os.getTokenFloat() : 1;
							os.mustScan();
						}
						else
						{
							ambient->attenuation = 1;
						}
					}
					else if (os.compareTokenNoCase("surround"))
					{
						ambient->type = SURROUND;
						os.mustScan();
						ambient->attenuation = -1;
					}
					// else if (os.compareTokenNoCase("world"))
					//{
					// todo
					//}

					if (os.compareTokenNoCase("continuous"))
					{
						ambient->type |= CONTINUOUS;
					}
					else if (os.compareTokenNoCase("random"))
					{
						ambient->type |= RANDOM;
						os.mustScanFloat();
						ambient->periodmin =
						    static_cast<int>(os.getTokenFloat() * TICRATE);
						os.mustScanFloat();
						ambient->periodmax =
						    static_cast<int>(os.getTokenFloat() * TICRATE);
					}
					else if (os.compareTokenNoCase("periodic"))
					{
						ambient->type |= PERIODIC;
						os.mustScanFloat();
						ambient->periodmin =
						    static_cast<int>(os.getTokenFloat() * TICRATE);
					}
					else
					{
						os.warning("Unknown ambient type (%s)\n", os.getToken().c_str());
					}

					os.mustScanFloat();
					ambient->volume = clamp(os.getTokenFloat(), 0.0f, 1.0f);
				}
				else if (os.compareTokenNoCase("map"))
				{
					// Hexen-style $MAP command
					char mapname[8];

					os.mustScanInt();
					snprintf(mapname, 8, "MAP%02d", os.getTokenInt());
					level_pwad_info_t& info = getLevelInfos().findByName(mapname);
					os.mustScan();
					if (info.mapname[0])
					{
						info.music = os.getToken();
					}
				}
				else if (os.compareTokenNoCase("alias"))
				{
					os.mustScan();
					const int sfxfrom = S_AddSound(os.getToken().c_str(), NULL);
					os.mustScan();
					S_sfx[sfxfrom].link = FindSoundTentative(os.getToken().c_str());
				}
				else if (os.compareTokenNoCase("random"))
				{
					std::vector<int> list;

					os.mustScan();
					const int owner = S_AddSound(os.getToken().c_str(), NULL);

					os.mustScan();
					os.assertTokenIs("{");
					while (os.scan() && !os.compareToken("}"))
					{
						const int sfxto = FindSoundTentative(os.getToken().c_str());

						if (owner == sfxto)
						{
							os.warning("Definition of random sound '%s' refers to itself "
							           "recursively.\n",
							           os.getToken().c_str());
							continue;
						}

						list.push_back(sfxto);
					}
					if (list.size() == 1)
					{
						// only one sound; treat as alias
						S_sfx[owner].link = list[0];
					}
					else if (list.size() > 1)
					{
						S_AddRandomSound(owner, list);
					}
				}
				else
				{
					os.warning("Unknown SNDINFO command %s\n", os.getToken().c_str());
					while (os.scan())
						if (os.crossed())
						{
							os.unScan();
							break;
						}
				}
			}
			else
			{
				// token is a logical sound mapping
				char name[MAX_SNDNAME + 1];

				strncpy(name, tok.c_str(), MAX_SNDNAME);
				name[MAX_SNDNAME] = 0;
				os.mustScan();
				S_AddSound(name, os.getToken().c_str());
			}
		}
	}
	S_HashSounds();
}

void A_Ambient(AActor *actor)
{
}

void S_ActivateAmbient(AActor *origin, int ambient)
{
}

VERSION_CONTROL (s_sound_cpp, "$Id$")
