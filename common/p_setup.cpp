// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 2006-2013 by The Odamex Team.
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
//		Do all the WAD I/O, get map description,
//		set up initial state and misc. LUTs.
//
//-----------------------------------------------------------------------------


#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <set>

#include "m_alloc.h"
#include "vectors.h"
#include "m_argv.h"
#include "z_zone.h"
#include "m_swap.h"
#include "m_bbox.h"
#include "g_game.h"
#include "i_system.h"
#include "w_wad.h"
#include "doomdef.h"
#include "p_local.h"
#include "p_acs.h"
#include "s_sound.h"
#include "doomstat.h"
#include "p_lnspec.h"
#include "v_palette.h"
#include "c_console.h"

#include "p_setup.h"

void SV_PreservePlayer(player_t &player);
void P_SpawnMapThing (mapthing2_t *mthing, int position);

void P_TranslateLineDef (line_t *ld, maplinedef_t *mld);
void P_TranslateTeleportThings (void);
int	P_TranslateSectorSpecial (int);

static void P_SetupLevelFloorPlane(sector_t *sector);
static void P_SetupLevelCeilingPlane(sector_t *sector);
static void P_SetupSlopes();
void P_InvertPlane(plane_t *plane);

extern unsigned int R_OldBlend;

//
// MAP related Lookup tables.
// Store VERTEXES, LINEDEFS, SIDEDEFS, etc.
//
int 			numvertexes;
vertex_t*		vertexes;

int 			numsegs;
seg_t*			segs;

int 			numsectors;
sector_t*		sectors;

int 			numsubsectors;
subsector_t*	subsectors;

int 			numnodes;
node_t* 		nodes;

int 			numlines;
line_t* 		lines;

int 			numsides;
side_t* 		sides;

// [RH] Set true if the map contains a BEHAVIOR lump
bool			HasBehavior = false;

// BLOCKMAP
// Created from axis aligned bounding box
// of the map, a rectangular array of
// blocks of size ...
// Used to speed up collision detection
// by spatial subdivision in 2D.
//
// Blockmap size.
int 			bmapwidth;
int 			bmapheight; 	// size in mapblocks

int				*blockmap;		// int for larger maps ([RH] Made int because BOOM does)
int				*blockmaplump;	// offsets in blockmap are from here

fixed_t 		bmaporgx;		// origin of block map
fixed_t 		bmaporgy;

AActor**		blocklinks;		// for thing chains



// REJECT
// For fast sight rejection.
// Speeds up enemy AI by skipping detailed
//	LineOf Sight calculation.
// Without special effect, this could be
//	used as a PVS lookup as well.
//
byte*			rejectmatrix;
BOOL			rejectempty;


// Maintain single and multi player starting spots.
int				MaxDeathmatchStarts;
mapthing2_t		*deathmatchstarts;
mapthing2_t		*deathmatch_p;

std::vector<mapthing2_t> playerstarts;
std::vector<mapthing2_t> voodoostarts;

//	[Toke - CTF - starts] Teamplay starts
size_t			MaxBlueTeamStarts;
size_t			MaxRedTeamStarts;

mapthing2_t		*blueteamstarts;
mapthing2_t		*redteamstarts;

mapthing2_t		*blueteam_p;
mapthing2_t		*redteam_p;

//
// P_LoadVertexes
//
void P_LoadVertexes (int lump)
{
	byte *data;
	int i;

	// Determine number of vertices:
	//	total lump length / vertex record length.
	numvertexes = W_LumpLength (lump) / sizeof(mapvertex_t);

	// Allocate zone memory for buffer.
	vertexes = (vertex_t *)Z_Malloc (numvertexes*sizeof(vertex_t), PU_LEVEL, 0);

	// Load data into cache.
	data = (byte *)W_CacheLumpNum (lump, PU_STATIC);

	// Copy and convert vertex coordinates,
	// internal representation as fixed.
	for (i = 0; i < numvertexes; i++)
	{
		vertexes[i].x = LESHORT(((mapvertex_t *)data)[i].x)<<FRACBITS;
		vertexes[i].y = LESHORT(((mapvertex_t *)data)[i].y)<<FRACBITS;
	}

	// Free buffer memory.
	Z_Free (data);
}



//
// P_LoadSegs
//
// killough 5/3/98: reformatted, cleaned up

void P_LoadSegs (int lump)
{
	int  i;
	byte *data;
	byte *vertchanged = (byte *)Z_Malloc (numvertexes,PU_LEVEL,0);	// phares 10/4/98
	line_t* line;		// phares 10/4/98
   // SoM: Changed variables to the correct types.
	angle_t ptp_angle;		// phares 10/4/98
	angle_t delta_angle;	// phares 10/4/98
	fixed_t dis;			// phares 10/4/98
	fixed_t dx,dy;			// phares 10/4/98
	int vnum1,vnum2;	// phares 10/4/98

	memset (vertchanged,0,numvertexes); // phares 10/4/98

	numsegs = W_LumpLength (lump) / sizeof(mapseg_t);
	segs = (seg_t *)Z_Malloc (numsegs*sizeof(seg_t), PU_LEVEL, 0);
	memset (segs, 0, numsegs*sizeof(seg_t));
	data = (byte *)W_CacheLumpNum (lump, PU_STATIC);

	// phares: 10/4/98: Vertchanged is an array that represents the vertices.
	// Mark those used by linedefs. A marked vertex is one that is not a
	// candidate for movement further down.

	line = lines;
	for (i = 0; i < numlines ; i++, line++)
	{
		vertchanged[line->v1 - vertexes] = vertchanged[line->v2 - vertexes] = 1;
	}

	for (i = 0; i < numsegs; i++)
	{
		seg_t *li = segs+i;
		mapseg_t *ml = (mapseg_t *) data + i;

		int side, linedef;
		line_t *ldef;

		short v = LESHORT(ml->v1);

		if(v < 0 || v >= numvertexes)
			I_Error("P_LoadSegs: invalid vertex %d", v);
		else
			li->v1 = &vertexes[v];

		v = LESHORT(ml->v2);

		if(v < 0 || v >= numvertexes)
			I_Error("P_LoadSegs: invalid vertex %d", v);
		else
			li->v2 = &vertexes[v];

		li->angle = (LESHORT(ml->angle))<<16;

// phares 10/4/98: In the case of a lineseg that was created by splitting
// another line, it appears that the line angle is inherited from the
// father line. Due to roundoff, the new vertex may have been placed 'off
// the line'. When you get close to such a line, and it is very short,
// it's possible that the roundoff error causes 'firelines', the thin
// lines that can draw from screen top to screen bottom occasionally. This
// is due to all the angle calculations that are done based on the line
// angle, the angles from the viewer to the vertices, and the viewer's
// angle in the world. In the case of firelines, the rounded-off position
// of one of the vertices determines one of these angles, and introduces
// an error in the scaling factor for mapping textures and determining
// where on the screen the ceiling and floor spans should be shown. For a
// fireline, the engine thinks the ceiling bottom and floor top are at the
// midpoint of the screen. So you get ceilings drawn all the way down to the
// screen midpoint, and floors drawn all the way up. Thus 'firelines'. The
// name comes from the original sighting, which involved a fire texture.
//
// To correct this, reset the vertex that was added so that it sits ON the
// split line.
//
// To know which of the two vertices was added, its number is greater than
// that of the last of the author-created vertices. If both vertices of the
// line were added by splitting, pick the higher-numbered one. Once you've
// changed a vertex, don't change it again if it shows up in another seg.
//
// To determine if there's an error in the first place, find the
// angle of the line between the two seg vertices. If it's one degree or more
// off, then move one vertex. This may seem insignificant, but one degree
// errors _can_ cause firelines.

		ptp_angle = R_PointToAngle2(li->v1->x,li->v1->y,li->v2->x,li->v2->y);
		dis = 0;
		delta_angle = ((ptp_angle-li->angle) >> ANGLETOFINESHIFT) * 360 / 8192;

		if (delta_angle != 0)
		{
			dx = (li->v1->x - li->v2->x)>>FRACBITS;
			dy = (li->v1->y - li->v2->y)>>FRACBITS;
			dis = ((int)sqrt((float)dx*(float)dx + (float)dy*(float)dy))<<FRACBITS;
			dx = finecosine[li->angle>>ANGLETOFINESHIFT];
			dy = finesine[li->angle>>ANGLETOFINESHIFT];
			vnum1 = li->v1 - vertexes;
			vnum2 = li->v2 - vertexes;
			if ((vnum2 > vnum1) && (vertchanged[vnum2] == 0))
			{
				li->v2->x = li->v1->x + FixedMul(dis,dx);
				li->v2->y = li->v1->y + FixedMul(dis,dy);
				vertchanged[vnum2] = 1; // this was changed
			}
			else if (vertchanged[vnum1] == 0)
			{
				li->v1->x = li->v2->x - FixedMul(dis,dx);
				li->v1->y = li->v2->y - FixedMul(dis,dy);
				vertchanged[vnum1] = 1; // this was changed
			}
		}

		li->offset = (LESHORT(ml->offset))<<16;
		linedef = LESHORT(ml->linedef);

		if(linedef < 0 || linedef >= numlines)
			I_Error("P_LoadSegs: invalid linedef %d", linedef);

		ldef = &lines[linedef];
		li->linedef = ldef;

		side = LESHORT(ml->side);

		if (side != 0 && side != 1)
			side = 1;	// assume invalid value means back

		li->sidedef = &sides[ldef->sidenum[side]];
		li->frontsector = sides[ldef->sidenum[side]].sector;

		// killough 5/3/98: ignore 2s flag if second sidedef missing:
		if (ldef->flags & ML_TWOSIDED && ldef->sidenum[side^1]!=R_NOSIDE)
			li->backsector = sides[ldef->sidenum[side^1]].sector;
		else
		{
			li->backsector = 0;
			ldef->flags &= ~ML_TWOSIDED;
		}
	
		// recalculate seg offsets. values in wads are untrustworthy.
		vertex_t *from = (side == 0)
			? ldef->v1			// right side: offset is from start of linedef
			: ldef->v2;			// left side: offset is from end of linedef
		vertex_t *to = li->v1;	// end point is start of seg, in both cases

		float dx = FIXED2FLOAT(to->x - from->x);
		float dy = FIXED2FLOAT(to->y - from->y);
		li->offset = FLOAT2FIXED(sqrt(dx * dx + dy * dy));

		dx = FIXED2FLOAT(li->v2->x - li->v1->x);
		dy = FIXED2FLOAT(li->v2->y - li->v1->y);
		li->length = FLOAT2FIXED(sqrt(dx * dx + dy* dy));
	}

	Z_Free (data);
	Z_Free(vertchanged); // phares 10/4/98
}


