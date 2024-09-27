// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1998-2006 by Randy Heit (ZDoom).
// Copyright (C) 2006-2024 by The Odamex Team.
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
//   Team Information.
//
//-----------------------------------------------------------------------------


#include "odamex.h"

#include <algorithm>
#include <sstream>
#include "cmdlib.h"
#include "teaminfo.h"
#include "v_textcolors.h"
#include "d_player.h"


EXTERN_CVAR(sv_teamsinplay)

TeamInfo s_Teams[NUMTEAMS];
TeamInfo s_NoTeam;

void InitTeamInfo()
{
	TeamInfo* teamInfo = &s_Teams[TEAM_BLUE];
	teamInfo->Team = TEAM_BLUE;
	teamInfo->Color = argb_t(255, 0, 0, 255);
	teamInfo->ColorStringUpper = "BLUE";
	teamInfo->ColorString = "Blue";
	teamInfo->TextColor = TEXTCOLOR_BLUE;
	teamInfo->ToastColor = TEXTCOLOR_LIGHTBLUE;
	teamInfo->TransColor = CR_BLUE;
	teamInfo->FountainColorArg = 3;
	teamInfo->TeamSpawnThingNum = 5080;
	teamInfo->FlagThingNum = 5130;
	teamInfo->FlagSocketSprite = SPR_BSOK;
	teamInfo->FlagSprite = SPR_BFLG;
	teamInfo->FlagDownSprite = SPR_BDWN;
	teamInfo->FlagWaypointSprite = SPR_WPBF;
	teamInfo->Points = 0;
	teamInfo->RoundWins = 0;

	teamInfo = &s_Teams[TEAM_RED];
	teamInfo->Team = TEAM_RED;
	teamInfo->Color = argb_t(255, 255, 0, 0);
	teamInfo->ColorStringUpper = "RED";
	teamInfo->ColorString = "Red";
	teamInfo->TextColor = TEXTCOLOR_RED;
	teamInfo->ToastColor = TEXTCOLOR_BRICK;
	teamInfo->TransColor = CR_RED;
	teamInfo->FountainColorArg = 1;
	teamInfo->TeamSpawnThingNum = 5081;
	teamInfo->FlagThingNum = 5131;
	teamInfo->FlagSocketSprite = SPR_RSOK;
	teamInfo->FlagSprite = SPR_RFLG;
	teamInfo->FlagDownSprite = SPR_RDWN;
	teamInfo->FlagCarrySprite = SPR_RCAR;
	teamInfo->FlagWaypointSprite = SPR_WPBF;
	teamInfo->Points = 0;
	teamInfo->RoundWins = 0;

	teamInfo = &s_Teams[TEAM_GREEN];
	teamInfo->Team = TEAM_GREEN;
	teamInfo->Color = argb_t(255, 0, 255, 0);
	teamInfo->ColorStringUpper = "GREEN";
	teamInfo->ColorString = "Green";
	teamInfo->TextColor = TEXTCOLOR_GREEN;
	teamInfo->ToastColor = TEXTCOLOR_GREEN;
	teamInfo->TransColor = CR_GREEN;
	teamInfo->FountainColorArg = 2;
	teamInfo->TeamSpawnThingNum = 5083;
	teamInfo->FlagThingNum = 5133;
	teamInfo->FlagSocketSprite = SPR_GSOK;
	teamInfo->FlagSprite = SPR_GFLG;
	teamInfo->FlagDownSprite = SPR_GDWN;
	teamInfo->FlagCarrySprite = SPR_GCAR;
	teamInfo->FlagWaypointSprite = SPR_WPBF;
	teamInfo->Points = 0;
	teamInfo->RoundWins = 0;

	s_NoTeam.Team = NUMTEAMS;
	s_NoTeam.Color = argb_t(255, 0, 255, 0);
	s_NoTeam.ColorStringUpper = "";
	s_NoTeam.ColorString = "";
	s_NoTeam.TextColor = TEXTCOLOR_GRAY;
	s_NoTeam.ToastColor = TEXTCOLOR_GRAY;
	s_NoTeam.TransColor = CR_GRAY;
	s_NoTeam.FountainColorArg = 0;
	s_NoTeam.TeamSpawnThingNum = 0;
	s_NoTeam.FlagSocketSprite = 0;
	s_NoTeam.FlagSprite = 0;
	s_NoTeam.FlagDownSprite = 0;
	s_NoTeam.FlagCarrySprite = 0;
	s_NoTeam.FlagWaypointSprite = 0;
	s_NoTeam.Points = 0;
	s_NoTeam.RoundWins = 0;
}

