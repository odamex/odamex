// Emacs style mode select   -*- C++ -*-
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
//   Type-safe bitflags
//   Inspired by https://www.foonathan.net/2017/03/implementation-challenge-bitmask/
//
//-----------------------------------------------------------------------------

// ============================================================================
//
// OFlags is a type-safe bitflag type that prevents common mistakes
// by splitting sets of flags into 3 conceptual catgories:
//
// 1. A flag set - This is OFlags itself. Used for storing flags in
//    an object. Flags can be checked, set, toggled, or cleared.
//
// 2. A flag combination - Used to set, toggle, and check flags in a flag set.
//    This is the type obtained by |ing individual flags togther.
//
// 3. A flag mask - Used to clear flags in a flag set. Obtained from ~ing
//    a combo or an individual flag.
//
// Converting between these types is done with the `mask` and `combo'
// functions. 'mask' converts a set, combo, or single flag into a mask, and
// 'combo' converts a set or a mask into a combo.
//
// ----------------------------------------------------------------------------
// Other notes:
//
// OFlags can only be templated on enum classes. Allowing plain enums
// would worsen its type-safety. If migrating old code, 'using enum'
// might be helpful.
//
// OFlags<E> requires that there is a function
// 'constexpr E enable_bitflag_operators(E)' that returns the variant
// of E corresponding to the highest bit used.
//
// ----------------------------------------------------------------------------
// Examples:
//
// enum class Flags : uint8_t
// {
//     Flag1 = 0x01,
//     Flag2 = 0x02,
//     Flag3 = 0x04,
// };
// using enum Flags;
// constexpr Flags enable_bitflag_operators(Flags) { return Flag3; }
//
// OFlags<Flags> flags = OFlags<Flags>::all_set();
//
// flags &= ~Flag1;       // clears Flag1
// flags &= Flag1;        // error: &= requires a mask
// flags &= mask(Flag1);  // clears all flags except for Flag1
//
// flags |= Flag1;        // sets Flag1
// flags |= ~Flag1;       // error: |= requires a combo or single flag
// flags |= combo(~Flag1) // sets all flags except Flag1
//
// // operator & with a single flag or combo returns a boolean
// // indicating whether or not the flag(s) is/are set
// if (flags & (Flag1|Flag3))
//
// // operator & with a mask returns a new flag_set with those flags cleared
// if (flags & ~(Flag1|Flag3)) // error: mask is not convertible to bool
//
// // maybe we want to check if two flag sets have any of the same flags set
// OFlags<Flags> flags2;
// // we can just convert one of them to a combo
// if (flags & combo(flags2))
//
// // similarly, one can be converted to a mask if needed
// // e.g. to check if a particular flag is set in both
// if ((flags & mask(flags2)) & Flag2)
//
// ============================================================================

#pragma once

#include <concepts>
#include <type_traits>
#include <stdint.h>
#include "util.h"

namespace OUtil
{

struct noflag_t {};
inline constexpr noflag_t noflag;

}

namespace flags_detail
{

using OUtil::to_underlying;
using OUtil::SafeBool;
using OUtil::noflag_t;
using OUtil::noflag;

template <typename T>
concept FlagEnum =
	OUtil::EnumClass<T> &&
	requires (T t) { enable_bitflag_operators(t); };

// TODO: C++26 someday in the far future this would be a job for reflection
// (enabling could be an annotation too)
template <typename T>
concept FlagEnumWithMax =
	FlagEnum<T> &&
	requires (T t) {
		{ enable_bitflag_operators(t) } -> std::same_as<T>;
	} &&
    requires {
        []() consteval {
            enable_bitflag_operators(T{});
        }();
    };

// TODO: C++23, pass `this` by value explicitly in non-modifying cases
// https://devblogs.microsoft.com/cppblog/cpp23-deducing-this/#pass-this-by-value
// can also use `this auto&& self` and forego the static_casting to Derived
// https://devblogs.microsoft.com/cppblog/cpp23-deducing-this/#crtp
template <FlagEnum E, typename Derived>
class flags_impl
{
protected:
	using underlying = std::underlying_type_t<E>;
	underlying m_value{0};

