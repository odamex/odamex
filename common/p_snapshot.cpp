// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2000-2006 by Sergey Makovkin (CSDoom .62).
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
//	Stores a limited view of actor, player, and sector objects at a particular
//  point in time.  Used with Unlagged, client-side prediction, and positional
//  interpolation
//
//-----------------------------------------------------------------------------


#include <math.h>
#include "actor.h"
#include "d_player.h"
#include "p_local.h"
#include "p_spec.h"
#include "m_vectors.h"

#include "p_snapshot.h"

static const int MAX_EXTRAPOLATION = 4;

static const fixed_t POS_LERP_THRESHOLD = 2 * FRACUNIT;
static const fixed_t SECTOR_LERP_THRESHOLD = 2 * FRACUNIT;

extern bool predicting;

// ============================================================================
//
// Snapshot implementation
//
// ============================================================================

Snapshot::Snapshot(int time) :
		mTime(time), mValid(time > 0),
		mAuthoritative(false), mContinuous(true),
		mInterpolated(false), mExtrapolated(false)
{
}

bool Snapshot::operator==(const Snapshot &other) const
{
	return	mTime == other.mTime &&
			mValid == other.mValid &&
			mAuthoritative == other.mAuthoritative &&
			mContinuous == other.mContinuous &&
			mInterpolated == other.mInterpolated &&
			mExtrapolated == other.mExtrapolated;
}

// ============================================================================
//
// ActorSnapshot implementation
//
// ============================================================================

ActorSnapshot::ActorSnapshot(int time) :
		Snapshot(time), mFields(0),
		mX(0), mY(0), mZ(0),
		mMomX(0), mMomY(0), mMomZ(0), mAngle(0), mPitch(0), mOnGround(true),
		mCeilingZ(0), mFloorZ(0), mReactionTime(0), mWaterLevel(0),
		mFlags(0), mFlags2(0), mFrame(0)
{
}
	
ActorSnapshot::ActorSnapshot(int time, const AActor *mo) :
		Snapshot(time), mFields(0xFFFFFFFF),
		mX(mo->x), mY(mo->y), mZ(mo->z),
		mMomX(mo->momx), mMomY(mo->momy), mMomZ(mo->momz),
		mAngle(mo->angle), mPitch(mo->pitch), mOnGround(mo->onground),
		mCeilingZ(mo->ceilingz), mFloorZ(mo->floorz),
		mReactionTime(mo->reactiontime), mWaterLevel(mo->waterlevel),
		mFlags(mo->flags), mFlags2(mo->flags2), mFrame(mo->frame)
{
}

bool ActorSnapshot::operator==(const ActorSnapshot &other) const
{
	return	Snapshot::operator==(other) &&
			mFields == other.mFields &&
			mX == other.mX &&
			mY == other.mY &&
			mZ == other.mZ &&
			mMomX == other.mMomX &&
			mMomY == other.mMomY &&
			mMomZ == other.mMomZ &&
			mAngle == other.mAngle &&
			mPitch == other.mPitch &&
			mOnGround == other.mOnGround &&
			mCeilingZ == other.mCeilingZ &&
			mFloorZ == other.mFloorZ &&
			mReactionTime == other.mReactionTime &&
			mWaterLevel == other.mWaterLevel &&
			mFlags == other.mFlags &&
			mFlags2 == other.mFlags2 &&
			mFrame == other.mFrame;
}

void ActorSnapshot::merge(const ActorSnapshot& other)
{
	if (other.mFields & ACT_POSITIONX)
		setX(other.mX);
	if (other.mFields & ACT_POSITIONY)
		setY(other.mY);
	if (other.mFields & ACT_POSITIONZ)
		setZ(other.mZ);
	if (other.mFields & ACT_MOMENTUMX)
		setMomX(other.mMomX);
	if (other.mFields & ACT_MOMENTUMY)
		setMomY(other.mMomY);
	if (other.mFields & ACT_MOMENTUMZ)
		setMomZ(other.mMomZ);
	if (other.mFields & ACT_ANGLE)
		setAngle(other.mAngle);
	if (other.mFields & ACT_PITCH)
		setPitch(other.mPitch);
	if (other.mFields & ACT_ONGROUND)
		setOnGround(other.mOnGround);
	if (other.mFields & ACT_CEILINGZ)
		setCeilingZ(other.mCeilingZ);
	if (other.mFields & ACT_FLOORZ)
		setFloorZ(other.mFloorZ);
	if (other.mFields & ACT_REACTIONTIME)
		setReactionTime(other.mReactionTime);
	if (other.mFields & ACT_WATERLEVEL)
		setWaterLevel(other.mWaterLevel);
	if (other.mFields & ACT_FLAGS)
		setFlags(other.mFlags);
	if (other.mFields & ACT_FLAGS2)
		setFlags2(other.mFlags2);
	if (other.mFields & ACT_FRAME)
		setFrame(other.mFrame);

	if (other.isContinuous())
		setContinuous(true);
	if (other.isAuthoritative())
		setAuthoritative(true);
}

