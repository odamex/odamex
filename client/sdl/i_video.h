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
#include "v_pixelformat.h"

#include <string>
#include <vector>
#include <cassert>

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

const PixelFormat* I_GetDefaultPixelFormat(int bpp);

void I_DrawLoadingIcon();


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
	IWindowSurface();
	IWindowSurface(uint16_t width, uint16_t height, const PixelFormat* format,
					void* buffer = NULL, uint16_t pitch = 0);
	IWindowSurface(IWindowSurface* base_surface, uint16_t width, uint16_t height);

	~IWindowSurface();

	DCanvas* getDefaultCanvas();
	DCanvas* createCanvas();
	void releaseCanvas(DCanvas* canvas);

	inline uint8_t* getBuffer()
	{
		#ifdef DEBUG
		if (!mLocks)
			return NULL;
		#endif
		return mSurfaceBuffer;
	}

	inline const uint8_t* getBuffer() const
	{
		#ifdef DEBUG
		if (!mLocks)
			return NULL;
		#endif
		return mSurfaceBuffer;
	}

	inline uint8_t* getBuffer(uint16_t x, uint16_t y)
	{
		#ifdef DEBUG
		if (!mLocks)
			return NULL;
		#endif
		return mSurfaceBuffer + int(y) * getPitch() + int(x) * getBytesPerPixel();
	}

	inline const uint8_t* getBuffer(uint16_t x, uint16_t y) const
	{
		#ifdef DEBUG
		if (!mLocks)
			return NULL;
		#endif
		return mSurfaceBuffer + int(y) * getPitch() + int(x) * getBytesPerPixel();
	}

	inline uint16_t getWidth() const
	{	return mWidth;	}

	inline uint16_t getHeight() const
	{	return mHeight;	}

	inline uint16_t getPitch() const
	{	return mPitch;	}

	inline uint16_t getPitchInPixels() const
	{	return mPitchInPixels;	}

	inline uint8_t getBitsPerPixel() const
	{	return mPixelFormat.getBitsPerPixel();	}

	inline uint8_t getBytesPerPixel() const
	{	return mPixelFormat.getBytesPerPixel();	}

	inline const PixelFormat* getPixelFormat() const
	{	return &mPixelFormat;	}

	void setPalette(const argb_t* palette_colors)
	{	memcpy(mPalette, palette_colors, 256 * sizeof(*mPalette));	}

	const argb_t* getPalette() const
	{	return mPalette;	}

	void blit(const IWindowSurface* source, int srcx, int srcy, int srcw, int srch,
			int destx, int desty, int destw, int desth);

	void clear();

	inline void lock()
	{
		mLocks++;
		assert(mLocks >= 1 && mLocks < 100);
	}

	inline void unlock()
	{
		mLocks--;
		assert(mLocks >= 0 && mLocks < 100);
	}

private:
	// disable copy constructor and assignment operator
	IWindowSurface(const IWindowSurface&);
	IWindowSurface& operator=(const IWindowSurface&);

	// Storage for all DCanvas objects allocated by this surface
	typedef std::vector<DCanvas*> DCanvasCollection;
	DCanvasCollection	mCanvasStore;

	DCanvas*			mCanvas;

	uint8_t*			mSurfaceBuffer;
	bool				mOwnsSurfaceBuffer;

	argb_t				mPalette[256];

	PixelFormat			mPixelFormat;

	uint16_t			mWidth;
	uint16_t			mHeight;

	uint16_t			mPitch;
	uint16_t			mPitchInPixels;		// mPitch / mPixelFormat.getBytesPerPixel()

	int16_t				mLocks;
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

	virtual uint16_t getWidth() const
	{	return getPrimarySurface()->getWidth();	}

	virtual uint16_t getHeight() const
	{	return getPrimarySurface()->getHeight();	}

	virtual uint8_t getBitsPerPixel() const
	{	return getPrimarySurface()->getBitsPerPixel();	}

	virtual int getBytesPerPixel() const
	{	return getPrimarySurface()->getBytesPerPixel();	}

	virtual bool isFullScreen() const = 0;

	virtual bool setMode(uint16_t width, uint16_t height, uint8_t bpp, bool fullscreen, bool vsync) = 0;

	virtual void lockSurface()
	{	getPrimarySurface()->lock();	}

	virtual void unlockSurface()
	{	getPrimarySurface()->unlock();	}

	virtual void refresh() { }

	virtual void setWindowTitle(const std::string& caption = "") { }
	virtual void setWindowIcon() { }

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
	{
		mPrimarySurface = I_AllocateSurface(320, 200, 8);
	}

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

	virtual bool setMode(uint16_t width, uint16_t height, uint8_t bpp, bool fullscreen, bool vsync)
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


#endif // __I_VIDEO_H__
