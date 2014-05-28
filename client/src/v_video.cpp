// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
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
//		Functions to draw patches (by post) directly to screen->
//		Functions to blit a block to the screen->
//
//-----------------------------------------------------------------------------



#include <stdio.h>
#include <assert.h>

#include "m_alloc.h"

#include "i_system.h"
#include "i_video.h"
#include "r_local.h"
#include "r_draw.h"
#include "r_plane.h"
#include "r_state.h"

#include "doomdef.h"
#include "doomdata.h"
#include "doomstat.h"
#include "d_main.h"

#include "c_console.h"
#include "hu_stuff.h"

#include "m_argv.h"
#include "m_bbox.h"
#include "m_swap.h"
#include "m_menu.h"

#include "v_video.h"
#include "v_text.h"

#include "w_wad.h"
#include "z_zone.h"

#include "c_cvars.h"
#include "c_dispatch.h"
#include "cmdlib.h"

#include "f_wipe.h"

IMPLEMENT_CLASS (DCanvas, DObject)

argb_t Col2RGB8[65][256];
palindex_t RGB32k[32][32][32];

void I_FlushInput();


// [RH] Set true when vid_setmode command has been executed
static bool setmodeneeded = false;
// [RH] Resolution to change to when setmodeneeded is true
int		NewWidth, NewHeight, NewBits;

//
//	V_ForceVideoModeAdjustment
//
// Tells the video subsystem to change the video mode and recalculate any
// lookup tables dependent on the video mode prior to drawing the next frame.
//
void V_ForceVideoModeAdjustment()
{
	setmodeneeded = true;
}


// [RH] The framebuffer is no longer a mere byte array.
// There's also only one, not four.
DCanvas *screen;

DBoundingBox dirtybox;

EXTERN_CVAR (vid_defwidth)
EXTERN_CVAR (vid_defheight)
EXTERN_CVAR (vid_32bpp)
EXTERN_CVAR (vid_vsync)
EXTERN_CVAR (vid_fullscreen)
EXTERN_CVAR (vid_320x200)
EXTERN_CVAR (vid_640x400)
EXTERN_CVAR (vid_autoadjust)
EXTERN_CVAR (vid_overscan)
EXTERN_CVAR (vid_ticker)

CVAR_FUNC_IMPL (vid_maxfps)
{
	if (var == 0)
	{
		capfps = false;
		maxfps = 99999.0f;
	}
	else
	{
		if (var < 35.0f)
			var.Set(35.0f);
		else
		{
			capfps = true;
			maxfps = var;
		}
	}
}

EXTERN_CVAR (ui_dimamount)
EXTERN_CVAR (ui_dimcolor)

CVAR_FUNC_IMPL (vid_fullscreen)
{
	V_ForceVideoModeAdjustment();
	NewWidth = I_GetVideoWidth();
	NewHeight = I_GetVideoHeight();
	NewBits = I_GetVideoBitDepth();
}

CVAR_FUNC_IMPL (vid_32bpp)
{
	V_ForceVideoModeAdjustment();
	NewBits = (int)vid_32bpp ? 32 : 8;
}


//
// V_MarkRect
//
void V_MarkRect(int x, int y, int width, int height)
{
	dirtybox.AddToBox(x, y);
	dirtybox.AddToBox(x + width - 1, y + height - 1);
}


//
// DCanvas::getCleanX
//
// Returns the real screen x coordinate given the virtual 320x200 x coordinate.
//
int DCanvas::getCleanX(int x) const
{
	return (x - 160) * CleanXfac + mSurface->getWidth() / 2;
}


//
// DCanvas::getCleanY
//
// Returns the real screen y coordinate given the virtual 320x200 y coordinate.
//
int DCanvas::getCleanY(int y) const
{
	return (y - 100) * CleanYfac + mSurface->getHeight() / 2;
}