void ActorSnapshot::toActor(AActor *mo) const
{
	if (!mo)
		return;
		
	if (mFields & (ACT_POSITIONX | ACT_POSITIONY | ACT_POSITIONZ))
	{
		fixed_t destx = mX, desty = mY, destz = mZ;
		if (isInterpolated() || isExtrapolated())
		{
			// need to check for collisions before moving
			P_TestActorMovement(mo, mX, mY, mZ, destx, desty, destz);
			
			#ifdef _SNAPSHOT_DEBUG_
			if (mX != destx || mY != desty || mZ != destz)
				Printf(PRINT_HIGH, "Snapshot %i: ActorSnapshot::toActor() clipping movement.\n", getTime()); 
			#endif // _SNAPSHOT_DEBUG_
		}
		
		// [SL] 2011-11-06 - Avoid setting the actor's floorz value if it hasn't moved.
		// This ensures the floorz value is correct for actors that have spawned too
		// close to a ledge but have not yet moved.
		if (mo->x != destx || mo->y != desty || mo->z != destz)
		{
			P_CheckPosition(mo, destx, desty);

			mo->UnlinkFromWorld();
			mo->x = destx;
			mo->y = desty;
			mo->z = destz;
		
			mo->ceilingz = tmceilingz;
			mo->floorz = tmfloorz;
			mo->dropoffz = tmdropoffz;
			mo->floorsector = tmfloorsector;	
			
			mo->LinkToWorld();
		}	
	}
		
	if (mFields & (ACT_MOMENTUMX | ACT_MOMENTUMY | ACT_MOMENTUMZ))
	{
		mo->momx = mMomX;
		mo->momy = mMomY;
		mo->momz = mMomZ;
	}
	
	// Only set a player's angle if he is alive.  Otherwise it will
	// interfere with the deathcam 
	if (mFields & ACT_ANGLE && (!mo->player || mo->player->playerstate != PST_DEAD))
		mo->angle = mAngle;
		
	if (mFields & ACT_PITCH)
		mo->pitch = mPitch;
	if (mFields & ACT_ONGROUND)
		mo->onground = mOnGround;
	if (mFields & ACT_CEILINGZ)
		mo->ceilingz = mCeilingZ;
	if (mFields & ACT_FLOORZ)
		mo->floorz = mFloorZ;		
	if (mFields & ACT_REACTIONTIME)
		mo->reactiontime = mReactionTime;
	if (mFields & ACT_WATERLEVEL)
		mo->waterlevel = mWaterLevel;
	if (mFields & ACT_FLAGS)
		mo->flags = mFlags;
	if (mFields & ACT_FLAGS2)
		mo->flags2 = mFlags2;
	if (mFields & ACT_FRAME)
		mo->frame = mFrame;
}

// ============================================================================
//
// PlayerSnapshot implementation
//
// ============================================================================

PlayerSnapshot::PlayerSnapshot(int time) :
		Snapshot(time), mFields(0),
		mActorSnap(time),
		mViewHeight(0), mDeltaViewHeight(0), mJumpTime(0)
{
}

PlayerSnapshot::PlayerSnapshot(int time, player_t *player) :
		Snapshot(time), mFields(0xFFFFFFFF),
		mActorSnap(time, player->mo),
		mViewHeight(player->viewheight), mDeltaViewHeight(player->deltaviewheight),
		mJumpTime(player->jumpTics)
{
}

bool PlayerSnapshot::operator==(const PlayerSnapshot &other) const
{
	return	Snapshot::operator==(other) &&
			mFields == other.mFields &&
			mActorSnap == other.mActorSnap &&
			mViewHeight == other.mViewHeight &&
			mDeltaViewHeight == other.mDeltaViewHeight &&
			mJumpTime == other.mJumpTime;
}

void PlayerSnapshot::merge(const PlayerSnapshot& other)
{
	mActorSnap.merge(other.mActorSnap);
	
	if (other.mFields & PLY_VIEWHEIGHT)
		setViewHeight(other.mViewHeight);
	if (other.mFields & PLY_DELTAVIEWHEIGHT)
		setDeltaViewHeight(other.mDeltaViewHeight);
	if (other.mFields & PLY_JUMPTIME)
		setJumpTime(other.mJumpTime);
}

void PlayerSnapshot::toPlayer(player_t *player) const
{
	if (!player || !player->mo)
		return;
		
	mActorSnap.toActor(player->mo);
	
	if (mFields & PLY_VIEWHEIGHT)
		player->viewheight = mViewHeight;
	if (mFields & PLY_DELTAVIEWHEIGHT)
		player->deltaviewheight = mDeltaViewHeight;
	if (mFields & PLY_JUMPTIME)
		player->jumpTics = mJumpTime;
}

	
// ============================================================================
//
// PlayerSnapshotManager implementation
//
// ============================================================================

PlayerSnapshotManager::PlayerSnapshotManager() :
	mMostRecent(0)
{
	clearSnapshots();
}

//
// PlayerSnapshotManager::clearSnapshots()
//
// Marks all of the snapshots in the container invalid, effectively
// clearing the container.
//
void PlayerSnapshotManager::clearSnapshots()
{
	// Set the time for all snapshots to an invalid value
	for (int i = 0; i < NUM_SNAPSHOTS; i++)
		mSnaps[i].setTime(-1);
		
	mMostRecent = 0;
}

//
// PlayerSnapshotManager::mValidSnapshot()
//
// Returns true if a snapshot at the given time is present in the container
//
bool PlayerSnapshotManager::mValidSnapshot(int time) const
{

	return ((time <= mMostRecent) && (mMostRecent - time <= NUM_SNAPSHOTS) &&
			(time > 0) && (mSnaps[time % NUM_SNAPSHOTS].isValid()) &&
			(mSnaps[time % NUM_SNAPSHOTS].getTime() == time));
}

//
// PlayerSnapshotManager::mInterpolateSnapshots()
//
// Returns a snapshot based on the snapshot at the time "to" with a position
// linearly interpolated between snapshots "from" and "to".
//
// Note: The caller of this function has the responsibility for verifying
// that the container has valid snapshots at the times "from" and "to".
//
PlayerSnapshot PlayerSnapshotManager::mInterpolateSnapshots(int from, int to, int time) const
{	
	// Assumes that range checking from and to has been performed by the caller
	const PlayerSnapshot *snapfrom = &mSnaps[from % NUM_SNAPSHOTS];
	const PlayerSnapshot *snapto = &mSnaps[to % NUM_SNAPSHOTS];
	
	if (to == from || !snapto->isContinuous())
		return *snapto;
		
	float amount = float(time - from) / float(to - from);
	
	return P_LerpPlayerPosition(*snapfrom, *snapto, amount);
}

