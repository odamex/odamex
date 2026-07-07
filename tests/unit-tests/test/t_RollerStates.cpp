#include "gtest/gtest.h"

#include <functional>

#include "odamex.h"

#include "PlayerStateRoller.h"

constexpr double TIME_STEP_MSEC = 1000.0 / static_cast<double>(TICRATE);

struct PseudoMessage
{
    int tic;
    PlayerItemDataType msg;
};

struct LatencyFixture
{
    int currentTic;
    int pingTimeInMsec;
    int roundTripTimeInTics;

    std::unordered_map<int, PseudoMessage, std::identity> messagesInFlight;

    PlayerStateRoller   rollerState;
    player_t            clientPlayer;
    bool                correctionWasRequired;

    explicit LatencyFixture(int pingTimeInMsec) :
        currentTic            (0),
        roundTripTimeInTics   (static_cast<int>(std::ceil(static_cast<double>(pingTimeInMsec) / TIME_STEP_MSEC)) + 1),  // +1 because of the server frame.
        messagesInFlight      (roundTripTimeInTics * 2),     // double bucket count for safety margin.
        correctionWasRequired (false)
    {
    }

    void Frame()
    {
        rollerState.Record(currentTic, clientPlayer);
        ++currentTic;
    }

    // Returns true if a "server response" was "received" at the start of this new tic.
    bool FrameAndProcessInventoryResponse()
    {
        messagesInFlight.erase(currentTic);

        Frame();

        auto serverResponseIter = messagesInFlight.find(currentTic);
        if (serverResponseIter != messagesInFlight.end())
        {
            const RollerResolveEnum result = rollerState.Resolve(serverResponseIter->second.tic, serverResponseIter->second.msg, clientPlayer);
            correctionWasRequired = (result == RollerResolveEnum::HISTORY_CHANGED);
            return true;
        }
        correctionWasRequired = false;
        return false;
    }

    PlayerItemDataType& FutureServerResponse()
    {
        const int futureTic = currentTic + roundTripTimeInTics;
        auto iter = messagesInFlight.find(futureTic);
        if (iter == messagesInFlight.end())
        {
            auto insertResult = messagesInFlight.insert( {futureTic, {currentTic, {}}});
            return insertResult.first->second.msg;
        }
        return iter->second.msg;
    }

    void SetHappyResponse()
    {
        auto& futureMessage = FutureServerResponse();
        futureMessage = PlayerItemDataType(clientPlayer);
    }

    template <typename Callable>
    bool DoPickupOnTic(int oldTic, Callable&& pickup)
    {
        auto optionalHistory = rollerState.GetStateAtTic(oldTic);
        const PlayerItemDataType currentClientSideState {clientPlayer};

        if (optionalHistory)
        {
            optionalHistory->get().ToPlayer(clientPlayer);
        }

        pickup(clientPlayer);

        if (optionalHistory)
        {
            const PlayerItemDataType modifiedHistory {clientPlayer};

            currentClientSideState.ToPlayer(clientPlayer);

            rollerState.Resolve(oldTic, modifiedHistory, clientPlayer);
            return true;
        }
        return false;
    }

};

struct PistolStartLatencyFixture : LatencyFixture, testing::TestWithParam<int>
{
    PistolStartLatencyFixture() :
        LatencyFixture(GetParam())
    {
        clientPlayer.weaponowned[wp_pistol] = true;
        clientPlayer.ammo       [am_clip]   =  50;
        clientPlayer.maxammo    [am_clip]   = 200;
    }
};

struct PistolStartLatencySingleShotSuite : PistolStartLatencyFixture
{
};

