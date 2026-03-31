#include "PlayerStateRoller.h"

#include <cassert>
#include <iso646.h>

#include "odamex.h"


PlayerStateRoller::PlayerStateRoller(int currentTic) :
    m_history       (TICRATE),           // 1 second of history.
    m_mostRecentTic (currentTic - 1),   // Setup history to start recording at the given tic.
    m_oldestTic     (currentTic)
{
}

bool PlayerStateRoller::Record(int currentTic, const player_t& player)
{
    if (currentTic == m_mostRecentTic + 1)
    {
        m_mostRecentTic = currentTic;

        if (m_history.size() == m_history.bucket_count())
        {
            m_history.erase(m_oldestTic);
            ++m_oldestTic;
        }
        m_history.emplace(currentTic, player);  // Construct ItemData from the player.
        return true;
    }
    return false;
}

namespace
{
    // ------------------------------------------------
    // The following functions are so naive in their
    // approach and use in order to let the compilers
    // vectorize them easily.
    // ------------------------------------------------

    /// Returns true if any non-zero deltas were produced.
    template <typename ElementType, size_t N>
    void FillDeltaArray(std::array<ElementType, N>&         o_delta,
                        const std::array<ElementType, N>&   i_lhs,
                        const std::array<ElementType, N>&   i_rhs)
    {
        // Don't compile if the array can't store negative values!
        static_assert(std::is_signed<ElementType>() == true);

        for (size_t i = 0; i < o_delta.size(); ++i)
        {
            o_delta[i] = i_lhs[i] - i_rhs[i];
        }
    }

    template <typename ElementType, size_t N>
    void ApplyDeltaArray(std::array<ElementType, N>&         io_array,
                         const std::array<ElementType, N>&   i_delta)
    {
        for (size_t i = 0; i < io_array.size(); ++i)
        {
            // According to optimization tests on godbolt, we get a nice SIMD vectorized
            // summation if we just do the addition without any checks.
            io_array[i] += i_delta[i];
        }
    }

    template <typename ElementType, size_t N>
    bool RequiresCorrection(const std::array<ElementType, N>& i_deltaArray)
    {
        for (size_t i = 0; i < i_deltaArray.size(); ++i)
        {
            if (i_deltaArray[i])
            {
                return true;
            }
        }
        return false;
    }
}

template <typename Callable>
void PlayerStateRoller::Roll(int i_oldTic, Callable&& i_callable)
{
    for (int rollingTic = i_oldTic; rollingTic <= m_mostRecentTic; ++rollingTic)
    {
        auto rollingIter = m_history.find(rollingTic);
        assert(rollingIter != m_history.end());

        i_callable(rollingIter);
    }
}


bool PlayerStateRoller::Resolve(int i_oldTic, const PlayerItemDataType& i_itemData, player_t& io_player)
{
    auto historyIter = m_history.find(i_oldTic);
    if (historyIter != m_history.end())
    {
        PlayerItemDataType deltaItemData;

        FillDeltaArray(deltaItemData.ammo,    i_itemData.ammo,    historyIter->second.ammo);
        FillDeltaArray(deltaItemData.maxammo, i_itemData.maxammo, historyIter->second.maxammo);

        const bool ammoRequiresRoll    = RequiresCorrection(deltaItemData.ammo);
        const bool maxammoRequiresRoll = RequiresCorrection(deltaItemData.maxammo);

        if (ammoRequiresRoll)
        {
            Roll(i_oldTic, [&deltaItemData](auto& rollingIter)
                {
                    ApplyDeltaArray(rollingIter->second.ammo, deltaItemData.ammo);
                });
        }

        if (maxammoRequiresRoll)
        {
            Roll(i_oldTic, [&deltaItemData](auto& rollingIter)
                {
                    ApplyDeltaArray(rollingIter->second.maxammo, deltaItemData.maxammo);
                });
        }

        bool weaponOwnedRequiresRoll = false;
        for (size_t i = 0; i < i_itemData.weaponowned.size(); ++i)
        {
            if (historyIter->second.weaponowned[i] != i_itemData.weaponowned[i])
            {
                weaponOwnedRequiresRoll = true;
                Roll(i_oldTic, [&i_itemData](auto& rollingIter)
                    {
                        // Just copy the whole array and be done with it - this is nothing but bools.
                        rollingIter->second.weaponowned = i_itemData.weaponowned;
                    });
                break;
            }
        }

        const bool readyweaponRequiresRoll = i_itemData.readyweapon != historyIter->second.readyweapon;
        if (readyweaponRequiresRoll)
        {
            Roll(i_oldTic, [&i_itemData](auto& rollingIter)
                {
                    rollingIter->second.readyweapon = i_itemData.readyweapon;
                });
        }

        const bool pendingweaponRequiresRoll = i_itemData.pendingweapon != historyIter->second.pendingweapon;
        if (pendingweaponRequiresRoll)
        {
            Roll(i_oldTic, [&i_itemData](auto& rollingIter)
                {
                    rollingIter->second.pendingweapon = i_itemData.pendingweapon;
                });
        }
    }

    return false;
}
