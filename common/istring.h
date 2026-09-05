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

#include <algorithm>
#include <cstring>
#include <string>
#include <string_view>
#include <type_traits>

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

	static constexpr void assign(char& c1, const char& c2) noexcept
	{
		c1 = to_ascii_lowercase(c2);
	}

	static constexpr char* assign(char* ptr, std::size_t count, char c2) noexcept
	{
		if (std::is_constant_evaluated())
		{
			for (std::size_t i = 0; i < count; i++)
			{
				ptr[i] = to_ascii_lowercase(c2);
			}
		}
		else
		{
			std::memset(ptr, to_ascii_lowercase(c2), count);
		}

		return ptr;
	}

	static constexpr char* copy(char* dest, const char* src, std::size_t count) noexcept
	{
		if (count == 0)
			return dest;

		std::ranges::transform(src, src + count, dest, to_ascii_lowercase);
		return dest;
	}

	static constexpr char* move(char* dest, const char* src, std::size_t count) noexcept
	{
		if (count == 0)
			return dest;

		if (std::is_constant_evaluated())
		{
			if (dest > src && dest < src + count)
			{
				for (std::size_t i = count; i-- > 0;)
					dest[i] = to_ascii_lowercase(src[i]);
			}
			else
			{
				copy(dest, src, count);
			}

			return dest;
		}

		std::memmove(dest, src, count * sizeof(char_type));
		std::ranges::transform(dest, dest + count, dest, to_ascii_lowercase);
		return dest;
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

[[nodiscard]]
constexpr IString StdStringToIString(const std::string_view sv)
{
	return {sv.begin(), sv.end()};
}

[[nodiscard]]
constexpr std::string IStringToStdString(const IStringView sv)
{
	return {sv.begin(), sv.end()};
}

[[nodiscard]]
constexpr std::string_view IStringToStdStringView(const IStringView sv)
	noexcept(noexcept(std::string_view(sv.data(), sv.length())))
{
	return {sv.data(), sv.length()};
}

[[nodiscard]]
constexpr auto operator<=>(const IStringView isv, const std::string_view sv)
	noexcept(noexcept(IStringView(sv.data(), sv.length())))
{
	return isv <=> IStringView{sv.data(), sv.length()};
}

[[nodiscard]]
constexpr IString operator""_is(const char* s, const std::size_t l)
{
	return {s, l};
}

[[nodiscard]]
consteval IStringView operator""_isv(const char* s, const std::size_t l)
	noexcept(noexcept(IStringView(s, l)))
{
	return {s, l};
}
