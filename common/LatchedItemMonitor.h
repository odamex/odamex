// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2026 by Jim Thoenen.
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
//  Utilities for monitoring changes to player attribute items.
//
//-----------------------------------------------------------------------------

#pragma once

#include <algorithm>
#include <array>
#include <functional>
#include <iso646.h>
#include <utility>
#include <vector>

/// Monitor for a single state variable.
template <typename ItemType>
class LatchedItemMonitor
{
	public:
		explicit LatchedItemMonitor(ItemType& i_itemRef) :
			m_itemRef(i_itemRef),
			m_latchedValue(),
			m_isArmed(false)
		{
		}

		// Deleted because we're storing references.
		LatchedItemMonitor(const LatchedItemMonitor&)  = delete;
		LatchedItemMonitor(LatchedItemMonitor&&)       = delete;

		LatchedItemMonitor& operator=(const LatchedItemMonitor&)  = delete;
		LatchedItemMonitor& operator=(LatchedItemMonitor&&)       = delete;

		/// Arms the monitor: begin monitoring the state.
		void Arm()
		{
			m_isArmed = true;
			m_latchedValue = m_itemRef;
		}

		/// Disarms the monitor, ending the monitor session.
		/// Returns true if the state has a different value from whatever it was at the time the monitor was Armed,
		/// false otherwise.
		bool EvaluateAsChanged()
		{
			if (m_isArmed)
			{
				m_isArmed = false;
				return m_itemRef != m_latchedValue;
			}
			return false;
		}

		const ItemType& CurrentValue()
		{
			return m_itemRef;
		}

	protected:
		ItemType& m_itemRef;
		ItemType  m_latchedValue;
		bool      m_isArmed;
};

/// Same thing as LatchedItemMonitor, except that it monitors an entire array of items,
/// and can be configured to use different types for the internal latch and a custom
/// comparison functor for determining value equality between an ItemType and a LatchType.
template <typename ItemType, size_t N, typename LatchType = ItemType, typename EqualsFunctor = std::equal_to<LatchType> >
class LatchedItemArrayMonitor
{
	public:
		explicit LatchedItemArrayMonitor(std::array<ItemType, N>& i_ref) :
			m_refs{ Build(i_ref, std::make_index_sequence<N>{}) }
		{}

		// Deleted because we're storing references.
		LatchedItemArrayMonitor(const LatchedItemArrayMonitor&)  = delete;
		LatchedItemArrayMonitor(LatchedItemArrayMonitor&&)       = delete;

		LatchedItemArrayMonitor& operator=(const LatchedItemArrayMonitor&)  = delete;
		LatchedItemArrayMonitor& operator=(LatchedItemArrayMonitor&&)       = delete;

		void Arm()
		{
			m_isArmed = true;
			std::copy(m_refs.begin(),
			          m_refs.end(),
			          m_latchedValues.begin());
		}

		bool EvaluateAsChanged()
		{
			if (m_isArmed)
			{
				m_isArmed = false;
				for (size_t i = 0; i < m_latchedValues.size(); ++i)
				{
					// With the default of std::equal_to, the below ultimately just becomes an operator==
					if (not m_equalsFunctor(m_latchedValues[i], m_refs[i]))
					{
						return true;
					}
				}
			}
			return false;
		}


	protected:

		template <size_t... Indexes>
		constexpr auto Build(std::array<ItemType, N>& data, std::index_sequence<Indexes...>) -> std::array<std::reference_wrapper<ItemType>, N>
		{
			return { { std::ref(data[Indexes])... } };
		}

		std::array<std::reference_wrapper<ItemType>, N> m_refs;
		std::array<LatchType, N>                        m_latchedValues;
		EqualsFunctor                                   m_equalsFunctor;
		bool                                            m_isArmed;

};

/// Same thing as LatchedItemArrayMonitor, but for a container whose size is only
/// known at runtime. The container itself is referenced rather than its elements,
/// so the monitor stays valid across a resize.
template <typename ContainerType>
class LatchedItemTableMonitor
{
	public:
		explicit LatchedItemTableMonitor(ContainerType& i_ref) :
			m_ref(i_ref),
			m_latchedValues(),
			m_isArmed(false)
		{}

		// Deleted because we're storing references.
		LatchedItemTableMonitor(const LatchedItemTableMonitor&)  = delete;
		LatchedItemTableMonitor(LatchedItemTableMonitor&&)       = delete;

		LatchedItemTableMonitor& operator=(const LatchedItemTableMonitor&)  = delete;
		LatchedItemTableMonitor& operator=(LatchedItemTableMonitor&&)       = delete;

		void Arm()
		{
			m_isArmed = true;
			m_latchedValues.assign(m_ref.begin(), m_ref.end());
		}

		bool EvaluateAsChanged()
		{
			if (m_isArmed)
			{
				m_isArmed = false;
				return m_latchedValues.size() != m_ref.size() ||
				       not std::equal(m_latchedValues.begin(), m_latchedValues.end(),
				                      m_ref.begin());
			}
			return false;
		}

	protected:
		ContainerType&                                     m_ref;
		std::vector<typename ContainerType::value_type>    m_latchedValues;
		bool                                               m_isArmed;
};
