#pragma once

#include <array>

#include "odamex.h"
#include "d_player.h"

/// This data type represents the states whose history we care to track and reconcile.
struct PlayerItemDataType
{
    std::array<int,  NUMAMMO>       ammo;
    std::array<int,  NUMAMMO>       maxammo;
    //std::array<int,  NUMPOWERS>     powers;
    weapontype_t                    readyweapon;
    weapontype_t                    pendingweapon;
    std::array<bool, NUMWEAPONS+1>  weaponowned;
    //std::array<bool, NUMCARDS>      cards;
    //bool                            backpack;

    PlayerItemDataType() :
        readyweapon     (wp_none),
        pendingweapon   (wp_none)
        //backpack        (false)
    {
        ammo.fill(0);
        maxammo.fill(0);
        //powers.fill(0);
        weaponowned.fill(false);
        //cards.fill(false);
    }

    explicit PlayerItemDataType(const player_t& player) :
        ammo            (player.ammo),
        maxammo         (player.maxammo),
        //powers          (player.powers),        // TODO: Fix the clearing of MF_SHADOW in PlayerThink.  It should not happen only when a decrement to zero happens.  Just check for value == 0.
        readyweapon     (player.readyweapon),
        pendingweapon   (player.pendingweapon),
        weaponowned     (player.weaponowned)
        //cards           (player.cards),
        //backpack        (player.backpack)
    {
    }

    template <typename ArrayType>
    static void Subtractor(ArrayType& outArray, const ArrayType& lhs, const ArrayType& rhs)
    {
        for (size_t i = 0; i < outArray.size(); ++i)
        {
            outArray[i] = lhs[i] - rhs[i];
        }
    }

    PlayerItemDataType operator-(const PlayerItemDataType& rhs) const
    {
        PlayerItemDataType result;

        Subtractor(result.ammo, ammo, rhs.ammo);
        Subtractor(result.maxammo, maxammo, rhs.maxammo);
    }
};
