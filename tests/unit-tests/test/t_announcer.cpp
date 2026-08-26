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
//   Tests for ONCRINFO parsing, the first blood announcement and the per-round
//   announcer reset.
//
//-----------------------------------------------------------------------------

#include "odamex.h"

#include <gtest/gtest.h>

#include "actor.h"
#include "cmdlib.h"
#include "g_announcer.h"
#include "g_gametype.h"
#include "g_levelstate.h"
#include "g_oncrinfo.h"
#include "i_system.h"
#include "s_sound.h"

EXTERN_CVAR(sv_gametype)
EXTERN_CVAR(sv_maxplayers)

namespace
{

// Player IDs used throughout. KILLER is the one credited with the kill under test.
constexpr int KILLER = 1;
constexpr int OTHER = 2;
constexpr int THIRD = 3;

// The sndinfo token the default announcer maps ANN_FIRSTBLOOD to. The announcement is
// dropped before it ever reaches the queue if S_FindSound can't resolve this.
constexpr const char* FIRSTBLOOD_SOUND = "vox/firstblood";

// PlayerQuery only returns players that are ingame and not spectating, and every
// scoreboard the announcer reads comes from a PlayerQuery.
void AddPlayer(const int id)
{
	player_t& player = ::players.emplace_back();
	player.id = static_cast<byte>(id);
	player.playerstate = PST_LIVE;
	player.spectator = false;
	player.userinfo.team = TEAM_NONE;
}

player_t& Player(const int id)
{
	return idplayer(static_cast<byte>(id));
}

// Mock up a SNDINFO table.
void RegisterSounds(const std::vector<const char*>& names)
{
	for (const char* name : names)
	{
		sfxinfo_t& sfx = ::S_sfx.emplace_back();
		M_StringCopy(sfx.name, name, MAX_SNDNAME + 1);
		sfx.data = nullptr;
		sfx.link = sfxinfo_t::NO_LINK;
		sfx.lumpnum = -1;
		sfx.ms = 1000;
	}

	S_HashSounds();
}

void SetLatchedCvar(cvar_t& cvar, const float value)
{
	cvar.Set(value);
	cvar_t::UnlatchCVars();
}

void SetLevelState(const LevelState::States state)
{
	SerializedLevelState sls = {};
	sls.state = state;
	::levelstate.unserialize(sls);
}

class AnnouncerTest : public ::testing::Test
{
  protected:
	void SetUp() override
	{
		m_oldClientside = ::clientside;
		m_oldMultiplayer = ::multiplayer;
		::clientside = true;
		::multiplayer = true;

		::players.clear();
		AddPlayer(KILLER);
		AddPlayer(OTHER);
		AddPlayer(THIRD);

		::level.time = 1;
		SetLevelState(LevelState::INGAME);

		// Non-duel deathmatch, which is the only place first blood plays.
		m_oldGametype = sv_gametype.value();
		m_oldMaxPlayers = sv_maxplayers.value();
		SetLatchedCvar(sv_gametype, GM_DM);
		SetLatchedCvar(sv_maxplayers, 4.0f);

		S_ClearSoundLumps();
		RegisterSounds({FIRSTBLOOD_SOUND, "vox/fight", "vox/threefragsleft",
		                "vox/youhavethelead"});

		AnnouncerManager::getInstance().reset();
		AnnouncerManager::getInstance().loadAnnouncerDefaults();
		AnnouncerManager::getInstance().resetAnnouncements();
	}

	void TearDown() override
	{
		AnnouncerManager::getInstance().reset();
		S_ClearSoundLumps();
		::players.clear();
		SetLatchedCvar(sv_gametype, m_oldGametype);
		SetLatchedCvar(sv_maxplayers, m_oldMaxPlayers);
		::clientside = m_oldClientside;
		::multiplayer = m_oldMultiplayer;
	}

	static AnnouncerManager& Announcer() { return AnnouncerManager::getInstance(); }

