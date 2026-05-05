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

template <typename ItemType, size_t N, typename LatchType = ItemType, typename EqualsFunctor = std::equal_to<LatchType> >
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
			m_isArmed = true;
			std::copy(m_refs.begin(),
			          m_refs.end(),
			          m_latchedValues.begin());
		}

		bool EvaluateAsChanged()
		{
			if (m_isArmed)
			{
				m_isArmed = false;
				for (size_t i = 0; i < m_latchedValues.size(); ++i)
				{
					// With the default of std::equal_to, the below ultimately just becomes an operator==
					if (not m_equalsFunctor(m_latchedValues[i], m_refs[i]))
					{
						return true;
					}
				}
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
		std::array<LatchType, N>                        m_latchedValues;
		EqualsFunctor                                   m_equalsFunctor;
		bool                                            m_isArmed;

};
