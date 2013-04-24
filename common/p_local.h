// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 2006-2013 by The Odamex Team.
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
//	Play functions, animation, global header.
//
//-----------------------------------------------------------------------------


#ifndef __P_LOCAL__
#define __P_LOCAL__

#ifndef __R_LOCAL__
#include "r_local.h"
#endif

#include <set>

#define FLOATSPEED		(FRACUNIT*4)

#define STEEPSLOPE		46341	// [RH] Minimum floorplane.c value for walking

#define MAXHEALTH		100
#define VIEWHEIGHT		(41*FRACUNIT)

// mapblocks are used to check movement
// against lines and things
#define MAPBLOCKUNITS	128
#define MAPBLOCKSIZE	(MAPBLOCKUNITS*FRACUNIT)
#define MAPBLOCKSHIFT	(FRACBITS+7)
#define MAPBMASK		(MAPBLOCKSIZE-1)
#define MAPBTOFRAC		(MAPBLOCKSHIFT-FRACBITS)


// player radius for movement checking
#define PLAYERRADIUS	16*FRACUNIT

// MAXRADIUS is for precalculated sector block boxes
// the spider demon is larger,
// but we do not have any moving sectors nearby
#define MAXRADIUS		32*FRACUNIT

//#define GRAVITY 		FRACUNIT
#define MAXMOVE 		(30*FRACUNIT)

#define STOPSPEED						0x1000
#define FRICTION						0xe800

#define USERANGE		(64*FRACUNIT)
#define MELEERANGE		(64*FRACUNIT)
#define MISSILERANGE	(32*64*FRACUNIT)

#define WATER_SINK_FACTOR		3
#define WATER_SINK_SMALL_FACTOR	4
#define WATER_SINK_SPEED		(FRACUNIT/2)
#define WATER_JUMP_SPEED		(FRACUNIT*7/2)

// follow a player exlusively for 3 seconds
#define BASETHRESHOLD	100

//
// P_PSPR
//
void P_SetupPsprites (player_t* curplayer);
void P_MovePsprites (player_t* curplayer);
void P_DropWeapon (player_t* player);


//
// P_USER
//
void P_FallingDamage (AActor *ent);
void P_PlayerThink (player_t *player);
bool P_AreTeammates(player_t &a, player_t &b);
bool P_CanSpy(player_t &viewer, player_t &other);

//
// P_MOBJ
//
#define ONFLOORZ		MININT
#define ONCEILINGZ		MAXINT

// Time interval for item respawning.
#define ITEMQUESIZE 	128

extern mapthing2_t		itemrespawnque[ITEMQUESIZE];
extern int				itemrespawntime[ITEMQUESIZE];
extern int				iquehead;
extern int				iquetail;

void 	P_ThrustMobj (AActor *mo, angle_t angle, fixed_t move);
void	P_RespawnSpecials (void);

bool	P_SetMobjState (AActor* mobj, statenum_t state, bool cl_update = false);

void	P_SpawnPuff (fixed_t x, fixed_t y, fixed_t z);
void	P_SpawnBlood (fixed_t x, fixed_t y, fixed_t z, int damage);
AActor* P_SpawnMissile (AActor* source, AActor* dest, mobjtype_t type);
void	P_SpawnPlayerMissile (AActor* source, mobjtype_t type);

void	P_RailAttack (AActor *source, int damage, int offset);	// [RH] Shoot a railgun
bool	P_HitFloor (AActor *thing);
//
// [RH] P_THINGS
//
extern int SpawnableThings[];
extern const int NumSpawnableThings;

BOOL	P_Thing_Spawn (int tid, int type, angle_t angle, BOOL fog);
BOOL	P_Thing_Projectile (int tid, int type, angle_t angle,
							fixed_t speed, fixed_t vspeed, BOOL gravity);
BOOL	P_ActivateMobj (AActor *mobj, AActor *activator);
BOOL	P_DeactivateMobj (AActor *mobj);

//
// P_ENEMY
//
void	P_NoiseAlert (AActor* target, AActor* emmiter);
void	P_SpawnBrainTargets(void);	// killough 3/26/98: spawn icon landings

extern struct brain_s {				// killough 3/26/98: global state of boss brain
	int easy, targeton;
} brain;


