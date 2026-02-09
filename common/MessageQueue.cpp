#include "MessageQueue.h"

buf_t& MessageQueue::Obtain()
{
    if (not m_freeStack.empty())
    {
        m_queue.emplace_back(std::move(m_freeStack.back()));
        m_freeStack.pop_back();
    }
    else
    {
        m_queue.emplace_back(MAX_UDP_PACKET);
    }
    return m_queue.back();
}

void MessageQueue::Emplace(buf_t& io_str)
{
    buf_t& obtainedBuffer = MessageQueue::Obtain();

    obtainedBuffer.swap(io_str);
    io_str.clear();
}

void MessageQueue::PopFromQueueToFreeStack()
{
    m_freeStack.emplace_back(std::move(m_queue.front()));
    m_queue.pop_front();
}

bool MessageQueue::Pop()
{
    if (not m_queue.empty())
    {
        PopFromQueueToFreeStack();
        return true;
    }
    return false;
}

void MessageQueue::Clear()
{
    while (not m_queue.empty())
    {
        PopFromQueueToFreeStack();
    }
}

size_t MessageQueue::Pack(buf_t& io_rawBuf, size_t i_maxBytes)
{
    size_t packedBytes = 0;

    while (not m_queue.empty())
    {
        auto& dataBuf = m_queue.front();
        if (packedBytes + dataBuf.size() > i_maxBytes)
        {
            break;
        }
        packedBytes += dataBuf.size();
        io_rawBuf.WriteChunk(dataBuf.data.get(), dataBuf.size());
        PopFromQueueToFreeStack();
    }

    return packedBytes;
}
