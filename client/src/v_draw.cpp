// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 2006-2026 by The Odamex Team.
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
//	V_DRAW
//
//-----------------------------------------------------------------------------


#include "odamex.h"

#include <algorithm>

#include "v_video.h"
#include "i_video.h"
#include "r_main.h"

#include "i_system.h"

#include "resources/res_texture.h"


// [RH] Stretch values for V_DrawPatchClean()
int CleanXfac, CleanYfac;

EXTERN_CVAR(hud_transparency)

// The current set of column drawers (set in V_SetResolution)
DCanvas::vdrawfunc *DCanvas::m_Drawfuncs;
DCanvas::vdrawsfunc *DCanvas::m_Drawsfuncs;


// Palettized versions of the column drawers
DCanvas::vdrawfunc DCanvas::Pfuncs[6] =
{
	DCanvas::DrawPatchP,
	DCanvas::DrawLucentPatchP,
	DCanvas::DrawTranslatedPatchP,
	DCanvas::DrawTlatedLucentPatchP,
	DCanvas::DrawColoredPatchP,
	DCanvas::DrawColorLucentPatchP,
};
DCanvas::vdrawsfunc DCanvas::Psfuncs[6] =
{
	DCanvas::DrawPatchSP,
	DCanvas::DrawLucentPatchSP,
	DCanvas::DrawTranslatedPatchSP,
	DCanvas::DrawTlatedLucentPatchSP,
	DCanvas::DrawColoredPatchSP,
	DCanvas::DrawColorLucentPatchSP
};

// Direct (true-color) versions of the column drawers
DCanvas::vdrawfunc DCanvas::Dfuncs[6] =
{
	DCanvas::DrawPatchD,
	DCanvas::DrawLucentPatchD,
	DCanvas::DrawTranslatedPatchD,
	DCanvas::DrawTlatedLucentPatchD,
	DCanvas::DrawColoredPatchD,
	DCanvas::DrawColorLucentPatchD,
};
DCanvas::vdrawsfunc DCanvas::Dsfuncs[6] =
{
	DCanvas::DrawPatchSD,
	DCanvas::DrawLucentPatchSD,
	DCanvas::DrawTranslatedPatchSD,
	DCanvas::DrawTlatedLucentPatchSD,
	DCanvas::DrawColoredPatchSD,
	DCanvas::DrawColorLucentPatchSD
};

translationref_t V_ColorMap;
int V_ColorFill;

// Palette lookup table for direct modes
shaderef_t V_Palette;


/*********************************/
/*								 */
/* The palletized column drawers */
/*								 */
/*********************************/

// Normal patch drawers
void DCanvas::DrawPatchP (const byte *source, byte *dest, int count, int pitch)
{
	if (count <= 0)
		return;

	do
	{
		palindex_t pixel = *source;
		if (pixel != 0)
		{
			*dest = pixel;
		}
		source++;
		dest += pitch;
	} while (--count);
}

void DCanvas::DrawPatchSP (const byte *source, byte *dest, int count, int pitch, int yinc)
{
	if (count <= 0)
		return;

	int c = 0;

	do
	{
		palindex_t pixel = source[c >> 16];
		if (pixel != 0)
		{
			*dest = pixel;
		}
		dest += pitch;
		c += yinc;
	} while (--count);
}


// Translucent patch drawers (always 50%) [ML] 3/2/10: Not anymore!
void DCanvas::DrawLucentPatchP (const byte *source, byte *dest, int count, int pitch)
{
	if (count <= 0 || !hud_transparency)
		return;

	if (::hud_transparency >= 1.0)
		return DrawPatchP(source, dest, count, pitch);

	argb_t *fg2rgb, *bg2rgb;

	{
		fixed_t fglevel, bglevel, translevel;

		translevel = static_cast<fixed_t>(0xFFFF * hud_transparency);
		fglevel = translevel & ~0x3ff;
		bglevel = FRACUNIT-fglevel;
		fg2rgb = Col2RGB8[fglevel>>10];
		bg2rgb = Col2RGB8[bglevel>>10];
	}

	do
	{
		palindex_t pixel = *source;
		if (pixel != 0)
		{
			unsigned int fg = pixel;
			unsigned int bg = *dest;

			fg = fg2rgb[fg];
			bg = bg2rgb[bg];
			fg = (fg+bg) | 0x1f07c1f;
			*dest = RGB32k[0][0][fg & (fg>>15)];
		}
		source++;
		dest += pitch;
	} while (--count);
}

