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
//      Refresh/rendering module, shared data struct definitions.
//
//-----------------------------------------------------------------------------

#pragma once

// Screenwidth.

// Some more or less basic data types
// we depend on.
#include "m_fixed.h"

// We rely on the thinker data struct
// to handle sound origins in sectors.
// SECTORS do store MObjs anyway.
#include "actor.h"

#include "dthinker.h"

// Silhouette, needed for clipping Segs (mainly)
// and sprites representing things.
#define SIL_NONE				0
#define SIL_BOTTOM				1
#define SIL_TOP 				2
#define SIL_BOTH				3

extern int MaxDrawSegs;

// The size of an 8K 4:3 display.
#define MAXWIDTH				8192
#define MAXHEIGHT				6144

//
// INTERNAL MAP TYPES
//	used by play and refresh
//

//
// The SECTORS record, at runtime.
// Stores things/mobjs.
//

#define NO_TOPTEXTURES             0x00000001
#define NO_BOTTOMTEXTURES          0x00000002
#define SECTOR_IS_CLOSED           0x00000004
#define NULL_SECTOR                0x00000008
#define MISSING_TOPTEXTURES        0x00000010
#define MISSING_BOTTOMTEXTURES     0x00000020

#define SECF_SECRET                0x00000040
#define SECF_WASSECRET             0x00000080
#define SECF_HIDDEN                0x00000100
#define SECF_ENDGODMODE            0x00000200
#define SECF_ENDLEVEL              0x00000400
#define SECF_DMGTERRAINFX          0x00000800
#define SECF_HAZARD                0x00001000
#define SECF_DMGUNBLOCKABLE        0x00002000
#define SECF_FRICTION              0x00004000
#define SECF_PUSH                  0x00008000
#define SECF_DAMAGEFLAGS (SECF_ENDGODMODE|SECF_ENDLEVEL|SECF_DMGTERRAINFX|SECF_HAZARD|SECF_DMGUNBLOCKABLE)
#define SECF_TRANSFERMASK (SECF_SECRET|SECF_WASSECRET|SECF_DAMAGEFLAGS|SECF_FRICTION|SECF_PUSH)

#define FRICTION_LOW 0xf900

//
// Your plain vanilla vertex.
// Note: transformed values not buffered locally,
//	like some DOOM-alikes ("wt", "WebView") did.
//
struct vertex_s
{
	fixed_t x, y;
};
typedef vertex_s vertex_t;

// Forward of LineDefs, for Sectors.
struct line_s;
struct sector_s;

class player_s;

//
// The SECTORS record, at runtime.
// Stores things/mobjs.
//

enum
{
	SECSPAC_Enter		= 1,	// Trigger when player enters
	SECSPAC_Exit		= 2,	// Trigger when player exits
	SECSPAC_HitFloor	= 4,	// Trigger when player hits floor
	SECSPAC_HitCeiling	= 8,	// Trigger when player hits ceiling
	SECSPAC_Use			= 16,	// Trigger when player uses
	SECSPAC_UseWall		= 32,	// Trigger when player uses a wall
	SECSPAC_EyesDive	= 64,	// Trigger when player eyes go below fake floor
	SECSPAC_EyesSurface = 128,	// Trigger when player eyes go above fake floor
	SECSPAC_EyesBelowC	= 256,	// Trigger when player eyes go below fake ceiling
	SECSPAC_EyesAboveC	= 512	// Triggen when player eyes go above fake ceiling
};

// Ceiling/floor flags
enum
{
	SECF_ABSLIGHTING	= 1		// floor/ceiling light is absolute, not relative
};

// Misc sector flags
enum
{
	SECF_SILENT			= 1,	// actors in sector make no noise
	SECF_FAKEFLOORONLY	= 2,	// when used as heightsec in R_FakeFlat, only copies floor
	SECF_CLIPFAKEPLANES = 4,	// as a heightsec, clip planes to target sector's planes
	SECF_NOFAKELIGHT	= 8,	// heightsec does not change lighting
	SECF_IGNOREHEIGHTSEC= 16	// heightsec is only for triggering sector actions
};

enum
{
	FAKED_Center,
	FAKED_BelowFloor,
	FAKED_AboveCeiling
};

