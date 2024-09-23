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


#include "odamex.h"

#include <algorithm>

#include "cl_main.h"

#include "m_alloc.h"
#include "i_sound.h"
#include "i_music.h"
#include "s_sound.h"
#include "s_sndseq.h"
#include "c_dispatch.h"
#include "z_zone.h"
#include "m_random.h"
#include "w_wad.h"
#include "p_local.h"
#include "cmdlib.h"
#include "i_video.h"
#include "v_video.h"
#include "v_text.h"
#include "m_vectors.h"
#include "m_fileio.h"
#include "gi.h"
#include "oscanner.h"

#define NORM_PITCH		128
#define NORM_PRIORITY	64
#define NORM_SEP		128

static const fixed_t S_STEREO_SWING = 96 * FRACUNIT;

struct channel_t
{
public:
	fixed_t*	pt;				// origin of sound
	fixed_t		x, y;			// origin if pt is NULL
	sfxinfo_t*	sfxinfo;		// sound information (if null, channel avail.)
	int 		handle;			// handle of the sound being played
	int			sound_id;
	int			entchannel;		// entity's sound channel
	float		attenuation;
	float		volume;
	int			priority;
	bool		loop;
	int			start_time;		// gametic the sound started in

	void clear()
	{
		pt = NULL;
		x = y = 0;
		sfxinfo = NULL;
		handle = -1;
		sound_id = -1;
		entchannel = CHAN_VOICE;
		attenuation = 0.0f;
		volume = 0.0f;
		priority = MININT;
		loop = false;
		start_time = 0;
	}
};

int sfx_empty;

// joek - hack for silent bfg
int sfx_noway, sfx_oof;

// [RH] Print sound debugging info?
cvar_t noisedebug ("noise", "0", "", CVARTYPE_BOOL, 0);

// the set of channels available
static channel_t *Channel;

// For ZDoom sound curve
static byte		*SoundCurve;

// Maximum volume of a sound effect.
CVAR_FUNC_IMPL(snd_sfxvolume)
{
	S_SetSfxVolume(var);
}

// Maximum volume of Music.
CVAR_FUNC_IMPL(snd_musicvolume)
{
	S_SetMusicVolume(var);
}

// Maximum volume of announcer sounds.
EXTERN_CVAR(snd_announcervolume)

CVAR_FUNC_IMPL (snd_channels)
{
	S_Stop();
	S_Init (snd_sfxvolume, snd_musicvolume);
}


// whether songs are mus_paused
static bool mus_paused;

// music currently being played
static struct mus_playing_t
{
	std::string name;
	int   handle;
} mus_playing;

EXTERN_CVAR (co_globalsound)
EXTERN_CVAR (co_zdoomsound)
EXTERN_CVAR (snd_musicsystem)

size_t numChannels;

//
// [RH] Print sound debug info. Called from D_Display()
//
void S_NoiseDebug()
{
	fixed_t ox, oy;

	int y = 32 * CleanYfac;
	if (gametic & 16)
		screen->DrawText (CR_TAN, 0, y, "*** SOUND DEBUG INFO ***");
	y += 8;

	screen->DrawText (CR_GREY, 0, y, "name");
	screen->DrawText (CR_GREY, 70, y, "x");
	screen->DrawText (CR_GREY, 120, y, "y");
	screen->DrawText (CR_GREY, 170, y, "vol");
	screen->DrawText (CR_GREY, 200, y, "pri");
	screen->DrawText (CR_GREY, 240, y, "dist");
	screen->DrawText (CR_GREY, 280, y, "chan");
	y += 8;

	for (unsigned int i = 0; ((i < numChannels) && (y < I_GetVideoHeight() - 16)); i++, y += 8)
	{
		if (Channel[i].sfxinfo)
		{
			char temp[16];
			fixed_t *origin = Channel[i].pt;

			if (Channel[i].attenuation <= 0 && listenplayer().camera)
			{
				ox = listenplayer().camera->x;
				oy = listenplayer().camera->y;
			}
			else if (origin)
			{
				ox = origin[0];
				oy = origin[1];
			}
			else
			{
				ox = Channel[i].x;
				oy = Channel[i].y;
			}
			const int color = Channel[i].loop ? CR_BROWN : CR_GREY;
			strcpy (temp, lumpinfo[Channel[i].sfxinfo->lumpnum].name);
			temp[8] = 0;
			screen->DrawText (color, 0, y, temp);
			snprintf (temp, 16, "%d", ox / FRACUNIT);
			screen->DrawText (color, 70, y, temp);
			snprintf(temp, 16, "%d", oy / FRACUNIT);
			screen->DrawText (color, 120, y, temp);
			snprintf(temp, 16, "%.2f", Channel[i].volume);
			screen->DrawText (color, 170, y, temp);
			snprintf(temp, 16, "%d", Channel[i].priority);
			screen->DrawText (color, 200, y, temp);
			snprintf(temp, 16, "%d", P_AproxDistance2 (listenplayer().camera, ox, oy) / FRACUNIT);
			screen->DrawText (color, 240, y, temp);
			snprintf(temp, 16, "%d", Channel[i].entchannel);
			screen->DrawText (color, 280, y, temp);
		}
		else
		{
			screen->DrawText (CR_GREY, 0, y, "------");
		}
	}
}

