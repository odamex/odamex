// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
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
//
// SDL 1.2 implementation of the IWindow class
//
//-----------------------------------------------------------------------------


#ifndef __I_SDLVIDEO_H__
#define __I_SDLVIDEO_H__

#include "i_sdl.h"
#include "i_video.h"

#ifdef SDL12
// ============================================================================
//
// ISDL12VideoCapabilities class interface
//
// Defines an interface for querying video capabilities. This includes listing
// availible video modes and supported operations.
//
// ============================================================================

class ISDL12VideoCapabilities : public IVideoCapabilities
{
public:
	ISDL12VideoCapabilities();
	virtual ~ISDL12VideoCapabilities() { }

	virtual const IVideoModeList* getSupportedVideoModes() const
	{	return &mModeList;	}

	virtual const EDisplayType getDisplayType() const
	{
		#ifdef GCONSOLE
		return DISPLAY_FullscreenOnly;
		#else
		return DISPLAY_Both;
		#endif
	}

	virtual const IVideoMode* getNativeMode() const
	{	return &mNativeMode;	}

private:
	IVideoModeList		mModeList;
	IVideoMode			mNativeMode;
};


// ============================================================================
//
// ISDL12DirectWindowSurfaceManager
//
// Helper class for IWindow to encapsulate the creation of a IWindowSurface
// primary surface and to assist in using it to refresh the window.
//
// This creates a IWindowSurface based on SDL's primary surface.
//
// ============================================================================

class ISDL12DirectWindowSurfaceManager : public IWindowSurfaceManager
{
public:
	ISDL12DirectWindowSurfaceManager(uint16_t width, uint16_t height, const PixelFormat* format);

	virtual ~ISDL12DirectWindowSurfaceManager();

	virtual const IWindowSurface* getWindowSurface() const
	{	return mSurface;	}

	virtual void lockSurface();
	virtual void unlockSurface();

	virtual void startRefresh();
	virtual void finishRefresh();

private:
	IWindowSurface*			mSurface;
};


// ============================================================================
//
// ISDL12SoftwareWindowSurfaceManager
//
// Helper class for IWindow to encapsulate the creation of a IWindowSurface
// primary surface and to assist in using it to refresh the window.
//
//
// ============================================================================

class ISDL12SoftwareWindowSurfaceManager : public IWindowSurfaceManager
{
public:
	ISDL12SoftwareWindowSurfaceManager(uint16_t width, uint16_t height, const PixelFormat* format);

	virtual ~ISDL12SoftwareWindowSurfaceManager();

	virtual const IWindowSurface* getWindowSurface() const
	{	return mSurface;	}

	virtual void lockSurface();
	virtual void unlockSurface();

	virtual void startRefresh();
	virtual void finishRefresh();

private:
	SDL_Surface*			mSDLSoftwareSurface;
	IWindowSurface*			mSurface;
};


// ============================================================================
//
// ISDL12Window class interface
//
// ============================================================================

class ISDL12Window : public IWindow
{
public:
	ISDL12Window(uint16_t width, uint16_t height, uint8_t bpp, bool fullscreen, bool vsync);

	virtual ~ISDL12Window();

	virtual const IWindowSurface* getPrimarySurface() const
	{
		if (mSurfaceManager)
			return mSurfaceManager->getWindowSurface();
		return NULL;
	}

	virtual IWindowSurface* getPrimarySurface()
	{
		return const_cast<IWindowSurface*>(static_cast<const ISDL12Window&>(*this).getPrimarySurface());
	}

	virtual uint16_t getWidth() const
	{	return mWidth;	}

	virtual uint16_t getHeight() const
	{	return mHeight;	}

	virtual uint8_t getBitsPerPixel() const
	{	return mBitsPerPixel;	}

	virtual int getBytesPerPixel() const
	{	return mBitsPerPixel >> 3;	}

	virtual const IVideoMode* getVideoMode() const
	{	return &mVideoMode;	}

	virtual const PixelFormat* getPixelFormat() const;

	virtual bool setMode(uint16_t width, uint16_t height, uint8_t bpp, bool fullscreen, bool vsync);

