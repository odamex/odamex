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
// Low-level video hardware management.
//
//-----------------------------------------------------------------------------

#include <stdio.h>
#include <stdlib.h>
#include <string>

#include "i_video.h"
#include "v_video.h"

#include "i_system.h"
#include "c_console.h"
#include "c_cvars.h"
#include "c_dispatch.h"
#include "m_argv.h"
#include "m_misc.h"
#include "i_sdlvideo.h"
#include "m_fileio.h"

#include "w_wad.h"

// [Russell] - Just for windows, display the icon in the system menu and
// alt-tab display
#if defined(_WIN32) && !defined(_XBOX)
	#include "win32inc.h"
    #include "SDL_syswm.h"
    #include "resource.h"
#endif	// _WIN32

// Global IWindow instance for the application window
static IWindow* window = NULL;

// Global IWindowSurface instance for the application window
static IWindowSurface* primary_surface = NULL;

// Global IWindowSurface instance constructed from primary_surface.
// Used when matting is required (letter-boxing/pillar-boxing)
static IWindowSurface* matted_surface = NULL;

// Global IWindowSurface instance of size 320x200 or 640x400.
// Emulates low-resolution video modes by rendering to a small surface
// and then stretch it to the primary surface after rendering is complete. 
static IWindowSurface* emulated_surface = NULL;

extern int NewWidth, NewHeight, NewBits, DisplayBits;

static int loading_icon_expire = -1;
static IWindowSurface* loading_icon_background_surface = NULL;

EXTERN_CVAR(vid_fullscreen)
EXTERN_CVAR(vid_vsync)
EXTERN_CVAR(vid_overscan)
EXTERN_CVAR(vid_displayfps)
EXTERN_CVAR(vid_ticker)
EXTERN_CVAR(vid_32bpp)
EXTERN_CVAR(vid_defwidth)
EXTERN_CVAR(vid_defheight)

CVAR_FUNC_IMPL(vid_overscan)
{
	V_ForceVideoModeAdjustment();
}

CVAR_FUNC_IMPL(vid_320x200)
{
	V_ForceVideoModeAdjustment();
}

CVAR_FUNC_IMPL(vid_640x400)
{
	V_ForceVideoModeAdjustment();
}

CVAR_FUNC_IMPL(vid_vsync)
{
	V_ForceVideoModeAdjustment();
}


//
// vid_listmodes
//
// Prints a list of all supported video modes, highlighting the current
// video mode. Requires I_VideoInitialized() to be true.
//
BEGIN_COMMAND(vid_listmodes)
{
	const IVideoModeList* modes = I_GetWindow()->getSupportedVideoModes();

	for (IVideoModeList::const_iterator it = modes->begin(); it != modes->end(); ++it)
	{
		if (it->getWidth() == I_GetWindow()->getWidth() && it->getHeight() == I_GetWindow()->getHeight())
			Printf_Bold("%4d x%5d\n", it->getWidth(), it->getHeight());
		else
			Printf(PRINT_HIGH, "%4d x%5d\n", it->getWidth(), it->getHeight());
	}
}
END_COMMAND(vid_listmodes)


//
// vid_currentmode
//
// Prints the current video mode. Requires I_VideoInitialized() to be true.
//
BEGIN_COMMAND(vid_currentmode)
{
	Printf(PRINT_HIGH, "%dx%dx%d\n",
			I_GetWindow()->getWidth(), I_GetWindow()->getHeight(),
			I_GetWindow()->getBitsPerPixel());
}
END_COMMAND(vid_currentmode)




// ****************************************************************************

// ============================================================================
//
// IWindowSurface abstract base class implementation
//
// Default implementation for the IWindowSurface base class.
//
// ============================================================================

//
// IWindowSurface::IWindowSurface
//
// Assigns the pointer to the window that owns this surface.
//
IWindowSurface::IWindowSurface(IWindow* window) :
	mWindow(window), mCanvas(NULL)
{ }


//
// IWindowSurface::~IWindowSurface
//
// Frees all of the DCanvas objects that were instantiated by this surface.
//
IWindowSurface::~IWindowSurface()
{
	// free all DCanvas objects allocated by this surface
	for (DCanvasCollection::iterator it = mCanvasStore.begin(); it != mCanvasStore.end(); ++it)
		delete *it;
}