void DCanvas::DrawLucentPatchSP (const byte *source, byte *dest, int count, int pitch, int yinc)
{
	if (count <= 0 || !hud_transparency)
		return;

	if (::hud_transparency >= 1.0)
		return DrawPatchSP(source, dest, count, pitch, yinc);

	argb_t *fg2rgb, *bg2rgb;
	int c = 0;

	{
		fixed_t fglevel, bglevel, translevel;

		translevel = static_cast<fixed_t>(0xFFFF * hud_transparency);
		fglevel = translevel & ~0x3ff;
		bglevel = FRACUNIT-fglevel;
		fg2rgb = Col2RGB8[fglevel>>10];
		bg2rgb = Col2RGB8[bglevel>>10];
	}

	do
	{
		palindex_t pixel = source[c >> 16];
		if (pixel != 0)
		{
			unsigned int fg = pixel;
			unsigned int bg = *dest;

			fg = fg2rgb[fg];
			bg = bg2rgb[bg];
			fg = (fg+bg) | 0x1f07c1f;
			*dest = RGB32k[0][0][fg & (fg>>15)];
		}
		dest += pitch;
		c += yinc;
	} while (--count);
}


// Translated patch drawers
void DCanvas::DrawTranslatedPatchP (const byte *source, byte *dest, int count, int pitch)
{
	if (count <= 0)
		return;

	do
	{
		palindex_t pixel = *source;
		if (pixel != 0)
		{
			*dest = V_ColorMap.tlate(pixel);
		}
		source++;
		dest += pitch;
	} while (--count);
}

void DCanvas::DrawTranslatedPatchSP (const byte *source, byte *dest, int count, int pitch, int yinc)
{
	if (count <= 0)
		return;

	int c = 0;

	do
	{
		palindex_t pixel = source[c >> 16];
		if (pixel != 0)
		{
			*dest = V_ColorMap.tlate(pixel);
		}
		dest += pitch;
		c += yinc;
	} while (--count);
}


// Translated, translucent patch drawers
void DCanvas::DrawTlatedLucentPatchP (const byte *source, byte *dest, int count, int pitch)
{
	if (count <= 0 || !hud_transparency)
		return;

	if (::hud_transparency >= 1.0)
		return DrawTranslatedPatchP(source, dest, count, pitch);

	argb_t *fg2rgb, *bg2rgb;

	{
		fixed_t fglevel, bglevel, translevel;

		translevel = static_cast<fixed_t>(0xFFFF * hud_transparency);
		fglevel = translevel & ~0x3ff;
		bglevel = FRACUNIT-fglevel;
		fg2rgb = Col2RGB8[fglevel>>10];
		bg2rgb = Col2RGB8[bglevel>>10];
	}

	do
	{
		palindex_t pixel = *source;
		if (pixel != 0)
		{
			unsigned int fg = V_ColorMap.tlate(pixel);
			unsigned int bg = *dest;

			fg = fg2rgb[fg];
			bg = bg2rgb[bg];
			fg = (fg+bg) | 0x1f07c1f;
			*dest = RGB32k[0][0][fg & (fg>>15)];
		}
		source++;
		dest += pitch;
	} while (--count);
}

void DCanvas::DrawTlatedLucentPatchSP (const byte *source, byte *dest, int count, int pitch, int yinc)
{
	if (count <= 0 || !hud_transparency)
		return;

	if (::hud_transparency >= 1.0)
		return DrawTranslatedPatchSP(source, dest, count, pitch, yinc);

	int c = 0;
	argb_t *fg2rgb, *bg2rgb;

	{
		fixed_t fglevel, bglevel, translevel;

		translevel = static_cast<fixed_t>(0xFFFF * hud_transparency);
		fglevel = translevel & ~0x3ff;
		bglevel = FRACUNIT-fglevel;
		fg2rgb = Col2RGB8[fglevel>>10];
		bg2rgb = Col2RGB8[bglevel>>10];
	}

	do
	{
		palindex_t pixel = source[c >> 16];
		if (pixel != 0)
		{
			unsigned int fg = V_ColorMap.tlate(pixel);
			unsigned int bg = *dest;

			fg = fg2rgb[fg];
			bg = bg2rgb[bg];
			fg = (fg+bg) | 0x1f07c1f;
			*dest = RGB32k[0][0][fg & (fg>>15)];
		}
		dest += pitch;
		c += yinc;
	} while (--count);
}


// Colored patch drawer
//
// Fills the texture's opaque pixels with V_ColorFill.
void DCanvas::DrawColoredPatchP (const byte *source, byte *dest, int count, int pitch)
{
	if (count <= 0)
		return;

	byte fill = static_cast<byte>(V_ColorFill);

	do
	{
		if (*source != 0)
		{
			*dest = fill;
		}
		source++;
		dest += pitch;
	} while (--count);
}

void DCanvas::DrawColoredPatchSP(const byte *source, byte *dest, int count, int pitch, int yinc)
{
	if (count <= 0)
		return;

	byte fill = static_cast<byte>(V_ColorFill);

	int c = 0;

	do
	{
		if (source[c >> 16] != 0)
		{
			*dest = fill;
		}
		dest += pitch;
		c += yinc;
	} while (--count);
}


