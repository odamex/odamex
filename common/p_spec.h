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
// DESCRIPTION:  none
//	Implements special effects:
//	Texture animation, height or lighting changes
//	 according to adjacent sectors, respective
//	 utility functions, etc.
//
//-----------------------------------------------------------------------------

#pragma once

#include <list>
#include "dsectoreffect.h"

typedef struct movingsector_s
{
	movingsector_s() :
		sector(NULL), moving_ceiling(false), moving_floor(false)
	{}
	
	sector_t	*sector;
	bool		moving_ceiling;
	bool		moving_floor;
} movingsector_t;

enum motionspeed_e
{
	SpeedSlow,
	SpeedNormal,
	SpeedFast,
	SpeedTurbo,
};

enum doorkind_e
{
	OdCDoor,
	ODoor,
	CdODoor,
	CDoor,
};

enum floortarget_e
{
	FtoHnF,
	FtoLnF,
	FtoNnF,
	FtoLnC,
	FtoC,
	FbyST,
	Fby24,
	Fby32,
};

enum floorchange_e
{
	FNoChg,
	FChgZero,
	FChgTxt,
	FChgTyp,
};

enum ceilingtarget_e
{
	CtoHnC,
	CtoLnC,
	CtoNnC,
	CtoHnF,
	CtoF,
	CbyST,
	Cby24,
	Cby32,
};

enum ceilingchange_e
{
	CNoChg,
	CChgZero,
	CChgTxt,
	CChgTyp,
};

extern std::list<movingsector_t> movingsectors;
extern std::list<sector_t*> specialdoors;
extern bool s_SpecialFromServer;

#define IgnoreSpecial !serverside && !s_SpecialFromServer
#define NO_TEXTURE 0;

#define CEILSPEED FRACUNIT

std::list<movingsector_t>::iterator P_FindMovingSector(sector_t *sector);
void P_AddMovingCeiling(sector_t *sector);
void P_AddMovingFloor(sector_t *sector);
void P_RemoveMovingCeiling(sector_t *sector);
void P_RemoveMovingFloor(sector_t *sector);
bool P_MovingCeilingCompleted(sector_t *sector);
bool P_MovingFloorCompleted(sector_t *sector);
bool P_HandleSpecialRepeat(line_t* line);
void P_ApplySectorDamage(player_t* player, int damage, int leak, int mod = 0);
void P_ApplySectorDamageNoRandom(player_t* player, int damage, int mod = 0);
void P_ApplySectorDamageNoWait(player_t* player, int damage, int mod = 0);
void P_ApplySectorDamageEndLevel(player_t* player);
void P_CollectSecretCommon(sector_t* sector, player_t* player);
int P_FindSectorFromTagOrLine(int tag, const line_t* line, int start);
int P_FindLineFromTag(int tag, int start);
bool P_FloorActive(const sector_t* sec);
bool P_LightingActive(const sector_t* sec);
bool P_CeilingActive(const sector_t* sec);
fixed_t P_ArgToSpeed(byte arg);
bool P_ArgToCrushType(byte arg);
void P_ResetSectorSpecial(sector_t* sector);
void P_CopySectorSpecial(sector_t* dest, sector_t* source);
byte P_ArgToChange(byte arg);
int P_ArgToCrush(byte arg);
int P_FindSectorFromLineTag(const line_t* line, int start);
int P_ArgToCrushMode(byte arg, bool slowdown);
fixed_t P_ArgsToFixed(fixed_t arg_i, fixed_t arg_f);
bool P_CheckTag(line_t* line);
void P_TransferSectorFlags(unsigned int*, unsigned int);

//jff 2/23/98 identify the special classes that can share sectors

typedef enum
{
	floor_special,
	ceiling_special,
	lighting_special
} special_e;

enum crushmode_e
{
	crushDoom = 0,
	crushHexen = 1,
	crushSlowdown = 2,
};

enum lifttarget_e
{
	F2LnF,
	F2NnF,
	F2LnC,
	LnF2HnF,
};

enum LineActivationType
{
	LineCross,
	LineUse,
	LineShoot,
	LinePush,
	LineACS,
};

enum zdoom_lock_t
{
	zk_none = 0,
	zk_red_card = 1,
	zk_blue_card = 2,
	zk_yellow_card = 3,
	zk_red_skull = 4,
	zk_blue_skull = 5,
	zk_yellow_skull = 6,
	zk_any = 100,
	zk_all = 101,
	zk_red = 129,
	zk_blue = 130,
	zk_yellow = 131,
	zk_redx = 132, // not sure why these redundant ones exist
	zk_bluex = 133,
	zk_yellowx = 134,
	zk_each_color = 229,
};

struct newspecial_s
{
	short special;
	unsigned int flags;
	int damageamount;
	int damageinterval;
	int damageleakrate;
};

#define FLOORSPEED FRACUNIT

bool P_CanUnlockZDoomDoor(player_t* player, zdoom_lock_t lock, bool remote);

