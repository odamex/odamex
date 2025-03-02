#include "gtest/gtest.h"
#include "errors.h"
#include "szp.h"

TEST(SZP, Init) {
	szp<int> ptr;
	EXPECT_EQ(static_cast<int*>(ptr), nullptr);
}

TEST(SZP, Assignment) {
	szp<int> ptr;
	ptr.init(new int(42));
	EXPECT_NE(static_cast<int*>(ptr), nullptr);
	EXPECT_EQ(*ptr, 42);
}

TEST(SZP, Dereferencing) {
	szp<int> ptr;
	ptr.init(new int(42));
	EXPECT_EQ(*ptr, 42);

	*ptr = 100;
	EXPECT_EQ(*ptr, 100);
}

TEST(SZP, Reset) {
	szp<int> ptr;
	ptr.init(new int(42));
	EXPECT_NE(static_cast<int*>(ptr), nullptr);
	EXPECT_EQ(*ptr, 42);

	ptr.update_all(nullptr);
	EXPECT_EQ(static_cast<int*>(ptr), nullptr);
}

TEST(SZP, Copy) {
	szp<int> ptr1;
	ptr1.init(new int(42));
	szp<int> ptr2 = ptr1;

	EXPECT_NE(static_cast<int*>(ptr1), nullptr);
	EXPECT_NE(static_cast<int*>(ptr2), nullptr);
	EXPECT_EQ(*ptr1, 42);
	EXPECT_EQ(*ptr2, 42);
}

TEST(SZP, CopyAndUpdate) {
	szp<int> ptr1;
	ptr1.init(new int(42));
	szp<int> ptr2 = ptr1;

	ptr2.update_all(new int(100));

	EXPECT_NE(static_cast<int*>(ptr1), nullptr);
	EXPECT_NE(static_cast<int*>(ptr2), nullptr);
	EXPECT_EQ(*ptr1, 100);
	EXPECT_EQ(*ptr2, 100);
}