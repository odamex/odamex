// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 2006-2020 by The Odamex Team.
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
//
weaponinfo_t	weaponinfo[NUMWEAPONS+1] =
{
	{
		// fist
		am_noammo,
		S_PUNCHUP,
		S_PUNCHDOWN,
		S_PUNCH,
		S_PUNCH1,
		S_NULL,
		(mobjtype_t)0,
		0,
		0,
		WPF_FLEEMELEE | WPF_AUTOSWITCHFROM | WPF_NOAUTOSWITCHTO
		},	
	{
		// pistol
		am_clip,
		S_PISTOLUP,
		S_PISTOLDOWN,
		S_PISTOL,
		S_PISTOL1,
		S_PISTOLFLASH,
		MT_CLIP,
		1,
		1,
		WPF_AUTOSWITCHFROM
	},	
	{
		// shotgun
		am_shell,
		S_SGUNUP,
		S_SGUNDOWN,
		S_SGUN,
		S_SGUN1,
		S_SGUNFLASH1,
		MT_SHOTGUN,
		1,
		1,
		WPF_NOFLAG
	},
	{
		// chaingun
		am_clip,
		S_CHAINUP,
		S_CHAINDOWN,
		S_CHAIN,
		S_CHAIN1,
		S_CHAINFLASH1,
		MT_CHAINGUN,
		1,
		1,
		WPF_NOFLAG
	},
	{
		// missile launcher
		am_misl,
		S_MISSILEUP,
		S_MISSILEDOWN,
		S_MISSILE,
		S_MISSILE1,
		S_MISSILEFLASH1,
		MT_MISC27,
		1,
		1,
		WPF_NOAUTOFIRE
	},
	{
		// plasma rifle
		am_cell,
		S_PLASMAUP,
		S_PLASMADOWN,
		S_PLASMA,
		S_PLASMA1,
		S_PLASMAFLASH1,
		MT_MISC28,
		1,
		1,
		WPF_NOFLAG
	},
	{
		// bfg 9000
		am_cell,
		S_BFGUP,
		S_BFGDOWN,
		S_BFG,
		S_BFG1,
		S_BFGFLASH1,
		MT_MISC25,
		40,
		40,
		WPF_NOAUTOFIRE
	},
	{
		// chainsaw
		am_noammo,
		S_SAWUP,
		S_SAWDOWN,
		S_SAW,
		S_SAW1,
		S_NULL,
		MT_MISC26,
		0,
		0,
		WPF_NOTHRUST | WPF_FLEEMELEE | WPF_NOAUTOSWITCHTO
	},
	{
		// super shotgun
		am_shell,
		S_DSGUNUP,
		S_DSGUNDOWN,
		S_DSGUN,
		S_DSGUN1,
		S_DSGUNFLASH1,
		MT_SUPERSHOTGUN,
		2,
		2,
		WPF_NOFLAG
	},
	{
		//NUMWEAPONS (player has no weapon including fist, ClearInventory)
		am_noammo,
		S_NOWEAPONUP,
		S_NOWEAPONDOWN,
		S_NOWEAPON,
		S_NOWEAPON,
		S_NOWEAPON,
		MT_MISC26,
		0,
		0,
		WPF_NOFLAG
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
		if (it->flags == IT_KEY && (card_t)it->offset == card)
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

void InitItems()
{
	num_items = sizeof(itemlist)/sizeof(itemlist[0]) - 1;
}


VERSION_CONTROL (d_items_cpp, "$Id$")

