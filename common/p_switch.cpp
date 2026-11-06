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
//	Switches, buttons. Two-state animation. Exits.
//
//-----------------------------------------------------------------------------


#include "odamex.h"

#include <algorithm>
#include <vector>

#include "p_local.h"
#include "p_lnspec.h"
#include "s_sound.h"
#include "r_state.h"
#include "z_zone.h"
#include "gi.h"
#include "svc_message.h"
#include "p_mapformat.h"
#include "m_ostring.h"
#include "m_random.h"
#include "cmdlib.h"
#include "oscanner.h"
#include "resources/res_main.h"
#include "resources/res_texture.h"

EXTERN_CVAR(co_zdoomsound)

//
// CHANGE THE TEXTURE OF A WALL SWITCH TO ITS OPPOSITE
//

class DActiveButton : public DThinker
{
	DECLARE_SERIAL (DActiveButton, DThinker);
public:
	enum EWhere : uint8_t
	{
		BUTTON_Top,
		BUTTON_Middle,
		BUTTON_Bottom,
		BUTTON_Nowhere
	};

	DActiveButton ();
	DActiveButton (line_t *, EWhere, int32_t switch_def, ResourceId res_id, bool flippable,
	               fixed_t x, fixed_t y);

	void RunThink () override;

	line_t	*m_Line;
	EWhere	m_Where;
	ResourceId m_ResId;	// texture before the press, restored if no pair leads back
	int32_t	m_Timer;
	fixed_t	m_X, m_Y;	// Location of timer sound
	int32_t	m_SwitchDef;	// index into switchdefs of the frames playing
	int32_t	m_Frame;
	bool	m_Flippable;	// returns once the switch has been on for BUTTONTIME
	bool	m_Returning;

private:
	void setTexture(ResourceId res_id);
	bool startFrame();
};

//
// What pressing a switch showing its pre-switched texture does.
//
// The frames play in order and the switch stays on the last one.
// 
// The pair is the definition for that last texture, which plays
// when a reusable switch returns.
//
// The SWITCHES lump gives each definition a single frame.
// ANIMDEFS switch definitions can have several, and their own sounds.
//
struct switchframe_t
{
	ResourceId texture;
	int32_t time_min;
	int32_t time_rnd;	// adds 0 to time_rnd - 1 tics
};

struct switchdef_t
{
	ResourceId pretexture;
	std::vector<switchframe_t> frames;
	OString sound;		// empty for the usual switch sounds
	int32_t pair = -1;	// index into switchdefs
};

static std::vector<switchdef_t> switchdefs;

// As in ZDoom, a sound that doesn't exist gets the usual switch sound instead.
bool P_SwitchSoundExists(const switchdef_t& def)
{
	return not def.sound.empty() and S_FindSound(def.sound.c_str()) != -1;
}

int32_t P_FindSwitchDef(const ResourceId pretexture)
{
	for (size_t i = 0; i < switchdefs.size(); i++)
		if (switchdefs[i].pretexture == pretexture)
			return static_cast<int32_t>(i);
	return -1;
}

int32_t P_StoreSwitchDef(const switchdef_t& def, const int32_t index)
{
	if (index >= 0)
	{
		switchdefs[index] = def;
		return index;
	}
	switchdefs.push_back(def);
	return static_cast<int32_t>(switchdefs.size() - 1);
}

//
// P_AddSwitchPair
//
// Adds a switch's on and off definitions.
//
// When replace is on, they take over from any definitions
// for the same textures, same behavior as ZDoom.
// 
// Without it a texture keeps the first pair that names it -- vanilla behavior.
//
void P_AddSwitchPair(const switchdef_t& on, const switchdef_t& off, bool replace)
{
	int32_t on_index = P_FindSwitchDef(on.pretexture);
	int32_t off_index = P_FindSwitchDef(off.pretexture);

	if (not replace and (on_index >= 0 or off_index >= 0))
	{
		if (on_index < 0)
			P_StoreSwitchDef(on, -1);
		if (off_index < 0)
			P_StoreSwitchDef(off, -1);
		return;
	}

	on_index = P_StoreSwitchDef(on, on_index);
	off_index = P_StoreSwitchDef(off, off_index);

	// A replaced definition no longer leads back to its old pair.
	for (size_t i = 0; i < switchdefs.size(); i++)
	{
		if (switchdefs[i].pair == on_index or switchdefs[i].pair == off_index)
			switchdefs[i].pair = -1;
	}

	switchdefs[on_index].pair = off_index;
	switchdefs[off_index].pair = on_index;
}

