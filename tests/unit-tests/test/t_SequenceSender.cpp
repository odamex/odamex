#include "gtest/gtest.h"

#include "odamex.h"

#include "SequenceSender.h"

#define REQUIRE(x) EXPECT_TRUE((x))

TEST(ReliableSequenceSender, IterEmptyDefault)
{
    SequenceSender sender1{10};

    auto iter = sender1.IterateUnackedPackets();

    SequenceQueueEntryType* entryPtr = iter.Next();

    REQUIRE(entryPtr == nullptr);
}

TEST(ReliableSequenceSender, DefaultEmptyBuffer)
{
    SequenceSender sender1(10);

    auto packet = sender1.ObtainSendPacket();

    REQUIRE(packet.sequence       == 0);
    REQUIRE(packet.buffer         != nullptr);
    REQUIRE(packet.buffer->size() == 0);
}

TEST(ReliableSequenceSender, OneSendOneAckAndNullIter)
{
    SequenceSender sender1(10);

    sender1.ObtainSendPacket();

    auto iter = sender1.IterateUnackedPackets();

    REQUIRE(iter.Next() != nullptr);
    REQUIRE(iter.Next() == nullptr);
    REQUIRE(iter.Next() == nullptr);

    iter = sender1.IterateUnackedPackets();

    REQUIRE(iter.Next() != nullptr);
    REQUIRE(iter.Next() == nullptr);
    REQUIRE(iter.Next() == nullptr);

    REQUIRE(sender1.Acknowledge(0));

    iter = sender1.IterateUnackedPackets();

    REQUIRE(iter.Next() == nullptr);

    REQUIRE(not sender1.Acknowledge(1));
    REQUIRE(not sender1.Acknowledge(0));
    REQUIRE(not sender1.Acknowledge(6));
}

TEST(ReliableSequenceSender, MultipleSendsAndAcks)
{
    SequenceSender sender1(10);

    sender1.ObtainSendPacket();
    sender1.ObtainSendPacket();
    sender1.ObtainSendPacket();

    auto iter = sender1.IterateUnackedPackets();

    REQUIRE(iter.Next() != nullptr);
    REQUIRE(iter.Next() != nullptr);
    REQUIRE(iter.Next() != nullptr);
    REQUIRE(iter.Next() == nullptr);

    REQUIRE(sender1.Acknowledge(0));
    REQUIRE(sender1.Acknowledge(1));
    REQUIRE(sender1.Acknowledge(2));

    iter = sender1.IterateUnackedPackets();
    REQUIRE(iter.Next() == nullptr);

    sender1.ObtainSendPacket();
    sender1.ObtainSendPacket();
    sender1.ObtainSendPacket();

    iter = sender1.IterateUnackedPackets();

    REQUIRE((iter.Next())->sequence == 3);
    REQUIRE((iter.Next())->sequence == 4);
    REQUIRE((iter.Next())->sequence == 5);
    REQUIRE(iter.Next() == nullptr);

    REQUIRE(sender1.Acknowledge(3));
    REQUIRE(sender1.Acknowledge(4));
    REQUIRE(sender1.Acknowledge(5));

    iter = sender1.IterateUnackedPackets();
    REQUIRE(iter.Next() == nullptr);
}

