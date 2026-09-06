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
//   Tests for spree bookkeeping, the spree HUD rules and SPREEDEF parsing.
//
//-----------------------------------------------------------------------------

#include "odamex.h"

#include <gtest/gtest.h>

#include "g_spree.h"
#include "g_multikill.h"
#include "g_spreedef.h"
#include "i_system.h"
#include "teaminfo.h"
#include "v_textcolors.h"
#include "actor.h"
#include "p_inter.h"
#include "g_gametype.h"

namespace
{

// The defaults give six spree levels, so anything past level 5 is a repeat.
constexpr int TOP_SPREE_LEVEL = 5;

// Player IDs used throughout. WATCHED is the player the HUD is following.
constexpr int WATCHED = 1;
constexpr int OTHER = 2;
constexpr int THIRD = 3;

void AddPlayer(const int id, const std::string& name)
{
	player_t& player = ::players.emplace_back();
	player.id = static_cast<byte>(id);
	player.userinfo.netname = name;
}

class SpreeTest : public ::testing::Test
{
  protected:
	void SetUp() override
	{
		::players.clear();
		AddPlayer(WATCHED, "Watched");
		AddPlayer(OTHER, "Other");
		AddPlayer(THIRD, "Third");

		::gametic = 10000;

		SpreeManager::getInstance().reset();
		SpreeManager::getInstance().loadSpreeDefaults();
	}

	void TearDown() override
	{
		SpreeManager::getInstance().reset();
		SpreeManager::getInstance().loadSpreeDefaults();
		::players.clear();
	}

	// Records a spree for a player as if the server had announced it that many tics ago.
	static void GiveSpree(const int playerId, const int level, const int ticsAgo = 0)
	{
		SpreeManager::getInstance().setRawSpree(playerId, level, ticsAgo);
	}

