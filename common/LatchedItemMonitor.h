#pragma once

#include <iso646.h>

template <typename ItemType>
class LatchedItemMonitor
{
    public:
        explicit LatchedItemMonitor(ItemType& i_itemRef) :
            m_itemRef(i_itemRef),
            m_latchedValue(),
            m_isArmed(false)
        {
        }

        LatchedItemMonitor(const LatchedItemMonitor&)  = delete;
        LatchedItemMonitor(const LatchedItemMonitor&&) = delete;

        LatchedItemMonitor& operator=(const LatchedItemMonitor&)  = delete;
        LatchedItemMonitor& operator=(const LatchedItemMonitor&&) = delete;

        void Arm()
        {
            m_isArmed = true;
            m_latchedValue = m_itemRef;
        }

        bool Fire()
        {
            if (m_isArmed)
            {
                m_isArmed = false;
                return m_itemRef != m_latchedValue;
            }
            return false;
        }

        const ItemType& CurrentValue()
        {
            return m_itemRef;
        }

    protected:
        ItemType& m_itemRef;
        ItemType  m_latchedValue;
        bool      m_isArmed;
};
