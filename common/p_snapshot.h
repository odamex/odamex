// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id: p_snapshot.h 2785 2012-02-18 23:22:07Z dr_sean $
//
// Copyright (C) 2000-2006 by Sergey Makovkin (CSDoom .62).
// Copyright (C) 2006-2012 by The Odamex Team.
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

#ifndef __P_SNAPSHOT_H__
#define __P_SNAPSHOT_H__

#include "m_fixed.h"
#include "doomdef.h"
#include "dsectoreffect.h"

class AActor;
class player_s;
typedef player_s player_t;
struct sector_s;
typedef sector_s sector_t;
struct line_s;
typedef line_s line_t;

class PlayerSnapshotManager;

extern int gametic;

#define NUM_SNAPSHOTS 32

//#define _WORLD_INDEX_DEBUG_
//#define _SNAPSHOT_DEBUG_

// ============================================================================
//
// Snapshot Base Class Interface
//
// ============================================================================

class Snapshot
{
public:
	Snapshot(int time = -1);	
	virtual ~Snapshot() {};

	bool operator==(const Snapshot &other) const;
	
	bool isValid() const { return mValid; }
	bool isAuthoritative() const { return mAuthoritative; }
	bool isContinuous() const { return mContinuous; }
	bool isInterpolated() const { return mInterpolated; }
	bool isExtrapolated() const { return mExtrapolated; }
	
	virtual void setAuthoritative(bool val) { mAuthoritative = val; }
	virtual void setContinuous(bool val) { mContinuous = val; }
	virtual void setInterpolated(bool val) { mInterpolated = val; }
	virtual void setExtrapolated(bool val) { mExtrapolated = val; }
	
	int getTime() const { return mTime; }
	void setTime(int time) { mTime = time; }

private:
	int			mTime;
	bool		mValid;	

	bool		mAuthoritative;
	bool		mContinuous;
	
	bool		mInterpolated;
	bool		mExtrapolated;
};


// ============================================================================
//
// ActorSnapshot Interface
//
// ============================================================================

class ActorSnapshot : public Snapshot
{
public:
	ActorSnapshot(int time = -1);	
	ActorSnapshot(int time, const AActor *mo);
	virtual ~ActorSnapshot() {};

	bool operator==(const ActorSnapshot &other) const;
	
	void toActor(AActor *mo) const;
	
	fixed_t getX() const			{ return mX; }
	fixed_t getY() const			{ return mY; }
	fixed_t getZ() const			{ return mZ; }
	fixed_t getMomX() const			{ return mMomX; }
	fixed_t getMomY() const			{ return mMomY; }
	fixed_t getMomZ() const			{ return mMomZ; }
	angle_t getAngle() const		{ return mAngle; }	
	angle_t getPitch() const		{ return mPitch; }	
	bool	getOnGround() const		{ return mOnGround; }
	fixed_t getCeilingZ() const		{ return mCeilingZ; }
	fixed_t getFloorZ() const		{ return mFloorZ; }
	int		getReactionTime() const	{ return mReactionTime; }
	int		getWaterLevel() const	{ return mWaterLevel; }
	int		getFlags() const		{ return mFlags; }
	int		getFlags2() const		{ return mFlags2; }
	int		getFrame() const		{ return mFrame; }

	void setX(fixed_t val)
	{
		mX = val;
		mFields |= ACT_POSITIONX;
	}

	void setY(fixed_t val)
	{
		mY = val;
		mFields |= ACT_POSITIONY;
	}

	void setZ(fixed_t val)
	{
		mZ = val;
		mFields |= ACT_POSITIONZ;
	}
	
	void setMomX(fixed_t val)
	{
		mMomX = val;
		mFields |= ACT_MOMENTUMX;
	}

	void setMomY(fixed_t val)
	{
		mMomY = val;
		mFields |= ACT_MOMENTUMY;
	}
	
	void setMomZ(fixed_t val)
	{
		mMomZ = val;
		mFields |= ACT_MOMENTUMZ;
	}
			
	void setAngle(angle_t val)
	{
		mAngle = val;
		mFields |= ACT_ANGLE;
	}
	
