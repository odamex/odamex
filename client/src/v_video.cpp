// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 2006-2020 by The Odamex Team.
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

#include "i_system.h"
#include "i_video.h"
#include "r_local.h"
#include "r_draw.h"
#include "r_state.h"

#include "doomdef.h"
#include "doomstat.h"
#include "d_main.h"

#include "c_console.h"

#include "m_argv.h"
#include "m_bbox.h"

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

// [RH] The framebuffer is no longer a mere byte array.
// There's also only one, not four.
DCanvas *screen;

static DBoundingBox dirtybox;

static bool V_DoSetResolution();
static void BuildTransTable(const argb_t* palette_colors);

// flag to indicate that V_AdjustVideoMode should try to change the video mode
static bool setmodeneeded = false;


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
		setmodeneeded = false;

		// [SL] surface buffer address will be changing
		// so just end the screen-wipe
		Wipe_Stop();

		// Change screen mode.
		if (!V_DoSetResolution())
			I_FatalError("Could not change screen mode");

		// Refresh the console.
		C_NewModeAdjust();

		// Recalculate various view parameters.
		R_ForceViewWindowResize();
	}
}


EXTERN_CVAR(vid_defwidth)
EXTERN_CVAR(vid_defheight)
EXTERN_CVAR(vid_32bpp)
EXTERN_CVAR(vid_fullscreen)
EXTERN_CVAR(vid_filter)
EXTERN_CVAR(vid_widescreen)
EXTERN_CVAR(sv_allowwidescreen)
EXTERN_CVAR(vid_vsync)
EXTERN_CVAR(vid_pillarbox)

static int vid_pillarbox_old = -1;


static IVideoMode V_GetRequestedVideoMode()
{
	int surface_bpp = vid_32bpp ? 32 : 8;
	EWindowMode window_mode = (EWindowMode)vid_fullscreen.asInt();
	bool vsync = (vid_vsync != 0.0f);
	const std::string stretch_mode(vid_filter);

	return IVideoMode(vid_defwidth.asInt(), vid_defheight.asInt(), surface_bpp, window_mode, vsync, stretch_mode);
}


bool V_CheckModeAdjustment()
{
	const IWindow* window = I_GetWindow();
	if (!window)
		return false;

	if (V_GetRequestedVideoMode() != window->getVideoMode())
		return true;

	bool using_widescreen = I_IsWideResolution();
	if (vid_widescreen && sv_allowwidescreen != using_widescreen)
		return true;

	if (vid_widescreen != using_widescreen)
		return true;

	if (vid_pillarbox_old != vid_pillarbox)
	{
		vid_pillarbox_old = vid_pillarbox;
		return true;
	}

	return false;
}


CVAR_FUNC_IMPL(vid_defwidth)
{
	if (var < 320 || var > MAXWIDTH)
		var.RestoreDefault();
	
	if (gamestate != GS_STARTUP && V_CheckModeAdjustment())
		V_ForceVideoModeAdjustment();
}


CVAR_FUNC_IMPL(vid_defheight)
{
	if (var < 200 || var > MAXHEIGHT)
		var.RestoreDefault();
	
	if (gamestate != GS_STARTUP && V_CheckModeAdjustment())
		V_ForceVideoModeAdjustment();
}


CVAR_FUNC_IMPL(vid_fullscreen)
{
	if (gamestate != GS_STARTUP && V_CheckModeAdjustment())
		V_ForceVideoModeAdjustment();
}


CVAR_FUNC_IMPL(vid_filter)
{
	if (gamestate != GS_STARTUP && V_CheckModeAdjustment())
		V_ForceVideoModeAdjustment();
}


CVAR_FUNC_IMPL(vid_32bpp)
{
	if (gamestate != GS_STARTUP && V_CheckModeAdjustment())
		V_ForceVideoModeAdjustment();
}


CVAR_FUNC_IMPL(vid_vsync)
{
	if (gamestate != GS_STARTUP && V_CheckModeAdjustment())
		V_ForceVideoModeAdjustment();
}


CVAR_FUNC_IMPL(vid_overscan)
{
	if (gamestate != GS_STARTUP)
		V_ForceVideoModeAdjustment();
}


CVAR_FUNC_IMPL(vid_320x200)
{
	if (gamestate != GS_STARTUP)
		V_ForceVideoModeAdjustment();
}


