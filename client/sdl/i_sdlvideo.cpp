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
//	SDL implementation of the IVideo class.
//
//-----------------------------------------------------------------------------

#include "i_sdl.h"
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
    #include <SDL_syswm.h>
    #include "resource.h"
#endif // WIN32

#include "i_video.h"
#include "v_video.h"

#include "v_palette.h"
#include "i_sdlvideo.h"
#include "i_system.h"
#include "i_input.h"

#include "m_argv.h"
#include "w_wad.h"
#include "c_dispatch.h"

#include "res_texture.h"

#ifdef _XBOX
#include "i_xbox.h"
#endif

EXTERN_CVAR (vid_fullscreen)
EXTERN_CVAR (vid_defwidth)
EXTERN_CVAR (vid_defheight)


// ****************************************************************************

#ifdef SDL12
// ============================================================================
//
// ISDL12VideoCapabilities class implementation
//
// ============================================================================

//
// I_AddSDL12VideoModes
//
// Queries SDL to find the supported video modes at the given bit depth
// and then adds them to modelist.
//
static void I_AddSDL12VideoModes(IVideoModeList* modelist, int bpp)
{
	SDL_PixelFormat format;
	memset(&format, 0, sizeof(format));
	format.BitsPerPixel = bpp;

	SDL_Rect** sdlmodes = SDL_ListModes(&format, SDL_ANYFORMAT |SDL_FULLSCREEN | SDL_SWSURFACE);

	if (sdlmodes)
	{
		if (sdlmodes == (SDL_Rect**)-1)
		{
			// SDL 1.2 documentation indicates the following
			// "-1: Any dimension is okay for the given format"
			// Shouldn't happen with SDL_FULLSCREEN flag though
			I_FatalError("SDL_ListModes returned -1. Internal error.\n");
			return;
		}

		// add the video modes reported by SDL
		while (*sdlmodes)
		{
			int width = (*sdlmodes)->w, height = (*sdlmodes)->h;
			if (width > 0 && width <= MAXWIDTH && height > 0 && height <= MAXHEIGHT)
			{
				// add this video mode to the list (both fullscreen & windowed)
				modelist->push_back(IVideoMode(width, height, bpp, false));
				modelist->push_back(IVideoMode(width, height, bpp, true));
			}
			++sdlmodes;
		}
	}
}

#ifdef _WIN32
static bool Is8bppFullScreen(const IVideoMode& mode)
{	return mode.getBitsPerPixel() == 8 && mode.isFullScreen();	}
#endif


//
// ISDL12VideoCapabilities::ISDL12VideoCapabilities
//
// Discovers the native desktop resolution and queries SDL for a list of
// supported fullscreen video modes.
//
// NOTE: discovering the native desktop resolution only works if this is called
// prior to the first SDL_SetVideoMode call!
//
// NOTE: SDL has palette issues in 8bpp fullscreen mode on Windows platforms. As
// a workaround, we force a 32bpp resolution in this case by removing all
// fullscreen 8bpp video modes on Windows platforms.
//
ISDL12VideoCapabilities::ISDL12VideoCapabilities() :
	IVideoCapabilities(),
	mNativeMode(SDL_GetVideoInfo()->current_w, SDL_GetVideoInfo()->current_h,
				SDL_GetVideoInfo()->vfmt->BitsPerPixel, true)
{
	I_AddSDL12VideoModes(&mModeList, 8);
	I_AddSDL12VideoModes(&mModeList, 32);

	// always add the following windowed modes (if windowed modes are supported)
	if (supportsWindowed())
	{
		if (supports8bpp())
		{
			mModeList.push_back(IVideoMode(320, 200, 8, false));
			mModeList.push_back(IVideoMode(320, 240, 8, false));
			mModeList.push_back(IVideoMode(640, 400, 8, false));
			mModeList.push_back(IVideoMode(640, 480, 8, false));
		}
		if (supports32bpp())
		{
			mModeList.push_back(IVideoMode(320, 200, 32, false));
			mModeList.push_back(IVideoMode(320, 240, 32, false));
			mModeList.push_back(IVideoMode(640, 400, 32, false));
			mModeList.push_back(IVideoMode(640, 480, 32, false));
		}
	}

	// reverse sort the modes
	std::sort(mModeList.begin(), mModeList.end(), std::greater<IVideoMode>());

	// get rid of any duplicates (SDL sometimes reports duplicates)
	mModeList.erase(std::unique(mModeList.begin(), mModeList.end()), mModeList.end());

	#ifdef _WIN32
	// fullscreen directx requires a 32-bit mode to fix broken palette
	// [Russell] - Use for gdi as well, fixes d2 map02 water
	// [SL] remove all fullscreen 8bpp modes
	mModeList.erase(std::remove_if(mModeList.begin(), mModeList.end(), Is8bppFullScreen), mModeList.end());
	#endif

	assert(supportsWindowed() || supportsFullScreen());
	assert(supports8bpp() || supports32bpp());
}



// ****************************************************************************

// ============================================================================
//
// ISDL12DirectWindowSurfaceManager implementation
//
// Helper class for IWindow to encapsulate the creation of a IWindowSurface
// primary surface and to assist in using it to refresh the window.
//
// ============================================================================

//
// ISDL12DirectWindowSurfaceManager::ISDL12DirectWindowSurfaceManager
//
ISDL12DirectWindowSurfaceManager::ISDL12DirectWindowSurfaceManager(
	uint16_t width, uint16_t height, const PixelFormat* format)
{
	SDL_Surface* sdl_surface = SDL_GetVideoSurface();
	assert(sdl_surface != NULL);
	mSurface = new IWindowSurface(width, height, format, sdl_surface->pixels, sdl_surface->pitch);
	assert(mSurface != NULL);
}


//
// ISDL12DirectWindowSurfaceManager::~ISDL12DirectWindowSurfaceManager
//
ISDL12DirectWindowSurfaceManager::~ISDL12DirectWindowSurfaceManager()
{
	delete mSurface;
}


//
// ISDL12DirectWindowSurfaceManager::lockSurface
//
void ISDL12DirectWindowSurfaceManager::lockSurface()
{
	SDL_Surface* sdl_surface = SDL_GetVideoSurface();
	assert(sdl_surface != NULL);
	if (SDL_MUSTLOCK(sdl_surface))
		SDL_LockSurface(sdl_surface);
}


//
// ISDL12DirectWindowSurfaceManager::unlockSurface
//
void ISDL12DirectWindowSurfaceManager::unlockSurface()
{
	SDL_Surface* sdl_surface = SDL_GetVideoSurface();
	assert(sdl_surface != NULL);
	if (SDL_MUSTLOCK(sdl_surface))
		SDL_UnlockSurface(sdl_surface);
}


