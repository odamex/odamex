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
//		Map Objects, MObj, definition and handling.
//
//-----------------------------------------------------------------------------

#pragma once

// Basics.
#include "tables.h"
#include "m_fixed.h"
#include "m_vectors.h"

// We need the thinker_t stuff.
#include "dthinker.h"

// We need the WAD data structure for Map things,
// from the THINGS lump.
#include "doomdata.h"

// States are tied to finite states are
//	tied to animation frames.
// Needs precompiled tables/data structures.
#include "info.h"

#include "szp.h"

// STL
#include <map>

//
// NOTES: AActor
//
// Actors are used to tell the refresh where to draw an image,
// tell the world simulation when objects are contacted,
// and tell the sound driver how to position a sound.
//
// The refresh uses the next and prev links to follow
// lists of things in sectors as they are being drawn.
// The sprite, frame, and angle elements determine which patch_t
// is used to draw the sprite if it is visible.
// The sprite and frame values are almost always set
// from state_t structures.
// The statescr.exe utility generates the states.h and states.c
// files that contain the sprite/frame numbers from the
// statescr.txt source file.
// The xyz origin point represents a point at the bottom middle
// of the sprite (between the feet of a biped).
// This is the default origin position for patch_ts grabbed
// with lumpy.exe.
// A walking creature will have its z equal to the floor
// it is standing on.
//
// The sound code uses the x,y, and subsector fields
// to do stereo positioning of any sound effited by the AActor.
//
// The play simulation uses the blocklinks, x,y,z, radius, height
// to determine when AActors are touching each other,
// touching lines in the map, or hit by trace lines (gunshots,
// lines of sight, etc).
// The AActor->flags element has various bit flags
// used by the simulation.
//
// Every actor is linked into a single sector
// based on its origin coordinates.
// The subsector_t is found with P_PointInSubsector(x,y),
// and the sector_t can be found with subsector->sector.
// The sector links are only used by the rendering code,
// the play simulation does not care about them at all.
//
// Any actor that needs to be acted upon by something else
// in the play world (block movement, be shot, etc) will also
// need to be linked into the blockmap.
// If the thing has the MF_NOBLOCK flag set, it will not use
// the block links. It can still interact with other things,
// but only as the instigator (missiles will run into other
// things, but nothing can run into a missile).
// Each block in the grid is 128*128 units, and knows about
// every line_t that it contains a piece of, and every
// interactable actor that has its origin contained.
//
// A valid actor is an actor that has the proper subsector_t
// filled in for its xy coordinates and is linked into the
// sector from which the subsector was made, or has the
// MF_NOSECTOR flag set (the subsector_t needs to be valid
// even if MF_NOSECTOR is set), and is linked into a blockmap
// block or has the MF_NOBLOCKMAP flag set.
// Links should only be modified by the P_[Un]SetThingPosition()
// functions.
// Do not change the MF_NO* flags while a thing is valid.
//
// Any questions?
//

//
// [SL] 2012-04-30 - A bit field to store a bool value for every player.
// 
class PlayerBitField
{
public:
	PlayerBitField() { clear(); }
	
	void clear()
	{
		memset(bitfield, 0, sizeof(bitfield));
	}

	void set(byte id)
	{
		int bytenum = id >> 3;
		int bitnum = id & bytemask;
	
		bitfield[bytenum] |= (1 << bitnum);
	}
	
	void unset(byte id)
	{
		int bytenum = id >> 3;
		int bitnum = id & bytemask;
	
		bitfield[bytenum] &= ~(1 << bitnum);
	}
	
	bool get(byte id) const
	{
		int bytenum = id >> 3;
		int bitnum = id & bytemask;	
	
		return ((bitfield[bytenum] & (1 << bitnum)) != 0);
	}
	
private:
	static const int bytesize = 8*sizeof(byte);
	static const int bytemask = bytesize - 1;
	
	// Hacky way of getting ceil() at compile-time
	static const size_t fieldsize = (MAXPLAYERS + bytemask) / bytesize;
	
	byte	bitfield[fieldsize];
};

//
// Misc. mobj flags
//
enum mobjflag_t
{
	// --- mobj.flags ---

