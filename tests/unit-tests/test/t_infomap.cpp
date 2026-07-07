#include "gtest/gtest.h"
#include "odamex.h"
#include "infomap.h"
#include "d_main.h"

class InfoMapTest : public ::testing::Test {
protected:
	void SetUp() override {
		D_InitializeDoomObjectTables();
	}
};

TEST_F(InfoMapTest, NameToMobj) {
	EXPECT_EQ(P_NameToMobj("Revenant"), MT_UNDEAD);
	EXPECT_EQ(P_NameToMobj("revenant"), MT_NULL);
}

TEST_F(InfoMapTest, INameToMobj) {
	EXPECT_EQ(P_INameToMobj("Revenant"), MT_UNDEAD);
	EXPECT_EQ(P_INameToMobj("revenant"), MT_UNDEAD);
}

TEST_F(InfoMapTest, MobjToName) {
	EXPECT_EQ(P_MobjToName(MT_UNDEAD), std::string("Revenant"));
}

TEST_F(InfoMapTest, NameToWeapon) {
	EXPECT_EQ(P_NameToWeapon("PlasmaRifle"), wp_plasma);
	EXPECT_EQ(P_NameToWeapon("plasmarifle"), wp_none);
}