	// Breaks playerId's spree, stamped as having happened ticsAgo tics ago.
	static void BreakSpree(const int endedId, const int enderId, const int level,
	                       const int ticsAgo = 0)
	{
		SpreeBreaker_t breaker;
		breaker.spreeEndedPlayerId = endedId;
		breaker.spreeEndedName = "Ended";
		breaker.spreeEnderPlayerId = enderId;
		breaker.spreeEnderName = "Ender";

		SpreeManager::getInstance().setRawSpreeBreaker(breaker, level, BR_PLAYER,
		                                               ticsAgo);
	}
};

// ==========================================================
// Big line: only ever the watched player's own live spree.
// ==========================================================

TEST_F(SpreeTest, BigLineShowsWatchedPlayersSpree)
{
	GiveSpree(WATCHED, 1);

	const SpreeHudLines_t lines = P_GetSpreeHudLines(WATCHED);

	ASSERT_NE(lines.bigSpree, nullptr);
	EXPECT_EQ(lines.bigSpree->playerId, WATCHED);
	EXPECT_EQ(lines.smallSpree, nullptr);
	EXPECT_EQ(lines.smallBreaker, nullptr);
}

TEST_F(SpreeTest, BigLineEmptyWithNoSprees)
{
	const SpreeHudLines_t lines = P_GetSpreeHudLines(WATCHED);

	EXPECT_EQ(lines.bigSpree, nullptr);
	EXPECT_EQ(lines.smallSpree, nullptr);
	EXPECT_EQ(lines.smallBreaker, nullptr);
}

TEST_F(SpreeTest, BigLineIgnoresOtherPlayersSprees)
{
	GiveSpree(OTHER, 1);

	const SpreeHudLines_t lines = P_GetSpreeHudLines(WATCHED);

	EXPECT_EQ(lines.bigSpree, nullptr);
	ASSERT_NE(lines.smallSpree, nullptr);
	EXPECT_EQ(lines.smallSpree->playerId, OTHER);
}

TEST_F(SpreeTest, StillDominatingIsNeverTheBigLine)
{
	GiveSpree(WATCHED, TOP_SPREE_LEVEL + 1);

	const SpreeHudLines_t lines = P_GetSpreeHudLines(WATCHED);

	EXPECT_EQ(lines.bigSpree, nullptr);
	ASSERT_NE(lines.smallSpree, nullptr);
	EXPECT_EQ(lines.smallSpree->playerId, WATCHED);
	EXPECT_TRUE(lines.smallSpree->stillDominating);
}

TEST_F(SpreeTest, TopLevelExactlyIsStillTheBigLine)
{
	GiveSpree(WATCHED, TOP_SPREE_LEVEL);

	const SpreeHudLines_t lines = P_GetSpreeHudLines(WATCHED);

	ASSERT_NE(lines.bigSpree, nullptr);
	EXPECT_FALSE(lines.bigSpree->stillDominating);
	EXPECT_EQ(lines.smallSpree, nullptr);
}

// ==========================================================
// Small line: everyone else, plus our own repeat.
// ==========================================================

TEST_F(SpreeTest, OwnSpreeIsNeverEchoedOnTheSmallLine)
{
	GiveSpree(WATCHED, 2);

	const SpreeHudLines_t lines = P_GetSpreeHudLines(WATCHED);

	ASSERT_NE(lines.bigSpree, nullptr);
	EXPECT_EQ(lines.smallSpree, nullptr);
}

TEST_F(SpreeTest, BigAndSmallLinesCoexist)
{
	GiveSpree(OTHER, 1);
	GiveSpree(WATCHED, 1);

	const SpreeHudLines_t lines = P_GetSpreeHudLines(WATCHED);

	ASSERT_NE(lines.bigSpree, nullptr);
	EXPECT_EQ(lines.bigSpree->playerId, WATCHED);
	ASSERT_NE(lines.smallSpree, nullptr);
	EXPECT_EQ(lines.smallSpree->playerId, OTHER);
}

TEST_F(SpreeTest, SmallLineTakesTheLatestOfSeveralSprees)
{
	GiveSpree(OTHER, 1, 60);  // oldest
	GiveSpree(THIRD, 1, 20);  // newest

	const SpreeHudLines_t lines = P_GetSpreeHudLines(WATCHED);

	ASSERT_NE(lines.smallSpree, nullptr);
	EXPECT_EQ(lines.smallSpree->playerId, THIRD);
}

TEST_F(SpreeTest, NewerBreakerWinsTheSmallLineOverASpree)
{
	GiveSpree(OTHER, 1, 60);
	BreakSpree(THIRD, OTHER, 1, 20);

	const SpreeHudLines_t lines = P_GetSpreeHudLines(WATCHED);

	ASSERT_NE(lines.smallBreaker, nullptr);
	EXPECT_EQ(lines.smallBreaker->spreeEndedPlayerId, THIRD);
	EXPECT_EQ(lines.smallSpree, nullptr);
}

TEST_F(SpreeTest, NewerSpreeWinsTheSmallLineOverABreaker)
{
	BreakSpree(THIRD, OTHER, 1, 60);
	GiveSpree(OTHER, 1, 20);

	const SpreeHudLines_t lines = P_GetSpreeHudLines(WATCHED);

	ASSERT_NE(lines.smallSpree, nullptr);
	EXPECT_EQ(lines.smallSpree->playerId, OTHER);
	EXPECT_EQ(lines.smallBreaker, nullptr);
}

TEST_F(SpreeTest, NewerBreakerWinsOverOurOwnRepeat)
{
	GiveSpree(WATCHED, TOP_SPREE_LEVEL + 1, 60);
	BreakSpree(THIRD, OTHER, 1, 20);

	const SpreeHudLines_t lines = P_GetSpreeHudLines(WATCHED);

	EXPECT_EQ(lines.bigSpree, nullptr);
	ASSERT_NE(lines.smallBreaker, nullptr);
	EXPECT_EQ(lines.smallSpree, nullptr);
}

TEST_F(SpreeTest, OurOwnRepeatWinsOverAnOlderSpree)
{
	GiveSpree(OTHER, 1, 60);
	GiveSpree(WATCHED, TOP_SPREE_LEVEL + 1, 20);

	const SpreeHudLines_t lines = P_GetSpreeHudLines(WATCHED);

	ASSERT_NE(lines.smallSpree, nullptr);
	EXPECT_EQ(lines.smallSpree->playerId, WATCHED);
	EXPECT_TRUE(lines.smallSpree->stillDominating);
}

TEST_F(SpreeTest, WatchingAnotherPlayerSwapsWhichSpreeIsBig)
{
	GiveSpree(WATCHED, 1);
	GiveSpree(OTHER, 1);

	const SpreeHudLines_t lines = P_GetSpreeHudLines(OTHER);

	ASSERT_NE(lines.bigSpree, nullptr);
	EXPECT_EQ(lines.bigSpree->playerId, OTHER);
	ASSERT_NE(lines.smallSpree, nullptr);
	EXPECT_EQ(lines.smallSpree->playerId, WATCHED);
}

// ==========================================================
// Record bookkeeping.
// ==========================================================

TEST_F(SpreeTest, StillDominatingIsDerivedWhenTheRecordIsCreated)
{
	// The netdemo rewind case - no prior record, straight to a repeat level.
	GiveSpree(WATCHED, TOP_SPREE_LEVEL + 2);

	const SpreeRecord_t& record = SpreeManager::getInstance().getSpreeRecord(WATCHED);

	EXPECT_TRUE(record.stillDominating);
	EXPECT_EQ(record.spreeLevel, TOP_SPREE_LEVEL + 2);
}

TEST_F(SpreeTest, StillDominatingIsDerivedWhenTheRecordIsUpgraded)
{
	GiveSpree(WATCHED, TOP_SPREE_LEVEL);
	EXPECT_FALSE(SpreeManager::getInstance().getSpreeRecord(WATCHED).stillDominating);

	GiveSpree(WATCHED, TOP_SPREE_LEVEL + 1);
	EXPECT_TRUE(SpreeManager::getInstance().getSpreeRecord(WATCHED).stillDominating);
}

TEST_F(SpreeTest, TicsAgoPlacesTheSpreeInThePast)
{
	GiveSpree(WATCHED, 1, 70);

	const SpreeRecord_t& record = SpreeManager::getInstance().getSpreeRecord(WATCHED);

	EXPECT_EQ(record.spreeStartTic, ::gametic - 70);
}

TEST_F(SpreeTest, LowerLevelsDoNotDowngradeARecord)
{
	GiveSpree(WATCHED, 3);
	GiveSpree(WATCHED, 1);

	EXPECT_EQ(SpreeManager::getInstance().getSpreeRecord(WATCHED).spreeLevel, 3);
}

TEST_F(SpreeTest, NegativeLevelsAreIgnored)
{
	EXPECT_FALSE(SpreeManager::getInstance().setRawSpree(WATCHED, -1, 0));
	EXPECT_FALSE(SpreeManager::getInstance().hasSpree(WATCHED));
}

TEST_F(SpreeTest, RemovingASpreeClearsTheBigLine)
{
	GiveSpree(WATCHED, 1);
	SpreeManager::getInstance().removeSpree(WATCHED);

	EXPECT_FALSE(SpreeManager::getInstance().hasSpree(WATCHED));
	EXPECT_EQ(P_GetSpreeHudLines(WATCHED).bigSpree, nullptr);
}

TEST_F(SpreeTest, SpreesFromTheFutureExpire)
{
	// A rewinded netdemo leaves records stamped ahead of the current tic.
	GiveSpree(WATCHED, 1);
	::gametic -= 100;

	SpreeManager::getInstance().expireOldSprees();

	EXPECT_FALSE(SpreeManager::getInstance().hasSpree(WATCHED));
}

TEST_F(SpreeTest, BreakersFromTheFutureExpire)
{
	BreakSpree(OTHER, THIRD, 1);
	::gametic -= 100;

	SpreeManager::getInstance().expireOldSprees();

	EXPECT_EQ(SpreeManager::getInstance().getSpreeBreaker().spreeEndedPlayerId, -1);
	EXPECT_EQ(P_GetSpreeHudLines(WATCHED).smallBreaker, nullptr);
}

TEST_F(SpreeTest, StaleBreakersExpire)
{
	BreakSpree(OTHER, THIRD, 1);
	::gametic += SPREE_DISPLAY_TICS + 1;

	SpreeManager::getInstance().expireOldSprees();

	EXPECT_EQ(SpreeManager::getInstance().getSpreeBreaker().spreeEndedPlayerId, -1);
}

TEST_F(SpreeTest, ClearingSpreesEmptiesTheHud)
{
	GiveSpree(WATCHED, 1);
	GiveSpree(OTHER, 1);
	BreakSpree(THIRD, OTHER, 1);

	SpreeManager::getInstance().clearSprees();

	const SpreeHudLines_t lines = P_GetSpreeHudLines(WATCHED);

	EXPECT_EQ(lines.bigSpree, nullptr);
	EXPECT_EQ(lines.smallSpree, nullptr);
	EXPECT_EQ(lines.smallBreaker, nullptr);
}

// ==========================================================
// Round boundaries.
// ==========================================================

class SpreeRoundTest : public ::testing::Test
{
  protected:
	void SetUp() override
	{
		::players.clear();
		AddPlayer(WATCHED, "Watched");
		AddPlayer(OTHER, "Other");

		::gametic = 10000;

		SpreeManager::getInstance().reset();
		SpreeManager::getInstance().loadSpreeDefaults();
		MultiKillManager::getInstance().reset();
		MultiKillManager::getInstance().loadMultiKillDefaults();
	}

