// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 2006-2024 by The Odamex Team.
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

#pragma once

#include "info.h"

class AActor;
class player_s;

// Weapon info: sprite frames, ammunition use.
struct weaponinfo_s
{
	ammotype_t	ammotype;
	statenum_t	upstate;
	statenum_t	downstate;
	statenum_t 	readystate;
	statenum_t	atkstate;
	statenum_t	flashstate;
	mobjtype_t	droptype;
	int			ammouse;
	int			minammo;

	// MBF21 Specifications
	int flags;
};
typedef struct weaponinfo_s weaponinfo_t;

extern	weaponinfo_t	weaponinfo[NUMWEAPONS+1];

// Weapon Flags (MBF21 SPECS)
#define WPF_NOFLAG			0
#define WPF_NOTHRUST		BIT(0)
#define WPF_SILENT			BIT(1)
#define WPF_NOAUTOFIRE		BIT(2)	
#define WPF_FLEEMELEE		BIT(3)	// Not Implemented yet (Missing MBF code?)...
#define WPF_AUTOSWITCHFROM	BIT(4)
#define WPF_NOAUTOSWITCHTO	BIT(5)



// Item stuff: (this is d_items.h, right?)

// gitem_t->flags
#define IT_WEAPON				1				// use makes active weapon
#define IT_AMMO 				2
#define IT_ARMOR				4
#define IT_KEY					8
#define IT_FLAG		 			16				// [Toke - CTF] Renamed this flag, it was not being used
#define IT_POWERUP				32				// Auto-activate item


struct gitem_s
{
		const char		*classname;
		BOOL	 		(*pickup)(player_s *ent, class AActor *other);
		void			(*use)(player_s *ent, struct gitem_s *item);
		byte			flags;
		byte			offset; 				// For Weapon, Ammo, Armor, Key: Offset in appropriate table
		byte			quantity;				// For Ammo: How much to pickup

		const char		*pickup_name;
};
typedef struct gitem_s gitem_t;

extern int num_items;

extern gitem_t itemlist[];

void InitItems (void);

// FindItem
gitem_t	*GetItemByIndex (int index);
gitem_t	*FindItemByClassname (const char *classname);
gitem_t *FindItem (const char *pickup_name);

gitem_t* FindCardItem(card_t card);

#define ITEM_INDEX(i)	((i)-itemlist)
