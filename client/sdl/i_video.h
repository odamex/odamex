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

#include <string>
#include <vector>

class DCanvas;
class IWindow;

// [RH] True if the display is not in a window
extern BOOL Fullscreen;

void I_InitHardware ();
void STACK_ARGS I_ShutdownHardware ();

// [RH] M_ScreenShot now accepts a filename parameter.
//		Pass a NULL to get the original behavior.
void I_ScreenShot(std::string filename);

IWindow* I_GetWindow();

int I_GetVideoWidth();
int I_GetVideoHeight();
int I_GetVideoBitDepth();

byte* I_GetFrameBuffer();
int I_GetSurfaceWidth();
int I_GetSurfaceHeight();
void I_SetWindowSize(int width, int height);
void I_SetSurfaceSize(int width, int height);

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


// ****************************************************************************

// forward declarations
class IWindow;

// ============================================================================
//
// IWindowSurface abstract base class interface
//
// Wraps the raw device surface and provides methods to access the raw surface
// buffer.
//
// ============================================================================

class IWindowSurface
{
public:
	IWindowSurface(IWindow* window);
	virtual ~IWindowSurface();

	IWindow* getWindow()
	{	return mWindow;	}

	const IWindow* getWindow() const
	{	return mWindow;	}

	DCanvas* createCanvas();
	void releaseCanvas(DCanvas* canvas);

	virtual byte* getBuffer() = 0;
	virtual const byte* getBuffer() const = 0;

	virtual void lock() { }
	virtual void unlock() { } 

	virtual int getWidth() const = 0;
	virtual int getHeight() const = 0;
	virtual int getPitch() const = 0;

	virtual int getBitsPerPixel() const = 0;

	virtual int getBytesPerPixel() const
	{	return getBitsPerPixel() / 8;	}

	virtual void setPalette(const argb_t* palette) = 0;
	virtual const argb_t* getPalette() const = 0;

	virtual void blit(const IWindowSurface* source, int srcx, int srcy, int srcw, int srch,
			int destx, int desty, int destw, int desth);

private:
	IWindow*			mWindow;

	// Storage for all DCanvas objects allocated by this surface
	typedef std::vector<DCanvas*> DCanvasCollection;
	DCanvasCollection	mCanvasStore;
};


// ============================================================================
//
// IDummyWindowSurface class interface
//
// Implementation of IWindowSurface that is useful for headless clients. The
// contents of the buffer are never used.
//
// ============================================================================

class IDummyWindowSurface : public IWindowSurface
{
public:
	IDummyWindowSurface(IWindow* window);
	virtual ~IDummyWindowSurface();

	virtual byte* getBuffer()
	{	return mSurfaceBuffer;	}

	virtual const byte* getBuffer() const
	{	return mSurfaceBuffer;	}

	virtual int getWidth() const
	{	return 320;	}

	virtual int getHeight() const
	{	return 240;	}

	virtual int getPitch() const
	{	return 320;	}

	virtual int getBitsPerPixel() const
	{	return 8;	}

	virtual void setPalette(const argb_t* palette)
	{ }

	virtual const argb_t* getPalette() const
	{	return mPalette;	}	

	virtual void blit(const IWindowSurface* source, int srcx, int srcy, int srcw, int srch,
			int destx, int desty, int destw, int desth)
	{ }

private:
	byte*			mSurfaceBuffer;
	argb_t			mPalette[256];
};


// ****************************************************************************

// ============================================================================
//
// IVideoMode class interface
//
// ============================================================================

class IVideoMode
{
public:
	IVideoMode(int width, int height) :
		mWidth(width), mHeight(height)
	{ }

	int getWidth() const
	{	return mWidth;	}

	int getHeight() const
	{	return mHeight;	}

	bool operator<(const IVideoMode& other) const
	{
		if (mWidth != other.mWidth)
			return mWidth < other.mWidth;
		if (mHeight != other.mHeight)
			return mHeight < other.mHeight;
		return false;
	}

	bool operator>(const IVideoMode& other) const
	{
		if (mWidth != other.mWidth)
			return mWidth > other.mWidth;
		if (mHeight != other.mHeight)
			return mHeight > other.mHeight;
		return false;
	}

	bool operator==(const IVideoMode& other) const
	{
		return mWidth == other.mWidth && mHeight == other.mHeight;
	}
	
private:
	int mWidth;
	int mHeight;
};

typedef std::vector<IVideoMode> IVideoModeList;


// ****************************************************************************

// ============================================================================
//
// IWindow abstract base class interface
//
// Defines an interface for video windows (including both windowed and
// full-screen modes).
// ============================================================================

class IWindow
{
public:
	virtual ~IWindow()
	{ }

	virtual const IVideoModeList* getSupportedVideoModes() const = 0;

	virtual IWindowSurface* getPrimarySurface() = 0;
	virtual const IWindowSurface* getPrimarySurface() const = 0;

	virtual int getWidth() const = 0;

	virtual int getHeight() const = 0;

	virtual int getBitsPerPixel() const = 0;

	virtual int getBytesPerPixel() const
	{	return getBitsPerPixel() / 8;	}

	virtual void setWindowed() = 0;

	virtual void setFullScreen() = 0;

	virtual bool isFullScreen() const = 0;

	virtual void refresh() { }

	virtual void resize(int width, int height) = 0;

	virtual void setWindowTitle(const std::string& caption = "") { }

	virtual std::string getVideoDriverName() const
	{	return "";	}

	virtual void setPalette(const argb_t* palette)
	{	getPrimarySurface()->setPalette(palette);	}

	virtual const argb_t* getPalette() const
	{	return getPrimarySurface()->getPalette();	}	
};


// ============================================================================
//
// IDummyWindow abstract base class interface
//
// denis - here is a blank implementation of IWindow that allows the client
// to run without actual video output (e.g. script-controlled demo testing)
//
// ============================================================================

class IDummyWindow : public IWindow
{
public:
	IDummyWindow() : IWindow()
	{	mPrimarySurface = new IDummyWindowSurface(this); }

	virtual ~IDummyWindow()
	{	delete mPrimarySurface;	}

	virtual const IVideoModeList* getSupportedVideoModes() const
	{	static IVideoModeList videomodes; return &videomodes;	}

	virtual IWindowSurface* getPrimarySurface()
	{	return mPrimarySurface;	}

	virtual const IWindowSurface* getPrimarySurface() const
	{	return mPrimarySurface;	}

	virtual int getWidth() const
	{	return 0;	}

	virtual int getHeight() const
	{	return 0;	}

	virtual int getBitsPerPixel() const
	{	return 8;	}

	virtual int getBytesPerPixel() const
	{	return 1;	}

	virtual void setWindowed() { }

	virtual void setFullScreen() { }

	virtual bool isFullScreen() const
	{	return true;	}

	virtual void resize(int width, int height) { }

private:
	IVideoModeList		mVideoModes;
	IWindowSurface*		mPrimarySurface;
};


#endif // __I_VIDEO_H__