	MF_SPECIAL		= BIT(0),	// call P_SpecialThing when touched
	MF_SOLID		= BIT(1),
	MF_SHOOTABLE	= BIT(2),
	MF_NOSECTOR		= BIT(3),	// don't use the sector links
								// (invisible but touchable)
	MF_NOBLOCKMAP	= BIT(4),	// don't use the blocklinks
								// (inert but displayable)
	MF_AMBUSH		= BIT(5),	// not activated by sound; deaf monster
	MF_JUSTHIT		= BIT(6),	// try to attack right back
	MF_JUSTATTACKED = BIT(7),	// take at least one step before attacking
	MF_SPAWNCEILING = BIT(8),	// hang from ceiling instead of floor
	MF_NOGRAVITY	= BIT(9),	// don't apply gravity every tic

	// movement flags
	MF_DROPOFF	= BIT(10),		// allow jumps from high places
	MF_PICKUP	= BIT(11),		// for players to pick up items
	MF_NOCLIP	= BIT(12),		// player cheat
	MF_SLIDE	= BIT(13),		// keep info about sliding along walls
	MF_FLOAT	= BIT(14),		// allow moves to any height, no gravity
	MF_TELEPORT = BIT(15),		// don't cross lines or look at heights
	MF_MISSILE	= BIT(16),		// don't hit same species, explode on block

	MF_DROPPED	= BIT(17),		// dropped by a demon, not level spawned
	MF_SHADOW	= BIT(18),		// actor is hard for monsters to see
	MF_NOBLOOD	= BIT(19),		// don't bleed when shot (use puff)
	MF_CORPSE	= BIT(20),		// don't stop moving halfway off a step
	MF_INFLOAT	= BIT(21),		// floating to a height for a move, don't
								// auto float to target's height

	MF_COUNTKILL = BIT(22),		// count towards intermission kill total
	MF_COUNTITEM = BIT(23),		// count towards intermission item total

	MF_SKULLFLY  = BIT(24),		// skull in flight
	MF_NOTDMATCH = BIT(25),		// don't spawn in death match (key cards)

	// Player sprites in multiplayer modes are modified
	//  using an internal color lookup table for re-indexing.
	// If 0x4 0x8 or 0xc, use a translation table for player colormaps
	MF_TRANSLATION = 0xc000000,

	MF_TOUCHY  = BIT(28), // MBF - UNUSED FOR NOW
	MF_BOUNCES = BIT(29), // MBF - PARTIAL IMPLEMENTATION
	MF_FRIEND  = BIT(30), // MBF - UNUSED FOR NOW

	// --- mobj.flags2 ---
	// Heretic flags
	MF2_LOGRAV			= BIT(0),   // alternate gravity setting
	MF2_WINDTHRUST		= BIT(1),   // gets pushed around by the wind
									// specials
	MF2_FLOORBOUNCE		= BIT(2),   // bounces off the floor
	MF2_BLASTED			= BIT(3),	// mobj can be on an edge because it was hit by a blast
	MF2_FLY				= BIT(4),   // fly mode is active
	MF2_FLOORCLIP		= BIT(5),   // if feet are allowed to be clipped
	MF2_SPAWNFLOAT		= BIT(6),   // spawn random float z
	MF2_NOTELEPORT		= BIT(7),   // does not teleport
	MF2_RIP				= BIT(8),   // missile rips through solid
									// targets
	MF2_PUSHABLE		= BIT(9),   // can be pushed by other moving
									// mobjs
	MF2_SLIDE			= BIT(10),  // slides against walls
	MF2_ONMOBJ			= BIT(11),  // mobj is resting on top of another
									// mobj
	MF2_PASSMOBJ		= BIT(12),  // Enable z block checking.  If on,
									// this flag will allow the mobj to
									// pass over/under other mobjs.
	MF2_CANNOTPUSH		= BIT(13),  // cannot push other pushable mobjs
	MF2_THRUGHOST		= BIT(14),  // missile will pass through ghosts [RH] was 8
	MF2_BOSS			= BIT(15),  // mobj is a major boss
	MF2_FIREDAMAGE		= BIT(16),  // does fire damage
	MF2_NODMGTHRUST		= BIT(17),  // does not thrust target when damaging
	MF2_TELESTOMP		= BIT(18),  // mobj can stomp another
	MF2_FLOATBOB		= BIT(19),  // use float bobbing z movement
	MF2_DONTDRAW		= BIT(20),  // don't generate a vissprite
	MF2_IMPACT			= BIT(21),  // an MF_MISSILE mobj can activate SPAC_IMPACT
	MF2_PUSHWALL		= BIT(22),  // mobj can push walls
	MF2_MCROSS			= BIT(23),  // can activate monster cross lines
	MF2_PCROSS			= BIT(24),  // can activate projectile cross lines
	MF2_CANTLEAVEFLOORPIC = BIT(25),// stay within a certain floor type
	MF2_NONSHOOTABLE	= BIT(26),  // mobj is totally non-shootable,
									// but still considered solid
	MF2_INVULNERABLE	= BIT(27),  // mobj is invulnerable
	MF2_DORMANT			= BIT(28),  // thing is dormant
	MF2_ICEDAMAGE		= BIT(29),  // does ice damage
	MF2_SEEKERMISSILE	= BIT(30),  // is a seeker (for reflection)
	MF2_REFLECTIVE		= BIT(31),  // reflects missiles