//
// Pixel format conversion function used by IWindowSurface::blit
//
template <typename SOURCE_PIXEL_T, typename DEST_PIXEL_T>
static inline DEST_PIXEL_T ConvertPixel(SOURCE_PIXEL_T value, const argb_t* palette);

template <>
inline palindex_t ConvertPixel(palindex_t value, const argb_t* palette)
{	return value;	}

template <>
inline argb_t ConvertPixel(palindex_t value, const argb_t* palette)
{	return palette[value];	}

template <>
inline palindex_t ConvertPixel(argb_t value, const argb_t* palette)
{	return 0;	}

template <>
inline argb_t ConvertPixel(argb_t value, const argb_t* palette)
{	return value;	}


template <typename SOURCE_PIXEL_T, typename DEST_PIXEL_T>
static void BlitLoop(DEST_PIXEL_T* dest, const SOURCE_PIXEL_T* source,
					int destpitchpixels, int srcpitchpixels, int destw, int desth,
					fixed_t xstep, fixed_t ystep, const argb_t* palette)
{
	fixed_t yfrac = 0;
	for (int y = 0; y < desth; y++)
	{
		if (sizeof(DEST_PIXEL_T) == sizeof(SOURCE_PIXEL_T) && xstep == FRACUNIT)
		{
			memcpy(dest, source, destw * sizeof(SOURCE_PIXEL_T));
		}
		else
		{
			fixed_t xfrac = 0;
			for (int x = 0; x < destw; x++)
			{
				dest[x] = ConvertPixel<SOURCE_PIXEL_T, DEST_PIXEL_T>(source[xfrac >> FRACBITS], palette);
				xfrac += xstep;
			}
		}

		dest += destpitchpixels;
		yfrac += ystep;
		
		source += srcpitchpixels * (yfrac >> FRACBITS);
		yfrac &= (FRACUNIT - 1);
	}
}


//
// IWindowSurface::blit
//
// Blits a surface into this surface, automatically scaling the source image
// to fit the destination dimensions.
//
void IWindowSurface::blit(const IWindowSurface* source_surface, int srcx, int srcy, int srcw, int srch,
			int destx, int desty, int destw, int desth)
{
	// clamp to source surface edges
	if (srcx < 0)
	{
		srcw += srcx;
		srcx = 0;
	}

	if (srcy < 0)
	{
		srch += srcy;
		srcy = 0;
	}

	if (srcx + srcw > source_surface->getWidth())
		srcw = source_surface->getWidth() - srcx;
	if (srcy + srch > source_surface->getHeight())
		srch = source_surface->getHeight() - srcy;

	if (srcw == 0 || srch == 0)
		return;

	// clamp to dest surface edges
	if (destx < 0)
	{
		destw += destx;
		destx = 0;
	}

	if (desty < 0)
	{
		desth += desty;
		desty = 0;
	}

	if (destx + destw > getWidth())
		destw = getWidth() - destx;
	if (desty + desth > getHeight())
		desth = getHeight() - desty;

	if (destw == 0 || desth == 0)
		return;

	fixed_t xstep = FixedDiv(srcw << FRACBITS, destw << FRACBITS);
	fixed_t ystep = FixedDiv(srch << FRACBITS, desth << FRACBITS);

	int srcbits = source_surface->getBitsPerPixel();
	int destbits = getBitsPerPixel();
	int srcpitchpixels = source_surface->getPitchInPixels();
	int destpitchpixels = getPitchInPixels();
	const argb_t* palette = V_GetDefaultPalette()->colors;

	if (srcbits == 8 && destbits == 8)
	{
		const palindex_t* source = (palindex_t*)source_surface->getBuffer() + srcy * srcpitchpixels + srcx;
		palindex_t* dest = (palindex_t*)getBuffer() + desty * destpitchpixels + destx;

		BlitLoop(dest, source, destpitchpixels, srcpitchpixels, destw, desth, xstep, ystep, palette);
	}
	else if (srcbits == 8 && destbits == 32)
	{
		if (palette == NULL)
			return;

		const palindex_t* source = (palindex_t*)source_surface->getBuffer() + srcy * srcpitchpixels + srcx;
		argb_t* dest = (argb_t*)getBuffer() + desty * destpitchpixels + destx;

		BlitLoop(dest, source, destpitchpixels, srcpitchpixels, destw, desth, xstep, ystep, palette);
	}
	else if (srcbits == 32 && destbits == 8)
	{
		// we can't quickly convert from 32bpp source to 8bpp dest so don't bother
		return;
	}
	else if (srcbits == 32 && destbits == 32)
	{
		const argb_t* source = (argb_t*)source_surface->getBuffer() + srcy * srcpitchpixels + srcx;
		argb_t* dest = (argb_t*)getBuffer() + desty * destpitchpixels + destx;

		BlitLoop(dest, source, destpitchpixels, srcpitchpixels, destw, desth, xstep, ystep, palette);
	}
}


