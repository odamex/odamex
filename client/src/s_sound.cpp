// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
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
#include "v_video.h"
#include "v_text.h"
#include "vectors.h"
#include "m_fileio.h"

#define S_WHICHEARS (consoleplayer().spectator ? displayplayer() : consoleplayer())

#define NORM_PITCH				128
#define NORM_PRIORITY				64
#define NORM_SEP				128

#define S_PITCH_PERTURB 		1
#define S_STEREO_SWING			(96<<FRACBITS)

// Distance tp origin when sounds should be maxed out.
// This should relate to movement clipping resolution
// (see BLOCKMAP handling).
// In the source code release: (160*0x10000).  Changed back to the
// Vanilla value of 200 (why was this changed?)
#define S_CLOSE_DIST		(200*0x10000)

#define S_ATTENUATOR		((S_CLIPPING_DIST-S_CLOSE_DIST)>>FRACBITS)
// choco goodness endeth

typedef struct
{
	fixed_t		*pt;		// origin of sound
	fixed_t		x,y;		// Origin if pt is NULL
	sfxinfo_t*	sfxinfo;	// sound information (if null, channel avail.)
	int 		handle;		// handle of the sound being played
	int			sound_id;
	int			entchannel;	// entity's sound channel
	int			basepriority;
	float		attenuation;
	float		volume;
	int			pitch;
	int			priority;
	BOOL		loop;
	int			timer;		// countdown until sound is destroyed
} channel_t;

// [RH] Hacks for pitch variance
int sfx_sawup, sfx_sawidl, sfx_sawful, sfx_sawhit;
int sfx_itemup, sfx_tink;

int sfx_plasma, sfx_chngun, sfx_chainguy, sfx_empty;

// joek - hack for silent bfg
int sfx_noway, sfx_oof;

// [RH] Print sound debugging info?
cvar_t noisedebug ("noise", "0", 0);

// the set of channels available
static channel_t *Channel;

// For ZDoom sound curve
static byte		*SoundCurve;

// Maximum volume of a sound effect.
// Internal default is max out of 0-15.
CVAR_FUNC_IMPL (snd_sfxvolume)
{
	S_SetSfxVolume (var);
}

// Maximum volume of Music.
CVAR_FUNC_IMPL (snd_musicvolume)
{
	S_SetMusicVolume (var);
}

// whether songs are mus_paused
static BOOL		mus_paused;

// music currently being played
static struct mus_playing_t
{
	std::string name;
	int   handle;
} mus_playing;

EXTERN_CVAR (snd_timeout)
EXTERN_CVAR (snd_channels)
EXTERN_CVAR (co_zdoomsoundcurve)
size_t			numChannels;

static int		nextcleanup;


static fixed_t P_AproxDistance2 (fixed_t *listener, fixed_t x, fixed_t y)
{
	// calculate the distance to sound origin
	//	and clip it if necessary
	if (listener)
	{
		fixed_t adx = abs (listener[0] - x);
		fixed_t ady = abs (listener[1] - y);
		// From _GG1_ p.428. Appox. eucledian distance fast.
		return adx + ady - ((adx < ady ? adx : ady)>>1);
	}
	else
		return 0;
}

