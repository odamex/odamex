// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2000-2006 by Sergey Makovkin (CSDoom .62).
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
//	Stores a limited view of actor, player, and sector objects at a particular
//  point in time.  Used with Unlagged, client-side prediction, and positional
//  interpolation
//
//-----------------------------------------------------------------------------

#pragma once

#include "m_fixed.h"
#include "dsectoreffect.h"
#include "actorflags.h"

class AActor;
class player_t;
struct sector_t;
struct line_t;

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
	Snapshot() = default;
	explicit Snapshot(int time):
	    mTime  { time },
	    mValid { time > 0 }
	{}
	virtual ~Snapshot() {};

	[[nodiscard]] bool operator==(const Snapshot &other) const = default;

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

	// TODO / Question:  Do we want to add the echoed client tic to the base snapshot?

private:
	int     mTime   { -1 };
	bool    mValid  { false };

	bool    mAuthoritative  { false };
	bool    mContinuous     { true  };

	bool    mInterpolated   { false };
	bool    mExtrapolated   { false };
};


// ============================================================================
//
// ActorSnapshot Interface
//
// ============================================================================

class ActorSnapshot : public Snapshot
{
public:
	ActorSnapshot() = default;
	explicit ActorSnapshot(int time) : Snapshot { time } {}
	ActorSnapshot(int time, const AActor *mo);
	~ActorSnapshot() override {};

	[[nodiscard]] bool operator==(const ActorSnapshot &other) const = default;

	void merge(const ActorSnapshot& other);

	void toActor(AActor *mo) const;

	[[nodiscard]] fixed_t     getX()            const { return mX; }
	[[nodiscard]] fixed_t     getY()            const { return mY; }
	[[nodiscard]] fixed_t     getZ()            const { return mZ; }
	[[nodiscard]] fixed_t     getMomX()         const { return mMomX; }
	[[nodiscard]] fixed_t     getMomY()         const { return mMomY; }
	[[nodiscard]] fixed_t     getMomZ()         const { return mMomZ; }
	[[nodiscard]] angle_t     getAngle()        const { return mAngle; }
	[[nodiscard]] angle_t     getPitch()        const { return mPitch; }
	[[nodiscard]] bool        getOnGround()     const { return mOnGround; }
	[[nodiscard]] fixed_t     getCeilingZ()     const { return mCeilingZ; }
	[[nodiscard]] fixed_t     getFloorZ()       const { return mFloorZ; }
	[[nodiscard]] int         getReactionTime() const { return mReactionTime; }
	[[nodiscard]] int         getWaterLevel()   const { return mWaterLevel; }
	[[nodiscard]] ActorFlags1 getFlags()        const { return mFlags; }
	[[nodiscard]] ActorFlags2 getFlags2()       const { return mFlags2; }
	[[nodiscard]] ActorFlags3 getFlags3()       const { return mFlags3; }
	[[nodiscard]] int         getFrame()        const { return mFrame; }

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

	void setFlags(ActorFlags1 val)
	{
		mFlags = val;
		mFields |= ACT_FLAGS;
	}

	void setFlags2(ActorFlags2 val)
	{
		mFlags2 = val;
		mFields |= ACT_FLAGS2;
	}

	void setFlags3(ActorFlags3 val)
	{
		mFlags3 = val;
		mFields |= ACT_FLAGS3;
	}

	void setFrame(int val)
	{
		mFrame = val;
		mFields |= ACT_FRAME;
	}

private:
	enum ActorFields
	{
		ACT_POSITIONX       =        0x1,
		ACT_POSITIONY       =        0x2,
		ACT_POSITIONZ       =        0x4,
		ACT_MOMENTUMX       =        0x8,
		ACT_MOMENTUMY       =       0x10,
		ACT_MOMENTUMZ       =       0x20,
		ACT_ANGLE           =       0x40,
		ACT_PITCH           =       0x80,
		ACT_CEILINGZ        =      0x100,
		ACT_FLOORZ          =      0x200,
		ACT_ONGROUND        =      0x400,
		ACT_FLAGS           =      0x800,
		ACT_FLAGS2          =     0x1000,
		ACT_FLAGS3          =     0x2000,
		ACT_REACTIONTIME    =     0x4000,
		ACT_WATERLEVEL      =     0x8000,
		ACT_FRAME           =    0x10000,