//
// IWindowSurface::clear
//
// Clears the surface to black.
//
void IWindowSurface::clear()
{
	const argb_t color(255, 0, 0, 0);

	lock();

	if (getBitsPerPixel() == 8)
	{
		const argb_t* palette_colors = V_GetDefaultPalette()->basecolors;
		palindex_t color_index = V_BestColor(palette_colors, color);
		palindex_t* dest = (palindex_t*)getBuffer();

		for (int y = 0; y < getHeight(); y++)
		{
			memset(dest, color_index, getWidth());
			dest += getPitchInPixels();
		}
	}
	else
	{
		argb_t* dest = (argb_t*)getBuffer();

		for (int y = 0; y < getHeight(); y++)
		{
			for (int x = 0; x < getWidth(); x++)
				dest[x] = color;

			dest += getPitchInPixels();
		}
	}

	unlock();
}


//
// IWindowSurface::createCanvas
//
// Generic factory function to instantiate a DCanvas object capable of drawing
// to this surface.
//
DCanvas* IWindowSurface::createCanvas()
{
	DCanvas* canvas = new DCanvas(this);
	mCanvasStore.push_back(canvas);

	return canvas;
}


//
// IWindowSurface::getDefaultCanvas
//
// Returns the default DCanvas object for the surface, creating it if it
// has not yet been instantiated.
//
DCanvas* IWindowSurface::getDefaultCanvas()
{
	if (mCanvas == NULL)
		mCanvas = createCanvas();

	return mCanvas;
}


//
// IWindowSurface::releaseCanvas
//
// Manually frees a DCanvas object that was instantiated by this surface
// via the createCanvas function. Note that the destructor takes care of
// freeing all of the instantiated DCanvas objects.
//
void IWindowSurface::releaseCanvas(DCanvas* canvas)
{
	if (canvas->getSurface() != this)
		I_Error("IWindowSurface::releaseCanvas: releasing canvas not owned by this surface\n");	

	// Remove the DCanvas pointer from the surface's list of allocated canvases
	DCanvasCollection::iterator it = std::find(mCanvasStore.begin(), mCanvasStore.end(), canvas);
	if (it != mCanvasStore.end())
		mCanvasStore.erase(it);

	if (canvas == mCanvas)
		mCanvas = NULL;

	delete canvas;
}



// ============================================================================
//
// IGenericWindowSurface class implementation
//
// Simple implementation of IWindowSurface for headless clients. 
//
// ============================================================================

//
// IGenericWindowSurface::IGenericWindowSurface
//
// Allocates a fixed-size buffer.
//
IGenericWindowSurface::IGenericWindowSurface(IWindow* window, int width, int height, int bpp) :
	IWindowSurface(window), mPalette(NULL), mWidth(width), mHeight(height), mBitsPerPixel(bpp)
{
	// TODO: make mSurfaceBuffer aligned to 16-byte boundary
	mPitch = mWidth * getBytesPerPixel();

	mSurfaceBuffer = new byte[mPitch * mHeight];
	mAllocatedSurfaceBuffer = true;
}


//
// IGenericWindowSurface::IGenericWindowSurface
//
// Initializes the IGenericWindowSurface from an existing IWindowSurface, but
// with possibly different values for width and height. If width or height
// are less than that of the existing surface, the new surface will be centered
// inside the existing surface.
//
IGenericWindowSurface::IGenericWindowSurface(IWindowSurface* base_surface, int width, int height) :
	IWindowSurface(base_surface->getWindow()), mPalette(base_surface->getPalette()),
	mBitsPerPixel(base_surface->getBitsPerPixel()), mPitch(base_surface->getPitch())
{
	mWidth = std::min(base_surface->getWidth(), width);
	mHeight = std::min(base_surface->getHeight(), height);
	
	// adjust mSurfaceBuffer so that the new surface is centered in base_surface
	int x = (base_surface->getWidth() - mWidth) / 2;
	int y = (base_surface->getHeight() - mHeight) / 2;

	mSurfaceBuffer = base_surface->getBuffer() +
			base_surface->getPitch() * y +
			base_surface->getBytesPerPixel() * x;

	mAllocatedSurfaceBuffer = false;
}


