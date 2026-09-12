#include "gtest/gtest.h"

#include "odamex.h"

#include "SequenceReceiver.h"

#define REQUIRE(x) EXPECT_TRUE((x)) // NOLINT (cppcoreguidelines-macro-usage)

struct ReliableSequenceReceiver : testing::Test
{
    SequenceQueueEntryType packet;
};

TEST_F(ReliableSequenceReceiver, BasicReceive)
{
    SequenceReceiver receiver(10);

    REQUIRE(receiver.NextPacket(packet.buf) == std::nullopt);

    receiver.RegisterReliablePacket(PacketHeaderType(0), packet.buf.size(), packet.buf);

    auto nextPacket = receiver.NextPacket(packet.buf);
    REQUIRE(nextPacket && nextPacket->sequence == 0);

    REQUIRE(receiver.NextPacket(packet.buf) == std::nullopt);

    // Duplicate receive.
    receiver.RegisterReliablePacket(PacketHeaderType(0), packet.buf.size(), packet.buf);
    REQUIRE(receiver.NextPacket(packet.buf) == std::nullopt);

    receiver.RegisterReliablePacket(PacketHeaderType(1), packet.buf.size(), packet.buf);

    REQUIRE(receiver.NextPacket(packet.buf) == PacketHeaderType(1));
    REQUIRE(receiver.NextPacket(packet.buf) == std::nullopt);
}

TEST_F(ReliableSequenceReceiver, MultiReceive)
{
    SequenceReceiver receiver(10);

    receiver.RegisterReliablePacket(PacketHeaderType(0), packet.buf.size(), packet.buf);
    receiver.RegisterReliablePacket(PacketHeaderType(1), packet.buf.size(), packet.buf);
    receiver.RegisterReliablePacket(PacketHeaderType(2), packet.buf.size(), packet.buf);
    receiver.RegisterReliablePacket(PacketHeaderType(3), packet.buf.size(), packet.buf);
    receiver.RegisterReliablePacket(PacketHeaderType(4), packet.buf.size(), packet.buf);

    REQUIRE(receiver.NextPacket(packet.buf) == PacketHeaderType(0));
    REQUIRE(receiver.NextPacket(packet.buf) == PacketHeaderType(1));
    REQUIRE(receiver.NextPacket(packet.buf) == PacketHeaderType(2));
    REQUIRE(receiver.NextPacket(packet.buf) == PacketHeaderType(3));
    REQUIRE(receiver.NextPacket(packet.buf) == PacketHeaderType(4));
    REQUIRE(receiver.NextPacket(packet.buf) == std::nullopt);
}

TEST_F(ReliableSequenceReceiver, OutOfSequence)
{
    SequenceReceiver receiver(10);

    receiver.RegisterReliablePacket(PacketHeaderType(2), packet.buf.size(), packet.buf);
    receiver.RegisterReliablePacket(PacketHeaderType(5), packet.buf.size(), packet.buf);
    receiver.RegisterReliablePacket(PacketHeaderType(3), packet.buf.size(), packet.buf);

    REQUIRE(receiver.NextPacket(packet.buf) == std::nullopt);

    receiver.RegisterReliablePacket(PacketHeaderType(0), packet.buf.size(), packet.buf);

    REQUIRE(receiver.NextPacket(packet.buf) == PacketHeaderType(0));
    REQUIRE(receiver.NextPacket(packet.buf) == std::nullopt);

    receiver.RegisterReliablePacket(PacketHeaderType(1), packet.buf.size(), packet.buf);

    REQUIRE(receiver.NextPacket(packet.buf) == PacketHeaderType(1));
    REQUIRE(receiver.NextPacket(packet.buf) == PacketHeaderType(2));
    REQUIRE(receiver.NextPacket(packet.buf) == PacketHeaderType(3));
    REQUIRE(receiver.NextPacket(packet.buf) == std::nullopt);
    REQUIRE(receiver.NextPacket(packet.buf) == std::nullopt);

    receiver.RegisterReliablePacket(PacketHeaderType(6), packet.buf.size(), packet.buf);
    REQUIRE(receiver.NextPacket(packet.buf) == std::nullopt);

    receiver.RegisterReliablePacket(PacketHeaderType(4), packet.buf.size(), packet.buf);
    REQUIRE(receiver.NextPacket(packet.buf) == PacketHeaderType(4));

    receiver.RegisterReliablePacket(PacketHeaderType(7), packet.buf.size(), packet.buf);
    REQUIRE(receiver.NextPacket(packet.buf) == PacketHeaderType(5));
    REQUIRE(receiver.NextPacket(packet.buf) == PacketHeaderType(6));
    REQUIRE(receiver.NextPacket(packet.buf) == PacketHeaderType(7));
    REQUIRE(receiver.NextPacket(packet.buf) == std::nullopt);
}