//
// S_UseMap8Volume
//
// Determines if it is appropriate to use the special ExM8 attentuation
// based on the current map number
// [ML] Now based on whether co_zdoomsound is on or it's a multiplayer not-coop game
//
static bool S_UseMap8Volume()
{
	if (co_zdoomsound || (multiplayer && sv_gametype != GM_COOP))
		return false;

    if (gameinfo.flags & GI_MAPxx)
	{
		// Doom2 style map naming (MAPxy)
		if (level.mapname[3] == '0' && level.mapname[4] == '8')
			return true;
	}
	else
	{
		// Doom1 style map naming (ExMy)
		if (level.mapname[3] == '8')
			return true;
	}

	return false;
}

//
// Internals.
//
static void S_StopChannel(unsigned int cnum);


//
// Initializes sound stuff, including volume
// Sets channels, SFX and music volume,
// allocates channel buffer, sets S_sfx lookup.
//
void S_Init(float sfxVolume, float musicVolume)
{
	SoundCurve = (byte *)W_CacheLumpNum(W_GetNumForName("SNDCURVE"), PU_STATIC);

	// [RH] Read in sound sequences
	NumSequences = 0;
	S_ParseSndSeq();

	S_SetSfxVolume(sfxVolume);
	S_SetMusicVolume(musicVolume);

	// Allocating the internal channels for mixing
	// (the maximum numer of sounds rendered
	// simultaneously) within zone memory.
	numChannels = snd_channels.asInt();
	Channel = (channel_t*)Z_Malloc(numChannels * sizeof(channel_t), PU_STATIC, 0);
	for (size_t i = 0; i < numChannels; i++)
		Channel[i].clear();

	I_SetChannels(numChannels);

	// no sounds are playing, and they are not mus_paused
	mus_paused = false;
}

/**
 * @brief Run cleanup assuming zone memory has been freed.
 */
void S_Deinit()
{
	::SoundCurve = NULL;
	::Channel = NULL;
}

//
// Kills playing sounds
//
void S_Stop()
{
	// kill all playing sounds at start of level
	//	(trust me - a good idea)
	for (unsigned i = 0; i < numChannels; i++)
		S_StopChannel(i);

	S_StopMusic();
}


//
// Per level startup code.
// Kills playing sounds at start of level,
// determines music if any, changes music.
//
void S_Start()
{
	// Kill all sound channels - but don't stop music.
	for (unsigned i = 0; i < numChannels; i++)
		S_StopChannel(i);

	// start new music for the level
	mus_paused = false;

	// [RH] This is a lot simpler now.
	S_ChangeMusic (std::string(level.music.c_str(), 8), true);
}


//
// S_CompareChannels
//
// A comparison function that determines which sound channel should
// take priority. Can be used with std::sort.
//
// Returns true if the first channel should precede the second.
//
bool S_CompareChannels(const channel_t &a, const channel_t &b)
{
	if (a.priority != b.priority)
		return a.priority > b.priority;

	if (a.volume != b.volume)
		return a.volume > b.volume;

	if (a.start_time != b.start_time)
		return a.start_time > b.start_time;

	return false;
}


//
// S_GetChannel
//
// Attempts to find an unused channel or a channel playing a sound with a
// lower priority than sound about to be played.
//
// Returns -1 if no channels are availible.
// Returns the number of the availible channel otherwise.
//
int S_GetChannel(sfxinfo_t* sfxinfo, float volume, int priority, unsigned max_instances)
{
	// not a valid sound
	if (::Channel == NULL || sfxinfo == NULL)
		return -1;

	// Sort the sound channels by descending priority levels
	std::sort(Channel, Channel + numChannels, S_CompareChannels);

	// store priority and volume in a temp channel to use with S_CompareChannels
	channel_t tempchan;
	tempchan.clear();

	tempchan.priority = priority;
	tempchan.volume = volume;
	tempchan.start_time = gametic;

	const int sound_id = S_FindSound(sfxinfo->name);

	// Limit the number of identical sounds playing at once
	// tries to keep the plasma rifle from hogging all the channels
	for (size_t i = 0, instances = 0; i < numChannels; i++)
	{
		if (Channel[i].sound_id == sound_id)
		{
			if (++instances >= max_instances)
				return S_CompareChannels(tempchan, Channel[i]) ? i : -1;
		}
	}

	// try to find the first empty channel
	for (size_t i = 0; i < numChannels; i++)
		if (Channel[i].sfxinfo == NULL)
			return i;

	// Find a channel with lower priority
	for (size_t i = numChannels - 1; i-- > 0;)
		if (S_CompareChannels(tempchan, Channel[i]))
			return i;

	return -1;
}


