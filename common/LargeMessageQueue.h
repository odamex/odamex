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
//  Queue for Large Messages that need to be fragmented and reassembled
//
//-----------------------------------------------------------------------------

#pragma once

#include "i_net.h"
#include "MessageQueue.h"

enum class FragmentationStateEnum
{
	NONE,                   ///< No large messages are enqueued.
	FIRST_FRAGMENT,         ///< This is the first fragment of a new large message.
	CONTINUATION_FRAGMENT,  ///< The next fragment in the sequence, but not the last.
	LAST_FRAGMENT,          ///< This is the last fragment of a large message.
	ONE_SHOT,               ///< The fragment is the entirety of the message. (both START and END)
	INVALID_FRAGMENT_SIZE,  ///< An invalid fragmentation was requested.
	MESSAGE_OVERFLOW,       ///< The fragment overflowed somehow...  Should never happen.
};

struct FragmentationResultType
{
	FragmentationStateEnum state { FragmentationStateEnum::NONE };
	size_t                 size  { 0 };

	FragmentationResultType(FragmentationStateEnum i_state, size_t i_size) :
	    state { i_state },
	    size  { i_size }
	{
	}
};

class LargeMessageQueue
{
	public:

		// Expose the queue's Write APIs.
		void Write(auto&&... args)
		{
			m_queue.Write(std::forward<decltype(args)>(args)...);
		}

		template <typename IteratorType>
		[[ nodiscard ]]
		FragmentationResultType NextFragment(size_t maxSize, IteratorType outIter)
		{
			if (m_queue.SizeInMessages() == 0)
				return FragmentationResultType {FragmentationStateEnum::NONE, 0};

			if (maxSize == 0)
				return FragmentationResultType {FragmentationStateEnum::INVALID_FRAGMENT_SIZE, 0};

			buf_t& bufferRef = m_queue.Front();

			// Did we get an empty message in the queue somehow?  Pop it off and try the next one.
			if (bufferRef.BytesLeftToRead() == 0)
			{
				m_queue.Pop();
				return NextFragment(maxSize, outIter);
			}

			const size_t numberOfBytesToExtract = std::min(maxSize, bufferRef.BytesLeftToRead());

			const bool  isAtStartOfMessage = bufferRef.TellRead() == 0;
			const byte* dataPtr            = bufferRef.ReadChunk(numberOfBytesToExtract);
			const bool  isAtEndOfMessage   = bufferRef.BytesLeftToRead() == 0;

			// This should never happen due to the above checks.  Still, play it safe.
			if (dataPtr == nullptr)
				return FragmentationResultType {FragmentationStateEnum::MESSAGE_OVERFLOW, 0};

			std::copy(dataPtr, dataPtr + numberOfBytesToExtract, outIter);

			if (isAtEndOfMessage)
			{
				m_queue.Pop();

				if (isAtStartOfMessage)
				{
					return FragmentationResultType {FragmentationStateEnum::ONE_SHOT, numberOfBytesToExtract};
				}
				return FragmentationResultType {FragmentationStateEnum::LAST_FRAGMENT, numberOfBytesToExtract};
			}
			if (isAtStartOfMessage)
			{
				return FragmentationResultType {FragmentationStateEnum::FIRST_FRAGMENT, numberOfBytesToExtract};
			}
			return FragmentationResultType {FragmentationStateEnum::CONTINUATION_FRAGMENT, numberOfBytesToExtract};
		}

	protected:

		static const size_t MAX_LARGE_MESSAGE_SIZE { 64 * 1024 };
		MessageQueue m_queue { MAX_LARGE_MESSAGE_SIZE };
};