//
// P_InitSwitchList
// Only called at game initialization.
//
// [RH] Rewritten to use a BOOM-style SWITCHES lump and remove the
//		MAXSWITCHES limit.
void P_InitSwitchList(void)
{
	switchdefs.clear();

	const ResourceId res_id = Res_GetResourceId("SWITCHES", NS_GLOBAL);
	const byte *alphSwitchList = Res_LoadResource<byte>(res_id, PU_STATIC);

	for (const byte* list_p = alphSwitchList; list_p[18] || list_p[19]; list_p += 20)
	{
		if (((gameinfo.maxSwitch & 15) >= (list_p[18] & 15)) &&
			((gameinfo.maxSwitch & ~15) == (list_p[18] & ~15)))
		{
			// [RH] Skip this switch if it can't be found.
			const ResourceId texture1 = Res_GetTextureResourceId(reinterpret_cast<const char*>(list_p) + 0, WALL, false);
			const ResourceId texture2 = Res_GetTextureResourceId(reinterpret_cast<const char*>(list_p) + 9, WALL, false);

			if (Res_CheckResource(texture1) and Res_CheckResource(texture2))
			{
				switchdef_t on, off;
				on.pretexture = texture1;
				on.frames.push_back({texture2, 0, 0});
				off.pretexture = texture2;
				off.frames.push_back({texture1, 0, 0});
				P_AddSwitchPair(on, off, false);
			}
		}
	}

	Res_ReleaseResource(res_id);
}

//
// P_ParseSwitchState
//
// Reads the on or off state of an ANIMDEFS switch definition:
//   [sound NAME] pic NAME tics N | range MIN MAX [pic ...]
// Returns false if a frame's texture is missing.
//
bool P_ParseSwitchState(OScanner& os, switchdef_t& def)
{
	bool frames_exist = true;

	while (os.scan())
	{
		if (os.compareTokenNoCase("sound"))
		{
			if (not def.sound.empty())
				os.error("Switch state already has a sound");
			os.mustScan();
			def.sound = os.getToken();
		}
		else if (os.compareTokenNoCase("pic"))
		{
			switchframe_t frame;
			os.mustScan();
			frame.texture = Res_GetTextureResourceId(OStringToUpper(os.getToken()), WALL, false);
			if (not Res_CheckResource(frame.texture))
				frames_exist = false;

			os.mustScan();
			if (os.compareTokenNoCase("tics"))
			{
				os.mustScanInt();
				frame.time_min = std::clamp(os.getTokenInt(), 0, 65535);
				frame.time_rnd = 0;
			}
			else if (os.compareTokenNoCase("range") or os.compareTokenNoCase("rand"))
			{
				os.mustScanInt();
				int time_min = std::clamp(os.getTokenInt(), 0, 65535);
				os.mustScanInt();
				int time_max = std::clamp(os.getTokenInt(), 0, 65535);
				if (time_min > time_max)
					std::swap(time_min, time_max);
				frame.time_min = time_min;
				frame.time_rnd = time_max - time_min + 1;
			}
			else
			{
				os.error("Must specify a duration for switch frame");
			}
			def.frames.push_back(frame);
		}
		else
		{
			os.unScan();
			break;
		}
	}

	if (def.frames.empty())
		os.error("Switch state needs at least one frame");

	return frames_exist;
}

//
// P_ProcessSwitchDef
//
// Reads an ANIMDEFS switch definition, following the "switch" keyword:
//   switch [doom [N]|heretic|hexen|strife|any] NAME [quest] on STATE [off STATE]
// The on state plays when a switch showing NAME is pressed and must end on a
// different texture.
// The off state plays when a reusable switch returns, and without one the switch
// goes straight back to NAME.
//
void P_ProcessSwitchDef(OScanner& os)
{
	bool this_game = true;

	os.mustScan();
	if (os.compareTokenNoCase("doom"))
	{
		// The episode number that may follow is deprecated.
		os.mustScan();
		if (not IsNum(os.getToken()))
			os.unScan();
	}
	else if (os.compareTokenNoCase("heretic") or os.compareTokenNoCase("hexen") or
	         os.compareTokenNoCase("strife"))
	{
		this_game = false;
	}
	else if (not os.compareTokenNoCase("any"))
	{
		os.unScan();
	}

	os.mustScan();
	const OString name = OStringToUpper(os.getToken());
	const ResourceId pretexture = Res_GetTextureResourceId(name, WALL, false);

	switchdef_t on, off;
	on.pretexture = pretexture;
	bool has_on = false, has_off = false, frames_exist = true;

	while (os.scan())
	{
		if (os.compareTokenNoCase("quest"))
		{
			// Strife's quest panels don't exist here.
			continue;
		}
		else if (os.compareTokenNoCase("on"))
		{
			if (has_on)
				os.error("Switch already has an on state");
			has_on = true;
			if (not P_ParseSwitchState(os, on))
				frames_exist = false;
		}
		else if (os.compareTokenNoCase("off"))
		{
			if (has_off)
				os.error("Switch already has an off state");
			has_off = true;
			if (not P_ParseSwitchState(os, off))
				frames_exist = false;
		}
		else
		{
			os.unScan();
			break;
		}
	}

	if (not this_game or not has_on or not Res_CheckResource(pretexture))
		return;

	if (not frames_exist)
	{
		PrintFmt(PRINT_HIGH, "Missing texture for switch {} in ANIMDEFS\n", name.c_str());
		return;
	}

	if (not has_off)
	{
		off.frames.push_back({pretexture, 0, 0});
		off.sound = on.sound;
	}
	off.pretexture = on.frames.back().texture;

	if (off.pretexture == pretexture)
		os.error("The on state for switch {} must end with a texture other than {}", name.c_str(), name.c_str());

	P_AddSwitchPair(on, off, true);
}