// [RH] Fill an area with a 64x64 flat texture
//		right and bottom are one pixel *past* the boundaries they describe.
void DCanvas::FlatFill(int left, int top, int right, int bottom, const byte* src) const
{
	int surface_advance = mSurface->getPitchInPixels() - right + left;

	if (mSurface->getBitsPerPixel() == 8)
	{
		palindex_t* dest = (palindex_t*)mSurface->getBuffer() + top * mSurface->getPitchInPixels() + left;

		for (int y = top; y < bottom; y++)
		{
			int x = left;
			while (x < right)
			{
				int amount = std::min(64 - (x & 63), right - x);
				memcpy(dest, src + ((y & 63) << 6) + (x & 63), amount);
				dest += amount;
				x += amount;
			}

			dest += surface_advance;
		}
	}
	else
	{
		argb_t* dest = (argb_t*)mSurface->getBuffer() + top * mSurface->getPitchInPixels() + left;

		for (int y = top; y < bottom; y++)
		{
			const byte* src_line = src + ((y & 63) << 6);
			for (int x = left; x < right; x++)
				*dest++ = V_Palette.shade(src_line[x & 63]);

			dest += surface_advance;
		}
	}
}


// [SL] Stretches a patch to fill the full-screen while maintaining a 4:3
// aspect ratio. Pillarboxing is used in widescreen resolutions.
void DCanvas::DrawPatchFullScreen(const patch_t* patch) const
{
	mSurface->clear();

	int surface_width = mSurface->getWidth(), surface_height = mSurface->getHeight();

	int destw, desth;

	if (I_IsProtectedResolution(I_GetVideoWidth(), I_GetVideoHeight()))
	{
		destw = surface_width; 
		desth = surface_height; 
	}
	else if (surface_width * 3 >= surface_height * 4)
	{
		destw = surface_height * 4 / 3;
		desth = surface_height;
	}
	else
	{
		destw = surface_width;
		desth = surface_width * 3 / 4;
	}

	int x = (surface_width - destw) / 2;
	int y = (surface_height - desth) / 2;

	DrawPatchStretched(patch, x, y, destw, desth);
}


// [RH] Set an area to a specified color
void DCanvas::Clear(int left, int top, int right, int bottom, argb_t color) const
{
	int surface_pitch_pixels = mSurface->getPitchInPixels();

	if (mSurface->getBitsPerPixel() == 8)
	{
		palindex_t color_index = V_BestColor(V_GetDefaultPalette()->basecolors, color);
		palindex_t* dest = (palindex_t*)mSurface->getBuffer() + top * surface_pitch_pixels + left;

		int line_length = (right - left) * sizeof(palindex_t);
		for (int y = top; y < bottom; y++)
		{
			memset(dest, color_index, line_length);
			dest += surface_pitch_pixels;
		}
	}
	else
	{
		color = V_GammaCorrect(color);
		argb_t* dest = (argb_t*)mSurface->getBuffer() + top * surface_pitch_pixels + left;

		for (int y = top; y < bottom; y++)
		{
			for (int x = 0; x < right - left; x++)
				dest[x] = color;
			dest += surface_pitch_pixels;
		}
	}
}