//
// P_LoadSubsectors
//
void P_LoadSubsectors (int lump)
{
	byte *data;
	int i;

	numsubsectors = W_LumpLength (lump) / sizeof(mapsubsector_t);
	subsectors = (subsector_t *)Z_Malloc (numsubsectors*sizeof(subsector_t),PU_LEVEL,0);
	data = (byte *)W_CacheLumpNum (lump,PU_STATIC);

	memset (subsectors, 0, numsubsectors*sizeof(subsector_t));

	for (i = 0; i < numsubsectors; i++)
	{
		subsectors[i].numlines = LESHORT(((mapsubsector_t *)data)[i].numsegs);
		subsectors[i].firstline = LESHORT(((mapsubsector_t *)data)[i].firstseg);
	}

	Z_Free (data);
}



//
// P_LoadSectors
//
void P_LoadSectors (int lump)
{
	byte*				data;
	int 				i;
	mapsector_t*		ms;
	sector_t*			ss;
	int					defSeqType;

	// denis - properly destroy sectors so that smart pointers they contain don't get screwed
	delete[] sectors;

	numsectors = W_LumpLength (lump) / sizeof(mapsector_t);

	// denis - properly construct sectors so that smart pointers they contain don't get screwed
	sectors = new sector_t[numsectors];
	memset(sectors, 0, sizeof(sector_t)*numsectors);

	data = (byte *)W_CacheLumpNum (lump, PU_STATIC);

	if (level.flags & LEVEL_SNDSEQTOTALCTRL)
		defSeqType = 0;
	else
		defSeqType = -1;

	ms = (mapsector_t *)data;
	ss = sectors;
	for (i = 0; i < numsectors; i++, ss++, ms++)
	{
		ss->floorheight = LESHORT(ms->floorheight)<<FRACBITS;
		ss->ceilingheight = LESHORT(ms->ceilingheight)<<FRACBITS;
		ss->floorpic = (short)R_FlatNumForName(ms->floorpic);
		ss->ceilingpic = (short)R_FlatNumForName(ms->ceilingpic);
		ss->lightlevel = LESHORT(ms->lightlevel);
		if (HasBehavior)
			ss->special = LESHORT(ms->special);
		else	// [RH] Translate to new sector special
			ss->special = P_TranslateSectorSpecial (LESHORT(ms->special));
		ss->tag = LESHORT(ms->tag);
		ss->thinglist = NULL;
		ss->touching_thinglist = NULL;		// phares 3/14/98
		ss->seqType = defSeqType;
		ss->nextsec = -1;	//jff 2/26/98 add fields to support locking out
		ss->prevsec = -1;	// stair retriggering until build completes

		// killough 3/7/98:
		ss->floor_xoffs = 0;
		ss->floor_yoffs = 0;	// floor and ceiling flats offsets
		ss->ceiling_xoffs = 0;
		ss->ceiling_yoffs = 0;

		ss->floor_xscale = FRACUNIT;	// [RH] floor and ceiling scaling
		ss->floor_yscale = FRACUNIT;
		ss->ceiling_xscale = FRACUNIT;
		ss->ceiling_yscale = FRACUNIT;

		ss->floor_angle = 0;	// [RH] floor and ceiling rotation
		ss->ceiling_angle = 0;

		ss->base_ceiling_angle = ss->base_ceiling_yoffs =
			ss->base_floor_angle = ss->base_floor_yoffs = 0;

		ss->heightsec = NULL;	// sector used to get floor and ceiling height
		ss->floorlightsec = NULL;	// sector used to get floor lighting
		// killough 3/7/98: end changes

		// killough 4/11/98 sector used to get ceiling lighting:
		ss->ceilinglightsec = NULL;

		// [SL] 2012-01-17 - init the sector's floor and ceiling planes
		// as level planes (constant value of z for all points)
		// Slopes will be setup later
		P_SetupLevelFloorPlane(ss);
		P_SetupLevelCeilingPlane(ss);
		
		ss->gravity = 1.0f;	// [RH] Default sector gravity of 1.0

		// [RH] Sectors default to white light with the default fade.
		//		If they are outside (have a sky ceiling), they use the outside fog.
		if (level.outsidefog != 0xff000000 && ss->ceilingpic == skyflatnum)
			ss->ceilingcolormap = ss->floorcolormap = GetSpecialLights (255,255,255,
				RPART(level.outsidefog),GPART(level.outsidefog),BPART(level.outsidefog));
		else
			ss->ceilingcolormap = ss->floorcolormap = GetSpecialLights (255,255,255,
				RPART(level.fadeto),GPART(level.fadeto),BPART(level.fadeto));

		ss->sky = 0;

		// killough 8/28/98: initialize all sectors to normal friction
		ss->friction = ORIG_FRICTION;
		ss->movefactor = ORIG_FRICTION_FACTOR;
	}

	Z_Free (data);
}


