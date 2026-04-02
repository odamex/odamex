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
        rollerState           (currentTic),
        correctionWasRequired (false)
    {
    }

    // Returns true if a "server response" was "received" at the start of this new tic.
    bool Frame()
    {
        rollerState.Record(currentTic, clientPlayer);

        messagesInFlight.erase(currentTic);
        ++currentTic;

        auto serverResponseIter = messagesInFlight.find(currentTic);
        if (serverResponseIter != messagesInFlight.end())
        {
            correctionWasRequired = rollerState.Resolve(serverResponseIter->second.tic, serverResponseIter->second.msg, clientPlayer);
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

};

struct PistolStartLatencyFixture : LatencyFixture, testing::TestWithParam<int>
{
    PistolStartLatencyFixture() :
        LatencyFixture(GetParam())  // Get fucked, google.
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
        const bool newMessageReceived = Frame();

        EXPECT_EQ(false, newMessageReceived);
    }

    EXPECT_EQ(true,  Frame());
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

        const bool newMessageReceived = Frame();

        EXPECT_EQ(false, newMessageReceived);
    }

    EXPECT_EQ(true,  Frame());
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
        EXPECT_EQ(false, Frame());
        clientPlayer.ammo[am_clip] -= 1;        // keep shootin'
    }

    EXPECT_EQ(90 - (roundTripTimeInTics - 1), clientPlayer.ammo[am_clip]);

    EXPECT_EQ(true, Frame());
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
        EXPECT_EQ(false, Frame());
    }

    EXPECT_EQ(true, clientPlayer.weaponowned[wp_missile]);
    EXPECT_EQ(2,    clientPlayer.ammo[am_misl]);

    EXPECT_EQ(true, Frame());
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

        EXPECT_EQ(false, Frame());
    }

    EXPECT_EQ(true, clientPlayer.weaponowned[wp_missile]);
    EXPECT_EQ(1,    clientPlayer.ammo[am_misl]);

    EXPECT_EQ(true, Frame());
    EXPECT_EQ(true, correctionWasRequired);

    EXPECT_EQ(false, clientPlayer.weaponowned[wp_missile]);
    EXPECT_EQ(0,     clientPlayer.ammo[am_misl]);
}

INSTANTIATE_TEST_SUITE_P(WeaponDesync,
                         PickupWeaponAndAmmoSuite,
                         testing::Range(10, 300, 10));