void DCanvas::Dim(int x1, int y1, int w, int h, const char* color_str, float famount) const
{
	int surface_width = mSurface->getWidth(), surface_height = mSurface->getHeight();
	int surface_pitch_pixels = mSurface->getPitchInPixels();

	if (x1 < 0 || x1 + w > surface_width || y1 < 0 || y1 + h > surface_height)
		return;

	if (famount <= 0.0f)
		return;
	else if (famount > 1.0f)
		famount = 1.0f;

	if (mSurface->getBitsPerPixel() == 8)
	{
		int bg;
		int x, y;

		int amount = (int)(famount * 64.0f);
		argb_t *fg2rgb = Col2RGB8[amount];
		argb_t *bg2rgb = Col2RGB8[64-amount];

		argb_t color = V_GetColorFromString(color_str);
		unsigned int fg = fg2rgb[V_BestColor(V_GetDefaultPalette()->basecolors, color)];

		palindex_t* dest = (palindex_t*)mSurface->getBuffer() + y1 * surface_pitch_pixels + x1;
		int advance = surface_pitch_pixels - w;

		int xcount = w / 4;
		int xcount_remainder = w % 4;

		for (y = h; y > 0; y--)
		{
			for (x = xcount; x > 0; x--)
			{
				// Unroll the loop for a speed improvement
				bg = bg2rgb[*dest];
				bg = (fg+bg) | 0x1f07c1f;
				*dest++ = RGB32k[0][0][bg&(bg>>15)];

				bg = bg2rgb[*dest];
				bg = (fg+bg) | 0x1f07c1f;
				*dest++ = RGB32k[0][0][bg&(bg>>15)];

				bg = bg2rgb[*dest];
				bg = (fg+bg) | 0x1f07c1f;
				*dest++ = RGB32k[0][0][bg&(bg>>15)];

				bg = bg2rgb[*dest];
				bg = (fg+bg) | 0x1f07c1f;
				*dest++ = RGB32k[0][0][bg&(bg>>15)];
			}
			for (x = xcount_remainder; x > 0; x--)
			{
				// account for widths that aren't multiples of 4
				bg = bg2rgb[*dest];
				bg = (fg+bg) | 0x1f07c1f;
				*dest++ = RGB32k[0][0][bg&(bg>>15)];
			}
			dest += advance;
		}
	}
	else
	{
		argb_t color = V_GammaCorrect(V_GetColorFromString(color_str));

		r_dimpatchD(mSurface, color, (int)(famount * 256.0f), x1, y1, w, h);
	}
}

void DCanvas::Dim(int x1, int y1, int w, int h) const
{
	if (ui_dimamount < 0.0f)
		ui_dimamount.Set (0.0f);
	else if (ui_dimamount > 1.0f)
		ui_dimamount.Set (1.0f);

	if (ui_dimamount == 0.0f)
		return;

	Dim(x1, y1, w, h, ui_dimcolor.cstring(), ui_dimamount);
}


BEGIN_COMMAND (setcolor)
{
	if (argc < 3)
	{
		Printf (PRINT_HIGH, "Usage: setcolor <cvar> <color>\n");
		return;
	}

	std::string name = C_ArgCombine(argc - 2, (const char **)(argv + 2));
	if (name.length())
	{
		std::string desc = V_GetColorStringByName (name.c_str());

		if (desc.length())
		{
			std::string setcmd = "set ";
			setcmd += argv[1];
			setcmd += " \"";
			setcmd += desc;
			setcmd += "\"";
			AddCommandString (setcmd);
		}
	}
}
END_COMMAND (setcolor)

// Build the tables necessary for translucency
static void BuildTransTable(const argb_t* palette_colors)
{
	// create the small RGB table
	for (int r = 0; r < 32; r++)
		for (int g = 0; g < 32; g++)
			for (int b = 0; b < 32; b++)
				RGB32k[r][g][b] = V_BestColor(palette_colors, (r<<3)|(r>>2), (g<<3)|(g>>2), (b<<3)|(b>>2));

	for (int x = 0; x < 65; x++)
		for (int y = 0; y < 256; y++)
			Col2RGB8[x][y] = (((palette_colors[y].r * x) >> 4) << 20)  |
							  ((palette_colors[y].g * x )>> 4) |
							 (((palette_colors[y].b * x) >> 4) << 10);
}

CVAR_FUNC_IMPL (vid_widescreen)
{
	V_ForceVideoModeAdjustment();
}

CVAR_FUNC_IMPL (sv_allowwidescreen)
{
	V_ForceVideoModeAdjustment();
}