	// --- mobj.flags3 ---
	// MBF21-specific flags
	                                // BIT0 will be MF2_LOGRAV
	MF3_SHORTMRANGE		= BIT(1),	// has short missile range (archvile)
	MF3_DMGIGNORED		= BIT(2),	// other things ignore its attacks (archvile)
	MF3_NORADIUSDMG		= BIT(3),	// doesn't take splash damage
	MF3_FORCERADIUSDMG	= BIT(4),	// does radius damage to everything, no exceptions
	MF3_HIGHERMPROB		= BIT(5),	// min prob. of miss. att. = 37.5% vs 22%
	MF3_RANGEHALF		= BIT(6),	// use half actual distance for missile attack probability
	MF3_NOTHRESHOLD     = BIT(7),   // has no targeting threshold (archvile)
	MF3_LONGMELEE		= BIT(8),   // long melee range
									// BIT 9 is MF2_BOSS -- RESERVED
	MF3_MAP07BOSS1		= BIT(10),	// is a MAP07 boss type 1 (666)
	MF3_MAP07BOSS2		= BIT(11),	// is a MAP07 boss type 2 (667)
	MF3_E1M8BOSS		= BIT(12),	// is an E1M8 boss
	MF3_E2M8BOSS		= BIT(13),	// is an E1M8 boss
	MF3_E3M8BOSS		= BIT(14),	// is an E3M8 boss
	MF3_E4M6BOSS		= BIT(15),	// is an E4M6 boss
	MF3_E4M8BOSS		= BIT(16),	// is an E4M8 boss
									// BIT 15 is MF2_RIP -- RESERVED
	MF3_FULLVOLSOUNDS	= BIT(18),	// full volume see / death sound

	// --- mobj.oflags ---
	// Odamex-specific flags
	MFO_NOSNAPZ			= BIT(0),	// ignore snapshot z this tic
	MFO_HEALTHPOOL		= BIT(1),	// global health pool that tracks killed HP
	MFO_INFIGHTINVUL	= BIT(2),	// invulnerable to infighting
	MFO_UNFLINCHING		= BIT(3),	// monster flinching reduced to 1 in 256
	MFO_ARMOR			= BIT(4),	// damage taken by monster is reduced
	MFO_QUICK			= BIT(5),	// speed of monster is increased
	MFO_NORAISE			= BIT(6),	// vile can't raise corpse
	MFO_BOSSPOOL		= BIT(7),	// boss health pool that tracks damage
	MFO_FULLBRIGHT		= BIT(8),	// monster is fullbright
	MFO_SPECTATOR		= BIT(9),	// GhostlyDeath -- thing is/was a spectator and can't be seen!
	MFO_FALLING			= BIT(10),	// [INTERNAL] for falling
};

#define MF_TRANSSHIFT	0x1A

#define TRANSLUC25			(FRACUNIT/4)
#define TRANSLUC33			(FRACUNIT/3)
#define TRANSLUC50			(FRACUNIT/2)
#define TRANSLUC66			((FRACUNIT*2)/3)
#define TRANSLUC75			((FRACUNIT*3)/4)

// killough 11/98: For torque simulation:
#define OVERDRIVE 6
#define MAXGEAR (OVERDRIVE+16)

struct baseline_t
{
	v3fixed_t pos;
	v3fixed_t mom;
	angle_t angle;
	uint32_t targetid;
	uint32_t tracerid;
	int movecount;
	byte movedir;
	byte rndindex;