enum SectorPropChanges
{
	SPC_FlatPic = 1,
	SPC_LightLevel = 2,
	SPC_Color = 4,
	SPC_Fade = 8,
	SPC_Gravity = 16,
	SPC_Panning = 32,
	SPC_Scale = 64,
	SPC_Rotation = 128,
	SPC_AlignBase = 256,
	SPC_Max = 512,
};

enum SideDefPropChanges
{
	SDPC_TexTop = 1,
	SDPC_TexMid = 2,
	SDPC_TexBottom = 4,
	SDPC_Max = 8,
};

//
// Plane
//
// Stores the coefficients for the variable that defines a plane (sloping sector)
struct plane_s
{
	// Planes are defined by the equation ax + by + cz + d = 0
	fixed_t		a, b, c, d;
	fixed_t		invc;		// pre-calculated 1/c, used to solve for z value
	fixed_t		texx, texy;
	sector_s	*sector;
};
typedef plane_s plane_t;

struct dyncolormap_s;

class DSectorEffect;

struct sector_s
{
	fixed_t 	floorheight;
	fixed_t 	ceilingheight;
	short		floorpic;
	short		ceilingpic;
	short		lightlevel;
	short		special;
	short		tag;
	int			nexttag,firsttag;	// killough 1/30/98: improves searches for tags.
	bool		secretsector;		// Ch0wW : This is a secret sector !
	unsigned int flags;				// [Blair] Let's use actual sector flags instead of shoehorning them in special

    // 0 = untraversed, 1,2 = sndlines -1
	int 				soundtraversed;

    // thing that made a sound (or null)
	AActor::AActorPtr 	soundtarget;

	// mapblock bounding box for height changes
	int 		blockbox[4];

	// origin for any sounds played by the sector
	fixed_t		soundorg[3];

    // if == validcount, already checked
	int 		validcount;

    // list of mobjs in sector
	AActor* 	thinglist;
	int			seqType;		// this sector's sound sequence
	int sky;

	// killough 8/28/98: friction is a sector property, not an mobj property.
	// these fields used to be in AActor, but presented performance problems
	// when processed as mobj properties. Fix is to make them sector properties.
	int friction, movefactor;

	// thinker_t for reversable actions
	DSectorEffect *floordata;			// jff 2/22/98 make thinkers on
	DSectorEffect *ceilingdata;			// floors, ceilings, lighting,
	DSectorEffect *lightingdata;		// independent of one another

	bool moveable;  // [csDoom] mark a sector as moveable if it is moving.
                    // If (sector->moveable) the server sends information
                    // about this sector when a client connects.

	// jff 2/26/98 lockout machinery for stairbuilding
	int stairlock;		// -2 on first locked -1 after thinker done 0 normally
	int prevsec;		// -1 or number of sector for previous step
	int nextsec;		// -1 or number of next step sector

	// killough 3/7/98: floor and ceiling texture offsets
	fixed_t   floor_xoffs,   floor_yoffs;
	fixed_t ceiling_xoffs, ceiling_yoffs;

	// [RH] floor and ceiling texture scales
	fixed_t   floor_xscale,   floor_yscale;
	fixed_t ceiling_xscale, ceiling_yscale;

	// [RH] floor and ceiling texture rotation
	angle_t	floor_angle, ceiling_angle;

	fixed_t base_ceiling_angle, base_ceiling_yoffs;
	fixed_t base_floor_angle, base_floor_yoffs;

	// killough 3/7/98: support flat heights drawn at another sector's heights
	sector_s *heightsec;		// other sector, or NULL if no other sector

	// killough 4/11/98: support for lightlevels coming from another sector
	sector_s *floorlightsec, *ceilinglightsec;

	argb_t bottommap, midmap, topmap; // killough 4/4/98: dynamic colormaps
											// [RH] these can also be blend values if
											//		the alpha mask is non-zero

	// list of mobjs that are at least partially in the sector
	// thinglist is a subset of touching_thinglist
	msecnode_s *touching_thinglist;				// phares 3/14/98

	int linecount;
	line_s **lines;		// [linecount] size

	float gravity;		// [RH] Sector gravity (1.0 is normal)
	int damageamount;
	int damageinterval;
	int leakrate;
	short mod;			// [RH] Means-of-death for applied damage
	dyncolormap_s *colormap;	// [RH] Per-sector colormap

