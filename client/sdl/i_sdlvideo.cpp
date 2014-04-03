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


#include <stdio.h>
#include <stdlib.h>
#include <cassert>

#include <algorithm>
#include <functional>
#include <string>
#include "doomstat.h"

// [Russell] - Just for windows, display the icon in the system menu and
// alt-tab display
#include "win32inc.h"
#if defined(_WIN32) && !defined(_XBOX)
    #include "SDL_syswm.h"
    #include "resource.h"
#endif // WIN32

#include "i_video.h"
#include "v_video.h"

#include "v_palette.h"
#include "i_sdlvideo.h"
#include "i_system.h"
#include "m_argv.h"
#include "m_memio.h"

#ifdef _XBOX
#include "i_xbox.h"
#endif

EXTERN_CVAR (vid_autoadjust)
EXTERN_CVAR (vid_vsync)
EXTERN_CVAR (vid_displayfps)
EXTERN_CVAR (vid_ticker)

CVAR_FUNC_IMPL(vid_vsync)
{
	setmodeneeded = true;
}

SDLVideo::SDLVideo(int parm)
{
	const SDL_version *SDLVersion = SDL_Linked_Version();

	if(SDLVersion->major != SDL_MAJOR_VERSION
		|| SDLVersion->minor != SDL_MINOR_VERSION)
	{
		I_FatalError("SDL version conflict (%d.%d.%d vs %d.%d.%d dll)\n",
			SDL_MAJOR_VERSION, SDL_MINOR_VERSION, SDL_PATCHLEVEL,
			SDLVersion->major, SDLVersion->minor, SDLVersion->patch);
		return;
	}

	if (SDL_InitSubSystem (SDL_INIT_VIDEO) == -1)
	{
		I_FatalError("Could not initialize SDL video.\n");
		return;
	}

	if(SDLVersion->patch != SDL_PATCHLEVEL)
	{
		Printf_Bold("SDL version warning (%d.%d.%d vs %d.%d.%d dll)\n",
			SDL_MAJOR_VERSION, SDL_MINOR_VERSION, SDL_PATCHLEVEL,
			SDLVersion->major, SDLVersion->minor, SDLVersion->patch);
	}

    // [Russell] - Just for windows, display the icon in the system menu and
    // alt-tab display
    #if WIN32 && !_XBOX
    HICON hIcon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_ICON1));

    if (hIcon)
    {
        HWND WindowHandle;

        SDL_SysWMinfo wminfo;
        SDL_VERSION(&wminfo.version)
        SDL_GetWMInfo(&wminfo);

        WindowHandle = wminfo.window;

        SendMessage(WindowHandle, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
        SendMessage(WindowHandle, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
    }
    #endif

   sdlScreen = NULL;
   infullscreen = false;
   screenw = screenh = screenbits = 0;
   palettechanged = false;
}

std::string SDLVideo::GetVideoDriverName()
{
  char driver[128];

  if((SDL_VideoDriverName(driver, 128)) == NULL)
  {
    char *pdrv; // Don't modify or free this

    if((pdrv = getenv("SDL_VIDEODRIVER")) == NULL)
      return ""; // Can't determine driver

    return std::string(pdrv); // Return the environment variable
  }

  return std::string(driver); // Return the name as provided by SDL
}


bool SDLVideo::FullscreenChanged (bool fs)
{
   if(fs != infullscreen)
      return true;

   return false;
}

void SDLVideo::SetWindowedScale (float scale)
{
   /// HAHA FIXME
}

bool SDLVideo::SetOverscan (float scale)
{
	int   ret = 0;

	if(scale > 1.0)
		return false;

#ifdef _XBOX
	if(xbox_SetScreenStretch( -(screenw - (screenw * scale)), -(screenh - (screenh * scale))) )
		ret = -1;
	if(xbox_SetScreenPosition( (screenw - (screenw * scale)) / 2, (screenh - (screenh * scale)) / 2) )
		ret = -1;
#endif

	if(ret)
		return false;

	return true;
}

bool SDLVideo::SetMode(int width, int height, int bits, bool fullscreen)
{
	Uint32 flags = (vid_vsync ? SDL_HWSURFACE | SDL_DOUBLEBUF : SDL_SWSURFACE);

	if (fullscreen)
		flags |= SDL_FULLSCREEN;
	else
		flags |= SDL_RESIZABLE;

	if (fullscreen && bits == 8)
		flags |= SDL_HWPALETTE;

	// TODO: check for multicore
	flags |= SDL_ASYNCBLIT;

	// [SL] SDL_SetVideoMode reinitializes DirectInput if DirectX is being used.
	// This interferes with RawWin32Mouse's input handlers so we need to
	// disable them prior to reinitalizing DirectInput...
	I_PauseMouse();

	int sbits = bits;

	#ifdef _WIN32
	// fullscreen directx requires a 32-bit mode to fix broken palette
	// [Russell] - Use for gdi as well, fixes d2 map02 water
	if (fullscreen)
		sbits = 32;
	#endif

#ifdef SDL_GL_SWAP_CONTROL
	SDL_GL_SetAttribute(SDL_GL_SWAP_CONTROL, vid_vsync);
#endif

	if (!(sdlScreen = SDL_SetVideoMode(width, height, sbits, flags)))
		return false;

	// [SL] ...and re-enable RawWin32Mouse's input handlers after
	// DirectInput is reinitalized.
	I_ResumeMouse();

	screenw = width;
	screenh = height;
	screenbits = bits;

	return true;
}


void SDLVideo::SetPalette(argb_t *palette)
{
	for (size_t i = 0; i < sizeof(newPalette)/sizeof(SDL_Color); i++)
	{
		newPalette[i].r = RPART(palette[i]);
		newPalette[i].g = GPART(palette[i]);
		newPalette[i].b = BPART(palette[i]);
	}
	palettechanged = true;
}

void SDLVideo::SetOldPalette(byte *doompalette)
{
	for (int i = 0; i < 256; ++i)
	{
		newPalette[i].r = newgamma[*doompalette++];
		newPalette[i].g = newgamma[*doompalette++];
		newPalette[i].b = newgamma[*doompalette++];
	}
	palettechanged = true;
}

void SDLVideo::UpdateScreen(DCanvas *canvas)
{
/*
	// Draws frame time and cumulative fps
	if (vid_displayfps)
		V_DrawFPSWidget();

    // draws little dots on the bottom of the screen
    if (vid_ticker)
		V_DrawFPSTicker();

	if (palettechanged)
	{
		// m_Private may or may not be the primary surface (sdlScreen)
		SDL_SetPalette((SDL_Surface*)canvas->m_Private, SDL_LOGPAL|SDL_PHYSPAL, newPalette, 0, 256);
		SDL_SetPalette(sdlScreen, SDL_LOGPAL|SDL_PHYSPAL, newPalette, 0, 256);
		palettechanged = false;
	}

	// If not writing directly to the screen blit to the primary surface
	if (canvas->m_Private != sdlScreen)
	{
		short w = (screenw - I_GetSurfaceWidth()) / 2;
		short h = (screenh - I_GetSurfaceHeight()) / 2;
		SDL_Rect dstrect = { w, h };
		SDL_BlitSurface((SDL_Surface*)canvas->m_Private, NULL, sdlScreen, &dstrect);
	}

	if (vid_vsync)
		SDL_Flip(sdlScreen);
	else
		SDL_UpdateRect(sdlScreen, 0, 0, 0, 0);
*/
}


void SDLVideo::ReadScreen (byte *block)
{
   // SoM: forget lastCanvas, let's just read from the screen, y0
   if(!sdlScreen)
      return;

   int y;
   byte *source;
   bool unlock = false;

   if(SDL_MUSTLOCK(sdlScreen))
   {
      unlock = true;
      SDL_LockSurface(sdlScreen);
   }

   source = (byte *)sdlScreen->pixels;

   for (y = 0; y < sdlScreen->h; y++)
   {
      memcpy (block, source, sdlScreen->w);
      block += sdlScreen->w;
      source += sdlScreen->pitch;
   }

   if(unlock)
      SDL_UnlockSurface(sdlScreen);
}


DCanvas *SDLVideo::AllocateSurface(int width, int height, int bits, bool primary)
{
	DCanvas *scrn = new DCanvas(I_GetPrimarySurface());

/*
	scrn->m_LockCount = 0;
	scrn->m_Palette = NULL;

	Uint32 flags = SDL_SWSURFACE;

	SDL_Surface* new_surface = SDL_CreateRGBSurface(flags, width, height, bits, 0, 0, 0, 0);

	if (!new_surface)
		I_FatalError("SDLVideo::AllocateSurface failed to allocate an SDL surface.");

	if (new_surface->pitch != (width * (bits / 8)) && vid_autoadjust)
		Printf(PRINT_HIGH, "Warning: SDLVideo::AllocateSurface got a surface with an abnormally wide pitch.\n");

	// determine format of 32bpp pixels
	if (bits == 32)
	{
		SDL_PixelFormat* fmt = new_surface->format;
		// find which byte is not used and use it for alpha (SDL always reports 0 for alpha)
		scrn->setAlphaShift(48 - (fmt->Rshift + fmt->Gshift + fmt->Bshift));
		scrn->setRedShift(fmt->Rshift);
		scrn->setGreenShift(fmt->Gshift);
		scrn->setBlueShift(fmt->Bshift);
	}
	else
	{
		scrn->setAlphaShift(24);
		scrn->setRedShift(16);
		scrn->setGreenShift(8);
		scrn->setBlueShift(0);
	}

	scrn->m_Private = new_surface;
*/

	return scrn;
}



void SDLVideo::ReleaseSurface(DCanvas *scrn)
{
/*
	if(scrn->m_Private == sdlScreen) // primary stays
		return;

	if (scrn->m_LockCount)
		scrn->Unlock();

	if (scrn->m_Private)
	{
		SDL_FreeSurface((SDL_Surface *)scrn->m_Private);
		scrn->m_Private = NULL;
	}

	scrn->DetachPalette ();

	delete scrn;
*/
}


void SDLVideo::LockSurface (DCanvas *scrn)
{
/*
   SDL_Surface *s = (SDL_Surface *)scrn->m_Private;

   if(SDL_MUSTLOCK(s))
   {
      if(SDL_LockSurface(s) == -1)
         I_FatalError("SDLVideo::LockSurface failed to lock a surface that required it...\n");

      scrn->m_LockCount ++;
   }

   scrn->buffer = (byte*)s->pixels;
*/
}


void SDLVideo::UnlockSurface (DCanvas *scrn)
{
/*
   if(!scrn->m_Private)
      return;

   SDL_UnlockSurface((SDL_Surface *)scrn->m_Private);
   scrn->buffer = NULL;
*/
}

bool SDLVideo::Blit (DCanvas *src, int sx, int sy, int sw, int sh, DCanvas *dst, int dx, int dy, int dw, int dh)
{
   return false;
}


// ****************************************************************************


// ============================================================================
//
// ISDL12WindowSurface class implementation
//
// Abstraction for SDL 1.2 drawing surfaces, wrapping the SDL_Surface struct.
//
// ============================================================================

//
// ISDL12WindowSurface::ISDL12WindowSurface
//
ISDL12WindowSurface::ISDL12WindowSurface(IWindow* window, int width, int height, int bpp) :
	IWindowSurface(window), mSDLSurface(NULL), mLocks(0)
{
	Uint32 flags = SDL_SWSURFACE;
	SDL_Surface* sdlsurface = SDL_CreateRGBSurface(flags, width, height, bpp, 0, 0, 0, 0);

	initializeFromSDLSurface(sdlsurface);
}


//
// ISDL12WindowSurface::ISDL12WindowSurface
//
// Constructs the surface using an existing SDL_Surface handle.
//
ISDL12WindowSurface::ISDL12WindowSurface(IWindow* window, SDL_Surface* sdlsurface) :
	IWindowSurface(window), mSDLSurface(NULL), mLocks(0)
{
	initializeFromSDLSurface(sdlsurface);
}


//
// ISDL12WindowSurface::initializeFromSDLSurface
//
// Private helper function for the constructors.
//
void ISDL12WindowSurface::initializeFromSDLSurface(SDL_Surface* sdlsurface)
{
	if (mSDLSurface)
		SDL_FreeSurface(mSDLSurface);

	mSDLSurface = sdlsurface;

	lock();
	mSurfaceBuffer = (byte*)mSDLSurface->pixels;
	mWidth = mSDLSurface->w;
	mHeight = mSDLSurface->h;
	mBitsPerPixel = mSDLSurface->format->BitsPerPixel;
	mBytesPerPixel = mBitsPerPixel / 8;
	mPitch = mSDLSurface->pitch;
	mPitchInPixels = mPitch / mBytesPerPixel;
	unlock();

	memset(mPalette, 0, 256 * sizeof(*mPalette));

	assert(mWidth >= 0 && mWidth <= MAXWIDTH);
	assert(mHeight >= 0 && mHeight <= MAXHEIGHT);
	assert(mBitsPerPixel == 8 || mBitsPerPixel == 32);
}


//
// ISDL12WindowSurface::~ISDL12WindowSurface
//
// Frees the SDL_Surface handle.
//
ISDL12WindowSurface::~ISDL12WindowSurface()
{
	if (mSDLSurface)
		SDL_FreeSurface(mSDLSurface);
	mSDLSurface = NULL;
}


//
// ISDL12WindowSurface::lock
//
// Locks the surface for direct pixel access. This must be called prior to
// accessing mSurfaceBuffer.
//
void ISDL12WindowSurface::lock()
{
	if (++mLocks == 1)
		SDL_LockSurface(mSDLSurface);

	assert(mLocks >= 1 && mLocks < 100);
}


//
// ISDL12WindowSurface::unlock
//
// Unlocks the surface after direct pixel access. This must be called after
// accessing mSurfaceBuffer.
//
void ISDL12WindowSurface::unlock()
{
	if (--mLocks == 0)
		SDL_UnlockSurface(mSDLSurface);

	assert(mLocks >= 0 && mLocks < 100);
}


//
// ISDL12WindowSurface::setPalette
//
// Accepts an array of 256 argb_t values.
//
void ISDL12WindowSurface::setPalette(const argb_t* palette)
{
	memcpy(mPalette, palette, 256 * sizeof(*mPalette));

	if (mBitsPerPixel == 8)
	{
		lock();

		assert(mSDLSurface->format->palette != NULL);
		assert(mSDLSurface->format->palette->ncolors == 256);
		SDL_Color* sdlcolors = mSDLSurface->format->palette->colors;
		for (int c = 0; c < 256; c++)
		{
			sdlcolors[c].r = RPART(palette[c]);
			sdlcolors[c].g = GPART(palette[c]);
			sdlcolors[c].b = BPART(palette[c]);
		}

		unlock();
	}
}


//
// ISDL12WindowSurface::setPalette
//
// Accepts an array of 768 palindex_t values, laid out as 256 groups
// of red, green, and blue values.
//
void ISDL12WindowSurface::setPalette(const palindex_t* palette)
{
	if (mBitsPerPixel == 8)
	{
		lock();

		assert(mSDLSurface->format->palette != NULL);
		assert(mSDLSurface->format->palette->ncolors == 256);
		SDL_Color* sdlcolors = mSDLSurface->format->palette->colors;
		for (int c = 0; c < 256; c++)
		{
			sdlcolors[c].r = *palette++;
			sdlcolors[c].g = *palette++;
			sdlcolors[c].b = *palette++;
			mPalette[c] = MAKEARGB(255, sdlcolors[c].r, sdlcolors[c].g, sdlcolors[c].b);
		}

		unlock();
	}
}


//
// ISDLWindowSurface::getPalette
//
const argb_t* ISDL12WindowSurface::getPalette() const
{
	return mPalette;
}


// ****************************************************************************

// ============================================================================
//
// ISDL12Window class implementation
//
// ============================================================================


//
// ISDL12Window::ISDL12Window
//
// Constructs a new application window using SDL 1.2.
// A ISDL12WindowSurface object is instantiated for frame rendering.
//
ISDL12Window::ISDL12Window(int width, int height, int bpp, bool fullscreen, bool vsync) :
	IWindow(),
	mPrimarySurface(NULL),
	mIsFullScreen(fullscreen), mUseVSync(vsync)
{
	const SDL_version* SDLVersion = SDL_Linked_Version();

	if (SDLVersion->major != SDL_MAJOR_VERSION || SDLVersion->minor != SDL_MINOR_VERSION)
	{
		I_FatalError("SDL version conflict (%d.%d.%d vs %d.%d.%d dll)\n",
			SDL_MAJOR_VERSION, SDL_MINOR_VERSION, SDL_PATCHLEVEL,
			SDLVersion->major, SDLVersion->minor, SDLVersion->patch);
		return;
	}

	if (SDLVersion->patch != SDL_PATCHLEVEL)
	{
		Printf_Bold("SDL version warning (%d.%d.%d vs %d.%d.%d dll)\n",
			SDL_MAJOR_VERSION, SDL_MINOR_VERSION, SDL_PATCHLEVEL,
			SDLVersion->major, SDLVersion->minor, SDLVersion->patch);
	}

	if (SDL_InitSubSystem(SDL_INIT_VIDEO) == -1)
	{
		I_FatalError("Could not initialize SDL video.\n");
		return;
	}

	buildVideoModeList();

	setMode(width, height, bpp, fullscreen, vsync);
}


//
// ISDL12Window::~ISDL12Window
//
ISDL12Window::~ISDL12Window()
{
	delete mPrimarySurface;
    SDL_QuitSubSystem(SDL_INIT_VIDEO);
}


//
// ISDL12Window::setWindowed
//
void ISDL12Window::setWindowed()
{
	setMode(mWidth, mHeight, mBitsPerPixel, false, mUseVSync);
}


//
// ISDL12Window::setFullScreen
//
void ISDL12Window::setFullScreen()
{
	setMode(mWidth, mHeight, mBitsPerPixel, true, mUseVSync);
}


//
// ISDL12Window::resize
//
// Resizes the window to the specified dimensions, instantiating a new
// surface object.
//
void ISDL12Window::resize(int width, int height)
{
	setMode(width, height, mBitsPerPixel, mIsFullScreen, mUseVSync);
}


//
// ISDL12Window::refresh
//
void ISDL12Window::refresh()
{
	SDL_Surface* sdlsurface = SDL_GetVideoSurface();

	if (mBitsPerPixel == 8)
	{
		Uint32 flags = SDL_LOGPAL | SDL_PHYSPAL;
		SDL_Color* sdlcolors = sdlsurface->format->palette->colors;
		SDL_SetPalette(sdlsurface, flags, sdlcolors, 0, 256);
	}


	// TODO: possibly blit from a software surface to the video screen

	if (mUseVSync)
		SDL_Flip(sdlsurface);
	else
		SDL_UpdateRect(sdlsurface, 0, 0, 0, 0);
}


//
// ISDL12Window::setWindowTitle
//
// Sets the title caption of the window.
//
void ISDL12Window::setWindowTitle(const std::string& str)
{
	SDL_WM_SetCaption(str.c_str(), str.c_str());
}


//
// ISDL12Window::getVideoDriverName
//
// Returns the name of the video driver that SDL is currently
// configured to use.
//
std::string ISDL12Window::getVideoDriverName() const
{
	char driver[128];

	if ((SDL_VideoDriverName(driver, 128)) == NULL)
	{
		const char* pdrv = getenv("SDL_VIDEODRIVER");

		if (pdrv == NULL)
			return "";
		return std::string(pdrv);
	}

	return std::string(driver);
}


//
// ISDL12Window::setMode
//
// Sets the window size to the specified size and frees the existing primary
// surface before instantiating a new primary surface.
// 
bool ISDL12Window::setMode(int width, int height, int bpp, bool fullscreen, bool vsync)
{
	assert(width <= MAXWIDTH);
	assert(height <= MAXHEIGHT);
	assert(bpp == 8 || bpp == 32);

	uint32_t flags = vsync ? SDL_HWSURFACE | SDL_DOUBLEBUF : SDL_SWSURFACE;

	if (fullscreen && !mVideoModes.empty())
		flags |= SDL_FULLSCREEN;
	else
		flags |= SDL_RESIZABLE;

	if (fullscreen && bpp == 8)
		flags |= SDL_HWPALETTE;

	// TODO: check for multicore
	flags |= SDL_ASYNCBLIT;

	// [SL] SDL_SetVideoMode reinitializes DirectInput if DirectX is being used.
	// This interferes with RawWin32Mouse's input handlers so we need to
	// disable them prior to reinitalizing DirectInput...
	I_PauseMouse();

	int sbits = bpp;

	#ifdef _WIN32
	// fullscreen directx requires a 32-bit mode to fix broken palette
	// [Russell] - Use for gdi as well, fixes d2 map02 water
	if (fullscreen)
		sbits = 32;
	#endif

	#ifdef SDL_GL_SWAP_CONTROL
	SDL_GL_SetAttribute(SDL_GL_SWAP_CONTROL, vsync);
	#endif

	SDL_Surface* sdlsurface = SDL_SetVideoMode(width, height, sbits, flags);

	// [SL] ...and re-enable RawWin32Mouse's input handlers after
	// DirectInput is reinitalized.
	I_ResumeMouse();

	if (sdlsurface == NULL)
		return false;

	mWidth = width;
	mHeight = height;
	mBitsPerPixel = bpp;
	mIsFullScreen = fullscreen;
	mUseVSync = vsync;

	// create a new IWindowSurface for the SDL_Surface handle that
	// was returned by SDL_SetVideoMode
	delete mPrimarySurface;
	mPrimarySurface = new ISDL12WindowSurface(this, sdlsurface);	

	return true;
}


//
// ISDL12Window::buildVideoModeList
//
// Queries SDL for the supported full screen video modes and populates
// the mVideoModes list.
//
void ISDL12Window::buildVideoModeList()
{
	mVideoModes.clear();

	// Fetch the list of fullscreen modes for this bpp setting:
	SDL_Rect** sdlmodes = SDL_ListModes(NULL, SDL_FULLSCREEN | SDL_SWSURFACE);

	if (sdlmodes == NULL)
	{
		// no fullscreen modes, but we could still try windowed
		Printf(PRINT_HIGH, "No fullscreen video modes are available.\n");
		return;
	}
	else if (sdlmodes == (SDL_Rect**)-1)
	{
		// SDL 1.2 documentation indicates the following
		// "-1: Any dimension is okay for the given format"
		// Shouldn't happen with SDL_FULLSCREEN flag though

		I_FatalError("SDL_ListModes returned -1. Internal error.\n");
		return;
	}

	// always add the following modes
	mVideoModes.push_back(IVideoMode(320, 200));
	mVideoModes.push_back(IVideoMode(320, 240));
	mVideoModes.push_back(IVideoMode(640, 400));
	mVideoModes.push_back(IVideoMode(640, 480));

	// add the full screen video modes reported by SDL	
	while (*sdlmodes)
	{
		int width = (*sdlmodes)->w, height = (*sdlmodes)->h;
		if (width > 0 && width <= MAXWIDTH && height > 0 && height <= MAXHEIGHT)
			mVideoModes.push_back(IVideoMode(width, height));
		++sdlmodes;
	}

	// reverse sort the modes
	std::sort(mVideoModes.begin(), mVideoModes.end(), std::greater<IVideoMode>());

	// get rid of any duplicates (SDL some times reports duplicates)
	mVideoModes.erase(std::unique(mVideoModes.begin(), mVideoModes.end()), mVideoModes.end());
}

VERSION_CONTROL (i_sdlvideo_cpp, "$Id$")

