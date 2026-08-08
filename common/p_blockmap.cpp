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
//		Blockmap loading and generation
//
//-----------------------------------------------------------------------------

#include <ranges>
#include <nonstd/scope.hpp>

#include "odamex.h"

#include "p_blockmap.h"
#include "p_setup.h"
#include "r_state.h"
#include "w_wad.h"

[[nodiscard]]
std::span<const int> blockmap_t::list(int x, int y) const
{
	// [RH] Get past starting 0 (from BOOM)
	// denis - not so fast, this breaks doom1.wad 1.9 demo1
	// [SL] The first entry in each block list appears to have been intended to
	// be used for a special purpose but instead contains garbage (most often
	// referencing linedef 0). Using this first entry (as vanilla Doom does) can
	// cause hitscan weapons to erroneously hit the first linedef entry regardless
	// of where that linedef is located in relation to the block.
	const std::span list = m_blocklists[(y * m_width) + x];
	if (!demoplayback && m_skipzerostart && !list.empty())
		return list.subspan(1);

	return list;
}

blockmap_t blockmap_t::loadVanilla(int lump)
{
	blockmap_t newblockmap;
	const size_t lump_size = W_LumpLength(lump) / sizeof(int16_t);
	auto *wadblockmaplump = W_CacheLumpNum<int16_t>(lump, PU_LEVEL);
	const auto guard = nonstd::make_scope_exit([&]{ Z_Free(wadblockmaplump); });

	// killough 3/1/98: Expand wad blockmap into larger internal one,
	// by treating all offsets except -1 as unsigned and zero-extending
	// them. This potentially doubles the size of blockmaps allowed,
	// because Doom originally considered the offsets as always signed.

	newblockmap.m_originx = INT2FIXED(LESHORT(wadblockmaplump[0]));
	newblockmap.m_originy = INT2FIXED(LESHORT(wadblockmaplump[1]));
	newblockmap.m_width = static_cast<uint16_t>(LESHORT(wadblockmaplump[2]));
	newblockmap.m_height = static_cast<uint16_t>(LESHORT(wadblockmaplump[3]));
	newblockmap.m_blocklists.resize(newblockmap.size());

	const size_t first_list = 4 + newblockmap.size();

	for (const auto i : std::views::iota(0, newblockmap.size()))
	{
		const auto offset = static_cast<size_t>(LESHORT(wadblockmaplump[i + 4]));
		if (offset < first_list || offset > lump_size)
			I_Error("Blockmap offset #{} ({}) is out of bounds.", i, offset);

		auto& list = newblockmap.m_blocklists[i];
		int16_t line = LESHORT(wadblockmaplump[offset]);
		size_t j = 1;
		while (line != -1)
		{
			if (static_cast<size_t>(line) > R_GetLines().size())
				I_Error("Blockmap list #{} contains non-existent line #{}", i, line);

			list.push_back(static_cast<uint16_t>(line));
			line = LESHORT(wadblockmaplump[offset + j]);
			j++;
		}
	}

	newblockmap.setSkipBlockStart();

	return newblockmap;
}

//
// jff 10/6/98
// New code added to speed up calculation of internal blockmap
// Algorithm is order of nlines*(ncols+nrows) not nlines*ncols*nrows
//

//
// Actually construct the blockmap lump from the level data
//
// This finds the intersection of each linedef with the column and
// row lines at the left and bottom of each blockmap cell. It then
// adds the line to all block lists touching the intersection.
//