	template <FlagEnum, typename>
	friend class flags_impl;

	constexpr explicit flags_impl(const underlying v) noexcept : m_value(v) {}

	constexpr Derived& derived_this() noexcept
	{
		return static_cast<Derived&>(*this);
	}

	constexpr const Derived& derived_this() const noexcept
	{
		return static_cast<const Derived&>(*this);
	}

public:
	[[nodiscard]]
	static constexpr Derived all_set() noexcept
	requires FlagEnumWithMax<E>
	{
		using unsigned_underlying = std::make_unsigned_t<underlying>;
		return Derived{static_cast<underlying>((static_cast<unsigned_underlying>(enable_bitflag_operators(E{})) << 1) - unsigned_underlying{1})};
	}

	[[nodiscard]]
	static constexpr Derived none_set() noexcept
	{
		return Derived{};
	}

	[[nodiscard]]
	static constexpr Derived unsafe_from_int(const std::same_as<underlying> auto v) noexcept
	{
		return Derived{v};
	}

	constexpr flags_impl() noexcept = default;
	constexpr explicit flags_impl(const E e) noexcept : m_value(to_underlying(e)) {}
	template <typename Derived2>
	constexpr explicit flags_impl(const flags_impl<E, Derived2> other) noexcept : m_value(other.m_value) {}

	constexpr Derived& clear(const E e) noexcept
	{
		m_value &= ~to_underlying(e);
		return derived_this();
	}

	constexpr Derived& clear() noexcept
	{
		m_value = 0;
		return derived_this();
	}

	constexpr Derived& set(const E e) noexcept
	{
		m_value |= to_underlying(e);
		return derived_this();
	}

	constexpr Derived& set(const E e, const SafeBool value) noexcept
	{
		if (value)
			set(e);
		else
			clear(e);

		return derived_this();
	}

	constexpr Derived& toggle(const E e) noexcept
	{
		m_value ^= to_underlying(e);
		return derived_this();
	}

	constexpr Derived& toggle() noexcept
	{
		m_value ^= all_set().to_int();
		return derived_this();
	}

	constexpr Derived& bitwise_or(const Derived other) noexcept
	{
		m_value |= other.m_value;
		return derived_this();
	}

	constexpr Derived& bitwise_and(const Derived other) noexcept
	{
		m_value &= other.m_value;
		return derived_this();
	}

	constexpr Derived& bitwise_xor(const Derived other) noexcept
	{
		m_value ^= other.m_value;
		return derived_this();
	}

	[[nodiscard]]
	constexpr Derived clear(const E e) const noexcept
	{
		// ugh, cast needed cause of integer promotion
		return Derived{static_cast<underlying>(m_value & ~to_underlying(e))};
	}

	[[nodiscard]]
	constexpr Derived clear() const noexcept
	{
		return none_set();
	}

	[[nodiscard]]
	constexpr Derived set(const E e) const noexcept
	{
		// ugh, cast needed cause of integer promotion
		return Derived{static_cast<underlying>(m_value | to_underlying(e))};
	}

	[[nodiscard]]
	constexpr Derived set(const E e, const SafeBool value) const noexcept
	{
		if (value)
			return set(e);

		return clear(e);
	}

	[[nodiscard]]
	constexpr Derived toggle(const E e) const noexcept
	{
		// ugh, cast needed cause of integer promotion
		return Derived{static_cast<underlying>(m_value ^ to_underlying(e))};
	}

	[[nodiscard]]
	constexpr Derived toggle() const noexcept
	{
		// ugh, cast needed cause of integer promotion
		return Derived{static_cast<underlying>(m_value ^ all_set().m_value)};
	}

	[[nodiscard]]
	constexpr Derived bitwise_or(const Derived other) const noexcept
	{
		// ugh, cast needed cause of integer promotion
		return Derived{static_cast<underlying>(m_value | other.m_value)};
	}