//
// PlayerSnapshotManager::mExtrapolateSnapshots()
//
// Returns a snapshot based on the snapshot at the time "from" with an
// estimated position calculated using the player's velocity.
//
// Note: The caller of this function has the responsibility for verifying
// that the container has valid snapshots at the time "from".
//
PlayerSnapshot PlayerSnapshotManager::mExtrapolateSnapshot(int from, int time) const
{
	// Assumes that range checking from has been performed by the caller
	const PlayerSnapshot *snapfrom = &mSnaps[from % NUM_SNAPSHOTS];
	
	float amount = time - from;
	
	return P_ExtrapolatePlayerPosition(*snapfrom, amount);
}

//
// PlayerSnapshotManager::addSnapshot()
//
// Inserts a new snapshot into the container, provided it is valid and not
// too old
//
void PlayerSnapshotManager::addSnapshot(const PlayerSnapshot &snap)
{
	int time = snap.getTime();
	
	if (!snap.isValid())
	{
		#ifdef _SNAPSHOT_DEBUG_
		Printf(PRINT_HIGH, "Snapshot %i: Not adding invalid player snapshot\n", time);
		#endif // _SNAPSHOT_DEBUG_
		return;
	}

	if (mMostRecent > snap.getTime() + NUM_SNAPSHOTS)
	{
		#ifdef _SNAPSHOT_DEBUG_
		Printf(PRINT_HIGH, "Snapshot %i: Not adding expired player snapshot\n", time);
		#endif // _SNAPSHOT_DEBUG_
		return;
	}

	PlayerSnapshot& dest = mSnaps[time % NUM_SNAPSHOTS];
	if (dest.getTime() == time)
	{
		// A valid snapshot for this time already exists. Merge it with this one.
		dest.merge(snap);
	}
	else
	{
		dest = snap;
	}

	if (time > mMostRecent)
		mMostRecent = time;
}

int PlayerSnapshotManager::mFindValidSnapshot(int starttime, int endtime) const
{
	if (starttime < mMostRecent - NUM_SNAPSHOTS || 
		endtime < mMostRecent - NUM_SNAPSHOTS ||
		starttime > mMostRecent || endtime > mMostRecent)
		return -1;
	
	if (endtime >= starttime)
	{
		for (int t = starttime; t <= endtime; t++)
			if (mValidSnapshot(t))
				return t;
	}
	else
	{
		for (int t = starttime; t >= endtime; t--)
			if (mValidSnapshot(t))
				return t;	
	}
	
	// Did not find any valid snapshots
	return -1;
}

//
// PlayerSnapshotManager::getSnapshot()
//
// Returns a snapshot from the container at a specified time.
// If there is not a snapshot matching the time, one is generated using
// interpolation or extrapolation.
//
PlayerSnapshot PlayerSnapshotManager::getSnapshot(int time) const
{
	if (time <= 0 || mMostRecent <= 0)
		return PlayerSnapshot();
		
	// Return the requested snapshot if availible
	if (mValidSnapshot(time))
		return mSnaps[time % NUM_SNAPSHOTS];
	
	// Should we extrapolate?
	if (time > mMostRecent)
	{
		int amount = time - mMostRecent;
		if (amount > MAX_EXTRAPOLATION)		// cap extrapolation
		{
			#ifdef _SNAPSHOT_DEBUG_
			Printf(PRINT_HIGH, "Extrap %i: PlayerSnapshotManager::getSnapshot() capping extrapolation past %i\n",
					time, mMostRecent);
			#endif // _SNAPSHOT_DEBUG_
			
			amount = MAX_EXTRAPOLATION;
		}
		
		#ifdef _SNAPSHOT_DEBUG_
		Printf(PRINT_HIGH, "Extrap %i: PlayerSnapshotManager::getSnapshot() extrapolating past %i\n",
				time, mMostRecent);
		#endif // _SNAPSHOT_DEBUG_

		return mExtrapolateSnapshot(mMostRecent, amount + mMostRecent);
	}
	
	// find the snapshot that precedes the desired time
	int pretime = mFindValidSnapshot(time, mMostRecent - NUM_SNAPSHOTS);

	// find the snapshot that follows the desired time
	int posttime = mFindValidSnapshot(time, mMostRecent);
	
	// Can we interpolate?
	if (pretime > 0 && posttime > 0 && time < posttime && time > pretime)
	{
		#ifdef _SNAPSHOT_DEBUG_
		Printf(PRINT_HIGH, "Lerp %i: PlayerSnapshotManager::getSnapshot() interpolating between %i and %i.\n",
					time, pretime, posttime);
		#endif // _SNAPSHOT_DEBUG_
		
		return mInterpolateSnapshots(pretime, posttime, time);
	}
	
	// No snapshots before the desired time?  Return the closest one to it
	if (pretime <= 0 && posttime > 0)
		return mSnaps[posttime % NUM_SNAPSHOTS];

	// Could not find a valid snapshot so return a blank (invalid) one
	return PlayerSnapshot();
}


// ============================================================================
//
// Actor and Player Helper functions
//
// ============================================================================

static fixed_t P_PositionDifference(const v3fixed_t &a, const v3fixed_t &b)
{
	v3fixed_t diff;
	M_SubVec3Fixed(&diff, &b, &a);

	return M_LengthVec3Fixed(&diff);
}

