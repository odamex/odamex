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
#include "i_input.h"

#include "m_argv.h"
#include "w_wad.h"

#include "res_texture.h"

#ifdef _XBOX
#include "i_xbox.h"
#endif


// ****************************************************************************

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
	mPrimarySurface(NULL),
	mWidth(0), mHeight(0), mBitsPerPixel(0), mVideoMode(0, 0, 0, false),
	mIsFullScreen(fullscreen), mUseVSync(vsync),
	mSDLSoftwareSurface(NULL),
	mNeedPaletteRefresh(true), mLocks(0)
{
}


//
// ISDL12Window::~ISDL12Window
//
ISDL12Window::~ISDL12Window()
{
	if (mSDLSoftwareSurface)
		SDL_FreeSurface(mSDLSoftwareSurface);

	delete mPrimarySurface;
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
	{
		SDL_Surface* sdlsurface = SDL_GetVideoSurface();
		if (SDL_MUSTLOCK(sdlsurface))
			SDL_LockSurface(sdlsurface);
		if (mSDLSoftwareSurface && SDL_MUSTLOCK(mSDLSoftwareSurface))
			SDL_LockSurface(mSDLSoftwareSurface);
	}

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
	{
		SDL_Surface* sdlsurface = SDL_GetVideoSurface();
		if (SDL_MUSTLOCK(sdlsurface))
			SDL_UnlockSurface(sdlsurface);
		if (mSDLSoftwareSurface && SDL_MUSTLOCK(mSDLSoftwareSurface))
			SDL_UnlockSurface(mSDLSoftwareSurface);
	}

	assert(mLocks >= 0 && mLocks < 100);
}


//
// ISDL12Window::refresh
//
void ISDL12Window::refresh()
{
	assert(mLocks == 0);		// window surface shouldn't be locked when blitting

	SDL_Surface* sdlsurface = SDL_GetVideoSurface();

	if (mNeedPaletteRefresh)
	{
		Uint32 flags = SDL_LOGPAL | SDL_PHYSPAL;

		if (sdlsurface->format->BitsPerPixel == 8)
			SDL_SetPalette(sdlsurface, flags, sdlsurface->format->palette->colors, 0, 256);
		if (mSDLSoftwareSurface && mSDLSoftwareSurface->format->BitsPerPixel == 8)
			SDL_SetPalette(mSDLSoftwareSurface, flags, mSDLSoftwareSurface->format->palette->colors, 0, 256);
	}

	mNeedPaletteRefresh = false;

	// handle 8in32 mode
	if (mSDLSoftwareSurface)
		SDL_BlitSurface(mSDLSoftwareSurface, NULL, sdlsurface, NULL);

	SDL_Flip(sdlsurface);
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
	#if WIN32 && !_XBOX
	// [SL] Use Win32-specific code to make use of multiple-icon sizes
	// stored in the executable resources. SDL 1.2 only allows a fixed
	// 32x32 px icon.
	//
	// [Russell] - Just for windows, display the icon in the system menu and
	// alt-tab display

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

	if (mSDLSoftwareSurface)
		I_SetSDL12Palette(mSDLSoftwareSurface, palette_colors);

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
static void I_BuildPixelFormatFromSDLSurface(const SDL_Surface* sdlsurface, PixelFormat* format, uint8_t desired_bpp)
{
	const SDL_PixelFormat* sdlformat = sdlsurface->format;

	// handle SDL not reporting correct Ashift/Aloss
	uint8_t aloss = desired_bpp == 32 ? 0 : 8;
	uint8_t ashift = desired_bpp == 32 ?  48 - sdlformat->Rshift - sdlformat->Gshift - sdlformat->Bshift : 0;
	
	// Create the PixelFormat specification
	*format = PixelFormat(
			desired_bpp,
			8 - aloss, 8 - sdlformat->Rloss, 8 - sdlformat->Gloss, 8 - sdlformat->Bloss,
			ashift, sdlformat->Rshift, sdlformat->Gshift, sdlformat->Bshift);
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
	delete mPrimarySurface;
	mPrimarySurface = NULL;

	if (mSDLSoftwareSurface)
		SDL_FreeSurface(mSDLSoftwareSurface);
	mSDLSoftwareSurface = NULL;

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

	SDL_Surface* sdlsurface = SDL_SetVideoMode(video_width, video_height, video_bpp, flags);

	// [SL] ...and re-enable RawWin32Mouse's input handlers after
	// DirectInput is reinitalized.
	I_ResumeMouse();

	if (!sdlsurface)
		return false;

	bool got_hardware_surface = (sdlsurface->flags & SDL_HWSURFACE) == SDL_HWSURFACE;

	bool create_software_surface = 
					(sdlsurface->pitch & 511) == 0 ||	// pitch is a multiple of 512 (thrashes the cache)
					got_hardware_surface;				// drawing directly to hardware surfaces is slower

	if (SDL_MUSTLOCK(sdlsurface))
		SDL_LockSurface(sdlsurface);		// lock prior to accessing pixel format

	PixelFormat format;
	I_BuildPixelFormatFromSDLSurface(sdlsurface, &format, video_bpp);

	if (create_software_surface)
	{
		// create a new IWindowSurface with its own frame buffer
		mPrimarySurface = new IWindowSurface(sdlsurface->w, sdlsurface->h, &format);
		
		uint32_t rmask = uint32_t(format.getRMax()) << format.getRShift();
		uint32_t gmask = uint32_t(format.getGMax()) << format.getGShift();
		uint32_t bmask = uint32_t(format.getBMax()) << format.getBShift();

		mSDLSoftwareSurface = SDL_CreateRGBSurfaceFrom(
					mPrimarySurface->getBuffer(),
					mPrimarySurface->getWidth(), mPrimarySurface->getHeight(),
					mPrimarySurface->getBitsPerPixel(),
					mPrimarySurface->getPitch(),
					rmask, gmask, bmask, 0);

		assert(mSDLSoftwareSurface->format->Rmask == sdlsurface->format->Rmask &&
				mSDLSoftwareSurface->format->Gmask == sdlsurface->format->Gmask &&
				mSDLSoftwareSurface->format->Bmask == sdlsurface->format->Bmask);
	}
	else
	{
		mPrimarySurface = new IWindowSurface(sdlsurface->w, sdlsurface->h, &format,
					sdlsurface->pixels, sdlsurface->pitch);
	}

	mWidth = mPrimarySurface->getWidth();
	mHeight = mPrimarySurface->getHeight();
	mBitsPerPixel = mPrimarySurface->getBitsPerPixel(); 
	mIsFullScreen = (sdlsurface->flags & SDL_FULLSCREEN) == SDL_FULLSCREEN;
	mUseVSync = vsync;

	mVideoMode = IVideoMode(video_width, video_height, video_bpp, video_fullscreen);

	if (SDL_MUSTLOCK(sdlsurface))
		SDL_UnlockSurface(sdlsurface);

	assert(mWidth >= 0 && mWidth <= MAXWIDTH);
	assert(mHeight >= 0 && mHeight <= MAXHEIGHT);
	assert(mBitsPerPixel == 8 || mBitsPerPixel == 32);

	// Tell argb_t the pixel format
	if (format.getBitsPerPixel() == 32)
		argb_t::setChannels(format.getAPos(), format.getRPos(), format.getGPos(), format.getBPos());
	else
		argb_t::setChannels(3, 2, 1, 0);

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


VERSION_CONTROL (i_sdlvideo_cpp, "$Id$")

