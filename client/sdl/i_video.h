// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 2006-2012 by The Odamex Team.
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
//	System specific interface stuff.
//	[Odamex] Fitted to work with SDL
//
//-----------------------------------------------------------------------------


#ifndef __I_VIDEO_H__
#define __I_VIDEO_H__

#include "doomtype.h"
#include "v_video.h"

// [RH] True if the display is not in a window
extern BOOL Fullscreen;


// [RH] Set the display mode
BOOL I_SetMode (int &width, int &height, int &bits);

// Returns true if the Video object has been set up and not yet destroyed
bool I_HardwareInitialized();

// Takes full 8 bit values.
void I_SetPalette (argb_t *palette);

/*
    12/3/06: Old Timey Palette method, used for red screen only as of
             this writing.  It accepts a byte instead of a DWORD.  It
             is restricted only to the red screen because of that.
*/
void I_SetOldPalette (byte *doompalette);

void I_BeginUpdate (void);		// [RH] Locks screen[0]
void I_FinishUpdate (void);
void I_FinishUpdateNoBlit (void);
void I_TempUpdate (void);

// Wait for vertical retrace or pause a bit.
void I_WaitVBL(int count);

void I_ReadScreen (byte *scr);

void I_BeginRead (void);
void I_EndRead (void);

void I_SetWindowCaption(const std::string& caption);
void I_SetWindowCaption(void);
void I_SetWindowIcon(void);

bool I_CheckResolution (int width, int height);
void I_ClosestResolution (int *width, int *height);
//bool I_SetResolution (int width, int height, int bpp);

bool I_CheckVideoDriver (const char *name);

bool I_SetOverscan (float scale);

void I_StartModeIterator ();
bool I_NextMode (int *width, int *height);

DCanvas *I_AllocateScreen (int width, int height, int bits, bool primary = false);
void I_FreeScreen (DCanvas *canvas);

void I_LockScreen (DCanvas *canvas);
void I_UnlockScreen (DCanvas *canvas);
void I_Blit (DCanvas *src, int srcx, int srcy, int srcwidth, int srcheight,
			 DCanvas *dest, int destx, int desty, int destwidth, int destheight);

enum EDisplayType
{
	DISPLAY_WindowOnly,
	DISPLAY_FullscreenOnly,
	DISPLAY_Both
};

EDisplayType I_DisplayType ();


#endif // __I_VIDEO_H__
