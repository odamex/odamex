#include <array>

#include "gtest/gtest.h"

#include "LargeMessageQueue.h"

struct LargeMessageQueueFixture : testing::Test
{
    LargeMessageQueue m_queue;
};

TEST_F(LargeMessageQueueFixture, EmptyQueue)
{
    std::array<char, 5> fragment;

    const FragmentationResultType result = m_queue.NextFragment(fragment.size(), fragment.begin());
    EXPECT_EQ(FragmentationStateEnum::NONE,     result.state);
    EXPECT_EQ(0,                                result.size);
}

TEST_F(LargeMessageQueueFixture, BasicSingleMessage)
{
    m_queue.Write(svc_print, "HelloWorld");

    std::array<char, 5> fragment;

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