void P_DestroyButtonThinkers()
{
	DActiveButton *button;
	TThinkerIterator<DActiveButton> iterator;

	while ((button = iterator.Next()))
		button->Destroy();
}

//
// Start a button counting down till it turns off.
// [RH] Rewritten to remove MAXBUTTONS limit and use temporary soundorgs.
//
void P_StartButton (line_t *line, DActiveButton::EWhere w, int32_t switch_def,
    ResourceId res_id, bool flippable, fixed_t x, fixed_t y)
{
	DActiveButton *button;
	TThinkerIterator<DActiveButton> iterator;

	// See if button is already pressed
	while ( (button = iterator.Next ()) )
	{
		if (button->m_Line == line)
			return;
	}

	new DActiveButton (line, w, switch_def, res_id, flippable, x, y);
}

bool P_SwitchDefShows(const switchdef_t& def, const ResourceId res_id, bool any_frame)
{
	if (def.pretexture == res_id)
		return true;

	if (any_frame)
	{
		for (const switchframe_t& frame : def.frames)
			if (frame.texture == res_id)
				return true;
	}

	return false;
}

//
// P_GetButtonTexturePtr
//
// Finds the part of the line's front side that shows a switch.
//
// When any_frame is false, only a texture that can be pressed counts,
// and switch_def is set to its definition.
//
// With it, a texture can be switched partway through an animation.
//
ResourceId* P_GetButtonTexturePtr(line_t* line, DActiveButton::EWhere& where,
                                     bool any_frame, int32_t* switch_def)
{
	where = static_cast<DActiveButton::EWhere>(0);

	if (line->sidenum[0] == R_NOSIDE)
		return NULL;

	side_t* side = &sides[line->sidenum[0]];

	for (size_t i = 0; i < switchdefs.size(); i++)
	{
		const switchdef_t& def = switchdefs[i];
		ResourceId* res_id_ptr = nullptr;

		if (P_SwitchDefShows(def, side->toptexture, any_frame))
		{
			where = DActiveButton::BUTTON_Top;
			res_id_ptr = &side->toptexture;
		}
		else if (P_SwitchDefShows(def, side->bottomtexture, any_frame))
		{
			where = DActiveButton::BUTTON_Bottom;
			res_id_ptr = &side->bottomtexture;
		}
		else if (P_SwitchDefShows(def, side->midtexture, any_frame))
		{
			where = DActiveButton::BUTTON_Middle;
			res_id_ptr = &side->midtexture;
		}

		if (res_id_ptr != nullptr)
		{
			if (switch_def != nullptr)
				*switch_def = static_cast<int32_t>(i);
			return res_id_ptr;
		}
	}

	return NULL;
}

const ResourceId P_GetButtonTexture(const line_t* line)
{
	DActiveButton::EWhere twhere;
	const ResourceId* res_id_ptr =
	    P_GetButtonTexturePtr(const_cast<line_t*>(line), twhere, true, nullptr);
	if (res_id_ptr != nullptr)
		return *res_id_ptr;
	return ResourceId::INVALID_ID;
}

void P_SetButtonTexture(line_t* line, const ResourceId new_res_id)
{
	if (Res_CheckResource(new_res_id))
	{
		DActiveButton::EWhere twhere;
		ResourceId* res_id_ptr = P_GetButtonTexturePtr(line, twhere, true, nullptr);

		if (res_id_ptr != nullptr and Res_CheckResource(*res_id_ptr))
			*res_id_ptr = new_res_id;
	}
}

