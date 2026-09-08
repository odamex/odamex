#include <array>

#include "gtest/gtest.h"

#include "LargeMessageQueue.h"

struct LargeMessageQueueFixture : testing::Test
{
    LargeMessageQueue m_queue;
};

TEST_F(LargeMessageQueueFixture, BasicSingleMessage)
{
    m_queue.Write(svc_print, "HelloWorld");

    std::array<char, 5> fragment;

    const FragmentationStateEnum firstResult = m_queue.NextFragment(fragment.size(), fragment.begin());

    // Please note - the queue put the miniheader on the message.
    EXPECT_EQ(FragmentationStateEnum::FIRST_FRAGMENT,   firstResult);
    EXPECT_EQ((std::array{char(svc_print), char(10), 'H','e','l'}), fragment);

    const FragmentationStateEnum nextResult = m_queue.NextFragment(1, fragment.begin());

    EXPECT_EQ(FragmentationStateEnum::CONTINUATION_FRAGMENT, nextResult);
    EXPECT_EQ((std::array{'l', char(10), 'H', 'e','l'}),     fragment);

    const FragmentationStateEnum thirdResult = m_queue.NextFragment(fragment.size(), fragment.begin());

    EXPECT_EQ(FragmentationStateEnum::CONTINUATION_FRAGMENT, thirdResult);
    EXPECT_EQ((std::array{'o','W','o','r','l'}),             fragment);

    const FragmentationStateEnum lastResult = m_queue.NextFragment(fragment.size(), fragment.begin());

    EXPECT_EQ(FragmentationStateEnum::LAST_FRAGMENT, lastResult);
    EXPECT_EQ((std::array{'d','W','o','r','l'}),     fragment);

    const FragmentationStateEnum emptyResult = m_queue.NextFragment(fragment.size(), fragment.begin());

    EXPECT_EQ(FragmentationStateEnum::NONE,       emptyResult);
    EXPECT_EQ((std::array{'d','W','o','r','l'}),  fragment);
}