  private:
	bool m_oldClientside = false;
	bool m_oldMultiplayer = false;
	float m_oldGametype = 0.0f;
	float m_oldMaxPlayers = 0.0f;
};

// ==========================================================
// First blood.
// ==========================================================

TEST_F(AnnouncerTest, AnnouncesWhenTheKillDrawsFirstBlood)
{
	Player(KILLER).fragcount = 1;

	P_CheckFirstBloodAnnouncement(Player(KILLER));

	EXPECT_TRUE(Announcer().isAnnouncing());
	EXPECT_TRUE(Announcer().hasFirstBloodBeenAnnounced());
}

TEST_F(AnnouncerTest, StaysQuietUntilSomebodyScores)
{
	P_CheckFirstBloodAnnouncement(Player(KILLER));

	EXPECT_FALSE(Announcer().isAnnouncing());

	// Nothing was drawn yet, so the announcement must still be available.
	EXPECT_FALSE(Announcer().hasFirstBloodBeenAnnounced());

	Player(KILLER).fragcount = 1;
	P_CheckFirstBloodAnnouncement(Player(KILLER));

	EXPECT_TRUE(Announcer().isAnnouncing());
}

// Joining a server mid-game, or rewinding a netdemo, drops us into a game whose first
// blood was drawn while we weren't watching.
// Make sure a suicide and refrag doesn't rearm first blood.
TEST_F(AnnouncerTest, StaysQuietWhenTheLoneFragBelongsToSomebodyElse)
{
	Player(OTHER).fragcount = 1;
	Player(KILLER).fragcount = -1; // Suicided.

	P_CheckFirstBloodAnnouncement(Player(KILLER));

	EXPECT_FALSE(Announcer().isAnnouncing());

	// We know blood was drawn, so nothing later should announce it either.
	EXPECT_TRUE(Announcer().hasFirstBloodBeenAnnounced());
}

TEST_F(AnnouncerTest, StaysQuietWhenTheGameIsAlreadyPastFirstBlood)
{
	Player(KILLER).fragcount = 2;

	P_CheckFirstBloodAnnouncement(Player(KILLER));

	EXPECT_FALSE(Announcer().isAnnouncing());
	EXPECT_TRUE(Announcer().hasFirstBloodBeenAnnounced());
}

TEST_F(AnnouncerTest, StaysQuietWhenTwoPlayersShareTheFirstFrag)
{
	Player(KILLER).fragcount = 1;
	Player(OTHER).fragcount = 1;

	P_CheckFirstBloodAnnouncement(Player(KILLER));

	EXPECT_FALSE(Announcer().isAnnouncing());
	EXPECT_TRUE(Announcer().hasFirstBloodBeenAnnounced());
}

TEST_F(AnnouncerTest, StaysQuietWhenSomebodyElseIsAlreadyOnTheBoard)
{
	Player(KILLER).fragcount = 1;
	Player(THIRD).fragcount = 3;

	P_CheckFirstBloodAnnouncement(Player(KILLER));

	EXPECT_FALSE(Announcer().isAnnouncing());
	EXPECT_TRUE(Announcer().hasFirstBloodBeenAnnounced());
}

TEST_F(AnnouncerTest, AnnouncesFirstBloodOnlyOnce)
{
	Player(KILLER).fragcount = 1;
	P_CheckFirstBloodAnnouncement(Player(KILLER));
	ASSERT_TRUE(Announcer().isAnnouncing());

	Announcer().clearQueue();
	P_CheckFirstBloodAnnouncement(Player(KILLER));

	EXPECT_FALSE(Announcer().isAnnouncing());
}

TEST_F(AnnouncerTest, DoesNotAnnounceFirstBloodOutsideDeathmatch)
{
	SetLatchedCvar(sv_gametype, GM_COOP);
	Player(KILLER).fragcount = 1;

	P_CheckFirstBloodAnnouncement(Player(KILLER));

	EXPECT_FALSE(Announcer().isAnnouncing());
}

TEST_F(AnnouncerTest, DoesNotAnnounceFirstBloodInADuel)
{
	SetLatchedCvar(sv_maxplayers, 2.0f);
	Player(KILLER).fragcount = 1;

	P_CheckFirstBloodAnnouncement(Player(KILLER));

	EXPECT_FALSE(Announcer().isAnnouncing());
}

TEST_F(AnnouncerTest, DoesNotAnnounceFirstBloodOutsideActiveGameplay)
{
	SetLevelState(LevelState::WARMUP);
	Player(KILLER).fragcount = 1;

	P_CheckFirstBloodAnnouncement(Player(KILLER));

	EXPECT_FALSE(Announcer().isAnnouncing());
	EXPECT_FALSE(Announcer().hasFirstBloodBeenAnnounced());
}

// ==========================================================
// What a round reset has to wipe.
// ==========================================================

TEST_F(AnnouncerTest, ClearingRoundStateDropsTheAnnouncedFlags)
{
	Announcer().setFightAnnounced();
	Announcer().setCountdownAnnounced(3);
	Announcer().setFirstBloodAnnounced();
	Announcer().setDisplayPlayerHasLead(true);
	Announcer().setLeadTied(true);
	Announcer().announceFragWarning3();

	G_ClearRoundState();

	EXPECT_FALSE(Announcer().hasFightBeenAnnounced());
	EXPECT_FALSE(Announcer().hasCountdownBeenAnnounced(3));
	EXPECT_FALSE(Announcer().hasFirstBloodBeenAnnounced());
	EXPECT_FALSE(Announcer().doesDisplayPlayerHaveLead());
	EXPECT_FALSE(Announcer().isLeadTied());
	EXPECT_FALSE(Announcer().hasFragWarning3BeenAnnounced());
}

TEST_F(AnnouncerTest, ClearingRoundStateDropsQueuedSounds)
{
	Announcer().queueSound(FIRSTBLOOD_SOUND);
	ASSERT_TRUE(Announcer().isAnnouncing());

	G_ClearRoundState();

	EXPECT_FALSE(Announcer().isAnnouncing());
}

TEST_F(AnnouncerTest, ClearingRoundStateDropsPendingSpreesAndMultiKills)
{
	Announcer().setPendingSpree(5, FIRSTBLOOD_SOUND, "");
	Announcer().setPendingMultiKill(5, FIRSTBLOOD_SOUND, "");
	ASSERT_EQ(Announcer().getPendingSpreeLevel(), 5);
	ASSERT_EQ(Announcer().getPendingMultiKillLevel(), 5);

	G_ClearRoundState();

	EXPECT_EQ(Announcer().getPendingSpreeLevel(), -1);
	EXPECT_EQ(Announcer().getPendingMultiKillLevel(), -1);

	// A lower level than the stale one has to be accepted again.
	Announcer().setPendingSpree(1, FIRSTBLOOD_SOUND, "");
	EXPECT_EQ(Announcer().getPendingSpreeLevel(), 1);
}

TEST_F(AnnouncerTest, FlushingPendingSoundsLeavesNothingBehind)
{
	Announcer().setPendingSpree(2, FIRSTBLOOD_SOUND, "");
	Announcer().setPendingMultiKill(2, FIRSTBLOOD_SOUND, "");

	Announcer().flushPendingSounds();

	EXPECT_EQ(Announcer().getPendingSpreeLevel(), -1);
	EXPECT_EQ(Announcer().getPendingMultiKillLevel(), -1);
}

// ==========================================================
// ONCRINFO parsing.
// ==========================================================

class OncrInfoTest : public ::testing::Test
{
  protected:
	void TearDown() override
	{
		AnnouncerManager::getInstance().reset();
		AnnouncerManager::getInstance().loadAnnouncerDefaults();
	}