// denis - query button
bool P_GetButtonInfo(const line_t *line, unsigned &state, unsigned &time)
{
	DActiveButton *button;
	TThinkerIterator<DActiveButton> iterator;

	// See if button is already pressed
	while ( (button = iterator.Next ()) )
	{
		if (button->m_Line == line)
		{
			state = button->m_Where;
			time = button->m_Timer;
			return true;
		}
	}

	return false;
}

bool P_SetButtonInfo(line_t *line, unsigned state, unsigned time)
{
	DActiveButton *button;
	TThinkerIterator<DActiveButton> iterator;

	// See if button is already pressed
	while ( (button = iterator.Next ()) )
	{
		if (button->m_Line == line)
		{
			button->m_Where = static_cast<DActiveButton::EWhere>(state);
			button->m_Timer = time;
			return true;
		}
	}

	return false;
}

void P_UpdateButtons(client_t *cl)
{
	DActiveButton *button;
	TThinkerIterator<DActiveButton> iterator;
	std::map<unsigned, bool> actedlines;

	// See if button is already pressed
	while ( (button = iterator.Next ()) )
	{
		if (button->m_Line == NULL) continue;

		unsigned l = button->m_Line - lines;
		unsigned state = 0, timer = 0;

		state = button->m_Where;
		timer = button->m_Timer;

		// record that we acted on this line:
		actedlines[l] = true;

		cl->messenger->Reliable().Write(SVC_Switch(lines[l], state, timer));
	}

	for (int l=0; l<numlines; l++)
	{
		// update all button state except those that have actors assigned:
		if (!actedlines[l] && lines[l].wastoggled)
		{
			cl->messenger->Reliable().Write(SVC_Switch(lines[l], 0, 0));
		}
	}
}

//
// Function that changes wall texture.
// Tell it if switch is ok to use again (1=yes, it's a button).
//
void P_ChangeSwitchTexture(line_t* line, int useAgain, bool playsound)
{
	const char *sound;

	// EXIT SWITCH?
	if (P_IsExitLine(line->special))
	{
		sound = "switches/exitbutn";
	}
	else
	{
		sound = "switches/normbutn";
	}

	if (!useAgain && P_HandleSpecialRepeat(line))
		line->special = 0;

	DActiveButton::EWhere twhere;
	int32_t switch_def = -1;
	ResourceId* res_id_ptr = P_GetButtonTexturePtr(line, twhere, false, &switch_def);

	if (res_id_ptr != nullptr)
	{
		const switchdef_t& def = switchdefs[switch_def];
		if (P_SwitchSoundExists(def))
			sound = def.sound.c_str();

		// [RH] The original code played the sound at buttonlist->soundorg,
		//		which wasn't necessarily anywhere near the switch if
		//		it was facing a big sector.
		fixed_t x = line->v1->x + (line->dx >> 1);
		fixed_t y = line->v1->y + (line->dy >> 1);
		if (playsound)
		{
			if (co_zdoomsound)
			{
				// [SL] 2011-05-27 - Play at a normal volume in the center
				// of the switch's linedef
				S_Sound(x, y, CHAN_BODY, sound, 1, ATTN_NORM);
			}
			else
			{
				// [SL] 2011-05-16 - Reverted the code to play the sound at full
				// volume anywhere on the map to emulate vanilla doom behavior
				S_Sound(CHAN_BODY, sound, 1, ATTN_NONE);
			}
		}

		const ResourceId pretexture = *res_id_ptr;
		*res_id_ptr = def.frames[0].texture;
		if (useAgain or def.frames.size() > 1)
			P_StartButton(line, twhere, switch_def, pretexture, useAgain, x, y);
		line->switchactive = true;
	}

	line->wastoggled = true;
}


IMPLEMENT_SERIAL (DActiveButton, DThinker)

DActiveButton::DActiveButton ()
{
	m_Line = NULL;
	m_Where = BUTTON_Nowhere;
	m_ResId = ResourceId::INVALID_ID;
	m_Timer = 0;
	m_X = 0;
	m_Y = 0;
	m_SwitchDef = -1;
	m_Frame = 0;
	m_Flippable = false;
	m_Returning = false;
}

