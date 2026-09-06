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

#include <deque>
#include <string>
#include <vector>

#include "i_net.h"

namespace google::protobuf
{
	class Message;
}

enum class FragmentationStateEnum
{
	NONE,           ///< No large messages are enqueued.
	START,          ///< This is the first fragment of a new large message.
	RUNNING,        ///< Fragmentation of a previously-started large message is ongoing.
	END,            ///< This is the last fragment of a large message.
};

class LargeMessageQueue
{
	public:
		void Write(const google::protobuf::Message& msg);
		void Write(msg_t id, const std::string& msg);

		[[ nodiscard ]]
		FragmentationStateEnum NextFragment(buf_t& o_buffer, size_t maxSize);

	protected:
		std::deque<buf_t>  m_queue;
		std::vector<buf_t> m_freeStack;
		std::string        m_serializationBuffer;
};