	void TearDown() override
	{
		SpreeManager::getInstance().reset();
		SpreeManager::getInstance().loadSpreeDefaults();
		MultiKillManager::getInstance().reset();
		MultiKillManager::getInstance().loadMultiKillDefaults();
		::players.clear();
	}
};

TEST_F(SpreeRoundTest, ClearingRoundStatsWipesSpreesAndMultiKills)
{
	SpreeManager::getInstance().setRawSpree(WATCHED, 2, 0);
	SpreeManager::getInstance().setRawSpree(OTHER, 1, 0);
	MultiKillManager::getInstance().addKill(WATCHED);

	ASSERT_TRUE(SpreeManager::getInstance().hasSpree(WATCHED));
	ASSERT_EQ(MultiKillManager::getInstance().getMultiKills(WATCHED).multiKills, 1);

	G_ClearRoundKillStats();

	EXPECT_FALSE(SpreeManager::getInstance().hasSpree(WATCHED));
	EXPECT_FALSE(SpreeManager::getInstance().hasSpree(OTHER));
	EXPECT_EQ(MultiKillManager::getInstance().getMultiKills(WATCHED).multiKills, 0);
}

TEST_F(SpreeRoundTest, ClearingRoundStatsWipesTheBreakerAndTheHud)
{
	SpreeManager::getInstance().setRawSpree(WATCHED, 2, 0);

	SpreeBreaker_t breaker;
	breaker.spreeEndedPlayerId = OTHER;
	breaker.spreeEndedName = "Other";
	breaker.spreeEnderPlayerId = WATCHED;
	breaker.spreeEnderName = "Watched";
	SpreeManager::getInstance().setRawSpreeBreaker(breaker, 1, BR_PLAYER, 0);

	G_ClearRoundKillStats();

	const SpreeHudLines_t lines = P_GetSpreeHudLines(WATCHED);

	EXPECT_EQ(lines.bigSpree, nullptr);
	EXPECT_EQ(lines.smallSpree, nullptr);
	EXPECT_EQ(lines.smallBreaker, nullptr);
}

TEST_F(SpreeRoundTest, ClearingRoundStatsAlsoDropsPoints)
{
	const player_t& player = idplayer(static_cast<byte>(WATCHED));

	for (int i = 0; i < 5; i++)
		SpreeManager::getInstance().recordPlayerKill(&player);

	ASSERT_EQ(SpreeManager::getInstance().getPoints(WATCHED), 5);

	G_ClearRoundKillStats();

	EXPECT_EQ(SpreeManager::getInstance().getPoints(WATCHED), 0);
}

// ==========================================================
// What counts towards a spree.
// ==========================================================

class SpreeScoringTest : public ::testing::Test
{
  protected:
	void SetUp() override
	{
		::players.clear();
		AddPlayer(WATCHED, "Watched");
		AddPlayer(OTHER, "Other");

		::gametic = 10000;

		// sv_gametype is latched, and latched cvars are deferred while a level is in
		// progress, which is what the default gamestate looks like here.
		m_oldGamestate = ::gamestate;
		::gamestate = GS_STARTUP;
		::sv_gametype.ForceSet(GM_DM);

		SpreeManager::getInstance().reset();
		SpreeManager::getInstance().loadSpreeDefaults();
	}