// The first frame of the switch definition is already showing.
DActiveButton::DActiveButton (line_t *line, EWhere where, int32_t switch_def, ResourceId res_id,
							  bool flippable, fixed_t x, fixed_t y)
{
	m_Line = line;
	m_Where = where;
	m_ResId = res_id;
	m_Timer = 0;
	m_X = x;
	m_Y = y;
	m_SwitchDef = switch_def;
	m_Frame = 0;
	m_Flippable = flippable;
	m_Returning = false;
	startFrame();
}

void DActiveButton::Serialize (FArchive &arc)
{
	Super::Serialize (arc);
	if (arc.IsStoring ())
	{
		int32_t texture =
		    static_cast<int32_t>((static_cast<uint32_t>(m_ResId) & 0xFFFF));
		arc << m_Line << m_Where << texture << m_Timer << m_X << m_Y
		    << m_SwitchDef << m_Frame << m_Flippable << m_Returning;
	}
	else
	{
		int32_t texture;
		arc >> m_Line >> m_Where >> texture >> m_Timer >> m_X >> m_Y
		    >> m_SwitchDef >> m_Frame >> m_Flippable >> m_Returning;
		if (texture == 0xFFFF)
			m_ResId = ResourceId::INVALID_ID;
		else
			m_ResId = texture;
	}
}

void DActiveButton::setTexture(const ResourceId res_id)
{
	switch (m_Where)
	{
	case BUTTON_Top:
		sides[m_Line->sidenum[0]].toptexture = res_id;
		break;

	case BUTTON_Middle:
		sides[m_Line->sidenum[0]].midtexture = res_id;
		break;

	case BUTTON_Bottom:
		sides[m_Line->sidenum[0]].bottomtexture = res_id;
		break;

	default:
		break;
	}
}

//
// DActiveButton::startFrame
//
// Times the frame that is showing. A reusable switch holds the last frame of
// its on state for BUTTONTIME. Returns false when nothing is left to do.
//
bool DActiveButton::startFrame()
{
	if (m_SwitchDef < 0 or static_cast<size_t>(m_SwitchDef) >= switchdefs.size())
		return false;

	const switchdef_t& def = switchdefs[m_SwitchDef];
	if (static_cast<size_t>(m_Frame) + 1 < def.frames.size())
	{
		const switchframe_t& frame = def.frames[m_Frame];
		m_Timer = frame.time_min;
		if (frame.time_rnd > 1)
			m_Timer += ((M_Random() << 8) | M_Random()) % frame.time_rnd;
		return true;
	}

	if (m_Flippable and not m_Returning)
	{
		m_Timer = BUTTONTIME;
		return true;
	}

	return false;
}

void DActiveButton::RunThink ()
{
	if (0 < --m_Timer)
		return;

	const bool valid_def =
	    m_SwitchDef >= 0 and static_cast<size_t>(m_SwitchDef) < switchdefs.size();

	if (valid_def and static_cast<size_t>(m_Frame) + 1 < switchdefs[m_SwitchDef].frames.size())
	{
		// The next frame of the state that is playing.
		m_Frame++;
		setTexture(switchdefs[m_SwitchDef].frames[m_Frame].texture);
		if (startFrame())
			return;
	}
	else if (not m_Returning)
	{
		// The switch has been on for BUTTONTIME, so it returns. Its pair plays
		// the way back, or without one it shows its old texture again.
		m_Returning = true;

		const char* sound = "switches/normbutn";
		const int32_t pair = valid_def ? switchdefs[m_SwitchDef].pair : -1;
		if (pair >= 0)
		{
			m_SwitchDef = pair;
			m_Frame = 0;
			setTexture(switchdefs[pair].frames[0].texture);
			if (P_SwitchSoundExists(switchdefs[pair]))
				sound = switchdefs[pair].sound.c_str();
		}
		else
		{
			setTexture(m_ResId);
		}

		if (co_zdoomsound)
		{
			// [SL] 2011-05-27 - Play at a normal volume in the center of the
			//  switch's linedef
			fixed_t x = m_Line->v1->x + (m_Line->dx >> 1);
			fixed_t y = m_Line->v1->y + (m_Line->dy >> 1);
			S_Sound (x, y, CHAN_BODY, sound, 1, ATTN_NORM);
		}
		else
		{
			// [SL] 2011-05-16 - Changed to play the switch resetting sound at the
			// map location 0,0 to emulate the bug in vanilla doom
			S_Sound (0, 0, CHAN_BODY, sound, 1, ATTN_NORM);
		}

		if (pair >= 0 and startFrame())
			return;
	}

	Destroy ();

	// A switch that doesn't return stays active.
	if (m_Returning)
		m_Line->switchactive = false;
}

VERSION_CONTROL (p_switch_cpp, "$Id$")