	// Flags are a varint, so order from most to least likely.
	static const uint32_t POSX = BIT(0);
	static const uint32_t POSY = BIT(1);
	static const uint32_t POSZ = BIT(2);
	static const uint32_t ANGLE = BIT(3);
	static const uint32_t MOVEDIR = BIT(4);
	static const uint32_t MOVECOUNT = BIT(5);
	static const uint32_t RNDINDEX = BIT(6);
	static const uint32_t TARGET = BIT(7);
	static const uint32_t TRACER = BIT(8);
	static const uint32_t MOMX = BIT(9);
	static const uint32_t MOMY = BIT(10);
	static const uint32_t MOMZ = BIT(11);

	baseline_t()
	    : angle(0), targetid(0), tracerid(0), movecount(0), movedir(0), rndindex(0)
	{
		pos.x = 0;
		pos.y = 0;
		pos.z = 0;
		mom.x = 0;
		mom.y = 0;
		mom.z = 0;
	}

	void Serialize(FArchive& arc)
	{
		if (arc.IsStoring())
		{
			arc << pos.x << pos.y << pos.z << mom.x << mom.y << mom.z << angle << targetid
			    << tracerid << movecount << movedir << rndindex;
		}
		else
		{
			arc >> pos.x >> pos.y >> pos.z >> mom.x >> mom.y >> mom.z >> angle >>
			    targetid >> tracerid >> movecount >> movedir >> rndindex;
		}
	}
};

// Map Object definition.
class AActor : public DThinker
{
	DECLARE_SERIAL (AActor, DThinker)
	typedef szp<AActor> AActorPtr;
	AActorPtr self;

	class AActorPtrCounted
	{
		AActorPtr ptr;

		public:

		AActorPtrCounted() {}

		AActorPtr &operator= (AActorPtr other)
		{
			if(ptr)
				ptr->refCount--;
			if(other)
				other->refCount++;
			ptr = other;
			return ptr;
		}

		AActorPtr &operator= (AActorPtrCounted other)
		{
			if(ptr)
				ptr->refCount--;
			if(other)
				other->refCount++;
			ptr = other.ptr;
			return ptr;
		}

		~AActorPtrCounted()
		{
			if(ptr)
				ptr->refCount--;
		}

		operator AActorPtr()
		{
			return ptr;
		}
		operator AActor*()
		{
			return ptr;
		}

		AActor &operator *()
		{
			return *ptr;
		}
		AActor *operator ->()
		{
			return ptr;
		}
	};

public:
	AActor ();
	AActor (const AActor &other);
	AActor &operator= (const AActor &other);
	AActor (fixed_t x, fixed_t y, fixed_t z, mobjtype_t type);
	void Destroy ();
	~AActor ();

	virtual void RunThink ();

    // Info for drawing: position.
    fixed_t		x;
    fixed_t		y;
    fixed_t		z;

	fixed_t		prevx;
	fixed_t		prevy;
	fixed_t		prevz;

	AActor			*snext, **sprev;	// links in sector (if needed)

    //More drawing info: to determine current sprite.
    angle_t		angle;	// orientation
	angle_t		prevangle;
    spritenum_t		sprite;	// used to find patch_t and flip value
    int			frame;	// might be ORed with FF_FULLBRIGHT
	fixed_t		pitch;
	angle_t		prevpitch;

	DWORD			effects;			// [RH] see p_effect.h

    // Interaction info, by BLOCKMAP.
    // Links in blocks (if needed).
	struct subsector_s		*subsector;

    // The closest interval over all contacted Sectors.
    fixed_t		floorz;
    fixed_t		ceilingz;
	fixed_t		dropoffz;
	struct sector_s		*floorsector;

    // For movement checking.
    fixed_t		radius;
    fixed_t		height;

    // Momentums, used to update position.
    fixed_t		momx;
    fixed_t		momy;
    fixed_t		momz;

    // If == validcount, already checked.
    int			validcount;

	mobjtype_t		type;
    mobjinfo_t*		info;	// &mobjinfo[mobj->type]
    int				tics;	// state tic counter
	state_t			*state;
	int				damage;			// For missiles	
	int				flags;
	int				flags2;	// Heretic flags
	int				flags3;	// MBF21 flags
	int				oflags;			// Odamex flags
	int				special1;		// Special info
	int				special2;		// Special info
	int 			health;

    // Movement direction, movement generation (zig-zagging).
    byte			movedir;	// 0-7
    int				movecount;	// when 0, select a new dir
	char			visdir;

    // Thing being chased/attacked (or NULL),
    // also the originator for missiles.
	AActorPtr		target;
	AActorPtr		lastenemy;		// Last known enemy -- killogh 2/15/98

    // Reaction time: if non 0, don't attack yet.
    // Used by player to freeze a bit after teleporting.
    int				reactiontime;