	[[nodiscard]]
	constexpr Derived bitwise_and(const Derived other) const noexcept
	{
		// ugh, cast needed cause of integer promotion
		return Derived{static_cast<underlying>(m_value & other.m_value)};
	}

	[[nodiscard]]
	constexpr Derived bitwise_xor(const Derived other) const noexcept
	{
		// ugh, cast needed cause of integer promotion
		return Derived{static_cast<underlying>(m_value ^ other.m_value)};
	}

	[[nodiscard]]
	constexpr SafeBool is_set(const E e) const noexcept
	{
		return {(m_value & to_underlying(e)) != underlying{0}};
	}

	[[nodiscard]]
	constexpr SafeBool all() const noexcept
	{
		return {m_value == all_set().m_value};
	}

	[[nodiscard]]
	constexpr SafeBool any() const noexcept
	{
		return {m_value != none_set().m_value};
	}

	[[nodiscard]]
	constexpr SafeBool none() const noexcept
	{
		return {m_value == none_set().m_value};
	}

	[[nodiscard]]
	constexpr underlying to_int() const noexcept
	{
		return m_value;
	}

	[[nodiscard]]
	constexpr explicit operator underlying() const noexcept
	{
		return m_value;
	}

	[[nodiscard]]
	friend constexpr bool operator==(const Derived flags, noflag_t) noexcept
	{
		return flags.m_value == none_set().m_value;
	}

	[[nodiscard]]
	friend constexpr bool operator==(const Derived lhs, const Derived rhs) noexcept
	{
		return lhs.m_value == rhs.m_value;
	}

	// TODO: this is just temporary and should be deleted as soon as its unused
	// this is here for limiting the impact on the mapinfo parser until merging with the
	// type safe mapinfo pr
	[[nodiscard, deprecated]]
	constexpr underlying& data() noexcept
	{
		return m_value;
	}

};

template <FlagEnum E>
class flag_combo final : public flags_impl<E, flag_combo<E>>
{
public:
	using flags_impl<E, flag_combo<E>>::flags_impl;
};

template <FlagEnum E>
class flag_mask final : public flags_impl<E, flag_mask<E>>
{
public:
	using flags_impl<E, flag_mask<E>>::flags_impl;
};

template <typename T, typename E>
concept FlagOrCombo =
	FlagEnum<E> &&
	(std::same_as<std::remove_cvref_t<T>, E> ||
	 std::same_as<std::remove_cvref_t<T>, flag_combo<E>>);

template <FlagEnum E>
class flag_set final : public flags_impl<E, flag_set<E>>
{
private:
	using base = flags_impl<E, flag_set<E>>;
	using base::base;
public:
	constexpr flag_set() noexcept = default;
	constexpr flag_set(noflag_t) noexcept : flag_set() {};
	constexpr flag_set(const FlagOrCombo<E> auto c) noexcept : base(c) {};

	constexpr flag_set& operator|=(const FlagOrCombo<E> auto c) noexcept
	{
		return this->bitwise_or(flag_set<E>{c});
	}

	constexpr flag_set& operator^=(const FlagOrCombo<E> auto c) noexcept
	{
		return this->bitwise_xor(flag_set<E>{c});
	}

	constexpr flag_set& operator&=(const flag_mask<E> m) noexcept
	{
		return this->bitwise_and(flag_set<E>{m});
	}

	constexpr flag_set operator~() const noexcept
	{
		return this->toggle();
	}