//
// ISDL12DirectWindowSurfaceManager::startRefresh
//
void ISDL12DirectWindowSurfaceManager::startRefresh()
{ }


//
// ISDL12DirectWindowSurfaceManager::finishRefresh
//
void ISDL12DirectWindowSurfaceManager::finishRefresh()
{
	SDL_Surface* sdl_surface = SDL_GetVideoSurface();
	assert(sdl_surface != NULL);
	SDL_Flip(sdl_surface);
}


// ============================================================================
//
// ISDL12SoftwareWindowSurfaceManager implementation
//
// Helper class for IWindow to encapsulate the creation of a IWindowSurface
// primary surface and to assist in using it to refresh the window.
//
//
// ============================================================================

//
// ISDL12SoftwareWindowSurfaceManager::ISDL12SoftwareWindowSurfaceManager
//
ISDL12SoftwareWindowSurfaceManager::ISDL12SoftwareWindowSurfaceManager(
	uint16_t width, uint16_t height, const PixelFormat* format)
{
	mSurface = new IWindowSurface(width, height, format);
	assert(mSurface != NULL);

	uint32_t rmask = uint32_t(format->getRMax()) << format->getRShift();
	uint32_t gmask = uint32_t(format->getGMax()) << format->getGShift();
	uint32_t bmask = uint32_t(format->getBMax()) << format->getBShift();

	mSDLSoftwareSurface = SDL_CreateRGBSurfaceFrom(
				mSurface->getBuffer(),
				mSurface->getWidth(), mSurface->getHeight(),
				mSurface->getBitsPerPixel(),
				mSurface->getPitch(),
				rmask, gmask, bmask, 0);

	assert(mSDLSoftwareSurface != NULL);

	SDL_Surface* sdl_surface = SDL_GetVideoSurface();
	assert(mSDLSoftwareSurface->format->Rmask == sdl_surface->format->Rmask &&
			mSDLSoftwareSurface->format->Gmask == sdl_surface->format->Gmask &&
			mSDLSoftwareSurface->format->Bmask == sdl_surface->format->Bmask);
}


//
// ISDL12SoftwareWindowSurfaceManager::~ISDL12SoftwareWindowSurfaceManager
//
ISDL12SoftwareWindowSurfaceManager::~ISDL12SoftwareWindowSurfaceManager()
{
	if (mSDLSoftwareSurface)
		SDL_FreeSurface(mSDLSoftwareSurface);

	delete mSurface;
}


//
// ISDL12SoftwareWindowSurfaceManager::lockSurface
//
void ISDL12SoftwareWindowSurfaceManager::lockSurface()
{
	SDL_Surface* sdl_surface = SDL_GetVideoSurface();
	if (SDL_MUSTLOCK(sdl_surface))
		SDL_LockSurface(sdl_surface);
	if (SDL_MUSTLOCK(mSDLSoftwareSurface))
		SDL_LockSurface(mSDLSoftwareSurface);
}


//
// ISDL12SoftwareWindowSurfaceManager::unlockSurface
//
void ISDL12SoftwareWindowSurfaceManager::unlockSurface()
{
	SDL_Surface* sdl_surface = SDL_GetVideoSurface();
	if (SDL_MUSTLOCK(sdl_surface))
		SDL_UnlockSurface(sdl_surface);
	if (SDL_MUSTLOCK(mSDLSoftwareSurface))
		SDL_UnlockSurface(mSDLSoftwareSurface);
}


//
// ISDL12SoftwareWindowSurfaceManager::startRefresh
//
void ISDL12SoftwareWindowSurfaceManager::startRefresh()
{
}


//
// ISDL12SoftwareWindowSurfaceManager::finishRefresh
//
void ISDL12SoftwareWindowSurfaceManager::finishRefresh()
{
	SDL_Surface* main_sdl_surface = SDL_GetVideoSurface();
	assert(main_sdl_surface != NULL);
	SDL_BlitSurface(mSDLSoftwareSurface, NULL, main_sdl_surface, NULL);
	SDL_Flip(main_sdl_surface);
}




// ============================================================================
//
// ISDL12Window class implementation
//
// ============================================================================


//
// ISDL12Window::ISDL12Window (if windowed modes are supported)
//
// Constructs a new application window using SDL 1.2.
// A ISDL12WindowSurface object is instantiated for frame rendering.
//
ISDL12Window::ISDL12Window(uint16_t width, uint16_t height, uint8_t bpp, bool fullscreen, bool vsync) :
	IWindow(),
	mSurfaceManager(NULL),
	mWidth(0), mHeight(0), mBitsPerPixel(0), mVideoMode(0, 0, 0, false),
	mIsFullScreen(fullscreen), mUseVSync(vsync),
	mNeedPaletteRefresh(true), mBlit(true), mIgnoreResize(false), mLocks(0)
{ }


//
// ISDL12Window::~ISDL12Window
//
ISDL12Window::~ISDL12Window()
{
	delete mSurfaceManager;
}


//
// ISDL12Window::lockSurface
//
// Locks the surface for direct pixel access. This must be called prior to
// accessing the primary surface's buffer.
//
void ISDL12Window::lockSurface()
{
	if (++mLocks == 1)
		mSurfaceManager->lockSurface();

	assert(mLocks >= 1 && mLocks < 100);
}


//
// ISDL12Window::unlockSurface
//
// Unlocks the surface after direct pixel access. This must be called after
// accessing the primary surface's buffer.
//
void ISDL12Window::unlockSurface()
{
	if (--mLocks == 0)
		mSurfaceManager->unlockSurface();

	assert(mLocks >= 0 && mLocks < 100);
}


