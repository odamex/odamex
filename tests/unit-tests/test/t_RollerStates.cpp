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
    player_t            reconciledPlayer;
    bool                correctionWasRequired;

    explicit LatencyFixture(int pingTimeInMsec) :
        currentTic            (0),
        roundTripTimeInTics   (static_cast<int>(std::ceil(static_cast<double>(pingTimeInMsec) / TIME_STEP_MSEC)) + 1),  // +1 because of the server frame.
        messagesInFlight      (roundTripTimeInTics * 2),     // double bucket count for safety margin.
        rollerState           (currentTic),
        correctionWasRequired (false)
    {
    }

    void Frame()
    {
        clientPlayer = reconciledPlayer;
        rollerState.Record(currentTic, clientPlayer);

        messagesInFlight.erase(currentTic);
        ++currentTic;

        auto& serverResponse = messagesInFlight[currentTic];
        correctionWasRequired = rollerState.Resolve(serverResponse.tic, serverResponse.msg, reconciledPlayer);
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

TEST_P(PistolStartLatencyFixture, HappyPistolStartAndShoot)
{
    // This is baby's first check.  Start with a basic sanity check.
    EXPECT_GT(roundTripTimeInTics, 0);

    EXPECT_EQ(clientPlayer.ammo[am_clip], 50);


}

INSTANTIATE_TEST_SUITE_P(VariousHappyPistolStartsAndShots,
                         PistolStartLatencyFixture,
                         testing::Range(10, 300, 10));

