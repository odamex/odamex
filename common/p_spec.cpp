
// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 2006-2015 by The Odamex Team.
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
//	*Implements special effects:
//	*Texture animation, height or lighting changes
//	*according to adjacent sectors, respective
//	*utility functions, etc.
//	*Line Tag handling. Line and Sector triggers.
//	*Implements donut linedef triggers
//	*Initializes and implements BOOM linedef triggers for
//	*Scrollers/Conveyors
//	*Friction
//	*Wind/Current
//
//-----------------------------------------------------------------------------


#include "m_alloc.h"
#include "doomdef.h"
#include "doomstat.h"
#include "gstrings.h"

#include "i_system.h"
#include "z_zone.h"
#include "m_argv.h"
#include "m_random.h"
#include "m_bbox.h"
#include "w_wad.h"

#include "r_local.h"
#include "p_local.h"
#include "p_lnspec.h"
#include "p_spec.h"
#include "p_acs.h"

#include "g_game.h"
#include "p_unlag.h"

#include "s_sound.h"
#include "sc_man.h"

// State.
#include "r_state.h"

#include "c_console.h"

// [RH] Needed for sky scrolling
#include "r_sky.h"

EXTERN_CVAR(sv_allowexit)
EXTERN_CVAR(sv_fragexitswitch)

std::list<movingsector_t> movingsectors;

//
// P_FindMovingSector
//
std::list<movingsector_t>::iterator P_FindMovingSector(sector_t *sector)
{
	std::list<movingsector_t>::iterator itr;
	for (itr = movingsectors.begin(); itr != movingsectors.end(); ++itr)
		if (sector == itr->sector)
			return itr;

	// not found
	return movingsectors.end();
}

//
// P_AddMovingCeiling
//
// Updates the movingsectors list to include the passed sector, which
// tracks which sectors currently have a moving ceiling/floor
//
void P_AddMovingCeiling(sector_t *sector)
{
	if (!sector)
		return;

	movingsector_t *movesec;

	// Check if this already exists
	std::list<movingsector_t>::iterator itr = P_FindMovingSector(sector);
	if (itr != movingsectors.end())
	{
		// this sector already is moving
		movesec = &(*itr);
	}
	else
	{
		movingsectors.push_back(movingsector_t());
		movesec = &(movingsectors.back());
	}

	movesec->sector = sector;
	movesec->moving_ceiling = true;

	sector->moveable = true;
	// [SL] 2012-05-04 - Register this sector as a moveable sector with the
	// reconciliation system for unlagging
	Unlag::getInstance().registerSector(sector);
}

//
// P_AddMovingFloor
//
// Updates the movingsectors list to include the passed sector, which
// tracks which sectors currently have a moving ceiling/floor
//
void P_AddMovingFloor(sector_t *sector)
{
	if (!sector)
		return;

	movingsector_t *movesec;

	// Check if this already exists
	std::list<movingsector_t>::iterator itr = P_FindMovingSector(sector);
	if (itr != movingsectors.end())
	{
		// this sector already is moving
		movesec = &(*itr);
	}
	else
	{
		movingsectors.push_back(movingsector_t());
		movesec = &(movingsectors.back());
	}

	movesec->sector = sector;
	movesec->moving_floor = true;

	sector->moveable = true;
	// [SL] 2012-05-04 - Register this sector as a moveable sector with the
	// reconciliation system for unlagging
	Unlag::getInstance().registerSector(sector);
}

//
// P_RemoveMovingCeiling
//
// Removes the passed sector from the movingsectors list, which tracks
// which sectors currently have a moving ceiling/floor
//
void P_RemoveMovingCeiling(sector_t *sector)
{
	if (!sector)
		return;

	std::list<movingsector_t>::iterator itr = P_FindMovingSector(sector);
	if (itr != movingsectors.end())
	{
		itr->moving_ceiling = false;

		// Does this sector have a moving floor as well?  If so, just
		// mark the ceiling as invalid but don't remove from the list
		if (!itr->moving_floor)
			movingsectors.erase(itr);

		return;
	}
}

//
// P_RemoveMovingFloor
//
// Removes the passed sector from the movingsectors list, which tracks
// which sectors currently have a moving ceiling/floor
//
void P_RemoveMovingFloor(sector_t *sector)
{
	if (!sector)
		return;

	std::list<movingsector_t>::iterator itr = P_FindMovingSector(sector);
	if (itr != movingsectors.end())
	{
		itr->moving_floor = false;

		// Does this sector have a moving ceiling as well?  If so, just
		// mark the floor as invalid but don't remove from the list
		if (!itr->moving_ceiling)
			movingsectors.erase(itr);

		return;
	}
}

bool P_MovingCeilingCompleted(sector_t *sector)
{
	if (!sector || !sector->ceilingdata)
		return true;

	if (sector->ceilingdata->IsA(RUNTIME_CLASS(DDoor)))
	{
		DDoor *door = static_cast<DDoor *>(sector->ceilingdata);
		return (door->m_Status == DDoor::destroy);
	}
	if (sector->ceilingdata->IsA(RUNTIME_CLASS(DCeiling)))
	{
		DCeiling *ceiling = static_cast<DCeiling *>(sector->ceilingdata);
		return (ceiling->m_Status == DCeiling::destroy);
	}
	if (sector->ceilingdata->IsA(RUNTIME_CLASS(DPillar)))
	{
		DPillar *pillar = static_cast<DPillar *>(sector->ceilingdata);
		return (pillar->m_Status == DPillar::destroy);
	}
	if (sector->ceilingdata->IsA(RUNTIME_CLASS(DElevator)))
	{
		DElevator *elevator = static_cast<DElevator *>(sector->ceilingdata);
		return (elevator->m_Status == DElevator::destroy);
	}

	return false;
}

bool P_MovingFloorCompleted(sector_t *sector)
{
	if (!sector || !sector->floordata)
		return true;

	if (sector->floordata->IsA(RUNTIME_CLASS(DPlat)))
	{
		DPlat *plat = static_cast<DPlat *>(sector->floordata);
		return (plat->m_Status == DPlat::destroy);
	}
	if (sector->floordata->IsA(RUNTIME_CLASS(DFloor)))
	{
		DFloor *floor = static_cast<DFloor *>(sector->floordata);
		return (floor->m_Status == DFloor::destroy);
	}

	return false;
}


EXTERN_CVAR (sv_allowexit)
extern bool	HasBehavior;

IMPLEMENT_SERIAL (DScroller, DThinker)
IMPLEMENT_SERIAL (DPusher, DThinker)

DScroller::DScroller ()
{
}

void DScroller::Serialize (FArchive &arc)
{
	Super::Serialize (arc);
	if (arc.IsStoring ())
	{
		arc << m_Type
			<< m_dx << m_dy
			<< m_Affectee
			<< m_Control
			<< m_LastHeight
			<< m_vdx << m_vdy
			<< m_Accel;
	}
	else
	{
		arc >> m_Type
			>> m_dx >> m_dy
			>> m_Affectee
			>> m_Control
			>> m_LastHeight
			>> m_vdx >> m_vdy
			>> m_Accel;
	}
}

DPusher::DPusher () : m_Type(p_push), m_Xmag(0), m_Ymag(0), m_Magnitude(0),
    m_Radius(0), m_X(0), m_Y(0), m_Affectee(0)
{
}

void DPusher::Serialize (FArchive &arc)
{
	Super::Serialize (arc);
	if (arc.IsStoring ())
	{
		arc << m_Type
			<< m_Source
			<< m_Xmag
			<< m_Ymag
			<< m_Magnitude
			<< m_Radius
			<< m_X
			<< m_Y
			<< m_Affectee;
	}
	else
	{
		arc >> m_Type
			>> m_Source->netid
			>> m_Xmag
			>> m_Ymag
			>> m_Magnitude
			>> m_Radius
			>> m_X
			>> m_Y
			>> m_Affectee;
	}
}

//
// Animating textures and planes
//
// [RH] Expanded to work with a Hexen ANIMDEFS lump
//
#define MAX_ANIM_FRAMES	32

typedef struct
{
	short 	basepic;
	short	numframes;
	byte 	istexture;
	byte	uniqueframes;
	byte	countdown;
	byte	curframe;
	byte 	speedmin[MAX_ANIM_FRAMES];
	byte	speedmax[MAX_ANIM_FRAMES];
	short	framepic[MAX_ANIM_FRAMES];
} anim_t;



#define MAXANIMS	32		// Really just a starting point

static anim_t*  lastanim;
static anim_t*  anims;
static size_t	maxanims;


// Factor to scale scrolling effect into mobj-carrying properties = 3/32.
// (This is so scrolling floors and objects on them can move at same speed.)
#define CARRYFACTOR ((fixed_t)(FRACUNIT*.09375))

// killough 3/7/98: Initialize generalized scrolling
static void P_SpawnScrollers(void);

static void P_SpawnFriction(void);		// phares 3/16/98
static void P_SpawnPushers(void);		// phares 3/20/98

static void ParseAnim (byte istex);

//
//		Animating line specials
//
//#define MAXLINEANIMS			64

//extern	short	numlinespecials;
//extern	line_t* linespeciallist[MAXLINEANIMS];

//
// [RH] P_InitAnimDefs
//
// This uses a Hexen ANIMDEFS lump to define the animation sequences
//
static void P_InitAnimDefs ()
{
	int lump = -1;

	while ((lump = wads.FindLump ("ANIMDEFS", lump)) != -1)
	{
		sc.OpenLumpNum (lump, "ANIMDEFS");

		while (sc.GetString ())
		{
			if (sc.Compare ("flat"))
			{
				ParseAnim (false);
			}
			else if (sc.Compare ("texture"))
			{
				ParseAnim (true);
			}
			else if (sc.Compare ("switch"))   // Don't support switchdef yet...
			{
				//P_ProcessSwitchDef ();
//				SC_ScriptError("switchdef not supported.");
			}
			else if (sc.Compare ("warp"))
			{
				sc.MustGetString ();
				if (sc.Compare ("flat"))
				{
					sc.MustGetString ();
					flatwarp[R_FlatNumForName (sc.String)] = true;
				}
				else if (sc.Compare ("texture"))
				{
					// TODO: Make texture warping work with wall textures
					sc.MustGetString ();
					R_TextureNumForName (sc.String);
				}
				else
				{
					sc.ScriptError (NULL, NULL);
				}
			}
		}
		sc.Close ();
	}
}

