#include <array>

#include "gtest/gtest.h"

#include "LargeMessageQueue.h"

struct LargeMessageQueueFixture : testing::Test
{
    LargeMessageQueue m_queue;
};

namespace
{
    size_t KB(auto value) { return value << 10; }
}

TEST_F(LargeMessageQueueFixture, EmptyQueue)
{
    std::array<char, 5> fragment{};

    const FragmentationResultType result = m_queue.NextFragment(fragment.size(), fragment.begin());
    EXPECT_EQ(FragmentationStateEnum::NONE,     result.state);
    EXPECT_EQ(0,                                result.size);
}

TEST_F(LargeMessageQueueFixture, BasicSingleMessage)
{
    m_queue.Write(svc_print, "HelloWorld");

    std::array<char, 5> fragment{};

    const FragmentationResultType firstResult = m_queue.NextFragment(fragment.size(), fragment.begin());

    // Please note - the queue put the miniheader on the message.
    EXPECT_EQ(FragmentationStateEnum::FIRST_FRAGMENT,   firstResult.state);
    EXPECT_EQ((std::array{char(svc_print), char(10), 'H','e','l'}), fragment);
    EXPECT_EQ(5, firstResult.size);

    const FragmentationResultType nextResult = m_queue.NextFragment(1, fragment.begin());

    EXPECT_EQ(FragmentationStateEnum::CONTINUATION_FRAGMENT, nextResult.state);
    EXPECT_EQ((std::array{'l', char(10), 'H', 'e','l'}),     fragment);
    EXPECT_EQ(1, nextResult.size);

    const FragmentationResultType thirdResult = m_queue.NextFragment(fragment.size(), fragment.begin());

    EXPECT_EQ(FragmentationStateEnum::CONTINUATION_FRAGMENT, thirdResult.state);
    EXPECT_EQ((std::array{'o','W','o','r','l'}),             fragment);
    EXPECT_EQ(5, thirdResult.size);

    const FragmentationResultType lastResult = m_queue.NextFragment(fragment.size(), fragment.begin());

    EXPECT_EQ(FragmentationStateEnum::LAST_FRAGMENT, lastResult.state);
    EXPECT_EQ((std::array{'d','W','o','r','l'}),     fragment);
    EXPECT_EQ(1, lastResult.size);

    const FragmentationResultType emptyResult = m_queue.NextFragment(fragment.size(), fragment.begin());

    EXPECT_EQ(FragmentationStateEnum::NONE,       emptyResult.state);
    EXPECT_EQ((std::array{'d','W','o','r','l'}),  fragment);
    EXPECT_EQ(0, emptyResult.size);
}