// Colored, translucent patch drawer
//
// This routine is the same for the stretched version since we don't
// care about the patch's actual contents, just it's outline.
void DCanvas::DrawColorLucentPatchP (const byte *source, byte *dest, int count, int pitch)
{
	if (count <= 0 || !hud_transparency)
		return;

	if (::hud_transparency >= 1.0)
		return DrawColoredPatchP(source, dest, count, pitch);

	argb_t *bg2rgb;

	{
		fixed_t fglevel, bglevel, translevel;

		translevel = static_cast<fixed_t>(0xFFFF * hud_transparency);
		fglevel = translevel & ~0x3ff;
		bglevel = FRACUNIT-fglevel;
		bg2rgb = Col2RGB8[bglevel>>10];
	}

	do
	{
		if (*source != 0)
		{
			unsigned int bg = bg2rgb[*dest];
			bg = (bg+bg) | 0x1f07c1f;
			*dest = RGB32k[0][0][bg & (bg>>15)];
		}
		source++;
		dest += pitch;
	} while (--count);
}

void DCanvas::DrawColorLucentPatchSP(const byte *source, byte *dest, int count, int pitch, int yinc)
{
	if (count <= 0 || !hud_transparency)
		return;

	if (::hud_transparency >= 1.0)
		return DrawColoredPatchSP(source, dest, count, pitch, yinc);

	argb_t *bg2rgb;

	{
		fixed_t fglevel, bglevel, translevel;

		translevel = static_cast<fixed_t>(0xFFFF * hud_transparency);
		fglevel = translevel & ~0x3ff;
		bglevel = FRACUNIT-fglevel;
		bg2rgb = Col2RGB8[bglevel>>10];
	}

	int c = 0;

	do
	{
		if (source[c >> 16] != 0)
		{
			unsigned int bg = bg2rgb[*dest];
			bg = (bg+bg) | 0x1f07c1f;
			*dest = RGB32k[0][0][bg & (bg>>15)];
		}
		dest += pitch;
		c += yinc;
	} while (--count);
}

/**************************/
/*						  */
/* The RGB column drawers */
/*						  */
/**************************/

// Normal patch drawers
void DCanvas::DrawPatchD (const byte *source, byte *dest, int count, int pitch)
{
	if (count <= 0)
		return;

	do
	{
		palindex_t pixel = *source;
		if (pixel != 0)
		{
			*(reinterpret_cast<argb_t*>(dest)) = V_Palette.shade(pixel);
		}
		source++;
		dest += pitch;
	} while (--count);
}

void DCanvas::DrawPatchSD (const byte *source, byte *dest, int count, int pitch, int yinc)
{
	if (count <= 0)
		return;

	int c = 0;

	do
	{
		palindex_t pixel = source[c >> 16];
		if (pixel != 0)
		{
			*(reinterpret_cast<argb_t*>(dest)) = V_Palette.shade(pixel);
		}
		dest += pitch;
		c += yinc;
	} while (--count);
}


// Translucent patch drawers (always 50%)
void DCanvas::DrawLucentPatchD (const byte *source, byte *dest, int count, int pitch)
{
	if (count <= 0 || !hud_transparency)
		return;

	if (::hud_transparency >= 1.0)
		return DrawPatchD(source, dest, count, pitch);

	int alpha = static_cast<int>(hud_transparency * 255);
	int invAlpha = 255 - alpha;

	do
	{
		palindex_t pixel = *source;
		if (pixel != 0)
		{
			argb_t fg = V_Palette.shade(pixel);
			argb_t bg = *(reinterpret_cast<argb_t*>(dest));
			*(reinterpret_cast<argb_t*>(dest)) = alphablend2a(bg, invAlpha, fg, alpha);
		}
		source++;
		dest += pitch;
	} while (--count);
}

void DCanvas::DrawLucentPatchSD (const byte *source, byte *dest, int count, int pitch, int yinc)
{
	if (count <= 0 || !hud_transparency)
		return;

	if (::hud_transparency >= 1.0)
		return DrawPatchSD(source, dest, count, pitch, yinc);

	int alpha = static_cast<int>(hud_transparency * 255);
	int invAlpha = 255 - alpha;

	int c = 0;

	do
	{
		palindex_t pixel = source[c >> 16];
		if (pixel != 0)
		{
			argb_t fg = V_Palette.shade(pixel);
			argb_t bg = *(reinterpret_cast<argb_t*>(dest));
			*(reinterpret_cast<argb_t*>(dest)) = alphablend2a(bg, invAlpha, fg, alpha);
		}
		dest += pitch;
		c += yinc;
	} while (--count);
}


// Translated patch drawers
void DCanvas::DrawTranslatedPatchD (const byte *source, byte *dest, int count, int pitch)
{
	if (count <= 0)
		return;

	do
	{
		palindex_t pixel = *source;
		if (pixel != 0)
		{
			*(reinterpret_cast<argb_t*>(dest)) = V_Palette.tlate(V_ColorMap, pixel);
		}
		source++;
		dest += pitch;
	} while (--count);
}

void DCanvas::DrawTranslatedPatchSD (const byte *source, byte *dest, int count, int pitch, int yinc)
{
	if (count <= 0)
		return;

	int c = 0;

	do
	{
		palindex_t pixel = source[c >> 16];
		if (pixel != 0)
		{
			*(reinterpret_cast<argb_t*>(dest)) = V_Palette.tlate(V_ColorMap, pixel);
		}
		dest += pitch;
		c += yinc;
	} while (--count);
}


