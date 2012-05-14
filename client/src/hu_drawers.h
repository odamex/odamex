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

#ifndef __HU_DRAWERS_H__
#define __HU_DRAWERS_H__

#include "r_defs.h"

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
           const int color);
void Dim(int x, int y,
         const unsigned short w, const unsigned short h,
         const float scale,
         const x_align_t x_align, const y_align_t y_align,
         const x_align_t x_origin, const y_align_t y_origin);
void DrawText(int x, int y, const float scale,
              const x_align_t x_align, const y_align_t y_align,
              const x_align_t x_origin, const y_align_t y_origin,
              const char* str, const int color,
              const bool force_opaque = false);
void DrawPatch(int x, int y, const float scale,
               const x_align_t x_align, const y_align_t y_align,
               const x_align_t x_origin, const y_align_t y_origin,
               const patch_t* patch, const bool force_opaque = false,
               const bool use_patch_offsets = false);
void DrawTranslatedPatch(int x, int y, const float scale,
                         const x_align_t x_align, const y_align_t y_align,
                         const x_align_t x_origin, const y_align_t y_origin,
                         const patch_t* patch, byte* translation,
                         const bool force_opaque = false,
                         const bool use_patch_offsets = false);
void DrawPatchStretched(int x, int y,
                        const unsigned short w, const unsigned short h,
                        const float scale,
                        const x_align_t x_align, const y_align_t y_align,
                        const x_align_t x_origin, const y_align_t y_origin,
                        const patch_t* patch, const bool force_opaque = false,
                        const bool use_patch_offsets = false);
void DrawPatchScaled(const int x, const int y,
                     unsigned short w, unsigned short h,
                     const float scale,
                     const x_align_t x_align, const y_align_t y_align,
                     const x_align_t x_origin, const y_align_t y_origin,
                     const patch_t* patch, const bool force_opaque = false,
                     const bool use_patch_offsets = false);
}

#endif