//
// P_LoadNodes
//
void P_LoadNodes (int lump)
{
	byte*		data;
	int 		i;
	int 		j;
	int 		k;
	mapnode_t*	mn;
	node_t* 	no;

	numnodes = W_LumpLength (lump) / sizeof(mapnode_t);
	nodes = (node_t *)Z_Malloc (numnodes*sizeof(node_t), PU_LEVEL, 0);
	data = (byte *)W_CacheLumpNum (lump, PU_STATIC);

	mn = (mapnode_t *)data;
	no = nodes;

	for (i = 0; i < numnodes; i++, no++, mn++)
	{
		no->x = LESHORT(mn->x)<<FRACBITS;
		no->y = LESHORT(mn->y)<<FRACBITS;
		no->dx = LESHORT(mn->dx)<<FRACBITS;
		no->dy = LESHORT(mn->dy)<<FRACBITS;
		for (j = 0; j < 2; j++)
		{
			no->children[j] = LESHORT(mn->children[j]);
			for (k = 0; k < 4; k++)
				no->bbox[j][k] = LESHORT(mn->bbox[j][k])<<FRACBITS;
		}
	}

	Z_Free (data);
}

//
// P_LoadThings
//
void P_LoadThings (int lump)
{
	mapthing2_t mt2;		// [RH] for translation
	byte *data = (byte *)W_CacheLumpNum (lump, PU_STATIC);
	mapthing_t *mt = (mapthing_t *)data;
	mapthing_t *lastmt = (mapthing_t *)(data + W_LumpLength (lump));

	playerstarts.clear();
	voodoostarts.clear();

	// [RH] ZDoom now uses Hexen-style maps as its native format. // denis - growwwwl
	//		Since this is the only place where Doom-style Things are ever
	//		referenced, we translate them into a Hexen-style thing.
	memset (&mt2, 0, sizeof(mt2));

	for ( ; mt < lastmt; mt++)
	{
		// [RH] At this point, monsters unique to Doom II were weeded out
		//		if the IWAD wasn't for Doom II. R_SpawnMapThing() can now
		//		handle these and more cases better, so we just pass it
		//		everything and let it decide what to do with them.

		// [RH] Need to translate the spawn flags to Hexen format.
		short flags = LESHORT(mt->options);
		mt2.flags = (short)((flags & 0xf) | 0x7e0);
		if (flags & BTF_NOTSINGLE)			mt2.flags &= ~MTF_SINGLE;
		if (flags & BTF_NOTDEATHMATCH)		mt2.flags &= ~MTF_DEATHMATCH;
		if (flags & BTF_NOTCOOPERATIVE)		mt2.flags &= ~MTF_COOPERATIVE;

		mt2.x = LESHORT(mt->x);
		mt2.y = LESHORT(mt->y);
		mt2.angle = LESHORT(mt->angle);
		mt2.type = LESHORT(mt->type);

		P_SpawnMapThing (&mt2, 0);
	}

	Z_Free (data);
}

// [RH]
// P_LoadThings2
//
// Same as P_LoadThings() except it assumes Things are
// saved Hexen-style. Position also controls which single-
// player start spots are spawned by filtering out those
// whose first parameter don't match position.
//
void P_LoadThings2 (int lump, int position)
{
	byte *data = (byte *)W_CacheLumpNum (lump, PU_STATIC);
	mapthing2_t *mt = (mapthing2_t *)data;
	mapthing2_t *lastmt = (mapthing2_t *)(data + W_LumpLength (lump));

	playerstarts.clear();
	voodoostarts.clear();

	for ( ; mt < lastmt; mt++)
	{
		// [RH] At this point, monsters unique to Doom II were weeded out
		//		if the IWAD wasn't for Doom II. R_SpawnMapThing() can now
		//		handle these and more cases better, so we just pass it
		//		everything and let it decide what to do with them.

		mt->thingid = LESHORT(mt->thingid);
		mt->x = LESHORT(mt->x);
		mt->y = LESHORT(mt->y);
		mt->z = LESHORT(mt->z);
		mt->angle = LESHORT(mt->angle);
		mt->type = LESHORT(mt->type);
		mt->flags = LESHORT(mt->flags);

		P_SpawnMapThing (mt, position);
	}

	Z_Free (data);
}

//
// P_LoadLineDefs
//
// killough 4/4/98: split into two functions, to allow sidedef overloading
//
// [RH] Actually split into four functions to allow for Hexen and Doom
//		linedefs.
void P_AdjustLine (line_t *ld)
{
	vertex_t *v1, *v2;

	ld->lucency = 255;	// [RH] Opaque by default

	v1 = ld->v1;
	v2 = ld->v2;

	ld->dx = v2->x - v1->x;
	ld->dy = v2->y - v1->y;

	if (ld->dx == 0)
		ld->slopetype = ST_VERTICAL;
	else if (ld->dy == 0)
		ld->slopetype = ST_HORIZONTAL;
	else
		ld->slopetype = (FixedDiv (ld->dy , ld->dx) > 0) ? ST_POSITIVE : ST_NEGATIVE;

	if (v1->x < v2->x)
	{
		ld->bbox[BOXLEFT] = v1->x;
		ld->bbox[BOXRIGHT] = v2->x;
	}
	else
	{
		ld->bbox[BOXLEFT] = v2->x;
		ld->bbox[BOXRIGHT] = v1->x;
	}

	if (v1->y < v2->y)
	{
		ld->bbox[BOXBOTTOM] = v1->y;
		ld->bbox[BOXTOP] = v2->y;
	}
	else
	{
		ld->bbox[BOXBOTTOM] = v2->y;
		ld->bbox[BOXTOP] = v1->y;
	}

	// [RH] Set line id (as appropriate) here
	if (ld->special == Line_SetIdentification ||
		ld->special == Teleport_Line ||
		ld->special == TranslucentLine ||
		ld->special == Scroll_Texture_Model) {
		ld->id = ld->args[0];
	}

	// denis - prevent buffer overrun
	if(*ld->sidenum == R_NOSIDE)
		return;

	// killough 4/4/98: support special sidedef interpretation below
	if (// [RH] Save Static_Init only if it's interested in the textures
		( (ld->special == Static_Init && ld->args[1] == Init_Color)
			|| ld->special != Static_Init) ) {
			sides[*ld->sidenum].special = ld->special;
			sides[*ld->sidenum].tag = ld->args[0];
		}
	else
		sides[*ld->sidenum].special = 0;
}

