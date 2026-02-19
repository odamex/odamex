#include "gtest/gtest.h"
#include "sarray.h"

TEST(SArray, InitEmpty) {
	SArray<int> array(10);
	EXPECT_EQ(array.size(), 0);
	EXPECT_EQ(array.capacity(), 10);
	EXPECT_TRUE(array.empty());
}

TEST(SArray, InsertAndRetrieve) {
	SArray<int> array(10);
	SArrayId id1 = array.insert(1);
	SArrayId id2 = array.insert(2);
	SArrayId id3 = array.insert(3);

	EXPECT_EQ(array.size(), 3);
	EXPECT_EQ(array[id1], 1);
	EXPECT_EQ(array[id2], 2);
	EXPECT_EQ(array[id3], 3);
}

TEST(SArray, Validate) {
	SArray<int> array(10);
	SArrayId id1 = array.insert(1);
	SArrayId id2 = array.insert(2);

	EXPECT_TRUE(array.validate(id1));
	EXPECT_TRUE(array.validate(id2));

	array.erase(id1);
	EXPECT_FALSE(array.validate(id1));
}

TEST(SArray, Erase) {
	SArray<int> array(10);
	SArrayId id1 = array.insert(1);
	SArrayId id2 = array.insert(2);

	array.erase(id1);
	EXPECT_EQ(array.size(), 1);
	EXPECT_FALSE(array.validate(id1));
	EXPECT_TRUE(array.validate(id2));
}

TEST(SArray, Iterate) {
	SArray<int> array(10);
	array.insert(1);
	array.insert(2);
	array.insert(3);

	int sum = 0;
	for (auto it = array.begin(); it != array.end(); ++it) {
		sum += *it;
	}

	EXPECT_EQ(sum, 6);
}

TEST(SArray, Clear) {
	SArray<int> array(10);
	array.insert(1);
	array.insert(2);
	array.insert(3);

	array.clear();
	EXPECT_EQ(array.size(), 0);
	EXPECT_TRUE(array.empty());
}

TEST(SArray, CopyConstruction) {
	SArray<int> array1(10);
	SArrayId id1 = array1.insert(1);
	SArrayId id2 = array1.insert(2);
	SArrayId id3 = array1.insert(3);

	SArray<int> array2(array1);

	EXPECT_EQ(array1.size(), 3);
	EXPECT_EQ(array2.size(), 3);
	EXPECT_EQ(array1[id1], array2[id1]);
	EXPECT_EQ(array1[id2], array2[id2]);
	EXPECT_EQ(array1[id3], array2[id3]);
}

TEST(SArray, CopyAssignment) {
	SArray<int> array1(10);
	SArrayId id1 = array1.insert(1);
	SArrayId id2 = array1.insert(2);
	SArrayId id3 = array1.insert(3);

	SArray<int> array2(10);
	array2 = array1;

	EXPECT_EQ(array1.size(), 3);
	EXPECT_EQ(array2.size(), 3);
	EXPECT_EQ(array1[id1], array2[id1]);
	EXPECT_EQ(array1[id2], array2[id2]);
	EXPECT_EQ(array1[id3], array2[id3]);
}

TEST(SArray, MoveConstruction) {
	SArray<int> array1(10);
	SArrayId id1 = array1.insert(1);
	SArrayId id2 = array1.insert(2);
	SArrayId id3 = array1.insert(3);

	SArray<int> array2(std::move(array1));

	EXPECT_TRUE(array1.empty());
	EXPECT_EQ(array2.size(), 3);
	EXPECT_EQ(array2[id1], 1);
	EXPECT_EQ(array2[id2], 2);
	EXPECT_EQ(array2[id3], 3);
}

TEST(SArray, MoveAssignment) {
	SArray<int> array1(10);
	SArrayId id1 = array1.insert(1);
	SArrayId id2 = array1.insert(2);
	SArrayId id3 = array1.insert(3);

	SArray<int> array2(10);
	array2 = std::move(array1);

	EXPECT_TRUE(array1.empty());
	EXPECT_EQ(array2.size(), 3);
	EXPECT_EQ(array2[id1], 1);
	EXPECT_EQ(array2[id2], 2);
	EXPECT_EQ(array2[id3], 3);
}

TEST(SArray, CopyAssignmentFromSmaller) {
	SArray<int> array1(10);
	array1.insert(1);
	array1.insert(2);
	array1.insert(3);

	SArray<int> array2(10);
	SArrayId id1 = array2.insert(5);

	array1 = array2;

	EXPECT_EQ(array1.size(), 1);
	EXPECT_EQ(array2.size(), 1);
	EXPECT_EQ(array1[id1], 5);
	EXPECT_EQ(array2[id1], 5);
}

TEST(SArray, MoveAssignmentFromSmaller) {
	SArray<int> array1(10);
	array1.insert(1);
	array1.insert(2);
	array1.insert(3);

	SArray<int> array2(10);
	SArrayId id1 = array2.insert(5);

	array1 = std::move(array2);

	EXPECT_TRUE(array2.empty());
	EXPECT_EQ(array1.size(), 1);
	EXPECT_EQ(array1[id1], 5);
}
