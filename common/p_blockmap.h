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

#pragma once

#include <vector>
#include <algorithm>

#include "m_bbox.h"
#include "m_fixed.h"
#include "p_mobj.h"

// mapblocks are used to check movement
// against lines and things
inline constexpr int MAPBLOCKUNITS = 128;
inline constexpr int MAPBLOCKSIZE  = (MAPBLOCKUNITS*FRACUNIT);
inline constexpr int MAPBLOCKSHIFT = (FRACBITS+7);
inline constexpr int MAPBMASK      = (MAPBLOCKSIZE-1);
inline constexpr int MAPBTOFRAC    = (MAPBLOCKSHIFT-FRACBITS);

class blockmap_t
{
	fixed_t m_originx = 0;
	fixed_t m_originy = 0;
	int m_width  = 0;
	int m_height = 0;
	std::vector<AActor*> m_blocklinks;
	std::vector<std::vector<int>> m_blocklists;
	bool m_skipzerostart = false;

	void setSkipBlockStart()
	{
		m_skipzerostart = std::ranges::all_of(m_blocklists, [](auto&& list){ return !list.empty() && list.front() == 0; });
	}

	static blockmap_t create();
	static blockmap_t loadVanilla(std::span<const int16_t> lump);
	static blockmap_t loadXBM1(std::span<const int32_t> lump);

public:

	static blockmap_t load(int lump);

	[[nodiscard]]
	bool containsCoordinate(int x, int y) const noexcept
	{
		return !(x < 0 || y < 0 || x >= m_width || y >= m_height);
	}

	[[nodiscard]]
	std::span<const int> list(int x, int y) const;

	[[nodiscard]]
	int width() const noexcept
	{
		return m_width;
	}

	[[nodiscard]]
	int height() const noexcept
	{
		return m_height;
	}

	[[nodiscard]]
	fixed_t originx() const noexcept
	{
		return m_originx;
	}

	[[nodiscard]]
	fixed_t originy() const noexcept
	{
		return m_originy;
	}

	[[nodiscard]]
	int size() const noexcept
	{
		return m_width * m_height;
	}
};

inline blockmap_t blockmap;
