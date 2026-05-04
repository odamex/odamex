#include "PlayerStateRoller.h"

#include <cassert>
#include <iso646.h>

#include "odamex.h"


PlayerStateRoller::PlayerStateRoller() :
	m_history       (TICRATE * 2),  // at least 2 seconds of history
	m_mostRecentTic (-1),           // Setup history to start recording at the given tic.
	m_oldestTic     (-1)
{
}

void PlayerStateRoller::Clear()
{
	m_history.clear();
	m_mostRecentTic = -1;
	m_oldestTic     = -1;
}

RollerRecordResultEnum PlayerStateRoller::Record(int currentTic, const player_t& player)
{
	if (m_mostRecentTic == -1)
	{
		m_mostRecentTic = currentTic - 1;
		m_oldestTic     = currentTic;
	}

	if (currentTic == ExpectedTic())
	{
		m_mostRecentTic = currentTic;

		// This check ensures that we're not going to grow the size of the hash table
		// because we age-out elements at the moment where a new one would cause a
		// reallocation and re-hash.  It's important to note that between this check
		// and the super-simple key-identity "hasher", we've essentially turned the
		// unordered_map into a fixed-size circular buffer.
		//
		if (m_history.size() == m_history.bucket_count())
		{
			m_history.erase(m_oldestTic);
			++m_oldestTic;
		}
		m_history.emplace(currentTic, player);  // Construct ItemData from the player.
		return RollerRecordResultEnum::SUCCESS;
	}
	else if (currentTic == m_mostRecentTic)
	{
		// We have a special case here that we only see during netdemo setup and initial playback:
		// We see two actual sim steps back-to-back with the same gametic.  In theory this is
		// allowable, but it's borderline.  Therefore we allow the most recent, current-time
		// sample to be replaced with another one from the same time, but we warn about it because
		// it should NOT be seen anywhere except in special circumstances.

		PlayerItemDataType& existingSample = m_history[currentTic];
		PlayerItemDataType  newSample {player};

		if (existingSample != newSample)
		{
			existingSample = newSample;
			return RollerRecordResultEnum::CURRENT_REPLACED;
		}
		return RollerRecordResultEnum::SUCCESS;
	}

	return currentTic > m_mostRecentTic ? RollerRecordResultEnum::INVALID_TIC_FUTURE : RollerRecordResultEnum::INVALID_TIC_PAST;
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
			io_array[i] = std::max(i_delta[i] + io_array[i], 0);
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
		//assert(rollingIter != m_history.end());

		i_callable(rollingIter);
	}
}

void PlayerStateRoller::ApplyMostRecentToPlayer(player_t& io_player)
{
	auto mostRecentIter = m_history.find(m_mostRecentTic);
	//assert(mostRecentIter != m_history.end());

	mostRecentIter->second.ToPlayer(io_player);
}

bool PlayerStateRoller::RollbackAmmo(HistoryTableType::iterator i_historyIter, const std::array<int, NUMAMMO>& i_ammo)
{
	std::array<int, NUMAMMO> ammoDelta;
	FillDeltaArray(ammoDelta, i_ammo, i_historyIter->second.ammo);

	if (RequiresCorrection(ammoDelta))
	{
		Roll(i_historyIter->first, [&ammoDelta](auto& rollingIter)
			{
				ApplyDeltaArray(rollingIter->second.ammo, ammoDelta);
			});
		return true;
	}
	return false;
}

bool PlayerStateRoller::RollbackMaxAmmo(HistoryTableType::iterator i_historyIter, const std::array<int, NUMAMMO>& i_maxAmmo)
{
	std::array<int, NUMAMMO> deltaMaxAmmo;
	FillDeltaArray(deltaMaxAmmo, i_maxAmmo, i_historyIter->second.maxammo);

	if (RequiresCorrection(deltaMaxAmmo))
	{
		Roll(i_historyIter->first, [&i_maxAmmo] (auto& rollingIter)
			{
				// Please note that we don't apply the delta across the history of maxammo.
				// The reason for that is if we have, say, an off-by-one tic prediction of
				// a pickup that affects maxammo, we don't want to wind up with a double-
				// value maxammo.
				rollingIter->second.maxammo = i_maxAmmo;
			});
		return true;
	}
	return false;
}

bool PlayerStateRoller::RollbackWeaponOwned(HistoryTableType::iterator i_historyIter, const std::array<bool, NUMWEAPONS>& i_weaponOwned)
{
    if (i_historyIter->second.weaponowned != i_weaponOwned)
    {
		Roll(i_historyIter->first, [&i_weaponOwned](auto& rollingIter)
			{
				rollingIter->second.weaponowned = i_weaponOwned;
			});
        return true;
    }
    return false;
}