		ACT_ALL_FIELDS      = 0xFFFFFFFF
	};

	unsigned int mFields { 0 };

	fixed_t mX      { 0 };
	fixed_t mY      { 0 };
	fixed_t mZ      { 0 };
	fixed_t mMomX   { 0 };
	fixed_t mMomY   { 0 };
	fixed_t mMomZ   { 0 };
	angle_t mAngle  { 0 };
	angle_t mPitch  { 0 };

	bool    mOnGround   { true };
	fixed_t mCeilingZ   { 0    };
	fixed_t mFloorZ     { 0    };

	int mReactionTime   { 0 };
	int mWaterLevel     { 0 };

	ActorFlags1 mFlags  { OUtil::noflag };
	ActorFlags2 mFlags2 { OUtil::noflag };
	ActorFlags3 mFlags3 { OUtil::noflag };
	int         mFrame  { 0 };
};


// ============================================================================
//
// PlayerSnapshot Interface
//
// ============================================================================

class PlayerSnapshot : public Snapshot
{
public:
	PlayerSnapshot() = default;
	explicit PlayerSnapshot(int time) : Snapshot { time }, mActorSnap { time } {}
	PlayerSnapshot(int time, const player_t& player, int echoedClientTic);
	~PlayerSnapshot() override {};

	[[nodiscard]] bool operator==(const PlayerSnapshot &other) const = default;

	void merge(const PlayerSnapshot& other);

	void toPlayer(player_t& player) const;

	[[nodiscard]] fixed_t   getViewHeight()     const { return mViewHeight; }
	[[nodiscard]] fixed_t   getDeltaViewHeight()const { return mDeltaViewHeight; }
	[[nodiscard]] int       getJumpTime()       const { return mJumpTime; }
	[[nodiscard]] int       getPlayerTic()      const { return mPlayerTic; }
	[[nodiscard]] int       getClientTic()      const { return mClientTic; }

	[[nodiscard]] fixed_t     getX()            const { return mActorSnap.getX(); }
	[[nodiscard]] fixed_t     getY()            const { return mActorSnap.getY(); }
	[[nodiscard]] fixed_t     getZ()            const { return mActorSnap.getZ(); }
	[[nodiscard]] fixed_t     getMomX()         const { return mActorSnap.getMomX(); }
	[[nodiscard]] fixed_t     getMomY()         const { return mActorSnap.getMomY(); }
	[[nodiscard]] fixed_t     getMomZ()         const { return mActorSnap.getMomZ(); }
	[[nodiscard]] angle_t     getAngle()        const { return mActorSnap.getAngle(); }
	[[nodiscard]] angle_t     getPitch()        const { return mActorSnap.getPitch(); }
	[[nodiscard]] bool        getOnGround()     const { return mActorSnap.getOnGround(); }
	[[nodiscard]] fixed_t     getCeilingZ()     const { return mActorSnap.getCeilingZ(); }
	[[nodiscard]] fixed_t     getFloorZ()       const { return mActorSnap.getFloorZ(); }
	[[nodiscard]] int         getReactionTime() const { return mActorSnap.getReactionTime(); }
	[[nodiscard]] int         getWaterLevel()   const { return mActorSnap.getWaterLevel(); }
	[[nodiscard]] ActorFlags1 getFlags()        const { return mActorSnap.getFlags(); }
	[[nodiscard]] ActorFlags2 getFlags2()       const { return mActorSnap.getFlags2(); }
	[[nodiscard]] ActorFlags3 getFlags3()       const { return mActorSnap.getFlags3(); }
	[[nodiscard]] int         getFrame()        const { return mActorSnap.getFrame(); }