TEST_F(LargeMessageQueueFixture, GiantMessage)
{
    std::string bigBoy(KB(63), '_');

    char crawler = 'a';
    for (auto iter = bigBoy.begin(); iter < bigBoy.end(); iter += 1024)
    {
        std::fill(iter, iter + 1024, crawler);
        crawler = crawler < 'z' ? crawler + 1 : 'a';
    }

    m_queue.Write(svc_print, bigBoy);

    // The first fragment has the mini-header.  Deal with that first so that we can have
    // a nice orderly test of the subsequent big chunks.

    buf_t firstFragment{16};

    const auto firstResult = m_queue.NextFragment(firstFragment.maxsize(), firstFragment.ptr());

    EXPECT_EQ(firstResult.size,  16);
    EXPECT_EQ(firstResult.state, FragmentationStateEnum::FIRST_FRAGMENT);

    firstFragment.setcursize(firstResult.size);

    EXPECT_EQ(firstFragment.ReadUnVarint(), svc_print);
    EXPECT_EQ(firstFragment.ReadUnVarint(), 63 * 1024);

    const size_t firstRunOfPayload = firstFragment.BytesLeftToRead();
    EXPECT_GT(firstRunOfPayload, 0);

    for (size_t i = 0; i < firstRunOfPayload; ++i)
    {
        EXPECT_EQ(firstFragment.ReadByte(), 'a');
    }

    auto testBigFragment = [&] (size_t size, char fillChar, FragmentationStateEnum state)
    {
        std::string fragment(size, '_');
        const std::string expectedValue(size, fillChar);

        const auto result = m_queue.NextFragment(fragment.size(), fragment.begin());

        EXPECT_EQ(result.state, state);
        EXPECT_EQ(result.size,  size);
        EXPECT_EQ(fragment,     expectedValue);
    };

    testBigFragment(1024 - firstRunOfPayload, 'a', FragmentationStateEnum::CONTINUATION_FRAGMENT);
    testBigFragment(1024 , 'b', FragmentationStateEnum::CONTINUATION_FRAGMENT);
    testBigFragment(1024 , 'c', FragmentationStateEnum::CONTINUATION_FRAGMENT);
    testBigFragment(1024 , 'd', FragmentationStateEnum::CONTINUATION_FRAGMENT);
    testBigFragment(1024 , 'e', FragmentationStateEnum::CONTINUATION_FRAGMENT);
    testBigFragment(1024 , 'f', FragmentationStateEnum::CONTINUATION_FRAGMENT);
    testBigFragment(1024 , 'g', FragmentationStateEnum::CONTINUATION_FRAGMENT);
    testBigFragment(1024 , 'h', FragmentationStateEnum::CONTINUATION_FRAGMENT);
    testBigFragment(1024 , 'i', FragmentationStateEnum::CONTINUATION_FRAGMENT);
    testBigFragment(1024 , 'j', FragmentationStateEnum::CONTINUATION_FRAGMENT);
    testBigFragment(1024 , 'k', FragmentationStateEnum::CONTINUATION_FRAGMENT);
    testBigFragment(1024 , 'l', FragmentationStateEnum::CONTINUATION_FRAGMENT);
    testBigFragment(1024 , 'm', FragmentationStateEnum::CONTINUATION_FRAGMENT);
    testBigFragment(1024 , 'n', FragmentationStateEnum::CONTINUATION_FRAGMENT);
    testBigFragment(1024 , 'o', FragmentationStateEnum::CONTINUATION_FRAGMENT);
    testBigFragment(1024 , 'p', FragmentationStateEnum::CONTINUATION_FRAGMENT);
    testBigFragment(1024 , 'q', FragmentationStateEnum::CONTINUATION_FRAGMENT);
    testBigFragment(1024 , 'r', FragmentationStateEnum::CONTINUATION_FRAGMENT);
    testBigFragment(1024 , 's', FragmentationStateEnum::CONTINUATION_FRAGMENT);
    testBigFragment(1024 , 't', FragmentationStateEnum::CONTINUATION_FRAGMENT);
    testBigFragment(1024 , 'u', FragmentationStateEnum::CONTINUATION_FRAGMENT);
    testBigFragment(1024 , 'v', FragmentationStateEnum::CONTINUATION_FRAGMENT);
    testBigFragment(1024 , 'w', FragmentationStateEnum::CONTINUATION_FRAGMENT);
    testBigFragment(1024 , 'x', FragmentationStateEnum::CONTINUATION_FRAGMENT);
    testBigFragment(1024 , 'y', FragmentationStateEnum::CONTINUATION_FRAGMENT);
    testBigFragment(1024 , 'z', FragmentationStateEnum::CONTINUATION_FRAGMENT);
    testBigFragment(1024 , 'a', FragmentationStateEnum::CONTINUATION_FRAGMENT);
    testBigFragment(1024 , 'b', FragmentationStateEnum::CONTINUATION_FRAGMENT);
    testBigFragment(1024 , 'c', FragmentationStateEnum::CONTINUATION_FRAGMENT);
    testBigFragment(1024 , 'd', FragmentationStateEnum::CONTINUATION_FRAGMENT);
    testBigFragment(1024 , 'e', FragmentationStateEnum::CONTINUATION_FRAGMENT);
    testBigFragment(1024 , 'f', FragmentationStateEnum::CONTINUATION_FRAGMENT);
    testBigFragment(1024 , 'g', FragmentationStateEnum::CONTINUATION_FRAGMENT);
    testBigFragment(1024 , 'h', FragmentationStateEnum::CONTINUATION_FRAGMENT);
    testBigFragment(1024 , 'i', FragmentationStateEnum::CONTINUATION_FRAGMENT);
    testBigFragment(1024 , 'j', FragmentationStateEnum::CONTINUATION_FRAGMENT);
    testBigFragment(1024 , 'k', FragmentationStateEnum::CONTINUATION_FRAGMENT);
    testBigFragment(1024 , 'l', FragmentationStateEnum::CONTINUATION_FRAGMENT);
    testBigFragment(1024 , 'm', FragmentationStateEnum::CONTINUATION_FRAGMENT);
    testBigFragment(1024 , 'n', FragmentationStateEnum::CONTINUATION_FRAGMENT);
    testBigFragment(1024 , 'o', FragmentationStateEnum::CONTINUATION_FRAGMENT);
    testBigFragment(1024 , 'p', FragmentationStateEnum::CONTINUATION_FRAGMENT);
    testBigFragment(1024 , 'q', FragmentationStateEnum::CONTINUATION_FRAGMENT);
    testBigFragment(1024 , 'r', FragmentationStateEnum::CONTINUATION_FRAGMENT);
    testBigFragment(1024 , 's', FragmentationStateEnum::CONTINUATION_FRAGMENT);
    testBigFragment(1024 , 't', FragmentationStateEnum::CONTINUATION_FRAGMENT);
    testBigFragment(1024 , 'u', FragmentationStateEnum::CONTINUATION_FRAGMENT);
    testBigFragment(1024 , 'v', FragmentationStateEnum::CONTINUATION_FRAGMENT);
    testBigFragment(1024 , 'w', FragmentationStateEnum::CONTINUATION_FRAGMENT);
    testBigFragment(1024 , 'x', FragmentationStateEnum::CONTINUATION_FRAGMENT);
    testBigFragment(1024 , 'y', FragmentationStateEnum::CONTINUATION_FRAGMENT);
    testBigFragment(1024 , 'z', FragmentationStateEnum::CONTINUATION_FRAGMENT);
    testBigFragment(1024 , 'a', FragmentationStateEnum::CONTINUATION_FRAGMENT);
    testBigFragment(1024 , 'b', FragmentationStateEnum::CONTINUATION_FRAGMENT);
    testBigFragment(1024 , 'c', FragmentationStateEnum::CONTINUATION_FRAGMENT);
    testBigFragment(1024 , 'd', FragmentationStateEnum::CONTINUATION_FRAGMENT);
    testBigFragment(1024 , 'e', FragmentationStateEnum::CONTINUATION_FRAGMENT);
    testBigFragment(1024 , 'f', FragmentationStateEnum::CONTINUATION_FRAGMENT);
    testBigFragment(1024 , 'g', FragmentationStateEnum::CONTINUATION_FRAGMENT);
    testBigFragment(1024 , 'h', FragmentationStateEnum::CONTINUATION_FRAGMENT);
    testBigFragment(1024 , 'i', FragmentationStateEnum::CONTINUATION_FRAGMENT);
    testBigFragment(1024 , 'j', FragmentationStateEnum::CONTINUATION_FRAGMENT);
    testBigFragment(1024 , 'k', FragmentationStateEnum::LAST_FRAGMENT);

}

