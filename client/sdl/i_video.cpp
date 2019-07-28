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
//
// Low-level video hardware management.
//
//-----------------------------------------------------------------------------

#include <cstdio>
#include <cstdlib>
#include <climits>
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
#include "i_input.h"
#include "m_fileio.h"

#include "w_wad.h"

// [Russell] - Just for windows, display the icon in the system menu and
// alt-tab display
#if defined(_WIN32) && !defined(_XBOX)
	#include "win32inc.h"
    #include "SDL_syswm.h"
    #include "resource.h"
#endif	// _WIN32

// Declared in doomtype.h as part of argb_t
uint8_t argb_t::a_num, argb_t::r_num, argb_t::g_num, argb_t::b_num;

// Global IVideoSubsystem instance for video startup and shutdown
static IVideoSubsystem* video_subsystem = NULL;

// Global IWindowSurface instance for the application window
static IWindowSurface* primary_surface = NULL;

// Global IWindowSurface instance for converting 8bpp data to a 32bpp surface
static IWindowSurface* converted_surface = NULL;

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

EXTERN_CVAR(vid_32bpp)
EXTERN_CVAR(vid_fullscreen)
EXTERN_CVAR(vid_vsync)
EXTERN_CVAR(vid_overscan)
EXTERN_CVAR(vid_320x200)
EXTERN_CVAR(vid_640x400)
EXTERN_CVAR(vid_autoadjust)
EXTERN_CVAR(vid_displayfps)
EXTERN_CVAR(vid_ticker)


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
// Initializes a new IWindowSurface given a width, height, and PixelFormat.
// If a buffer is not given, one will be allocated and marked for deallocation
// from the destructor. If the pitch is not given, the width will be used as
// the basis for the pitch.
//
IWindowSurface::IWindowSurface(uint16_t width, uint16_t height, const PixelFormat* format,
								void* buffer, uint16_t pitch) :
	mCanvas(NULL),
	mSurfaceBuffer((uint8_t*)buffer), mOwnsSurfaceBuffer(buffer == NULL),
	mPalette(V_GetDefaultPalette()->colors), mPixelFormat(*format),
	mWidth(width), mHeight(height), mPitch(pitch), mLocks(0)
{
	const uintptr_t alignment = 16;

	// Not given a pitch? Just base pitch on the given width
	if (pitch == 0)
	{
		// make the pitch a multiple of the alignment value
		mPitch = (mWidth * mPixelFormat.getBytesPerPixel() + alignment - 1) & ~(alignment - 1);
		// add a little to the pitch to prevent cache thrashing if it's 512 or 1024
		if ((mPitch & 511) == 0)
			mPitch += alignment;
	}

	mPitchInPixels = mPitch / mPixelFormat.getBytesPerPixel();

	if (mOwnsSurfaceBuffer)
	{
		uint8_t* buffer = new uint8_t[mPitch * mHeight + alignment];

		// calculate the offset from buffer to the next aligned memory address
		uintptr_t offset = ((uintptr_t)(buffer + alignment) & ~(alignment - 1)) - (uintptr_t)buffer;

		mSurfaceBuffer = buffer + offset;

		// verify we'll have enough room to store offset immediately
		// before mSurfaceBuffer's address and that offset's value is sane.
		assert(offset > 0 && offset <= alignment);

		// store mSurfaceBuffer's offset from buffer so that mSurfaceBuffer
		// can be properly freed later.
		buffer[offset - 1] = offset;
	}
}


//
// IWindowSurface::IWindowSurface
//
// Initializes the IWindowSurface from an existing IWindowSurface, but
// with possibly different values for width and height. If width or height
// are less than that of the existing surface, the new surface will be centered
// inside the existing surface.
//
IWindowSurface::IWindowSurface(IWindowSurface* base_surface, uint16_t width, uint16_t height) :
	mCanvas(NULL), mOwnsSurfaceBuffer(false),
	mPixelFormat(*base_surface->getPixelFormat()),
	mPitch(base_surface->getPitch()), mPitchInPixels(base_surface->getPitchInPixels()),
	mLocks(0)
{
	mWidth = std::min(base_surface->getWidth(), width);
	mHeight = std::min(base_surface->getHeight(), height);

	// adjust mSurfaceBuffer so that the new surface is centered in base_surface
	uint16_t x = (base_surface->getWidth() - mWidth) / 2;
	uint16_t y = (base_surface->getHeight() - mHeight) / 2;

	mSurfaceBuffer = (uint8_t*)base_surface->getBuffer(x, y);

	mPalette = base_surface->mPalette;
}


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

	// calculate the buffer's original address when freeing mSurfaceBuffer
	if (mOwnsSurfaceBuffer)
	{
		ptrdiff_t offset = mSurfaceBuffer[-1];
		delete [] (mSurfaceBuffer - offset);
	}
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
	
	const argb_t* palette = source_surface->getPalette();

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