// Single happy shot
// -----------------
TEST_P(PistolStartLatencySingleShotSuite, HappyPistolStartAndShoot)
{
    // This is baby's first check.  Start with a basic sanity check.
    EXPECT_GT(roundTripTimeInTics, 0);

    EXPECT_EQ(clientPlayer.ammo[am_clip], 50);

    clientPlayer.ammo[am_clip] -= 1;
    SetHappyResponse();

    for (int i = 0; i < roundTripTimeInTics - 1; ++i)
    {
        const bool newMessageReceived = FrameAndProcessInventoryResponse();

        EXPECT_EQ(false, newMessageReceived);
    }

    EXPECT_EQ(true,  FrameAndProcessInventoryResponse());
    EXPECT_EQ(false, correctionWasRequired);
}

INSTANTIATE_TEST_SUITE_P(VariousHappyPistolStartsAndShots,
                         PistolStartLatencySingleShotSuite,
                         testing::Range(10, 300, 10));

// Multiple happy shots
// ---------------------
struct PistolStartLatencyMultiShotSuite : PistolStartLatencyFixture
{
};

TEST_P(PistolStartLatencyMultiShotSuite, HappyPistolStartAndShots)
{
    // This is baby's first check.  Start with a basic sanity check.
    EXPECT_GT(roundTripTimeInTics, 0);

    EXPECT_EQ(clientPlayer.ammo[am_clip], 50);

    clientPlayer.ammo[am_clip] -= 1;
    SetHappyResponse();

    for (int i = 0; i < roundTripTimeInTics - 1; ++i)
    {
        // Odd-numbered tic?  Pull trigger.
        if (i & 0x1)
        {
            clientPlayer.ammo[am_clip] -= 1;
        }

        const bool newMessageReceived = FrameAndProcessInventoryResponse();

        EXPECT_EQ(false, newMessageReceived);
    }

    EXPECT_EQ(true,  FrameAndProcessInventoryResponse());
    EXPECT_EQ(false, correctionWasRequired);
}

INSTANTIATE_TEST_SUITE_P(VariousHappyPistolStartsAndMultiShots,
                         PistolStartLatencyMultiShotSuite,
                         testing::Range(10, 300, 10));

// Multiple shots with desynched ammo pickup
// -----------------------------------------
struct PistolStartMultiShotGhostAmmoPickupSuite : PistolStartLatencyFixture
{
};

TEST_P(PistolStartMultiShotGhostAmmoPickupSuite, BasicTest)
{
    // Do some shooting.
    clientPlayer.ammo[am_clip] = 40;

    // Locally mispredict a pickup.
    //
    clientPlayer.ammo[am_clip] = 90;

    // Pretend that we send a generic notification that stimulates the server to respond to us in one full round-trip period.
    // Only, that response is going to indicate that we DID NOT pickup the ammo!
    {
        auto& msgRef = FutureServerResponse();
        msgRef.ammo[am_clip] = 40;
    }

    EXPECT_GT(roundTripTimeInTics, 1);
    for (int i = 0; i < roundTripTimeInTics - 1; ++i)
    {
        EXPECT_EQ(false, FrameAndProcessInventoryResponse());
        clientPlayer.ammo[am_clip] -= 1;        // keep shootin'
    }

    EXPECT_EQ(90 - (roundTripTimeInTics - 1), clientPlayer.ammo[am_clip]);

    EXPECT_EQ(true, FrameAndProcessInventoryResponse());
    EXPECT_EQ(true, correctionWasRequired);

    EXPECT_EQ(40 - (roundTripTimeInTics - 1), clientPlayer.ammo[am_clip]);        // Ammo corrected??
};

INSTANTIATE_TEST_SUITE_P(GhostAmmoPickup,
                         PistolStartMultiShotGhostAmmoPickupSuite,
                         testing::Range(10, 300, 10));

// Pickup weapon and ammo desync
// -----------------------------------------
struct PickupWeaponAndAmmoSuite : PistolStartLatencyFixture
{
};

