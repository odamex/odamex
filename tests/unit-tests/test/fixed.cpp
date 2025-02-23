#include "gtest/gtest.h"
#include "odamex.h"
#include "m_fixed.h"

// TODO: tests for overflows, we want to catch things like when some of the 64 bit stuff had accidental conversions to 32 bit

TEST(FixedMath, IntConversion) {
	EXPECT_EQ(FIXED2INT(INT2FIXED(0)), 0);
	EXPECT_EQ(FIXED2INT(INT2FIXED(1)), 1);
	EXPECT_EQ(FIXED2INT(INT2FIXED(2)), 2);
	EXPECT_EQ(FIXED2INT(INT2FIXED(10)), 10);
	EXPECT_EQ(FIXED2INT(INT2FIXED(1234)), 1234);
	EXPECT_EQ(FIXED2INT(INT2FIXED(32767)), 32767);
}

// these are all off by one, is this correct or not?
TEST(FixedMath, NegativeIntConversion) {
	EXPECT_EQ(FIXED2INT(INT2FIXED(-1)), -1);
	EXPECT_EQ(FIXED2INT(INT2FIXED(-2)), -2);
	EXPECT_EQ(FIXED2INT(INT2FIXED(-10)), -10);
	EXPECT_EQ(FIXED2INT(INT2FIXED(-1234)), -1234);
	EXPECT_EQ(FIXED2INT(INT2FIXED(-32768)), -32768);
}