// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2006-2024 by The Odamex Team.
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
//	SDL 1.2 implementation of the IVideo class.
//
//-----------------------------------------------------------------------------


#include "odamex.h"

#include "i_video_sdl12.h"

#include "i_sdl.h"
#include <stdlib.h>
#include <cassert>

#include <algorithm>
#include <functional>

#include "i_video.h"
#include "v_video.h"

#include "v_palette.h"
#include "i_system.h"
#include "i_input.h"
#include "i_icon.h"

#include "c_dispatch.h"

// [Russell] - Just for windows, display the icon in the system menu and alt-tab display
#include "win32inc.h"
#if defined(_WIN32) && !defined(_XBOX)
    #include <SDL_syswm.h>
    #include "resource.h"
#endif // WIN32

#ifdef _XBOX
#include "i_xbox.h"
#endif


EXTERN_CVAR (vid_fullscreen)
EXTERN_CVAR (vid_widescreen)
EXTERN_CVAR (vid_pillarbox)


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

	SDL_Rect** sdlmodes = SDL_ListModes(&format, SDL_ANYFORMAT | SDL_FULLSCREEN | SDL_SWSURFACE);

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
				modelist->push_back(IVideoMode(width, height, bpp, WINDOW_Windowed));
				modelist->push_back(IVideoMode(width, height, bpp, WINDOW_Fullscreen));
			}
			++sdlmodes;
		}
	}
}