// killough 3/7/98: Add generalized scroll effects

class DScroller : public DThinker
{
	DECLARE_SERIAL (DScroller, DThinker)
public:
	enum EScrollType
	{
		sc_side,
		sc_floor,
		sc_ceiling,
		sc_carry,
		sc_carry_ceiling	// killough 4/11/98: carry objects hanging on ceilings

	};

	DScroller (EScrollType type, fixed_t dx, fixed_t dy, int control, int affectee, int accel);
	DScroller (fixed_t dx, fixed_t dy, const line_t *l, int control, int accel);

	void RunThink ();

	bool AffectsWall (int wallnum) { return m_Type == sc_side && m_Affectee == wallnum; }
	int GetWallNum () { return m_Type == sc_side ? m_Affectee : -1; }
	void SetRate (fixed_t dx, fixed_t dy) { m_dx = dx; m_dy = dy; }
	bool IsType(EScrollType type) const { return type == m_Type; }
	int GetAffectee() const { return m_Affectee; }
	int GetAccel() const { return m_Accel; }
	int GetControl() const { return m_Control; }

	EScrollType GetType() const { return m_Type; }
	fixed_t GetScrollX() const { return m_dx; }
	fixed_t GetScrollY() const { return m_dy; }

protected:
	EScrollType m_Type;		// Type of scroll effect
	fixed_t m_dx, m_dy;		// (dx,dy) scroll speeds
	int m_Affectee;			// Number of affected sidedef, sector, tag, or whatever
	int m_Control;			// Control sector (-1 if none) used to control scrolling
	fixed_t m_LastHeight;      	// Last known height of control sector
	fixed_t m_vdx, m_vdy;	        // Accumulated velocity if accelerative
	int m_Accel;			// Whether it's accelerative

private:
	DScroller ();
};

inline FArchive &operator<< (FArchive &arc, DScroller::EScrollType type)
{
	return arc << (BYTE)type;
}
inline FArchive &operator>> (FArchive &arc, DScroller::EScrollType &out)
{
	BYTE in; arc >> in; out = (DScroller::EScrollType)in; return arc;
}

inline bool P_WallScrollType(DScroller::EScrollType type)
{
	if (type == DScroller::sc_side)
	{
		return true;
	}

	return false;
}

inline bool P_FloorScrollType(DScroller::EScrollType type)
{
	if (type == DScroller::sc_floor)
	{
		return true;
	}

	return false;
}

inline bool P_CeilingScrollType(DScroller::EScrollType type)
{
	if (type == DScroller::sc_ceiling)
	{
		return true;
	}

	return false;
}

// phares 3/20/98: added new model of Pushers for push/pull effects

class DPusher : public DThinker
{
	DECLARE_SERIAL (DPusher, DThinker)
public:
	enum EPusher
	{
		p_push,
		p_pull,
		p_wind,
		p_current
	};

	DPusher ();
	DPusher (EPusher type, line_t *l, int magnitude, int angle, AActor *source, int affectee);
	int CheckForSectorMatch (EPusher type, int tag)
	{
		if (m_Type == type && sectors[m_Affectee].tag == tag)
			return m_Affectee;
		else
			return -1;
	}
	void ChangeValues (int magnitude, int angle)
	{
		angle_t ang = (angle<<24) >> ANGLETOFINESHIFT;
		m_Xmag = (magnitude * finecosine[ang]) >> FRACBITS;
		m_Ymag = (magnitude * finesine[ang]) >> FRACBITS;
		m_Magnitude = magnitude;
	}

	virtual void RunThink ();

protected:
	EPusher m_Type;
	AActor::AActorPtr m_Source;		// Point source if point pusher
	int m_Xmag;				// X Strength
	int m_Ymag;				// Y Strength
	int m_Magnitude;		// Vector strength for point pusher
	int m_Radius;			// Effective radius for point pusher
	int m_X;				// X of point source if point pusher
	int m_Y;				// Y of point source if point pusher
	int m_Affectee;			// Number of affected sector

	friend BOOL PIT_PushThing (AActor *thing);
};

inline FArchive &operator<< (FArchive &arc, DPusher::EPusher type)
{
	return arc << (BYTE)type;
}
inline FArchive &operator>> (FArchive &arc, DPusher::EPusher &out)
{
	BYTE in; arc >> in; out = (DPusher::EPusher)in; return arc;
}

BOOL P_CheckKeys (player_t *p, card_t lock, BOOL remote);

// Define values for map objects
#define MO_TELEPORTMAN			14


// [RH] If a deathmatch game, checks to see if noexit is enabled.
//		If so, it kills the player and returns false. Otherwise,
//		it returns true, and the player is allowed to live.
BOOL	CheckIfExitIsGood (AActor *self);

// at game start
void	P_InitPicAnims (void);

// [Blair] ZDoom sector specials
void	P_SpawnZDoomSectorSpecials (void);

