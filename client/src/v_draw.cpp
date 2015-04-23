// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
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
//	V_DRAW
//
//-----------------------------------------------------------------------------


#include <stdio.h>
#include <stdlib.h>

#include "doomtype.h"
#include "v_video.h"
#include "i_video.h"
#include "r_main.h"
#include "m_swap.h"

#include "i_system.h"

#include "cmdlib.h"

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
	(vdrawsfunc)DCanvas::DrawColoredPatchP,
	(vdrawsfunc)DCanvas::DrawColorLucentPatchP
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
	(vdrawsfunc)DCanvas::DrawColoredPatchD,
	(vdrawsfunc)DCanvas::DrawColorLucentPatchD
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
		*dest = *source++;
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
		*dest = source[c >> 16];
		dest += pitch;
		c += yinc;
	} while (--count);
}


// Translucent patch drawers (always 50%) [ML] 3/2/10: Not anymore!
void DCanvas::DrawLucentPatchP (const byte *source, byte *dest, int count, int pitch)
{
	if (count <= 0)
		return;

	argb_t *fg2rgb, *bg2rgb;

	{
		fixed_t fglevel, bglevel, translevel;

		translevel = (fixed_t)(0xFFFF * hud_transparency);
		fglevel = translevel & ~0x3ff;
		bglevel = FRACUNIT-fglevel;
		fg2rgb = Col2RGB8[fglevel>>10];
		bg2rgb = Col2RGB8[bglevel>>10];
	}

	do
	{
		unsigned int fg = *source++;
		unsigned int bg = *dest;

		fg = fg2rgb[fg];
		bg = bg2rgb[bg];
		fg = (fg+bg) | 0x1f07c1f;
		*dest = RGB32k[0][0][fg & (fg>>15)];
		dest += pitch;
	} while (--count);
}

void DCanvas::DrawLucentPatchSP (const byte *source, byte *dest, int count, int pitch, int yinc)
{
	if (count <= 0)
		return;

	argb_t *fg2rgb, *bg2rgb;
	int c = 0;

	{
		fixed_t fglevel, bglevel, translevel;

		translevel = (fixed_t)(0xFFFF * hud_transparency);
		fglevel = translevel & ~0x3ff;
		bglevel = FRACUNIT-fglevel;
		fg2rgb = Col2RGB8[fglevel>>10];
		bg2rgb = Col2RGB8[bglevel>>10];
	}

	do
	{
		unsigned int fg = source[c >> 16];
		unsigned int bg = *dest;

		fg = fg2rgb[fg];
		bg = bg2rgb[bg];
		fg = (fg+bg) | 0x1f07c1f;
		*dest = RGB32k[0][0][fg & (fg>>15)];
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
		*dest = V_ColorMap.tlate(*source++);
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
		*dest = V_ColorMap.tlate(source[c >> 16]);
		dest += pitch;
		c += yinc;
	} while (--count);
}


// Translated, translucent patch drawers
void DCanvas::DrawTlatedLucentPatchP (const byte *source, byte *dest, int count, int pitch)
{
	if (count <= 0)
		return;

	argb_t *fg2rgb, *bg2rgb;

	{
		fixed_t fglevel, bglevel, translevel;

		translevel = (fixed_t)(0xFFFF * hud_transparency);
		fglevel = translevel & ~0x3ff;
		bglevel = FRACUNIT-fglevel;
		fg2rgb = Col2RGB8[fglevel>>10];
		bg2rgb = Col2RGB8[bglevel>>10];
	}

	do
	{
		unsigned int fg = V_ColorMap.tlate(*source++);
		unsigned int bg = *dest;

		fg = fg2rgb[fg];
		bg = bg2rgb[bg];
		fg = (fg+bg) | 0x1f07c1f;
		*dest = RGB32k[0][0][fg & (fg>>15)];
		dest += pitch;
	} while (--count);
}