//
// V_UsePillarBox
//
// Determines if the display should use pillarboxing. If the resolution is a
// widescreen mode and either the user or the server doesn't allow
// widescreen usage, use pillarboxing.
//
bool V_UsePillarBox()
{
	int width = I_GetVideoWidth(), height = I_GetVideoHeight();

	if (width == 0 || height == 0)
		return false;

	if (I_IsProtectedResolution(width, height))
		return false;

	if (vid_320x200 || vid_640x400)
		return 3 * width > 4 * height;

	return (!vid_widescreen || (!serverside && !sv_allowwidescreen))
		&& (3 * width > 4 * height);
}

//
// V_UseLetterBox
//
// Determines if the display should use letterboxing. If the resolution is a
// standard 4:3 mode and both the user and the server allow widescreen
// usage, use letterboxing.
//
bool V_UseLetterBox()
{
	int width = I_GetVideoWidth(), height = I_GetVideoHeight();

	if (width == 0 || height == 0)
		return false;

	if (I_IsProtectedResolution(width, height))
		return false;

	if (vid_320x200 || vid_640x400)
		return 3 * width <= 4 * height;

	return (vid_widescreen && (serverside || sv_allowwidescreen))
		&& (3 * width <= 4 * height);
}

//
// V_UseWidescreen
//
//
bool V_UseWidescreen()
{
	int width = I_GetVideoWidth(), height = I_GetVideoHeight();

	if (width == 0 || height == 0)
		return false;

	if (I_IsProtectedResolution(width, height))
		return false;

	if (vid_320x200 || vid_640x400)
		return 3 * width > 4 * height;

	return (vid_widescreen && (serverside || sv_allowwidescreen))
		&& (3 * width > 4 * height);
}


//
// V_SetResolution
//
bool V_SetResolution(int width, int height, int bpp)
{
	bool fullscreen = (vid_fullscreen != 0.0f);
	bool vsync = (vid_vsync != 0.0f);

	I_SetVideoMode(width, height, bpp, fullscreen, vsync);
	if (!I_VideoInitialized())
		return false;

	V_Init();

	return true;
}

BEGIN_COMMAND(vid_setmode)
{
	int width = 0, height = 0;
	int bpp = (int)vid_32bpp ? 32 : 8;

	// No arguments
	if (argc == 1)
	{
		Printf(PRINT_HIGH, "Usage: vid_setmode <width> <height>\n");
		return;
	}

	// Width
	if (argc > 1)
		width = atoi(argv[1]);

	// Height (optional)
	if (argc > 2)
		height = atoi(argv[2]);
	if (height == 0)
		height = I_GetVideoHeight();

	if (width < 320 || height < 200)
		Printf(PRINT_HIGH, "%dx%d is too small.  Minimum resolution is 320x200.\n", width, height);

	if (width > MAXWIDTH || height > MAXHEIGHT)
		Printf(PRINT_HIGH, "%dx%d is too large.  Maximum resolution is %dx%d.\n", width, height, MAXWIDTH, MAXHEIGHT);

	// The actual change of resolution will take place
	// near the beginning of D_Display().
	if (gamestate != GS_STARTUP)
	{
		V_ForceVideoModeAdjustment();
		NewWidth = width;
		NewHeight = height;
		NewBits = bpp;
	}
}
END_COMMAND (vid_setmode)

BEGIN_COMMAND (checkres)
{
    Printf (PRINT_HIGH, "Resolution: %d x %d x %d (%s)\n",
			I_GetVideoWidth(), I_GetVideoHeight(), I_GetVideoBitDepth(),
        	(vid_fullscreen ? "FULLSCREEN" : "WINDOWED")); // NES - Simple resolution checker.
}
END_COMMAND (checkres)


//
// V_Close
//
void STACK_ARGS V_Close()
{
	R_ShutdownColormaps();
}