//
// S_AdjustZdoomSoundParams
//
// Utilizes the sndcurve lump to mimic volume and stereo separation
// calculations from ZDoom 1.22
//
// [AM] We always play sounds now - even if they're out of range, since we
//      might step back in range during the sound.
//
static void AdjustSoundParamsZDoom(const AActor* listener, fixed_t x, fixed_t y,
                                   float* vol, int* sep)
{
	static const fixed_t MAX_SND_DIST = 2025 * FRACUNIT;
	static const fixed_t MIN_SND_DIST = 1 * FRACUNIT;
	const int approx_dist = P_AproxDistance(listener->x - x, listener->y - y);

	if (approx_dist > MAX_SND_DIST)
	{
		*vol = 0;
		*sep = NORM_SEP;
	}
	else if (approx_dist < MIN_SND_DIST)
	{
		*vol = snd_sfxvolume;
		*sep = NORM_SEP;
	}
	else
	{
		const float attenuation = static_cast<float>(SoundCurve[approx_dist >> FRACBITS]) / 128.0f;

		*vol = snd_sfxvolume * attenuation;

		// angle of source to listener
		angle_t angle = R_PointToAngle2(listener->x, listener->y, x, y);
		if (angle > listener->angle)
			angle = angle - listener->angle;
		else
			angle = angle + (0xffffffff - listener->angle);

		// stereo separation
		*sep =
		    NORM_SEP -
		    (FixedMul(S_STEREO_SWING, finesine[angle >> ANGLETOFINESHIFT]) >> FRACBITS);
	}
}


//
// S_AdjustSoundParamsDoom
//
// Changes volume and stereo-separation from the norm of a sound effect to
// be played. If the sound is not audible, returns false.
//
// joek - from Choco Doom
//
// [SL] 2011-05-26 - Changed function parameters to accept x, y instead
// of a fixed_t* for the sound origin
//
// [AM] We always play sounds now - even if they're out of range, since we
//      might step back in range during the sound.
//
static void AdjustSoundParamsDoom(const AActor* listener, fixed_t x, fixed_t y,
                                  float* vol, int* sep)
{
	static const fixed_t S_CLIPPING_DIST = 1200 * FRACUNIT;
	static const fixed_t S_CLOSE_DIST = 200 * FRACUNIT;
	fixed_t approx_dist = P_AproxDistance(listener->x - x, listener->y - y);

	if (S_UseMap8Volume())
		approx_dist = MIN(approx_dist, S_CLIPPING_DIST);

	if (approx_dist > S_CLIPPING_DIST)
	{
		*vol = 0;
		*sep = NORM_SEP;
	}
	else if (approx_dist < S_CLOSE_DIST)
	{
		*vol = snd_sfxvolume;
		*sep = NORM_SEP;
	}
	else
	{
		float attenuation = FIXED2FLOAT(
		    FixedDiv(S_CLIPPING_DIST - approx_dist, S_CLIPPING_DIST - S_CLOSE_DIST));

		// HACKY STUFF: We want to leave the attenuation, but not to make everything LOUDER.
		// On MAP 8, Doom makes a minimum volume of sounds which is 15/128 (0.192).
		// We then set that value as the strict minimum for those maps.
		if (S_UseMap8Volume() && attenuation < 0.192)
			attenuation = 0.192;

		*vol = snd_sfxvolume * attenuation;

		// angle of source to listener
		angle_t angle = R_PointToAngle2(listener->x, listener->y, x, y);
		if (angle > listener->angle)
			angle = angle - listener->angle;
		else
			angle = angle + (0xffffffff - listener->angle);

		// stereo separation
		*sep =
		    NORM_SEP -
		    (FixedMul(S_STEREO_SWING, finesine[angle >> ANGLETOFINESHIFT]) >> FRACBITS);
	}
}



//
// S_AdjustSoundParams
//
static bool AdjustSoundParams(const AActor* listener, fixed_t x, fixed_t y, float* vol,
                              int* sep)
{
	*vol = 0.0f;
	*sep = NORM_SEP;

	if (!listener)
		return false;

	if (co_zdoomsound)
		AdjustSoundParamsZDoom(listener, x, y, vol, sep);
	else
		AdjustSoundParamsDoom(listener, x, y, vol, sep);

	return true;
}