void DCanvas::DrawTlatedLucentPatchSP (const byte *source, byte *dest, int count, int pitch, int yinc)
{
	if (count <= 0)
		return;

	int c = 0;
	argb_t *fg2rgb, *bg2rgb;

	{
		fixed_t fglevel, bglevel, translevel;

		translevel = (fixed_t)(0xFFFF * hud_transparency);
		fglevel = translevel & ~0x3ff;
		bglevel = FRACUNIT-fglevel;
		fg2rgb = Col2RGB8[fglevel>>10];
		bg2rgb = Col2RGB8[bglevel>>10];
	}

	do
	{
		unsigned int fg = V_ColorMap.tlate(source[c >> 16]);
		unsigned int bg = *dest;

		fg = fg2rgb[fg];
		bg = bg2rgb[bg];
		fg = (fg+bg) | 0x1f07c1f;
		*dest = RGB32k[0][0][fg & (fg>>15)];
		dest += pitch;
		c += yinc;
	} while (--count);
}


// Colored patch drawer
//
// This routine is the same for the stretched version since we don't
// care about the patch's actual contents, just it's outline.
void DCanvas::DrawColoredPatchP (const byte *source, byte *dest, int count, int pitch)
{
	if (count <= 0)
		return;

	byte fill = (byte)V_ColorFill;

	do
	{
		*dest = fill;
		dest += pitch;
	} while (--count);
}


// Colored, translucent patch drawer
//
// This routine is the same for the stretched version since we don't
// care about the patch's actual contents, just it's outline.
void DCanvas::DrawColorLucentPatchP (const byte *source, byte *dest, int count, int pitch)
{
	if (count <= 0)
		return;

	argb_t *bg2rgb;

	{
		fixed_t fglevel, bglevel, translevel;

		translevel = (fixed_t)(0xFFFF * hud_transparency);
		fglevel = translevel & ~0x3ff;
		bglevel = FRACUNIT-fglevel;
		bg2rgb = Col2RGB8[bglevel>>10];
	}

	do
	{
		unsigned int bg = bg2rgb[*dest];
		bg = (bg+bg) | 0x1f07c1f;
		*dest = RGB32k[0][0][bg & (bg>>15)];
		dest += pitch;
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
		*((argb_t *)dest) = V_Palette.shade(*source++);
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
		*((argb_t *)dest) = V_Palette.shade(source[c >> 16]);
		dest += pitch;
		c += yinc;
	} while (--count);
}


// Translucent patch drawers (always 50%)
void DCanvas::DrawLucentPatchD (const byte *source, byte *dest, int count, int pitch)
{
	if (count <= 0)
		return;

	int alpha = (int)(hud_transparency * 255);
	int invAlpha = 255 - alpha;

	do
	{
		argb_t fg = V_Palette.shade(*source++);
		argb_t bg = *((argb_t *)dest);
		*((argb_t *)dest) = alphablend2a(bg, invAlpha, fg, alpha);
		dest += pitch;
	} while (--count);
}

void DCanvas::DrawLucentPatchSD (const byte *source, byte *dest, int count, int pitch, int yinc)
{
	if (count <= 0)
		return;

	int alpha = (int)(hud_transparency * 255);
	int invAlpha = 255 - alpha;

	int c = 0;

	do
	{
		argb_t fg = V_Palette.shade(source[c >> 16]);
		argb_t bg = *((argb_t *)dest);
		*((argb_t *)dest) = alphablend2a(bg, invAlpha, fg, alpha);
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
		*((argb_t *)dest) = V_Palette.tlate(V_ColorMap, *source++);
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
		*((argb_t *)dest) = V_Palette.tlate(V_ColorMap, source[c >> 16]);
		dest += pitch;
		c += yinc;
	} while (--count);
}


// Translated, translucent patch drawers
void DCanvas::DrawTlatedLucentPatchD (const byte *source, byte *dest, int count, int pitch)
{
	if (count <= 0)
		return;

	int alpha = (int)(hud_transparency * 255);
	int invAlpha = 255 - alpha;

	do
	{
		argb_t fg = V_Palette.tlate(V_ColorMap, *source++);
		argb_t bg = *((argb_t *)dest);
		*((argb_t *)dest) = alphablend2a(bg, invAlpha, fg, alpha);
		dest += pitch;
	} while (--count);
}