static void ParseAnim (byte istex)
{
	anim_t sink;
	short picnum;
	anim_t *place;
	byte min, max;
	int frame;

	sc.MustGetString ();
	picnum = istex ? R_CheckTextureNumForName (sc.String)
		: wads.CheckNumForName (sc.String, ns_flats) - firstflat;

	if (picnum == -1)
	{ // Base pic is not present, so skip this definition
		place = &sink;
	}
	else
	{
		for (place = anims; place < lastanim; place++)
		{
			if (place->basepic == picnum && place->istexture == istex)
			{
				break;
			}
		}
		if (place == lastanim)
		{
			lastanim++;
			if (lastanim > anims + maxanims)
			{
				size_t newmax = maxanims ? maxanims*2 : MAXANIMS;
				anims = (anim_t *)Realloc (anims, newmax*sizeof(*anims));
				place = anims + maxanims;
				lastanim = place + 1;
				maxanims = newmax;
			}
		}
		// no decals on animating textures by default
		//if (istex)
		//{
		//	texturenodecals[picnum] = 1;
		//}
	}

	place->uniqueframes = true;
	place->curframe = 0;
	place->numframes = 0;
	place->basepic = picnum;
	place->istexture = istex;
	memset (place->speedmin, 1, MAX_ANIM_FRAMES * sizeof(*place->speedmin));
	memset (place->speedmax, 1, MAX_ANIM_FRAMES * sizeof(*place->speedmax));

	while (sc.GetString ())
	{
		/*if (SC_Compare ("allowdecals"))
		{
			if (istex && picnum >= 0)
			{
				texturenodecals[picnum] = 0;
			}
			continue;
		}
		else*/ if (!sc.Compare ("pic"))
		{
			sc.UnGet ();
			break;
		}

		if (place->numframes == MAX_ANIM_FRAMES)
		{
			sc.ScriptError ("Animation has too many frames");
		}

		min = max = 1;	// Shut up, GCC

		sc.MustGetNumber ();
		frame = sc.Number;
		sc.MustGetString ();
		if (sc.Compare ("tics"))
		{
			sc.MustGetNumber ();
			if (sc.Number < 0)
				sc.Number = 0;
			else if (sc.Number > 255)
				sc.Number = 255;
			min = max = sc.Number;
		}
		else if (sc.Compare ("rand"))
		{
			sc.MustGetNumber ();
			min = sc.Number >= 0 ? sc.Number : 0;
			sc.MustGetNumber ();
			max = sc.Number <= 255 ? sc.Number : 255;
		}
		else
		{
			sc.ScriptError ("Must specify a duration for animation frame");
		}

		place->speedmin[place->numframes] = min;
		place->speedmax[place->numframes] = max;
		place->framepic[place->numframes] = frame + picnum - 1;
		place->numframes++;
	}

	if (place->numframes < 2)
	{
		sc.ScriptError ("Animation needs at least 2 frames");
	}

	place->countdown = place->speedmin[0];
}

/*
 *P_InitPicAnims
 *
 *Load the table of animation definitions, checking for existence of
 *the start and end of each frame. If the start doesn't exist the sequence
 *is skipped, if the last doesn't exist, BOOM exits.
 *
 *Wall/Flat animation sequences, defined by name of first and last frame,
 *The full animation sequence is given using all lumps between the start
 *and end entry, in the order found in the WAD file.
 *
 *This routine modified to read its data from a predefined lump or
 *PWAD lump called ANIMATED rather than a static table in this module to
 *allow wad designers to insert or modify animation sequences.
 *
 *Lump format is an array of byte packed animdef_t structures, terminated
 *by a structure with istexture == -1. The lump can be generated from a
 *text source file using SWANTBLS.EXE, distributed with the BOOM utils.
 *The standard list of switches and animations is contained in the example
 *source text file DEFSWANI.DAT also in the BOOM util distribution.
 *
 *[RH] Rewritten to support BOOM ANIMATED lump but also make absolutely
 *no assumptions about how the compiler packs the animdefs array.
 *
 */
void P_InitPicAnims (void)
{
	byte *animdefs, *anim_p;

	// denis - allow reinitialisation
	if(anims)
	{
		M_Free(anims);
		lastanim = 0;
		maxanims = 0;
	}

	// [RH] Load an ANIMDEFS lump first
	P_InitAnimDefs ();

	if (wads.CheckNumForName ("ANIMATED") == -1)
		return;

	animdefs = (byte *)wads.CacheLumpName ("ANIMATED", PU_STATIC);

	// Init animation

		for (anim_p = animdefs; *anim_p != 255; anim_p += 23)
		{
			// 1/11/98 killough -- removed limit by array-doubling
			if (lastanim >= anims + maxanims)
			{
				size_t newmax = maxanims ? maxanims*2 : MAXANIMS;
				anims = (anim_t *)Realloc(anims, newmax*sizeof(*anims));   // killough
				lastanim = anims + maxanims;
				maxanims = newmax;
			}

			if (*anim_p /* .istexture */ & 1)
			{
				// different episode ?
				if (R_CheckTextureNumForName (anim_p + 10 /* .startname */) == -1 ||
					R_CheckTextureNumForName (anim_p + 1 /* .endname */) == -1)
					continue;

				lastanim->basepic = R_TextureNumForName (anim_p + 10 /* .startname */);
				lastanim->numframes = R_TextureNumForName (anim_p + 1 /* .endname */)
									  - lastanim->basepic + 1;
				/*if (*anim_p & 2)
				{ // [RH] Bit 1 set means allow decals on walls with this texture
					texturenodecals[lastanim->basepic] = 0;
				}
				else
				{
					texturenodecals[lastanim->basepic] = 1;
				}*/
			}
			else
			{
				if (wads.CheckNumForName ((char *)anim_p + 10 /* .startname */, ns_flats) == -1 ||
					wads.CheckNumForName ((char *)anim_p + 1 /* .startname */, ns_flats) == -1)
					continue;

				lastanim->basepic = R_FlatNumForName (anim_p + 10 /* .startname */);
				lastanim->numframes = R_FlatNumForName (anim_p + 1 /* .endname */)
									  - lastanim->basepic + 1;
			}

			lastanim->istexture = *anim_p /* .istexture */;
			lastanim->uniqueframes = false;
			lastanim->curframe = 0;

			if (lastanim->numframes < 2)
				Printf (PRINT_HIGH,"P_InitPicAnims: bad cycle from %s to %s",
						 anim_p + 10 /* .startname */,
						 anim_p + 1 /* .endname */);

			lastanim->speedmin[0] = lastanim->speedmax[0] = lastanim->countdown =
						/* .speed */
						(anim_p[19] << 0) |
						(anim_p[20] << 8) |
						(anim_p[21] << 16) |
						(anim_p[22] << 24);

			lastanim->countdown--;

			lastanim++;
		}
	Z_Free (animdefs);
}


//
// UTILITIES
//



// [RH]
// P_NextSpecialSector()
//
// Returns the next special sector attached to this sector
// with a certain special.
sector_t *P_NextSpecialSector (sector_t *sec, int type, sector_t *nogood)
{
	sector_t *tsec;
	int i;

	for (i = 0; i < sec->linecount; i++)
	{
		line_t *ln = sec->lines[i];

		if (!(ln->flags & ML_TWOSIDED) ||
			!(tsec = ln->frontsector))
			continue;

		if (sec == tsec)
		{
			tsec = ln->backsector;
			if (sec == tsec)
				continue;
		}

		if (tsec == nogood)
			continue;

		if ((tsec->special & 0x00ff) == type)
		{
			return tsec;
		}
	}
	return NULL;
}

//
// P_FindLowestFloorSurrounding()
// FIND LOWEST FLOOR HEIGHT IN SURROUNDING SECTORS
//
fixed_t P_FindLowestFloorSurrounding (sector_t* sec)
{
	int i;
	line_t *check;
	sector_t *other;
	fixed_t height = P_FloorHeight(sec);

	for (i = 0; i < sec->linecount; i++)
	{
		check = sec->lines[i];
		other = getNextSector (check,sec);

		if (!other)
			continue;

		fixed_t v1height =
			P_FloorHeight(sec->lines[i]->v1->x, sec->lines[i]->v1->y, other);
		fixed_t v2height =
			P_FloorHeight(sec->lines[i]->v1->x, sec->lines[i]->v1->y, other);

		if (v1height < height)
			height = v1height;
		if (v2height < height)
			height = v2height;
	}
	return height;
}



//
// P_FindHighestFloorSurrounding()
// FIND HIGHEST FLOOR HEIGHT IN SURROUNDING SECTORS
//
fixed_t P_FindHighestFloorSurrounding (sector_t *sec)
{
	int i;
	line_t *check;
	sector_t *other;
	fixed_t height = MININT;

	for (i = 0; i < sec->linecount; i++)
	{
		check = sec->lines[i];
		other = getNextSector(check,sec);

		if (!other)
			continue;

		fixed_t v1height =
			P_FloorHeight(sec->lines[i]->v1->x, sec->lines[i]->v1->y, other);
		fixed_t v2height =
			P_FloorHeight(sec->lines[i]->v1->x, sec->lines[i]->v1->y, other);

		if (v1height > height)
			height = v1height;
		if (v2height > height)
			height = v2height;
	}
	return height;
}

//
// P_FindNextHighestFloor()
//
// Passed a sector and a floor height, returns the fixed point value
// of the smallest floor height in a surrounding sector larger than
// the floor height passed. If no such height exists the floorheight
// passed is returned.
//
// [SL] Changed to use ZDoom 1.23's version of this function to account
// for sloped sectors.
//
fixed_t P_FindNextHighestFloor (sector_t *sec)
{
	sector_t *other;
	fixed_t ogheight = P_FloorHeight(sec);
	fixed_t height = MAXINT;

    for (int i = 0; i < sec->linecount; i++)
    {
		if (NULL != (other = getNextSector(sec->lines[i], sec)))
        {
        	vertex_t *v = sec->lines[i]->v1;
			fixed_t ofloor = P_FloorHeight(v->x, v->y, other);

			if (ofloor < height && ofloor > ogheight)
				height = ofloor;

			v = sec->lines[i]->v2;
			ofloor = P_FloorHeight(v->x, v->y, other);

			if (ofloor < height && ofloor > ogheight)
                height = ofloor;
        }
    }

    if (height == MAXINT)
    	height = ogheight;

    return height;
}


//
// P_FindNextLowestFloor()
//
// Passed a sector and a floor height, returns the fixed point value
// of the largest floor height in a surrounding sector smaller than
// the floor height passed. If no such height exists the floorheight
// passed is returned.
//
// jff 02/03/98 Twiddled Lee's P_FindNextHighestFloor to make this
// [SL] Changed to use ZDoom 1.23's version of this function to account
// for sloped sectors.
//
fixed_t P_FindNextLowestFloor(sector_t *sec)
{
	sector_t *other;
	fixed_t ogheight = P_FloorHeight(sec);
	fixed_t height = MININT;

    for (int i = 0; i < sec->linecount; i++)
    {
		if (NULL != (other = getNextSector(sec->lines[i], sec)))
        {
        	vertex_t *v = sec->lines[i]->v1;
			fixed_t ofloor = P_FloorHeight(v->x, v->y, other);

			if (ofloor > height && ofloor < ogheight)
				height = ofloor;

			v = sec->lines[i]->v2;
			ofloor = P_FloorHeight(v->x, v->y, other);

			if (ofloor > height && ofloor < ogheight)
                height = ofloor;
        }
    }

    if (height == MININT)
    	height = ogheight;

    return height;
}