//
// P_MAPUTL
//
typedef struct
{
	fixed_t 	x;
	fixed_t 	y;
	fixed_t 	dx;
	fixed_t 	dy;

} divline_t;

typedef struct
{
	fixed_t 	frac;			// along trace line
	BOOL 	isaline;
	union {
		AActor* thing;
		line_t* line;
	}					d;
} intercept_t;

#define MAXINTERCEPTS	128

extern TArray<intercept_t> intercepts;

typedef BOOL (*traverser_t) (intercept_t *in);

subsector_t* P_PointInSubsector(fixed_t x, fixed_t y);
fixed_t P_AproxDistance (fixed_t dx, fixed_t dy);
fixed_t P_AproxDistance2 (fixed_t *pos_array, fixed_t x, fixed_t y);
fixed_t P_AproxDistance2 (AActor *mo, fixed_t x, fixed_t y);
fixed_t P_AproxDistance2 (AActor *a, AActor *b);

bool P_ActorInFOV(AActor* origin, AActor* mo , float f, fixed_t dist);

int 	P_PointOnLineSide (fixed_t x, fixed_t y, const line_t *line);
int 	P_PointOnDivlineSide (fixed_t x, fixed_t y, const divline_t *line);
void	P_MakeDivline (const line_t *li, divline_t *dl);
fixed_t P_InterceptVector (const divline_t *v2, const divline_t *v1);
int 	P_BoxOnLineSide (const fixed_t *tmbox, const line_t *ld);

extern fixed_t			opentop;
extern fixed_t			openbottom;
extern fixed_t			openrange;
extern fixed_t			lowfloor;

void P_LineOpening (const line_t *linedef, fixed_t x, fixed_t y, fixed_t refx=MINFIXED, fixed_t refy=0);

BOOL P_BlockLinesIterator (int x, int y, BOOL(*func)(line_t*) );
BOOL P_BlockThingsIterator (int x, int y, BOOL(*func)(AActor*), AActor *start=NULL);

#define PT_ADDLINES 	1
#define PT_ADDTHINGS	2
#define PT_EARLYOUT 	4

extern divline_t		trace;

BOOL
P_PathTraverse
( fixed_t		x1,
  fixed_t		y1,
  fixed_t		x2,
  fixed_t		y2,
  int			flags,
  BOOL		(*trav) (intercept_t *));

// [ML] 2/1/10: Break out P_PointToAngle from R_PointToAngle2 (from EE)
angle_t P_PointToAngle(fixed_t xo, fixed_t yo, fixed_t x, fixed_t y);

//
// P_MAP
//

// If "floatok" true, move would be ok
// if within "tmfloorz - tmceilingz".
extern BOOL				floatok;
extern fixed_t			tmfloorz;
extern fixed_t			tmceilingz;
extern msecnode_t		*sector_list;		// phares 3/16/98
extern AActor			*BlockingMobj;
extern line_t			*BlockingLine;		// Used only by P_Move
											// This is not necessarily a *blocking* line

extern fixed_t			tmdropoffz; //Needed in b_move.c
extern sector_t			*tmfloorsector;

extern	line_t* 		ceilingline;

void	P_TestActorMovement(AActor *mo, fixed_t tryx, fixed_t tryy, fixed_t tryz,
						fixed_t &destx, fixed_t &desty, fixed_t &destz);
bool	P_TestMobjZ (AActor *actor);
BOOL	P_TestMobjLocation (AActor *mobj);
bool	P_CheckPosition (AActor *thing, fixed_t x, fixed_t y);
AActor	*P_CheckOnmobj (AActor *thing);
void	P_FakeZMovement (AActor *mo);
bool	P_CheckSlopeWalk (AActor *actor, fixed_t &xmove, fixed_t &ymove);
BOOL	P_TryMove (AActor* thing, fixed_t x, fixed_t y, bool dropoff, bool onfloor = false);
BOOL	P_TeleportMove (AActor* thing, fixed_t x, fixed_t y, fixed_t z, BOOL telefrag);	// [RH] Added z and telefrag parameters
void	P_SlideMove (AActor* mo);
bool	P_CheckSight (const AActor* t1, const AActor* t2);
void	P_UseLines (player_t* player);
void	P_ApplyTorque(AActor *mo);
void	P_CopySector(sector_t *dest, sector_t *src);

