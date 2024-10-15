// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 2006-2020 by The Odamex Team.
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

#pragma once

#include "v_pixelformat.h"

#include <cassert>
#include <cstdlib>

enum EDisplayType
{
	DISPLAY_WindowOnly,
	DISPLAY_FullscreenOnly,
	DISPLAY_Both
};

enum EWindowMode
{
	WINDOW_Windowed = 0,
	WINDOW_Fullscreen = 1,
	WINDOW_DesktopFullscreen = 2,
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

void I_SetVideoMode(const IVideoMode& video_mode);

const IVideoCapabilities* I_GetVideoCapabilities();
int I_GetMonitorCount();
IWindow* I_GetWindow();
IWindowSurface* I_GetPrimarySurface();
DCanvas* I_GetPrimaryCanvas();

IWindowSurface* I_GetEmulatedSurface();
void I_BlitEmulatedSurface();

IWindowSurface* I_AllocateSurface(int width, int height, int bpp);
void I_FreeSurface(IWindowSurface* &surface);

int I_GetVideoWidth();
int I_GetVideoHeight();
int I_GetVideoBitDepth();

int I_GetSurfaceWidth();
int I_GetSurfaceHeight();

bool I_IsProtectedResolution(const IWindowSurface* surface = I_GetPrimarySurface());
bool I_IsProtectedResolution(int width, int height);
bool I_IsWideResolution(int width, int height);
bool I_IsWideResolution(const IWindowSurface* surface = I_GetPrimarySurface());

void I_SetPalette(const argb_t* palette);

void I_BeginUpdate();
void I_FinishUpdate();

void I_WaitVBL(int count);

void I_SetWindowCaption(const std::string& caption = "");
void I_SetWindowIcon();

std::string I_GetVideoModeString(const IVideoMode& mode);
std::string I_GetVideoDriverName();

const PixelFormat* I_Get8bppPixelFormat();
const PixelFormat* I_Get32bppPixelFormat();

void I_DrawLoadingIcon();

int I_GetAspectCorrectWidth(int surface_height, int asset_height, int asset_width);


// ****************************************************************************

// ============================================================================
//
// IVideoMode class interface
//
// ============================================================================

class IVideoMode
{
public:
	IVideoMode(uint16_t _width, uint16_t _height, uint8_t _bpp, EWindowMode _window_mode,
				bool _vsync = false, std::string _stretch_mode = "") :
		width(_width), height(_height), bpp(_bpp), window_mode(_window_mode),
		vsync(_vsync), stretch_mode(_stretch_mode)
	{ }

	bool isFullScreen() const
	{	return window_mode != WINDOW_Windowed;	}

	bool operator==(const IVideoMode& other) const
	{
		return width == other.width && height == other.height &&
			bpp == other.bpp && 
			window_mode == other.window_mode &&
			vsync == other.vsync &&
			stretch_mode == other.stretch_mode;
	}

	bool operator!=(const IVideoMode& other) const
	{
		return !(operator==(other));
	}

	bool operator<(const IVideoMode& other) const
	{
		if (width != other.width)
			return width < other.width;
		if (height != other.height)
			return height < other.height;
		if (bpp != other.bpp)
			return bpp < other.bpp;
		if (window_mode != other.window_mode)
			return (int)window_mode < (int)other.window_mode;
		if (vsync != other.vsync)
			return (int)vsync < (int)other.vsync;
		if (stretch_mode != other.stretch_mode)
			return stretch_mode < other.stretch_mode;
		return false;
	}

	bool operator>(const IVideoMode& other) const
	{
		return !operator==(other) && !operator<(other);
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
		return width > 0 && height > 0 && (bpp == 8 || bpp == 32);
	}

	bool isWideScreen() const
	{
		return I_IsWideResolution(width, height);
	}

	double getAspectRatio() const
	{
		return double(width) / double(height);
	}

	double getPixelAspectRatio() const
	{
		// assume that all widescreen modes have square pixels
		if (isWideScreen())
			return 1.0;

		return double(width) * 0.75 / double(height);
	}

	uint16_t	width;
	uint16_t	height;
	uint8_t		bpp;
	EWindowMode	window_mode;
	bool		vsync;
	std::string	stretch_mode;
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

	virtual EDisplayType getDisplayType() const = 0;

	virtual bool supports8bpp() const
	{
		const IVideoModeList* modelist = getSupportedVideoModes();
		for (IVideoModeList::const_iterator it = modelist->begin(); it != modelist->end(); ++it)
		{
			if (it->bpp == 8)
				return true;
		}
		return false;
	}

	virtual bool supports32bpp() const
	{
		const IVideoModeList* modelist = getSupportedVideoModes();
		for (IVideoModeList::const_iterator it = modelist->begin(); it != modelist->end(); ++it)
		{
			if (it->bpp == 32)
				return true;
		}
		return false;
	}

	virtual const IVideoMode& getNativeMode() const = 0;
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
	IDummyVideoCapabilities() : IVideoCapabilities(), mVideoMode(320, 200, 8, WINDOW_Windowed)
	{	mModeList.push_back(mVideoMode);	}

	virtual ~IDummyVideoCapabilities() { }

	virtual const IVideoModeList* getSupportedVideoModes() const
	{	return &mModeList;	}

	virtual EDisplayType getDisplayType() const
	{	return DISPLAY_WindowOnly;	}

