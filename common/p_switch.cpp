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
//	Switches, buttons. Two-state animation. Exits.
//
//-----------------------------------------------------------------------------


#include <map>

#include "doomdef.h"
#include "p_local.h"
#include "p_lnspec.h"
#include "s_sound.h"
#include "doomstat.h"
#include "r_state.h"
#include "z_zone.h"
#include "gi.h"
#include "m_ostring.h"
#include "resources/res_main.h"
#include "resources/res_texture.h"

EXTERN_CVAR(co_zdoomsound)
extern int numtextures;

//
// CHANGE THE TEXTURE OF A WALL SWITCH TO ITS OPPOSITE
//

class DActiveButton : public DThinker
{
	DECLARE_SERIAL (DActiveButton, DThinker);
public:
	enum EWhere
	{
		BUTTON_Top,
		BUTTON_Middle,
		BUTTON_Bottom,
		BUTTON_Nowhere
	};

	DActiveButton ();
	DActiveButton (line_t *, EWhere, ResourceId res_id, SDWORD time, fixed_t x, fixed_t y);

	void RunThink ();

	line_t	*m_Line;
	EWhere	m_Where;
	ResourceId m_ResId;
	SDWORD	m_Timer;
	fixed_t	m_X, m_Y;	// Location of timer sound

	friend FArchive &operator<< (FArchive &arc, EWhere where)
	{
		return arc << (BYTE)where;
	}
	friend FArchive &operator>> (FArchive &arc, EWhere &out)
	{
		BYTE in; arc >> in; out = (EWhere)in; return arc;
	}
};

static ResourceId* switchlist;
static int  numswitches;

//
// P_InitSwitchList
// Only called at game initialization.
//
// [RH] Rewritten to use a BOOM-style SWITCHES lump and remove the
//		MAXSWITCHES limit.
void P_InitSwitchList(void)
{
	const ResourceId res_id = Res_GetResourceId("SWITCHES", global_directory_name);
	const byte *alphSwitchList = (byte*)Res_LoadResource(res_id, PU_STATIC);
	const byte *list_p;
	int i;

	for (i = 0, list_p = alphSwitchList; list_p[18] || list_p[19]; list_p += 20, i++)
		;

	if (i == 0)
	{
		switchlist = (ResourceId *)Z_Malloc(sizeof(*switchlist), PU_STATIC, 0);
		*switchlist = -1;
		numswitches = 0;
	}
	else
	{
		switchlist = (ResourceId *)Z_Malloc(sizeof(*switchlist)*(i*2+1), PU_STATIC, 0);

		for (i = 0, list_p = alphSwitchList; list_p[18] || list_p[19]; list_p += 20)
		{
			if (((gameinfo.maxSwitch & 15) >= (list_p[18] & 15)) &&
				((gameinfo.maxSwitch & ~15) == (list_p[18] & ~15)))
			{
				// [RH] Skip this switch if it can't be found.
				const ResourceId texture1 = Res_GetTextureResourceId(OStringToUpper((const char*)list_p + 0, 8), WALL);
				const ResourceId texture2 = Res_GetTextureResourceId(OStringToUpper((const char*)list_p + 9, 8), WALL);

				if (texture1 != ResourceId::INVALID_ID)
				{
					switchlist[i++] = texture1;
					switchlist[i++] = texture2;
				}
			}
		}
		numswitches = i/2;
		switchlist[i] = -1;
	}

	Res_ReleaseResource(res_id);
}

//
// Start a button counting down till it turns off.
// [RH] Rewritten to remove MAXBUTTONS limit and use temporary soundorgs.
//
static void P_StartButton (line_t *line, DActiveButton::EWhere w, ResourceId res_id,
						   int time, fixed_t x, fixed_t y)
{
	DActiveButton *button;
	TThinkerIterator<DActiveButton> iterator;

	// See if button is already pressed
	while ( (button = iterator.Next ()) )
	{
		if (button->m_Line == line)
			return;
	}

	new DActiveButton (line, w, res_id, time, x, y);
}

ResourceId* P_GetButtonTexturePtr(line_t* line, ResourceId* alt_texture, DActiveButton::EWhere& where)
{
	where = (DActiveButton::EWhere)0;

	side_t* side = &sides[line->sidenum[0]];

	for (int i = 0; i < numswitches * 2; i++)
	{
		if (switchlist[i] == side->toptexture)
		{
			*alt_texture = switchlist[i ^ 1];
			where = DActiveButton::BUTTON_Top;
			return &side->toptexture;
		}
		else if (switchlist[i] == side->bottomtexture)
		{
			*alt_texture = switchlist[i ^ 1];
			where = DActiveButton::BUTTON_Bottom;
			return &side->bottomtexture;
		}
		else if (switchlist[i] == side->midtexture)
		{
			*alt_texture = switchlist[i ^ 1];
			where = DActiveButton::BUTTON_Middle;
			return &side->midtexture;
		}
	}

	return NULL;
}

const ResourceId* P_GetButtonTexturePtr(const line_t* line, ResourceId* alt_texture, DActiveButton::EWhere& where)
{
	return const_cast<ResourceId*>(P_GetButtonTexturePtr(const_cast<line_t*>(line), alt_texture, where));
}