	// TODO: finish this up
	// left unfinished until we know the exact needs for this
	// template <FlagEnum E2>
	// [[nodiscard]]
	// static constexpr flag_set<E> unsafe_from_other(const flag_set<E2> other)
	// {
	// 	// should we enforce that they have the same underlying type?
	// 	return flag_set<E>::unsafe_from_int(other.to_int());
	// 	// or should we not?
	// 	return flag_set<E>::unsafe_from_int(static_cast<base::underlying>(other.to_int()));
	// }
};

template <FlagEnum E>
[[nodiscard]]
constexpr flag_combo<E> combo(const flag_mask<E> m) noexcept
{
	return flag_combo<E>{m};
}

template <FlagEnum E>
[[nodiscard]]
constexpr flag_combo<E> combo(const flag_set<E> s) noexcept
{
	return flag_combo<E>{s};
}

template <FlagEnum E>
[[nodiscard]]
constexpr flag_mask<E> mask(const flag_combo<E> s) noexcept
{
	return flag_mask<E>{s};
}

template <FlagEnum E>
[[nodiscard]]
constexpr flag_mask<E> mask(const flag_set<E> s) noexcept
{
	return flag_mask<E>{s};
}

template <FlagEnum E>
[[nodiscard]]
constexpr flag_combo<E> operator|(const flag_combo<E> c, const E e) noexcept
{
	return c.set(e);
}

template <FlagEnum E>
[[nodiscard]]
constexpr flag_combo<E> operator|(const flag_combo<E> c1, const flag_combo<E> c2) noexcept
{
	return c1.bitwise_or(c2);
}

template <FlagEnum E>
[[nodiscard]]
constexpr flag_set<E> operator|(const flag_set<E> s, const FlagOrCombo<E> auto c) noexcept
{
	return s.bitwise_or(flag_set<E>{c});
}

template <FlagEnum E>
[[nodiscard]]
constexpr SafeBool operator&(const flag_set<E> s, const E e) noexcept
{
	return s.is_set(e);
}

template <FlagEnum E>
[[nodiscard]]
constexpr SafeBool operator&(const flag_set<E> s, const flag_combo<E> c) noexcept
{
	return {static_cast<bool>(combo(s).bitwise_and(c).to_int())};
}

template <FlagEnum E>
[[nodiscard]]
constexpr flag_set<E> operator&(const flag_set<E> s, const flag_mask<E> m) noexcept
{
	return s.bitwise_and(flag_set<E>{m});
}

template <FlagEnum E>
[[nodiscard]]
constexpr flag_mask<E> operator&(const flag_mask<E> m1, const flag_mask<E> m2) noexcept
{
	return m1.bitwise_and(m2);
}

template <FlagEnum E>
[[nodiscard]]
constexpr flag_combo<E> operator~(const flag_mask<E> m) noexcept
{
	return flag_combo<E>{m}.toggle();
}

template <FlagEnum E>
[[nodiscard]]
constexpr flag_mask<E> operator~(const flag_combo<E> c) noexcept
{
	return flag_mask<E>{c}.toggle();
}

template <FlagEnum E>
[[nodiscard]]
constexpr flag_set<E> operator^(const flag_set<E> s, const FlagOrCombo<E> auto c) noexcept
{
	return s.bitwise_xor(flag_set<E>{c});
}

template <FlagEnum E>
[[nodiscard]]
constexpr bool operator==(const flag_set<E> s, const FlagOrCombo<E> auto c) noexcept
{
	return combo(s) == c;
}

template <FlagEnum E>
[[nodiscard]]
constexpr bool operator==(const flag_combo<E> c, const E e) noexcept
{
	return c == flag_combo<E>{e};
}

}

template <flags_detail::FlagEnumWithMax E>
[[nodiscard]]
constexpr flags_detail::flag_mask<E> operator~(const E e) noexcept
{
	return flags_detail::flag_mask<E>::all_set().clear(e);
}

template <flags_detail::FlagEnum E>
[[nodiscard]]
constexpr flags_detail::flag_combo<E> operator|(const E e1, const E e2) noexcept
{
	return flags_detail::flag_combo<E>{e1} | e2;
}

template <flags_detail::FlagEnum E>
[[nodiscard]]
constexpr flags_detail::flag_mask<E> mask(const E e) noexcept
{
	return flags_detail::flag_mask<E>{e};
}

template <flags_detail::FlagEnum E>
using OFlags = flags_detail::flag_set<E>;