TEST_F(LargeMessageQueueFixture, MultiMessage)
{
    m_queue.Write(svc_print, "HelloWorld");
    m_queue.Write(svc_print, "FooBar");

    std::array<char, 8> fragment{};

    auto result = m_queue.NextFragment(fragment.size(), fragment.begin());

    EXPECT_EQ(result.size, 8);
    EXPECT_EQ(result.state, FragmentationStateEnum::FIRST_FRAGMENT);
    EXPECT_EQ(fragment[0], svc_print);
    EXPECT_EQ(fragment[1], 10);
    EXPECT_EQ((std::string{fragment.begin() + 2, fragment.end()}), "HelloW");

    result = m_queue.NextFragment(fragment.size(), fragment.begin());

    EXPECT_EQ(result.size, 4);
    EXPECT_EQ(result.state, FragmentationStateEnum::LAST_FRAGMENT);
    EXPECT_EQ((std::string{fragment.begin(), fragment.begin() + result.size}), "orld");

    result = m_queue.NextFragment(fragment.size(), fragment.begin());

    EXPECT_EQ(result.size, 8);
    EXPECT_EQ(result.state, FragmentationStateEnum::ONE_SHOT);
    EXPECT_EQ(fragment[0], svc_print);
    EXPECT_EQ(fragment[1], 6);
    EXPECT_EQ((std::string{fragment.begin() + 2, fragment.begin() + result.size}), "FooBar");

    result = m_queue.NextFragment(fragment.size(), fragment.begin());

    EXPECT_EQ(result.size, 0);
    EXPECT_EQ(result.state, FragmentationStateEnum::NONE);
}

TEST_F(LargeMessageQueueFixture, DefaultLargeMessage)
{
    LargeMessage msg;
    EXPECT_EQ(0,    msg.TotalSize());
    EXPECT_EQ(0,    msg.CurrentSize());
    EXPECT_EQ(true, msg.IsComplete());

    auto someData = std::to_array("Hey there.");

    EXPECT_EQ(false, msg.Append(someData.data(), someData.size()));
    EXPECT_EQ(0,     msg.TotalSize());
    EXPECT_EQ(0,     msg.CurrentSize());
    EXPECT_EQ(true,  msg.IsComplete());
}

