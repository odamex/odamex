#pragma once

#include <array>

#include "odamex.h"
#include "d_player.h"

    struct PspriteStateType
    {
        statenum_t  statenum{-1};
        int         tics    {-1};
        //fixed_t     sx      { 0};
        //fixed_t     sy      { 0};

        bool operator==(const PspriteStateType&) const = default;

        static PspriteStateType FromPsprite(const pspdef_t& pspdef)
        {
            PspriteStateType result;

            result.statenum = pspdef.state ? pspdef.state->statenum : static_cast<statenum_t>(-1);
            result.tics     = pspdef.tics;

            return result;
        }

        template <size_t N>
        static std::array<PspriteStateType, N> FromPlayer(const std::array<pspdef_t, N> psprites)
        {
            std::array<PspriteStateType, N> results;

            for (size_t i = 0; i < N; ++i)
            {
                results[i].statenum = psprites[i].state ? psprites[i].state->statenum : static_cast<statenum_t>(-1);
                results[i].tics     = psprites[i].tics;
            }
            return results;
        }
    };

/// This data type represents the states whose history we care to track and reconcile.
struct PlayerItemDataType
{
	std::array<int,  NUMAMMO>       ammo;
	std::array<int,  NUMAMMO>       maxammo;
	int                             health;
	int                             armorpoints;
	int                             armortype;      // 0 thru 2...
	int                             lives;
	std::array<int,  NUMPOWERS>     powers;
	weapontype_t                    readyweapon;
	weapontype_t                    pendingweapon;
	std::array<bool, NUMWEAPONS>    weaponowned;
	std::array<bool, NUMCARDS>      cards;
	bool                            backpack;
	uint32_t                        cheats;

    std::array<PspriteStateType, NUMPSPRITES> psprites;

	bool operator==(const PlayerItemDataType&) const = default;

	PlayerItemDataType() :
		health          (0),
		armorpoints     (0),
		armortype       (0),
		lives           (0),
		readyweapon     (wp_none),
		pendingweapon   (wp_none),
		backpack        (false),
		cheats          (false)
	{
		ammo.fill(0);
		maxammo.fill(0);
		powers.fill(0);
		weaponowned.fill(false);
		cards.fill(false);
	}

	explicit PlayerItemDataType(const player_t& player) :
		ammo            (player.ammo),
		maxammo         (player.maxammo),
		health          (player.health),
		armorpoints     (player.armorpoints),
		armortype       (player.armortype),
		lives           (player.lives),
		powers          (player.powers),        // TODO: Fix the clearing of MF_SHADOW in PlayerThink.  It should not happen only when a decrement to zero happens.  Just check for value == 0.
		readyweapon     (player.readyweapon),
		pendingweapon   (player.pendingweapon),
		weaponowned     (player.weaponowned),
		cards           (player.cards),
		backpack        (player.backpack),
		cheats          (player.cheats),
		psprites        (PspriteStateType::FromPlayer(player.psprites))
	{
	}

	void ToPlayer(player_t& player) const
	{
		player.ammo            = ammo;
		player.maxammo         = maxammo;
		//player.powers          = powers;        // TODO: Fix the clearing of MF_SHADOW in PlayerThink.  It should not happen only when a decrement to zero happens.  Just check for value == 0.
		player.readyweapon     = readyweapon;
		player.pendingweapon   = pendingweapon;
		player.weaponowned     = weaponowned;
		//player.cards           = cards;
		//player.backpack        = backpack;
	}

	template <typename StreamType>
	friend StreamType& operator<<(StreamType& io_stream, const PlayerItemDataType& i_thisRef)
	{
		io_stream
		    << i_thisRef.ammo
		    << i_thisRef.maxammo
		    << static_cast<int>(i_thisRef.readyweapon)
		    << static_cast<int>(i_thisRef.pendingweapon)
		    << i_thisRef.weaponowned;

		return io_stream;
	}

	template <typename StreamType>
	friend StreamType& operator>>(StreamType& io_stream, PlayerItemDataType& o_thisRef)
	{
		int temp_readyweapon    {0};
		int temp_pendingweapon  {0};

		io_stream
		    >> o_thisRef.ammo
		    >> o_thisRef.maxammo
		    >> temp_readyweapon
		    >> temp_pendingweapon
		    >> o_thisRef.weaponowned;

		o_thisRef.readyweapon   = static_cast<weapontype_t>(temp_readyweapon);
		o_thisRef.pendingweapon = static_cast<weapontype_t>(temp_pendingweapon);

		return io_stream;
	}
};
