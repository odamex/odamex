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
//		Map Objects, MObj, definition and handling.
//
//-----------------------------------------------------------------------------


#ifndef __ACTOR_H__
#define __ACTOR_H__

// Basics.
#include "tables.h"
#include "m_fixed.h"

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
#include <vector>
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
typedef enum
{
// --- mobj.flags ---

	MF_SPECIAL		= 0x00000001,	// call P_SpecialThing when touched
	MF_SOLID		= 0x00000002,
	MF_SHOOTABLE	= 0x00000004,
	MF_NOSECTOR		= 0x00000008,	// don't use the sector links
									// (invisible but touchable)
	MF_NOBLOCKMAP	= 0x00000010,	// don't use the blocklinks
									// (inert but displayable)
	MF_AMBUSH		= 0x00000020,	// not activated by sound; deaf monster
	MF_JUSTHIT		= 0x00000040,	// try to attack right back
	MF_JUSTATTACKED	= 0x00000080,	// take at least one step before attacking
	MF_SPAWNCEILING	= 0x00000100,	// hang from ceiling instead of floor
	MF_NOGRAVITY	= 0x00000200,	// don't apply gravity every tic

// movement flags
	MF_DROPOFF		= 0x00000400,	// allow jumps from high places
	MF_PICKUP		= 0x00000800,	// for players to pick up items
	MF_NOCLIP		= 0x00001000,	// player cheat
	MF_SLIDE		= 0x00002000,	// keep info about sliding along walls
	MF_FLOAT		= 0x00004000,	// allow moves to any height, no gravity
	MF_TELEPORT		= 0x00008000,	// don't cross lines or look at heights
	MF_MISSILE		= 0x00010000,	// don't hit same species, explode on block

	MF_DROPPED		= 0x00020000,	// dropped by a demon, not level spawned
	MF_SHADOW		= 0x00040000,	// actor is hard for monsters to see
	MF_NOBLOOD		= 0x00080000,	// don't bleed when shot (use puff)
	MF_CORPSE		= 0x00100000,	// don't stop moving halfway off a step
	MF_INFLOAT		= 0x00200000,	// floating to a height for a move, don't
									// auto float to target's height

	MF_COUNTKILL	= 0x00400000,	// count towards intermission kill total
	MF_COUNTITEM	= 0x00800000,	// count towards intermission item total

	MF_SKULLFLY		= 0x01000000,	// skull in flight
	MF_NOTDMATCH	= 0x02000000,	// don't spawn in death match (key cards)

    // Player sprites in multiplayer modes are modified
    //  using an internal color lookup table for re-indexing.
    // If 0x4 0x8 or 0xc, use a translation table for player colormaps
    MF_TRANSLATION	= 0xc000000,

	MF_UNMORPHED	= 0x10000000,	// [RH] Actor is the unmorphed version of something else
	MF_FALLING		= 0x20000000,
    MF_SPECTATOR	= 0x40000000,	// GhostlyDeath -- thing is/was a spectator and can't be seen!
	MF_ICECORPSE	= 0x80000000,	// a frozen corpse (for blasting) [RH] was 0x800000

// --- mobj.flags2 ---

	MF2_LOGRAV			= 0x00000001,	// alternate gravity setting
	MF2_WINDTHRUST		= 0x00000002,	// gets pushed around by the wind
										// specials
	MF2_FLOORBOUNCE		= 0x00000004,	// bounces off the floor
	MF2_BLASTED			= 0x00000008,	// missile will pass through ghosts
	MF2_FLY				= 0x00000010,	// fly mode is active
	MF2_FLOORCLIP		= 0x00000020,	// if feet are allowed to be clipped
	MF2_SPAWNFLOAT		= 0x00000040,	// spawn random float z
	MF2_NOTELEPORT		= 0x00000080,	// does not teleport
	MF2_RIP				= 0x00000100,	// missile rips through solid
										// targets
	MF2_PUSHABLE		= 0x00000200,	// can be pushed by other moving
										// mobjs
	MF2_SLIDE			= 0x00000400,	// slides against walls
	MF2_ONMOBJ			= 0x00000800,	// mobj is resting on top of another
										// mobj
	MF2_PASSMOBJ		= 0x00001000,	// Enable z block checking.  If on,
										// this flag will allow the mobj to
										// pass over/under other mobjs.
	MF2_CANNOTPUSH		= 0x00002000,	// cannot push other pushable mobjs
	MF2_THRUGHOST		= 0x00004000,	// missile will pass through ghosts [RH] was 8
	MF2_BOSS			= 0x00008000,	// mobj is a major boss
	MF2_FIREDAMAGE		= 0x00010000,	// does fire damage
	MF2_NODMGTHRUST		= 0x00020000,	// does not thrust target when damaging
	MF2_TELESTOMP		= 0x00040000,	// mobj can stomp another
	MF2_FLOATBOB		= 0x00080000,	// use float bobbing z movement
	MF2_DONTDRAW		= 0x00100000,	// don't generate a vissprite
	MF2_IMPACT			= 0x00200000, 	// an MF_MISSILE mobj can activate SPAC_IMPACT
	MF2_PUSHWALL		= 0x00400000, 	// mobj can push walls
	MF2_MCROSS			= 0x00800000,	// can activate monster cross lines
	MF2_PCROSS			= 0x01000000,	// can activate projectile cross lines
	MF2_CANTLEAVEFLOORPIC = 0x02000000,	// stay within a certain floor type
	MF2_NONSHOOTABLE	= 0x04000000,	// mobj is totally non-shootable,
										// but still considered solid
	MF2_INVULNERABLE	= 0x08000000,	// mobj is invulnerable
	MF2_DORMANT			= 0x10000000,	// thing is dormant
	MF2_ICEDAMAGE		= 0x20000000,	// does ice damage
	MF2_SEEKERMISSILE	= 0x40000000,	// is a seeker (for reflection)
	MF2_REFLECTIVE		= 0x80000000,	// reflects missiles

	// --- mobj.oflags ---
	// Odamex-specific flags
	MFO_NOSNAPZ			= 1 << 0		// ignore snapshot z this tic
} mobjflag_t;

#define MF_TRANSSHIFT	0x1A

#define TRANSLUC25			(FRACUNIT/4)
#define TRANSLUC33			(FRACUNIT/3)
#define TRANSLUC50			(FRACUNIT/2)
#define TRANSLUC66			((FRACUNIT*2)/3)
#define TRANSLUC75			((FRACUNIT*3)/4)

// killough 11/98: For torque simulation:
#define OVERDRIVE 6
#define MAXGEAR (OVERDRIVE+16)

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

	int             netid;          // every object has its own netid
	short			tid;			// thing identifier

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

	AActorPtr ptr(){ return AActorPtr(self); }
	
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


#endif



