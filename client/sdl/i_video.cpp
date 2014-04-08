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
//	SDL hardware access for Video Rendering (?)
//
//-----------------------------------------------------------------------------


#include <stdio.h>
#include <stdlib.h>

#include <string>

#if 0
#undef MINCHAR
#undef MAXCHAR
#undef MINSHORT
#undef MAXSHORT
#undef MINLONG
#undef MAXLONG
#endif

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

// Global IWindow instance for the application window
static IWindow* window;


extern constate_e ConsoleState;
extern int NewWidth, NewHeight, NewBits, DisplayBits;

EXTERN_CVAR(vid_fullscreen)
EXTERN_CVAR(vid_vsync)
EXTERN_CVAR(vid_overscan)
EXTERN_CVAR(vid_displayfps)
EXTERN_CVAR(vid_ticker)
EXTERN_CVAR(vid_32bpp)
EXTERN_CVAR(vid_defwidth)
EXTERN_CVAR(vid_defheight)

CVAR_FUNC_IMPL (vid_winscale)
{
/*
	if (Video)
	{
		Video->SetWindowedScale(var);
		NewWidth = I_GetVideoWidth();
		NewHeight = I_GetVideoHeight(); 
		NewBits = I_GetVideoBitDepth(); 
		setmodeneeded = true;
	}
*/
}

CVAR_FUNC_IMPL (vid_overscan)
{
/*
	if (Video)
		Video->SetOverscan(var);
*/
}

void STACK_ARGS I_ShutdownHardware()
{
	delete window;
	window = NULL;
}


static int I_GetParmValue(const char* name)
{
	const char* valuestr = Args.CheckValue(name);
	if (valuestr)
		return atoi(valuestr);
	return 0;
}

//
// I_InitHardware
//
// Initializes the application window and a few miscellaneous video functions.
//
void I_InitHardware()
{
	char str[2] = { 0, 0 };
	str[0] = '1' - !Args.CheckParm("-devparm");
	vid_ticker.SetDefault(str);

	int width = I_GetParmValue("-width");
	int height = I_GetParmValue("-height");
	int bpp = I_GetParmValue("-bits");
	
	if (width == 0 && height == 0)
	{
		width = vid_defwidth.asInt();
		height = vid_defheight.asInt();
	}
	else if (width == 0)
	{
		width = (height * 8) / 6;
	}
	else if (height == 0)
	{
		height = (width * 6) / 8;
	}

	if (bpp == 0 || (bpp != 8 && bpp != 32))
		bpp = vid_32bpp ? 32 : 8;

/*
	if (vid_autoadjust)
		I_ClosestResolution(&width, &height);

	if (!V_SetResolution (width, height, bits))
		I_FatalError ("Could not set resolution to %d x %d x %d %s\n", width, height, bits,
            (vid_fullscreen ? "FULLSCREEN" : "WINDOWED"));
	else
        AddCommandString("checkres");
*/

	bool fullscreen = vid_fullscreen;
	bool vsync = vid_vsync;

	I_SetVideoMode(width, height, bpp, fullscreen, vsync);
	if (!I_VideoInitialized())
		I_FatalError("Failed to initialize display");

	atterm(I_ShutdownHardware);

	I_SetWindowCaption();
//	Video->SetWindowedScale(vid_winscale);
}


// VIDEO WRAPPERS ---------------------------------------------------------


// Set the window caption
void I_SetWindowCaption(const std::string& caption)
{
	// [Russell] - A basic version string that will eventually get replaced
	//             better than "Odamex SDL Alpha Build 001" or something :P    

	std::string title("Odamex - ");
	title += DOTVERSIONSTR;
		
	if (!caption.empty())
		title += " " + caption;

	window->setWindowTitle(title);
}


// Set the window icon
void I_SetWindowIcon(void)
{

}


EDisplayType I_DisplayType()
{
	return window->getDisplayType();
}

bool I_SetOverscan (float scale)
{
	return false;
//	return Video->SetOverscan (scale);
}