	void setAuthoritative(bool val) override
	{
		Snapshot::setAuthoritative(val);
		mActorSnap.setAuthoritative(val);
	}

	void setContinuous(bool val) override
	{
		Snapshot::setContinuous(val);
		mActorSnap.setContinuous(val);
	}

	void setInterpolated(bool val) override
	{
		Snapshot::setInterpolated(val);
		mActorSnap.setInterpolated(val);
	}

	void setExtrapolated(bool val) override
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

	void setFlags(ActorFlags1 val)
	{
		mActorSnap.setFlags(val);
		mFields |= PLY_FLAGS;
	}

	void setFlags2(ActorFlags2 val)
	{
		mActorSnap.setFlags2(val);
		mFields |= PLY_FLAGS2;
	}

	void setFlags3(ActorFlags3 val)
	{
		mActorSnap.setFlags3(val);
		mFields |= PLY_FLAGS3;
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

	void setPlayerTic(int val)
	{
		mPlayerTic = val;
		mFields |= PLY_TIC;
	}

	void setClientTic(int val)
	{
		mClientTic = val;
		mFields |= PLY_CLIENT_TIC;
	}

	friend PlayerSnapshot P_LerpPlayerPosition(const PlayerSnapshot&, const PlayerSnapshot&, float);
	friend PlayerSnapshot P_ExtrapolatePlayerPosition(const PlayerSnapshot&, float);

private:
	enum PlayerFields
	{
		PLY_POSITIONX           =        0x1,
		PLY_POSITIONY           =        0x2,
		PLY_POSITIONZ           =        0x4,
		PLY_MOMENTUMX           =        0x8,
		PLY_MOMENTUMY           =       0x10,
		PLY_MOMENTUMZ           =       0x20,
		PLY_ANGLE               =       0x40,
		PLY_PITCH               =       0x80,
		PLY_CEILINGZ            =      0x100,
		PLY_FLOORZ              =      0x200,
		PLY_ONGROUND            =      0x400,
		PLY_FLAGS               =      0x800,
		PLY_FLAGS2              =     0x1000,
		PLY_FLAGS3              =     0x2000,
		PLY_REACTIONTIME        =     0x4000,
		PLY_WATERLEVEL          =     0x8000,
		PLY_FRAME               =    0x10000,
		PLY_VIEWHEIGHT          =    0x20000,
		PLY_DELTAVIEWHEIGHT     =    0x40000,
		PLY_JUMPTIME            =    0x80000,
		PLY_TIC                 =   0x100000,
		PLY_CLIENT_TIC          =   0x200000,

		PLY_ALL_FIELDS          = 0xFFFFFFFF
	};

	unsigned int    mFields     { 0 };  // bitfield of data present in snapshot
	ActorSnapshot   mActorSnap;

	fixed_t         mViewHeight     { 0 };
	fixed_t         mDeltaViewHeight{ 0 };
	int             mJumpTime       { 0 };
	int             mPlayerTic      { 0 };
	int             mClientTic      { 0 };
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

	[[ nodiscard ]] bool ValidSnapshotExists(int time) const;

private:
	int mFindValidSnapshot(int starttime, int endtime) const;
	PlayerSnapshot mInterpolateSnapshots(int from, int to, int time) const;
	PlayerSnapshot mExtrapolateSnapshot(int from, int time) const;

	std::array<PlayerSnapshot, NUM_SNAPSHOTS> mSnaps;
	int                                       mMostRecent;
};


// ============================================================================
//
// SectorSnapshot Class Interface
//
// ============================================================================

class SectorSnapshot : public Snapshot
{
public:
	SectorSnapshot() = default;
	explicit SectorSnapshot(int time) : Snapshot { time } {}
	SectorSnapshot(int time, sector_t *sector);
	~SectorSnapshot() override {};

