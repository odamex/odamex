// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1998-2006 by Randy Heit (ZDoom).
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
//	New line and sector specials
//
//-----------------------------------------------------------------------------


#ifndef __P_LNSPEC_H__
#define __P_LNSPEC_H__

#include "r_defs.h"

typedef enum {
    // Removed 11/3/06 by ML - No more polyobjects! (1-9)
    // [ML] 9/9/10 - They're back baby!
    Polyobj_StartLine = 1,
	Polyobj_RotateLeft = 2,
	Polyobj_RotateRight = 3,
	Polyobj_Move = 4,
	Polyobj_ExplicitLine = 5,
	Polyobj_MoveTimes8 = 6,
	Polyobj_DoorSwing = 7,
	Polyobj_DoorSlide = 8,
	Line_Horizon = 9,
	Door_Close = 10,
	Door_Open = 11,
	Door_Raise = 12,
	Door_LockedRaise = 13,
	Door_Animated = 14,
	Autosave = 15,
	Transfer_WallLight = 16,
	Thing_Raise = 17,
	StartConversation = 18,
	Thing_Stop = 19,
	Floor_LowerByValue = 20,
	Floor_LowerToLowest = 21,
	Floor_LowerToNearest = 22,
	Floor_RaiseByValue = 23,
	Floor_RaiseToHighest = 24,
	Floor_RaiseToNearest = 25,

	Stairs_BuildDown = 26,
	Stairs_BuildUp = 27,

	Floor_RaiseAndCrush = 28,

	Pillar_Build = 29,
	Pillar_Open = 30,

	Stairs_BuildDownSync = 31,
	Stairs_BuildUpSync = 32,
	ForceField = 33,
	Clear_ForceField = 34,
	Floor_RaiseByValueTimes8 = 35,
	Floor_LowerByValueTimes8 = 36,
	Floor_MoveToValue = 37,
	Ceiling_Waggle = 38,
	Teleport_ZombieChanger = 39,

	Ceiling_LowerByValue = 40,
	Ceiling_RaiseByValue = 41,
	Ceiling_CrushAndRaise = 42,
	Ceiling_LowerAndCrush = 43,
	Ceiling_CrushStop = 44,
	Ceiling_CrushRaiseAndStay = 45,
	Floor_CrushStop = 46,
	Ceiling_MoveToValue = 47,

	Sector_Attach3dMidtex = 48,
	GlassBreak = 49,
	ExtraFloor_LightOnly = 50,
	Sector_SetLink = 51,

	Scroll_Wall = 52,
	Line_SetTextureOffset = 53,

	Sector_ChangeFlags = 54,

	Line_SetBlocking = 55,
	Line_SetTextureScale = 56,

	Sector_SetPortal = 57,
	Sector_CopyScroller = 58,

	Polyobj_OR_MoveToSpot = 59,

	Plat_PerpetualRaise = 60,
	Plat_Stop = 61,
	Plat_DownWaitUpStay = 62,
	Plat_DownByValue = 63,
	Plat_UpWaitDownStay = 64,
	Plat_UpByValue = 65,

	Floor_LowerInstant = 66,
	Floor_RaiseInstant = 67,
	Floor_MoveToValueTimes8 = 68,

	Ceiling_MoveToValueTimes8 = 69,

	Teleport = 70,
	Teleport_NoFog = 71,

	ThrustThing = 72,
	DamageThing = 73,

	Teleport_NewMap = 74,
	Teleport_EndGame = 75,
	Teleport_Other = 76,
	Teleport_Group = 77,
	Teleport_InSector = 78,
	Thing_SetConversation = 79,

	ACS_Execute = 80,
	ACS_Suspend = 81,
	ACS_Terminate = 82,
	ACS_LockedExecute = 83,
	ACS_ExecuteWithResult = 84,
	ACS_LockedExecuteDoor = 85,

	Polyobj_MoveToSpot = 86,
	Polyobj_Stop = 87,
	Polyobj_MoveTo = 88,

	Polyobj_OR_MoveTo = 89,
	Polyobj_OR_RotateLeft = 90,
	Polyobj_OR_RotateRight = 91,
	Polyobj_OR_Move = 92,
	Polyobj_OR_MoveTimes8 = 93,

	Pillar_BuildAndCrush = 94,

	FloorAndCeiling_LowerByValue = 95,
	FloorAndCeiling_RaiseByValue = 96,

	Ceiling_LowerAndCrushDist = 97,

	Sector_SetTranslucent = 98,

	Floor_RaiseAndCrushDoom = 99,

	Scroll_Texture_Left = 100,
	Scroll_Texture_Right = 101,
	Scroll_Texture_Up = 102,
	Scroll_Texture_Down = 103,

	Ceiling_CrushAndRaiseSilentDist = 104,

	Door_WaitRaise = 105,
	Door_WaitClose = 106,

	Line_SetPortalTarget = 107,

	Light_ForceLightning = 109,
	Light_RaiseByValue = 110,
	Light_LowerByValue = 111,
	Light_ChangeToValue = 112,
	Light_Fade = 113,
	Light_Glow = 114,
	Light_Flicker = 115,
	Light_Strobe = 116,
	Light_Stop = 117,

	Plane_Copy = 118,

	Thing_Damage = 119,

	Radius_Quake = 120,	// Earthquake

	Line_SetIdentification = 121,

	Thing_Move = 125,
	Thing_SetSpecial = 127,
    ThrustThingZ = 128,

	UsePuzzleItem = 129,

	Thing_Activate = 130,
	Thing_Deactivate = 131,
	Thing_Remove = 132,
	Thing_Destroy = 133,
	Thing_Projectile = 134,
	Thing_Spawn = 135,
	Thing_ProjectileGravity = 136,
	Thing_SpawnNoFog = 137,

	Floor_Waggle = 138,

	Thing_SpawnFacing = 139,

	Sector_ChangeSound = 140,

	Player_SetTeam = 145,

	Team_Score = 152,
	Team_GivePoints = 153,

// [RH] Begin new specials for ZDoom
	Teleport_NoStop = 154,

	Line_SetPortal = 156,

	SetGlobalFogParameter = 157,

	FS_Execute = 158,

	Sector_SetPlaneReflection = 159,
	Sector_Set3dFloor = 160,
	Sector_SetContents = 161,

	Ceiling_CrushAndRaiseDist = 168,
	Generic_Crusher2 = 169,

	Sector_SetCeilingScale2 = 170,
	Sector_SetFloorScale2 = 171,

	Plat_UpNearestWaitDownStay = 172,
	Noise_Alert = 173,

	SendToCommunicator = 174,

	Thing_ProjectileIntercept = 175,
	Thing_ChangeTID = 176,
	Thing_Hate = 177,
	Thing_ProjectileAimed = 178,

	Change_Skill = 179,

	Thing_SetTranslation = 180,

	Plane_Align = 181,
	Line_Mirror = 182,
	Line_AlignCeiling = 183,
	Line_AlignFloor = 184,

	Sector_SetRotation = 185,
	Sector_SetCeilingPanning = 186,
	Sector_SetFloorPanning = 187,
	Sector_SetCeilingScale = 188,
	Sector_SetFloorScale = 189,

	Static_Init = 190,

	SetPlayerProperty = 191,

	Ceiling_LowerToHighestFloor = 192,
	Ceiling_LowerInstant = 193,
	Ceiling_RaiseInstant = 194,
	Ceiling_CrushRaiseAndStayA = 195,
	Ceiling_CrushAndRaiseA = 196,
	Ceiling_CrushAndRaiseSilentA = 197,
	Ceiling_RaiseByValueTimes8 = 198,
	Ceiling_LowerByValueTimes8 = 199,

	Generic_Floor = 200,
	Generic_Ceiling = 201,
	Generic_Door = 202,
	Generic_Lift = 203,
	Generic_Stairs = 204,
	Generic_Crusher = 205,

	Plat_DownWaitUpStayLip = 206,
	Plat_PerpetualRaiseLip = 207,

	TranslucentLine = 208,

	Transfer_Heights = 209,
	Transfer_FloorLight = 210,
	Transfer_CeilingLight = 211,

	Sector_SetColor = 212,
	Sector_SetFade = 213,
	Sector_SetDamage = 214,

	Teleport_Line = 215,

	Sector_SetGravity = 216,

	Stairs_BuildUpDoom = 217,

	Sector_SetWind = 218,
	Sector_SetFriction = 219,
	Sector_SetCurrent = 220,

	Scroll_Texture_Both = 221,
	Scroll_Texture_Model = 222,
	Scroll_Floor = 223,
	Scroll_Ceiling = 224,
	Scroll_Texture_Offsets = 225,
    ACS_ExecuteAlways = 226,
	PointPush_SetForce = 227,

	Plat_RaiseAndStayTx0 = 228,

	Thing_SetGoal = 229,

	Plat_UpByValueStayTx = 230,
	Plat_ToggleCeiling = 231,

	Light_StrobeDoom = 232,
	Light_MinNeighbor = 233,
	Light_MaxNeighbor = 234,

	Floor_TransferTrigger = 235,
	Floor_TransferNumeric = 236,

	ChangeCamera = 237,

	Floor_RaiseToLowestCeiling = 238,
	Floor_RaiseByValueTxTy = 239,
	Floor_RaiseByTexture = 240,
	Floor_LowerToLowestTxTy = 241,
	Floor_LowerToHighest = 242,

	Exit_Normal = 243,
	Exit_Secret = 244,

	Elevator_RaiseToNearest = 245,
	Elevator_MoveToFloor = 246,
	Elevator_LowerToNearest = 247,

	HealThing = 248,
	Door_CloseWaitOpen = 249,

	Floor_Donut = 250,

	FloorAndCeiling_LowerRaise = 251,

	Ceiling_RaiseToNearest = 252,
	Ceiling_LowerToLowest = 253,
	Ceiling_LowerToFloor = 254,
	Ceiling_CrushRaiseAndStaySilA = 255,

	Floor_LowerToHighestEE = 256,
	Floor_RaiseToLowest = 257,
	Floor_LowerToLowestCeiling = 258,
	Floor_RaiseToCeiling = 259,
	Floor_ToCeilingInstant = 260,
	Floor_LowerByTexture = 261,

	Ceiling_RaiseToHighest = 262,
	Ceiling_ToHighestInstant = 263,
	Ceiling_LowerToNearest = 264,
	Ceiling_RaiseToLowest = 265,
	Ceiling_RaiseToHighestFloor = 266,
	Ceiling_ToFloorInstant = 267,
	Ceiling_RaiseByTexture = 268,
	Ceiling_LowerByTexture = 269,

	Stairs_BuildDownDoom = 270,
	Stairs_BuildUpDoomSync = 271,
	Stairs_BuildDownDoomSync = 272,
	Stairs_BuildUpDoomCrush = 273,

	Door_AnimatedClose = 274,

	Floor_Stop = 275,

	Ceiling_Stop = 276,

	Sector_SetFloorGlow = 277,
	Sector_SetCeilingGlow = 278,

	Floor_MoveToValueAndCrush = 279,
	Ceiling_MoveToValueAndCrush = 280,
	Line_SetAutomapFlags = 281,
	Line_SetAutomapStyle = 282,
} linespecial_t;

