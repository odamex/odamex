// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
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
//   Tests for multi kill bookkeeping.
//
//-----------------------------------------------------------------------------

#include "odamex.h"

#include <gtest/gtest.h>

#include "actor.h"
#include "g_multikill.h"
#include "i_system.h"
#include "teaminfo.h"

namespace
{

constexpr int FIRST_MULTI_LEVEL = 2;
constexpr int TOP_MULTI_LEVEL = 11;

constexpr int KILLER = 1;
constexpr int VICTIM = 2;

void AddPlayer(const int id, const std::string& name, const team_t team = TEAM_NONE)
{
	player_t& player = ::players.emplace_back();
	player.id = static_cast<byte>(id);
	player.userinfo.netname = name;
	player.userinfo.team = team;
}

class MultiKillTest : public ::testing::Test
{
  protected:
	void SetUp() override
	{
		::players.clear();
		AddPlayer(KILLER, "Killer");
		AddPlayer(VICTIM, "Victim");

		::gametic = 10000;

		m_oldGamestate = ::gamestate;
		::gamestate = GS_STARTUP;
		::sv_gametype.ForceSet(GM_DM);

		MultiKillManager::getInstance().reset();
		MultiKillManager::getInstance().loadMultiKillDefaults();
	}

	void TearDown() override
	{
		::sv_gametype.ForceSet(GM_COOP);
		::gamestate = m_oldGamestate;
		MultiKillManager::getInstance().reset();
		MultiKillManager::getInstance().loadMultiKillDefaults();
		::players.clear();
	}

	static const MultiKillTics_s& Status(const int playerId)
	{
		return MultiKillManager::getInstance().getMultiKills(playerId);
	}

	static void AddKills(const int playerId, const int count)
	{
		for (int i = 0; i < count; i++)
			MultiKillManager::getInstance().addKill(playerId);
	}

	static void TicUntilLapsed(const int playerId)
	{
		const int window = Status(playerId).ticsRemaining;

		for (int i = 0; i < window; i++)
			MultiKillManager::getInstance().ticPlayerMultiKill(playerId);
	}

