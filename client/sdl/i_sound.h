// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 2006-2013 by The Odamex Team.
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


#ifndef __I_SOUND__
#define __I_SOUND__

#include "doomdef.h"

#include "doomstat.h"
#include "s_sound.h"



// Init at program start...
void I_InitSound();

// ... shut down and relase at program termination.
void STACK_ARGS I_ShutdownSound (void);

void I_SetSfxVolume (float volume);

//
//	SFX I/O
//

// Initialize channels
void I_SetChannels (int);

// load a sound from disk
void I_LoadSound (struct sfxinfo_struct *sfx);

// Starts a sound in a particular sound channel.
int
I_StartSound
(int			id,
 float			vol,
 int			sep,
 int			pitch,
 bool			loop);

// Stops a sound channel.
void I_StopSound(int handle);

// Called by S_*() functions
//	to see if a channel is still playing.
// Returns 0 if no longer playing, 1 if playing.
int I_SoundIsPlaying(int handle);

// Updates the volume, separation,
//	and pitch of a sound channel.
void I_UpdateSoundParams(int handle, float vol, int sep, int pitch);

#endif