TEST_F(LargeMessageQueueFixture, Reassembly)
{
    const std::string greatAdvice = "We can't stop here!  This is BAT COUNTRY!";
    m_queue.Write(svc_print, greatAdvice);

    LargeMessage msg;

    const size_t totalSize = m_queue.GetMessageSize();
    EXPECT_EQ(totalSize, greatAdvice.size() + 2);       // +2 for the mini header.

    EXPECT_EQ(true, msg.Restart(totalSize));

    EXPECT_EQ(43,    msg.TotalSize());
    EXPECT_EQ(0,     msg.CurrentSize());
    EXPECT_EQ(false, msg.IsComplete());

    std::array<char, 8> fragment{};

    auto fragmentInfo = m_queue.NextFragment(fragment.size(), fragment.begin());
    EXPECT_EQ(fragmentInfo.state, FragmentationStateEnum::FIRST_FRAGMENT);
    EXPECT_EQ(true,  msg.Append(fragment.data(), fragmentInfo.size));
    EXPECT_EQ(43,    msg.TotalSize());
    EXPECT_EQ(8,     msg.CurrentSize());
    EXPECT_EQ(false, msg.IsComplete());

    fragmentInfo = m_queue.NextFragment(fragment.size(), fragment.begin());
    EXPECT_EQ(fragmentInfo.state, FragmentationStateEnum::CONTINUATION_FRAGMENT);
    EXPECT_EQ(true,  msg.Append(fragment.data(), fragmentInfo.size));
    EXPECT_EQ(43,    msg.TotalSize());
    EXPECT_EQ(16,    msg.CurrentSize());
    EXPECT_EQ(false, msg.IsComplete());

    fragmentInfo = m_queue.NextFragment(fragment.size(), fragment.begin());
    EXPECT_EQ(fragmentInfo.state, FragmentationStateEnum::CONTINUATION_FRAGMENT);
    EXPECT_EQ(true,  msg.Append(fragment.data(), fragmentInfo.size));
    EXPECT_EQ(43,    msg.TotalSize());
    EXPECT_EQ(24,    msg.CurrentSize());
    EXPECT_EQ(false, msg.IsComplete());

    fragmentInfo = m_queue.NextFragment(fragment.size(), fragment.begin());
    EXPECT_EQ(fragmentInfo.state, FragmentationStateEnum::CONTINUATION_FRAGMENT);
    EXPECT_EQ(true,  msg.Append(fragment.data(), fragmentInfo.size));
    EXPECT_EQ(43,    msg.TotalSize());
    EXPECT_EQ(32,    msg.CurrentSize());
    EXPECT_EQ(false, msg.IsComplete());

    fragmentInfo = m_queue.NextFragment(fragment.size(), fragment.begin());
    EXPECT_EQ(fragmentInfo.state, FragmentationStateEnum::CONTINUATION_FRAGMENT);
    EXPECT_EQ(true,  msg.Append(fragment.data(), fragmentInfo.size));
    EXPECT_EQ(43,    msg.TotalSize());
    EXPECT_EQ(40,    msg.CurrentSize());
    EXPECT_EQ(false, msg.IsComplete());

    fragmentInfo = m_queue.NextFragment(fragment.size(), fragment.begin());
    EXPECT_EQ(fragmentInfo.state, FragmentationStateEnum::LAST_FRAGMENT);
    EXPECT_EQ(true,  msg.Append(fragment.data(), fragmentInfo.size));
    EXPECT_EQ(43,    msg.TotalSize());
    EXPECT_EQ(43,    msg.CurrentSize());
    EXPECT_EQ(true,  msg.IsComplete());

    auto& buffer = msg.GetBufferRef();
    EXPECT_EQ(svc_print, buffer.ReadUnVarint());

    const size_t payloadSize = buffer.ReadUnVarint();
    EXPECT_EQ(41, payloadSize);

    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    const std::string receivedAdvice (reinterpret_cast<char*>(buffer.ReadChunk(payloadSize)), payloadSize);
    EXPECT_EQ(receivedAdvice, "We can't stop here!  This is BAT COUNTRY!");

    // Now, test adding more data than we should.

    EXPECT_EQ(false, msg.Append(fragment.data(), fragmentInfo.size));
    EXPECT_EQ(43,    msg.TotalSize());
    EXPECT_EQ(43,    msg.CurrentSize());
    EXPECT_EQ(true,  msg.IsComplete());
}
