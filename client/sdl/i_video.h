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
#include <cstdlib>

enum EDisplayType
{
	DISPLAY_WindowOnly,
	DISPLAY_FullscreenOnly,
	DISPLAY_Both
};

// forward definitions
class IVideoMode;
class IVideoCapabilities;
class IWindow;
class IWindowSurface;
class DCanvas;

void I_InitHardware();
void STACK_ARGS I_ShutdownHardware();
bool I_VideoInitialized();

void I_SetVideoMode(int width, int height, int bpp, bool fullscreen, bool vsync);
void I_SetWindowSize(int width, int height);

const IVideoCapabilities* I_GetVideoCapabilities();
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

std::string I_GetVideoModeString(const IVideoMode* mode);
std::string I_GetVideoDriverName();

const PixelFormat* I_Get8bppPixelFormat();
const PixelFormat* I_Get32bppPixelFormat();

void I_DrawLoadingIcon();


// ****************************************************************************

// ============================================================================
//
// IVideoMode class interface
//
// ============================================================================

class IVideoMode
{
public:
	IVideoMode(uint16_t width, uint16_t height, uint8_t bpp, bool fullscreen) :
		mWidth(width), mHeight(height), mBitsPerPixel(bpp), mFullScreen(fullscreen)
	{ }

	uint16_t getWidth() const
	{	return mWidth;	}

	uint16_t getHeight() const
	{	return mHeight;	}

	uint8_t getBitsPerPixel() const
	{	return mBitsPerPixel;	}

	bool isFullScreen() const
	{	return mFullScreen;	}

	bool operator==(const IVideoMode& other) const
	{
		return mWidth == other.mWidth && mHeight == other.mHeight &&
			mBitsPerPixel == other.mBitsPerPixel && mFullScreen == other.mFullScreen;
	}

	bool operator!=(const IVideoMode& other) const
	{
		return !(operator==(other));
	}

	bool operator<(const IVideoMode& other) const
	{
		if (mWidth != other.mWidth)
			return mWidth < other.mWidth;
		if (mHeight != other.mHeight)
			return mHeight < other.mHeight;
		if (mBitsPerPixel != other.mBitsPerPixel)
			return mBitsPerPixel < other.mBitsPerPixel;
		if (mFullScreen != other.mFullScreen)
			return (int)mFullScreen < (int)other.mFullScreen;
		return false;
	}

	bool operator>(const IVideoMode& other) const
	{
		if (mWidth != other.mWidth)
			return mWidth > other.mWidth;
		if (mHeight != other.mHeight)
			return mHeight > other.mHeight;
		if (mBitsPerPixel != other.mBitsPerPixel)
			return mBitsPerPixel > other.mBitsPerPixel;
		if (mFullScreen != other.mFullScreen)
			return (int)mFullScreen > (int)other.mFullScreen;
		return false;
	}

	bool operator<=(const IVideoMode& other) const
	{
		return operator<(other) || operator==(other);
	}

	bool operator>=(const IVideoMode& other) const
	{
		return operator>(other) || operator==(other);
	}

	bool isValid() const
	{
		return mWidth > 0 && mHeight > 0 && (mBitsPerPixel == 8 || mBitsPerPixel == 32);
	}

	bool isWideScreen() const
	{
		if ((mWidth == 320 && mHeight == 200) || (mWidth == 640 && mHeight == 400))
			return false;

		// consider the mode widescreen if it's width-to-height ratio is
		// closer to 16:10 than it is to 4:3
		return abs(15 * (int)mWidth - 20 * (int)mHeight) > abs(15 * (int)mWidth - 24 * (int)mHeight);
	}

	float getAspectRatio() const
	{
		return float(mWidth) / float(mHeight);
	}

	float getPixelAspectRatio() const
	{
		// assume that all widescreen modes have square pixels
		if (isWideScreen())
			return 1.0f;

		return mWidth * 0.75f / mHeight;
	}

private:
	uint16_t	mWidth;
	uint16_t	mHeight;
	uint8_t		mBitsPerPixel;
	bool		mFullScreen;
};

typedef std::vector<IVideoMode> IVideoModeList;


// ****************************************************************************

// ============================================================================
//
// IVideoCapabilities abstract base class interface
//
// Defines an interface for querying video capabilities. This includes listing
// availible video modes and supported operations.
//
// ============================================================================

class IVideoCapabilities
{
public:
	virtual ~IVideoCapabilities() { }

	virtual const IVideoModeList* getSupportedVideoModes() const = 0;

	virtual bool supportsFullScreen() const
	{	return getDisplayType() == DISPLAY_FullscreenOnly || getDisplayType() == DISPLAY_Both;	}

	virtual bool supportsWindowed() const
	{	return getDisplayType() == DISPLAY_WindowOnly || getDisplayType() == DISPLAY_Both;	}

	virtual const EDisplayType getDisplayType() const = 0;

	virtual bool supports8bpp() const
	{
		const IVideoModeList* modelist = getSupportedVideoModes();
		for (IVideoModeList::const_iterator it = modelist->begin(); it != modelist->end(); ++it)
		{
			if (it->getBitsPerPixel() == 8)
				return true;
		}
		return false;
	}

