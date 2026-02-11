// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1998-2006 by Randy Heit (ZDoom 1.22).
// Copyright (C) 2006-2026 by The Odamex Team.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// DESCRIPTION:
//  Queue for passing information between threads
//  Largely based on https://wg21.link/p0260r19
//
//-----------------------------------------------------------------------------

#pragma once

#include <queue>
#include <mutex>
#include <condition_variable>
#include <optional>
#include <nonstd/expected.hpp>

enum class oconqueue_status_t { success, empty, full, closed };

/**
 * @class OConcurrentQueue
 * @brief A bounded or unbounded thread-safe queue
 */
template <typename T,
          typename = std::enable_if_t<std::is_move_constructible_v<T>>>
class OConcurrentQueue final
{
	// TODO: maybe use a fixed-size ring buffer instead?
	std::queue<T> m_queue;
	mutable std::mutex m_mutex;
	std::condition_variable m_isNotEmpty;
	std::condition_variable m_isNotFull;
	bool m_closed = false;
	const size_t bound = 0; // unbounded by default
public:
	OConcurrentQueue() = default;
	explicit OConcurrentQueue(size_t b) : bound(b) {}
	~OConcurrentQueue() = default;
	OConcurrentQueue(const OConcurrentQueue&) = delete;
	OConcurrentQueue(OConcurrentQueue&&) = delete;
	OConcurrentQueue& operator=(const OConcurrentQueue&) = delete;
	OConcurrentQueue& operator=(OConcurrentQueue&&) = delete;

	/**
	 * @brief Pop the front of the queue and return it in an optional.
	 *        If the queue is open, block until the queue has an element
	 *        to pop.
	 *
	 * @return `std::nullopt` if the queue is empty and closed, otherwise
	 *         the element popped from the queue
	 */
	std::optional<T> pop()
	{
		std::unique_lock lock(m_mutex);
		// don't block if queue empty, just return std::nullopt
		if (m_queue.empty())
		{
			if (m_closed)
				return std::nullopt;
			else
				m_isNotEmpty.wait(lock, [this]{ return !m_queue.empty(); });
		}

		std::optional<T> val(std::move(m_queue.front()));
		m_queue.pop();
		lock.unlock();
		m_isNotFull.notify_one();
		return val;
	}

	/**
	 * @brief Non-blocking variant of `pop`.
	 *
	 *        Pop and return the front of the queue if the queue is non-empty.
	 *
	 * @return An `expected` containing either the popped element or a status
	 *         code indicating the reason an element could not be popped.
	 */
	nonstd::expected<T, oconqueue_status_t> try_pop()
	{
		std::unique_lock lock(m_mutex);
		// don't block if queue empty, just return empty oconqueue_status_t
		if (m_queue.empty())
		{
			if (m_closed)
				return nonstd::make_unexpected(oconqueue_status_t::closed);
			else
				return nonstd::make_unexpected(oconqueue_status_t::empty);
		}

		nonstd::expected<T, oconqueue_status_t> val(std::move(m_queue.front()));
		m_queue.pop();
		lock.unlock();
		m_isNotFull.notify_one();
		return val;
	}

	/**
	 * @brief Push an element to the queue via copy-construction.
	 *        If the queue is full, block until there is space available
	 *
	 *
	 * @param x value to enqueue
	 *
	 * @return `true` if `x` was successfully pushed, `false` if the queue is closed
	 */
	bool push(const T& x)
	{
		{
			std::unique_lock lock(m_mutex);
			if (m_closed)
				return false;

			// if queue bound has been reached, block until space is available
			if (bound)
				m_isNotFull.wait(lock, [this]{ return m_queue.size() < bound; });

			m_queue.push(x);
		}
		m_isNotEmpty.notify_one();
		return true;
	}

	/**
	 * @brief Push an element to the queue via move-construction.
	 *        If the queue is full, block until there is space available
	 *
	 *
	 * @param x value to enqueue
	 *
	 * @return `true` if `x` was successfully pushed, `false` if the queue is closed
	 */
	bool push(T&& x)
	{
		{
			std::unique_lock lock(m_mutex);
			if (m_closed)
				return false;

			// if queue bound has been reached, block until space is available
			if (bound)
				m_isNotFull.wait(lock, [this]{ return m_queue.size() < bound; });

			m_queue.push(std::move(x));
		}
		m_isNotEmpty.notify_one();
		return true;
	}

	/**
	 * @brief Construct an element in place at the back of the queue.
	 *        If the queue is full, block until there is space available
	 *
	 * @param args Arguments to pass to the constructor of `T`
	 *
	 * @return `true` if the element was successfully emplaced, `false` if the queue is closed
	 */
	template <typename... Args,
	          typename = std::enable_if_t<std::is_constructible_v<T, Args&&...>>>
	bool emplace(Args&&... args)
	{
		{
			std::unique_lock lock(m_mutex);
			if (m_closed)
				return false;

			// if queue bound has been reached, block until space is available
			if (bound)
				m_isNotFull.wait(lock, [this]{ return m_queue.size() < bound; });

			m_queue.emplace(std::forward<Args>(args)...);
		}
		m_isNotEmpty.notify_one();
		return true;
	}

	/**
	 * @brief Non-blocking variant of `push`
	 *
	 *        Push an element to the queue via copy-construction.
	 *
	 * @param x value to enqueue
	 *
	 * @return status code indicating success or the reason for failure
	 */
	oconqueue_status_t try_push(const T& x)
	{
		{
			std::unique_lock lock(m_mutex);
			if (m_closed)
				return oconqueue_status_t::closed;

			if (bound && m_queue.size() >= bound)
				return oconqueue_status_t::full;

			m_queue.push(x);
		}
		m_isNotEmpty.notify_one();
		return oconqueue_status_t::success;
	}

	/**
	 * @brief Non-blocking variant of `push`
	 *
	 *        Push an element to the queue via move-construction.
	 *
	 * @param x value to enqueue
	 *
	 * @return status code indicating success or the reason for failure
	 */
	oconqueue_status_t try_push(T&& x)
	{
		{
			std::unique_lock lock(m_mutex);
			if (m_closed)
				return oconqueue_status_t::closed;

			if (bound && m_queue.size() >= bound)
				return oconqueue_status_t::full;

			m_queue.push(std::move(x));
		}
		m_isNotEmpty.notify_one();
		return oconqueue_status_t::success;
	}

	/**
	 * @brief Non-blocking variant of `emplace`
	 *
	 *        Construct an element in place at the back of the queue.
	 *
	 *
	 * @param args Arguments to pass to the constructor of `T`
	 *
	 * @return status code indicating success or the reason for failure
	 */
	template <typename... Args,
	          typename = std::enable_if_t<std::is_constructible_v<T, Args&&...>>>
	oconqueue_status_t try_emplace(Args&&... args)
	{
		{
			std::unique_lock lock(m_mutex);
			if (m_closed)
				return oconqueue_status_t::closed;

			if (bound)
				return oconqueue_status_t::full;

			m_queue.emplace(std::forward<Args>(args)...);
		}
		m_isNotEmpty.notify_one();
		return oconqueue_status_t::success;
	}

	/**
	 * @brief Close the queue so that no more elements can be pushed
	 */
	void close() noexcept
	{
		{
			std::scoped_lock lock(m_mutex);
			m_closed = true;
		}
		m_isNotFull.notify_all();
		m_isNotEmpty.notify_all();
	}

	/**
	 * @return `true` if the queue has been closed
	 */
	bool is_closed() const noexcept
	{
		std::scoped_lock lock(m_mutex);
		return m_closed;
	}
};