static fixed_t P_AproxDistance2 (AActor *listener, fixed_t x, fixed_t y)
{
	if (listener)
		return P_AproxDistance2 (&listener->x, x, y);
	else
		return 0;
}

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

	for (i = 0;((i < numChannels) && (y < screen->height - 16)); i++, y += 8)
	{
		if (Channel[i].sfxinfo)
		{
			char temp[16];
			fixed_t *origin = Channel[i].pt;

			if (Channel[i].attenuation <= 0 && S_WHICHEARS.camera)
			{
				ox = S_WHICHEARS.camera->x;
				oy = S_WHICHEARS.camera->y;
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
			sprintf (temp, "%g", Channel[i].volume);
			screen->DrawText (color, 170, y, temp);
			sprintf (temp, "%d", Channel[i].priority);
			screen->DrawText (color, 200, y, temp);
			sprintf (temp, "%d", P_AproxDistance2 (S_WHICHEARS.camera, ox, oy) / FRACUNIT);
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
	int curvelump = W_GetNumForName ("SNDCURVE");
	SoundCurve = (byte *)W_CacheLumpNum (curvelump, PU_STATIC);

	unsigned int i;

	//Printf (PRINT_HIGH, "S_Init: default sfx volume %f\n", sfxVolume);

	// [RH] Read in sound sequences
	NumSequences = 0;
	S_ParseSndSeq ();

	S_SetSfxVolume (sfxVolume);
	S_SetMusicVolume (musicVolume);

	// Allocating the internal channels for mixing
	// (the maximum numer of sounds rendered
	// simultaneously) within zone memory.
	numChannels = snd_channels.asInt();
	Channel = (channel_t *) Z_Malloc (numChannels*sizeof(channel_t), PU_STATIC, 0);
	for (i = 0; i < numChannels; i++)
	{
		// Initialize the channel's variables
		memset(&Channel[i], 0, sizeof(Channel[i]));
		Channel[i].sound_id = -1;
		Channel[i].handle = -1;
		Channel[i].attenuation = 0.0f;
		Channel[i].volume = 0.0f;
	}
	I_SetChannels (numChannels);

	// no sounds are playing, and they are not mus_paused
	mus_paused = 0;

	// Note that sounds have not been cached (yet).
	for (int j = 1; j < numsfx; j++)
		S_sfx[j].usefulness = -1;
}

//
// Kills playing sounds
//
void S_Stop (void)
{
	unsigned int cnum;

	// kill all playing sounds at start of level
	//	(trust me - a good idea)
	for (cnum = 0; cnum < numChannels; cnum++)
		if (Channel[cnum].sfxinfo)
			S_StopChannel (cnum);

	// start new music for the level
	mus_paused = 0;

	// [RH] This is a lot simpler now.
	S_ChangeMusic (std::string(level.music, 8), true);

	nextcleanup = 15;
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

	nextcleanup = 15;
}


//
// S_getChannel :
//   If none available, return -1.  Otherwise channel #.
//
//   joek - added from choco, editied slightly
int
		S_getChannel
		( void*		origin,
		  sfxinfo_t*	sfxinfo,
		  int 		priority)
{
    // channel number to use
	int cnum = -1;
	int i;

    // Find an open channel
	for (i=0; i < (int)numChannels; i++)
	{
		if (!Channel[i].sfxinfo)	// No sfx playing here (sfxinfo == NULL)
		{
			cnum = i;
			break;
		}
	}

	// No open channels, cnum hasn't changed
	if (cnum == -1)
	{
		// Look for lower priority
		for (i=0 ; i < (int)numChannels ; i++)
			if (Channel[i].priority <= priority)
			{
				cnum = i;
				break;
			}

		// Still no channels we can use, don't bother with that sound
		if(cnum == -1)
		{
			return -1;
		}
		else
		{
	    		// Otherwise, kick out lower priority.
			S_StopChannel(cnum);
		}
	}

    // channel is decided to be cnum.
	Channel[cnum].sfxinfo = sfxinfo;
	Channel[cnum].pt = (fixed_t *) origin;
	if (Channel[cnum].pt)
	{
		Channel[cnum].x = Channel[cnum].pt[0];
		Channel[cnum].y = Channel[cnum].pt[1];
	}

	return cnum;
}

EXTERN_CVAR (co_level8soundfeature)


//
// S_AdjustZdoomSoundParams
//
// Utilizes the sndcurve lump to mimic volume and stereo separation
// calculations from ZDoom 1.22

int S_AdjustZdoomSoundParams(	AActor*	listener,
								fixed_t	x,
								fixed_t	y,
								float*	vol,
								int*	sep,
								int*	pitch)
{
	const int MAX_SND_DIST = 2025;
	
	fixed_t* listener_pt;
	if (listener)
		listener_pt = &(listener->x);
	else
		listener_pt = NULL;
		
	int dist = (int)(FIXED2FLOAT(P_AproxDistance2 (listener_pt, x, y)));
		
	if (dist >= MAX_SND_DIST)
	{
		if (co_level8soundfeature && level.levelnum == 8)
		{
			dist = MAX_SND_DIST;
		}
		else
		{
			*vol = 0.0f;	// too far away to hear
			return 0;
		}
	}
	else if (dist < 0)
	{
		dist = 0;
	}

	*vol = (SoundCurve[dist] * snd_sfxvolume) / 128.0f;
	if (dist > 0 && listener)
	{
		angle_t angle = R_PointToAngle2(listener->x, listener->y, x, y);
		if (angle > listener->angle)
			angle = angle - listener->angle;
		else
			angle = angle + (0xffffffff - listener->angle);
		angle >>= ANGLETOFINESHIFT;
		*sep = NORM_SEP - (FixedMul (S_STEREO_SWING, finesine[angle])>>FRACBITS);
	}
	else
	{
		*sep = NORM_SEP;
	}
	
	return (*vol > 0);
}

//
// Changes volume, stereo-separation, and pitch variables
//  from the norm of a sound effect to be played.
// If the sound is not audible, returns a 0.
// Otherwise, modifies parameters and returns 1.
//
// joek - from Choco Doom
//
// [SL] 2011-05-26 - Changed function parameters to accept x,y instead
// of a fixed_t* for the sound origin
int S_AdjustSoundParams(AActor*		listener,
		  				fixed_t		x,
		  				fixed_t		y,
		  				float*		vol,
		  				int*		sep,
		  				int*		pitch )
{
	fixed_t	approx_dist;
	fixed_t	adx;
	fixed_t	ady;
	angle_t	angle;

	if(!listener)
		return 0;

    // calculate the distance to sound origin
    //  and clip it if necessary
	adx = abs(listener->x - x);
	ady = abs(listener->y - y);

    // From _GG1_ p.428. Appox. eucledian distance fast.
	approx_dist = adx + ady - ((adx < ady ? adx : ady)>>1);

	// GhostlyDeath <November 16, 2008> -- ExM8 has the full volume effect
	// [Russell] - Change this to an option and remove the dependence on
	// we run doom 1 or not
	if ((multiplayer && !co_level8soundfeature) && level.levelnum != 8 && approx_dist > S_CLIPPING_DIST)
		return 0;

    // angle of source to listener
	angle = R_PointToAngle2(listener->x, listener->y, x, y);

	if (angle > listener->angle)
		angle = angle - listener->angle;
	else
		angle = angle + (0xffffffff - listener->angle);

	angle >>= ANGLETOFINESHIFT;

    // stereo separation
	*sep = 128 - (FixedMul(S_STEREO_SWING,finesine[angle])>>FRACBITS);

    // volume calculation
	if (approx_dist < S_CLOSE_DIST)
	{
		*vol = snd_sfxvolume;
		*sep = NORM_SEP;
	}
	else if (co_level8soundfeature && level.levelnum == 8)
	{
		if (approx_dist > S_CLIPPING_DIST)
			approx_dist = S_CLIPPING_DIST;

		*vol = 15+ ((snd_sfxvolume-15)
				*((S_CLIPPING_DIST - approx_dist)>>FRACBITS)) / S_ATTENUATOR;
	}
	else
	{
	// distance effect
		*vol = (snd_sfxvolume
				* ((S_CLIPPING_DIST - approx_dist)>>FRACBITS))
	    / S_ATTENUATOR;
	}

	return (*vol > 0);
}


//
// joek - choco's S_StartSoundAtVolume with some zdoom code
// a bit of a whore of a funtion but she works ok
//

static void S_StartSound (fixed_t *pt, fixed_t x, fixed_t y, int channel,
	                  int sfx_id, float volume, float attenuation, bool looping)
{

	int		rc;
	int		sep;
	int		pitch;
	int		priority, basepriority;
	sfxinfo_t*	sfx;
	int		cnum;
	int handle;

  	// check for bogus sound #
	if (sfx_id < 1 || sfx_id > numsfx)
	{
		Printf(PRINT_HIGH,"Bad sfx #: %d\n", sfx_id);
                return;
	}

	// check volume +ve
	if(volume <= 0)
		return;

	sfx = &S_sfx[sfx_id];

  	// check for bogus sound lump
	if (sfx->lumpnum < 0 || sfx->lumpnum > (int)numlumps)
	{
		// [ML] We don't have to announce it though do we?
		//Printf(PRINT_HIGH,"Bad sfx lump #: %d\n", sfx->lumpnum);
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

// Remove some duplicate sounds (mainly for plasma)
	 S_StopSoundID(sfx_id);

	if (sfx->link)
		sfx = sfx->link;

	if (!sfx->data) {
		I_LoadSound (sfx);
		if (sfx->link)
			sfx = sfx->link;
	}
	
	
	if (S_WHICHEARS.mo && attenuation != ATTN_NONE)
	{
	
  		// Check to see if it is audible, and if not, modify the params
		if (co_zdoomsoundcurve)
		{
			rc = S_AdjustZdoomSoundParams(S_WHICHEARS.mo, x, y, &volume, &sep, &pitch);
		}
		else
		{
			rc = S_AdjustSoundParams(S_WHICHEARS.mo, x, y, &volume, &sep, &pitch);
		}

		if (x == S_WHICHEARS.mo->x && y == S_WHICHEARS.mo->y)
		{
			sep = NORM_SEP;
		}
		
		if (!rc)
			return;
	}
	else
	{
		sep = NORM_SEP;
	}

	pitch = NORM_PITCH;

	if (sfx->lumpnum == sfx_empty)
	{
		basepriority = -1000;
	}
	else if (channel == CHAN_ANNOUNCERE || channel == CHAN_ANNOUNCERF)
	{
		basepriority = 300;
	}
	else if (attenuation <= ATTN_NONE)
	{
		basepriority = 200;
	}
	else
	{
		switch (channel)
		{
			case CHAN_WEAPON:
				basepriority = 100;
				break;
			case CHAN_VOICE:
				basepriority = 75;
				break;
			case CHAN_BODY:
				basepriority = 50;
				break;
			case CHAN_ITEM:
				basepriority = 25;
				break;
			default:
				basepriority = 50;
				break;
		}
		if (attenuation == ATTN_NORM)
			basepriority += 50;
	}
	priority = basepriority;

  // hacks to vary the sfx pitches
	if (sfx_id >= sfx_sawup && sfx_id <= sfx_sawhit)
	{
		pitch += 8 - (M_Random()&15);

		if (pitch<0)
			pitch = 0;
		else if (pitch>255)
			pitch = 255;
	}
	else if (sfx_id != sfx_itemup && sfx_id != sfx_tink)
	{
		pitch += 16 - (M_Random()&31);

		if (pitch<0)
			pitch = 0;
		else if (pitch>255)
			pitch = 255;
	}

	// joek - hack for silent bfg
	if(sfx_id == sfx_noway || sfx_id == sfx_oof)
		S_StopSound (pt);

	S_StopSound (pt, channel);

  	// try to find a channel
	cnum = S_getChannel(pt, sfx, priority);

  	// no channel found
	if (cnum < 0)
		return;

	handle = I_StartSound(sfx_id,
			     volume,
			     sep,
			     Channel[cnum].pitch,
			     looping);

	// I_StartSound can not find an empty channel.  Make sure this channel is clear
	if (handle < 0)
	{
		Channel[cnum].handle = -1;
		Channel[cnum].sfxinfo = NULL;
		Channel[cnum].pt = NULL;
		S_StopChannel(cnum);
		return;
	}

  // Assigns the handle to one of the channels in the
  //  mix/output buffer.
	Channel[cnum].handle = handle;

  // increase the usefulness
	if (sfx->usefulness++ < 0)
		sfx->usefulness = 1;

	Channel[cnum].sound_id = sfx_id;
	Channel[cnum].pt = pt;
	Channel[cnum].sfxinfo = sfx;
	Channel[cnum].priority = priority;
	Channel[cnum].basepriority = basepriority;
	Channel[cnum].entchannel = channel;
	Channel[cnum].attenuation = attenuation;
	Channel[cnum].volume = volume;
	Channel[cnum].x = x;
	Channel[cnum].y = y;
	Channel[cnum].loop = looping;
	Channel[cnum].timer = 0;		// used to time-out sounds that don't stop
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
	if (ent->subsector->sector->MoreFlags & SECF_SILENT)
		return;	
	S_StartSound (&ent->x, 0, 0, channel, sound_id, volume, attenuation, false);
}

void S_SoundID (fixed_t *pt, int channel, int sound_id, float volume, int attenuation)
{
	S_StartSound (pt, 0, 0, channel, sound_id, volume, attenuation, false);
}

void S_LoopedSoundID (AActor *ent, int channel, int sound_id, float volume, int attenuation)
{
	if (ent->subsector->sector->MoreFlags & SECF_SILENT)
		return;	
	S_StartSound (&ent->x, 0, 0, channel, sound_id, volume, attenuation, true);
}

void S_LoopedSoundID (fixed_t *pt, int channel, int sound_id, float volume, int attenuation)
{
	S_StartSound (pt, 0, 0, channel, sound_id, volume, attenuation, true);
}

static void S_StartNamedSound (AActor *ent, fixed_t *pt, fixed_t x, fixed_t y, int channel,
                               const char *name, float volume, float attenuation, bool looping)
{
	int sfx_id = -1;
	
	if (name == NULL ||
		(ent && ent != (AActor *)(~0) && ent->subsector->sector->MoreFlags & SECF_SILENT))
	{
		return;
	}
	
	if (*name == '*') {
		// Sexed sound
		char nametemp[128];
		const char templat[] = "player/%s/%s";
                // Hacks away! -joek
		//const char *genders[] = { "male", "female", "cyborg" };
                const char *genders[] = { "male", "male", "male" };
		player_t *player;

		sfx_id = -1;
		if ( ent != (AActor *)(~0) && (player = ent->player) ) {
			sprintf (nametemp, templat, skins[player->userinfo.skin].name, name + 1);
			sfx_id = S_FindSound (nametemp);
			if (sfx_id == -1) {
				sprintf (nametemp, templat, genders[player->userinfo.gender], name + 1);
				sfx_id = S_FindSound (nametemp);
			}
		}
		if (sfx_id == -1) {
			sprintf (nametemp, templat, "male", name + 1);
			sfx_id = S_FindSound (nametemp);
		}
	} else
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
	if(channel == CHAN_ITEM && ent != S_WHICHEARS.camera)
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

void S_StopSound (fixed_t *pt)
{
	for (unsigned int i = 0; i < numChannels; i++)
		if (Channel[i].sfxinfo && (Channel[i].pt == pt))
		{
			S_StopChannel (i);
		}
}

void S_StopSound (fixed_t *pt, int channel)
{
	for (unsigned int i = 0; i < numChannels; i++)
		if (Channel[i].sfxinfo
			&& Channel[i].pt == pt // denis - fixme - security - wouldn't this cause invalid access elsewhere, if an object was destroyed?
			&& Channel[i].entchannel == channel)
			S_StopChannel (i);
}

void S_StopSound (AActor *ent, int channel)
{
	S_StopSound (&ent->x, channel);
}

void S_StopAllChannels (void)
{
	unsigned int i;

	for (i = 0; i < numChannels; i++)
		if (Channel[i].sfxinfo)
			S_StopChannel (i);
}

//
// joek - Hexen style S_StopSoundID
// returns 1 on success (taken out a couple sounds) or 0 on fail
// Has a limit imposed like hexen one so things don't go quiet all of a sudden
// when using a plasma rifle or whatever

#define DUPLICATE_LIMIT 2

bool S_StopSoundID(int sound_id)
{
	int i;
	int duplicates = 0;

	for(i = 0; i < (int)numChannels; i++)
	{
		if(Channel[i].sound_id == sound_id)
		{
			duplicates++;

			// Limit reached so take this one out
			if(duplicates > DUPLICATE_LIMIT)
				S_StopChannel(i);
		}
	}

	// If we've taken any out, return 1
	return duplicates>DUPLICATE_LIMIT;
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
	if (mus_playing.handle && !mus_paused)
	{
		I_PauseSong(mus_playing.handle);
		mus_paused = true;
	}
}

void S_ResumeSound (void)
{
	if (mus_playing.handle && mus_paused)
	{
		I_ResumeSong (mus_playing.handle);
		mus_paused = false;
	}
}

//
// Updates music & sounds
//
// joek - from choco again
void S_UpdateSounds (void *listener_p)
{
	int		audible;
	int		cnum;
	float		volume;
	int		sep;
	int		pitch;
	sfxinfo_t*	sfx;
	channel_t*	c;

	AActor *listener = (AActor *)listener_p;

    // Clean up unused data.
    // This is currently not done for 16bit (sounds cached static).
    // DOS 8bit remains.
    /*if (gametic > nextcleanup)
	{
	for (i=1 ; i<NUMSFX ; i++)
	{
	if (S_sfx[i].usefulness < 1
	&& S_sfx[i].usefulness > -1)
	{
	if (--S_sfx[i].usefulness == -1)
	{
	Z_ChangeTag(S_sfx[i].data, PU_CACHE);
	S_sfx[i].data = 0;
}
}
}
	nextcleanup = gametic + 15;
}*/

	for (cnum=0 ; cnum < (int)numChannels ; cnum++)
	{
		c = &Channel[cnum];
		sfx = c->sfxinfo;

		// [SL] 2011-06-30 - Stop a sound if it hasn't stopped yet
		if (++c->timer > snd_timeout * TICRATE && snd_timeout > 0)
		{
			S_StopChannel(cnum);	
			continue;
		}

		if (c->sfxinfo)
		{
			if (I_SoundIsPlaying(c->handle))
			{
		// initialize parameters
				volume = snd_sfxvolume;
				pitch = NORM_PITCH;
				sep = NORM_SEP;

				if (sfx->link)
				{
					pitch = Channel[cnum].pitch;
					volume += Channel[cnum].volume;

					if (volume <= 0)
					{
						S_StopChannel(cnum);
						continue;
					}
					else if (volume > snd_sfxvolume)
					{
						volume = snd_sfxvolume;
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
					
					
					if (co_zdoomsoundcurve)
					{
						audible = S_AdjustZdoomSoundParams(	listener, 
															x, y,
															&volume,
															&sep,
															&pitch);
					}
					else
					{					
						audible = S_AdjustSoundParams(	listener, 
														x, y,
														&volume,
														&sep,
														&pitch);
					}

					if (!audible)
					{
						S_StopChannel(cnum);
					}
					else
						I_UpdateSoundParams(c->handle, volume, sep, c->pitch);
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
	int lumpnum;
	void *data = NULL;
	int len = 0;
	FILE *f;

	if (mus_playing.name == musicname)
		return;

	if (!musicname.length() || musicname[0] == 0)
	{
		// Don't choke if the map doesn't have a song attached
		S_StopMusic ();
		return;
	}

	// shutdown old music
	S_StopMusic();

	if (!(f = fopen (musicname.c_str(), "rb")))
	{
		if ((lumpnum = W_CheckNumForName (musicname.c_str())) == -1)
		{
			Printf (PRINT_HIGH, "Music lump \"%s\" not found\n", musicname.c_str());
			return;
		}

		mus_playing.handle = I_RegisterSong((char *)W_CacheLumpNum(lumpnum, PU_CACHE), W_LumpLength (lumpnum));
	}
	else
	{
		lumpnum = -1;
		len = M_FileLength (f);
		data = Malloc (len);
		fread (data, len, 1, f);
		fclose (f);

		mus_playing.handle = I_RegisterSong((char *)data, len);

		M_Free(data);
	}

	// load & register it
	mus_playing.name = musicname;

	// play it
	I_PlaySong(mus_playing.handle, looping);
}

void S_StopMusic (void)
{
	if (mus_playing.name.length())
	{
		if (mus_paused)
			I_ResumeSong(mus_playing.handle);

		I_StopSong(mus_playing.handle);
		I_UnRegisterSong(mus_playing.handle);

		mus_playing.name = "";
		mus_playing.handle = 0;
	}
}

static void S_StopChannel (unsigned int cnum)
{
	unsigned int i;
	channel_t* c;

	if(cnum > numChannels - 1 || cnum < 0)
	{
		printf("Trying to stop invalid channel %d\n", cnum);
		return;
	}

	c = &Channel[cnum];

	if (c->sfxinfo && c->handle >= 0)
	{
		// stop the sound playing
		I_StopSound (c->handle);

		// check to see
		//	if other channels are playing the sound
		for (i = 0; i < numChannels; i++)
		{
			if (cnum != i && c->sfxinfo == Channel[i].sfxinfo)
			{
				break;
			}
		}

		// degrade usefulness of sound data
		c->sfxinfo->usefulness--;

		c->sfxinfo = NULL;
		c->handle = -1;
		c->sound_id = -1;
		c->pt = NULL;
		c->priority = 0;
		c->basepriority = 0;
	}
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
	S_sfx[numsfx].usefulness = 0;
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

	int lump = W_CheckNumForName (lumpname);

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
	int lastlump, lump;
	char *sndinfo;
	char *data;

	S_ClearSoundLumps ();

	lastlump = 0;
	while ((lump = W_FindLump ("SNDINFO", &lastlump)) != -1) {
		sndinfo = (char *)W_CacheLumpNum (lump, PU_CACHE);

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
					memset (ambient, 0, sizeof(struct AmbientSound));

					sndinfo = COM_Parse (sndinfo);
					strncpy (ambient->sound, com_token, MAX_SNDNAME);
					ambient->sound[MAX_SNDNAME] = 0;
					ambient->attenuation = 0;

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

	// [RH] Hack for pitch varying
	sfx_sawup = S_FindSound ("weapons/sawup");
	sfx_sawidl = S_FindSound ("weapons/sawidle");
	sfx_sawful = S_FindSound ("weapons/sawfull");
	sfx_sawhit = S_FindSound ("weapons/sawhit");
	sfx_itemup = S_FindSound ("misc/i_pkup");
	sfx_tink = S_FindSound ("misc/chat2");

	sfx_plasma = S_FindSound ("weapons/plasmaf");
	sfx_chngun = S_FindSound ("weapons/chngun");
	sfx_chainguy = S_FindSound ("chainguy/attack");
	sfx_empty = W_CheckNumForName ("dsempty");

	sfx_noway = S_FindSoundByLump (W_CheckNumForName ("dsnoway"));
	sfx_oof = S_FindSoundByLump (W_CheckNumForName ("dsoof"));
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
}

void A_Ambient (AActor *actor)
{
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
	struct AmbientSound *amb = &Ambients[ambient];

	if (!(amb->type & 3) && !amb->periodmin)
	{
		sfxinfo_t *sfx = S_sfx + S_FindSound (amb->sound);

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

	if (argc > 2)
	{
		loopmus = atoi (argv[2]);
		S_ChangeMusic (std::string(argv[1]), loopmus);
	}
	else if (argc == 2)
	{
		S_ChangeMusic (std::string(argv[1]), 1);
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