//
// P_FindNextLowestCeiling()
//
// Passed a sector and a ceiling height, returns the fixed point value
// of the largest ceiling height in a surrounding sector smaller than
// the ceiling height passed. If no such height exists the ceiling height
// passed is returned.
//
// jff 02/03/98 Twiddled Lee's P_FindNextHighestFloor to make this
// [SL] Changed to use ZDoom 1.23's version of this function to account
// for sloped sectors.
//
fixed_t P_FindNextLowestCeiling (sector_t *sec)
{
	sector_t *other;
	fixed_t ogheight = P_CeilingHeight(sec);
	fixed_t height = MININT;

    for (int i = 0; i < sec->linecount; i++)
    {
		if (NULL != (other = getNextSector(sec->lines[i], sec)))
        {
        	vertex_t *v = sec->lines[i]->v1;
			fixed_t oceiling = P_CeilingHeight(v->x, v->y, other);

			if (oceiling > height && oceiling < ogheight)
				height = oceiling;

			v = sec->lines[i]->v2;
			oceiling = P_CeilingHeight(v->x, v->y, other);

			if (oceiling > height && oceiling < ogheight)
                height = oceiling;
        }
    }

    if (height == MININT)
    	height = ogheight;

    return height;
}


//
// P_FindNextHighestCeiling()
//
// Passed a sector and a ceiling height, returns the fixed point value
// of the smallest ceiling height in a surrounding sector larger than
// the ceiling height passed. If no such height exists the ceiling height
// passed is returned.
//
// jff 02/03/98 Twiddled Lee's P_FindNextHighestFloor to make this
// [SL] Changed to use ZDoom 1.23's version of this function to account
// for sloped sectors.
//
fixed_t P_FindNextHighestCeiling (sector_t *sec)
{
	sector_t *other;
	fixed_t ogheight = P_CeilingHeight(sec);
	fixed_t height = MAXINT;

    for (int i = 0; i < sec->linecount; i++)
    {
		if (NULL != (other = getNextSector(sec->lines[i], sec)))
        {
        	vertex_t *v = sec->lines[i]->v1;
			fixed_t oceiling = P_CeilingHeight(v->x, v->y, other);

			if (oceiling < height && oceiling > ogheight)
				height = oceiling;

			v = sec->lines[i]->v2;
			oceiling = P_CeilingHeight(v->x, v->y, other);

			if (oceiling < height && oceiling > ogheight)
                height = oceiling;
        }
    }

    if (height == MAXINT)
    	height = ogheight;

    return height;
}

//
// FIND LOWEST CEILING IN THE SURROUNDING SECTORS
//
fixed_t P_FindLowestCeilingSurrounding (sector_t *sec)
{
	int i;
	line_t *check;
	sector_t *other;
	fixed_t height = MAXINT;

	for (i = 0; i < sec->linecount; i++)
	{
		check = sec->lines[i];
		other = getNextSector(check,sec);

		if (!other)
			continue;

		fixed_t v1height =
			P_CeilingHeight(sec->lines[i]->v1->x, sec->lines[i]->v1->y, other);
		fixed_t v2height =
			P_CeilingHeight(sec->lines[i]->v1->x, sec->lines[i]->v1->y, other);

		if (v1height < height)
			height = v1height;
		if (v2height < height)
			height = v2height;
	}
	return height;
}


//
// FIND HIGHEST CEILING IN THE SURROUNDING SECTORS
//
fixed_t P_FindHighestCeilingSurrounding (sector_t *sec)
{
	int i;
	line_t *check;
	sector_t *other;
	fixed_t height = MININT;

	for (i = 0; i < sec->linecount; i++)
	{
		check = sec->lines[i];
		other = getNextSector (check,sec);

		if (!other)
			continue;

		fixed_t v1height =
			P_CeilingHeight(sec->lines[i]->v1->x, sec->lines[i]->v1->y, other);
		fixed_t v2height =
			P_CeilingHeight(sec->lines[i]->v1->x, sec->lines[i]->v1->y, other);

		if (v1height > height)
			height = v1height;
		if (v2height > height)
			height = v2height;
	}
	return height;
}


//
// P_FindShortestTextureAround()
//
// Passed a sector number, returns the shortest lower texture on a
// linedef bounding the sector.
//
// jff 02/03/98 Add routine to find shortest lower texture
//
fixed_t P_FindShortestTextureAround (int secnum)
{
	int minsize = MAXINT;
	side_t *side;
	int i;
	sector_t *sec = &sectors[secnum];

	for (i = 0; i < sec->linecount; i++)
	{
		if (twoSided (secnum, i))
		{
			side = getSide (secnum, i, 0);
			if (side->bottomtexture >= 0)
				if (textureheight[side->bottomtexture] < minsize)
					minsize = textureheight[side->bottomtexture];
			side = getSide (secnum, i, 1);
			if (side->bottomtexture >= 0)
				if (textureheight[side->bottomtexture] < minsize)
					minsize = textureheight[side->bottomtexture];
		}
	}
	return minsize;
}


//
// P_FindShortestUpperAround()
//
// Passed a sector number, returns the shortest upper texture on a
// linedef bounding the sector.
//
// Note: If no upper texture exists 32000*FRACUNIT is returned.
//       but if compatibility then MAXINT is returned
//
// jff 03/20/98 Add routine to find shortest upper texture
//
fixed_t P_FindShortestUpperAround (int secnum)
{
	int minsize = MAXINT;
	side_t *side;
	int i;
	sector_t *sec = &sectors[secnum];

	for (i = 0; i < sec->linecount; i++)
	{
		if (twoSided (secnum, i))
		{
			side = getSide (secnum,i,0);
			if (side->toptexture >= 0)
				if (textureheight[side->toptexture] < minsize)
					minsize = textureheight[side->toptexture];
			side = getSide (secnum,i,1);
			if (side->toptexture >= 0)
				if (textureheight[side->toptexture] < minsize)
					minsize = textureheight[side->toptexture];
		}
	}
	return minsize;
}


//
// P_FindModelFloorSector()
//
// Passed a floor height and a sector number, return a pointer to a
// a sector with that floor height across the lowest numbered two sided
// line surrounding the sector.
//
// Note: If no sector at that height bounds the sector passed, return NULL
//
// jff 02/03/98 Add routine to find numeric model floor
//  around a sector specified by sector number
// jff 3/14/98 change first parameter to plain height to allow call
//  from routine not using floormove_t
// [SL] Changed to use ZDoom 1.23's version of this function to account
// for sloped sectors.
//
sector_t *P_FindModelFloorSector (fixed_t floordestheight, int secnum)
{
	sector_t *other, *sec = &sectors[secnum];

    //jff 5/23/98 don't disturb sec->linecount while searching
    // but allow early exit in old demos
    for (int i = 0; i < sec->linecount; i++)
    {
        other = getNextSector(sec->lines[i], sec);
        if (other != NULL &&
        	(P_FloorHeight(sec->lines[i]->v1->x, sec->lines[i]->v1->y, other) == floordestheight ||
        	 P_FloorHeight(sec->lines[i]->v2->x, sec->lines[i]->v2->y, other) == floordestheight))
        {
            return other;
        }
    }
    return NULL;
}


//
// P_FindModelCeilingSector()
//
// Passed a ceiling height and a sector number, return a pointer to a
// a sector with that ceiling height across the lowest numbered two sided
// line surrounding the sector.
//
// Note: If no sector at that height bounds the sector passed, return NULL
//
// jff 02/03/98 Add routine to find numeric model ceiling
//  around a sector specified by sector number
//  used only from generalized ceiling types
// jff 3/14/98 change first parameter to plain height to allow call
//  from routine not using ceiling_t
// [SL] Changed to use ZDoom 1.23's version of this function to account
// for sloped sectors.
//
sector_t *P_FindModelCeilingSector (fixed_t ceildestheight, int secnum)
{
	sector_t *other, *sec = &sectors[secnum];

    //jff 5/23/98 don't disturb sec->linecount while searching
    // but allow early exit in old demos
    for (int i = 0; i < sec->linecount; i++)
    {
        other = getNextSector(sec->lines[i], sec);
        if (other != NULL &&
        	(P_CeilingHeight(sec->lines[i]->v1->x, sec->lines[i]->v1->y, other) == ceildestheight ||
        	 P_CeilingHeight(sec->lines[i]->v2->x, sec->lines[i]->v2->y, other) == ceildestheight))
        {
            return other;
        }
    }
    return NULL;
}


//
// RETURN NEXT SECTOR # THAT LINE TAG REFERS TO
//

// Find the next sector with a specified tag.
// Rewritten by Lee Killough to use chained hashing to improve speed

int P_FindSectorFromTag (int tag, int start)
{
	start = start >= 0 ? sectors[start].nexttag :
		sectors[(unsigned) tag % (unsigned) numsectors].firsttag;
	while (start >= 0 && sectors[start].tag != tag)
		start = sectors[start].nexttag;
	return start;
}

// killough 4/16/98: Same thing, only for linedefs

int P_FindLineFromID (int id, int start)
{
	start = start >= 0 ? lines[start].nextid :
		lines[(unsigned) id % (unsigned) numlines].firstid;
	while (start >= 0 && lines[start].id != id)
		start = lines[start].nextid;
	return start;
}

// Hash the sector tags across the sectors and linedefs.
static void P_InitTagLists (void)
{
	register int i;

	for (i=numsectors; --i>=0; )		// Initially make all slots empty.
		sectors[i].firsttag = -1;
	for (i=numsectors; --i>=0; )		// Proceed from last to first sector
	{									// so that lower sectors appear first
		int j = (unsigned) sectors[i].tag % (unsigned) numsectors;	// Hash func
		sectors[i].nexttag = sectors[j].firsttag;	// Prepend sector to chain
		sectors[j].firsttag = i;
	}

	// killough 4/17/98: same thing, only for linedefs

	for (i=numlines; --i>=0; )			// Initially make all slots empty.
		lines[i].firstid = -1;
	for (i=numlines; --i>=0; )        // Proceed from last to first linedef
	{									// so that lower linedefs appear first
		int j = (unsigned) lines[i].id % (unsigned) numlines;	// Hash func
		lines[i].nextid = lines[j].firstid;	// Prepend linedef to chain
		lines[j].firstid = i;
	}
}


//
// Find minimum light from an adjacent sector
//
int P_FindMinSurroundingLight (sector_t *sector, int max)
{
	int 		i;
	int 		min;
	line_t* 	line;
	sector_t*	check;

	min = max;
	for (i=0 ; i < sector->linecount ; i++)
	{
		line = sector->lines[i];
		check = getNextSector(line,sector);

		if (!check)
			continue;

		if (check->lightlevel < min)
			min = check->lightlevel;
	}
	return min;
}