// ****************************************************************************

//
// I_GetVideoModeString
//
// Returns a string with a text description of the given video mode.
//
std::string I_GetVideoModeString(const IVideoMode* mode)
{
	char str[50];
	sprintf(str, "%dx%d %dbpp (%s)", mode->getWidth(), mode->getHeight(), mode->getBitsPerPixel(),
			mode->isFullScreen() ? "fullscreen" : "windowed");

	return std::string(str);
}


//
// I_IsModeSupported
//
// Helper function for I_ValidateVideoMode. Returns true if there is a video
// mode availible with the desired bpp and screen mode.
//
static bool I_IsModeSupported(uint8_t bpp, bool fullscreen)
{
	const IVideoModeList* modelist = I_GetVideoCapabilities()->getSupportedVideoModes();

	for (IVideoModeList::const_iterator it = modelist->begin(); it != modelist->end(); ++it)
		if (it->isFullScreen() == fullscreen && it->getBitsPerPixel() == bpp)
			return true;

	return false;
}


//
// I_ValidateVideoMode
//
// Transforms the given video mode into a mode that is valid for the current
// video hardware capabilities.
//
static IVideoMode I_ValidateVideoMode(const IVideoMode* mode)
{
	const IVideoModeList* modelist = I_GetVideoCapabilities()->getSupportedVideoModes();

	uint16_t desired_width = mode->getWidth(), desired_height = mode->getHeight();
	uint8_t desired_bpp = mode->getBitsPerPixel();
	bool desired_fullscreen = mode->isFullScreen();

	// Ensure the display type is adhered to
	if (I_GetVideoCapabilities()->supportsFullScreen() == false)
		desired_fullscreen = false;
	else if (I_GetVideoCapabilities()->supportsWindowed() == false)
		desired_fullscreen = true;

	// check if the given bit-depth is supported
	if (!I_IsModeSupported(desired_bpp, desired_fullscreen))
	{
		desired_bpp = desired_bpp ^ (32 | 8);			// toggle bpp between 8 and 32

		// check if the new bit-depth is supported
		if (!I_IsModeSupported(desired_bpp, desired_fullscreen))
			return IVideoMode(0, 0, 0, false);		// return an invalid video mode
	}

	desired_width = clamp<uint16_t>(desired_width, 320, MAXWIDTH);
	desired_height = clamp<uint16_t>(desired_height, 200, MAXHEIGHT);

	IVideoMode desired_mode(desired_width, desired_height, desired_bpp, desired_fullscreen);

	if (!desired_fullscreen || !vid_autoadjust)
		return desired_mode;

	unsigned int closest_dist = UINT_MAX;
	const IVideoMode* closest_mode = NULL;

	for (int iteration = 0; iteration < 2; iteration++)
	{
		for (IVideoModeList::const_iterator it = modelist->begin(); it != modelist->end(); ++it)
		{
			if (*it == desired_mode)		// perfect match?
				return *it;

			if (it->getBitsPerPixel() == desired_bpp && it->isFullScreen() == desired_fullscreen)
			{
				if (iteration == 0 && (it->getWidth() < desired_width || it->getHeight() < desired_height))
					continue;

				unsigned int dist = (it->getWidth() - desired_width) * (it->getWidth() - desired_width)
						+ (it->getHeight() - desired_height) * (it->getHeight() - desired_height);

				if (dist < closest_dist)
				{
					closest_dist = dist;
					closest_mode = &(*it);
				}
			}
		}

		if (closest_mode != NULL)
			return *closest_mode;
	}

	return IVideoMode(0, 0, 0, false);		// return an invalid video mode
}