//
// IGenericWindowSurface::~IGenericWindowSurface
//
IGenericWindowSurface::~IGenericWindowSurface()
{
	if (mAllocatedSurfaceBuffer)
		delete [] mSurfaceBuffer;
}


// ============================================================================
//
// IWindow class implementation
//
// ============================================================================

//
// IWindow::getClosestMode
//
// Returns the closest video mode to the specified dimensions. Note that this
// requires

IVideoMode IWindow::getClosestMode(int width, int height)
{
	const IVideoModeList* modes = getSupportedVideoModes();

	unsigned int closest_dist = MAXWIDTH * MAXWIDTH + MAXHEIGHT * MAXHEIGHT;
	int closest_width = 0, closest_height = 0;

	for (int iteration = 0; iteration < 2; iteration++)
	{
		for (IVideoModeList::const_iterator it = modes->begin(); it != modes->end(); ++it)
		{
			if (it->getWidth() == width && it->getHeight() == height)
				return *it;

			if (iteration == 0 && (it->getWidth() < width || it->getHeight() < height))
				continue;

			unsigned int dist = (it->getWidth() - width) * (it->getWidth() - width)
					+ (it->getHeight() - height) * (it->getHeight() - height);
			
			if (dist < closest_dist)
			{
				closest_dist = dist;
				closest_width = it->getWidth();
				closest_height = it->getHeight();
			}
		}
	
		if (closest_width > 0 && closest_height > 0)
			return IVideoMode(closest_width, closest_height);
	}

	return IVideoMode(closest_width, closest_height);
}


// ****************************************************************************

//
// I_ClampVideoMode
//
// Sanitizes the given video mode dimensions, preventing them from being smaller
// than 320x200 and larger than MAXWIDTH, MAXHEIGHT.
//
static IVideoMode I_ClampVideoMode(int width, int height)
{
	if (I_IsProtectedResolution(I_GetVideoWidth(), I_GetVideoHeight()))
	{
		if (width < 320 || width < 200)
			width = 320, height = 200;
	}
	else
	{
		if (width < 320 || height < 240)
			width = 320, height = 240;
	}

	if (width > MAXWIDTH || height > MAXHEIGHT)
	{
		bool widescreen = width * 3 > height * 4;
		if (widescreen && MAXHEIGHT * 16 / 9 <= MAXWIDTH)
			width = MAXHEIGHT * 9 / 16, height = MAXHEIGHT;
		else if (widescreen && MAXWIDTH * 9 / 16 <= MAXHEIGHT)
			width = MAXWIDTH, height = MAXWIDTH * 9 / 16;
		else if (!widescreen && MAXHEIGHT * 4 / 3 <= MAXWIDTH)
			width = MAXHEIGHT * 4 / 3, height = MAXHEIGHT;
		else if (!widescreen && MAXWIDTH * 3 / 4 <= MAXHEIGHT)
			width = MAXWIDTH, height = MAXWIDTH * 3 / 4;
	}

	return IVideoMode(width, height);
}