//
// P_ExtrapolateActorPosition
//
//
ActorSnapshot P_ExtrapolateActorPosition(const ActorSnapshot &from, float amount)
{
	fixed_t amount_fixed = FLOAT2FIXED(amount);
	
	if (amount_fixed <= 0)
		return from;
	
	v3fixed_t velocity, pos_new;
	M_SetVec3Fixed(&pos_new, from.getX(), from.getY(), from.getZ());
	M_SetVec3Fixed(&velocity, from.getMomX(), from.getMomY(), from.getMomZ());
	M_ScaleVec3Fixed(&velocity, &velocity, amount_fixed);
	M_AddVec3Fixed(&pos_new, &pos_new, &velocity);
			
	ActorSnapshot newsnapshot(from);
	newsnapshot.setAuthoritative(false);
	newsnapshot.setExtrapolated(true);
	newsnapshot.setX(pos_new.x);
	newsnapshot.setY(pos_new.y);	
	newsnapshot.setZ(pos_new.z);	

	return newsnapshot;	
}


PlayerSnapshot P_ExtrapolatePlayerPosition(const PlayerSnapshot &from, float amount)
{
	PlayerSnapshot newsnap(from);
	newsnap.mActorSnap = P_ExtrapolateActorPosition(from.mActorSnap, amount);
	newsnap.setExtrapolated(newsnap.mActorSnap.isExtrapolated());
	
	return newsnap;
}


//
// P_LerpActorPosition
//
// Returns a snapshot containing a position that is a percentage of the distance
// between the positions contained in from and to.  Besides the position, the
// returned snapshot will be identical to the snapshot to.
//
// Note that amount will most often be between 0.0 and 1.0, however it can be
// larger than 1.0 as an alternative method of extrapolating a position.
//
ActorSnapshot P_LerpActorPosition(const ActorSnapshot &from, const ActorSnapshot &to, float amount)
{
	fixed_t amount_fixed = FLOAT2FIXED(amount);
	
	if (amount_fixed <= 0)
		return from;

	// Calculate the distance between the positions
	v3fixed_t pos_from, pos_to;
	M_SetVec3Fixed(&pos_from, from.getX(), from.getY(), from.getZ());
	M_SetVec3Fixed(&pos_to, to.getX(), to.getY(), to.getZ());
	fixed_t pos_delta = P_PositionDifference(pos_from, pos_to);

	#ifdef _SNAPSHOT_DEBUG_
	if (pos_delta)
		Printf(PRINT_HIGH, "Lerp: MF2_ONMOBJ = %s\n", from.getFlags2() & MF2_ONMOBJ ? "yes" : "no");
	#endif // _SNAPSHOT_DEBUG_
				
	if (pos_delta <= POS_LERP_THRESHOLD || !to.isContinuous())
	{
		// snap directly to the new position
		#ifdef _SNAPSHOT_DEBUG_
		if (pos_delta)
			Printf(PRINT_HIGH, "Lerp: %d Snapping to position (delta %d)\n",
								gametic, pos_delta >> FRACBITS);
		#endif // _SNAPSHOT_DEBUG_
		return to;
	}

	// Calculate the interpolated position between pos_from and pos_to
	v3fixed_t pos_new;
	M_SubVec3Fixed(&pos_new, &pos_to, &pos_from);
	M_ScaleVec3Fixed(&pos_new, &pos_new, amount_fixed);
	M_AddVec3Fixed(&pos_new, &pos_new, &pos_from);
	
	#ifdef _SNAPSHOT_DEBUG_
	Printf(PRINT_HIGH, "Lerp: %d, Lerping to position (delta %d)\n",
						gametic, pos_delta >> FRACBITS);
	#endif // _SNAPSHOT_DEBUG_

	// lerp the angle
	int anglediff = int(to.getAngle()) - int(from.getAngle());
	angle_t angle = from.getAngle() + FixedMul(anglediff, amount_fixed);	

	#ifdef _SNAPSHOT_DEBUG_
	if (anglediff)
		Printf(PRINT_HIGH, "Lerp: %d, Lerping to angle (delta %d)\n",
							gametic, anglediff >> ANGLETOFINESHIFT);
	#endif // _SNAPSHOT_DEBUG_

	ActorSnapshot newsnapshot(to);
	newsnapshot.setAuthoritative(false);
	newsnapshot.setInterpolated(true);
	newsnapshot.setX(pos_new.x);
	newsnapshot.setY(pos_new.y);
	newsnapshot.setZ(pos_new.z);
	newsnapshot.setAngle(angle);

	return newsnapshot;
}


PlayerSnapshot P_LerpPlayerPosition(const PlayerSnapshot &from, const PlayerSnapshot &to, float amount)
{
	PlayerSnapshot newsnap(to);
	newsnap.mActorSnap = P_LerpActorPosition(from.mActorSnap, to.mActorSnap, amount);
	newsnap.setInterpolated(newsnap.mActorSnap.isInterpolated());
	
	return newsnap;
}


//
// P_SetPlayerSnapshotNoPosition
//
// Handles the special case where we want the player's position to not change
// but would like to apply all of the other player characteristics from the
// snapshot to the player.
//
void P_SetPlayerSnapshotNoPosition(player_t *player, const PlayerSnapshot &snap)
{
	if (!player || !player->mo)
		return;
		
	fixed_t x = player->mo->x;
	fixed_t y = player->mo->y;
	fixed_t z = player->mo->z;
	fixed_t ceilingz = player->mo->ceilingz;
	fixed_t floorz = player->mo->floorz;
	fixed_t momx = player->mo->momx;
	fixed_t momy = player->mo->momy;
	fixed_t momz = player->mo->momz;
		
	snap.toPlayer(player);
	
	player->mo->UnlinkFromWorld();
	player->mo->x = x;
	player->mo->y = y;
	player->mo->z = z;
	player->mo->ceilingz = ceilingz;
	player->mo->floorz = floorz;
	player->mo->momx = momx;
	player->mo->momy = momy;
	player->mo->momz = momz;
	player->mo->LinkToWorld();
}


