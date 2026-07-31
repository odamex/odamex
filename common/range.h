// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
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
//	Utility type for handling numeric ranges
//
//-----------------------------------------------------------------------------

#pragma once

#include <algorithm>
#include <type_traits>
// #include <ranges>

namespace OUtil
{

struct unchecked_t {};
inline constexpr unchecked_t unchecked{};

enum class clusivity_t {
	inclusive,
	exclusive
};

template <typename T, clusivity_t clusivity, typename = std::enable_if_t<std::is_integral_v<T>>>
// TODO: C++20, use the version below (swap include from <type_traits> to <concepts>)
// template <std::integral T>
class range
{
private:
	T m_low;
	T m_high;

public:
	// lets just make sure theres no bugs about the order being wrong
	constexpr range(T a, T b) noexcept : m_low(std::min(a, b)), m_high(std::max(a, b)) {}
	// but, just in case someone wants to use this in a really hot path and somehow it makes an actual difference
	constexpr range(unchecked_t, T low, T high) noexcept : m_low(low), m_high(high) {}

	constexpr bool contains(T x) const noexcept
	{
		if constexpr (clusivity == clusivity_t::inclusive)
			return m_low <= x && x <= m_high;
		else
			return m_low <= x && x < m_high;
	}

	constexpr T clamp(T x) const noexcept
	{
		if constexpr (clusivity == clusivity_t::inclusive)
			return std::clamp(x, m_low, m_high);
		else
			return std::clamp(x, m_low, m_high - 1);
	}

	// constexpr auto view() const
	// requires (clusivity == clusivity_t::inclusive)
	// {
	// 	// TODO: figure this out, or maybe just don't bother and use iota directly idk
	// }

	// constexpr auto view() const
	// requires (clusivity == clusivity_t::exclusive)
	// {
	// 	return std::views::iota(m_low, m_high);
	// }
};

template <typename T>
using exclusive_range = range<T, clusivity_t::exclusive>;

template <typename T>
using inclusive_range = range<T, clusivity_t::inclusive>;

} // namespace OUtil
