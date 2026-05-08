#pragma once

#include <functional>
#include <unordered_map>
#include <utility>

enum class AwarenessEnum
{
    NOT_AWARE = 0,
    ALWAYS_AWARE,       ///< Permanently fully aware.  Avatars and all players.  Key part of nth_element comparator.
    FULLY_AWARE,        ///< All messages active, full fidelity.
    SEMI_AWARE,         ///< Reduced reliable traffic.
    BARELY_AWARE,       ///< On life support.
};

template <size_t MAX_PLAYER_COUNT>
class ActorAwarenessState
{
    public:
        ActorAwarenessState() :
            m_player(MAX_PLAYER_COUNT)
        {
        }

        bool IsAware(size_t playerId) const
        {
            return Get(playerId) != AwarenessEnum::NOT_AWARE;
        }

        AwarenessEnum Get(size_t playerId) const
        {
            auto iter = m_player.find(playerId);
            if (iter != m_player.end())
            {
                return iter->second;
            }
            return AwarenessEnum::NOT_AWARE;
        }

        AwarenessEnum Set(size_t playerId, AwarenessEnum awareness)
        {
            return std::exchange(m_player[playerId], awareness);
        }

    protected:

        // We use an unordered_map configured to be a table so that, if by some crazy
        // error, we get fed player IDs > 255, this continues to just work and be
        // constant-time access.
        std::unordered_map<size_t, AwarenessEnum, std::identity> m_player;
};