// killough 4/4/98: delay using sidedefs until they are loaded
void P_FinishLoadingLineDefs (void)
{
	int i, linenum;
	register line_t *ld = lines;

	for (i = numlines, linenum = 0; i--; ld++, linenum++)
	{
		ld->frontsector = ld->sidenum[0]!=R_NOSIDE ? sides[ld->sidenum[0]].sector : 0;
		ld->backsector  = ld->sidenum[1]!=R_NOSIDE ? sides[ld->sidenum[1]].sector : 0;
		if (ld->sidenum[0] != R_NOSIDE)
			sides[ld->sidenum[0]].linenum = linenum;
		if (ld->sidenum[1] != R_NOSIDE)
			sides[ld->sidenum[1]].linenum = linenum;

		switch (ld->special)
		{						// killough 4/11/98: handle special types
			int j;

			case TranslucentLine:			// killough 4/11/98: translucent 2s textures
#if 0
				lump = sides[*ld->sidenum].special;		// translucency from sidedef
				if (!ld->tag)							// if tag==0,
					ld->tranlump = lump;				// affect this linedef only
				else
					for (j=0;j<numlines;j++)			// if tag!=0,
						if (lines[j].tag == ld->tag)	// affect all matching linedefs
							lines[j].tranlump = lump;
#else
				// [RH] Second arg controls how opaque it is.
				if (!ld->args[0])
					ld->lucency = (byte)ld->args[1];
				else
					for (j = 0; j < numlines; j++)
						if (lines[j].id == ld->args[0])
							lines[j].lucency = (byte)ld->args[1];
#endif
				break;
		}
	}
}

void P_LoadLineDefs (int lump)
{
	byte *data;
	int i;
	line_t *ld;

	numlines = W_LumpLength (lump) / sizeof(maplinedef_t);
	lines = (line_t *)Z_Malloc (numlines*sizeof(line_t), PU_LEVEL, 0);
	memset (lines, 0, numlines*sizeof(line_t));
	data = (byte *)W_CacheLumpNum (lump, PU_STATIC);

	ld = lines;
	for (i=0 ; i<numlines ; i++, ld++)
	{
		maplinedef_t *mld = ((maplinedef_t *)data) + i;

		// [RH] Translate old linedef special and flags to be
		//		compatible with the new format.
		P_TranslateLineDef (ld, mld);

		short v = LESHORT(mld->v1);

		if(v < 0 || v >= numvertexes)
			I_Error("P_LoadLineDefs: invalid vertex %d", v);
		else
			ld->v1 = &vertexes[v];

		v = LESHORT(mld->v2);

		if(v < 0 || v >= numvertexes)
			I_Error("P_LoadLineDefs: invalid vertex %d", v);
		else
			ld->v2 = &vertexes[v];

		ld->sidenum[0] = LESHORT(mld->sidenum[0]);
		ld->sidenum[1] = LESHORT(mld->sidenum[1]);

		if(ld->sidenum[0] >= numsides)
			ld->sidenum[0] = R_NOSIDE;
		if(ld->sidenum[1] >= numsides)
			ld->sidenum[1] = R_NOSIDE;

		P_AdjustLine (ld);
	}

	Z_Free (data);
}

// [RH] Same as P_LoadLineDefs() except it uses Hexen-style LineDefs.
void P_LoadLineDefs2 (int lump)
{
	byte*				data;
	int 				i;
	maplinedef2_t*		mld;
	line_t* 			ld;

	numlines = W_LumpLength (lump) / sizeof(maplinedef2_t);
	lines = (line_t *)Z_Malloc (numlines*sizeof(line_t), PU_LEVEL,0 );
	memset (lines, 0, numlines*sizeof(line_t));
	data = (byte *)W_CacheLumpNum (lump, PU_STATIC);

	mld = (maplinedef2_t *)data;
	ld = lines;
	for (i = 0; i < numlines; i++, mld++, ld++)
	{
		int j;

		for (j = 0; j < 5; j++)
			ld->args[j] = mld->args[j];

		ld->flags = LESHORT(mld->flags);
		ld->special = mld->special;

		short v = LESHORT(mld->v1);

		if(v < 0 || v >= numvertexes)
			I_Error("P_LoadLineDefs: invalid vertex %d", v);
		else
			ld->v1 = &vertexes[v];

		v = LESHORT(mld->v2);

		if(v < 0 || v >= numvertexes)
			I_Error("P_LoadLineDefs: invalid vertex %d", v);
		else
			ld->v2 = &vertexes[v];

		ld->sidenum[0] = LESHORT(mld->sidenum[0]);
		ld->sidenum[1] = LESHORT(mld->sidenum[1]);

		if(ld->sidenum[0] >= numsides)
			ld->sidenum[0] = R_NOSIDE;
		if(ld->sidenum[1] >= numsides)
			ld->sidenum[1] = R_NOSIDE;

		P_AdjustLine (ld);
	}

	Z_Free (data);
}

//
// P_LoadSideDefs
//
// killough 4/4/98: split into two functions
void P_LoadSideDefs (int lump)
{
	numsides = W_LumpLength (lump) / sizeof(mapsidedef_t);
	sides = (side_t *)Z_Malloc (numsides*sizeof(side_t), PU_LEVEL, 0);
	memset (sides, 0, numsides*sizeof(side_t));
}

// [RH] Figure out blends for deep water sectors
static void SetTexture (short *texture, unsigned int *blend, char *name)
{
	if ((*blend = R_ColormapNumForName (name)) == 0) {
		if ((*texture = R_CheckTextureNumForName (name)) == -1) {
			char name2[9];
			char *stop;
			strncpy (name2, name, 8);
			name2[8] = 0;
			*blend = strtoul (name2, &stop, 16);
			*texture = 0;
		} else {
			*blend = 0;
		}
	} else {
		*texture = 0;
	}
}

static void SetTextureNoErr (short *texture, unsigned int *color, char *name)
{
	if ((*texture = R_CheckTextureNumForName (name)) == -1) {
		char name2[9];
		char *stop;
		strncpy (name2, name, 8);
		name2[8] = 0;
		*color = strtoul (name2, &stop, 16);
		*texture = 0;
	}
}

// killough 4/4/98: delay using texture names until
// after linedefs are loaded, to allow overloading.
// killough 5/3/98: reformatted, cleaned up

void P_LoadSideDefs2 (int lump)
{
	byte *data = (byte *)W_CacheLumpNum(lump,PU_STATIC);
	int  i;

	for (i=0; i<numsides; i++)
	{
		register mapsidedef_t *msd = (mapsidedef_t *) data + i;
		register side_t *sd = sides + i;
		register sector_t *sec;

		sd->textureoffset = LESHORT(msd->textureoffset)<<FRACBITS;
		sd->rowoffset = LESHORT(msd->rowoffset)<<FRACBITS;
		sd->linenum = -1;

		// killough 4/4/98: allow sidedef texture names to be overloaded
		// killough 4/11/98: refined to allow colormaps to work as wall
		// textures if invalid as colormaps but valid as textures.

		sd->sector = sec = &sectors[LESHORT(msd->sector)];
		switch (sd->special)
		{
		  case Transfer_Heights:	// variable colormap via 242 linedef
			  // [RH] The colormap num we get here isn't really a colormap,
			  //	  but a packed ARGB word for blending, so we also allow
			  //	  the blend to be specified directly by the texture names
			  //	  instead of figuring something out from the colormap.
			SetTexture (&sd->bottomtexture, &sec->bottommap, msd->bottomtexture);
			SetTexture (&sd->midtexture, &sec->midmap, msd->midtexture);
			SetTexture (&sd->toptexture, &sec->topmap, msd->toptexture);
			break;

		  case Static_Init:
			// [RH] Set sector color and fog
			// upper "texture" is light color
			// lower "texture" is fog color
			{
				unsigned int color = 0xffffff, fog = 0x000000;

				SetTextureNoErr (&sd->bottomtexture, &fog, msd->bottomtexture);
				SetTextureNoErr (&sd->toptexture, &color, msd->toptexture);
				sd->midtexture = R_TextureNumForName (msd->midtexture);

				if (fog != 0x000000 || color != 0xffffff) {
					int s;
					dyncolormap_t *colormap = GetSpecialLights
						(RPART(color),	GPART(color),	BPART(color),
						 RPART(fog),	GPART(fog),		BPART(fog));

					for (s = 0; s < numsectors; s++) {
						if (sectors[s].tag == sd->tag)
							sectors[s].ceilingcolormap =
								sectors[s].floorcolormap = colormap;
					}
				}
			}
			break;

/*
		  case TranslucentLine:	// killough 4/11/98: apply translucency to 2s normal texture
			sd->midtexture = strncasecmp("TRANMAP", msd->midtexture, 8) ?
				(sd->special = W_CheckNumForName(msd->midtexture)) < 0 ||
				W_LumpLength(sd->special) != 65536 ?
				sd->special=0, R_TextureNumForName(msd->midtexture) :
					(sd->special++, 0) : (sd->special=0);
			sd->toptexture = R_TextureNumForName(msd->toptexture);
			sd->bottomtexture = R_TextureNumForName(msd->bottomtexture);
			break;
*/
		  default:			// normal cases
			sd->midtexture = R_TextureNumForName(msd->midtexture);
			sd->toptexture = R_TextureNumForName(msd->toptexture);
			sd->bottomtexture = R_TextureNumForName(msd->bottomtexture);
			break;
		}
	}
	Z_Free (data);
}


