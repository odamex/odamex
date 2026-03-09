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

extern byte* Ranges;
EXTERN_CVAR(hud_transparency)

namespace hud {

// Round float to short integer.  Used by the scaling function.
short roundToShort(float f)
{
	if (f >= 0.0f)
		return (short)(f + 0.5f);
	else
		return (short)(f - 0.5f);
}

// Return the number of scaled available horizontal pixels to draw on.
int XSize(const float scale)
{
	int x_scale = std::max(1, int(scale * CleanXfac));
	return I_GetSurfaceWidth() / x_scale;
}

// Return the number of scaled available vertical pixels to draw on.
int YSize(const float scale)
{
	int y_scale = std::max(1, int(scale * CleanYfac));
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
	x_scale = std::max(1, int(scale * CleanXfac));
	y_scale = std::max(1, int(scale * CleanYfac));

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


// Draw hu_font text.
void DrawText(int x, int y, const float scale,
              const x_align_t x_align, const y_align_t y_align,
              const x_align_t x_origin, const y_align_t y_origin,
              const char* str, const int color,
              const bool force_opaque)
{
	// No string?  Don't bother with this function.
	if (!str)
		return;

	// Calculate width and height of string
	unsigned short w = V_StringWidth(str);
	unsigned short h = V_LineHeight();

	// Turn our scaled coordinates into real coordinates.
	int x_scale, y_scale;
	calculateOrigin(x, y, w, h, scale, x_scale, y_scale, x_align, y_align, x_origin, y_origin);

	if (force_opaque)
		screen->DrawTextStretched(color, x, y, str, x_scale, y_scale);
	else
		screen->DrawTextStretchedLuc(color, x, y, str, x_scale, y_scale);
}

struct shadow_text_metrics_t
{
	int width;
	int glyphs;
};

static shadow_text_metrics_t GetShadowTextMetrics(const char* str)
{
	shadow_text_metrics_t metrics = {0, 0};
	if (!str)
		return metrics;

	for (const unsigned char* p = reinterpret_cast<const unsigned char*>(str); *p != '\0'; ++p)
	{
		if (*p == TEXTCOLOR_ESCAPE && p[1] != '\0')
		{
			++p;
			continue;
		}
		if (*p == '\n')
			break;

		int c = toupper((*p) & 0x7f) - HU_FONTSTART;
		if (c < 0 || c >= HU_FONTSIZE || hu_font[c].empty())
			metrics.width += 4;
		else
			metrics.width += W_ResolvePatchHandle(hu_font[c])->width();

		metrics.glyphs++;
	}

	if (metrics.glyphs > 1)
		metrics.width += (metrics.glyphs - 1) * 2;

	return metrics;
}

// Draw hu_font text with an offset shadow pass.
void DrawShadowedText(int x, int y, const float scale,
                      const x_align_t x_align, const y_align_t y_align,
                      const x_align_t x_origin, const y_align_t y_origin,
                      const char* str, const int color,
                      const int shadow_color,
                      const int shadow_x_offset,
                      const int shadow_y_offset,
                      const bool force_opaque)
{
	if (!str)
		return;

	const shadow_text_metrics_t metrics = GetShadowTextMetrics(str);
	unsigned short w = metrics.width;
	unsigned short h = V_LineHeight();
	int x_scale, y_scale;
	calculateOrigin(x, y, w, h, scale, x_scale, y_scale, x_align, y_align, x_origin, y_origin);

	int draw_x = x;
	int glyph_index = 0;
	int current_color = color;

	for (const unsigned char* p = reinterpret_cast<const unsigned char*>(str); *p != '\0'; ++p)
	{
		if (*p == TEXTCOLOR_ESCAPE && p[1] != '\0')
		{
			const int new_color = V_GetTextColor(reinterpret_cast<const char*>(p));
			if (new_color >= 0)
				current_color = new_color;
			++p;
			continue;
		}
		if (*p == '\n')
			break;

		int c = toupper((*p) & 0x7f) - HU_FONTSTART;
		int glyph_width = 4;
		const patch_t* glyph = NULL;
		if (c >= 0 && c < HU_FONTSIZE && !hu_font[c].empty())
		{
			glyph = W_ResolvePatchHandle(hu_font[c]);
			glyph_width = glyph->width();
		}

		if (glyph)
		{
			const int safe_shadow_color =
			    (shadow_color >= 0 && shadow_color < NUM_TEXT_COLORS) ? shadow_color : CR_BLACK;
			V_ColorMap = translationref_t(Ranges + safe_shadow_color * 256);
			V_ColorFill = V_ColorMap.tlate(0x00);
			if (force_opaque)
				screen->DrawColoredPatchStretched(
				    glyph, draw_x + shadow_x_offset * x_scale,
				    y + shadow_y_offset * y_scale,
				    glyph->width() * x_scale, glyph->height() * y_scale);
			else
			{
				const float saved_hud_transparency = ::hud_transparency;
				float shadow_transparency = saved_hud_transparency * 0.5f;
				if (shadow_transparency < 0.0f)
					shadow_transparency = 0.0f;
				else if (shadow_transparency > 1.0f)
					shadow_transparency = 1.0f;
				::hud_transparency = shadow_transparency;
				screen->DrawColoredLucentPatchStretched(
				    glyph, draw_x + shadow_x_offset * x_scale,
				    y + shadow_y_offset * y_scale,
				    glyph->width() * x_scale, glyph->height() * y_scale);
				::hud_transparency = saved_hud_transparency;
			}

			const int fg_color = (current_color >= 0 && current_color < NUM_TEXT_COLORS)
			                         ? current_color
			                         : color;
			V_ColorMap = translationref_t(Ranges + fg_color * 256);
			if (force_opaque)
				screen->DrawTranslatedPatchStretched(
				    glyph, draw_x, y,
				    glyph->width() * x_scale, glyph->height() * y_scale);
			else
				screen->DrawTranslatedLucentPatchStretched(
				    glyph, draw_x, y,
				    glyph->width() * x_scale, glyph->height() * y_scale);
		}

		draw_x += glyph_width * x_scale;
		if (glyph_index < metrics.glyphs - 1)
			draw_x += 2 * x_scale;
		glyph_index++;
	}
}


// Draw a patch.
void DrawPatch(int x, int y, const float scale,
               const x_align_t x_align, const y_align_t y_align,
               const x_align_t x_origin, const y_align_t y_origin,
               const patch_t* patch, const bool force_opaque,
               const bool use_patch_offsets)
{
	if (patch == NULL)
		return;

	// Calculate width and height of patch
	unsigned short w = patch->width();
	unsigned short h = patch->height();

	// Turn our scaled coordinates into real coordinates.
	int x_scale, y_scale;
	calculateOrigin(x, y, w, h, scale, x_scale, y_scale, x_align, y_align, x_origin, y_origin);

	if (!use_patch_offsets)
	{
		// Negate scaled patch offsets.
		x += patch->leftoffset() * x_scale;
		y += patch->topoffset() * y_scale;
	}

	if (force_opaque)
		screen->DrawPatchStretched(patch, x, y, w * x_scale, h * y_scale);
	else
		screen->DrawLucentPatchStretched(patch, x, y, w * x_scale, h * y_scale);
}

// Draw a color-translated patch.
void DrawTranslatedPatch(int x, int y, const float scale,
                         const x_align_t x_align, const y_align_t y_align,
                         const x_align_t x_origin, const y_align_t y_origin,
                         const patch_t* patch, byte* translation,
                         const bool force_opaque, const bool use_patch_offsets)
{
	if (patch == NULL)
		return;

	// Calculate width and height of patch
	unsigned short w = patch->width();
	unsigned short h = patch->height();

	// Turn our scaled coordinates into real coordinates.
	int x_scale, y_scale;
	calculateOrigin(x, y, w, h, scale, x_scale, y_scale, x_align, y_align, x_origin, y_origin);

	if (!use_patch_offsets)
	{
		// Negate scaled patch offsets.
		x += patch->leftoffset() * x_scale;
		y += patch->topoffset() * y_scale;
	}

	V_ColorMap = translationref_t(translation);

	if (force_opaque)
		screen->DrawTranslatedPatchStretched(patch, x, y, w * x_scale, h * y_scale);
	else
		screen->DrawTranslatedLucentPatchStretched(patch, x, y, w * x_scale, h * y_scale);
}


// Draw a patch stretched to a specific width and height.
void DrawPatchStretched(int x, int y,
                        const unsigned short w, const unsigned short h,
                        const float scale,
                        const x_align_t x_align, const y_align_t y_align,
                        const x_align_t x_origin, const y_align_t y_origin,
                        const patch_t* patch, const bool force_opaque,
                        const bool use_patch_offsets)
{
	if (patch == NULL)
		return;

	// Turn our scaled coordinates into real coordinates.
	int x_scale, y_scale;
	calculateOrigin(x, y, w, h, scale, x_scale, y_scale, x_align, y_align, x_origin, y_origin);

	if (!use_patch_offsets)
	{
		// Negate scaled patch offsets.
		x += (roundToShort(patch->leftoffset() * ((float)w / patch->width()))) * x_scale;
		y += (roundToShort(patch->topoffset() * ((float)h / patch->height()))) * y_scale;
	}

	if (force_opaque)
		screen->DrawPatchStretched(patch, x, y, w * x_scale, h * y_scale);
	else
		screen->DrawLucentPatchStretched(patch, x, y, w * x_scale, h * y_scale);
}

// Draw a patch scaled to a specific width and height, preserving aspect ratio.
void DrawPatchScaled(const int x, const int y,
                     unsigned short w, unsigned short h,
                     const float scale,
                     const x_align_t x_align, const y_align_t y_align,
                     const x_align_t x_origin, const y_align_t y_origin,
                     const patch_t* patch, const bool force_opaque,
                     const bool use_patch_offsets)
{
	if (patch == NULL)
		return;

	// Calculate aspect ratios of patch and destination.
	float patch_aspect = patch->width() / (float)patch->height();
	float dest_aspect = w / (float)h;

	if (patch_aspect < dest_aspect) {
		// Destination is wider than patch.  Keep height, calculate width.
		w = (patch->width() * h) / patch->height();
	} else if (patch_aspect > dest_aspect) {
		// Destination is taller than patch.  Keep width, calculate height.
		h = (patch->height() * w) / patch->width();
	}

	// Call the 'stretched' drawer with our new dest. width and height.
	DrawPatchStretched(x, y, w, h, scale, x_align, y_align, x_origin, y_origin,
	                   patch, force_opaque, use_patch_offsets);
}


}	// end of namespace hud
