#include "gtest/gtest.h"
#include "odamex.h"
#include "m_doomobjcontainer.h"

class DoomObjectContainerIterators : public ::testing::Test
{
protected:
	DoomObjectContainer<int> c;

	void SetUp() override
	{
		// 3 elements with indices 10, 11, 12, and values 1, 2, 3
		int arr[] = {1, 2, 3};
		c.insert(arr, 10);
	}
};

TEST_F(DoomObjectContainerIterators, PostFixDerefReturnsEqualObject) {
	auto it = c.begin();
	auto v = *it;
	EXPECT_EQ(v, *it++);
}

TEST_F(DoomObjectContainerIterators, DerefReturnsDistinctObjects)
{
	auto it = c.begin();
	auto&& first = *it;
	++it;
	auto&& second = *it;

	EXPECT_NE(&first, &second);
}

TEST_F(DoomObjectContainerIterators, StructuredBindingAliasing)
{
	auto it = c.begin();

	auto [id1, obj1] = *it;
	++it;
	auto [id2, obj2] = *it;

	EXPECT_NE(id1, id2);
	EXPECT_NE(&id1, &id2);
	EXPECT_NE(obj1, obj2);
	EXPECT_NE(&obj1, &obj2);
}

TEST_F(DoomObjectContainerIterators, StructuredBindingAliasing2)
{
	auto it = c.begin();

	auto&& [id1, obj1] = *it;
	++it;
	auto&& [id2, obj2] = *it;

	EXPECT_NE(id1, id2);
	EXPECT_NE(&id1, &id2);
	EXPECT_NE(obj1, obj2);
	EXPECT_NE(&obj1, &obj2);
}

TEST_F(DoomObjectContainerIterators, NestedIterationIsStable)
{
	int outer_count = 0;
	int inner_total = 0;

	for ([[maybe_unused]] auto&& outer : c)
	{
		++outer_count;

		int inner_count = 0;
		for ([[maybe_unused]] auto&& inner : c)
			++inner_count;

		EXPECT_EQ(inner_count, 3);
		inner_total += inner_count;
	}

	EXPECT_EQ(outer_count, 3);
	EXPECT_EQ(inner_total, 9);

	for (auto&& [outer_idx, outer_obj] : c)
	{
		auto original_idx = outer_idx;
		auto& original_obj = outer_obj;

		for (auto&& inner : c)
		{
			std::ignore = inner.second;
		}

		EXPECT_EQ(outer_idx, original_idx);
		EXPECT_EQ(outer_obj, original_obj);
	}
}

TEST_F(DoomObjectContainerIterators, ReferenceSurvivesIncrement)
{
	auto it = c.begin();
	auto& first = it->first;
	auto firstcopy = first;

	EXPECT_EQ(first, firstcopy);

	++it;
	*it;

	EXPECT_EQ(first, firstcopy);
}

TEST_F(DoomObjectContainerIterators, IteratorsAreIndependent)
{
	auto it1 = c.begin();
	auto it2 = c.begin();
	++it2;

	auto&& a = *it1;
	auto&& b = *it2;

	EXPECT_NE(&a, &b);
}

TEST_F(DoomObjectContainerIterators, IteratorCopyIndependent)
{
	auto it1 = c.begin();
	auto it2 = it1;

	auto&& a = *it1;
	++it2;
	auto&& b = *it2;

	EXPECT_NE(&a, &b);
}

TEST_F(DoomObjectContainerIterators, RangeForStable)
{
	std::vector<int*> seen;

	for (auto&& [id, obj] : c)
		seen.push_back(&obj);

	std::sort(seen.begin(), seen.end());
	auto unique_end = std::unique(seen.begin(), seen.end());
	EXPECT_EQ(unique_end, seen.end());
}

// TODO: Add a lot more tests. This is a very core part of the engine and we want to be sure it works as perfectly as possible