//
// I_DoSetVideoMode
//
// Helper function for I_SetVideoMode.
//
static void I_DoSetVideoMode(int width, int height, int bpp, bool fullscreen, bool vsync)
{
	I_FreeSurface(matted_surface);
	matted_surface = NULL;
	I_FreeSurface(emulated_surface);
	emulated_surface = NULL;

	if (I_IsHeadless())
	{
		delete window;
		window = new IDummyWindow();
		primary_surface = window->getPrimarySurface();
		screen = primary_surface->getDefaultCanvas();
		return;
	}

	// Ensure the display type is adhered to
//	if (I_DisplayType() == DISPLAY_WindowOnly)
//		fullscreen = false;
//	else if (I_DisplayType() == DISPLAY_FullscreenOnly)
//		fullscreen = true;

	if (fullscreen)
		I_ResumeMouse();
	else
		I_PauseMouse();

	IVideoMode mode = I_ClampVideoMode(width, height);

	if (window)
		window->setMode(mode.getWidth(), mode.getHeight(), bpp, fullscreen, vsync);
	else
		window = new ISDL12Window(mode.getWidth(), mode.getHeight(), bpp, fullscreen, vsync);

	if (!I_VideoInitialized())
		return;

	window->getPrimarySurface()->lock();

	// Set up the primary and emulated surfaces
	primary_surface = window->getPrimarySurface();
	int surface_width = primary_surface->getWidth(), surface_height = primary_surface->getHeight();

	// clear window's surface to all black
	primary_surface->clear();

	// [SL] Determine the size of the matted surface.
	// A matted surface will be used if pillar-boxing or letter-boxing are used, or
	// if vid_320x200/vid_640x400 are being used in a wide-screen video mode, or
	// if vid_overscan is used to create a matte around the entire screen.
	//
	// Only vid_overscan creates a matted surface if the video mode is 320x200 or 640x400.
	//

	if (vid_overscan < 1.0f)
	{
		surface_width *= vid_overscan;
		surface_height *= vid_overscan;
	}

	if (!I_IsProtectedResolution(I_GetVideoWidth(), I_GetVideoHeight()))
	{
		if (vid_320x200 || vid_640x400)
			surface_width = surface_height * 4 / 3;
		else if (V_UsePillarBox())
			surface_width = surface_height * 4 / 3; 
		else if (V_UseLetterBox())
			surface_height = surface_width * 9 / 16;
	}

	// Ensure matted surface dimensions are sane and sanitized.
	IVideoMode surface_mode = I_ClampVideoMode(surface_width, surface_height);
	surface_width = surface_mode.getWidth(), surface_height = surface_mode.getHeight();

	// Is matting being used? Create matted_surface based on the primary_surface.
	if (surface_width != primary_surface->getWidth() ||
		surface_height != primary_surface->getHeight())
	{
		matted_surface = new IGenericWindowSurface(primary_surface, surface_width, surface_height);
		primary_surface = matted_surface;
	}

	// Create emulated_surface for emulating low resolution modes.
	if (vid_320x200)
	{
		int bpp = primary_surface->getBitsPerPixel();
		emulated_surface = new IGenericWindowSurface(I_GetWindow(), 320, 200, bpp);
		emulated_surface->clear();
	}
	else if (vid_640x400)
	{
		int bpp = primary_surface->getBitsPerPixel();
		emulated_surface = new IGenericWindowSurface(I_GetWindow(), 640, 400, bpp);
		emulated_surface->clear();
	}

	screen = primary_surface->getDefaultCanvas();

	window->getPrimarySurface()->unlock();
}


//
// I_CheckVideoModeMessage
//
static void I_CheckVideoModeMessage(int width, int height, int bpp, bool fullscreen)
{
	if (I_IsHeadless())
		return;

	if (I_GetVideoWidth() != width || I_GetVideoHeight() != height)
		Printf(PRINT_HIGH, "Could not set resolution to %dx%dx%d %s. Using resolution " \
							"%dx%dx%d %s instead.\n",
							width, height, bpp, (fullscreen ? "FULLSCREEN" : "WINDOWED"),
							I_GetVideoWidth(), I_GetVideoHeight(), I_GetWindow()->getBitsPerPixel(),
							I_GetWindow()->isFullScreen() ? "FULLSCREEN" : "WINDOWED");
}


//
// I_SetVideoMode
//
// Main function to set the video mode at the hardware level.
//
void I_SetVideoMode(int width, int height, int bpp, bool fullscreen, bool vsync)
{
	int temp_bpp = bpp;

	I_DoSetVideoMode(width, height, temp_bpp, fullscreen, vsync);
	if (I_VideoInitialized())
	{
		I_CheckVideoModeMessage(width, height, bpp, fullscreen);
		return;
	}

	// Try the opposite bit mode:
	temp_bpp = bpp == 32 ? 8 : 32;
	I_DoSetVideoMode(width, height, temp_bpp, fullscreen, vsync);
	if (I_VideoInitialized())
	{
		I_CheckVideoModeMessage(width, height, bpp, fullscreen);
		return;
	}

	// Couldn't find a suitable resolution
	I_ShutdownHardware();		// ensure I_VideoInitialized returns false
}


//
// I_VideoInitialized
//
// Returns true if the video subsystem has been initialized.
//
bool I_VideoInitialized()
{
	return window != NULL && window->getPrimarySurface() != NULL;
}


