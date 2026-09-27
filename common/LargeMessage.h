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
//  Class for reassembling large messages
//
//-----------------------------------------------------------------------------

#pragma once

#include "i_net.h"

class LargeMessage
{
	public:
		static const size_t MAX_SIZE { 64 << 10 };      // 64 KB:  The absolute largest message payload we allow.

		bool Restart(size_t totalLength)
		{
			m_buffer.clear();

			if (totalLength <= m_buffer.maxsize())
			{
				m_buffer.setcursize(totalLength);
				return true;
			}
			return false;
		}

		bool Append(const void* data, size_t length)
		{
			if (IsComplete())
				return false;

			if (m_buffer.TellWrite() + length > m_buffer.size())
				return false;

			m_buffer.WriteChunk(data, length);
			return true;
		}

		[[ nodiscard ]]
		size_t TotalSize() const { return m_buffer.size(); }

		[[ nodiscard ]]
		size_t CurrentSize() const { return m_buffer.TellWrite(); }

		[[ nodiscard ]]
		bool IsEmpty() const { return CurrentSize() == 0; }

		[[ nodiscard ]]
		bool IsComplete() const { return CurrentSize() == TotalSize(); }

		[[ nodiscard ]]
		buf_t& GetBufferRef() { return m_buffer; }

	protected:
		buf_t m_buffer { MAX_SIZE };
};
