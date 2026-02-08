#include "MessageQueue.h"

void MessageQueue::Emplace(std::string& io_str)
{
    if (not m_freeStack.empty())
    {
        m_queue.emplace_back(std::move(m_freeStack.back()));
        m_freeStack.pop_back();
    }
    else
    {
        m_queue.emplace_back();
    }
    m_size += io_str.size();

    m_queue.back().swap(io_str);
    io_str.clear();

    return m_queue.back();
}

void MessageQueue::PopFromQueueToFreeStack()
{
    m_freeStack.emplace_back(std::move(m_queue.front()));
    m_queue.pop_front();

    m_size -= m_freeStack.back().size();
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