bool PlayerStateRoller::RollbackWeaponSelection(HistoryTableType::iterator i_historyIter, const weapontype_t i_readyWeapon, const weapontype_t i_pendingWeapon)
{
	if (i_readyWeapon < NUMWEAPONS and i_pendingWeapon < NUMWEAPONS
	    and (i_historyIter->second.readyweapon != i_readyWeapon
	        or i_historyIter->second.pendingweapon != i_pendingWeapon))
	{
		Roll(i_historyIter->first, [i_readyWeapon, i_pendingWeapon](auto& rollingIter)
			{
				rollingIter->second.readyweapon   = i_readyWeapon;
				rollingIter->second.pendingweapon = i_pendingWeapon;
			});
		return true;
	}
	return false;
}

bool PlayerStateRoller::RollbackPowers(HistoryTableType::iterator i_historyIter, const std::array<int, NUMPOWERS> i_powers)
{
	std::array<int, NUMPOWERS> powersDelta;
	FillDeltaArray(powersDelta, i_powers, i_historyIter->second.powers);

	if (RequiresCorrection(powersDelta))
    {
		Roll(i_historyIter->first, [&powersDelta, &i_powers] (auto& rollingIter)
			{
                // Powers are a sequence of counters (except for pw_allmap), so a delta roll is appropriate, with
                // a simple assignment of pw_allmap.
                ApplyDeltaArray(rollingIter->second.powers, powersDelta);

                rollingIter->second.powers[pw_allmap] = i_powers[pw_allmap];
			});
        return true;
    }
    return false;
}


bool PlayerStateRoller::ResolveAmmo(int i_oldTic, const std::array<int, NUMAMMO>& i_ammo, player_t& io_player)
{
	auto historyIter = m_history.find(i_oldTic);
	if (historyIter != m_history.end() and RollbackAmmo(historyIter, i_ammo))
    {
        ApplyMostRecentToPlayer(io_player);
        return true;
    }
    return false;
}

bool PlayerStateRoller::ResolveMaxAmmo(int i_oldTic, const std::array<int, NUMAMMO>& i_maxAmmo, player_t& io_player)
{
	auto historyIter = m_history.find(i_oldTic);
	if (historyIter != m_history.end() and RollbackMaxAmmo(historyIter, i_maxAmmo))
    {
        ApplyMostRecentToPlayer(io_player);
        return true;
    }
    return false;
}

bool PlayerStateRoller::ResolveWeaponOwned(int i_oldTic, const std::array<bool, NUMWEAPONS>& i_weaponOwned, player_t& io_player)
{
	auto historyIter = m_history.find(i_oldTic);
	if (historyIter != m_history.end() and RollbackWeaponOwned(historyIter, i_weaponOwned))
	{
		ApplyMostRecentToPlayer(io_player);
		return true;
	}
	return false;
}

bool PlayerStateRoller::ResolveWeaponSelection(int i_oldTic, const weapontype_t i_readyWeapon, const weapontype_t i_pendingWeapon, player_t& io_player)
{
	auto historyIter = m_history.find(i_oldTic);
	if (historyIter != m_history.end() and RollbackWeaponSelection(historyIter, i_readyWeapon, i_pendingWeapon))
	{
		ApplyMostRecentToPlayer(io_player);
		return true;
	}
    return false;
}

bool PlayerStateRoller::ResolvePowers(int i_oldTic, const std::array<int, NUMPOWERS>& i_powers, player_t& io_player)
{
	auto historyIter = m_history.find(i_oldTic);
	if (historyIter != m_history.end() and RollbackPowers(historyIter, i_powers))
	{
		ApplyMostRecentToPlayer(io_player);
		return true;
	}
    return false;
}

namespace
{
    class PspriteRoller
    {
        public:
            explicit PspriteRoller(const PspriteStateType& i_psprite,
                                   psprnum_t               i_pspriteNum) :
                m_rollingPsprite (i_psprite),
                m_pspriteNum     (i_pspriteNum)
            {
            }

            void operator()(std::unordered_map<int, PlayerItemDataType>::iterator& historyIter)
            {
                PspriteStateType& historicalPsprite = historyIter->second.psprites[m_pspriteNum];
                //if (historicalPsprite.statenum == static_cast<statenum_t>(-1))
                //{
                    historicalPsprite = m_rollingPsprite;

                    Advance();
                //}
            }

            // This function advances our rollingPsprite to what it should be next tic.
            void Advance()
            {
                // First thing, make sure our own idea of the current state is valid.
                // If it was null in the player, it's -1 here, which results in end().
                auto currentRollingStateIter = ::states.find(m_rollingPsprite.statenum);
                if (currentRollingStateIter == ::states.end())
                {
                    return;
                }

                // The following logic matches P_MovePsprite.  The reason we don't use that function directly
                // is that we don't want to trigger actions here.  We're just propagating what we believe
                // history should reflect if a rollback changed it.
                if (m_rollingPsprite.tics != -1)
                {
                    --m_rollingPsprite.tics;

                    while (m_rollingPsprite.tics == 0)
                    {
                        auto nextRollingStateIter = ::states.find(currentRollingStateIter->second.nextstate);
                        if (nextRollingStateIter == ::states.end())
                        {
                            return;
                        }
                        currentRollingStateIter = nextRollingStateIter;

                        m_rollingPsprite.tics     = currentRollingStateIter->second.tics;
                        m_rollingPsprite.statenum = currentRollingStateIter->second.statenum;
                    }
                }
            }