//
// S_CalculateSoundPriority
//
// Determines a priority number for the sound effect, where the higher number
// indicates greater priority should be given to this sound effect.
//
int S_CalculateSoundPriority(const fixed_t* pt, int channel, int attenuation)
{
	if (channel == CHAN_ANNOUNCER || channel == CHAN_GAMEINFO)
		return 1000;
	if (channel == CHAN_INTERFACE)
		return 800;

	int priority = 0;

	// Set up the sound channel's priority
	switch (channel)
	{
		case CHAN_WEAPON:
			priority = 150;
			break;
		case CHAN_VOICE:
			priority = 100;
			break;
		case CHAN_BODY:
			priority = 75;
			break;
		case CHAN_ITEM:
			priority = 0;
			break;
	}

	if (attenuation == ATTN_NONE)
		priority += 50;
	else if (attenuation == ATTN_IDLE || attenuation == ATTN_STATIC)
		priority -= 50;

	// Give extra priority to sounds made by the player we're viewing
	if (listenplayer().camera && pt == &listenplayer().camera->x)
		priority += 500;

	return priority;
}

static int ResolveSound(int soundid)
{
	const sfxinfo_t& sfx = S_sfx[soundid];

	if (sfx.israndom)
	{
		while (S_sfx[soundid].israndom)
		{
			std::vector<int>& list = S_rnd[soundid];
			soundid = list[P_Random() % static_cast<int>(list.size())];
		}
		return soundid;
	}

	return sfx.link;
}

//
// S_StartSound
//
// joek - choco's S_StartSoundAtVolume with some zdoom code
// a bit of a whore of a funtion but she works ok
//
static void S_StartSound(fixed_t* pt, fixed_t x, fixed_t y, int channel,
	                     int sfx_id, float volume, int attenuation, bool looping)
{
	if (volume <= 0.0f)
		return;

	if (!consoleplayer().mo && channel != CHAN_INTERFACE)
		return;

  	// check for bogus sound #
	if (sfx_id < 1 || sfx_id > static_cast<int>(S_sfx.size()) - 1)
	{
		DPrintf("Bad sfx #: %d\n", sfx_id);
		return;
	}

	sfxinfo_t* sfxinfo = &S_sfx[sfx_id];

	while (sfxinfo->link != static_cast<int>(sfxinfo_t::NO_LINK))
	{
		sfx_id = ResolveSound(sfxinfo->link);
		sfxinfo = &S_sfx[sfx_id];
	}

	if (!sfxinfo->data)
	{
		I_LoadSound(sfxinfo);
		while (sfxinfo->link != static_cast<int>(sfxinfo_t::NO_LINK))
		{
			sfx_id = ResolveSound(sfxinfo->link);
			sfxinfo = &S_sfx[sfx_id];
		}
	}

  	// check for bogus sound lump
	if (sfxinfo->lumpnum < 0 || sfxinfo->lumpnum > static_cast<int>(numlumps))
	{
		DPrintf("Bad sfx lump #: %d\n", sfxinfo->lumpnum);
		return;
	}

	if (pt == (fixed_t *)(~0))
	{
		pt = NULL;
	}
	else if (pt != NULL)
	{
		x = pt[0];
		y = pt[1];
	}

	if (sfxinfo->lumpnum == sfx_empty)
		return;

	int sep;

	if (listenplayer().camera && attenuation != ATTN_NONE)
	{
  		// Check to see if it is audible, and if not, modify the params
		if (!AdjustSoundParams(listenplayer().camera, x, y, &volume, &sep))
			return;
	}
	else
	{
		sep = NORM_SEP;

		if (channel == CHAN_ANNOUNCER)
			volume = snd_announcervolume;
		else
			volume = snd_sfxvolume;
	}

	const int priority = S_CalculateSoundPriority(pt, channel, attenuation);

	// joek - hack for silent bfg - stop player's weapon sounds if grunting
	if (sfx_id == sfx_noway || sfx_id == sfx_oof)
	{
		for (size_t i = 0; i < numChannels; i++)
		{
			if (Channel[i].sfxinfo && (Channel[i].pt == pt) && Channel[i].entchannel == CHAN_WEAPON)
				S_StopChannel(i);
		}
	}

	// [AM] Announcers take longer to mentally parse than other SFX, so don't
	//      let them cut each other off.  We run the risk of it turning into a
	//      muddled mess, but cutting it off is indecipherable in every case.
	if (channel != CHAN_ANNOUNCER)
		S_StopSound(pt, channel);

	// How many instances of the same sfx can be playing concurrently
	// Allow 3 of all sounds except announcer sfx
	const unsigned int max_instances = (channel == CHAN_ANNOUNCER) ? 1 : 3;

	// try to find a channel
	const int cnum = S_GetChannel(sfxinfo, volume, priority, max_instances);

	// no channel found
	if (cnum < 0)
		return;

	// make sure the channel isn't playing anything
	S_StopChannel(cnum);

	const int handle = I_StartSound(sfx_id, volume, sep, NORM_PITCH, looping);

	// I_StartSound can not find an empty channel
	if (handle < 0)
		return;

	Channel[cnum].handle = handle;
	Channel[cnum].sfxinfo = sfxinfo;
	Channel[cnum].sound_id = sfx_id;
	Channel[cnum].pt = pt;
	Channel[cnum].priority = priority;
	Channel[cnum].entchannel = channel;
	Channel[cnum].attenuation = attenuation;
	Channel[cnum].volume = volume;
	Channel[cnum].x = x;
	Channel[cnum].y = y;
	Channel[cnum].loop = looping;
	Channel[cnum].start_time = gametic;
}

