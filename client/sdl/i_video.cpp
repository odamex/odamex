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
#include "g_game.h"

#ifdef USE_PNG
	#define PNG_SKIP_SETJMP_CHECK
	#include <setjmp.h>		// used for error handling by libpng

	#include <zlib.h>
	#include <png.h>

	#if (PNG_LIBPNG_VER < 10400)
		// [SL] add data types to support versions of libpng prior to 1.4.0

		/* png_alloc_size_t is guaranteed to be no smaller than png_size_t,
		 * and no smaller than png_uint_32.  Casts from png_size_t or png_uint_32
		 * to png_alloc_size_t are not necessary; in fact, it is recommended
		 * not to use them at all so that the compiler can complain when something
		 * turns out to be problematic.
		 * Casts in the other direction (from png_alloc_size_t to png_size_t or
		 * png_uint_32) should be explicitly applied; however, we do not expect
		 * to encounter practical situations that require such conversions.
		 */

		#if defined(__TURBOC__) && !defined(__FLAT__)
		   typedef unsigned long png_alloc_size_t;
		#else
		#  if defined(_MSC_VER) && defined(MAXSEG_64K)
			 typedef unsigned long    png_alloc_size_t;
		#  else
			 /* This is an attempt to detect an old Windows system where (int) is
			  * actually 16 bits, in that case png_malloc must have an argument with a
			  * bigger size to accomodate the requirements of the library.
			  */
		#    if (defined(_Windows) || defined(_WINDOWS) || defined(_WINDOWS_)) && \
				(!defined(INT_MAX) || INT_MAX <= 0x7ffffffeL)
			   typedef DWORD         png_alloc_size_t;
		#    else
			   typedef png_size_t    png_alloc_size_t;
		#    endif
		#  endif
		#endif
	#endif	// PNG_LIBPNG_VER < 10400
#endif	// USE_PNG

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

EXTERN_CVAR(cl_screenshotname)
EXTERN_CVAR(gammalevel)
EXTERN_CVAR(vid_gammatype)


#ifdef USE_PNG	// was libpng included in the build?
//
// I_SetPNGPalette
//
// Compose a palette of png_color from a palette of argb_t colors,
// then set the png's PLTE chunk appropriately
// Note: palette is assumed to contain 256 colors
//
static void I_SetPNGPalette(png_struct* png_ptr, png_info* info_ptr, const argb_t* palette)
{
	if (png_get_color_type(png_ptr, info_ptr) != 3)
	{
		Printf(PRINT_HIGH, "I_SetPNGPalette: Cannot create PNG PLTE chunk in 32-bit mode\n");
		return;
	}

	png_color pngpalette[256];

	for (int i = 0; i < 256; i++)
	{
		pngpalette[i].red   = (png_byte)RPART(palette[i]);
		pngpalette[i].green = (png_byte)GPART(palette[i]);
		pngpalette[i].blue  = (png_byte)BPART(palette[i]);
	}

	png_set_PLTE(png_ptr, info_ptr, pngpalette, 256);
}


//
// I_SetPNGComments
//
// Write comment lines to PNG file's tEXt chunk
//
static void I_SetPNGComments(png_struct *png_ptr, png_info *info_ptr, time_t *now)
{
	#ifdef PNG_TEXT_SUPPORTED
	const int PNG_TEXT_LINES = 6;
	png_text pngtext[PNG_TEXT_LINES];
	int text_line = 0;
	
	// write text lines uncompressed
	for (int i = 0; i < PNG_TEXT_LINES; i++)
		pngtext[i].compression = PNG_TEXT_COMPRESSION_NONE;
	
	pngtext[text_line].key = (png_charp)"Description";
	pngtext[text_line].text = (png_charp)("Odamex " DOTVERSIONSTR " Screenshot");
	text_line++;
	
	char datebuf[80];
	const char *dateformat = "%A, %B %d, %Y, %I:%M:%S %p GMT";
	strftime(datebuf, sizeof(datebuf) / sizeof(char), dateformat, gmtime(now));
	
	pngtext[text_line].key = (png_charp)"Created Time";
	pngtext[text_line].text = (png_charp)datebuf;
	text_line++;
	
	pngtext[text_line].key = (png_charp)"Game Mode";
	pngtext[text_line].text = (png_charp)(M_ExpandTokens("%g").c_str());
	text_line++;
	
	pngtext[text_line].key = (png_charp)"In-Game Video Mode";
	pngtext[text_line].text =
		(I_GetVideoBitDepth() == 8) ? (png_charp)"8bpp" : (png_charp)"32bpp";
	text_line++;
	
	char gammabuf[20];	// large enough to not overflow with three digits of precision
	sprintf(gammabuf, "%#.3f", gammalevel.value());
	
	pngtext[text_line].key = (png_charp)"In-game Gamma Correction Level";
	pngtext[text_line].text = (png_charp)gammabuf;
	text_line++;
	
	pngtext[text_line].key = (png_charp)"In-Game Gamma Correction Type";
	pngtext[text_line].text =
		(vid_gammatype == 0) ? (png_charp)"Classic Doom" : (png_charp)"ZDoom";
	text_line++;
	
	png_set_text(png_ptr, info_ptr, pngtext, PNG_TEXT_LINES);
	#else
	Printf(PRINT_HIGH, "I_SetPNGComments: Skipping PNG tEXt chunk\n");
	#endif // PNG_TEXT_SUPPORTED
}


