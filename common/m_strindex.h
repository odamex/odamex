// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2021 by Alex Mayfield.
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
//  A class that "indexes" a string with a unique ID  Mostly used to compress
//  text tokens that are often repeated.
//
//-----------------------------------------------------------------------------

#ifndef __M_STRINDEX__
#define __M_STRINDEX__

#include <string>
#include <vector>

#include "doomtype.h"
#include "hashtable.h"

// [AM] It might be possible to save memory from double string allocations,
//      but I don't think it's worth the extra complexity at this point.

class OStringIndexer
{
	std::vector<std::string> m_inverse;
	uint32_t m_firstTransmitIndex;

  public:
	typedef OHashTable<std::string, uint32_t> Indexes;
	Indexes indexes;

	OStringIndexer() : m_firstTransmitIndex(0)
	{
	}

	static OStringIndexer maplistFactory();

	/**
	 * @brief Set the previously created index as the last untransmitted
	 *        index.  All future indexes will be transmitted.
	 */
	void setTransmit()
	{
		m_firstTransmitIndex = m_inverse.size();
	}

	/**
	 * @brief Return true if the given ID should be transmitted, otherwise
	 *        false.
	 */
	bool shouldTransmit(const uint32_t idx) const
	{
		return idx >= m_firstTransmitIndex;
	}

	/**
	 * @brief Return a unique index for a specific string.  If one does not
	 *        exist, a new index will be allocated and returned.
	 */
	uint32_t getIndex(const std::string& str)
	{
		uint32_t index;
		Indexes::iterator it = indexes.find(str);
		if (it == indexes.end())
		{
			index = m_inverse.size();
			indexes.insert(std::make_pair(str, index));
			m_inverse.push_back(str);
		}
		else
		{
			index = it->second;
		}
		return index;
	}

	/**
	 * @brief Return the string for a given index.
	 */
	const std::string& getString(const uint32_t idx)
	{
		static std::string empty;
		if (idx >= m_inverse.size())
			return empty;
		return m_inverse.at(idx);
	}

	/**
	 * @brief Set a specific index to a specific value.
	 */
	void setIndex(const uint32_t idx, const std::string& str)
	{
		indexes.insert(std::make_pair(str, idx));
		if (m_inverse.size() <= idx)
			m_inverse.resize(idx + 1);
		m_inverse.at(idx) = str;
	}
};

#endif