//
// jff 10/6/98
// New code added to speed up calculation of internal blockmap
// Algorithm is order of nlines*(ncols+nrows) not nlines*ncols*nrows
//

#define blkshift 7               /* places to shift rel position for cell num */
#define blkmask ((1<<blkshift)-1)/* mask for rel position within cell */
#define blkmargin 0              /* size guardband around map used */
                                 // jff 10/8/98 use guardband>0
                                 // jff 10/12/98 0 ok with + 1 in rows,cols

typedef struct linelist_t        // type used to list lines in each block
{
	int		num;
	struct	linelist_t *next;
} linelist_t;

//
// Subroutine to add a line number to a block list
// It simply returns if the line is already in the block
//

static void AddBlockLine
(
	linelist_t **lists,
	int *count,
	int *done,
	int blockno,
	DWORD lineno
)
{
	linelist_t *l;

	if (done[blockno])
		return;

	l = new linelist_t;
	l->num = lineno;
	l->next = lists[blockno];
	lists[blockno] = l;
	count[blockno]++;
	done[blockno] = 1;
}

//
// Actually construct the blockmap lump from the level data
//
// This finds the intersection of each linedef with the column and
// row lines at the left and bottom of each blockmap cell. It then
// adds the line to all block lists touching the intersection.
//

void P_CreateBlockMap()
{
	int xorg,yorg;					// blockmap origin (lower left)
	int nrows,ncols;				// blockmap dimensions
	linelist_t **blocklists=NULL;	// array of pointers to lists of lines
	int *blockcount=NULL;			// array of counters of line lists
	int *blockdone=NULL;			// array keeping track of blocks/line
	int NBlocks;					// number of cells = nrows*ncols
	DWORD linetotal=0;				// total length of all blocklists
	int i,j;
	int map_minx=MAXINT;			// init for map limits search
	int map_miny=MAXINT;
	int map_maxx=MININT;
	int map_maxy=MININT;

	// scan for map limits, which the blockmap must enclose

	for (i = 0; i < numvertexes; i++)
	{
		fixed_t t;

		if ((t=vertexes[i].x) < map_minx)
			map_minx = t;
		else if (t > map_maxx)
			map_maxx = t;
		if ((t=vertexes[i].y) < map_miny)
			map_miny = t;
		else if (t > map_maxy)
			map_maxy = t;
	}
	map_minx >>= FRACBITS;    // work in map coords, not fixed_t
	map_maxx >>= FRACBITS;
	map_miny >>= FRACBITS;
	map_maxy >>= FRACBITS;

	// set up blockmap area to enclose level plus margin

	xorg = map_minx-blkmargin;
	yorg = map_miny-blkmargin;
	ncols = (map_maxx+blkmargin-xorg+1+blkmask)>>blkshift;	//jff 10/12/98
	nrows = (map_maxy+blkmargin-yorg+1+blkmask)>>blkshift;	//+1 needed for
	NBlocks = ncols*nrows;									//map exactly 1 cell

	// create the array of pointers on NBlocks to blocklists
	// also create an array of linelist counts on NBlocks
	// finally make an array in which we can mark blocks done per line

	blocklists = new linelist_t *[NBlocks];
	memset (blocklists, 0, NBlocks*sizeof(linelist_t *));
	blockcount = new int[NBlocks];
	memset (blockcount, 0, NBlocks*sizeof(int));
	blockdone = new int[NBlocks];

	// initialize each blocklist, and enter the trailing -1 in all blocklists
	// note the linked list of lines grows backwards

	for (i = 0; i < NBlocks; i++)
	{
		blocklists[i] = new linelist_t;
		blocklists[i]->num = -1;
		blocklists[i]->next = NULL;
		blockcount[i]++;
	}

	// For each linedef in the wad, determine all blockmap blocks it touches,
	// and add the linedef number to the blocklists for those blocks

	for (i = 0; i < numlines; i++)
	{
		int x1 = lines[i].v1->x>>FRACBITS;		// lines[i] map coords
		int y1 = lines[i].v1->y>>FRACBITS;
		int x2 = lines[i].v2->x>>FRACBITS;
		int y2 = lines[i].v2->y>>FRACBITS;
		int dx = x2-x1;
		int dy = y2-y1;
		int vert = !dx;							// lines[i] slopetype
		int horiz = !dy;
		int spos = (dx^dy) > 0;
		int sneg = (dx^dy) < 0;
		int bx,by;								// block cell coords
		int minx = x1>x2? x2 : x1;				// extremal lines[i] coords
		int maxx = x1>x2? x1 : x2;
		int miny = y1>y2? y2 : y1;
		int maxy = y1>y2? y1 : y2;

		// no blocks done for this linedef yet

		memset (blockdone, 0, NBlocks*sizeof(int));

		// The line always belongs to the blocks containing its endpoints

		bx = (x1-xorg) >> blkshift;
		by = (y1-yorg) >> blkshift;
		AddBlockLine (blocklists, blockcount, blockdone, by*ncols+bx, i);
		bx = (x2-xorg) >> blkshift;
		by = (y2-yorg) >> blkshift;
		AddBlockLine (blocklists, blockcount, blockdone, by*ncols+bx, i);

		// For each column, see where the line along its left edge, which
		// it contains, intersects the Linedef i. Add i to each corresponding
		// blocklist.

		if (!vert)    // don't interesect vertical lines with columns
		{
			for (j=0;j<ncols;j++)
			{
				// intersection of Linedef with x=xorg+(j<<blkshift)
				// (y-y1)*dx = dy*(x-x1)
				// y = dy*(x-x1)+y1*dx;

				int x = xorg+(j<<blkshift);		// (x,y) is intersection
				int y = (dy*(x-x1))/dx+y1;
				int yb = (y-yorg)>>blkshift;	// block row number
				int yp = (y-yorg)&blkmask;		// y position within block

				if (yb<0 || yb>nrows-1)			// outside blockmap, continue
					continue;

				if (x<minx || x>maxx)			// line doesn't touch column
					continue;

				// The cell that contains the intersection point is always added

				AddBlockLine(blocklists,blockcount,blockdone,ncols*yb+j,i);

				// if the intersection is at a corner it depends on the slope
				// (and whether the line extends past the intersection) which
				// blocks are hit

				if (yp==0)			// intersection at a corner
				{
					if (sneg)		//   \ - blocks x,y-, x-,y
					{
						if (yb>0 && miny<y)
							AddBlockLine(blocklists, blockcount, blockdone, ncols*(yb-1)+j, i);
						if (j>0 && minx<x)
							AddBlockLine(blocklists, blockcount, blockdone, ncols*yb+j-1, i);
					}
					else if (spos)	//   / - block x-,y-
					{
						if (yb>0 && j>0 && minx<x)
							AddBlockLine(blocklists,blockcount,blockdone,ncols*(yb-1)+j-1,i);
					}
					else if (horiz)	//   - - block x-,y
					{
						if (j>0 && minx<x)
							AddBlockLine(blocklists,blockcount,blockdone,ncols*yb+j-1,i);
					}
				}
				else if (j>0 && minx<x)	// else not at corner: x-,y
					AddBlockLine(blocklists,blockcount,blockdone,ncols*yb+j-1,i);
			}
		}

		// For each row, see where the line along its bottom edge, which
		// it contains, intersects the Linedef i. Add i to all the corresponding
		// blocklists.

		if (!horiz)
		{
			for (j=0;j<nrows;j++)
			{
				// intersection of Linedef with y=yorg+(j<<blkshift)
				// (x,y) on Linedef i satisfies: (y-y1)*dx = dy*(x-x1)
				// x = dx*(y-y1)/dy+x1;

				int y = yorg+(j<<blkshift);		// (x,y) is intersection
				int x = (dx*(y-y1))/dy+x1;
				int xb = (x-xorg)>>blkshift;	// block column number
				int xp = (x-xorg)&blkmask;		// x position within block

				if (xb<0 || xb>ncols-1)			// outside blockmap, continue
					continue;

				if (y<miny || y>maxy)			 // line doesn't touch row
					continue;

				// The cell that contains the intersection point is always added

				AddBlockLine (blocklists, blockcount, blockdone, ncols*j+xb, i);

				// if the intersection is at a corner it depends on the slope
				// (and whether the line extends past the intersection) which
				// blocks are hit

				if (xp==0)			// intersection at a corner
				{
					if (sneg)       //   \ - blocks x,y-, x-,y
					{
						if (j>0 && miny<y)
							AddBlockLine (blocklists, blockcount, blockdone, ncols*(j-1)+xb, i);
						if (xb>0 && minx<x)
							AddBlockLine (blocklists, blockcount, blockdone, ncols*j+xb-1, i);
					}
					else if (vert)  //   | - block x,y-
					{
						if (j>0 && miny<y)
							AddBlockLine (blocklists, blockcount, blockdone, ncols*(j-1)+xb, i);
					}
					else if (spos)  //   / - block x-,y-
					{
						if (xb>0 && j>0 && miny<y)
							AddBlockLine (blocklists, blockcount, blockdone, ncols*(j-1)+xb-1, i);
					}
				}
				else if (j>0 && miny<y) // else not on a corner: x,y-
					AddBlockLine (blocklists, blockcount, blockdone, ncols*(j-1)+xb, i);
			}
		}
	}

	// Add initial 0 to all blocklists
	// count the total number of lines (and 0's and -1's)
	memset (blockdone, 0, NBlocks*sizeof(int));
	for (i = 0, linetotal = 0; i < NBlocks; i++)
	{
		AddBlockLine (blocklists, blockcount, blockdone, i, 0);
		linetotal += blockcount[i];
	}

	// Create the blockmap lump
	blockmaplump = (int *)Z_Malloc(sizeof(*blockmaplump) * (4+NBlocks+linetotal), PU_LEVEL, 0);

	// blockmap header
	//
	// Rjy: P_CreateBlockMap should not initialise bmaporg{x,y} as P_LoadBlockMap
	// does so again, resulting in their being left-shifted by FRACBITS twice.
	//
	// Thus any map having its blockmap built by the engine would have its
	// origin at (0,0) regardless of where the walls and monsters actually are,
	// breaking all collision detection.
	//
	// Instead have P_CreateBlockMap create blockmaplump only, so that both
	// clauses of the conditional in P_LoadBlockMap have the same effect, and
	// bmap* are only initialised from blockmaplump[0..3] once in the latter.
	//
	blockmaplump[0] = xorg;
	blockmaplump[1] = yorg;
	blockmaplump[2] = ncols;
	blockmaplump[3] = nrows;

	// offsets to lists and block lists
	for (i = 0; i < NBlocks; i++)
	{
		linelist_t *bl = blocklists[i];
		DWORD offs = blockmaplump[4+i] =   // set offset to block's list
			(i? blockmaplump[4+i-1] : 4+NBlocks) + (i? blockcount[i-1] : 0);

		// add the lines in each block's list to the blockmaplump
		// delete each list node as we go

		while (bl)
		{
			linelist_t *tmp = bl->next;
			blockmaplump[offs++] = bl->num;
			delete[] bl;
			bl = tmp;
		}
	}

	// free all temporary storage
	delete[] blocklists;
	delete[] blockcount;
	delete[] blockdone;
}

