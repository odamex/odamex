#pragma once

#include <deque>
#include <vector>

#include "i_net.h"

class MessageQueue
{
    public:

        /*
        size_t SizeInBytes() const { return std::accumulate(m_queue.first(),
                                                            m_queue.last(),
                                                            0,
                                                            [](size_t x, const buf_t& buf)
                                                            {
                                                                return x + buf.size();
                                                            }); }
        */
        size_t SizeInMessages() const { return m_queue.size(); }

        // Pushing messages
        buf_t& Obtain();
        void Emplace(buf_t& io_str);

        // Using messages.
        const buf_t& Front() const { return m_queue.front(); }

        // Popping messages.
        bool Pop();
        void Clear();

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

    protected:

        void PopFromQueueToFreeStack();

        std::deque<buf_t>  m_queue;
        size_t                   m_size {0};
        std::vector<buf_t> m_freeStack;
};