typedef enum {
	Init_Gravity = 0,
	Init_Color = 1,
	Init_Damage = 2,
	NUM_STATIC_INITS,
	Init_TransferSky = 255
} staticinit_t;

typedef enum {
	Light_Phased = 1,
	LightSequenceStart = 2,
	LightSequenceSpecial1 = 3,
	LightSequenceSpecial2 = 4,

	Stairs_Special1 = 26,
	Stairs_Special2 = 27,

	// [RH] Equivalents for DOOM's sector specials
	dLight_Flicker = 65,
	dLight_StrobeFast = 66,
	dLight_StrobeSlow = 67,
	dLight_Strobe_Hurt = 68,
	dDamage_Hellslime = 69,
	dDamage_Nukage = 71,
	dLight_Glow = 72,
	dSector_DoorCloseIn30 = 74,
	dDamage_End = 75,
	dLight_StrobeSlowSync = 76,
	dLight_StrobeFastSync = 77,
	dSector_DoorRaiseIn5Mins = 78,
	dSector_LowFriction = 79,
	dDamage_SuperHellslime = 80,
	dLight_FireFlicker = 81,
	dDamage_LavaWimpy = 82,
	dDamage_LavaHefty = 83,
	dScroll_EastLavaDamage = 84,
	hDamage_Sludge = 85,
	
	sLight_Strobe_Hurt = 104,
	sDamage_Hellslime = 105,
	Damage_InstantDeath = 115,
	sDamage_SuperHellslime = 116,
	Scroll_Strife_Current = 118,
	Sector_Hidden = 195,
	Sector_Heal = 196,
	Light_IndoorLightning2 = 198,
	Light_IndoorLightning1 = 199,

	Sky2 = 200,

	Scroll_North_Slow = 201,
	Scroll_North_Medium = 202,
	Scroll_North_Fast = 203,
	Scroll_East_Slow = 204,
	Scroll_East_Medium = 205,
	Scroll_East_Fast = 206,
	Scroll_South_Slow = 207,
	Scroll_South_Medium = 208,
	Scroll_South_Fast = 209,
	Scroll_West_Slow = 210,
	Scroll_West_Medium = 211,
	Scroll_West_Fast = 212,
	Scroll_NorthWest_Slow = 213,
	Scroll_NorthWest_Medium = 214,
	Scroll_NorthWest_Fast = 215,
	Scroll_NorthEast_Slow = 216,
	Scroll_NorthEast_Medium = 217,
	Scroll_NorthEast_Fast = 218,
	Scroll_SouthEast_Slow = 219,
	Scroll_SouthEast_Medium = 220,
	Scroll_SouthEast_Fast = 221,
	Scroll_SouthWest_Slow = 222,
	Scroll_SouthWest_Medium = 223,
	Scroll_SouthWest_Fast = 224,

	// heretic-type scrollers
	Scroll_Carry_East5 = 225,
	Scroll_Carry_East10 = 226,
	Scroll_Carry_East25 = 227,
	Scroll_Carry_East30 = 228,
	Scroll_Carry_East35 = 229,
	Scroll_Carry_North5 = 230,
	Scroll_Carry_North10 = 231,
	Scroll_Carry_North25 = 232,
	Scroll_Carry_North30 = 233,
	Scroll_Carry_North35 = 234,
	Scroll_Carry_South5 = 235,
	Scroll_Carry_South10 = 236,
	Scroll_Carry_South25 = 237,
	Scroll_Carry_South30 = 238,
	Scroll_Carry_South35 = 239,
	Scroll_Carry_West5 = 240,
	Scroll_Carry_West10 = 241,
	Scroll_Carry_West25 = 242,
	Scroll_Carry_West30 = 243,
	Scroll_Carry_West35 = 244
} sectorspecial_t;