// Translated, translucent patch drawers
void DCanvas::DrawTlatedLucentPatchD (const byte *source, byte *dest, int count, int pitch)
{
	if (count <= 0 || !hud_transparency)
		return;

	if (::hud_transparency >= 1.0)
		return DrawTranslatedPatchD(source, dest, count, pitch);

	int alpha = static_cast<int>(hud_transparency * 255);
	int invAlpha = 255 - alpha;

	do
	{
		palindex_t pixel = *source;
		if (pixel != 0)
		{
			argb_t fg = V_Palette.tlate(V_ColorMap, pixel);
			argb_t bg = *(reinterpret_cast<argb_t*>(dest));
			*(reinterpret_cast<argb_t*>(dest)) = alphablend2a(bg, invAlpha, fg, alpha);
		}
		source++;
		dest += pitch;
	} while (--count);
}

void DCanvas::DrawTlatedLucentPatchSD (const byte *source, byte *dest, int count, int pitch, int yinc)
{
	if (count <= 0 || !hud_transparency)
		return;

	if (::hud_transparency >= 1.0)
		return DrawTranslatedPatchSD(source, dest, count, pitch, yinc);

	int alpha = static_cast<int>(hud_transparency * 255);
	int invAlpha = 255 - alpha;

	int c = 0;

	do
	{
		palindex_t pixel = source[c >> 16];
		if (pixel != 0)
		{
			argb_t fg = V_Palette.tlate(V_ColorMap, pixel);
			argb_t bg = *(reinterpret_cast<argb_t*>(dest));
			*(reinterpret_cast<argb_t*>(dest)) = alphablend2a(bg, invAlpha, fg, alpha);
		}
		dest += pitch;
		c += yinc;
	} while (--count);
}


// Colored patch drawer
//
// This routine is the same for the stretched version since we don't
// care about the patch's actual contents, just it's outline.
void DCanvas::DrawColoredPatchD (const byte *source, byte *dest, int count, int pitch)
{
	if (count <= 0)
		return;

	argb_t color = V_Palette.shade(V_ColorFill);
	do
	{
		if (*source != 0)
		{
			*(reinterpret_cast<argb_t*>(dest)) = color;
		}
		source++;
		dest += pitch;
	} while (--count);
}

void DCanvas::DrawColoredPatchSD(const byte *source, byte *dest, int count, int pitch, int yinc)
{
	if (count <= 0)
		return;

	argb_t color = V_Palette.shade(V_ColorFill);

	int c = 0;

	do
	{
		if (source[c >> 16] != 0)
		{
			*(reinterpret_cast<argb_t*>(dest)) = color;
		}
		dest += pitch;
		c += yinc;
	} while (--count);
}


// Colored, translucent patch drawer
//
// Like DrawColoredPatchD, only the texture's opaque (non mask color) pixels
// are drawn.
void DCanvas::DrawColorLucentPatchD (const byte *source, byte *dest, int count, int pitch)
{
	if (count <= 0 || !hud_transparency)
		return;

	if (::hud_transparency >= 1.0)
		return DrawColoredPatchD(source, dest, count, pitch);

	int alpha = static_cast<int>(hud_transparency * 255);
	int invAlpha = 255 - alpha;

	argb_t fg = V_Palette.shade(V_ColorFill);

	do
	{
		if (*source != 0)
		{
			argb_t bg = *(reinterpret_cast<argb_t*>(dest));
			*(reinterpret_cast<argb_t*>(dest)) = alphablend2a(bg, invAlpha, fg, alpha);
		}
		source++;
		dest += pitch;
	} while (--count);
}

void DCanvas::DrawColorLucentPatchSD(const byte *source, byte *dest, int count, int pitch, int yinc)
{
	if (count <= 0 || !hud_transparency)
		return;

	if (::hud_transparency >= 1.0)
		return DrawColoredPatchSD(source, dest, count, pitch, yinc);

	int alpha = static_cast<int>(hud_transparency * 255);
	int invAlpha = 255 - alpha;

	argb_t fg = V_Palette.shade(V_ColorFill);

	int c = 0;

	do
	{
		if (source[c >> 16] != 0)
		{
			argb_t bg = *(reinterpret_cast<argb_t*>(dest));
			*(reinterpret_cast<argb_t*>(dest)) = alphablend2a(bg, invAlpha, fg, alpha);
		}
		dest += pitch;
		c += yinc;
	} while (--count);
}

/***********************************/
/*                                 */
/* The native ARGB column drawers  */
/*                                 */
/***********************************/