//
// ISDL12Window::getEvents
//
// Retrieves events for the application window and processes them.
//
void ISDL12Window::getEvents()
{
	// Force SDL to gather events from input devices. This is called
	// implicitly from SDL_PollEvent but since we're using SDL_PeepEvents to
	// process only mouse events, SDL_PumpEvents is necessary.
	SDL_PumpEvents();

	// Retrieve chunks of up to 1024 events from SDL
	int num_events = 0;
	const int max_events = 1024;
	SDL_Event sdl_events[max_events];

	// set mask to get all events except keyboard, mouse, and joystick events
	const int event_mask = SDL_ALLEVENTS & ~SDL_KEYEVENTMASK & ~SDL_MOUSEEVENTMASK & ~SDL_JOYEVENTMASK;

	while ((num_events = SDL_PeepEvents(sdl_events, max_events, SDL_GETEVENT, event_mask)))
	{
		for (int i = 0; i < num_events; i++)
		{
			const SDL_Event& sdl_ev = sdl_events[i];

			if (sdl_ev.type == SDL_QUIT)
			{
				AddCommandString("quit");
			}
			else if (sdl_ev.type == SDL_VIDEORESIZE)
			{
				// Resizable window mode resolutions
				if (!vid_fullscreen && !mIgnoreResize)
				{
					char tmp[256];
					sprintf(tmp, "vid_setmode %i %i", sdl_ev.resize.w, sdl_ev.resize.h);
					AddCommandString(tmp);
				}
			}
			else if (sdl_ev.type == SDL_ACTIVEEVENT)
			{
				// Debugging messages
				if (sdl_ev.active.state & SDL_APPMOUSEFOCUS)
					DPrintf("SDL_ACTIVEEVENT SDL_APPMOUSEFOCUS %s\n", sdl_ev.active.gain ? "gained" : "lost");
				if (sdl_ev.active.state & SDL_APPINPUTFOCUS)
					DPrintf("SDL_ACTIVEEVENT SDL_APPINPUTFOCUS %s\n", sdl_ev.active.gain ? "gained" : "lost");
				if (sdl_ev.active.state & SDL_APPACTIVE)
					DPrintf("SDL_ACTIVEEVENT SDL_APPACTIVE %s\n", sdl_ev.active.gain ? "gained" : "lost");

				// TODO: do we need to do anything here anymore?
			}
		}
	}

	mIgnoreResize = false;
}


//
// ISDL12Window::startRefresh
//
void ISDL12Window::startRefresh()
{
	getEvents();

	mSurfaceManager->startRefresh();
}


