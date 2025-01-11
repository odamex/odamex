
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


#include "odamex.h"

#include "m_alloc.h"
#include "gstrings.h"

#include "z_zone.h"
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
#include "oscanner.h"

// State.
#include "r_state.h"

#include "c_console.h"

// [RH] Needed for sky scrolling
#include "r_sky.h"
#include <g_gametype.h>

// [Blair] Map format
#include "p_mapformat.h"

#include "p_boomfspec.h"
#include "p_zdoomhexspec.h"

EXTERN_CVAR(sv_allowexit)
EXTERN_CVAR(sv_fragexitswitch)
EXTERN_CVAR(co_boomphys)

std::list<movingsector_t> movingsectors;
std::list<sector_t*> specialdoors;
bool s_SpecialFromServer;

int P_FindSectorFromLineTag(int tag, int start);
BOOL EV_DoDoor(DDoor::EVlDoor type, line_t* line, AActor* thing, int tag, int speed,
               int delay, card_t lock);
bool P_ShootCompatibleSpecialLine(AActor* thing, line_t* line);
bool P_ActivateZDoomLine(line_t* line, AActor* mo, int side,
                                 unsigned int activationType);
bool P_UseCompatibleSpecialLine(AActor* thing, line_t* line, int side,
                                        bool bossaction);

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

fixed_t P_ArgsToFixed(fixed_t arg_i, fixed_t arg_f)
{
	return (arg_i << FRACBITS) + (arg_f << FRACBITS) / 100;
}

int P_ArgToCrushMode(byte arg, bool slowdown)
{
	static const crushmode_e map[] = {crushDoom, crushHexen, crushSlowdown};

	if (arg >= 1 && arg <= 3)
		return map[arg - 1];

	return slowdown ? crushSlowdown : crushDoom;
}

int P_FindSectorFromLineTag(const line_t* line, int start)
{
	start = start >= 0 ? sectors[start].nexttag
	                   : sectors[(unsigned)line->id % (unsigned)numsectors].firsttag;
	while (start >= 0 && sectors[start].tag != line->id)
		start = sectors[start].nexttag;
	return start;
}


int P_FindLineFromLineTag(const line_t* line, int start)
{
	start = start >= 0 ? lines[start].nextid
	                   : lines[(unsigned)line->id % (unsigned)numlines].firstid;
	while (start >= 0 && lines[start].id != line->id)
		start = lines[start].nextid;
	return start;
}

int P_FindLineFromTag(int tag, int start)
{
	start = start >= 0 ? lines[start].nextid
	                   : lines[(unsigned)tag % (unsigned)numlines].firstid;
	while (start >= 0 && lines[start].id != tag)
		start = lines[start].nextid;
	return start;
}

unsigned int P_ResetSectorTransferFlags(const unsigned int flags)
{
	return (flags & ~SECF_TRANSFERMASK);
}

void P_ResetSectorSpecial(sector_t* sector)
{
	sector->special = 0;
	sector->damageamount = 0;
	sector->damageinterval = 0;
	sector->leakrate = 0;
	sector->flags = P_ResetSectorTransferFlags(sector->flags);
}

void P_CopySectorSpecial(sector_t* dest, sector_t* source)
{
	dest->special = source->special;
	dest->damageamount = source->damageamount;
	dest->damageinterval = source->damageinterval;
	dest->leakrate = source->leakrate;
	P_TransferSectorFlags(&dest->flags, source->flags);
}

void P_ResetTransferSpecial(newspecial_s* newspecial)
{
	newspecial->special = 0;
	newspecial->damageamount = 0;
	newspecial->damageinterval = 0;
	newspecial->damageleakrate = 0;
	newspecial->flags = P_ResetSectorTransferFlags(newspecial->flags);
}

void P_TransferSectorFlags(unsigned int* dest, unsigned int source)
{
	*dest &= ~SECF_TRANSFERMASK;
	*dest |= source & SECF_TRANSFERMASK;
}

byte P_ArgToChange(byte arg)
{
	static const byte ChangeMap[8] = {0, 1, 5, 3, 7, 2, 6, 0};

	return (arg < 8) ? ChangeMap[arg] : 0;
}

int P_ArgToCrush(byte arg)
{
	return (arg > 0) ? arg : NO_CRUSH;
}

/*
 * P_IsUnderDamage
 *
 * killough 9/9/98:
 *
 * Returns nonzero if the object is under damage based on
 * their current position.
 */
int P_IsUnderDamage(AActor* actor)
{
	const struct msecnode_s* seclist;
	const DCeiling* cr; // Crushing ceiling
	int dir = 0;
	for (seclist = actor->touching_sectorlist; seclist; seclist = seclist->m_tnext)
	{
		if ((cr = (DCeiling*)seclist->m_sector->ceilingdata) && cr->m_Status == 2) // Down
		{
			cr->m_Crush > NO_CRUSH ? dir = 1 : dir = 0;
		}
	}
	return dir;
}

/*
*
* P_IsFriendlyThing
* @brief Helper function to determine if a particular thing is of friendly origin.
*
* @param actor Source actor
* @param friendshiptest Thing to test friendliness
*/
bool P_IsFriendlyThing(AActor* actor, AActor* friendshiptest)
{
	if (friendshiptest->flags & MF_FRIEND)
	{
		if (G_IsCoopGame())
		{
			return true;
		}
		else if (actor->player && friendshiptest->target && friendshiptest->target->player &&
		    actor->player->userinfo.team == friendshiptest->target->player->userinfo.team)
		{
			return true;
		}
		else
		{
			return false;
		}
	}
	else
	{
		return false;
	}
}

