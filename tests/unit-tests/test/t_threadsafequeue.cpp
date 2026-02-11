#include "gtest/gtest.h"
#include "threadsafequeue.h"
#include <thread>
#include <vector>

using IntQueue = OConcurrentQueue<int>;

TEST(OConcurrentQueue, BasicPushPop) {
	IntQueue q;

	EXPECT_TRUE(q.push(42));
	auto val = q.pop();
	ASSERT_TRUE(val.has_value());
	EXPECT_EQ(*val, 42);

	// queue should now be empty
	auto empty_val = q.try_pop();
	ASSERT_FALSE(empty_val.has_value());
	EXPECT_EQ(empty_val.error(), oconqueue_status_t::empty);
}

TEST(OConcurrentQueue, TryPushTryPop) {
	IntQueue q(2); // bounded queue

	EXPECT_TRUE(q.push(1));
	EXPECT_EQ(q.try_push(2), oconqueue_status_t::success);

	// queue is full
	EXPECT_EQ(q.try_push(3), oconqueue_status_t::full);

	// pop one element
	auto val = q.try_pop();
	ASSERT_TRUE(val.has_value());
	EXPECT_EQ(*val, 1);

	// now push should succeed
	EXPECT_EQ(q.try_push(3), oconqueue_status_t::success);
}

TEST(OConcurrentQueue, CloseQueueEmpty) {
	IntQueue q;
	EXPECT_FALSE(q.is_closed());

	q.close();
	EXPECT_TRUE(q.is_closed());

	// after close, push returns false
	EXPECT_FALSE(q.push(1));

	// try_push returns closed
	EXPECT_EQ(q.try_push(1), oconqueue_status_t::closed);

	// pop returns nullopt if empty
	EXPECT_FALSE(q.pop().has_value());

	// try_pop returns closed
	auto val = q.try_pop();
	ASSERT_FALSE(val.has_value());
	EXPECT_EQ(val.error(), oconqueue_status_t::closed);
}

TEST(OConcurrentQueue, CloseQueueNonEmpty) {
	IntQueue q;
	EXPECT_FALSE(q.is_closed());

	q.push(1);
	q.push(2);

	q.close();
	EXPECT_TRUE(q.is_closed());

	// after close, push returns false
	EXPECT_FALSE(q.push(3));

	// try_push returns closed
	EXPECT_EQ(q.try_push(4), oconqueue_status_t::closed);

	// queue still has elements
	auto val1 = q.pop();
	ASSERT_TRUE(val1.has_value());
	EXPECT_EQ(*val1, 1);
	auto val2 = q.try_pop();
	ASSERT_TRUE(val2.has_value());
	EXPECT_EQ(*val2, 2);

	// now try_pop returns closed
	val2 = q.try_pop();
	ASSERT_FALSE(val2.has_value());
	EXPECT_EQ(val2.error(), oconqueue_status_t::closed);
}

TEST(OConcurrentQueue, BlockingPopPushThreaded) {
	IntQueue q(2);

	std::thread producer([&]{
		for (int i = 0; i < 5; ++i) {
			q.push(i);
		}
	});

	std::vector<int> results;
	std::thread consumer([&]{
		for (int i = 0; i < 5; ++i) {
			auto val = q.pop();
			ASSERT_TRUE(val.has_value());
			results.push_back(*val);
		}
	});

	producer.join();
	consumer.join();

	EXPECT_EQ(results.size(), 5);
	for (int i = 0; i < 5; ++i) {
		EXPECT_EQ(results[i], i);
	}
}

TEST(OConcurrentQueue, Emplace) {
	struct Foo {
		int x;
		std::string s;
		Foo(int a, std::string b) : x(a), s(b) {}
	};

	OConcurrentQueue<Foo> q;

	EXPECT_TRUE(q.emplace(123, "hello"));
	auto val = q.pop();
	ASSERT_TRUE(val.has_value());
	EXPECT_EQ(val->x, 123);
	EXPECT_EQ(val->s, "hello");

	EXPECT_EQ(q.try_emplace(456, "goodbye"), oconqueue_status_t::success);
	val = q.pop();
	ASSERT_TRUE(val.has_value());
	EXPECT_EQ(val->x, 456);
	EXPECT_EQ(val->s, "goodbye");
}

TEST(OConcurrentQueue, MoveOnlyType)
{
	struct MoveOnly {
		int value;
		MoveOnly(int v) : value(v) {}

		MoveOnly(const MoveOnly&) = delete;
		MoveOnly& operator=(const MoveOnly&) = delete;

		MoveOnly(MoveOnly&&) noexcept = default;
		MoveOnly& operator=(MoveOnly&&) noexcept = default;
	};

	OConcurrentQueue<MoveOnly> q;

	EXPECT_TRUE(q.emplace(1));
	EXPECT_TRUE(q.push(MoveOnly{2}));
	MoveOnly mo1{3};
	EXPECT_TRUE(q.push(std::move(mo1)));

	EXPECT_EQ(q.try_emplace(4), oconqueue_status_t::success);
	EXPECT_EQ(q.try_push(MoveOnly{5}), oconqueue_status_t::success);
	MoveOnly mo2{6};
	EXPECT_EQ(q.try_push(std::move(mo2)), oconqueue_status_t::success);

	auto val = q.pop();
	ASSERT_TRUE(val.has_value());
	EXPECT_EQ(val->value, 1);

	auto val2 = q.try_pop();
	ASSERT_TRUE(val2.has_value());
	EXPECT_EQ(val2->value, 2);
}