void DCanvas::DrawTlatedLucentPatchSD (const byte *source, byte *dest, int count, int pitch, int yinc)
{
	if (count <= 0)
		return;

	int alpha = (int)(hud_transparency * 255);
	int invAlpha = 255 - alpha;

	int c = 0;

	do
	{
		argb_t fg = V_Palette.tlate(V_ColorMap, source[c >> 16]);
		argb_t bg = *((argb_t *)dest);
		*((argb_t *)dest) = alphablend2a(bg, invAlpha, fg, alpha);
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
		*((argb_t *)dest) = color;
		dest += pitch;
	} while (--count);
}


// Colored, translucent patch drawer
//
// This routine is the same for the stretched version since we don't
// care about the patch's actual contents, just it's outline.
void DCanvas::DrawColorLucentPatchD (const byte *source, byte *dest, int count, int pitch)
{
	if (count <= 0)
		return;

	int alpha = (int)(hud_transparency * 255);
	int invAlpha = 255 - alpha;

	argb_t fg = V_Palette.shade(V_ColorFill);

	do
	{
		argb_t bg = *((argb_t *)dest);
		*((argb_t *)dest) = alphablend2a(bg, invAlpha, fg, alpha);
		dest += pitch;
	} while (--count);
}



/******************************/
/*							  */
/* The patch drawing wrappers */
/*							  */
/******************************/

//
// V_DrawWrapper
// Masks a column based masked pic to the screen.
//
void DCanvas::DrawWrapper(EWrapperCode drawer, const patch_t *patch, int x, int y) const
{
	int surface_width = mSurface->getWidth(), surface_height = mSurface->getHeight();
	int surface_pitch = mSurface->getPitch();
	int colstep = mSurface->getBytesPerPixel();
	vdrawfunc	drawfunc;

	y -= patch->topoffset();
	x -= patch->leftoffset();

#ifdef RANGECHECK
	if (x < 0 ||x + patch->width() > surface_width || y < 0 || y + patch->height() > surface_height)
	{
	  // Printf (PRINT_HIGH, "Patch at %d,%d exceeds LFB\n", x,y );
	  // No I_Error abort - what is up with TNT.WAD?
	  DPrintf ("DCanvas::DrawWrapper: bad patch (ignored)\n");
	  return;
	}
#endif

	if (mSurface->getBitsPerPixel() == 8)
		drawfunc = Pfuncs[drawer];
	else
		drawfunc = Dfuncs[drawer];

	// mark if this is the primary drawing surface
	if (mSurface == I_GetPrimarySurface())
		V_MarkRect(x, y, patch->width(), patch->height());

	byte* desttop = mSurface->getBuffer() + y * surface_pitch + x * colstep;

	int patchwidth = patch->width();

	for (int col = 0; col < patchwidth; x++, col++, desttop += colstep)
	{
		tallpost_t *post =
				(tallpost_t *)((byte *)patch + LELONG(patch->columnofs[col]));

		// step through the posts in a column
		while (!post->end())
		{
			drawfunc(post->data(), desttop + post->topdelta * surface_pitch, post->length, surface_pitch);
			post = post->next();
		}
	}
}

