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

TEST(ReliableSequenceSender, OneSendOneAckAndNullIter)
{
    SequenceSender sender1(10);

    sender1.ObtainSendPacket(1);

    auto iter = sender1.IterateUnackedPackets();

    REQUIRE(iter.Next() != nullptr);
    REQUIRE(iter.Next() == nullptr);
    REQUIRE(iter.Next() == nullptr);


    iter = sender1.IterateUnackedPackets();

    REQUIRE(iter.Next() != nullptr);
    REQUIRE(iter.Next() == nullptr);
    REQUIRE(iter.Next() == nullptr);

    sender1.Acknowledge(1);

    iter = sender1.IterateUnackedPackets();

    REQUIRE(iter.Next() == nullptr);

    sender1.Acknowledge(1);
    sender1.Acknowledge(0);
    sender1.Acknowledge(6);
}

TEST(ReliableSequenceSender, MultipleSendsAndAcks)
{
    SequenceSender sender1(10);

    sender1.ObtainSendPacket(1);
    sender1.ObtainSendPacket(2);
    sender1.ObtainSendPacket(3);

    auto iter = sender1.IterateUnackedPackets();

    REQUIRE(iter.Next() != nullptr);
    REQUIRE(iter.Next() != nullptr);
    REQUIRE(iter.Next() != nullptr);
    REQUIRE(iter.Next() == nullptr);

    sender1.Acknowledge(1);
    sender1.Acknowledge(2);
    sender1.Acknowledge(3);

    iter = sender1.IterateUnackedPackets();
    REQUIRE(iter.Next() == nullptr);

    sender1.ObtainSendPacket(4);
    sender1.ObtainSendPacket(5);
    sender1.ObtainSendPacket(6);

    iter = sender1.IterateUnackedPackets();

    REQUIRE((iter.Next())->sequence == 4);
    REQUIRE((iter.Next())->sequence == 5);
    REQUIRE((iter.Next())->sequence == 6);
    REQUIRE(iter.Next() == nullptr);

    sender1.Acknowledge(4);
    sender1.Acknowledge(5);
    sender1.Acknowledge(6);

    iter = sender1.IterateUnackedPackets();
    REQUIRE(iter.Next() == nullptr);
}

TEST(ReliableSequenceSender, MultipleOutOfOrder)
{
    SequenceSender sender1(100);

    sender1.ObtainSendPacket(10);
    sender1.ObtainSendPacket(11);
    sender1.ObtainSendPacket(12);

    auto iter = sender1.IterateUnackedPackets();

    REQUIRE((iter.Next())->sequence == 10);
    REQUIRE((iter.Next())->sequence == 11);
    REQUIRE((iter.Next())->sequence == 12);
    REQUIRE(iter.Next() == nullptr);

    sender1.Acknowledge(11);

    iter = sender1.IterateUnackedPackets();

    REQUIRE((iter.Next())->sequence == 10);
    REQUIRE((iter.Next())->sequence == 12);
    REQUIRE(iter.Next() == nullptr);

    sender1.ObtainSendPacket(13);

    iter = sender1.IterateUnackedPackets();

    REQUIRE((iter.Next())->sequence == 10);
    REQUIRE((iter.Next())->sequence == 12);
    REQUIRE((iter.Next())->sequence == 13);
    REQUIRE(iter.Next() == nullptr);

    sender1.Acknowledge(10);

    iter = sender1.IterateUnackedPackets();

    REQUIRE((iter.Next())->sequence == 12);
    REQUIRE((iter.Next())->sequence == 13);
    REQUIRE(iter.Next() == nullptr);

    sender1.ObtainSendPacket(14);
    sender1.ObtainSendPacket(15);
    sender1.ObtainSendPacket(16);

    iter = sender1.IterateUnackedPackets();

    REQUIRE((iter.Next())->sequence == 12);
    REQUIRE((iter.Next())->sequence == 13);
    REQUIRE((iter.Next())->sequence == 14);
    REQUIRE((iter.Next())->sequence == 15);
    REQUIRE((iter.Next())->sequence == 16);
    REQUIRE(iter.Next() == nullptr);

    sender1.Acknowledge(16);

    iter = sender1.IterateUnackedPackets();
    REQUIRE((iter.Next())->sequence == 12);
    REQUIRE((iter.Next())->sequence == 13);
    REQUIRE((iter.Next())->sequence == 14);
    REQUIRE((iter.Next())->sequence == 15);
    REQUIRE(iter.Next() == nullptr);

    sender1.Acknowledge(13);

    iter = sender1.IterateUnackedPackets();
    REQUIRE((iter.Next())->sequence == 12);
    REQUIRE((iter.Next())->sequence == 14);
    REQUIRE((iter.Next())->sequence == 15);
    REQUIRE(iter.Next() == nullptr);

    sender1.Acknowledge(12);

    iter = sender1.IterateUnackedPackets();
    REQUIRE((iter.Next())->sequence == 14);
    REQUIRE((iter.Next())->sequence == 15);
    REQUIRE(iter.Next() == nullptr);

    sender1.Acknowledge(15);

    iter = sender1.IterateUnackedPackets();
    REQUIRE((iter.Next())->sequence == 14);
    REQUIRE(iter.Next() == nullptr);

    sender1.Acknowledge(14);

    iter = sender1.IterateUnackedPackets();
    REQUIRE(iter.Next() == nullptr);
}

TEST(ReliableSequenceSender, FullUp)
{
    SequenceSender sender1(5);

    sender1.ObtainSendPacket(0);
    sender1.ObtainSendPacket(1);
    sender1.ObtainSendPacket(2);
    sender1.ObtainSendPacket(3);
    sender1.ObtainSendPacket(4);

    auto iter = sender1.IterateUnackedPackets();
    SequenceQueueEntryType* entry;
    REQUIRE((entry = iter.Next()) != nullptr && entry->sequence == 0);
    REQUIRE((entry = iter.Next()) != nullptr && entry->sequence == 1);
    REQUIRE((entry = iter.Next()) != nullptr && entry->sequence == 2);
    REQUIRE((entry = iter.Next()) != nullptr && entry->sequence == 3);
    REQUIRE((entry = iter.Next()) != nullptr && entry->sequence == 4);
    REQUIRE((entry = iter.Next()) == nullptr);
}

TEST(ReliableSequenceSender, NormalWrap)
{
    SequenceSender sender1(5);

    sender1.ObtainSendPacket(3);
    sender1.ObtainSendPacket(4);
    sender1.ObtainSendPacket(5);
    sender1.ObtainSendPacket(6);

    auto iter = sender1.IterateUnackedPackets();
    SequenceQueueEntryType* entry;
    REQUIRE((entry = iter.Next()) != nullptr && entry->sequence == 3);
    REQUIRE((entry = iter.Next()) != nullptr && entry->sequence == 4);
    REQUIRE((entry = iter.Next()) != nullptr && entry->sequence == 5);
    REQUIRE((entry = iter.Next()) != nullptr && entry->sequence == 6);
    REQUIRE((entry = iter.Next()) == nullptr);
}

