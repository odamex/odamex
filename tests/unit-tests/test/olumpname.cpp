#include "gtest/gtest.h"
#include "odamex.h"
#include "olumpname.h"

// todo: test comparison and substrings

TEST(OLumpName, InitEmpty) {
    OLumpName lumpname;
    EXPECT_TRUE(lumpname.empty());
    EXPECT_EQ(lumpname.length(), 0);
}

TEST(OLumpName, InitUppercase) {
    OLumpName lumpname = "test";
    EXPECT_STREQ(lumpname.c_str(), "TEST");
    EXPECT_EQ(lumpname.length(), 4);
}

TEST(OLumpName, InitTruncate) {
    OLumpName lumpname = "test123456";
    EXPECT_STREQ(lumpname.c_str(), "TEST1234");
    EXPECT_EQ(lumpname.length(), 8);
}