#pragma once

#include <vector>
#include <unordered_map>

#include "SequenceQueueEntryType.h"

class PacketTable
{
    public:
        struct IntIdentity
        {
            size_t operator()(const int key) const { return key; }
        };

        using HashTableType = std::unordered_map<int, SequenceQueueEntryType, IntIdentity>;
        using iterator      = HashTableType::iterator;

        explicit PacketTable(size_t i_initialSize) :
            m_hashTable(i_initialSize)
        {
            m_hashTable.max_load_factor(3.0f);   // why not...?
        }

        std::pair<iterator, bool>   Acquire(int sequence);
        bool                        Release(iterator pos);

        iterator find(int sequence) { return m_hashTable.find(sequence); }
        //iterator erase(iterator pos) { return m_hashTable.erase(pos); }

        iterator begin() { return m_hashTable.begin(); }
        iterator end()   { return m_hashTable.end();   }

        size_t size() const { return m_hashTable.size(); }

    protected:

        HashTableType m_hashTable;
        std::vector<SequenceQueueEntryType> m_freePackets;

};
