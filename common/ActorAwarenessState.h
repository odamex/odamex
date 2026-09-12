// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2026 by Jim Thoenen.
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
//  Utility to capture a mobj's corresponding level of awareness for each player.
//
//-----------------------------------------------------------------------------

#pragma once

#include <array>

enum class AwarenessEnum
{
	NOT_AWARE = 0,
	ALWAYS_AWARE,       ///< Permanently fully aware.  Avatars and all players.  Key part of nth_element comparator.
	FULLY_AWARE,        ///< All messages active, full fidelity.
	SEMI_AWARE,         ///< Reduced reliable traffic.
	BARELY_AWARE,       ///< On life support.

	AWARENESS_LEVEL_COUNT
};

template <size_t MAX_PLAYER_COUNT>
class ActorAwarenessState
{
	public:
		ActorAwarenessState()
		{
			m_player.fill(AwarenessEnum::NOT_AWARE);
		}

		bool IsAware(size_t playerId) const
		{
			return Get(playerId) != AwarenessEnum::NOT_AWARE;
		}

		AwarenessEnum Get(size_t playerId) const
		{
			return m_player[playerId];
		}

		void Set(size_t playerId, AwarenessEnum awareness)
		{
			m_player[playerId] = awareness;
		}

	protected:

		std::array<AwarenessEnum, MAX_PLAYER_COUNT> m_player;
};
