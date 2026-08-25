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
//		Map Objects, MObj, definition and handling.
//
//-----------------------------------------------------------------------------

#pragma once

#include <array>

// Basics.
#include "doomdef.h"

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

#include "teamdef.h"

#include "ActorAwarenessState.h"

#include "p_blockmap.h"

#include "actorflags.h"

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
	v3fixed_t pos       { 0, 0, 0 };
	v3fixed_t mom       { 0, 0, 0 };
	angle_t angle       { 0 };
	uint32_t targetid   { 0 };
	uint32_t tracerid   { 0 };
	int movecount       { 0 };
	byte movedir        { 0 };
	byte rndindex       { 0 };

	// Flags are a varint, so order from most to least likely.
	static constexpr uint32_t POSX = BIT(0);
	static constexpr uint32_t POSY = BIT(1);
	static constexpr uint32_t POSZ = BIT(2);
	static constexpr uint32_t ANGLE = BIT(3);
	static constexpr uint32_t MOVEDIR = BIT(4);
	static constexpr uint32_t MOVECOUNT = BIT(5);
	static constexpr uint32_t RNDINDEX = BIT(6);
	static constexpr uint32_t TARGET = BIT(7);
	static constexpr uint32_t TRACER = BIT(8);
	static constexpr uint32_t MOMX = BIT(9);
	static constexpr uint32_t MOMY = BIT(10);
	static constexpr uint32_t MOMZ = BIT(11);

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

enum class CredibilityEnum
{
	NOT_CREDIBLE = 0,
	ALWAYS_CREDIBLE,
	FULLY_CREDIBLE,
	SEMI_CREDIBLE,
	CHALLENGED_CREDIBILITY,

	CREDIBILITY_LEVEL_COUNT
};

class AActor;

class CredibilityState
{
	public:

		[[nodiscard]]
		CredibilityEnum Get() const
		{
			return m_credibility;
		}

		[[nodiscard]]
		bool IsCredible() const
		{
			return m_credibility == CredibilityEnum::FULLY_CREDIBLE
			    or m_credibility == CredibilityEnum::ALWAYS_CREDIBLE;
		}

		void Update(const AActor& mobj);

		void Challenge()
		{
			m_credibility = CredibilityEnum::CHALLENGED_CREDIBILITY;
		}

		void Lionize()
		{
			m_credibility = CredibilityEnum::ALWAYS_CREDIBLE;
		}

		template <typename StreamType>
		friend StreamType& operator<<(StreamType& io_stream, const CredibilityState& i_thisRef)
		{
			io_stream
			    << i_thisRef.m_credibility
			    << i_thisRef.m_crediblePosition
			    << i_thisRef.m_predictedMotionTicCount
			    ;
			return io_stream;
		}

		template <typename StreamType>
		friend StreamType& operator>>(StreamType& io_stream, CredibilityState& o_thisRef)
		{
			io_stream
			    >> o_thisRef.m_credibility
			    >> o_thisRef.m_crediblePosition
			    >> o_thisRef.m_predictedMotionTicCount
			    ;
			return io_stream;
		}

	protected:

		// Start off fully-credible so that triggers and other things can fire immediately if needed on the client
		// and so that the server doesn't have to do anything with this - everything on the server is credible.
		//
		CredibilityEnum m_credibility { CredibilityEnum::FULLY_CREDIBLE };
		v3fixed_t       m_crediblePosition { 0, 0, 0 };
		int             m_predictedMotionTicCount { 0 };
};

// Map Object definition.
class AActor : public DThinker
{
	DECLARE_SERIAL (AActor, DThinker)
	using AActorPtr = szp<AActor>;
	AActorPtr self;

	class AActorPtrCounted
	{
		AActorPtr ptr;

		public:

		AActorPtrCounted() = default;

		// TODO: should these be returning AActorPtrCounted& instead?
		// clang-tidy gives warnings about this
		AActorPtr &operator= (const AActorPtr& other)
		{
			if(ptr)
				ptr->refCount--;
			if(other)
				const_cast<AActorPtr&>(other)->refCount++; // TODO: should refCount maybe be declared as mutable?
			ptr = other;
			return ptr;
		}

