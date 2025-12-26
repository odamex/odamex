// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 2006-2025 by The Odamex Team.
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
// CTF and team actor definitions
//
//-----------------------------------------------------------------------------

#pragma once

#include "doomdata.h"
#include "info.h"
#include "actor.h"


// data associated with a flag
struct flagdata
{
	// Does this flag have a spawn yet?
	bool flaglocated;

	// Actor when being carried by a player, follows player
	AActor::AActorPtr actor;

	// Integer representation of WHO has each flag (player id)
	byte flagger;
	int	pickup_time;

	// True if the flag is currently grabbed for the first time.
	bool firstgrab;

	// Flag locations
	int x, y, z;

	// Flag Timout Counters
	int timeout;

	// True when a flag has been dropped
	flag_state_t state;

	// Used for the blinking flag indicator on the statusbar
	int sb_tick;
};

struct TeamInfo
{
	team_t Team;
	std::string ColorStringUpper;
	std::string ColorString;
	argb_t Color;
	std::string TextColor;
	std::string ToastColor;
	int TransColor;

	int FountainColorArg;

	int TeamSpawnThingNum;
	int FlagThingNum;
	std::vector<mapthing2_t> Starts;

	int FlagSocketSprite;
	int FlagSprite;
	int FlagDownSprite;
	int FlagCarrySprite;
	int FlagWaypointSprite;

	int Points;
	int RoundWins;
	flagdata FlagData;

	const std::string ColorizedTeamName();
	int LivesPool();
};

/**
 * @brief A collection of pointers to teams, commonly called a "view".
 */
typedef std::vector<TeamInfo*> TeamsView;

class TeamQuery
{
	enum SortTypes
	{
		SORT_NONE,
		SORT_LIVES,
		SORT_SCORE,
		SORT_WINS,
	};

	enum SortFilters
	{
		SFILTER_NONE,
		SFILTER_MAX,
		SFILTER_NOT_MAX,
	};

	SortTypes m_sort;
	SortFilters m_sortFilter;

  public:
	TeamQuery() : m_sort(SORT_NONE), m_sortFilter(SFILTER_NONE)
	{
	}

	/**
	 * @brief Return teams sorted by greatest number of total lives.
	 *
	 * @return A mutated TeamQuery to chain off of.
	 */
	TeamQuery& sortLives()
	{
		m_sort = SORT_LIVES;
		return *this;
	}

	/**
	 * @brief Return teams sorted by highest score.
	 *
	 * @return A mutated TeamQuery to chain off of.
	 */
	TeamQuery& sortScore()
	{
		m_sort = SORT_SCORE;
		return *this;
	}

	/**
	 * @brief Return teams sorted by highest wins.
	 *
	 * @return A mutated TeamQuery to chain off of.
	 */
	TeamQuery& sortWins()
	{
		m_sort = SORT_WINS;
		return *this;
	}

	/**
	 * @brief Given a sort, filter so only the top item remains.  In the case
	 *        of a tie, multiple items are returned.
	 *
	 * @return A mutated TeamQuery to chain off of.
	 */
	TeamQuery& filterSortMax()
	{
		m_sortFilter = SFILTER_MAX;
		return *this;
	}

	/**
	 * @brief Given a sort, filter so only things other than the top item
	 *        remains.
	 *
	 * @return A mutated TeamQuery to chain off of.
	 */
	TeamQuery& filterSortNotMax()
	{
		m_sortFilter = SFILTER_NOT_MAX;
		return *this;
	}

	TeamsView execute();
};

void InitTeamInfo();
void TeamInfo_ResetScores(bool fullreset = true);
TeamInfo* GetTeamInfo(team_t team);
std::string V_GetTeamColor(TeamInfo* team);
std::string V_GetTeamColor(team_t ateam);