	void clear();
	void toSector(sector_t *sector) const;

	void setCeilingMoverType(movertype_t v) { mCeilingMoverType = v; }
	void setFloorMoverType(movertype_t val) { mFloorMoverType = val; }
	void setSector(sector_t *val)           { mSector = val; }
	void setCeilingType(int val)            { mCeilingType = val; }
	void setFloorType(int val)              { mFloorType = val; }
	void setCeilingTag(int val)             { mCeilingTag = val; }
	void setFloorTag(int val)               { mFloorTag = val; }
	void setCeilingLine(line_t *val)        { mCeilingLine = val; }
	void setFloorLine(line_t *val)          { mFloorLine = val; }
	void setCeilingHeight(fixed_t val)      { mCeilingHeight = val; }
	void setFloorHeight(fixed_t val)        { mFloorHeight = val; }
	void setCeilingSpeed(fixed_t val)       { mCeilingSpeed = val; }
	void setFloorSpeed(fixed_t val)         { mFloorSpeed = val; }
	void setCeilingDestination(fixed_t val) { mCeilingDestination = val; }
	void setFloorDestination(fixed_t val)   { mFloorDestination = val; }
	void setCeilingDirection(int val)       { mCeilingDirection = val; }
	void setFloorDirection(int val)         { mFloorDirection = val; }
	void setCeilingOldDirection(int val)    { mCeilingOldDirection = val; }
	void setFloorOldDirection(int val)      { mFloorOldDirection = val; }
	void setCeilingTexture(int16_t val)     { mCeilingTexture = val; }
	void setFloorTexture(int16_t val)       { mFloorTexture = val; }
	void setCeilingSpecial(int16_t val)     { mNewCeilingSpecial = val; }
	void setFloorSpecial(int16_t val)       { mNewFloorSpecial = val; }
	void setCeilingLow(fixed_t val)         { mCeilingLow = val; }
	void setCeilingHigh(fixed_t val)        { mCeilingHigh = val; }
	void setFloorLow(fixed_t val)           { mFloorLow = val; }
	void setFloorHigh(fixed_t val)          { mFloorHigh = val; }
	void setCeilingCrush(bool val)          { mCeilingCrush = val; }
	void setFloorCrush(bool val)            { mFloorCrush = val; }
	void setSilent(bool val)                { mSilent = val; }
	void setCeilingWait(int val)            { mCeilingWait = val; }
	void setFloorWait(int val)              { mFloorWait = val; }
	void setCeilingCounter(int val)         { mCeilingCounter = val; }
	void setFloorCounter(int val)           { mFloorCounter = val; }
	void setResetCounter(int val)           { mResetCounter = val; }
	void setCeilingStatus(int val)          { mCeilingStatus = val; }
	void setFloorStatus(int val)            { mFloorStatus = val; }
	void setOldFloorStatus(int val)         { mOldFloorStatus = val; }
	void setCrusherSpeed1(fixed_t val)      { mCrusherSpeed1 = val; }
	void setCrusherSpeed2(fixed_t val)      { mCrusherSpeed2 = val; }
	void setStepTime(int val)               { mStepTime = val; }
	void setPerStepTime(int val)            { mPerStepTime = val; }
	void setPauseTime(int val)              { mPauseTime = val; }
	void setOrgHeight(int val)              { mOrgHeight = val; }
	void setDelay(int val)                  { mDelay = val; }
	void setFloorLip(fixed_t val)           { mFloorLip = val; }
	void setFloorOffset(fixed_t val)        { mFloorOffset = val; }
	void setCeilingChange(int val)          { mCeilingChange = val; }
	void setFloorChange(int val)            { mFloorChange = val; }

