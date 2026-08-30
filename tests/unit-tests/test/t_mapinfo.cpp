#include "gtest/gtest.h"
#include "odamex.h"

#include "g_level.h"
#include "g_mapinfo.h"
#include "p_lnspec.h"

namespace
{

class MapInfoParse : public ::testing::Test
{
  protected:
	void SetUp() override { reset(); }
	void TearDown() override { reset(); }

	static void reset()
	{
		getLevelInfos().clear();
		getClusterInfos().clear();
		::HexenHack = false;
	}

	// Drives the real parser over an in-memory MAPINFO and hands back the level it
	// defined.
	static level_pwad_info_t& parse(const std::string& text,
	                                const char* mapname = "MAP01")
	{
		G_ParseMapInfoString(text, "TESTINFO");
		return getLevelInfos().findByName(mapname);
	}
};

TEST_F(MapInfoParse, IntKeysLandInTheRightFields)
{
	const level_pwad_info_t& info =
	    parse("map MAP01 \"Test\" { levelnum = 42 par = 30 airsupply = 7 }");

	EXPECT_TRUE(info.exists());
	EXPECT_EQ(info.level_name, "Test");
	EXPECT_EQ(info.levelnum, 42);
	EXPECT_EQ(info.partime, 30);
	EXPECT_EQ(info.airsupply, 7);
}

TEST_F(MapInfoParse, FloatKeysLandInTheRightFields)
{
	const level_pwad_info_t& info =
	    parse("map MAP01 \"Test\" { gravity = 400.0 aircontrol = 0.75 }");

	EXPECT_FLOAT_EQ(info.gravity, 400.0f);
	EXPECT_FLOAT_EQ(info.aircontrol, 0.75f);
}

TEST_F(MapInfoParse, MapNameKeys)
{
	const level_pwad_info_t& info =
	    parse("map MAP01 \"Test\" { next = MAP02 secretnext = MAP31 }");

	EXPECT_EQ(info.nextmap, "MAP02");
	EXPECT_EQ(info.secretmap, "MAP31");
}

TEST_F(MapInfoParse, NumericNextMapBecomesMapName)
{
	const level_pwad_info_t& info = parse("map MAP01 \"Test\" { next = 5 }");

	EXPECT_EQ(info.nextmap, "MAP05");
}

TEST_F(MapInfoParse, SkyKeysAreSeparateSlots)
{
	const level_pwad_info_t& info =
	    parse("map MAP01 \"Test\" { sky1 = SKY1, 2.0 sky2 = SKY2 }");

	EXPECT_EQ(info.skypic, "SKY1");
	EXPECT_EQ(info.skypic2, "SKY2");
	EXPECT_EQ(info.sky1ScrollDelta, FLOAT2FIXED(2.0f));
}

TEST_F(MapInfoParse, SetFlagKeysSetTheFirstFlagWord)
{
	const level_pwad_info_t& info =
	    parse("map MAP01 \"Test\" { nointermission doublesky evenlighting }");

	EXPECT_TRUE(info.flags.is_set(LEVEL_NOINTERMISSION));
	EXPECT_TRUE(info.flags.is_set(LEVEL_DOUBLESKY));
	EXPECT_TRUE(info.flags.is_set(LEVEL_EVENLIGHTING));
	EXPECT_FALSE(info.flags.is_set(LEVEL_MONSTERSTELEFRAG));
}

TEST_F(MapInfoParse, SCFlagKeysSupersedeTheirCounterpart)
{
	const level_pwad_info_t& jump = parse("map MAP01 \"Test\" { allowjump nojump }");
	EXPECT_TRUE(jump.flags.is_set(LEVEL_JUMP_NO));
	EXPECT_FALSE(jump.flags.is_set(LEVEL_JUMP_YES));

	reset();

	const level_pwad_info_t& look =
	    parse("map MAP01 \"Test\" { nofreelook allowfreelook }");
	EXPECT_TRUE(look.flags.is_set(LEVEL_FREELOOK_YES));
	EXPECT_FALSE(look.flags.is_set(LEVEL_FREELOOK_NO));
}

TEST_F(MapInfoParse, CompatCrossdropoffUsesTheSecondFlagWord)
{
	const level_pwad_info_t& info = parse("map MAP01 \"Test\" { compat_crossdropoff }");

	EXPECT_TRUE(info.flags2.is_set(LEVEL2_COMPAT_CROSSDROPOFF));
	EXPECT_FALSE(info.flags.is_set(LEVEL_COMPAT_DROPOFF));
}

TEST_F(MapInfoParse, CompatFlagAcceptsAnExplicitValue)
{
	const level_pwad_info_t& on = parse("map MAP01 \"Test\" { compat_shorttex = 1 }");
	EXPECT_TRUE(on.flags.is_set(LEVEL_COMPAT_SHORTTEX));

	reset();

	const level_pwad_info_t& off = parse("map MAP01 \"Test\" { compat_shorttex = 0 }");
	EXPECT_FALSE(off.flags.is_set(LEVEL_COMPAT_SHORTTEX));
}

// The infighting keys share a mask in the second flag word.
TEST_F(MapInfoParse, InfightingKeysSupersedeEachOther)
{
	const level_pwad_info_t& info =
	    parse("map MAP01 \"Test\" { noinfighting totalinfighting }");

	EXPECT_TRUE(info.flags2.is_set(LEVEL2_TOTALINFIGHTING));
	EXPECT_FALSE(info.flags2.is_set(LEVEL2_NOINFIGHTING));
}

TEST_F(MapInfoParse, TitlePatchCanHideTheAuthorName)
{
	const level_pwad_info_t& plain = parse("map MAP01 \"Test\" { titlepatch = CWILV00 }");
	EXPECT_EQ(plain.pname, "CWILV00");
	EXPECT_FALSE(plain.flags2.is_set(LEVEL2_HIDEAUTHORNAME));

	reset();

	const level_pwad_info_t& hidden =
	    parse("map MAP01 \"Test\" { titlepatch = CWILV00, hideauthorname }");
	EXPECT_EQ(hidden.pname, "CWILV00");
	EXPECT_TRUE(hidden.flags2.is_set(LEVEL2_HIDEAUTHORNAME));
}

TEST_F(MapInfoParse, InterPicSplitsLumpFromScript)
{
	const level_pwad_info_t& info =
	    parse("map MAP01 \"Test\" { enterpic = INTERPIC exitpic = \"$SCRIPT\" }");

	EXPECT_EQ(info.enterpic, "INTERPIC");
	EXPECT_TRUE(info.enterscript.empty());
	EXPECT_EQ(info.exitscript, "SCRIPT");
	EXPECT_TRUE(info.exitpic.empty());
}

TEST_F(MapInfoParse, Map07SpecialAddsBothBossActions)
{
	const level_pwad_info_t& info = parse("map MAP01 \"Test\" { map07special }");

	ASSERT_EQ(info.bossactions.size(), 2u);

	// mancubus: lower the floor tagged 666
	EXPECT_EQ(info.bossactions[0].type, MT_FATSO);
	EXPECT_EQ(info.bossactions[0].special,
	          lineSpecialValue(doomLineSpecial_t::S1_Floor_LowerToLowest));
	EXPECT_EQ(info.bossactions[0].tag, BOSSACTION_TAG);

	// arachnotron: raise the floor tagged 667
	EXPECT_EQ(info.bossactions[1].type, MT_BABY);
	EXPECT_EQ(info.bossactions[1].special,
	          lineSpecialValue(doomLineSpecial_t::W1_Floor_RaiseByTexture));
	EXPECT_EQ(info.bossactions[1].tag, BOSSACTION_TAG_ALT);
}

TEST_F(MapInfoParse, SpecialActionKeysUseTheirDoomLineType)
{
	const level_pwad_info_t& exitlevel =
	    parse("map MAP01 \"Test\" { baronspecial specialaction_exitlevel }");
	ASSERT_EQ(exitlevel.bossactions.size(), 1u);
	EXPECT_EQ(exitlevel.bossactions[0].type, MT_BRUISER);
	EXPECT_EQ(exitlevel.bossactions[0].special,
	          lineSpecialValue(doomLineSpecial_t::S1_Exit_Normal));

	reset();

	const level_pwad_info_t& opendoor =
	    parse("map MAP01 \"Test\" { cyberdemonspecial specialaction_opendoor }");
	ASSERT_EQ(opendoor.bossactions.size(), 1u);
	EXPECT_EQ(opendoor.bossactions[0].type, MT_CYBORG);
	EXPECT_EQ(opendoor.bossactions[0].special,
	          lineSpecialValue(doomLineSpecial_t::W1_Door_OpenFast));
	EXPECT_EQ(opendoor.bossactions[0].tag, BOSSACTION_TAG);
}

// Old-style MAPINFO has no braces and no '=' signs.
TEST_F(MapInfoParse, OldStyleBlockParses)
{
	const level_pwad_info_t& info =
	    parse("map MAP01 \"Test\"\nlevelnum 42\npar 30\nnointermission\n");

	EXPECT_EQ(info.levelnum, 42);
	EXPECT_EQ(info.partime, 30);
	EXPECT_TRUE(info.flags.is_set(LEVEL_NOINTERMISSION));
}

// defaultmap seeds every map defined after it.
TEST_F(MapInfoParse, DefaultMapAppliesToLaterMaps)
{
	G_ParseMapInfoString("defaultmap { par = 90 nointermission }\n"
	                     "map MAP01 \"One\" { levelnum = 1 }\n"
	                     "map MAP02 \"Two\" { levelnum = 2 }\n",
	                     "TESTINFO");

	const level_pwad_info_t& one = getLevelInfos().findByName("MAP01");
	const level_pwad_info_t& two = getLevelInfos().findByName("MAP02");

	EXPECT_EQ(one.partime, 90);
	EXPECT_EQ(two.partime, 90);
	EXPECT_TRUE(one.flags.is_set(LEVEL_NOINTERMISSION));
	EXPECT_TRUE(two.flags.is_set(LEVEL_NOINTERMISSION));
	EXPECT_EQ(one.levelnum, 1);
	EXPECT_EQ(two.levelnum, 2);
}

TEST_F(MapInfoParse, ClusterBlockKeys)
{
	G_ParseMapInfoString("cluster 1 { entertext = \"Entering\" exittext = \"Leaving\" "
	                     "hub }",
	                     "TESTINFO");

	const cluster_info_t& cluster = getClusterInfos().findByCluster(1);

	EXPECT_TRUE(cluster.exists());
	EXPECT_EQ(cluster.entertext, "Entering");
	EXPECT_EQ(cluster.exittext, "Leaving");
	EXPECT_TRUE(cluster.flags & CLUSTER_HUB);
}

TEST_F(MapInfoParse, MapKeyPointsAtItsCluster)
{
	const level_pwad_info_t& info = parse("map MAP01 \"Test\" { cluster = 3 }");

	EXPECT_EQ(info.cluster, 3);
}

// Unknown keys inside a new-style block are skipped rather than aborting the parse.
TEST_F(MapInfoParse, UnknownKeyDoesNotDerailTheBlock)
{
	const level_pwad_info_t& info =
	    parse("map MAP01 \"Test\" { levelnum = 42 someunknownkey = 3 par = 30 }");

	EXPECT_EQ(info.levelnum, 42);
	EXPECT_EQ(info.partime, 30);
}

TEST(MapInfoHelpers, StripAuthorPrefix)
{
	EXPECT_EQ(G_StripAuthorPrefix("by: Some Person"), "Some Person");
	EXPECT_EQ(G_StripAuthorPrefix("Author: Some Person"), "Some Person");
	EXPECT_EQ(G_StripAuthorPrefix("Some Person"), "Some Person");

	// Stripping that would leave nothing keeps the original.
	EXPECT_EQ(G_StripAuthorPrefix("by: "), "by: ");
}

TEST(MapInfoHelpers, MapNameToLevelNum)
{
	level_pwad_info_t info{};

	info.mapname = "MAP07";
	G_MapNameToLevelNum(info);
	EXPECT_EQ(info.levelnum, 7);

	info = level_pwad_info_t{};
	info.mapname = "E2M4";
	G_MapNameToLevelNum(info);
	EXPECT_EQ(info.levelnum, 14);

	// Anything else leaves the number alone.
	info = level_pwad_info_t{};
	info.levelnum = 99;
	info.mapname = "TITLEMAP";
	G_MapNameToLevelNum(info);
	EXPECT_EQ(info.levelnum, 99);
}

} // namespace
