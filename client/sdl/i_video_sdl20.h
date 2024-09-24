// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
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
//
// SDL 2.0 implementation of the IWindow class
//
//-----------------------------------------------------------------------------

#pragma once

#include "i_sdl.h"
#include "i_video.h"

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

	virtual EDisplayType getDisplayType() const
	{
		#ifdef GCONSOLE
		return DISPLAY_FullscreenOnly;
		#else
		return DISPLAY_Both;
		#endif
	}

	virtual const IVideoMode& getNativeMode() const
	{	return mNativeMode;	}

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
	ISDL20TextureWindowSurfaceManager(uint16_t width, uint16_t height, const PixelFormat* format, ISDL20Window* window,
			bool vsync, const char *render_scale_quality = NULL);

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
	ISDL20Window(uint16_t width, uint16_t height, uint8_t bpp, EWindowMode window_mode, bool vsync);

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
	{	return mVideoMode.width;	}

	virtual uint16_t getHeight() const
	{	return mVideoMode.height;	}

	virtual uint8_t getBitsPerPixel() const
	{	return mVideoMode.bpp;	}

	virtual int getBytesPerPixel() const
	{	return mVideoMode.bpp >> 3;	}

	virtual const IVideoMode& getVideoMode() const
	{	return mVideoMode;	}

	virtual const PixelFormat* getPixelFormat() const
	{	return &mPixelFormat;	}

	virtual bool setMode(const IVideoMode& video_mode);

	virtual bool isFullScreen() const
	{	return mVideoMode.isFullScreen();	}

	virtual EWindowMode getWindowMode() const
	{	return mVideoMode.window_mode;		}

	virtual bool isFocused() const;

	virtual void flashWindow() const;

	virtual bool usingVSync() const
	{	return mVideoMode.vsync;	}

	virtual void enableRefresh()
	{	mBlit = true;		}

	virtual void disableRefresh()
	{
		mBlit = false;
		mSurfaceManager->lockSurface();
		mSurfaceManager->getWindowSurface()->clear();
		mSurfaceManager->finishRefresh();
		mSurfaceManager->unlockSurface();
	}

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

	uint16_t getCurrentWidth() const;
	uint16_t getCurrentHeight() const;
	EWindowMode getCurrentWindowMode() const;

	SDL_Window*			mSDLWindow;

	IWindowSurfaceManager* mSurfaceManager;

	IVideoMode			mVideoMode;
	PixelFormat			mPixelFormat;

	bool				mNeedPaletteRefresh;
	bool				mBlit;

	bool				mMouseFocus;
	bool				mKeyboardFocus;

	int					mAcceptResizeEventsTime;

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

	virtual int getMonitorCount() const;

private:
	const IVideoCapabilities*		mVideoCapabilities;

	IWindow*						mWindow;
};
#endif	// SDL20