// [RH] P_CheckKeys
//
//	Returns true if the player has the desired key,
//	false otherwise.

BOOL P_CheckKeys (player_t *p, card_t lock, BOOL remote)
{
	if ((lock & 0x7f) == NoKey)
		return true;

	if (!p)
		return false;

	int msg = 0;
	BOOL bc, rc, yc, bs, rs, ys;
	BOOL equiv = lock & 0x80;

        lock = (card_t)(lock & 0x7f);

	bc = p->cards[it_bluecard];
	rc = p->cards[it_redcard];
	yc = p->cards[it_yellowcard];
	bs = p->cards[it_blueskull];
	rs = p->cards[it_redskull];
	ys = p->cards[it_yellowskull];

	if (equiv) {
		bc = bs = (bc || bs);
		rc = rs = (rc || rs);
		yc = ys = (yc || ys);
	}

	switch (lock) {
		default:		// Unknown key; assume we have it
			return true;

		case AnyKey:
			if (bc || bs || rc || rs || yc || ys)
				return true;
			msg = PD_ANY;
			break;

		case AllKeys:
			if (bc && bs && rc && rs && yc && ys)
				return true;
			msg = equiv ? PD_ALL3 : PD_ALL6;
			break;

		case RCard:
			if (rc)
				return true;
			msg = equiv ? (remote ? PD_REDO : PD_REDK) : PD_REDC;
			break;

		case BCard:
			if (bc)
				return true;
			msg = equiv ? (remote ? PD_BLUEO : PD_BLUEK) : PD_BLUEC;
			break;

		case YCard:
			if (yc)
				return true;
			msg = equiv ? (remote ? PD_YELLOWO : PD_YELLOWK) : PD_YELLOWC;
			break;

		case RSkull:
			if (rs)
				return true;
			msg = equiv ? (remote ? PD_REDO : PD_REDK) : PD_REDS;
			break;

		case BSkull:
			if (bs)
				return true;
			msg = equiv ? (remote ? PD_BLUEO : PD_BLUEK) : PD_BLUES;
			break;

		case YSkull:
			if (ys)
				return true;
			msg = equiv ? (remote ? PD_YELLOWO : PD_YELLOWK) : PD_YELLOWS;
			break;
	}

	// If we get here, we don't have the right key,
	// so print an appropriate message and grunt.
	if (p->mo == consoleplayer().camera)
	{
		int keytrysound = S_FindSound ("misc/keytry");
		if (keytrysound > -1)
			UV_SoundAvoidPlayer (p->mo, CHAN_VOICE, "misc/keytry", ATTN_NORM);
		else
			UV_SoundAvoidPlayer (p->mo, CHAN_VOICE, "player/male/grunt1", ATTN_NORM);
		C_MidPrint (GStrings(msg), p);
	}

	return false;
}

void OnChangedSwitchTexture (line_t *line, int useAgain);
void OnActivatedLine (line_t *line, AActor *mo, int side, int activationType);

//
// EVENTS
// Events are operations triggered by using, crossing,
// or shooting special lines, or by timed thinkers.
//

//
// P_HandleSpecialRepeat
//
// If a line's special function is not supposed to be repeatable,
// remove the line special function from the line. This takes
// into account special circumstances like exit specials that are
// supposed to frag the triggering player during online play.
//
void P_HandleSpecialRepeat(line_t* line)
{
	// [SL] Don't remove specials from fragging exit line specials
	if ((line->special == Exit_Normal || line->special == Exit_Secret ||
		 line->special == Teleport_EndGame || line->special == Teleport_NewMap) &&
		(!sv_allowexit && sv_fragexitswitch))
		return;

	if (!(line->flags & ML_REPEAT_SPECIAL))
		line->special = 0;
}


//
// P_CrossSpecialLine - TRIGGER
// Called every time a thing origin is about
//  to cross a line with a non 0 special.
//
void
P_CrossSpecialLine
( int		linenum,
  int		side,
  AActor*	thing,
  bool      FromServer)
{
    line_t*	line = &lines[linenum];

	if (!P_CanActivateSpecials(thing, line))
		return;

	if(thing)
	{
		//	Triggers that other things can activate
		if (!thing->player)
		{
		    if (!(GET_SPAC(line->flags) == SPAC_CROSS)
                && !(GET_SPAC(line->flags) == SPAC_MCROSS))
                return;

			// Things that should NOT trigger specials...
			switch(thing->type)
			{
				case MT_ROCKET:
				case MT_PLASMA:
				case MT_BFG:
				case MT_TROOPSHOT:
				case MT_HEADSHOT:
				case MT_BRUISERSHOT:
					return;
					break;

				default: break;
			}

            // This breaks the ability for the eyes to activate the silent teleporter lines
            // in boomedit.wad, but without it vanilla demos break.
            switch (line->special)
            {
				case Teleport:
				case Teleport_NoFog:
				case Teleport_Line:
				break;

                default:
                    if(!(line->flags & ML_MONSTERSCANACTIVATE))
                        return;
                break;
            }

		}
		else
		{
		    if (!(GET_SPAC(line->flags) == SPAC_CROSS) &&
                !(GET_SPAC(line->flags) == SPAC_CROSSTHROUGH))
                return;

			// Likewise, player should not trigger monster lines
			if(GET_SPAC(line->flags) == SPAC_MCROSS)
				return;

			// And spectators should only trigger teleporters
			if (thing->player->spectator)
			{
				switch (line->special)
				{
					case Teleport:
					case Teleport_NoFog:
					case Teleport_NewMap:
					case Teleport_EndGame:
					case Teleport_Line:
						break;
					default:
						return;
						break;
				}
			}
		}

		// Do not teleport on the wrong side
		if(side)
		{
			switch(line->special)
			{
				case Teleport:
				case Teleport_NoFog:
				case Teleport_NewMap:
				case Teleport_EndGame:
				case Teleport_Line:
					return;
					break;
				default: break;
			}
		}
	}

	TeleportSide = side;

	LineSpecials[line->special] (line, thing, line->args[0],
					line->args[1], line->args[2],
					line->args[3], line->args[4]);

	P_HandleSpecialRepeat(line);

	OnActivatedLine(line, thing, side, 0);
}

//
// P_ShootSpecialLine - IMPACT SPECIALS
// Called when a thing shoots a special line.
//
void
P_ShootSpecialLine
( AActor*	thing,
  line_t*	line,
  bool      FromServer)
{
	if (!P_CanActivateSpecials(thing, line))
		return;

	if(thing)
	{
		if (!(GET_SPAC(line->flags) == SPAC_IMPACT))
			return;

		if (thing->flags & MF_MISSILE)
			return;

		if (!thing->player && !(line->flags & ML_MONSTERSCANACTIVATE))
			return;
	}

	//TeleportSide = side;

	LineSpecials[line->special] (line, thing, line->args[0],
					line->args[1], line->args[2],
					line->args[3], line->args[4]);

	P_HandleSpecialRepeat(line);

	OnActivatedLine(line, thing, 0, 2);

	if(serverside)
	{
		P_ChangeSwitchTexture (line, line->flags & ML_REPEAT_SPECIAL, true);
		OnChangedSwitchTexture (line, line->flags & ML_REPEAT_SPECIAL);
	}
}


//
// P_UseSpecialLine
// Called when a thing uses a special line.
// Only the front sides of lines are usable.
//
bool
P_UseSpecialLine
( AActor*	thing,
  line_t*	line,
  int		side,
  bool      FromServer)
{
	if (!P_CanActivateSpecials(thing, line))
		return false;

	// Err...
	// Use the back sides of VERY SPECIAL lines...
	if (side)
	{
		switch(line->special)
		{
		case Exit_Secret:
			// Sliding door open&close
			// UNUSED?
			break;

		default:
			return false;
			break;
		}
	}

	if(thing)
	{
		if ((GET_SPAC(line->flags) != SPAC_USE) &&
			(GET_SPAC(line->flags) != SPAC_PUSH) &&
            (GET_SPAC(line->flags) != SPAC_USETHROUGH))
			return false;

		// Switches that other things can activate.
		if (!thing->player)
		{
			// not for monsters?
			if (!(line->flags & ML_MONSTERSCANACTIVATE))
				return false;

			// never open secret doors
			if (line->flags & ML_SECRET)
				return false;
		}
		else
		{
			// spectators and dead players can't use switches
			if(thing->player->spectator ||
                           thing->player->playerstate != PST_LIVE)
				return false;
		}
	}

    TeleportSide = side;

	if(LineSpecials[line->special] (line, thing, line->args[0],
					line->args[1], line->args[2],
					line->args[3], line->args[4]))
	{
		P_HandleSpecialRepeat(line);

		OnActivatedLine(line, thing, side, 1);

		if(serverside && GET_SPAC(line->flags) != SPAC_PUSH)
		{
			P_ChangeSwitchTexture (line, line->flags & ML_REPEAT_SPECIAL, true);
			OnChangedSwitchTexture (line, line->flags & ML_REPEAT_SPECIAL);
		}
	}

    return true;
}


//
// P_PushSpecialLine
// Called when a thing pushes a special line, only in advanced map format
// Only the front sides of lines are pushable.
//
bool
P_PushSpecialLine
( AActor*	thing,
  line_t*	line,
  int		side,
  bool      FromServer)
{
	if (!P_CanActivateSpecials(thing, line))
		return false;

	// Err...
	// Use the back sides of VERY SPECIAL lines...
	if (side)
		return false;

	if(thing)
	{
		if (GET_SPAC(line->flags) != SPAC_PUSH)
			return false;

		// Switches that other things can activate.
		if (!thing->player)
		{
			// not for monsters?
			if (!(line->flags & ML_MONSTERSCANACTIVATE))
				return false;

			// never open secret doors
			if (line->flags & ML_SECRET)
				return false;
		}
		else
		{
			// spectators and dead players can't push walls
			if(thing->player->spectator ||
                           thing->player->playerstate != PST_LIVE)
				return false;
		}
	}

    TeleportSide = side;

	if(LineSpecials[line->special] (line, thing, line->args[0],
					line->args[1], line->args[2],
					line->args[3], line->args[4]))
	{
		P_HandleSpecialRepeat(line);

		OnActivatedLine(line, thing, side, 3);

		if(serverside)
		{
			P_ChangeSwitchTexture (line, line->flags & ML_REPEAT_SPECIAL, true);
			OnChangedSwitchTexture (line, line->flags & ML_REPEAT_SPECIAL);
		}
	}

    return true;
}