	bool alwaysfake;	// [RH] Always apply heightsec modifications?
	byte waterzone;		// [RH] Sector is underwater?
	WORD MoreFlags;		// [RH] Misc sector flags

	// [RH] Action specials for sectors. Like Skull Tag, but more
	// flexible in a Bloody way. SecActTarget forms a list of actors
	// joined by their tracer fields. When a potential sector action
	// occurs, SecActTarget's TriggerAction method is called.
	// [AM] Use the ZDoom 1.22 AActor system instead.
	AActor::AActorPtr SecActTarget;

	// [SL] 2012-01-16 - planes for sloping ceilings/floors
	plane_t floorplane, ceilingplane;
	int SectorChanges;
};
typedef sector_s sector_t;




//
// The SideDef.
//
struct side_s
{
    // add this to the calculated texture column
    fixed_t	textureoffset;

    // add this to the calculated texture top
    fixed_t	rowoffset;

    // Texture indices.
    // We do not maintain names here.
    short	toptexture;
    short	bottomtexture;
    short	midtexture;

    // Sector the SideDef is facing.
    sector_t*	sector;

	// [RH] Bah. Had to add these for BOOM stuff
	short		linenum;
	short		special;
	short		tag;
	int SidedefChanges;
};
typedef side_s side_t;


//
// Move clipping aid for LineDefs.
//
typedef enum
{
	ST_HORIZONTAL,
	ST_VERTICAL,
	ST_POSITIVE,
	ST_NEGATIVE
} slopetype_t;

#define R_NOSIDE ((unsigned short)(-1))

struct line_s
{
    // Vertices, from v1 to v2.
    vertex_t*	v1;
    vertex_t*	v2;

    // Precalculated v2 - v1 for side checking.
    fixed_t	dx;
    fixed_t	dy;

    // Animation related.
    unsigned int flags;		// [Blair]MBF21 compatibility
	short		special;    // [Blair] Change to short for compatibility
	byte		lucency;	// <--- translucency (0-255/255=opaque)

	// Visual appearance: SideDefs.
    //  sidenum[1] will be R_NOSIDE if one sided
	unsigned short sidenum[2];

    // Neat. Another bounding box, for the extent
    //  of the LineDef.
    fixed_t	bbox[4];

    // To aid move clipping.
    slopetype_t	slopetype;

    // Front and back sector.
    // Note: redundant? Can be retrieved from SideDefs.
    sector_t*	frontsector;
    sector_t*	backsector;

    // if == validcount, already checked
    int		validcount;

	short		id;			// <--- same as tag or set with Line_SetIdentification
	short		args[5];	// <--- hexen-style arguments
							//		note that these are shorts in order to support
							//		the tag parameter from DOOM.
	int			firstid, nextid;
	bool wastoggled;
	bool switchactive;
	bool PropertiesChanged;
	bool SidedefChanged;
};
typedef line_s line_t;

// phares 3/14/98
//
// Sector list node showing all sectors an object appears in.
//
// There are two threads that flow through these nodes. The first thread
// starts at touching_thinglist in a sector_t and flows through the m_snext
// links to find all mobjs that are entirely or partially in the sector.
// The second thread starts at touching_sectorlist in a AActor and flows
// through the m_tnext links to find all sectors a thing touches. This is
// useful when applying friction or push effects to sectors. These effects
// can be done as thinkers that act upon all objects touching their sectors.
// As an mobj moves through the world, these nodes are created and
// destroyed, with the links changed appropriately.
//
// For the links, NULL means top or end of list.

typedef struct msecnode_s
{
	sector_t			*m_sector;	// a sector containing this object
	AActor				*m_thing;	// this object
	msecnode_s	*m_tprev;	// prev msecnode_t for this thing
	msecnode_s	*m_tnext;	// next msecnode_t for this thing
	msecnode_s	*m_sprev;	// prev msecnode_t for this sector
	msecnode_s	*m_snext;	// next msecnode_t for this sector
	BOOL visited;	// killough 4/4/98, 4/7/98: used in search algorithms
} msecnode_t;

//
// The LineSeg.
//
struct seg_s
{
	vertex_t*	v1;
	vertex_t*	v2;

	fixed_t 	offset;

	angle_t 	angle;

	side_t* 	sidedef;
	line_t* 	linedef;

