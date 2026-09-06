#include "gtest/gtest.h"
#include "istring.h"

using namespace std::string_literals;
using namespace std::string_view_literals;

void PrintTo(const IString& value, std::ostream* os)
{
	*os << '"' << IStringToStdStringView(value) << '"';
}

void PrintTo(const IStringView value, std::ostream* os)
{
	*os << '"' << IStringToStdStringView(value) << '"';
}

TEST(IString, Equality)
{
	IString test{"TEST"};
	EXPECT_EQ(test, "test");
	EXPECT_EQ(test, "TEST");
	EXPECT_STREQ(test.c_str(), "TEST");
	EXPECT_EQ(test, "test"s);
	EXPECT_EQ(test, "test"sv);
	EXPECT_EQ(test, "test"_is);
	EXPECT_EQ(test, "test"_isv);
}

TEST(IString, Find)
{
	IString test{"ABCabcxyzXYZ"};
	EXPECT_EQ(test.find('a'), 0);
	EXPECT_EQ(test.find('A'), 0);
	EXPECT_EQ(test.find('b'), 1);
	EXPECT_EQ(test.find('B'), 1);
	EXPECT_EQ(test.find('c'), 2);
	EXPECT_EQ(test.find('C'), 2);
	EXPECT_EQ(test.find('x'), 6);
	EXPECT_EQ(test.find('X'), 6);
	EXPECT_EQ(test.find('y'), 7);
	EXPECT_EQ(test.find('Y'), 7);
	EXPECT_EQ(test.find('z'), 8);
	EXPECT_EQ(test.find('Z'), 8);
};

TEST(IString, Append)
{
	IString istr{};
	istr += "hello";
	istr += " WORLD";
	istr += ", hElLo"s;
	istr += " odamex"sv;
	EXPECT_EQ(istr, "hello world, hello odamex");
	EXPECT_STREQ(istr.c_str(), "hello WORLD, hElLo odamex");
}

TEST(IString, Hash)
{
	EXPECT_EQ(std::hash<IString>{}("hello"), std::hash<IString>{}("HELLO"));
	EXPECT_EQ(std::hash<IStringView>{}("hello"), std::hash<IStringView>{}("HELLO"));
	EXPECT_EQ(std::hash<IString>{}("hello"), std::hash<IStringView>{}("HELLO"));
	EXPECT_EQ(std::hash<IStringView>{}("hello"), std::hash<IString>{}("HELLO"));
}