//
// P_PlayerInSpecialSector
// Called every tic frame
//	that the player origin is in a special sector
//
void P_PlayerInSpecialSector (player_t *player)
{
	// Spectators should not be affected by special sectors
	if (player->spectator)
		return;

	sector_t *sector = player->mo->subsector->sector;
	int special = sector->special & ~SECRET_MASK;

	// Falling, not all the way down yet?
	if (player->mo->z != P_FloorHeight(player->mo) && !player->mo->waterlevel)
		return;

	// Has hitten ground.
	// [RH] Normal DOOM special or BOOM specialized?
	if (special >= dLight_Flicker && special <= 255)
	{
		switch (special)
		{
		  case Damage_InstantDeath:
			P_DamageMobj (player->mo, NULL, NULL, 999, MOD_UNKNOWN);
			break;

		  case dDamage_Hellslime:
			// HELLSLIME DAMAGE
			if (!player->powers[pw_ironfeet])
				if (!(level.time&0x1f))
					P_DamageMobj (player->mo, NULL, NULL, 10, MOD_SLIME);
			break;

		  case dDamage_Nukage:
			// NUKAGE DAMAGE
			if (!player->powers[pw_ironfeet])
				if (!(level.time&0x1f))
					P_DamageMobj (player->mo, NULL, NULL, 5, MOD_LAVA);
			break;

		  case hDamage_Sludge:
			if (!player->powers[pw_ironfeet] && !(level.time&0x1f))
				P_DamageMobj (player->mo, NULL, NULL, 4, MOD_SLIME);
			break;

		  case dDamage_SuperHellslime:
			// SUPER HELLSLIME DAMAGE
		  case dLight_Strobe_Hurt:
			// STROBE HURT
			if (!player->powers[pw_ironfeet]
				|| (P_Random ()<5) )
			{
				if (!(level.time&0x1f))
					P_DamageMobj (player->mo, NULL, NULL, 20, MOD_SLIME);
			}
			break;

		  case dDamage_End:
			// EXIT SUPER DAMAGE! (for E1M8 finale)
			player->cheats &= ~CF_GODMODE;

			if (!(level.time & 0x1f))
				P_DamageMobj (player->mo, NULL, NULL, 20, MOD_UNKNOWN);

			if(sv_gametype == GM_COOP || sv_allowexit)
			{
				if (gamestate == GS_LEVEL && player->health <= 10)
					G_ExitLevel(0, 1);
			}
			break;

		  case dDamage_LavaWimpy:
		  case dScroll_EastLavaDamage:
			if (!(level.time & 15))
				P_DamageMobj(player->mo, NULL, NULL, 5, MOD_LAVA);

			break;

		  case dDamage_LavaHefty:
			if(!(level.time & 15))
				P_DamageMobj(player->mo, NULL, NULL, 8, MOD_LAVA);

			break;

		  default:
			// [RH] Ignore unknown specials
			break;
		}
	}
	else
	{
		//jff 3/14/98 handle extended sector types for secrets and damage
		switch (special & DAMAGE_MASK) {
			case 0x000: // no damage
				break;
			case 0x100: // 2/5 damage per 31 ticks
				if (!player->powers[pw_ironfeet])
					if (!(level.time&0x1f))
						P_DamageMobj (player->mo, NULL, NULL, 5, MOD_LAVA);
				break;
			case 0x200: // 5/10 damage per 31 ticks
				if (!player->powers[pw_ironfeet])
					if (!(level.time&0x1f))
						P_DamageMobj (player->mo, NULL, NULL, 10, MOD_SLIME);
				break;
			case 0x300: // 10/20 damage per 31 ticks
				if (!player->powers[pw_ironfeet]
					|| (P_Random(player->mo)<5))	// take damage even with suit
				{
					if (!(level.time&0x1f))
						P_DamageMobj (player->mo, NULL, NULL, 20, MOD_SLIME);
				}
				break;
		}

		// [RH] Apply any customizable damage
		if (sector->damage) {
			if (sector->damage < 20) {
				if (!player->powers[pw_ironfeet] && !(level.time&0x1f))
					P_DamageMobj (player->mo, NULL, NULL, sector->damage, sector->mod);
			} else if (sector->damage < 50) {
				if ((!player->powers[pw_ironfeet] || (P_Random(player->mo)<5))
					 && !(level.time&0x1f))
					P_DamageMobj (player->mo, NULL, NULL, sector->damage, sector->mod);
			} else {
				P_DamageMobj (player->mo, NULL, NULL, sector->damage, sector->mod);
			}
		}

		if (sector->special & SECRET_MASK) {
			player->secretcount++;
			level.found_secrets++;
			sector->special &= ~SECRET_MASK;

#ifdef CLIENT_APP
			if (player->mo == consoleplayer().camera)
				C_RevealSecret();
#endif
		}
	}
}

//
// P_UpdateSpecials
// Animate planes, scroll walls, etc.
//

void P_UpdateSpecials (void)
{
	anim_t *anim;
	int i;

	// ANIMATE FLATS AND TEXTURES GLOBALLY
	// [RH] Changed significantly to work with ANIMDEFS lumps
	for (anim = anims; anim < lastanim; anim++)
	{
		if (--anim->countdown == 0)
		{
			int speedframe;

			anim->curframe = (anim->numframes) ?
					(anim->curframe + 1) % anim->numframes : 0;

			speedframe = (anim->uniqueframes) ? anim->curframe : 0;

			if (anim->speedmin[speedframe] == anim->speedmax[speedframe])
				anim->countdown = anim->speedmin[speedframe];
			else
				anim->countdown = M_Random() %
					(anim->speedmax[speedframe] - anim->speedmin[speedframe]) +
					anim->speedmin[speedframe];
		}

		if (anim->uniqueframes)
		{
			int pic = anim->framepic[anim->curframe];

			if (anim->istexture)
				for (i = 0; i < anim->numframes; i++)
					texturetranslation[anim->framepic[i]] = pic;
			else
				for (i = 0; i < anim->numframes; i++)
					flattranslation[anim->framepic[i]] = pic;
		}
		else
		{
			for (i = anim->basepic; i < anim->basepic + anim->numframes; i++)
			{
				int pic = anim->basepic + (anim->curframe + i) % anim->numframes;

				if (anim->istexture)
					texturetranslation[i] = pic;
				else
					flattranslation[i] = pic;
			}
		}
	}

	// [ML] 5/11/06 - Remove sky scrolling ability
}



//
// SPECIAL SPAWNING
//

CVAR_FUNC_IMPL (sv_forcewater)
{
	if (gamestate == GS_LEVEL)
	{
		int i;
		byte set = var ? 2 : 0;

		for (i = 0; i < numsectors; i++)
		{
			if (sectors[i].heightsec &&
				!(sectors[i].heightsec->MoreFlags & SECF_IGNOREHEIGHTSEC) &&
				sectors[i].heightsec->waterzone != 1)

				sectors[i].heightsec->waterzone = set;
		}
	}
}

//
// P_SpawnSpecials
// After the map has been loaded, scan for specials
//	that spawn thinkers
//