blockmap_t blockmap_t::create()
{
	static constexpr int blkshift = 7;                  /* places to shift rel position for cell num */
	static constexpr int blkmask = (1 << blkshift) - 1; /* mask for rel position within cell */

	blockmap_t newblockmap;

	std::vector<std::vector<int32_t>> blocklists; // array of pointers to lists of lines
	std::vector<bool> blockdone; // array keeping track of blocks/line

	//
	// Subroutine to add a line number to a block list
	// It simply returns if the line is already in the block
	//
	const auto AddBlockLine = [&blocklists, &blockdone]
	(
		int blockno,
		int32_t lineno
	)
	{
		if (blockdone[blockno])
			return;

		blocklists[blockno].push_back(lineno);
		blockdone[blockno] = true;
	};

	// scan for map limits, which the blockmap must enclose
	int map_minx = limits::MAXINT;
	int map_miny = limits::MAXINT;
	int map_maxx = limits::MININT;
	int map_maxy = limits::MININT;
	for (auto& vertex : R_GetVertices())
	{
		fixed_t t = vertex.x;
		if (t < map_minx)
			map_minx = t;
		else if (t > map_maxx)
			map_maxx = t;

		t = vertex.y;
		if (t < map_miny)
			map_miny = t;
		else if (t > map_maxy)
			map_maxy = t;
	}
	map_minx >>= FRACBITS;    // work in map coords, not fixed_t
	map_maxx >>= FRACBITS;
	map_miny >>= FRACBITS;
	map_maxy >>= FRACBITS;

	// set up blockmap area to enclose level plus margin

	const int xorg = map_minx; // blockmap origin (lower left)
	const int yorg = map_miny;
	const int ncols = (map_maxx-xorg+1+blkmask)>>blkshift; //jff 10/12/98
	const int nrows = (map_maxy-yorg+1+blkmask)>>blkshift; //+1 needed for map exactly 1 cell

	const auto BlockIndex = [ncols](int x, int y){ return (y * ncols) + x; };

	const int NBlocks = ncols * nrows; // number of cells

	// create the array of pointers on NBlocks to blocklists
	// also create an array of linelist counts on NBlocks
	// finally make an array in which we can mark blocks done per line

	blocklists.resize(NBlocks);
	blockdone.resize(NBlocks);

	// For each linedef in the wad, determine all blockmap blocks it touches,
	// and add the linedef number to the blocklists for those blocks

	for (int i = 0; i < static_cast<int>(R_GetLines().size()); i++)
	{
		const int x1 = lines[i].v1->x>>FRACBITS; // lines[i] map coords
		const int y1 = lines[i].v1->y>>FRACBITS;
		const int x2 = lines[i].v2->x>>FRACBITS;
		const int y2 = lines[i].v2->y>>FRACBITS;
		const int dx = x2 - x1;
		const int dy = y2 - y1;
		const bool vert = (dx == 0);             // lines[i] slopetype
		const bool horiz = (dy == 0);
		const bool spos = (dx ^ dy) > 0;
		const bool sneg = (dx ^ dy) < 0;
		const int minx = x1 > x2 ? x2 : x1;        // extremal lines[i] coords
		const int maxx = x1 > x2 ? x1 : x2;
		const int miny = y1 > y2 ? y2 : y1;
		const int maxy = y1 > y2 ? y1 : y2;

		// no blocks done for this linedef yet

		blockdone.assign(NBlocks, false);

		// The line always belongs to the blocks containing its endpoints

		int bx = (x1-xorg) >> blkshift;
		int by = (y1-yorg) >> blkshift;
		AddBlockLine(BlockIndex(bx, by), i);
		bx = (x2-xorg) >> blkshift;
		by = (y2-yorg) >> blkshift;
		AddBlockLine(BlockIndex(bx, by), i);

		// For each column, see where the line along its left edge, which
		// it contains, intersects the Linedef i. Add i to each corresponding
		// blocklist.

		if (!vert)    // don't interesect vertical lines with columns
		{
			for (int j = 0; j < ncols; j++)
			{
				// intersection of Linedef with x=xorg+(j<<blkshift)
				// (y-y1)*dx = dy*(x-x1)
				// y = dy*(x-x1)+y1*dx;

				const int x = xorg+(j<<blkshift);		// (x,y) is intersection
				const int y = ((dy*(x-x1))/dx)+y1;
				const int yb = (y-yorg)>>blkshift;	// block row number
				const int yp = (y-yorg)&blkmask;		// y position within block

				if (yb<0 || yb>nrows-1)			// outside blockmap, continue
					continue;

				if (x<minx || x>maxx)			// line doesn't touch column
					continue;

				// The cell that contains the intersection point is always added

				AddBlockLine(BlockIndex(j, yb), i);

				// if the intersection is at a corner it depends on the slope
				// (and whether the line extends past the intersection) which
				// blocks are hit

				if (yp==0)			// intersection at a corner
				{
					if (sneg)		//   \ - blocks x,y-, x-,y
					{
						if (yb>0 && miny<y)
							AddBlockLine(BlockIndex(j, yb - 1), i);
						if (j>0 && minx<x)
							AddBlockLine(BlockIndex(j - 1, yb), i);
					}
					else if (spos)	//   / - block x-,y-
					{
						if (yb>0 && j>0 && minx<x)
							AddBlockLine(BlockIndex(j - 1, yb - 1), i);
					}
					else if (horiz)	//   - - block x-,y
					{
						if (j>0 && minx<x)
							AddBlockLine(BlockIndex(j - 1, yb), i);
					}
				}
				else if (j>0 && minx<x)	// else not at corner: x-,y
					AddBlockLine(BlockIndex(j - 1, yb), i);
			}
		}

		// For each row, see where the line along its bottom edge, which
		// it contains, intersects the Linedef i. Add i to all the corresponding
		// blocklists.

		if (!horiz)
		{
			for (int j = 0; j < nrows; j++)
			{
				// intersection of Linedef with y=yorg+(j<<blkshift)
				// (x,y) on Linedef i satisfies: (y-y1)*dx = dy*(x-x1)
				// x = dx*(y-y1)/dy+x1;

				const int y = yorg+(j<<blkshift);		// (x,y) is intersection
				const int x = ((dx*(y-y1))/dy)+x1;
				const int xb = (x-xorg)>>blkshift;	// block column number
				const int xp = (x-xorg)&blkmask;		// x position within block

				if (xb<0 || xb>ncols-1)			// outside blockmap, continue
					continue;

				if (y<miny || y>maxy)			 // line doesn't touch row
					continue;

				// The cell that contains the intersection point is always added

				AddBlockLine(BlockIndex(xb, j), i);

				// if the intersection is at a corner it depends on the slope
				// (and whether the line extends past the intersection) which
				// blocks are hit

				if (xp==0)			// intersection at a corner
				{
					if (sneg)       //   \ - blocks x,y-, x-,y
					{
						if (j>0 && miny<y)
							AddBlockLine(BlockIndex(xb, j - 1), i);
						if (xb>0 && minx<x)
							AddBlockLine(BlockIndex(xb - 1, j), i);
					}
					else if (vert)  //   | - block x,y-
					{
						if (j>0 && miny<y)
							AddBlockLine(BlockIndex(xb, j - 1), i);
					}
					else if (spos)  //   / - block x-,y-
					{
						if (xb>0 && j>0 && miny<y)
							AddBlockLine(BlockIndex(xb - 1, j - 1), i);
					}
				}
				else if (j>0 && miny<y) // else not on a corner: x,y-
					AddBlockLine (BlockIndex(xb, j - 1), i);
			}
		}
	}

	for (auto& list : blocklists)
	{
		std::ranges::reverse(list);
	}

	// blockmap header
	newblockmap.m_originx = INT2FIXED(xorg);
	newblockmap.m_originy = INT2FIXED(yorg);
	newblockmap.m_height = nrows;
	newblockmap.m_width = ncols;

	newblockmap.m_blocklists = std::move(blocklists);

	newblockmap.setSkipBlockStart();

	return newblockmap;
}
