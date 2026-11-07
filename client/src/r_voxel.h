// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2023 by Andrew Apted.
// Copyright (C) 2026 by The Odamex Team.
//
// Software voxel rendering based on the original work by Andrew Apted,
// https://gitlab.com/andwj/voxel-rend
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
//  Software voxel rendering interface.
//
//-----------------------------------------------------------------------------

#pragma once

class AActor;
struct rendercontext_t;
struct vissprite_t;

/**
 * @brief  Parse VOXELDEF lumps and load every voxel model for the currently
 *         loaded sprites, discarding any previously loaded models.
 *
 * @detail Called from D_Init whenever the resource set changes. Voxels in
 *         VX_START/VX_END markers of loaded WADs take priority over .kvx files
 *         found in r_voxeldir.
 */
void VX_Init();

/**
 * @brief  Discard a slice's per frame voxel projections.
 *
 * @detail Must be called for each slice at the start of each rendered view,
 *         with its vissprites, since every voxel from the previous frame
 *         points into memory released by this function. Each slice keeps its
 *         own projections, so the slices can project at the same time.
 */
void VX_ClearVoxels(rendercontext_t& ctx);

/**
 * @brief  Project the sprites of sectors near the viewer that the BSP walk
 *         culled.
 *
 * @detail A sprite is a flat billboard, so R_RenderBSPNode only needs to add
 *         sprites from subsectors it decides are visible. A voxel has depth:
 *         an actor standing just behind the viewer, or in a subsector hidden
 *         by a one-sided line, can still have most of its model in view.
 *         This walks the BSP a second time without view culling and calls
 *         R_AddSprites for every sector within VX_NEAR_RADIUS of the viewer.
 *         Sectors that the main walk already handled are skipped by
 *         R_AddSprites' validcount check, so nothing is added twice.
 *
 *         Must be called after R_RenderBSPNode, so the sectors it added are
 *         already marked, and before R_DrawMasked sorts the vissprites.
 */
void VX_NearbySprites(rendercontext_t& ctx);

/**
 * @brief  Whether a thing in this frame would be drawn as a voxel.
 *
 * @detail A voxel can reach the screen when its flat sprite does not (a
 *         corpse is far longer than its sprite), so we need to account
 *         for that when deciding to draw a voxel or not.
 *
 * @param thing Actor being projected.
 * @param frame Sprite frame index, without FF_FULLBRIGHT.
 */
bool VX_HasVoxel(const AActor* thing, int frame);

/**
 * @brief  Replace a projected sprite with its voxel model, if one exists.
 *
 * @detail Called from R_ProjectSprite after the vissprite has been filled in
 *         for the flat sprite. On success, vis->voxel is set and the
 *         vissprite's screen extent and heights are replaced with the voxel's,
 *         otherwise vis is left untouched and the sprite is drawn as usual.
 *
 * @param thing Actor being projected.
 * @param frame Sprite frame index, without FF_FULLBRIGHT.
 * @param vis Vissprite generated for thing.
 * @return true if vis will be drawn as a voxel.
 */
bool VX_ProjectVoxel(rendercontext_t& ctx, const AActor* thing, int frame, vissprite_t* vis);

/**
 * @brief  Draw a vissprite whose voxel field was set by VX_ProjectVoxel.
 *
 * @detail Called from R_DrawSprite in place of R_DrawVisSprite, once the
 *         context's mfloorclip and mceilingclip have been set up for the
 *         sprite. Draws only within spr's x1..x2, which R_DrawSprite has
 *         clamped to the slice.
 */
void VX_DrawVoxel(rendercontext_t& ctx, vissprite_t* spr);
