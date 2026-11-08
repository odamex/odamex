// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2026 Jess W
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
//   Controllable freecam object for various use cases
//
//-----------------------------------------------------------------------------
#pragma once

#include "odamex.h"
#include <list>

inline constexpr byte nullcam_id = 0;

// the freecam that is auto-added when CL_FreecamAllowedOnLevel
inline constexpr byte firstcam_id = 1;

// the freecam currently being displayed and controlled
inline byte freecam_id = 0;

struct save_pos_t
{
	fixed_t x{0};
	fixed_t y{0};
	fixed_t z{0};
	fixed_t angle{0};
	fixed_t pitch{0};

	void clear() { x = y = z = angle = pitch = 0; }
};

class freecam_t
{
public:
	freecam_t(byte id, mapthing2_t spawn, bool noclip);

	byte id;

	AActor::AActorPtr camera;
	AActor::AActorPtr mo;

	struct ticcmd_t cmd;

	void build(mapthing2_t spawn, bool setSpawnPos, bool noclip);
	void persistPosition();
	void moveToPosition(fixed_t x, fixed_t y, fixed_t z, angle_t angle);
	void reset();
	[[nodiscard]] bool needsPosition() const;
	[[nodiscard]] bool wipedOnLevelChange() const;
	static freecam_t* getfreecam(byte id);

	inline static std::string prevmap;

private:
	struct save_pos_t persist;
};

inline std::list<freecam_t> freecams;