	virtual bool supports32bpp() const
	{
		const IVideoModeList* modelist = getSupportedVideoModes();
		for (IVideoModeList::const_iterator it = modelist->begin(); it != modelist->end(); ++it)
		{
			if (it->getBitsPerPixel() == 32)
				return true;
		}
		return false;
	}

	virtual const IVideoMode* getNativeMode() const = 0;
};


// ============================================================================
//
// IDummyVideoCapabilities class interface
//
// For use with headless clients.
//
// ============================================================================

class IDummyVideoCapabilities : public IVideoCapabilities
{
public:
	IDummyVideoCapabilities() : IVideoCapabilities(), mVideoMode(320, 200, 8, false)
	{	mModeList.push_back(mVideoMode);	}

	virtual ~IDummyVideoCapabilities() { }

	virtual const IVideoModeList* getSupportedVideoModes() const
	{	return &mModeList;	}

	virtual const EDisplayType getDisplayType() const
	{	return DISPLAY_WindowOnly;	}

	virtual const IVideoMode* getNativeMode() const
	{	return &mVideoMode;	}

private:
	IVideoModeList		mModeList;
	IVideoMode			mVideoMode;
};


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
		return mSurfaceBuffer;
	}

	inline const uint8_t* getBuffer() const
	{
		return mSurfaceBuffer;
	}

	inline uint8_t* getBuffer(uint16_t x, uint16_t y)
	{
		return mSurfaceBuffer + int(y) * getPitch() + int(x) * getBytesPerPixel();
	}

	inline const uint8_t* getBuffer(uint16_t x, uint16_t y) const
	{
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

	void setPalette(const argb_t* palette)
	{	mPalette = palette;	}

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

	const argb_t*		mPalette;

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

	virtual const IVideoMode* getVideoMode() const = 0;

	virtual bool isFullScreen() const
	{	return getVideoMode()->isFullScreen();	}

	virtual bool usingVSync() const
	{	return false;	}

	virtual bool setMode(uint16_t width, uint16_t height, uint8_t bpp, bool fullscreen, bool vsync) = 0;

	virtual void lockSurface() { }
	virtual void unlockSurface() { }

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
	IDummyWindow() : IWindow(), mPrimarySurface(NULL), mVideoMode(320, 200, 8, true)
	{ }

	virtual ~IDummyWindow()
	{	delete mPrimarySurface;	}

	virtual IWindowSurface* getPrimarySurface()
	{	return mPrimarySurface;	}

	virtual const IWindowSurface* getPrimarySurface() const
	{	return mPrimarySurface;	}

	virtual const IVideoMode* getVideoMode() const
	{	return &mVideoMode;	}

	virtual bool setMode(uint16_t width, uint16_t height, uint8_t bpp, bool fullscreen, bool vsync)
	{
		if (mPrimarySurface == NULL)
		{
			// ignore the requested mode and setup the hardcoded mode
			width = mVideoMode.getWidth();
			height = mVideoMode.getHeight();
			bpp = mVideoMode.getBitsPerPixel();
			mPrimarySurface = I_AllocateSurface(width, height, bpp);
		}
		return mPrimarySurface != NULL;
	}

	virtual bool isFullScreen() const
	{	return mVideoMode.isFullScreen();	}


	virtual std::string getVideoDriverName() const
	{
		static const std::string driver_name("headless");
		return driver_name;
	}

private:
	// disable copy constructor and assignment operator
	IDummyWindow(const IDummyWindow&);
	IDummyWindow& operator=(const IDummyWindow&);

	IWindowSurface*		mPrimarySurface;

	IVideoMode			mVideoMode;
};


// ****************************************************************************

// ============================================================================
//
// IVideoSubsystem abstract base class interface
//
// Provides intialization and shutdown mechanics for the video subsystem.
// This is really an abstract factory pattern as it instantiates a family
// of concrete types.
//
// ============================================================================

class IVideoSubsystem
{
public:
	virtual ~IVideoSubsystem() { }

	virtual const IVideoCapabilities* getVideoCapabilities() const = 0;

	virtual IWindow* getWindow() = 0;
	virtual const IWindow* getWindow() const = 0;
};


// ============================================================================
//
// IDummyVideoSubsystem class interface
//
// Video subsystem for headless clients.
//
// ============================================================================

class IDummyVideoSubsystem : public IVideoSubsystem
{
public:
	IDummyVideoSubsystem() : IVideoSubsystem()
	{
		mVideoCapabilities = new IDummyVideoCapabilities();
		mWindow = new IDummyWindow();
	}

	virtual ~IDummyVideoSubsystem()
	{
		delete mWindow;
		delete mVideoCapabilities;
	}

	virtual const IVideoCapabilities* getVideoCapabilities() const
	{	return mVideoCapabilities;	}

	virtual IWindow* getWindow()
	{	return mWindow;	}

	virtual const IWindow* getWindow() const
	{	return mWindow;	}
	
private:
	const IVideoCapabilities*		mVideoCapabilities;

	IWindow*						mWindow;
};
	

#endif // __I_VIDEO_H__
