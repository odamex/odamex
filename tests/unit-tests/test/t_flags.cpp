#include <gtest/gtest.h>
#include <stdint.h>
#include "flags.h"

enum class TestEnum : int8_t
{
	Flag1 = 0x1,
	Flag2 = 0x2,
	Flag3 = 0x4,
};

constexpr TestEnum enable_bitflag_operators(TestEnum) { return TestEnum::Flag3; }
using enum TestEnum;
using TestFlags = OFlags<TestEnum>;


void CheckFlags(TestFlags flags, bool oneSet, bool twoSet, bool threeSet)
{
	EXPECT_EQ(flags.is_set(Flag1), oneSet);
	EXPECT_EQ(flags & Flag1, oneSet);
	EXPECT_EQ(flags.is_set(Flag2), twoSet);
	EXPECT_EQ(flags & Flag2, twoSet);
	EXPECT_EQ(flags.is_set(Flag3), threeSet);
	EXPECT_EQ(flags & Flag3, threeSet);

	if (oneSet && twoSet && threeSet)
	{
		EXPECT_TRUE(flags.all());
		EXPECT_TRUE(flags.any());
		EXPECT_FALSE(flags.none());
		EXPECT_NE(flags, TestFlags{});
		EXPECT_NE(flags, OUtil::noflag);
		EXPECT_EQ(flags, TestFlags::all_set());
		EXPECT_NE(flags, TestFlags::none_set());
		EXPECT_TRUE(flags & (Flag1|Flag2|Flag3));
		EXPECT_EQ(flags.to_int(), 0b111);
	}

	if (oneSet || twoSet || threeSet)
	{
		EXPECT_TRUE(flags.any());
		EXPECT_FALSE(flags.none());
		EXPECT_NE(flags, TestFlags{});
		EXPECT_NE(flags, OUtil::noflag);
		EXPECT_NE(flags, TestFlags::none_set());
	}
	else
	{
		EXPECT_TRUE(flags.none());
		EXPECT_FALSE(flags.any());
		EXPECT_FALSE(flags.all());
		EXPECT_EQ(flags, TestFlags{});
		EXPECT_EQ(flags, OUtil::noflag);
		EXPECT_NE(flags, TestFlags::all_set());
		EXPECT_EQ(flags, TestFlags::none_set());
		EXPECT_EQ(flags.to_int(), 0);
	}

	if (oneSet && !twoSet && !threeSet)
	{
		EXPECT_EQ(flags, Flag1);
		EXPECT_EQ(flags, combo(~Flag2 & ~Flag3));
		EXPECT_TRUE(flags & Flag1);
		EXPECT_EQ(flags.to_int(), 0b001);
	}
	else if (!oneSet && twoSet && !threeSet)
	{
		EXPECT_EQ(flags, Flag2);
		EXPECT_EQ(flags, combo(~Flag1 & ~Flag3));
		EXPECT_TRUE(flags & Flag2);
		EXPECT_EQ(flags.to_int(), 0b010);
	}
	else if (!oneSet && !twoSet && threeSet)
	{
		EXPECT_EQ(flags, Flag3);
		EXPECT_EQ(flags, combo(~Flag1 & ~Flag2));
		EXPECT_TRUE(flags & Flag3);
		EXPECT_EQ(flags.to_int(), 0b100);
	}
	else if (oneSet && twoSet && !threeSet)
	{
		EXPECT_EQ(flags, Flag1|Flag2);
		EXPECT_EQ(flags, combo(~Flag3));
		EXPECT_TRUE(flags & (Flag1|Flag2));
		EXPECT_EQ(flags.to_int(), 0b011);
	}
	else if (!oneSet && twoSet && threeSet)
	{
		EXPECT_EQ(flags, Flag2|Flag3);
		EXPECT_EQ(flags, combo(~Flag1));
		EXPECT_TRUE(flags & (Flag2|Flag3));
		EXPECT_EQ(flags.to_int(), 0b110);
	}
	else if (oneSet && !twoSet && threeSet)
	{
		EXPECT_EQ(flags, Flag1|Flag3);
		EXPECT_EQ(flags, combo(~Flag2));
		EXPECT_TRUE(flags & (Flag1|Flag3));
		EXPECT_EQ(flags.to_int(), 0b101);
	}
}

