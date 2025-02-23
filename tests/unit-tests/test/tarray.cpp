// #include "gtest/gtest.h"
// #include "odamex.h"
// #include "m_alloc.h"
// #include "tarray.h"

// TEST(TArray, InitEmpty) {
//     TArray<int> array;
//     EXPECT_EQ(array.Size(), 0);
//     EXPECT_EQ(array.Max(), 0);
// }

// TEST(TArray, InitWithSize) {
//     TArray<int> array(10);
//     EXPECT_EQ(array.Size(), 0);
//     EXPECT_EQ(array.Max(), 10);
// }

// TEST(TArray, PushAndRetrieve) {
//     TArray<int> array;
//     array.Push(1);
//     array.Push(2);
//     array.Push(3);

//     EXPECT_EQ(array.Size(), 3);
//     EXPECT_EQ(array[0], 1);
//     EXPECT_EQ(array[1], 2);
//     EXPECT_EQ(array[2], 3);
// }

// TEST(TArray, Pop) {
//     TArray<int> array;
//     array.Push(1);
//     array.Push(2);
//     array.Push(3);

//     int item;
//     EXPECT_TRUE(array.Pop(item));
//     EXPECT_EQ(item, 3);
//     EXPECT_EQ(array.Size(), 2);

//     EXPECT_TRUE(array.Pop(item));
//     EXPECT_EQ(item, 2);
//     EXPECT_EQ(array.Size(), 1);

//     EXPECT_TRUE(array.Pop(item));
//     EXPECT_EQ(item, 1);
//     EXPECT_EQ(array.Size(), 0);

//     EXPECT_FALSE(array.Pop(item));
// }

// TEST(TArray, Resize) {
//     TArray<int> array;
//     array.Push(1);
//     array.Push(2);
//     array.Push(3);

//     array.Resize(5);
//     EXPECT_EQ(array.Size(), 5);

//     array.Resize(2);
//     EXPECT_EQ(array.Size(), 2);
//     EXPECT_EQ(array[0], 1);
//     EXPECT_EQ(array[1], 2);
// }

// TEST(TArray, Clear) {
//     TArray<int> array;
//     array.Push(1);
//     array.Push(2);
//     array.Push(3);

//     array.Clear();
//     EXPECT_EQ(array.Size(), 0);
// }

// TEST(TArray, Grow) {
//     TArray<int> array;
//     array.Push(1);
//     array.Push(2);
//     array.Push(3);

//     array.Grow(10);
//     EXPECT_GE(array.Max(), 13);
// }

// TEST(TArray, Reserve) {
//     TArray<int> array;
//     array.Push(1);
//     array.Push(2);
//     array.Push(3);

//     size_t place = array.Reserve(5);
//     EXPECT_EQ(place, 3);
//     EXPECT_EQ(array.Size(), 8);
// }