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

IMPLEMENT_CLASS (DCanvas, DObject)

argb_t Col2RGB8[65][256];
palindex_t RGB32k[32][32][32];

void I_FlushInput();

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

// [RH] Set true when vid_setmode command has been executed
BOOL	setmodeneeded = false;
// [RH] Resolution to change to when setmodeneeded is true
int		NewWidth, NewHeight, NewBits;

CVAR_FUNC_IMPL (vid_fullscreen)
{
	setmodeneeded = true;
	NewWidth = I_GetVideoWidth();
	NewHeight = I_GetVideoHeight();
	NewBits = I_GetVideoBitDepth();
}

CVAR_FUNC_IMPL (vid_32bpp)
{
	setmodeneeded = true;
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
	int width = mSurface->getWidth(), height = mSurface->getHeight();
	Clear(0, 0, width, height, 0);

	if (width == 320 && height == 200)
	{
		DrawPatch(patch, 0, 0);
	}
	else if (width * 3 > height * 4)
	{
		// widescreen resolution - draw pic in 4:3 ratio in center of screen
		int picwidth = 4 * height / 3;
		int picheight = height;
		DrawPatchStretched(patch, (width - picwidth) / 2, 0, picwidth, picheight);
	}
	else
	{
		// 4:3 resolution - draw pic to the entire screen
		DrawPatchStretched(patch, 0, 0, width, height);
	}
}