	gamestate_t m_oldGamestate = GS_LEVEL;
};

// ==========================================================
// Counting kills.
// ==========================================================

TEST_F(MultiKillTest, UnknownPlayersHaveNoMultiKills)
{
	EXPECT_EQ(Status(KILLER).multiKills, 0);
	EXPECT_EQ(Status(KILLER).ticsRemaining, 0);
}

TEST_F(MultiKillTest, KillsStackUp)
{
	MultiKillManager::getInstance().addKill(KILLER);
	EXPECT_EQ(Status(KILLER).multiKills, 1);

	MultiKillManager::getInstance().addKill(KILLER);
	EXPECT_EQ(Status(KILLER).multiKills, 2);

	MultiKillManager::getInstance().addKill(KILLER);
	EXPECT_EQ(Status(KILLER).multiKills, 3);
}

TEST_F(MultiKillTest, EachKillStampsTheCurrentTicAndRefillsTheTimer)
{
	MultiKillManager::getInstance().addKill(KILLER);
	const int firstWindow = Status(KILLER).ticsRemaining;

	EXPECT_EQ(Status(KILLER).lastKillTime, ::gametic);
	EXPECT_GT(firstWindow, 0);

	// Let some of the window run down, then simulate another kill.
	for (int i = 0; i < 10; i++)
		MultiKillManager::getInstance().ticPlayerMultiKill(KILLER);

	EXPECT_EQ(Status(KILLER).ticsRemaining, firstWindow - 10);

	::gametic += 10;
	MultiKillManager::getInstance().addKill(KILLER);

	EXPECT_EQ(Status(KILLER).ticsRemaining, firstWindow);
	EXPECT_EQ(Status(KILLER).lastKillTime, ::gametic);
}

TEST_F(MultiKillTest, PlayersAreCountedSeparately)
{
	AddKills(KILLER, 3);
	AddKills(VICTIM, 1);

	EXPECT_EQ(Status(KILLER).multiKills, 3);
	EXPECT_EQ(Status(VICTIM).multiKills, 1);
}

// ==========================================================
// The timer.
// ==========================================================

TEST_F(MultiKillTest, StreaksLapseWhenTheTimerRunsOut)
{
	AddKills(KILLER, 3);
	TicUntilLapsed(KILLER);

	EXPECT_EQ(Status(KILLER).multiKills, 0);
}

TEST_F(MultiKillTest, ALapsedStreakStartsOverAtOne)
{
	AddKills(KILLER, 3);
	TicUntilLapsed(KILLER);

	MultiKillManager::getInstance().addKill(KILLER);

	EXPECT_EQ(Status(KILLER).multiKills, 1);
}

TEST_F(MultiKillTest, TicingAnUnknownPlayerIsHarmless)
{
	MultiKillManager::getInstance().ticPlayerMultiKill(KILLER);

	EXPECT_EQ(Status(KILLER).multiKills, 0);
}

TEST_F(MultiKillTest, KillsFromTheFutureAreDropped)
{
	// A rewinded netdemo leaves kills stamped ahead of the current tic.
	AddKills(KILLER, 2);
	::gametic -= 100;

	MultiKillManager::getInstance().ticPlayerMultiKill(KILLER);

	EXPECT_EQ(Status(KILLER).multiKills, 0);
}

TEST_F(MultiKillTest, ErasingAndClearingDropStreaks)
{
	AddKills(KILLER, 2);
	MultiKillManager::getInstance().eraseMultiKills(KILLER);
	EXPECT_EQ(Status(KILLER).multiKills, 0);

	AddKills(KILLER, 2);
	AddKills(VICTIM, 2);
	MultiKillManager::getInstance().clearMultiTics();
	EXPECT_EQ(Status(KILLER).multiKills, 0);
	EXPECT_EQ(Status(VICTIM).multiKills, 0);
}

// ==========================================================
// Multi-Kill Levels
// ==========================================================

TEST_F(MultiKillTest, LevelsBelowTwoAreEmptyPlaceholders)
{
	EXPECT_TRUE(MultiKillManager::getInstance().getMultiKillLevel(0).multikilltext.empty());
	EXPECT_TRUE(MultiKillManager::getInstance().getMultiKillLevel(1).multikilltext.empty());
}

TEST_F(MultiKillTest, RealLevelsStartAtTwo)
{
	EXPECT_FALSE(MultiKillManager::getInstance()
	                 .getMultiKillLevel(FIRST_MULTI_LEVEL)
	                 .multikilltext.empty());
}

TEST_F(MultiKillTest, LevelsAboveTheTopClampToTheTop)
{
	const MultiKillLevel_s& top =
	    MultiKillManager::getInstance().getMultiKillLevel(TOP_MULTI_LEVEL);
	const MultiKillLevel_s& past =
	    MultiKillManager::getInstance().getMultiKillLevel(TOP_MULTI_LEVEL + 50);

	EXPECT_FALSE(top.multikilltext.empty());
	EXPECT_EQ(top.multikilltext, past.multikilltext);
}

TEST_F(MultiKillTest, NegativeLevelsAreEmptyRatherThanTheTopLevel)
{
	const MultiKillLevel_s& level = MultiKillManager::getInstance().getMultiKillLevel(-1);
	const MultiKillLevel_s& top =
	    MultiKillManager::getInstance().getMultiKillLevel(TOP_MULTI_LEVEL);

	EXPECT_TRUE(level.multikilltext.empty());
	EXPECT_NE(level.multikilltext, top.multikilltext);
}

TEST_F(MultiKillTest, EveryDefaultLevelHasDistinctText)
{
	std::vector<std::string> seen;

	for (int level = FIRST_MULTI_LEVEL; level <= TOP_MULTI_LEVEL; level++)
	{
		const std::string text =
		    MultiKillManager::getInstance().getMultiKillLevel(level).multikilltext;

		EXPECT_FALSE(text.empty()) << "level " << level << " has no text";
		EXPECT_EQ(std::count(seen.begin(), seen.end(), text), 0)
		    << "level " << level << " repeats \"" << text << "\"";

		seen.push_back(text);
	}
}

TEST_F(MultiKillTest, TheIntervalIsSetInSecondsAndKeptInTics)
{
	std::vector<MultiKillLevel_s> levels;
	levels.emplace_back();
	levels.emplace_back();
	levels.emplace_back("Double Kill!", "", CR_WHITE);

	MultiKillManager::getInstance().setMultiKillLevels(levels, 3);
	MultiKillManager::getInstance().addKill(KILLER);

	EXPECT_EQ(Status(KILLER).ticsRemaining, 3 * TICRATE);
}

// ==========================================================
// P_ProcessMultiKills scoring rules
// ==========================================================

class MultiKillScoringTest : public MultiKillTest
{
  protected:
	static AActor* ActorFor(const int playerId)
	{
		auto* actor = new AActor();
		actor->player = &idplayer(static_cast<byte>(playerId));
		return actor;
	}
};

TEST_F(MultiKillScoringTest, AKillCountsTowardsTheKillersStreak)
{
	P_ProcessMultiKills(ActorFor(KILLER), &idplayer(static_cast<byte>(VICTIM)));

	EXPECT_EQ(Status(KILLER).multiKills, 1);
}

TEST_F(MultiKillScoringTest, DyingEndsTheVictimsStreak)
{
	AddKills(VICTIM, 3);

	P_ProcessMultiKills(ActorFor(KILLER), &idplayer(static_cast<byte>(VICTIM)));

	EXPECT_EQ(Status(VICTIM).multiKills, 0);
}

TEST_F(MultiKillScoringTest, KillingYourselfDoesNotCount)
{
	P_ProcessMultiKills(ActorFor(KILLER), &idplayer(static_cast<byte>(KILLER)));

	EXPECT_EQ(Status(KILLER).multiKills, 0);
}

TEST_F(MultiKillScoringTest, KillingATeammateDoesNotCount)
{
	::sv_gametype.ForceSet(GM_TEAMDM);
	idplayer(static_cast<byte>(KILLER)).userinfo.team = TEAM_BLUE;
	idplayer(static_cast<byte>(VICTIM)).userinfo.team = TEAM_BLUE;

	P_ProcessMultiKills(ActorFor(KILLER), &idplayer(static_cast<byte>(VICTIM)));

	EXPECT_EQ(Status(KILLER).multiKills, 0);
}

TEST_F(MultiKillScoringTest, KillingAnOpponentCountsInTeamGames)
{
	::sv_gametype.ForceSet(GM_TEAMDM);
	idplayer(static_cast<byte>(KILLER)).userinfo.team = TEAM_BLUE;
	idplayer(static_cast<byte>(VICTIM)).userinfo.team = TEAM_RED;

	P_ProcessMultiKills(ActorFor(KILLER), &idplayer(static_cast<byte>(VICTIM)));

	EXPECT_EQ(Status(KILLER).multiKills, 1);
}

TEST_F(MultiKillScoringTest, CoopKillsDoNotCount)
{
	::sv_gametype.ForceSet(GM_COOP);

	P_ProcessMultiKills(ActorFor(KILLER), &idplayer(static_cast<byte>(VICTIM)));

	EXPECT_EQ(Status(KILLER).multiKills, 0);
}

TEST_F(MultiKillScoringTest, EnvironmentalDeathsHaveNoKillerToCredit)
{
	AddKills(VICTIM, 2);

	P_ProcessMultiKills(nullptr, &idplayer(static_cast<byte>(VICTIM)));

	EXPECT_EQ(Status(VICTIM).multiKills, 0);
}

} // namespace
