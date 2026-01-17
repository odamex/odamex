#include "gtest/gtest.h"
#include "odamex.h"
#include "olumpname.h"

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

TEST(OLumpName, Substring) {
	OLumpName lumpname = "test1234";
	OLumpName substr = lumpname.substr(0, 7);
	EXPECT_STREQ(substr.c_str(), "TEST123");
	EXPECT_EQ(substr.length(), 7);
}

TEST(OLumpName, AtBoundsCheck) {
	OLumpName lumpname = "test";
	char dummy;
	EXPECT_NO_THROW(dummy = lumpname.at(4));
	EXPECT_ANY_THROW(dummy = lumpname.at(5));
}

TEST(OLumpName, Equality) {
	OLumpName lumpname = "test";
	EXPECT_TRUE(lumpname == "TEST");
	EXPECT_TRUE(lumpname == std::string("TEST"));
	EXPECT_TRUE(lumpname == OLumpName("TEST"));
	EXPECT_TRUE(lumpname == "test");
	EXPECT_TRUE(lumpname == std::string("test"));
	EXPECT_TRUE(lumpname == OLumpName("test"));
	EXPECT_FALSE(lumpname == "TEST1");
	EXPECT_FALSE(lumpname == std::string("TEST1"));
	EXPECT_FALSE(lumpname == OLumpName("TEST1"));
	EXPECT_FALSE(lumpname == "test1");
	EXPECT_FALSE(lumpname == std::string("test1"));
	EXPECT_FALSE(lumpname == OLumpName("test1"));
}

TEST(OLumpName, Inequality) {
	OLumpName lumpname = "test";
	EXPECT_FALSE(lumpname != "TEST");
	EXPECT_FALSE(lumpname != std::string("TEST"));
	EXPECT_FALSE(lumpname != OLumpName("TEST"));
	EXPECT_FALSE(lumpname != "test");
	EXPECT_FALSE(lumpname != std::string("test"));
	EXPECT_FALSE(lumpname != OLumpName("test"));
	EXPECT_TRUE(lumpname != "TEST1");
	EXPECT_TRUE(lumpname != std::string("TEST1"));
	EXPECT_TRUE(lumpname != OLumpName("TEST1"));
	EXPECT_TRUE(lumpname != "test1");
	EXPECT_TRUE(lumpname != std::string("test1"));
	EXPECT_TRUE(lumpname != OLumpName("test1"));
}