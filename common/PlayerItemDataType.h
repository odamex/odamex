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
	std::array<bool, NUMWEAPONS>    weaponowned;
	//std::array<bool, NUMCARDS>      cards;
	//bool                            backpack;

	bool operator==(const PlayerItemDataType&) const = default;

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

	void ToPlayer(player_t& player)
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