    // If >0, the target will be chased
    // no matter what (even if shot)
    int			threshold;

    // Additional info record for player avatars only.
    // Only valid if type == MT_PLAYER
	player_s*	player;

    // Player number last looked for.
    unsigned int	lastlook;

    // For nightmare respawn.
    mapthing2_t		spawnpoint;

	// Thing being chased/attacked for tracers.
	AActorPtr		tracer;
	byte			special;		// special
	byte			args[5];		// special arguments

	AActor			*inext, *iprev;	// Links to other mobjs in same bucket

	// denis - playerids of players to whom this object has been sent
	// [SL] changed to use a bitfield instead of a vector for O(1) lookups
	PlayerBitField	players_aware;

	AActorPtr		goal;			// Monster's goal if not chasing anything
	translationref_t translation;	// Translation table (or NULL)
	fixed_t			translucency;	// 65536=fully opaque, 0=fully invisible
	byte			waterlevel;		// 0=none, 1=feet, 2=waist, 3=eyes
	SWORD			gear;			// killough 11/98: used in torque simulation

	bool			onground;		// NES - Fixes infinite jumping bug like a charm.

	// a linked list of sectors where this object appears
	struct msecnode_s	*touching_sectorlist;				// phares 3/14/98

	short           deadtic;        // tics after player's death
	int             oldframe;

	unsigned char	rndindex;		// denis - because everything should have a random number generator, for prediction

	// ThingIDs
	static void ClearTIDHashes ();
	void AddToHash ();
	void RemoveFromHash ();
	AActor *FindByTID (int tid) const;
	static AActor *FindByTID (const AActor *first, int tid);
	AActor *FindGoal (int tid, int kind) const;
	static AActor *FindGoal (const AActor *first, int tid, int kind);

	uint32_t		netid;          // every object has its own netid
	short			tid;			// thing identifier
	baseline_t		baseline;		// Baseline data for mobj sent to clients
	bool			baseline_set;	// Have we set our baseline yet?

private:
	static const size_t TIDHashSize = 256;
	static const size_t TIDHashMask = TIDHashSize - 1;
	static AActor *TIDHash[TIDHashSize];
	static inline int TIDHASH (int key) { return key & TIDHashMask; }

	friend class FActorIterator;

public:
	void LinkToWorld ();
	void UnlinkFromWorld ();

	void SetOrigin (fixed_t x, fixed_t y, fixed_t z);

	AActorPtr ptr(){ return self; }
	
	//
	// ActorBlockMapListNode
	//
	// [SL] A container for the linked list nodes for all of the mapblocks that
	// an actor can be standing in.  Vanilla Doom only considered an actor to
	// be in the mapblock where its center was located, even if it was
	// overlapping other blocks.
	//
	class ActorBlockMapListNode
	{
	public:
		ActorBlockMapListNode(AActor *mo);
		void Link();
		void Unlink();
		AActor* Next(int bmx, int bmy);

	private:
		void clear();
		size_t getIndex(int bmx, int bmy);
		
		static const size_t BLOCKSX = 3;
		static const size_t BLOCKSY = 3;

		AActor		*actor;
			
		// the top-left blockmap the actor is in
		int			originx;
		int			originy;
		// the number of blocks the actor occupies
		int			blockcntx;
		int			blockcnty;

		// the next and previous actors in each of the possible blockmaps
		// this actor can inhabit
		AActor		*next[BLOCKSX * BLOCKSY];
		AActor		**prev[BLOCKSX * BLOCKSY];
	};
	
	ActorBlockMapListNode bmapnode;
};

typedef std::vector<AActor::AActorPtr> AActors;

class FActorIterator
{
public:
	FActorIterator (int i) : base (NULL), id (i)
	{
	}
	AActor *Next ()
	{
		if (id == 0)
			return NULL;
		if (!base)
			base = AActor::FindByTID(NULL, id);
		else
			base = base->inext;

		while (base && base->tid != id)
			base = base->inext;

		return base;
	}
private:
	AActor *base;
	int id;
};


template<class T>
class TActorIterator : public FActorIterator
{
public:
	TActorIterator (int id) : FActorIterator (id) {}
	T *Next ()
	{
		AActor *actor;
		do
		{
			actor = FActorIterator::Next ();
		} while (actor && !actor->IsKindOf (RUNTIME_CLASS(T)));
		return static_cast<T *>(actor);
	}
};