bool I_SetMode(int& width, int& height, int& bpp)
{
	bool fullscreen = false;
	int temp_bpp = bpp;

	switch (I_DisplayType())
	{
	case DISPLAY_WindowOnly:
		fullscreen = false;
		I_PauseMouse();
		break;
	case DISPLAY_FullscreenOnly:
		fullscreen = true;
		I_ResumeMouse();
		break;
	case DISPLAY_Both:
		fullscreen = vid_fullscreen ? true : false;
		fullscreen ? I_ResumeMouse() : I_PauseMouse();
		break;
	}

	I_SetVideoMode(width, height, temp_bpp, fullscreen, vid_vsync);
	if (I_VideoInitialized())
		return true;

	// Try the opposite bit mode:
	temp_bpp = bpp == 32 ? 8 : 32;
	I_SetVideoMode(width, height, temp_bpp, fullscreen, vid_vsync);
	if (I_VideoInitialized())
		return true;

	// Switch the bit mode back:
	temp_bpp = bpp;

	// Try the closest resolution:
	I_ClosestResolution(&width, &height);
	I_SetVideoMode(width, height, temp_bpp, fullscreen, vid_vsync);
	if (I_VideoInitialized())
		return true;

	// Try the opposite bit mode:
	temp_bpp = bpp == 32 ? 8 : 32;
	I_SetVideoMode(width, height, temp_bpp, fullscreen, vid_vsync);
	if (I_VideoInitialized())
		return true;

	// Just couldn't get it:
	return false;

	//I_FatalError ("Mode %dx%dx%d is unavailable\n",
	//			width, height, bpp);
}


//
// I_CheckResolution
//
// Returns true if the application window can be resized to the specified
// dimensions. For full-screen windows, this is limited to supported
// video modes only.
//
bool I_CheckResolution(int width, int height)
{
	const IVideoModeList* modes = I_GetWindow()->getSupportedVideoModes();
	for (IVideoModeList::const_iterator it = modes->begin(); it != modes->end(); ++it)
	{
		if (it->getWidth() == width && it->getHeight() == height)
			return true;
	}

	// [AM] We only care about correct resolutions if we're fullscreen.
	return !(I_GetWindow()->isFullScreen());
}


void I_ClosestResolution(int* width, int* height)
{
	const IVideoModeList* modes = I_GetWindow()->getSupportedVideoModes();

	unsigned int closest_dist = MAXWIDTH * MAXWIDTH + MAXHEIGHT * MAXHEIGHT;
	int closest_width = 0, closest_height = 0;

//	Video->FullscreenChanged (vid_fullscreen ? true : false);

	for (int iteration = 0; iteration < 2; iteration++)
	{
		for (IVideoModeList::const_iterator it = modes->begin(); it != modes->end(); ++it)
		{
			if (it->getWidth() == *width && it->getHeight() == *height)
				return;

			if (iteration == 0 && (it->getWidth() < *width || it->getHeight() < *height))
				continue;

			unsigned int dist = (it->getWidth() - *width) * (it->getWidth() - *width)
					+ (it->getHeight() - *height) * (it->getHeight() - *height);
			
			if (dist < closest_dist)
			{
				closest_dist = dist;
				closest_width = it->getWidth();
				closest_height = it->getHeight();
			}
		}
	
		if (closest_width > 0 && closest_height > 0)
		{
			*width = closest_width;
			*height = closest_height;
			return;
		}
	}
}

bool I_CheckVideoDriver(const char* name)
{
	if (!name)
		return false;
	return iequals(I_GetWindow()->getVideoDriverName(), name);
}


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
			memcpy(dest, source, srcpitchpixels * sizeof(SOURCE_PIXEL_T));
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
		yfrac -= (yfrac >> FRACBITS);
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

	fixed_t xstep = FixedDiv((srcw + 1) << FRACBITS, destw << FRACBITS);
	fixed_t ystep = FixedDiv((srch + 1)<< FRACBITS, desth << FRACBITS);

	int srcbits = source_surface->getBitsPerPixel();
	int destbits = getBitsPerPixel();
	int srcpitchpixels = source_surface->getPitchInPixels();
	int destpitchpixels = getPitchInPixels();

	if (srcbits == 8 && destbits == 8)
	{
		const palindex_t* source = (palindex_t*)source_surface->getBuffer() + srcy * srcpitchpixels + srcx;
		palindex_t* dest = (palindex_t*)getBuffer() + desty * destpitchpixels + destx;

		BlitLoop(dest, source, destpitchpixels, srcpitchpixels, destw, desth, xstep, ystep, getPalette());
	}
	else if (srcbits == 8 && destbits == 32)
	{
		const palindex_t* source = (palindex_t*)source_surface->getBuffer() + srcy * srcpitchpixels + srcx;
		argb_t* dest = (argb_t*)getBuffer() + desty * destpitchpixels + destx;

		BlitLoop(dest, source, destpitchpixels, srcpitchpixels, destw, desth, xstep, ystep, getPalette());
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

		BlitLoop(dest, source, destpitchpixels, srcpitchpixels, destw, desth, xstep, ystep, getPalette());
	}
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
// IDummyWindowSurface class implementation
//
// Simple implementation of IWindowSurface for headless clients. 
//
// ============================================================================

//
// IDummyWindowSurface::IDummyWindowSurface
//
// Allocates a fixed-size buffer.
//
IDummyWindowSurface::IDummyWindowSurface(IWindow* window) :
	IWindowSurface(window)
{
	// TODO: make mSurfaceBuffer aligned to 16-byte boundary
	mSurfaceBuffer = new byte[getPitch() * getHeight() * getBytesPerPixel()];
	memset(mPalette, 0, 256 * sizeof(*mPalette));
}