// [RH] Set an area to a specified color
void DCanvas::Clear(int left, int top, int right, int bottom, int color) const
{
	int surface_pitch_pixels = mSurface->getPitchInPixels();

	if (mSurface->getBitsPerPixel() == 8)
	{
		palindex_t* dest = (palindex_t*)mSurface->getBuffer()
					+ top * surface_pitch_pixels + left;

		int line_length = (right - left) * sizeof(palindex_t);
		for (int y = top; y < bottom; y++)
		{
			memset(dest, color, line_length);
			dest += surface_pitch_pixels;
		}
	}
	else
	{
		argb_t* dest = (argb_t*)mSurface->getBuffer()
					+ top * surface_pitch_pixels + left;

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

		fixed_t amount = (fixed_t)(famount * 64.0f);
		argb_t *fg2rgb = Col2RGB8[amount];
		argb_t *bg2rgb = Col2RGB8[64-amount];
		unsigned int fg = fg2rgb[V_GetColorFromString(GetDefaultPalette()->basecolors, color_str)];

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
		argb_t color = V_GetColorFromString(NULL, color_str);
		color = MAKERGB(newgamma[RPART(color)], newgamma[GPART(color)], newgamma[BPART(color)]);
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

std::string V_GetColorStringByName (const char *name)
{
	/* Note: The X11R6RGB lump used by this function *MUST* end
	 * with a NULL byte. This is so that COM_Parse is able to
	 * detect the end of the lump.
	 */
	char *rgbNames, *data, descr[5*3];
	int c[3], step;

	if (!(rgbNames = (char *)W_CacheLumpName ("X11R6RGB", PU_CACHE))) {
		Printf (PRINT_HIGH, "X11R6RGB lump not found\n");
		return NULL;
	}

	// skip past the header line
	data = strchr (rgbNames, '\n');
	step = 0;

	while ( (data = COM_Parse (data)) ) {
		if (step < 3) {
			c[step++] = atoi (com_token);
		} else {
			step = 0;
			if (*data >= ' ') {		// In case this name contains a space...
				char *newchar = com_token + strlen(com_token);

				while (*data >= ' ') {
					*newchar++ = *data++;
				}
				*newchar = 0;
			}

			if (!stricmp (com_token, name)) {
				sprintf (descr, "%04x %04x %04x",
						 (c[0] << 8) | c[0],
						 (c[1] << 8) | c[1],
						 (c[2] << 8) | c[2]);
				return descr;
			}
		}
	}
	return "";
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
static void BuildTransTable (argb_t *palette)
{
	{
		int r, g, b;

		// create the small RGB table
		for (r = 0; r < 32; r++)
			for (g = 0; g < 32; g++)
				for (b = 0; b < 32; b++)
					RGB32k[r][g][b] = BestColor (palette,
						(r<<3)|(r>>2), (g<<3)|(g>>2), (b<<3)|(b>>2), 256);
	}

	{
		int x, y;

		for (x = 0; x < 65; x++)
			for (y = 0; y < 256; y++)
				Col2RGB8[x][y] = (((RPART(palette[y])*x)>>4)<<20)  |
								  ((GPART(palette[y])*x)>>4) |
								 (((BPART(palette[y])*x)>>4)<<10);
	}
}

CVAR_FUNC_IMPL (vid_widescreen)
{
	static bool last_value = !var;	// force setmodeneeded when loading cvar
	if (last_value != var)
		setmodeneeded = true;
	last_value = var;
}

CVAR_FUNC_IMPL (sv_allowwidescreen)
{
	// change setmodeneeded when the value of sv_allowwidescreen
	// changes our ability to use wide-fov
	bool wide_fov = V_UseWidescreen() || V_UseLetterBox();
	static bool last_value = !wide_fov;

	if (last_value != wide_fov)
		setmodeneeded = true;
	last_value = wide_fov;
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
static bool V_DoModeSetup(int width, int height, int bpp)
{
	bool fullscreen = false;

	if (I_DisplayType() == DISPLAY_WindowOnly)
	{
		fullscreen = false;
		I_PauseMouse();
	}
	else if (I_DisplayType() == DISPLAY_FullscreenOnly)
	{
		fullscreen = true;
		I_ResumeMouse();
	}
	else
	{
		fullscreen = vid_fullscreen ? true : false;
		fullscreen ? I_ResumeMouse() : I_PauseMouse();
	}

	I_SetVideoMode(width, height, bpp, fullscreen, vid_vsync);
	if (!I_VideoInitialized())
		return false;

	int surface_width = I_GetSurfaceWidth(), surface_height = I_GetSurfaceHeight();

	// This uses the smaller of the two results. It's still not ideal but at least
	// this allows hud_scaletext to have some purpose...
	CleanXfac = CleanYfac = std::max(1, std::min(surface_width / 320, surface_height / 200));

	screen = I_GetPrimarySurface()->getDefaultCanvas();

	V_ForceBlend (0,0,0,0);
	GammaAdjustPalettes();
	RefreshPalettes();
	R_ReinitColormap();

	R_InitColumnDrawers();

	// [SL] 2011-11-30 - Prevent the player's view angle from moving
	I_FlushInput();

	return true;
}

bool V_SetResolution(int width, int height, int bpp)
{
	// Make sure we don't set the resolution smaller than Doom's original 320x200
	// resolution. Bad things might happen.
	width = clamp(width, 320, MAXWIDTH);
	height = clamp(height, 200, MAXHEIGHT);

	return V_DoModeSetup(width, height, bpp);
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
		setmodeneeded = true;
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
// V_InitPalette
//
void V_InitPalette (void)
{
	// [RH] Initialize palette subsystem
	if (!(InitPalettes ("PLAYPAL")))
		I_FatalError ("Could not initialize palette");

	BuildTransTable(GetDefaultPalette()->basecolors);

	V_ForceBlend (0, 0, 0, 0);

	RefreshPalettes ();

	assert(GetDefaultPalette()->maps.colormap != NULL);
	assert(GetDefaultPalette()->maps.shademap != NULL);
	V_Palette = shaderef_t(&GetDefaultPalette()->maps, 0); // (unsigned int *)DefaultPalette->colors;
}

//
// V_Close
//
//
void STACK_ARGS V_Close()
{
	// screen is automatically free'd by the primary surface
	if (screen)
		screen = NULL;
}



//
// V_Init
//
void V_Init()
{
	bool firstTime = true;
	if (firstTime)
		atterm(V_Close);

	if (!I_VideoInitialized())
		I_FatalError("Failed to initialize display");

	// TODO: [SL] set up CleanXfac/CleanYfac without calling V_SetResolution
	IWindow* window = I_GetWindow();
	V_SetResolution(window->getWidth(), window->getHeight(), window->getBitsPerPixel());

	setsizeneeded = true;

	I_SetWindowCaption();
	I_SetWindowIcon();

//	Video->SetWindowedScale(vid_winscale);

	V_InitPalette();

	C_InitConsole(I_GetSurfaceWidth(), I_GetSurfaceHeight());
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
		screen->Clear(0, I_GetSurfaceHeight() - 8, len * 8, I_GetSurfaceHeight(), 0);
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
		palindex_t* dest = (palindex_t*)(surface->getBuffer() + (surface_height - 1) * surface_pitch);

		int i = 0;
		for (i = 0; i < tics*2; i += 2)
			dest[i] = 0xFF;
		for ( ; i < 20*2; i += 2)
			dest[i] = 0x00;
	}
	else
	{
		argb_t* dest = (argb_t*)(surface->getBuffer() + (surface_height - 1) * surface_pitch);

		int i = 0;
		for (i = 0; i < tics*2; i += 2)
			dest[i] = MAKEARGB(255, 255, 255, 255);
		for ( ; i < 20*2; i += 2)
			dest[i] = MAKEARGB(255, 0, 0, 0);
	}
}

VERSION_CONTROL (v_video_cpp, "$Id$")