//
// ISDL12Window::finishRefresh
//
void ISDL12Window::finishRefresh()
{
	assert(mLocks == 0);		// window surface shouldn't be locked when blitting

	SDL_Surface* sdlsurface = SDL_GetVideoSurface();

	if (mNeedPaletteRefresh)
	{
		Uint32 flags = SDL_LOGPAL | SDL_PHYSPAL;

		if (sdlsurface->format->BitsPerPixel == 8)
			SDL_SetPalette(sdlsurface, flags, sdlsurface->format->palette->colors, 0, 256);
//		if (mSDLSoftwareSurface && mSDLSoftwareSurface->format->BitsPerPixel == 8)
//			SDL_SetPalette(mSDLSoftwareSurface, flags, mSDLSoftwareSurface->format->palette->colors, 0, 256);
	}

	mNeedPaletteRefresh = false;

	if (mBlit)
	{
		mSurfaceManager->finishRefresh();
	}
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
// ISDL12Window::setWindowIcon
//
// Sets the icon for the application window, which will appear in the
// window manager's task list.
//
void ISDL12Window::setWindowIcon()
{
	#if defined(_WIN32) && !defined(_XBOX)
	// [SL] Use Win32-specific code to make use of multiple-icon sizes
	// stored in the executable resources. SDL 1.2 only allows a fixed
	// 32x32 px icon.
	//
	// [Russell] - Just for windows, display the icon in the system menu and
	// alt-tab display

	// TODO: FIXME
	#if 0
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

	#else

/*
	texhandle_t icon_handle = texturemanager.getHandle("ICON", Texture::TEX_PNG);
	const Texture* icon_texture = texturemanager.getTexture(icon_handle);
	const int icon_width = icon_texture->getWidth(), icon_height = icon_texture->getHeight();

	SDL_Surface* icon_sdlsurface = SDL_CreateRGBSurface(0, icon_width, icon_height, 8, 0, 0, 0, 0);
	Res_TransposeImage((byte*)icon_sdlsurface->pixels, icon_texture->getData(), icon_width, icon_height);

	SDL_SetColorKey(icon_sdlsurface, SDL_SRCCOLORKEY, 0);

	// set the surface palette
	const argb_t* palette_colors = V_GetDefaultPalette()->basecolors;
	SDL_Color* sdlcolors = icon_sdlsurface->format->palette->colors;
	for (int c = 0; c < 256; c++)
	{
		sdlcolors[c].r = palette_colors[c].r;
		sdlcolors[c].g = palette_colors[c].g;
		sdlcolors[c].b = palette_colors[c].b;
	}

	SDL_WM_SetIcon(icon_sdlsurface, NULL);
	SDL_FreeSurface(icon_sdlsurface);
*/

	#endif
}


//
// ISDL12Window::isFocused
//
// Returns true if this window has input focus.
//
bool ISDL12Window::isFocused() const
{
	SDL_PumpEvents();
	int app_state = SDL_GetAppState();
	return (app_state & SDL_APPINPUTFOCUS) && (app_state & SDL_APPACTIVE);
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
// I_SetSDL12Palette
//
// Helper function for ISDL12Window::setPalette
//
static void I_SetSDL12Palette(SDL_Surface* sdlsurface, const argb_t* palette_colors)
{
	if (sdlsurface->format->BitsPerPixel == 8)
	{
		assert(sdlsurface->format->palette != NULL);
		assert(sdlsurface->format->palette->ncolors == 256);

		SDL_Color* sdlcolors = sdlsurface->format->palette->colors;
		for (int c = 0; c < 256; c++)
		{
			argb_t color = palette_colors[c];
			sdlcolors[c].r = color.getr();
			sdlcolors[c].g = color.getg();
			sdlcolors[c].b = color.getb();
		}
	}
}


//
// ISDL12Window::setPalette
//
// Saves the given palette and updates it during refresh.
//
void ISDL12Window::setPalette(const argb_t* palette_colors)
{
	lockSurface();

	I_SetSDL12Palette(SDL_GetVideoSurface(), palette_colors);

//	if (mSDLSoftwareSurface)
//		I_SetSDL12Palette(mSDLSoftwareSurface, palette_colors);

	getPrimarySurface()->setPalette(palette_colors);

	mNeedPaletteRefresh = true;

	unlockSurface();
}


//
// I_BuildPixelFormatFromSDLSurface
//
// Helper function that extracts information about the pixel format
// from an SDL_Surface and uses it to initialize a PixelFormat object.
// Note: the SDL_Surface should be locked prior to calling this.
//
static void I_BuildPixelFormatFromSDLSurface(const SDL_Surface* sdlsurface, PixelFormat* format)
{
	const SDL_PixelFormat* sdlformat = sdlsurface->format;
	uint8_t bpp = sdlformat->BitsPerPixel;

	// handle SDL not reporting correct Ashift/Aloss
	uint8_t aloss = bpp == 32 ? 0 : 8;
	uint8_t ashift = bpp == 32 ?  48 - sdlformat->Rshift - sdlformat->Gshift - sdlformat->Bshift : 0;

	// Create the PixelFormat specification
	*format = PixelFormat(
			bpp,
			8 - aloss, 8 - sdlformat->Rloss, 8 - sdlformat->Gloss, 8 - sdlformat->Bloss,
			ashift, sdlformat->Rshift, sdlformat->Gshift, sdlformat->Bshift);
}


//
// ISDL12Window::getPixelFormat
//
const PixelFormat* ISDL12Window::getPixelFormat() const
{
    static PixelFormat format;
    const SDL_Surface* sdl_surface = SDL_GetVideoSurface();
	I_BuildPixelFormatFromSDLSurface(sdl_surface, &format);
    return &format;
}


//
// ISDL12Window::setMode
//
// Sets the window size to the specified size and frees the existing primary
// surface before instantiating a new primary surface. This function performs
// no sanity checks on the desired video mode.
//
// NOTE: If a hardware surface is obtained or the surface's screen pitch
// will create cache thrashing (tested by pitch & 511 == 0), a SDL software
// surface will be created and used for drawing video frames. This software
// surface is then blitted to the screen at the end of the frame, prior to
// calling SDL_Flip.
//
bool ISDL12Window::setMode(uint16_t video_width, uint16_t video_height, uint8_t video_bpp,
							bool video_fullscreen, bool vsync)
{
	uint32_t flags = 0;

	if (vsync)
		flags |= SDL_HWSURFACE | SDL_DOUBLEBUF;
	else
		flags |= SDL_SWSURFACE;

	if (video_fullscreen)
		flags |= SDL_FULLSCREEN;
	else
		flags |= SDL_RESIZABLE;

	if (video_fullscreen && video_bpp == 8)
		flags |= SDL_HWPALETTE;

	// TODO: check for multicore
	flags |= SDL_ASYNCBLIT;

	//if (video_fullscreen)
	//	flags = ((flags & (~SDL_SWSURFACE)) | SDL_HWSURFACE);

	#ifdef SDL_GL_SWAP_CONTROL
	SDL_GL_SetAttribute(SDL_GL_SWAP_CONTROL, vsync);
	#endif

	// [SL] SDL_SetVideoMode reinitializes DirectInput if DirectX is being used.
	// This interferes with RawWin32Mouse's input handlers so we need to
	// disable them prior to reinitalizing DirectInput...
	I_PauseMouse();

	SDL_Surface* sdl_surface = SDL_SetVideoMode(video_width, video_height, video_bpp, flags);
	if (sdl_surface == NULL)
	{
		I_FatalError("I_SetVideoMode: unable to set video mode %ux%ux%u (%s): %s\n",
				video_width, video_height, video_bpp, video_fullscreen ? "fullscreen" : "windowed",
				SDL_GetError());
		return false;
	}

	assert(sdl_surface == SDL_GetVideoSurface());

	// [SL] ...and re-enable RawWin32Mouse's input handlers after
	// DirectInput is reinitalized.
	I_ResumeMouse();

	const PixelFormat* format = getPixelFormat();

	// just in case SDL couldn't set the exact video mode we asked for...
	mWidth = sdl_surface->w;
	mHeight = sdl_surface->h;
	mBitsPerPixel = format->getBitsPerPixel();
	mIsFullScreen = (sdl_surface->flags & SDL_FULLSCREEN) == SDL_FULLSCREEN;
	mUseVSync = vsync;

	if (SDL_MUSTLOCK(sdl_surface))
		SDL_LockSurface(sdl_surface);		// lock prior to accessing pixel format

	delete mSurfaceManager;

	bool got_hardware_surface = (sdl_surface->flags & SDL_HWSURFACE) == SDL_HWSURFACE;

	bool create_software_surface =
					(sdl_surface->pitch & 511) == 0 ||	// pitch is a multiple of 512 (thrashes the cache)
					got_hardware_surface;				// drawing directly to hardware surfaces is slower

	if (create_software_surface)
		mSurfaceManager = new ISDL12SoftwareWindowSurfaceManager(mWidth, mHeight, format);
	else
		mSurfaceManager = new ISDL12DirectWindowSurfaceManager(mWidth, mHeight, format);

	assert(mSurfaceManager != NULL);
	assert(getPrimarySurface() != NULL);

	if (SDL_MUSTLOCK(sdl_surface))
		SDL_UnlockSurface(sdl_surface);

	mVideoMode = IVideoMode(mWidth, mHeight, mBitsPerPixel, mIsFullScreen);

	assert(mWidth >= 0 && mWidth <= MAXWIDTH);
	assert(mHeight >= 0 && mHeight <= MAXHEIGHT);
	assert(mBitsPerPixel == 8 || mBitsPerPixel == 32);

	// Tell argb_t the pixel format
	if (format->getBitsPerPixel() == 32)
		argb_t::setChannels(format->getAPos(), format->getRPos(), format->getGPos(), format->getBPos());
	else
		argb_t::setChannels(3, 2, 1, 0);

	// [SL] SDL can create SDL_VIDEORESIZE events in response to SDL_SetVideoMode
	// and we need to filter those out.
	mIgnoreResize = true;

	return true;
}


// ****************************************************************************

// ============================================================================
//
// ISDL12VideoSubsystem class implementation
//
// ============================================================================


//
// ISDL12VideoSubsystem::ISDL12VideoSubsystem
//
// Initializes SDL video and sets a few SDL configuration options.
//
ISDL12VideoSubsystem::ISDL12VideoSubsystem() : IVideoSubsystem()
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

	mVideoCapabilities = new ISDL12VideoCapabilities();

	mWindow = new ISDL12Window(640, 480, 8, false, false);
}


//
// ISDL12VideoSubsystem::~ISDL12VideoSubsystem
//
ISDL12VideoSubsystem::~ISDL12VideoSubsystem()
{
	delete mWindow;
	delete mVideoCapabilities;

    SDL_QuitSubSystem(SDL_INIT_VIDEO);
}
#endif	// SDL12



#ifdef SDL20
// ============================================================================
//
// ISDL20VideoCapabilities class implementation
//
// ============================================================================

//
// I_AddSDL20VideoModes
//
// Queries SDL to find the supported video modes at the given bit depth
// and then adds them to modelist.
//
static void I_AddSDL20VideoModes(IVideoModeList* modelist, int bpp)
{
	int display_index = 0;
	SDL_DisplayMode mode = { SDL_PIXELFORMAT_UNKNOWN, 0, 0, 0, 0 };

	int display_mode_count = SDL_GetNumDisplayModes(display_index);
	if (display_mode_count < 1)
	{
		I_FatalError("SDL_GetNumDisplayModes failed: %s", SDL_GetError());
		return;
	}

	for (int i = 0; i < display_mode_count; i++)
	{
		if (SDL_GetDisplayMode(display_index, i, &mode) != 0)
		{
			I_FatalError("SDL_GetDisplayMode failed: %s", SDL_GetError());
			return;
		}

		int width = mode.w, height = mode.h;
		// add this video mode to the list (both fullscreen & windowed)
		modelist->push_back(IVideoMode(width, height, bpp, false));
		modelist->push_back(IVideoMode(width, height, bpp, true));
	}
}

#ifdef _WIN32
static bool Is8bppFullScreen(const IVideoMode& mode)
{	return mode.getBitsPerPixel() == 8 && mode.isFullScreen();	}
#endif


//
// ISDL20VideoCapabilities::ISDL20VideoCapabilities
//
// Discovers the native desktop resolution and queries SDL for a list of
// supported fullscreen video modes.
//
// NOTE: discovering the native desktop resolution only works if this is called
// prior to the first SDL_SetVideoMode call!
//
// NOTE: SDL has palette issues in 8bpp fullscreen mode on Windows platforms. As
// a workaround, we force a 32bpp resolution in this case by removing all
// fullscreen 8bpp video modes on Windows platforms.
//
ISDL20VideoCapabilities::ISDL20VideoCapabilities() :
	IVideoCapabilities(),
	mNativeMode(0, 0, 0, false)
{
	const int display_index = 0;
	int bpp;
	Uint32 Rmask,Gmask,Bmask,Amask;

	SDL_DisplayMode sdl_display_mode;
	if (SDL_GetDesktopDisplayMode(display_index, &sdl_display_mode) != 0)
	{
		I_FatalError("SDL_GetDesktopDisplayMode failed: %s", SDL_GetError());
		return;
	}

    // SDL_BITSPERPIXEL appears to not translate the mode properly, sometimes it reports
	// a 24bpp mode when it is actually 32bpp, this function clears that up
	SDL_PixelFormatEnumToMasks(sdl_display_mode.format,&bpp,&Rmask,&Gmask,&Bmask,&Amask);

	mNativeMode = IVideoMode(sdl_display_mode.w, sdl_display_mode.h, bpp, true);

	I_AddSDL20VideoModes(&mModeList, 8);
	I_AddSDL20VideoModes(&mModeList, 32);

	// always add the following windowed modes (if windowed modes are supported)
	if (supportsWindowed())
	{
		if (supports8bpp())
		{
			mModeList.push_back(IVideoMode(320, 200, 8, false));
			mModeList.push_back(IVideoMode(320, 240, 8, false));
			mModeList.push_back(IVideoMode(640, 400, 8, false));
			mModeList.push_back(IVideoMode(640, 480, 8, false));
		}
		if (supports32bpp())
		{
			mModeList.push_back(IVideoMode(320, 200, 32, false));
			mModeList.push_back(IVideoMode(320, 240, 32, false));
			mModeList.push_back(IVideoMode(640, 400, 32, false));
			mModeList.push_back(IVideoMode(640, 480, 32, false));
		}
	}

	// reverse sort the modes
	std::sort(mModeList.begin(), mModeList.end(), std::greater<IVideoMode>());

	// get rid of any duplicates (SDL sometimes reports duplicates)
	mModeList.erase(std::unique(mModeList.begin(), mModeList.end()), mModeList.end());

	#ifdef _WIN32
	// fullscreen directx requires a 32-bit mode to fix broken palette
	// [Russell] - Use for gdi as well, fixes d2 map02 water
	// [SL] remove all fullscreen 8bpp modes
	mModeList.erase(std::remove_if(mModeList.begin(), mModeList.end(), Is8bppFullScreen), mModeList.end());
	#endif

	assert(supportsWindowed() || supportsFullScreen());
	assert(supports8bpp() || supports32bpp());
}



// ****************************************************************************

// ============================================================================
//
// ISDL20TextureWindowSurfaceManager implementation
//
// Helper class for IWindow to encapsulate the creation of a IWindowSurface
// primary surface and to assist in using it to refresh the window.
//
// ============================================================================

//
// ISDL20TextureWindowSurfaceManager::ISDL20TextureWindowSurfaceManager
//
ISDL20TextureWindowSurfaceManager::ISDL20TextureWindowSurfaceManager(
	uint16_t width, uint16_t height, const PixelFormat* format, ISDL20Window* window, bool vsync) :
		mWindow(window),
		mSDLRenderer(NULL), mSDLTexture(NULL),
		mSurface(NULL), m8bppTo32BppSurface(NULL),
        mWidth(width), mHeight(height)
{
	assert(mWindow != NULL);
    assert(mWindow->mSDLWindow != NULL);

	memcpy(&mFormat, format, sizeof(mFormat));

    // Select the best RENDER_SCALE_QUALITY that is supported
    const char* scale_hints[] = {"best", "linear", "nearest", ""};
    for (int i = 0; scale_hints[i][0] != '\0'; i++)
    {
        if (SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, scale_hints[i]))
            break;
    }

	uint32_t renderer_flags = SDL_RENDERER_ACCELERATED;
	if (vsync)
		renderer_flags |= SDL_RENDERER_PRESENTVSYNC;

	mSDLRenderer = SDL_CreateRenderer(mWindow->mSDLWindow, -1, renderer_flags);

	if (mSDLRenderer == NULL)
		I_FatalError("I_InitVideo: unable to create SDL2 renderer: %s\n", SDL_GetError());

	// Ensure the game window is clear, even if using -noblit
	SDL_SetRenderDrawColor(mSDLRenderer, 0, 0, 0, 255);
	SDL_RenderClear(mSDLRenderer);
	SDL_RenderPresent(mSDLRenderer);

	uint32_t texture_flags = SDL_TEXTUREACCESS_STREAMING;

    SDL_DisplayMode sdl_mode;
    SDL_GetWindowDisplayMode(mWindow->mSDLWindow, &sdl_mode);

	mSDLTexture = SDL_CreateTexture(
				mSDLRenderer,
				sdl_mode.format,
				texture_flags,
				mWidth, mHeight);

	if (mSDLTexture == NULL)
		I_FatalError("I_InitVideo: unable to create SDL2 texture: %s\n", SDL_GetError());

	mSurface = new IWindowSurface(width, height, &mFormat);
    if (mSurface->getBitsPerPixel() ==8)
        m8bppTo32BppSurface = new IWindowSurface(width, height, mWindow->getPixelFormat());
}


