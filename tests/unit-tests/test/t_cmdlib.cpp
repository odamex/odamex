#include "gtest/gtest.h"
#include "odamex.h"
#include "cmdlib.h"

TEST(CmdLib, CopyString) {
    const char* str1 = "test";
    const char* str2 = copystring(str1);
    EXPECT_STREQ(str2, "test");
    EXPECT_STREQ(str2, str1);
}

TEST(CmdLib, CopyNullString) {
    const char* str1 = nullptr;
    const char* str2 = copystring(str1);
    EXPECT_STREQ(str2, "");
    EXPECT_EQ(str2[0], '\0');
}

TEST(CmdLib, CopyEmptyString) {
    const char* str1 = "";
    const char* str2 = copystring(str1);
    EXPECT_STREQ(str2, "");
    EXPECT_EQ(str2[0], '\0');
}

TEST(CmdLib, CheckWildcardsNullArgs) {
    const char* str1 = nullptr;
    const char* str2 = "test";
    EXPECT_TRUE(CheckWildcards(str1, str2));
    str1 = "test";
    str2 = nullptr;
    EXPECT_TRUE(CheckWildcards(str1, str2));
    str1 = nullptr;
    str2 = nullptr;
    EXPECT_TRUE(CheckWildcards(str1, str2));
}

TEST(CmdLib, TrimString) {
    // spaces
    std::string test = "hello ";
    EXPECT_EQ(TrimString(test), "hello");
    test = " hello";
    EXPECT_EQ(TrimString(test), "hello");
    test = " hello ";
    EXPECT_EQ(TrimString(test), "hello");
    // tabs
    test = "	hello	";
    EXPECT_EQ(TrimString(test), "hello");
    // newlines
    test = "\nhello\n";
    EXPECT_EQ(TrimString(test), "hello");
    // carriage return
    test = "\rhello\r";
    EXPECT_EQ(TrimString(test), "hello");
    test = "\r\nhello\r\n";
    EXPECT_EQ(TrimString(test), "hello");
    // the other things
    test = "\vhello\v";
    EXPECT_EQ(TrimString(test), "hello");
    test = "\fhello\f";
    EXPECT_EQ(TrimString(test), "hello");
    test = "\rhello\r";
    EXPECT_EQ(TrimString(test), "hello");
    // everything
    test = "\r\v\f \n\thello\n \r\f \v";
    EXPECT_EQ(TrimString(test), "hello");
    // in the middle
    test = "he   llo";
    EXPECT_EQ(TrimString(test), "he   llo");
}
TEST(CmdLib, TicsToShortTimeTenths) {
    // Tenths round down, so the display never overshoots what it counts to.
    EXPECT_EQ("9.9", TicsToShortTime(10 * TICRATE - 1, 10 * TICRATE, true));
    EXPECT_EQ("1.0", TicsToShortTime(TICRATE, 10 * TICRATE, true));
    EXPECT_EQ("0.9", TicsToShortTime(TICRATE - 1, 10 * TICRATE, true));
    EXPECT_EQ("0.1", TicsToShortTime(4, 10 * TICRATE, true));

    // The last few tics before the event read 0.0 - it has not happened yet.
    EXPECT_EQ("0.0", TicsToShortTime(3, 10 * TICRATE, true));
    EXPECT_EQ("0.0", TicsToShortTime(1, 10 * TICRATE, true));
}

TEST(CmdLib, TicsToShortTimeWholeSeconds) {
    // Counting down rounds up, so it never claims less time than is left.
    EXPECT_EQ("11", TicsToShortTime(10 * TICRATE + 1, 10 * TICRATE, true));
    EXPECT_EQ("11", TicsToShortTime(11 * TICRATE, 10 * TICRATE, true));
    EXPECT_EQ("12", TicsToShortTime(11 * TICRATE + 1, 10 * TICRATE, true));

    // Counting up rounds down, so it never claims more time than has elapsed.
    EXPECT_EQ("11", TicsToShortTime(11 * TICRATE, 10 * TICRATE, false));
    EXPECT_EQ("11", TicsToShortTime(12 * TICRATE - 1, 10 * TICRATE, false));
    EXPECT_EQ("12", TicsToShortTime(12 * TICRATE, 10 * TICRATE, false));
}

TEST(CmdLib, TicsToShortTimeThreshold) {
    // The threshold is inclusive, which is what keeps a ceilinged handover from
    // flashing a value for a single tic.
    EXPECT_EQ("10.0", TicsToShortTime(10 * TICRATE, 10 * TICRATE, true));
    EXPECT_EQ("11", TicsToShortTime(10 * TICRATE + 1, 10 * TICRATE, true));

    // A zero threshold means whole seconds only.
    EXPECT_EQ("0", TicsToShortTime(0, 0, true));
    EXPECT_EQ("1", TicsToShortTime(1, 0, true));
    EXPECT_EQ("3", TicsToShortTime(3 * TICRATE, 0, false));

    // Every tenth is reachable and the sequence never rises as tics fall.
    std::string prev;
    for (int t = 10 * TICRATE; t > 0; t--) {
        const std::string cur = TicsToShortTime(t, 10 * TICRATE, true);
        if (!prev.empty())
            EXPECT_LE(std::stod(cur), std::stod(prev)) << "at tic " << t;
        prev = cur;
    }
    EXPECT_EQ("0.0", prev);
}

TEST(CmdLib, TicsToShortTimeNegative) {
    // Negative counts as zero rather than printing a minus sign.
    EXPECT_EQ("0.0", TicsToShortTime(-1, 10 * TICRATE, true));
    EXPECT_EQ("0.0", TicsToShortTime(-1000, 10 * TICRATE, true));
    EXPECT_EQ("0", TicsToShortTime(-1, 0, true));
}