//
// P_AddMovingCeiling
//
// Updates the movingsectors list to include the passed sector, which
// tracks which sectors currently have a moving ceiling/floor
//
void P_AddMovingCeiling(sector_t *sector)
{
	if (!sector || (clientside && consoleplayer().spectator))
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
	if (!sector || (clientside && consoleplayer().spectator))
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
	if (arc.IsStoring())
	{
		arc << m_Type << m_Source << m_Xmag << m_Ymag << m_Magnitude << m_Radius << m_X
		    << m_Y << m_Affectee;
	}
	else
	{
		arc >> m_Type;
		arc.ReadObject((DObject*&)*m_Source, DPusher::StaticType());
		arc >> m_Xmag >> m_Ymag >> m_Magnitude >> m_Radius >> m_X >> m_Y >> m_Affectee;
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

// killough 3/7/98: Initialize generalized scrolling
static void P_SpawnScrollers();

static void P_SpawnFriction();		// phares 3/16/98
static void P_SpawnPushers();		// phares 3/20/98
static void P_SpawnExtra();

static void ParseAnim(OScanner &os, byte istex);

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
	// [DE] this hacky try/catch is here so WADs with ZDoom ANIMDEFS extensions
	// don't crash for now. Once TextureManager is fully implemented I'll just
	// implement the extensions properly there
	try
	{
		int lump = -1;

		while ((lump = W_FindLump("ANIMDEFS", lump)) != -1)
		{
			const char* buffer = static_cast<char*>(W_CacheLumpNum(lump, PU_STATIC));

			OScannerConfig config = {
			    "ANIMDEFS", // lumpName
			    false,      // semiComments
			    true,       // cComments
			};
			OScanner os = OScanner::openBuffer(config, buffer, buffer + W_LumpLength(lump));

			while (os.scan())
			{
				if (os.compareTokenNoCase("flat"))
				{
					ParseAnim(os, false);
				}
				else if (os.compareTokenNoCase("texture"))
				{
					ParseAnim(os, true);
				}
				else if (os.compareTokenNoCase(
				             "switch")) // Don't support switchdef yet...
				{
					// P_ProcessSwitchDef();
					// os.error("switchdef not supported.");
				}
				else if (os.compareTokenNoCase("warp"))
				{
					os.mustScan();
					if (os.compareTokenNoCase("flat"))
					{
						os.mustScan();
						flatwarp[R_FlatNumForName(os.getToken().c_str())] = true;
					}
					else if (os.compareTokenNoCase("texture"))
					{
						// TODO: Make texture warping work with wall textures
						os.mustScan();
						R_TextureNumForName(os.getToken().c_str());
					}
					else
					{
						os.error("Unknown error reading in ANIMDEFS");
					}
				}
			}
		}
	}
    catch (CRecoverableError &)
    {

    }
}

static void ParseAnim(OScanner &os, byte istex)
{
	anim_t sink;
	anim_t *place;

	os.mustScan();
	const short picnum = istex
		                     ? R_CheckTextureNumForName(os.getToken().c_str())
		                     : W_CheckNumForName(os.getToken().c_str(), ns_flats) - firstflat;

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
				const size_t newmax = maxanims ? maxanims * 2 : MAXANIMS;
				anims = static_cast<anim_t*>(Realloc(anims, newmax * sizeof(*anims)));
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

	while (os.scan())
	{
		/*if (os.compareTokenNoCase("allowdecals"))
		{
			if (istex && picnum >= 0)
			{
				texturenodecals[picnum] = 0;
			}
			continue;
		}
		else*/ if (!os.compareTokenNoCase("pic"))
		{
			os.unScan();
			break;
		}

		if (place->numframes == MAX_ANIM_FRAMES)
		{
			os.error("Animation has too many frames");
		}

		byte min = 1;
		byte max = 1;

		os.mustScanInt();
		const int frame = os.getTokenInt();
		os.mustScan();
		if (os.compareToken("tics"))
		{
			os.mustScanInt();
			min = max = clamp(os.getTokenInt(), 0, 255);
		}
		else if (os.compareToken("rand"))
		{
			os.mustScanInt();
			int num = os.getTokenInt();
			min = num >= 0 ? num : 0;
			os.mustScanInt();
			num = os.getTokenInt();
			max = num <= 255 ? num : 255;
		}
		else
		{
			os.error("Must specify a duration for animation frame");
		}

		place->speedmin[place->numframes] = min;
		place->speedmax[place->numframes] = max;
		place->framepic[place->numframes] = frame + picnum - 1;
		place->numframes++;
	}

	if (place->numframes < 2)
	{
		os.error("Animation needs at least 2 frames");
	}

	place->countdown = place->speedmin[0];
}

//
// P_CheckTag()
//
// Passed a line, returns true if the tag is non-zero or the line special
// allows no tag without harm. If compatibility, all linedef specials are
// allowed to have zero tag.
//
// Note: Only line specials activated by walkover, pushing, or shooting are
//       checked by this routine.
//
// jff 2/27/98 Added to check for zero tag allowed for regular special types
//
bool P_CheckTag(line_t* line)
{
	/* tag not zero, allowed, or
	 * killough 11/98: compatibility option */
	if (line->id) // e6y
		return true;

	switch (line->special)
	{
	case 1: // Manual door specials
	case 26:
	case 27:
	case 28:
	case 31:
	case 32:
	case 33:
	case 34:
	case 117:
	case 118:

	case 139: // Lighting specials
	case 170:
	case 79:
	case 35:
	case 138:
	case 171:
	case 81:
	case 13:
	case 192:
	case 169:
	case 80:
	case 12:
	case 194:
	case 173:
	case 157:
	case 104:
	case 193:
	case 172:
	case 156:
	case 17:

	case 195: // Thing teleporters
	case 174:
	case 97:
	case 39:
	case 126:
	case 125:
	case 210:
	case 209:
	case 208:
	case 207:

	case 11: // Exits
	case 52:
	case 197:
	case 51:
	case 124:
	case 198:
	case 2069:
	case 2070:
	case 2071:
	case 2072:
	case 2073:
	case 2074:

	case 48: // Scrolling walls
	case 85:
	case 2082:
	case 2083:
		return true; // zero tag allowed

	default:
		break;
	}
	if (!demoplayback && line->special >= GenCrusherBase && line->special <= GenEnd)
	{
		if ((line->special & 6) != 6) // e6y //jff 2/27/98 all non-manual
			return false;             // generalized types require tag
		else
			return true;
	}
	return false; // zero tag not allowed
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

	if (W_CheckNumForName ("ANIMATED") == -1)
		return;

	animdefs = (byte *)W_CacheLumpName ("ANIMATED", PU_STATIC);

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
				if (W_CheckNumForName ((char *)anim_p + 10 /* .startname */, ns_flats) == -1 ||
					W_CheckNumForName ((char *)anim_p + 1 /* .startname */, ns_flats) == -1)
					continue;

				lastanim->basepic = R_FlatNumForName (anim_p + 10 /* .startname */);
				lastanim->numframes = R_FlatNumForName (anim_p + 1 /* .endname */)
									  - lastanim->basepic + 1;
			}

			lastanim->istexture = *anim_p /* .istexture */;
			lastanim->uniqueframes = false;
			lastanim->curframe = 0;

			if (lastanim->numframes < 2)
				Printf (PRINT_WARNING, "P_InitPicAnims: bad cycle from %s to %s",
						 anim_p + 10 /* .startname */,
						 anim_p + 1 /* .endname */);

			lastanim->speedmin[0] = lastanim->speedmax[0] = lastanim->countdown =
						/* .speed */
						(anim_p[19] << 0) |
						(anim_p[20] << 8) |
						(anim_p[21] << 16) |
						(anim_p[22] << 24);

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
fixed_t P_FindShortestTextureAround (sector_t *sec)
{
	int minsize = MAXINT;
	side_t *side;
	int i;
	int mintex = co_boomphys ? 1 : 0;

	for (i = 0; i < sec->linecount; i++)
	{
		if (sec->lines[i]->flags & ML_TWOSIDED)
		{
			side = &sides[(sec->lines[i])->sidenum[0]];
			if (side->bottomtexture >= mintex && textureheight[side->bottomtexture] < minsize)
				minsize = textureheight[side->bottomtexture];

			side = &sides[(sec->lines[i])->sidenum[1]];
			if (side->bottomtexture >= mintex && textureheight[side->bottomtexture] < minsize)
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
fixed_t P_FindShortestUpperAround (sector_t *sec)
{
	int minsize = MAXINT;
	side_t *side;
	int i;
	int mintex = co_boomphys ? 1 : 0;

	for (i = 0; i < sec->linecount; i++)
	{
		if (sec->lines[i]->flags & ML_TWOSIDED)
		{
			side = &sides[(sec->lines[i])->sidenum[0]];
			if (side->toptexture >= mintex && textureheight[side->toptexture] < minsize)
				minsize = textureheight[side->toptexture];

			side = &sides[(sec->lines[i])->sidenum[1]];
			if (side->toptexture >= mintex && textureheight[side->toptexture] < minsize)
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
sector_t *P_FindModelFloorSector (fixed_t floordestheight, sector_t *sec)
{
	sector_t *other;

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
sector_t *P_FindModelCeilingSector (fixed_t ceildestheight, sector_t *sec)
{
	sector_t *other;

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

bool P_CeilingActive(const sector_t* sec)
{
	return sec->ceilingdata != NULL || (sec->floordata != NULL);
}

bool P_FloorActive(const sector_t* sec)
{
	return sec->floordata != NULL || (sec->ceilingdata != NULL);
}

bool P_LightingActive(const sector_t* sec)
{
	return sec->lightingdata != NULL;
}

int P_FindSectorFromTagOrLine(int tag, const line_t* line, int start)
{
	if (tag == 0)
	{
		if (!line || !line->backsector || line->backsector - sectors == start)
		{
			return -1;
		}

		return line->backsector - sectors;
	}
	else
	{
		return P_FindSectorFromTag(tag, start);
	}
}

/*
* @brief checks to see if a ZDoom-style door can be unlocked.
*
* @param player: Player to key check
* @param lock: ZDoom lock type
* All ZDoom lock types are supported but Odamex is missing
* key prism types.
*/
bool P_CanUnlockZDoomDoor(player_t* player, zdoom_lock_t lock, bool remote)
{
	if (!player)
		return false;

	const OString* msg = NULL;

	switch (lock)
	{
	case zk_none:
		break;
	case zk_red_card:
		if (!player->cards[it_redcard])
		{
			msg = remote ? &PD_REDO : &PD_REDC;
		}
		else
		{
			return true;
		}
		break;
	case zk_blue_card:
		if (!player->cards[it_bluecard])
		{
			msg = remote ? &PD_BLUEO : &PD_BLUEC;
		}
		else
		{
			return true;
		}
		break;
	case zk_yellow_card:
		if (!player->cards[it_yellowcard])
		{
			msg = remote ? &PD_YELLOWO : &PD_YELLOWC;
		}
		else
		{
			return true;
		}
		break;
	case zk_red_skull:
		if (!player->cards[it_redskull])
		{
			msg = remote ? &PD_REDO : &PD_REDS;
		}
		else
		{
			return true;
		}
		break;
	case zk_blue_skull:
		if (!player->cards[it_blueskull])
		{
			msg = remote ? &PD_BLUEO : &PD_BLUES;
		}
		else
		{
			return true;
		}
		break;
	case zk_yellow_skull:
		if (!player->cards[it_yellowskull])
		{
			msg = remote ? &PD_YELLOWO : &PD_YELLOWS;
		}
		else
		{
			return true;
		}
		break;
	case zk_any:
		if (!player->cards[it_redcard] && !player->cards[it_redskull] &&
		    !player->cards[it_bluecard] && !player->cards[it_blueskull] &&
		    !player->cards[it_yellowcard] && !player->cards[it_yellowskull])
		{
			msg = &PD_ANY;
		}
		else
		{
			return true;
		}
		break;
	case zk_all:
		if (!player->cards[it_redcard] || !player->cards[it_redskull] ||
		    !player->cards[it_bluecard] || !player->cards[it_blueskull] ||
		    !player->cards[it_yellowcard] || !player->cards[it_yellowskull])
		{
			msg = &PD_ALL6;
		}
		else
		{
			return true;
		}
		break;
	case zk_red:
	case zk_redx:
		if (!player->cards[it_redcard] && !player->cards[it_redskull])
		{
			msg = remote ? &PD_REDO : &PD_REDK;
		}
		else
		{
			return true;
		}
		break;
	case zk_blue:
	case zk_bluex:
		if (!player->cards[it_bluecard] && !player->cards[it_blueskull])
		{
			msg = remote ? &PD_BLUEO : &PD_BLUEK;
		}
		else
		{
			return true;
		}
		break;
	case zk_yellow:
	case zk_yellowx:
		if (!player->cards[it_yellowcard] && !player->cards[it_yellowskull])
		{
			msg = remote ? &PD_YELLOWO : &PD_YELLOWK;
		}
		else
		{
			return true;
		}
		break;
	case zk_each_color:
		if ((!player->cards[it_redcard] && !player->cards[it_redskull]) ||
		    (!player->cards[it_bluecard] && !player->cards[it_blueskull]) ||
		    (!player->cards[it_yellowcard] && !player->cards[it_yellowskull]))
		{
			msg = &PD_ALL3;
		}
		else
		{
			return true;
		}
	default:
		break;
	}

	// If we get here, we don't have the right key,
	// so print an appropriate message and grunt.
	if (player->mo == consoleplayer().camera)
	{
		int keytrysound = S_FindSound("misc/keytry");
		if (keytrysound > -1)
		{
			UV_SoundAvoidPlayer(player->mo, CHAN_VOICE, "misc/keytry", ATTN_NORM);
		}
		else
		{
			UV_SoundAvoidPlayer(player->mo, CHAN_VOICE, "player/male/grunt1", ATTN_NORM);
		}

		if (msg != NULL)
		{
			C_MidPrint(GStrings(*msg), player);
		}
	}

	return false;
}

//
// P_CanUnlockGenDoor()
//
// Passed a generalized locked door linedef and a player, returns whether
// the player has the keys necessary to unlock that door.
//
// Note: The linedef passed MUST be a generalized locked door type
//       or results are undefined.
//
// jff 02/05/98 routine added to test for unlockability of
//  generalized locked doors
//
bool P_CanUnlockGenDoor(line_t* line, player_t* player)
{
	if (!player)
		return false;

	const OString* msg = NULL;

	// does this line special distinguish between skulls and keys?
	int skulliscard = (line->special & LockedNKeys) >> LockedNKeysShift;

	// determine for each case of lock type if player's keys are adequate
	switch ((line->special & LockedKey) >> LockedKeyShift)
	{
	case 0: // AnyKey
		if (!player->cards[it_redcard] && !player->cards[it_redskull] &&
		    !player->cards[it_bluecard] && !player->cards[it_blueskull] &&
		    !player->cards[it_yellowcard] && !player->cards[it_yellowskull])
		{
			msg = &PD_ANY; // Ty 03/27/98 - externalized
		}
		else
		{
			return true;
		}
		break;
	case 1: // RCard
		if (!player->cards[it_redcard] && (!skulliscard || !player->cards[it_redskull]))
		{
			msg = skulliscard ? &PD_REDK : &PD_REDC; // Ty 03/27/98 - externalized
		}
		else
		{
			return true;
		}
		break;
	case 2: // BCard
		if (!player->cards[it_bluecard] && (!skulliscard || !player->cards[it_blueskull]))
		{
			msg = skulliscard ? &PD_BLUEK : &PD_BLUEC; // Ty 03/27/98 - externalized
		}
		else
		{
			return true;
		}
		break;
	case 3: // YCard
		if (!player->cards[it_yellowcard] &&
		    (!skulliscard || !player->cards[it_yellowskull]))
		{
			msg = skulliscard ? &PD_YELLOWK : &PD_YELLOWC; // Ty 03/27/98 - externalized
		}
		else
		{
			return true;
		}
		break;
	case 4: // RSkull
		if (!player->cards[it_redskull] && (!skulliscard || !player->cards[it_redcard]))
		{
			msg = skulliscard ? &PD_REDK : &PD_REDS; // Ty 03/27/98 - externalized
		}
		else
		{
			return true;
		}
		break;
	case 5: // BSkull
		if (!player->cards[it_blueskull] && (!skulliscard || !player->cards[it_bluecard]))
		{
			msg = skulliscard ? &PD_BLUEK : &PD_BLUES; // Ty 03/27/98 - externalized
		}
		else
		{
			return true;
		}
		break;
	case 6: // YSkull
		if (!player->cards[it_yellowskull] &&
		    (!skulliscard || !player->cards[it_yellowcard]))
		{
			msg = skulliscard ? &PD_YELLOWK : &PD_YELLOWS; // Ty 03/27/98 - externalized
		}
		else
		{
			return true;
		}
		break;
	case 7: // AllKeys
		if (!skulliscard &&
		    (!player->cards[it_redcard] || !player->cards[it_redskull] ||
		     !player->cards[it_bluecard] || !player->cards[it_blueskull] ||
		     !player->cards[it_yellowcard] || !player->cards[it_yellowskull]))
		{
			msg = &PD_ALL6; // Ty 03/27/98 - externalized
		}
		else if (skulliscard &&
		    ((!player->cards[it_redcard] && !player->cards[it_redskull]) ||
		     (!player->cards[it_bluecard] && !player->cards[it_blueskull]) ||
		     // e6y
		     // Compatibility with buggy MBF behavior when 3-key door works with only 2
		     // keys There is no more desync on 10sector.wad\ts27-137.lmp
		     // http://www.doomworld.com/tas/ts27-137.zip

		     /*
		     (!player->cards[it_yellowcard] &&
		      (compatibility_level == mbf_compatibility &&
		               !prboom_comp[PC_FORCE_CORRECT_CODE_FOR_3_KEYS_DOORS_IN_MBF].state
		           ? player->cards[it_yellowskull]
		           : !player->cards[it_yellowskull]))))
				   // [Blair] No MBF Compat options...yet
			*/
			(!player->cards[it_yellowcard] && !player->cards[it_yellowskull])))

		{
			msg = &PD_ALL3; // Ty 03/27/98 - externalized
		}
		else
		{
			return true;
		}
		break;
	}
	// If we get here, we don't have the right key,
	// so print an appropriate message and grunt.
	if (player->mo == consoleplayer().camera)
	{
		int keytrysound = S_FindSound("misc/keytry");
		if (keytrysound > -1)
		{
			UV_SoundAvoidPlayer(player->mo, CHAN_VOICE, "misc/keytry", ATTN_NORM);
		}
		else
		{
			UV_SoundAvoidPlayer(player->mo, CHAN_VOICE, "player/male/grunt1", ATTN_NORM);
		}

		if (msg != NULL)
		{
			C_MidPrint(GStrings(*msg), player);
		}
	}

	return false;
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

	const OString* msg = NULL;
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
			msg = &PD_ANY;
			break;

		case AllKeys:
			if (bc && bs && rc && rs && yc && ys)
				return true;
			msg = equiv ? &PD_ALL3 : &PD_ALL6;
			break;

		case RCard:
			if (rc)
				return true;
			msg = equiv ? (remote ? &PD_REDO : &PD_REDK) : &PD_REDC;
			break;

		case BCard:
			if (bc)
				return true;
			msg = equiv ? (remote ? &PD_BLUEO : &PD_BLUEK) : &PD_BLUEC;
			break;

		case YCard:
			if (yc)
				return true;
			msg = equiv ? (remote ? &PD_YELLOWO : &PD_YELLOWK) : &PD_YELLOWC;
			break;

		case RSkull:
			if (rs)
				return true;
			msg = equiv ? (remote ? &PD_REDO : &PD_REDK) : &PD_REDS;
			break;

		case BSkull:
			if (bs)
				return true;
			msg = equiv ? (remote ? &PD_BLUEO : &PD_BLUEK) : &PD_BLUES;
			break;

		case YSkull:
			if (ys)
				return true;
			msg = equiv ? (remote ? &PD_YELLOWO : &PD_YELLOWK) : &PD_YELLOWS;
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

		if (msg != NULL)
			C_MidPrint(GStrings(*msg), p);
	}

	return false;
}

void OnChangedSwitchTexture (line_t *line, int useAgain);
void SV_OnActivatedLine(line_t* line, AActor* mo, const int side,
                        const LineActivationType activationType, const bool bossaction);

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
bool P_HandleSpecialRepeat(line_t* line)
{
	// [SL] Don't remove specials from fragging exit line specials
	if (P_IsExitLine(line->special) &&
	    (!sv_allowexit && sv_fragexitswitch))
		return false;
	else
		return true;
}

//
// P_CrossSpecialLine - TRIGGER
// Called every time a thing origin is about
//  to cross a line with a non 0 special.
//
void P_CrossSpecialLine(line_t*	line, int side, AActor* thing, bool bossaction)
{
	TeleportSide = side;

	// spectators and dead players can't cross special lines
	// [Blair] Unless they're teleport lines.
	if (thing && thing->player &&
	    (thing->player->spectator || thing->player->playerstate != PST_LIVE) &&
	    !P_IsTeleportLine(line->special))
		return;

	if (!bossaction && !P_CanActivateSpecials(thing, line))
		return;

	bool result = false;

	if (thing)
	{
		result = map_format.cross_special_line(line, side, thing, bossaction);
	}

	if (result)
	{
		SV_OnActivatedLine(line, thing, side, LineCross, bossaction);

		bool repeat;

		if (map_format.getZDoom())
			repeat = (line->flags & ML_REPEATSPECIAL) != 0 && P_HandleSpecialRepeat(line);
		else
			repeat = P_IsSpecialBoomRepeatable(line->special);

		if (!repeat && !bossaction && serverside)
		{
			if (!(thing->player &&
			      (thing->player->spectator || thing->player->playerstate != PST_LIVE)))
			{
				// Not a real texture change, but propigate special change to the
				// clients
				P_ChangeSwitchTexture(line, repeat, false);
				OnChangedSwitchTexture(line, repeat);
			}
		}
	}
}

//
// P_ShootSpecialLine - IMPACT SPECIALS
// Called when a thing shoots a special line.
//
void P_ShootSpecialLine(AActor*	thing, line_t* line)
{
	if (!P_CanActivateSpecials(thing, line))
		return;

	if(thing)
	{
		if (map_format.getZDoom() && !(line->flags & ML_SPAC_IMPACT))
			return;

		if (thing->flags & MF_MISSILE)
			return;

		if (map_format.getZDoom() && !thing->player && thing->type != MT_AVATAR &&
		    !(line->flags & ML_MONSTERSCANACTIVATE))
			return;
	}

	//TeleportSide = side;

	bool lineresult;

	if (map_format.getZDoom()) // All zdoom specials can be impact activated
	{
		lineresult = LineSpecials[line->special](line, thing, line->args[0], line->args[1],
		                            line->args[2], line->args[3], line->args[4]);
	}
	else // Only certain specials from Doom/Boom can be impact activated
	{
		lineresult = P_ShootCompatibleSpecialLine(thing, line);
	}

	if(serverside && lineresult)
	{
		SV_OnActivatedLine(line, thing, 0, LineShoot, false);

		if (lineresult)
		{
			bool repeat;

			if (map_format.getZDoom())
				repeat = line->flags & ML_REPEATSPECIAL;
			else
				repeat = P_IsSpecialBoomRepeatable(line->special);

			P_ChangeSwitchTexture(line, repeat, true);
			OnChangedSwitchTexture(line, repeat);
		}
	}
}


//
// P_UseSpecialLine
// Called when a thing uses a special line.
// Only the front sides of lines are usable.
// Returns false if this isn't a door that can be opened
//
bool P_UseSpecialLine(AActor* thing, line_t* line, int side, bool bossaction)
{
 	if (!bossaction && !P_CanActivateSpecials(thing, line))
		return false;

	if(!bossaction && thing)
	{
		// Switches that other things can activate.
		if (!thing->player && thing->type != MT_AVATAR)
		{
			// not for monsters?
			if (map_format.getZDoom() && !(line->flags & ML_MONSTERSCANACTIVATE))
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

	bool result;

	TeleportSide = side;

	if (map_format.getZDoom())
		result = P_ActivateZDoomLine(line, thing, side, ML_SPAC_USE);
	else
		result = P_UseCompatibleSpecialLine(thing, line, side, bossaction);

 	if (result)
	{
		// May need to move this higher as the special is gone in Boom by this point.
		SV_OnActivatedLine(line, thing, side, LineUse, bossaction);

		if (map_format.getZDoom() && !bossaction)
		{
			bool repeat = (line->flags & ML_REPEATSPECIAL) != 0 && P_HandleSpecialRepeat(line);
			P_ChangeSwitchTexture(line, repeat, true);
			OnChangedSwitchTexture(line, repeat);
		}

		return true;
	}
	else
	{
		return false;
	}
}


//
// P_PushSpecialLine
// Called when a thing pushes a special line, only in advanced map format
// Only the front sides of lines are pushable.
//
bool P_PushSpecialLine(AActor* thing, line_t* line, int side)
{
	if (!map_format.getZDoom())
		return false;

	if (!P_CanActivateSpecials(thing, line))
		return false;

	// Err...
	// Use the back sides of VERY SPECIAL lines...
	if (side)
		return false;

	if(thing)
	{
		if (!(line->flags & ML_SPAC_PUSH))
			return false;

		// Switches that other things can activate.
		if (!thing->player && thing->type != MT_AVATAR)
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
			// [Blair] Unless they're teleport walls.
			if((thing->player->spectator ||
                thing->player->playerstate != PST_LIVE) &&
				!P_IsTeleportLine(line->special))
				return false;
		}
	}

    TeleportSide = side;

	if(LineSpecials[line->special] (line, thing, line->args[0],
					line->args[1], line->args[2],
					line->args[3], line->args[4]))
	{
		SV_OnActivatedLine(line, thing, side, LinePush, false);

		if (serverside && !(thing->player && (thing->player->spectator ||
		                                      thing->player->playerstate != PST_LIVE)))
		{
			bool repeat;

			if (map_format.getZDoom())
				repeat = line->flags & ML_REPEATSPECIAL && P_HandleSpecialRepeat(line);
			else
				repeat = P_IsSpecialBoomRepeatable(line->special);

			P_ChangeSwitchTexture(line, repeat, true);
			OnChangedSwitchTexture(line, repeat);
		}
	}

    return true;
}

void P_ApplySectorDamageNoWait(player_t* player, int damage, int mod)
{
	P_DamageMobj(player->mo, NULL, NULL, damage, mod);
}

void P_ApplySectorDamageNoRandom(player_t* player, int damage, int mod)
{
	if (!player->powers[pw_ironfeet])
		if (!(level.time & 0x1f))
			P_DamageMobj(player->mo, NULL, NULL, damage, mod);
}

void P_ApplySectorDamage(player_t* player, int damage, int leak, int mod)
{
	if (!player->powers[pw_ironfeet] || (leak && P_Random(player->mo)<leak))
		if (!(level.time & 0x1f))
			P_DamageMobj(player->mo, NULL, NULL, damage, mod);
}

void P_ApplySectorDamageEndLevel(player_t* player)
{
	//if (comp[comp_god])
	player->cheats &= ~CF_GODMODE;

	if (!(level.time & 0x1f))
		P_DamageMobj(player->mo, NULL, NULL, 20);

	if (player->health <= 10)
		if (sv_allowexit)
			G_ExitLevel(0, 1);
}

#ifdef SERVER_APP
void SV_UpdateSecret(sector_t& sector, player_t &player);
#endif

void P_CollectSecretCommon(sector_t* sector, player_t* player)
{
	player->secretcount++;
	level.found_secrets++;
	sector->flags &= ~SECF_SECRET;

#ifdef SERVER_APP
	SV_UpdateSecret(*sector, *player); // Update the sector to all clients so that they
	                                   // don't discover an already found secret.
#else
	if (player->mo == consoleplayer().camera)
		C_RevealSecret(); // Display the secret revealed message
#endif
}

void P_CollectSecretVanilla(sector_t* sector, player_t* player)
{
	sector->special = 0;
	P_CollectSecretCommon(sector, player);
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

	// Update sky column offsets
	sky1columnoffset += level.sky1ScrollDelta & 0xffffff;
	sky2columnoffset += level.sky2ScrollDelta & 0xffffff;
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

/*
* @brief Sets up the world state depending on map format.
* Sets up map sector line specials, thinkers, and other things
* needed to make Doom interesting.
*/
void P_SetupWorldState(void)
{
	sector_t* sector;
	int i;

	//	Init special SECTORs.
	sector = sectors;
	for (i = 0; i < numsectors; i++, sector++)
	{
		map_format.init_sector_special(sector);
	}

	// Init other misc stuff

	// P_InitTagLists() must be called before P_FindSectorFromTag()
	// or P_FindLineFromID() can be called.
	P_SpawnScrollers(); // killough 3/7/98: Add generalized scrollers
	P_SpawnFriction();  // phares 3/12/98: New friction model using linedefs
	P_SpawnPushers();   // phares 3/20/98: New pusher model using linedefs
	P_SpawnExtra();		// [Blair] Finish any linedef types that affect sectors

	// [RH] Start running any open scripts on this map
	if (level.behavior != NULL)
	{
		level.behavior->StartTypedScripts(SCRIPT_Open, NULL, 0, 0, 0, false);
	}
}

void P_AddSectorSecret(sector_t* sector)
{
	sector->secretsector = true;
	level.total_secrets++;
	sector->flags |= SECF_SECRET | SECF_WASSECRET;
}

void P_SetupSectorDamage(sector_t* sector, short amount, byte interval, byte leakrate,
                         unsigned int flags)
{
	// Only set if damage is not yet initialized.
	if (sector->damageamount)
		return;

	sector->damageamount = amount;
	sector->damageinterval = interval;
	sector->leakrate = leakrate;
	sector->flags = (sector->flags & ~SECF_DAMAGEFLAGS) | (flags & SECF_DAMAGEFLAGS);
}

void P_ClearNonGeneralizedSectorSpecial(sector_t* sector)
{
	// jff 3/14/98 clear non-generalized sector type
	sector->special &= map_format.getGeneralizedMask();
}

void P_DestroyScrollerThinkers()
{
	// Destroy scrollers
	DScroller* scroller;
	TThinkerIterator<DScroller> siterator;

	while ((scroller = siterator.Next()))
		scroller->Destroy();
}

void P_DestroyLightThinkers()
{
	// Destroy fireflicker
	DFireFlicker* fireflicker;
	TThinkerIterator<DFireFlicker> ffliterator;

	while ((fireflicker = ffliterator.Next()))
		fireflicker->Destroy();

	// Destroy flicker
	DFlicker* flicker;
	TThinkerIterator<DFlicker> fliterator;

	while ((flicker = fliterator.Next()))
		flicker->Destroy();

	// Destroy lightflash
	DLightFlash* flash;
	TThinkerIterator<DLightFlash> flashiterator;

	while ((flash = flashiterator.Next()))
		flash->Destroy();

	// Destroy strobe
	DStrobe* strobe;
	TThinkerIterator<DStrobe> strobeiterator;

	while ((strobe = strobeiterator.Next()))
		strobe->Destroy();

	// Destroy glow
	DGlow* glow;
	TThinkerIterator<DGlow> glowiterator;

	while ((glow = glowiterator.Next()))
		glow->Destroy();

	// Destroy glow2
	DGlow2* glow2;
	TThinkerIterator<DGlow2> glow2iterator;

	while ((glow2 = glow2iterator.Next()))
		glow2->Destroy();

	// Destroy phased
	DPhased* phased;
	TThinkerIterator<DPhased> phasediterator;

	while ((phased = phasediterator.Next()))
		phased->Destroy();
}

void P_SpawnPhasedLight(sector_t* sector, int base, int index)
{
	int i, b;

	if (index == -1)
	{ // sector->lightlevel as the index
		i = 63 - (sector->lightlevel & 63);
	}
	else
	{
		i = index;
	}

	b = base & 255;

	new DPhased(sector, b, i);

	P_ClearNonGeneralizedSectorSpecial(sector);
}

void P_SpawnLightSequence(sector_t* sector)
{
	new DPhased(sector);

	P_ClearNonGeneralizedSectorSpecial(sector);
}

void P_SpawnGlowingLight(sector_t* sector)
{
	new DGlow(sector);

	P_ClearNonGeneralizedSectorSpecial(sector);
}

void P_SpawnLightFlash(sector_t* sector)
{
	new DLightFlash(sector);

	P_ClearNonGeneralizedSectorSpecial(sector);
}

void P_SpawnFireFlicker(sector_t* sector)
{
	new DFireFlicker(sector);

	P_ClearNonGeneralizedSectorSpecial(sector);
}

void P_SpawnStrobeFlash(sector_t* sector, int utics, int ltics, bool inSync)
{
	new DStrobe(sector, utics, ltics, inSync);

	P_ClearNonGeneralizedSectorSpecial(sector);
}

static void P_SpawnExtra()
{
	for (int i = 0; i < numlines; i++)
	{
		map_format.spawn_extra(i);
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
		fixed_t height = sector->ceilingheight + sector->floorheight;

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
		sector_t *sec;
		fixed_t height, waterheight;	// killough 4/4/98: add waterheight
		msecnode_t *node;
		AActor *thing;

		case sc_side:				// killough 3/7/98: Scroll wall texture
			sides[m_Affectee].textureoffset += dx;
			sides[m_Affectee].rowoffset += dy;
			break;

		case sc_floor:				// killough 3/7/98: Scroll floor texture
			sectors[m_Affectee].floor_xoffs += dx;
			sectors[m_Affectee].floor_yoffs += dy;
			break;

		case sc_ceiling:			// killough 3/7/98: Scroll ceiling texture
			sectors[m_Affectee].ceiling_xoffs += dx;
			sectors[m_Affectee].ceiling_yoffs += dy;
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
		fixed_t height = sector->ceilingheight + sector->floorheight;

		m_LastHeight = height;
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
		fixed_t height = sector->ceilingheight + sector->floorheight;

		m_LastHeight = height;
	}
	m_Affectee = *l->sidenum;
}

// Initialize the scrollers
static void P_SpawnScrollers(void)
{
	int i;
	line_t *l = lines;

	for (i = 0; i < numlines; i++, l++)
	{
		map_format.spawn_scroller(l, i);
	}
}

fixed_t P_ArgToSpeed(byte arg)
{
	return (fixed_t)arg * FRACUNIT / 8;
}

bool P_ArgToCrushType(byte arg)
{
	return arg == 2;
}

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
		map_format.spawn_friction(l);
	}
}

void P_ApplySectorFriction(int tag, int value, bool use_thinker)
{
	int friction, movefactor, s;

	friction = (0x1EB8 * value) / 0x80 + 0xD000;

	// The following check might seem odd. At the time of movement,
	// the move distance is multiplied by 'friction/0x10000', so a
	// higher friction value actually means 'less friction'.

	if (friction > ORIG_FRICTION) // ice
		movefactor = ((0x10092 - friction) * (0x70)) / 0x158;
	else
		movefactor = ((friction - 0xDB34) * (0xA)) / 0x80;


	// killough 8/28/98: prevent odd situations
	if (friction > FRACUNIT)
		friction = FRACUNIT;
	if (friction < 0)
		friction = 0;
	if (movefactor < 32)
		movefactor = 32;

	for (s = -1; (s = P_FindSectorFromTag(tag, s)) >= 0;)
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

		// e6y: boom's friction code for boom compatibility
		//if (use_thinker)
		//	new DFriction(friction, movefactor, s);

		sectors[s].friction = friction;
		sectors[s].movefactor = movefactor;
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

	if (!(sec->flags & SECF_PUSH))
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

	for (i = 0; i < numlines; i++, l++)
	{
		map_format.spawn_pusher(l);
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