void P_SpawnSpecials (void)
{
	sector_t*	sector;
	int 		i;

	//	Init special SECTORs.
	sector = sectors;
	for (i = 0; i < numsectors; i++, sector++)
	{
		if (!sector->special)
			continue;

		// [RH] All secret sectors are marked with a BOOM-ish bitfield
		if (sector->special & SECRET_MASK)
			level.total_secrets++;

		switch (sector->special & 0xff)
		{
			// [RH] Normal DOOM/Hexen specials. We clear off the special for lights
			//	  here instead of inside the spawners.

		case dLight_Flicker:
			// FLICKERING LIGHTS
			new DLightFlash (sector);
			sector->special &= 0xff00;
			break;

		case dLight_StrobeFast:
			// STROBE FAST
			new DStrobe (sector, STROBEBRIGHT, FASTDARK, false);
			sector->special &= 0xff00;
			break;

		case dLight_StrobeSlow:
			// STROBE SLOW
			new DStrobe (sector, STROBEBRIGHT, SLOWDARK, false);
			sector->special &= 0xff00;
			break;

		case dLight_Strobe_Hurt:
			// STROBE FAST/DEATH SLIME
			new DStrobe (sector, STROBEBRIGHT, FASTDARK, false);
			break;

		case dLight_Glow:
			// GLOWING LIGHT
			new DGlow (sector);
			sector->special &= 0xff00;
			break;

		case dSector_DoorCloseIn30:
			// DOOR CLOSE IN 30 SECONDS
			P_SpawnDoorCloseIn30 (sector);
			break;

		case dLight_StrobeSlowSync:
			// SYNC STROBE SLOW
			new DStrobe (sector, STROBEBRIGHT, SLOWDARK, true);
			sector->special &= 0xff00;
			break;

		case dLight_StrobeFastSync:
			// SYNC STROBE FAST
			new DStrobe (sector, STROBEBRIGHT, FASTDARK, true);
			sector->special &= 0xff00;
			break;

		case dSector_DoorRaiseIn5Mins:
			// DOOR RAISE IN 5 MINUTES
			P_SpawnDoorRaiseIn5Mins (sector);
			break;

		case dLight_FireFlicker:
			// fire flickering
			new DFireFlicker (sector);
			sector->special &= 0xff00;
			break;

		  // [RH] Hexen-like phased lighting
		case LightSequenceStart:
			new DPhased (sector);
			break;

		case Light_Phased:
			new DPhased (sector, 48, 63 - (sector->lightlevel & 63));
			break;

		case Sky2:
			sector->sky = PL_SKYFLAT;
			break;

		default:
			// [RH] Try for normal Hexen scroller
			if ((sector->special & 0xff) >= Scroll_North_Slow &&
				(sector->special & 0xff) <= Scroll_SouthWest_Fast)
			{
				static signed char hexenScrollies[24][2] =
				{
					{  0,  1 }, {  0,  2 }, {  0,  4 },
					{ -1,  0 }, { -2,  0 }, { -4,  0 },
					{  0, -1 }, {  0, -2 }, {  0, -4 },
					{  1,  0 }, {  2,  0 }, {  4,  0 },
					{  1,  1 }, {  2,  2 }, {  4,  4 },
					{ -1,  1 }, { -2,  2 }, { -4,  4 },
					{ -1, -1 }, { -2, -2 }, { -4, -4 },
					{  1, -1 }, {  2, -2 }, {  4, -4 }
				};
				int i = (sector->special & 0xff) - Scroll_North_Slow;
				int dx = hexenScrollies[i][0] * (FRACUNIT/2);
				int dy = hexenScrollies[i][1] * (FRACUNIT/2);

				new DScroller (DScroller::sc_floor, dx, dy, -1, sector-sectors, 0);
				// Hexen scrolling floors cause the player to move
				// faster than they scroll. I do this just for compatibility
				// with Hexen and recommend using Killough's more-versatile
				// scrollers instead.
				dx = FixedMul (-dx, CARRYFACTOR*2);
				dy = FixedMul (dy, CARRYFACTOR*2);
				new DScroller (DScroller::sc_carry, dx, dy, -1, sector-sectors, 0);
				sector->special &= 0xff00;
			}
		break;
		}
	}

	// Init other misc stuff

	// P_InitTagLists() must be called before P_FindSectorFromTag()
	// or P_FindLineFromID() can be called.

	P_InitTagLists();   // killough 1/30/98: Create xref tables for tags
	P_SpawnScrollers(); // killough 3/7/98: Add generalized scrollers
	P_SpawnFriction();	// phares 3/12/98: New friction model using linedefs
	P_SpawnPushers();	// phares 3/20/98: New pusher model using linedefs

	for (i=0; i<numlines; i++)
		switch (lines[i].special)
		{
			int s;
			sector_t *sec;

		// killough 3/7/98:
		// support for drawn heights coming from different sector
		case Transfer_Heights:
			sec = sides[*lines[i].sidenum].sector;
			DPrintf("Sector tagged %d: TransferHeights \n",sec->tag);
			if (sv_forcewater)
			{
				sec->waterzone = 2;
			}
			if (lines[i].args[1] & 2)
			{
				sec->MoreFlags |= SECF_FAKEFLOORONLY;
			}
			if (lines[i].args[1] & 4)
			{
				sec->MoreFlags |= SECF_CLIPFAKEPLANES;
				DPrintf("Sector tagged %d: CLIPFAKEPLANES \n",sec->tag);
			}
			if (lines[i].args[1] & 8)
			{
				sec->waterzone = 1;
				DPrintf("Sector tagged %d: Sets waterzone=1 \n",sec->tag);
			}
			if (lines[i].args[1] & 16)
			{
				sec->MoreFlags |= SECF_IGNOREHEIGHTSEC;
				DPrintf("Sector tagged %d: IGNOREHEIGHTSEC \n",sec->tag);
			}
			if (lines[i].args[1] & 32)
			{
				sec->MoreFlags |= SECF_NOFAKELIGHT;
				DPrintf("Sector tagged %d: NOFAKELIGHTS \n",sec->tag);
			}
			for (s = -1; (s = P_FindSectorFromTag(lines[i].args[0],s)) >= 0;)
			{
				sectors[s].heightsec = sec;
			}

			DPrintf("Sector tagged %d: MoreFlags: %u \n",sec->tag,sec->MoreFlags);
			break;

		// killough 3/16/98: Add support for setting
		// floor lighting independently (e.g. lava)
		case Transfer_FloorLight:
			sec = sides[*lines[i].sidenum].sector;
			for (s = -1; (s = P_FindSectorFromTag(lines[i].args[0],s)) >= 0;)
				sectors[s].floorlightsec = sec;
			break;

		// killough 4/11/98: Add support for setting
		// ceiling lighting independently
		case Transfer_CeilingLight:
			sec = sides[*lines[i].sidenum].sector;
			for (s = -1; (s = P_FindSectorFromTag(lines[i].args[0],s)) >= 0;)
				sectors[s].ceilinglightsec = sec;
			break;

		// [RH] ZDoom Static_Init settings
		case Static_Init:
			switch (lines[i].args[1])
			{
			case Init_Gravity:
				{
				float grav = ((float)P_AproxDistance (lines[i].dx, lines[i].dy)) / (FRACUNIT * 100.0f);
				for (s = -1; (s = P_FindSectorFromTag(lines[i].args[0],s)) >= 0;)
					sectors[s].gravity = grav;
				}
				break;

			//case Init_Color:
			// handled in P_LoadSideDefs2()

			case Init_Damage:
				{
					int damage = P_AproxDistance (lines[i].dx, lines[i].dy) >> FRACBITS;
					for (s = -1; (s = P_FindSectorFromTag(lines[i].args[0],s)) >= 0;)
					{
						sectors[s].damage = damage;
						sectors[s].mod = MOD_UNKNOWN;
					}
				}
				break;

			// killough 10/98:
			//
			// Support for sky textures being transferred from sidedefs.
			// Allows scrolling and other effects (but if scrolling is
			// used, then the same sector tag needs to be used for the
			// sky sector, the sky-transfer linedef, and the scroll-effect
			// linedef). Still requires user to use F_SKY1 for the floor
			// or ceiling texture, to distinguish floor and ceiling sky.

			case Init_TransferSky:
				for (s = -1; (s = P_FindSectorFromTag(lines[i].args[0],s)) >= 0;)
					sectors[s].sky = (i+1) | PL_SKYFLAT;
				break;
			}
			break;
		}


	// [RH] Start running any open scripts on this map
	if (level.behavior != NULL)
	{
		level.behavior->StartTypedScripts (SCRIPT_Open, NULL);
	}
}

// killough 2/28/98:
//
// This function, with the help of r_plane.c and r_bsp.c, supports generalized
// scrolling floors and walls, with optional mobj-carrying properties, e.g.
// conveyor belts, rivers, etc. A linedef with a special type affects all
// tagged sectors the same way, by creating scrolling and/or object-carrying
// properties. Multiple linedefs may be used on the same sector and are
// cumulative, although the special case of scrolling a floor and carrying
// things on it, requires only one linedef. The linedef's direction determines
// the scrolling direction, and the linedef's length determines the scrolling
// speed. This was designed so that an edge around the sector could be used to
// control the direction of the sector's scrolling, which is usually what is
// desired.
//
// Process the active scrollers.
//
// This is the main scrolling code
// killough 3/7/98

void DScroller::RunThink ()
{
	fixed_t dx = m_dx, dy = m_dy;

	if (m_Control != -1)
	{	// compute scroll amounts based on a sector's height changes
		sector_t *sector = &sectors[m_Control];
		fixed_t centerfloor = P_FloorHeight(sector->soundorg[0], sector->soundorg[1], sector);
		fixed_t centerceiling = P_FloorHeight(sector->soundorg[0], sector->soundorg[1], sector);

		fixed_t height = centerfloor + centerceiling;
		fixed_t delta = height - m_LastHeight;
		m_LastHeight = height;
		dx = FixedMul(dx, delta);
		dy = FixedMul(dy, delta);
	}

	// killough 3/14/98: Add acceleration
	if (m_Accel)
	{
		m_vdx = (dx += m_vdx);
		m_vdy = (dy += m_vdy);
	}

	if (!(dx | dy))			// no-op if both (x,y) offsets 0
		return;

	switch (m_Type)
	{
		side_t *side;
		sector_t *sec;
		fixed_t height, waterheight;	// killough 4/4/98: add waterheight
		msecnode_t *node;
		AActor *thing;

		case sc_side:				// killough 3/7/98: Scroll wall texture
			side = sides + m_Affectee;
			side->textureoffset += dx;
			side->rowoffset += dy;
			break;

		case sc_floor:				// killough 3/7/98: Scroll floor texture
			sec = sectors + m_Affectee;
			sec->floor_xoffs += dx;
			sec->floor_yoffs += dy;
			break;

		case sc_ceiling:			// killough 3/7/98: Scroll ceiling texture
			sec = sectors + m_Affectee;
			sec->ceiling_xoffs += dx;
			sec->ceiling_yoffs += dy;
			break;

		case sc_carry:
		{
			// killough 3/7/98: Carry things on floor
			// killough 3/20/98: use new sector list which reflects true members
			// killough 3/27/98: fix carrier bug
			// killough 4/4/98: Underwater, carry things even w/o gravity
			sec = sectors + m_Affectee;
			height = P_HighestHeightOfFloor(sec);
			waterheight = sec->heightsec &&
				P_HighestHeightOfFloor(sec->heightsec) > height ?
				P_HighestHeightOfFloor(sec->heightsec) : MININT;

			for (node = sec->touching_thinglist; node; node = node->m_snext)
				if (!((thing = node->m_thing)->flags & MF_NOCLIP) &&
					(!(thing->flags & MF_NOGRAVITY || thing->z > height) ||
					 thing->z < waterheight))
				  {
					// Move objects only if on floor or underwater,
					// non-floating, and clipped.
					thing->momx += dx;
					thing->momy += dy;
				  }
			break;
		}

		case sc_carry_ceiling:       // to be added later
			break;
	}
}

//
// Add_Scroller()
//
// Add a generalized scroller to the thinker list.
//
// type: the enumerated type of scrolling: floor, ceiling, floor carrier,
//   wall, floor carrier & scroller
//
// (dx,dy): the direction and speed of the scrolling or its acceleration
//
// control: the sector whose heights control this scroller's effect
//   remotely, or -1 if no control sector
//
// affectee: the index of the affected object (sector or sidedef)
//
// accel: non-zero if this is an accelerative effect
//

DScroller::DScroller (EScrollType type, fixed_t dx, fixed_t dy,
					  int control, int affectee, int accel)
{
	m_Type = type;
	m_dx = dx;
	m_dy = dy;
        m_vdx = 0;
        m_vdy = 0;
        m_Accel = accel;
	if ((m_Control = control) != -1)
	{
		sector_t *sector = &sectors[control];
		fixed_t centerfloor =
			P_FloorHeight(sector->soundorg[0], sector->soundorg[1], sector);
		fixed_t centerceiling =
			P_CeilingHeight(sector->soundorg[0], sector->soundorg[1], sector);

		m_LastHeight = centerfloor + centerceiling;
	}
	m_Affectee = affectee;
}

// Adds wall scroller. Scroll amount is rotated with respect to wall's
// linedef first, so that scrolling towards the wall in a perpendicular
// direction is translated into vertical motion, while scrolling along
// the wall in a parallel direction is translated into horizontal motion.
//
// killough 5/25/98: cleaned up arithmetic to avoid drift due to roundoff

DScroller::DScroller (fixed_t dx, fixed_t dy, const line_t *l,
					 int control, int accel)
{
	fixed_t x = abs(l->dx), y = abs(l->dy), d;
	if (y > x)
		d = x, x = y, y = d;
	d = FixedDiv (x, finesine[(tantoangle[FixedDiv(y,x) >> DBITS] + ANG90)
						  >> ANGLETOFINESHIFT]);
	x = -FixedDiv (FixedMul(dy, l->dy) + FixedMul(dx, l->dx), d);
	y = -FixedDiv (FixedMul(dx, l->dy) - FixedMul(dy, l->dx), d);

	m_Type = sc_side;
	m_dx = x;
	m_dy = y;
	m_vdx = m_vdy = 0;
	m_Accel = accel;

	if ((m_Control = control) != -1)
	{
		sector_t *sector = &sectors[control];
		fixed_t centerfloor =
			P_FloorHeight(sector->soundorg[0], sector->soundorg[1], sector);
		fixed_t centerceiling =
			P_CeilingHeight(sector->soundorg[0], sector->soundorg[1], sector);

		m_LastHeight = centerfloor + centerceiling;
	}
	m_Affectee = *l->sidenum;
}

// Amount (dx,dy) vector linedef is shifted right to get scroll amount
#define SCROLL_SHIFT 5

