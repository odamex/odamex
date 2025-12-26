// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2006-2025 by The Odamex Team.
// Copyright (C) 2024-2025 by Christian Bernard.
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
//  DoomObjectContainer - replaces arrays for sprnames, mobjinfo, states, etc
//
//-----------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------
// odamex enum definitions with negative indices (similar to id24 specification) are in
// info.h using negative indices prevents overriding during dehacked (though id24 allows
// this)
//
// id24 allows 0x80000000-0x8FFFFFFF (or 0x8000-0x8FFF for 16 bit values) for negative indices
// for source port implementation id24 spec no longer uses enum to define the indices for
// doom objects, but until that is implemented enums will continue to be used
//----------------------------------------------------------------------------------------------

#pragma once

#include "i_system.h"
#include "m_stacktrace.h"
#include <nonstd/span.hpp>
#include <functional>
#include <unordered_map>
#include <vector>
#include <typeinfo>
#include <optional>

// Forward declarations:
struct state_t;
struct mobjinfo_t;

//----------------------------------------------------------------------------------------------
// DoomObjectContainer is a wrapper around std::unordered_map that provides an operator[]
// that cleanly errors with a helpful message when attempts to access a nonexistent element
// are made, as well as insertionmethods tailored to the types of structures the default objects
// are definied within
//----------------------------------------------------------------------------------------------

template <typename ObjType, typename IdxType = int32_t>
class DoomObjectContainer
{
	using LookupTable = std::unordered_map<IdxType, ObjType*>;
	using InOrderTable = std::vector<std::unique_ptr<ObjType>>;
	using DoomObjectContainerType = DoomObjectContainer<ObjType, IdxType>;

	LookupTable m_lookuptable;
	InOrderTable m_inordertable;

public:
	template <typename table_iterator>
	class generic_iterator {
	public:
		using iterator_category = std::forward_iterator_tag;
		using value_type        = std::pair<IdxType, ObjType&>;
		using difference_type   = std::ptrdiff_t;
		using pointer           = value_type*;
		using reference         = value_type&;

		generic_iterator(table_iterator it) : m_it(it) {}

		reference operator*() const {
			m_value.emplace(m_it->first, *m_it->second);
			return *m_value;
		}

		pointer operator->() const {
			m_value.emplace(m_it->first, *m_it->second);
			return &*m_value;
		}

		generic_iterator& operator++() { ++m_it; return *this; }
		generic_iterator operator++(int) { generic_iterator tmp = *this; ++m_it; return tmp; }

		bool operator==(const generic_iterator& other) const { return m_it == other.m_it; }
		bool operator!=(const generic_iterator& other) const { return m_it != other.m_it; }

	private:
		table_iterator m_it;
		mutable std::optional<value_type> m_value;
	};

	using iterator = generic_iterator<typename LookupTable::iterator>;
	using const_iterator = generic_iterator<typename LookupTable::const_iterator>;

	// Construction and Destruction
	explicit DoomObjectContainer() = default;
	explicit DoomObjectContainer(size_t count)  : m_lookuptable(count), m_inordertable() {
		m_inordertable.reserve(count);
	}
	~DoomObjectContainer() = default;

	// Operators
	DoomObjectContainer& operator=(DoomObjectContainer&& other) = default;
	DoomObjectContainer& operator=(const DoomObjectContainer& other)
	{
		if (this == &other)
			return *this;

		clear();
		for (const auto& [idx, ptr] : other.m_lookuptable)
		{
			auto new_ptr = std::make_unique<ObjType>(*ptr);
			m_lookuptable[idx] = new_ptr.get();
			m_inordertable.push_back(std::move(new_ptr));
		}

		return *this;
	}

	ObjType& operator[](int idx)
	{
		iterator it = this->m_lookuptable.find(idx);
		if (it == this->end())
			I_Error("Attempt to access invalid {} at index {}\n{}", typeid(ObjType).name(), idx, M_GetStacktrace());

		return it->second;
	};

	const ObjType& operator[](int idx) const
	{
		const_iterator it = this->m_lookuptable.find(idx);
		if (it == this->end())
			I_Error("Attempt to access invalid {} at index {}\n{}", typeid(ObjType).name(), idx, M_GetStacktrace());

		return it->second;
	};

	// Capacity and Size
	size_t capacity() const { return this->m_inordertable.capacity(); }
	size_t size() const { return this->m_inordertable.size(); }
	void reserve(size_t new_cap)
	{
		this->m_lookuptable.reserve(new_cap);
		this->m_inordertable.reserve(new_cap);
	}

	// Clearing
	void clear()
	{
		this->m_lookuptable.clear();
		this->m_inordertable.clear();
	}

	// Insertion
	ObjType& insert(ObjType obj, IdxType idx)
	{
		m_inordertable.push_back(std::make_unique<ObjType>(obj));
		return *(this->m_lookuptable[idx] = m_inordertable.back().get());
	}

	void insert(nonstd::span<ObjType> objs, IdxType start_idx)
	{
		IdxType idx = start_idx;
		reserve(m_lookuptable.size() + objs.size()); // reserve is not additive
		for (const auto& obj : objs)
			insert(obj, idx++);
	}

	template <typename T = ObjType, typename = std::enable_if_t<std::is_same_v<T, std::string>>>
	void insert(nonstd::span<const char*> objs, IdxType start_idx)
	{
		IdxType idx = start_idx;
		reserve(m_lookuptable.size() + objs.size()); // reserve is not additive
		for (const auto& obj : objs)
			insert(obj == nullptr ? "" : obj, idx++);
	}

	// Lookup
	iterator find(IdxType idx) { return this->m_lookuptable.find(idx); }
	const_iterator find(IdxType idx) const { return this->m_lookuptable.find(idx); }
	bool contains(IdxType idx) const { return this->find(idx) != this->end(); }

	// Iterators
	iterator begin() { return this->m_lookuptable.begin(); }
	iterator end() { return this->m_lookuptable.end(); }
	const_iterator begin() const { return this->m_lookuptable.begin(); }
	const_iterator end() const { return this->m_lookuptable.end(); }
	const_iterator cbegin() const { return this->m_lookuptable.begin(); }
	const_iterator cend() const { return this->m_lookuptable.end(); }

	// Sort vector and rebuild map
	void rebuildMap(std::function<bool(const ObjType&, const ObjType&)> sorter, std::function<IdxType(const ObjType&)> indexgetter)
	{
		std::vector<ObjType*> temp{};
		temp.reserve(m_inordertable.size());
		std::transform(m_inordertable.begin(), m_inordertable.end(), std::back_inserter(temp), [](const auto& p) { return p.get(); });
		std::sort(temp.begin(), temp.end(), [&](const ObjType* lhs, const ObjType* rhs){ return sorter(*lhs, *rhs); });
		m_lookuptable.clear();
		for (ObjType* p : temp)
		{
			const IdxType index = indexgetter(*p);
			if constexpr (std::is_same_v<ObjType, mobjinfo_t> || std::is_same_v<ObjType, mobjinfo_t*>)
			{
				if (m_lookuptable.find(index) == m_lookuptable.end())
					m_lookuptable[index] = p;
			}
			else
			{
				m_lookuptable[index] = p;
			}
		}
	}
};
