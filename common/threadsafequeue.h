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

	enum class status_t { success, empty, full, closed };

	using value_type = T;

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

	nonstd::expected<T, status_t> try_pop()
	{
		std::unique_lock lock(m_mutex);
		// don't block if queue empty, just return empty status_t
		if (m_queue.empty())
		{
			if (m_closed)
				return nonstd::make_unexpected(status_t::closed);
			else
				return nonstd::make_unexpected(status_t::empty);
		}

		nonstd::expected<T, status_t> val(std::move(m_queue.front()));
		m_queue.pop();
		lock.unlock();
		m_isNotFull.notify_one();
		return val;
	}

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

	status_t try_push(const T& x)
	{
		{
			std::unique_lock lock(m_mutex);
			if (m_closed)
				return status_t::closed;

			if (bound && m_queue.size() >= bound)
				return status_t::full;

			m_queue.push(x);
		}
		m_isNotEmpty.notify_one();
		return status_t::success;
	}

	status_t try_push(T&& x)
	{
		{
			std::unique_lock lock(m_mutex);
			if (m_closed)
				return status_t::closed;

			if (bound && m_queue.size() >= bound)
				return status_t::full;

			m_queue.push(std::move(x));
		}
		m_isNotEmpty.notify_one();
		return status_t::success;
	}

	template <typename... Args,
	          typename = std::enable_if_t<std::is_constructible_v<T, Args&&...>>>
	status_t try_emplace(Args&&... args)
	{
		{
			std::unique_lock lock(m_mutex);
			if (m_closed)
				return status_t::closed;

			if (bound)
				return status_t::full;

			m_queue.emplace(std::forward<Args>(args)...);
		}
		m_isNotEmpty.notify_one();
		return status_t::success;
	}

	void close() noexcept
	{
		{
			std::scoped_lock lock(m_mutex);
			m_closed = true;
		}
		m_isNotFull.notify_all();
		m_isNotEmpty.notify_all();
	}

	bool is_closed() const noexcept
	{
		std::scoped_lock lock(m_mutex);
		return m_closed;
	}
};