// Initialize the scrollers
static void P_SpawnScrollers(void)
{
	int i;
	line_t *l = lines;

	for (i = 0; i < numlines; i++, l++)
	{
		fixed_t dx = 0;	// direction and speed of scrolling
		fixed_t dy = 0;
		int control = -1, accel = 0;		// no control sector or acceleration
		int special = l->special;

		// killough 3/7/98: Types 245-249 are same as 250-254 except that the
		// first side's sector's heights cause scrolling when they change, and
		// this linedef controls the direction and speed of the scrolling. The
		// most complicated linedef since donuts, but powerful :)
		//
		// killough 3/15/98: Add acceleration. Types 214-218 are the same but
		// are accelerative.

		if (special == Scroll_Ceiling ||
			special == Scroll_Floor ||
			special == Scroll_Texture_Model)
		{
			if (l->args[1] & 3)
			{
				// if 1, then displacement
				// if 2, then accelerative (also if 3)
				control = sides[*l->sidenum].sector - sectors;
				if (l->args[1] & 2)
					accel = 1;
			}
			if (special == Scroll_Texture_Model || l->args[1] & 4)
			{
				// The line housing the special controls the
				// direction and speed of scrolling.
				dx = l->dx >> SCROLL_SHIFT;
				dy = l->dy >> SCROLL_SHIFT;
			}
			else
			{
				// The speed and direction are parameters to the special.
				dx = (l->args[3] - 128) * (FRACUNIT / 32);
				dy = (l->args[4] - 128) * (FRACUNIT / 32);
			}
		}

		switch (special)
		{
			register int s;

			case Scroll_Ceiling:
				for (s=-1; (s = P_FindSectorFromTag (l->args[0],s)) >= 0;)
					new DScroller (DScroller::sc_ceiling, -dx, dy, control, s, accel);
				break;

			case Scroll_Floor:
				if (l->args[2] != 1)
					// scroll the floor
					for (s=-1; (s = P_FindSectorFromTag (l->args[0],s)) >= 0;)
						new DScroller (DScroller::sc_floor, -dx, dy, control, s, accel);

				if (l->args[2] > 0) {
					// carry objects on the floor
					dx = FixedMul(dx,CARRYFACTOR);
					dy = FixedMul(dy,CARRYFACTOR);
					for (s=-1; (s = P_FindSectorFromTag (l->args[0],s)) >= 0;)
						new DScroller (DScroller::sc_carry, dx, dy, control, s, accel);
				}
				break;

			// killough 3/1/98: scroll wall according to linedef
			// (same direction and speed as scrolling floors)
			case Scroll_Texture_Model:
				for (s=-1; (s = P_FindLineFromID (l->args[0],s)) >= 0;)
					if (s != i)
						new DScroller (dx, dy, lines+s, control, accel);
				break;

			case Scroll_Texture_Offsets:
				// killough 3/2/98: scroll according to sidedef offsets
				s = lines[i].sidenum[0];
				new DScroller (DScroller::sc_side, -sides[s].textureoffset,
							   sides[s].rowoffset, -1, s, accel);
				break;

			case Scroll_Texture_Left:
				new DScroller (DScroller::sc_side, l->args[0] * (FRACUNIT/64), 0,
							   -1, lines[i].sidenum[0], accel);
				break;

			case Scroll_Texture_Right:
				new DScroller (DScroller::sc_side, l->args[0] * (-FRACUNIT/64), 0,
							   -1, lines[i].sidenum[0], accel);
				break;

			case Scroll_Texture_Up:
				new DScroller (DScroller::sc_side, 0, l->args[0] * (FRACUNIT/64),
							   -1, lines[i].sidenum[0], accel);
				break;

			case Scroll_Texture_Down:
				new DScroller (DScroller::sc_side, 0, l->args[0] * (-FRACUNIT/64),
							   -1, lines[i].sidenum[0], accel);
				break;

			case Scroll_Texture_Both:
				if (l->args[0] == 0) {
					dx = (l->args[1] - l->args[2]) * (FRACUNIT/64);
					dy = (l->args[4] - l->args[3]) * (FRACUNIT/64);
					new DScroller (DScroller::sc_side, dx, dy, -1, lines[i].sidenum[0], accel);
				}
				break;

			default:
				break;
		}
	}
}

// killough 3/7/98 -- end generalized scroll effects

////////////////////////////////////////////////////////////////////////////
//
// FRICTION EFFECTS
//
// phares 3/12/98: Start of friction effects

// As the player moves, friction is applied by decreasing the x and y
// momentum values on each tic. By varying the percentage of decrease,
// we can simulate muddy or icy conditions. In mud, the player slows
// down faster. In ice, the player slows down more slowly.
//
// The amount of friction change is controlled by the length of a linedef
// with type 223. A length < 100 gives you mud. A length > 100 gives you ice.
//
// Also, each sector where these effects are to take place is given a
// new special type _______. Changing the type value at runtime allows
// these effects to be turned on or off.
//
// Sector boundaries present problems. The player should experience these
// friction changes only when his feet are touching the sector floor. At
// sector boundaries where floor height changes, the player can find
// himself still 'in' one sector, but with his feet at the floor level
// of the next sector (steps up or down). To handle this, Thinkers are used
// in icy/muddy sectors. These thinkers examine each object that is touching
// their sectors, looking for players whose feet are at the same level as
// their floors. Players satisfying this condition are given new friction
// values that are applied by the player movement code later.

//
// killough 8/28/98:
//
// Completely redid code, which did not need thinkers, and which put a heavy
// drag on CPU. Friction is now a property of sectors, NOT objects inside
// them. All objects, not just players, are affected by it, if they touch
// the sector's floor. Code simpler and faster, only calling on friction
// calculations when an object needs friction considered, instead of doing
// friction calculations on every sector during every tic.
//
// Although this -might- ruin Boom demo sync involving friction, it's the only
// way, short of code explosion, to fix the original design bug. Fixing the
// design bug in Boom's original friction code, while maintaining demo sync
// under every conceivable circumstance, would double or triple code size, and
// would require maintenance of buggy legacy code which is only useful for old
// demos. Doom demos, which are more important IMO, are not affected by this
// change.
//
// [RH] On the other hand, since I've given up on trying to maintain demo
//		sync between versions, these considerations aren't a big deal to me.
//
/////////////////////////////
//
// Initialize the sectors where friction is increased or decreased

static void P_SpawnFriction(void)
{
	int i;
	line_t *l = lines;

	for (i = 0 ; i < numlines ; i++,l++)
	{
		if (l->special == Sector_SetFriction)
		{
			int length, s;
			fixed_t friction, movefactor;

			if (l->args[1])
			{	// [RH] Allow setting friction amount from parameter
				length = l->args[1] <= 200 ? l->args[1] : 200;
			}
			else
			{
				length = P_AproxDistance(l->dx,l->dy)>>FRACBITS;
			}
			friction = (0x1EB8*length)/0x80 + 0xD000;

			// killough 8/28/98: prevent odd situations
			if (friction > FRACUNIT)
				friction = FRACUNIT;
			if (friction < 0)
				friction = 0;

			// The following check might seem odd. At the time of movement,
			// the move distance is multiplied by 'friction/0x10000', so a
			// higher friction value actually means 'less friction'.

			// [RH] Twiddled these values so that momentum on ice (with
			//		friction 0xf900) is the same as in Heretic/Hexen.
			if (friction > ORIG_FRICTION)	// ice
//				movefactor = ((0x10092 - friction)*(0x70))/0x158;
				movefactor = ((0x10092 - friction) * 1024) / 4352 + 568;
			else
				movefactor = ((friction - 0xDB34)*(0xA))/0x80;

			// killough 8/28/98: prevent odd situations
			if (movefactor < 32)
				movefactor = 32;

			for (s = -1; (s = P_FindSectorFromTag(l->args[0],s)) >= 0 ; )
			{
				// killough 8/28/98:
				//
				// Instead of spawning thinkers, which are slow and expensive,
				// modify the sector's own friction values. Friction should be
				// a property of sectors, not objects which reside inside them.
				// Original code scanned every object in every friction sector
				// on every tic, adjusting its friction, putting unnecessary
				// drag on CPU. New code adjusts friction of sector only once
				// at level startup, and then uses this friction value.

				sectors[s].friction = friction;
				sectors[s].movefactor = movefactor;
			}
		}
	}
}

//
// phares 3/12/98: End of friction effects
//
////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////
//
// PUSH/PULL EFFECT
//
// phares 3/20/98: Start of push/pull effects
//
// This is where push/pull effects are applied to objects in the sectors.
//
// There are four kinds of push effects
//
// 1) Pushing Away
//
//    Pushes you away from a point source defined by the location of an
//    MT_PUSH Thing. The force decreases linearly with distance from the
//    source. This force crosses sector boundaries and is felt w/in a circle
//    whose center is at the MT_PUSH. The force is felt only if the point
//    MT_PUSH can see the target object.
//
// 2) Pulling toward
//
//    Same as Pushing Away except you're pulled toward an MT_PULL point
//    source. This force crosses sector boundaries and is felt w/in a circle
//    whose center is at the MT_PULL. The force is felt only if the point
//    MT_PULL can see the target object.
//
// 3) Wind
//
//    Pushes you in a constant direction. Full force above ground, half
//    force on the ground, nothing if you're below it (water).
//
// 4) Current
//
//    Pushes you in a constant direction. No force above ground, full
//    force if on the ground or below it (water).
//
// The magnitude of the force is controlled by the length of a controlling
// linedef. The force vector for types 3 & 4 is determined by the angle
// of the linedef, and is constant.
//
// For each sector where these effects occur, the sector special type has
// to have the PUSH_MASK bit set. If this bit is turned off by a switch
// at run-time, the effect will not occur. The controlling sector for
// types 1 & 2 is the sector containing the MT_PUSH/MT_PULL Thing.


#define PUSH_FACTOR 7

/////////////////////////////
//
// Add a push thinker to the thinker list

DPusher::DPusher (DPusher::EPusher type, line_t *l, int magnitude, int angle,
				  AActor *source, int affectee)
{
	m_Source = source ? source->ptr() : AActor::AActorPtr();
	m_Type = type;
	if (l)
	{
		m_Xmag = l->dx>>FRACBITS;
		m_Ymag = l->dy>>FRACBITS;
		m_Magnitude = P_AproxDistance (m_Xmag, m_Ymag);
	}
	else
	{ // [RH] Allow setting magnitude and angle with parameters
		ChangeValues (magnitude, angle);
	}
	if (source) // point source exist?
	{
		m_Radius = (m_Magnitude) << (FRACBITS+1); // where force goes to zero
		m_X = m_Source->x;
		m_Y = m_Source->y;
	}
	m_Affectee = affectee;
}

