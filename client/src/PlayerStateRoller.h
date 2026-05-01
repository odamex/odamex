#pragma once

#include <unordered_map>

#include "PlayerItemDataType.h"

class player_t;

enum class RollerRecordResultEnum
{
	SUCCESS,            ///< All good.
	CURRENT_REPLACED,   ///< Current tic already had an entry, but it was replaced with something.
	INVALID_TIC_FUTURE, ///< The tic was NOT recorded because it was too far in the future.
	INVALID_TIC_PAST,   ///< The tic was NOT recorded because it was in the past.
};

class PlayerStateRoller
{
	public:

		/// Constructor: Build a state roller with recorded history starting at the upcoming first recorded tic.
		PlayerStateRoller();

		/// Add the current player state to history for the current gametic.  It is assumed and
		/// required that tic numbers given to this function only ever be incrementing by
		/// 1 for each successive call.  If current state is added to history, then SUCCESS is
		/// returned.  Otherwise, an enumeral describing the error condition is returned.
		RollerRecordResultEnum Record(int currentTic, const player_t& player);

		/// Resolve the canonical statement about player data at the given tic against recorded
		/// history.  If history is rewritten, then the resulting state is rolled forward, the
		/// ultimate resulting player data is written into the given player structure, and
		/// true is returned.  Otherwise, history and the player state are unmodified and false
		/// is returned.
		bool Resolve(int i_oldTic, const PlayerItemDataType& i_itemData, player_t& io_player);

		/// Resolve the canonical statement about player ammo at the given tic against recorded
		/// history.  If history is rewritten, then the resulting state is rolled forward, the
		/// ultimate resulting ammo count is written into the given player structure, and
		/// true is returned.  Otherwise, history and the player state are unmodified and false
		/// is returned.
		///
		/// The ammo roll is unique in that tic-to-tic deltas are preserved with the assumption
		/// that the player can be locally firing the weapon, consuming ammo that should not
		/// necessarily be replenished when a rollback happens.
		bool ResolveAmmo(int i_oldTic, const ammotype_t i_ammoType, int i_ammoCount, player_t& io_player);

		/// Similar to ResolveAmmo, except it works on maxammo and does not roll deltas - changes
		/// to this value are treated as absolute, coarse adjustments.
		bool ResolveMaxAmmo(int i_oldTic, const ammotype_t i_ammoType, int i_maxAmmoQuantity, player_t& io_player);

		/// Similar to ResolveAmmoMax: absolute adjustment of weapon ownership state.
		bool ResolveWeaponOwned(int i_oldTic, const weapontype_t i_weaponType, bool i_isOwned, player_t& io_player);

		/// Resolve the canonical statement about player weapon selection.
		/// Returns true if the player's weapon selection changed as a result, false otherwise.
		bool ResolveWeaponSelection(int i_oldTic, const weapontype_t i_readyWeapon, const weapontype_t i_pendingWeapon, player_t& io_player);

        bool ResolvePsprites(int i_oldTic, const psprnum_t i_pspriteNum, const PspriteStateType& i_psprite, player_t& io_player);

		/// Generic stream-in operator.
		template <typename StreamType>
		friend StreamType& operator<<(StreamType& io_stream, const PlayerStateRoller& i_thisRef)
		{
			io_stream << i_thisRef.m_history.size();
			for (const auto& [tic, historyItem] : i_thisRef.m_history)
			{
				io_stream << tic << historyItem;
			}
			io_stream << i_thisRef.m_mostRecentTic;
			io_stream << i_thisRef.m_oldestTic;

			return io_stream;
		}

		/// Generic stream-out operator.
		template <typename StreamType>
		friend StreamType& operator>>(StreamType& io_stream, PlayerStateRoller& o_thisRef)
		{
			o_thisRef.m_history.clear();

			size_t historySize {0};
			io_stream >> historySize;

			for (size_t i = 0; i < historySize; ++i)
			{
				int tic {0};
				io_stream >> tic;
				io_stream >> o_thisRef.m_history[tic];
			}
			io_stream >> o_thisRef.m_mostRecentTic;
			io_stream >> o_thisRef.m_oldestTic;

			return io_stream;
		}

        std::optional<std::reference_wrapper<const PlayerItemDataType>> GetStateAtTic(int i_oldTic) const
        {
            const auto iter = m_history.find(i_oldTic);
            if (iter != m_history.end())
            {
                return std::cref(iter->second);
            }
            return {};
        }

		int ExpectedTic() const { return m_mostRecentTic + 1; }

	protected:

		template <typename Callable>
		void Roll(int i_oldTic, Callable&& i_callable);

		struct IdentityHasher
		{
			size_t operator()(int ticNumber) const { return static_cast<size_t>(ticNumber); }
		};

		// One second worth of state history, keyed on *client* tic number.
		std::unordered_map<int, PlayerItemDataType, IdentityHasher> m_history;

		int m_mostRecentTic;
		int m_oldestTic;
};
