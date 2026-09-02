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
inline constexpr bool is_scoped_enum_v = [](){
	if constexpr (std::is_enum_v<T>)
		return not std::is_convertible_v<T, std::underlying_type_t<T>>;

	return false;
}();

template <typename T>
struct is_scoped_enum : std::bool_constant<is_scoped_enum_v<T>> {};

template <typename T>
concept EnumClass = is_scoped_enum_v<T>;

template <typename T>
concept CEnum = std::is_enum_v<T> and not is_scoped_enum_v<T>;

// C++23's std::to_underlying
template <Enum E>
[[nodiscard]]
constexpr auto to_underlying(const E e) noexcept
{
	return static_cast<std::underlying_type_t<E>>(e);
}

// concept to help prevent implicit conversions from ints
// in function arguments
template <typename B>
// should this have remove_cvref_t?
concept Bool = std::same_as<B, bool>;

// type-safe wrapper around bool
class SafeBool
{
private:
	bool m_value = false;
public:
	constexpr SafeBool() = default;
	// not explicit so that we can have implicit conversion *from* bool
	// while using the concept to make sure that multiple steps of implicit conversions do not work
	// making it work for *only* bool
	constexpr SafeBool(const Bool auto b) noexcept : m_value(b) {};
	[[nodiscard]] constexpr bool to_bool() const noexcept { return m_value; }
	[[nodiscard]] constexpr explicit operator bool () const noexcept { return m_value; }
    [[nodiscard]] constexpr bool operator==(const SafeBool other) const { return m_value == other.m_value; }
};

// Helper for use of std::visit with lambdas
template<class... Ts>
struct visitor : Ts... { using Ts::operator()...; };
// This shouldn't be needed in C++20, but for some reason macOS builds fail without it
template<class... Ts>
visitor(Ts...) -> visitor<Ts...>;

template <std::integral T>
[[nodiscard]]
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
