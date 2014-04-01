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

#include <sstream>
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

static IVideo *Video;

EXTERN_CVAR(vid_fullscreen)
EXTERN_CVAR(vid_overscan)
EXTERN_CVAR(vid_ticker)
EXTERN_CVAR(vid_32bpp)
EXTERN_CVAR(vid_defwidth)
EXTERN_CVAR(vid_defheight)

CVAR_FUNC_IMPL (vid_winscale)
{
	if (Video)
	{
		Video->SetWindowedScale(var);
		NewWidth = I_GetVideoWidth();
		NewHeight = I_GetVideoHeight(); 
		NewBits = I_GetVideoBitDepth(); 
		setmodeneeded = true;
	}
}

CVAR_FUNC_IMPL (vid_overscan)
{
	if (Video)
		Video->SetOverscan(var);
}

void STACK_ARGS I_ShutdownHardware ()
{
	if (Video)
		delete Video, Video = NULL;

	delete window;
	window = NULL;
		
    SDL_QuitSubSystem(SDL_INIT_VIDEO);
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
	
	if (width == 0)
	{
		if (height == 0)
		{
			width = vid_defwidth.asInt();
			height = vid_defheight.asInt();
		}
		else
		{
			width = (height * 8) / 6;
		}
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

	bool fullscreen = false;
	bool vsync = false;

	if (Args.CheckParm("-novideo"))
	{
		Video = new IVideo();
		window = new IDummyWindow();
	}
	else
	{
		Video = new SDLVideo(0);
		window = new ISDL12Window(width, height, bpp, fullscreen, vsync);
	}

	if (Video == NULL)
		I_FatalError ("Failed to initialize display");

	atterm(I_ShutdownHardware);

	Video->SetWindowedScale(vid_winscale);
}

bool I_HardwareInitialized()
{
	return (Video != NULL);
}

/** Remaining code is common to Win32 and Linux **/

// VIDEO WRAPPERS ---------------------------------------------------------

void I_BeginUpdate ()
{
	screen->Lock ();
}

void I_FinishUpdateNoBlit ()
{
	screen->Unlock ();
}

void I_TempUpdate ()
{
	Video->UpdateScreen (screen);
}

void I_FinishUpdate ()
{
	if (noblit == false)
		Video->UpdateScreen(screen);

	screen->Unlock(); // SoM: we should probably do this, eh?
}

void I_ReadScreen (byte *block)
{
	Video->ReadScreen (block);
}

void I_SetPalette (DWORD *pal)
{
	Video->SetPalette (pal);
}


// Set the window caption
void I_SetWindowCaption(const std::string& caption)
{
	// [Russell] - A basic version string that will eventually get replaced
	//             better than "Odamex SDL Alpha Build 001" or something :P    
	std::ostringstream title;

	title << "Odamex - " << DOTVERSIONSTR;
		
	if(caption.size())
	{
		title << " " << caption;
	}

	// [Russell] - Update window caption with name
	SDL_WM_SetCaption (title.str().c_str(), title.str().c_str());
}

void I_SetWindowCaption(void)
{
	I_SetWindowCaption("");
}

// Set the window icon
void I_SetWindowIcon(void)
{

}

extern DWORD IndexedPalette[256];
EXTERN_CVAR(cl_screenshotname)
EXTERN_CVAR(gammalevel)
EXTERN_CVAR(vid_gammatype)


#ifdef USE_PNG	// was libpng included in the build?
//
// I_SetPNGPalette
//
// Compose a palette of png_color from a palette of SDL_Color,
// then set the png's PLTE chunk appropriately
// Note: sdlpalette is assumed to contain 256 colors
//
static void I_SetPNGPalette(png_struct* png_ptr, png_info* info_ptr, const SDL_Color* sdlpalette)
{
	if (!I_GetVideoBitDepth() == 8)
	{
		Printf(PRINT_HIGH, "I_SetPNGPalette: Cannot create PNG PLTE chunk in 32-bit mode\n");
		return;
	}

	png_color pngpalette[256];

	for (int i = 0; i < 256; i++)
	{
		pngpalette[i].red   = (png_byte)sdlpalette[i].r;
		pngpalette[i].green = (png_byte)sdlpalette[i].g;
		pngpalette[i].blue  = (png_byte)sdlpalette[i].b;
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
// Converts the SDL_Surface to PNG format and saves it to filename.
// Supporting function for I_ScreenShot to output PNG files.
//
static int I_SavePNG(const std::string& filename, SDL_Surface* surface, SDL_Color* colors)
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

	SDL_LockSurface(surface);

	png_uint_32 width = I_GetVideoWidth();
	png_uint_32 height = I_GetVideoHeight();

	// is the screen paletted or 32-bit RGBA?
	// note: we don't want to the preserve A channel in the screenshot if screen is RGBA
	int png_colortype = I_GetVideoBitDepth() == 8 ? PNG_COLOR_TYPE_PALETTE : PNG_COLOR_TYPE_RGB;
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
	int png_bpp = (I_GetVideoBitDepth() == 8) ? 1 : 3;
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
	if (I_GetVideoBitDepth() == 8)
	{
		I_SetPNGPalette(png_ptr, info_ptr, colors);
		
		const palindex_t* source = (palindex_t*)surface->pixels;
		const int pitch_remainder = surface->pitch / sizeof(palindex_t) - width;

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
		const argb_t* source = (argb_t*)surface->pixels;
		const int pitch_remainder = surface->pitch / sizeof(argb_t) - width;

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
	SDL_UnlockSurface(surface);
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
// Converts the SDL_Surface to BMP format and saves it to filename.
//
static int I_SaveBMP(const std::string& filename, SDL_Surface* surface, SDL_Color* colors)
{
	int result = SDL_SaveBMP(surface, filename.c_str());
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
	screen->Lock();

	SDL_Surface* surface = SDL_CreateRGBSurfaceFrom(screen->buffer, screen->width,
									   screen->height, screen->bits, screen->pitch,
									   0, 0, 0, 0);

	screen->Unlock();

	if (surface == NULL)
	{
		Printf(PRINT_HIGH, "CreateRGBSurfaceFrom failed: %s\n", SDL_GetError());
		return;
	}

	if (I_GetVideoBitDepth() == 8)
	{
		// Set up the palette for our screen shot
		const argb_t* pal = IndexedPalette;

		for (int i = 0; i < 256; i += 1, pal++)
		{
			colors[i].r = RPART(*pal);
			colors[i].g = GPART(*pal);
			colors[i].b = BPART(*pal);
			colors[i].unused = 0;
		}

		SDL_SetColors(surface, colors, 0, 256);
	}

	#ifdef USE_PNG
	int result = I_SavePNG(filename, surface, colors);
	if (result != 0)
	{
		Printf(PRINT_HIGH, "I_SavePNG Error: Returned error code %d\n", result);
		SDL_FreeSurface(surface);
		return;
	}
	#else
	int result = I_SaveBMP(filename, surface, colors);
	if (result != 0)
	{
		Printf(PRINT_HIGH, "SDL_SaveBMP Error: %s\n", SDL_GetError());
		SDL_FreeSurface(surface);
		return;
	}
	#endif	// USE_PNG

	SDL_FreeSurface(surface);
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

//
// I_SetOldPalette - Just for the red screen for now I guess
//
void I_SetOldPalette(palindex_t* doompalette)
{
    Video->SetOldPalette(doompalette);
}

EDisplayType I_DisplayType ()
{
	return Video->GetDisplayType ();
}

bool I_SetOverscan (float scale)
{
	return Video->SetOverscan (scale);
}

int I_GetVideoWidth()
{
	if (Video)
		return Video->GetWidth();
	else
		return 0;
}

int I_GetVideoHeight()
{
	if (Video)
		return Video->GetHeight();
	else
		return 0;
}

int I_GetVideoBitDepth()
{
	if (Video)
		return Video->GetBitDepth();
	else
		return 0;
}

bool I_SetMode(int &width, int &height, int &bits)
{
	bool fs = false;
	int tbits = bits;

	switch (Video->GetDisplayType ())
	{
	case DISPLAY_WindowOnly:
		fs = false;
		I_PauseMouse();
		break;
	case DISPLAY_FullscreenOnly:
		fs = true;
		I_ResumeMouse();
		break;
	case DISPLAY_Both:
		fs = vid_fullscreen ? true : false;

		fs ? I_ResumeMouse() : I_PauseMouse();

		break;
	}

	if (Video->SetMode(width, height, tbits, fs))
		return true;

	// Try the opposite bit mode:
	tbits = bits == 32 ? 8 : 32;
	if (Video->SetMode(width, height, tbits, fs))
		return true;

	// Switch the bit mode back:
	tbits = bits;

	// Try the closest resolution:
	I_ClosestResolution (&width, &height);
	if (Video->SetMode(width, height, tbits, fs))
		return true;

	// Try the opposite bit mode:
	tbits = bits == 32 ? 8 : 32;
	if (Video->SetMode(width, height, tbits, fs))
		return true;

	// Just couldn't get it:
	return false;
	//I_FatalError ("Mode %dx%dx%d is unavailable\n",
	//			width, height, bits);
}

bool I_CheckResolution(int width, int height)
{
	int twidth, theight;

	Video->FullscreenChanged(vid_fullscreen ? true : false);
	Video->StartModeIterator();
	while (Video->NextMode (&twidth, &theight))
	{
		if (width == twidth && height == theight)
			return true;
	}

	// [AM] We only care about correct resolutions if we're fullscreen.
	return !vid_fullscreen;
}

void I_ClosestResolution (int *width, int *height)
{
	int twidth, theight;
	int cwidth = 0, cheight = 0;
	int iteration;
	DWORD closest = 4294967295u;

	Video->FullscreenChanged (vid_fullscreen ? true : false);
	for (iteration = 0; iteration < 2; iteration++)
	{
		Video->StartModeIterator ();
		while (Video->NextMode (&twidth, &theight))
		{
			if (twidth == *width && theight == *height)
				return;

			if (iteration == 0 && (twidth < *width || theight < *height))
				continue;

			DWORD dist = (twidth - *width) * (twidth - *width)
				+ (theight - *height) * (theight - *height);

			if (dist < closest)
			{
				closest = dist;
				cwidth = twidth;
				cheight = theight;
			}
		}
		if (closest != 4294967295u)
		{
			*width = cwidth;
			*height = cheight;
			return;
		}
	}
}

bool I_CheckVideoDriver (const char *name)
{
	if(!name)
		return false;

	return (std::string(name) == Video->GetVideoDriverName());
}

void I_StartModeIterator ()
{
	Video->StartModeIterator ();
}

bool I_NextMode (int *width, int *height)
{
	return Video->NextMode (width, height);
}

DCanvas *I_AllocateScreen (int width, int height, int bits, bool primary)
{
	DCanvas *scrn = Video->AllocateSurface (width, height, bits, primary);

	if (!scrn)
	{
		I_FatalError ("Failed to allocate a %dx%dx%d surface",
					  width, height, bits);
	}

	return scrn;
}

void I_FreeScreen (DCanvas *canvas)
{
	Video->ReleaseSurface (canvas);
}

void I_LockScreen (DCanvas *canvas)
{
	Video->LockSurface (canvas);
}

void I_UnlockScreen (DCanvas *canvas)
{
	Video->UnlockSurface (canvas);
}

void I_Blit (DCanvas *src, int srcx, int srcy, int srcwidth, int srcheight,
			 DCanvas *dest, int destx, int desty, int destwidth, int destheight)
{
    if (!src->m_Private || !dest->m_Private)
		return;

    if (!src->m_LockCount)
		src->Lock ();
    if (!dest->m_LockCount)
		dest->Lock ();

	if (//!Video->CanBlit() ||
		!Video->Blit (src, srcx, srcy, srcwidth, srcheight,
					  dest, destx, desty, destwidth, destheight))
	{
		fixed_t fracxstep, fracystep;
		fixed_t fracx, fracy;
		int x, y;

		if(!destheight || !destwidth)
			return;

		fracy = srcy << FRACBITS;
		fracystep = (srcheight << FRACBITS) / destheight;
		fracxstep = (srcwidth << FRACBITS) / destwidth;

		if (src->is8bit() == dest->is8bit())
		{
			// INDEX8 -> INDEX8 or ARGB8888 -> ARGB8888

			if (dest->is8bit())
			{
				// INDEX8 -> INDEX8
				byte *destline, *srcline;

			if (fracxstep == FRACUNIT)
			{
				for (y = desty; y < desty + destheight; y++, fracy += fracystep)
				{
					memcpy (dest->buffer + y * dest->pitch + destx,
							src->buffer + (fracy >> FRACBITS) * src->pitch + srcx,
							destwidth);
				}
			}
			else
			{
				for (y = desty; y < desty + destheight; y++, fracy += fracystep)
				{
					srcline = src->buffer + (fracy >> FRACBITS) * src->pitch + srcx;
					destline = dest->buffer + y * dest->pitch + destx;
					for (x = fracx = 0; x < destwidth; x++, fracx += fracxstep)
					{
						destline[x] = srcline[fracx >> FRACBITS];
					}
				}
			}
		}
			else
			{
				// ARGB8888 -> ARGB8888
				argb_t *destline, *srcline;

				if (fracxstep == FRACUNIT)
				{
					for (y = desty; y < desty + destheight; y++, fracy += fracystep)
					{
						memcpy ((argb_t *)(dest->buffer + y * dest->pitch) + destx,
								(argb_t *)(src->buffer + (fracy >> FRACBITS) * src->pitch) + srcx,
								destwidth * (dest->bits / 8));
					}
				}
				else
				{
					for (y = desty; y < desty + destheight; y++, fracy += fracystep)
					{
						srcline = (argb_t *)(src->buffer + (fracy >> FRACBITS) * src->pitch) + srcx;
						destline = (argb_t *)(dest->buffer + y * dest->pitch) + destx;
						for (x = fracx = 0; x < destwidth; x++, fracx += fracxstep)
						{
							destline[x] = srcline[fracx >> FRACBITS];
						}
					}
				}
			}
		}
		else if (!src->is8bit() && dest->is8bit())
		{
			// ARGB8888 -> INDEX8
			I_FatalError ("Can't I_Blit() an ARGB8888 source to\nan INDEX8 destination");
		}
		else
		{
			// INDEX8 -> ARGB8888 (Palette set in V_Palette)
			argb_t *destline;
			byte *srcline;

			if (fracxstep == FRACUNIT)
			{
				// No X-scaling
				for (y = desty; y < desty + destheight; y++, fracy += fracystep)
				{
					srcline = src->buffer + (fracy >> FRACBITS) * src->pitch + srcx;
					destline = (argb_t *)(dest->buffer + y * dest->pitch) + destx;
					for (x = 0; x < destwidth; x++)
					{
						destline[x] = V_Palette.shade(srcline[x]);
					}
				}
			}
			else
			{
				// With X-scaling
				for (y = desty; y < desty + destheight; y++, fracy += fracystep)
				{
					srcline = src->buffer + (fracy >> FRACBITS) * src->pitch + srcx;
					destline = (argb_t *)(dest->buffer + y * dest->pitch) + destx;
					for (x = fracx = 0; x < destwidth; x++, fracx += fracxstep)
					{
						destline[x] = V_Palette.shade(srcline[fracx >> FRACBITS]);
					}
				}
			}
		}
	}

    if (!src->m_LockCount)
		I_UnlockScreen (src);
    if (!dest->m_LockCount)
		I_UnlockScreen (dest);
}

// denis - here is a blank implementation of IVideo that allows the client
// to run without actual video output (e.g. script-controlled demo testing)
EDisplayType IVideo::GetDisplayType () { return DISPLAY_Both; }

std::string IVideo::GetVideoDriverName () { return ""; }

bool IVideo::FullscreenChanged (bool fs) { return true; }
void IVideo::SetWindowedScale (float scale) {}
bool IVideo::CanBlit () { return true; }

bool IVideo::SetOverscan (float scale) { return true; }

int IVideo::GetWidth() const { return 0; }
int IVideo::GetHeight() const { return 0; }
int IVideo::GetBitDepth() const { return 0; }

bool IVideo::SetMode (int width, int height, int bits, bool fs) { return true; }
void IVideo::SetPalette (argb_t *palette) {}

void IVideo::SetOldPalette (byte *doompalette) {}
void IVideo::UpdateScreen (DCanvas *canvas) {}
void IVideo::ReadScreen (byte *block) {}

int IVideo::GetModeCount () { return 1; }
void IVideo::StartModeIterator () {}
bool IVideo::NextMode (int *width, int *height) { static int w = 320, h = 240; width = &w; height = &h; return false; }

DCanvas *IVideo::AllocateSurface (int width, int height, int bits, bool primary)
{
	DCanvas *scrn = new DCanvas;

	scrn->width = width;
	scrn->height = height;
	scrn->bits = bits;
	scrn->m_LockCount = 0;
	scrn->m_Palette = NULL;
	// TODO(jsd): Align to 16-byte boundaries for SSE2 optimization!
	scrn->buffer = new byte[width*height*(bits/8)];
	scrn->pitch = width * (bits / 8);

	return scrn;
}

void IVideo::ReleaseSurface (DCanvas *scrn)
{
	delete[] scrn->buffer;
	delete scrn;
}

void IVideo::LockSurface (DCanvas *scrn) {}
void IVideo::UnlockSurface (DCanvas *scrn)  {}
bool IVideo::Blit (DCanvas *src, int sx, int sy, int sw, int sh,
			   DCanvas *dst, int dx, int dy, int dw, int dh) { return true; }

BEGIN_COMMAND (vid_listmodes)
{
	int width, height;

	Video->StartModeIterator ();
		while (Video->NextMode (&width, &height))
		if (width == DisplayWidth && height == DisplayHeight)
			Printf_Bold ("%4d x%5d\n", width, height);
			else
			Printf (PRINT_HIGH, "%4d x%5d\n", width, height);
}
END_COMMAND (vid_listmodes)

BEGIN_COMMAND (vid_currentmode)
{
	Printf (PRINT_HIGH, "%dx%dx%d\n", DisplayWidth, DisplayHeight, DisplayBits);
}
END_COMMAND (vid_currentmode)








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
	mWindow(window)
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
					int destpitch, int srcpitch, int destw, int desth,
					int xstep, int ystep, const argb_t* palette)
{
	fixed_t yfrac = 0;
	for (int y = 0; y < desth; y++)
	{
		fixed_t xfrac = 0;

		for (int x = 0; x < destw; x++)
		{

			dest[x] = ConvertPixel<SOURCE_PIXEL_T, DEST_PIXEL_T>(source[xfrac >> FRACBITS], palette);
			xfrac += xstep;
		}

		dest += destpitch;
		yfrac += ystep;
		
		source += srcpitch * (yfrac >> FRACBITS);
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
	srcw = std::min(srcx + srcw, srcx + source_surface->getWidth());
	srch = std::min(srcy + srch, srcy + source_surface->getHeight());

	if (srcw == 0 || srch == 0)
		return;

	// clamp to destination surface edges
	destw = std::min(destx + destw, destx + getWidth());
	desth = std::min(desty + desth, desty + getHeight());
	
	if (destw == 0 || desth == 0)
		return;

	fixed_t xstep = FixedDiv(srcw << FRACBITS, destw << FRACBITS);
	fixed_t ystep = FixedDiv(srch << FRACBITS, desth << FRACBITS);

	int srcbits = source_surface->getBitsPerPixel();
	int destbits = getBitsPerPixel();
	int srcpitch = source_surface->getPitch() * 8 / srcbits;
	int destpitch = getPitch() * 8 / destbits;

	if (srcbits == 8 && destbits == 8)
	{
		const palindex_t* source = (palindex_t*)source_surface->getBuffer() + srcy * srcpitch + srcx;
		palindex_t* dest = (palindex_t*)getBuffer() + desty * destpitch + destx;

		BlitLoop(dest, source, destpitch, srcpitch, destw, desth, xstep, ystep, getPalette());
	}
	else if (srcbits == 8 && destbits == 32)
	{
		const palindex_t* source = (palindex_t*)source_surface->getBuffer() + srcy * srcpitch + srcx;
		argb_t* dest = (argb_t*)getBuffer() + desty * destpitch + destx;

		BlitLoop(dest, source, destpitch, srcpitch, destw, desth, xstep, ystep, getPalette());
	}
	else if (srcbits == 32 && destbits == 8)
	{
		// we can't quickly convert from 32bpp source to 8bpp dest so don't bother
		return;
	}
	else if (srcbits == 32 && destbits == 32)
	{
		const argb_t* source = (argb_t*)source_surface->getBuffer() + srcy * srcpitch + srcx;
		argb_t* dest = (argb_t*)getBuffer() + desty * destpitch + destx;

		BlitLoop(dest, source, destpitch, srcpitch, destw, desth, xstep, ystep, getPalette());
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
	DCanvas* canvas = new DCanvas;

	canvas->width = getWidth();
	canvas->height = getHeight();
	canvas->bits = getBitsPerPixel();
	canvas->pitch = getPitch();

	canvas->buffer = getBuffer();

	canvas->m_LockCount = 0;
	canvas->m_Palette = NULL;

	mCanvasStore.push_back(canvas);

	return canvas;
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
	if (canvas->buffer != getBuffer())
		I_Error("IWindowSurface::releaseCanvas: releasing canvas not owned by this surface\n");	

	// Remove the DCanvas pointer from the surface's list of allocated canvases
	DCanvasCollection::iterator it = std::find(mCanvasStore.begin(), mCanvasStore.end(), canvas);
	if (it != mCanvasStore.end())
		mCanvasStore.erase(it);

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
// I_GetFrameBuffer
//
// Returns a pointer to the raw frame buffer for the game window's primary
// surface.
//
byte* I_GetFrameBuffer()
{
	if (!window)
		return NULL;

	return window->getPrimarySurface()->getBuffer();
}


//
// I_GetSurfaceWidth
//
int I_GetSurfaceWidth()
{
	return surface_width;
}


//
// I_GetSurfaceHeight
//
int I_GetSurfaceHeight()
{
	return surface_height;
}


//
// I_SetWindowSize
//
// Resizes the application window to the specified size.
//
void I_SetWindowSize(int width, int height)
{
	if (window)
		window->resize(width, height);
}


//
// I_SetSurfaceSize
//
// Resizes the drawing surface to the specified size.
// TODO: Surface size should be completely independent of window size.
//
void I_SetSurfaceSize(int width, int height)
{
	if (window)
		window->resize(width, height);
}


VERSION_CONTROL (i_video_cpp, "$Id$")