//
// ISDL20TextureWindowSurfaceManager::~ISDL20TextureWindowSurfaceManager
//
ISDL20TextureWindowSurfaceManager::~ISDL20TextureWindowSurfaceManager()
{
    delete m8bppTo32BppSurface;
	delete mSurface;

	if (mSDLTexture)
		SDL_DestroyTexture(mSDLTexture);
	if (mSDLRenderer)
		SDL_DestroyRenderer(mSDLRenderer);
}


//
// ISDL20TextureWindowSurfaceManager::lockSurface
//
void ISDL20TextureWindowSurfaceManager::lockSurface()
{ }


//
// ISDL20TextureWindowSurfaceManager::unlockSurface
//
void ISDL20TextureWindowSurfaceManager::unlockSurface()
{ }


//
// ISDL20TextureWindowSurfaceManager::startRefresh
//
void ISDL20TextureWindowSurfaceManager::startRefresh()
{ }


//
// ISDL20TextureWindowSurfaceManager::finishRefresh
//
void ISDL20TextureWindowSurfaceManager::finishRefresh()
{
    if (mSurface->getBitsPerPixel() == 8)
    {
        m8bppTo32BppSurface->blit(mSurface, 0, 0, mSurface->getWidth(), mSurface->getHeight(),
                0, 0, m8bppTo32BppSurface->getWidth(), m8bppTo32BppSurface->getHeight());
	    SDL_UpdateTexture(mSDLTexture, NULL, m8bppTo32BppSurface->getBuffer(), m8bppTo32BppSurface->getPitch());
    }
    else
    {
	   SDL_UpdateTexture(mSDLTexture, NULL, mSurface->getBuffer(), mSurface->getPitch());
    }
	SDL_RenderCopy(mSDLRenderer, mSDLTexture, NULL, NULL);
	SDL_RenderPresent(mSDLRenderer);
}




