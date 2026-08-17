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

    receiver.RegisterReliablePacket(0, packet.buf.size(), packet.buf);

    auto sequence = receiver.NextPacket(packet.buf);
    REQUIRE(sequence == 0);
    REQUIRE(receiver.NextPacket(packet.buf) < 0);

    // Duplicate receive.
    receiver.RegisterReliablePacket(0, packet.buf.size(), packet.buf);
    REQUIRE(receiver.NextPacket(packet.buf) < 0);

    receiver.RegisterReliablePacket(1, packet.buf.size(), packet.buf);

    REQUIRE(receiver.NextPacket(packet.buf) == 1);
    REQUIRE(receiver.NextPacket(packet.buf) < 0);
}

TEST_F(ReliableSequenceReceiver, MultiReceive)
{
    SequenceReceiver receiver(10);

    receiver.RegisterReliablePacket(0, packet.buf.size(), packet.buf);
    receiver.RegisterReliablePacket(1, packet.buf.size(), packet.buf);
    receiver.RegisterReliablePacket(2, packet.buf.size(), packet.buf);
    receiver.RegisterReliablePacket(3, packet.buf.size(), packet.buf);
    receiver.RegisterReliablePacket(4, packet.buf.size(), packet.buf);

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

    receiver.RegisterReliablePacket(2, packet.buf.size(), packet.buf);
    receiver.RegisterReliablePacket(5, packet.buf.size(), packet.buf);
    receiver.RegisterReliablePacket(3, packet.buf.size(), packet.buf);

    REQUIRE(receiver.NextPacket(packet.buf) < 0);

    receiver.RegisterReliablePacket(0, packet.buf.size(), packet.buf);

    REQUIRE(receiver.NextPacket(packet.buf) == 0);
    REQUIRE(receiver.NextPacket(packet.buf) < 0);

    receiver.RegisterReliablePacket(1, packet.buf.size(), packet.buf);

    REQUIRE(receiver.NextPacket(packet.buf) == 1);
    REQUIRE(receiver.NextPacket(packet.buf) == 2);
    REQUIRE(receiver.NextPacket(packet.buf) == 3);
    REQUIRE(receiver.NextPacket(packet.buf) < 0);
    REQUIRE(receiver.NextPacket(packet.buf) < 0);

    receiver.RegisterReliablePacket(6, packet.buf.size(), packet.buf);
    REQUIRE(receiver.NextPacket(packet.buf) < 0);

    receiver.RegisterReliablePacket(4, packet.buf.size(), packet.buf);
    REQUIRE(receiver.NextPacket(packet.buf) == 4);

    receiver.RegisterReliablePacket(7, packet.buf.size(), packet.buf);
    REQUIRE(receiver.NextPacket(packet.buf) == 5);
    REQUIRE(receiver.NextPacket(packet.buf) == 6);
    REQUIRE(receiver.NextPacket(packet.buf) == 7);
    REQUIRE(receiver.NextPacket(packet.buf) < 0);
}

TEST_F(ReliableSequenceReceiver, BestEffortPortionOrderQueuing)
{
    SequenceReceiver receiver(10);

    packet.buf.WriteUnVarint(11); receiver.RegisterBestEffortPacket(0, packet.buf.size(), packet.buf); packet.buf.clear();
    packet.buf.WriteUnVarint(44); receiver.RegisterBestEffortPacket(0, packet.buf.size(), packet.buf); packet.buf.clear();
    packet.buf.WriteUnVarint(66); receiver.RegisterBestEffortPacket(0, packet.buf.size(), packet.buf); packet.buf.clear();

    packet.buf.WriteUnVarint(333); receiver.RegisterBestEffortPacket(1, packet.buf.size(), packet.buf); packet.buf.clear();
    packet.buf.WriteUnVarint(666); receiver.RegisterBestEffortPacket(1, packet.buf.size(), packet.buf); packet.buf.clear();

    packet.buf.WriteUnVarint(100);
    receiver.RegisterReliablePacket(1, packet.buf.size(), packet.buf);
    packet.buf.clear();

    // Not ready to receive anything - all is backed up on reliable packet 1.
    REQUIRE(receiver.NextPacket(packet.buf) < 0);

    packet.buf.WriteUnVarint(700);
    receiver.RegisterReliablePacket(0, packet.buf.size(), packet.buf);
    packet.buf.clear();

    REQUIRE(receiver.NextPacket(packet.buf) == 0);  REQUIRE(packet.buf.ReadUnVarint() == 700);
    REQUIRE(receiver.NextPacket(packet.buf) == 0);  REQUIRE(packet.buf.ReadUnVarint() ==  11);
    REQUIRE(receiver.NextPacket(packet.buf) == 0);  REQUIRE(packet.buf.ReadUnVarint() ==  44);
    REQUIRE(receiver.NextPacket(packet.buf) == 0);  REQUIRE(packet.buf.ReadUnVarint() ==  66);

    REQUIRE(receiver.NextPacket(packet.buf) == 1);  REQUIRE(packet.buf.ReadUnVarint() == 100);

    packet.buf.clear(); packet.buf.WriteUnVarint(200); receiver.RegisterReliablePacket(2, packet.buf.size(), packet.buf);
    packet.buf.clear(); packet.buf.WriteUnVarint(300); receiver.RegisterBestEffortPacket(2, packet.buf.size(), packet.buf);

    REQUIRE(receiver.NextPacket(packet.buf) == 1);  REQUIRE(packet.buf.ReadUnVarint() == 333);
    REQUIRE(receiver.NextPacket(packet.buf) == 1);  REQUIRE(packet.buf.ReadUnVarint() == 666);
    REQUIRE(receiver.NextPacket(packet.buf) == 2);  REQUIRE(packet.buf.ReadUnVarint() == 200);
    REQUIRE(receiver.NextPacket(packet.buf) == 2);  REQUIRE(packet.buf.ReadUnVarint() == 300);
}