// every tic
void	P_UpdateSpecials (void);

// when needed
void    P_CrossSpecialLine (line_t* line, int side, AActor* thing, bool bossaction);
void    P_ShootSpecialLine (AActor* thing, line_t* line);
bool    P_UseSpecialLine (AActor* thing, line_t* line, int side, bool bossaction);
bool    P_PushSpecialLine (AActor* thing, line_t* line, int	side);

void    P_PlayerInZDoomSector (player_t *player);
void P_ApplySectorFriction(int tag, int value, bool use_thinker);

//
// getSector()
// Will return a sector_t*
//	given the number of the current sector,
//	the line number and the side (0/1) that you want.
//
inline sector_t *getSector (int currentSector, int line, int side)
{
	return sides[ (sectors[currentSector].lines[line])->sidenum[side] ].sector;
}


//
// getNextSector()
// Return sector_t * of sector next to current.
// NULL if not two-sided line
//
inline sector_t *getNextSector (line_t *line, sector_t *sec)
{
	if (!(line->flags & ML_TWOSIDED))
		return NULL;

	return line->frontsector == sec ? (line->backsector != sec ? line->backsector : NULL)
	                                : line->frontsector;
}


fixed_t	P_FindLowestFloorSurrounding (sector_t *sec);
fixed_t	P_FindHighestFloorSurrounding (sector_t *sec);

fixed_t	P_FindNextHighestFloor (sector_t *sec);
fixed_t P_FindNextLowestFloor (sector_t* sec);

fixed_t	P_FindLowestCeilingSurrounding (sector_t *sec);		// jff 2/04/98
fixed_t	P_FindHighestCeilingSurrounding (sector_t *sec);	// jff 2/04/98

fixed_t P_FindNextLowestCeiling (sector_t *sec);		// jff 2/04/98
fixed_t P_FindNextHighestCeiling (sector_t *sec);	// jff 2/04/98

fixed_t P_FindShortestTextureAround (sector_t *sec);	// jff 2/04/98
fixed_t P_FindShortestUpperAround (sector_t *sec);		// jff 2/04/98

sector_t* P_FindModelFloorSector (fixed_t floordestheight, sector_t *sec);	//jff 02/04/98
sector_t* P_FindModelCeilingSector (fixed_t ceildestheight, sector_t *sec);	//jff 02/04/98

int		P_FindSectorFromTag (int tag, int start);
int		P_FindLineFromID (int id, int start);

int		P_FindMinSurroundingLight (sector_t *sector, int max);

sector_t *P_NextSpecialSector (sector_t *sec, int type, sector_t *back2);	// [RH]



//
// P_LIGHTS
//

class DLighting : public DSectorEffect
{
	DECLARE_SERIAL (DLighting, DSectorEffect);
public:
	DLighting (sector_t *sector);
protected:
	DLighting ();
};

class DFireFlicker : public DLighting
{
	DECLARE_SERIAL (DFireFlicker, DLighting)
public:
	DFireFlicker (sector_t *sector);
	DFireFlicker (sector_t *sector, int upper, int lower);
	void		RunThink ();
	int GetMaxLight() const { return m_MaxLight; }
	int GetMinLight() const { return m_MinLight; }
protected:
	int 		m_Count;
	int 		m_MaxLight;
	int 		m_MinLight;
private:
	DFireFlicker ();
};

class DFlicker : public DLighting
{
	DECLARE_SERIAL (DFlicker, DLighting)
public:
	DFlicker (sector_t *sector, int upper, int lower);
	void		RunThink ();
	int GetMaxLight() const { return m_MaxLight; }
	int GetMinLight() const { return m_MinLight; }
protected:
	int 		m_Count;
	int 		m_MaxLight;
	int 		m_MinLight;
private:
	DFlicker ();
};

class DLightFlash : public DLighting
{
	DECLARE_SERIAL (DLightFlash, DLighting)
public:
	DLightFlash (sector_t *sector);
	DLightFlash (sector_t *sector, int min, int max);
	void		RunThink ();
	int GetMaxLight() const { return m_MaxLight; }
	int GetMinLight() const { return m_MinLight; }
protected:
	int 		m_Count;
	int 		m_MaxLight;
	int 		m_MinLight;
	int 		m_MaxTime;
	int 		m_MinTime;
private:
	DLightFlash ();
};

class DStrobe : public DLighting
{
	DECLARE_SERIAL (DStrobe, DLighting)
public:
	DStrobe (sector_t *sector, int utics, int ltics, bool inSync);
	DStrobe (sector_t *sector, int upper, int lower, int utics, int ltics);
	void		RunThink ();
	int GetMaxLight() const { return m_MaxLight; }
	int GetMinLight() const { return m_MinLight; }
	int GetDarkTime() const { return m_DarkTime; }
	int GetBrightTime() const { return m_BrightTime; }
	int GetCount() const { return m_Count; }
	void SetCount(int count) { m_Count = count; }
protected:
	int 		m_Count;
	int 		m_MinLight;
	int 		m_MaxLight;
	int 		m_DarkTime;
	int 		m_BrightTime;
private:
	DStrobe ();
};