const ResourceId P_GetButtonTexture(const line_t* line)
{
	DActiveButton::EWhere twhere;
	ResourceId alt_res_id;
	const ResourceId* res_id_ptr = P_GetButtonTexturePtr(line, &alt_res_id, twhere);
	if (res_id_ptr != NULL)
		return *res_id_ptr;
	return ResourceId::INVALID_ID;
}

void P_SetButtonTexture(line_t* line, const ResourceId new_res_id)
{
	if (new_res_id != ResourceId::INVALID_ID)
	{
		DActiveButton::EWhere twhere;
		ResourceId alt_res_id;
		ResourceId* res_id_ptr = (ResourceId*)P_GetButtonTexturePtr(line, &alt_res_id, twhere);

		if (res_id_ptr != NULL && *res_id_ptr != ResourceId::INVALID_ID)
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
			button->m_Where = (DActiveButton::EWhere)state;
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

		MSG_WriteMarker(&cl->reliablebuf, svc_switch);
		MSG_WriteLong(&cl->reliablebuf, l);
		MSG_WriteByte(&cl->reliablebuf, lines[l].switchactive);
		MSG_WriteByte(&cl->reliablebuf, lines[l].special);
		MSG_WriteByte(&cl->reliablebuf, state);
		MSG_WriteShort(&cl->reliablebuf, P_GetButtonTexture(&lines[l]));
		MSG_WriteLong(&cl->reliablebuf, timer);
	}

	for (int l=0; l<numlines; l++)
	{
		// update all button state except those that have actors assigned:
		if (!actedlines[l] && lines[l].wastoggled)
		{
			MSG_WriteMarker(&cl->reliablebuf, svc_switch);
			MSG_WriteLong(&cl->reliablebuf, l);
			MSG_WriteByte(&cl->reliablebuf, lines[l].switchactive);
			MSG_WriteByte(&cl->reliablebuf, lines[l].special);
			MSG_WriteByte(&cl->reliablebuf, 0);
			MSG_WriteShort(&cl->reliablebuf, P_GetButtonTexture(&lines[l]));
			MSG_WriteLong(&cl->reliablebuf, 0);
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

	if (!useAgain)
		line->special = 0;

	// EXIT SWITCH?
	if (line->special == Exit_Normal ||
		line->special == Exit_Secret ||
		line->special == Teleport_NewMap ||
		line->special == Teleport_EndGame)
		sound = "switches/exitbutn";
	else
		sound = "switches/normbutn";

	DActiveButton::EWhere twhere;
	ResourceId alt_res_id = ResourceId::INVALID_ID;
	ResourceId* res_id_ptr = P_GetButtonTexturePtr(line, &alt_res_id, twhere);

	if (res_id_ptr != NULL)
	{
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

		if (useAgain)
			P_StartButton(line, twhere, *res_id_ptr, BUTTONTIME, x, y);
		*res_id_ptr = alt_res_id;
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
}

DActiveButton::DActiveButton (line_t *line, EWhere where, ResourceId res_id,
							  SDWORD time, fixed_t x, fixed_t y)
{
	m_Line = line;
	m_Where = where;
	m_ResId = res_id;
	m_Timer = time;
	m_X = x;
	m_Y = y;
}

void DActiveButton::Serialize (FArchive &arc)
{
	Super::Serialize (arc);
	if (arc.IsStoring ())
	{
		SDWORD texture = (SDWORD)((uint32_t)m_ResId & 0xFFFF);
		arc << m_Line << m_Where << texture << m_Timer << m_X << m_Y;
	}
	else
	{
		SDWORD texture;
		arc >> m_Line >> m_Where >> texture >> m_Timer >> m_X >> m_Y;
		if (texture == 0xFFFF)
			m_ResId = ResourceId::INVALID_ID;
		else
			m_ResId = texture;
	}
}

void DActiveButton::RunThink ()
{
	if (0 >= --m_Timer)
	{
		switch (m_Where)
		{
		case BUTTON_Top:
			sides[m_Line->sidenum[0]].toptexture = m_ResId;
			break;

		case BUTTON_Middle:
			sides[m_Line->sidenum[0]].midtexture = m_ResId;
			break;

		case BUTTON_Bottom:
			sides[m_Line->sidenum[0]].bottomtexture = m_ResId;
			break;

		default:
			break;
		}

		if (co_zdoomsound)
		{
			// [SL] 2011-05-27 - Play at a normal volume in the center of the
			//  switch's linedef
			fixed_t x = m_Line->v1->x + (m_Line->dx >> 1);
			fixed_t y = m_Line->v1->y + (m_Line->dy >> 1);
			S_Sound (x, y, CHAN_BODY, "switches/normbutn", 1, ATTN_NORM);
		}
		else
		{
			// [SL] 2011-05-16 - Changed to play the switch resetting sound at the
			// map location 0,0 to emulate the bug in vanilla doom
			S_Sound (0, 0, CHAN_BODY, "switches/normbutn", 1, ATTN_NORM);
		}
		Destroy ();
		m_Line->switchactive = false;
	}
}

VERSION_CONTROL (p_switch_cpp, "$Id$")

