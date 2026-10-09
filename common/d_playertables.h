// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
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
//  Per-player inventory storage for the weapon and ammo tables, whose sizes are
//  only known once DeHackEd patches have been parsed.
//
//-----------------------------------------------------------------------------

#pragma once

#include <algorithm>
#include <type_traits>
#include <vector>

#include "d_items.h"
#include "i_system.h"

struct WeaponSlotTag
{
	static const DehSlotMap& slots() { return ::WeaponSlots; }
	static const char* name() { return "weapon"; }
};

struct AmmoSlotTag
{
	static const DehSlotMap& slots() { return ::AmmoSlots; }
	static const char* name() { return "ammo"; }
};

/// One value per entry of the weapon or ammo table, addressed by the sparse
/// index a DeHackEd patch uses but stored densely in allocation-order slots.
/// Slot order is what goes over the wire and into savegames.
template <typename T, typename SlotTag>
class PlayerTableArray
{
	// std::vector<bool> hands out proxy references, which cannot be returned by
	// the element accessors below, so flags are stored a byte apiece.
	using storage_type = std::conditional_t<std::is_same_v<T, bool>, uint8_t, T>;

  public:
	using value_type = storage_type;

	PlayerTableArray() { resetToTable(); }

	/// Resize to the current table and clear every entry. Table sizes only
	/// change when DeHackEd patches are (re)loaded, which never happens while a
	/// game is in progress.
	void resetToTable()
	{
		m_data.assign(SlotTag::slots().size(), storage_type{});
	}

	storage_type& operator[](int32_t idx) { return m_data[checkedSlot(idx)]; }
	const storage_type& operator[](int32_t idx) const { return m_data[checkedSlot(idx)]; }

	/// Whether an index maps to an entry that actually exists.
	bool has(int32_t idx) const { return SlotTag::slots().slotOf(idx) >= 0; }

	storage_type& atSlot(size_t slot) { return m_data[slot]; }
	const storage_type& atSlot(size_t slot) const { return m_data[slot]; }

	size_t size() const { return m_data.size(); }
	void fill(storage_type value) { std::fill(m_data.begin(), m_data.end(), value); }

	auto begin() { return m_data.begin(); }
	auto end() { return m_data.end(); }
	auto begin() const { return m_data.begin(); }
	auto end() const { return m_data.end(); }

	bool operator==(const PlayerTableArray&) const = default;

	template <typename StreamType>
	friend StreamType& operator<<(StreamType& io_stream, const PlayerTableArray& i_thisRef)
	{
		io_stream << i_thisRef.m_data.size();
		for (size_t slot = 0; slot < i_thisRef.m_data.size(); slot++)
		{
			io_stream << SlotTag::slots().indexAtSlot(slot) << i_thisRef.m_data[slot];
		}
		return io_stream;
	}

	template <typename StreamType>
	friend StreamType& operator>>(StreamType& io_stream, PlayerTableArray& o_thisRef)
	{
		o_thisRef.resetToTable();

		size_t count{0};
		io_stream >> count;

		for (size_t i = 0; i < count; i++)
		{
			int32_t idx{0};
			storage_type value{};
			io_stream >> idx >> value;

			// Entries for indices this table no longer defines are discarded.
			const int slot = SlotTag::slots().slotOf(idx);
			if (slot >= 0)
			{
				o_thisRef.m_data[slot] = value;
			}
		}
		return io_stream;
	}

  private:
	size_t checkedSlot(int32_t idx) const
	{
		const int slot = SlotTag::slots().slotOf(idx);
		if (slot < 0)
		{
			I_Error("Attempt to access player {} inventory at invalid index {}",
			        SlotTag::name(), idx);
		}
		return size_t(slot);
	}

	std::vector<storage_type> m_data;
};

using PlayerWeaponFlags = PlayerTableArray<bool, WeaponSlotTag>;
using PlayerAmmoCounts  = PlayerTableArray<int, AmmoSlotTag>;

template <typename T>
struct is_player_table : std::false_type
{
};

template <typename T, typename SlotTag>
struct is_player_table<PlayerTableArray<T, SlotTag>> : std::true_type
{
};

template <typename T>
concept PlayerTable = is_player_table<std::remove_cvref_t<T>>::value;