class DGlow : public DLighting
{
	DECLARE_SERIAL (DGlow, DLighting)
public:
	DGlow (sector_t *sector);
	void		RunThink ();
protected:
	int 		m_MinLight;
	int 		m_MaxLight;
	int 		m_Direction;
private:
	DGlow ();
};

// [RH] Glow from Light_Glow and Light_Fade specials
class DGlow2 : public DLighting
{
	DECLARE_SERIAL (DGlow2, DLighting)
public:
	DGlow2 (sector_t *sector, int start, int end, int tics, bool oneshot);
	void		RunThink ();
	int GetStart() const { return m_Start; }
	int GetEnd() const { return m_End; }
	int GetMaxTics() const { return m_MaxTics; }
	bool GetOneShot() const { return m_OneShot; }
protected:
	int			m_Start;
	int			m_End;
	int			m_MaxTics;
	int			m_Tics;
	bool		m_OneShot;
private:
	DGlow2 ();
};

// [RH] Phased light thinker
class DPhased : public DLighting
{
	DECLARE_SERIAL (DPhased, DLighting)
public:
	DPhased (sector_t *sector);
	DPhased (sector_t *sector, int baselevel, int phase);
	void		RunThink ();
	byte GetBaseLevel() const { return m_BaseLevel; }
	byte GetPhase() const { return m_Phase; }
protected:
	byte		m_BaseLevel;
	byte		m_Phase;
private:
	DPhased ();
	DPhased (sector_t *sector, int baselevel);
	int PhaseHelper (sector_t *sector, int index, int light, sector_t *prev);
};

#define GLOWSPEED				8
#define STROBEBRIGHT			5
#define FASTDARK				15
#define SLOWDARK				TICRATE

void	EV_StartLightFlickering (int tag, int upper, int lower);
void	EV_StartLightStrobing (int tag, int upper, int lower, int utics, int ltics);
void	EV_StartLightStrobing (int tag, int utics, int ltics);
void	EV_TurnTagLightsOff (int tag);
void	EV_LightTurnOn (int tag, int bright);
void	EV_LightChange (int tag, int value);
int     EV_LightTurnOnPartway(int tag, int level);

void	P_SpawnGlowingLight (sector_t *sector);

void	EV_StartLightGlowing (int tag, int upper, int lower, int tics);
void	EV_StartLightFading (int tag, int value, int tics);


//
// P_SWITCH
//
typedef struct
{
	OLumpName	name1;
	OLumpName	name2;
	short		episode;

} switchlist_t;

#define STAIR_USE_SPECIALS 1
#define STAIR_SYNC 2

// 1 second, in ticks.
#define BUTTONTIME		TICRATE

void	P_ChangeSwitchTexture (line_t *line, int useAgain, bool playsound);

void	P_InitSwitchList ();

void	P_ProcessSwitchDef ();

short P_GetButtonTexture(line_t* line);
bool	P_GetButtonInfo (line_t *line, unsigned &state, unsigned &time);
bool	P_SetButtonInfo (line_t *line, unsigned state, unsigned time);

void	P_UpdateButtons (client_t *cl);

//
// P_PLATS
//
class DPlat : public DMovingFloor
{
	DECLARE_SERIAL (DPlat, DMovingFloor);
public:
	enum EPlatState
	{
		init = 0,
		up,
		down,
		waiting,
		in_stasis,
		midup,
		middown,
		finished,
		destroy,
		state_size
	};

	enum EPlatType
	{
		perpetualRaise,
		downWaitUpStay,
		raiseAndChange,
		raiseToNearestAndChange,
		blazeDWUS,
		genLift, // jff added to support generalized Plat types
		genPerpetual,
		toggleUpDn, // jff 3/14/98 added to support instant toggle type
		platPerpetualRaise,
		platDownWaitUpStay,
		platDownWaitUpStayStone,
		platUpNearestWaitDownStay,
		platUpWaitDownStay,
		platDownByValue,
		platUpByValue,
		platUpByValueStay,
		platRaiseAndStay,
		platToggle,
		platDownToNearestFloor,
		platDownToLowestCeiling,
		platRaiseAndStayLockout
	};

	void RunThink ();

	void SetState(byte state, int count) { m_Status = (EPlatState)state; m_Count = count; }
	void GetState(byte &state, int &count) { state = (byte)m_Status; count = m_Count; }

	DPlat(sector_t *sector);
	DPlat(sector_t *sector, DPlat::EPlatType type, fixed_t height, int speed, int delay, fixed_t lip);
	DPlat(sector_t* sector, int target, int delay, int speed, int trigger); // [Blair] Boom Generic Plat type
	DPlat* Clone(sector_t* sec) const;
	friend void P_SetPlatDestroy(DPlat *plat);
	
