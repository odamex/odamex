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

TEST(ReliableSequenceSender, FullUp)
{
    SequenceSender sender1(5);

    sender1.ObtainSendPacket();
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

