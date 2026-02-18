// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2026 by The Odamex Team.
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
//  The packet sequence hash table classes.
//
//-----------------------------------------------------------------------------sx
#pragma once

#include <vector>
#include <unordered_map>

#include "SequenceQueueEntryType.h"

// Forward declaration.  Doing this so that SinglePacketTable and MultiPacketTable can be
// placed at the top of this file, where they're more visible.
template <typename MapType>
class PacketTable;

// This functor is our packet tables' hasher.  It causes the sequence numbers / keys to be
// directly used as the bucket-selector value.
struct PacketIntIdentity
{
	size_t operator()(const int key) const { return static_cast<size_t>(key); }
};

using SinglePacketTable = PacketTable<std::unordered_map     <int, SequenceQueueEntryType, PacketIntIdentity> >;
using MultiPacketTable  = PacketTable<std::unordered_multimap<int, SequenceQueueEntryType, PacketIntIdentity> >;

// This class implements a hash table for storing packet data, keyed on sequence number, and
// uses a stack of free-packet objects for reuse.
template <typename MapType>
class PacketTable
{
	public:
		using iterator = typename MapType::iterator;

		// Constructor.  Set the initial size to the number of sequences you'd like to optimally
		// support out of the gate.  If unsure, 256 is a safe bet.
		explicit PacketTable(size_t i_initialSize) :
			m_hashTable(i_initialSize)
		{
			m_hashTable.max_load_factor(3.0f);   // why not...?
		}

		decltype(auto) Emplace(int sequence)
		{
			if (not m_freePackets.empty())
			{
				auto result = m_hashTable.emplace(sequence, std::move(m_freePackets.back()));
				m_freePackets.pop_back();
				return result;
			}
			else
			{
				auto result = m_hashTable.emplace(sequence, MAX_UDP_PACKET);
				return result;
			}
		}

		bool Erase(iterator pos)
		{
			if (pos != m_hashTable.end())
			{
				m_freePackets.push_back(std::move(pos->second));
				m_freePackets.back().buf.clear();
				m_hashTable.erase(pos);
				return true;
			}
			return false;
		}

		iterator find(int sequence) { return m_hashTable.find(sequence); }

		iterator begin() { return m_hashTable.begin(); }
		iterator end()   { return m_hashTable.end();   }

		size_t size() const { return m_hashTable.size(); }

	protected:

		MapType                             m_hashTable;
		std::vector<SequenceQueueEntryType> m_freePackets;
};
