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
//		Do all the WAD I/O, get map description,
//		set up initial state and misc. LUTs.
//
//-----------------------------------------------------------------------------


#include "odamex.h"

#include <stdlib.h>
#include <math.h>
#include <set>
#include <zlib.h>
#include <nonstd/scope.hpp>

#include "m_alloc.h"
#include "m_vectors.h"
#include "m_argv.h"
#include "z_zone.h"
#include "m_bbox.h"
#include "g_game.h"
#include "i_system.h"
#include "w_wad.h"
#include "p_local.h"
#include "p_acs.h"
#include "s_sound.h"
#include "p_lnspec.h"
#include "v_palette.h"
#include "c_console.h"
#include "p_horde.h"
#include "g_gametype.h"

#include "p_mobj.h"
#include "p_setup.h"
#include "p_hordespawn.h"
#include "p_mapformat.h"
#include "g_musinfo.h"
#include "r_sky.h"
#include "p_compdb.h"
#include "cl_freecam.h"

void SV_PreservePlayer(player_t &player);
void P_SpawnMapThing (mapthing2_t& mthing, int position);
void P_SpawnAvatars();
void P_TranslateTeleportThings();

uint32_t P_TranslateCompatibleLineFlags(const uint32_t flags, const bool reserved);
uint32_t P_TranslateZDoomLineFlags(const uint32_t flags);
void P_SpawnCompatibleSectorSpecial(sector_t* sector);

namespace {
void P_SetupLevelFloorPlane(sector_t *sector);
void P_SetupLevelCeilingPlane(sector_t *sector);
void P_SetupSlopes();
}

void P_InvertPlane(plane_t *plane);
void P_SetupWorldState();
int P_TranslateSectorSpecial(int special);

extern dyncolormap_t NormalLight;
extern AActor* shootthing;

EXTERN_CVAR(g_thingfilter)

bool			g_ValidLevel = false;

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

std::vector<int>	originalLightLevels; // Needed for map resets

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
bool			rejectempty;

// Maintain single and multi player starting spots.
std::vector<mapthing2_t> DeathMatchStarts;
std::vector<mapthing2_t> playerstarts;
std::vector<mapthing2_t> voodoostarts;

// Maintain list of helpers to spawn in a given map
std::vector<HelperSpawns> helperspawns;

