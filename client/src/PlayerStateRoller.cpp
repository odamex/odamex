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
	template <typename DeltaElementType, typename ElementType, size_t N>
	void FillDeltaArray(std::array<DeltaElementType, N>&    o_delta,
	                    const std::array<ElementType, N>&   i_lhs,
	                    const std::array<ElementType, N>&   i_rhs)
	{
		// Don't compile if the array can't store negative values!
		static_assert(std::is_signed<DeltaElementType>() == true);

		for (size_t i = 0; i < o_delta.size(); ++i)
		{
			o_delta[i] = static_cast<DeltaElementType>(i_lhs[i]) - static_cast<DeltaElementType>(i_rhs[i]);
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

		i_callable(rollingIter->second);
	}
	i_callable(m_currentState);
}

void PlayerStateRoller::ApplyMostRecentToPlayer(player_t& io_player)
{
	m_currentState.ToPlayer(io_player);
}

std::optional<PlayerStateRoller::HistoryTableType::iterator> PlayerStateRoller::ObtainHistory(int i_oldTic, const player_t& i_player)
{
    auto historyIter = m_history.find(i_oldTic);
    if (historyIter != m_history.end())
    {
        m_currentState.FromPlayer(i_player);
        return historyIter;
    }
    return {};
}


bool PlayerStateRoller::RollbackAmmo(HistoryTableType::iterator i_historyIter, const std::array<int, NUMAMMO>& i_ammo)
{
	std::array<int, NUMAMMO> ammoDelta;
	FillDeltaArray(ammoDelta, i_ammo, i_historyIter->second.ammo);

	if (RequiresCorrection(ammoDelta))
	{
		Roll(i_historyIter->first, [&ammoDelta](auto& rollingState)
			{
				ApplyDeltaArray(rollingState.ammo, ammoDelta);
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
		Roll(i_historyIter->first, [&i_maxAmmo] (auto& rollingState)
			{
				// Please note that we don't apply the delta across the history of maxammo.
				// The reason for that is if we have, say, an off-by-one tic prediction of
				// a pickup that affects maxammo, we don't want to wind up with a double-
				// value maxammo.
				rollingState.maxammo = i_maxAmmo;
			});
		return true;
	}
	return false;
}

bool PlayerStateRoller::RollbackWeaponOwned(HistoryTableType::iterator i_historyIter, const std::array<bool, NUMWEAPONS>& i_weaponOwned)
{
	if (i_historyIter->second.weaponowned != i_weaponOwned)
	{
		Roll(i_historyIter->first, [&i_weaponOwned](auto& rollingState)
			{
				rollingState.weaponowned = i_weaponOwned;
			});
		return true;
	}
	return false;
}

bool PlayerStateRoller::RollbackWeaponSelection(HistoryTableType::iterator i_historyIter, const weapontype_t i_readyWeapon, const weapontype_t i_pendingWeapon)
{
	if (   i_historyIter->second.readyweapon   != i_readyWeapon
	    or i_historyIter->second.pendingweapon != i_pendingWeapon)
	{
		Roll(i_historyIter->first, [i_readyWeapon, i_pendingWeapon](auto& rollingState)
			{
				rollingState.readyweapon   = i_readyWeapon;
				rollingState.pendingweapon = i_pendingWeapon;
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
		Roll(i_historyIter->first, [&powersDelta, &i_powers] (auto& rollingState)
			{
			    // Powers are a sequence of counters (except for pw_allmap), so a delta roll is appropriate, with
			    // a simple assignment of pw_allmap.
			    ApplyDeltaArray(rollingState.powers, powersDelta);

			    rollingState.powers[pw_allmap] = i_powers[pw_allmap];
			});
		return true;
	}
	return false;
}

bool PlayerStateRoller::RollbackHealth(HistoryTableType::iterator i_historyIter, const int i_health)
{
    if (const int healthDelta = i_health - i_historyIter->second.health)
    {
        Roll(i_historyIter->first, [& healthDelta] (auto& rollingState)
        {
            rollingState.health += healthDelta;
        });
        return true;
    }
    return false;
}

bool PlayerStateRoller::RollbackArmorpoints(HistoryTableType::iterator i_historyIter, const int i_armorpoints)
{
    if (const int armorpointsDelta = i_armorpoints - i_historyIter->second.armorpoints)
    {
        Roll(i_historyIter->first, [& armorpointsDelta] (auto& rollingState)
        {
            rollingState.armorpoints += armorpointsDelta;
        });
        return true;
    }
    return false;
}

bool PlayerStateRoller::RollbackArmortype(HistoryTableType::iterator i_historyIter, const int i_armortype)
{
    if (const int armortypeDelta = i_armortype - i_historyIter->second.armortype)
    {
        Roll(i_historyIter->first, [& armortypeDelta] (auto& rollingState)
        {
            rollingState.armortype += armortypeDelta;
        });
        return true;
    }
    return false;
}

bool PlayerStateRoller::RollbackLives(HistoryTableType::iterator i_historyIter, const int i_lives)
{
    if (const int livesDelta = i_lives - i_historyIter->second.lives)
    {
        Roll(i_historyIter->first, [& livesDelta] (auto& rollingState)
        {
            rollingState.lives += livesDelta;
        });
        return true;
    }
    return false;
}

bool PlayerStateRoller::RollbackBackpack(HistoryTableType::iterator i_historyIter, const bool i_backpack)
{
    if (const int backpackDelta = int(i_backpack) - int(i_historyIter->second.backpack))
    {
        Roll(i_historyIter->first, [& backpackDelta] (auto& rollingState)
        {
            rollingState.backpack = bool(int(rollingState.backpack) + backpackDelta);
        });
        return true;
    }
    return false;
}

bool PlayerStateRoller::RollbackCards(HistoryTableType::iterator i_historyIter, const std::array<bool, NUMCARDS>& i_cards)
{
	std::array<int, NUMCARDS> cardsDelta;
	FillDeltaArray(cardsDelta, i_cards, i_historyIter->second.cards);
    if (RequiresCorrection(cardsDelta))
    {
        Roll(i_historyIter->first, [&cardsDelta](auto& rollingState)
        {
            for (size_t i = 0; i < cardsDelta.size(); ++i)
            {
                rollingState.cards[i] = bool(int(rollingState.cards[i]) + cardsDelta[i]);
            }
        });
        return true;
    }
    return false;
}

bool PlayerStateRoller::RollbackCheats(HistoryTableType::iterator i_historyIter, const uint32_t i_cheats)
{
    if (i_historyIter->second.cheats != i_cheats)
    {
        // Note - it's important that the delta be signed!  We may need to remove a value.
        std::array<int, sizeof(i_cheats) * 8> deltaCheats;
        for (size_t i = 0; i < deltaCheats.size(); ++i)
        {
            deltaCheats[i] = int((i_cheats >> i) & 0x1) - int((i_historyIter->second.cheats >> i) & 0x1);
        }

        Roll(i_historyIter->first, [& deltaCheats] (auto& rollingState)
        {
            for (size_t i = 0; i < deltaCheats.size(); ++i)
            {
                const bool correctedCheatValue = bool(int((rollingState.cheats >> i) & 0x1) + deltaCheats[i]);
                if (correctedCheatValue)
                {
                    rollingState.cheats |= (1 << i);
                }
                else
                {
                    rollingState.cheats &= ~(1 << i);
                }
            }
        });

        return true;
    }

    return false;
}

bool PlayerStateRoller::ResolveAmmo(int i_oldTic, const std::array<int, NUMAMMO>& i_ammo, player_t& io_player)
{
	auto historyIter = ObtainHistory(i_oldTic, io_player);
	if (historyIter and RollbackAmmo(*historyIter, i_ammo))
	{
		ApplyMostRecentToPlayer(io_player);
		return true;
	}
	return false;
}

bool PlayerStateRoller::ResolveMaxAmmo(int i_oldTic, const std::array<int, NUMAMMO>& i_maxAmmo, player_t& io_player)
{
	auto historyIter = ObtainHistory(i_oldTic, io_player);
	if (historyIter and RollbackMaxAmmo(*historyIter, i_maxAmmo))
	{
		ApplyMostRecentToPlayer(io_player);
		return true;
	}
	return false;
}

bool PlayerStateRoller::ResolveWeaponOwned(int i_oldTic, const std::array<bool, NUMWEAPONS>& i_weaponOwned, player_t& io_player)
{
	auto historyIter = ObtainHistory(i_oldTic, io_player);
	if (historyIter and RollbackWeaponOwned(*historyIter, i_weaponOwned))
	{
		ApplyMostRecentToPlayer(io_player);
		return true;
	}
	return false;
}

bool PlayerStateRoller::ResolveWeaponSelection(int i_oldTic, const weapontype_t i_readyWeapon, const weapontype_t i_pendingWeapon, player_t& io_player)
{
	auto historyIter = ObtainHistory(i_oldTic, io_player);
	if (historyIter and RollbackWeaponSelection(*historyIter, i_readyWeapon, i_pendingWeapon))
	{
		ApplyMostRecentToPlayer(io_player);
		return true;
	}
	return false;
}

bool PlayerStateRoller::ResolvePowers(int i_oldTic, const std::array<int, NUMPOWERS>& i_powers, player_t& io_player)
{
	auto historyIter = ObtainHistory(i_oldTic, io_player);
	if (historyIter and RollbackPowers(*historyIter, i_powers))
	{
		ApplyMostRecentToPlayer(io_player);
		return true;
	}
	return false;
}

bool PlayerStateRoller::RollbackPsprites(HistoryTableType::iterator i_historyIter, const std::array<PspriteStateType, NUMPSPRITES>& i_psprites)
{
	bool result = false;

	for (size_t pspriteNum = 0; pspriteNum < i_psprites.size(); ++pspriteNum)
	{
		if (i_historyIter->second.psprites[pspriteNum] != i_psprites[pspriteNum])
		{
			// The following is based on the idea that if the server told us we actually had a different psprite
			// state at some point in history, we want to allow that to roll forward as long as the history records
			// a passive "slide" through the sprites as the client experienced them.
			//
			// So we march forward through history and the moment we see a recorded sprite state that disagrees with
			// a passive "slide", we guesstimate that something happened on the client to get the psprites off the
			// "happy path," which probably was intentional and gameplay-important.  So we stop rolling forward in
			// that case and just let the current state be.  We really hope that the server sends another rollback
			// statement that accounts for what happened.
			//
			// However if we find that history is just a passive "slide" through psprite states, then we replace that
			// history with our passive rolling Psprite state rooted at whatever the server said it should be.

			PspriteStateType rollingPsprite           {i_psprites[pspriteNum]};
			PspriteStateType passiveHistoricalPsprite {i_historyIter->second.psprites[pspriteNum]};

			auto roller = [pspriteNum, &rollingPsprite, &passiveHistoricalPsprite] (PlayerItemDataType& rollingState) -> bool
			{
				if (rollingState.psprites[pspriteNum] != passiveHistoricalPsprite)
				{
					// Something unexpected happened in history!  break now, the roll forward can't complete.
					return false;
				}
				if (rollingState.psprites[pspriteNum] == rollingPsprite)
				{
					// Did we somehow converge back up with history?  Stop rolling.
					return false;
				}
				rollingState.psprites[pspriteNum] = rollingPsprite;
				passiveHistoricalPsprite          = passiveHistoricalPsprite.Next();
				rollingPsprite                    = rollingPsprite.Next();
				return true;
			};

			int rollingTic = i_historyIter->first;
			for (; rollingTic <= m_mostRecentTic; ++rollingTic)
			{
				auto historyIter = m_history.find(rollingTic);
                if (not roller(historyIter->second))
				{
					break;
				}
			}

			// Got all the way to the end of history with a changed state!
			// Roll over the current state and essentially OR the success result.
			if (rollingTic > m_mostRecentTic)
			{
				result = roller(m_currentState);
			}
		}
	}
	return result;
}

bool PlayerStateRoller::ResolvePsprites(int i_oldTic, const std::array<PspriteStateType, NUMPSPRITES>& i_psprites, player_t& io_player)
{
	auto historyIter = ObtainHistory(i_oldTic, io_player);
	if (historyIter and RollbackPsprites(*historyIter, i_psprites))
	{
		ApplyMostRecentToPlayer(io_player);
		return true;
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
	// Special case:  A "pre-history" statement of our data.  Just apply it to current.
	if (i_oldTic < 0)
	{
		i_itemData.ToPlayer(io_player);
		return RollerResolveEnum::CURRENT_STATE_CHANGED;
	}

	auto historyIter = ObtainHistory(i_oldTic, io_player);
	if (historyIter)
	{
		// Ammo and maxammo are up first.  Because it's very possible for ammo quantities to have
		// continued changing in the time between now and the moment that we triggered the inventory
		// check, we don't just simply set the ammo quantities.  We calculate the delta between the
		// given inventory report and the inventory that we had at the time.  We then apply that
		// delta forward to every history entry up to the present.  This ensures that any further
		// ammo changes we made since the divergence in history is preserved.  i.e. if I fired 3
		// shots since I had a desync, those shots remain fired and the ammo count ultimately
		// still reflects those shots, just from a corrected starting point.

		const bool ammoRequiredRoll             = RollbackAmmo           (*historyIter, i_itemData.ammo);
		const bool maxammoRequiredRoll          = RollbackMaxAmmo        (*historyIter, i_itemData.maxammo);
		const bool weaponOwnedRequiredRoll      = RollbackWeaponOwned    (*historyIter, i_itemData.weaponowned);
		const bool weaponSelectionRequiredRoll  = RollbackWeaponSelection(*historyIter, i_itemData.readyweapon, i_itemData.pendingweapon);
		const bool powersRequiredRoll           = RollbackPowers         (*historyIter, i_itemData.powers);
		const bool pspritesRequiredRoll         = RollbackPsprites       (*historyIter, i_itemData.psprites);

		const bool healthRequiredRoll       = RollbackHealth        (*historyIter, i_itemData.health);
		const bool armorpointsRequiredRoll  = RollbackArmorpoints   (*historyIter, i_itemData.armorpoints);
		const bool armortypeRequiredRoll    = RollbackArmortype     (*historyIter, i_itemData.armortype);
		const bool livesRequiredRoll        = RollbackLives         (*historyIter, i_itemData.lives);
		const bool backpackRequiredRoll     = RollbackBackpack      (*historyIter, i_itemData.backpack);
		const bool cardsRequiredRoll        = RollbackCards         (*historyIter, i_itemData.cards);
		const bool cheatsRequiredRoll       = RollbackCheats        (*historyIter, i_itemData.cheats);

		const bool historyWasChanged = ammoRequiredRoll or
		                               maxammoRequiredRoll or
		                               weaponOwnedRequiredRoll or
		                               weaponSelectionRequiredRoll or
		                               powersRequiredRoll or
		                               pspritesRequiredRoll or
		                               healthRequiredRoll or
		                               armorpointsRequiredRoll or
		                               armortypeRequiredRoll or
		                               livesRequiredRoll or
		                               backpackRequiredRoll or
		                               cardsRequiredRoll or
		                               cheatsRequiredRoll;

		if (historyWasChanged)
		{
			m_currentState.ToPlayer(io_player);

			return RollerResolveEnum::HISTORY_CHANGED;
		}
		return RollerResolveEnum::NO_CHANGE;
	}
	return RollerResolveEnum::INVALID_TIC;
}