// [RH] Equivalents for BOOM's generalized sector types

#define DAMAGE_MASK     0x60
#define DAMAGE_SHIFT    5
#define SECRET_MASK     0x80
#define SECRET_SHIFT    7
#define FRICTION_MASK   0x100
#define FRICTION_SHIFT  8
#define PUSH_MASK       0x200
#define PUSH_SHIFT      9

// mbf21
#define DEATH_MASK         0x1000 // bit 12
#define KILL_MONSTERS_MASK 0x2000 // bit 13

// Define masks, shifts, for fields in generalized linedef types
// (from BOOM's p_psec.h file)

#define GenEnd          (0x8000)
#define GenFloorBase	(0x6000)
#define GenCeilingBase	(0x4000)
#define GenDoorBase		(0x3c00)
#define GenLockedBase	(0x3800)
#define GenLiftBase		(0x3400)
#define GenStairsBase	(0x3000)
#define GenCrusherBase	(0x2F80)

#define TriggerType     0x0007
#define TriggerTypeShift 0

#define OdamexStaticInits (333)

// define masks and shifts for the floor type fields

#define FloorCrush            0x1000
#define FloorChange           0x0c00
#define FloorTarget           0x0380
#define FloorDirection        0x0040
#define FloorModel            0x0020
#define FloorSpeed            0x0018