	void PlayPlatSound ();

	fixed_t 	m_Speed;
	fixed_t 	m_Low;
	fixed_t 	m_High;
	int 		m_Wait;
	int 		m_Count;
	EPlatState	m_Status;
	EPlatState	m_OldStatus;
	bool 		m_Crush;
	int 		m_Tag;
	EPlatType	m_Type;
	fixed_t		m_Height;
	fixed_t		m_Lip;

protected:

	void Reactivate ();
	void Stop ();

private:
	DPlat ();

	friend BOOL	EV_DoPlat (int tag, line_t *line, EPlatType type,
						   fixed_t height, int speed, int delay, fixed_t lip, int change);
	friend void EV_StopPlat (int tag);
	friend void P_ActivateInStasis (int tag);
	friend BOOL EV_DoGenLift(line_t* line);
};

inline FArchive &operator<< (FArchive &arc, DPlat::EPlatType type)
{
	return arc << (BYTE)type;
}
inline FArchive &operator>> (FArchive &arc, DPlat::EPlatType &out)
{
	BYTE in; arc >> in; out = (DPlat::EPlatType)in; return arc;
}
inline FArchive &operator<< (FArchive &arc, DPlat::EPlatState state)
{
	return arc << (BYTE)state;
}
inline FArchive &operator>> (FArchive &arc, DPlat::EPlatState &out)
{
	BYTE in; arc >> in; out = (DPlat::EPlatState)in; return arc;
}

//
// [RH]
// P_PILLAR
//

class DPillar : public DMover
{
	DECLARE_SERIAL (DPillar, DMover)
public:
	enum EPillarState
	{
		init = 0,
		finished,
		destroy,
		state_size
	};
	
	enum EPillar
	{
		pillarBuild,
		pillarOpen

	};

	DPillar ();
	DPillar(sector_t* sector, EPillar type, fixed_t speed, fixed_t height,
	        fixed_t height2, int crush, bool hexencrush);
	DPillar* Clone(sector_t* sec) const;
	friend void P_SetPillarDestroy(DPillar *pillar);	
	friend bool EV_DoZDoomPillar(DPillar::EPillar type, line_t* line, int tag,
	                             fixed_t speed, fixed_t floordist, fixed_t ceilingdist,
	                             int crush, bool hexencrush);
	
	void RunThink ();
	void PlayPillarSound();

	EPillar		m_Type;
	fixed_t		m_FloorSpeed;
	fixed_t		m_CeilingSpeed;
	fixed_t		m_FloorTarget;
	fixed_t		m_CeilingTarget;
	int			m_Crush;
	bool		m_HexenCrush;
	
	EPillarState m_Status;

};

inline FArchive &operator<< (FArchive &arc, DPillar::EPillar type)
{
	return arc << (BYTE)type;
}
inline FArchive &operator>> (FArchive &arc, DPillar::EPillar &out)
{
	BYTE in; arc >> in; out = (DPillar::EPillar)in; return arc;
}
inline FArchive &operator<< (FArchive &arc, DPillar::EPillarState state)
{
	return arc << (BYTE)state;
}
inline FArchive &operator>> (FArchive &arc, DPillar::EPillarState &out)
{
	BYTE in; arc >> in; out = (DPillar::EPillarState)in; return arc;
}

BOOL EV_DoPillar (DPillar::EPillar type, int tag, fixed_t speed, fixed_t height,
				  fixed_t height2, bool crush);
void P_SpawnDoorCloseIn30 (sector_t *sec);
void P_SpawnDoorRaiseIn5Mins (sector_t *sec);

//
// P_DOORS
//
class DDoor : public DMovingCeiling
{
	DECLARE_SERIAL (DDoor, DMovingCeiling)
public:
	enum EVlDoor
	{
		doorClose,
		doorOpen,
		doorRaise,
		doorRaiseIn5Mins,
		doorCloseWaitOpen,
		close30ThenOpen,
		blazeRaise,
		blazeOpen,
		blazeClose,
		waitRaiseDoor,
		waitCloseDoor,

		// jff 02/05/98 add generalize door types
		genRaise,
		genBlazeRaise,
		genOpen,
		genBlazeOpen,
		genClose,
		genBlazeClose,
		genCdO,
		genBlazeCdO,
	};

	enum EDoorState
	{
		init = 0,
		opening,
		closing,
		waiting,
		reopening,
		finished,
		destroy,
		state_size
	};

	DDoor (sector_t *sector);
	// Boom Generic Door
	DDoor(sector_t* sec, line_t* ln, int delay, int time, int trigger,
	      int speed);
	// Boom Generic Locked Door
	DDoor(sector_t* sec, line_t* ln, int kind, int trigger, int speed);
	// Boom Compatible DDoor
    DDoor (sector_t *sec, line_t *ln, EVlDoor type, fixed_t speed, int delay);
	// ZDoom Compatible DDoor
	DDoor(sector_t* sec, line_t* ln, EVlDoor type, fixed_t speed, int topwait,
	      byte lighttag, int topcountdown);
	DDoor* Clone(sector_t* sec) const;

