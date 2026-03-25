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

TEST(CmdLib, ParseISOTime) {
    using namespace std::chrono_literals;
    using namespace std::chrono;
    std::string strtime = "2026-03-2519:12:16Z";
    auto parsed = StrParseISOTime(strtime);
    EXPECT_FALSE(parsed.has_value());
    strtime = "2026-03-25T19:12:16Z";
    parsed = StrParseISOTime(strtime);
    ASSERT_TRUE(parsed.has_value());
    auto tp = parsed.value();
    auto expected = sys_days{2026y/March/25} + 19h + 12min + 16s;
    EXPECT_EQ(tp, expected);
}

TEST(CmdLib, FormatISOTime) {
    using namespace std::chrono_literals;
    using namespace std::chrono;
    auto tp = sys_days{2026y/March/25} + 19h + 12min + 16s;
    EXPECT_EQ(StrFormatISOTime(tp), "2026-03-25T19:12:16Z");
}

TEST(CmdLib, StrToTimeForever) {
    auto expect_forever = [](std::string str){
        auto result = StrToTime(str);
        ASSERT_TRUE(result.has_value());
        EXPECT_FALSE(result.value().has_value());
    };
    expect_forever("forever");
    expect_forever("permanent");
    expect_forever("eternity");
    expect_forever("Eternity");
    expect_forever(" forever ");
}

TEST(CmdLib, StrToTimeErrors) {
    auto expect_err = [](std::string str){
        auto result = StrToTime(str);
        EXPECT_FALSE(result.has_value());
    };
    expect_err("2 forever");
    expect_err("");
    expect_err("     ");
    expect_err("2 huors");
    expect_err("hours 2");
}

// Not sure how to test valid non-forever times, since it depends on current system time