	void TearDown() override
	{
		::sv_gametype.ForceSet(GM_COOP);
		::gamestate = m_oldGamestate;
		SpreeManager::getInstance().reset();
		SpreeManager::getInstance().loadSpreeDefaults();
		::players.clear();
	}

	static int Points(const int playerId)
	{
		return SpreeManager::getInstance().getPoints(playerId);
	}

	static player_t& Player(const int playerId)
	{
		return idplayer(static_cast<byte>(playerId));
	}

	// Actors are left to leak - tearing a DObject down needs a level's worth of state
	// that these tests do not stand up.
	static AActor* ActorFor(const int playerId)
	{
		auto* actor = new AActor();
		actor->player = &Player(playerId);
		return actor;
	}

	// Gives the player a body, which the friendly monster check needs.
	static void GiveBody(const int playerId)
	{
		Player(playerId).mo = ActorFor(playerId)->ptr();
	}

	static AActor* Monster(const bool friendly)
	{
		auto* monster = new AActor();

		if (friendly)
			monster->flags |= MF_FRIEND;
		else
			monster->flags &= ~MF_FRIEND;

		return monster;
	}

	gamestate_t m_oldGamestate = GS_LEVEL;
};

TEST_F(SpreeScoringTest, KillingAnOpponentCountsTowardsASpree)
{
	P_ProcessSpreeKill(ActorFor(WATCHED), &Player(OTHER));

	EXPECT_EQ(Points(WATCHED), 1);
}

TEST_F(SpreeScoringTest, KillingATeammateDoesNotCountTowardsASpree)
{
	::sv_gametype.ForceSet(GM_TEAMDM);
	Player(WATCHED).userinfo.team = TEAM_BLUE;
	Player(OTHER).userinfo.team = TEAM_BLUE;

	P_ProcessSpreeKill(ActorFor(WATCHED), &Player(OTHER));

	EXPECT_EQ(Points(WATCHED), 0);
}

TEST_F(SpreeScoringTest, KillingAnOpponentStillCountsInTeamGames)
{
	::sv_gametype.ForceSet(GM_TEAMDM);
	Player(WATCHED).userinfo.team = TEAM_BLUE;
	Player(OTHER).userinfo.team = TEAM_RED;

	P_ProcessSpreeKill(ActorFor(WATCHED), &Player(OTHER));

	EXPECT_EQ(Points(WATCHED), 1);
}

TEST_F(SpreeScoringTest, KillingYourselfDoesNotCountTowardsASpree)
{
	P_ProcessSpreeKill(ActorFor(WATCHED), &Player(WATCHED));

	EXPECT_EQ(Points(WATCHED), 0);
}

TEST_F(SpreeScoringTest, DyingWipesTheVictimsPoints)
{
	P_ProcessSpreeKill(ActorFor(OTHER), &Player(WATCHED));
	P_ProcessSpreeKill(ActorFor(OTHER), &Player(WATCHED));
	EXPECT_EQ(Points(OTHER), 2);

	P_ProcessSpreeKill(ActorFor(WATCHED), &Player(OTHER));

	EXPECT_EQ(Points(OTHER), 0);
}

TEST_F(SpreeScoringTest, KillsDoNotCountInCoop)
{
	::sv_gametype.ForceSet(GM_COOP);

	P_ProcessSpreeKill(ActorFor(WATCHED), &Player(OTHER));

	EXPECT_EQ(Points(WATCHED), 0);
}

TEST_F(SpreeScoringTest, MonsterDamageCountsInCoop)
{
	::sv_gametype.ForceSet(GM_COOP);
	GiveBody(WATCHED);

	P_ProcessSpreeDamage(&Player(WATCHED), Monster(false), 500);

	EXPECT_EQ(Points(WATCHED), 500);
}

TEST_F(SpreeScoringTest, FriendlyMonsterDamageDoesNotCountInCoop)
{
	// Coop shares friendlies between every player, so chipping at one is never worth
	// spree points.
	::sv_gametype.ForceSet(GM_COOP);
	GiveBody(WATCHED);

	P_ProcessSpreeDamage(&Player(WATCHED), Monster(true), 500);

	EXPECT_EQ(Points(WATCHED), 0);
}

TEST_F(SpreeScoringTest, FriendlyDamageIsRecognisedInCoop)
{
	// The same test gates the damage score, so it is worth pinning directly.
	::sv_gametype.ForceSet(GM_COOP);
	GiveBody(WATCHED);

	EXPECT_TRUE(P_IsFriendlyDamage(&Player(WATCHED), Monster(true)));
	EXPECT_FALSE(P_IsFriendlyDamage(&Player(WATCHED), Monster(false)));
}

TEST_F(SpreeScoringTest, FriendlyDamageNeedsBothAThingAndABody)
{
	::sv_gametype.ForceSet(GM_COOP);

	// No body yet, so there is nobody to be friendly with.
	EXPECT_FALSE(P_IsFriendlyDamage(&Player(WATCHED), Monster(true)));

	GiveBody(WATCHED);
	EXPECT_FALSE(P_IsFriendlyDamage(&Player(WATCHED), nullptr));
	EXPECT_FALSE(P_IsFriendlyDamage(nullptr, Monster(true)));
}

TEST_F(SpreeScoringTest, FriendliesAreNotSharedOutsideCoop)
{
	// Outside coop a friendly only belongs to the player who spawned it, so someone
	// else's friendly is fair game.
	::sv_gametype.ForceSet(GM_DM);
	GiveBody(WATCHED);

	AActor* someoneElsesFriendly = Monster(true);
	someoneElsesFriendly->friend_playerid = OTHER;

	EXPECT_FALSE(P_IsFriendlyDamage(&Player(WATCHED), someoneElsesFriendly));
}

TEST_F(SpreeScoringTest, DamageDoesNotCountOutsideCoop)
{
	GiveBody(WATCHED);

	P_ProcessSpreeDamage(&Player(WATCHED), Monster(false), 500);

	EXPECT_EQ(Points(WATCHED), 0);
}

// ==========================================================
// Name colors in spree and breaker messages.
// ==========================================================

class SpreeColorTest : public ::testing::Test
{
  protected:
	void SetUp() override
	{
		InitTeamInfo();

		::players.clear();
		AddPlayer(WATCHED, "Watched");
		AddPlayer(OTHER, "Other");
		::players.front().userinfo.team = TEAM_BLUE;
		::players.back().userinfo.team = TEAM_RED;

		::gametic = 10000;

		// sv_gametype is latched, and latched cvars are deferred while a level is in
		// progress, which is what the default gamestate looks like here.
		m_oldGamestate = ::gamestate;
		::gamestate = GS_STARTUP;
		::sv_gametype.ForceSet(GM_TEAMDM);

		SpreeManager::getInstance().reset();
		SpreeManager::getInstance().loadSpreeDefaults();
	}

