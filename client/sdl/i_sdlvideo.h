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

class SDLVideo : public IVideo
{
   public:
	SDLVideo(int parm);
	virtual ~SDLVideo (void) {};

	virtual std::string GetVideoDriverName();

	virtual bool CanBlit (void) {return false;}
	virtual EDisplayType GetDisplayType (void) {return DISPLAY_Both;}

	virtual bool FullscreenChanged (bool fs);
	virtual void SetWindowedScale (float scale);
	virtual bool SetOverscan (float scale);

	virtual int GetWidth() const { return screenw; }
	virtual int GetHeight() const { return screenh; }
	virtual int GetBitDepth() const { return screenbits; }

	virtual bool SetMode (int width, int height, int bits, bool fs);
	virtual void SetPalette (DWORD *palette);

	/* 12/3/06: HACK - Add SetOldPalette to accomodate classic redscreen - ML*/
	virtual void SetOldPalette (byte *doompalette);

	virtual void UpdateScreen (DCanvas *canvas);
	virtual void ReadScreen (byte *block);

	virtual int GetModeCount (void);
	virtual void StartModeIterator ();
	virtual bool NextMode (int *width, int *height);

	virtual DCanvas *AllocateSurface (int width, int height, int bits, bool primary = false);
	virtual void ReleaseSurface (DCanvas *scrn);
	virtual void LockSurface (DCanvas *scrn);
	virtual void UnlockSurface (DCanvas *scrn);
	virtual bool Blit (DCanvas *src, int sx, int sy, int sw, int sh,
					   DCanvas *dst, int dx, int dy, int dw, int dh);

   protected:

   struct vidMode_t
   {
		int width, height;

      bool operator<(const vidMode_t& right) const
      {
            if (width < right.width)
               return true;
            else if (width == right.width && height < right.height)
               return true;
         return false;
      }

      bool operator>(const vidMode_t& right) const
      {
            if (width > right.width)
               return true;
			else if (width == right.width && height > right.height)
               return true;
         return false;
      }

      bool operator==(const vidMode_t& right) const
      {
         return (width == right.width &&
					height == right.height);
      }
   };

   std::vector<vidMode_t> vidModeList;
   size_t vidModeIterator;

   SDL_Surface *sdlScreen;
   bool infullscreen;
   int screenw, screenh;
   int screenbits;

   SDL_Color newPalette[256];
   SDL_Color palette[256];
   bool palettechanged;
};




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

	virtual int getBitsPerPixel() const
	{	return mBitsPerPixel;	}

	virtual int getBytesPerPixel() const
	{	return mBytesPerPixel;	}

	virtual void setPalette(const argb_t* palette);
	virtual const argb_t* getPalette() const;

private:
	SDL_Surface*		mSDLSurface;
	byte*				mSurfaceBuffer;

	argb_t				mPalette[256];

	int					mWidth;
	int					mHeight;
	int					mPitch;
	int					mBitsPerPixel;
	int					mBytesPerPixel;		// optimization: pre-calculate
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

	virtual void setWindowed();

	virtual void setFullScreen();

	virtual bool isFullScreen() const
	{	return mIsFullScreen;	}

	virtual void resize(int width, int height);

	virtual void refresh();

	virtual void setWindowTitle(const std::string& str = "");

	virtual std::string getVideoDriverName() const;

private:
	bool setMode(int width, int height, int bpp, bool fullscreen, bool vsync);
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

