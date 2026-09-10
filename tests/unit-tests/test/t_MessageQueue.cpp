#include "gtest/gtest.h"

#include "odamex.h"

#include "MessageQueue.h"

#define REQUIRE(x) EXPECT_TRUE((x))

struct MessageQueueFixture : testing::Test
{
    MessageQueue m_queue;
};

TEST_F(MessageQueueFixture, BasicByteSize)
{
    REQUIRE(0 == m_queue.SizeInBytes());
    REQUIRE(0 == m_queue.SizeInMessages());

    auto& msg1 = m_queue.Obtain();

    REQUIRE(0 == m_queue.SizeInBytes());
    REQUIRE(1 == m_queue.SizeInMessages());

    msg1.WriteByte(1);
    msg1.WriteLong(0xfedface);

    REQUIRE(5 == m_queue.SizeInBytes());
    REQUIRE(1 == m_queue.SizeInMessages());

    auto& msg2 = m_queue.Obtain();

    REQUIRE(5 == m_queue.SizeInBytes());
    REQUIRE(2 == m_queue.SizeInMessages());

    msg2.WriteShort(0xea7);

    REQUIRE(7 == m_queue.SizeInBytes());
    REQUIRE(2 == m_queue.SizeInMessages());

    m_queue.Pop();

    REQUIRE(2 == m_queue.SizeInBytes());
    REQUIRE(1 == m_queue.SizeInMessages());

    m_queue.Pop();

    REQUIRE(0 == m_queue.SizeInBytes());
    REQUIRE(0 == m_queue.SizeInMessages());
}
