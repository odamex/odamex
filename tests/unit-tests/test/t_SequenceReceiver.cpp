#include "gtest/gtest.h"

#include "odamex.h"

#include "SequenceReceiver.h"

#define REQUIRE(x) EXPECT_TRUE((x))

struct ReliableSequenceReceiverData
{
    SequenceQueueEntryType packet;
};

struct ReliableSequenceReceiver : ReliableSequenceReceiverData, testing::Test
{
};

TEST_F(ReliableSequenceReceiver, BasicReceive)
{
    SequenceReceiver receiver(10);

    REQUIRE(receiver.NextPacket(packet.buf) < 0);

    receiver.RegisterReceivedPacket(0, packet.buf);

    auto sequence = receiver.NextPacket(packet.buf);
    REQUIRE(sequence == 0);
    REQUIRE(receiver.NextPacket(packet.buf) < 0);

    // Duplicate receive.
    receiver.RegisterReceivedPacket(0, packet.buf);
    REQUIRE(receiver.NextPacket(packet.buf) < 0);

    receiver.RegisterReceivedPacket(1, packet.buf);

    REQUIRE(receiver.NextPacket(packet.buf) == 1);
    REQUIRE(receiver.NextPacket(packet.buf) < 0);
}

TEST_F(ReliableSequenceReceiver, MultiReceive)
{
    SequenceReceiver receiver(10);

    receiver.RegisterReceivedPacket(0, packet.buf);
    receiver.RegisterReceivedPacket(1, packet.buf);
    receiver.RegisterReceivedPacket(2, packet.buf);
    receiver.RegisterReceivedPacket(3, packet.buf);
    receiver.RegisterReceivedPacket(4, packet.buf);

    REQUIRE(receiver.NextPacket(packet.buf) == 0);
    REQUIRE(receiver.NextPacket(packet.buf) == 1);
    REQUIRE(receiver.NextPacket(packet.buf) == 2);
    REQUIRE(receiver.NextPacket(packet.buf) == 3);
    REQUIRE(receiver.NextPacket(packet.buf) == 4);
    REQUIRE(receiver.NextPacket(packet.buf) < 0);
}

TEST_F(ReliableSequenceReceiver, OutOfSequence)
{
    SequenceReceiver receiver(10);

    receiver.RegisterReceivedPacket(2, packet.buf);
    receiver.RegisterReceivedPacket(5, packet.buf);
    receiver.RegisterReceivedPacket(3, packet.buf);

    REQUIRE(receiver.NextPacket(packet.buf) < 0);

    receiver.RegisterReceivedPacket(0, packet.buf);

    REQUIRE(receiver.NextPacket(packet.buf) == 0);
    REQUIRE(receiver.NextPacket(packet.buf) < 0);

    receiver.RegisterReceivedPacket(1, packet.buf);

    REQUIRE(receiver.NextPacket(packet.buf) == 1);
    REQUIRE(receiver.NextPacket(packet.buf) == 2);
    REQUIRE(receiver.NextPacket(packet.buf) == 3);
    REQUIRE(receiver.NextPacket(packet.buf) < 0);
    REQUIRE(receiver.NextPacket(packet.buf) < 0);

    receiver.RegisterReceivedPacket(6, packet.buf);
    REQUIRE(receiver.NextPacket(packet.buf) < 0);

    receiver.RegisterReceivedPacket(4, packet.buf);
    REQUIRE(receiver.NextPacket(packet.buf) == 4);

    receiver.RegisterReceivedPacket(7, packet.buf);
    REQUIRE(receiver.NextPacket(packet.buf) == 5);
    REQUIRE(receiver.NextPacket(packet.buf) == 6);
    REQUIRE(receiver.NextPacket(packet.buf) == 7);
    REQUIRE(receiver.NextPacket(packet.buf) < 0);

}