//
// DCanvas::DrawGlyphBlended
//
// Blits one antialiased glyph to a 32bpp surface.
//
// The glyph arrives as two planes: the palette index of the fill under each
// pixel, and how much of that pixel the glyph actually covers. Each pixel is
// translated and shaded to a true color the same way the palettized drawers
// would, then blended with the background by its coverage -- which is what
// turns a stairstepped edge into a smooth one. 'level' (0-255) scales the
// whole glyph's opacity for translucent text.
//
// Coordinates are the glyph's top-left corner in screen pixels -- anything
// falling outside the surface is clipped away.
//
void DCanvas::DrawGlyphBlended(const palindex_t* fill, const byte* coverage,
		int width, int height, int x, int y, int level) const
{
	if (!fill || !coverage || level <= 0)
		return;

	const int surface_width = mSurface->getWidth();
	const int surface_height = mSurface->getHeight();
	const int surface_pitch = mSurface->getRowStepInBytes();

	// clip to the surface
	const int x1 = std::max(x, 0);
	const int y1 = std::max(y, 0);
	const int x2 = std::min(x + width, surface_width);
	const int y2 = std::min(y + height, surface_height);

	if (x1 >= x2 || y1 >= y2)
		return;

	V_MarkRect(x1, y1, x2 - x1, y2 - y1);

	byte* buffer = mSurface->getBuffer();

	for (int col = x1; col < x2; col++)
	{
		// the planes are column-major, matching the glyph's texture
		const int plane_column = (col - x) * height;
		byte* dest = buffer + y1 * surface_pitch + col * mSurface->getColStepInBytes();

		for (int row = y1; row < y2; row++)
		{
			const int plane_index = plane_column + (row - y);
			const int alpha = coverage[plane_index] * level / 255;

			if (alpha > 0)
			{
				const argb_t color = V_Palette.tlate(V_ColorMap, fill[plane_index]);
				argb_t* pixel = reinterpret_cast<argb_t*>(dest);

				if (alpha >= 255)
					*pixel = color;
				else
					*pixel = alphablend2a(*pixel, 255 - alpha, color, alpha);
			}

			dest += surface_pitch;
		}
	}
}


//
// V_DrawARGBColumn
//
// Draws one column of a texture's native true-color image plane to a
// 32bpp surface, blending each pixel with the background using the
// pixel's own alpha. 'level' (0-255) scales the overall opacity for
// lucent drawing.
//
static void V_DrawARGBColumn(const argb_t* source, byte* dest, int count, int pitch, int level)
{
	if (count <= 0)
		return;

	do
	{
		const argb_t pixel = *source;
		const int alpha = (pixel.geta() * level) / 255;
		if (alpha >= 255)
		{
			*(reinterpret_cast<argb_t*>(dest)) = pixel;
		}
		else if (alpha > 0)
		{
			argb_t bg = *(reinterpret_cast<argb_t*>(dest));
			*(reinterpret_cast<argb_t*>(dest)) = alphablend2a(bg, 255 - alpha, pixel, alpha);
		}
		source++;
		dest += pitch;
	} while (--count);
}

//
// V_DrawARGBColumnS
//
// Scaled version of V_DrawARGBColumn.
//
static void V_DrawARGBColumnS(const argb_t* source, byte* dest, int count, int pitch, int yinc, int level)
{
	if (count <= 0)
		return;

	int c = 0;

	do
	{
		const argb_t pixel = source[c >> FRACBITS];
		const int alpha = (pixel.geta() * level) / 255;
		if (alpha >= 255)
		{
			*(reinterpret_cast<argb_t*>(dest)) = pixel;
		}
		else if (alpha > 0)
		{
			argb_t bg = *(reinterpret_cast<argb_t*>(dest));
			*(reinterpret_cast<argb_t*>(dest)) = alphablend2a(bg, 255 - alpha, pixel, alpha);
		}
		dest += pitch;
		c += yinc;
	} while (--count);
}

//
// V_ARGBDrawLevel
//
// Determines whether the given texture should be drawn through its
// native ARGB image plane on this surface with the given drawer, and at
// what opacity level (0-255). Returns -1 when the native path does not
// apply (palettized texture, 8bpp surface, or a translated/colored
// drawer with no true-color equivalent) and 0 when the patch is fully
// transparent and should not be drawn at all.
//
static int V_ARGBDrawLevel(const DCanvas::EWrapperCode drawer, const Texture* texture,
                           const IWindowSurface* surface)
{
	if (!texture->mARGBData || surface->getBitsPerPixel() != 32)
		return -1;

	if (drawer == DCanvas::EWrapper_Normal)
		return 255;

	if (drawer == DCanvas::EWrapper_Lucent)
		return std::clamp(static_cast<int>(hud_transparency * 255), 0, 255);

	return -1;
}


/******************************/
/*                            */
/* The patch drawing wrappers */
/*                            */
/******************************/

/**
 * @brief Masks a column based masked pic to the screen.
 *
 * @param drawer Draw code to use.
 * @param Texture Patch to draw.  Attempting to draw a NULL patch will have no effect.
 * @param x X coordinate of patch.
 * @param y Y coordinate of patch.
 */
