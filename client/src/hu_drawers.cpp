// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2012 by Alex Mayfield.
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
//   HUD drawing functions.
//
//-----------------------------------------------------------------------------


#include "odamex.h"

#include "hu_drawers.h"
#include "i_video.h"
#include "v_video.h"
#include "v_text.h"
#include "v_font.h"
#include "resources/res_texture.h"

namespace hud {

// Round float to short integer.  Used by the scaling function.
short roundToShort(float f)
{
	return static_cast<short>(std::round(f));
}

// Return the number of scaled available horizontal pixels to draw on.
int XSize(const float scale)
{
	int x_scale = std::max(1, static_cast<int>(scale * CleanXfac));
	return I_GetSurfaceWidth() / x_scale;
}

// Return the number of scaled available vertical pixels to draw on.
int YSize(const float scale)
{
	int y_scale = std::max(1, static_cast<int>(scale * CleanYfac));
	return I_GetSurfaceHeight() / y_scale;
}


// Calculate the starting x and y coordinate and proper scaling factors for
// each of the HUD drawers.
void calculateOrigin(int& x, int& y,
                     const unsigned short w, const unsigned short h,
                     const float scale, int& x_scale, int& y_scale,
                     const x_align_t x_align, const y_align_t y_align,
                     const x_align_t x_origin, const y_align_t y_origin)
{
	int surface_width = I_GetSurfaceWidth(), surface_height = I_GetSurfaceHeight();

	// No such thing as "absolute origin".
	if (x_origin == X_ABSOLUTE || y_origin == Y_ABSOLUTE)
		return;

	// Since Doom's assets are so low-resolution, scaling is done by simple
	// doubling/tripling/etc. of the pixels with no alising.
	x_scale = std::max(1, static_cast<int>(scale * CleanXfac));
	y_scale = std::max(1, static_cast<int>(scale * CleanYfac));

	// "Alignment" is the side of the screen that the passed x and y values
	// are relative to.  Note that for X_RIGHT and Y_BOTTOM, the coordinate
	// system is flippxed.
	switch (x_align)
	{
	case X_LEFT:
		x = x * x_scale;
		break;
	case X_CENTER:
		x = (surface_width >> 1) + (x * x_scale);
		break;
	case X_RIGHT:
		x = surface_width - (x * x_scale);
		break;
	case X_ABSOLUTE:
		x = (x * surface_width) / (320 * x_scale);
		break;
	}

	switch (y_align)
	{
	case Y_TOP:
		y = y * y_scale;
		break;
	case Y_MIDDLE:
		y = (surface_height >> 1) + (y * y_scale);
		break;
	case Y_BOTTOM:
		y = surface_height - (y * y_scale);
		break;
	case Y_ABSOLUTE:
		y = (y * surface_height) / (200 * y_scale);
		break;
	}

	// "Origin" is the corner of the patch/whatever that the drawing function
	// should appear to begin drawing from.  All DCanvas patch drawers begin
	// at the top left, so for other cases we need to offset our x and y.
	switch (x_origin)
	{
	case X_CENTER:
		x = x - ((w * x_scale) >> 1);
		break;
	case X_RIGHT:
		x = x - (w * x_scale);
		break;
	default:
		break;
	}

	switch (y_origin)
	{
	case Y_MIDDLE:
		y = y - ((h * y_scale) >> 1);
		break;
	case Y_BOTTOM:
		y = y - (h * y_scale);
		break;
	default:
		break;
	}
}

// Fill an area with a solid color.
void Clear(int x, int y,
           const unsigned short w, const unsigned short h,
           const float scale,
           const x_align_t x_align, const y_align_t y_align,
           const x_align_t x_origin, const y_align_t y_origin,
           const argb_t color)
{
	// Turn our scaled coordinates into real coordinates.
	int x_scale, y_scale;
	calculateOrigin(x, y, w, h, scale, x_scale, y_scale, x_align, y_align, x_origin, y_origin);

	int maxHeight = I_GetSurfaceHeight();
	int maxWidth = I_GetSurfaceWidth();

	if (x + (w * x_scale) > maxWidth)
		return;
	if (y + (h * y_scale) > maxHeight)
		return;

	screen->Clear(x, y, x + (w * x_scale), y + (h * y_scale), color);
}

// Fill an area with a dimmed box.
void Dim(int x, int y,
         const unsigned short w, const unsigned short h,
         const float scale,
         const x_align_t x_align, const y_align_t y_align,
         const x_align_t x_origin, const y_align_t y_origin)
{
	// Turn our scaled coordinates into real coordinates.
	int x_scale, y_scale;
	calculateOrigin(x, y, w, h, scale, x_scale, y_scale, x_align, y_align, x_origin, y_origin);
	screen->Dim(x, y, w * x_scale, h * y_scale);
}


// Width of a string in HUD units, matching what DrawText will produce.
int GetTextWidth(const char* str, const float scale, const fontface_t face)
{
	if (!str)
		return 0;

	const int x_scale = std::max(1, static_cast<int>(scale * CleanXfac));
	return V_GetFaceFont(face, x_scale)->getTextWidth(str) / x_scale;
}

brokenlines_t* BreakLines(const char* str, const int maxwidth, const float scale,
                          const fontface_t face)
{
	const int x_scale = std::max(1, static_cast<int>(scale * CleanXfac));
	return V_BreakLinesFontPixels(V_GetFaceFont(face, x_scale), maxwidth * x_scale,
	                              reinterpret_cast<const byte*>(str));
}

// Height of a line of text in HUD units, matching what DrawText will produce.
int GetLineHeight(const float scale, const fontface_t face)
{
	const int y_scale = std::max(1, static_cast<int>(scale * CleanYfac));
	return V_GetFaceFont(face, y_scale)->getHeight() / y_scale;
}


// Draw text with the HUD font.
void DrawText(int x, int y, const float scale,
              const x_align_t x_align, const y_align_t y_align,
              const x_align_t x_origin, const y_align_t y_origin,
              const char* str, const int color,
              const bool force_opaque,
              const fontface_t face)
{
	// No string?  Don't bother with this function.
	if (!str)
		return;

	// Turn our scaled coordinates into real coordinates.
	int x_scale = 1, y_scale = 1;
	calculateOrigin(x, y, 0, 0, scale, x_scale, y_scale, x_align, y_align, x_origin, y_origin);

	// Calculate width and height of string
	const OFont* font = V_GetFaceFont(face, x_scale);
	const int w = font->getTextWidth(str);
	const int h = font->getHeight();

	// apply the origin offset ourselves, in real pixels
	if (x_origin == X_CENTER)
		x -= w >> 1;
	else if (x_origin == X_RIGHT)
		x -= w;

	if (y_origin == Y_MIDDLE)
		y -= h >> 1;
	else if (y_origin == Y_BOTTOM)
		y -= h;

	screen->DrawFontText(font, color, x, y, str, force_opaque);
}


// Draw a patch.
void DrawTexture(int x, int y, const float scale,
               const x_align_t x_align, const y_align_t y_align,
               const x_align_t x_origin, const y_align_t y_origin,
               const Texture* texture, const bool force_opaque,
               const bool use_patch_offsets)
{
	// Calculate width and height of patch
	unsigned short w = texture->mWidth;
	unsigned short h = texture->mHeight;

	// Turn our scaled coordinates into real coordinates.
	int x_scale, y_scale;
	calculateOrigin(x, y, w, h, scale, x_scale, y_scale, x_align, y_align, x_origin, y_origin);

	if (!use_patch_offsets)
	{
		// Negate scaled patch offsets.
		x += texture->mOffsetX * x_scale;
		y += texture->mOffsetY * y_scale;
	}

	if (force_opaque)
		screen->DrawTextureStretched(texture, x, y, w * x_scale, h * y_scale);
	else
		screen->DrawLucentTextureStretched(texture, x, y, w * x_scale, h * y_scale);
}

// Draw a color-translated patch.
void DrawTranslatedTexture(int x, int y, const float scale,
                         const x_align_t x_align, const y_align_t y_align,
                         const x_align_t x_origin, const y_align_t y_origin,
                         const Texture* texture, byte* translation,
                         const bool force_opaque, const bool use_patch_offsets)
{
	// Calculate width and height of patch
	unsigned short w = texture->mWidth;
	unsigned short h = texture->mHeight;

	// Turn our scaled coordinates into real coordinates.
	int x_scale, y_scale;
	calculateOrigin(x, y, w, h, scale, x_scale, y_scale, x_align, y_align, x_origin, y_origin);

	if (!use_patch_offsets)
	{
		// Negate scaled patch offsets.
		x += texture->mOffsetX * x_scale;
		y += texture->mOffsetY * y_scale;
	}

	V_ColorMap = translationref_t(translation);

	if (force_opaque)
		screen->DrawTranslatedTextureStretched(texture, x, y, w * x_scale, h * y_scale);
	else
		screen->DrawTranslatedLucentTextureStretched(texture, x, y, w * x_scale, h * y_scale);
}


// Draw a patch stretched to a specific width and height.
void DrawTextureStretched(int x, int y,
                        const unsigned short w, const unsigned short h,
                        const float scale,
                        const x_align_t x_align, const y_align_t y_align,
                        const x_align_t x_origin, const y_align_t y_origin,
                        const Texture* texture, const bool force_opaque,
                        const bool use_patch_offsets)
{
	// Turn our scaled coordinates into real coordinates.
	int x_scale, y_scale;
	calculateOrigin(x, y, w, h, scale, x_scale, y_scale, x_align, y_align, x_origin, y_origin);

	if (!use_patch_offsets)
	{
		// Negate scaled patch offsets.
		x += (roundToShort(texture->leftoffset() * (static_cast<float>(w) / texture->width()))) * x_scale;
		y += (roundToShort(texture->topoffset() * (static_cast<float>(h) / texture->height()))) * y_scale;
	}

	if (force_opaque)
		screen->DrawTextureStretched(texture, x, y, w * x_scale, h * y_scale);
	else
		screen->DrawLucentTextureStretched(texture, x, y, w * x_scale, h * y_scale);
}

// Draw a patch scaled to a specific width and height, preserving aspect ratio.
void DrawTextureScaled(const int x, const int y,
                     unsigned short w, unsigned short h,
                     const float scale,
                     const x_align_t x_align, const y_align_t y_align,
                     const x_align_t x_origin, const y_align_t y_origin,
                     const Texture* texture, const bool force_opaque,
                     const bool use_patch_offsets)
{
	// Calculate aspect ratios of patch and destination.
	float patch_aspect = texture->width() / static_cast<float>(texture->height());
	float dest_aspect = w / static_cast<float>(h);

	if (patch_aspect < dest_aspect) {
		// Destination is wider than patch.  Keep height, calculate width.
		w = (texture->mWidth * h) / texture->mHeight;
	} else if (patch_aspect > dest_aspect) {
		// Destination is taller than patch.  Keep width, calculate height.
		h = (texture->mHeight * w) / texture->mWidth;
	}

	// Call the 'stretched' drawer with our new dest. width and height.
	DrawTextureStretched(x, y, w, h, scale, x_align, y_align, x_origin, y_origin,
	                   texture, force_opaque, use_patch_offsets);
}


}	// end of namespace hud