TEST_P(PickupWeaponAndAmmoSuite, BasicTest)
{
    // Starting point:  player's running around, locally predicts a pickup, sends a pickup notice, sets state.
    //
    SetHappyResponse();     // Ensures the server response has the pre-pickup state.
    clientPlayer.weaponowned[wp_missile] = true;
    clientPlayer.ammo       [am_misl]    = 2;

    EXPECT_GT(roundTripTimeInTics, 1);
    for (int i = 0; i < roundTripTimeInTics - 1; ++i)
    {
        EXPECT_EQ(false, FrameAndProcessInventoryResponse());
    }

    EXPECT_EQ(true, clientPlayer.weaponowned[wp_missile]);
    EXPECT_EQ(2,    clientPlayer.ammo[am_misl]);

    EXPECT_EQ(true, FrameAndProcessInventoryResponse());
    EXPECT_EQ(true, correctionWasRequired);

    EXPECT_EQ(false, clientPlayer.weaponowned[wp_missile]);
    EXPECT_EQ(0,     clientPlayer.ammo[am_misl]);
}

TEST_P(PickupWeaponAndAmmoSuite, TestWithInterimFire)
{
    // Starting point:  player's running around, locally predicts a pickup, sends a pickup notice, sets state.
    //
    SetHappyResponse();     // Ensures the server response has the pre-pickup state.
    clientPlayer.weaponowned[wp_missile] = true;
    clientPlayer.ammo       [am_misl]    = 2;

    EXPECT_GT(roundTripTimeInTics, 1);
    for (int i = 0; i < roundTripTimeInTics - 1; ++i)
    {
        if (i == roundTripTimeInTics - 2)
        {
            clientPlayer.ammo[am_misl] -= 1;        // Fire!
        }

        EXPECT_EQ(false, FrameAndProcessInventoryResponse());
    }

    EXPECT_EQ(true, clientPlayer.weaponowned[wp_missile]);
    EXPECT_EQ(1,    clientPlayer.ammo[am_misl]);

    EXPECT_EQ(true, FrameAndProcessInventoryResponse());
    EXPECT_EQ(true, correctionWasRequired);

    EXPECT_EQ(false, clientPlayer.weaponowned[wp_missile]);
    EXPECT_EQ(0,     clientPlayer.ammo[am_misl]);
}

INSTANTIATE_TEST_SUITE_P(WeaponDesync,
                         PickupWeaponAndAmmoSuite,
                         testing::Range(10, 300, 10));

// ======================================================================================
// Regression tests
//

struct PistolStartRegressionFixture : LatencyFixture, testing::TestWithParam<int>
{
    PistolStartRegressionFixture() :
        LatencyFixture(GetParam())
    {
        PlayerItemDataType basicStart;

        basicStart.weaponowned[wp_none]   = true;
        basicStart.weaponowned[wp_fist]   = true;
        basicStart.weaponowned[wp_pistol] = true;
        basicStart.ammo       [am_clip]   =  50;
        basicStart.maxammo    [am_clip]   = 200;

        basicStart.health   = 100;
        basicStart.lives    = 1;
        basicStart.readyweapon   = wp_pistol;
        basicStart.pendingweapon = wp_nochange;

        basicStart.psprites[ps_weapon].statenum = S_PISTOL;
        basicStart.psprites[ps_weapon].tics     = 1;

        basicStart.ToPlayer(clientPlayer);
    }
};

TEST_P(PistolStartRegressionFixture, HealthPickupDisappear)
{
    EXPECT_EQ(8, roundTripTimeInTics);      // 200 msec ping, lots of tics in the latency pipeline

    clientPlayer.health = 87;

    Frame();
    Frame();
    Frame();
    Frame();
    Frame();
    Frame();
    Frame();
    Frame();

    // Message came in from server that we picked up a stimpack 5 tics ago.

    DoPickupOnTic(currentTic - 5, [](auto& player)
    {
        player.health += 10;
    });

    EXPECT_EQ(97, clientPlayer.health);

    // 2 tics later, an ammo pickup.

    Frame();
    Frame();

    DoPickupOnTic(currentTic - 5, [](auto& player)
    {
        player.ammo[am_clip] += 5;
    });

    // Verify that health did not change.
    EXPECT_EQ(97, clientPlayer.health);
}

