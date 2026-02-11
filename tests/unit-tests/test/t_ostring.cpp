#include "gtest/gtest.h"
#include "m_ostring.h"

TEST(OString, InitEmpty) {
	OString str;
	EXPECT_TRUE(str.empty());
	EXPECT_EQ(str.size(), 0);
	EXPECT_STREQ(str.c_str(), "");
}

TEST(OString, InitFromString) {
	std::string stdStr = "Hello, World!";
	OString str(stdStr);
	EXPECT_FALSE(str.empty());
	EXPECT_EQ(str.size(), stdStr.size());
	EXPECT_EQ(str, stdStr);
	EXPECT_STREQ(str.c_str(), stdStr.c_str());
}

TEST(OString, InitFromCString) {
	const char* cStr = "Hello, World!";
	OString str(cStr);
	EXPECT_FALSE(str.empty());
	EXPECT_EQ(str.size(), strlen(cStr));
	EXPECT_EQ(str, cStr);
	EXPECT_STREQ(str.c_str(), cStr);
}

TEST(OString, Assignment) {
	OString str1 = "Hello";
	OString str2;
	str2 = str1;
	EXPECT_EQ(str1, str2);
	EXPECT_STREQ(str1.c_str(), str2.c_str());

	std::string stdStr = "World";
	str2 = stdStr;
	EXPECT_EQ(str2, stdStr);
	EXPECT_STREQ(str2.c_str(), stdStr.c_str());

	const char* cStr = "!";
	str2 = cStr;
	EXPECT_EQ(str2, cStr);
	EXPECT_STREQ(str2.c_str(), cStr);
}

TEST(OString, AssignOString) {
	OString str1 = "Hello";
	OString str2;
	str2.assign(str1);
	EXPECT_EQ(str1, str2);
	EXPECT_STREQ(str1.c_str(), str2.c_str());
}

TEST(OString, AssignStdString) {
	std::string stdStr = "Hello";
	OString str;
	str.assign(stdStr);
	EXPECT_EQ(str, stdStr);
	EXPECT_STREQ(str.c_str(), stdStr.c_str());
}

TEST(OString, AssignCString) {
	const char* cStr = "Hello";
	OString str;
	str.assign(cStr);
	EXPECT_EQ(str, cStr);
	EXPECT_STREQ(str.c_str(), cStr);
}

TEST(OString, AssignCharAndSize) {
	OString str;
	str.assign(5, 'A');
	EXPECT_EQ(str, "AAAAA");
	EXPECT_STREQ(str.c_str(), "AAAAA");
}

TEST(OString, AssignIterator) {
	std::string stdStr = "Hello";
	OString str;
	str.assign(stdStr.begin(), stdStr.end());
	EXPECT_EQ(str, stdStr);
	EXPECT_STREQ(str.c_str(), stdStr.c_str());
}

TEST(OString, Comparison) {
	OString str1 = "Hello";
	OString str2 = "Hello";
	OString str3 = "World";

	EXPECT_TRUE(str1 == str2);
	EXPECT_FALSE(str1 == str3);

	EXPECT_TRUE(str1 != str3);
	EXPECT_FALSE(str1 != str2);

	EXPECT_TRUE(str1 < str3);
	EXPECT_FALSE(str3 < str1);

	EXPECT_TRUE(str3 > str1);
	EXPECT_FALSE(str1 > str3);

	EXPECT_TRUE(str1 <= str2);
	EXPECT_TRUE(str1 <= str3);

	EXPECT_TRUE(str3 >= str1);
	EXPECT_TRUE(str1 >= str2);
}

TEST(OString, Substr) {
	OString str = "Hello, World!";
	OString substr = str.substr(7, 5);
	EXPECT_EQ(substr, "World");
	EXPECT_STREQ(substr.c_str(), "World");
}

TEST(OString, Find) {
	OString str = "Hello, World!";
	EXPECT_EQ(str.find("World"), 7);
	EXPECT_EQ(str.find("world"), OString::npos);
	EXPECT_STREQ(str.substr(str.find("World"), 5).c_str(), "World");
}

TEST(OString, RFind) {
	OString str = "Hello, World! Hello!";
	EXPECT_EQ(str.rfind("Hello"), 14);
	EXPECT_EQ(str.rfind("hello"), OString::npos);
	EXPECT_STREQ(str.substr(str.rfind("Hello"), 5).c_str(), "Hello");
}

TEST(OString, FindFirstOf) {
	OString str = "Hello, World!";
	EXPECT_EQ(str.find_first_of("o"), 4);
	EXPECT_EQ(str.find_first_of("z"), OString::npos);
	EXPECT_STREQ(str.substr(str.find_first_of("o"), 1).c_str(), "o");
}

TEST(OString, FindLastOf) {
	OString str = "Hello, World!";
	EXPECT_EQ(str.find_last_of("o"), 8);
	EXPECT_EQ(str.find_last_of("z"), OString::npos);
	EXPECT_STREQ(str.substr(str.find_last_of("o"), 1).c_str(), "o");
}

TEST(OString, Clear) {
	OString str = "Hello, World!";
	str.clear();
	EXPECT_TRUE(str.empty());
	EXPECT_EQ(str.size(), 0);
	EXPECT_STREQ(str.c_str(), "");
}