fixed_t P_PlaneZ(fixed_t x, fixed_t y, const plane_t *plane);
double P_PlaneZ(double x, double y, const plane_t *plane);
fixed_t P_FloorHeight(fixed_t x, fixed_t y, const sector_t *sec = NULL);
fixed_t P_FloorHeight(const AActor *mo);
fixed_t P_FloorHeight(const sector_t *sector);
fixed_t P_CeilingHeight(fixed_t x, fixed_t y, const sector_t *sec = NULL);
fixed_t P_CeilingHeight(const AActor *mo);
fixed_t P_CeilingHeight(const sector_t *sector);
fixed_t P_LowestHeightOfCeiling(sector_t *sector);
fixed_t P_LowestHeightOfFloor(sector_t *sector);
fixed_t P_HighestHeightOfCeiling(sector_t *sector);
fixed_t P_HighestHeightOfFloor(sector_t *sector);

bool P_IsPlaneLevel(const plane_t *plane);
bool P_IdenticalPlanes(const plane_t *pl1, const plane_t *pl2);
void P_InvertPlane(plane_t *plane);
void P_ChangeCeilingHeight(sector_t *sector, fixed_t amount);
void P_ChangeFloorHeight(sector_t *sector, fixed_t amount);
void P_SetCeilingHeight(sector_t *sector, fixed_t value);
void P_SetFloorHeight(sector_t *sector, fixed_t value);
bool P_PointOnPlane(const plane_t *plane, fixed_t x, fixed_t y, fixed_t z);
bool P_PointAbovePlane(const plane_t *plane, fixed_t x, fixed_t y, fixed_t z);
bool P_PointBelowPlane(const plane_t *plane, fixed_t x, fixed_t y, fixed_t z);

struct v3fixed_t;
v3fixed_t P_LinePlaneIntersection(const plane_t *plane, const v3fixed_t &lineorg, const v3fixed_t &linedir);


bool P_CheckSightEdges(const AActor* t1, const AActor* t2, float radius_boost);

bool	P_ChangeSector (sector_t* sector, bool crunch);

extern	AActor*	linetarget; 	// who got hit (or NULL)

fixed_t P_AimLineAttack (AActor *t1, angle_t angle, fixed_t distance);
fixed_t P_AutoAimLineAttack(AActor* actor, angle_t& angle, const angle_t spread, const int tracers, fixed_t distance);
void	P_LineAttack (AActor *t1, angle_t angle, fixed_t distance, fixed_t slope, int damage);

// [RH] Position the chasecam
void	P_AimCamera (AActor *t1);
extern	fixed_t CameraX, CameraY, CameraZ;

// [RH] Means of death
void	P_RadiusAttack (AActor *spot, AActor *source, int damage, int distance, bool hurtSelf, int mod);

void	P_DelSeclist(msecnode_t *);							// phares 3/16/98
void	P_CreateSecNodeList(AActor*,fixed_t,fixed_t);		// phares 3/14/98
int		P_GetMoveFactor(const AActor *mo, int *frictionp);	// phares  3/6/98
int		P_GetFriction(const AActor *mo, int *frictionfactor);
BOOL	Check_Sides(AActor *, int, int);					// phares


//
// P_SETUP
//
extern byte*			rejectmatrix;	// for fast sight rejection
extern BOOL				rejectempty;
extern int*				blockmaplump;	// offsets in blockmap are from here
extern int*				blockmap;
extern int				bmapwidth;
extern int				bmapheight; 	// in mapblocks
extern fixed_t			bmaporgx;
extern fixed_t			bmaporgy;		// origin of block map
extern AActor** 		blocklinks; 	// for thing chains

extern std::set<short>	movable_sectors;


//
// P_INTER
//
extern int				maxammo[NUMAMMO];
extern int				clipammo[NUMAMMO];

void P_GiveSpecial(player_t *player, AActor *special);
void P_TouchSpecialThing (AActor *special, AActor *toucher);

void P_DamageMobj (AActor *target, AActor *inflictor, AActor *source, int damage, int mod=0, int flags=0);

#define DMG_NO_ARMOR		1