// ============================================================================
//
// SectorSnapshot implementation
//
// ============================================================================


SectorSnapshot::SectorSnapshot(int time) :
	Snapshot(time),	mCeilingMoverType(SEC_INVALID), mFloorMoverType(SEC_INVALID),
	mSector(NULL), mCeilingType(0), mFloorType(0), mCeilingTag(0), mFloorTag(0),
	mCeilingLine(NULL), mFloorLine(NULL), mCeilingHeight(0), mFloorHeight(0),
	mCeilingSpeed(0), mFloorSpeed(0), mCeilingDestination(0), mFloorDestination(0),
	mCeilingDirection(0), mFloorDirection(0), mCeilingOldDirection(0), mFloorOldDirection(0),
	mCeilingTexture(0), mFloorTexture(0),
	mNewCeilingSpecial(0), mNewFloorSpecial(0), mCeilingLow(0), mCeilingHigh(0),
	mFloorLow(0), mFloorHigh(0), mCeilingCrush(false), mFloorCrush(false), mSilent(false),
	mCeilingWait(0), mFloorWait(0), mCeilingCounter(0), mFloorCounter(0), mResetCounter(0),
	mCeilingStatus(0), mFloorStatus(0), mOldFloorStatus(0),
	mCrusherSpeed1(0), mCrusherSpeed2(0), mStepTime(0), mPerStepTime(0), mPauseTime(0),
	mOrgHeight(0), mDelay(0), mFloorLip(0), mFloorOffset(0), mCeilingChange(0), mFloorChange(0)
{
}

SectorSnapshot::SectorSnapshot(int time, sector_t *sector) :
	Snapshot(time), mCeilingMoverType(SEC_INVALID), mFloorMoverType(SEC_INVALID),
	mSector(sector)
{
	if (!sector)
	{
		clear();	
		return;
	}
	
	mCeilingHeight		= P_CeilingHeight(sector);
	mFloorHeight		= P_FloorHeight(sector);
	
	if (sector->floordata)
	{
		if (sector->floordata->IsA(RUNTIME_CLASS(DElevator)))
		{
			DElevator *elevator = static_cast<DElevator *>(sector->floordata);
			mCeilingMoverType	= SEC_ELEVATOR;
			mFloorMoverType		= SEC_ELEVATOR;
			mCeilingType		= elevator->m_Type;
			mFloorType			= elevator->m_Type;
			mCeilingStatus		= elevator->m_Status;
			mFloorStatus		= elevator->m_Status;
			mCeilingDirection	= elevator->m_Direction;
			mFloorDirection		= elevator->m_Direction;
			mCeilingDestination	= elevator->m_CeilingDestHeight;
			mFloorDestination	= elevator->m_FloorDestHeight;
			mCeilingSpeed		= elevator->m_Speed;
			mFloorSpeed			= elevator->m_Speed;
		}
		else if (sector->floordata->IsA(RUNTIME_CLASS(DPillar)))
		{
			DPillar *pillar		= static_cast<DPillar *>(sector->floordata);
			mCeilingMoverType	= SEC_PILLAR;
			mFloorMoverType		= SEC_PILLAR;	
			mCeilingType		= pillar->m_Type;
			mFloorType			= pillar->m_Type;
			mCeilingStatus		= pillar->m_Status;
			mFloorStatus		= pillar->m_Status;		
			mCeilingSpeed		= pillar->m_CeilingSpeed;
			mFloorSpeed			= pillar->m_FloorSpeed;
			mCeilingDestination	= pillar->m_CeilingTarget;
			mFloorDestination	= pillar->m_FloorTarget;
			mCeilingCrush		= pillar->m_Crush;
			mFloorCrush			= pillar->m_Crush;
		}
		else if (sector->floordata->IsA(RUNTIME_CLASS(DFloor)))
		{
			DFloor *floor		= static_cast<DFloor *>(sector->floordata);
			mFloorMoverType		= SEC_FLOOR;			
			mFloorType			= floor->m_Type;
			mFloorStatus		= floor->m_Status;
			mFloorCrush			= floor->m_Crush;
			mFloorDirection		= floor->m_Direction;
			mNewFloorSpecial	= floor->m_NewSpecial;
			mFloorTexture		= floor->m_Texture;
			mFloorDestination	= floor->m_FloorDestHeight;
			mFloorSpeed			= floor->m_Speed;
			mStepTime			= floor->m_StepTime;
			mPerStepTime		= floor->m_PerStepTime;	
			mResetCounter		= floor->m_ResetCount;
			mPauseTime			= floor->m_PauseTime;
			mDelay				= floor->m_Delay;
			mOrgHeight			= floor->m_OrgHeight;
			mFloorLine			= floor->m_Line;
			mFloorOffset		= floor->m_Height;
			mFloorChange		= floor->m_Change;
		}
		else if (sector->floordata->IsA(RUNTIME_CLASS(DPlat)))
		{
			DPlat *plat			= static_cast<DPlat *>(sector->floordata);
			mFloorMoverType		= SEC_PLAT;			
			mFloorType			= plat->m_Type;
			mFloorTag			= plat->m_Tag;			
			mFloorCrush			= plat->m_Crush;
			mFloorSpeed			= plat->m_Speed;
			mFloorLow			= plat->m_Low;
			mFloorHigh			= plat->m_High;
			mFloorWait			= plat->m_Wait;
			mFloorCounter		= plat->m_Count;
			mFloorStatus		= plat->m_Status;
			mOldFloorStatus		= plat->m_OldStatus;
			mFloorOffset		= plat->m_Height;
			mFloorLip			= plat->m_Lip;
		}
	}
	
	if (sector->ceilingdata)
	{
		if (sector->ceilingdata->IsA(RUNTIME_CLASS(DCeiling)))
		{
			DCeiling *ceiling	= static_cast<DCeiling *>(sector->ceilingdata);
			mCeilingMoverType	= SEC_CEILING;			
			mCeilingType		= ceiling->m_Type;
			mCeilingStatus		= ceiling->m_Status;
			mCeilingTag			= ceiling->m_Tag;
			mCeilingCrush		= ceiling->m_Crush;
			mSilent				= (ceiling->m_Silent != 0);
			mCeilingLow			= ceiling->m_BottomHeight;
			mCeilingHigh		= ceiling->m_TopHeight;
			mCeilingSpeed		= ceiling->m_Speed;
			mCeilingDirection	= ceiling->m_Direction;
			mCeilingTexture		= ceiling->m_Texture;
			mNewCeilingSpecial	= ceiling->m_NewSpecial;
			mCrusherSpeed1		= ceiling->m_Speed1;
			mCrusherSpeed2		= ceiling->m_Speed2;
			mCeilingOldDirection = ceiling->m_OldDirection;
		}
		else if (sector->ceilingdata->IsA(RUNTIME_CLASS(DDoor)))
		{
			DDoor *door			= static_cast<DDoor *>(sector->ceilingdata);
			mCeilingMoverType	= SEC_DOOR;
			mCeilingType		= door->m_Type;
			mCeilingHigh		= door->m_TopHeight;
			mCeilingSpeed		= door->m_Speed;
			mCeilingWait		= door->m_TopWait;
			mCeilingCounter		= door->m_TopCountdown;
			mCeilingStatus		= door->m_Status;
			mCeilingLine		= door->m_Line;
		}
	}
}

