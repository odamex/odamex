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
 * @brief  Discard the per frame voxel projections.
 *
 * @detail Must be called once at the start of each rendered view, since
           every voxel from the previous frame points into memory released
           by this function.
 */
void VX_ClearVoxels();

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
void VX_NearbySprites();

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
bool VX_ProjectVoxel(const AActor* thing, int frame, vissprite_t* vis);

/**
 * @brief  Draw a vissprite whose voxel field was set by VX_ProjectVoxel.
 *
 * @detail Called from R_DrawSprite in place of R_DrawVisSprite, once
 *         mfloorclip and mceilingclip have been set up for the sprite.
 */
void VX_DrawVoxel(vissprite_t* spr);