//
// IDummyWindowSurface::~IDummyWindowSurface
//
IDummyWindowSurface::~IDummyWindowSurface()
{
	delete [] mSurfaceBuffer;
}



// ****************************************************************************

// cached values of window->getPrimarySurface()->getWidth() and getHeight()
// these should be updated every time the window or surface are resized
static int surface_width, surface_height;


void I_SetVideoMode(int width, int height, int bpp, bool fullscreen, bool vsync)
{
	if (window != NULL)
		delete window;

/*
	if (vid_autoadjust)
		I_ClosestResolution(&width, &height);

	if (!V_SetResolution (width, height, bits))
		I_FatalError ("Could not set resolution to %d x %d x %d %s\n", width, height, bits,
            (vid_fullscreen ? "FULLSCREEN" : "WINDOWED"));
	else
        AddCommandString("checkres");
*/

	if (Args.CheckParm("-novideo"))
		window = new IDummyWindow();
	else
		window = new ISDL12Window(width, height, bpp, fullscreen, vsync);
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
	return window->getWidth();
}


//
// I_GetVideoHeight
//
// Returns the height of the current video mode. Assumes that the video
// window has already been created.
//
int I_GetVideoHeight()
{
	return window->getHeight();
}


//
// I_GetVideoBitDepth
//
// Returns the bits per pixelof the current video mode. Assumes that the video
// window has already been created.
//
int I_GetVideoBitDepth()
{
	return window->getBitsPerPixel();
}


//
// I_GetPrimarySurface
//
// Returns a pointer to the application window's primary surface object.
//
IWindowSurface* I_GetPrimarySurface()
{
	return window->getPrimarySurface();
}


//
// I_AllocateSurface
//
// Creates a new (non-primary) surface and returns it.
//
IWindowSurface* I_AllocateSurface(int width, int height, int bpp)
{
	return new ISDL12WindowSurface(I_GetWindow(), width, height, bpp);
}


//
// I_FreeSurface
//
void I_FreeSurface(IWindowSurface* surface)
{
	delete surface;
}


//
// I_GetFrameBuffer
//
// Returns a pointer to the raw frame buffer for the game window's primary
// surface.
//
byte* I_GetFrameBuffer()
{
	return I_GetPrimarySurface()->getBuffer();
}


//
// I_GetSurfaceWidth
//
int I_GetSurfaceWidth()
{
	return I_GetPrimarySurface()->getWidth();
}


//
// I_GetSurfaceHeight
//
int I_GetSurfaceHeight()
{
	return I_GetPrimarySurface()->getHeight();
}


//
// I_BeginUpdate
//
// Called at the start of drawing a new video frame. This locks the primary
// surface for drawing.
//
void I_BeginUpdate()
{
	I_GetPrimarySurface()->lock();
}


//
// I_FinishUpdate
//
// Called at the end of drawing a video frame. This unlocks the primary
// surface for drawing and blits the contents to the video screen.
//
void I_FinishUpdate()
{
	if (noblit == false)
	{
		// Draws frame time and cumulative fps
		if (vid_displayfps)
			V_DrawFPSWidget();

		// draws little dots on the bottom of the screen
		if (vid_ticker)
			V_DrawFPSTicker();

		window->refresh();
	}

	I_GetPrimarySurface()->unlock();
}


void I_ReadScreen(byte *block)
{
//	Video->ReadScreen (block);
}


//
// I_SetPalette
//
void I_SetPalette(const argb_t* palette)
{
	window->setPalette(palette);
}


//
// I_SetOldPalette
//
// Sets the window's palette using a raw PLAYPAL palette.
//
void I_SetOldPalette(const palindex_t* palette)
{
	window->setPalette(palette);
}


//
// I_SetWindowSize
//
// Resizes the application window to the specified size.
//
void I_SetWindowSize(int width, int height)
{
	I_GetWindow()->resize(width, height);
}


//
// I_SetSurfaceSize
//
// Resizes the drawing surface to the specified size.
// TODO: Surface size should be completely independent of window size.
//
void I_SetSurfaceSize(int width, int height)
{
	I_GetWindow()->resize(width, height);
}


//
// I_IsProtectedReslotuion
//
// [ML] If this is 320x200 or 640x400, the resolutions
// "protected" from aspect ratio correction.
//
bool I_IsProtectedResolution()
{
	int width = I_GetPrimarySurface()->getWidth();
	int height = I_GetPrimarySurface()->getHeight();
 
	return (width == 320 && height == 200) || (width == 640 && height == 400);
}

VERSION_CONTROL (i_video_cpp, "$Id$")