// jff 10/6/98
// End new code added to speed up calculation of internal blockmap

//
// P_LoadBlockMap
//
// [RH] Changed this some
//
void P_LoadBlockMap (int lump)
{
	int count;

	if (Args.CheckParm("-blockmap") || (count = W_LumpLength(lump)/2) >= 0x10000 || count < 4) 
		P_CreateBlockMap();
	else
	{
		short *wadblockmaplump = (short *)W_CacheLumpNum (lump, PU_LEVEL);
		int i;
		blockmaplump = (int *)Z_Malloc(sizeof(*blockmaplump) * count, PU_LEVEL, 0);

		// killough 3/1/98: Expand wad blockmap into larger internal one,
		// by treating all offsets except -1 as unsigned and zero-extending
		// them. This potentially doubles the size of blockmaps allowed,
		// because Doom originally considered the offsets as always signed.

		blockmaplump[0] = LESHORT(wadblockmaplump[0]);
		blockmaplump[1] = LESHORT(wadblockmaplump[1]);
		blockmaplump[2] = (DWORD)(LESHORT(wadblockmaplump[2])) & 0xffff;
		blockmaplump[3] = (DWORD)(LESHORT(wadblockmaplump[3])) & 0xffff;

		for (i=4 ; i<count ; i++)
		{
			short t = LESHORT(wadblockmaplump[i]);          // killough 3/1/98
			blockmaplump[i] = t == -1 ? (DWORD)0xffffffff : (DWORD) t & 0xffff;
		}

		Z_Free (wadblockmaplump);
	}

	bmaporgx = blockmaplump[0]<<FRACBITS;
	bmaporgy = blockmaplump[1]<<FRACBITS;
	bmapwidth = blockmaplump[2];
	bmapheight = blockmaplump[3];

	// clear out mobj chains
	count = sizeof(*blocklinks) * bmapwidth*bmapheight;
	blocklinks = (AActor **)Z_Malloc (count, PU_LEVEL, 0);
	memset (blocklinks, 0, count);
	blockmap = blockmaplump+4;
}