void S_SoundID(int channel, int sound_id, float volume, int attenuation)
{
	S_StartSound((fixed_t *)NULL, 0, 0, channel, sound_id, volume, attenuation, false);
}

void S_SoundID(fixed_t x, fixed_t y, int channel, int sound_id, float volume, int attenuation)
{
	S_StartSound((fixed_t *)NULL, x, y, channel, sound_id, volume, attenuation, false);
}

void S_SoundID(AActor *ent, int channel, int sound_id, float volume, int attenuation)
{
	if (!ent)
		return;

	if (ent->subsector && ent->subsector->sector &&
		ent->subsector->sector->MoreFlags & SECF_SILENT)
		return;
	S_StartSound (&ent->x, 0, 0, channel, sound_id, volume, attenuation, false);
}

void S_SoundID(fixed_t *pt, int channel, int sound_id, float volume, int attenuation)
{
	S_StartSound(pt, 0, 0, channel, sound_id, volume, attenuation, false);
}

void S_LoopedSoundID(AActor *ent, int channel, int sound_id, float volume, int attenuation)
{
	if (!ent)
		return;

	if (ent->subsector && ent->subsector->sector &&
		ent->subsector->sector->MoreFlags & SECF_SILENT)
		return;
	S_StartSound(&ent->x, 0, 0, channel, sound_id, volume, attenuation, true);
}

void S_LoopedSoundID(fixed_t *pt, int channel, int sound_id, float volume, int attenuation)
{
	S_StartSound(pt, 0, 0, channel, sound_id, volume, attenuation, true);
}

static void S_StartNamedSound(AActor *ent, fixed_t *pt, fixed_t x, fixed_t y, int channel,
                              const char *name, float volume, int attenuation, bool looping)
{
	if (!consoleplayer().mo && channel != CHAN_INTERFACE)
		return;

	const std::string soundname = name ? name : "";

	if (soundname.empty() ||
			(ent && ent != (AActor *)(~0) && ent->subsector && ent->subsector->sector &&
			ent->subsector->sector->MoreFlags & SECF_SILENT))
	{
		return;
	}

	int sfx_id = -1;

	if (soundname[0] == '*')
	{
		// Sexed sound
		char nametemp[128];
		const char templat[] = "player/%s/%s";
                // Hacks away! -joek
		//const char *genders[] = { "male", "female", "cyborg" };
                const char *genders[] = { "male", "male", "male" };
		player_t *player;

		sfx_id = -1;
		if (ent && ent != (AActor *)(~0) && (player = ent->player))
		{
			snprintf(nametemp, 128, templat, "base", soundname.substr(1).c_str());
			sfx_id = S_FindSound(nametemp);
			if (sfx_id == -1)
			{
				snprintf(nametemp, 128, templat, genders[player->userinfo.gender], soundname.substr(1).c_str());
				sfx_id = S_FindSound(nametemp);
			}
		}
		if (sfx_id == -1)
		{
			snprintf(nametemp, 128, templat, "male", soundname.substr(1).c_str());
			sfx_id = S_FindSound(nametemp);
		}
	}
	else
		sfx_id = S_FindSound(soundname.c_str());

	if (sfx_id == -1)
	{
		DPrintf("Unknown sound %s\n", soundname.c_str());
		return;
	}

	if (ent && ent != (AActor *)(~0))
		S_StartSound(&ent->x, x, y, channel, sfx_id, volume, attenuation, looping);
	else if (pt)
		S_StartSound(pt, x, y, channel, sfx_id, volume, attenuation, looping);
	else
		S_StartSound((fixed_t *)ent, x, y, channel, sfx_id, volume, attenuation, looping);
}

// [Russell] - Hack to stop multiple plat stop sounds
void S_PlatSound(fixed_t *pt, int channel, const char *name, float volume, int attenuation)
{
    if (!predicting)
        S_StartNamedSound(NULL, pt, 0, 0, channel, name, volume, attenuation, false);
}