// ============================================================================
//
// ISDL20Window class implementation
//
// ============================================================================


//
// ISDL20Window::ISDL20Window (if windowed modes are supported)
//
// Constructs a new application window using SDL 1.2.
// A ISDL20WindowSurface object is instantiated for frame rendering.
//
ISDL20Window::ISDL20Window(uint16_t width, uint16_t height, uint8_t bpp, bool fullscreen, bool vsync) :
	IWindow(),
	mSDLWindow(NULL), mSurfaceManager(NULL),
	mWidth(0), mHeight(0), mBitsPerPixel(0), mVideoMode(0, 0, 0, false),
	mIsFullScreen(fullscreen), mUseVSync(vsync),
	mNeedPaletteRefresh(true), mBlit(true),
	mMouseFocus(false), mKeyboardFocus(false),
	mLocks(0)
{
	setRendererDriver();
	const char* driver_name = getRendererDriver();
	Printf(PRINT_HIGH, "V_Init: rendering mode \"%s\"\n", driver_name);

	uint32_t window_flags = SDL_WINDOW_SHOWN;

	// Reduce the flickering on start up for the opengl driver on Windows
	#ifdef _WIN32
	if (strncmp(driver_name, "opengl", strlen(driver_name)) == 0)
		window_flags |= SDL_WINDOW_OPENGL;
	#endif

	if (fullscreen)
		window_flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
	else
		window_flags |= SDL_WINDOW_RESIZABLE;

	mSDLWindow = SDL_CreateWindow(
			"",			// Empty title for now - it will be set later
			SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
			width, height,
			window_flags);

	if (mSDLWindow == NULL)
		I_FatalError("I_InitVideo: unable to create window: %s\n", SDL_GetError());

    discoverNativePixelFormat();

	mWidth = width;
	mHeight = height;
    mBitsPerPixel = bpp;

	mMouseFocus = mKeyboardFocus = true;
}


//
// ISDL20Window::~ISDL20Window
//
ISDL20Window::~ISDL20Window()
{
	delete mSurfaceManager;

	if (mSDLWindow)
		SDL_DestroyWindow(mSDLWindow);
}


//
// ISDL20Window::setRendererDriver
//
void ISDL20Window::setRendererDriver()
{
    const char* drivers[] = {"direct3d", "opengl", "opengles2", "opengles", "software", ""};
    for (int i = 0; drivers[i][0] != '\0'; i++)
    {
        if (SDL_SetHint(SDL_HINT_RENDER_DRIVER, drivers[i]))
            break;
    }
}


//
// ISDL20Window::getRendererDriver
//
const char* ISDL20Window::getRendererDriver() const
{
    static char driver_name[20];
    memset(driver_name, 0, sizeof(driver_name));
    const char* hint_value = SDL_GetHint(SDL_HINT_RENDER_DRIVER);
    if (hint_value)
        strncpy(driver_name, hint_value, sizeof(driver_name) - 1);
    return driver_name;
}


//
// ISDL20Window::lockSurface
//
// Locks the surface for direct pixel access. This must be called prior to
// accessing the primary surface's buffer.
//
void ISDL20Window::lockSurface()
{
	if (++mLocks == 1)
		mSurfaceManager->lockSurface();

	assert(mLocks >= 1 && mLocks < 100);
}


//
// ISDL20Window::unlockSurface
//
// Unlocks the surface after direct pixel access. This must be called after
// accessing the primary surface's buffer.
//
void ISDL20Window::unlockSurface()
{
	if (--mLocks == 0)
		mSurfaceManager->unlockSurface();

	assert(mLocks >= 0 && mLocks < 100);
}


//
// ISDL20Window::getEvents
//
// Retrieves events for the application window and processes them.
//
void ISDL20Window::getEvents()
{
	// Force SDL to gather events from input devices. This is called
	// implicitly from SDL_PollEvent but since we're using SDL_PeepEvents to
	// process only mouse events, SDL_PumpEvents is necessary.
	SDL_PumpEvents();

	// Retrieve chunks of up to 1024 events from SDL
	int num_events = 0;
	const int max_events = 1024;
	SDL_Event sdl_events[max_events];

	while ((num_events = SDL_PeepEvents(sdl_events, max_events, SDL_GETEVENT, SDL_QUIT, SDL_SYSWMEVENT)))
	{
		for (int i = 0; i < num_events; i++)
		{
			const SDL_Event& sdl_ev = sdl_events[i];

			if (sdl_ev.type == SDL_QUIT ||
				(sdl_ev.type == SDL_WINDOWEVENT && sdl_ev.window.event == SDL_WINDOWEVENT_CLOSE))
			{
				AddCommandString("quit");
			}
			else if (sdl_ev.type == SDL_WINDOWEVENT)
			{
				if (sdl_ev.window.event == SDL_WINDOWEVENT_SHOWN)
				{
					DPrintf("SDL_WINDOWEVENT_SHOWN\n");
				}
				else if (sdl_ev.window.event == SDL_WINDOWEVENT_HIDDEN)
				{
					DPrintf("SDL_WINDOWEVENT_HIDDEN\n");
				}
				else if (sdl_ev.window.event == SDL_WINDOWEVENT_EXPOSED)
				{
					DPrintf("SDL_WINDOWEVENT_EXPOSED\n");
				}
				else if (sdl_ev.window.event == SDL_WINDOWEVENT_MINIMIZED)
				{
					DPrintf("SDL_WINDOWEVENT_MINIMIZED\n");
				}
				else if (sdl_ev.window.event == SDL_WINDOWEVENT_MAXIMIZED)
				{
					DPrintf("SDL_WINDOWEVENT_MAXIMIZED\n");
				}
				else if (sdl_ev.window.event == SDL_WINDOWEVENT_RESTORED)
				{
					DPrintf("SDL_WINDOWEVENT_RESTORED\n");
				}
				else if (sdl_ev.window.event == SDL_WINDOWEVENT_ENTER)
				{
					DPrintf("SDL_WINDOWEVENT_ENTER\n");
					mMouseFocus = true;
				}
				else if (sdl_ev.window.event == SDL_WINDOWEVENT_LEAVE)
				{
					DPrintf("SDL_WINDOWEVENT_LEAVE\n");
					mMouseFocus = false;
				}
				else if (sdl_ev.window.event == SDL_WINDOWEVENT_FOCUS_GAINED)
				{
					DPrintf("SDL_WINDOWEVENT_FOCUS_GAINED\n");
					mKeyboardFocus = true;
				}
				else if (sdl_ev.window.event == SDL_WINDOWEVENT_FOCUS_LOST)
				{
					DPrintf("SDL_WINDOWEVENT_FOCUS_LOST\n");
					mKeyboardFocus = false;
				}
				else if (sdl_ev.window.event == SDL_WINDOWEVENT_RESIZED)
				{
					// Resizable window mode resolutions
					if (!vid_fullscreen)
					{
						char tmp[256];
						sprintf(tmp, "vid_setmode %i %i", sdl_ev.window.data1, sdl_ev.window.data2);
						AddCommandString(tmp);
					}
				}
			}
		}
	}
}