//
// V_Init
//
void V_Init()
{
	if (!I_VideoInitialized())
		I_FatalError("Failed to initialize display");

	V_InitPalette("PLAYPAL");

	R_InitColormaps();

	int surface_width = I_GetSurfaceWidth(), surface_height = I_GetSurfaceHeight();

	// This uses the smaller of the two results. It's still not ideal but at least
	// this allows hud_scaletext to have some purpose...
	CleanXfac = CleanYfac = std::max(1, std::min(surface_width / 320, surface_height / 200));

	R_InitColumnDrawers();

	// [SL] 2011-11-30 - Prevent the player's view angle from moving
	I_FlushInput();

	I_SetWindowCaption(D_GetTitleString());
	I_SetWindowIcon();

	// notify the console of changes in the screen resolution
	C_NewModeAdjust();

	BuildTransTable(V_GetDefaultPalette()->basecolors);
}


//
// V_AdjustVideoMode
//
// Checks if the video mode needs to be changed and calls several video mode
// dependent initialization routines if it does. This should be called at the
// start of drawing a video frame.
//
void V_AdjustVideoMode()
{
	if (setmodeneeded)
	{
		// [SL] surface buffer address will be changing
		// so just end the screen-wipe
		Wipe_Stop();

		// Change screen mode.
		if (!V_SetResolution(NewWidth, NewHeight, NewBits))
			I_FatalError("Could not change screen mode");

		// Refresh the console.
		C_NewModeAdjust();

		// Recalculate various view parameters.
		R_ForceViewWindowResize();

		setmodeneeded = false;
	}
}


//
// V_DrawFPSWidget
//
void V_DrawFPSWidget()
{
	static const dtime_t ONE_SECOND = I_ConvertTimeFromMs(1000);

	static dtime_t last_time = I_GetTime();
	static dtime_t time_accum = 0;
	static unsigned int frame_count = 0;

	dtime_t current_time = I_GetTime();
	dtime_t delta_time = current_time - last_time;
	last_time = current_time;
	frame_count++;

	if (delta_time > 0)
	{
		static double last_fps = 0.0;
		static char fpsbuff[40];

		double delta_time_ms = 1000.0 * double(delta_time) / ONE_SECOND;
		int len = sprintf(fpsbuff, "%5.1fms (%.2f fps)", delta_time_ms, last_fps);
		screen->Clear(0, I_GetSurfaceHeight() - 8, len * 8, I_GetSurfaceHeight(), argb_t(0, 0, 0));
		screen->PrintStr(0, I_GetSurfaceHeight() - 8, fpsbuff, CR_GRAY);

		time_accum += delta_time;

		// calculate last_fps every 1000ms
		if (time_accum > ONE_SECOND)
		{
			last_fps = double(ONE_SECOND * frame_count) / time_accum;
			time_accum = 0;
			frame_count = 0;
		}
	}
}


//
// V_DrawFPSTicker
//
void V_DrawFPSTicker()
{
	int current_tic = int(I_GetTime() * TICRATE / I_ConvertTimeFromMs(1000));
	static int last_tic = current_tic;
	
	int tics = clamp(current_tic - last_tic, 0, 20);
	last_tic = current_tic;

	IWindowSurface* surface = I_GetPrimarySurface();
	int surface_height = surface->getHeight();
	int surface_pitch = surface->getPitch();

	if (surface->getBitsPerPixel() == 8)
	{
		const palindex_t oncolor = 255;
		const palindex_t offcolor = 0;
		palindex_t* dest = (palindex_t*)(surface->getBuffer() + (surface_height - 1) * surface_pitch);

		int i = 0;
		for (i = 0; i < tics*2; i += 2)
			dest[i] = oncolor;
		for ( ; i < 20*2; i += 2)
			dest[i] = offcolor;
	}
	else
	{
		const argb_t oncolor(255, 255, 255);
		const argb_t offcolor(0, 0, 0);

		argb_t* dest = (argb_t*)(surface->getBuffer() + (surface_height - 1) * surface_pitch);

		int i = 0;
		for (i = 0; i < tics*2; i += 2)
			dest[i] = oncolor;
		for ( ; i < 20*2; i += 2)
			dest[i] = offcolor;
	}
}

VERSION_CONTROL (v_video_cpp, "$Id$")