	// Sector references.
	// Could be retrieved from linedef, too.
	sector_t*	frontsector;
	sector_t*	backsector;		// NULL for one-sided lines

	fixed_t		length;
};
typedef seg_s seg_t;

// ===== Polyobj data =====
typedef struct FPolyObj
{
	int			numsegs;
	seg_t		**segs;
	fixed_t		startSpot[3];
	vertex_t	*originalPts;	// used as the base for the rotations
	vertex_t	*prevPts; 		// use to restore the old point values
	angle_t		angle;
	int			tag;			// reference tag assigned in HereticEd
	int			bbox[4];
	int			validcount;
	BOOL		crush; 			// should the polyobj attempt to crush mobjs?
	int			seqType;
	fixed_t		size;			// polyobj size (area of POLY_AREAUNIT == size of FRACUNIT)
	DThinker	*specialdata;	// pointer to a thinker, if the poly is moving
} polyobj_t;

typedef struct polyblock_s
{
	polyobj_t *polyobj;
	polyblock_s *prev;
	polyblock_s *next;
} polyblock_t;

//
// A SubSector.
// References a Sector.
// Basically, this is a list of LineSegs,
//	indicating the visible walls that define
//	(all or some) sides of a convex BSP leaf.
//
typedef struct subsector_s
{
	sector_t		*sector;
	unsigned int	numlines;
	unsigned int	firstline;
	polyobj_t	    *poly;
} subsector_t;

//
// BSP node.
//

// Indicate a leaf.
#define NF_SUBSECTOR	0x80000000

struct node_s
{
	// Partition line.
	fixed_t			x;
	fixed_t			y;
	fixed_t			dx;
	fixed_t			dy;
	fixed_t			bbox[2][4];		// Bounding box for each child.
	unsigned int	children[2];	// If NF_SUBSECTOR its a subsector.
};
typedef node_s node_t;



// posts are runs of non masked source pixels
struct post_t
{
	byte topdelta; // -1 is the last post in a column
	byte length;   // length data bytes follows

	/**
	 * @brief Return the post's absolute topdelta accounting for tall
	 *        patches, which treat topdelta as relative.
	 * 
	 * @param lastAbs Last absolute topdelta.
	 */
	int abs(const int lastAbs) const
	{
		if (topdelta <= lastAbs)
			return lastAbs + topdelta;
		else
			return topdelta;
	}

	/**
	 * @brief Size of the post, including header.
	 */
	uint32_t size() const
	{
		return length + 3;
	}
	
	/**
	 * @brief Return a pointer to post data.
	 */
	byte* data() const
	{
		return (byte*)(this) + 3;
	}

	/**
	 * @brief Return a pointer to the next post in the column.
	 */
	post_t* next() const
	{
		return (post_t*)((byte*)this + length + 4);
	}

	/**
	 * @brief Check if the post ends the column.
	 */
	bool end() const
	{
		return topdelta == 0xFF;
	}
};

// column_t is a list of 0 or more post_t, (byte)-1 terminated
typedef post_t	column_t;

struct tallpost_t
{
	unsigned short topdelta;
	unsigned short length;

	uint32_t size() const
	{
		return length + 4;
	}
	byte* data() const
	{
		return (byte*)(this) + 4;
	}
	tallpost_t* next() const
	{
		return (tallpost_t*)((byte*)(this) + 4 + length);
	}
	bool end() const
	{
		return topdelta == 0xFFFF;
	}
	void writeend()
	{
		topdelta = 0xFFFF;
	}
};

//
// OTHER TYPES
//

struct drawseg_s
{
	seg_t*			curline;

    int				x1;
    int				x2;

    fixed_t			scale1;
    fixed_t			scale2;
    fixed_t			scalestep;

	fixed_t			light, lightstep;

    // 0=none, 1=bottom, 2=top, 3=both
    int				silhouette;

    // Pointers to lists for sprite clipping,
    //  all three adjusted so [x1] is first value.
    int*			sprtopclip;
    int*			sprbottomclip;
	tallpost_t**	midposts;
};
typedef drawseg_s drawseg_t;


