// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 2006-2014 by The Odamex Team.
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

class DCanvas;

// [RH] True if the display is not in a window
extern BOOL Fullscreen;

void I_InitHardware ();
void STACK_ARGS I_ShutdownHardware ();

// [RH] M_ScreenShot now accepts a filename parameter.
//		Pass a NULL to get the original behavior.
void I_ScreenShot(std::string filename);

int I_GetVideoWidth();
int I_GetVideoHeight();
int I_GetVideoBitDepth();

// [RH] Set the display mode
bool I_SetMode (int &width, int &height, int &bits);

// Returns true if the Video object has been set up and not yet destroyed
bool I_HardwareInitialized();

void I_SetPalette(argb_t* palette);
void I_SetOldPalette(palindex_t* doompalette);

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

DCanvas* I_AllocateScreen(int width, int height, int bits, bool primary = false);
void I_FreeScreen(DCanvas* canvas);

void I_LockScreen(DCanvas* canvas);
void I_UnlockScreen(DCanvas* canvas);
void I_Blit(DCanvas* src, int srcx, int srcy, int srcwidth, int srcheight,
			DCanvas* dest, int destx, int desty, int destwidth, int destheight);

enum EDisplayType
{
	DISPLAY_WindowOnly,
	DISPLAY_FullscreenOnly,
	DISPLAY_Both
};

EDisplayType I_DisplayType ();



class IVideo
{
 public:
	virtual ~IVideo () {}

	virtual std::string GetVideoDriverName();

	virtual EDisplayType GetDisplayType ();
	virtual bool FullscreenChanged (bool fs);
	virtual void SetWindowedScale (float scale);
	virtual bool CanBlit ();

	virtual bool SetOverscan (float scale);

	virtual int GetWidth() const;
	virtual int GetHeight() const;
	virtual int GetBitDepth() const;

	virtual bool SetMode (int width, int height, int bits, bool fs);
	virtual void SetPalette (argb_t *palette);
	
	/* 12/3/06: HACK - Add SetOldPalette to accomodate classic redscreen - ML*/
	virtual void SetOldPalette (byte *doompalette);
		
	virtual void UpdateScreen(DCanvas* canvas);
	virtual void ReadScreen (byte *block);

	virtual int GetModeCount ();
	virtual void StartModeIterator ();
	virtual bool NextMode (int *width, int *height);

	virtual DCanvas* AllocateSurface(int width, int height, int bits, bool primary = false);
	virtual void ReleaseSurface(DCanvas* scrn);
	virtual void LockSurface(DCanvas* scrn);
	virtual void UnlockSurface(DCanvas* scrn);
	virtual bool Blit(DCanvas* src, int sx, int sy, int sw, int sh,
					DCanvas* dst, int dx, int dy, int dw, int dh);
};


#endif // __I_VIDEO_H__
