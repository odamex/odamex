#include "SequenceQueue.h"

#include "i_net.h"

std::pair<SequenceQueue::iterator, bool> SequenceQueue::Acquire(int sequence)
{
    std::pair<iterator, bool> result;
    if (not m_freePackets.empty())
    {
        result = m_hashTable.emplace(sequence, std::move(m_freePackets.back()));
        m_freePackets.pop_back();
    }
    else
    {
        result = m_hashTable.emplace(sequence, MAX_UDP_PACKET);
    }
    return result;
}

bool SequenceQueue::Release(iterator pos)
{
    if (pos != m_hashTable.end())
    {
        m_freePackets.push_back(std::move(pos->second));
        m_hashTable.erase(pos);
        return true;
    }
    return false;
}