void DCanvas::DrawWrapper(EWrapperCode drawer, const Texture* texture, int x, int y) const
{
	int surface_width = mSurface->getWidth(), surface_height = mSurface->getHeight();
	int surface_pitch = mSurface->getRowStepInBytes();
	int colstep = mSurface->getColStepInBytes();
	vdrawfunc	drawfunc;

	y -= texture->mOffsetY;
	x -= texture->mOffsetX;

	// [FG] automatically center wide patches without horizontal offset
	// (taken from dsda but inverted since we center above this)
	if (texture->mWidth > 320 && texture->mOffsetX != 0)
		x += (texture->mWidth - 320) / 2;

#ifdef RANGECHECK
	if (x < 0 ||x + texture->mWidth > surface_width || y < 0 || y + texture->mHeight > surface_height)
	{
	  // Printf (PRINT_HIGH, "Patch at %d,%d exceeds LFB\n", x,y );
	  // No I_Error abort - what is up with TNT.WAD?
	  DPrintFmt("DCanvas::DrawWrapper: bad texture (ignored)\n");
	  return;
	}
#endif

	if (mSurface->getBitsPerPixel() == 8)
		drawfunc = Pfuncs[drawer];
	else
		drawfunc = Dfuncs[drawer];

	// mark if this is the primary drawing surface
	if (mSurface == I_GetPrimarySurface())
		V_MarkRect(x, y, texture->mWidth, texture->mHeight);

	byte* desttop = mSurface->getBuffer() + y * surface_pitch + x * colstep;

	// True-color (PNG) textures are drawn natively on 32bpp surfaces
	// with per-pixel alpha blending, other drawers and 8bpp surfaces use
	// the palettized approximation below.
	const int argb_level = V_ARGBDrawLevel(drawer, texture, mSurface);
	if (argb_level >= 0)
	{
		if (argb_level > 0)
		{
			for (int col = 0; col < texture->mWidth; col++, desttop += colstep)
				V_DrawARGBColumn(texture->getARGBColumn(col), desttop,
				                 texture->mHeight, surface_pitch, argb_level);
		}
		return;
	}

	for (int col = 0; col < texture->mWidth; x++, col++, desttop += colstep)
	{
		const palindex_t* col_data = texture->getColumn(col);
		drawfunc(col_data, desttop, texture->mHeight, surface_pitch);
	}
}

/**
 * @brief Masks a column based masked pic to the screen stretching it to fit the given dimensions.
 *
 * @param drawer Draw code to use.
 * @param patch
 * @param x0
 * @param y0
 * @param destwidth
 * @param destheight
 */
void DCanvas::DrawSWrapper(EWrapperCode drawer, const Texture* texture, int x0, int y0,
                           const int destwidth, const int destheight) const
{
	if (!texture || texture->mWidth <= 0 || texture->mHeight <= 0 || destwidth <= 0 || destheight <= 0)
		return;

	if (destwidth == texture->mWidth && destheight == texture->mHeight)
	{
		// Perfect 1:1 mapping, so we use the unscaled draw wrapper.
		DrawWrapper(drawer, texture, x0, y0);
		return;
	}

	int surface_width = mSurface->getWidth(), surface_height = mSurface->getHeight();
	int surface_pitch = mSurface->getRowStepInBytes();
	int colstep = mSurface->getColStepInBytes();
	vdrawsfunc drawfunc;

	// [AM] Adding 1 to the inc variables leads to fewer weird scaling
	//      artifacts since it forces col to roll over to the next real number
	//      a column-of-real-pixels sooner.
	int xinc = (texture->mWidth << FRACBITS) / destwidth;
	int yinc = (texture->mHeight << FRACBITS) / destheight;
	// [jsd] only adding 1 in cases where scaling is non-integral:
	if (xinc & (FRACUNIT-1)) {
		xinc++;
	}
	if (yinc & (FRACUNIT-1)) {
		yinc++;
	}
	int xmul = (destwidth << FRACBITS) / texture->mWidth;
	int ymul = (destheight << FRACBITS) / texture->mHeight;

	y0 -= (texture->mOffsetY * ymul) >> FRACBITS;
	x0 -= (texture->mOffsetX * xmul) >> FRACBITS;

#ifdef RANGECHECK
	if (x0 < 0 || x0 + destwidth > surface_width || y0 < 0 || y0 + destheight > surface_height)
	{
		DPrintFmt("DCanvas::DrawSWrapper: bad patch dimensions ({} x {}) (ignored)\n", texture->mWidth, texture->mHeight);
		return;
	}
#endif

	if (mSurface->getBitsPerPixel() == 8)
		drawfunc = Psfuncs[drawer];
	else
		drawfunc = Dsfuncs[drawer];

	// mark if this is the primary drawing surface
	if (mSurface == I_GetPrimarySurface())
		V_MarkRect(x0, y0, destwidth, destheight);

	byte* desttop = mSurface->getBuffer()+ (y0 * surface_pitch) + (x0 * colstep);
	int w = std::min(destwidth * xinc, texture->mWidth << FRACBITS);

	// True-color (PNG) textures are drawn natively on 32bpp surfaces
	// with per-pixel alpha blending, other drawers and 8bpp surfaces use
	// the palettized approximation below.
	const int argb_level = V_ARGBDrawLevel(drawer, texture, mSurface);
	if (argb_level >= 0)
	{
		if (argb_level > 0)
		{
			for (int col = 0; col < w; col += xinc, desttop += colstep)
				V_DrawARGBColumnS(texture->getARGBColumn(col >> FRACBITS), desttop,
				                  (texture->mHeight * ymul) >> FRACBITS, surface_pitch,
				                  yinc, argb_level);
		}
		return;
	}

	for (int col = 0; col < w; col += xinc, desttop += colstep)
	{
		const palindex_t* col_data = texture->getColumn(col >> FRACBITS);
		drawfunc(col_data, desttop, (texture->mHeight * ymul) >> FRACBITS, surface_pitch, yinc);
	}
}

