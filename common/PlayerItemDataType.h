#pragma once

#include <array>

#include "odamex.h"
#include "d_player.h"
#include "p_local.h"

struct PspriteStateType
{
	statenum_t  statenum{-1};
	int         tics    {-1};
	//fixed_t     sx      { 0};
	//fixed_t     sy      { 0};

	bool operator==(const PspriteStateType&) const = default;

	PspriteStateType& operator=(const pspdef_t& pspdef)
	{
		statenum = pspdef.state ? pspdef.state->statenum : static_cast<statenum_t>(-1);
		tics     = pspdef.tics;
		return *this;
	}

	template <typename StreamType>
	friend StreamType& operator<<(StreamType& io_stream, const PspriteStateType& i_thisRef)
	{
		io_stream
		    << i_thisRef.statenum
		    << i_thisRef.tics
		    ;
		return io_stream;
	}

	template <typename StreamType>
	friend StreamType& operator>>(StreamType& io_stream, PspriteStateType& o_thisRef)
	{
		io_stream
		    >> o_thisRef.statenum
		    >> o_thisRef.tics
		    ;
		return io_stream;
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
		cheats          (player.cheats)
	{
		static_assert(std::tuple_size_v<decltype(psprites)> ==
		              std::tuple_size_v<decltype(player.psprites)>);

		for (size_t i = 0; i < psprites.size(); ++i)
		{
			psprites[i] = player.psprites[i];
		}
	}

	void ToPlayer(player_t& player) const
	{
		player.ammo            = ammo;
		player.maxammo         = maxammo;
		player.health          = health;
		player.armorpoints     = armorpoints;
		player.armortype       = armortype;
		player.lives           = lives;
		player.powers          = powers;        // TODO: Fix the clearing of MF_SHADOW in PlayerThink.  It should not happen only when a decrement to zero happens.  Just check for value == 0.
		player.readyweapon     = readyweapon;
		player.pendingweapon   = pendingweapon;
		player.weaponowned     = weaponowned;
		player.cards           = cards;
		player.backpack        = backpack;

		if (not player.spectator)
		{
			player.cheats = cheats;
		}

        // TODO: psprites.
        //

	// Sync mo health with player health
	// For crosshaircolor, etc.
	if (player.mo)
    {
		player.mo->health = player.health;
    }

	for (size_t i = 0; i < psprites.size(); ++i)
    {
        auto iter = ::states.find(psprites[i].statenum);
        player.psprites[i].state = iter != ::states.end() ? &iter->second : nullptr;
        player.psprites[i].tics = psprites[i].tics;
    }

        // From PlayerInfo:
//for (int i = 0; i < NUMPSPRITES; i++)
//    P_SetPsprite(p, i, stnum[i]);

        // TODO: Special state update functions.
        P_SetPlayerPowerupStatuses(player, player.powers);

	}

	template <typename StreamType>
	friend StreamType& operator<<(StreamType& io_stream, const PlayerItemDataType& i_thisRef)
	{
		io_stream
		    << i_thisRef.ammo
		    << i_thisRef.maxammo
		    << i_thisRef.health
		    << i_thisRef.armorpoints
		    << i_thisRef.armortype
		    << i_thisRef.lives
		    << i_thisRef.powers
		    << i_thisRef.readyweapon
		    << i_thisRef.pendingweapon
		    << i_thisRef.weaponowned
		    << i_thisRef.cards
		    << i_thisRef.backpack
		    << i_thisRef.cheats
		    << i_thisRef.psprites
		    ;

		return io_stream;
	}

	template <typename StreamType>
	friend StreamType& operator>>(StreamType& io_stream, PlayerItemDataType& o_thisRef)
	{
		io_stream
		    >> o_thisRef.ammo
		    >> o_thisRef.maxammo
		    >> o_thisRef.health
		    >> o_thisRef.armorpoints
		    >> o_thisRef.armortype
		    >> o_thisRef.lives
		    >> o_thisRef.powers
		    >> o_thisRef.readyweapon
		    >> o_thisRef.pendingweapon
		    >> o_thisRef.weaponowned
		    >> o_thisRef.cards
		    >> o_thisRef.backpack
		    >> o_thisRef.cheats
		    >> o_thisRef.psprites
		    ;

		return io_stream;
	}
};