//
// I_SetVideoMode
//
// Main function to set the video mode at the hardware level.
//
void I_SetVideoMode(int width, int height, int surface_bpp, bool fullscreen, bool vsync)
{
	// ensure the requested mode is valid
	IVideoMode desired_mode(width, height, surface_bpp, fullscreen);
	IVideoMode mode = I_ValidateVideoMode(&desired_mode);
	assert(mode.isValid());

	IWindow* window = I_GetWindow();

	window->setMode(mode.getWidth(), mode.getHeight(), mode.getBitsPerPixel(), mode.isFullScreen(), vsync);
	I_ForceUpdateGrab();

	// [SL] 2011-11-30 - Prevent the player's view angle from moving
	I_FlushInput();
		
	// Set up the primary and emulated surfaces
	primary_surface = window->getPrimarySurface();
	int surface_width = primary_surface->getWidth(), surface_height = primary_surface->getHeight();

	I_FreeSurface(converted_surface);
	I_FreeSurface(matted_surface);
	I_FreeSurface(emulated_surface);

	// Handle a requested 8bpp surface when the video capabilities only support 32bpp
	if (surface_bpp != mode.getBitsPerPixel())
	{
		const PixelFormat* format = surface_bpp == 8 ? I_Get8bppPixelFormat() : I_Get32bppPixelFormat();
		converted_surface = new IWindowSurface(surface_width, surface_height, format);
		primary_surface = converted_surface;
	}

	// clear window's surface to all black;
	primary_surface->lock();
	primary_surface->clear();
	primary_surface->unlock();

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
	surface_width = clamp<uint16_t>(surface_width, 320, MAXWIDTH);
	surface_height = clamp<uint16_t>(surface_height, 200, MAXHEIGHT);
	
	// Anything wider than 16:9 starts to look more distorted and provides even more advantage, so for now
	// if the monitor aspect ratio is wider than 16:9, clamp it to that (TODO: Aspect ratio selection)
	if (surface_width / surface_height > 16 / 9)
		surface_width = (surface_height * 16) / 9;

	// Is matting being used? Create matted_surface based on the primary_surface.
	if (surface_width != primary_surface->getWidth() ||
		surface_height != primary_surface->getHeight())
	{
		matted_surface = new IWindowSurface(primary_surface, surface_width, surface_height);
		primary_surface = matted_surface;
	}

	// Create emulated_surface for emulating low resolution modes.
	if (vid_320x200)
	{
		emulated_surface = new IWindowSurface(320, 200, primary_surface->getPixelFormat());
		emulated_surface->clear();
	}
	else if (vid_640x400)
	{
		emulated_surface = new IWindowSurface(640, 400, primary_surface->getPixelFormat());
		emulated_surface->clear();
	}

	screen = primary_surface->getDefaultCanvas();

	assert(I_VideoInitialized());

	if (*window->getVideoMode() != desired_mode)
		DPrintf("I_SetVideoMode: could not set video mode to %s. Using %s instead.\n",
						I_GetVideoModeString(&desired_mode).c_str(),
						I_GetVideoModeString(window->getVideoMode()).c_str());
	else
		DPrintf("I_SetVideoMode: set video mode to %s\n",
					I_GetVideoModeString(window->getVideoMode()).c_str());

	const argb_t* palette = V_GetGamePalette()->colors;
	if (matted_surface)
		matted_surface->setPalette(palette);
	if (emulated_surface)
		emulated_surface->setPalette(palette);
	if (converted_surface)
		converted_surface->setPalette(palette);

	// handle the -noblit parameter when playing a LMP demo
	if (noblit)
		window->disableRefresh();
	else
		window->enableRefresh();
}


//
// I_VideoInitialized
//
// Returns true if the video subsystem has been initialized.
//
bool I_VideoInitialized()
{
	return video_subsystem != NULL && I_GetWindow() != NULL && I_GetWindow()->getPrimarySurface() != NULL;
}


//
// I_ShutdownHardware
//
// Destroys the application window and frees its memory.
//
void STACK_ARGS I_ShutdownHardware()
{
	I_FreeSurface(loading_icon_background_surface);

	delete video_subsystem;
	video_subsystem = NULL;
}


//
// I_InitHardware
//
void I_InitHardware()
{
	if (I_IsHeadless())
	{
		video_subsystem = new IDummyVideoSubsystem();
	}
	else
	{
		#if defined(SDL12)
		video_subsystem = new ISDL12VideoSubsystem();
		#elif defined(SDL20)
		video_subsystem = new ISDL20VideoSubsystem();
		#endif
		assert(video_subsystem != NULL);

		const IVideoMode* native_mode = I_GetVideoCapabilities()->getNativeMode();
		Printf(PRINT_HIGH, "I_InitHardware: native resolution: %s\n", I_GetVideoModeString(native_mode).c_str());
	}
}


//
// I_GetVideoCapabilities
//
const IVideoCapabilities* I_GetVideoCapabilities()
{
	// a valid IWindow is not needed for querying video capabilities
	// so don't call I_VideoInitialized
	if (video_subsystem)
		return video_subsystem->getVideoCapabilities();
	return NULL;
}


//
// I_GetWindow
//
// Returns a pointer to the application window object.
//
IWindow* I_GetWindow()
{
	if (video_subsystem)
		return video_subsystem->getWindow();
	return NULL;
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
		return I_GetWindow()->getWidth();
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
		return I_GetWindow()->getHeight();
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
		return I_GetWindow()->getBitsPerPixel();
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
	const PixelFormat* format;

	if (I_GetPrimarySurface() && bpp == I_GetPrimarySurface()->getBitsPerPixel())
		format = I_GetPrimarySurface()->getPixelFormat();
	else if (bpp == 8)
		format = I_Get8bppPixelFormat();
	else
		format = I_Get32bppPixelFormat();

	return new IWindowSurface(width, height, format);
}


