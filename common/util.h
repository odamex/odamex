//-----------------------------------------------------------------------------
//
// $Id:
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
//   Miscellaneous utility stuff
//
//-----------------------------------------------------------------------------

#pragma once

#include <type_traits>
#include <concepts>
#include <iterator>

namespace OUtil
{

template <typename T>
concept Enum = std::is_enum_v<T>;

// C++23's std::is_scoped_enum
template <typename T>
struct is_scoped_enum :
	std::bool_constant<
		std::is_enum_v<T> and
		not std::is_convertible_v<T, std::underlying_type_t<T>>
	>
{};

template <typename T>
inline constexpr bool is_scoped_enum_v = is_scoped_enum<T>::value;

template <typename T>
concept EnumClass = is_scoped_enum_v<T>;

template <typename T>
concept CEnum = std::is_enum_v<T> and not is_scoped_enum_v<T>;

// C++23's std::to_underlying
template <Enum E>
constexpr auto to_underlying(const E e) noexcept
{
	return static_cast<std::underlying_type_t<E>>(e);
}

// prevent implicit conversions from ints
template <typename B>
concept Bool = std::same_as<B, bool>;

// Wrapper for easy iteration over containers in reverse with ranged for loops
template <typename T>
struct reverse_wrapper
{
	T& iterable;
	constexpr auto begin() noexcept(noexcept(std::rbegin(iterable))) { return std::rbegin(iterable); }
	constexpr auto end() noexcept(noexcept(std::rend(iterable))) { return std::rend(iterable); }
};

/**
 * @brief Reverse the iteration in a range-based for loop
 */
template <typename T>
constexpr reverse_wrapper<T> reverse(T& iterable) { return { iterable }; }

// Wrapper for skipping the first N elements in a range-based for loop
template <typename T>
struct drop_wrapper
{
	T& iterable;
	size_t count;

	constexpr auto begin() {
		auto it = std::begin(iterable);
		auto end_it = std::end(iterable);
		for (size_t i = 0; i < count && it != end_it; ++i)
			++it;
		return it;
	}

	constexpr auto end() noexcept(noexcept(std::end(iterable))) { return std::end(iterable); }
};

/**
 * @brief Skip the first `count` elements in a range-based for loop
 */
template <typename T>
constexpr drop_wrapper<T> drop(T& iterable, std::size_t count) { return { iterable, count }; }

// Helper for use of std::visit with lambdas
template<class... Ts>
struct visitor : Ts... { using Ts::operator()...; };
// This shouldn't be needed in C++20, but for some reason macOS builds fail without it
template<class... Ts>
visitor(Ts...) -> visitor<Ts...>;

template <std::integral T>
constexpr auto to_unsigned(T x)
{
	return static_cast<std::make_unsigned_t<T>>(x);
}

// C++23 std::unreachable
[[noreturn]] inline void unreachable()
{
#if defined(_MSC_VER) && !defined(__clang__)
	__assume(false);
#else
	__builtin_unreachable();
#endif
}

}