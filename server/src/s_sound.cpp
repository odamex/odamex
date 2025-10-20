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

typedef enum
{
	AMB_TYPE_NONE,
    AMB_TYPE_POINT,
    AMB_TYPE_WORLD,
} amb_type_t;

typedef enum
{
	AMB_MODE_NONE,
    AMB_MODE_CONTINUOUS,
    AMB_MODE_RANDOM,
    AMB_MODE_PERIODIC,
} amb_mode_t;

static struct AmbientSound {
	amb_type_t	type;		// Ambient sound type
	amb_mode_t	mode;		// Ambient sound mode
	int			periodmin;	// # of tics between repeats
	int			periodmax;	// max # of tics for random ambients
	float		volume;		// relative volume of sound
	float		attenuation; // Used for distance scaling
	char		sound[MAX_SNDNAME+1]; // Logical name of sound to play
} Ambients[256];

void S_HashSounds()
{
	// Mark all buckets as empty
	for (auto& sfx : S_sfx)
		sfx.index = ~0;

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
	S_sfx.emplace_back();
	sfxinfo_t& new_sfx = S_sfx.back();

	// logicalname MUST be <= MAX_SNDNAME chars long
	M_StringCopy(new_sfx.name, logicalname, MAX_SNDNAME + 1);
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
						os.warning("Bad ambient index ({})\n", index);
						ambient = &dummy;
					}
					else
					{
						ambient = Ambients + index;
					}

					ambient->type = AMB_TYPE_NONE;
					ambient->mode = AMB_MODE_NONE;
					ambient->periodmin = 0;
					ambient->periodmax = 0;
					ambient->volume = 0.0f;

					os.mustScan();
					strncpy(ambient->sound, os.getToken().c_str(), MAX_SNDNAME);
					ambient->sound[MAX_SNDNAME] = 0;
					ambient->attenuation = 0.0f; // No change by default

					os.mustScan();
					if (os.compareTokenNoCase("point"))
					{
						ambient->type = AMB_TYPE_POINT;
						os.mustScan();

						if (IsRealNum(os.getToken().c_str()))
						{
							if (os.getTokenFloat() > 0.0f)
							{
								ambient->attenuation = os.getTokenFloat();
							}

							os.mustScan();
						}
					}
					else
					{
						ambient->type = AMB_TYPE_WORLD;

						if (os.compareTokenNoCase("surround") ||
						    os.compareTokenNoCase("world"))
						{
							os.mustScan();
						}
					}

					if (os.compareTokenNoCase("continuous"))
					{
						ambient->mode = AMB_MODE_CONTINUOUS;
					}
					else if (os.compareTokenNoCase("random"))
					{
						ambient->mode = AMB_MODE_RANDOM;
						os.mustScanFloat();
						ambient->periodmin =
						    static_cast<int>(os.getTokenFloat() * TICRATE);
						os.mustScanFloat();
						ambient->periodmax =
						    static_cast<int>(os.getTokenFloat() * TICRATE);
					}
					else if (os.compareTokenNoCase("periodic"))
					{
						ambient->mode = AMB_MODE_PERIODIC;
						os.mustScanFloat();
						ambient->periodmin =
						    static_cast<int>(os.getTokenFloat() * TICRATE);
					}
					else
					{
						os.warning("Unknown ambient type ({})\n", os.getToken());
					}

					ambient->periodmin = MAX(0, ambient->periodmin);
					ambient->periodmax = MAX(ambient->periodmin, ambient->periodmax);

					os.mustScanFloat();
					ambient->volume = clamp(os.getTokenFloat(), 0.0f, 1.0f);

					if (ambient->mode == AMB_MODE_NONE || ambient->volume == 0.0f ||
					    (ambient->mode != AMB_MODE_CONTINUOUS &&
					     ambient->periodmin == 0 && ambient->periodmax == 0))
					{
						// Ignore bad ambient sounds
						ambient->type = AMB_TYPE_NONE;
					}
				}
				else if (os.compareTokenNoCase("map"))
				{
					// Hexen-style $MAP command
					os.mustScanInt();
					OLumpName mapname = fmt::format("MAP{:02d}", os.getTokenInt());
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
							os.warning("Definition of random sound '{}' refers to itself "
							           "recursively.\n",
							           os.getToken());
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
					os.warning("Unknown SNDINFO command {}\n", os.getToken());
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