//
// I_ShutdownHardware
//
// Destroys the application window and frees its memory.
//
void STACK_ARGS I_ShutdownHardware()
{
	delete window;
	window = NULL;

	if (loading_icon_background_surface)
		I_FreeSurface(loading_icon_background_surface);
	loading_icon_background_surface = NULL;
}


//
// I_InitHardware
//
void I_InitHardware()
{
	static bool initialized = false;
	if (!initialized)
	{
		atterm(I_ShutdownHardware);
		initialized = true;
	}
}


//
// I_GetWindow
//
// Returns a pointer to the application window object.
//
IWindow* I_GetWindow()
{
	return window;
}


//
// I_GetVideoWidth
//
// Returns the width of the current video mode. Assumes that the video
// window has already been created.
//
int I_GetVideoWidth()
{
	if (I_VideoInitialized())
		return window->getWidth();
	return 0;
}


//
// I_GetVideoHeight
//
// Returns the height of the current video mode. Assumes that the video
// window has already been created.
//
int I_GetVideoHeight()
{
	if (I_VideoInitialized())
		return window->getHeight();
	return 0;
}


//
// I_GetVideoBitDepth
//
// Returns the bits per pixelof the current video mode. Assumes that the video
// window has already been created.
//
int I_GetVideoBitDepth()
{
	if (I_VideoInitialized())
		return window->getBitsPerPixel();
	return 0;
}


//
// I_GetPrimarySurface
//
// Returns a pointer to the application window's primary surface object.
//
IWindowSurface* I_GetPrimarySurface()
{
	return primary_surface;
}


//
// I_GetPrimaryCanvas
//
// Returns a pointer to the primary surface's default canvas.
//
DCanvas* I_GetPrimaryCanvas()
{
	if (I_VideoInitialized())
		return I_GetPrimarySurface()->getDefaultCanvas();
	return NULL;
}


//
// I_GetEmulatedSurface
//
IWindowSurface* I_GetEmulatedSurface()
{
	return emulated_surface;
}


//
// I_BlitEmulatedSurface
//
// Blits the emulated surface (320x200 or 640x400) to the primary surface,
// stretching it to fill the entire screen.
//
void I_BlitEmulatedSurface()
{
	if (emulated_surface)
	{
		primary_surface->blit(emulated_surface, 0, 0,
				emulated_surface->getWidth(), emulated_surface->getHeight(),
				0, 0, primary_surface->getWidth(), primary_surface->getHeight());
	}
}


//
// I_AllocateSurface
//
// Creates a new (non-primary) surface and returns it.
//
IWindowSurface* I_AllocateSurface(int width, int height, int bpp)
{
	return new IGenericWindowSurface(I_GetWindow(), width, height, bpp);
}


//
// I_FreeSurface
//
void I_FreeSurface(IWindowSurface* surface)
{
	delete surface;
}


//
// I_GetSurfaceWidth
//
int I_GetSurfaceWidth()
{
	if (I_VideoInitialized())
		return I_GetPrimarySurface()->getWidth();
	return 0;
}


//
// I_GetSurfaceHeight
//
int I_GetSurfaceHeight()
{
	if (I_VideoInitialized())
		return I_GetPrimarySurface()->getHeight();
	return 0;
}


//
// I_IsProtectedReslotuion
//
// [ML] If this is 320x200 or 640x400, the resolutions
// "protected" from aspect ratio correction.
//
bool I_IsProtectedResolution(int width, int height)
{
	return (width == 320 && height == 200) || (width == 640 && height == 400);
}

bool I_IsProtectedResolution(const IWindowSurface* surface)
{
	return I_IsProtectedResolution(surface->getWidth(), surface->getHeight());
}


//
// I_DrawLoadingIcon
//
// Sets a flag to indicate that I_FinishUpdate should draw the loading (disk)
// icon in the lower right corner of the screen.
//
void I_DrawLoadingIcon()
{
	loading_icon_expire = gametic;
}