// Patches.
// A patch holds one or more columns.
// Patches are used for sprites and all masked pictures, and we compose
// textures from the TEXTURE1/2 lists of patches.
struct patch_s
{
private:
	short			_width;			// bounding box size
	short			_height;
	short			_leftoffset; 	// pixels to the left of origin
	short			_topoffset;		// pixels below the origin

public:
	int columnofs[8]; // only [width] used
	// the [0] is &columnofs[width]

	short width() const
	{
		return LESHORT(_width);
	}
	short height() const
	{
		return LESHORT(_height);
	}
	short leftoffset() const
	{
		return LESHORT(_leftoffset);
	}
	short topoffset() const
	{
		return LESHORT(_topoffset);
	}
	uint32_t* ofs() const
	{
		return (uint32_t*)((byte*)this + 8);
	}
	uint32_t datastart() const
	{
		return 8 + 4 * width();
	}
	post_t* post(const uint32_t ofs)
	{
		return (post_t*)((byte*)this + ofs);
	}
	tallpost_t* tallpost(const uint32_t ofs)
	{
		return (tallpost_t*)((byte*)this + ofs);
	}
};
typedef patch_s patch_t;




// A vissprite_t is a thing
//	that will be drawn during a refresh.
// I.e. a sprite object that is partly visible.
struct vissprite_s
{
    int				x1;
    int				x2;
	int				y1;
	int				y2;

    // for line side calculation
    fixed_t			gx;
    fixed_t			gy;

    // global bottom / top for silhouette clipping
    fixed_t			gzb;
    fixed_t			gzt;

    // horizontal position of x1
    fixed_t			startfrac;

	fixed_t			xscale, yscale;

    // negative if flipped
    fixed_t			xiscale;

	fixed_t			depth;
	fixed_t			texturemid;
	int				patch;

    // for color translation and shadow draw,
    //  maxbright frames as well
    shaderef_t		colormap;

	int 			mobjflags;
	bool			spectator;		// [Blair] Mark if this visprite belongs to a spectator.

	translationref_t translation;	// [RH] for translation;
	sector_t*		heightsec;		// killough 3/27/98: height sector for underwater/fake ceiling
	fixed_t			translucency;
	byte			FakeFlat;		// [RH] which side of fake/floor ceiling sprite is on

	AActor*			mo;
};
typedef vissprite_s vissprite_t;

//
// Sprites are patches with a special naming convention
//  so they can be recognized by R_InitSprites.
// The base name is NNNNFx or NNNNFxFx, with
//  x indicating the rotation, x = 0, 1-7.
// The sprite and frame specified by a thing_t
//  is range checked at run time.
// A sprite is a patch_t that is assumed to represent
//  a three dimensional object and may have multiple
//  rotations pre drawn.
// Horizontal flipping is used to save space,
//  thus NNNNF2F5 defines a mirrored patch.
// Some sprites will only have one picture used
// for all views: NNNNF0
//
struct spriteframe_s
{
    // If false use 0 for any position.
    // Note: as eight entries are available,
    //  we might as well insert the same name eight times.
	bool	rotate;

    // Lump to use for view angles 0-15.
    short	lump[16];

    // Flip bit (1 = flip) to use for view angles 0-15.
    byte	flip[16];

	// [RH] Move some data out of spritewidth, spriteoffset,
	//		and spritetopoffset arrays.
	fixed_t		width[16];
	fixed_t		topoffset[16];
	fixed_t		offset[16];
};
typedef spriteframe_s spriteframe_t;

//
// A sprite definition:
//	a number of animation frames.
//
struct spritedef_s
{
	int 			numframes;
	spriteframe_t	*spriteframes;
};
typedef spritedef_s spritedef_t;

//
// The infamous visplane
//
struct visplane_s
{
	visplane_s *next;		// Next visplane in hash chain -- killough

	plane_t		secplane;

	int			picnum;
	int			lightlevel;
	fixed_t		xoffs, yoffs;		// killough 2/28/98: Support scrolling flats
	int			minx;
	int			maxx;

	shaderef_t	colormap;			// [RH] Support multiple colormaps
	fixed_t		xscale, yscale;		// [RH] Support flat scaling
	angle_t		angle;				// [RH] Support flat rotation

	unsigned int *bottom;			// [RH] bottom and top arrays are dynamically
	unsigned int pad;				//		allocated immediately after the
	unsigned int top[3];			//		visplane.
};
typedef visplane_s visplane_t;