	void setPitch(angle_t val)
	{
		mPitch = val;
		mFields |= ACT_PITCH;
	}
	
	void setOnGround(bool val)
	{
		mOnGround = val;
		mFields |= ACT_ONGROUND;
	}
	
	void setCeilingZ(fixed_t val)
	{
		mCeilingZ = val;
		mFields |= ACT_CEILINGZ;
	}

	void setFloorZ(fixed_t val)
	{
		mFloorZ = val;
		mFields |= ACT_FLOORZ;
	}

	void setReactionTime(int val)
	{
		mReactionTime = val;
		mFields |= ACT_REACTIONTIME;
	}
	
	void setWaterLevel(int val)
	{
		mWaterLevel = val;
		mFields |= ACT_WATERLEVEL;
	}
	
	void setFlags(int val)
	{
		mFlags = val;
		mFields |= ACT_FLAGS;
	}
	
	void setFlags2(int val)
	{
		mFlags2 = val;
		mFields |= ACT_FLAGS2;
	}	

	void setFrame(int val)
	{
		mFrame = val;
		mFields |= ACT_FRAME;
	}

private:
	enum ActorFields {
		ACT_POSITIONX			= 0x00000001,
		ACT_POSITIONY			= 0x00000002,
		ACT_POSITIONZ			= 0x00000004,	
		ACT_MOMENTUMX			= 0x00000008,
		ACT_MOMENTUMY			= 0x00000010,
		ACT_MOMENTUMZ			= 0x00000020,
		ACT_ANGLE				= 0x00000040,
		ACT_PITCH				= 0x00000080,
		ACT_CEILINGZ			= 0x00000100,
		ACT_FLOORZ				= 0x00000200,
		ACT_ONGROUND			= 0x00000400,
		ACT_FLAGS				= 0x00000800,
		ACT_FLAGS2				= 0x00001000,
		ACT_REACTIONTIME		= 0x00002000,
		ACT_WATERLEVEL			= 0x00004000,
		ACT_FRAME				= 0x00008000
	};
	
	unsigned int	mFields;
	
	fixed_t			mX;
	fixed_t			mY;
	fixed_t			mZ;
	fixed_t			mMomX;
	fixed_t			mMomY;
	fixed_t			mMomZ;
	angle_t			mAngle;
	angle_t			mPitch;

	bool			mOnGround;
	fixed_t			mCeilingZ;	
	fixed_t			mFloorZ;
	
	int				mReactionTime;
	int				mWaterLevel;
	
	int				mFlags;
	int				mFlags2;
	int				mFrame;
};


// ============================================================================
//
// PlayerSnapshot Interface
//
// ============================================================================

class PlayerSnapshot : public Snapshot
{
public:
	PlayerSnapshot(int time = -1);
	PlayerSnapshot(int time, player_t *player);
	virtual ~PlayerSnapshot() {};

	bool operator==(const PlayerSnapshot &other) const;
	
	void toPlayer(player_t *player) const;

	fixed_t getViewHeight() const		{ return mViewHeight; }
	fixed_t getDeltaViewHeight() const	{ return mDeltaViewHeight; }
	int		getJumpTime() const			{ return mJumpTime; }

	fixed_t getX() const				{ return mActorSnap.getX(); }
	fixed_t getY() const				{ return mActorSnap.getY(); }
	fixed_t getZ() const				{ return mActorSnap.getZ(); }
	fixed_t getMomX() const				{ return mActorSnap.getMomX(); }
	fixed_t getMomY() const				{ return mActorSnap.getMomY(); }
	fixed_t getMomZ() const				{ return mActorSnap.getMomZ(); }
	angle_t getAngle() const			{ return mActorSnap.getAngle(); }	
	angle_t getPitch() const			{ return mActorSnap.getPitch(); }	
	bool	getOnGround() const			{ return mActorSnap.getOnGround(); }
	fixed_t getCeilingZ() const			{ return mActorSnap.getCeilingZ(); }
	fixed_t getFloorZ() const			{ return mActorSnap.getFloorZ(); }
	int		getReactionTime() const		{ return mActorSnap.getReactionTime(); }
	int		getWaterLevel() const		{ return mActorSnap.getWaterLevel(); }
	int		getFlags() const			{ return mActorSnap.getFlags(); }
	int		getFlags2() const			{ return mActorSnap.getFlags2(); }
	int		getFrame() const			{ return mActorSnap.getFrame(); }