void S_Sound(int channel, const char *name, float volume, int attenuation)
{
	// [SL] 2011-05-27 - This particular S_Sound() function is only used for sounds
	// that should be full volume regardless of location.  Ignore the specified
	// attenuation and use ATTN_NONE instead.
	S_StartNamedSound((AActor *)NULL, NULL, 0, 0, channel, name, volume, ATTN_NONE, false);
}

void S_Sound(AActor *ent, int channel, const char *name, float volume, int attenuation)
{
	if(!co_globalsound && channel == CHAN_ITEM && ent != listenplayer().camera)
		return;

	S_StartNamedSound(ent, NULL, 0, 0, channel, name, volume, attenuation, false);
}

void S_Sound(fixed_t *pt, int channel, const char *name, float volume, int attenuation)
{
	S_StartNamedSound(NULL, pt, 0, 0, channel, name, volume, attenuation, false);
}

void S_LoopedSound(AActor *ent, int channel, const char *name, float volume, int attenuation)
{
	S_StartNamedSound(ent, NULL, 0, 0, channel, name, volume, attenuation, true);
}

void S_LoopedSound(fixed_t *pt, int channel, const char *name, float volume, int attenuation)
{
	S_StartNamedSound(NULL, pt, 0, 0, channel, name, volume, attenuation, true);
}

void S_Sound(fixed_t x, fixed_t y, int channel, const char *name, float volume, int attenuation)
{
	S_StartNamedSound((AActor *)(~0), NULL, x, y, channel, name, volume, attenuation, false);
}


//
// S_StopChannel
//
static void S_StopChannel(unsigned int cnum)
{
	if (::Channel == NULL)
		return;

	if (cnum >= numChannels)
	{
		DPrintf("Trying to stop invalid channel %d\n", cnum);
		return;
	}

	channel_t* c = &Channel[cnum];

	if (c->sfxinfo && c->handle >= 0)
		I_StopSound(c->handle);

	c->clear();
}


void S_StopSound(fixed_t *pt)
{
	for (unsigned int i = 0; i < numChannels; i++)
		if (Channel[i].sfxinfo && (Channel[i].pt == pt))
		{
			S_StopChannel(i);
		}
}

void S_StopSound(fixed_t *pt, int channel)
{
	if (::Channel == NULL)
		return;

	for (unsigned int i = 0; i < numChannels; i++)
		if (Channel[i].sfxinfo
			&& Channel[i].pt == pt // denis - fixme - security - wouldn't this cause invalid access elsewhere, if an object was destroyed?
			&& Channel[i].entchannel == channel)
			S_StopChannel(i);
}

void S_StopSound(AActor *ent, int channel)
{
	S_StopSound(&ent->x, channel);
}

void S_StopAllChannels()
{
	for (unsigned i = 0; i < numChannels; i++)
		S_StopChannel(i);
}


// Moves all the sounds from one thing to another. If the destination is
// NULL, then the sound becomes a positioned sound.
void S_RelinkSound(AActor *from, AActor *to)
{
	if (::Channel == NULL)
		return;

	if (!from)
		return;

	const fixed_t *frompt = &from->x;
	fixed_t *topt = to ? &to->x : NULL;

	for (unsigned int i = 0; i < numChannels; i++)
	{
		if (Channel[i].pt == frompt)
		{
			Channel[i].pt = topt;
			Channel[i].x = frompt[0];
			Channel[i].y = frompt[1];
		}
	}
}

bool S_GetSoundPlayingInfo(fixed_t *pt, int sound_id)
{
	for (unsigned int i = 0; i < numChannels; i++)
	{
		if (Channel[i].pt == pt && Channel[i].sound_id == sound_id)
			return true;
	}
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
	if (!mus_paused)
	{
		I_PauseSong();
		mus_paused = true;
	}
}

void S_ResumeSound()
{
	if (mus_paused)
	{
		I_ResumeSong();
		mus_paused = false;
	}
}

