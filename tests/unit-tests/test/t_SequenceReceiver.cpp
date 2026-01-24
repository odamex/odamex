#include "gtest/gtest.h"

#include "odamex.h"

#include "SequenceReceiver.h"

#define REQUIRE(x) EXPECT_TRUE((x))

struct ReliableSequenceReceiverData
{
    QueueEntryType packet;
};

struct ReliableSequenceReceiver : ReliableSequenceReceiverData, testing::Test
{
};

TEST_F(ReliableSequenceReceiver, BasicReceive)
{
    SequenceReceiver receiver(10);

    REQUIRE(receiver.NextPacket() == nullptr);

    receiver.RegisterReceivedPacket(0, packet.buf);

    auto packetPtr = receiver.NextPacket();
    REQUIRE(packetPtr != nullptr);
    REQUIRE(packetPtr->sequence == 0);
    REQUIRE(receiver.NextPacket() == nullptr);

    // Duplicate receive.
    receiver.RegisterReceivedPacket(0, packet.buf);
    REQUIRE(receiver.NextPacket() == nullptr);

    receiver.RegisterReceivedPacket(1, packet.buf);

    REQUIRE((packetPtr = receiver.NextPacket()) and packetPtr->sequence == 1);
    REQUIRE(receiver.NextPacket() == nullptr);
}

TEST_F(ReliableSequenceReceiver, MultiReceive)
{
    SequenceReceiver receiver(10);

    receiver.RegisterReceivedPacket(0, packet.buf);
    receiver.RegisterReceivedPacket(1, packet.buf);
    receiver.RegisterReceivedPacket(2, packet.buf);
    receiver.RegisterReceivedPacket(3, packet.buf);
    receiver.RegisterReceivedPacket(4, packet.buf);

    QueueEntryType* packetPtr;
    REQUIRE((packetPtr = receiver.NextPacket()) and packetPtr->sequence == 0);
    REQUIRE((packetPtr = receiver.NextPacket()) and packetPtr->sequence == 1);
    REQUIRE((packetPtr = receiver.NextPacket()) and packetPtr->sequence == 2);
    REQUIRE((packetPtr = receiver.NextPacket()) and packetPtr->sequence == 3);
    REQUIRE((packetPtr = receiver.NextPacket()) and packetPtr->sequence == 4);
    REQUIRE(receiver.NextPacket() == nullptr);
}

TEST_F(ReliableSequenceReceiver, OutOfSequence)
{
    SequenceReceiver receiver(10);

    receiver.RegisterReceivedPacket(2, packet.buf);
    receiver.RegisterReceivedPacket(5, packet.buf);
    receiver.RegisterReceivedPacket(3, packet.buf);

    REQUIRE(receiver.NextPacket() == nullptr);

    receiver.RegisterReceivedPacket(0, packet.buf);

    QueueEntryType* packetPtr;
    REQUIRE((packetPtr = receiver.NextPacket()) and packetPtr->sequence == 0);
    REQUIRE(receiver.NextPacket() == nullptr);

    receiver.RegisterReceivedPacket(1, packet.buf);

    REQUIRE((packetPtr = receiver.NextPacket()) and packetPtr->sequence == 1);
    REQUIRE((packetPtr = receiver.NextPacket()) and packetPtr->sequence == 2);
    REQUIRE((packetPtr = receiver.NextPacket()) and packetPtr->sequence == 3);
    REQUIRE(receiver.NextPacket() == nullptr);
    REQUIRE(receiver.NextPacket() == nullptr);

    receiver.RegisterReceivedPacket(6, packet.buf);
    REQUIRE(receiver.NextPacket() == nullptr);

    receiver.RegisterReceivedPacket(4, packet.buf);
    REQUIRE((packetPtr = receiver.NextPacket()) and packetPtr->sequence == 4);

    receiver.RegisterReceivedPacket(7, packet.buf);
    REQUIRE((packetPtr = receiver.NextPacket()) and packetPtr->sequence == 5);
    REQUIRE((packetPtr = receiver.NextPacket()) and packetPtr->sequence == 6);
    REQUIRE((packetPtr = receiver.NextPacket()) and packetPtr->sequence == 7);
    REQUIRE(receiver.NextPacket() == nullptr);

}