	virtual const IVideoMode& getNativeMode() const
	{	return mVideoMode;	}

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

	inline const uint8_t* getBuffer() const
	{
		return mSurfaceBuffer;
	}

	inline uint8_t* getBuffer()
	{
		return const_cast<uint8_t*>(static_cast<const IWindowSurface&>(*this).getBuffer());
	}

	inline const uint8_t* getBuffer(uint16_t x, uint16_t y) const
	{
		return mSurfaceBuffer + int(y) * getPitch() + int(x) * getBytesPerPixel();
	}

	inline uint8_t* getBuffer(uint16_t x, uint16_t y)
	{
		return const_cast<uint8_t*>(static_cast<const IWindowSurface&>(*this).getBuffer(x, y));
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

	void blitcrop(const IWindowSurface* source, int srcx, int srcy, int srcw, int srch,
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


// ============================================================================
//
// IWindowSurfaceManager
//
// Helper class for IWindow to encapsulate the creation of a IWindowSurface
// primary surface and to assist in using it to refresh the window.
//
// ============================================================================

class IWindowSurfaceManager
{
public:
	virtual ~IWindowSurfaceManager() { }

	virtual const IWindowSurface* getWindowSurface() const = 0;

	virtual IWindowSurface* getWindowSurface()
	{
		return const_cast<IWindowSurface*>(static_cast<const IWindowSurfaceManager&>(*this).getWindowSurface());
	}

	virtual void lockSurface() { }
	virtual void unlockSurface() { }

	virtual void startRefresh() { }
	virtual void finishRefresh() { }
};


// ============================================================================
//
// IDummyWindowSurfaceManager interface & implementation
//
// Blank implementation of IWindowSurfaceManager
//
// ============================================================================

class IDummyWindowSurfaceManager : public IWindowSurfaceManager
{
public:
	IDummyWindowSurfaceManager()
	{	mSurface = I_AllocateSurface(320, 200, 8);	}

	virtual ~IDummyWindowSurfaceManager()
	{	delete mSurface;	}

	virtual const IWindowSurface* getWindowSurface() const
	{	return mSurface;	}

	virtual void lockSurface()
	{	mSurface->lock();	}

	virtual void unlockSurface()
	{	mSurface->unlock();	}

	virtual void startRefresh() { }
	virtual void finishRefresh() { }

private:
	IWindowSurface*		mSurface;
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

	virtual const IWindowSurface* getPrimarySurface() const = 0;

	virtual IWindowSurface* getPrimarySurface()
	{
		return const_cast<IWindowSurface*>(static_cast<const IWindow&>(*this).getPrimarySurface());
	}

	virtual uint16_t getWidth() const
	{	return getPrimarySurface()->getWidth();	}

	virtual uint16_t getHeight() const
	{	return getPrimarySurface()->getHeight();	}

	virtual uint8_t getBitsPerPixel() const
	{	return getPrimarySurface()->getBitsPerPixel();	}

	virtual int getBytesPerPixel() const
	{	return getPrimarySurface()->getBytesPerPixel();	}

	virtual const IVideoMode& getVideoMode() const = 0;

	virtual const PixelFormat* getPixelFormat() const = 0;

	virtual bool isFullScreen() const
	{	return getVideoMode().isFullScreen();	}

	virtual EWindowMode getWindowMode() const = 0;

	virtual bool isFocused() const
	{	return false;	}

	virtual void flashWindow() const { }

	virtual bool usingVSync() const
	{	return false;	}

	virtual bool setMode(const IVideoMode& video_mode) = 0;

	virtual void lockSurface() { }
	virtual void unlockSurface() { }

	virtual void enableRefresh() { }
	virtual void disableRefresh() { }

	virtual void startRefresh() { }
	virtual void finishRefresh() { }

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
	IDummyWindow() :
		IWindow(), mPrimarySurface(NULL), mVideoMode(320, 200, 8, WINDOW_Windowed),
		mPixelFormat(8, 0, 0, 0, 0, 0, 0, 0, 0)
	{ }

	virtual ~IDummyWindow()
	{	delete mPrimarySurface;	}

	virtual const IWindowSurface* getPrimarySurface() const
	{	return mPrimarySurface;	}

	virtual const IVideoMode& getVideoMode() const
	{	return mVideoMode;	}

	virtual const PixelFormat* getPixelFormat() const
	{	return &mPixelFormat;	}

	virtual bool setMode(const IVideoMode& video_mode)
	{
		if (mPrimarySurface == NULL)
		{
			// ignore the requested mode and setup the hardcoded mode
			mPrimarySurface = I_AllocateSurface(mVideoMode.width, mVideoMode.height, mVideoMode.bpp);
		}
		return mPrimarySurface != NULL;
	}

	virtual bool isFullScreen() const
	{	return mVideoMode.isFullScreen();	}

	virtual EWindowMode getWindowMode() const
	{	return mVideoMode.window_mode;	}

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
	PixelFormat			mPixelFormat;
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

	virtual const IWindow* getWindow() const = 0;

	virtual IWindow* getWindow()
	{
		return const_cast<IWindow*>(static_cast<const IVideoSubsystem&>(*this).getWindow());
	}

	virtual int getMonitorCount() const
	{
		return 1;
	}
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

	virtual const IWindow* getWindow() const
	{	return mWindow;	}

private:
	const IVideoCapabilities*		mVideoCapabilities;

	IWindow*						mWindow;
};