//
// I_FreeSurface
//
void I_FreeSurface(IWindowSurface* &surface)
{
	delete surface;
	surface = NULL;
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
// I_IsWideResolution
//
bool I_IsWideResolution(int width, int height)
{
	if (I_IsProtectedResolution(width, height))
		return false;

	// consider the mode widescreen if it's width-to-height ratio is
	// closer to 16:10 than it is to 4:3
	return abs(15 * width - 20 * height) > abs(15 * width - 24 * height);
}

bool I_IsWideResolution(const IWindowSurface* surface)
{
	return I_IsWideResolution(surface->getWidth(), surface->getHeight());
}


//
// I_LockAllSurfaces
//
static void I_LockAllSurfaces()
{
	if (emulated_surface)
		emulated_surface->lock();
	if (matted_surface)
		matted_surface->lock();
	if (converted_surface)
		converted_surface->lock();
	primary_surface->lock();

	I_GetWindow()->lockSurface();
}


//
// I_UnlockAllSurfaces
//
static void I_UnlockAllSurfaces()
{
	I_GetWindow()->unlockSurface();

	primary_surface->unlock();

	if (converted_surface)
		converted_surface->unlock();
	if (matted_surface)
		matted_surface->unlock();
	if (emulated_surface)
		emulated_surface->unlock();
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
	const patch_t* diskpatch = wads.CachePatch("STDISK");
	IWindowSurface* surface = I_GetPrimarySurface();

	surface->lock();

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
		I_FreeSurface(loading_icon_background_surface);

		loading_icon_background_surface = I_AllocateSurface(w, h, bpp);
	}

	loading_icon_background_surface->lock();

	loading_icon_background_surface->blit(surface, x, y, w, h, 0, 0, w, h);
	surface->getDefaultCanvas()->DrawPatchStretched(diskpatch, ofsx, ofsy, w, h);

	loading_icon_background_surface->unlock();
	surface->unlock();
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

	surface->lock();
	loading_icon_background_surface->lock();

	int w = loading_icon_background_surface->getWidth();
	int h = loading_icon_background_surface->getHeight();
	int x = surface->getWidth() - w;
	int y = surface->getHeight() - h;

	surface->blit(loading_icon_background_surface, 0, 0, w, h, x, y, w, h);

	loading_icon_background_surface->unlock();
	surface->unlock();
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
		I_GetWindow()->startRefresh();

		I_LockAllSurfaces();
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

		// Handle blitting our 8bpp surface to the 32bpp video window surface
		if (converted_surface)
		{
			IWindowSurface* real_primary_surface = I_GetWindow()->getPrimarySurface();
			real_primary_surface->blit(converted_surface,
					0, 0, converted_surface->getWidth(), converted_surface->getHeight(),
					0, 0, real_primary_surface->getWidth(), real_primary_surface->getHeight());
		}

		I_UnlockAllSurfaces();

		I_GetWindow()->finishRefresh();

		// restores the background underneath the disk loading icon in the lower right corner
		if (gametic <= loading_icon_expire)
			I_RestoreLoadingIcon();
	}
}


//
// I_SetPalette
//
void I_SetPalette(const argb_t* palette)
{
	if (I_VideoInitialized())
		I_GetWindow()->setPalette(palette);
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

	I_GetWindow()->setWindowTitle(title);
}


//
// I_SetWindowIcon
//
// Set the window icon (currently Windows only).
//
void I_SetWindowIcon()
{
	I_GetWindow()->setWindowIcon();
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


//
// I_Get8bppPixelFormat
//
// Returns the platform's PixelFormat for 8bpp video modes.
//
const PixelFormat* I_Get8bppPixelFormat()
{
	static PixelFormat format(8, 0, 0, 0, 0, 0, 0, 0, 0);
	return &format;
}


//
// I_Get32bppPixelFormat
//
// Returns the platform's PixelFormat for 32bpp video modes.
// This changes on certain platforms depending on the current video mode
// in use (or fullscreen vs windowed).
//
const PixelFormat* I_Get32bppPixelFormat()
{
	if (I_GetPrimarySurface() && I_GetPrimarySurface()->getBitsPerPixel() == 32)
		return I_GetPrimarySurface()->getPixelFormat();

	#ifdef __BIG_ENDIAN__
	static PixelFormat format(32, 0, 0, 0, 0, 0, 8, 16, 24);
	#else
	static PixelFormat format(32, 0, 0, 0, 0, 24, 16, 8, 0);
	#endif

	return &format;
}

VERSION_CONTROL (i_video_cpp, "$Id$")


