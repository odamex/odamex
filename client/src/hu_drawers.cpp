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

#include "hu_drawers.h"
#include "v_video.h"
#include "v_text.h"

namespace hud {

// Calculate the starting x and y coordinate and proper scaling factors for
// each of the HUD drawers.
void calculateOrigin(int& x, int& y,
                     const unsigned short w, const unsigned short h,
                     const float scale, int& x_scale, int& y_scale,
                     const x_align_t x_align, const y_align_t y_align,
                     const x_align_t x_origin, const y_align_t y_origin) {
	if (x_origin == X_ABSOLUTE || y_origin == Y_ABSOLUTE) {
		// No such thing as "absolute origin".
		return;
	}

	// Since Doom's assets are so low-resolution, scaling is done by simple
	// doubling/tripling/etc. of the pixels with no alising.
	x_scale = scale * CleanXfac;
	if (x_scale < 1) {
		x_scale = 1;
	}

	y_scale = scale * CleanYfac;
	if (y_scale < 1) {
		y_scale = 1;
	}

	// "Alignment" is the side of the screen that the passed x and y values
	// are relative to.  Note that for X_RIGHT and Y_BOTTOM, the coordinate
	// system is flippxed.
	switch (x_align) {
	case X_LEFT:
		x = x * x_scale;
		break;
	case X_CENTER:
		x = (screen->width >> 1) + (x * x_scale);
		break;
	case X_RIGHT:
		x = screen->width - (x * x_scale);
		break;
	case X_ABSOLUTE:
		x = (x * screen->width) / (320 * x_scale);
		break;
	}

	switch (y_align) {
	case Y_TOP:
		y = y * y_scale;
		break;
	case Y_MIDDLE:
		y = (screen->height >> 1) + (y * y_scale);
		break;
	case Y_BOTTOM:
		y = screen->height - (y * y_scale);
		break;
	case Y_ABSOLUTE:
		y = (y * screen->height) / (200 * y_scale);
		break;
	}

	// "Origin" is the corner of the patch/whatever that the drawing function
	// should appear to begin drawing from.  All DCanvas patch drawers begin
	// at the top left, so for other cases we need to offset our x and y.
	switch (x_origin) {
	case X_CENTER:
		x = x - ((w * x_scale) >> 1);
		break;
	case X_RIGHT:
		x = x - (w * x_scale);
		break;
	default:
		break;
	}

	switch (y_origin) {
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

// Round float to short integer.  Used by the scaling function.
short roundToShort(float f) {
	if (f >= 0.0f) {
		return (short)(f + 0.5f);
	} else {
		return (short)(f - 0.5f);
	}
}

// Return the number of scaled available horizontal pixels to draw on.
int XSize(const float scale) {
	int x_scale = scale * CleanXfac;
	if (x_scale < 1) {
		x_scale = 1;
	}
	return screen->width / x_scale;
}

// Return the number of scaled available horizontal pixels to draw on.
int YSize(const float scale) {
	int y_scale = scale * CleanYfac;
	if (y_scale < 1) {
		y_scale = 1;
	}
	return screen->height / y_scale;
}

// Fill an area with a solid color.
void Clear(int x, int y,
           const unsigned short w, const unsigned short h,
           const float scale,
           const x_align_t x_align, const y_align_t y_align,
           const x_align_t x_origin, const y_align_t y_origin,
           const int color) {
	// Turn our scaled coordinates into real coordinates.
	int x_scale, y_scale;
	calculateOrigin(x, y, w, h, scale, x_scale, y_scale,
	                x_align, y_align, x_origin, y_origin);
	screen->Clear(x, y, x + (w * x_scale), y + (h * y_scale), color);
}

// Fill an area with a dimmed box.
void Dim(int x, int y,
         const unsigned short w, const unsigned short h,
         const float scale,
         const x_align_t x_align, const y_align_t y_align,
         const x_align_t x_origin, const y_align_t y_origin) {
	// Turn our scaled coordinates into real coordinates.
	int x_scale, y_scale;
	calculateOrigin(x, y, w, h, scale, x_scale, y_scale,
	                x_align, y_align, x_origin, y_origin);
	screen->Dim(x, y, w * x_scale, h * y_scale);
}

// Draw hu_font text.
void DrawText(int x, int y, const float scale,
              const x_align_t x_align, const y_align_t y_align,
              const x_align_t x_origin, const y_align_t y_origin,
              const char* str, const int color,
              const bool force_opaque) {
	// No string?  Don't bother with this function.
	if (!str) {
		return;
	}

	// Calculate width and height of string
	unsigned short w = V_StringWidth(str);
	unsigned short h = 7;

	// Turn our scaled coordinates into real coordinates.
	int x_scale, y_scale;
	calculateOrigin(x, y, w, h, scale, x_scale, y_scale,
	                x_align, y_align, x_origin, y_origin);

	if (force_opaque) {
		screen->DrawTextStretched(color, x, y, str, x_scale, y_scale);
	} else {
		screen->DrawTextStretchedLuc(color, x, y, str, x_scale, y_scale);
	}
}

// Draw a patch.
void DrawPatch(int x, int y, const float scale,
               const x_align_t x_align, const y_align_t y_align,
               const x_align_t x_origin, const y_align_t y_origin,
               const patch_t* patch, const bool force_opaque,
               const bool use_patch_offsets) {
	// Calculate width and height of patch
	unsigned short w = patch->width();
	unsigned short h = patch->height();

	// Turn our scaled coordinates into real coordinates.
	int x_scale, y_scale;
	calculateOrigin(x, y, w, h, scale, x_scale, y_scale,
	                x_align, y_align, x_origin, y_origin);

	if (!use_patch_offsets) {
		// Negate scaled patch offsets.
		x += patch->leftoffset() * x_scale;
		y += patch->topoffset() * y_scale;
	}

	if (force_opaque) {
		screen->DrawPatchStretched(patch, x, y,
		                           w * x_scale, h * y_scale);
	} else {
		screen->DrawLucentPatchStretched(patch, x, y,
		                                 w * x_scale, h * y_scale);
	}
}

// Draw a color-translated patch.
void DrawTranslatedPatch(int x, int y, const float scale,
                         const x_align_t x_align, const y_align_t y_align,
                         const x_align_t x_origin, const y_align_t y_origin,
                         const patch_t* patch, byte* translation,
                         const bool force_opaque, const bool use_patch_offsets) {
	// Calculate width and height of patch
	unsigned short w = patch->width();
	unsigned short h = patch->height();

	// Turn our scaled coordinates into real coordinates.
	int x_scale, y_scale;
	calculateOrigin(x, y, w, h, scale, x_scale, y_scale,
	                x_align, y_align, x_origin, y_origin);

	if (!use_patch_offsets) {
		// Negate scaled patch offsets.
		x += patch->leftoffset() * x_scale;
		y += patch->topoffset() * y_scale;
	}

	V_ColorMap = translationref_t(translation);

	if (force_opaque) {
		screen->DrawTranslatedPatchStretched(patch, x, y,
		                                     w * x_scale, h * y_scale);
	} else {
		screen->DrawTranslatedLucentPatchStretched(patch, x, y,
		                                           w * x_scale, h * y_scale);
	}
}


// Draw a patch stretched to a specific width and height.
void DrawPatchStretched(int x, int y,
                        const unsigned short w, const unsigned short h,
                        const float scale,
                        const x_align_t x_align, const y_align_t y_align,
                        const x_align_t x_origin, const y_align_t y_origin,
                        const patch_t* patch, const bool force_opaque,
                        const bool use_patch_offsets) {
	// Turn our scaled coordinates into real coordinates.
	int x_scale, y_scale;
	calculateOrigin(x, y, w, h, scale, x_scale, y_scale,
	                x_align, y_align, x_origin, y_origin);

	if (!use_patch_offsets) {
		// Negate scaled patch offsets.
		x += (roundToShort(patch->leftoffset() * ((float)w / patch->width()))) * x_scale;
		y += (roundToShort(patch->topoffset() * ((float)h / patch->height()))) * y_scale;
	}

	if (force_opaque) {
		screen->DrawPatchStretched(patch, x, y,
		                           w * x_scale, h * y_scale);
	} else {
		screen->DrawLucentPatchStretched(patch, x, y,
		                                 w * x_scale,
		                                 h * y_scale);
	}
}

// Draw a patch scaled to a specific width and height, preserving aspect ratio.
void DrawPatchScaled(const int x, const int y,
                     unsigned short w, unsigned short h,
                     const float scale,
                     const x_align_t x_align, const y_align_t y_align,
                     const x_align_t x_origin, const y_align_t y_origin,
                     const patch_t* patch, const bool force_opaque,
                     const bool use_patch_offsets) {
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

}
