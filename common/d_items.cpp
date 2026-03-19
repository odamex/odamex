// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 2006-2026 by The Odamex Team.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// DESCRIPTION:
//	Items: key cards, artifacts, weapon, ammunition.
//
//-----------------------------------------------------------------------------


#include "odamex.h"

// We are referring to sprite numbers.
#include "info.h"

#include "d_items.h"
#include "teaminfo.h"


//
// PSPRITE ACTIONS for waepons.
// This struct controls the weapon animations.
//
// Each entry is:
//   ammo/amunition type
//  upstate
//  downstate
// readystate
// atkstate, i.e. attack/fire/hit frame
// flashstate, muzzle flash
// droptype
// ammouse
// minammo
// mbf21 flags
// ammopershot
// internal flags
//
weaponinfo_t weaponinfo[NUMWEAPONS+1] =
{
	{
		// fist
		.ammotype      = am_noammo,
		.upstate       = S_PUNCHUP,
		.downstate     = S_PUNCHDOWN,
		.readystate    = S_PUNCH,
		.atkstate      = S_PUNCH1,
		.flashstate    = S_NULL,
		.droptype      = MT_PLAYER, // TODO: should this be MT_NULL instead of MT_PLAYER??
		.ammouse       = 0,
		.minammo       = 0,
		.flags         = WPF_FLEEMELEE | WPF_AUTOSWITCHFROM | WPF_NOAUTOSWITCHTO,
		.ammopershot   = 1,
		.internalflags = WIF_NOFLAG
	},
	{
		// pistol
		.ammotype      = am_clip,
		.upstate       = S_PISTOLUP,
		.downstate     = S_PISTOLDOWN,
		.readystate    = S_PISTOL,
		.atkstate      = S_PISTOL1,
		.flashstate    = S_PISTOLFLASH,
		.droptype      = MT_CLIP,
		.ammouse       = 1,
		.minammo       = 1,
		.flags         = WPF_AUTOSWITCHFROM,
		.ammopershot   = 1,
		.internalflags = WIF_NOFLAG
	},
	{
		// shotgun
		.ammotype      = am_shell,
		.upstate       = S_SGUNUP,
		.downstate     = S_SGUNDOWN,
		.readystate    = S_SGUN,
		.atkstate      = S_SGUN1,
		.flashstate    = S_SGUNFLASH1,
		.droptype      = MT_SHOTGUN,
		.ammouse       = 1,
		.minammo       = 1,
		.flags         = WPF_NOFLAG,
		.ammopershot   = 1,
		.internalflags = WIF_NOFLAG
	},
	{
		// chaingun
		.ammotype      = am_clip,
		.upstate       = S_CHAINUP,
		.downstate     = S_CHAINDOWN,
		.readystate    = S_CHAIN,
		.atkstate      = S_CHAIN1,
		.flashstate    = S_CHAINFLASH1,
		.droptype      = MT_CHAINGUN,
		.ammouse       = 1,
		.minammo       = 1,
		.flags         = WPF_NOFLAG,
		.ammopershot   = 1,
		.internalflags = WIF_NOFLAG
	},
	{
		// missile launcher
		.ammotype      = am_misl,
		.upstate       = S_MISSILEUP,
		.downstate     = S_MISSILEDOWN,
		.readystate    = S_MISSILE,
		.atkstate      = S_MISSILE1,
		.flashstate    = S_MISSILEFLASH1,
		.droptype      = MT_MISC27,
		.ammouse       = 1,
		.minammo       = 1,
		.flags         = WPF_NOAUTOFIRE,
		.ammopershot   = 1,
		.internalflags = WIF_NOFLAG
	},
	{
		// plasma rifle
		.ammotype      = am_cell,
		.upstate       = S_PLASMAUP,
		.downstate     = S_PLASMADOWN,
		.readystate    = S_PLASMA,
		.atkstate      = S_PLASMA1,
		.flashstate    = S_PLASMAFLASH1,
		.droptype      = MT_MISC28,
		.ammouse       = 1,
		.minammo       = 1,
		.flags         = WPF_NOFLAG,
		.ammopershot   = 1,
		.internalflags = WIF_NOFLAG
	},
	{
		// bfg 9000
		.ammotype      = am_cell,
		.upstate       = S_BFGUP,
		.downstate     = S_BFGDOWN,
		.readystate    = S_BFG,
		.atkstate      = S_BFG1,
		.flashstate    = S_BFGFLASH1,
		.droptype      = MT_MISC25,
		.ammouse       = 40,
		.minammo       = 40,
		.flags         = WPF_NOAUTOFIRE,
		.ammopershot   = 40,
		.internalflags = WIF_NOFLAG
	},
	{
		// chainsaw
		.ammotype      = am_noammo,
		.upstate       = S_SAWUP,
		.downstate     = S_SAWDOWN,
		.readystate    = S_SAW,
		.atkstate      = S_SAW1,
		.flashstate    = S_NULL,
		.droptype      = MT_MISC26,
		.ammouse       = 0,
		.minammo       = 0,
		.flags         = WPF_NOTHRUST | WPF_FLEEMELEE | WPF_NOAUTOSWITCHTO,
		.ammopershot   = 1,
		.internalflags = WIF_NOFLAG
	},
	{
		// super shotgun
		.ammotype      = am_shell,
		.upstate       = S_DSGUNUP,
		.downstate     = S_DSGUNDOWN,
		.readystate    = S_DSGUN,
		.atkstate      = S_DSGUN1,
		.flashstate    = S_DSGUNFLASH1,
		.droptype      = MT_SUPERSHOTGUN,
		.ammouse       = 2,
		.minammo       = 2,
		.flags         = WPF_NOFLAG,
		.ammopershot   = 2,
		.internalflags = WIF_NOFLAG
	},
	{
		//NUMWEAPONS (player has no weapon including fist, ClearInventory)
		.ammotype      = am_noammo,
		.upstate       = S_NOWEAPONUP,
		.downstate     = S_NOWEAPONDOWN,
		.readystate    = S_NOWEAPON,
		.atkstate      = S_NOWEAPON,
		.flashstate    = S_NOWEAPON,
		.droptype      = MT_MISC26,
		.ammouse       = 0,
		.minammo       = 0,
		.flags         = WPF_NOFLAG,
		.ammopershot   = 0,
		.internalflags = WIF_NOFLAG
	},
};