		AActorPtr &operator= (const AActorPtrCounted& other)
		{
			if(ptr)
				ptr->refCount--;
			if(other)
				const_cast<AActorPtrCounted&>(other)->refCount++; // TODO: should refCount maybe be declared as mutable?
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

		operator const AActorPtr() const
		{
			return ptr;
		}
		operator const AActor*() const
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
		const AActor &operator *() const
		{
			return *ptr;
		}
		const AActor *operator ->() const
		{
			return ptr;
		}
	};

public:
	AActor ();
	AActor (const AActor &other);
	AActor &operator= (const AActor &other);
	AActor (fixed_t x, fixed_t y, fixed_t z, int32_t type);
	void Destroy () override;
	~AActor () override;

	void* operator new(size_t size);
	void operator delete(void* ptr, size_t size);

	void RunThink () override;
	void PostThink() override;

    // Info for drawing: position.
    fixed_t		x = 0;
    fixed_t		y = 0;
    fixed_t		z = 0;

	fixed_t		prevx = 0;
	fixed_t		prevy = 0;
	fixed_t		prevz = 0;

	AActor		*snext = nullptr, **sprev = nullptr;	// links in sector (if needed)

    //More drawing info: to determine current sprite.
    angle_t		angle = 0;	// orientation
	angle_t		prevangle = 0;
    int32_t		sprite = SPR_UNKN;	// used to find patch_t and flip value
    int			frame = 0;	// might be ORed with FF_FULLBRIGHT
	fixed_t		pitch = 0;
	angle_t		prevpitch = 0;

	uint32_t	effects = 0;			// [RH] see p_effect.h

	struct subsector_t *subsector = nullptr;

	// Keep these values together because they're hit by R_ProjectSprite,
	// PIT_FindTarget, and PIT_CheckThing, which are heavy hitters on maps
	// with a ton of monsters. This way, the cache can more easily hit
	// due to locality.
	ActorFlags1		flags  = ActorFlags1::none_set();
	ActorFlags2		flags2 = ActorFlags2::none_set(); // Heretic flags
	ActorFlags3		flags3 = ActorFlags3::none_set(); // MBF21 flags
	ActorOFlags		oflags = ActorOFlags::none_set(); // Odamex flags
	int				statusflags = 0; // Flags indicating a players status to other players
	int 			health = 0;
	int32_t			type = MT_UNKNOWNTHING;
	fixed_t			translucency = 0;	// 65536=fully opaque, 0=fully invisible
	translationref_t translation;	// Translation table (or NULL)

	// Additional info record for player avatars only.
	// Only valid if type == MT_PLAYER
	player_t*	player = nullptr;

    // The closest interval over all contacted Sectors.
    fixed_t		floorz = 0;
    fixed_t		ceilingz = 0;
	fixed_t		dropoffz = 0;
	struct sector_t	*floorsector = nullptr;

    // For movement checking.
    fixed_t		radius = 0;
    fixed_t		height = 0;

    // Momentums, used to update position.
    fixed_t		momx = 0;
    fixed_t		momy = 0;
    fixed_t		momz = 0;

    // If == validcount, already checked.
    int			validcount = 0;

    mobjinfo_t*		info = nullptr;	// &mobjinfo[mobj->type]
    int				tics = 0;	// state tic counter
	const state_t	*state = nullptr;
	int				damage = 0;			// For missiles
	int				special1 = 0;		// Special info
	int				special2 = 0;		// Special info

    // Movement direction, movement generation (zig-zagging).
    byte			movedir = 0;	// 0-7
    int				movecount = 0;	// when 0, select a new dir
	char			visdir = 0;

    // Thing being chased/attacked (or NULL),
    // also the originator for missiles.
	AActorPtr		target;
	AActorPtr		lastenemy;		// Last known enemy -- killogh 2/15/98

    // Reaction time: if non 0, don't attack yet.
    // Used by player to freeze a bit after teleporting.
    int				reactiontime = 0;

    // If >0, the target will be chased
    // no matter what (even if shot)
    int			threshold = 0;

    // Player number last looked for.
    unsigned int	lastlook = 0;

    // For nightmare respawn.
    mapthing2_t		spawnpoint{};

	// Thing being chased/attacked for tracers.
	AActorPtr           tracer;
	byte                special = 0;    // special
	std::array<byte, 5> args = {};       // special arguments

	AActor			*inext = nullptr, *iprev = nullptr;	// Links to other mobjs in same bucket

	ActorAwarenessState<MAXPLAYERS> playersAware;