TEST_P(PistolStartRegressionFixture, ArmorPickupDisappear)
{
    EXPECT_EQ(8, roundTripTimeInTics);      // 200 msec ping, lots of tics in the latency pipeline

    clientPlayer.armorpoints = 0;

    Frame();
    Frame();
    Frame();
    Frame();
    Frame();
    Frame();
    Frame();
    Frame();

    // Message came in from server that we picked up armor 5 tics ago.

    DoPickupOnTic(currentTic - 5, [](auto& player)
    {
        player.armorpoints  = 100;
        player.armortype    = 1;
    });

    EXPECT_EQ(100, clientPlayer.armorpoints);
    EXPECT_EQ(1,   clientPlayer.armortype);

    // 1 tic later, a little more armor.
    Frame();

    DoPickupOnTic(currentTic - 5, [](auto& player)
    {
        player.armorpoints += 1;
    });

    // 2 tics later, an ammo pickup.

    Frame();
    Frame();

    DoPickupOnTic(currentTic - 5, [](auto& player)
    {
        player.ammo[am_clip] += 5;
    });

    // Verify that armor did not change.
    EXPECT_EQ(101, clientPlayer.armorpoints);
    EXPECT_EQ(1,   clientPlayer.armortype);
}

TEST_P(PistolStartRegressionFixture, LivesVsPickup)
{
    EXPECT_EQ(8, roundTripTimeInTics);      // 200 msec ping, lots of tics in the latency pipeline

    clientPlayer.lives = 2;

    Frame();
    Frame();
    Frame();
    Frame();
    Frame();
    Frame();
    Frame();
    Frame();

    // Player killed, lose a life.

    DoPickupOnTic(currentTic - 5, [](auto& player)
    {
        player.lives -= 1;
    });

    EXPECT_EQ(1, clientPlayer.lives);

    // 2 tics later, an ammo pickup.

    Frame();
    Frame();

    DoPickupOnTic(currentTic - 5, [](auto& player)
    {
        player.ammo[am_clip] += 5;
    });

    // Verify that lives did not change.
    EXPECT_EQ(1, clientPlayer.lives);
}

TEST_P(PistolStartRegressionFixture, PredictedWeaponPickup)
{
    EXPECT_EQ(8, roundTripTimeInTics);      // 200 msec ping, lots of tics in the latency pipeline

    Frame();
    Frame();
    Frame();
    Frame();
    Frame();
    Frame();
    Frame();
    Frame();

    // Predicted pickup
    clientPlayer.weaponowned[wp_supershotgun] = true;

    // But before an inventory check can come in, an ammo pickup.
    Frame();
    Frame();

    DoPickupOnTic(currentTic - 5, [](auto& player)
    {
        player.ammo[am_clip] += 5;
    });

    // Verify that ssg did not change.
    EXPECT_EQ(true, clientPlayer.weaponowned[wp_supershotgun]);
}

TEST_P(PistolStartRegressionFixture, HordeWeaponPickup)
{
    EXPECT_EQ(8, roundTripTimeInTics);      // 200 msec ping, lots of tics in the latency pipeline

    Frame();
    Frame();
    Frame();
    Frame();
    Frame();
    Frame();
    Frame();
    Frame();

    // Big ol' horde pickup.
    DoPickupOnTic(currentTic - 5, [](auto& player)
    {
        player.weaponowned[wp_supershotgun] = true;
        player.ammo[am_clip]  += 50;
        player.ammo[am_shell] += 16;
    });

    // But right behind it is a single clip.
    Frame();
    Frame();

    DoPickupOnTic(currentTic - 5, [](auto& player)
    {
        player.ammo[am_clip] += 5;
    });

    // Verify that ssg did not change.
    EXPECT_EQ(true, clientPlayer.weaponowned[wp_supershotgun]);
    EXPECT_EQ(105,  clientPlayer.ammo[am_clip]);
}

INSTANTIATE_TEST_SUITE_P(PickupRollbackBug,
                         PistolStartRegressionFixture,
                         testing::Values(200));