	friend void P_SetDoorDestroy(DDoor *door);
	
	void RunThink ();
	void PlayDoorSound();	

	EVlDoor		m_Type;
	fixed_t 	m_TopHeight;
	fixed_t 	m_Speed;

	// tics to wait at the top
	int 		m_TopWait;
	// (keep in case a door going down is reset)
	// when it reaches 0, start going down
	int 		m_TopCountdown;

	EDoorState	m_Status;

    line_t      *m_Line;

	int			m_LightTag; // ZDoom compat

protected:
	friend BOOL	EV_DoDoor (DDoor::EVlDoor type, line_t *line, AActor *thing,
                                   int tag, int speed, int delay, card_t lock);
    friend BOOL EV_DoZDoomDoor(DDoor::EVlDoor type, line_t* line, AActor* mo, byte tag,
	                         byte speed_byte, int topwait, zdoom_lock_t lock,
	                         byte lightTag, bool boomgen, int topcountdown);
	friend void P_SpawnDoorCloseIn30 (sector_t *sec);
	friend void P_SpawnDoorRaiseIn5Mins (sector_t *sec);

private:
	DDoor ();

};

inline FArchive &operator<< (FArchive &arc, DDoor::EVlDoor type)
{
	return arc << (BYTE)type;
}
inline FArchive &operator>> (FArchive &arc, DDoor::EVlDoor &out)
{
	BYTE in; arc >> in; out = (DDoor::EVlDoor)in; return arc;
}
inline FArchive &operator<< (FArchive &arc, DDoor::EDoorState state)
{
	return arc << (BYTE)state;
}
inline FArchive &operator>> (FArchive &arc, DDoor::EDoorState &out)
{
	BYTE in; arc >> in; out = (DDoor::EDoorState)in; return arc;
}

//
// P_CEILNG
//

// [RH] Changed these
class DCeiling : public DMovingCeiling
{
	DECLARE_SERIAL (DCeiling, DMovingCeiling)
public:
	enum ECeilingState
	{
		init = 0,
		up,
		down,
		waiting,
		finished,
		destroy,
		state_size
	};
	
	enum ECeiling
	{
		lowerToFloor,
		raiseToHighest,
		lowerToLowest,
		lowerToMaxFloor,
		lowerAndCrush,
		crushAndRaise,
		fastCrushAndRaise,
		silentCrushAndRaise,

		ceilLowerByValue,
		ceilRaiseByValue,
		ceilMoveToValue,
		ceilLowerToHighestFloor,
		ceilLowerInstant,
		ceilRaiseInstant,
		ceilCrushAndRaise,
		ceilLowerAndCrush,
		ceilCrushRaiseAndStay,
		ceilRaiseToNearest,
		ceilLowerToLowest,
		ceilLowerToFloor,

		// The following are only used by Generic_Ceiling
		ceilRaiseToHighest,
		ceilLowerToHighest,
		ceilRaiseToLowest,
		ceilLowerToNearest,
		ceilRaiseToHighestFloor,
		ceilRaiseToFloor,
		ceilRaiseByTexture,
		ceilLowerByTexture,

		genCeiling,
		genCeilingChg0,
		genCeilingChgT,
		genCeilingChg,

		genCrusher,
		genSilentCrusher,
	};

	DCeiling (sector_t *sec);
	DCeiling (sector_t *sec, fixed_t speed1, fixed_t speed2, int silent);
	DCeiling (sector_t* sec, line_t* line, int speed,
	         int target, int crush, int change, int direction, int model);
	DCeiling(sector_t* sec, line_t* line, int silent, int speed);
	DCeiling* Clone(sector_t* sec) const;
	friend void P_SetCeilingDestroy(DCeiling *ceiling);
	
	void RunThink ();
	void PlayCeilingSound();	
	
	ECeiling	m_Type;
	crushmode_e m_CrushMode;
	fixed_t 	m_BottomHeight;
	fixed_t 	m_TopHeight;
	fixed_t 	m_Speed;
	fixed_t		m_Speed1;		// [RH] dnspeed of crushers
	fixed_t		m_Speed2;		// [RH] upspeed of crushers
	int 		m_Crush;
	int			m_Silent;
	int 		m_Direction;	// 1 = up, 0 = waiting, -1 = down

	// [RH] Need these for BOOM-ish transferring ceilings
	int			m_Texture;
	short		m_NewSpecial;
	DWORD		m_NewFlags;
	short		m_NewDamageRate;
	byte		m_NewLeakRate;
	byte		m_NewDmgInterval;

	// ID
	int 		m_Tag;
	int 		m_OldDirection;
	
	ECeilingState m_Status;
	
protected:


private:
	DCeiling ();