namespace {

//
// P_LoadVertexes
//
void P_LoadVertexes (int lump)
{
	// Determine number of vertices:
	//	total lump length / vertex record length.
	numvertexes = W_LumpLength (lump) / sizeof(mapvertex_t);

	// Allocate zone memory for buffer.
	vertexes = (vertex_t *)Z_Malloc (numvertexes*sizeof(vertex_t), PU_LEVEL, 0);

	// Load data into cache.
	byte* data = (byte *)W_CacheLumpNum (lump, PU_STATIC);

	// Copy and convert vertex coordinates,
	// internal representation as fixed.
	for (int i = 0; i < numvertexes; i++)
	{
		vertexes[i].x = LESHORT(((mapvertex_t *)data)[i].x)<<FRACBITS;
		vertexes[i].y = LESHORT(((mapvertex_t *)data)[i].y)<<FRACBITS;
	}

	// Free buffer memory.
	Z_Free (data);
}

void P_LoadSegsHelper(int side, short angle, int linedef, seg_t *li)
{
	li->angle = (angle)<<16;

	if(linedef < 0 || linedef >= numlines)
		I_Error("P_LoadSegsHelper: invalid linedef {}", linedef);

	line_t* ldef = &lines[linedef];
	li->linedef = ldef;

	if (side != 0 && side != 1)
		side = 1;	// assume invalid value means back

	li->sidedef = &sides[ldef->sidenum[side]];
	li->frontsector = sides[ldef->sidenum[side]].sector;

	// killough 5/3/98: ignore 2s flag if second sidedef missing:
	if (ldef->flags & ML_TWOSIDED && ldef->sidenum[side^1]!=R_NOSIDE)
		li->backsector = sides[ldef->sidenum[side^1]].sector;
	else
	{
		li->backsector = nullptr;
		ldef->flags &= ~ML_TWOSIDED;
	}

	// recalculate seg offsets. values in wads are untrustworthy.
	const vertex_t *from = (side == 0)
	    ? ldef->v1               // right side: offset is from start of linedef
	    : ldef->v2;              // left side: offset is from end of linedef
	const vertex_t *to = li->v1; // end point is start of seg, in both cases

	float dx = FIXED2FLOAT(to->x - from->x);
	float dy = FIXED2FLOAT(to->y - from->y);
	li->offset = FLOAT2FIXED(sqrt(dx * dx + dy * dy));

	dx = FIXED2FLOAT(li->v2->x - li->v1->x);
	dy = FIXED2FLOAT(li->v2->y - li->v1->y);
	li->length = FLOAT2FIXED(sqrt(dx * dx + dy* dy));
}


//
// P_LoadSegs
//
template <typename MapSegType>
void P_LoadSegs (int lump)
{
	if (!W_LumpLength(lump))
	{
		I_Error(
		    "P_LoadSegs: SEGS lump is empty - levels without nodes are not supported.");
	}

	numsegs = W_LumpLength (lump) / sizeof(MapSegType);
	segs = (seg_t*) Z_Malloc(numsegs * sizeof(seg_t), PU_LEVEL, 0);
	memset(segs, 0, numsegs * sizeof(seg_t));
	byte* const data = (byte*) W_CacheLumpNum(lump, PU_STATIC);

	for (int i = 0; i < numsegs; i++)
	{
		seg_t* const li = segs + i;
		const MapSegType *ml = (MapSegType*) data + i;
		auto v = LESWAP(ml->v1);

		if(v >= numvertexes)
			I_Error("P_LoadSegs: invalid vertex {}", v);
		else
			li->v1 = &vertexes[v];

		v = LESWAP(ml->v2);

		if(v >= numvertexes)
			I_Error("P_LoadSegs: invalid vertex {}", v);
		else
			li->v2 = &vertexes[v];

		P_LoadSegsHelper(LESHORT(ml->side), LESHORT(ml->angle), LESHORT(ml->linedef), li);
	}

	Z_Free (data);
}

//
// P_LoadSubsectors
//
template <typename MapSubsectorType>
void P_LoadSubsectors(int lump)
{
	if (!W_LumpLength(lump))
	{
		I_Error(
		    "P_LoadSubsectors: SSECTORS lump is empty - levels without nodes are not supported.");
	}

	numsubsectors = W_LumpLength (lump) / sizeof(MapSubsectorType);
	subsectors = (subsector_t *)Z_Malloc (numsubsectors*sizeof(subsector_t),PU_LEVEL,0);
	byte* data = (byte *)W_CacheLumpNum (lump,PU_STATIC);

	memset (subsectors, 0, numsubsectors*sizeof(subsector_t));

	for (int i = 0; i < numsubsectors; i++)
	{
		subsectors[i].numlines = LESWAP<decltype(MapSubsectorType::numsegs)>(((MapSubsectorType *)data)[i].numsegs);
		subsectors[i].firstline = LESWAP<decltype(MapSubsectorType::firstseg)>(((MapSubsectorType *)data)[i].firstseg);
	}

	Z_Free(data);
}



//
// P_LoadSectors
//
void P_LoadSectors (int lump)
{
	// denis - properly destroy sectors so that smart pointers they contain don't get screwed
	delete[] sectors;
	originalLightLevels.clear();

	numsectors = W_LumpLength(lump) / sizeof(mapsector_t);

	// denis - properly construct sectors so that smart pointers they contain don't get screwed
	sectors = new sector_t[numsectors];

	byte* data = (byte*)W_CacheLumpNum(lump, PU_STATIC);

	const int defSeqType = (level.flags & LEVEL_SNDSEQTOTALCTRL) ? 0 : -1;

	const mapsector_t* ms = (mapsector_t*)data;
	sector_t* ss = sectors;
	for (int i = 0; i < numsectors; i++, ss++, ms++)
	{
		ss->floorheight = LESHORT(ms->floorheight)<<FRACBITS;
		ss->ceilingheight = LESHORT(ms->ceilingheight)<<FRACBITS;
		ss->floorpic = (short)R_FlatNumForName(ms->floorpic);
		ss->ceilingpic = (short)R_FlatNumForName(ms->ceilingpic);
		ss->lightlevel = LESHORT(ms->lightlevel);
		originalLightLevels.push_back(LESHORT(ms->lightlevel));
		ss->special = LESHORT(ms->special);
		ss->secretsector = !!(ss->special&SECRET_MASK);
		ss->tag = LESHORT(ms->tag);
		ss->thinglist = NULL;
		ss->touching_thinglist = NULL;		// phares 3/14/98
		ss->seqType = defSeqType;
		ss->nextsec = -1;	//jff 2/26/98 add fields to support locking out
		ss->prevsec = -1;	// stair retriggering until build completes

		// damage
		ss->damageamount = 0;
		ss->damageinterval = 0;
		ss->leakrate = 0;

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
		// [SL] no fog is indicated by outsidefog_color == 0xFF, 0, 0, 0
		bool fog = level.outsidefog_color[0] != 0xFF || level.outsidefog_color[1] != 0 ||
					level.outsidefog_color[2] != 0 || level.outsidefog_color[3] != 0;

		if (fog && R_IsSkyFlat(ss->ceilingpic))
			ss->colormap = GetSpecialLights(255, 255, 255,
									level.outsidefog_color[1], level.outsidefog_color[2], level.outsidefog_color[3]);
		else
			ss->colormap = &NormalLight;

		ss->sky = 0;

		// killough 8/28/98: initialize all sectors to normal friction
		ss->friction = ORIG_FRICTION;
		ss->movefactor = ORIG_FRICTION_FACTOR;
	}

	Z_Free (data);
}

enum class nodetype_t {
	XNOD,
	ZNOD,
	DEEP,
	XGLN,
	XGL2,
	XGL3,
	ZGLN,
	ZGL2,
	ZGL3,
	STANDARD
};

nodetype_t P_CheckNodeType(int lump) {
	byte *data = (byte *) W_CacheLumpNum(lump, PU_STATIC);
	auto guard = nonstd::make_scope_exit([&]{ Z_ChangeTag(data, PU_CACHE); });

	static constexpr struct {
        std::string_view bytes;
        nodetype_t type;
    } node_types[] = {
        {"xNd4\0\0\0\0", nodetype_t::DEEP},
        {"XNOD", nodetype_t::XNOD},
        {"ZNOD", nodetype_t::ZNOD},
        {"XGLN", nodetype_t::XGLN},
        {"XGL2", nodetype_t::XGL2},
        {"XGL3", nodetype_t::XGL3},
        {"ZGLN", nodetype_t::ZGLN},
        {"ZGL2", nodetype_t::ZGL2},
        {"ZGL3", nodetype_t::ZGL3},
    };

	const auto it = std::find_if(
		std::begin(node_types), std::end(node_types),
		[&](const auto& nodetype) {
			return memcmp(data, nodetype.bytes.data(), nodetype.bytes.size()) == 0;
	});

	if (it != std::end(node_types)) {
		return it->type;
	}

	return nodetype_t::STANDARD;
}

//
// P_LoadNodes
//
template <typename MapNodeType>
void P_LoadNodes(int lump)
{
	if (!W_LumpLength(lump))
	{
		I_Error(
		    "P_LoadNodes: NODES lump is empty - levels without nodes are not supported.");
	}

	numnodes = W_LumpLength(lump) / sizeof(MapNodeType);
	nodes = (node_t*) Z_Malloc(numnodes * sizeof(node_t), PU_LEVEL, 0);
	byte* data = (byte*) W_CacheLumpNum(lump, PU_STATIC);

	const MapNodeType* mn = (MapNodeType *)data;
	node_t* no = nodes;

	for (int i = 0; i < numnodes; i++, no++, mn++)
	{
		no->x = LESHORT(mn->x) << FRACBITS;
		no->y = LESHORT(mn->y) << FRACBITS;
		no->dx = LESHORT(mn->dx) << FRACBITS;
		no->dy = LESHORT(mn->dy) << FRACBITS;
		for (int j = 0; j < 2; j++)
		{
			uint32_t child = LESWAP(mn->children[j]);

			// account for children's promotion to 32 bits
			if constexpr (std::is_same_v<std::remove_extent_t<decltype(MapNodeType::children)>, uint16_t>)
			{
				if (child == 0xffff)
					child = 0xffffffff;
				else if (child & 0x8000)
					child = (child & ~0x8000) | NF_SUBSECTOR;
			}

			no->children[j] = child;

			for (int k = 0; k < 4; k++)
				no->bbox[j][k] = LESHORT(mn->bbox[j][k]) << FRACBITS;
		}
	}

	Z_Free (data);
}

byte* P_DecompressNodes(byte* data, size_t len) {
	int err;

	// first estimate for compression rate:
	// output buffer size == 2.5 * input size
	int outlen = 2.5 * len;
	byte* output = (byte*)Z_Malloc(outlen, PU_STATIC, 0);

	// initialize stream state for decompression
	z_stream* zstream = (z_stream*)M_Malloc(sizeof(*zstream));
	memset(zstream, 0, sizeof(*zstream));
	zstream->next_in = data + 4;
	zstream->avail_in = static_cast<uInt>(len - 4);
	zstream->next_out = output;
	zstream->avail_out = outlen;

	if (inflateInit(zstream) != Z_OK)
		I_Error("P_DecompressNodes: Error during ZDBSP nodes decompression initialization!");

	// resize if output buffer runs full
	while ((err = inflate(zstream, Z_SYNC_FLUSH)) == Z_OK)
	{
		int outlen_old = outlen;
		outlen = 2 * outlen_old;
		output = (byte*)Z_Realloc(output, outlen, PU_STATIC, 0);
		zstream->next_out = output + outlen_old;
		zstream->avail_out = outlen - outlen_old;
	}

	if (err != Z_STREAM_END)
		I_Error("P_DecompressNodes: Error during ZDBSP nodes decompression!");

	fmt::print(stderr, "P_DecompressNodes: ZDBSP nodes compression ratio {:.3f}\n",
	           (float)zstream->total_out/zstream->total_in);

	if (inflateEnd(zstream) != Z_OK)
		I_Error("P_DecompressNodes: Error during ZDBSP nodes decompression shut-down!");

	M_Free(zstream);
	return output;
}

byte* P_LoadSegs_XNOD(byte* p) {
	numsegs = LELONG(*(uint32_t *)p); p += 4;
	segs = (seg_t *) Z_Malloc(numsegs * sizeof(*segs), PU_LEVEL, 0);
	memset(segs, 0, numsegs * sizeof(*segs));

	for (int i = 0; i < numsegs; i++)
	{
		const uint32_t v1 = LELONG(*(uint32_t *)p); p += 4;
		const uint32_t v2 = LELONG(*(uint32_t *)p); p += 4;
		const uint16_t ld = LESHORT(*(uint16_t *)p); p += 2;
		uint8_t side = *(uint8_t *)p; p += 1;

		if (side != 0 && side != 1)
			side = 1;

		seg_t *seg = &segs[i];
		line_t *line = &lines[ld];

		seg->v1 = &vertexes[v1];
		seg->v2 = &vertexes[v2];

		seg->linedef = line;
		seg->sidedef = &sides[line->sidenum[side]];

		seg->frontsector = seg->sidedef->sector;
		if (line->flags & ML_TWOSIDED && line->sidenum[side^1] != R_NOSIDE)
			seg->backsector = sides[line->sidenum[side^1]].sector;
		else
		{
			seg->backsector = NULL;
			line->flags &= ~ML_TWOSIDED;
		}

		seg->angle = R_PointToAngle2(seg->v1->x, seg->v1->y, seg->v2->x, seg->v2->y);

		// a short version of the offset calculation in P_LoadSegs
		const vertex_t *origin = (side == 0) ? line->v1 : line->v2;
		const float dx = FIXED2FLOAT(seg->v1->x - origin->x);
		const float dy = FIXED2FLOAT(seg->v1->y - origin->y);
		seg->offset = FLOAT2FIXED(sqrt(dx * dx + dy * dy));
	}

	return p;
}

template<typename LineType>
byte* P_LoadSegs_XGL(byte* p)
{
	static_assert(
        std::is_same_v<LineType, uint16_t> || std::is_same_v<LineType, uint32_t>,
        "P_LoadSegs_XGL can only be instantiated with uint16_t or uint32_t"
    );

	numsegs = LELONG(*(uint32_t *)p); p += 4;
	segs = (seg_t *) Z_Malloc(numsegs * sizeof(*segs), PU_LEVEL, 0);
	memset(segs, 0, numsegs * sizeof(*segs));

	for (int i = 0; i < numsubsectors; i++)
	{
		for (uint32_t j = 0; j < subsectors[i].numlines; j++)
		{
			const uint32_t v1 = LELONG(*(uint32_t *)p); p += 4;
			const uint32_t partner = LELONG(*(uint32_t *)p); p += 4;
			const LineType ld = LESWAP(*(LineType *)p); p += sizeof(LineType);
			const uint8_t side = *(uint8_t *)p; p += 1;

			seg_t* seg = &segs[subsectors[i].firstline + j];

			seg->v1 = &vertexes[v1];
			if (j == 0)
				seg[subsectors[i].numlines - 1].v2 = seg->v1;
			else
				seg[-1].v2 = seg->v1;

			if (ld != std::numeric_limits<LineType>::max())
			{
				if (ld >= numlines)
				{
					I_Error("P_LoadSegs_XGL: seg {}, {} references a non-existent linedef {}", i, j, ld);
				}

				line_t* line = &lines[ld];
				seg->linedef = line;

				if (side != 0 && side != 1)
				{
					I_Error("P_LoadSegs_XGL: seg {}, {} references a non-existent sidedef {}", i, j, side);
				}

				seg->sidedef = &sides[line->sidenum[side]];

				if (line->sidenum[side] != NO_INDEX)
				{
					seg->frontsector = sides[line->sidenum[side]].sector;
				}
				else
				{
					seg->frontsector = nullptr;
					fmt::print(stderr, "P_LoadSegs_XGL: front of seg {}, {} has no sidedef\n", i, j);
				}

				if ((line->flags & ML_TWOSIDED) &&
				    (line->sidenum[side ^ 1] != NO_INDEX))
					seg->backsector = sides[line->sidenum[side ^ 1]].sector;
				else
				{
					seg->backsector = nullptr;
					line->flags &= ~ML_TWOSIDED;
				}

				// a short version of the offset calculation in P_LoadSegs
				const vertex_t *origin = (side == 0) ? line->v1 : line->v2;
				const float dx = FIXED2FLOAT(seg->v1->x - origin->x);
				const float dy = FIXED2FLOAT(seg->v1->y - origin->y);
				seg->offset = FLOAT2FIXED(sqrt(dx * dx + dy * dy));
			}
			else
			{
				seg->angle = 0;
				seg->offset = 0;
				seg->sidedef = nullptr;
				seg->linedef = nullptr;
				seg->frontsector = segs[subsectors[i].firstline].frontsector;
				seg->backsector = seg->frontsector;
			}
		}

		for (uint32_t j = 0; j < subsectors[i].numlines; j++)
		{
			seg_t* seg = &segs[subsectors[i].firstline + j];

			if (seg->linedef)
				seg->angle = R_PointToAngle2(seg->v1->x, seg->v1->y, seg->v2->x, seg->v2->y);
		}
	}
	return p;
}

//
// P_LoadXNOD - load ZDBSP extended nodes
// returns false if nodes are not extended to fall back to original nodes
//
void P_LoadExtendedNodes(int lump, nodetype_t nodetype)
{
	const bool compressed = [&](){
		switch (nodetype)
		{
			case nodetype_t::ZNOD:
			case nodetype_t::ZGLN:
			case nodetype_t::ZGL2:
			case nodetype_t::ZGL3:
				return true;
			default:
				return false;
		}
	}();
	byte *data = static_cast<byte *>(W_CacheLumpNum(lump, PU_STATIC));
	byte* data_decompressed = nullptr;

	auto guard = nonstd::make_scope_exit([&]{
		Z_Free(data);
		Z_Free(data_decompressed);
	});

	byte *p;
	// [EB] decompress compressed nodes
	// adapted from Crispy Doom
	if (compressed)
	{
		p = data_decompressed = P_DecompressNodes(data, W_LumpLength(lump));
	}
	else
	{
		p = data + 4; // skip the magic number
	}

	// Load vertices
	const uint32_t numorgvert = LELONG(*(uint32_t *)p); p += 4;
	const uint32_t numnewvert = LELONG(*(uint32_t *)p); p += 4;

	vertex_t *newvert = (vertex_t *) Z_Malloc((numorgvert + numnewvert)*sizeof(*newvert), PU_LEVEL, 0);

	memcpy(newvert, vertexes, numorgvert*sizeof(*newvert));
	memset(&newvert[numorgvert], 0, numnewvert * sizeof(*newvert));

	for (uint32_t i = 0; i < numnewvert; i++)
	{
		vertex_t *v = &newvert[numorgvert+i];
		v->x = LELONG(*(int32_t *)p); p += 4;
		v->y = LELONG(*(int32_t *)p); p += 4;
	}

	// Adjust linedefs - since we reallocated the vertex array,
	// all vertex pointers in linedefs must be updated

	for (int i = 0; i < numlines; i++)
	{
		lines[i].v1 = newvert + (lines[i].v1 - vertexes);
		lines[i].v2 = newvert + (lines[i].v2 - vertexes);
	}

	// nuke the old list, update globals to point to the new list
	Z_Free(vertexes);
	vertexes = newvert;
	numvertexes = numorgvert + numnewvert;

	// Load subsectors

	numsubsectors = LELONG(*(uint32_t *)p); p += 4;
	subsectors = (subsector_t *) Z_Malloc(numsubsectors * sizeof(*subsectors), PU_LEVEL, 0);
	memset(subsectors, 0, numsubsectors * sizeof(*subsectors));

	uint32_t first_seg = 0;

	for (int i = 0; i < numsubsectors; i++)
	{
		subsectors[i].firstline = first_seg;
		subsectors[i].numlines = LELONG(*(uint32_t *)p); p += 4;
		first_seg += subsectors[i].numlines;
	}

	// Load segs

	if (nodetype == nodetype_t::XNOD || nodetype == nodetype_t::ZNOD)
		p = P_LoadSegs_XNOD(p);
	else if (nodetype == nodetype_t::XGLN || nodetype == nodetype_t::ZGLN)
		p = P_LoadSegs_XGL<uint16_t>(p);
	else
		p = P_LoadSegs_XGL<uint32_t>(p);

	// Load nodes

	numnodes = LELONG(*(uint32_t *)p); p += 4;
	nodes = (node_t *) Z_Malloc(numnodes * sizeof(*nodes), PU_LEVEL, 0);
	memset(nodes, 0, numnodes * sizeof(*nodes));

	for (int i = 0; i < numnodes; i++)
	{
		node_t *node = &nodes[i];

		if (nodetype == nodetype_t::XGL3 || nodetype == nodetype_t::ZGL3)
		{
			node->x = LELONG(*(int32_t *)p); p += 4;
			node->y = LELONG(*(int32_t *)p); p += 4;
			node->dx = LELONG(*(int32_t *)p); p += 4;
			node->dy = LELONG(*(int32_t *)p); p += 4;
		}
		else
		{
			node->x = LESHORT(*(int16_t *)p)<<FRACBITS; p += 2;
			node->y = LESHORT(*(int16_t *)p)<<FRACBITS; p += 2;
			node->dx = LESHORT(*(int16_t *)p)<<FRACBITS; p += 2;
			node->dy = LESHORT(*(int16_t *)p)<<FRACBITS; p += 2;
		}

		for (int j = 0; j < 2; j++)
		{
			for (int k = 0; k < 4; k++)
			{
				node->bbox[j][k] = LESHORT(*(int16_t *)p)<<FRACBITS; p += 2;
			}
		}

		for (int j = 0; j < 2; j++)
		{
			node->children[j] = LELONG(*(uint32_t *)p); p += 4;
		}
	}
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

	P_HordeClearSpawns();
	playerstarts.clear();
	voodoostarts.clear();
	DeathMatchStarts.clear();
	for (int iTeam = 0; iTeam < NUMTEAMS; iTeam++)
		GetTeamInfo((team_t)iTeam)->Starts.clear();

	// [RH] ZDoom now uses Hexen-style maps as its native format. // denis - growwwwl
	//		Since this is the only place where Doom-style Things are ever
	//		referenced, we translate them into a Hexen-style thing.
	for ( ; mt < lastmt; mt++)
	{
		// [AM] Ensure that we get a fresh mapthing every iteration - sometimes
		//      P_SpawnMapThing mutates a part of the mapthing that the map
		//      data doesn't care about, and we don't want it to carry over
		//      between iterations.
		memset(&mt2, 0, sizeof(mt2));

		// [RH] At this point, monsters unique to Doom II were weeded out
		//		if the IWAD wasn't for Doom II. R_SpawnMapThing() can now
		//		handle these and more cases better, so we just pass it
		//		everything and let it decide what to do with them.

		// [RH] Need to translate the spawn flags to Hexen format.
		short flags = LESHORT(mt->options);
		if (flags & BTF_RESERVED || demoplayback) flags &= BTF_RESERVED_MASK;
		mt2.flags = (short)((flags & 0xf) | 0x7e0);
		if (flags & BTF_NOTSINGLE)
		{
			#ifdef SERVER_APP
			if (G_IsCoopGame())
			{
				if (g_thingfilter == 1)
					mt2.flags |= MTF_FILTER_COOPWPN;
				else if (g_thingfilter == 2)
					mt2.flags &= ~MTF_COOPERATIVE;
			}
			else
			#endif
				mt2.flags &= ~MTF_SINGLE;
		}
		if (flags & BTF_NOTDEATHMATCH)		mt2.flags &= ~MTF_DEATHMATCH;
		if (flags & BTF_NOTCOOPERATIVE)		mt2.flags &= ~MTF_COOPERATIVE;
		if (flags & BTF_FRIEND)				mt2.flags |= MTF_FRIENDLY;

		mt2.x = LESHORT(mt->x);
		mt2.y = LESHORT(mt->y);
		mt2.angle = LESHORT(mt->angle);
		mt2.type = LESHORT(mt->type);

		// clientside freecam start pos
		if (netdemo.isPlaying() && Freecam::needPosition() && P_IsNetplaySpawn(mt2.type))
		{
			Freecam::setStartPosition(mt2.x << FRACBITS, mt2.y << FRACBITS, ONFLOORZ, mt2.angle << FRACBITS);
		}

		P_SpawnMapThing (mt2, 0);
	}

	// Sort by player number if starts are not in order
	std::sort(playerstarts.begin(), playerstarts.end(), [](const mapthing2_t& p1, const mapthing2_t& p2){
		return P_GetMapThingPlayerNumber(p1) < P_GetMapThingPlayerNumber(p2);
	});

	P_SpawnAvatars();

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

	P_HordeClearSpawns();
	playerstarts.clear();
	voodoostarts.clear();
	DeathMatchStarts.clear();
	for (int iTeam = 0; iTeam < NUMTEAMS; iTeam++)
		GetTeamInfo((team_t)iTeam)->Starts.clear();

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

		P_SpawnMapThing(*mt, position);
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
	ld->lucency = 255;	// [RH] Opaque by default

	const vertex_t* v1 = ld->v1;
	const vertex_t* v2 = ld->v2;

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

	if (map_format.getZDoom())
	{
		// [RH] Set line id (as appropriate) here
		if (ld->special == Line_SetIdentification || ld->special == Teleport_Line ||
		    ld->special == TranslucentLine || ld->special == Scroll_Texture_Model)
		{
			ld->id = ld->args[0];
		}
	}
	else
	{
		if (P_IsThingNoFogTeleportLine(ld->special))
		{
			if (ld->id == 0)
			{
				// Untagged teleporters teleport to tid 1.
				ld->args[0] = 1;
			}
			else
			{
				ld->args[2] = ld->id;
				ld->args[0] = 0;
			}
		}
		else if (ld->special >= OdamexStaticInits &&
		         ld->special < OdamexStaticInits + NUM_STATIC_INITS)
		{
			// An Odamex Static_Init special
			ld->args[0] = ld->id;
			ld->args[1] = ld->special - OdamexStaticInits;
		}
		else if (ld->special >= 340 && ld->special <= 347)
		{
			// [SL] 2012-01-30 - convert to ZDoom Plane_Align special for
			// sloping sectors
			// [Blair] Massage this a bit to work with map_format
			switch (ld->special)
			{
			case 340: // Slope the Floor in front of the line
				ld->args[0] = 1;
				break;
			case 341: // Slope the Ceiling in front of the line
				ld->args[1] = 1;
				break;
			case 342: // Slope the Floor+Ceiling in front of the line
				ld->args[0] = ld->args[1] = 1;
				break;
			case 343: // Slope the Floor behind the line
				ld->args[0] = 2;
				break;
			case 344: // Slope the Ceiling behind the line
				ld->args[1] = 2;
				break;
			case 345: // Slope the Floor+Ceiling behind the line
				ld->args[0] = ld->args[1] = 2;
				break;
			case 346: // Slope the Floor behind+Ceiling in front of the line
				ld->args[0] = 2;
				ld->args[1] = 1;
				break;
			case 347: // Slope the Floor in front+Ceiling behind the line
				ld->args[0] = 1;
				ld->args[1] = 2;
			}
		}
	}

	// denis - prevent buffer overrun
	if(*ld->sidenum == R_NOSIDE)
		return;

	if (map_format.getZDoom())
	{
		// killough 4/4/98: support special sidedef interpretation below
		if ( // [RH] Save Static_Init only if it's interested in the textures
		    ((ld->special == Static_Init && ld->args[1] == Init_Color) ||
		     ld->special != Static_Init))
		{
			sides[*ld->sidenum].special = ld->special;
			sides[*ld->sidenum].tag = ld->args[0];
		}
		else
		{
			sides[*ld->sidenum].special = 0;
		}
	}
	else
	{
		// killough 4/4/98: support special sidedef interpretation below
		if (ld->special >= OdamexStaticInits + 1 ||
		    ld->special <= OdamexStaticInits + NUM_STATIC_INITS)
		{
			sides[*ld->sidenum].special = ld->special;
			sides[*ld->sidenum].tag = ld->args[0];
		}
		else
		{
			sides[*ld->sidenum].special = 0;
		}
	}
}

// killough 4/4/98: delay using sidedefs until they are loaded
void P_FinishLoadingLineDefs (void)
{
	line_t *ld = lines;

	for (int i = numlines, linenum = 0; i--; ld++, linenum++)
	{
		ld->frontsector = ld->sidenum[0]!=R_NOSIDE ? sides[ld->sidenum[0]].sector : 0;
		ld->backsector  = ld->sidenum[1]!=R_NOSIDE ? sides[ld->sidenum[1]].sector : 0;
		if (ld->sidenum[0] != R_NOSIDE)
			sides[ld->sidenum[0]].linenum = linenum;
		if (ld->sidenum[1] != R_NOSIDE)
			sides[ld->sidenum[1]].linenum = linenum;

		map_format.post_process_linedef_special(ld);
	}
}

void P_LoadLineDefs (const int lump)
{
	numlines = W_LumpLength (lump) / sizeof(maplinedef_t);
	lines = (line_t *)Z_Malloc (numlines*sizeof(line_t), PU_LEVEL, 0);
	memset (lines, 0, numlines*sizeof(line_t));
	byte* data = (byte *)W_CacheLumpNum (lump, PU_STATIC);
	auto guard = nonstd::make_scope_exit([&]{ Z_Free(data); });

	const bool reservedLine = P_GetLevelCompData(::level.level_fingerprint).reservedLineFlag;

	line_t* ld = lines;
	for (int i = 0; i < numlines; i++, ld++)
	{
		const maplinedef_t *mld = ((maplinedef_t *)data) + i;

		ld->flags = static_cast<uint32_t>(mld->flags);
		ld->special = mld->special;
		ld->id = mld->tag;
		ld->args[0] = 0;
		ld->args[1] = 0;
		ld->args[2] = 0;
		ld->args[3] = 0;
		ld->args[4] = 0;

		ld->flags = P_TranslateCompatibleLineFlags(ld->flags, reservedLine);

		uint16_t v = LESHORT(mld->v1);

		if(v >= numvertexes)
			I_Error("P_LoadLineDefs: invalid vertex {}", v);
		else
			ld->v1 = &vertexes[v];

		v = LESHORT(mld->v2);

		if(v >= numvertexes)
			I_Error("P_LoadLineDefs: invalid vertex {}", v);
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
}

// [RH] Same as P_LoadLineDefs() except it uses Hexen-style LineDefs.
void P_LoadLineDefs2 (int lump)
{
	numlines = W_LumpLength (lump) / sizeof(maplinedef2_t);
	lines = (line_t *)Z_Malloc (numlines*sizeof(line_t), PU_LEVEL,0 );
	memset (lines, 0, numlines*sizeof(line_t));
	byte* data = (byte *)W_CacheLumpNum (lump, PU_STATIC);

	maplinedef2_t* mld = (maplinedef2_t *)data;
	line_t* ld = lines;
	for (int i = 0; i < numlines; i++, mld++, ld++)
	{
		for (int j = 0; j < 5; j++)
			ld->args[j] = mld->args[j];

		ld->flags = LESHORT(mld->flags);
		ld->special = mld->special;

		ld->flags = P_TranslateZDoomLineFlags(ld->flags);

		uint16_t v = LESHORT(mld->v1);

		if(v >= numvertexes)
			I_Error("P_LoadLineDefs2: invalid vertex {}", v);
		else
			ld->v1 = &vertexes[v];

		v = LESHORT(mld->v2);

		if(v >= numvertexes)
			I_Error("P_LoadLineDefs2: invalid vertex {}", v);
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


//
// P_GetColorFromTextureName
//
// Converts a texture name to an ARGB8888 value.
// The texture name should contain 4 hexadecimal byte values
// in the following order: alpha, red, green, blue.
//
argb_t P_GetColorFromTextureName(const char* name)
{
	// work around name not being a properly terminated string
	const OLumpName name2 = name;

	unsigned long value = strtoul(name2.c_str(), NULL, 16);

	const int a = (value >> 24) & 0xFF;
	const int r = (value >> 16) & 0xFF;
	const int g = (value >> 8) & 0xFF;
	const int b = value & 0xFF;

	return argb_t(a, r, g, b);
}

// killough 4/4/98: delay using texture names until
// after linedefs are loaded, to allow overloading.
// killough 5/3/98: reformatted, cleaned up

void P_LoadSideDefs2 (int lump)
{
	byte* data = (byte*)W_CacheLumpNum(lump, PU_STATIC);

	for (int i = 0; i < numsides; i++)
	{
		mapsidedef_t* msd = (mapsidedef_t*)data + i;
		side_t* sd = sides + i;
		sector_t* sec;

		sd->textureoffset = LESHORT(msd->textureoffset)<<FRACBITS;
		sd->rowoffset = LESHORT(msd->rowoffset)<<FRACBITS;
		sd->linenum = -1;
		sd->sector = sec = &sectors[LESHORT(msd->sector)];

		// killough 4/4/98: allow sidedef texture names to be overloaded
		// killough 4/11/98: refined to allow colormaps to work as wall
		// textures if invalid as colormaps but valid as textures.

		map_format.post_process_sidedef_special(sd, msd, sec, i);
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

struct linelist_t        // type used to list lines in each block
{
	int	num;
	linelist_t *next;
};

//
// Actually construct the blockmap lump from the level data
//
// This finds the intersection of each linedef with the column and
// row lines at the left and bottom of each blockmap cell. It then
// adds the line to all block lists touching the intersection.
//

void P_CreateBlockMap()
{
	std::unique_ptr<linelist_t*[]> blocklists; // array of pointers to lists of lines
	std::unique_ptr<int[]> blockcount; // array of counters of line lists
	std::unique_ptr<bool[]> blockdone; // array keeping track of blocks/line

	//
	// Subroutine to add a line number to a block list
	// It simply returns if the line is already in the block
	//

	const auto AddBlockLine = [&blocklists, &blockdone, &blockcount]
	(
		int blockno,
		uint32_t lineno
	)
	{
		if (blockdone[blockno])
			return;

		linelist_t* l = new linelist_t;
		l->num = lineno;
		l->next = blocklists[blockno];
		blocklists[blockno] = l;
		blockcount[blockno]++;
		blockdone[blockno] = true;
	};

	// scan for map limits, which the blockmap must enclose
	int map_minx = limits::MAXINT;
	int map_miny = limits::MAXINT;
	int map_maxx = limits::MININT;
	int map_maxy = limits::MININT;
	for (int i = 0; i < numvertexes; i++)
	{
		fixed_t t;

		if ((t = vertexes[i].x) < map_minx)
			map_minx = t;
		else if (t > map_maxx)
			map_maxx = t;

		if ((t = vertexes[i].y) < map_miny)
			map_miny = t;
		else if (t > map_maxy)
			map_maxy = t;
	}
	map_minx >>= FRACBITS;    // work in map coords, not fixed_t
	map_maxx >>= FRACBITS;
	map_miny >>= FRACBITS;
	map_maxy >>= FRACBITS;

	// set up blockmap area to enclose level plus margin

	const int xorg = map_minx-blkmargin; // blockmap origin (lower left)
	const int yorg = map_miny-blkmargin;
	const int ncols = (map_maxx+blkmargin-xorg+1+blkmask)>>blkshift; //jff 10/12/98
	const int nrows = (map_maxy+blkmargin-yorg+1+blkmask)>>blkshift; //+1 needed for map exactly 1 cell

	const auto BlockIndex = [ncols](int x, int y){ return (y * ncols) + x; };

	const int NBlocks = ncols*nrows; // number of cells

	// create the array of pointers on NBlocks to blocklists
	// also create an array of linelist counts on NBlocks
	// finally make an array in which we can mark blocks done per line

	blocklists = std::make_unique<linelist_t*[]>(NBlocks);
	std::fill_n(blocklists.get(), NBlocks, nullptr);
	blockcount = std::make_unique<int[]>(NBlocks);
	std::fill_n(blockcount.get(), NBlocks, 0);
	blockdone = std::make_unique<bool[]>(NBlocks);

	// initialize each blocklist, and enter the trailing -1 in all blocklists
	// note the linked list of lines grows backwards

	for (int i = 0; i < NBlocks; i++)
	{
		blocklists[i] = new linelist_t;
		blocklists[i]->num = -1;
		blocklists[i]->next = NULL;
		blockcount[i]++;
	}

	// For each linedef in the wad, determine all blockmap blocks it touches,
	// and add the linedef number to the blocklists for those blocks

	for (int i = 0; i < numlines; i++)
	{
		const int x1 = lines[i].v1->x>>FRACBITS; // lines[i] map coords
		const int y1 = lines[i].v1->y>>FRACBITS;
		const int x2 = lines[i].v2->x>>FRACBITS;
		const int y2 = lines[i].v2->y>>FRACBITS;
		const int dx = x2 - x1;
		const int dy = y2 - y1;
		const bool vert = (dx == 0);             // lines[i] slopetype
		const bool horiz = (dy == 0);
		const bool spos = (dx ^ dy) > 0;
		const bool sneg = (dx ^ dy) < 0;
		int bx,by;                              // block cell coords
		const int minx = x1 > x2 ? x2 : x1;        // extremal lines[i] coords
		const int maxx = x1 > x2 ? x1 : x2;
		const int miny = y1 > y2 ? y2 : y1;
		const int maxy = y1 > y2 ? y1 : y2;

		// no blocks done for this linedef yet

		std::fill_n(blockdone.get(), NBlocks, false);

		// The line always belongs to the blocks containing its endpoints

		bx = (x1-xorg) >> blkshift;
		by = (y1-yorg) >> blkshift;
		AddBlockLine (BlockIndex(bx, by), i);
		bx = (x2-xorg) >> blkshift;
		by = (y2-yorg) >> blkshift;
		AddBlockLine (BlockIndex(bx, by), i);

		// For each column, see where the line along its left edge, which
		// it contains, intersects the Linedef i. Add i to each corresponding
		// blocklist.

		if (!vert)    // don't interesect vertical lines with columns
		{
			for (int j = 0; j < ncols; j++)
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

				AddBlockLine(BlockIndex(j, yb), i);

				// if the intersection is at a corner it depends on the slope
				// (and whether the line extends past the intersection) which
				// blocks are hit

				if (yp==0)			// intersection at a corner
				{
					if (sneg)		//   \ - blocks x,y-, x-,y
					{
						if (yb>0 && miny<y)
							AddBlockLine(BlockIndex(j, yb - 1), i);
						if (j>0 && minx<x)
							AddBlockLine(BlockIndex(j - 1, yb), i);
					}
					else if (spos)	//   / - block x-,y-
					{
						if (yb>0 && j>0 && minx<x)
							AddBlockLine(BlockIndex(j - 1, yb - 1), i);
					}
					else if (horiz)	//   - - block x-,y
					{
						if (j>0 && minx<x)
							AddBlockLine(BlockIndex(j - 1, yb), i);
					}
				}
				else if (j>0 && minx<x)	// else not at corner: x-,y
					AddBlockLine(BlockIndex(j - 1, yb), i);
			}
		}

		// For each row, see where the line along its bottom edge, which
		// it contains, intersects the Linedef i. Add i to all the corresponding
		// blocklists.

		if (!horiz)
		{
			for (int j = 0; j < nrows; j++)
			{
				// intersection of Linedef with y=yorg+(j<<blkshift)
				// (x,y) on Linedef i satisfies: (y-y1)*dx = dy*(x-x1)
				// x = dx*(y-y1)/dy+x1;

				const int y = yorg+(j<<blkshift);		// (x,y) is intersection
				const int x = (dx*(y-y1))/dy+x1;
				const int xb = (x-xorg)>>blkshift;	// block column number
				const int xp = (x-xorg)&blkmask;		// x position within block

				if (xb<0 || xb>ncols-1)			// outside blockmap, continue
					continue;

				if (y<miny || y>maxy)			 // line doesn't touch row
					continue;

				// The cell that contains the intersection point is always added

				AddBlockLine (BlockIndex(xb, j), i);

				// if the intersection is at a corner it depends on the slope
				// (and whether the line extends past the intersection) which
				// blocks are hit

				if (xp==0)			// intersection at a corner
				{
					if (sneg)       //   \ - blocks x,y-, x-,y
					{
						if (j>0 && miny<y)
							AddBlockLine (BlockIndex(xb, j - 1), i);
						if (xb>0 && minx<x)
							AddBlockLine (BlockIndex(xb - 1, j), i);
					}
					else if (vert)  //   | - block x,y-
					{
						if (j>0 && miny<y)
							AddBlockLine (BlockIndex(xb, j - 1), i);
					}
					else if (spos)  //   / - block x-,y-
					{
						if (xb>0 && j>0 && miny<y)
							AddBlockLine (BlockIndex(xb - 1, j - 1), i);
					}
				}
				else if (j>0 && miny<y) // else not on a corner: x,y-
					AddBlockLine (BlockIndex(xb, j - 1), i);
			}
		}
	}

	// Add initial 0 to all blocklists
	// count the total number of lines (and 0's and -1's)
	std::fill_n(blockdone.get(), NBlocks, false);
	uint32_t linetotal = 0;
	for (int i = 0; i < NBlocks; i++)
	{
		AddBlockLine (i, 0);
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
	for (int i = 0; i < NBlocks; i++)
	{
		linelist_t *bl = blocklists[i];
		uint32_t offs = blockmaplump[4+i] =   // set offset to block's list
			(i? blockmaplump[4+i-1] : 4+NBlocks) + (i? blockcount[i-1] : 0);

		// add the lines in each block's list to the blockmaplump
		// delete each list node as we go

		while (bl)
		{
			linelist_t *tmp = bl->next;
			blockmaplump[offs++] = bl->num;
			delete bl;
			bl = tmp;
		}
	}
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
		blockmaplump[2] = (uint32_t)(LESHORT(wadblockmaplump[2])) & 0xffff;
		blockmaplump[3] = (uint32_t)(LESHORT(wadblockmaplump[3])) & 0xffff;

		for (i=4 ; i<count ; i++)
		{
			short t = LESHORT(wadblockmaplump[i]);          // killough 3/1/98
			blockmaplump[i] = t == -1 ? (uint32_t)0xffffffff : (uint32_t) t & 0xffff;
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

/*
* @brief P_GenerateUniqueMapFingerPrint
*
* Creates a unique map fingerprint used to identify a unique map.
* Based on a few key lumps that makes a map unique.
*
* @param maplumpnum - Lump offset number of the specified map
* If it is, use it as part of the map calculation.
*/
void P_GenerateUniqueMapFingerPrint(int maplumpnum)
{
	unsigned int length = 0;

	typedef std::vector<byte> LevelLumps;
	LevelLumps levellumps;

	const byte* thingbytes = static_cast<const byte*>(W_CacheLumpNum(maplumpnum+ML_THINGS, PU_STATIC));
	const byte* lindefbytes = static_cast<const byte*>(W_CacheLumpNum(maplumpnum+ML_LINEDEFS, PU_STATIC));
	const byte* sidedefbytes = static_cast<const byte*>(W_CacheLumpNum(maplumpnum+ML_SIDEDEFS, PU_STATIC));
	const byte* vertexbytes = static_cast<const byte*>(W_CacheLumpNum(maplumpnum+ML_VERTEXES, PU_STATIC));
	const byte* segsbytes = static_cast<const byte*>(W_CacheLumpNum(maplumpnum+ML_SEGS, PU_STATIC));
	const byte* ssectorsbytes = static_cast<const byte*>(W_CacheLumpNum(maplumpnum+ML_SSECTORS, PU_STATIC));
	const byte* sectorsbytes = static_cast<const byte*>(W_CacheLumpNum(maplumpnum+ML_SECTORS, PU_STATIC));

	levellumps.insert(levellumps.end(), W_LumpLength(maplumpnum+ML_THINGS), *thingbytes);
	levellumps.insert(levellumps.end(), W_LumpLength(maplumpnum+ML_LINEDEFS), *lindefbytes);
	levellumps.insert(levellumps.end(), W_LumpLength(maplumpnum+ML_SIDEDEFS), *sidedefbytes);
	levellumps.insert(levellumps.end(), W_LumpLength(maplumpnum+ML_VERTEXES), *vertexbytes);
	levellumps.insert(levellumps.end(), W_LumpLength(maplumpnum+ML_SEGS), *segsbytes);
	levellumps.insert(levellumps.end(), W_LumpLength(maplumpnum+ML_SSECTORS), *ssectorsbytes);
	levellumps.insert(levellumps.end(), W_LumpLength(maplumpnum+ML_SECTORS), *sectorsbytes);

	length = W_LumpLength(maplumpnum+ML_THINGS) + W_LumpLength(maplumpnum+ML_LINEDEFS) +
	         W_LumpLength(maplumpnum+ML_SIDEDEFS) + W_LumpLength(maplumpnum+ML_VERTEXES) +
			 W_LumpLength(maplumpnum + ML_SEGS) + W_LumpLength(maplumpnum + ML_SSECTORS) +
			 W_LumpLength(maplumpnum + ML_SECTORS);

	fhfprint_t fingerprint = W_FarmHash128(levellumps.data(), length);

	::level.level_fingerprint = fingerprint;
}
//
// P_GroupLines
// Builds sector line lists and subsector sector numbers.
// Finds block bounding boxes for sectors.
//
int P_GroupLines()
{
	// look up sector number for each subsector
	for (int i = 0; i < numsubsectors; i++)
	{
		if (subsectors[i].firstline >= (unsigned int)numsegs)
			I_Error("subsector[{}].firstline exceeds numsegs ({})", i, numsegs);
		subsectors[i].sector = segs[subsectors[i].firstline].sidedef->sector;
	}

	// count number of lines in each sector
	int total = 0;
	for (auto& line : R_GetLines())
	{
		total++;
		if (!line.frontsector && line.backsector)
		{
			// swap front and backsectors if a one-sided linedef
			// does not have a front sector
			line.frontsector = line.backsector;
			line.backsector = nullptr;
		}

        if (line.frontsector)
            line.frontsector->linecount++;

		if (line.backsector && line.backsector != line.frontsector)
		{
			line.backsector->linecount++;
			total++;
		}
	}

	// build line tables for each sector
	line_t** linebuffer = (line_t **)Z_Malloc (total*sizeof(line_t *), PU_LEVEL, 0);
	sector_t* sector = sectors;
	DBoundingBox bbox;
	for (int i = 0 ; i < numsectors ; i++, sector++)
	{
		bbox.ClearBox ();
		sector->lines = linebuffer;
		for (auto& line : R_GetLines())
		{
			if (line.frontsector == sector || line.backsector == sector)
			{
				*linebuffer++ = &line;
				bbox.AddToBox (line.v1->x, line.v1->y);
				bbox.AddToBox (line.v2->x, line.v2->y);
			}
		}
		if (linebuffer - sector->lines != sector->linecount)
			I_Error("P_GroupLines: miscounted");

		// set the soundorg to the middle of the bounding box
		sector->soundorg[0] = (bbox.Right()+bbox.Left())/2;
		sector->soundorg[1] = (bbox.Top()+bbox.Bottom())/2;

		// adjust bounding box to map blocks
		int block = (bbox.Top()-bmaporgy+MAXRADIUS)>>MAPBLOCKSHIFT;
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
	return total;
}

//
// P_RemoveSlimeTrails()
//
// killough 10/98
//
// Slime trails are inherent to Doom's coordinate system -- i.e. there is
// nothing that a node builder can do to prevent slime trails ALL of the time,
// because it's a product of the integer coodinate system, and just because
// two lines pass through exact integer coordinates, doesn't necessarily mean
// that they will intersect at integer coordinates. Thus we must allow for
// fractional coordinates if we are to be able to split segs with node lines,
// as a node builder must do when creating a BSP tree.
//
// A wad file does not allow fractional coordinates, so node builders are out
// of luck except that they can try to limit the number of splits (they might
// also be able to detect the degree of roundoff error and try to avoid splits
// with a high degree of roundoff error). But we can use fractional coordinates
// here, inside the engine. It's like the difference between square inches and
// square miles, in terms of granularity.
//
// For each vertex of every seg, check to see whether it's also a vertex of
// the linedef associated with the seg (i.e, it's an endpoint). If it's not
// an endpoint, and it wasn't already moved, move the vertex towards the
// linedef by projecting it using the law of cosines. Formula:
//
//      2        2                         2        2
//    dx  x0 + dy  x1 + dx dy (y0 - y1)  dy  y0 + dx  y1 + dx dy (x0 - x1)
//   {---------------------------------, ---------------------------------}
//                  2     2                            2     2
//                dx  + dy                           dx  + dy
//
// (x0,y0) is the vertex being moved, and (x1,y1)-(x1+dx,y1+dy) is the
// reference linedef.
//
// Segs corresponding to orthogonal linedefs (exactly vertical or horizontal
// linedefs), which comprise at least half of all linedefs in most wads, don't
// need to be considered, because they almost never contribute to slime trails
// (because then any roundoff error is parallel to the linedef, which doesn't
// cause slime). Skipping simple orthogonal lines lets the code finish quicker.
//
// Please note: This section of code is not interchangable with TeamTNT's
// code which attempts to fix the same problem.
//
// Firelines (TM) is a Rezistered Trademark of MBF Productions
//

void P_RemoveSlimeTrails()
{
	byte* hit = (byte *)Z_Malloc(numvertexes, PU_LEVEL, 0);
	memset(hit, 0, numvertexes * sizeof(byte));

	for (int i = 0; i < numsegs; i++)
	{
		const line_t *l = segs[i].linedef;		// The parent linedef

		// We can ignore orthogonal lines
		if (l->slopetype != ST_VERTICAL && l->slopetype != ST_HORIZONTAL)
		{
			vertex_t *v = segs[i].v1;
			do
			{
				if (!hit[v - vertexes])				// If we haven't processed vertex
				{
					hit[v - vertexes] = 1;			// Mark this vertex as processed
					if (v != l->v1 && v != l->v2)	// Exclude endpoints of linedefs
					{
						// Project the vertex back onto the parent linedef
						const int64_t dx2 = (l->dx >> FRACBITS) * (l->dx >> FRACBITS);
						const int64_t dy2 = (l->dy >> FRACBITS) * (l->dy >> FRACBITS);
						const int64_t dxy = (l->dx >> FRACBITS) * (l->dy >> FRACBITS);
						const int64_t s = dx2 + dy2;
						const fixed_t x0 = v->x, y0 = v->y, x1 = l->v1->x, y1 = l->v1->y;
						v->x = (fixed_t)((dx2 * x0 + dy2 * x1 + dxy * (y0 - y1)) / s);
						v->y = (fixed_t)((dy2 * y0 + dx2 * y1 + dxy * (x0 - x1)) / s);
					}
				}  // Obsfucated C contest entry:   :)
			} while ((v != segs[i].v2) && (v = segs[i].v2));
		}
	}

	Z_Free(hit);
}

//
// [RH] P_LoadBehavior
//
void P_LoadBehavior (int lumpnum)
{
	byte *behavior = (byte *)W_CacheLumpNum (lumpnum, PU_LEVEL);

	level.behavior = std::make_unique<FBehavior>(behavior, lumpinfo[lumpnum].size);

	if (!level.behavior->IsGood ())
	{
		level.behavior.reset();
	}
}

// Hash the sector tags across the sectors and linedefs.
void P_InitTagLists(void)
{
	for (int i = numsectors; --i >= 0; )		// Initially make all slots empty.
		sectors[i].firsttag = -1;
	for (int i = numsectors; --i >= 0; )		// Proceed from last to first sector
	{									// so that lower sectors appear first
		int j = (unsigned)sectors[i].tag % (unsigned)numsectors;	// Hash func
		sectors[i].nexttag = sectors[j].firsttag;	// Prepend sector to chain
		sectors[j].firsttag = i;
	}

	// killough 4/17/98: same thing, only for linedefs

	for (int i = numlines; --i >= 0; )			// Initially make all slots empty.
		lines[i].firstid = -1;
	for (int i = numlines; --i >= 0; )        // Proceed from last to first linedef
	{									// so that lower linedefs appear first
		int j = (unsigned)lines[i].id % (unsigned)numlines;	// Hash func
		lines[i].nextid = lines[j].firstid;	// Prepend linedef to chain
		lines[j].firstid = i;
	}
}


void P_SetupLevelFloorPlane(sector_t *sector)
{
	if (!sector)
		return;

	sector->floorplane.a = sector->floorplane.b = 0;
	sector->floorplane.c = sector->floorplane.invc = FRACUNIT;
	sector->floorplane.d = -sector->floorheight;
	sector->floorplane.texx = sector->floorplane.texy = 0;
	sector->floorplane.sector = sector;
}

void P_SetupLevelCeilingPlane(sector_t *sector)
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
void P_SetupPlane(sector_t* sec, line_t* line, bool floor)
{
	if (!sec || !line || !line->backsector)
		return;

	// Find the vertex comprising the sector that is farthest from the
	// slope's reference line

	int bestdist = 0;
	line_t** probe = sec->lines;
	vertex_t *refvert = (*sec->lines)->v1;

	for (int i = sec->linecount*2; i > 0; i--)
	{
		vertex_t *vert;

		// Do calculations with only the upper bits, because the lower ones
		// are all zero, and we would overflow for a lot of distances if we
		// kept them around.

		if (i & 1)
			vert = (*probe++)->v2;
		else
			vert = (*probe)->v1;
		const int dist = abs (((line->v1->y - vert->y) >> FRACBITS) * (line->dx >> FRACBITS) -
		                      ((line->v1->x - vert->x) >> FRACBITS) * (line->dy >> FRACBITS));

		if (dist > bestdist)
		{
			bestdist = dist;
			refvert = vert;
		}
	}

	const sector_t* refsec = line->frontsector == sec ? line->backsector : line->frontsector;
	plane_t* srcplane = floor ? &sec->floorplane : &sec->ceilingplane;
	const fixed_t srcheight = floor ? sec->floorheight : sec->ceilingheight;
	const fixed_t destheight = floor ? refsec->floorheight : refsec->ceilingheight;

	v3float_t p, v1, v2, cross;
	M_SetVec3f(&p, line->v1->x, line->v1->y, destheight);
	M_SetVec3f(&v1, line->dx, line->dy, 0);
	M_SetVec3f(&v2, refvert->x - line->v1->x, refvert->y - line->v1->y, srcheight - destheight);

	M_CrossProductVec3f(&cross, &v1, &v2);
	M_NormalizeVec3f(&cross, &cross);

	// Fix backward normals
	if ((cross.z < 0 && floor == true) || (cross.z > 0 && floor == false))
	{
		cross.x = -cross.x;
		cross.y = -cross.y;
		cross.z = -cross.z;
	}

	srcplane->a = FLOAT2FIXED(cross.x);
	srcplane->b = FLOAT2FIXED(cross.y);
	srcplane->c = FLOAT2FIXED(cross.z);
	srcplane->invc = FLOAT2FIXED(1.f/cross.z);
	srcplane->d = -FixedMul(srcplane->a, line->v1->x) - FixedMul(srcplane->b, line->v1->y) - FixedMul(srcplane->c, destheight);
	srcplane->texx = refvert->x;
	srcplane->texy = refvert->y;
}

void P_SetupSlopes()
{
	for (line_t& line : R_GetLines())
	{
		if ((map_format.getZDoom() && line.special == Plane_Align) ||
		    (line.special >= 340 && line.special <= 347))
		{
			line.special = 0;
			line.id = line.args[2];

			// Floor plane?
			int align_side = line.args[0] & 3;
			if (align_side == 1)
				P_SetupPlane(line.frontsector, &line, true);
			else if (align_side == 2)
				P_SetupPlane(line.backsector, &line, true);

			// Ceiling plane?
			align_side = line.args[1] & 3;
			if (align_side == 0)
				align_side = (line.args[0] >> 2) & 3;

			if (align_side == 1)
				P_SetupPlane(line.frontsector, &line, false);
			else if (align_side == 2)
				P_SetupPlane(line.backsector, &line, false);
		}
	}
}

void P_LoadReject(int lumpnum, int totallines)
{
	// [SL] 2011-07-01 - Check to see if the reject table is of the proper size
	// If it's empty, the reject table should be ignored when
	// calling P_CheckSight
	// [EB] and if exists, but is too small, pad it with 0s until its the right size
	const auto lumpsize = W_LumpLength(lumpnum);
	const uint32_t correctsize = (numsectors * numsectors + 7) / 8;
	// TODO: if we end up using reject to optimize netcodea and it makes a significant difference,
	// build reject lumps when its completely empty
	if (!demoplayback && lumpsize == 0)
	{
		DPrintFmt("Reject matrix is empty and will be ignored.\n");
		rejectempty = true;
	}
	else if (lumpsize < correctsize)
	{
		DPrintFmt("Reject matrix is not valid. It will be padded to the correct size.\n");
		rejectmatrix = static_cast<byte*>(Z_Malloc(correctsize, PU_LEVEL, nullptr));
		W_ReadLump(lumpnum, rejectmatrix);
		memset(rejectmatrix + lumpsize, 0, correctsize - lumpsize);
		// vanilla doom just reads pass the edge of the reject table if its too small
		// so we replace the start of the padding with what would likely have been in memory
		// when playing on MS-DOS
		if (demoplayback) {
			uint32_t rejectpad[4] =
			{
				0,       // Size
				0,       // Part of z_zone block header
				50,      // PU_LEVEL
				0x1d4a11 // DOOM_CONST_ZONEID
			};

			rejectpad[0] = ((totallines * 4 + 3) & ~3) + 24;
			byte* dest = rejectmatrix + lumpsize;

			for (uint32_t i = 0; i < (correctsize - lumpsize) && i < sizeof(rejectpad); i++)
			{
				uint32_t byte_num = i % 4;
				*dest = (rejectpad[i / 4] >> (byte_num * 8)) & 0xff;
				dest++;
			}
		}
	}
	else
	{
		rejectmatrix = static_cast<byte*>(W_CacheLumpNum(lumpnum, PU_LEVEL));
	}
}

} // namespace

//
// P_SetupLevel
//
extern polyblock_t **PolyBlockMap;

// [RH] position indicates the start spot to spawn at
void P_SetupLevel (const char *lumpname, int position)
{
	level.total_monsters = level.respawned_monsters = level.total_items = level.total_secrets =
		level.killed_monsters = level.found_items = level.found_secrets =
		wminfo.maxfrags = 0;
	level.level_fingerprint.clear();
	wminfo.partime = 180;

	if (!savegamerestore)
	{
		for (auto& player : players)
		{
			player.killcount = player.secretcount = player.itemcount = 0;
		}
	}

	// To use the correct nodes for

	// Initial height of PointOfView will be set by player think.
	consoleplayer().viewz = 1;

	// Make sure all sounds are stopped before Z_FreeTags.
	S_Start ();

	S_ClearMusInfo();

	// [RH] Clear all ThingID hash chains.
	AActor::ClearTIDHashes ();

	// [RH] clear out the mid-screen message
	C_MidPrint (NULL);

	PolyBlockMap = NULL;

	// [AM] So shootthing isn't a wild pointer on map swtich.
	shootthing = NULL;

	DThinker::DestroyAllThinkers ();
	Z_FreeTags (PU_LEVEL, PU_LEVELMAX);
	g_ValidLevel = false;		// [AM] False until the level is loaded.
	NormalLight.next = NULL;	// [RH] Z_FreeTags frees all the custom colormaps

	// [AM] Every new level starts with fresh netids.
	P_ClearAllNetIds();

	P_ClearHelpers();

	// UNUSED W_Profile ();

	// find map num
	const int lumpnum = W_GetNumForName (lumpname);

	// [RH] Check if this map is Hexen-style.
	//		LINEDEFS and THINGS need to be handled accordingly.
	//		If it is, we also need to distinguish between projectile cross and hit
	HasBehavior = W_CheckLumpName (lumpnum+ML_BEHAVIOR, "BEHAVIOR");
	//oldshootactivation = !HasBehavior;

	// note: most of this ordering is important

	// [RH] Load in the BEHAVIOR lump
	if (level.behavior != NULL)
	{
		level.behavior.reset();
	}

	// [Blair] Create map fingerprint
	P_GenerateUniqueMapFingerPrint(lumpnum);

	if (HasBehavior)
	{
		P_LoadBehavior (lumpnum+ML_BEHAVIOR);
		map_format.P_ApplyZDoomMapFormat();
	}
	else
	{
		map_format.P_ApplyDefaultMapFormat();
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

	const nodetype_t nodetype = W_LumpLength(lumpnum+ML_NODES) > 0 ?
	                            P_CheckNodeType(lumpnum+ML_NODES) :
	                            P_CheckNodeType(lumpnum+ML_SSECTORS);

	switch (nodetype) {
		case nodetype_t::XNOD:
		case nodetype_t::ZNOD:
			P_LoadExtendedNodes(lumpnum+ML_NODES, nodetype);
			break;

		case nodetype_t::XGLN:
		case nodetype_t::ZGLN:
		case nodetype_t::XGL2:
		case nodetype_t::ZGL2:
		case nodetype_t::XGL3:
		case nodetype_t::ZGL3:
			P_LoadExtendedNodes(lumpnum+ML_SSECTORS, nodetype);
			break;

		case nodetype_t::DEEP:
			P_LoadSubsectors<mapsubsector_deepbsp_t>(lumpnum+ML_SSECTORS);
			P_LoadNodes<mapnode_deepbsp_t>(lumpnum+ML_NODES);
			P_LoadSegs<mapseg_deepbsp_t>(lumpnum+ML_SEGS);
			break;

		default:
			P_LoadSubsectors<mapsubsector_t>(lumpnum+ML_SSECTORS);
			P_LoadNodes<mapnode_t>(lumpnum+ML_NODES);
			P_LoadSegs<mapseg_t>(lumpnum+ML_SEGS);
	}

	P_LoadReject(lumpnum + ML_REJECT, P_GroupLines());

	// [SL] don't move seg vertices if compatibility is cruical
	if (!demoplayback)
		P_RemoveSlimeTrails();

	P_SetupSlopes();

    po_NumPolyobjs = 0;

	P_InitTagLists();   // killough 1/30/98: Create xref tables for tags

	if (!HasBehavior)
		P_LoadThings (lumpnum+ML_THINGS);
	else
		P_LoadThings2 (lumpnum+ML_THINGS, position);	// [RH] Load Hexen-style things

	if (!HasBehavior)
		P_TranslateTeleportThings(); // [RH] Assign teleport destination TIDs

    PO_Init ();

    if (serverside)
    {
		for (auto& player : players)
		{
			SV_PreservePlayer(player);

			if (player.ingame())
			{
				// if deathmatch, randomly spawn the active players
				// denis - this function checks for deathmatch internally
				G_DeathMatchSpawnPlayer(player);
			}
		}
    }

	// clear special respawning queue
	itemrespawnque = {};

	// killough 3/26/98: Spawn icon landings:
	P_SpawnBrainTargets();

	// set up world state
	P_SetupWorldState();

	P_SetupHelpers();

	// build subsector connect matrix
	//	UNUSED P_ConnectSubsectors ();

#ifdef CLIENT_APP
	// preload graphics
	if (precache)
		R_PrecacheLevel ();
#endif

	// [AM] Level is now safely loaded.
	g_ValidLevel = true;
}

// c++11 semantics moves vector on return
static std::vector<spriteinfo_t*> P_GetSpriteInfos ()
{
	std::vector<spriteinfo_t*> infos;
	for(auto it = sprnames.begin();it != sprnames.end();++it)
	{
		spriteinfo_t* spriteinfo = (spriteinfo_t*) Z_Malloc(sizeof(spriteinfo_t), PU_STATIC, nullptr);
		spriteinfo->sprite = Z_StrDup(it->second.data(), PU_STATIC);
		spriteinfo->spritenum = it->first;
		infos.push_back(spriteinfo);
	}
	std::sort(infos.begin(), infos.end(), [](spriteinfo_t* lhs, spriteinfo_t* rhs) {
		return lhs->spritenum < rhs->spritenum;
	});
	return infos;
}

//
// P_Init
//
void P_Init (void)
{
	P_InitSwitchList ();
	P_InitPicAnims ();
	// code below ASSUMES the sprites are in-order rather than passing an order down-ward
	std::vector<spriteinfo_t*> infos = P_GetSpriteInfos ();
	R_InitSprites(infos);
	InitTeamInfo();
	P_InitHorde();
}

//
// P_SetTransferHeightBlends
//
// Reads the texture name from the mapsidedef for the given side. If the
// texture name matches the name of a valid Boom colormap lump, the
// sidedef's texture value is cleared and the colormap's blend color
// value is used for the appropriate sector blend. If the texture name
// is an ARGB value in hexadecimal, that value is used for the appropriate
// sector blend.
//
void P_SetTransferHeightBlends(side_t* sd, const mapsidedef_t* msd)
{
	sector_t* sec = &sectors[LESHORT(msd->sector)];

	// for each of the texture tiers (bottom, middle, and top)
	for (int i = 0; i < 3; i++)
	{
		short* texture_num;
		argb_t* blend_color;
		const char* texture_name;

		if (i == 0)				// bottom textures
		{
			texture_num = &sd->bottomtexture;
			blend_color = &sec->bottommap;
			texture_name = msd->bottomtexture;
		}
		else if (i == 1)		// mid textures
		{
			texture_num = &sd->midtexture;
			blend_color = &sec->midmap;
			texture_name = msd->midtexture;
		}
		else					// top textures
		{
			texture_num = &sd->toptexture;
			blend_color = &sec->topmap;
			texture_name = msd->toptexture;
		}

		*blend_color = argb_t(0, 255, 255, 255);
		*texture_num = 0;

		int colormap_index = R_ColormapNumForName(texture_name);
		if (colormap_index != 0)
		{
			*blend_color = R_BlendForColormap(colormap_index);
		}
		else
		{
			*texture_num = R_CheckTextureNumForName(texture_name);
			if (*texture_num == -1)
			{
				*texture_num = 0;
				if (strnicmp(texture_name, "WATERMAP", 8) == 0)
					*blend_color = argb_t(0x80, 0, 0x4F, 0xA5);
				else
					*blend_color = P_GetColorFromTextureName(texture_name);
			}
		}
	}
}

//

void SetTextureNoErr (short *texture, unsigned int *color, char *name)
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

CVAR_FUNC_IMPL(sv_intermissionlimit)
{
	if (G_IsCoopGame() && var < 10) {
		var.Set(10.0);	// Force to 10 seconds minimum
	} else if (var < 1) {
		var.RestoreDefault();
	}

	level.inttimeleft = var;
}

VERSION_CONTROL (p_setup_cpp, "$Id$")