	virtual void setAuthoritative(bool val)
	{
		Snapshot::setAuthoritative(val);
		mActorSnap.setAuthoritative(val);
	}
	
	virtual void setContinuous(bool val)
	{
		Snapshot::setContinuous(val);
		mActorSnap.setContinuous(val);
	}
	
	virtual void setInterpolated(bool val)
	{
		Snapshot::setInterpolated(val);
		mActorSnap.setInterpolated(val);
	}
	
	virtual void setExtrapolated(bool val)
	{
		Snapshot::setExtrapolated(val);
		mActorSnap.setExtrapolated(val);
	}
	
	void setX(fixed_t val)
	{
		mActorSnap.setX(val);
		mFields |= PLY_POSITIONX;
	}
	
	void setY(fixed_t val)
	{
		mActorSnap.setY(val);
		mFields |= PLY_POSITIONY;		
	}	
	
	void setZ(fixed_t val)
	{
		mActorSnap.setZ(val);
		mFields |= PLY_POSITIONZ;
	}
	
	void setMomX(fixed_t val)
	{
		mActorSnap.setMomX(val);
		mFields |= PLY_MOMENTUMX;
	}

	void setMomY(fixed_t val)
	{
		mActorSnap.setMomY(val);
		mFields |= PLY_MOMENTUMY;
	}
	
	void setMomZ(fixed_t val)
	{
		mActorSnap.setMomZ(val);
		mFields |= PLY_MOMENTUMZ;
	}
			
	void setAngle(angle_t val)
	{
		mActorSnap.setAngle(val);
		mFields |= PLY_ANGLE;
	}

	void setPitch(angle_t val)
	{
		mActorSnap.setPitch(val);
		mFields |= PLY_PITCH;
	}
	
	void setCeilingZ(fixed_t val)
	{
		mActorSnap.setCeilingZ(val);
		mFields |= PLY_CEILINGZ;
	}

	void setFloorZ(fixed_t val)
	{
		mActorSnap.setFloorZ(val);
		mFields |= PLY_FLOORZ;
	}

	void setOnGround(fixed_t val)
	{
		mActorSnap.setOnGround(val != 0);
		mFields |= PLY_ONGROUND;
	}
	
	void setReactionTime(int val)
	{
		mActorSnap.setReactionTime(val);
		mFields |= PLY_REACTIONTIME;
	}

	void setFlags(int val)
	{
		mActorSnap.setFlags(val);
		mFields |= PLY_FLAGS;
	}	

	void setFlags2(int val)
	{
		mActorSnap.setFlags2(val);
		mFields |= PLY_FLAGS2;
	}	

	void setFrame(int val)
	{
		mActorSnap.setFrame(val);
		mFields |= PLY_FRAME;
	}
			
	void setWaterLevel(int val)
	{
		mActorSnap.setWaterLevel(val);
		mFields |= PLY_WATERLEVEL;
	}
	
	void setViewHeight(fixed_t val)
	{
		mViewHeight = val;
		mFields |= PLY_VIEWHEIGHT;
	}
	
	void setDeltaViewHeight(fixed_t val)
	{
		mDeltaViewHeight = val;
		mFields |= PLY_DELTAVIEWHEIGHT;
	}
	
	void setJumpTime(int val)
	{
		mJumpTime = val;
		mFields |= PLY_JUMPTIME;
	}