	virtual bool isFullScreen() const
	{	return mIsFullScreen;	}

	virtual bool isFocused() const;

	virtual bool usingVSync() const
	{	return mUseVSync;	}

	virtual void enableRefresh()
	{	mBlit = true;		}

	virtual void disableRefresh()
	{	mBlit = false;		}

	virtual void startRefresh();
	virtual void finishRefresh();

	virtual void lockSurface();
	virtual void unlockSurface();

	virtual void setWindowTitle(const std::string& str = "");
	virtual void setWindowIcon();

	virtual std::string getVideoDriverName() const;

	virtual void setPalette(const argb_t* palette);

private:
	// disable copy constructor and assignment operator
	ISDL12Window(const ISDL12Window&);
	ISDL12Window& operator=(const ISDL12Window&);

	void getEvents();

	IWindowSurfaceManager*	mSurfaceManager;

	uint16_t			mWidth;
	uint16_t			mHeight;
	uint8_t				mBitsPerPixel;

	IVideoMode			mVideoMode;

	bool				mIsFullScreen;
	bool				mUseVSync;

	bool				mNeedPaletteRefresh;
	bool				mBlit;

	bool				mIgnoreResize;

	int16_t				mLocks;
};

// ****************************************************************************

// ============================================================================
//
// ISDL12VideoSubsystem class interface
//
// Provides intialization and shutdown mechanics for the video subsystem.
// This is really an abstract factory pattern as it instantiates a family
// of concrete types.
//
// ============================================================================

class ISDL12VideoSubsystem : public IVideoSubsystem
{
public:
	ISDL12VideoSubsystem();
	virtual ~ISDL12VideoSubsystem();

	virtual const IVideoCapabilities* getVideoCapabilities() const
	{	return mVideoCapabilities;	}

	virtual const IWindow* getWindow() const
	{	return mWindow;	}

private:
	const IVideoCapabilities*		mVideoCapabilities;

	IWindow*						mWindow;
};
#endif	// SDL12


// ----------------------------------------------------------------------------


#ifdef SDL20

class ISDL20Window;

// ============================================================================
//
// ISDL20VideoCapabilities class interface
//
// Defines an interface for querying video capabilities. This includes listing
// availible video modes and supported operations.
//
// ============================================================================

class ISDL20VideoCapabilities : public IVideoCapabilities
{
public:
	ISDL20VideoCapabilities();
	virtual ~ISDL20VideoCapabilities() { }

	virtual const IVideoModeList* getSupportedVideoModes() const
	{	return &mModeList;	}

	virtual const EDisplayType getDisplayType() const
	{
		#ifdef GCONSOLE
		return DISPLAY_FullscreenOnly;
		#else
		return DISPLAY_Both;
		#endif
	}

	virtual const IVideoMode* getNativeMode() const
	{	return &mNativeMode;	}

private:
	IVideoModeList		mModeList;
	IVideoMode			mNativeMode;
};


// ============================================================================
//
// ISDL20TextureWindowSurfaceManager
//
// Helper class for IWindow to encapsulate the creation of a IWindowSurface
// primary surface and to assist in using it to refresh the window.
//
// This creates a SDL_Renderer instance and a SDL_Texture. An IWindowSurface
// using the SDL_Texture as a basis is created for direct rendering.
//
// ============================================================================

class ISDL20TextureWindowSurfaceManager : public IWindowSurfaceManager
{
public:
	ISDL20TextureWindowSurfaceManager(uint16_t width, uint16_t height, const PixelFormat* format, ISDL20Window* window, bool vsync);

	virtual ~ISDL20TextureWindowSurfaceManager();

	virtual IWindowSurface* getWindowSurface()
	{	return mSurface;	}

	virtual const IWindowSurface* getWindowSurface() const
	{	return mSurface;	}

	virtual void lockSurface();
	virtual void unlockSurface();

	virtual void startRefresh();
	virtual void finishRefresh();

private:
	ISDL20Window*			mWindow;
	SDL_Renderer*			mSDLRenderer;
	SDL_Texture*			mSDLTexture;

