#pragma once

#include <unordered_map>

#include "PlayerItemDataType.h"

class player_t;

class PlayerStateRoller
{
    public:

        /// Constructor: Build a state roller with recorded history starting at the given tic.
        explicit PlayerStateRoller(int currentTic);

        /// Add the current player state to history for the current gametic.  It is assumed and
        /// required that tic numbers given to this function only ever be incrementing by
        /// 1 for each successive call.  If current state is added to history, then true is
        /// returned.  Otherwise, false is returned.
        bool Record(int currentTic, const player_t& player);

        /// Resolve the canonical statement about player data at the given tic against recorded
        /// history.  If history is rewritten, then the resulting state is rolled forward, the
        /// ultimate resulting player data is written into the given player structure, and
        /// true is returned.  Otherwise, history and the player state are unmodified and false
        /// is returned.
        bool Resolve(int i_oldTic, const PlayerItemDataType& i_itemData, player_t& io_player);

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