	friend PlayerSnapshot P_LerpPlayerPosition(const PlayerSnapshot&, const PlayerSnapshot&, float);
	friend PlayerSnapshot P_ExtrapolatePlayerPosition(const PlayerSnapshot&, float);	
	
private:
	enum PlayerFields {
		PLY_POSITIONX			= 0x00000001,
		PLY_POSITIONY			= 0x00000002,
		PLY_POSITIONZ			= 0x00000004,	
		PLY_MOMENTUMX			= 0x00000008,
		PLY_MOMENTUMY			= 0x00000010,
		PLY_MOMENTUMZ			= 0x00000020,
		PLY_ANGLE				= 0x00000040,
		PLY_PITCH				= 0x00000080,
		PLY_CEILINGZ			= 0x00000100,
		PLY_FLOORZ				= 0x00000200,
		PLY_ONGROUND			= 0x00000400,
		PLY_FLAGS				= 0x00000800,
		PLY_FLAGS2				= 0x00001000,
		PLY_REACTIONTIME		= 0x00002000,
		PLY_WATERLEVEL			= 0x00004000,
		PLY_FRAME				= 0x00008000,
		PLY_VIEWHEIGHT			= 0x00010000,
		PLY_DELTAVIEWHEIGHT		= 0x00020000,
		PLY_JUMPTIME			= 0x00040000
	};
	
	unsigned int	mFields;		// bitfield of data present in snapshot
	ActorSnapshot	mActorSnap;

	fixed_t			mViewHeight;
	fixed_t			mDeltaViewHeight;
	int				mJumpTime;
};


// ============================================================================
//
// PlayerSnapshotManager Interface
//
// ============================================================================

class PlayerSnapshotManager
{
public:
	PlayerSnapshotManager();	
	
	void clearSnapshots();
	
	int getMostRecentTime() const { return mMostRecent; }
	
	void addSnapshot(const PlayerSnapshot &snap);
	PlayerSnapshot getSnapshot(int time) const;

private:
	bool mValidSnapshot(int time) const;
	int mFindValidSnapshot(int starttime, int endtime) const;
	PlayerSnapshot mInterpolateSnapshots(int from, int to, int time) const;
	PlayerSnapshot mExtrapolateSnapshot(int from, int time) const;
	
	PlayerSnapshot	mSnaps[NUM_SNAPSHOTS];
	int				mMostRecent;
};


// ============================================================================
//
// SectorSnapshot Class Interface
//
// ============================================================================

class SectorSnapshot : public Snapshot
{
public:
	SectorSnapshot(int time = -1);
	SectorSnapshot(int time, sector_t *sector);
	virtual ~SectorSnapshot() {};

	void clear();
	void toSector(sector_t *sector) const;

	void setCeilingMoverType(movertype_t v)	{ mCeilingMoverType = v; }
	void setFloorMoverType(movertype_t val)	{ mFloorMoverType = val; }
	void setSector(sector_t *val)			{ mSector = val; }
	void setCeilingType(int val)			{ mCeilingType = val; }
	void setFloorType(int val)				{ mFloorType = val; }	
	void setCeilingTag(int val)				{ mCeilingTag = val; }
	void setFloorTag(int val)				{ mFloorTag = val; }	
	void setCeilingLine(line_t *val)		{ mCeilingLine = val; }
	void setFloorLine(line_t *val)			{ mFloorLine = val; }	
	void setCeilingHeight(fixed_t val)		{ mCeilingHeight = val; }
	void setFloorHeight(fixed_t val)		{ mFloorHeight = val; }
	void setCeilingSpeed(fixed_t val)		{ mCeilingSpeed = val; }	
	void setFloorSpeed(fixed_t val)			{ mFloorSpeed = val; }
	void setCeilingDestination(fixed_t val)	{ mCeilingDestination = val; }
	void setFloorDestination(fixed_t val)	{ mFloorDestination = val; }
	void setCeilingDirection(int val)		{ mCeilingDirection = val; }
	void setFloorDirection(int val)			{ mFloorDirection = val; }
	void setCeilingOldDirection(int val)	{ mCeilingOldDirection = val; }
	void setFloorOldDirection(int val)		{ mFloorOldDirection = val; }
	void setCeilingTexture(short val)		{ mCeilingTexture = val; }
	void setFloorTexture(short val)			{ mFloorTexture = val; }
	void setCeilingSpecial(short val)		{ mNewCeilingSpecial = val; }
	void setFloorSpecial(short val)			{ mNewFloorSpecial = val; }
	void setCeilingLow(fixed_t val)			{ mCeilingLow = val; }
	void setCeilingHigh(fixed_t val)		{ mCeilingHigh = val; }
	void setFloorLow(fixed_t val)			{ mFloorLow = val; }
	void setFloorHigh(fixed_t val)			{ mFloorHigh = val; }
	void setCeilingCrush(bool val)			{ mCeilingCrush = val; }
	void setFloorCrush(bool val)			{ mFloorCrush = val; }	
	void setSilent(bool val)				{ mSilent = val; }
	void setCeilingWait(int val)			{ mCeilingWait = val; }
	void setFloorWait(int val)				{ mFloorWait = val; }	
	void setCeilingCounter(int val)			{ mCeilingCounter = val; }
	void setFloorCounter(int val)			{ mFloorCounter = val; }	
	void setResetCounter(int val)			{ mResetCounter = val; }
	void setCeilingStatus(int val)			{ mCeilingStatus = val; }
	void setFloorStatus(int val)			{ mFloorStatus = val; }	
	void setOldFloorStatus(int val)			{ mOldFloorStatus = val; }
	void setCrusherSpeed1(fixed_t val)		{ mCrusherSpeed1 = val; }
	void setCrusherSpeed2(fixed_t val)		{ mCrusherSpeed2 = val; }
	void setStepTime(int val)				{ mStepTime = val; }
	void setPerStepTime(int val)			{ mPerStepTime = val; }
	void setPauseTime(int val)				{ mPauseTime = val; }
	void setOrgHeight(int val)				{ mOrgHeight = val; }
	void setDelay(int val)					{ mDelay = val; }
	void setFloorLip(fixed_t val)			{ mFloorLip = val; }
	void setFloorOffset(fixed_t val)		{ mFloorOffset = val; }
	void setCeilingChange(int val)			{ mCeilingChange = val; }
	void setFloorChange(int val)			{ mFloorChange = val; }
		
