// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
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
//	SDL implementation of the IVideo class.
//
//-----------------------------------------------------------------------------


#ifndef __SDLVIDEO_H__
#define __SDLVIDEO_H__

#include <SDL.h>
#include <list>
#include "i_video.h"
#include "c_console.h"

// ============================================================================
//
// ISDL12WindowSurface class interface
//
// Wraps the raw device surface and provides methods to access the raw surface
// buffer.
//
// ============================================================================

class ISDL12WindowSurface : public IWindowSurface
{
public:
	ISDL12WindowSurface(IWindow* window, int width, int height, int bpp);

	ISDL12WindowSurface(IWindow* window, SDL_Surface* sdlsurface);

	virtual ~ISDL12WindowSurface();

	virtual byte* getBuffer()
	{	return mSurfaceBuffer;	}

	virtual const byte* getBuffer() const
	{	return mSurfaceBuffer;	}

	virtual void lock();
	virtual void unlock();

	virtual int getWidth() const
	{	return mWidth;	}

	virtual int getHeight() const
	{	return mHeight;	}

	virtual int getPitch() const
	{	return mPitch;	}

	virtual int getPitchInPixels() const
	{	return mPitchInPixels;	}

	virtual int getBitsPerPixel() const
	{	return mBitsPerPixel;	}

	virtual int getBytesPerPixel() const
	{	return mBytesPerPixel;	}

	virtual void setPalette(const argb_t* palette);
	virtual const argb_t* getPalette() const;

private:
	// disable copy constructor and assignment operator
	ISDL12WindowSurface(const ISDL12WindowSurface&);
	ISDL12WindowSurface& operator=(const ISDL12WindowSurface&);

	void initializeFromSDLSurface(SDL_Surface* sdlsurface);

	SDL_Surface*		mSDLSurface;
	byte*				mSurfaceBuffer;

	argb_t				mPalette[256];

	int					mWidth;
	int					mHeight;
	int					mPitch;
	int					mPitchInPixels;
	int					mBitsPerPixel;
	int					mBytesPerPixel;		// optimization: pre-calculate

	int					mLocks;
	bool				mFreeSDLSurface;
};



// ============================================================================
//
// ISDL12Window class interface
//
// ============================================================================

class ISDL12Window : public IWindow
{
public:
	ISDL12Window(int width, int height, int bpp, bool fullscreen, bool vsync);

	virtual ~ISDL12Window();

	virtual const IVideoModeList* getSupportedVideoModes() const
	{	return &mVideoModes;	}

	virtual const EDisplayType getDisplayType() const
	{	return DISPLAY_Both;	}

	virtual IWindowSurface* getPrimarySurface()
	{	return mPrimarySurface;	}

	virtual const IWindowSurface* getPrimarySurface() const
	{	return mPrimarySurface;	}

	virtual int getWidth() const
	{	return mWidth;	}

	virtual int getHeight() const
	{	return mHeight;	}

	virtual int getBitsPerPixel() const
	{	return mBitsPerPixel;	}

	virtual bool setMode(int width, int height, int bpp, bool fullscreen, bool vsync);

	virtual bool isFullScreen() const
	{	return mIsFullScreen;	}

	virtual void refresh();

	virtual void setWindowTitle(const std::string& str = "");

	virtual std::string getVideoDriverName() const;

private:
	// disable copy constructor and assignment operator
	ISDL12Window(const ISDL12Window&);
	ISDL12Window& operator=(const ISDL12Window&);

	void buildVideoModeList();

	IVideoModeList		mVideoModes;
	IWindowSurface*		mPrimarySurface;

	int					mWidth;
	int					mHeight;
	int					mBitsPerPixel;

	bool				mIsFullScreen;
	bool				mUseVSync;
};

#endif