//
// Updates music & sounds
//
// joek - from choco again
void S_UpdateSounds(void* listener_p)
{
	if (::Channel == NULL)
		return;

	AActor* listener = (AActor*)listener_p;
	for (int cnum = 0; cnum < (int)numChannels; cnum++)
	{
		const channel_t* c = &Channel[cnum];
		const sfxinfo_t* sfx = c->sfxinfo;

		if (c->sfxinfo)
		{
			if (I_SoundIsPlaying(c->handle))
			{
				// initialize parameters
				int sep = NORM_SEP;

				float maxvolume;
				if (Channel[cnum].entchannel == CHAN_ANNOUNCER)
					maxvolume = snd_announcervolume;
				else
					maxvolume = snd_sfxvolume;

				float volume = maxvolume;

				if (sfx->link != static_cast<int>(sfxinfo_t::NO_LINK))
				{
					volume += Channel[cnum].volume;

					if (volume <= 0)
					{
						S_StopChannel(cnum);
						continue;
					}
					else if (volume > maxvolume)
					{
						volume = maxvolume;
					}
				}

				// check non-local sounds for distance clipping
				//  or modify their params
				if (listener && &(listener->x) != c->pt && c->attenuation != ATTN_NONE)
				{
					fixed_t x, y;
					if (c->pt) // [SL] 2011-05-29
					{
						x = c->pt[0]; // update the sound coorindates
						y = c->pt[1]; // for moving actors
					}
					else
					{
						x = c->x;
						y = c->y;
					}

					if (AdjustSoundParams(listener, x, y, &volume, &sep))
						I_UpdateSoundParams(c->handle, volume, sep, NORM_PITCH);
					else
						S_StopChannel(cnum);
				}
			}
			else
			{
				// if channel is allocated but sound has stopped,
				// free it
				S_StopChannel(cnum);
			}
		}
	}
	// kill music if it is a single-play && finished
	// if (	mus_playing
	//      && !I_QrySongPlaying(mus_playing->handle)
	//      && !mus_paused )
	// S_StopMusic();
}

void S_UpdateMusic()
{
	I_UpdateMusic();
}

void S_SetMusicVolume(float volume)
{
	if (volume < 0.0 || volume > 1.0)
		Printf (PRINT_HIGH, "Attempt to set music volume at %f\n", volume);
	else
		I_SetMusicVolume (volume);
}

void S_SetSfxVolume(float volume)
{
	if (volume < 0.0 || volume > 1.0)
		Printf (PRINT_HIGH, "Attempt to set sfx volume at %f\n", volume);
	else
		I_SetSfxVolume (volume);
}

//
// Starts some music with the music id found in sounds.h.
//
void S_StartMusic(const char *m_id)
{
	S_ChangeMusic (m_id, false);
}

// [RH] S_ChangeMusic() now accepts the name of the music lump.
// It's up to the caller to figure out what that name is.
void S_ChangeMusic(std::string musicname, int looping)
{
	// [SL] Avoid caching music lumps if we're not playing music
	if (snd_musicsystem == MS_NONE)
		return;

	if (mus_playing.name == musicname)
		return;

	if (!musicname.length() || musicname[0] == 0)
	{
		// Don't choke if the map doesn't have a song attached
		S_StopMusic();
		return;
	}

	byte* data = NULL;
	size_t length = 0;
	FILE *f;

	if (!(f = fopen (musicname.c_str(), "rb")))
	{
		int lumpnum;
		if ((lumpnum = W_CheckNumForName (musicname.c_str())) == -1)
		{
			Printf (PRINT_HIGH, "Music lump \"%s\" not found\n", musicname.c_str());
			return;
		}

		data = static_cast<byte*>(W_CacheLumpNum(lumpnum, PU_CACHE));
		length = W_LumpLength(lumpnum);
		I_PlaySong(data, length, (looping != 0));
    }
    else
	{
		length = M_FileLength(f);
		data = static_cast<byte*>(Malloc(length));
		const size_t result = fread(data, length, 1, f);
		fclose(f);

		if (result == 1)
			I_PlaySong(data, length, (looping != 0));
		M_Free(data);
	}

	mus_playing.name = musicname;
}

void S_StopMusic()
{
	I_StopSong();

	mus_playing.name = "";
}



// [RH] ===============================
//
//	Ambient sound and SNDINFO routines
//
// =============================== [RH]

std::vector<sfxinfo_t> S_sfx;	// [RH] This is no longer defined in sounds.c
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
							ambient->attenuation = (os.getTokenFloat() > 0) ? os.getTokenFloat() : 1;
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
					//else if (os.compareTokenNoCase("world"))
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
						ambient->periodmin = static_cast<int>(os.getTokenFloat() * TICRATE);
						os.mustScanFloat();
						ambient->periodmax = static_cast<int>(os.getTokenFloat() * TICRATE);
					}
					else if (os.compareTokenNoCase("periodic"))
					{
						ambient->type |= PERIODIC;
						os.mustScanFloat();
						ambient->periodmin = static_cast<int>(os.getTokenFloat() * TICRATE);
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
							       "recursively.\n", os.getToken().c_str());
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

	sfx_empty = W_CheckNumForName("dsempty");
	sfx_noway = S_FindSoundByLump(W_CheckNumForName("dsnoway"));
	sfx_oof = S_FindSoundByLump(W_CheckNumForName("dsoof"));
}