// [RH] Means of death flags (based on Quake2's)
#define MOD_UNKNOWN			0
#define MOD_FIST			1
#define MOD_PISTOL			2
#define MOD_SHOTGUN			3
#define MOD_CHAINGUN		4
#define MOD_ROCKET			5
#define MOD_R_SPLASH		6
#define MOD_PLASMARIFLE		7
#define MOD_BFG_BOOM		8
#define MOD_BFG_SPLASH		9
#define MOD_CHAINSAW		10
#define MOD_SSHOTGUN		11
#define MOD_WATER			12
#define MOD_SLIME			13
#define MOD_LAVA			14
#define MOD_CRUSH			15
#define MOD_TELEFRAG		16
#define MOD_FALLING			17
#define MOD_SUICIDE			18
#define MOD_BARREL			19
#define MOD_EXIT			20
#define MOD_SPLASH			21
#define MOD_HIT				22
#define MOD_RAILGUN			23
#define MOD_FRIENDLY_FIRE	0x80000000

extern	int MeansOfDeath;

//
// PO_MAN
//
typedef enum
{
	PODOOR_NONE,
	PODOOR_SLIDE,
	PODOOR_SWING,
	
	NUMTYPES	
} podoortype_t;

inline FArchive &operator<< (FArchive &arc, podoortype_t type)
{
	return arc << (BYTE)type;
}
inline FArchive &operator>> (FArchive &arc, podoortype_t &out)
{
	BYTE in; arc >> in; out = (podoortype_t)in; return arc;
}

class DPolyAction : public DThinker
{
	DECLARE_SERIAL (DPolyAction, DThinker)
public:
	DPolyAction (int polyNum);
protected:
	DPolyAction ();
	int m_PolyObj;
	int m_Speed;
	int m_Dist;

	friend void ThrustMobj (AActor *actor, seg_t *seg, polyobj_t *po);
};

class DRotatePoly : public DPolyAction
{
	DECLARE_SERIAL (DRotatePoly, DPolyAction)
public:
	DRotatePoly (int polyNum);
	void RunThink ();
protected:
	friend BOOL EV_RotatePoly (line_t *line, int polyNum, int speed, int byteAngle, int direction, BOOL overRide);
private:
	DRotatePoly ();
};

class DMovePoly : public DPolyAction
{
	DECLARE_SERIAL (DMovePoly, DPolyAction)
public:
	DMovePoly (int polyNum);
	void RunThink ();
protected:
	DMovePoly ();
	int m_Angle;
	fixed_t m_xSpeed; // for sliding walls
	fixed_t m_ySpeed;

	friend BOOL EV_MovePoly (line_t *line, int polyNum, int speed, angle_t angle, fixed_t dist, BOOL overRide);
};

class DPolyDoor : public DMovePoly
{
	DECLARE_SERIAL (DPolyDoor, DMovePoly)
public:
	DPolyDoor (int polyNum, podoortype_t type);
	void RunThink ();
protected:
	int m_Direction;
	int m_TotalDist;
	int m_Tics;
	int m_WaitTics;
	podoortype_t m_Type;
	bool m_Close;

	friend BOOL EV_OpenPolyDoor (line_t *line, int polyNum, int speed, angle_t angle, int delay, int distance, podoortype_t type);
private:
	DPolyDoor ();
};

// [RH] Data structure for P_SpawnMapThing() to keep track
//		of polyobject-related things.
typedef struct polyspawns_s
{
	struct polyspawns_s *next;
	fixed_t x;
	fixed_t y;
	short angle;
	short type;
} polyspawns_t;

enum
{
	PO_HEX_ANCHOR_TYPE = 3000,
	PO_HEX_SPAWN_TYPE,
	PO_HEX_SPAWNCRUSH_TYPE,

	// [RH] Thing numbers that don't conflict with Doom things
	PO_ANCHOR_TYPE = 9300,
	PO_SPAWN_TYPE,
	PO_SPAWNCRUSH_TYPE
};

#define PO_LINE_START 1 // polyobj line start special
#define PO_LINE_EXPLICIT 5

extern polyobj_t *polyobjs; // list of all poly-objects on the level
extern int po_NumPolyobjs;
extern polyspawns_t *polyspawns;	// [RH] list of polyobject things to spawn


BOOL PO_MovePolyobj (int num, int x, int y);
BOOL PO_RotatePolyobj (int num, angle_t angle);
void PO_Init (void);
BOOL PO_Busy (int polyobj);

//
// P_SPEC
//
#include "p_spec.h"


#endif	// __P_LOCAL__


