// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
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

#include "minilzo.h"
// [RH] Output buffer size for LZO compression.
//		Extra space in case uncompressable.
#define OUT_LEN(a)		((a) + (a) / 64 + 16 + 3)

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

#include "c_console.h"
#include "hu_stuff.h"

#include "m_argv.h"
#include "m_bbox.h"
#include "m_swap.h"
#include "m_menu.h"

#include "i_video.h"
#include "v_video.h"
#include "v_text.h"

#include "w_wad.h"
#include "z_zone.h"

#include "c_cvars.h"
#include "c_dispatch.h"
#include "cmdlib.h"

IMPLEMENT_CLASS (DCanvas, DObject)

int DisplayWidth, DisplayHeight, DisplayBits;

unsigned int Col2RGB8[65][256];
byte RGB32k[32][32][32];


// [RH] The framebuffer is no longer a mere byte array.
// There's also only one, not four.
DCanvas *screen;

DBoundingBox dirtybox;

EXTERN_CVAR (vid_defwidth)
EXTERN_CVAR (vid_defheight)
EXTERN_CVAR (vid_defbits)
EXTERN_CVAR (autoadjust_video_settings)

EXTERN_CVAR (dimamount)
EXTERN_CVAR (dimcolor)

extern "C" {
palette_t *DefaultPalette;
}

// [RH] Set true when vid_setmode command has been executed
BOOL	setmodeneeded = false;
// [RH] Resolution to change to when setmodeneeded is true
int		NewWidth, NewHeight, NewBits;

EXTERN_CVAR (vid_fullscreen)

//
// V_MarkRect
//
void V_MarkRect (int x, int y, int width, int height)
{
	dirtybox.AddToBox (x, y);
	dirtybox.AddToBox (x+width-1, y+height-1);
}


DCanvas::DCanvas ()
{
	buffer = NULL;
	m_LockCount = 0;
	m_Private = NULL;
}

DCanvas::~DCanvas ()
{
}

// [RH] Fill an area with a 64x64 flat texture
//		right and bottom are one pixel *past* the boundaries they describe.
void DCanvas::FlatFill (int left, int top, int right, int bottom, const byte *src) const
{
	int x, y;
	int advance;
	int width;

	width = right - left;
	right = width >> 6;

	if (is8bit())
	{
		byte *dest;

		advance = pitch - width;
		dest = buffer + top * pitch + left;

		for (y = top; y < bottom; y++)
		{
			for (x = 0; x < right; x++)
			{
				memcpy (dest, src + ((y&63)<<6), 64);
				dest += 64;
			}

			if (width & 63)
			{
				memcpy (dest, src + ((y&63)<<6), width & 63);
				dest += width & 63;
			}
			dest += advance;
		}
	}
	else
	{
		unsigned int *dest;
		int z;
		const byte *l;

		advance = (pitch - (width << 2)) >> 2;
		dest = (unsigned int *)(buffer + top * pitch + (left << 2));

		for (y = top; y < bottom; y++)
		{
			l = src + ((y&63)<<6);
			for (x = 0; x < right; x++)
			{
				for (z = 0; z < 64; z += 4, dest += 4)
				{
					// Try and let the optimizer pair this on a Pentium
					// (even though VC++ doesn't anyway)
					dest[0] = V_Palette[l[z]];
					dest[1] = V_Palette[l[z+1]];
					dest[2] = V_Palette[l[z+2]];
					dest[3] = V_Palette[l[z+3]];
				}
			}

			if (width & 63)
			{
				// Do any odd pixel left over
				if (width & 1)
					*dest++ = V_Palette[l[0]];

				// Do the rest of the pixels
				for (z = 1; z < (width & 63); z += 2, dest += 2)
				{
					dest[0] = V_Palette[l[z]];
					dest[1] = V_Palette[l[z+1]];
				}
			}

			dest += advance;
		}
	}
}


// [RH] Set an area to a specified color
void DCanvas::Clear (int left, int top, int right, int bottom, int color) const
{
	int x, y;

	if (is8bit())
	{
		byte *dest;

		dest = buffer + top * pitch + left;
		x = right - left;
		for (y = top; y < bottom; y++)
		{
			memset (dest, color, x);
			dest += pitch;
		}
	}
	else
	{
		unsigned int *dest;

		dest = (unsigned int *)(buffer + top * pitch + (left << 2));
		right -= left;

		for (y = top; y < bottom; y++)
		{
			for (x = 0; x < right; x++)
			{
				dest[x] = color;
			}
			dest += pitch >> 2;
		}
	}
}