//
// I_SavePNG
//
// Converts the surface to PNG format and saves it to filename.
// Supporting function for I_ScreenShot to output PNG files.
//
static int I_SavePNG(const std::string& filename, IWindowSurface* surface)
{
	FILE* fp = fopen(filename.c_str(), "wb");
	png_struct *png_ptr;
	png_info *info_ptr;
	time_t now = time(NULL); // used for PNG text comments

	if (fp == NULL)
	{
		Printf(PRINT_HIGH, "I_SavePNG: Could not open %s for writing\n", filename.c_str());
		return -1;
	}

	// Initialize png_struct for writing
	png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if (png_ptr == NULL)
	{
		fclose(fp);
		Printf(PRINT_HIGH, "I_SavePNG: png_create_write_struct failed\n");
		return -1;
	}

	// Init png_info struct
	info_ptr = png_create_info_struct(png_ptr);
	if (info_ptr == NULL)
	{
		fclose(fp);
		png_destroy_write_struct(&png_ptr, (png_infop*)NULL);
		Printf(PRINT_HIGH, "I_SavePNG: png_create_info_struct failed\n");
		return -1;
	}
	
	// libpng instances compiled without PNG_NO_SETJMP expect this;
	// PNG_ABORT() is invoked instead if PNG_SETJMP_SUPPORTED was not defined
	// see include/pnglibconf.h for libpng feature support macros
	#ifdef PNG_SETJMP_SUPPORTED
	int setjmp_result = setjmp(png_jmpbuf(png_ptr));
	if (setjmp_result != 0)
	{
		fclose(fp);
		png_destroy_write_struct(&png_ptr, &info_ptr);
		Printf(PRINT_HIGH, "I_SavePNG: setjmp failed with error code %d\n", setjmp_result);
		return -1;
	}
	#endif // PNG_SETJMP_SUPPORTED

	surface->lock();
	png_uint_32 width = surface->getWidth();
	png_uint_32 height = surface->getHeight();

	// is the screen paletted or 32-bit RGBA?
	// note: we don't want to the preserve A channel in the screenshot if screen is RGBA
	int png_colortype = surface->getBitsPerPixel() == 8 ? PNG_COLOR_TYPE_PALETTE : PNG_COLOR_TYPE_RGB;
	// write image dimensions to png file's IHDR chunk
	png_set_IHDR
		(png_ptr, info_ptr,
		width, height,
		8, // per channel; this is valid regardless of whether we're using 8bpp or 32bpp
		png_colortype,
		PNG_INTERLACE_NONE,
		PNG_COMPRESSION_TYPE_DEFAULT,
		PNG_FILTER_TYPE_DEFAULT);

	// determine bpp mode, allocate memory space for PNG pixel data
	int png_bpp = (surface->getBitsPerPixel() == 8) ? 1 : 3;
	png_byte** row_ptrs = (png_byte**)png_malloc(png_ptr, (png_alloc_size_t)(height * sizeof(png_byte*)));
	png_byte* row;
	
	for (unsigned int rownum = 0; rownum < height; rownum++)
	{
		row = (png_byte*)png_malloc(png_ptr, (png_alloc_size_t)(sizeof(uint8_t) * width * png_bpp));
		
		if (row != NULL)
		{
			row_ptrs[rownum] = row;
		}
		else
		{
			for (unsigned int i = 0; i < rownum; i++)
				png_free(png_ptr, row_ptrs[i]);

			png_free(png_ptr, row_ptrs);
			png_destroy_write_struct(&png_ptr, &info_ptr);
			fclose(fp);
			
			Printf(PRINT_HIGH, "I_SavePNG: Not enough RAM to create PNG file\n");
			return -1;
		}
	}
	
	// write PNG in either paletted or RGB form, according to the current screen mode
	if (surface->getBitsPerPixel() == 8)
	{
		I_SetPNGPalette(png_ptr, info_ptr, surface->getPalette());
		
		const palindex_t* source = (palindex_t*)surface->getBuffer();
		const int pitch_remainder = surface->getPitchInPixels() - width;

		for (unsigned int y = 0; y < height; y++)
		{
			row = row_ptrs[y];

			for (unsigned int x = 0; x < width; x++)
			{
				// gather color index from current pixel of SDL surface,
				// copy it to current pixel of PNG row
				// note: this assumes that the PNG and SDL surface palettes match
				palindex_t pixel = *source++;
				*row++ = (png_byte)pixel;
			}

			source += pitch_remainder;
		}
	}
	else
	{
		const argb_t* source = (argb_t*)surface->getBuffer();
		const int pitch_remainder = surface->getPitchInPixels() - width;

		for (unsigned int y = 0; y < height; y++)
		{
			row = row_ptrs[y];
			
			for (unsigned int x = 0; x < width; x++)
			{
				// gather color components from current pixel of SDL surface
				// note: SDL surface's alpha channel is ignored if present
				argb_t pixel = *source++;

				// write color components to current pixel of PNG row
				// note: PNG is a big-endian file format
				*row++ = (png_byte)RPART(pixel);
				*row++ = (png_byte)GPART(pixel);
				*row++ = (png_byte)BPART(pixel);
			}

			source += pitch_remainder;
		}
	}
	
	// commit PNG image data to file
	surface->unlock();
	png_init_io(png_ptr, fp);
	I_SetPNGComments(png_ptr, info_ptr, &now);
	
	// set PNG timestamp
	#ifdef PNG_tIME_SUPPORTED
	png_time pngtime;
	png_convert_from_time_t(&pngtime, now);
	png_set_tIME(png_ptr, info_ptr, &pngtime);
	#else
	Printf(PRINT_HIGH, "I_SavePNG: Skipping PNG tIME chunk\n");
	#endif // PNG_tIME_SUPPORTED
	
	png_set_rows(png_ptr, info_ptr, row_ptrs);
	png_write_png(png_ptr, info_ptr, PNG_TRANSFORM_IDENTITY, NULL);
	
	// free allocated PNG image data
	for (unsigned int y = 0; y < height; y++)
		png_free(png_ptr, row_ptrs[y]);

	png_free(png_ptr, row_ptrs);
	png_destroy_write_struct(&png_ptr, &info_ptr);
	
	fclose(fp);
	return 0;
}

