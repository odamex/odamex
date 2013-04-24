// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
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
// DESCRIPTION:
//	SDL hardware access for Video Rendering (?)
//
//-----------------------------------------------------------------------------


#ifndef __HARDWARE_H__
#define __HARDWARE_H__

#include "i_video.h"

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

	virtual bool SetMode (int width, int height, int bits, bool fs);
	virtual void SetPalette (DWORD *palette);
	
	/* 12/3/06: HACK - Add SetOldPalette to accomodate classic redscreen - ML*/
	virtual void SetOldPalette (byte *doompalette);
		
	virtual void UpdateScreen (DCanvas *canvas);
	virtual void ReadScreen (byte *block);

	virtual int GetModeCount ();
	virtual void StartModeIterator (int bits);
	virtual bool NextMode (int *width, int *height);

	virtual DCanvas *AllocateSurface (int width, int height, int bits, bool primary = false);
	virtual void ReleaseSurface (DCanvas *scrn);
	virtual void LockSurface (DCanvas *scrn);
	virtual void UnlockSurface (DCanvas *scrn);
	virtual bool Blit (DCanvas *src, int sx, int sy, int sw, int sh,
					   DCanvas *dst, int dx, int dy, int dw, int dh);
};

class IInputDevice
{
 public:
	virtual ~IInputDevice () {}
	virtual void ProcessInput (bool parm) = 0;
};

class IKeyboard : public IInputDevice
{
 public:
	virtual void ProcessInput (bool consoleOpen) = 0;
	virtual void SetKeypadRemapping (bool remap) = 0;
	virtual void GetKeyRepeats (int &delay, int &rate)
		{
			delay = (250*TICRATE)/1000;
			rate = TICRATE / 15;
		}
};

class IMouse : public IInputDevice
{
 public:
	virtual void SetGrabbed (bool grabbed) = 0;
	virtual void ProcessInput (bool active) = 0;
};

class IJoystick : public IInputDevice
{
 public:
	enum EJoyProp
	{
		JOYPROP_SpeedMultiplier,
		JOYPROP_XSensitivity,
		JOYPROP_YSensitivity,
		JOYPROP_XThreshold,
		JOYPROP_YThreshold
	};
	virtual void SetProperty (EJoyProp prop, float val) = 0;
};

void I_InitHardware ();
void STACK_ARGS I_ShutdownHardware ();

// [RH] M_ScreenShot now accepts a filename parameter.
//		Pass a NULL to get the original behavior.
void I_ScreenShot(std::string filename);

#endif	// __HARDWARE_H__
