#include "gtest/gtest.h"

#include "MovingAverage.h"

template <typename SampleType, size_t POWER_OF_TWO_SAMPLES>
struct MovingTestFixture : testing::Test
{
    MovingAverage<SampleType, POWER_OF_TWO_SAMPLES> average;
};

using ThirtyTwoInts = MovingTestFixture<int, 5>;

TEST_F(ThirtyTwoInts, NullOptUntilFull)
{
    for (int i = 1; i < 32; ++i)
    {
        auto result = average.Update(i);
        EXPECT_EQ(result, std::nullopt);
    }

    auto value = average.Update(32);

    EXPECT_EQ(value.has_value(), true);
    EXPECT_EQ(*value,            16);
}

TEST_F(ThirtyTwoInts, AllOnes)
{
    for (int i = 1; i < 32; ++i)
    {
        EXPECT_EQ(average.Update(1), std::nullopt);
    }
    EXPECT_EQ(average.Update(1), 1);
}

TEST_F(ThirtyTwoInts, AllTwos)
{
    for (int i = 1; i < 32; ++i)
    {
        EXPECT_EQ(average.Update(2), std::nullopt);
    }
    EXPECT_EQ(average.Update(2), 2);
}

TEST_F(ThirtyTwoInts, ConvergeTowardsThirtyTwo)
{
    // Put the averager through a rough start towards a constant value.
    for (int i = 1; i < 32; ++i)
    {
        EXPECT_EQ(average.Update(i), std::nullopt);
    }
    EXPECT_EQ(average.Update(32), 16);                   // Right between 0 and 32.

    EXPECT_EQ(average.Update(32), 17);
    EXPECT_EQ(average.Update(32), 17);
    EXPECT_EQ(average.Update(32), 17);
    EXPECT_EQ(average.Update(32), 18);
    EXPECT_EQ(average.Update(32), 18);
    EXPECT_EQ(average.Update(32), 19);
    EXPECT_EQ(average.Update(32), 19);
    EXPECT_EQ(average.Update(32), 20);
    EXPECT_EQ(average.Update(32), 20);
    EXPECT_EQ(average.Update(32), 20);
    EXPECT_EQ(average.Update(32), 21);
    EXPECT_EQ(average.Update(32), 21);
    EXPECT_EQ(average.Update(32), 21);
    EXPECT_EQ(average.Update(32), 22);
    EXPECT_EQ(average.Update(32), 22);
    EXPECT_EQ(average.Update(32), 22);
    EXPECT_EQ(average.Update(32), 23);
    EXPECT_EQ(average.Update(32), 23);
    EXPECT_EQ(average.Update(32), 23);
    EXPECT_EQ(average.Update(32), 24);
    EXPECT_EQ(average.Update(32), 24);
    EXPECT_EQ(average.Update(32), 24);
    EXPECT_EQ(average.Update(32), 24);
    EXPECT_EQ(average.Update(32), 25);
    EXPECT_EQ(average.Update(32), 25);
    EXPECT_EQ(average.Update(32), 25);
    EXPECT_EQ(average.Update(32), 25);
    EXPECT_EQ(average.Update(32), 25);
    EXPECT_EQ(average.Update(32), 26);
    EXPECT_EQ(average.Update(32), 26);
    EXPECT_EQ(average.Update(32), 26);
    EXPECT_EQ(average.Update(32), 26);

    EXPECT_EQ(average.Update(32), 26);
    EXPECT_EQ(average.Update(32), 27);
    EXPECT_EQ(average.Update(32), 27);
    EXPECT_EQ(average.Update(32), 27);
    EXPECT_EQ(average.Update(32), 27);
    EXPECT_EQ(average.Update(32), 27);
    EXPECT_EQ(average.Update(32), 27);
    EXPECT_EQ(average.Update(32), 28);
    EXPECT_EQ(average.Update(32), 28);
    EXPECT_EQ(average.Update(32), 28);
    EXPECT_EQ(average.Update(32), 28);
    EXPECT_EQ(average.Update(32), 28);
    EXPECT_EQ(average.Update(32), 28);
    EXPECT_EQ(average.Update(32), 28);
    EXPECT_EQ(average.Update(32), 28);
    EXPECT_EQ(average.Update(32), 29);
    EXPECT_EQ(average.Update(32), 29);
    EXPECT_EQ(average.Update(32), 29);
    EXPECT_EQ(average.Update(32), 29);
    EXPECT_EQ(average.Update(32), 29);
    EXPECT_EQ(average.Update(32), 29);
    EXPECT_EQ(average.Update(32), 29);
    EXPECT_EQ(average.Update(32), 29);
    EXPECT_EQ(average.Update(32), 29);
    EXPECT_EQ(average.Update(32), 29);
    EXPECT_EQ(average.Update(32), 29);
    EXPECT_EQ(average.Update(32), 30);
    EXPECT_EQ(average.Update(32), 30);
    EXPECT_EQ(average.Update(32), 30);
    EXPECT_EQ(average.Update(32), 30);
    EXPECT_EQ(average.Update(32), 30);
    EXPECT_EQ(average.Update(32), 30);

    EXPECT_EQ(average.Update(32), 30);
    EXPECT_EQ(average.Update(32), 30);
    EXPECT_EQ(average.Update(32), 30);
    EXPECT_EQ(average.Update(32), 30);
    EXPECT_EQ(average.Update(32), 30);
    EXPECT_EQ(average.Update(32), 30);
    EXPECT_EQ(average.Update(32), 30);
    EXPECT_EQ(average.Update(32), 30);
    EXPECT_EQ(average.Update(32), 30);
    EXPECT_EQ(average.Update(32), 30);
    EXPECT_EQ(average.Update(32), 31);
    EXPECT_EQ(average.Update(32), 31);
    EXPECT_EQ(average.Update(32), 31);
    EXPECT_EQ(average.Update(32), 31);
    EXPECT_EQ(average.Update(32), 31);
    EXPECT_EQ(average.Update(32), 31);
    EXPECT_EQ(average.Update(32), 31);
    EXPECT_EQ(average.Update(32), 31);
    EXPECT_EQ(average.Update(32), 31);
    EXPECT_EQ(average.Update(32), 31);
    EXPECT_EQ(average.Update(32), 31);
    EXPECT_EQ(average.Update(32), 31);
    EXPECT_EQ(average.Update(32), 31);
    EXPECT_EQ(average.Update(32), 31);
    EXPECT_EQ(average.Update(32), 31);
    EXPECT_EQ(average.Update(32), 31);
    EXPECT_EQ(average.Update(32), 31);
    EXPECT_EQ(average.Update(32), 31);
    EXPECT_EQ(average.Update(32), 31);
    EXPECT_EQ(average.Update(32), 31);
    EXPECT_EQ(average.Update(32), 31);
    EXPECT_EQ(average.Update(32), 31);
    EXPECT_EQ(average.Update(32), 31);
    EXPECT_EQ(average.Update(32), 31);
    EXPECT_EQ(average.Update(32), 31);
    EXPECT_EQ(average.Update(32), 31);
    EXPECT_EQ(average.Update(32), 31);
    EXPECT_EQ(average.Update(32), 31);
    EXPECT_EQ(average.Update(32), 31);
    EXPECT_EQ(average.Update(32), 31);
    EXPECT_EQ(average.Update(32), 31);
    EXPECT_EQ(average.Update(32), 32);
    EXPECT_EQ(average.Update(32), 32);
    EXPECT_EQ(average.Update(32), 32);
    EXPECT_EQ(average.Update(32), 32);
    EXPECT_EQ(average.Update(32), 32);
    EXPECT_EQ(average.Update(32), 32);
    EXPECT_EQ(average.Update(32), 32);
    EXPECT_EQ(average.Update(32), 32);
    EXPECT_EQ(average.Update(32), 32);
    EXPECT_EQ(average.Update(32), 32);
    EXPECT_EQ(average.Update(32), 32);
    EXPECT_EQ(average.Update(32), 32);
    EXPECT_EQ(average.Update(32), 32);
    EXPECT_EQ(average.Update(32), 32);
    EXPECT_EQ(average.Update(32), 32);
    EXPECT_EQ(average.Update(32), 32);
    EXPECT_EQ(average.Update(32), 32);
    EXPECT_EQ(average.Update(32), 32);
    EXPECT_EQ(average.Update(32), 32);
    EXPECT_EQ(average.Update(32), 32);
    EXPECT_EQ(average.Update(32), 32);
    EXPECT_EQ(average.Update(32), 32);
    EXPECT_EQ(average.Update(32), 32);
}

TEST_F(ThirtyTwoInts, PseudoRandom)
{
    // This test is meant to reflect someone with a realistic amount of "severe" jitter.
    average.Update(2);
    average.Update(4);
    average.Update(2);
    average.Update(5);
    average.Update(5);
    average.Update(3);
    average.Update(5);
    average.Update(2);
    average.Update(4);
    average.Update(4);
    average.Update(2);
    average.Update(5);
    average.Update(2);
    average.Update(4);
    average.Update(3);
    average.Update(4);
    average.Update(5);
    average.Update(2);
    average.Update(2);
    average.Update(2);
    average.Update(5);
    average.Update(2);
    average.Update(2);
    average.Update(2);
    average.Update(4);
    average.Update(4);
    average.Update(5);
    average.Update(5);
    average.Update(2);
    average.Update(4);
    average.Update(4);
    EXPECT_EQ(average.Update(3), 3);
}