TEST(ReliableSequenceSender, MultipleOutOfOrder)
{
    SequenceSender sender1(100);

    sender1.ObtainSendPacket();
    sender1.ObtainSendPacket();
    sender1.ObtainSendPacket();

    auto iter = sender1.IterateUnackedPackets();

    REQUIRE((iter.Next())->sequence == 0);
    REQUIRE((iter.Next())->sequence == 1);
    REQUIRE((iter.Next())->sequence == 2);
    REQUIRE(iter.Next() == nullptr);

    REQUIRE(sender1.Acknowledge(1));

    iter = sender1.IterateUnackedPackets();

    REQUIRE((iter.Next())->sequence == 0);
    REQUIRE((iter.Next())->sequence == 2);
    REQUIRE(iter.Next() == nullptr);

    sender1.ObtainSendPacket();

    iter = sender1.IterateUnackedPackets();

    REQUIRE((iter.Next())->sequence == 0);
    REQUIRE((iter.Next())->sequence == 2);
    REQUIRE((iter.Next())->sequence == 3);
    REQUIRE(iter.Next() == nullptr);

    REQUIRE(sender1.Acknowledge(0));

    iter = sender1.IterateUnackedPackets();

    REQUIRE((iter.Next())->sequence == 2);
    REQUIRE((iter.Next())->sequence == 3);
    REQUIRE(iter.Next() == nullptr);

    sender1.ObtainSendPacket();
    sender1.ObtainSendPacket();
    sender1.ObtainSendPacket();

    iter = sender1.IterateUnackedPackets();

    REQUIRE((iter.Next())->sequence == 2);
    REQUIRE((iter.Next())->sequence == 3);
    REQUIRE((iter.Next())->sequence == 4);
    REQUIRE((iter.Next())->sequence == 5);
    REQUIRE((iter.Next())->sequence == 6);
    REQUIRE(iter.Next() == nullptr);

    REQUIRE(sender1.Acknowledge(6));

    iter = sender1.IterateUnackedPackets();
    REQUIRE((iter.Next())->sequence == 2);
    REQUIRE((iter.Next())->sequence == 3);
    REQUIRE((iter.Next())->sequence == 4);
    REQUIRE((iter.Next())->sequence == 5);
    REQUIRE(iter.Next() == nullptr);

    REQUIRE(sender1.Acknowledge(3));

    iter = sender1.IterateUnackedPackets();
    REQUIRE((iter.Next())->sequence == 2);
    REQUIRE((iter.Next())->sequence == 4);
    REQUIRE((iter.Next())->sequence == 5);
    REQUIRE(iter.Next() == nullptr);

    REQUIRE(sender1.Acknowledge(2));

    iter = sender1.IterateUnackedPackets();
    REQUIRE((iter.Next())->sequence == 4);
    REQUIRE((iter.Next())->sequence == 5);
    REQUIRE(iter.Next() == nullptr);

    REQUIRE(sender1.Acknowledge(5));

    iter = sender1.IterateUnackedPackets();
    REQUIRE((iter.Next())->sequence == 4);
    REQUIRE(iter.Next() == nullptr);

    REQUIRE(sender1.Acknowledge(4));

    iter = sender1.IterateUnackedPackets();
    REQUIRE(iter.Next() == nullptr);
}

TEST(ReliableSequenceSender, NormalWrap)
{
    SequenceSender sender1(5);

    sender1.ObtainSendPacket();
    sender1.ObtainSendPacket();
    sender1.ObtainSendPacket();

    sender1.Acknowledge(0);
    sender1.Acknowledge(1);
    sender1.Acknowledge(2);

    sender1.ObtainSendPacket();
    sender1.ObtainSendPacket();
    sender1.ObtainSendPacket();
    sender1.ObtainSendPacket();

    auto iter = sender1.IterateUnackedPackets();
    SequenceQueueEntryType* entry;
    REQUIRE((entry = iter.Next()) != nullptr && entry->sequence == 3);
    REQUIRE((entry = iter.Next()) != nullptr && entry->sequence == 4);
    REQUIRE((entry = iter.Next()) != nullptr && entry->sequence == 5);
    REQUIRE((entry = iter.Next()) != nullptr && entry->sequence == 6);
    REQUIRE((entry = iter.Next()) == nullptr);
}

TEST(ReliableSequenceSender, FullUpWrapReuse)
{
    SequenceSender sender1(5);

    auto packet0 = sender1.ObtainSendPacket();

    packet0.buffer->WriteLong(0x000Fd00d);

    REQUIRE(packet0.buffer->size() == 4);

    sender1.ObtainSendPacket();
    sender1.ObtainSendPacket();
    sender1.ObtainSendPacket();
    sender1.ObtainSendPacket();

    auto iter = sender1.IterateUnackedPackets();
    SequenceQueueEntryType* entry;
    REQUIRE((entry = iter.Next()) != nullptr && entry->sequence == 0);
    REQUIRE((entry = iter.Next()) != nullptr && entry->sequence == 1);
    REQUIRE((entry = iter.Next()) != nullptr && entry->sequence == 2);
    REQUIRE((entry = iter.Next()) != nullptr && entry->sequence == 3);
    REQUIRE((entry = iter.Next()) != nullptr && entry->sequence == 4);
    REQUIRE((entry = iter.Next()) == nullptr);

    sender1.Acknowledge(0);
    sender1.Acknowledge(1);
    sender1.Acknowledge(2);
    sender1.Acknowledge(3);
    sender1.Acknowledge(4);

    auto packet5 = sender1.ObtainSendPacket();

    REQUIRE(packet5.sequence       == 5);
    REQUIRE(packet5.buffer         == packet0.buffer);
    REQUIRE(packet5.buffer->size() == 0);
}

TEST(ReliableSequenceSender, Exhaustion)
{
    SequenceSender sender1(5);

    auto packet = sender1.ObtainSendPacket();

    REQUIRE(packet.sequence == 0);

    sender1.ObtainSendPacket();
    sender1.ObtainSendPacket();
    sender1.ObtainSendPacket();
    packet = sender1.ObtainSendPacket();

    REQUIRE(packet.sequence == 4);

    REQUIRE(sender1.GetMode() == SequenceSender::NORMAL);

    packet = sender1.ObtainSendPacket();

    REQUIRE(packet.sequence == -1);
    REQUIRE(packet.buffer   == nullptr);

    REQUIRE(sender1.GetMode() == SequenceSender::RECOVERY);
}