void SectorSnapshot::clear()
{
	setTime(-1);
	mCeilingMoverType = SEC_INVALID;
	mFloorMoverType = SEC_INVALID;
}

void SectorSnapshot::toSector(sector_t *sector) const
{
	if (!sector)
		return;

	P_SetCeilingHeight(sector, mCeilingHeight);
	P_SetFloorHeight(sector, mFloorHeight);
	P_ChangeSector(sector, false);

	if (mCeilingMoverType == SEC_PILLAR && mCeilingStatus != DPillar::destroy)
	{
		if (sector->ceilingdata && !sector->ceilingdata->IsA(RUNTIME_CLASS(DPillar)))
		{
			sector->ceilingdata->Destroy();
			sector->ceilingdata = NULL;

		}
		if (sector->floordata && !sector->ceilingdata->IsA(RUNTIME_CLASS(DPillar)))
		{
			sector->floordata->Destroy();
			sector->floordata = NULL;
		}
		
		if (!sector->ceilingdata)
		{
			sector->ceilingdata = new DPillar();
			sector->floordata = sector->ceilingdata;
		}
		
		DPillar *pillar				= static_cast<DPillar *>(sector->ceilingdata);
		pillar->m_Type				= static_cast<DPillar::EPillar>(mCeilingType);
		pillar->m_Status			= static_cast<DPillar::EPillarState>(mCeilingStatus);
		pillar->m_CeilingSpeed		= mCeilingSpeed;
		pillar->m_FloorSpeed		= mFloorSpeed;
		pillar->m_CeilingTarget		= mCeilingDestination;
		pillar->m_FloorTarget		= mFloorDestination;
		pillar->m_Crush				= mCeilingCrush;
	}
	
	if (mCeilingMoverType == SEC_ELEVATOR && mCeilingStatus != DElevator::destroy)
	{
		if (sector->ceilingdata && !sector->ceilingdata->IsA(RUNTIME_CLASS(DElevator)))
		{
			sector->ceilingdata->Destroy();
			sector->ceilingdata = NULL;
		}
		if (sector->floordata && !sector->floordata->IsA(RUNTIME_CLASS(DElevator)))
		{
				sector->floordata->Destroy();
				sector->floordata = NULL;
		}
		
		if (!sector->ceilingdata)
		{
			sector->ceilingdata = new DElevator(sector);
			sector->floordata = sector->ceilingdata;
		}
	
		DElevator *elevator			= static_cast<DElevator *>(sector->ceilingdata);
		elevator->m_Type			= static_cast<DElevator::EElevator>(mCeilingType);
		elevator->m_Status			= static_cast<DElevator::EElevatorState>(mCeilingStatus);		
		elevator->m_Direction		= mCeilingDirection;
		elevator->m_CeilingDestHeight = mCeilingDestination;
		elevator->m_FloorDestHeight	= mFloorDestination;
		elevator->m_Speed			= mCeilingSpeed;
	}

	if (mCeilingMoverType == SEC_CEILING && mCeilingStatus != DCeiling::destroy)
	{
		if (sector->ceilingdata && !sector->ceilingdata->IsA(RUNTIME_CLASS(DCeiling)))
		{
			sector->ceilingdata->Destroy();
			sector->ceilingdata = NULL;
		}
		
		if (!sector->ceilingdata)
			sector->ceilingdata = new DCeiling(sector);
		
		DCeiling *ceiling			= static_cast<DCeiling *>(sector->ceilingdata);
		ceiling->m_Type				= static_cast<DCeiling::ECeiling>(mCeilingType);
		ceiling->m_Status			= static_cast<DCeiling::ECeilingState>(mCeilingStatus);
		ceiling->m_Tag				= mCeilingTag;
		ceiling->m_BottomHeight		= mCeilingLow;
		ceiling->m_TopHeight		= mCeilingHigh;
		ceiling->m_Speed			= mCeilingSpeed;
		ceiling->m_Speed1			= mCrusherSpeed1;
		ceiling->m_Speed2			= mCrusherSpeed2;
		ceiling->m_Crush			= mCeilingCrush;
		ceiling->m_Silent			= mSilent;
		ceiling->m_Direction		= mCeilingDirection;
		ceiling->m_OldDirection		= mCeilingOldDirection;
		ceiling->m_Texture			= mCeilingTexture;
		ceiling->m_NewSpecial		= mNewCeilingSpecial;
	}
		
	if (mCeilingMoverType == SEC_DOOR && mCeilingStatus != DDoor::destroy)
	{
		if (sector->ceilingdata && !sector->ceilingdata->IsA(RUNTIME_CLASS(DDoor)))
		{
			sector->ceilingdata->Destroy();
			sector->ceilingdata = NULL;
		}
		
		if (!sector->ceilingdata)
		{
			sector->ceilingdata =
				new DDoor(sector, mCeilingLine,
						  static_cast<DDoor::EVlDoor>(mCeilingType),
						  mCeilingSpeed, mCeilingWait);
		}

		DDoor *door					= static_cast<DDoor *>(sector->ceilingdata);
		door->m_Type				= static_cast<DDoor::EVlDoor>(mCeilingType);
		door->m_Status				= static_cast<DDoor::EDoorState>(mCeilingStatus);
		door->m_TopHeight			= mCeilingHigh;
		door->m_Speed				= mCeilingSpeed;
		door->m_TopWait				= mCeilingWait;
		door->m_TopCountdown		= mCeilingCounter;
		door->m_Line				= mCeilingLine;
	}

	if (mFloorMoverType == SEC_FLOOR && mFloorStatus != DFloor::destroy)
	{
		if (sector->floordata && !sector->floordata->IsA(RUNTIME_CLASS(DFloor)))
		{
			sector->floordata->Destroy();
			sector->floordata = NULL;
		}
		
		if (!sector->floordata)
		{
			sector->floordata =
				new DFloor(sector, static_cast<DFloor::EFloor>(mFloorType),
						   mFloorLine, mFloorSpeed, mFloorOffset,
						   mFloorCrush, mFloorChange);			
		}
		
		DFloor *floor				= static_cast<DFloor *>(sector->floordata);
		floor->m_Type				= static_cast<DFloor::EFloor>(mFloorType);
		floor->m_Status				= static_cast<DFloor::EFloorState>(mFloorStatus);
		floor->m_Crush				= mFloorCrush;
		floor->m_Direction			= mFloorDirection;
		floor->m_NewSpecial			= mNewFloorSpecial;
		floor->m_FloorDestHeight	= mFloorDestination;
		floor->m_Speed				= mFloorSpeed;
		floor->m_ResetCount			= mResetCounter;
		floor->m_OrgHeight			= mOrgHeight;
		floor->m_Delay				= mDelay;
		floor->m_PauseTime			= mPauseTime;
		floor->m_StepTime			= mStepTime;
		floor->m_PerStepTime		= mPerStepTime;
		floor->m_Line				= mFloorLine;
		floor->m_Height				= mFloorOffset;
		floor->m_Change				= mFloorChange;
	}
		
	if (mFloorMoverType == SEC_PLAT && mFloorStatus != DPlat::destroy)
	{
		if (sector->floordata && !sector->floordata->IsA(RUNTIME_CLASS(DPlat)))
		{
			sector->floordata->Destroy();
			sector->floordata = NULL;
		}
		
		if (!sector->floordata)
		{
			sector->floordata =
				new DPlat(sector, static_cast<DPlat::EPlatType>(mFloorType),
						  mFloorOffset, mFloorSpeed, mFloorWait, mFloorLip);
		}

		DPlat *plat					= static_cast<DPlat *>(sector->floordata);
		plat->m_Type				= static_cast<DPlat::EPlatType>(mFloorType);
		plat->m_Tag					= mFloorTag;
		plat->m_Status				= static_cast<DPlat::EPlatState>(mFloorStatus);
		plat->m_OldStatus			= static_cast<DPlat::EPlatState>(mOldFloorStatus);		
		plat->m_Crush				= mFloorCrush;
		plat->m_Low					= mFloorLow;
		plat->m_High				= mFloorHigh;
		plat->m_Speed				= mFloorSpeed;
		plat->m_Wait				= mFloorWait;
		plat->m_Count				= mFloorCounter;
		plat->m_Height				= mFloorOffset;
		plat->m_Lip					= mFloorLip;
	}
}