	movertype_t	getCeilingMoverType() const	{ return mCeilingMoverType; }
	movertype_t	getFloorMoverType() const	{ return mFloorMoverType; }	
	sector_t* getSector() const				{ return mSector; }
	int		getCeilingType() const			{ return mCeilingType; }
	int		getFloorType() const			{ return mFloorType; }	
	int		getCeilingTag() const			{ return mCeilingTag; }
	int		getFloorTag() const				{ return mFloorTag; }	
	line_t*	getCeilingLine() const			{ return mCeilingLine; }
	line_t*	getFloorLine() const			{ return mFloorLine; }			
	fixed_t	getCeilingHeight() const		{ return mCeilingHeight; }
	fixed_t	getFloorHeight() const			{ return mFloorHeight; }
	fixed_t getCeilingSpeed() const			{ return mCeilingSpeed; }
	fixed_t getFloorSpeed() const			{ return mFloorSpeed; }	
	fixed_t getCeilingDestination() const	{ return mCeilingDestination; }
	fixed_t getFloorDestination() const		{ return mFloorDestination; }
	int		getCeilingDirection() const		{ return mCeilingDirection; }
	int		getFloorDirection() const		{ return mFloorDirection; }
	int		getCeilingOldDirection() const	{ return mCeilingOldDirection; }
	int		getFloorOldDirection() const	{ return mFloorOldDirection; }
	short	getCeilingTexture() const		{ return mCeilingTexture; }
	short	getFloorTexture() const			{ return mFloorTexture; }
	short	getCeilingSpecial() const		{ return mNewCeilingSpecial; }
	short	getFloorSpecial() const			{ return mNewFloorSpecial; }
	fixed_t	getCeilingLow() const			{ return mCeilingLow; }
	fixed_t	getCeilingHigh() const			{ return mCeilingHigh; }	
	fixed_t	getFloorLow() const				{ return mFloorLow; }
	fixed_t	getFloorHigh() const			{ return mFloorHigh; }
	bool	getCeilingCrush() const			{ return mCeilingCrush; }
	bool	getFloorCrush() const			{ return mFloorCrush; }	
	bool	getSilent() const				{ return mSilent; }
	int		getCeilingWait() const			{ return mCeilingWait; }
	int		getFloorWait() const			{ return mFloorWait; }	
	int		getCeilingCounter() const		{ return mCeilingCounter; }
	int		getFloorCounter() const			{ return mFloorCounter; }	
	int		getResetCounter() const			{ return mResetCounter; }
	int		getCeilingStatus() const		{ return mCeilingStatus; }
	int		getFloorStatus() const			{ return mFloorStatus; }	
	int		getOldFloorStatus() const		{ return mOldFloorStatus; }
	fixed_t	getCrusherSpeed1() const		{ return mCrusherSpeed1; }
	fixed_t	getCrusherSpeed2() const		{ return mCrusherSpeed1; }
	int		getStepTime() const				{ return mStepTime; }
	int		getPerStepTime() const			{ return mPerStepTime; }
	int		getPauseTime() const			{ return mPauseTime; }
	int		getOrgHeight() const			{ return mOrgHeight; }
	int		getDelay() const				{ return mDelay; }
	fixed_t	getFloorLip() const				{ return mFloorLip; }
	fixed_t	getFloorOffset() const			{ return mFloorOffset; }
	int		getCeilingChange() const		{ return mCeilingChange; }
	int		getFloorChange() const			{ return mFloorChange; }

private:
	movertype_t		mCeilingMoverType;
	movertype_t		mFloorMoverType;

