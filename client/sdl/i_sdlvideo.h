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
//
// SDL 1.2 implementation of the IWindow class
//
//-----------------------------------------------------------------------------


#ifndef __I_SDLVIDEO_H__
#define __I_SDLVIDEO_H__

#include <SDL.h>
#include "i_video.h"

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

	virtual const IVideoModeList* getSupportedVideoModes() const
	{	return &mVideoModes;	}

	virtual const EDisplayType getDisplayType() const
	{	return DISPLAY_Both;	}

	virtual IWindowSurface* getPrimarySurface()
	{	return mPrimarySurface;	}

	virtual const IWindowSurface* getPrimarySurface() const
	{	return mPrimarySurface;	}

	virtual uint16_t getWidth() const
	{	return mWidth;	}

	virtual uint16_t getHeight() const
	{	return mHeight;	}

	virtual uint8_t getBitsPerPixel() const
	{	return mBitsPerPixel;	}

	virtual int getBytesPerPixel() const
	{	return mBitsPerPixel >> 3;	}

	virtual bool setMode(uint16_t width, uint16_t height, uint8_t bpp, bool fullscreen, bool vsync);

	virtual bool isFullScreen() const
	{	return mIsFullScreen;	}

	virtual void refresh();

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

	void buildVideoModeList();

	IVideoModeList		mVideoModes;
	IWindowSurface*		mPrimarySurface;

	uint16_t			mWidth;
	uint16_t			mHeight;
	uint8_t				mBitsPerPixel;

	bool				mIsFullScreen;
	bool				mUseVSync;

	bool				m8in32;
	SDL_Surface*		mSDLSoftwareSurface;

	bool				mNeedPaletteRefresh;

	int16_t				mLocks;
};

#endif	// __I_SDLVIDEO_H__