#ifdef _WIN32
static bool Is8bppFullScreen(const IVideoMode& mode)
{	return mode.bpp == 8 && mode.isFullScreen();	}
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
				SDL_GetVideoInfo()->vfmt->BitsPerPixel, WINDOW_Fullscreen)
{
	I_AddSDL12VideoModes(&mModeList, 8);
	I_AddSDL12VideoModes(&mModeList, 32);

	// always add the following windowed modes (if windowed modes are supported)
	if (supportsWindowed())
	{
		if (supports8bpp())
		{
			mModeList.push_back(IVideoMode(320, 200, 8, WINDOW_Windowed));
			mModeList.push_back(IVideoMode(320, 240, 8, WINDOW_Windowed));
			mModeList.push_back(IVideoMode(640, 400, 8, WINDOW_Windowed));
			mModeList.push_back(IVideoMode(640, 480, 8, WINDOW_Windowed));
		}
		if (supports32bpp())
		{
			mModeList.push_back(IVideoMode(320, 200, 32, WINDOW_Windowed));
			mModeList.push_back(IVideoMode(320, 240, 32, WINDOW_Windowed));
			mModeList.push_back(IVideoMode(640, 400, 32, WINDOW_Windowed));
			mModeList.push_back(IVideoMode(640, 480, 32, WINDOW_Windowed));
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
ISDL12Window::ISDL12Window(uint16_t width, uint16_t height, uint8_t bpp, EWindowMode window_mode, bool vsync) :
	IWindow(),
	mSurfaceManager(NULL),
	mVideoMode(0, 0, 0, WINDOW_Windowed),
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
				if ((EWindowMode)vid_fullscreen.asInt() == WINDOW_Windowed && !mIgnoreResize)
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

	HICON hIcon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_ICON1));

	if (hIcon)
	{
		SDL_SysWMinfo wminfo;
		SDL_VERSION(&wminfo.version)
		SDL_GetWMInfo(&wminfo);
		HWND WindowHandle = wminfo.window;

		if (WindowHandle)
		{
			SendMessage(WindowHandle, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
			SendMessage(WindowHandle, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
		}
	}
	#endif	// _WIN32 && !_XBOX
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
bool ISDL12Window::setMode(const IVideoMode& video_mode)
{
	bool is_windowed = video_mode.window_mode != WINDOW_Fullscreen;

	uint32_t flags = 0;

	if (video_mode.vsync)
		flags |= SDL_HWSURFACE | SDL_DOUBLEBUF;
	else
		flags |= SDL_SWSURFACE;

	if (is_windowed)
	{
		flags |= SDL_RESIZABLE;
	}
	else
	{
		flags |= SDL_FULLSCREEN;
		if (video_mode.bpp == 8)
			flags |= SDL_HWPALETTE;
	}

	flags |= SDL_ASYNCBLIT;

	#ifdef SDL_GL_SWAP_CONTROL
	SDL_GL_SetAttribute(SDL_GL_SWAP_CONTROL, video_mode.vsync);
	#endif

	SDL_Surface* sdl_surface = SDL_SetVideoMode(video_mode.width, video_mode.height, video_mode.bpp, flags);
	if (sdl_surface == NULL)
	{
		I_FatalError("I_SetVideoMode: unable to set video mode %ux%ux%u (%s): %s\n",
				video_mode.width, video_mode.height, video_mode.bpp, is_windowed ? "windowed" : "fullscreen",
				SDL_GetError());
		return false;
	}

	assert(sdl_surface == SDL_GetVideoSurface());

	const PixelFormat* format = getPixelFormat();

	// just in case SDL couldn't set the exact video mode we asked for...
	mVideoMode.width = sdl_surface->w;
	mVideoMode.height = sdl_surface->h;
	//mVideoMode.bpp = format->getBitsPerPixel();
	mVideoMode.bpp = video_mode.bpp;
	if ((sdl_surface->flags & SDL_FULLSCREEN) == SDL_FULLSCREEN)
		mVideoMode.window_mode = WINDOW_Fullscreen;
	else
		mVideoMode.window_mode = WINDOW_Windowed;
	mVideoMode.vsync = video_mode.vsync;
	mVideoMode.stretch_mode = video_mode.stretch_mode;

	if (SDL_MUSTLOCK(sdl_surface))
		SDL_LockSurface(sdl_surface);		// lock prior to accessing pixel format

	delete mSurfaceManager;

	bool got_hardware_surface = (sdl_surface->flags & SDL_HWSURFACE) == SDL_HWSURFACE;

	bool create_software_surface =
					(sdl_surface->pitch & 511) == 0 ||	// pitch is a multiple of 512 (thrashes the cache)
					got_hardware_surface;				// drawing directly to hardware surfaces is slower

	if (create_software_surface)
		mSurfaceManager = new ISDL12SoftwareWindowSurfaceManager(mVideoMode.width, mVideoMode.height, format);
	else
		mSurfaceManager = new ISDL12DirectWindowSurfaceManager(mVideoMode.width, mVideoMode.height, format);

	assert(mSurfaceManager != NULL);
	assert(getPrimarySurface() != NULL);

	if (SDL_MUSTLOCK(sdl_surface))
		SDL_UnlockSurface(sdl_surface);

	// Tell argb_t the pixel format
	if (format->getBitsPerPixel() == 32)
		argb_t::setChannels(format->getAPos(), format->getRPos(), format->getGPos(), format->getBPos());
	else
	{
#if defined(__SWITCH__)
		argb_t::setChannels(0, 3, 2, 1);
#else
		argb_t::setChannels(3, 2, 1, 0);
#endif
	}

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
		Printf(PRINT_WARNING, "SDL version warning (%d.%d.%d vs %d.%d.%d dll)\n",
			SDL_MAJOR_VERSION, SDL_MINOR_VERSION, SDL_PATCHLEVEL,
			SDLVersion->major, SDLVersion->minor, SDLVersion->patch);
	}

	if (SDL_InitSubSystem(SDL_INIT_VIDEO) == -1)
	{
		I_FatalError("Could not initialize SDL video.\n");
		return;
	}

	mVideoCapabilities = new ISDL12VideoCapabilities();

	mWindow = new ISDL12Window(640, 480, 8, WINDOW_Windowed, false);
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

VERSION_CONTROL (i_video_sdl12_cpp, "$Id$")