#define FloorCrushShift           12
#define FloorChangeShift          10
#define FloorTargetShift           7
#define FloorDirectionShift        6
#define FloorModelShift            5
#define FloorSpeedShift            3

// define masks and shifts for the ceiling type fields

#define CeilingCrush          0x1000
#define CeilingChange         0x0c00
#define CeilingTarget         0x0380
#define CeilingDirection      0x0040
#define CeilingModel          0x0020
#define CeilingSpeed          0x0018

#define CeilingCrushShift         12
#define CeilingChangeShift        10
#define CeilingTargetShift         7
#define CeilingDirectionShift      6
#define CeilingModelShift          5
#define CeilingSpeedShift          3

// define masks and shifts for the lift type fields

#define LiftTarget            0x0300
#define LiftDelay             0x00c0
#define LiftMonster           0x0020
#define LiftSpeed             0x0018

#define LiftTargetShift            8
#define LiftDelayShift             6
#define LiftMonsterShift           5
#define LiftSpeedShift             3

// define masks and shifts for the stairs type fields

#define StairIgnore           0x0200
#define StairDirection        0x0100
#define StairStep             0x00c0
#define StairMonster          0x0020
#define StairSpeed            0x0018

#define StairIgnoreShift           9
#define StairDirectionShift        8
#define StairStepShift             6
#define StairMonsterShift          5
#define StairSpeedShift            3

// define masks and shifts for the crusher type fields

#define CrusherSilent         0x0040
#define CrusherMonster        0x0020
#define CrusherSpeed          0x0018

#define CrusherSilentShift         6
#define CrusherMonsterShift        5
#define CrusherSpeedShift          3

// define masks and shifts for the door type fields

#define DoorDelay             0x0300
#define DoorMonster           0x0080
#define DoorKind              0x0060
#define DoorSpeed             0x0018

#define DoorDelayShift             8
#define DoorMonsterShift           7
#define DoorKindShift              5
#define DoorSpeedShift             3

// define masks and shifts for the locked door type fields