//
// ISDL20Window::startRefresh
//
void ISDL20Window::startRefresh()
{
	getEvents();
}


//
// ISDL20Window::finishRefresh
//
void ISDL20Window::finishRefresh()
{
	assert(mLocks == 0);		// window surface shouldn't be locked when blitting

	if (mNeedPaletteRefresh)
	{
//		if (sdlsurface->format->BitsPerPixel == 8)
//			SDL_SetSurfacePalette(sdlsurface, sdlsurface->format->palette);
	}

	mNeedPaletteRefresh = false;

	if (mBlit)
		mSurfaceManager->finishRefresh();
}


//
// ISDL20Window::setWindowTitle
//
// Sets the title caption of the window.
//
void ISDL20Window::setWindowTitle(const std::string& str)
{
	SDL_SetWindowTitle(mSDLWindow, str.c_str());
}


//
// ISDL20Window::setWindowIcon
//
// Sets the icon for the application window, which will appear in the
// window manager's task list.
//
void ISDL20Window::setWindowIcon()
{
	#if defined(_WIN32) && !defined(_XBOX)
	// [SL] Use Win32-specific code to make use of multiple-icon sizes
	// stored in the executable resources. SDL 1.2 only allows a fixed
	// 32x32 px icon.
	//
	// [Russell] - Just for windows, display the icon in the system menu and
	// alt-tab display

	// TODO: Add the icon resources to the wad file and load them with the
	// other code instead
	HICON hIcon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_ICON1));

	if (hIcon)
	{
		HWND WindowHandle;

		SDL_SysWMinfo wminfo;
		SDL_VERSION(&wminfo.version)
		SDL_GetWindowWMInfo(mSDLWindow, &wminfo);

		WindowHandle = wminfo.info.win.window;

		SendMessage(WindowHandle, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
		SendMessage(WindowHandle, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
	}

	#else

/*
	texhandle_t icon_handle = texturemanager.getHandle("ICON", Texture::TEX_PNG);
	const Texture* icon_texture = texturemanager.getTexture(icon_handle);
	const int icon_width = icon_texture->getWidth(), icon_height = icon_texture->getHeight();

	SDL_Surface* icon_sdlsurface = SDL_CreateRGBSurface(0, icon_width, icon_height, 8, 0, 0, 0, 0);
	Res_TransposeImage((byte*)icon_sdlsurface->pixels, icon_texture->getData(), icon_width, icon_height);

	SDL_SetColorKey(icon_sdlsurface, SDL_SRCCOLORKEY, 0);

	// set the surface palette
	const argb_t* palette_colors = V_GetDefaultPalette()->basecolors;
	SDL_Color* sdlcolors = icon_sdlsurface->format->palette->colors;
	for (int c = 0; c < 256; c++)
	{
		sdlcolors[c].r = palette_colors[c].r;
		sdlcolors[c].g = palette_colors[c].g;
		sdlcolors[c].b = palette_colors[c].b;
	}

	SDL_WM_SetIcon(icon_sdlsurface, NULL);
	SDL_FreeSurface(icon_sdlsurface);
*/

	#endif
}


//
// ISDL20Window::isFocused
//
// Returns true if this window has input focus.
//
bool ISDL20Window::isFocused() const
{
	return mMouseFocus && mKeyboardFocus;
}


//
// ISDL20Window::getVideoDriverName
//
// Returns the name of the video driver that SDL is currently
// configured to use.
//
std::string ISDL20Window::getVideoDriverName() const
{
	const char* driver_name = SDL_GetCurrentVideoDriver();
	if (driver_name)
		return std::string(driver_name);
	return "";
}


//
// I_SetSDL20Palette
//
// Helper function for ISDL20Window::setPalette
//
static void I_SetSDL20Palette(SDL_Surface* sdlsurface, const argb_t* palette_colors)
{
	if (sdlsurface->format->BitsPerPixel == 8)
	{
		assert(sdlsurface->format->palette != NULL);
		assert(sdlsurface->format->palette->ncolors == 256);

		SDL_Color* sdlcolors = sdlsurface->format->palette->colors;
		for (int c = 0; c < 256; c++)
		{
			argb_t color = palette_colors[c];
			sdlcolors[c].r = color.getr();
			sdlcolors[c].g = color.getg();
			sdlcolors[c].b = color.getb();
		}
	}
}


//
// ISDL20Window::setPalette
//
// Saves the given palette and updates it during refresh.
//
void ISDL20Window::setPalette(const argb_t* palette_colors)
{
	lockSurface();

	getPrimarySurface()->setPalette(palette_colors);

	mNeedPaletteRefresh = true;

	unlockSurface();
}


//
// I_BuildPixelFormatFromSDLPixelFormatEnum
//
// Helper function that extracts information about the pixel format
// from an SDL_PixelFormatEnum value and uses it to initialize a PixelFormat
// object.
//
static void I_BuildPixelFormatFromSDLPixelFormatEnum(uint32_t sdl_fmt, PixelFormat* format)
{
	uint32_t amask, rmask, gmask, bmask;
	int bpp;
	SDL_PixelFormatEnumToMasks(sdl_fmt, &bpp, &rmask, &gmask, &bmask, &amask);

	uint32_t rshift = 0, rloss = 8;
	for (uint32_t n = rmask; (n & 1) == 0; n >>= 1, rshift++) { }
	for (uint32_t n = rmask >> rshift; (n & 1) == 1; n >>= 1, rloss--) { }

	uint32_t gshift = 0, gloss = 8;
	for (uint32_t n = gmask; (n & 1) == 0; n >>= 1, gshift++) { }
	for (uint32_t n = gmask >> gshift; (n & 1) == 1; n >>= 1, gloss--) { }

	uint32_t bshift = 0, bloss = 8;
	for (uint32_t n = bmask; (n & 1) == 0; n >>= 1, bshift++) { }
	for (uint32_t n = bmask >> bshift; (n & 1) == 1; n >>= 1, bloss--) { }

	// handle SDL not reporting correct value for amask
	uint8_t ashift = bpp == 32 ?  48 - rshift - gshift - bshift : 0;
	uint8_t aloss = bpp == 32 ? 0 : 8;

	// Create the PixelFormat specification
	*format = PixelFormat(bpp, 8 - aloss, 8 - rloss, 8 - gloss, 8 - bloss, ashift, rshift, gshift, bshift);
}


//
// ISDL20Window::discoverNativePixelFormat
//
void ISDL20Window::discoverNativePixelFormat()
{
    SDL_DisplayMode sdl_mode;
    SDL_GetWindowDisplayMode(mSDLWindow, &sdl_mode);
    I_BuildPixelFormatFromSDLPixelFormatEnum(sdl_mode.format, &mPixelFormat);
}


//
// ISDL20Window::buildSurfacePixelFormat
//
// Creates a PixelFormat instance based on the user's requested BPP
// and the native desktop video mode.
//
PixelFormat ISDL20Window::buildSurfacePixelFormat(uint8_t bpp)
{
    uint8_t native_bpp = getPixelFormat()->getBitsPerPixel();
    if (bpp == 8)
        return PixelFormat(8, 0, 0, 0, 0, 0, 0, 0, 0);
    else if (bpp == 32 && native_bpp == 32)
        return *getPixelFormat();
    else
        I_Error("Invalid video surface conversion from %i-bit to %i-bit", bpp, native_bpp);
    return PixelFormat();   // shush warnings regarding no return value
}


//
// ISDL20Window::setMode
//
// Sets the window size to the specified size and frees the existing primary
// surface before instantiating a new primary surface. This function performs
// no sanity checks on the desired video mode.
//
// NOTE: If a hardware surface is obtained or the surface's screen pitch
// will create cache thrashing (tested by pitch & 511 == 0), a SDL software
// surface will be created and used for drawing video frames. This software
// surface is then blitted to the screen at the end of the frame, prior to
// calling SDL_Flip.
//
bool ISDL20Window::setMode(uint16_t video_width, uint16_t video_height, uint8_t video_bpp,
							bool video_fullscreen, bool vsync)
{
	// [SL] SDL_SetVideoMode reinitializes DirectInput if DirectX is being used.
	// This interferes with RawWin32Mouse's input handlers so we need to
	// disable them prior to reinitalizing DirectInput...
	I_PauseMouse();

	uint32_t fullscreen_flags = 0;
	if (video_fullscreen)
		fullscreen_flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
	SDL_SetWindowFullscreen(mSDLWindow, fullscreen_flags);

	SDL_SetWindowSize(mSDLWindow, video_width, video_height);
    SDL_SetWindowPosition(mSDLWindow, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);

	delete mSurfaceManager;

    PixelFormat format = buildSurfacePixelFormat(video_bpp);
	mSurfaceManager = new ISDL20TextureWindowSurfaceManager(
			video_width, video_height,
			&format,
			this,
			vsync);

	mWidth = video_width;
	mHeight = video_height;
    mBitsPerPixel = video_bpp;

	mIsFullScreen = SDL_GetWindowFlags(mSDLWindow) & (SDL_WINDOW_FULLSCREEN | SDL_WINDOW_FULLSCREEN_DESKTOP);
	mUseVSync = vsync;

	mVideoMode = IVideoMode(mWidth, mHeight, mBitsPerPixel, mIsFullScreen);

	assert(mWidth >= 0 && mWidth <= MAXWIDTH);
	assert(mHeight >= 0 && mHeight <= MAXHEIGHT);
	assert(mBitsPerPixel == 8 || mBitsPerPixel == 32);

	// Tell argb_t the pixel format
	if (format.getBitsPerPixel() == 32)
		argb_t::setChannels(format.getAPos(), format.getRPos(), format.getGPos(), format.getBPos());
	else
		argb_t::setChannels(3, 2, 1, 0);

	// [SL] ...and re-enable RawWin32Mouse's input handlers after
	// DirectInput is reinitalized.
	I_ResumeMouse();

	return true;
}


// ****************************************************************************

// ============================================================================
//
// ISDL20VideoSubsystem class implementation
//
// ============================================================================


//
// ISDL20VideoSubsystem::ISDL20VideoSubsystem
//
// Initializes SDL video and sets a few SDL configuration options.
//
ISDL20VideoSubsystem::ISDL20VideoSubsystem() : IVideoSubsystem()
{
	SDL_version linked, compiled;
	SDL_GetVersion(&linked);
	SDL_VERSION(&compiled);

	if (linked.major != compiled.major || linked.minor != compiled.minor)
	{
		I_FatalError("SDL version conflict (%d.%d.%d vs %d.%d.%d dll)\n",
			compiled.major, compiled.minor, compiled.patch,
			linked.major, linked.minor, linked.patch);
		return;
	}

	if (linked.patch != compiled.patch)
	{
		Printf_Bold("SDL version warning (%d.%d.%d vs %d.%d.%d dll)\n",
			compiled.major, compiled.minor, compiled.patch,
			linked.major, linked.minor, linked.patch);
	}

	if (SDL_InitSubSystem(SDL_INIT_VIDEO) == -1)
	{
		I_FatalError("Could not initialize SDL video.\n");
		return;
	}

	mVideoCapabilities = new ISDL20VideoCapabilities();

	mWindow = new ISDL20Window(640, 480, 8, false, false);
}


//
// ISDL20VideoSubsystem::~ISDL20VideoSubsystem
//
ISDL20VideoSubsystem::~ISDL20VideoSubsystem()
{
	delete mWindow;
	delete mVideoCapabilities;

    SDL_QuitSubSystem(SDL_INIT_VIDEO);
}
#endif	// SDL20

VERSION_CONTROL (i_sdlvideo_cpp, "$Id$")