//
// V_DrawIWrapper
// Like V_DrawWrapper except it will stretch the patches as
// needed for non-320x200 screens.
//
void DCanvas::DrawIWrapper(EWrapperCode drawer, const Texture* texture, int x0, int y0) const
{
	int surface_width = mSurface->getWidth(), surface_height = mSurface->getHeight();

	if (surface_width == 320 && surface_height == 200)
		DrawWrapper(drawer, texture, x0, y0);
	else
		DrawSWrapper(drawer, texture,
			 (surface_width * x0) / 320, (surface_height * y0) / 200,
			 (surface_width * texture->mWidth) / 320, (surface_height * texture->mHeight) / 200);
}

//
// V_DrawCWrapper
// Like V_DrawIWrapper, except it only uses integral multipliers.
//
void DCanvas::DrawCWrapper(EWrapperCode drawer, const Texture* texture, int x0, int y0) const
{
	int surface_width = mSurface->getWidth(), surface_height = mSurface->getHeight();

	if (CleanXfac == 1 && CleanYfac == 1)
		DrawWrapper(drawer, texture, (x0-160) + (surface_width/2), (y0-100) + (surface_height/2));
	else
		DrawSWrapper(drawer, texture,
			(x0-160)*CleanXfac+(surface_width/2), (y0-100)*CleanYfac+(surface_height/2),
			texture->mWidth * CleanXfac, texture->mHeight * CleanYfac);
}

//
// V_DrawCNMWrapper
// Like V_DrawCWrapper, except it doesn't adjust the x and y coordinates.
//
void DCanvas::DrawCNMWrapper(EWrapperCode drawer, const Texture* texture, int x0, int y0) const
{
	if (CleanXfac == 1 && CleanYfac == 1)
		DrawWrapper(drawer, texture, x0, y0);
	else
		DrawSWrapper(drawer, texture, x0, y0,
						texture->mWidth * CleanXfac,
						texture->mHeight * CleanYfac);
}


/********************************/
/*								*/
/* Other miscellaneous routines */
/*								*/
/********************************/


//
// DCanvas::DrawTextureFlipped
//
// Mirrored 1:1 draw in SURFACE coordinates -- the exact twin of DrawTexture, and
// it must stay that way. This used to remap from a virtual 320x200 space while
// DrawTexture did not, and callers pick between them per sprite frame, so
// mirrored frames landed in a different coordinate space from unmirrored ones.
//
void DCanvas::DrawTextureFlipped(const Texture* texture, int x0, int y0) const
{
	if (!texture)
		return;

	DrawTextureFlippedStretched(texture, x0, y0, texture->mWidth, texture->mHeight);
}