void TeamInfo_ResetScores(bool fullreset)
{
	// Clear teamgame state.
	Players::iterator it;
	for (size_t i = 0; i < NUMTEAMS; i++)
	{
		for (it = players.begin(); it != players.end(); ++it)
			it->flags[i] = false;

		TeamInfo* teamInfo = GetTeamInfo((team_t)i);
		teamInfo->FlagData.flagger = 0;
		teamInfo->FlagData.state = flag_home;
		teamInfo->FlagData.firstgrab = false;
		teamInfo->Points = 0;

		if (fullreset)
			teamInfo->RoundWins = 0;
	}
}

TeamInfo* GetTeamInfo(team_t team)
{
	if (team < 0 || team >= NUMTEAMS)
	{
		return &s_NoTeam;
	}

	return &s_Teams[team];
}

static bool cmpLives(TeamInfo* a, TeamInfo* b)
{
	return a->LivesPool() < b->LivesPool();
}

static bool cmpScore(TeamInfo* a, TeamInfo* b)
{
	return a->Points < b->Points;
}

static bool cmpWins(TeamInfo* a, const TeamInfo* b)
{
	return a->RoundWins < b->RoundWins;
}

/**
 * @brief Execute the query.
 *
 * @return Results of the query.
 */
TeamsView TeamQuery::execute()
{
	TeamsView results;

	for (size_t i = 0; i < NUMTEAMS; i++)
	{
		results.push_back(&::s_Teams[i]);
	}

	// Sort our list of players if needed.
	switch (m_sort)
	{
	case SORT_NONE:
		break;
	case SORT_LIVES:
		std::sort(results.rbegin(), results.rend(), cmpLives);
		if (m_sortFilter == SFILTER_MAX || m_sortFilter == SFILTER_NOT_MAX)
		{
			// Since it's sorted, we know the top scoring team is at the front.
			int top = results.at(0)->LivesPool();
			for (TeamsView::iterator it = results.begin(); it != results.end();)
			{
				bool cmp = (m_sortFilter == SFILTER_MAX) ? (*it)->LivesPool() != top
				                                         : (*it)->LivesPool() == top;
				if (cmp)
				{
					it = results.erase(it);
				}
				else
				{
					++it;
				}
			}
		}
		break;
	case SORT_SCORE:
		std::sort(results.rbegin(), results.rend(), cmpScore);
		if (m_sortFilter == SFILTER_MAX || m_sortFilter == SFILTER_NOT_MAX)
		{
			// Since it's sorted, we know the top scoring team is at the front.
			int top = results.at(0)->Points;
			for (TeamsView::iterator it = results.begin(); it != results.end();)
			{
				bool cmp = (m_sortFilter == SFILTER_MAX) ? (*it)->Points != top
				                                         : (*it)->Points == top;
				if (cmp)
				{
					it = results.erase(it);
				}
				else
				{
					++it;
				}
			}
		}
		break;
	case SORT_WINS:
		std::sort(results.rbegin(), results.rend(), cmpWins);
		if (m_sortFilter == SFILTER_MAX || m_sortFilter == SFILTER_NOT_MAX)
		{
			// Since it's sorted, we know the top winning team is at the front.
			int top = results.at(0)->RoundWins;
			for (TeamsView::iterator it = results.begin(); it != results.end();)
			{
				bool cmp = (m_sortFilter == SFILTER_MAX) ? (*it)->RoundWins != top
				                                         : (*it)->RoundWins == top;
				if (cmp)
				{
					it = results.erase(it);
				}
				else
				{
					++it;
				}
			}
		}
		break;
	}

	return results;
}

std::string V_GetTeamColor(team_t ateam)
{
	std::string buf;
	TeamInfo* team = GetTeamInfo(ateam);
	StrFormat(buf, "%s%s%s", team->TextColor.c_str(), team->ColorStringUpper.c_str(),
	          TEXTCOLOR_NORMAL);
	return buf;
}

std::string V_GetTeamColor(UserInfo userinfo)
{
	std::string buf;
	TeamInfo* team = GetTeamInfo(userinfo.team);
	StrFormat(buf, "%s%s%s", team->TextColor.c_str(), team->ColorStringUpper.c_str(), TEXTCOLOR_NORMAL);
	return buf;
}

const std::string TeamInfo::ColorizedTeamName()
{
	std::string buf;
	StrFormat(buf, "%s%s%s", TextColor.c_str(), ColorStringUpper.c_str(),
	          TEXTCOLOR_NORMAL);
	return buf;
}

int TeamInfo::LivesPool()
{
	int pool = 0;
	PlayerResults pr = PlayerQuery().hasLives().execute();

	for (PlayersView::const_iterator it = pr.players.begin(); it != pr.players.end();
	     ++it)
	{
		team_t team = (*it)->userinfo.team;
		if (team != Team)
			continue;

		pool += (*it)->lives;
	}

	return pool;
}
