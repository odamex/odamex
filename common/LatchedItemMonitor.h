#pragma once

#include <array>
#include <functional>
#include <iso646.h>
#include <utility>

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
        LatchedItemMonitor(LatchedItemMonitor&&)       = delete;

        LatchedItemMonitor& operator=(const LatchedItemMonitor&)  = delete;
        LatchedItemMonitor& operator=(LatchedItemMonitor&&)       = delete;

        void Arm()
        {
            m_isArmed = true;
            m_latchedValue = m_itemRef;
        }

        bool EvaluateAsChanged()
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

template <typename ItemType, size_t N>
class LatchedItemArrayMonitor
{
    public:
        explicit LatchedItemArrayMonitor(std::array<ItemType, N>& i_ref) :
            m_refs{ Build(i_ref, std::make_index_sequence<N>{}) }
        {}

        LatchedItemArrayMonitor(const LatchedItemArrayMonitor&)  = delete;
        LatchedItemArrayMonitor(LatchedItemArrayMonitor&&)       = delete;

        LatchedItemArrayMonitor& operator=(const LatchedItemArrayMonitor&)  = delete;
        LatchedItemArrayMonitor& operator=(LatchedItemArrayMonitor&&)       = delete;

        void Arm()
        {
            m_isArmed.fill(true);
            m_latchedValues = m_refs;
        }

        bool EvaluateAsChanged(size_t index)
        {
            if (m_isArmed[index])
            {
                m_isArmed[index] = false;
                return m_latchedValues[index] != m_refs[index];
            }
            return false;
        }


    protected:

        template <size_t... Indexes>
        constexpr auto Build(std::array<ItemType, N>& data, std::index_sequence<Indexes...>) -> std::array<std::reference_wrapper<ItemType>, N>
        {
            return { { std::ref(data[Indexes])... } };
        }

        std::array<std::reference_wrapper<ItemType>, N> m_refs;
        std::array<ItemType, N>                         m_latchedValues;
        std::array<bool, N>                             m_isArmed;

};