CVAR_FUNC_IMPL(vid_640x400)
{
	if (gamestate != GS_STARTUP)
		V_ForceVideoModeAdjustment();
}


CVAR_FUNC_IMPL (vid_widescreen)
{
	if (gamestate != GS_STARTUP && V_CheckModeAdjustment())
		V_ForceVideoModeAdjustment();
}


CVAR_FUNC_IMPL(vid_pillarbox)
{
	if (gamestate != GS_STARTUP && V_CheckModeAdjustment())
		V_ForceVideoModeAdjustment();
}

//
// Only checks to see if the widescreen mode is proper compared to sv_allowwidescreen.
//
// Doing a full check on Windows results in strange flashing behavior in fullscreen
// because there's an 32-bit surface being rendered to as 8-bit.
//
static bool CheckWideModeAdjustment()
{
	bool using_widescreen = I_IsWideResolution();
	if (vid_widescreen && sv_allowwidescreen != using_widescreen)
		return true;

	if (vid_widescreen != using_widescreen)
		return true;

	return false;
}

CVAR_FUNC_IMPL (sv_allowwidescreen)
{
	if (gamestate != GS_STARTUP && CheckWideModeAdjustment())
		V_ForceVideoModeAdjustment();
}


CVAR_FUNC_IMPL(vid_maxfps)
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