// ============================================================================
//
// SectorrSnapshotManager implementation
//
// ============================================================================

SectorSnapshotManager::SectorSnapshotManager() :
	mMostRecent(0)
{
	clearSnapshots();
}

//
// SectorSnapshotManager::clearSnapshots()
//
// Marks all of the snapshots in the container invalid, effectively
// clearing the container.
//
void SectorSnapshotManager::clearSnapshots()
{
	// Set the time for all snapshots to an invalid value
	for (int i = 0; i < NUM_SNAPSHOTS; i++)
		mSnaps[i].clear();
		
	mMostRecent = 0;
}

//
// SectorSnapshotManager::mValidSnapshot()
//
// Returns true if a snapshot at the given time is present in the container
//
bool SectorSnapshotManager::mValidSnapshot(int time) const
{
	return ((time <= mMostRecent) && (mMostRecent - time <= NUM_SNAPSHOTS) &&
			(time > 0) && (mSnaps[time % NUM_SNAPSHOTS].isValid()) &&
			(mSnaps[time % NUM_SNAPSHOTS].getTime() == time) &&
			(mSnaps[time % NUM_SNAPSHOTS].getCeilingMoverType() != SEC_INVALID ||
			 mSnaps[time % NUM_SNAPSHOTS].getFloorMoverType() != SEC_INVALID));
}

