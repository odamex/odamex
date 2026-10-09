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

#pragma once

#include <array>
#include <unordered_map>
#include <vector>

#include "info.h"
#include "m_doomobjcontainer.h"

class AActor;
class player_t;

// Weapon info: sprite frames, ammunition use.
struct weaponinfo_t
{
	int32_t     id = -1;    // index this weapon is registered under

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
	int ammopershot; // works slightly different from zdoom ammouse, and needs different defaults

	int internalflags;

	// ID24 Specifications
	int  slot                     = -1;  // which slot this weapon binds to
	int  slotpriority             = -1;  // selection priority within that slot
	int  switchpriority           = -1;  // priority when autoswitching
	bool initialowned             = false;
	bool initialraised            = false;
	int  allowswitchifownedweapon = -1;
	int  noswitchifownedweapon    = -1;
	int  allowswitchifowneditem   = -1;
	int  noswitchifowneditem      = -1;
	OLumpName carouselicon        = "SMUNKN";
};

// Ammo info.
//
// Vanilla only had a per-clip amount and a maximum.
// ID24 makes every pickup quantity and the per-skill
// multipliers independently definable.
struct ammoinfo_t
{
	int32_t id = -1;        // index this ammo type is registered under

	int clipammo = 0;       // "Per ammo"
	int maxammo  = 0;       // "Max ammo"

	// ID24 Specifications
	int initialammo          = 0;
	int maxupgradedammo      = 0;
	int boxammo              = 0;
	int backpackammo         = 0;
	int weaponammo           = 0;
	int droppedclipammo      = 0;
	int droppedboxammo       = 0;
	int droppedbackpackammo  = 0;
	int droppedweaponammo    = 0;
	int deathmatchweaponammo = 0;

	// Multiplier applied to collected ammo counts, one per skill level.
	std::array<fixed_t, 5> skillmult = {2 * FRACUNIT, FRACUNIT, FRACUNIT,
	                                    FRACUNIT, 2 * FRACUNIT};

	// Fills in every derived quantity from clipammo/maxammo using the vanilla
	// relationships.
	// Per ID24 this runs whenever a DeHackEd Ammo block sets only "Per ammo"
	// and/or "Max ammo".
	void deriveQuantities();
};

// Maps a sparse weapon or ammo index onto a dense, allocation-ordered slot.
// Weapon IDs are dynamic and must match between client and server,
// so this exists in both.
class DehSlotMap
{
  public:
	template <typename ObjType>
	void rebuild(const DoomObjectContainer<ObjType, int32_t>& container)
	{
		m_indexAtSlot.clear();
		m_slotOfIndex.clear();

		container.forEachInOrder([this](const ObjType& obj) {
			if (m_slotOfIndex.try_emplace(obj.id, int(m_indexAtSlot.size())).second)
				m_indexAtSlot.push_back(obj.id);
		});

		// Objects allocated at their own slot number can skip the hash lookup.
		m_identityCount = 0;
		while (size_t(m_identityCount) < m_indexAtSlot.size() &&
		       m_indexAtSlot[m_identityCount] == m_identityCount)
		{
			m_identityCount++;
		}
	}

	size_t  size() const { return m_indexAtSlot.size(); }
	int32_t indexAtSlot(size_t slot) const { return m_indexAtSlot[slot]; }

	// Returns the slot for an index, or -1 if the index is not registered.
	int slotOf(int32_t idx) const
	{
		if (uint32_t(idx) < uint32_t(m_identityCount))
			return int(idx);

		const auto it = m_slotOfIndex.find(idx);
		return it != m_slotOfIndex.end() ? it->second : -1;
	}

	bool contains(int32_t idx) const { return slotOf(idx) >= 0; }

  private:
	std::vector<int32_t>           m_indexAtSlot;
	std::unordered_map<int32_t, int> m_slotOfIndex;
	int32_t                        m_identityCount = 0;
};

extern DoomObjectContainer<weaponinfo_t, int32_t> weaponinfo;
extern DoomObjectContainer<ammoinfo_t, int32_t>   ammoinfo;

extern DehSlotMap WeaponSlots;
extern DehSlotMap AmmoSlots;

// Installs the built-in weapon and ammo tables.
void D_InitWeaponAmmoTables();

// Rebuilds the weapon and ammo slot maps.
//
// Must be called after loading a WAD with ID24 DeHacked.
void D_RebuildWeaponAmmoSlots();

// Weapon Flags (MBF21 SPECS)
#define WPF_NOFLAG			0
#define WPF_NOTHRUST		BIT(0)
#define WPF_SILENT			BIT(1)
#define WPF_NOAUTOFIRE		BIT(2)
#define WPF_FLEEMELEE		BIT(3)
#define WPF_AUTOSWITCHFROM	BIT(4)
#define WPF_NOAUTOSWITCHTO	BIT(5)

// Weapon internal flags
#define WIF_NOFLAG			0
#define WIF_ENABLEAPS		BIT(0)


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
		bool	 		(*pickup)(player_t *ent, class AActor *other);
		void			(*use)(player_t *ent, struct gitem_s *item);
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