	static void Parse(const std::string& text)
	{
		AnnouncerManager::getInstance().reset();
		G_ParseOncrInfoBuffer(text.data(), text.length());
	}

	static std::string Pack(const std::string& name, const std::string& body)
	{
		return "{\nname = \"" + name + "\"\n" + body + "}\n";
	}

	static AnnouncerManager& Announcer() { return AnnouncerManager::getInstance(); }

	static std::string TokenFor(const std::string& announcer, const std::string& event)
	{
		AnnouncerManager::getInstance().loadAnnouncerByName(announcer);
		return AnnouncerManager::getInstance().getTokenForEvent(event);
	}
};

TEST_F(OncrInfoTest, LoadsAPackWithNothingButAName)
{
	Parse(Pack("Bare", ""));

	EXPECT_TRUE(Announcer().isAnnouncerLoaded("Bare"));
}

TEST_F(OncrInfoTest, RejectsAPackWithoutAName)
{
	EXPECT_THROW(Parse("{\ndescription = \"No name here\"\n}\n"), CRecoverableError);
}

TEST_F(OncrInfoTest, KeepsTheMetadataFields)
{
	Parse(Pack("Metadata", "description = \"A test announcer\"\n"
	                       "author = \"Somebody\"\n"));

	const AnnouncerMetaData_s& metadata = Announcer().getAnnouncerMetadata("Metadata");

	EXPECT_EQ(metadata.name, "Metadata");
	EXPECT_EQ(metadata.description, "A test announcer");
	EXPECT_EQ(metadata.author, "Somebody");
}

TEST_F(OncrInfoTest, MapsNamedTokensToTheirSounds)
{
	Parse(Pack("Named", "firstblood = vox/blood\n"
	                    "youwin = vox/win\n"));

	EXPECT_EQ(TokenFor("Named", std::string(ANN_FIRSTBLOOD)), "vox/blood");
	EXPECT_EQ(TokenFor("Named", std::string(ANN_YOUWIN)), "vox/win");
}

TEST_F(OncrInfoTest, TreatsTokenNamesAsCaseInsensitive)
{
	Parse(Pack("Shouty", "FIRSTBLOOD = vox/blood\n"));

	EXPECT_EQ(TokenFor("Shouty", std::string(ANN_FIRSTBLOOD)), "vox/blood");
}

TEST_F(OncrInfoTest, MapsSpreeAndMultiKillLevels)
{
	Parse(Pack("Levels", "spree 3 = vox/spree/three\n"
	                     "multi 2 = vox/multi/double\n"));

	EXPECT_EQ(TokenFor("Levels", "spree 3"), "vox/spree/three");
	EXPECT_EQ(TokenFor("Levels", "multi 2"), "vox/multi/double");
}

TEST_F(OncrInfoTest, IgnoresAnUnknownTokenAndKeepsParsing)
{
	Parse(Pack("Resilient", "notarealtoken = vox/nope\n"
	                        "firstblood = vox/blood\n"));

	EXPECT_TRUE(Announcer().isAnnouncerLoaded("Resilient"));
	EXPECT_EQ(TokenFor("Resilient", std::string(ANN_FIRSTBLOOD)), "vox/blood");
}

TEST_F(OncrInfoTest, LoadsSeveralPacksFromOneLump)
{
	Parse(Pack("First", "firstblood = vox/one\n") +
	      Pack("Second", "firstblood = vox/two\n"));

	EXPECT_TRUE(Announcer().isAnnouncerLoaded("First"));
	EXPECT_TRUE(Announcer().isAnnouncerLoaded("Second"));
	EXPECT_EQ(TokenFor("First", std::string(ANN_FIRSTBLOOD)), "vox/one");
	EXPECT_EQ(TokenFor("Second", std::string(ANN_FIRSTBLOOD)), "vox/two");
}

// A second block under the same name tops the first up rather than replacing it.
TEST_F(OncrInfoTest, MergesPacksThatShareAName)
{
	Parse(Pack("Merged", "firstblood = vox/blood\n") +
	      Pack("Merged", "youwin = vox/win\n"));

	EXPECT_EQ(TokenFor("Merged", std::string(ANN_FIRSTBLOOD)), "vox/blood");
	EXPECT_EQ(TokenFor("Merged", std::string(ANN_YOUWIN)), "vox/win");
}

TEST_F(OncrInfoTest, LaterBlocksOverrideAnEarlierSound)
{
	Parse(Pack("Merged", "firstblood = vox/old\n") +
	      Pack("Merged", "firstblood = vox/new\n"));

	EXPECT_EQ(TokenFor("Merged", std::string(ANN_FIRSTBLOOD)), "vox/new");
}

TEST_F(OncrInfoTest, SkipsCComments)
{
	Parse(Pack("Commented", "// firstblood = vox/commentedout\n"
	                        "firstblood = vox/blood\n"));

	EXPECT_EQ(TokenFor("Commented", std::string(ANN_FIRSTBLOOD)), "vox/blood");
}

TEST_F(OncrInfoTest, ReturnsNothingForAnEventThePackDoesNotDefine)
{
	Parse(Pack("Sparse", "firstblood = vox/blood\n"));

	EXPECT_EQ(TokenFor("Sparse", std::string(ANN_YOUWIN)), "");
}

} // namespace
