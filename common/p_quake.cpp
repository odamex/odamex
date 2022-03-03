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
//   Earthquake implementation.
//
//-----------------------------------------------------------------------------


#include "odamex.h"

#include "p_local.h"
#include "actor.h"
#include "m_bbox.h"
#include "m_random.h"
#include "s_sound.h"

class DEarthquake : public DThinker
{
	DECLARE_SERIAL (DEarthquake, DThinker);
public:
	DEarthquake (AActor *center, int intensity, int duration, int damrad, int tremrad);
	virtual void RunThink ();
	virtual void DestroyedPointer(DObject *obj);

	AActor *m_Spot;
	fixed_t m_TremorBox[4];
	fixed_t m_DamageBox[4];
	int m_Intensity;
	int m_Countdown;
private:
	DEarthquake () {}
};

IMPLEMENT_SERIAL(DEarthquake, DThinker)

void DEarthquake::DestroyedPointer(DObject *obj)
{
	if(obj == m_Spot)
		m_Spot = NULL;
}

void DEarthquake::Serialize (FArchive &arc)
{
	int i;

	if (arc.IsStoring ())
	{
		arc << m_Spot << m_Intensity << m_Countdown;
		for (i = 0; i < 4; i++)
			arc << m_TremorBox[i] << m_DamageBox[i];
	}
	else
	{
		arc >> m_Spot >> m_Intensity >> m_Countdown;
		for (i = 0; i < 4; i++)
			arc >> m_TremorBox[i] >> m_DamageBox[i];
	}
}

void DEarthquake::RunThink ()
{
	if (level.time % 48 == 0)
		S_Sound (m_Spot, CHAN_BODY, "world/quake", 1, ATTN_NORM);
		
	if (serverside)
	{
		for (Players::iterator it = players.begin();it != players.end();++it)
		{
			if (it->ingame() && !(it->cheats & CF_NOCLIP))
			{
				AActor *mo = it->mo;

				if (!(level.time & 7) &&
					 mo->x >= m_DamageBox[BOXLEFT] && mo->x < m_DamageBox[BOXRIGHT] &&
					 mo->y >= m_DamageBox[BOXTOP] && mo->y < m_DamageBox[BOXBOTTOM])
				{
					int mult = 1024 * m_Intensity;
					P_DamageMobj (mo, NULL, NULL, m_Intensity / 2, MOD_UNKNOWN);
					mo->momx += (P_Random ()-128) * mult;
					mo->momy += (P_Random ()-128) * mult;
				}

				if (mo->x >= m_TremorBox[BOXLEFT] && mo->x < m_TremorBox[BOXRIGHT] &&
					 mo->y >= m_TremorBox[BOXTOP] && mo->y < m_TremorBox[BOXBOTTOM])
				{
					it->xviewshift = m_Intensity;
				}
			}
		}
	}

	if (--m_Countdown == 0)
	{
		Destroy ();
	}
}

static void setbox (fixed_t *box, AActor *c, fixed_t size)
{
	if (size)
	{
		box[BOXLEFT] = c->x - size + 1;
		box[BOXRIGHT] = c->x + size;
		box[BOXTOP] = c->y - size + 1;
		box[BOXBOTTOM] = c->y + size;
	} else
		box[BOXLEFT] = box[BOXRIGHT] = box[BOXTOP] = box[BOXBOTTOM] = 0;
}

DEarthquake::DEarthquake (AActor *center, int intensity, int duration,
						  int damrad, int tremrad)
{
	m_Spot = center;
	setbox (m_TremorBox, center, tremrad * FRACUNIT * 64);
	setbox (m_DamageBox, center, damrad * FRACUNIT * 64);
	m_Intensity = intensity;
	m_Countdown = duration;
}

BOOL P_StartQuake (int tid, int intensity, int duration, int damrad, int tremrad)
{
	AActor *center = NULL;
	bool res = false;

	if (intensity > 9)
		intensity = 9;
	else if (intensity < 1)
		intensity = 1;

	while ( (center = AActor::FindByTID (center, tid)) )
	{
		res = true;
		new DEarthquake (center, intensity, duration, damrad, tremrad);
	}
	
	return res;
}