//
// P_GroupLines
// Builds sector line lists and subsector sector numbers.
// Finds block bounding boxes for sectors.
//
void P_GroupLines (void)
{
	line_t**			linebuffer;
	int 				i;
	int 				j;
	int 				total;
	line_t* 			li;
	sector_t*			sector;
	DBoundingBox		bbox;
	int 				block;

	// look up sector number for each subsector
	for (i = 0; i < numsubsectors; i++)
	{
		if(subsectors[i].firstline >= numsegs)
			I_Error("subsector[%d].firstline exceeds numsegs (%d)", i, numlines);
		subsectors[i].sector = segs[subsectors[i].firstline].sidedef->sector;
	}

	// count number of lines in each sector
	li = lines;
	total = 0;
	for (i = 0; i < numlines; i++, li++)
	{
		total++;
		if (!li->frontsector && li->backsector)
		{
			// swap front and backsectors if a one-sided linedef
			// does not have a front sector
			li->frontsector = li->backsector;
			li->backsector = NULL;
		}

        if (li->frontsector)
            li->frontsector->linecount++;

		if (li->backsector && li->backsector != li->frontsector)
		{
			li->backsector->linecount++;
			total++;
		}
	}

	// build line tables for each sector
	linebuffer = (line_t **)Z_Malloc (total*sizeof(line_t *), PU_LEVEL, 0);
	sector = sectors;
	for (i=0 ; i<numsectors ; i++, sector++)
	{
		bbox.ClearBox ();
		sector->lines = linebuffer;
		li = lines;
		for (j=0 ; j<numlines ; j++, li++)
		{
			if (li->frontsector == sector || li->backsector == sector)
			{
				*linebuffer++ = li;
				bbox.AddToBox (li->v1->x, li->v1->y);
				bbox.AddToBox (li->v2->x, li->v2->y);
			}
		}
		if (linebuffer - sector->lines != sector->linecount)
			I_Error ("P_GroupLines: miscounted");

		// set the soundorg to the middle of the bounding box
		sector->soundorg[0] = (bbox.Right()+bbox.Left())/2;
		sector->soundorg[1] = (bbox.Top()+bbox.Bottom())/2;

		// adjust bounding box to map blocks
		block = (bbox.Top()-bmaporgy+MAXRADIUS)>>MAPBLOCKSHIFT;
		block = block >= bmapheight ? bmapheight-1 : block;
		sector->blockbox[BOXTOP]=block;

		block = (bbox.Bottom()-bmaporgy-MAXRADIUS)>>MAPBLOCKSHIFT;
		block = block < 0 ? 0 : block;
		sector->blockbox[BOXBOTTOM]=block;

		block = (bbox.Right()-bmaporgx+MAXRADIUS)>>MAPBLOCKSHIFT;
		block = block >= bmapwidth ? bmapwidth-1 : block;
		sector->blockbox[BOXRIGHT]=block;

		block = (bbox.Left()-bmaporgx-MAXRADIUS)>>MAPBLOCKSHIFT;
		block = block < 0 ? 0 : block;
		sector->blockbox[BOXLEFT]=block;
	}

}

//
// [RH] P_LoadBehavior
//
void P_LoadBehavior (int lumpnum)
{
	byte *behavior = (byte *)W_CacheLumpNum (lumpnum, PU_LEVEL);

	level.behavior = new FBehavior (behavior, lumpinfo[lumpnum].size);

	if (!level.behavior->IsGood ())
	{
		delete level.behavior;
		level.behavior = NULL;
	}
}

//
// P_AllocStarts
//
void P_AllocStarts(void)
{
	if (!deathmatchstarts)
	{
		MaxDeathmatchStarts = 16;	// [RH] Default. Increased as needed.
		deathmatchstarts = (mapthing2_t *)Malloc (MaxDeathmatchStarts * sizeof(mapthing2_t));
	}
	deathmatch_p = deathmatchstarts;

	//	[Toke - CTF]
	if (!blueteamstarts) // [Toke - CTF - starts]
	{
		MaxBlueTeamStarts = 16;
		blueteamstarts = (mapthing2_t *)Malloc (MaxBlueTeamStarts * sizeof(mapthing2_t));
	}
	blueteam_p = blueteamstarts;

	if (!redteamstarts) // [Toke - CTF - starts]
	{
		MaxRedTeamStarts = 16;
		redteamstarts = (mapthing2_t *)Malloc (MaxRedTeamStarts * sizeof(mapthing2_t));
	}
	redteam_p = redteamstarts;
}

//
// P_SetupLevel
//
extern dyncolormap_t NormalLight;
extern polyblock_t **PolyBlockMap;

// [RH] position indicates the start spot to spawn at
void P_SetupLevel (char *lumpname, int position)
{
	size_t i, lumpnum;

	level.total_monsters = level.total_items = level.total_secrets =
		level.killed_monsters = level.found_items = level.found_secrets =
		wminfo.maxfrags = 0;
	wminfo.partime = 180;

	if (!savegamerestore)
	{
		for (i = 0; i < players.size(); i++)
		{
			players[i].killcount = players[i].secretcount
				= players[i].itemcount = 0;
		}
	}

	// Initial height of PointOfView will be set by player think.
	consoleplayer().viewz = 1;

	// Make sure all sounds are stopped before Z_FreeTags.
	S_Start ();

	// [RH] Clear all ThingID hash chains.
	AActor::ClearTIDHashes ();

	// [RH] clear out the mid-screen message
	C_MidPrint (NULL);

	PolyBlockMap = NULL;

	DThinker::DestroyAllThinkers ();
	Z_FreeTags (PU_LEVEL, PU_PURGELEVEL-1);
	NormalLight.next = NULL;	// [RH] Z_FreeTags frees all the custom colormaps

	// UNUSED W_Profile ();

	// find map num
	lumpnum = W_GetNumForName (lumpname);

	// [RH] Check if this map is Hexen-style.
	//		LINEDEFS and THINGS need to be handled accordingly.
	//		If it is, we also need to distinguish between projectile cross and hit
	HasBehavior = W_CheckLumpName (lumpnum+ML_BEHAVIOR, "BEHAVIOR");
	//oldshootactivation = !HasBehavior;

	// note: most of this ordering is important

	// [RH] Load in the BEHAVIOR lump
	if (level.behavior != NULL)
	{
		delete level.behavior;
		level.behavior = NULL;
	}
	if (HasBehavior)
	{
		P_LoadBehavior (lumpnum+ML_BEHAVIOR);
	}

    level.time = 0;

	P_LoadVertexes (lumpnum+ML_VERTEXES);
	P_LoadSectors (lumpnum+ML_SECTORS);
	P_LoadSideDefs (lumpnum+ML_SIDEDEFS);
	if (!HasBehavior)
		P_LoadLineDefs (lumpnum+ML_LINEDEFS);
	else
		P_LoadLineDefs2 (lumpnum+ML_LINEDEFS);	// [RH] Load Hexen-style linedefs
	P_LoadSideDefs2 (lumpnum+ML_SIDEDEFS);
	P_FinishLoadingLineDefs ();
	P_LoadBlockMap (lumpnum+ML_BLOCKMAP);
	P_LoadSubsectors (lumpnum+ML_SSECTORS);
	P_LoadNodes (lumpnum+ML_NODES);
	P_LoadSegs (lumpnum+ML_SEGS);

	rejectmatrix = (byte *)W_CacheLumpNum (lumpnum+ML_REJECT, PU_LEVEL);
	{
		// [SL] 2011-07-01 - Check to see if the reject table is of the proper size
		// If it's too short, the reject table should be ignored when
		// calling P_CheckSight
		if (W_LumpLength(lumpnum + ML_REJECT) < ((unsigned int)ceil((float)(numsectors * numsectors / 8))))
		{
			DPrintf("Reject matrix is not valid and will be ignored.\n");
			rejectempty = true;
		}
	}
	P_GroupLines ();
	P_SetupSlopes();

    po_NumPolyobjs = 0;

	P_AllocStarts();

	if (!HasBehavior)
		P_LoadThings (lumpnum+ML_THINGS);
	else
		P_LoadThings2 (lumpnum+ML_THINGS, position);	// [RH] Load Hexen-style things

	if (!HasBehavior)
		P_TranslateTeleportThings ();	// [RH] Assign teleport destination TIDs

    PO_Init ();

    if (serverside)
    {
		for (i = 0 ; i < players.size() ; i++)
		{
			SV_PreservePlayer(players[i]);

			if (players[i].ingame())
			{
				// if deathmatch, randomly spawn the active players
				// denis - this function checks for deathmatch internally
				G_DeathMatchSpawnPlayer (players[i]); 
			}
		}
    }

	// clear special respawning que
	iquehead = iquetail = 0;

	// killough 3/26/98: Spawn icon landings:
	P_SpawnBrainTargets();

	// set up world state
	P_SpawnSpecials ();

	// build subsector connect matrix
	//	UNUSED P_ConnectSubsectors ();

	R_OldBlend = ~0;

	// preload graphics
	if (clientside && precache)
		R_PrecacheLevel ();
}