	friend BOOL EV_DoCeiling (DCeiling::ECeiling type, line_t *line,
		int tag, fixed_t speed, fixed_t speed2, fixed_t height,
		bool crush, int silent, int change);
	friend BOOL EV_CeilingCrushStop (int tag);
	friend void P_ActivateInStasisCeiling (int tag);
	friend BOOL EV_ZDoomCeilingCrushStop(int tag, bool remove);
};

inline FArchive &operator<< (FArchive &arc, DCeiling::ECeiling type)
{
	return arc << (BYTE)type;
}
inline FArchive &operator>> (FArchive &arc, DCeiling::ECeiling &type)
{
	BYTE in; arc >> in; type = (DCeiling::ECeiling)in; return arc;
}
inline FArchive &operator<< (FArchive &arc, DCeiling::ECeilingState state)
{
	return arc << (BYTE)state;
}
inline FArchive &operator>> (FArchive &arc, DCeiling::ECeilingState &out)
{
	BYTE in; arc >> in; out = (DCeiling::ECeilingState)in; return arc;
}


//
// P_FLOOR
//

class DFloor : public DMovingFloor
{
	DECLARE_SERIAL (DFloor, DMovingFloor)
public:
	enum EFloorState
	{
		init = 0,
		up,
		down,
		waiting,
		finished,
		destroy,
		state_size
	};
	
	enum EFloor
	{
		floorLowerToLowest,
		floorLowerToNearest,
		floorLowerToHighest,
		floorLowerByValue,
		floorRaiseByValue,
		floorRaiseToHighest,
		floorRaiseToNearest,
		floorRaiseAndCrush,
		floorRaiseAndCrushDoom,
		floorCrushStop,
		floorLowerInstant,
		floorRaiseInstant,
		floorMoveToValue,
		floorRaiseToLowestCeiling,
		floorRaiseByTexture,

		floorLowerAndChange,
		floorRaiseAndChange,

		floorRaiseToLowest,
		floorRaiseToCeiling,
		floorLowerToLowestCeiling,
		floorLowerByTexture,
		floorLowerToCeiling,

		donutRaise,

		genBuildStair,
		buildStair,
		waitStair,
		resetStair,

		// Not to be used as parameters to EV_DoFloor()
		genFloor,
		genFloorChg0,
		genFloorChgT,
		genFloorChg
	};

	// [RH] Changed to use Hexen-ish specials
	enum EStair
	{
		buildUp,
		buildDown
	};

	DFloor(sector_t *sec);
	DFloor(sector_t *sec, DFloor::EFloor floortype, line_t *line, fixed_t speed,
		   fixed_t height, bool crush, int change);
	DFloor(sector_t* sec, line_t* line, int speed,
	       int target, int crush, int change, int direction, int model);
	DFloor(sector_t* sec, DFloor::EFloor floortype, line_t* line, fixed_t speed,
	               fixed_t height, int crush, int change, bool hexencrush,
	               bool hereticlower);
	DFloor* Clone(sector_t* sec) const;
	friend void P_SetFloorDestroy(DFloor *floor);
	friend BOOL EV_DoGenFloor(line_t* line);
	friend BOOL EV_DoGenStairs(line_t* line);
		
	void RunThink ();
	void PlayFloorSound();	

	EFloor	 	m_Type;
	EFloorState	m_Status;
	int 		m_Crush;
	bool		m_HexenCrush;
	int 		m_Direction;
	short		m_NewSpecial;
	DWORD		m_NewFlags;
	short		m_NewDamageRate;
	byte		m_NewLeakRate;
	byte		m_NewDmgInterval;
	short		m_Texture;
	fixed_t 	m_FloorDestHeight;
	fixed_t 	m_Speed;

	// [RH] New parameters used to reset and delay stairs
	int			m_ResetCount;
	int			m_OrgHeight;
	int			m_Delay;
	int			m_PauseTime;
	int			m_StepTime;
	int			m_PerStepTime;
	
	fixed_t		m_Height;
	line_t		*m_Line;
	int			m_Change;

protected:
	friend BOOL EV_BuildStairs (int tag, DFloor::EStair type, line_t *line,
		fixed_t stairsize, fixed_t speed, int delay, int reset, int igntxt,
		int usespecials);
	friend BOOL EV_DoFloor (DFloor::EFloor floortype, line_t *line, int tag,
		fixed_t speed, fixed_t height, bool crush, int change);
	friend int EV_DoDonut (line_t* line);
	friend BOOL EV_DoZDoomDonut(int tag, line_t* line, fixed_t pillarspeed,
	                            fixed_t slimespeed);
	friend int P_SpawnDonut(int tag, line_t* line, fixed_t pillarspeed, fixed_t slimespeed);
	friend BOOL EV_DoZDoomFloor(DFloor::EFloor floortype, line_t* line, int tag,
	                            fixed_t speed, fixed_t height, int crush, int change,
	                            bool hexencrush, bool hereticlower);