	[[nodiscard]] movertype_t getCeilingMoverType() const { return mCeilingMoverType; }
	[[nodiscard]] movertype_t getFloorMoverType() const   { return mFloorMoverType; }
	[[nodiscard]] sector_t* getSector() const             { return mSector; }
	[[nodiscard]] int     getCeilingType() const          { return mCeilingType; }
	[[nodiscard]] int     getFloorType() const            { return mFloorType; }
	[[nodiscard]] int     getCeilingTag() const           { return mCeilingTag; }
	[[nodiscard]] int     getFloorTag() const             { return mFloorTag; }
	[[nodiscard]] line_t* getCeilingLine() const          { return mCeilingLine; }
	[[nodiscard]] const line_t*   getFloorLine() const    { return mFloorLine; }
	[[nodiscard]] fixed_t getCeilingHeight() const        { return mCeilingHeight; }
	[[nodiscard]] fixed_t getFloorHeight() const          { return mFloorHeight; }
	[[nodiscard]] fixed_t getCeilingSpeed() const         { return mCeilingSpeed; }
	[[nodiscard]] fixed_t getFloorSpeed() const           { return mFloorSpeed; }
	[[nodiscard]] fixed_t getCeilingDestination() const   { return mCeilingDestination; }
	[[nodiscard]] fixed_t getFloorDestination() const     { return mFloorDestination; }
	[[nodiscard]] int     getCeilingDirection() const     { return mCeilingDirection; }
	[[nodiscard]] int     getFloorDirection() const       { return mFloorDirection; }
	[[nodiscard]] int     getCeilingOldDirection() const  { return mCeilingOldDirection; }
	[[nodiscard]] int     getFloorOldDirection() const    { return mFloorOldDirection; }
	[[nodiscard]] int16_t getCeilingTexture() const       { return mCeilingTexture; }
	[[nodiscard]] int16_t getFloorTexture() const         { return mFloorTexture; }
	[[nodiscard]] int16_t getCeilingSpecial() const       { return mNewCeilingSpecial; }
	[[nodiscard]] int16_t getFloorSpecial() const         { return mNewFloorSpecial; }
	[[nodiscard]] fixed_t getCeilingLow() const           { return mCeilingLow; }
	[[nodiscard]] fixed_t getCeilingHigh() const          { return mCeilingHigh; }
	[[nodiscard]] fixed_t getFloorLow() const             { return mFloorLow; }
	[[nodiscard]] fixed_t getFloorHigh() const            { return mFloorHigh; }
	[[nodiscard]] bool    getCeilingCrush() const         { return mCeilingCrush; }
	[[nodiscard]] bool    getFloorCrush() const           { return mFloorCrush; }
	[[nodiscard]] bool    getSilent() const               { return mSilent; }
	[[nodiscard]] int     getCeilingWait() const          { return mCeilingWait; }
	[[nodiscard]] int     getFloorWait() const            { return mFloorWait; }
	[[nodiscard]] int     getCeilingCounter() const       { return mCeilingCounter; }
	[[nodiscard]] int     getFloorCounter() const         { return mFloorCounter; }
	[[nodiscard]] int     getResetCounter() const         { return mResetCounter; }
	[[nodiscard]] int     getCeilingStatus() const        { return mCeilingStatus; }
	[[nodiscard]] int     getFloorStatus() const          { return mFloorStatus; }
	[[nodiscard]] int     getOldFloorStatus() const       { return mOldFloorStatus; }
	[[nodiscard]] fixed_t getCrusherSpeed1() const        { return mCrusherSpeed1; }
	[[nodiscard]] fixed_t getCrusherSpeed2() const        { return mCrusherSpeed1; }
	[[nodiscard]] int     getStepTime() const             { return mStepTime; }
	[[nodiscard]] int     getPerStepTime() const          { return mPerStepTime; }
	[[nodiscard]] int     getPauseTime() const            { return mPauseTime; }
	[[nodiscard]] int     getOrgHeight() const            { return mOrgHeight; }
	[[nodiscard]] int     getDelay() const                { return mDelay; }
	[[nodiscard]] fixed_t getFloorLip() const             { return mFloorLip; }
	[[nodiscard]] fixed_t getFloorOffset() const          { return mFloorOffset; }
	[[nodiscard]] int     getCeilingChange() const        { return mCeilingChange; }
	[[nodiscard]] int     getFloorChange() const          { return mFloorChange; }

private:
	movertype_t     mCeilingMoverType   { SEC_INVALID };
	movertype_t     mFloorMoverType     { SEC_INVALID };

