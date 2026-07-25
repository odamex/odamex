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

#pragma once

#include "r_defs.h"
#include "v_text.h"

class Texture;
struct brokenlines_t;

namespace hud {

enum x_align_t {
	X_LEFT, X_CENTER, X_RIGHT, X_ABSOLUTE
};

enum y_align_t {
	Y_TOP, Y_MIDDLE, Y_BOTTOM, Y_ABSOLUTE
};

int XSize(const float scale);
int YSize(const float scale);
void Clear(int x, int y,
           const unsigned short w, const unsigned short h,
           const float scale,
           const x_align_t x_align, const y_align_t y_align,
           const x_align_t x_origin, const y_align_t y_origin,
           const argb_t color,
           const int y_pixel_nudge = 0);
void Dim(int x, int y,
         const unsigned short w, const unsigned short h,
         const float scale,
         const x_align_t x_align, const y_align_t y_align,
         const x_align_t x_origin, const y_align_t y_origin);
int GetTextWidth(const char* str, const float scale, const fontface_t face = FACE_SMALL);
brokenlines_t* BreakLines(const char* str, const int maxwidth, const float scale,
                          const fontface_t face = FACE_SMALL);
int GetLineHeight(const float scale, const fontface_t face = FACE_SMALL);
void DrawText(int x, int y, const float scale,
              const x_align_t x_align, const y_align_t y_align,
              const x_align_t x_origin, const y_align_t y_origin,
              const char* str, const int color,
              const bool force_opaque = false,
              const fontface_t face = FACE_SMALL,
              const int y_pixel_nudge = 0);
void DrawTexture(int x, int y, const float scale,
               const x_align_t x_align, const y_align_t y_align,
               const x_align_t x_origin, const y_align_t y_origin,
               const Texture* texture, const bool force_opaque = false,
               const bool use_patch_offsets = false);
void DrawTranslatedTexture(int x, int y, const float scale,
                         const x_align_t x_align, const y_align_t y_align,
                         const x_align_t x_origin, const y_align_t y_origin,
                         const Texture* texture, byte* translation,
                         const bool force_opaque = false,
                         const bool use_patch_offsets = false,
                         const int y_pixel_nudge = 0);
void DrawTextureStretched(int x, int y,
                        const unsigned short w, const unsigned short h,
                        const float scale,
                        const x_align_t x_align, const y_align_t y_align,
                        const x_align_t x_origin, const y_align_t y_origin,
                        const Texture* texture, const bool force_opaque = false,
                        const bool use_patch_offsets = false);
void DrawTextureScaled(const int x, const int y,
                     unsigned short w, unsigned short h,
                     const float scale,
                     const x_align_t x_align, const y_align_t y_align,
                     const x_align_t x_origin, const y_align_t y_origin,
                     const Texture* texture, const bool force_opaque = false,
                     const bool use_patch_offsets = false);

inline void DrawPatch(int x, int y, const float scale,
                      const x_align_t xa, const y_align_t ya,
                      const x_align_t xo, const y_align_t yo,
                      const Texture* texture, const bool force_opaque = false,
                      const bool use_patch_offsets = false)
{
	DrawTexture(x, y, scale, xa, ya, xo, yo, texture, force_opaque, use_patch_offsets);
}

inline void DrawTranslatedPatch(int x, int y, const float scale,
                                const x_align_t xa, const y_align_t ya,
                                const x_align_t xo, const y_align_t yo,
                                const Texture* texture, byte* translation,
                                const bool force_opaque = false,
                                const bool use_patch_offsets = false)
{
	DrawTranslatedTexture(x, y, scale, xa, ya, xo, yo, texture, translation, force_opaque, use_patch_offsets);
}

inline void DrawPatchStretched(int x, int y,
                               const unsigned short w, const unsigned short h,
                               const float scale,
                               const x_align_t xa, const y_align_t ya,
                               const x_align_t xo, const y_align_t yo,
                               const Texture* texture, const bool force_opaque = false,
                               const bool use_patch_offsets = false)
{
	DrawTextureStretched(x, y, w, h, scale, xa, ya, xo, yo, texture, force_opaque, use_patch_offsets);
}

inline void DrawPatchScaled(const int x, const int y,
                            unsigned short w, unsigned short h,
                            const float scale,
                            const x_align_t xa, const y_align_t ya,
                            const x_align_t xo, const y_align_t yo,
                            const Texture* texture, const bool force_opaque = false,
                            const bool use_patch_offsets = false)
{
	DrawTextureScaled(x, y, w, h, scale, xa, ya, xo, yo, texture, force_opaque, use_patch_offsets);
}

}