#else	// USE_PNG

//
// I_SaveBMP
//
// Converts the surface to BMP format and saves it to filename.
// Note: this uses SDL 1.2 for writing to BMP format and may be deprecated
// in the future.
//
static int I_SaveBMP(const std::string& filename, IWindowSurface* surface)
{
	surface->lock();
	
	SDL_Surface* sdlsurface = SDL_CreateRGBSurfaceFrom(surface->getBuffer(),
								surface->getWidth(), surface->getHeight(),
								surface->getBitsPerPixel(), surface->getPitch(),
								0, 0, 0, 0);

	surface->unlock();

	if (sdlsurface == NULL)
	{
		Printf(PRINT_HIGH, "CreateRGBSurfaceFrom failed: %s\n", SDL_GetError());
		return;
	}

	if (surface->getBitsPerPixel() == 8)
	{
		const argb_t* palette = surface->getPalette();
		SDL_Color colors[256];

		for (int i = 0; i < 256; i ++, palette++)
		{
			colors[i].r = RPART(*palette);
			colors[i].g = GPART(*palette);
			colors[i].b = BPART(*palette);
			colors[i].unused = 0;
		}

		SDL_SetColors(sdlsurface, colors, 0, 256);
	}

	int result = SDL_SaveBMP(sdlsurface, filename.c_str());

	return result;
}

#endif // USE_PNG


bool M_FindFreeName(std::string &filename, const std::string &extension);

//
// I_ScreenShot
//
// Dumps the contents of the screen framebuffer to a file. The default output
// format is PNG (if libpng is found at compile-time) with BMP as the fallback.
//
void I_ScreenShot(std::string filename)
{
	SDL_Color colors[256];
	
	// is PNG supported?
	#ifdef USE_PNG
	const std::string extension("png");
	#else
	const std::string extension("bmp");
	#endif	// USE_PNG

	// If no filename was passed, use the screenshot format variable.
	if (filename.empty())
		filename = cl_screenshotname.cstring();

	// Expand tokens
	filename = M_ExpandTokens(filename).c_str();

	// If the file already exists, append numbers.
	if (!M_FindFreeName(filename, extension))
	{
		Printf(PRINT_HIGH, "I_ScreenShot: Delete some screenshots\n");
		return;
	}

	// Create an SDL_Surface object from our screen buffer
	IWindowSurface* primary_surface = I_GetPrimarySurface();

	#ifdef USE_PNG
	int result = I_SavePNG(filename, primary_surface); 
	if (result != 0)
	{
		Printf(PRINT_HIGH, "I_SavePNG Error: Returned error code %d\n", result);
		return;
	}
	#else
	int result = I_SaveBMP(filename, primary_surface); 
	if (result != 0)
	{
		Printf(PRINT_HIGH, "I_SaveBMP Error: Returned error code %d\n", result);
		return;
	}
	#endif	// USE_PNG

	Printf(PRINT_HIGH, "Screenshot taken: %s\n", filename.c_str());
}


BEGIN_COMMAND (screenshot)
{
	if (argc == 1)
		G_ScreenShot(NULL);
	else
		G_ScreenShot(argv[1]);
}
END_COMMAND (screenshot)

CVAR_FUNC_IMPL (cl_screenshotname)
{
	// No empty format strings allowed.
	if (strlen(var.cstring()) == 0)
		var.RestoreDefault();
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