TEST(OFlags, Constructors)
{
	CheckFlags(TestFlags{}, false, false, false);
	CheckFlags(TestFlags::none_set(), false, false, false);
	CheckFlags(TestFlags::all_set(), true, true, true);
	CheckFlags(TestFlags{Flag1}, true, false, false);
	CheckFlags(TestFlags{Flag1|Flag3}, true, false, true);
	CheckFlags(TestFlags{OUtil::noflag}, false, false, false);
	CheckFlags(TestFlags::unsafe_from_int(static_cast<int8_t>(0b000)), false, false, false);
	CheckFlags(TestFlags::unsafe_from_int(static_cast<int8_t>(0b001)), true, false, false);
	CheckFlags(TestFlags::unsafe_from_int(static_cast<int8_t>(0b010)), false, true, false);
	CheckFlags(TestFlags::unsafe_from_int(static_cast<int8_t>(0b011)), true, true, false);
	CheckFlags(TestFlags::unsafe_from_int(static_cast<int8_t>(0b100)), false, false, true);
	CheckFlags(TestFlags::unsafe_from_int(static_cast<int8_t>(0b101)), true, false, true);
	CheckFlags(TestFlags::unsafe_from_int(static_cast<int8_t>(0b110)), false, true, true);
	CheckFlags(TestFlags::unsafe_from_int(static_cast<int8_t>(0b111)), true, true, true);
}

TEST(OFlags, SetClear)
{
	// non-const
	TestFlags flags;
	flags.set(Flag1);
	CheckFlags(flags, true, false, false);
	flags.set(Flag2, true);
	CheckFlags(flags, true, true, false);
	flags.set(Flag1, false);
	flags.set(Flag3, true);
	CheckFlags(flags, false, true, true);
	flags.clear();
	CheckFlags(flags, false, false, false);
	flags = TestFlags::all_set();
	flags.clear(Flag2);
	CheckFlags(flags, true, false, true);

	// const
	const TestFlags flags2;
	CheckFlags(flags2.set(Flag1), true, false, false);
	CheckFlags(flags2.set(Flag2, true), false, true, false);
	CheckFlags(flags2.set(Flag2).set(Flag2, false), false, false, false);
	const TestFlags flags3 = TestFlags::all_set();
	CheckFlags(flags3.clear(), false, false, false);
	CheckFlags(flags3.clear(Flag2), true, false, true);
}

TEST(OFlags, Toggle)
{
	// non-const
	TestFlags flags;
	flags.toggle(Flag1);
	CheckFlags(flags, true, false, false);
	flags.toggle(Flag1);
	CheckFlags(flags, false, false, false);
	flags.toggle();
	CheckFlags(flags, true, true, true);

	// const
	const TestFlags flags2;
	CheckFlags(flags2.toggle(Flag1), true, false, false);
	CheckFlags(flags2.toggle(), true, true, true);
	CheckFlags(flags2.toggle(Flag2).toggle(), true, false, true);
}

TEST(OFlags, Operators)
{
	TestFlags flags;
	flags |= Flag1;
	CheckFlags(flags, true, false, false);
	flags = ~flags;
	CheckFlags(flags, false, true, true);
	flags &= ~(Flag2|Flag3);
	CheckFlags(flags, false, false, false);
	flags = flags | Flag2;
	CheckFlags(flags, false, true, false);
	flags ^= Flag2 | Flag3;
	CheckFlags(flags, false, false, true);
	flags |= ~mask(Flag3);
	CheckFlags(flags, true, true, true);
	// Only have 3 flags so gonna have a duplicate in this expression
	// NOLINTNEXTLINE(misc-redundant-expression)
	flags &= mask((Flag1 | Flag2) | (Flag2 | Flag3)) & ~Flag2;
	CheckFlags(flags, true, false, true);
	flags = flags & ~Flag1;
	CheckFlags(flags, false, false, true);
	flags = flags ^ (Flag1 | Flag3);
	CheckFlags(flags, true, false, false);
	TestFlags flags2;
	flags2 |= Flag3;
	flags |= combo(flags2);
	CheckFlags(flags, true, false, true);
	flags &= mask(flags2);
	CheckFlags(flags, false, false, true);
}