int num_items;

// [RH] Guess what. These next three functions are from Quake2:
//	g_items.c

/*
===============
GetItemByIndex
===============
*/
gitem_t	*GetItemByIndex (int index)
{
	if (index == 0 || index >= num_items)
		return NULL;

	return &itemlist[index];
}


/*
===============
FindItemByClassname

===============
*/
gitem_t	*FindItemByClassname (const char *classname)
{
	int		i;
	gitem_t	*it;

	it = itemlist;
	for (i = 0; i < num_items; i++, it++)
		if (it->classname && !stricmp(it->classname, classname))
			return it;

	return NULL;
}

/*
===============
FindItem

===============
*/
gitem_t	*FindItem (const char *pickup_name)
{
	int		i;
	gitem_t	*it;

	it = itemlist;
	for (i = 0; i < num_items; i++, it++)
		if (it->pickup_name && !stricmp(it->pickup_name, pickup_name))
			return it;

	return NULL;
}

gitem_t* FindCardItem(card_t card)
{
	int		i;
	gitem_t* it;

	it = itemlist;
	for (i = 0; i < num_items; i++, it++)
		if (it->flags == IT_KEY && static_cast<card_t>(it->offset) == card)
			return it;

	return NULL;
}


// Item info
// Used mainly by the give command. Hopefully will
// become more general-purpose later.
// (Yes, this was inspired by Quake 2)
gitem_t itemlist[] = {
	{
		"",
		NULL,
		NULL,
		0,
		0,
		0,
		""
	},	// leave index 0 alone

	{
		"item_armor_basic",
		NULL,
		NULL,
		IT_ARMOR,
		1,
		0,
		"Basic Armor"
	},

	{
		"item_armor_mega",
		NULL,
		NULL,
		IT_ARMOR,
		2,
		0,
		"Mega Armor"
	},

	{
		"item_armor_bonus",
		NULL,
		NULL,
		IT_ARMOR,
		1,
		0,
		"Armor Bonus"
	},

	{
		"weapon_fist",
		NULL,
		NULL,
		IT_WEAPON,
		wp_fist,
		0,
		"Fist"
	},

	{
		"weapon_chainsaw",
		NULL,
		NULL,
		IT_WEAPON,
		wp_chainsaw,
		0,
		"Chainsaw"
	},

	{
		"weapon_pistol",
		NULL,
		NULL,
		IT_WEAPON,
		wp_pistol,
		0,
		"Pistol"
	},

	{
		"weapon_shotgun",
		NULL,
		NULL,
		IT_WEAPON,
		wp_shotgun,
		0,
		"Shotgun"
	},

	{
		"weapon_supershotgun",
		NULL,
		NULL,
		IT_WEAPON,
		wp_supershotgun,
		0,
		"Super Shotgun"
	},

	{
		"weapon_chaingun",
		NULL,
		NULL,
		IT_WEAPON,
		wp_chaingun,
		0,
		"Chaingun"
	},

	{
		"weapon_rocketlauncher",
		NULL,
		NULL,
		IT_WEAPON,
		wp_missile,
		0,
		"Rocket Launcher"
	},

	{
		"weapon_plasmagun",
		NULL,
		NULL,
		IT_WEAPON,
		wp_plasma,
		0,
		"Plasma Gun"
	},

	{
		"weapon_bfg",
		NULL,
		NULL,
		IT_WEAPON,
		wp_bfg,
		0,
		"BFG9000"
	},

	{
		"ammo_bullets",
		NULL,
		NULL,
		IT_AMMO,
		am_clip,
		1,
		"Bullets"
	},

	{
		"ammo_shells",
		NULL,
		NULL,
		IT_AMMO,
		am_shell,
		1,
		"Shells"
	},

	{
		"ammo_cells",
		NULL,
		NULL,
		IT_AMMO,
		am_cell,
		1,
		"Cells"
	},

	{
		"ammo_rocket",
		NULL,
		NULL,
		IT_AMMO,
		am_misl,
		1,
		"Rockets"
	},

	//
	// POWERUP ITEMS
	//
	{
		"item_invulnerability",
		NULL,
		NULL,
		IT_POWERUP,
		pw_invulnerability,
		0,
		"Invulnerability"
	},

	{
		"item_berserk",
		NULL,
		NULL,
		IT_POWERUP,
		pw_strength,
		0,
		"Berserk"
	},

	{
		"item_invisibility",
		NULL,
		NULL,
		IT_POWERUP,
		pw_invisibility,
		0,
		"Invisibility"
	},

	{
		"item_ironfeet",
		NULL,
		NULL,
		IT_POWERUP,
		pw_ironfeet,
		0,
		"Radiation Suit"
	},

	{
		"item_allmap",
		NULL,
		NULL,
		IT_POWERUP,
		pw_allmap,
		0,
		"Computer Map"
	},

	{
		"item_visor",
		NULL,
		NULL,
		IT_POWERUP,
		pw_infrared,
		0,
		"Light Amplification Visor"
	},

	//
	// KEYS
	//

	{
		"key_blue_card",
		NULL,
		NULL,
		IT_KEY,
		it_bluecard,
		0,
		"Blue Keycard"
	},

	{
		"key_yellow_card",
		NULL,
		NULL,
		IT_KEY,
		it_yellowcard,
		0,
		"Yellow Keycard"
	},

	{
		"key_red_card",
		NULL,
		NULL,
		IT_KEY,
		it_redcard,
		0,
		"Red Keycard"
	},

	{
		"key_blue_skull",
		NULL,
		NULL,
		IT_KEY,
		it_blueskull,
		0,
		"Blue Skull Key"
	},

	{
		"key_yellow_skull",
		NULL,
		NULL,
		IT_KEY,
		it_yellowskull,
		0,
		"Yellow Skull Key"
	},

	{
		"key_red_skull",
		NULL,
		NULL,
		IT_KEY,
		it_redskull,
		0,
		"Red Skull Key"
	},

	// ---------------------------------------------------------------------------------------------------------
	// [Toke - CTF] CTF Flags

	{
		"blue_flag",
		NULL,
		NULL,
		IT_FLAG,
		TEAM_BLUE,
		0,
		"Blue Flag"
	},


	{
		"red_flag",
		NULL,
		NULL,
		IT_FLAG,
		TEAM_RED,
		0,
		"Red Flag"
	},

	{
		"green_flag",
		NULL,
		NULL,
		IT_FLAG,
		TEAM_GREEN,
		0,
		"Green Flag"
	},
				// end of list marker
	{
	    "",
	    NULL,
	    NULL,
	    0,
	    0,
	    0,
	    ""
    }
};

void InitItems (void)
{
	num_items = sizeof(itemlist)/sizeof(itemlist[0]) - 1;
}


VERSION_CONTROL (d_items_cpp, "$Id$")