	void TearDown() override
	{
		::sv_gametype.ForceSet(GM_COOP);
		::gamestate = m_oldGamestate;
		SpreeManager::getInstance().reset();
		SpreeManager::getInstance().loadSpreeDefaults();
		::players.clear();
	}

	gamestate_t m_oldGamestate = GS_LEVEL;

	static std::string TeamColor(const team_t team)
	{
		return GetTeamInfo(team)->ToastColor;
	}

	// A name as it appears in a message once its color has been applied.
	static std::string Colored(const std::string& color, const std::string& name)
	{
		return color + name + TEXTCOLOR_NORMAL;
	}

	static std::string BreakerText()
	{
		return SpreeManager::getInstance().getSpreeBreaker().spreeEndedBroadcastText;
	}

	// Raises playerId's spree to level 0 the way real kills would.
	static void EarnSpree(const int playerId)
	{
		const player_t& player = idplayer(static_cast<byte>(playerId));

		for (int i = 0; i < 5; i++)
			SpreeManager::getInstance().recordPlayerKill(&player);
	}

	static void RawBreaker(const SpreeBreakerType type, const int enderId,
	                       const std::string& enderName)
	{
		SpreeBreaker_t breaker;
		breaker.spreeEndedPlayerId = WATCHED;
		breaker.spreeEndedName = "Watched";
		breaker.spreeEnderPlayerId = enderId;
		breaker.spreeEnderName = enderName;

		SpreeManager::getInstance().setRawSpreeBreaker(breaker, 0, type, 0);
	}
};

TEST_F(SpreeColorTest, BreakerColorsBothPlayersByTeam)
{
	RawBreaker(BR_PLAYER, OTHER, "Other");

	const std::string text = BreakerText();

	EXPECT_NE(text.find(Colored(TeamColor(TEAM_BLUE), "Watched")), std::string::npos);
	EXPECT_NE(text.find(Colored(TeamColor(TEAM_RED), "Other")), std::string::npos);
}

TEST_F(SpreeColorTest, BreakerNeverLeavesTheEnderUncolored)
{
	// The ender used to fall through as TEAM_NONE and render gray.
	RawBreaker(BR_PLAYER, OTHER, "Other");

	const std::string text = BreakerText();

	EXPECT_EQ(text.find(Colored(TeamColor(TEAM_NONE), "Other")), std::string::npos);
}

TEST_F(SpreeColorTest, SelfKillBreakerColorsTheVictimInTheEnderSlot)
{
	// A self kill puts the victim in the killer slot, so their own team color has to
	// follow them there.
	RawBreaker(BR_SELF, WATCHED, "Watched");

	const std::string text = BreakerText();

	EXPECT_NE(text.find(Colored(TeamColor(TEAM_BLUE), "Watched")), std::string::npos);
	EXPECT_EQ(text.find(Colored(TeamColor(TEAM_NONE), "Watched")), std::string::npos);
}

TEST_F(SpreeColorTest, SelfKillBreakerColorsTheVictimServerSide)
{
	// setSpreeBreaker with no source is the server's self kill path.
	EarnSpree(WATCHED);
	SpreeManager::getInstance().setSpreeBreaker(nullptr, &idplayer(WATCHED));

	const std::string text = BreakerText();

	EXPECT_NE(text.find(Colored(TeamColor(TEAM_BLUE), "Watched")), std::string::npos);
	EXPECT_EQ(text.find(Colored(TeamColor(TEAM_NONE), "Watched")), std::string::npos);
}

TEST_F(SpreeColorTest, PlayerBreakerColorsTheEnderServerSide)
{
	EarnSpree(WATCHED);

	auto* killer = new AActor();
	killer->player = &idplayer(static_cast<byte>(OTHER));

	SpreeManager::getInstance().setSpreeBreaker(killer,
	                                            &idplayer(static_cast<byte>(WATCHED)));

	const std::string text = BreakerText();

	EXPECT_NE(text.find(Colored(TeamColor(TEAM_BLUE), "Watched")), std::string::npos);
	EXPECT_NE(text.find(Colored(TeamColor(TEAM_RED), "Other")), std::string::npos);
	EXPECT_EQ(text.find(Colored(TeamColor(TEAM_NONE), "Other")), std::string::npos);
}

TEST_F(SpreeColorTest, MonsterBreakerColorsTheVictimButNotTheMonster)
{
	RawBreaker(BR_MONSTER, -1, "Cyberdemon");

	const std::string text = BreakerText();

	EXPECT_NE(text.find(Colored(TeamColor(TEAM_BLUE), "Watched")), std::string::npos);
	EXPECT_EQ(text.find(Colored(TeamColor(TEAM_BLUE), "Cyberdemon")), std::string::npos);
	EXPECT_EQ(text.find(Colored(TeamColor(TEAM_RED), "Cyberdemon")), std::string::npos);
}

TEST_F(SpreeColorTest, SpreeMessageColorsThePlayerByTeam)
{
	SpreeManager::getInstance().setRawSpree(OTHER, 0, 0);

	const std::string text =
	    SpreeManager::getInstance().getSpreeRecord(OTHER).spree.spreeBroadcastText;

	EXPECT_NE(text.find(Colored(TeamColor(TEAM_RED), "Other")), std::string::npos);
}

TEST_F(SpreeColorTest, EachTeamGetsItsOwnColor)
{
	SpreeManager::getInstance().setRawSpree(WATCHED, 0, 0);
	SpreeManager::getInstance().setRawSpree(OTHER, 0, 0);

	const std::string blue =
	    SpreeManager::getInstance().getSpreeRecord(WATCHED).spree.spreeBroadcastText;
	const std::string red =
	    SpreeManager::getInstance().getSpreeRecord(OTHER).spree.spreeBroadcastText;

	EXPECT_NE(blue.find(Colored(TeamColor(TEAM_BLUE), "Watched")), std::string::npos);
	EXPECT_NE(red.find(Colored(TeamColor(TEAM_RED), "Other")), std::string::npos);
	EXPECT_NE(TeamColor(TEAM_BLUE), TeamColor(TEAM_RED));
}

TEST_F(SpreeColorTest, NonTeamGamesUseGoldForEveryone)
{
	::sv_gametype.ForceSet(GM_DM);

	SpreeManager::getInstance().setRawSpree(OTHER, 0, 0);
	RawBreaker(BR_PLAYER, OTHER, "Other");

	const std::string spree =
	    SpreeManager::getInstance().getSpreeRecord(OTHER).spree.spreeBroadcastText;
	const std::string breaker = BreakerText();

	EXPECT_NE(spree.find(Colored(TEXTCOLOR_GOLD, "Other")), std::string::npos);
	EXPECT_NE(breaker.find(Colored(TEXTCOLOR_GOLD, "Watched")), std::string::npos);
	EXPECT_NE(breaker.find(Colored(TEXTCOLOR_GOLD, "Other")), std::string::npos);
}

} // namespace