//
// V_DrawSWrapper
// Masks a column based masked pic to the screen
// stretching it to fit the given dimensions.
//
void DCanvas::DrawSWrapper(EWrapperCode drawer, const patch_t* patch, int x0, int y0,
                           const int destwidth, const int destheight) const
{
	if (!patch || patch->width() <= 0 || patch->height() <= 0 ||
	    destwidth <= 0 || destheight <= 0)
		return;

	if (destwidth == patch->width() && destheight == patch->height())
	{
		// Perfect 1:1 mapping, so we use the unscaled draw wrapper.
		DrawWrapper(drawer, patch, x0, y0);
		return;
	}

	int surface_width = mSurface->getWidth(), surface_height = mSurface->getHeight();
	int surface_pitch = mSurface->getPitch();
	int colstep = mSurface->getBytesPerPixel();
	vdrawsfunc drawfunc;

	// [AM] Adding 1 to the inc variables leads to fewer weird scaling
	//      artifacts since it forces col to roll over to the next real number
	//      a column-of-real-pixels sooner.
	int xinc = (patch->width() << FRACBITS) / destwidth + 1;
	int yinc = (patch->height() << FRACBITS) / destheight + 1;
	int xmul = (destwidth << FRACBITS) / patch->width();
	int ymul = (destheight << FRACBITS) / patch->height();

	y0 -= (patch->topoffset() * ymul) >> FRACBITS;
	x0 -= (patch->leftoffset() * xmul) >> FRACBITS;

#ifdef RANGECHECK
	if (x0 < 0 || x0 + destwidth > surface_width || y0 < 0 || y0 + destheight > surface_height)
	{
		DPrintf("DCanvas::DrawSWrapper: bad patch (ignored)\n");
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
	int w = MIN(destwidth * xinc, patch->width() << FRACBITS);

	for (int col = 0; col < w; col += xinc, desttop += colstep)
	{
		tallpost_t *post =
				(tallpost_t *)((byte *)patch + LELONG(patch->columnofs[col >> FRACBITS]));

		// step through the posts in a column
		while (!post->end())
		{
			drawfunc(post->data(),
			         desttop + (((post->topdelta * ymul)) >> FRACBITS) * surface_pitch,
			         (post->length * ymul) >> FRACBITS,
			         surface_pitch, yinc);

			post = post->next();
		}
	}
}

//
// V_DrawIWrapper
// Like V_DrawWrapper except it will stretch the patches as
// needed for non-320x200 screens.
//
void DCanvas::DrawIWrapper(EWrapperCode drawer, const patch_t *patch, int x0, int y0) const
{
	int surface_width = mSurface->getWidth(), surface_height = mSurface->getHeight();

	if (surface_width == 320 && surface_height == 200)
		DrawWrapper(drawer, patch, x0, y0);
	else
		DrawSWrapper(drawer, patch,
			 (surface_width * x0) / 320, (surface_height * y0) / 200,
			 (surface_width * patch->width()) / 320, (surface_height * patch->height()) / 200);
}

//
// V_DrawCWrapper
// Like V_DrawIWrapper, except it only uses integral multipliers.
//
void DCanvas::DrawCWrapper(EWrapperCode drawer, const patch_t *patch, int x0, int y0) const
{
	int surface_width = mSurface->getWidth(), surface_height = mSurface->getHeight();

	if (CleanXfac == 1 && CleanYfac == 1)
		DrawWrapper(drawer, patch, (x0-160) + (surface_width/2), (y0-100) + (surface_height/2));
	else
		DrawSWrapper(drawer, patch,
			(x0-160)*CleanXfac+(surface_width/2), (y0-100)*CleanYfac+(surface_height/2),
			patch->width() * CleanXfac, patch->height() * CleanYfac);
}

//
// V_DrawCNMWrapper
// Like V_DrawCWrapper, except it doesn't adjust the x and y coordinates.
//
void DCanvas::DrawCNMWrapper(EWrapperCode drawer, const patch_t *patch, int x0, int y0) const
{
	if (CleanXfac == 1 && CleanYfac == 1)
		DrawWrapper(drawer, patch, x0, y0);
	else
		DrawSWrapper(drawer, patch, x0, y0,
						patch->width() * CleanXfac,
						patch->height() * CleanYfac);
}


/********************************/
/*								*/
/* Other miscellaneous routines */
/*								*/
/********************************/


//
// V_DrawPatchFlipped
// Masks a column based masked pic to the screen.
// Flips horizontally, e.g. to mirror face.
//
// Like V_DrawIWrapper except it only uses one drawing function and draws
// the patch flipped horizontally.
//
void DCanvas::DrawPatchFlipped(const patch_t *patch, int x0, int y0) const
{
	int surface_width = mSurface->getWidth(), surface_height = mSurface->getHeight();
	int surface_pitch = mSurface->getPitch();
	int colstep = mSurface->getBytesPerPixel();

	vdrawsfunc	drawfunc;
	int			destwidth, destheight;

	x0 = (surface_width * x0) / 320;
	y0 = (surface_height * y0) / 200;
	destwidth = (surface_width * patch->width()) / 320;
	destheight = (surface_height * patch->height()) / 200;

	if (!patch || patch->width() <= 0 || patch->height() <= 0 || destwidth <= 0 || destheight <= 0)
		return;

	int xinc = (patch->width() << 16) / destwidth + 1;
	int yinc = (patch->height() << 16) / destheight + 1;
	int xmul = (destwidth << 16) / patch->width();
	int ymul = (destheight << 16) / patch->height();

	y0 -= (patch->topoffset() * ymul) >> 16;
	x0 -= (patch->leftoffset() * xmul) >> 16;

#ifdef RANGECHECK
	if (x0 < 0 || x0 + destwidth > surface_width || y0 < 0 || y0 + destheight > surface_height)
	{
		//Printf ("Patch at %d,%d exceeds LFB\n", x0,y0 );
		DPrintf ("DCanvas::DrawPatchFlipped: bad patch (ignored)\n");
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

	for (int col = (destwidth - 1) * xinc; col >= 0 ; col -= xinc, desttop += colstep)
	{
		tallpost_t *post =
				(tallpost_t *)((byte *)patch + LELONG(patch->columnofs[col >> 16]));

		// step through the posts in a column
		while (!post->end())
		{
			drawfunc (post->data(), desttop + (((post->topdelta * ymul)) >> 16) * surface_pitch,
					  (post->length * ymul) >> 16, surface_pitch, yinc);

			post = post->next();
		}
	}
}


//
// V_DrawBlock
// Draw a linear block of pixels into the view buffer.
//
void DCanvas::DrawBlock(int x, int y, int width, int height, const byte *src) const
{
	int surface_width = mSurface->getWidth(), surface_height = mSurface->getHeight();
	int surface_pitch = mSurface->getPitch();
	int colstep = mSurface->getBytesPerPixel();
	int line_length = surface_width * colstep;

#ifdef RANGECHECK
	if (x < 0 || x + width > surface_width || y < 0 || y + height > surface_height)
		I_Error("Bad DCanvas::DrawBlock");
#endif

	V_MarkRect(x, y, width, height);

	byte* dest = mSurface->getBuffer() + y * surface_pitch + x * colstep;

	while (height--)
	{
		memcpy(dest, src, line_length);
		src += line_length;
		dest += surface_pitch;
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
	int surface_pitch = mSurface->getPitch();
	int colstep = mSurface->getBytesPerPixel();
	int line_length = surface_width * colstep;

#ifdef RANGECHECK
	if (x < 0 || x + width > surface_width || y < 0 || y + height > surface_height)
		I_Error("Bad V_GetBlock");
#endif

	const byte* src = mSurface->getBuffer() + y * surface_pitch + x * colstep;

	while (height--)
	{
		memcpy(dest, src, line_length);
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
			int x, int y, int width, int height, int sourcepitchpixels)
{
	const PIXEL_T* source = (PIXEL_T*)sourcebuffer + y * sourcepitchpixels + x;
	PIXEL_T* dest = (PIXEL_T*)destbuffer;

	for (int col = x; col < x + width; col++)
	{
		const PIXEL_T* sourceptr = source++;
		for (int row = y; row < y + height; row++)
		{
			*dest++ = *sourceptr;
			sourceptr += sourcepitchpixels;
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
				x, y, width, height, mSurface->getPitchInPixels());
	else
		V_GetTransposedBlockGeneric<argb_t>(destbuffer, mSurface->getBuffer(),
				x, y, width, height, mSurface->getPitchInPixels());
}

VERSION_CONTROL (v_draw_cpp, "$Id$")