TEST_F(ReliableSequenceReceiver, BestEffortPortionOrderQueuing)
{
    SequenceReceiver receiver(10);

    packet.buf.WriteUnVarint(11); receiver.RegisterBestEffortPacket(PacketHeaderType(0), packet.buf.size(), packet.buf); packet.buf.clear();
    packet.buf.WriteUnVarint(44); receiver.RegisterBestEffortPacket(PacketHeaderType(0), packet.buf.size(), packet.buf); packet.buf.clear();
    packet.buf.WriteUnVarint(66); receiver.RegisterBestEffortPacket(PacketHeaderType(0), packet.buf.size(), packet.buf); packet.buf.clear();

    packet.buf.WriteUnVarint(333); receiver.RegisterBestEffortPacket(PacketHeaderType(1), packet.buf.size(), packet.buf); packet.buf.clear();
    packet.buf.WriteUnVarint(666); receiver.RegisterBestEffortPacket(PacketHeaderType(1), packet.buf.size(), packet.buf); packet.buf.clear();

    packet.buf.WriteUnVarint(100);
    receiver.RegisterReliablePacket(PacketHeaderType(1), packet.buf.size(), packet.buf);
    packet.buf.clear();

    // Not ready to receive anything - all is backed up on reliable packet 1.
    REQUIRE(receiver.NextPacket(packet.buf) == std::nullopt);

    packet.buf.WriteUnVarint(700);
    receiver.RegisterReliablePacket(PacketHeaderType(0), packet.buf.size(), packet.buf);
    packet.buf.clear();

    REQUIRE(receiver.NextPacket(packet.buf) == PacketHeaderType(0));  REQUIRE(packet.buf.ReadUnVarint() == 700);
    REQUIRE(receiver.NextPacket(packet.buf) == PacketHeaderType(0));  REQUIRE(packet.buf.ReadUnVarint() ==  11);
    REQUIRE(receiver.NextPacket(packet.buf) == PacketHeaderType(0));  REQUIRE(packet.buf.ReadUnVarint() ==  44);
    REQUIRE(receiver.NextPacket(packet.buf) == PacketHeaderType(0));  REQUIRE(packet.buf.ReadUnVarint() ==  66);

    REQUIRE(receiver.NextPacket(packet.buf) == PacketHeaderType(1));  REQUIRE(packet.buf.ReadUnVarint() == 100);

    packet.buf.clear(); packet.buf.WriteUnVarint(200); receiver.RegisterReliablePacket(PacketHeaderType(2), packet.buf.size(), packet.buf);
    packet.buf.clear(); packet.buf.WriteUnVarint(300); receiver.RegisterBestEffortPacket(PacketHeaderType(2), packet.buf.size(), packet.buf);

    REQUIRE(receiver.NextPacket(packet.buf) == PacketHeaderType(1));  REQUIRE(packet.buf.ReadUnVarint() == 333);
    REQUIRE(receiver.NextPacket(packet.buf) == PacketHeaderType(1));  REQUIRE(packet.buf.ReadUnVarint() == 666);
    REQUIRE(receiver.NextPacket(packet.buf) == PacketHeaderType(2));  REQUIRE(packet.buf.ReadUnVarint() == 200);
    REQUIRE(receiver.NextPacket(packet.buf) == PacketHeaderType(2));  REQUIRE(packet.buf.ReadUnVarint() == 300);
}
