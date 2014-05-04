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
#include "m_swap.h"

#include <string>
#include <vector>

enum EDisplayType
{
	DISPLAY_WindowOnly,
	DISPLAY_FullscreenOnly,
	DISPLAY_Both
};

// forward definitions
class DCanvas;
class IWindow;
class IWindowSurface;

void I_InitHardware();
void STACK_ARGS I_ShutdownHardware();
bool I_VideoInitialized();

void I_SetVideoMode(int width, int height, int bpp, bool fullscreen, bool vsync);
void I_SetWindowSize(int width, int height);

IWindow* I_GetWindow();
IWindowSurface* I_GetPrimarySurface();
DCanvas* I_GetPrimaryCanvas();

IWindowSurface* I_GetEmulatedSurface();
void I_BlitEmulatedSurface();

IWindowSurface* I_AllocateSurface(int width, int height, int bpp);
void I_FreeSurface(IWindowSurface* surface);

int I_GetVideoWidth();
int I_GetVideoHeight();
int I_GetVideoBitDepth();

int I_GetSurfaceWidth();
int I_GetSurfaceHeight();

bool I_IsProtectedResolution(const IWindowSurface* surface = I_GetPrimarySurface());
bool I_IsProtectedResolution(int width, int height);

void I_SetPalette(const argb_t* palette);

void I_BeginUpdate();
void I_FinishUpdate();

void I_WaitVBL(int count);

void I_SetWindowCaption(const std::string& caption = "");
void I_SetWindowIcon();

std::string I_GetVideoDriverName();

EDisplayType I_DisplayType();


#ifdef __BIG_ENDIAN__
static const int ashift = 0;
static const int rshift = 8;
static const int gshift = 16;
static const int bshift = 24;
#else
static const int ashift = 24;
static const int rshift = 16;
static const int gshift = 8;
static const int bshift = 0;
#endif

static inline unsigned int APART(argb_t color)
{	return (color >> ashift) & 0xFF;	}

static inline unsigned int RPART(argb_t color)
{	return (color >> rshift) & 0xFF;	}

static inline unsigned int GPART(argb_t color)
{	return (color >> gshift) & 0xFF;	}

static inline unsigned int BPART(argb_t color)
{	return (color >> bshift) & 0xFF;	}

static inline argb_t MAKERGB(unsigned int r, unsigned int g, unsigned int b)
{	return (r << rshift) | (g << gshift) | (b << bshift);	}

static inline argb_t MAKEARGB(unsigned int a, unsigned int r, unsigned int g, unsigned int b)
{	return (a << ashift) | (r << rshift) | (g << gshift) | (b << bshift);	}


// ****************************************************************************

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

	DCanvas* getDefaultCanvas();
	DCanvas* createCanvas();
	void releaseCanvas(DCanvas* canvas);

	virtual byte* getBuffer() = 0;
	virtual const byte* getBuffer() const = 0;

	virtual void lock() { }
	virtual void unlock() { } 

	virtual int getWidth() const = 0;
	virtual int getHeight() const = 0;
	virtual int getPitch() const = 0;

	virtual int getPitchInPixels() const
	{	return getPitch() / getBytesPerPixel();	}

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

	DCanvas*			mCanvas;
};


// ============================================================================
//
// IGenericWindowSurface class interface
//
// Implementation of IWindowSurface that is useful for headless clients. The
// contents of the buffer are never used.
//
// ============================================================================

class IGenericWindowSurface : public IWindowSurface
{
public:
	IGenericWindowSurface(IWindow* window, int width, int height, int bpp);

	IGenericWindowSurface(IWindowSurface* base_surface, int width, int height);

	virtual ~IGenericWindowSurface();

	virtual byte* getBuffer()
	{	return mSurfaceBuffer;	}

	virtual const byte* getBuffer() const
	{	return mSurfaceBuffer;	}

	virtual int getWidth() const
	{	return mWidth;	}

	virtual int getHeight() const
	{	return mHeight;	}

	virtual int getPitch() const
	{	return mPitch;	}

	virtual int getBitsPerPixel() const
	{	return mBitsPerPixel;	}

	virtual void setPalette(const argb_t* palette)
	{	mPalette = palette;	}

	virtual const argb_t* getPalette() const
	{	return mPalette;	}	

private:
	// disable copy constructor and assignment operator
	IGenericWindowSurface(const IGenericWindowSurface&);
	IGenericWindowSurface& operator=(const IGenericWindowSurface&);

	byte*			mSurfaceBuffer;
	bool			mAllocatedSurfaceBuffer;

	const argb_t*	mPalette;

	int				mWidth;
	int				mHeight;
	int				mBitsPerPixel;
	int				mPitch;
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
	virtual const EDisplayType getDisplayType() const = 0;

	virtual IVideoMode getClosestMode(int width, int height);

	virtual IWindowSurface* getPrimarySurface() = 0;
	virtual const IWindowSurface* getPrimarySurface() const = 0;

	virtual int getWidth() const = 0;
	virtual int getHeight() const = 0;
	virtual int getBitsPerPixel() const = 0;

	virtual int getBytesPerPixel() const
	{	return getBitsPerPixel() / 8;	}

	virtual bool isFullScreen() const = 0;

	virtual bool setMode(int width, int height, int bpp, bool fullscreen, bool vsync) = 0;

	virtual void refresh() { }

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
	{	mPrimarySurface = new IGenericWindowSurface(this, 320, 200, 8); }

	virtual ~IDummyWindow()
	{	delete mPrimarySurface;	}

	virtual const IVideoModeList* getSupportedVideoModes() const
	{	return &mVideoModes;	}

	virtual const EDisplayType getDisplayType() const
	{	return DISPLAY_Both;	}

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

	virtual bool setMode(int width, int height, int bpp, bool fullscreen, bool vsync)
	{	return true;	}

	virtual bool isFullScreen() const
	{	return true;	}

private:
	// disable copy constructor and assignment operator
	IDummyWindow(const IDummyWindow&);
	IDummyWindow& operator=(const IDummyWindow&);

	IVideoModeList		mVideoModes;
	IWindowSurface*		mPrimarySurface;
};


// ****************************************************************************

#endif // __I_VIDEO_H__