//
// SectorSnapshotManager::empty()
//
// Returns true if the container does not contain any valid snapshots 
//
bool SectorSnapshotManager::empty()
{
	return (!mValidSnapshot(mMostRecent));
}

//
// SectorSnapshotManager::addSnapshot()
//
// Inserts a new snapshot into the container, provided it is valid and not
// too old
//
void SectorSnapshotManager::addSnapshot(const SectorSnapshot &newsnap)
{
	int time = newsnap.getTime();
	
	if (!newsnap.isValid())
	{
		#ifdef _SNAPSHOT_DEBUG_
		Printf(PRINT_HIGH, "Snapshot %i: Not adding invalid sector snapshot\n", time);
		#endif // _SNAPSHOT_DEBUG_
		return;
	}
	
	if (mMostRecent > newsnap.getTime() + NUM_SNAPSHOTS)
	{
		#ifdef _SNAPSHOT_DEBUG_
		Printf(PRINT_HIGH, "Snapshot %i: Not adding expired sector snapshot\n", time);
		#endif // _SNAPSHOT_DEBUG_
		return;
	}

	mSnaps[time % NUM_SNAPSHOTS] = newsnap;

	if (time > mMostRecent)
		mMostRecent = time;
}


//
// SectorSnapshotManager::getSnapshot()
//
// Returns a snapshot from the container at a specified time.
// If there is not a snapshot matching the time, one is generated by
// running the moving sector's thinker function.
//
SectorSnapshot SectorSnapshotManager::getSnapshot(int time) const
{
	if (time <= 0 || mMostRecent <= 0)
		return SectorSnapshot();
	
	// Return the requested snapshot if availible
	if (mValidSnapshot(time))
		return mSnaps[time % NUM_SNAPSHOTS];
	
	// Find the snapshot in the container that preceeds the desired time
	int prevsnaptime = time;
	while (--prevsnaptime > mMostRecent - NUM_SNAPSHOTS)
	{
		if (mValidSnapshot(prevsnaptime))
		{
			const SectorSnapshot *snap = &mSnaps[prevsnaptime % NUM_SNAPSHOTS];
			
			// turn off any sector movement sounds from RunThink()
			bool oldpredicting = predicting;
			predicting = true;
		
			// create a temporary sector for the snapshot and run the
			// sector movement til we get to the desired time
			sector_t tempsector;
			P_CopySector(&tempsector, snap->getSector());
			
			// set values for the Z parameter of the sector's planes so that
			// P_SetCeilingHeight/P_SetFloorHeight will work properly
			tempsector.floorplane.c = tempsector.floorplane.invc = FRACUNIT;
			tempsector.ceilingplane.c = tempsector.ceilingplane.invc = -FRACUNIT;
						
			snap->toSector(&tempsector);

			for (int i = 0; i < time - prevsnaptime; i++)
			{
				if (tempsector.ceilingdata)
					tempsector.ceilingdata->RunThink();			
				if (tempsector.floordata && 
					tempsector.floordata != tempsector.ceilingdata)
					tempsector.floordata->RunThink();
			}
			
			SectorSnapshot newsnap(time, &tempsector);

			// clean up allocated memory
			if (tempsector.ceilingdata)
			{
				tempsector.ceilingdata->Destroy();
				delete tempsector.ceilingdata;
			}
				
			if (tempsector.floordata)
			{
				tempsector.floordata->Destroy();
				delete tempsector.floordata;
			}

			if (tempsector.lightingdata)
			{
				tempsector.lightingdata->Destroy();
				delete tempsector.lightingdata;
			}

			// restore sector movement sounds
			predicting = oldpredicting;
			
			return newsnap;
		}
	}
		
	// Could not find a valid snapshot so return a blank (invalid) one
	return SectorSnapshot();
}


bool P_CeilingSnapshotDone(SectorSnapshot *snap)
{
	if (!snap || !snap->isValid() || snap->getCeilingMoverType() == SEC_INVALID)
		return true;
			
	if ((snap->getCeilingMoverType() == SEC_CEILING &&
		 snap->getCeilingStatus() == DCeiling::destroy) ||
		(snap->getCeilingMoverType() == SEC_DOOR &&
		 snap->getCeilingStatus() == DDoor::destroy) ||
		(snap->getCeilingMoverType() == SEC_PILLAR &&
		 snap->getCeilingStatus() == DPillar::destroy) ||
		(snap->getCeilingMoverType() == SEC_ELEVATOR &&
		 snap->getCeilingStatus() == DElevator::destroy))
		return true;
		
	return false;
}

bool P_FloorSnapshotDone(SectorSnapshot *snap)
{
	if (!snap || !snap->isValid() || snap->getFloorMoverType() == SEC_INVALID)
		return true;
			
	if ((snap->getFloorMoverType() == SEC_FLOOR &&
		 snap->getFloorStatus() == DFloor::destroy) ||
		(snap->getFloorMoverType() == SEC_PLAT &&
		 snap->getFloorStatus() == DPlat::destroy) ||
		(snap->getFloorMoverType() == SEC_PILLAR &&
		 snap->getFloorStatus() == DPillar::destroy) ||
		(snap->getFloorMoverType() == SEC_ELEVATOR &&
		 snap->getFloorStatus() == DElevator::destroy))
		return true;
		
	return false;
}

VERSION_CONTROL (p_snapshot_cpp, "$Id$")


