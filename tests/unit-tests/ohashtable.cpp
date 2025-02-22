#include "gtest/gtest.h"
// yea need to fix the cmake to include this in a better way
#include "../../common/hashtable.h"

TEST(OHashTable, InsertAndRetrieve) {
    OHashTable<int, std::string> table;

    table.insert(std::make_pair(1, "one"));
    table.insert(std::make_pair(2, "two"));
    table.insert(std::make_pair(3, "three"));

    EXPECT_EQ(table[1], "one");
    EXPECT_EQ(table[2], "two");
    EXPECT_EQ(table[3], "three");
}

TEST(OHashTable, UpdateElement) {
    OHashTable<int, std::string> table;

    table.insert(std::make_pair(1, "one"));
    table.insert(std::make_pair(1, "uno"));

    EXPECT_EQ(table[1], "uno");
}

TEST(OHashTable, EraseKey) {
    OHashTable<int, std::string> table;

    table.insert(std::make_pair(1, "one"));
    table.insert(std::make_pair(2, "two"));

    table.erase(1);

    EXPECT_EQ(table.find(1), table.end());
    EXPECT_EQ(table[2], "two");
}

TEST(OHashTable, EraseIterator) {
    OHashTable<int, std::string> table;

    table.insert(std::make_pair(1, "one"));
    table.insert(std::make_pair(2, "two"));

    table.erase(table.find(1));

    EXPECT_EQ(table.find(1), table.end());
    EXPECT_EQ(table[2], "two");
}

TEST(OHashTable, CheckSize) {
    OHashTable<int, std::string> table;

    EXPECT_EQ(table.size(), 0);

    table.insert(std::make_pair(1, "one"));
    table.insert(std::make_pair(2, "two"));

    EXPECT_EQ(table.size(), 2);

    table.erase(1);

    EXPECT_EQ(table.size(), 1);
}

TEST(OHashTable, CheckEmpty) {
    OHashTable<int, std::string> table;

    EXPECT_TRUE(table.empty());

    table.insert(std::make_pair(1, "one"));

    EXPECT_FALSE(table.empty());
}

TEST(OHashTable, ClearTable) {
    OHashTable<int, std::string> table;

    table.insert(std::make_pair(1, "one"));
    table.insert(std::make_pair(2, "two"));

    table.clear();

    EXPECT_TRUE(table.empty());
}

TEST(OHashTable, ClearLargeTable) {
    OHashTable<int, int> table;

    for (int i = 0; i < 10000; i++) {
        table[i] = i;
    }

    EXPECT_EQ(table.size(), 10000);

    table.clear();

    EXPECT_EQ(table.size(), 0);
    EXPECT_TRUE(table.empty());
}

TEST(OHashTable, EraseIterators) {
    OHashTable<int, std::string> table;

    table.insert(std::make_pair(1, "one"));
    table.insert(std::make_pair(2, "two"));

    table.erase(table.begin(), table.end());

    EXPECT_EQ(table.size(), 0);
    EXPECT_TRUE(table.empty());
}

TEST(OHashTable, EraseIteratorsLargeTable) {
    OHashTable<int, int> table;

    for (int i = 0; i < 10000; i++) {
        table[i] = i;
    }

    EXPECT_EQ(table.size(), 10000);

    table.erase(table.begin(), table.end());

    EXPECT_EQ(table.size(), 0);
    EXPECT_TRUE(table.empty());
}