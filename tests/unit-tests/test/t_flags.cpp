#include <gtest/gtest.h>
#include <stdint.h>
#include "flags.h"

enum class TestEnum : int8_t
{
	Flag1 = 0x1,
	Flag2 = 0x2,
	Flag3 = 0x4,
	Flag4 = 0x8,
};

consteval TestEnum enable_bitflag_operators(TestEnum) { return TestEnum::Flag4; }

TEST(OFlags, CheckEnumSet)
{
	using enum TestEnum;
	using TestFlags = OFlags<TestEnum>;
	TestFlags flags;
	EXPECT_EQ(flags, TestFlags::none_set());
	EXPECT_FALSE(flags.is_set(Flag1));
	EXPECT_FALSE(flags & Flag1);
	EXPECT_FALSE(flags.is_set(Flag2));
	EXPECT_FALSE(flags & Flag2);
	EXPECT_FALSE(flags.is_set(Flag3));
	EXPECT_FALSE(flags & Flag3);
	EXPECT_FALSE(flags.is_set(Flag4));
	EXPECT_FALSE(flags & Flag4);
	flags = TestFlags::all_set();
	EXPECT_TRUE(flags.is_set(Flag1));
	EXPECT_TRUE(flags & Flag1);
	EXPECT_TRUE(flags.is_set(Flag2));
	EXPECT_TRUE(flags & Flag2);
	EXPECT_TRUE(flags.is_set(Flag3));
	EXPECT_TRUE(flags & Flag3);
	EXPECT_TRUE(flags.is_set(Flag4));
	EXPECT_TRUE(flags & Flag4);
}

TEST(OFlags, CheckComboSet)
{
	using enum TestEnum;
	using TestFlags = OFlags<TestEnum>;
	TestFlags flags;
	EXPECT_FALSE(flags & (Flag1 | Flag2));
	EXPECT_FALSE(flags & (Flag3 | Flag4));
	flags = TestFlags::all_set();
	EXPECT_TRUE(flags & (Flag1 | Flag2));
	EXPECT_TRUE(flags & (Flag3 | Flag4));
	flags.clear();
	flags |= Flag1;
	flags |= Flag3;
	EXPECT_TRUE(flags & (Flag1 | Flag2));
	EXPECT_TRUE(flags & (Flag3 | Flag4));
	EXPECT_FALSE(flags & (Flag2 | Flag4));
}