void DCanvas::Dim () const
{
	if (dimamount < 0)
		dimamount.Set (0.0f);
	else if (dimamount > 1)
		dimamount.Set (1.0f);

	if (dimamount == 0)
		return;

	if (is8bit())
	{
		unsigned int *bg2rgb;
		unsigned int fg;
		int gap;
		byte *spot;
		int x, y;

		{
			unsigned int *fg2rgb;
			fixed_t amount;

			amount = (fixed_t)(dimamount * 64);
			fg2rgb = Col2RGB8[amount];
			bg2rgb = Col2RGB8[64-amount];
			fg = fg2rgb[V_GetColorFromString (DefaultPalette->basecolors, dimcolor.cstring())];
		}

		spot = buffer;
		gap = pitch - width;
		for (y = 0; y < height; y++)
		{
			for (x = 0; x < width; x++)
			{
				unsigned int bg = bg2rgb[*spot];
				bg = (fg+bg) | 0x1f07c1f;
				*spot++ = RGB32k[0][0][bg&(bg>>15)];
			}
			spot += gap;
		}
	}
	else
	{
		int x, y;
		int *line;
		int fill = V_GetColorFromString (NULL, dimcolor.cstring());

		line = (int *)(screen->buffer);

		if (dimamount == 1.0)
		{
			fill = (fill >> 2) & 0x3f3f3f;
			for (y = 0; y < height; y++)
			{
				for (x = 0; x < width; x++)
				{
					line[x] = (line[x] - ((line[x] >> 2) & 0x3f3f3f)) + fill;
				}
				line += pitch >> 2;
			}
		}
		else if (dimamount == 2.0)
		{
			fill = (fill >> 1) & 0x7f7f7f;
			for (y = 0; y < height; y++)
			{
				for (x = 0; x < width; x++)
				{
					line[x] = ((line[x] >> 1) & 0x7f7f7f) + fill;
				}
				line += pitch >> 2;
			}
		}
		else if (dimamount == 3.0)
		{
			fill = fill - ((fill >> 2) & 0x3f3f3f);
			for (y = 0; y < height; y++)
			{
				for (x = 0; x < width; x++)
				{
					line[x] = ((line[x] >> 2) & 0x3f3f3f) + fill;
				}
				line += pitch >> 2;
			}
		}
	}
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

	std::string name = BuildString (argc - 2, (const char **)(argv + 2));
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
			AddCommandString (setcmd.c_str());
		}
	}
}
END_COMMAND (setcolor)

// Build the tables necessary for translucency
static void BuildTransTable (DWORD *palette)
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

void DCanvas::Lock ()
{
	m_LockCount++;
	if (m_LockCount == 1)
	{
		I_LockScreen (this);

		if (this == screen)
		{
			if (dc_pitch != pitch << detailyshift)
			{
				dc_pitch = pitch << detailyshift;
				R_InitFuzzTable ();
#ifdef USEASM
				ASM_PatchPitch ();
#endif
			}

			if ((is8bit() ? 1 : 4) << detailxshift != ds_colsize)
			{
				ds_colsize = (is8bit() ? 1 : 4) << detailxshift;
#ifdef USEASM
				ASM_PatchColSize ();
#endif
			}
		}
	}
}

void DCanvas::Unlock ()
{
	if (m_LockCount)
		if (--m_LockCount == 0)
			I_UnlockScreen (this);
}

void DCanvas::Blit (int srcx, int srcy, int srcwidth, int srcheight,
			 DCanvas *dest, int destx, int desty, int destwidth, int destheight)
{
	I_Blit (this, srcx, srcy, srcwidth, srcheight, dest, destx, desty, destwidth, destheight);
}


//
// V_SetResolution
//
BOOL V_DoModeSetup (int width, int height, int bits)
{
	// Free the virtual framebuffer
	if(screen)
	{
		I_FreeScreen(screen);
		screen = NULL;
	}

	I_SetMode (width, height, bits);
	
	/*
	CleanXfac = ((height * 4)/3) / 320;

	if(!CleanXfac)
		CleanXfac = 1;
		
	// [ML] The height determines the scale, these should always be the same 
	// or stuff like the menu will be stretched on widescreen resolutions
	CleanYfac = CleanXfac;
	*/
	
	// [ML] 7/30/10: Going back to this style, they'll still be the same in the end
	// This uses the smaller of the two results. It's still not ideal but at least
	// this allows con_scaletext to have some purpose...
	
    CleanXfac = ((height * 4)/3) / 320; 
    CleanYfac = height / 200; 
    if (CleanXfac < CleanYfac) CleanYfac = CleanXfac; 
    if (CleanYfac < CleanXfac) CleanXfac = CleanYfac;



	CleanWidth = width / CleanXfac;
	CleanHeight = height / CleanYfac;

	DisplayWidth = width;
	DisplayHeight = height;
	DisplayBits = bits;

	// Allocate a new virtual framebuffer
	screen = I_AllocateScreen (width, height, bits, true);

	V_ForceBlend (0,0,0,0);
	if (bits == 8)
		RefreshPalettes ();

	R_InitColumnDrawers (screen->is8bit());
	R_MultiresInit ();

//	M_RefreshModesList (); // [Toke - crap]

	return true;
}