static void SetTicker(int *tics, AmbientSound *ambient)
{
	if ((ambient->type & CONTINUOUS) == CONTINUOUS)
	{
		*tics = 1;
	}
	else if (ambient->type & RANDOM)
	{
		*tics = (int)(((float)rand() / (float)RAND_MAX) *
				(float)(ambient->periodmax - ambient->periodmin)) +
				ambient->periodmin;
	}
	else
	{
		*tics = ambient->periodmin;
	}

	// [SL] 2012-04-27 - Do not allow updates every 0 tics as it causes
	// an infinite loop
	if (*tics == 0)
		*tics = 1;
}

void A_Ambient(AActor *actor)
{
	if (!actor)
		return;

	AmbientSound *ambient = &Ambients[actor->args[0]];

	if ((ambient->type & CONTINUOUS) == CONTINUOUS)
	{
		if (S_GetSoundPlayingInfo (actor, S_FindSound (ambient->sound)))
			return;

		if (ambient->sound[0])
		{
			S_StartNamedSound (actor, NULL, 0, 0, CHAN_BODY,
				ambient->sound, ambient->volume, ambient->attenuation, true);

			SetTicker (&actor->tics, ambient);
		}
		else
		{
			actor->Destroy();
		}
	}
	else
	{
		if (ambient->sound[0])
		{
			S_StartNamedSound (actor, NULL, 0, 0, CHAN_BODY,
				ambient->sound, ambient->volume, ambient->attenuation, false);

			SetTicker (&actor->tics, ambient);
		}
		else
		{
			actor->Destroy ();
		}
	}
}

void S_ActivateAmbient(AActor *origin, int ambient)
{
	if (!origin)
		return;

	AmbientSound *amb = &Ambients[ambient];

	if (!(amb->type & 3) && !amb->periodmin)
	{
		const int sndnum = S_FindSound(amb->sound);
		if (sndnum == 0)
			return;

		sfxinfo_t *sfx = &S_sfx[sndnum];

		// Make sure the sound has been loaded so we know how long it is
		if (!sfx->data)
			I_LoadSound (sfx);
		amb->periodmin = (sfx->ms * TICRATE) / 1000;
	}

	if (amb->type & (RANDOM|PERIODIC))
		SetTicker (&origin->tics, amb);
	else
		origin->tics = 1;
}

BEGIN_COMMAND (snd_soundlist)
{
	for (unsigned i = 0; i < S_sfx.size(); i++)
		if (S_sfx[i].lumpnum != -1)
		{
			const OLumpName lumpname = lumpinfo[S_sfx[i].lumpnum].name;
			Printf(PRINT_HIGH, "%3d. %s (%s)\n", i+1, S_sfx[i].name, lumpname.c_str());
		}
		// todo: check if sounds are multiple lumps rather than just one (i.e. random sounds)
		else
			Printf (PRINT_HIGH, "%3d. %s **not present**\n", i+1, S_sfx[i].name);
}
END_COMMAND (snd_soundlist)

BEGIN_COMMAND (snd_soundlinks)
{
	for (unsigned i = 0; i < S_sfx.size(); i++)
		if (S_sfx[i].link != static_cast<int>(sfxinfo_t::NO_LINK))
			Printf(PRINT_HIGH, "%s -> %s\n", S_sfx[i].name, S_sfx[S_sfx[i].link].name);
}
END_COMMAND (snd_soundlinks)

BEGIN_COMMAND (snd_restart)
{
	S_Stop ();
	S_Init (snd_sfxvolume, snd_musicvolume);
}
END_COMMAND (snd_restart)

BEGIN_COMMAND (changemus)
{
	if (argc == 1)
	{
	    Printf(PRINT_HIGH, "Usage: changemus lumpname [loop]");
	    Printf(PRINT_HIGH, "\n");
	    Printf(PRINT_HIGH, "Plays music from an internal lump, loop\n");
	    Printf(PRINT_HIGH, "parameter determines if the music should play\n");
	    Printf(PRINT_HIGH, "continuously or not, (1 or 0, default: 1)\n");

	    return;
	}

	const std::string musicname(argv[1]);

	if (argc > 2)
	{
		const int loopmus = (atoi(argv[2]) != 0);
		S_ChangeMusic(musicname, loopmus);
	}
	else if (argc == 2)
	{
		S_ChangeMusic(musicname, true);
	}
}
END_COMMAND (changemus)

//
// UV_SoundAvoidCl
// Sends a sound to clients, but doesn't send it to client 'player'.
//
void UV_SoundAvoidPlayer(AActor *mo, byte channel, const char *name, byte attenuation)
{
	S_Sound(mo, channel, name, 1, attenuation);
}

VERSION_CONTROL (s_sound_cpp, "$Id$")
