// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id: p_snapshot.cpp 2785 2012-02-18 23:22:07Z dr_sean $
//
// Copyright (C) 2000-2006 by Sergey Makovkin (CSDoom .62).
// Copyright (C) 2006-2010 by The Odamex Team.
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
#include "vectors.h"

#include "p_snapshot.h"

static const int MAX_EXTRAPOLATION = 4;

static const fixed_t POS_LERP_THRESHOLD = 2 * FRACUNIT;
static const fixed_t SECTOR_LERP_THRESHOLD = 2 * FRACUNIT;

// ============================================================================
//
// Snapshot implementation
//
// ============================================================================

Snapshot::Snapshot(const Snapshot &other) :
		mTime(other.mTime),
		mValid(other.mValid), mAuthoritative(other.mAuthoritative),
		mContinuous(other.mContinuous), mInterpolated(other.mInterpolated),
		mExtrapolated(other.mExtrapolated)
{
}

void Snapshot::operator=(const Snapshot &other)
{
	mTime = other.mTime;
	mValid = other.mValid;
	mAuthoritative = other.mAuthoritative;
	mContinuous = other.mContinuous;
	mInterpolated = other.mInterpolated;
	mExtrapolated = other.mExtrapolated;
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

ActorSnapshot::ActorSnapshot(const ActorSnapshot &other) :
		Snapshot(other), mFields(other.mFields),
		mX(other.mX), mY(other.mY), mZ(other.mZ),
		mMomX(other.mMomX), mMomY(other.mMomY), mMomZ(other.mMomZ),
		mAngle(other.mAngle), mPitch(other.mPitch), mOnGround(other.mOnGround),
		mCeilingZ(other.mCeilingZ), mFloorZ(other.mFloorZ),
		mReactionTime(other.mReactionTime), mWaterLevel(other.mWaterLevel),
		mFlags(other.mFlags), mFlags2(other.mFlags2), mFrame(other.mFrame)
{
}

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

void ActorSnapshot::operator=(const ActorSnapshot &other)
{
	Snapshot::operator=(other);
	mFields = other.mFields;
	mX = other.mX;
	mY = other.mY;
	mZ = other.mZ;
	mMomX = other.mMomX;
	mMomY = other.mMomY;
	mMomZ = other.mMomZ;
	mAngle = other.mAngle;
	mPitch = other.mPitch;
	mOnGround = other.mOnGround;
	mCeilingZ = other.mCeilingZ;
	mFloorZ = other.mFloorZ;
	mReactionTime = other.mReactionTime;
	mWaterLevel = other.mWaterLevel;
	mFlags = other.mFlags;
	mFlags2 = other.mFlags2;
	mFrame = other.mFrame;
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
				
			mo->LinkToWorld();
		}	
	}
		
	if (mFields & (ACT_MOMENTUMX | ACT_MOMENTUMY | ACT_MOMENTUMZ))
	{
		mo->momx = mMomX;
		mo->momy = mMomY;
		mo->momz = mMomZ;
	}
	
	if (mFields & ACT_ANGLE)
		mo->angle = mAngle;
	if (mFields & ACT_PITCH)
		mo->angle = mPitch;
	if (mFields & ACT_ONGROUND)
		mo->onground = mOnGround;
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

PlayerSnapshot::PlayerSnapshot(const PlayerSnapshot &other) :
		Snapshot(other), mFields(other.mFields),
		mActorSnap(other.mActorSnap),
		mViewHeight(other.mViewHeight), mDeltaViewHeight(other.mDeltaViewHeight),
		mJumpTime(other.mJumpTime)
{
}

PlayerSnapshot::PlayerSnapshot(int time) :
		Snapshot(time), mFields(0),
		mActorSnap(time),
		mViewHeight(VIEWHEIGHT), mDeltaViewHeight(0), mJumpTime(0)
{
}

PlayerSnapshot::PlayerSnapshot(int time, player_t *player) :
		Snapshot(time), mFields(0xFFFFFFFF),
		mActorSnap(time, player->mo),
		mViewHeight(player->viewheight), mDeltaViewHeight(player->deltaviewheight),
		mJumpTime(player->jumpTics)
{
}

void PlayerSnapshot::operator=(const PlayerSnapshot &other)
{
	Snapshot::operator=(other);
	mFields = other.mFields;
	mActorSnap = other.mActorSnap;
	mViewHeight = other.mViewHeight;
	mDeltaViewHeight = other.mDeltaViewHeight;
	mJumpTime = other.mJumpTime;
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

PlayerSnapshotManager::PlayerSnapshotManager(const PlayerSnapshotManager &other) :
	mMostRecent(other.mMostRecent)
{
	for (int i = 0; i < NUM_SNAPSHOTS; i++)
		mSnaps[i] = other.mSnaps[i];
}

PlayerSnapshotManager &PlayerSnapshotManager::operator=(const PlayerSnapshotManager &other)
{
	for (int i = 0; i < NUM_SNAPSHOTS; i++)
		mSnaps[i] = other.mSnaps[i];
		
	mMostRecent = other.mMostRecent;
	
	return *this;	
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

	mSnaps[time % NUM_SNAPSHOTS] = snap;

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
// Helper functions
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
		
	player->mo->angle = snap.getAngle();
	player->mo->pitch = snap.getPitch();
}

VERSION_CONTROL (p_snapshot_cpp, "$Id: p_snapshot.cpp 2785 2012-02-18 23:22:07Z dr_sean $")