	AActorPtr		goal;			// Monster's goal if not chasing anything
	byte			waterlevel = 0;		// 0=none, 1=feet, 2=waist, 3=eyes
	int16_t			gear = 0;			// killough 11/98: used in torque simulation

	bool			onground = false;		// NES - Fixes infinite jumping bug like a charm.

	// a linked list of sectors where this object appears
	struct msecnode_t	*touching_sectorlist = nullptr;				// phares 3/14/98

	int16_t           deadtic = 0;        // tics after player's death

	unsigned char   rndindex = 0;       // denis - because everything should have a random number generator, for prediction
	unsigned char   spawnRndindex = 0;

	byte friend_playerid = 0; // playerid of the player who spawned this actor

	team_t friend_teamid = TEAM_NONE; // team of the player who spawned this actor

	// killough 9/9/98: How long a monster pursues a target.
	int16_t pursuecount = 0;

	// killough 9/8/98: monster strafing
	int16_t strafecount = 0;

	// ThingIDs
	static void ClearTIDHashes ();
	void AddToHash ();
	void RemoveFromHash ();
	[[nodiscard]] AActor *FindByTID (int tid) const;
	[[nodiscard]] static AActor *FindByTID (const AActor *first, int tid);
	[[nodiscard]] AActor *FindGoal (int tid, int kind) const;
	[[nodiscard]] static AActor *FindGoal (const AActor *first, int tid, int kind);

	uint32_t		netid = 0;          // every object has its own netid
	int16_t			tid = 0;			// thing identifier
	baseline_t		baseline{};		// Baseline data for mobj sent to clients
	bool			baseline_set = false;	// Have we set our baseline yet?

	// Server: The primary mode that the mobj is in.  Dictated by top-level state transitions.
	// Client: Latched primary mode.  If different from the server, the state is transitioned to follow along accordingly.
	MobjModeEnum mode = MobjModeEnum::SPAWN;

	// Server: The tic on which this mobj was sent an UpdateMobj.
	// Client: the tic on which this mobj received an UpdateMobj.
	int updatedDuringLocalTic = -1;

	int updatedDuringServerTic = -1;

	// Server:  The tic on which this mobj was actually spawned. Used to for determining correct initial
	//          state and rnd index to send to clients.  *Not communicated to the client.*
	int spawnTic;

	// Client:  The monotonic "gametic" for this mobj - use this to perform mobj-internal timed operations
	//          so that the predictions occur with the same order and delay that they do on the server.
	int mobjtic;

	CredibilityState credibility;

private:
	static constexpr size_t TIDHashSize = 256;
	static constexpr size_t TIDHashMask = TIDHashSize - 1;
	static AActor *TIDHash[TIDHashSize];
	static int TIDHASH (int key) { return key & TIDHashMask; }

	friend class FActorIterator;

	void ClearFriendly();
	void RemoveFromActorList();

public:
	//
	// ActorClassList
	//
	// Intrusive doubly-linked list of friendly/hostile monsters, equivalent
	// to MBF's thinkerclasscap[th_friends/th_enemies] chains.
	//
	class ActorClassList
	{
	public:
		[[nodiscard]] AActor* Head() const { return m_head; }
		[[nodiscard]] bool empty() const { return m_head == nullptr; }
		[[nodiscard]] size_t Count() const { return m_count; }
		void Append(AActor* mo);
		void Remove(AActor* mo);
		void MoveFrontToEnd(AActor* upto);
		void Clear();

	private:
		AActor* m_head = nullptr;
		AActor* m_tail = nullptr;
		size_t m_count = 0;
	};

	// next/previous actor in s_friendlies/s_hostiles; managed by ActorClassList
	AActor* tlnext = nullptr;
	AActor* tlprev = nullptr;

	// next/previous actor in the list of all actors; linked on construction,
	// unlinked on destruction.  Iterating this instead of the thinker list
	// skips the runtime type check on every thinker.
	AActor* anext = nullptr;
	AActor* aprev = nullptr;

	static AActor* FirstActor() { return s_allhead; }

	// next/previous actor in the list of actors with nonzero effects, so
	// P_RunEffects only visits actors that actually have effects. Always
	// assign effects through SetEffects to keep the list in sync.
	AActor* enext = nullptr;
	AActor* eprev = nullptr;

