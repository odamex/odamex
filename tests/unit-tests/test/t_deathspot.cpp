#include "gtest/gtest.h"

#include "odamex.h"

#include "g_deathspot.h"

namespace
{

struct DeathSpotFixture : testing::Test
{
	DeathSpotManager m_spots;
};

TEST_F(DeathSpotFixture, EmptyManagerHasNothing)
{
	EXPECT_FALSE(m_spots.hasDeathSpot(1));
}

TEST_F(DeathSpotFixture, RecordsAndReturnsASpot)
{
	m_spots.setDeathSpot(1, 64 * FRACUNIT, -128 * FRACUNIT, 32 * FRACUNIT, ANG90);

	EXPECT_TRUE(m_spots.hasDeathSpot(1));

	const DeathSpot_s& spot = m_spots.getDeathSpot(1);
	EXPECT_EQ(64 * FRACUNIT, spot.x);
	EXPECT_EQ(-128 * FRACUNIT, spot.y);
	EXPECT_EQ(32 * FRACUNIT, spot.z);
	EXPECT_EQ(ANG90, spot.angle);
}

TEST_F(DeathSpotFixture, PlayersAreIndependent)
{
	m_spots.setDeathSpot(1, 64 * FRACUNIT, 0, 0, 0);

	EXPECT_TRUE(m_spots.hasDeathSpot(1));
	EXPECT_FALSE(m_spots.hasDeathSpot(2));
}

TEST_F(DeathSpotFixture, DyingAgainReplacesTheSpot)
{
	m_spots.setDeathSpot(1, 64 * FRACUNIT, 0, 0, 0);
	m_spots.setDeathSpot(1, 96 * FRACUNIT, 0, 0, 0);

	EXPECT_EQ(96 * FRACUNIT, m_spots.getDeathSpot(1).x);
}

TEST_F(DeathSpotFixture, EraseAndClear)
{
	m_spots.setDeathSpot(1, 0, 0, 0, 0);
	m_spots.setDeathSpot(2, 0, 0, 0, 0);

	m_spots.eraseDeathSpot(1);
	EXPECT_FALSE(m_spots.hasDeathSpot(1));
	EXPECT_TRUE(m_spots.hasDeathSpot(2));

	// Erasing a player who has no spot is not an error.
	m_spots.eraseDeathSpot(1);
	EXPECT_FALSE(m_spots.hasDeathSpot(1));

	m_spots.clearDeathSpots();
	EXPECT_FALSE(m_spots.hasDeathSpot(2));
}

TEST_F(DeathSpotFixture, SpotIsKeptAtFullPrecision)
{
	const fixed_t x = 64 * FRACUNIT + (FRACUNIT / 2);
	const angle_t angle = ANG45 + 12345;

	m_spots.setDeathSpot(1, x, 0, 0, angle);

	EXPECT_EQ(x, m_spots.getDeathSpot(1).x);
	EXPECT_EQ(angle, m_spots.getDeathSpot(1).angle);
}

TEST_F(DeathSpotFixture, MissingPlayerReadsAsAnEmptySpot)
{
	const DeathSpot_s& spot = m_spots.getDeathSpot(99);

	EXPECT_EQ(0, spot.x);
	EXPECT_EQ(0, spot.y);
	EXPECT_EQ(0, spot.z);
	EXPECT_EQ(0u, spot.angle);
}

} // namespace