  private:
	DFloor ();
};

inline FArchive &operator<< (FArchive &arc, DFloor::EFloor type)
{
	return arc << (BYTE)type;
}
inline FArchive &operator>> (FArchive &arc, DFloor::EFloor &type)
{
	BYTE in; arc >> in; type = (DFloor::EFloor)in; return arc;
}
inline FArchive &operator<< (FArchive &arc, DFloor::EFloorState state)
{
	return arc << (BYTE)state;
}
inline FArchive &operator>> (FArchive &arc, DFloor::EFloorState &out)
{
	BYTE in; arc >> in; out = (DFloor::EFloorState)in; return arc;
}

class DElevator : public DMover
{
	DECLARE_SERIAL (DElevator, DMover)
public:
	enum EElevatorState
	{
		init = 0,
		finished,
		destroy,
		state_size
	};

	enum EElevator
	{
		elevateUp,
		elevateDown,
		elevateCurrent,
		// [RH] For FloorAndCeiling_Raise/Lower
		elevateRaise,
		elevateLower
	};

	DElevator (sector_t *sec);
	DElevator* Clone(sector_t* sec) const;
	friend void P_SetElevatorDestroy(DElevator *elevator);	

	void RunThink ();
	void PlayElevatorSound();

	EElevator	m_Type;
	int			m_Direction;
	fixed_t		m_FloorDestHeight;
	fixed_t		m_CeilingDestHeight;
	fixed_t		m_Speed;
	
	EElevatorState m_Status;
	
protected:
	friend BOOL EV_DoElevator (line_t *line, DElevator::EElevator type, fixed_t speed,
		fixed_t height, int tag);
    friend bool EV_DoZDoomElevator(line_t* line, DElevator::EElevator type, fixed_t speed,
	                        fixed_t height, int tag);

private:
	DElevator ();
};

inline FArchive &operator<< (FArchive &arc, DElevator::EElevator type)
{
	return arc << (BYTE)type;
}
inline FArchive &operator>> (FArchive &arc, DElevator::EElevator &out)
{
	BYTE in; arc >> in; out = (DElevator::EElevator)in; return arc;
}
inline FArchive &operator<< (FArchive &arc, DElevator::EElevatorState state)
{
	return arc << (BYTE)state;
}
inline FArchive &operator>> (FArchive &arc, DElevator::EElevatorState &out)
{
	BYTE in; arc >> in; out = (DElevator::EElevatorState)in; return arc;
}

// Waggle
/*
class DWaggle : public DMover
{
	DECLARE_SERIAL(DWaggle, DMover)
  public:
	enum EWaggleState
	{
		init = 0,
		expand,
		reduce,
		stable,
		finished,
		destroy,
		state_size
	};
	DWaggle(sector_t* sec);
	DWaggle(sector_t* sector, int height, int speed, int offset, int timer,
	                 bool ceiling);
	DWaggle* Clone(sector_t* sec) const;
	friend void P_SetWaggleDestroy(DWaggle* waggle);

	void RunThink();

	fixed_t m_OriginalHeight;
	fixed_t m_Accumulator;
	fixed_t m_AccDelta;
	fixed_t m_TargetScale;
	fixed_t m_Scale;
	fixed_t m_ScaleDelta;
	fixed_t m_StartTic; // [Blair] Client will predict a created (or serialized) waggle based on the start tic.
	int m_Ticker;
	int m_State;
	bool m_Ceiling;
	DWaggle();

  protected:
	friend BOOL EV_StartPlaneWaggle(int tag, line_t* line, int height, int speed,
	                                int offset, int timer, bool ceiling);
};
*/
//jff 3/15/98 pure texture/type change for better generalized support
enum EChange
{
	trigChangeOnly,
	numChangeOnly
};

BOOL EV_DoChange (line_t *line, EChange changetype, int tag);



//
// P_TELEPT
//
BOOL EV_Teleport (int tid, int tag, int arg0, int side, AActor *thing, int nostop);
BOOL EV_LineTeleport (line_t *line, int side, AActor *thing);
BOOL EV_SilentTeleport(int tid, int useangle, int tag, int keepheight, line_t* line,
                       int side, AActor* thing);
BOOL EV_SilentLineTeleport (line_t *line, int side, AActor *thing, int id,
							BOOL reverse);

//
// [RH] ACS (see also p_acs.h)
//

bool P_StartScript (AActor *who, line_t *where, int script, const char *map, int lineSide,
					int arg0, int arg1, int arg2, int always);
void P_SuspendScript (int script, const char *map);
void P_TerminateScript (int script, const char *map);
void P_StartOpenScripts (void);
void P_DoDeferedScripts (void);


//
// [RH] p_quake.c
//
BOOL P_StartQuake (int tid, int intensity, int duration, int damrad, int tremrad);

// [AM] Trigger actor specials.
bool A_TriggerAction(AActor *mo, AActor *triggerer, int activationType);