//
// I_BlitLoadingIcon
//
// Takes care of the actual drawing of the loading icon.
//
static void I_BlitLoadingIcon()
{
	const patch_t* diskpatch = W_CachePatch("STDISK");
	IWindowSurface* surface = I_GetPrimarySurface();
	int bpp = surface->getBitsPerPixel();

	int scale = std::min(CleanXfac, CleanYfac);
	int w = diskpatch->width() * scale;
	int h = diskpatch->height() * scale;
	int x = surface->getWidth() - w;
	int y = surface->getHeight() - h;

	// offset x and y for the lower right corner of the screen
	int ofsx = x + (scale * diskpatch->leftoffset());
	int ofsy = y + (scale * diskpatch->topoffset());

	// save the area where the icon will be drawn to an off-screen surface
	// so that it can be restored after the frame is blitted
	if (!loading_icon_background_surface ||
		loading_icon_background_surface->getWidth() != w ||
		loading_icon_background_surface->getHeight() != h ||
		loading_icon_background_surface->getBitsPerPixel() != bpp)
	{
		if (loading_icon_background_surface)
			I_FreeSurface(loading_icon_background_surface);

		loading_icon_background_surface = I_AllocateSurface(w, h, bpp);
	}

	loading_icon_background_surface->blit(surface, x, y, w, h, 0, 0, w, h);

	surface->getDefaultCanvas()->DrawPatchStretched(diskpatch, ofsx, ofsy, w, h);
}


//
// I_RestoreLoadingIcon
//
// Restores the background area that was saved prior to blitting the
// loading icon.
//
static void I_RestoreLoadingIcon()
{
	IWindowSurface* surface = I_GetPrimarySurface();

	int w = loading_icon_background_surface->getWidth();
	int h = loading_icon_background_surface->getHeight();
	int x = surface->getWidth() - w;
	int y = surface->getHeight() - h;

	surface->blit(loading_icon_background_surface, 0, 0, w, h, x, y, w, h);
}


//
// I_BeginUpdate
//
// Called at the start of drawing a new video frame. This locks the primary
// surface for drawing.
//
void I_BeginUpdate()
{
	if (I_VideoInitialized())
	{
		window->getPrimarySurface()->lock();

		if (matted_surface)
			matted_surface->lock();
		if (emulated_surface)
			emulated_surface->lock();
	}
}


//
// I_FinishUpdate
//
// Called at the end of drawing a video frame. This unlocks the primary
// surface for drawing and blits the contents to the video screen.
//
void I_FinishUpdate()
{
	if (I_VideoInitialized())
	{
		// Draws frame time and cumulative fps
		if (vid_displayfps)
			V_DrawFPSWidget();

		// draws little dots on the bottom of the screen
		if (vid_ticker)
			V_DrawFPSTicker();

		// draws a disk loading icon in the lower right corner
		if (gametic <= loading_icon_expire)
			I_BlitLoadingIcon();

		if (emulated_surface)
			emulated_surface->unlock();
		if (matted_surface)
			matted_surface->unlock();

		window->getPrimarySurface()->unlock();

		if (noblit == false)
			window->refresh();

		// restores the background underneath the disk loading icon in the lower right corner
		if (gametic <= loading_icon_expire)
		{
			window->getPrimarySurface()->lock();
			I_RestoreLoadingIcon();
			window->getPrimarySurface()->unlock();
		}
	}
}


//
// I_SetPalette
//
void I_SetPalette(const argb_t* palette)
{
	if (I_VideoInitialized())
	{
		window->setPalette(palette);

		primary_surface->setPalette(palette);

		if (matted_surface)
			matted_surface->setPalette(palette);
		if (emulated_surface)
			emulated_surface->setPalette(palette);
	}
}


//
// I_SetWindowSize
//
// Resizes the application window to the specified size.
//
void I_SetWindowSize(int width, int height)
{
	if (I_VideoInitialized())
	{
		int bpp = vid_32bpp ? 32 : 8;
		I_SetVideoMode(width, height, bpp, vid_fullscreen, vid_vsync);
	}
}


//
// I_SetWindowCaption
//
// Set the window caption.
//
void I_SetWindowCaption(const std::string& caption)
{
	// [Russell] - A basic version string that will eventually get replaced

	std::string title("Odamex ");
	title += DOTVERSIONSTR;
		
	if (!caption.empty())
		title += " - " + caption;

	window->setWindowTitle(title);
}


//
// I_SetWindowIcon
//
// Set the window icon (currently Windows only).
//
void I_SetWindowIcon()
{
	window->setWindowIcon();
}


//
// I_DisplayType
//
EDisplayType I_DisplayType()
{
	return window->getDisplayType();
}


//
// I_GetVideoDriverName
//
// Returns the name of the current video driver in-use.
//
std::string I_GetVideoDriverName()
{
	if (I_VideoInitialized())
		return I_GetWindow()->getVideoDriverName();
	return std::string();
}



VERSION_CONTROL (i_video_cpp, "$Id$")