	sector_t*		mSector;
	
	int				mCeilingType;
	int				mFloorType;
	int				mCeilingTag;
	int				mFloorTag;
	line_t*			mCeilingLine;
	line_t*			mFloorLine;
		
	fixed_t			mCeilingHeight;
	fixed_t			mFloorHeight;
		
	fixed_t			mCeilingSpeed;
	fixed_t			mFloorSpeed;
	
	fixed_t			mCeilingDestination;
	fixed_t			mFloorDestination;
	
	int				mCeilingDirection;
	int				mFloorDirection;
	
	int				mCeilingOldDirection;
	int				mFloorOldDirection;
	
	short			mCeilingTexture;
	short			mFloorTexture;
	
	short			mNewCeilingSpecial;
	short			mNewFloorSpecial;

	fixed_t			mCeilingLow;
	fixed_t			mCeilingHigh;
	
	fixed_t			mFloorLow;
	fixed_t			mFloorHigh;
	
	bool			mCeilingCrush;
	bool			mFloorCrush;
	bool			mSilent;
	int				mCeilingWait;
	int				mFloorWait;	
	int				mCeilingCounter;
	int				mFloorCounter;
	int				mResetCounter;
	int				mCeilingStatus;
	int				mFloorStatus;
	int				mOldFloorStatus;
	
	fixed_t			mCrusherSpeed1;
	fixed_t			mCrusherSpeed2;
	
	int				mStepTime;
	int				mPerStepTime;
	int				mPauseTime;
	int				mOrgHeight;
	int				mDelay;
	
	fixed_t			mFloorLip;
	fixed_t			mFloorOffset;
	
	int				mCeilingChange;
	int				mFloorChange;
};


// ============================================================================
//
// SectorSnapshotManager Interface
//
// ============================================================================

class SectorSnapshotManager
{
public:
	SectorSnapshotManager();

	bool empty();
	void clearSnapshots();
	
	int getMostRecentTime() const { return mMostRecent; }
	
	void addSnapshot(const SectorSnapshot &snap);
	SectorSnapshot getSnapshot(int time) const;
	
private:
	bool mValidSnapshot(int time) const;
	
	SectorSnapshot	mSnaps[NUM_SNAPSHOTS];
	int				mMostRecent;
};


// ============================================================================
//
// Helper functions
//
// ============================================================================

void P_SetPlayerSnapshotNoPosition(player_t *player, const PlayerSnapshot &snap);

ActorSnapshot P_LerpActorPosition(const ActorSnapshot &from, const ActorSnapshot &to, float amount);
ActorSnapshot P_ExtrapolateActorPosition(const ActorSnapshot &from, float amount);

PlayerSnapshot P_LerpPlayerPosition(const PlayerSnapshot &from, const PlayerSnapshot &to, float amount);
PlayerSnapshot P_ExtrapolatePlayerPosition(const PlayerSnapshot &from, float amount);

bool P_CeilingSnapshotDone(SectorSnapshot *snap);
bool P_FloorSnapshotDone(SectorSnapshot *snap);

#endif	// __P_SNAPSHOT_H__

