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
// DESCRIPTION:
//	The not so system specific sound interface.
//
//-----------------------------------------------------------------------------


#include <algorithm>

#include "cl_main.h"

#include "m_alloc.h"
#include "i_system.h"
#include "i_sound.h"
#include "i_music.h"
#include "s_sound.h"
#include "s_sndseq.h"
#include "c_dispatch.h"
#include "z_zone.h"
#include "m_random.h"
#include "w_wad.h"
#include "doomdef.h"
#include "p_local.h"
#include "doomstat.h"
#include "cmdlib.h"
#include "i_video.h"
#include "v_video.h"
#include "v_text.h"
#include "m_vectors.h"
#include "m_fileio.h"
#include "gi.h"

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
static BOOL		mus_paused;

// music currently being played
static struct mus_playing_t
{
	std::string name;
	int   handle;
} mus_playing;

EXTERN_CVAR (co_globalsound)
EXTERN_CVAR (co_zdoomsound)
EXTERN_CVAR (snd_musicsystem)

size_t			numChannels;

//
// [RH] Print sound debug info. Called from D_Display()
//
void S_NoiseDebug (void)
{
	fixed_t ox, oy;
	unsigned int i;
	int y;
	int color;

	y = 32 * CleanYfac;
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

	for (i = 0;((i < numChannels) && (y < I_GetVideoHeight() - 16)); i++, y += 8)
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
			color = Channel[i].loop ? CR_BROWN : CR_GREY;
			strcpy (temp, lumpinfo[Channel[i].sfxinfo->lumpnum].name);
			temp[8] = 0;
			screen->DrawText (color, 0, y, temp);
			sprintf (temp, "%d", ox / FRACUNIT);
			screen->DrawText (color, 70, y, temp);
			sprintf (temp, "%d", oy / FRACUNIT);
			screen->DrawText (color, 120, y, temp);
			sprintf (temp, "%.2f", Channel[i].volume);
			screen->DrawText (color, 170, y, temp);
			sprintf (temp, "%d", Channel[i].priority);
			screen->DrawText (color, 200, y, temp);
			sprintf (temp, "%d", P_AproxDistance2 (listenplayer().camera, ox, oy) / FRACUNIT);
			screen->DrawText (color, 240, y, temp);
			sprintf (temp, "%d", Channel[i].entchannel);
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
// based on the current map number and the status of co_level8soundfeature
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
static void S_StopChannel (unsigned int cnum);


//
// Initializes sound stuff, including volume
// Sets channels, SFX and music volume,
// allocates channel buffer, sets S_sfx lookup.
//
void S_Init (float sfxVolume, float musicVolume)
{
	SoundCurve = (byte *)wads.CacheLumpNum(wads.GetNumForName("SNDCURVE"), PU_STATIC);

	// [RH] Read in sound sequences
	NumSequences = 0;
	S_ParseSndSeq ();

	S_SetSfxVolume (sfxVolume);
	S_SetMusicVolume (musicVolume);

	// Allocating the internal channels for mixing
	// (the maximum numer of sounds rendered
	// simultaneously) within zone memory.
	numChannels = snd_channels.asInt();
	Channel = (channel_t*)Z_Malloc(numChannels * sizeof(channel_t), PU_STATIC, 0);
	for (size_t i = 0; i < numChannels; i++)
		Channel[i].clear();

	I_SetChannels (numChannels);

	// no sounds are playing, and they are not mus_paused
	mus_paused = 0;
}


//
// Kills playing sounds
//
void S_Stop (void)
{
	// kill all playing sounds at start of level
	//	(trust me - a good idea)
	for (size_t i = 0; i < numChannels; i++)
		S_StopChannel(i);

	S_StopMusic();
}


//
// Per level startup code.
// Kills playing sounds at start of level,
// determines music if any, changes music.
//
void S_Start (void)
{
	S_Stop();

	// start new music for the level
	mus_paused = 0;

	// [RH] This is a lot simpler now.
	S_ChangeMusic (std::string(level.music, 8), true);
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
	if (!sfxinfo)
		return -1;

	// Sort the sound channels by descending priority levels
	std::sort(Channel, Channel + numChannels, S_CompareChannels);

	// store priority and volume in a temp channel to use with S_CompareChannels
	channel_t tempchan;
	tempchan.clear();

	tempchan.priority = priority;
	tempchan.volume = volume;
	tempchan.start_time = gametic;

	int sound_id = S_FindSound(sfxinfo->name);

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
	for (size_t i = numChannels - 1; i >= 0; i--)
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
bool S_AdjustSoundParamsZDoom(	const AActor*	listener,
								fixed_t			x,
								fixed_t			y,
								float*			vol,
								int*			sep)
{
	static const fixed_t MAX_SND_DIST = 2025 * FRACUNIT;
	static const fixed_t MIN_SND_DIST = 1 * FRACUNIT;
	int approx_dist = P_AproxDistance(listener->x - x, listener->y - y);

	if (S_UseMap8Volume())
		approx_dist = MIN(approx_dist, MAX_SND_DIST);

	if (approx_dist > MAX_SND_DIST)
		return false;

	if (approx_dist < MIN_SND_DIST)
	{
		*vol = snd_sfxvolume;
		*sep = NORM_SEP;
	}
	else
	{
		float attenuation = float(SoundCurve[approx_dist >> FRACBITS]) / 128.0f;
		if (S_UseMap8Volume())
			*vol = 1.0f + (snd_sfxvolume - 1.0f) * attenuation;
		else
			*vol = snd_sfxvolume * attenuation;

		// angle of source to listener
		angle_t angle = R_PointToAngle2(listener->x, listener->y, x, y);
		if (angle > listener->angle)
			angle = angle - listener->angle;
		else
			angle = angle + (0xffffffff - listener->angle);

		// stereo separation
		*sep = NORM_SEP - (FixedMul(S_STEREO_SWING, finesine[angle >> ANGLETOFINESHIFT]) >> FRACBITS);
	}

	return (*vol > 0.0f);
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
bool S_AdjustSoundParamsDoom(	const AActor*	listener,
								fixed_t			x,
								fixed_t			y,
								float*			vol,
								int*			sep)
{
	static const fixed_t S_CLIPPING_DIST = 1200 * FRACUNIT;
	static const fixed_t S_CLOSE_DIST = 200 * FRACUNIT;
	fixed_t approx_dist = P_AproxDistance(listener->x - x, listener->y - y);

	if (S_UseMap8Volume())
		approx_dist = MIN(approx_dist, S_CLIPPING_DIST);

	if (approx_dist > S_CLIPPING_DIST)
		return false;

	if (approx_dist < S_CLOSE_DIST)
	{
		*vol = snd_sfxvolume;
		*sep = NORM_SEP;
	}
	else
	{
		float attenuation = FIXED2FLOAT(FixedDiv(S_CLIPPING_DIST - approx_dist, S_CLIPPING_DIST - S_CLOSE_DIST));
		if (S_UseMap8Volume())
			*vol = 1.0f + (snd_sfxvolume - 1.0f) * attenuation;
		else
			*vol = snd_sfxvolume * attenuation;

		// angle of source to listener
		angle_t angle = R_PointToAngle2(listener->x, listener->y, x, y);
		if (angle > listener->angle)
			angle = angle - listener->angle;
		else
			angle = angle + (0xffffffff - listener->angle);

		// stereo separation
		*sep = NORM_SEP - (FixedMul(S_STEREO_SWING, finesine[angle >> ANGLETOFINESHIFT]) >> FRACBITS);
	}

	return (*vol > 0.0f);
}



//
// S_AdjustSoundParams
//
bool S_AdjustSoundParams(	const AActor*	listener,
		  					fixed_t			x,
		  					fixed_t			y,
		  					float*			vol,
		  					int*			sep)
{
	*vol = 0.0f;
	*sep = NORM_SEP;

	if (!listener)
		return false;

	if (co_zdoomsound)
		return S_AdjustSoundParamsZDoom(listener, x, y, vol, sep);
	else
		return S_AdjustSoundParamsDoom(listener, x, y, vol, sep);
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


//
// S_StartSound
//
// joek - choco's S_StartSoundAtVolume with some zdoom code
// a bit of a whore of a funtion but she works ok
//
static void S_StartSound(fixed_t* pt, fixed_t x, fixed_t y, int channel,
	                  int sfx_id, float volume, int attenuation, bool looping)
{
	int		sep;

	if (volume <= 0.0f)
		return;

	if (!consoleplayer().mo && channel != CHAN_INTERFACE)
		return;

  	// check for bogus sound #
	if (sfx_id < 1 || sfx_id > numsfx)
	{
		DPrintf("Bad sfx #: %d\n", sfx_id);
		return;
	}

	sfxinfo_t* sfxinfo = &S_sfx[sfx_id];

  	// check for bogus sound lump
	if (sfxinfo->lumpnum < 0 || sfxinfo->lumpnum > (int)numlumps)
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

	if (sfxinfo->link)
		sfxinfo = sfxinfo->link;

	if (!sfxinfo->data)
	{
		I_LoadSound(sfxinfo);
		if (sfxinfo->link)
			sfxinfo = sfxinfo->link;
	}

	if (sfxinfo->lumpnum == sfx_empty)
		return;

	if (listenplayer().camera && attenuation != ATTN_NONE)
	{
  		// Check to see if it is audible, and if not, modify the params
		if (!S_AdjustSoundParams(listenplayer().camera, x, y, &volume, &sep))
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

	int priority = S_CalculateSoundPriority(pt, channel, attenuation);

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
	unsigned int max_instances = (channel == CHAN_ANNOUNCER) ? 1 : 3;

	// try to find a channel
	int cnum = S_GetChannel(sfxinfo, volume, priority, max_instances);

	// no channel found
	if (cnum < 0)
		return;

	// make sure the channel isn't playing anything
	S_StopChannel(cnum);

	int handle = I_StartSound(sfx_id, volume, sep, NORM_PITCH, looping);

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

void S_SoundID (int channel, int sound_id, float volume, int attenuation)
{
	S_StartSound ((fixed_t *)NULL, 0, 0, channel, sound_id, volume, attenuation, false);
}

void S_SoundID (fixed_t x, fixed_t y, int channel, int sound_id, float volume, int attenuation)
{
	S_StartSound ((fixed_t *)NULL, x, y, channel, sound_id, volume, attenuation, false);
}

void S_SoundID (AActor *ent, int channel, int sound_id, float volume, int attenuation)
{
	if (!ent)
		return;

	if (ent->subsector && ent->subsector->sector &&
		ent->subsector->sector->MoreFlags & SECF_SILENT)
		return;
	S_StartSound (&ent->x, 0, 0, channel, sound_id, volume, attenuation, false);
}

void S_SoundID (fixed_t *pt, int channel, int sound_id, float volume, int attenuation)
{
	S_StartSound (pt, 0, 0, channel, sound_id, volume, attenuation, false);
}

void S_LoopedSoundID (AActor *ent, int channel, int sound_id, float volume, int attenuation)
{
	if (!ent)
		return;

	if (ent->subsector && ent->subsector->sector &&
		ent->subsector->sector->MoreFlags & SECF_SILENT)
		return;
	S_StartSound (&ent->x, 0, 0, channel, sound_id, volume, attenuation, true);
}

void S_LoopedSoundID (fixed_t *pt, int channel, int sound_id, float volume, int attenuation)
{
	S_StartSound (pt, 0, 0, channel, sound_id, volume, attenuation, true);
}

static void S_StartNamedSound (AActor *ent, fixed_t *pt, fixed_t x, fixed_t y, int channel,
                               const char *name, float volume, int attenuation, bool looping)
{
	int sfx_id = -1;

	if (!consoleplayer().mo && channel != CHAN_INTERFACE)
		return;

	if (name == NULL || strlen(name) == 0 ||
			(ent && ent != (AActor *)(~0) && ent->subsector && ent->subsector->sector &&
			ent->subsector->sector->MoreFlags & SECF_SILENT))
	{
		return;
	}

	if (*name == '*')
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
			sprintf(nametemp, templat, "base", name + 1);
			sfx_id = S_FindSound(nametemp);
			if (sfx_id == -1)
			{
				sprintf(nametemp, templat, genders[player->userinfo.gender], name + 1);
				sfx_id = S_FindSound (nametemp);
			}
		}
		if (sfx_id == -1)
		{
			sprintf(nametemp, templat, "male", name + 1);
			sfx_id = S_FindSound (nametemp);
		}
	}
	else
		sfx_id = S_FindSound (name);

	if (sfx_id == -1)
		DPrintf ("Unknown sound %s\n", name);

	if (ent && ent != (AActor *)(~0))
		S_StartSound (&ent->x, x, y, channel, sfx_id, volume, attenuation, looping);
	else if (pt)
		S_StartSound (pt, x, y, channel, sfx_id, volume, attenuation, looping);
	else
		S_StartSound ((fixed_t *)ent, x, y, channel, sfx_id, volume, attenuation, looping);
}

// [Russell] - Hack to stop multiple plat stop sounds
void S_PlatSound (fixed_t *pt, int channel, const char *name, float volume, int attenuation)
{
    if (!predicting)
        S_StartNamedSound (NULL, pt, 0, 0, channel, name, volume, attenuation, false);
}

void S_Sound (int channel, const char *name, float volume, int attenuation)
{
	// [SL] 2011-05-27 - This particular S_Sound() function is only used for sounds
	// that should be full volume regardless of location.  Ignore the specified
	// attenuation and use ATTN_NONE instead.
	S_StartNamedSound ((AActor *)NULL, NULL, 0, 0, channel, name, volume, ATTN_NONE, false);
}

void S_Sound (AActor *ent, int channel, const char *name, float volume, int attenuation)
{
	if(!co_globalsound && channel == CHAN_ITEM && ent != listenplayer().camera)
		return;

	S_StartNamedSound (ent, NULL, 0, 0, channel, name, volume, attenuation, false);
}

void S_Sound (fixed_t *pt, int channel, const char *name, float volume, int attenuation)
{
	S_StartNamedSound (NULL, pt, 0, 0, channel, name, volume, attenuation, false);
}

void S_LoopedSound (AActor *ent, int channel, const char *name, float volume, int attenuation)
{
	S_StartNamedSound (ent, NULL, 0, 0, channel, name, volume, attenuation, true);
}

void S_LoopedSound (fixed_t *pt, int channel, const char *name, float volume, int attenuation)
{
	S_StartNamedSound (NULL, pt, 0, 0, channel, name, volume, attenuation, true);
}

void S_Sound (fixed_t x, fixed_t y, int channel, const char *name, float volume, int attenuation)
{
	S_StartNamedSound ((AActor *)(~0), NULL, x, y, channel, name, volume, attenuation, false);
}


//
// S_StopChannel
//
static void S_StopChannel(unsigned int cnum)
{
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


void S_StopSound (fixed_t *pt)
{
	for (unsigned int i = 0; i < numChannels; i++)
		if (Channel[i].sfxinfo && (Channel[i].pt == pt))
		{
			S_StopChannel(i);
		}
}

void S_StopSound (fixed_t *pt, int channel)
{
	for (unsigned int i = 0; i < numChannels; i++)
		if (Channel[i].sfxinfo
			&& Channel[i].pt == pt // denis - fixme - security - wouldn't this cause invalid access elsewhere, if an object was destroyed?
			&& Channel[i].entchannel == channel)
			S_StopChannel(i);
}

void S_StopSound (AActor *ent, int channel)
{
	S_StopSound (&ent->x, channel);
}

void S_StopAllChannels(void)
{
	for (size_t i = 0; i < numChannels; i++)
		S_StopChannel(i);
}


// Moves all the sounds from one thing to another. If the destination is
// NULL, then the sound becomes a positioned sound.
void S_RelinkSound (AActor *from, AActor *to)
{
	unsigned int i;

	if (!from)
		return;

	fixed_t *frompt = &from->x;
	fixed_t *topt = to ? &to->x : NULL;

	for (i = 0; i < numChannels; i++) {
		if (Channel[i].pt == frompt) {
			Channel[i].pt = topt;
			Channel[i].x = frompt[0];
			Channel[i].y = frompt[1];
		}
	}
}

bool S_GetSoundPlayingInfo (fixed_t *pt, int sound_id)
{
	unsigned int i;

	for (i = 0; i < numChannels; i++)
	{
		if (Channel[i].pt == pt && Channel[i].sound_id == sound_id)
			return true;
	}
	return false;
}

bool S_GetSoundPlayingInfo (AActor *ent, int sound_id)
{
	return S_GetSoundPlayingInfo (ent ? &ent->x : NULL, sound_id);
}

//
// Stop and resume music, during game PAUSE.
//
void S_PauseSound (void)
{
	if (!mus_paused)
	{
		I_PauseSong();
		mus_paused = true;
	}
}

void S_ResumeSound (void)
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
void S_UpdateSounds (void *listener_p)
{
	int		cnum;
	float		volume;
	int		sep;
	sfxinfo_t*	sfx;
	channel_t*	c;

	AActor *listener = (AActor *)listener_p;

	for (cnum=0 ; cnum < (int)numChannels ; cnum++)
	{
		c = &Channel[cnum];
		sfx = c->sfxinfo;

		if (c->sfxinfo)
		{
			if (I_SoundIsPlaying(c->handle))
			{
				// initialize parameters
				sep = NORM_SEP;

				float maxvolume;
				if (Channel[cnum].entchannel == CHAN_ANNOUNCER)
					maxvolume = snd_announcervolume;
				else
					maxvolume = snd_sfxvolume;

				volume = maxvolume;

				if (sfx->link)
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
					if (c->pt)		// [SL] 2011-05-29
					{
						x = c->pt[0];	// update the sound coorindates
						y = c->pt[1];	// for moving actors
					}
					else
					{
						x = c->x;
						y = c->y;
					}

					if (S_AdjustSoundParams(listener, x, y, &volume, &sep))
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

void S_SetMusicVolume (float volume)
{
	if (volume < 0.0 || volume > 1.0)
		Printf (PRINT_HIGH, "Attempt to set music volume at %f\n", volume);
	else
		I_SetMusicVolume (volume);
}

void S_SetSfxVolume (float volume)
{
	if (volume < 0.0 || volume > 1.0)
		Printf (PRINT_HIGH, "Attempt to set sfx volume at %f\n", volume);
	else
		I_SetSfxVolume (volume);
}

//
// Starts some music with the music id found in sounds.h.
//
void S_StartMusic (const char *m_id)
{
	S_ChangeMusic (m_id, false);
}

// [RH] S_ChangeMusic() now accepts the name of the music lump.
// It's up to the caller to figure out what that name is.
void S_ChangeMusic (std::string musicname, int looping)
{
	
	// [SL] Avoid caching music lumps if we're not playing music
	if (snd_musicsystem == MS_NONE)
		return;
		
	if (mus_playing.name == musicname)
		return;

	if (!musicname.length() || musicname[0] == 0)
	{
		// Don't choke if the map doesn't have a song attached
		S_StopMusic ();
		return;
	}

	byte* data = NULL;
	size_t length = 0;
	int lumpnum;
	FILE *f;


	if (!(f = fopen (musicname.c_str(), "rb")))
	{
		if ((lumpnum = wads.CheckNumForName (musicname.c_str())) == -1)
		{
			Printf (PRINT_HIGH, "Music lump \"%s\" not found\n", musicname.c_str());
			return;
		}

		data = static_cast<byte*>(wads.CacheLumpNum(lumpnum, PU_CACHE));
		length = wads.LumpLength(lumpnum);
		I_PlaySong(data, length, (looping != 0));
    }
    else
	{
		lumpnum = -1;
		length = M_FileLength(f);
		data = static_cast<byte*>(Malloc(length));
		size_t result = fread(data, length, 1, f);
		fclose(f);

		if (result == 1)
			I_PlaySong(data, length, (looping != 0));
		M_Free(data);
	}

	mus_playing.name = musicname;
}

void S_StopMusic (void)
{
	I_StopSong();

	mus_playing.name = "";
}



// [RH] ===============================
//
//	Ambient sound and SNDINFO routines
//
// =============================== [RH]

sfxinfo_t *S_sfx;	// [RH] This is no longer defined in sounds.c
static int maxsfx;	// [RH] Current size of S_sfx array.
int numsfx;			// [RH] Current number of sfx defined.

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

void S_HashSounds (void)
{
	int i;
	unsigned j;

	// Mark all buckets as empty
	for (i = 0; i < numsfx; i++)
		S_sfx[i].index = ~0;

	// Now set up the chains
	for (i = 0; i < numsfx; i++) {
		j = MakeKey (S_sfx[i].name) % (unsigned)numsfx;
		S_sfx[i].next = S_sfx[j].index;
		S_sfx[j].index = i;
	}
}

int S_FindSound (const char *logicalname)
{
	if(!numsfx)
		return -1;

	int i = S_sfx[MakeKey (logicalname) % (unsigned)numsfx].index;

	while ((i != -1) && strnicmp (S_sfx[i].name, logicalname, MAX_SNDNAME))
		i = S_sfx[i].next;

	return i;
}

int S_FindSoundByLump (int lump)
{
	if (lump != -1) {
		int i;

		for (i = 0; i < numsfx; i++)
			if (S_sfx[i].lumpnum == lump)
				return i;
	}
	return -1;
}

int S_AddSoundLump (char *logicalname, int lump)
{
	if (numsfx == maxsfx) {
		maxsfx = maxsfx ? maxsfx*2 : 128;
		S_sfx = (sfxinfo_struct *)Realloc (S_sfx, maxsfx * sizeof(*S_sfx));
	}

	// logicalname MUST be < MAX_SNDNAME chars long
	strcpy (S_sfx[numsfx].name, logicalname);
	S_sfx[numsfx].data = NULL;
	S_sfx[numsfx].link = NULL;
	S_sfx[numsfx].lumpnum = lump;
	return numsfx++;
}

void S_ClearSoundLumps()
{
	M_Free(S_sfx);

	numsfx = 0;
	maxsfx = 0;
}

int S_AddSound (char *logicalname, char *lumpname)
{
	int sfxid;

	// If the sound has already been defined, change the old definition.
	for (sfxid = 0; sfxid < numsfx; sfxid++)
		if (0 == stricmp (logicalname, S_sfx[sfxid].name))
			break;

	int lump = wads.CheckNumForName (lumpname);

	// Otherwise, prepare a new one.
	if (sfxid == numsfx)
		sfxid = S_AddSoundLump (logicalname, lump);
	else
		S_sfx[sfxid].lumpnum = lump;

	return sfxid;
}

// S_ParseSndInfo
// Parses all loaded SNDINFO lumps.
void S_ParseSndInfo (void)
{
	char *sndinfo;
	char *data;

	S_ClearSoundLumps ();

	int lump = -1;
	while ((lump = wads.FindLump ("SNDINFO", lump)) != -1)
	{
		sndinfo = (char *)wads.CacheLumpNum (lump, PU_CACHE);

		while ( (data = COM_Parse (sndinfo)) ) {
			if (com_token[0] == ';') {
				// Handle comments from Hexen MAPINFO lumps
				while (*sndinfo && *sndinfo != ';')
					sndinfo++;
				while (*sndinfo && *sndinfo != '\n')
					sndinfo++;
				continue;
			}
			sndinfo = data;
			if (com_token[0] == '$') {
				// com_token is a command

				if (!stricmp (com_token + 1, "ambient")) {
					// $ambient <num> <logical name> [point [atten]|surround] <type> [secs] <relative volume>
					struct AmbientSound *ambient, dummy;
					int index;

					sndinfo = COM_Parse (sndinfo);
					index = atoi (com_token);
					if (index < 0 || index > 255) {
						Printf (PRINT_HIGH, "Bad ambient index (%d)\n", index);
						ambient = &dummy;
					} else {
						ambient = Ambients + index;
					}
                    
                    ambient->type = 0;
                    ambient->periodmin = 0;
                    ambient->periodmax = 0;
                    ambient->volume = 0.0f;

					sndinfo = COM_Parse (sndinfo);
					strncpy (ambient->sound, com_token, MAX_SNDNAME);
					ambient->sound[MAX_SNDNAME] = 0;
					ambient->attenuation = 0.0f;

					sndinfo = COM_Parse (sndinfo);
					if (!stricmp (com_token, "point")) {
						float attenuation;

						ambient->type = POSITIONAL;
						sndinfo = COM_Parse (sndinfo);
						attenuation = (float)atof (com_token);
						if (attenuation > 0)
						{
							ambient->attenuation = attenuation;
							sndinfo = COM_Parse (sndinfo);
						}
						else
						{
							ambient->attenuation = 1;
						}
					} else if (!stricmp (com_token, "surround")) {
						ambient->type = SURROUND;
						sndinfo = COM_Parse (sndinfo);
						ambient->attenuation = -1;
					}

					if (!stricmp (com_token, "continuous")) {
						ambient->type |= CONTINUOUS;
					} else if (!stricmp (com_token, "random")) {
						ambient->type |= RANDOM;
						sndinfo = COM_Parse (sndinfo);
						ambient->periodmin = (int)(atof (com_token) * TICRATE);
						sndinfo = COM_Parse (sndinfo);
						ambient->periodmax = (int)(atof (com_token) * TICRATE);
					} else if (!stricmp (com_token, "periodic")) {
						ambient->type |= PERIODIC;
						sndinfo = COM_Parse (sndinfo);
						ambient->periodmin = (int)(atof (com_token) * TICRATE);
					} else {
						Printf (PRINT_HIGH, "Unknown ambient type (%s)\n", com_token);
					}

					sndinfo = COM_Parse (sndinfo);
					ambient->volume = (float)atof (com_token);
					if (ambient->volume > 1)
						ambient->volume = 1;
					else if (ambient->volume < 0)
						ambient->volume = 0;
				} else if (!stricmp (com_token + 1, "map")) {
					// Hexen-style $MAP command
					level_info_t *info;

					sndinfo = COM_Parse (sndinfo);
					sprintf (com_token, "MAP%02d", atoi (com_token));
					info = FindLevelInfo (com_token);
					sndinfo = COM_Parse (sndinfo);
					if (info->mapname[0])
					{
						strncpy (info->music, com_token, 9); // denis - todo -string limit?
						std::transform(info->music, info->music + strlen(info->music), info->music, toupper);
					}
				} else {
					Printf (PRINT_HIGH, "Unknown SNDINFO command %s\n", com_token);
					while (*sndinfo != '\n' && *sndinfo != '\0')
						sndinfo++;
				}
			} else {
				// com_token is a logical sound mapping
				char name[MAX_SNDNAME+1];

				strncpy (name, com_token, MAX_SNDNAME);
				name[MAX_SNDNAME] = 0;
				sndinfo = COM_Parse (sndinfo);
				S_AddSound (name, com_token);
			}
		}
	}
	S_HashSounds ();

	sfx_empty = wads.CheckNumForName ("dsempty");
	sfx_noway = S_FindSoundByLump (wads.CheckNumForName ("dsnoway"));
	sfx_oof = S_FindSoundByLump (wads.CheckNumForName ("dsoof"));
}


static void SetTicker (int *tics, struct AmbientSound *ambient)
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

void A_Ambient (AActor *actor)
{
	if (!actor)
		return;

	struct AmbientSound *ambient = &Ambients[actor->args[0]];

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
			actor->Destroy ();
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

void S_ActivateAmbient (AActor *origin, int ambient)
{
	if (!origin)
		return;

	struct AmbientSound *amb = &Ambients[ambient];

	if (!(amb->type & 3) && !amb->periodmin)
	{
		int sndnum = S_FindSound(amb->sound);
		if (sndnum == 0)
			return;

		sfxinfo_t *sfx = S_sfx + sndnum;

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
	char lumpname[9];
	int i;

	lumpname[8] = 0;
	for (i = 0; i < numsfx; i++)
		if (S_sfx[i].lumpnum != -1)
		{
			strncpy (lumpname, lumpinfo[S_sfx[i].lumpnum].name, 8);
			Printf (PRINT_HIGH, "%3d. %s (%s)\n", i+1, S_sfx[i].name, lumpname);
		}
		else
			Printf (PRINT_HIGH, "%3d. %s **not present**\n", i+1, S_sfx[i].name);
}
END_COMMAND (snd_soundlist)

BEGIN_COMMAND (snd_soundlinks)
{
	int i;

	for (i = 0; i < numsfx; i++)
		if (S_sfx[i].link)
			Printf (PRINT_HIGH, "%s -> %s\n", S_sfx[i].name, S_sfx[i].link->name);
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
	int loopmus;

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
		loopmus = (atoi(argv[2]) != 0);
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
void UV_SoundAvoidPlayer (AActor *mo, byte channel, const char *name, byte attenuation)
{
	S_Sound(mo, channel, name, 1, attenuation);
}

VERSION_CONTROL (s_sound_cpp, "$Id$")

