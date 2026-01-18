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