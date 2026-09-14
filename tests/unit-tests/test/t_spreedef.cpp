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
//   Tests for SPREEDEF parsing.
//
//-----------------------------------------------------------------------------

#include "odamex.h"

#include <gtest/gtest.h>

#include "g_spree.h"
#include "g_multikill.h"
#include "g_spreedef.h"
#include "i_system.h"

namespace
{

// The defaults give six spree levels, so anything past level 5 is a repeat.
constexpr int TOP_SPREE_LEVEL = 5;

constexpr int WATCHED = 1;

void AddPlayer(const int id, const std::string& name)
{
	player_t& player = ::players.emplace_back();
	player.id = static_cast<byte>(id);
	player.userinfo.netname = name;
}

class SpreeDefTest : public ::testing::Test
{
  protected:
	void TearDown() override
	{
		SpreeManager::getInstance().reset();
		SpreeManager::getInstance().loadSpreeDefaults();
		MultiKillManager::getInstance().reset();
		MultiKillManager::getInstance().loadMultiKillDefaults();
	}

	static void Parse(const std::string& text)
	{
		G_ParseSpreeDefBuffer(text.data(), text.length());
	}

	// A SPREEDEF with all the required keywords, so tests only vary in the level blocks.
	static std::string WithLevels(const std::string& levels)
	{
		return "spreekillinterval = 5\n"
		       "spreedamageinterval = 5000\n"
		       "multikillinterval = 4\n"
		       "spreeendedplayertext = \"%o's %s was ended by %k\"\n"
		       "spreeendedselftext = \"%k killed %hself\"\n"
		       "spreeendedmonstertext = \"%o's %s was ended by a %k\"\n"
		       "repeatingspreetext = \"%k is STILL %s!\"\n" +
		       levels;
	}

	static std::string Spree(const int level)
	{
		return "spree " + std::to_string(level) +
		       " = { color = white text = \"Spree\" broadcasttext = \"%k!\" }\n";
	}

	static std::string Multi(const int level)
	{
		return "multi " + std::to_string(level) + " = { color = white text = \"Kill\" }\n";
	}
};

TEST_F(SpreeDefTest, ParsesLevelsInOrder)
{
	EXPECT_NO_THROW(Parse(WithLevels(Spree(1) + Spree(2) + Spree(3) + Multi(2) +
	                                 Multi(3) + Multi(4))));
}

TEST_F(SpreeDefTest, SpreeLevelsMustStartAtOne)
{
	EXPECT_THROW(Parse(WithLevels(Spree(2))), CRecoverableError);
	EXPECT_THROW(Parse(WithLevels(Spree(0))), CRecoverableError);
}

TEST_F(SpreeDefTest, SpreeLevelsMustNotSkip)
{
	EXPECT_THROW(Parse(WithLevels(Spree(1) + Spree(3))), CRecoverableError);
}

TEST_F(SpreeDefTest, SpreeLevelsMustNotRepeatOrGoBackwards)
{
	EXPECT_THROW(Parse(WithLevels(Spree(1) + Spree(1))), CRecoverableError);
	EXPECT_THROW(Parse(WithLevels(Spree(1) + Spree(2) + Spree(1))), CRecoverableError);
}

TEST_F(SpreeDefTest, MultiLevelsMustStartAtTwo)
{
	EXPECT_THROW(Parse(WithLevels(Spree(1) + Multi(1))), CRecoverableError);

	// The old check let a lump start at 3, which silently shifted every level down one.
	EXPECT_THROW(Parse(WithLevels(Spree(1) + Multi(3))), CRecoverableError);
}

TEST_F(SpreeDefTest, MultiLevelsMustNotSkip)
{
	EXPECT_THROW(Parse(WithLevels(Spree(1) + Multi(2) + Multi(4))), CRecoverableError);
}

TEST_F(SpreeDefTest, MultiLevelsMustNotRepeatOrGoBackwards)
{
	EXPECT_THROW(Parse(WithLevels(Spree(1) + Multi(2) + Multi(2))), CRecoverableError);
	EXPECT_THROW(Parse(WithLevels(Spree(1) + Multi(3) + Multi(2))), CRecoverableError);
}

TEST_F(SpreeDefTest, RequiredKeywordsAreEnforced)
{
	// Every required keyword except repeatingspreetext.
	const std::string missingRepeat =
	    "spreekillinterval = 5\n"
	    "spreedamageinterval = 5000\n"
	    "spreeendedplayertext = \"a\"\n"
	    "spreeendedselftext = \"b\"\n"
	    "spreeendedmonstertext = \"c\"\n" +
	    Spree(1);

	EXPECT_THROW(Parse(missingRepeat), CRecoverableError);
}

TEST_F(SpreeDefTest, UnknownTokensError)
{
	EXPECT_THROW(Parse("notakeyword = 5\n"), CRecoverableError);
}

TEST_F(SpreeDefTest, EmptyLumpFallsBackToDefaults)
{
	EXPECT_NO_THROW(Parse(""));

	// The defaults are six spree levels, so level 5 is the last real one.
	SpreeManager::getInstance().setRawSpree(WATCHED, TOP_SPREE_LEVEL, 0);
	EXPECT_FALSE(SpreeManager::getInstance().getSpreeRecord(WATCHED).stillDominating);
}

TEST_F(SpreeDefTest, ParsedLevelCountDrivesStillDominating)
{
	Parse(WithLevels(Spree(1) + Spree(2)));

	::players.clear();
	AddPlayer(WATCHED, "Watched");
	::gametic = 10000;

	// Two levels means index 1 is the top, so index 2 is a repeat.
	SpreeManager::getInstance().setRawSpree(WATCHED, 1, 0);
	EXPECT_FALSE(SpreeManager::getInstance().getSpreeRecord(WATCHED).stillDominating);

	SpreeManager::getInstance().setRawSpree(WATCHED, 2, 0);
	EXPECT_TRUE(SpreeManager::getInstance().getSpreeRecord(WATCHED).stillDominating);

	::players.clear();
}


} // namespace