BOOL V_SetResolution (int width, int height, int bits)
{
	int oldwidth, oldheight;
	int oldbits;

	if (screen) {
		oldwidth = screen->width;
		oldheight = screen->height;
		oldbits = DisplayBits;
	} else {
		// Harmless if screen wasn't allocated
		oldwidth = width;
		oldheight = height;
		oldbits = bits;
	}

	if ((int)(autoadjust_video_settings)) {
		I_ClosestResolution (&width, &height, bits);
		if (!I_CheckResolution (width, height, bits)) {				// Try specified resolution
			if (!I_CheckResolution (oldwidth, oldheight, oldbits)) {// Try previous resolution (if any)
		   		return false;
			} else {
				width = oldwidth;
				height = oldheight;
				bits = oldbits;
			}
		}
	}
	return V_DoModeSetup (width, height, bits);
}

BEGIN_COMMAND (vid_setmode)
{
	BOOL	goodmode = false;
	int		width = 0, height = screen->height;
	int		bits = DisplayBits;

	if (argc > 1) {
		width = atoi (argv[1]);
		if (argc > 2) {
			height = atoi (argv[2]);
			if (argc > 3) {
				bits = 8;
				//bits = atoi (argv[3]);
			}
		}
	}

	if (width) {
		if (I_CheckResolution (width, height, bits))
			goodmode = true;
	}

	if (goodmode) {
		// The actual change of resolution will take place
		// near the beginning of D_Display().
		if (gamestate != GS_STARTUP) {
			setmodeneeded = true;
			NewWidth = width;
			NewHeight = height;
			NewBits = bits;
		}
	} else if (width) {
		Printf (PRINT_HIGH, "Unknown resolution %d x %d x %d\n", width, height, bits);
	} else {
      // SoM: enforce 8-bit modes for now
		Printf (PRINT_HIGH, "Usage: vid_setmode <width> <height>\n");
	}
}
END_COMMAND (vid_setmode)

BEGIN_COMMAND (checkres)
{
    Printf (PRINT_HIGH, "Resolution: %d x %d x %d (%s)\n", screen->width, screen->height, screen->bits,
        (vid_fullscreen ? "FULLSCREEN" : "WINDOWED")); // NES - Simple resolution checker.
}
END_COMMAND (checkres)

//
// V_InitPalette
//

void V_InitPalette (void)
{
	// [RH] Initialize palette subsystem
	if (!(DefaultPalette = InitPalettes ("PLAYPAL")))
		I_FatalError ("Could not initialize palette");

	V_Palette = (unsigned int *)DefaultPalette->colors;

	BuildTransTable (DefaultPalette->basecolors);

	V_ForceBlend (0, 0, 0, 0);

	if(DisplayBits == 8)
		RefreshPalettes ();
}

//
// V_Init
//

void V_Init (void)
{
	int width, height, bits;

	width = height = bits = 0;

	const char *w = Args.CheckValue ("-width");
	const char *h = Args.CheckValue ("-height");
	const char *b = Args.CheckValue ("-bits");

	if (w)
		width = atoi (w);

	if (h)
		height = atoi (h);

	if (b)
		bits = atoi (b);

	if (width == 0)
	{
		if (height == 0)
		{
			width = (int)(vid_defwidth);
			height = (int)(vid_defheight);
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

	if (bits == 0)
	{
      // SoM: We have to enforce 8-bit for now
		//bits = (int)(vid_defbits);
      bits = 8;
	}

    if ((int)(autoadjust_video_settings))
        I_ClosestResolution (&width, &height, bits);

	if (!V_SetResolution (width, height, bits))
		I_FatalError ("Could not set resolution to %d x %d x %d %s\n", width, height, bits,
            (vid_fullscreen ? "FULLSCREEN" : "WINDOWED"));
	else
        AddCommandString("checkres");

	V_InitConChars (0xf7);
	C_InitConsole (screen->width, screen->height, true);

	V_InitPalette ();
}

void DCanvas::AttachPalette (palette_t *pal)
{
	if (m_Palette == pal)
		return;

	DetachPalette ();

	pal->usecount++;
	m_Palette = pal;
}


void DCanvas::DetachPalette ()
{
	if (m_Palette)
	{
		FreePalette (m_Palette);
		m_Palette = NULL;
	}
}

VERSION_CONTROL (v_video_cpp, "$Id$")

