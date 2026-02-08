#pragma once

#include <deque>
#include <string>
#include <vector>

class MessageQueue
{
    public:

        size_t SizeInBytes() const { return m_size; }
        size_t SizeInMessages() const { return m_queue.size(); }

        void Emplace(std::string& io_str);
        const std::string& Front() const { m_queue.front(); }
        bool Pop();

        void Clear();

    protected:

        void PopFromQueueToFreeStack();

        std::deque<std::string>  m_queue;
        size_t                   m_size {0};
        std::vector<std::string> m_freeStack;
};
