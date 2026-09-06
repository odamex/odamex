// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
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
//  Case insensitive specializations of basic_string and basic_string_view
//
//-----------------------------------------------------------------------------

#pragma once

#include <cstdint>
#include <cstring>
#include <string>
#include <string_view>

struct case_insensitive_char_traits : std::char_traits<char>
{
	static constexpr char to_ascii_lowercase(const char c) noexcept
	{
		return c >= 'A' && c <= 'Z' ? static_cast<char>(c + ('a' - 'A')) : c;
	}

	static constexpr int to_ascii_lowercase_int(const int c) noexcept
	{
		return c >= 'A' && c <= 'Z' ? c + ('a' - 'A') : c;
	}

	static constexpr bool eq(const char a, const char b) noexcept
	{
		return to_ascii_lowercase(a) == to_ascii_lowercase(b);
	}

	static constexpr bool lt(const char a, const char b) noexcept
	{
		return static_cast<unsigned char>(to_ascii_lowercase(a)) < static_cast<unsigned char>(to_ascii_lowercase(b));
	}

	static constexpr int compare(const char* s1, const char* s2, std::size_t count) noexcept
	{
		for (std::size_t i = 0; i < count; i++)
		{
			if (lt(s1[i], s2[i]))
				return -1;

			if (lt(s2[i], s1[i]))
				return 1;
		}

		return 0;
	}

	static constexpr const char* find(const char* ptr, std::size_t count, const char& ch) noexcept
	{
		for (std::size_t i = 0; i < count; i++)
		{
			if (eq(ptr[i], ch))
				return ptr + i;
		}

		return nullptr;
	}

	static constexpr int eof() noexcept
	{
		return -1;
	}

	static constexpr int not_eof(const int e) noexcept
	{
		return e == eof() ? 0 : e;
	}

	static constexpr bool eq_int_type(const int a, const int b) noexcept
	{
		static_assert(eof() == to_ascii_lowercase_int(eof()));
		return to_ascii_lowercase_int(a) == to_ascii_lowercase_int(b);
	}
};

using IString     = std::basic_string     <char, case_insensitive_char_traits>;
using IStringView = std::basic_string_view<char, case_insensitive_char_traits>;

#if __cpp_lib_constexpr_string >= 201907L
	#define ISTRING_CONSTEXPR constexpr
#else
	#define ISTRING_CONSTEXPR inline
#endif

[[nodiscard]]
ISTRING_CONSTEXPR IString StdStringToIString(const std::string_view sv)
{
	return {sv.begin(), sv.end()};
}

[[nodiscard]]
ISTRING_CONSTEXPR std::string IStringToStdString(const IStringView sv)
{
	return {sv.begin(), sv.end()};
}

[[nodiscard]]
constexpr IStringView StdStringToIStringView(const std::string_view sv)
	noexcept(noexcept(IStringView{sv.data(), sv.length()}))
{
	return {sv.data(), sv.length()};
}

[[nodiscard]]
constexpr std::string_view IStringToStdStringView(const IStringView sv)
	noexcept(noexcept(std::string_view{sv.data(), sv.length()}))
{
	return {sv.data(), sv.length()};
}

[[nodiscard]]
constexpr auto operator<=>(const IStringView isv, const std::string_view sv)
	noexcept(noexcept(IStringView{sv.data(), sv.length()}))
{
	return isv <=> IStringView{sv.data(), sv.length()};
}

ISTRING_CONSTEXPR IString& operator+=(IString& lhs, std::string_view rhs)
{
	return lhs += IStringView(rhs.data(), rhs.length());
}

ISTRING_CONSTEXPR std::string& operator+=(std::string& lhs, IStringView rhs)
{
	return lhs += std::string_view(rhs.data(), rhs.length());
}

[[nodiscard]]
ISTRING_CONSTEXPR IString operator""_is(const char* s, const std::size_t l)
{
	return {s, l};
}

[[nodiscard]]
consteval IStringView operator""_isv(const char* s, const std::size_t l)
	noexcept(noexcept(IStringView{s, l}))
{
	return {s, l};
}

#undef ISTRING_CONSTEXPR

// maybe we should wrap std::hash<std::string_view> so that the hashes are equivalent?
// the tradeoff is that there's no way to do that without allocating
// the other way of getting that would be normalizing IString but that means we lose
// case preservation
template<>
struct std::hash<IStringView>
{
	constexpr std::size_t operator()(IStringView str) const noexcept
	{
		constexpr std::uint32_t offset = 0x811c9dc5;
		constexpr std::uint32_t prime = 0x1000193;
		std::uint32_t hash = offset;

		for (char value : str)
		{
			hash = hash ^ static_cast<std::uint8_t>(
				IStringView::traits_type::to_ascii_lowercase(value)
			);
			hash *= prime;
		}

		return hash;
	}
};

template<>
struct std::hash<IString>
{
	constexpr std::size_t operator()(const IString& str) const noexcept
	{
		return std::hash<IStringView>{}(str);
	}
};
