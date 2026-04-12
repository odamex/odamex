#include "PlayerStateRoller.h"

#include <cassert>
#include <iso646.h>

#include "odamex.h"


PlayerStateRoller::PlayerStateRoller() :
	m_history       (TICRATE),      // at least 1 second of history
	m_mostRecentTic (-1),           // Setup history to start recording at the given tic.
	m_oldestTic     (-1)
{
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
		assert(rollingIter != m_history.end());

		i_callable(rollingIter);
	}
}

bool PlayerStateRoller::ResolveAmmo(int i_oldTic, const ammotype_t i_ammoType, int i_ammoCount, player_t& io_player)
{
	auto historyIter = m_history.find(i_oldTic);
	if (historyIter != m_history.end() and i_ammoType < NUMAMMO)
	{
		const int ammoDelta = i_ammoCount - historyIter->second.ammo[i_ammoType];
		if (ammoDelta)
		{
			Roll(i_oldTic, [&ammoDelta, i_ammoType](auto& rollingIter)
				{
					int& historicalAmmoRef = rollingIter->second.ammo[i_ammoType];
					historicalAmmoRef = std::max(historicalAmmoRef + ammoDelta, 0);
				});

			auto mostRecentIter = m_history.find(m_mostRecentTic);
			assert(mostRecentIter != m_history.end());

			io_player.ammo[i_ammoType] = mostRecentIter->second.ammo[i_ammoType];
			return true;
		}
	}
	return false;
}

bool PlayerStateRoller::Resolve(int i_oldTic, const PlayerItemDataType& i_itemData, player_t& io_player)
{
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

		const bool playerObjectRequiresRefresh =    ammoRequiresRoll
		                                         or maxammoRequiresRoll
		                                         or weaponOwnedRequiresRoll
		                                         or readyweaponRequiresRoll
		                                         or pendingweaponRequiresRoll;

		if (playerObjectRequiresRefresh)
		{
			auto mostRecentIter = m_history.find(m_mostRecentTic);
			assert(mostRecentIter != m_history.end());

			mostRecentIter->second.ToPlayer(io_player);
		}

		return playerObjectRequiresRefresh;
	}

	return false;
}
