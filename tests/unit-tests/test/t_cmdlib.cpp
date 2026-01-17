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