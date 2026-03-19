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
//		Functions for loading, freeing, and drawing page image surfaces.
//		
//
//-----------------------------------------------------------------------------



#include "odamex.h"

#include "d_pages.h"

#include "i_video.h"
#include "v_video.h"
#include "w_wad.h"

static int D_GetDisplayPageHeight(const int page_height)
{
	return (page_height == 200) ? page_height + (page_height / 5) : page_height;
}

bool D_LoadPageImage(page_image_t& page, const OLumpName& lumpname, const bool is_raw)
{
	D_FreePageImage(page);

	if (lumpname.empty())
	{
		return false;
	}

	int page_width = 0;
	int page_height = 0;

	if (is_raw)
	{
		const int lumpnum = W_GetNumForName(lumpname);
		const unsigned lump_length = W_LumpLength(lumpnum);
		page_height = 200;
		page_width = (lump_length % page_height == 0) ? lump_length / page_height : 320;

		page.surface = I_AllocateSurface(page_width, page_height, 8);
		DCanvas* canvas = page.surface->getDefaultCanvas();
		page.surface->lock();

		const byte* raw_page = W_CacheLumpName<byte>(lumpname, PU_CACHE);
		canvas->DrawBlock(0, 0, page_width, page_height, raw_page);

		page.surface->unlock();
	}
	else
	{
		const patch_t* patch = W_CachePatch(lumpname);
		page_width = patch->width();
		page_height = patch->height();

		page.surface = I_AllocateSurface(page_width, page_height, I_GetVideoBitDepth());
		DCanvas* canvas = page.surface->getDefaultCanvas();
		page.surface->lock();
		canvas->DrawPatch(patch, 0, 0);
		page.surface->unlock();
	}

	page.width = page_width;
	page.height = page_height;
	page.display_height = D_GetDisplayPageHeight(page_height);
	return true;
}

void D_FreePageImage(page_image_t& page)
{
	I_FreeSurface(page.surface);
	page.surface = nullptr;
	page.width = 0;
	page.height = 0;
	page.display_height = 0;
}

void D_DrawPageImage(const page_image_t& page, IWindowSurface* dest_surface, const bool clear)
{
	const int surface_width = dest_surface->getWidth();
	const int surface_height = dest_surface->getHeight();

	if (clear)
	{
		dest_surface->clear();
	}

	int destw;
	int desth;

	if (I_IsProtectedResolution(I_GetVideoWidth(), I_GetVideoHeight()))
	{
		destw = surface_width;
		desth = surface_height;
	}
	else if (surface_width * 3 >= surface_height * 4)
	{
		destw = surface_height * 4 / 3;
		desth = surface_height;
	}
	else
	{
		destw = surface_width;
		desth = surface_width * 3 / 4;
	}

	destw = I_GetAspectCorrectWidth(desth, page.display_height, page.width);

	page.surface->lock();
	dest_surface->blitcrop(page.surface, 0, 0, page.surface->getWidth(), page.surface->getHeight(),
	                      (surface_width - destw) / 2, (surface_height - desth) / 2, destw, desth);
	page.surface->unlock();
}