//
// P_Init
//
void P_Init (void)
{
	P_InitSwitchList ();
	P_InitPicAnims ();
	R_InitSprites (sprnames);
}


// [ML] Do stuff when the timelimit is reset
// Where else can I put this??
CVAR_FUNC_IMPL (sv_timelimit)
{
	if (var < 0)
		var.Set(0.0f);

	// timeleft is transmitted as a short so cap the sv_timelimit at the maximum
	// for timeleft, which is 9.1 hours
	if (var > MAXSHORT / 60)
		var.Set(MAXSHORT / 60);

	level.timeleft = var * TICRATE * 60;
}

CVAR_FUNC_IMPL (sv_intermissionlimit)
{
	if (var < 0)
		var.Set(0.0f);

	// intermissionleft is transmitted as a short so cap the sv_timelimit at the maximum
	// for timeleft, which is 9.1 hours
	if (var > MAXSHORT)
		var.Set(MAXSHORT);

	level.inttimeleft = (var < 1 ? DEFINTSECS : var);
}


static void P_SetupLevelFloorPlane(sector_t *sector)
{
	if (!sector)
		return;
	
	sector->floorplane.a = sector->floorplane.b = 0;
	sector->floorplane.c = sector->floorplane.invc = FRACUNIT;
	sector->floorplane.d = -sector->floorheight;
	sector->floorplane.texx = sector->floorplane.texy = 0;
	sector->floorplane.sector = sector;
}

static void P_SetupLevelCeilingPlane(sector_t *sector)
{
	if (!sector)
		return;
	
	sector->ceilingplane.a = sector->ceilingplane.b = 0;
	sector->ceilingplane.c = sector->ceilingplane.invc = -FRACUNIT;
	sector->ceilingplane.d = sector->ceilingheight;
	sector->ceilingplane.texx = sector->ceilingplane.texy = 0;
	sector->ceilingplane.sector = sector;
}

//
// P_SetupPlane()
//
// Takes a line with the special property Plane_Align and its facing sector
// and calculates the planar equation for the slope formed by the floor or
// ceiling of this sector.  The equation coefficients are stored in a plane_t
// structure and saved either to the sector's ceilingplan or floorplane.
//
void P_SetupPlane(sector_t *refsector, line_t *refline, bool floor)
{
	if (!refsector || !refline || !refline->backsector)
		return;

	double refv1x = FIXED2DOUBLE(refline->v1->x);
	double refv1y = FIXED2DOUBLE(refline->v1->y);

	double refdx = FIXED2DOUBLE(refline->dx);
	double refdy = FIXED2DOUBLE(refline->dy);
	
	vertex_t *farthest_vertex = NULL;
	double farthest_distance = 0.0;

	// Find the vertex comprising the sector that is farthest from the
	// slope's reference line
	for (int linenum = 0; linenum < refsector->linecount; linenum++)
	{
		line_t *line = refsector->lines[linenum];
		if (!line)
			continue;
		
		// Calculate distance from vertex 1 of this line
		double dist = abs((refv1y - FIXED2DOUBLE(line->v1->y)) * refdx -
						  (refv1x - FIXED2DOUBLE(line->v1->x)) * refdy);
		if (dist > farthest_distance)
		{
			farthest_distance = dist;
			farthest_vertex = line->v1;
		}
	
		// Calculate distance from vertex 2 of this line
		dist = abs((refv1y - FIXED2DOUBLE(line->v2->y)) * refdx -
				   (refv1x - FIXED2DOUBLE(line->v2->x)) * refdy);
		if (dist > farthest_distance)
		{
			farthest_distance = dist;
			farthest_vertex = line->v2;
		}	
	}
	
	if (farthest_distance <= 0.0)
		return;

	sector_t *align_sector = (refsector == refline->frontsector) ?
		refline->backsector : refline->frontsector;

	// Now we have three points, which can define a plane:
	// The two vertices making up refline and farthest_vertex
	
	fixed_t z1 = floor ? align_sector->floorheight : align_sector->ceilingheight;
	fixed_t z2 = floor ? refsector->floorheight : refsector->ceilingheight;
	
	// bail if the plane is perfectly level
	if (z1 == z2)
		return;

	v3double_t p1, p2, p3;
	M_SetVec3(&p1, refline->v1->x, refline->v1->y, z1);
	M_SetVec3(&p2, refline->v2->x, refline->v2->y, z1);
	M_SetVec3(&p3, farthest_vertex->x, farthest_vertex->y, z2);

	// Define the plane by drawing two vectors originating from
	// point p2:  the vector from p2 to p1 and from p2 to p3
	// Then take the crossproduct of those vectors to get the normal vector
	// for the plane, which provides the planar equation's coefficients
	v3double_t vector1, vector2;
	M_SubVec3(&vector1, &p1, &p2);
	M_SubVec3(&vector2, &p3, &p2);

	v3double_t normal;
	M_CrossProductVec3(&normal, &vector1, &vector2);
	M_NormalizeVec3(&normal, &normal);
	
	plane_t *plane = floor ? &refsector->floorplane : &refsector->ceilingplane;
	plane->a = DOUBLE2FIXED(normal.x);
	plane->b = DOUBLE2FIXED(normal.y);
	plane->c = DOUBLE2FIXED(normal.z);
	plane->invc = DOUBLE2FIXED(1.0 / normal.z);
	plane->d = -DOUBLE2FIXED(M_DotProductVec3(&normal, &p1));

	// Flip inverted normals
	if ((floor && normal.z < 0.0) || (!floor && normal.z > 0.0))
		P_InvertPlane(plane);

	// determine the point that can be used for aligning wall textures
	// we use the point on the plane that has the same Z value as
	// ceilingheight/floorheight
	plane->texx = refline->v1->x;
	plane->texy = refline->v1->y;

	if ((floor && refsector->floorheight != align_sector->floorheight) ||
	   (!floor && refsector->ceilingheight != align_sector->ceilingheight))
	{
		plane->texx = farthest_vertex->x;
		plane->texy = farthest_vertex->y;
	}
}


static void P_SetupSlopes()
{
	for (int i = 0; i < numlines; i++)
	{
		line_t *line = &lines[i];

		if (line->special == Plane_Align)
		{
			line->special = 0;
			line->id = line->args[2];
			
			// Floor plane?
			int align_side = line->args[0] & 3;
			if (align_side == 1)
				P_SetupPlane(line->frontsector, line, true);
			else if (align_side == 2)
				P_SetupPlane(line->backsector, line, true);
				
			// Ceiling plane?
			align_side = line->args[1] & 3;
			if (align_side == 0)
				align_side = (line->args[0] >> 2) & 3;
			
			if (align_side == 1)
				P_SetupPlane(line->frontsector, line, false);
			else if (align_side == 2)
				P_SetupPlane(line->backsector, line, false);
		}
	}
}


VERSION_CONTROL (p_setup_cpp, "$Id$")

