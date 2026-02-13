#pragma once

#include <deque>
#include <string>
#include <vector>

#include "i_net.h"

/// This class represents a simple message queue mechanism that maintains its own
/// stack of free message buffers for reuse.
class MessageQueue
{
	public:

		size_t SizeInMessages() const { return m_queue.size(); }

		// Pushing messages
		buf_t& Obtain();
		void Emplace(buf_t& io_str);

		// Using messages.
		const buf_t& Front() const { return m_queue.front(); }

		// Popping messages.
		bool Pop();
		void Clear();

		// Call a given packer method for every message in the queue, popping messages
		// as we go.
		//
		// The packer method must return something that evaluates in a boolean context.
		// If it returns / evaluates true, then the given message is popped and the process
		// repeats until the whole queue is packed and popped.  If it returns / evaluates
		// as false, then processing stops, the given message is not popped, this Pack
		// function returns, and another call to Pack will resume from that message.
		template <typename PackerCallable>
		void Pack(PackerCallable&& packerMethod)
		{
			while (not m_queue.empty())
			{
				if (packerMethod(m_queue.front()))
				{
					PopFromQueueToFreeStack();
				}
				else
				{
					break;
				}
			}
		}

        std::string& GetSerializationBufferRef() { return m_serializationBuffer; }
	protected:

		void PopFromQueueToFreeStack();

		std::deque<buf_t>  m_queue;
		std::vector<buf_t> m_freeStack;
        std::string        m_serializationBuffer;
};