        protected:
            PspriteStateType m_rollingPsprite;
            psprnum_t        m_pspriteNum;
    };
}

bool PlayerStateRoller::ResolvePsprites(int i_oldTic, const psprnum_t i_pspriteNum, const PspriteStateType& i_psprite, player_t& io_player)
{
	auto historyIter = m_history.find(i_oldTic);
	if (historyIter != m_history.end() and i_pspriteNum < NUMPSPRITES)
	{
        if (historyIter->second.psprites[i_pspriteNum] != i_psprite)
        {
            Roll(i_oldTic, PspriteRoller(i_psprite, i_pspriteNum));
		ApplyMostRecentToPlayer(io_player);

            //TODO: Apply the end result to the player.
            return true;
        }
	}
	return false;
}

namespace
{
    template <typename DataType>
    bool Update(DataType& io_obj, const DataType& i_value)
    {
        if (io_obj != i_value)
        {
            io_obj = i_value;
            return true;
        }
        return false;
    }
}

RollerResolveEnum PlayerStateRoller::Resolve(int i_oldTic, const PlayerItemDataType& i_itemData, player_t& io_player)
{
    // No rollback - just set the current state NOW.
    if (i_oldTic <= 0)
    {
        i_itemData.ToPlayer(io_player);
        return RollerResolveEnum::CURRENT_STATE_CHANGED;
    }

	auto historyIter = m_history.find(i_oldTic);
	if (historyIter != m_history.end())
	{
		// Ammo and maxammo are up first.  Because it's very possible for ammo quantities to have
		// continued changing in the time between now and the moment that we triggered the inventory
		// check, we don't just simply set the ammo quantities.  We calculate the delta between the
		// given inventory report and the inventory that we had at the time.  We then apply that
		// delta forward to every history entry up to the present.  This ensures that any further
		// ammo changes we made since the divergence in history is preserved.  i.e. if I fired 3
		// shots since I had a desync, those shots remain fired and the ammo count ultimately
		// still reflects those shots, just from a corrected starting point.

        const bool ammoRequiredRoll             = RollbackAmmo           (historyIter, i_itemData.ammo);
		const bool maxammoRequiredRoll          = RollbackMaxAmmo        (historyIter, i_itemData.maxammo);
		const bool weaponOwnedRequiredRoll      = RollbackWeaponOwned    (historyIter, i_itemData.weaponowned);
        const bool weaponSelectionRequiredRoll  = RollbackWeaponSelection(historyIter, i_itemData.readyweapon, i_itemData.pendingweapon);
        const bool powersRequiredRoll           = RollbackPowers         (historyIter, i_itemData.powers);

        const bool historyWasChanged = ammoRequiredRoll or
                                       maxammoRequiredRoll or
                                       weaponOwnedRequiredRoll or
                                       weaponSelectionRequiredRoll or
                                       powersRequiredRoll;

        // Now cover the fields that we don't actually rollback, just apply because the server dictates so.
		auto mostRecentIter = m_history.find(m_mostRecentTic);
		//assert(mostRecentIter != m_history.end());

        const bool healthUpdated        = Update(mostRecentIter->second.health,     i_itemData.health);
        const bool armorpointsUpdated   = Update(mostRecentIter->second.armorpoints,i_itemData.armorpoints);
        const bool armortypeUpdated     = Update(mostRecentIter->second.armortype,  i_itemData.armortype);
        const bool livesUpdated         = Update(mostRecentIter->second.lives,      i_itemData.lives);
        const bool cardsUpdated         = Update(mostRecentIter->second.cards,      i_itemData.cards);
        const bool backpackUpdated      = Update(mostRecentIter->second.backpack,   i_itemData.backpack);
        const bool cheatsUpdated        = Update(mostRecentIter->second.cheats,     i_itemData.cheats);

        const bool immediateStateWasUpdated = healthUpdated or
                                              armorpointsUpdated or
                                              armortypeUpdated or
                                              livesUpdated or
                                              cardsUpdated or
                                              backpackUpdated or
                                              cheatsUpdated;

        const bool pspritesUpdated = false; // For now

		if (historyWasChanged or immediateStateWasUpdated)
		{
            mostRecentIter->second.ToPlayer(io_player);

            if (historyWasChanged)
            {
                if (immediateStateWasUpdated)
                {
                    return RollerResolveEnum::HISTORY_AND_CURRENT_STATE_CHANGED;
                }
                return RollerResolveEnum::HISTORY_CHANGED;
            }
            return RollerResolveEnum::CURRENT_STATE_CHANGED;
		}
        return RollerResolveEnum::NO_CHANGE;
	}
	return RollerResolveEnum::INVALID_TIC;
}