	sector_t*       mSector { nullptr };

	int             mCeilingType    { 0 };
	int             mFloorType      { 0 };
	int             mCeilingTag     { 0 };
	int             mFloorTag       { 0 };
	line_t*         mCeilingLine    { nullptr };
	const line_t*   mFloorLine      { nullptr };

	fixed_t         mCeilingHeight  { 0 };
	fixed_t         mFloorHeight    { 0 };

	fixed_t         mCeilingSpeed   { 0 };
	fixed_t         mFloorSpeed     { 0 };

	fixed_t         mCeilingDestination { 0 };
	fixed_t         mFloorDestination   { 0 };

	int             mCeilingDirection   { 0 };
	int             mFloorDirection     { 0 };

	int             mCeilingOldDirection{ 0 };
	int             mFloorOldDirection  { 0 };

	int16_t         mCeilingTexture { 0 };
	int16_t         mFloorTexture   { 0 };

	int16_t         mNewCeilingSpecial  { 0 };
	int16_t         mNewFloorSpecial    { 0 };

	fixed_t         mCeilingLow { 0 };
	fixed_t         mCeilingHigh{ 0 };

	fixed_t         mFloorLow   { 0 };
	fixed_t         mFloorHigh  { 0 };

	bool            mCeilingCrush   { false };
	bool            mFloorCrush     { false };
	bool            mSilent         { false };
	int             mCeilingWait    { 0 };
	int             mFloorWait      { 0 };
	int             mCeilingCounter { 0 };
	int             mFloorCounter   { 0 };
	int             mResetCounter   { 0 };
	int             mCeilingStatus  { 0 };
	int             mFloorStatus    { 0 };
	int             mOldFloorStatus { 0 };

	fixed_t         mCrusherSpeed1  { 0 };
	fixed_t         mCrusherSpeed2  { 0 };

	int             mStepTime       { 0 };
	int             mPerStepTime    { 0 };
	int             mPauseTime      { 0 };
	int             mOrgHeight      { 0 };
	int             mDelay          { 0 };

	fixed_t         mFloorLip       { 0 };
	fixed_t         mFloorOffset    { 0 };

	int             mCeilingChange  { 0 };
	int             mFloorChange    { 0 };
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

	bool empty() const;
	void clearSnapshots();

	int getMostRecentTime() const { return mMostRecent; }

	void addSnapshot(const SectorSnapshot &snap);
	SectorSnapshot getSnapshot(int time) const;

private:
	bool mValidSnapshot(int time) const;

	std::array<SectorSnapshot, NUM_SNAPSHOTS> mSnaps;
	int                                       mMostRecent;
};


// ============================================================================
//
// Helper functions
//
// ============================================================================

void P_SetPlayerSnapshotNoPosition(player_t& player, const PlayerSnapshot &snap);

ActorSnapshot P_LerpActorPosition(const ActorSnapshot &from, const ActorSnapshot &to, float amount);
ActorSnapshot P_ExtrapolateActorPosition(const ActorSnapshot &from, float amount);

PlayerSnapshot P_LerpPlayerPosition(const PlayerSnapshot &from, const PlayerSnapshot &to, float amount);
PlayerSnapshot P_ExtrapolatePlayerPosition(const PlayerSnapshot &from, float amount);

bool P_CeilingSnapshotDone(SectorSnapshot *snap);
bool P_FloorSnapshotDone(SectorSnapshot *snap);
