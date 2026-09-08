#pragma once

#include "i_net.h"

class LargeMessage
{
	public:
		static const size_t MAX_SIZE { 64 * 1024 };

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

		bool IsComplete() const { return m_buffer.TellWrite() == m_buffer.size(); }

	protected:
		buf_t m_buffer { MAX_SIZE };
};