	IWindowSurface*			mSurface;
	IWindowSurface*			m8bppTo32BppSurface;

	uint16_t				mWidth;
	uint16_t				mHeight;

	PixelFormat				mFormat;

	bool mDrawLogicalRect;
	SDL_Rect mLogicalRect;

	SDL_Renderer* createRenderer(bool vsync) const;
};


// ============================================================================
//
// ISDL20Window class interface
//
// ============================================================================

class ISDL20Window : public IWindow
{
public:
	ISDL20Window(uint16_t width, uint16_t height, uint8_t bpp, bool fullscreen, bool vsync);

	virtual ~ISDL20Window();

	virtual const IWindowSurface* getPrimarySurface() const
	{
		if (mSurfaceManager)
			return mSurfaceManager->getWindowSurface();
		return NULL;
	}

	virtual IWindowSurface* getPrimarySurface()
	{
		return const_cast<IWindowSurface*>(static_cast<const ISDL20Window&>(*this).getPrimarySurface());
	}

	virtual uint16_t getWidth() const
	{	return mWidth;	}

	virtual uint16_t getHeight() const
	{	return mHeight;	}

	virtual uint8_t getBitsPerPixel() const
	{	return mBitsPerPixel;	}

	virtual int getBytesPerPixel() const
	{	return mBitsPerPixel >> 3;	}

	virtual const IVideoMode* getVideoMode() const
	{	return &mVideoMode;	}

	virtual const PixelFormat* getPixelFormat() const
	{	return &mPixelFormat;	}

	virtual bool setMode(uint16_t width, uint16_t height, uint8_t bpp, bool fullscreen, bool vsync);

	virtual bool isFullScreen() const
	{	return mIsFullScreen;	}

	virtual bool isFocused() const;

	virtual bool usingVSync() const
	{	return mUseVSync;	}

	virtual void enableRefresh()
	{	mBlit = true;		}

	virtual void disableRefresh()
	{	mBlit = false;		}

	virtual void startRefresh();
	virtual void finishRefresh();


	virtual void lockSurface();
	virtual void unlockSurface();

	virtual void setWindowTitle(const std::string& str = "");
	virtual void setWindowIcon();

	virtual std::string getVideoDriverName() const;

	virtual void setPalette(const argb_t* palette);

private:
	// disable copy constructor and assignment operator
	ISDL20Window(const ISDL20Window&);
	ISDL20Window& operator=(const ISDL20Window&);

	friend class ISDL20TextureWindowSurfaceManager;

	void discoverNativePixelFormat();
	PixelFormat buildSurfacePixelFormat(uint8_t bpp);
	void setRendererDriver();
	bool isRendererDriverAvailable(const char* driver) const;
	const char* getRendererDriver() const;
	void getEvents();

	SDL_Window*			mSDLWindow;

	IWindowSurfaceManager* mSurfaceManager;

	uint16_t			mWidth;
	uint16_t			mHeight;
	uint8_t				mBitsPerPixel;

	IVideoMode			mVideoMode;
	PixelFormat			mPixelFormat;

	bool				mIsFullScreen;
	bool				mUseVSync;

	bool				mNeedPaletteRefresh;
	bool				mBlit;

	bool				mMouseFocus;
	bool				mKeyboardFocus;

	int16_t				mLocks;
};

// ****************************************************************************

// ============================================================================
//
// ISDL20VideoSubsystem class interface
//
// Provides intialization and shutdown mechanics for the video subsystem.
// This is really an abstract factory pattern as it instantiates a family
// of concrete types.
//
// ============================================================================

class ISDL20VideoSubsystem : public IVideoSubsystem
{
public:
	ISDL20VideoSubsystem();
	virtual ~ISDL20VideoSubsystem();

	virtual const IVideoCapabilities* getVideoCapabilities() const
	{	return mVideoCapabilities;	}

	virtual const IWindow* getWindow() const
	{	return mWindow;	}

private:
	const IVideoCapabilities*		mVideoCapabilities;

	IWindow*						mWindow;
};
#endif	// SDL20

#endif	// __I_SDLVIDEO_H__