//
// vid_listmodes
//
// Prints a list of all supported video modes, highlighting the current
// video mode. Requires I_VideoInitialized() to be true.
//
BEGIN_COMMAND(vid_listmodes)
{
	const IVideoModeList* modelist = I_GetVideoCapabilities()->getSupportedVideoModes();

	for (IVideoModeList::const_iterator it = modelist->begin(); it != modelist->end(); ++it)
	{
		if (*it == I_GetWindow()->getVideoMode())
			Printf_Bold("%s\n", I_GetVideoModeString(*it).c_str());
		else
			Printf(PRINT_HIGH, "%s\n", I_GetVideoModeString(*it).c_str());
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
	std::string pixel_string;

	const PixelFormat* format = I_GetWindow()->getPrimarySurface()->getPixelFormat();
	if (format->getBitsPerPixel() == 8)
	{
		pixel_string = "palettized";
	}
	else
	{
		char temp_str[9] = { 0 };
		argb_t* d1 = (argb_t*)temp_str;
		argb_t* d2 = (argb_t*)temp_str + 1;

		d1->seta('A'); d1->setr('R'); d1->setg('G'); d1->setb('B');
		d2->seta('0' + format->getABits()); d2->setr('0' + format->getRBits());
		d2->setg('0' + format->getGBits()); d2->setb('0' + format->getBBits());

		pixel_string = std::string(temp_str);
	}

	const IVideoMode& mode = I_GetWindow()->getVideoMode();
	Printf(PRINT_HIGH, "%s %s surface\n",
			I_GetVideoModeString(mode).c_str(), pixel_string.c_str());
}
END_COMMAND(vid_currentmode)


BEGIN_COMMAND(checkres)
{
	Printf(PRINT_HIGH, "%dx%d\n", I_GetVideoWidth(), I_GetVideoHeight());
}
END_COMMAND(checkres)


//
// vid_setmode
//
// Sets the video mode resolution.
// The actual change of resolution will take place at the start of the next frame.
//
BEGIN_COMMAND(vid_setmode)
{
	int width = 0, height = 0;

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
		height = vid_defheight;

	if (width < 320 || height < 200)
	{
		Printf(PRINT_HIGH, "%dx%d is too small.  Minimum resolution is 320x200.\n", width, height);
		return;
	}

	if (width > MAXWIDTH || height > MAXHEIGHT)
	{
		Printf(PRINT_HIGH, "%dx%d is too large.  Maximum resolution is %dx%d.\n", width, height, MAXWIDTH, MAXHEIGHT);
		return;
	}

	vid_defwidth.Set(width);
	vid_defheight.Set(height);
}
END_COMMAND (vid_setmode)


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
// V_DoSetResolution
//
// Changes the video resolution to the given dimensions. This should only
// be called at the begining of drawing a frame (or during startup)!
//
static bool V_DoSetResolution()
{
	const IVideoMode requested_mode = V_GetRequestedVideoMode();

	I_SetVideoMode(requested_mode);
	if (!I_VideoInitialized())
		return false;

	V_Init();

	return true;
}



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
	{
		int video_width = M_GetParmValue("-width");
		int video_height = M_GetParmValue("-height");
		int video_bpp = M_GetParmValue("-bits");

		// ensure the width & height cvars are sane
		if (vid_defwidth.asInt() <= 0 || vid_defheight.asInt() <= 0)
		{
			vid_defwidth.RestoreDefault();
			vid_defheight.RestoreDefault();
		}
		
		if (video_width == 0 && video_height == 0)
		{
			video_width = vid_defwidth.asInt();
			video_height = vid_defheight.asInt();
		}
		else if (video_width == 0)
		{
			video_width = video_height * 4 / 3;
		}
		else if (video_height == 0)
		{
			video_height = video_width * 3 / 4;
		}

		vid_defwidth.Set(video_width);
		vid_defheight.Set(video_height);

		if (video_bpp == 0 || (video_bpp != 8 && video_bpp != 32))
			video_bpp = vid_32bpp ? 32 : 8;
		vid_32bpp.Set(video_bpp == 32);

		V_DoSetResolution();

		Printf(PRINT_HIGH, "V_Init: using %s video driver.\n", I_GetVideoDriverName().c_str());
	}

	if (!I_VideoInitialized())
		I_FatalError("Failed to initialize display");

	V_InitPalette("PLAYPAL");

	if (realcolormaps.colormap)
		Z_Free(realcolormaps.colormap);
	if (realcolormaps.shademap)
		Z_Free(realcolormaps.shademap);

	R_InitColormaps();

	int surface_width = I_GetSurfaceWidth(), surface_height = I_GetSurfaceHeight();

	// This uses the smaller of the two results. It's still not ideal but at least
	// this allows hud_scaletext to have some purpose...
	CleanXfac = CleanYfac = std::max(1, std::min(surface_width / 320, surface_height / 200));

	R_InitColumnDrawers();

	I_SetWindowCaption(D_GetTitleString());
	I_SetWindowIcon();

	// notify the console of changes in the screen resolution
	C_NewModeAdjust();

	BuildTransTable(V_GetDefaultPalette()->basecolors);

	vid_pillarbox_old = vid_pillarbox;
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
// V_DrawTickerDot
//
// Templated helper function for V_DrawFPSTicker. It draws a single dot for
// the vanilla Doom FPS ticker, scaled to make the dot roughly the same size
// as it would be in 320x200 video modes.
//
template <typename PIXEL_T>
static void V_DrawTickerDot(IWindowSurface* surface, int n, PIXEL_T color)
{
	const int dot_width = CleanXfac, dot_height = CleanYfac;
	const int pitch_in_pixels = surface->getPitchInPixels();

	PIXEL_T* dest = (PIXEL_T*)surface->getBuffer() +
			(surface->getHeight() - 1 - dot_height) * pitch_in_pixels +
			2 * n * dot_width; 

	for (int y = 0; y < dot_height; y++)
	{
		for (int x = 0; x < dot_width; x++)
			dest[x] = color;
		dest += pitch_in_pixels;
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

	if (I_GetPrimarySurface()->getBitsPerPixel() == 8)
	{
		const palindex_t oncolor = 255;
		const palindex_t offcolor = 0;

		int n = 0;
		for (n = 0; n < tics; n++)
			V_DrawTickerDot<palindex_t>(I_GetPrimarySurface(), n, oncolor);
		for ( ; n < 20; n++)
			V_DrawTickerDot<palindex_t>(I_GetPrimarySurface(), n, offcolor);
	}
	else
	{
		const argb_t oncolor(255, 255, 255);
		const argb_t offcolor(0, 0, 0);

		int n = 0;
		for (n = 0; n < tics; n++)
			V_DrawTickerDot<argb_t>(I_GetPrimarySurface(), n, oncolor);
		for ( ; n < 20; n++)
			V_DrawTickerDot<argb_t>(I_GetPrimarySurface(), n, offcolor);
	}
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


EXTERN_CVAR (ui_dimamount)
EXTERN_CVAR (ui_dimcolor)

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
			Col2RGB8[x][y] = (((palette_colors[y].getr() * x) >> 4) << 20)  |
							  ((palette_colors[y].getg() * x )>> 4) |
							 (((palette_colors[y].getb() * x) >> 4) << 10);
}


VERSION_CONTROL (v_video_cpp, "$Id$")