/////////////////////////////
//
// PIT_PushThing determines the angle and magnitude of the effect.
// The object's x and y momentum values are changed.
//
// tmpusher belongs to the point source (MT_PUSH/MT_PULL).
//

DPusher *tmpusher; // pusher structure for blockmap searches

BOOL PIT_PushThing (AActor *thing)
{
	if (thing->player &&
		!(thing->flags & (MF_NOGRAVITY | MF_NOCLIP)))
	{
		int sx = tmpusher->m_X;
		int sy = tmpusher->m_Y;
		int dist = P_AproxDistance (thing->x - sx,thing->y - sy);
		int speed = (tmpusher->m_Magnitude -
					((dist>>FRACBITS)>>1))<<(FRACBITS-PUSH_FACTOR-1);

		// If speed <= 0, you're outside the effective radius. You also have
		// to be able to see the push/pull source point.

		if (speed > 0 && P_CheckSight(thing, tmpusher->m_Source))
		{
			angle_t pushangle = P_PointToAngle (thing->x, thing->y, sx, sy);
			if (tmpusher->m_Source->type == MT_PUSH)
				pushangle += ANG180;    // away
			pushangle >>= ANGLETOFINESHIFT;
			thing->momx += FixedMul (speed, finecosine[pushangle]);
			thing->momy += FixedMul (speed, finesine[pushangle]);
		}
	}
	return true;
}

/////////////////////////////
//
// T_Pusher looks for all objects that are inside the radius of
// the effect.
//
extern fixed_t tmbbox[4];

void DPusher::RunThink ()
{
	sector_t *sec;
	AActor *thing;
	msecnode_t *node;
	int xspeed,yspeed;
	int xl,xh,yl,yh,bx,by;
	int radius;
	int ht = 0;

	sec = sectors + m_Affectee;

	// Be sure the special sector type is still turned on. If so, proceed.
	// Else, bail out; the sector type has been changed on us.

	if (!(sec->special & PUSH_MASK))
		return;

	// For constant pushers (wind/current) there are 3 situations:
	//
	// 1) Affected Thing is above the floor.
	//
	//    Apply the full force if wind, no force if current.
	//
	// 2) Affected Thing is on the ground.
	//
	//    Apply half force if wind, full force if current.
	//
	// 3) Affected Thing is below the ground (underwater effect).
	//
	//    Apply no force if wind, full force if current.
	//
	// Apply the effect to clipped players only for now.
	//
	// In Phase II, you can apply these effects to Things other than players.

	if (m_Type == p_push)
	{
		// Seek out all pushable things within the force radius of this
		// point pusher. Crosses sectors, so use blockmap.

		tmpusher = this; // MT_PUSH/MT_PULL point source
		radius = m_Radius; // where force goes to zero
		tmbbox[BOXTOP]    = m_Y + radius;
		tmbbox[BOXBOTTOM] = m_Y - radius;
		tmbbox[BOXRIGHT]  = m_X + radius;
		tmbbox[BOXLEFT]   = m_X - radius;

		xl = (tmbbox[BOXLEFT] - bmaporgx - MAXRADIUS)>>MAPBLOCKSHIFT;
		xh = (tmbbox[BOXRIGHT] - bmaporgx + MAXRADIUS)>>MAPBLOCKSHIFT;
		yl = (tmbbox[BOXBOTTOM] - bmaporgy - MAXRADIUS)>>MAPBLOCKSHIFT;
		yh = (tmbbox[BOXTOP] - bmaporgy + MAXRADIUS)>>MAPBLOCKSHIFT;
		for (bx=xl ; bx<=xh ; bx++)
			for (by=yl ; by<=yh ; by++)
				P_BlockThingsIterator (bx, by, PIT_PushThing);
		return;
	}

	// constant pushers p_wind and p_current

	if (sec->heightsec) // special water sector?
		ht = P_FloorHeight(sec->heightsec);
	node = sec->touching_thinglist; // things touching this sector
	for ( ; node ; node = node->m_snext)
	{
		thing = node->m_thing;
		if (!thing->player || (thing->flags & (MF_NOGRAVITY | MF_NOCLIP)))
			continue;
		if (m_Type == p_wind)
		{
			if (sec->heightsec == NULL ||
				sec->heightsec->MoreFlags & SECF_IGNOREHEIGHTSEC) // NOT special water sector
			{
				if (thing->z > thing->floorz) // above ground
				{
					xspeed = m_Xmag; // full force
					yspeed = m_Ymag;
				}
				else // on ground
				{
					xspeed = (m_Xmag)>>1; // half force
					yspeed = (m_Ymag)>>1;
				}
			}
			else // special water sector
			{
				if (thing->z > ht) // above ground
				{
					xspeed = m_Xmag; // full force
					yspeed = m_Ymag;
				}
				else if (thing->player->viewz < ht) // underwater
					xspeed = yspeed = 0; // no force
				else // wading in water
				{
					xspeed = (m_Xmag)>>1; // half force
					yspeed = (m_Ymag)>>1;
				}
			}
		}
		else // p_current
		{
			if (sec->heightsec == NULL ||
				sec->heightsec->MoreFlags & SECF_IGNOREHEIGHTSEC)
			{ // NOT special water sector
				if (thing->z > P_FloorHeight(sec)) // above ground
					xspeed = yspeed = 0; // no force
				else // on ground
				{
					xspeed = m_Xmag; // full force
					yspeed = m_Ymag;
				}
			}
			else
			{ // special water sector
				if (thing->z > ht) // above ground
					xspeed = yspeed = 0; // no force
				else // underwater
				{
					xspeed = m_Xmag; // full force
					yspeed = m_Ymag;
				}
			}
		}
		thing->momx += xspeed<<(FRACBITS-PUSH_FACTOR);
		thing->momy += yspeed<<(FRACBITS-PUSH_FACTOR);
	}
}

/////////////////////////////
//
// P_GetPushThing() returns a pointer to an MT_PUSH or MT_PULL thing,
// NULL otherwise.

AActor *P_GetPushThing (int s)
{
	AActor* thing;
	sector_t* sec;

	sec = sectors + s;
	thing = sec->thinglist;
	while (thing)
	{
		switch (thing->type)
		{
		  case MT_PUSH:
		  case MT_PULL:
			return thing;
		  default:
			break;
		}
		thing = thing->snext;
	}
	return NULL;
}

/////////////////////////////
//
// Initialize the sectors where pushers are present
//

static void P_SpawnPushers(void)
{
	int i;
	line_t *l = lines;
	register int s;

	for (i = 0; i < numlines; i++, l++)
		switch (l->special)
		{
		  case Sector_SetWind: // wind
			for (s = -1; (s = P_FindSectorFromTag (l->args[0],s)) >= 0 ; )
				new DPusher (DPusher::p_wind, l->args[3] ? l : NULL, l->args[1], l->args[2], NULL, s);
			break;
		  case Sector_SetCurrent: // current
			for (s = -1; (s = P_FindSectorFromTag (l->args[0],s)) >= 0 ; )
				new DPusher (DPusher::p_current, l->args[3] ? l : NULL, l->args[1], l->args[2], NULL, s);
			break;
		  case PointPush_SetForce: // push/pull
			if (l->args[0]) {	// [RH] Find thing by sector
				for (s = -1; (s = P_FindSectorFromTag (l->args[0], s)) >= 0 ; )
				{
					AActor *thing = P_GetPushThing (s);
					if (thing) {	// No MT_P* means no effect
						// [RH] Allow narrowing it down by tid
						if (!l->args[1] || l->args[1] == thing->tid)
							new DPusher (DPusher::p_push, l->args[3] ? l : NULL, l->args[2],
										 0, thing, s);
					}
				}
			} else {	// [RH] Find thing by tid
				AActor *thing = NULL;

				while ( (thing = AActor::FindByTID (thing, l->args[1])) )
				{
					if (thing->type == MT_PUSH || thing->type == MT_PULL)
						new DPusher (DPusher::p_push, l->args[3] ? l : NULL, l->args[2],
									 0, thing, thing->subsector->sector - sectors);
				}
			}
			break;
		}
}

//
// phares 3/20/98: End of Pusher effects
//
////////////////////////////////////////////////////////////////////////////

// [AM] Trigger a special associated with an actor.
bool A_CheckTrigger(AActor *mo, AActor *triggerer) {
	if (mo->special &&
		(triggerer->player ||
		 ((mo->flags & MF_AMBUSH) && (triggerer->flags2 & MF2_MCROSS)) ||
		 ((mo->flags2 & MF2_DORMANT) && (triggerer->flags2 & MF2_PCROSS)))) {
		int savedSide = TeleportSide;
		TeleportSide = 0;
		bool res = (LineSpecials[mo->special](NULL, triggerer, mo->args[0],
											 mo->args[1], mo->args[2],
											 mo->args[3], mo->args[4]) != 0);
		TeleportSide = savedSide;
		return res;
	}
	return false;
}

// [AM] Selectively trigger a list of sector action specials that are linked by
//      their tracer fields based on the passed activation type.
bool A_TriggerAction(AActor *mo, AActor *triggerer, int activationType) {
	bool trigger_action = false;

	// The mobj type must agree with the activation type.
	switch (mo->type) {
	case MT_SECACTENTER:
		trigger_action = ((activationType & SECSPAC_Enter) != 0);
		break;
	case MT_SECACTEXIT:
		trigger_action = ((activationType & SECSPAC_Exit) != 0);
		break;
	case MT_SECACTHITFLOOR:
		trigger_action = ((activationType & SECSPAC_HitFloor) != 0);
		break;
	case MT_SECACTHITCEIL:
		trigger_action = ((activationType & SECSPAC_HitCeiling) != 0);
		break;
	case MT_SECACTUSE:
		trigger_action = ((activationType & SECSPAC_Use) != 0);
		break;
	case MT_SECACTUSEWALL:
		trigger_action = ((activationType & SECSPAC_UseWall) != 0);
		break;
	case MT_SECACTEYESDIVE:
		trigger_action = ((activationType & SECSPAC_EyesDive) != 0);
		break;
	case MT_SECACTEYESSURFACE:
		trigger_action = ((activationType & SECSPAC_EyesSurface) != 0);
		break;
	case MT_SECACTEYESBELOWC:
		trigger_action = ((activationType & SECSPAC_EyesBelowC) != 0);
		break;
	case MT_SECACTEYESABOVEC:
		trigger_action = ((activationType & SECSPAC_EyesAboveC) != 0);
		break;
	default:
		// This isn't a sector action mobj.
		break;
	}

	if (trigger_action) {
		trigger_action = A_CheckTrigger(mo, triggerer);
	}

	// The tracer field could potentially contain a pointer to another
	// actor special.
	if (mo->tracer != NULL) {
		return trigger_action | A_TriggerAction(mo->tracer, triggerer, activationType);
	}

	return trigger_action;
}

VERSION_CONTROL (p_spec_cpp, "$Id$")