	void SetEffects(uint32_t neweffects);
	static AActor* FirstEffectsActor() { return s_effectshead; }

private:
	static AActor* s_effectshead;

	void LinkEffectsList();
	void UnlinkEffectsList();

	// the class list this actor is currently linked into, if any
	ActorClassList* tlist = nullptr;

	// Actor lists to make friendly/unfriendly targeting
	// a little less taxing...
	static ActorClassList s_friendlies;
	static ActorClassList s_hostiles;

	// intrusive list of every AActor in existence
	static AActor* s_allhead;
	static AActor* s_alltail;

	void LinkAllActorsList();
	void UnlinkAllActorsList();

public:

	void LinkToWorld ();
	void UnlinkFromWorld ();

	void SetOrigin (fixed_t x, fixed_t y, fixed_t z);
	void ResetFlagsToDefault();

	[[nodiscard]] OUtil::SafeBool IsFriendly() const { return flags & MF_FRIEND; }
	void SetFriendly(OUtil::SafeBool isFriendly, const AActor* owner);
	void UpdateActorLists();
	static void ClearActorLists();
	static ActorClassList& GetFriendlies() { return s_friendlies; }
	static ActorClassList& GetHostiles() { return s_hostiles; }

	AActorPtr ptr() { return self; }

	//
	// ActorBlockMapListNode
	//
	// [SL] A container for the linked list nodes for all of the mapblocks that
	// an actor can be standing in.  Vanilla Doom only considered an actor to
	// be in the mapblock where its center was located, even if it was
	// overlapping other blocks.
	//
	// The per-block next/prev links live in a fixed-size array inside the
	// actor itself; nearly every actor spans at most a 3x3 block area
	// (radius up to 128 covers the spider mastermind), so walking a blockmap
	// chain never chases a separate heap allocation per actor. Larger
	// custom actors fall back to a heap array.
	//
	class ActorBlockMapListNode
	{
	public:
		explicit ActorBlockMapListNode(AActor* mo);
		ActorBlockMapListNode(const ActorBlockMapListNode& other);
		ActorBlockMapListNode& operator=(const ActorBlockMapListNode& other);
		~ActorBlockMapListNode();

		void Link();
		void Unlink();

		[[nodiscard]]
		AActor* Next(int bmx, int bmy) const
		{
			if (not blockmap.containsCoordinate(bmx, bmy))
				return nullptr;

			return m_next[getIndex(bmx, bmy)];
		}

	private:
		void clear();
		void ensureCapacity(int blockcnt);
		void copyFrom(const ActorBlockMapListNode& other);

		[[nodiscard]]
		size_t getIndex(int bmx, int bmy) const
		{
			// Out-of-range queries (including the cleared state and the
			// vanilla single-block case) fall back to index 0, which is
			// always a valid slot.
			if (bmx < m_originx || bmx > m_originx + m_blockcntx - 1 ||
				bmy < m_originy || bmy > m_originy + m_blockcnty - 1)
				return 0;

			return ((bmy - m_originy) * m_blockcntx) + bmx - m_originx;
		}

		// the number of block links stored inline before falling back to
		// the heap - covers actors up to radius 128 (a 3x3 block area)
		static const int INLINE_BLOCKS = 9;

		AActor* m_actor;

		// the top-left blockmap the actor is in
		int m_originx;
		int m_originy;
		// the number of blocks the actor occupies
		int m_blockcntx;
		int m_blockcnty;

		// the next and previous actors in each of the possible blockmaps
		// this actor can inhabit - point at the inline arrays unless the
		// actor spans more than INLINE_BLOCKS blocks
		int m_capacity;
		std::array<AActor*,  INLINE_BLOCKS> m_inlinenext{};
		std::array<AActor**, INLINE_BLOCKS> m_inlineprev{};
		AActor**  m_next;
		AActor*** m_prev;
	};

	// Interaction info, by BLOCKMAP.
	// Links in blocks (if needed).
	ActorBlockMapListNode bmapnode;
};

using AActors = std::vector<AActor::AActorPtr>;

class FActorIterator
{
public:
	FActorIterator (int i) : id (i)
	{
	}
	AActor *Next ()
	{
		if (id == 0)
			return nullptr;
		if (!base)
			base = AActor::FindByTID(NULL, id);
		else
			base = base->inext;

		while (base && base->tid != id)
			base = base->inext;

		return base;
	}
private:
	AActor *base = nullptr;
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
