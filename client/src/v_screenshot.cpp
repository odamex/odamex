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
// Screenshots
//
//-----------------------------------------------------------------------------

#include <SDL.h>

#include <stdio.h>
#include <stdlib.h>
#include <string>

#include "doomtype.h"
#include "i_video.h"

#include "c_dispatch.h"
#include "m_misc.h"
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

bool M_FindFreeName(std::string &filename, const std::string &extension);

EXTERN_CVAR(gammalevel)
EXTERN_CVAR(vid_gammatype)

CVAR_FUNC_IMPL(cl_screenshotname)
{
	// No empty format strings allowed.
	if (strlen(var.cstring()) == 0)
		var.RestoreDefault();
}

BEGIN_COMMAND(screenshot)
{
	if (argc == 1)
		G_ScreenShot(NULL);
	else
		G_ScreenShot(argv[1]);
}
END_COMMAND(screenshot)


#ifdef USE_PNG	// was libpng included in the build?

//
// V_SetPNGPalette
//
// Compose a palette of png_color from a palette of argb_t colors,
// then set the png's PLTE chunk appropriately
// Note: palette is assumed to contain 256 colors
//
static void V_SetPNGPalette(png_struct* png_ptr, png_info* info_ptr, const argb_t* palette_colors)
{
	if (png_get_color_type(png_ptr, info_ptr) != 3)
	{
		Printf(PRINT_HIGH, "I_SetPNGPalette: Cannot create PNG PLTE chunk in 32-bit mode\n");
		return;
	}

	png_color pngpalette[256];

	for (int i = 0; i < 256; i++)
	{
		pngpalette[i].red   = (png_byte)(palette_colors[i].getr());
		pngpalette[i].green = (png_byte)(palette_colors[i].getg());
		pngpalette[i].blue  = (png_byte)(palette_colors[i].getb());
	}

	png_set_PLTE(png_ptr, info_ptr, pngpalette, 256);
}


//
// V_SetPNGComments
//
// Write comment lines to PNG file's tEXt chunk
//
static void V_SetPNGComments(png_struct *png_ptr, png_info *info_ptr, time_t* now)
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
	pngtext[text_line].text = (I_GetPrimarySurface()->getBitsPerPixel() == 8)
				? (png_charp)"8bpp" : (png_charp)"32bpp";
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
// V_SavePNG
//
// Converts the surface to PNG format and saves it to filename.
// Supporting function for I_ScreenShot to output PNG files.
//
static int V_SavePNG(const std::string& filename, IWindowSurface* surface)
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
		V_SetPNGPalette(png_ptr, info_ptr, surface->getPalette());
		
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
				*row++ = (png_byte)pixel.getr();
				*row++ = (png_byte)pixel.getg();
				*row++ = (png_byte)pixel.getb();
			}

			source += pitch_remainder;
		}
	}
	
	// commit PNG image data to file
	surface->unlock();
	png_init_io(png_ptr, fp);
	V_SetPNGComments(png_ptr, info_ptr, &now);
	
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

#endif	// USE_PNG


#ifndef USE_PNG	// not using libpng?
//
// V_SaveBMP
//
// Converts the surface to BMP format and saves it to filename.
// Note: this uses SDL 1.2 for writing to BMP format and may be deprecated
// in the future.
//
static int V_SaveBMP(const std::string& filename, IWindowSurface* surface)
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
		return -1;
	}

	if (surface->getBitsPerPixel() == 8)
	{
		const argb_t* palette = surface->getPalette();
		SDL_Color colors[256];

		for (int i = 0; i < 256; i ++, palette++)
		{
			colors[i].r = palette->getr();
			colors[i].g = palette->getg();
			colors[i].b = palette->getb();
			colors[i].unused = 0;
		}

		SDL_SetColors(sdlsurface, colors, 0, 256);
	}

	int result = SDL_SaveBMP(sdlsurface, filename.c_str());

	if (result != 0)
		Printf(PRINT_HIGH, "SDL_SaveBMP failed: %s\n", SDL_GetError());

	return result;
}
#endif	// !USE_PNG


//
// V_ScreenShot
//
// Dumps the contents of the screen framebuffer to a file. The default output
// format is PNG (if libpng is found at compile-time) with BMP as the fallback.
//
void V_ScreenShot(std::string filename)
{
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
	int result = V_SavePNG(filename, primary_surface); 
	if (result != 0)
	{
		Printf(PRINT_HIGH, "I_SavePNG Error: Returned error code %d\n", result);
		return;
	}
	#else
	int result = V_SaveBMP(filename, primary_surface); 
	if (result != 0)
	{
		Printf(PRINT_HIGH, "I_SaveBMP Error: Returned error code %d\n", result);
		return;
	}
	#endif	// USE_PNG

	Printf(PRINT_HIGH, "Screenshot taken: %s\n", filename.c_str());
}


VERSION_CONTROL (v_screenshot_cpp, "$Id$")