#define LockedNKeys           0x0200
#define LockedKey             0x01c0
#define LockedKind            0x0020
#define LockedSpeed           0x0018

#define LockedNKeysShift           9
#define LockedKeyShift             6
#define LockedKindShift            5
#define LockedSpeedShift           3

#define ZDOOM_DAMAGE_MASK     0x0300
#define ZDOOM_SECRET_MASK     0x0400
#define ZDOOM_FRICTION_MASK   0x0800
#define ZDOOM_PUSH_MASK       0x1000

// Speeds for ceilings/crushers (x/8 units per tic)
//	(Hexen crushers go up at half the speed that they go down)
#define C_SLOW 8
#define C_NORMAL 16
#define C_FAST 32
#define C_TURBO 64

#define CEILWAIT 150

// Speeds for floors (x/8 units per tic)
#define F_SLOW 8
#define F_NORMAL 16
#define F_FAST 32
#define F_TURBO 64

// Speeds for doors (x/8 units per tic)
#define D_SLOW 16
#define D_NORMAL 32
#define D_FAST 64
#define D_TURBO 128

#define VDOORWAIT 150
#define VDOORSPEED (FRACUNIT * 2)

// Speeds for stairs (x/8 units per tic)
#define S_SLOW 2
#define S_NORMAL 4
#define S_FAST 16
#define S_TURBO 32

// Speeds for plats (Hexen plats stop 8 units above the floor)
#define P_SLOW 8
#define P_NORMAL 16
#define P_FAST 32
#define P_TURBO 64

#define PLATWAIT 105

#define ELEVATORSPEED 32

// Speeds for donut slime and pillar (x/8 units per tic)
#define DORATE 4

// Texture scrollers operate at a rate of x/64 units per tic.
#define SCROLL_UNIT 64

// define names for the TriggerType field of the general linedefs

typedef enum
{
	WalkOnce,
	WalkMany,
	SwitchOnce,
	SwitchMany,
	GunOnce,
	GunMany,
	PushOnce,
	PushMany
} triggertype_e;

// Line special translation structure
typedef struct
{
	byte flags;
	byte newspecial;
	byte args[5];
} xlat_t;

#define TAG 123 // Special value that gets replaced with the line tag

#define WALK ((byte)((SPAC_CROSS << ML_SPAC_SHIFT) >> 8))
#define USE ((byte)((SPAC_USE << ML_SPAC_SHIFT) >> 8))
#define SHOOT ((byte)((SPAC_IMPACT << ML_SPAC_SHIFT) >> 8))
#define MONST ((byte)(ML_MONSTERSCANACTIVATE >> 8))
#define MONWALK ((byte)((SPAC_MCROSS << ML_SPAC_SHIFT) >> 8))
#define REP ((byte)(ML_REPEAT_SPECIAL >> 8))

struct line_s;
class AActor;

typedef BOOL (*lnSpecFunc)(struct line_s	*line,
						   class AActor		*activator,
						   int				arg1,
						   int				arg2,
						   int				arg3,
						   int				arg4,
						   int				arg5);

extern lnSpecFunc LineSpecials[283];

BOOL EV_CeilingCrushStop (int tag);
int EV_DoDonut (int tag, fixed_t pillarspeed, fixed_t slimespeed);
void EV_StopPlat (int tag);
int EV_ZDoomFloorCrushStop(int tag);
bool EV_DoZDoomDonut(int tag, line_t* line, fixed_t pillarspeed, fixed_t slimespeed);
bool EV_DoZDoomCeiling(DCeiling::ECeiling type, line_t* line, byte tag, fixed_t speed,
                       fixed_t speed2, fixed_t height, int crush, byte silent, int change,
                       crushmode_e crushmode);
bool EV_ZDoomCeilingCrushStop(int tag, bool remove);
bool EV_StartPlaneWaggle(int tag, line_t* line, int height, int speed, int offset,
                         int timer, bool ceiling);
bool EV_CompatibleTeleport(int tag, line_t* line, int side, AActor* thing, int flags);

bool P_LineSpecialMovesSector(byte special);
bool P_CanActivateSpecials(AActor* mo, line_t* line);
bool P_ActorInSpecialSector(AActor* actor);

int P_FindLineFromLineTag(const line_t* line, int start);
int P_IsUnderDamage(const AActor* actor);
bool P_IsOnLift(const AActor* actor);
int P_IsUnderDamage(AActor* actor);
static bool P_CheckRange(AActor* actor, fixed_t range);
void EV_LightSetMinNeighbor(int tag);
void EV_LightSetMaxNeighbor(int tag);

extern int TeleportSide;

#endif //__P_LNSPEC_H__