//
// DCanvas::DrawTextureFlippedStretched
//
// Mirrored draw scaled to destwidth x destheight -- the twin of
// DrawTextureStretched, sharing its inc rounding so a mirrored frame and an
// unmirrored one of equal size come out the same size.
//
void DCanvas::DrawTextureFlippedStretched(const Texture* texture, int x0, int y0,
		const int destwidth, const int destheight) const
{
	if (!texture || texture->mWidth <= 0 || texture->mHeight <= 0 || destwidth <= 0 || destheight <= 0)
		return;

	int surface_width = mSurface->getWidth(), surface_height = mSurface->getHeight();
	int surface_pitch = mSurface->getRowStepInBytes();
	int colstep = mSurface->getColStepInBytes();

	vdrawsfunc	drawfunc;

	// Same rounding as DrawSWrapper: round the step up only when the scale is
	// non-integral, so the two agree on the final size.
	int xinc = (texture->mWidth << FRACBITS) / destwidth;
	int yinc = (texture->mHeight << FRACBITS) / destheight;
	if (xinc & (FRACUNIT - 1))
		xinc++;
	if (yinc & (FRACUNIT - 1))
		yinc++;
	int xmul = (destwidth << FRACBITS) / texture->mWidth;
	int ymul = (destheight << FRACBITS) / texture->mHeight;

	y0 -= (texture->mOffsetY * ymul) >> FRACBITS;
	// flipped drawing measures the x offset from the right-hand edge
	x0 -= ((texture->mWidth - texture->mOffsetX) * xmul) >> FRACBITS;

#ifdef RANGECHECK
	if (x0 < 0 || x0 + destwidth > surface_width || y0 < 0 || y0 + destheight > surface_height)
	{
		//Printf ("Patch at %d,%d exceeds LFB\n", x0,y0 );
		DPrintFmt("DCanvas::DrawPatchFlipped: bad patch (ignored)\n");
		return;
	}
#endif

	if (mSurface->getBitsPerPixel() == 8)
		drawfunc = Psfuncs[EWrapper_Normal];
	else
		drawfunc = Dsfuncs[EWrapper_Normal];

	if (mSurface == I_GetPrimarySurface())
		V_MarkRect(x0, y0, destwidth, destheight);

	byte* desttop = mSurface->getBuffer()+ y0 * surface_pitch + x0 * colstep;

	const int first_col = (destwidth - 1) * xinc;
	const int max_col = texture->mWidth - 1;

	// true-color (PNG) textures are drawn natively on 32bpp surfaces
	// with per-pixel alpha blending, same as DrawWrapper/DrawSWrapper
	const int argb_level = V_ARGBDrawLevel(EWrapper_Normal, texture, mSurface);
	if (argb_level >= 0)
	{
		if (argb_level > 0)
		{
			for (int col = first_col; col >= 0 ; col -= xinc, desttop += colstep)
				V_DrawARGBColumnS(texture->getARGBColumn(std::min(col >> FRACBITS, max_col)),
				                  desttop, (texture->mHeight * ymul) >> FRACBITS,
				                  surface_pitch, yinc, argb_level);
		}
		return;
	}

	for (int col = first_col; col >= 0 ; col -= xinc, desttop += colstep)
	{
		const palindex_t* col_data = texture->getColumn(std::min(col >> FRACBITS, max_col));
		drawfunc(col_data, desttop, (texture->mHeight * ymul) >> FRACBITS, surface_pitch, yinc);
	}
}




//
// DCanvas::GetBlock
//
// Gets a linear block of pixels from the view buffer.
//
void DCanvas::GetBlock(int x, int y, int width, int height, byte *dest) const
{
	int surface_width = mSurface->getWidth(), surface_height = mSurface->getHeight();
	int surface_pitch = mSurface->getRowStepInBytes();
	int colstep = mSurface->getColStepInBytes();
	// the caller's packed linear block, so never the surface's column step
	int line_length = surface_width * mSurface->getBytesPerPixel();

#ifdef RANGECHECK
	if (x < 0 || x + width > surface_width || y < 0 || y + height > surface_height)
		I_Error("Bad V_GetBlock");
#endif

	const byte* src = mSurface->getBuffer() + y * surface_pitch + x * colstep;

	// A row of the block is strided, because the surface stores columns.
	//
	// The loop below walks surface_width, not the width parameter, and steps the
	// packed buffer by line_length -- so it is correct only for a FULL-WIDTH
	// block. Every caller (the wipes) passes one; assert it rather than leaving
	// it for the first sub-block caller to discover.
	assert(width == surface_width);
	const int pixelsize = mSurface->getBytesPerPixel();
	while (height--)
	{
		const byte* srcpixel = src;
		byte* destpixel = dest;
		for (int col = 0; col < surface_width; col++)
		{
			memcpy(destpixel, srcpixel, pixelsize);
			srcpixel += colstep;
			destpixel += pixelsize;
		}
		src += surface_pitch;
		dest += line_length;
	}
}


//
// DCanvas::GetTransposedBlock
//
// Gets a transposed block of pixels from the view buffer.
//

template<typename PIXEL_T>
static inline void V_GetTransposedBlockGeneric(byte* destbuffer, const byte* sourcebuffer,
			int x, int y, int width, int height, int rowstep, int colstep)
{
	const PIXEL_T* source = reinterpret_cast<const PIXEL_T*>(sourcebuffer) + y * rowstep + x * colstep;
	PIXEL_T* dest = reinterpret_cast<PIXEL_T*>(destbuffer);

	for (int col = x; col < x + width; col++)
	{
		const PIXEL_T* sourceptr = source;
		source += colstep;

		for (int row = y; row < y + height; row++)
		{
			*dest++ = *sourceptr;
			sourceptr += rowstep;
		}
	}
}

void DCanvas::GetTransposedBlock(int x, int y, int width, int height, byte* destbuffer) const
{
	int surface_width = mSurface->getWidth(), surface_height = mSurface->getHeight();

#ifdef RANGECHECK
	if (x < 0 ||x + width > surface_width || y < 0 || y + height > surface_height)
		I_Error ("Bad V_GetTransposedBlock");
#endif

	if (mSurface->getBitsPerPixel() == 8)
		V_GetTransposedBlockGeneric<palindex_t>(destbuffer, mSurface->getBuffer(),
				x, y, width, height,
				mSurface->getRowStepInPixels(), mSurface->getColStepInPixels());
	else
		V_GetTransposedBlockGeneric<argb_t>(destbuffer, mSurface->getBuffer(),
				x, y, width, height,
				mSurface->getRowStepInPixels(), mSurface->getColStepInPixels());
}

VERSION_CONTROL (v_draw_cpp, "$Id$")

