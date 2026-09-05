// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1998-2006 by Randy Heit (ZDoom).
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
//	New line and sector specials
//
//-----------------------------------------------------------------------------

#pragma once

#include "r_defs.h"
#include "flags.h"

#include <concepts>

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

// Doom linedef types.
//
// Copied from Helion, with some changes

enum class doomLineSpecial_t : int16_t
{
	DR_Door_Raise = 1,
	W1_Door_Open = 2,
	W1_Door_Close = 3,
	W1_Door_Raise = 4,
	W1_Floor_RaiseToLowestCeiling = 5,
	W1_Ceiling_CrushAndRaiseAFast = 6,
	S1_Stairs_BuildUpDoom = 7,
	W1_Stairs_BuildUpDoom = 8,
	S1_Floor_Donut = 9,
	W1_Plat_DownWaitUpStayLipFast = 10,
	S1_Exit_Normal = 11,
	W1_Light_MaxNeighbor = 12,
	W1_Light_ChangeToValue255 = 13,
	S1_Plat_UpByValueStayTxBy32 = 14,
	S1_Plat_UpByValueStayTxBy24 = 15,
	W1_Door_CloseWaitOpen = 16,
	W1_Light_StrobeDoom = 17,
	S1_Floor_RaiseToNearest = 18,
	W1_Floor_LowerToHighest = 19,
	S1_Plat_RaiseAndStayTx0 = 20,
	S1_Plat_DownWaitUpStayLipFast = 21,
	W1_Plat_RaiseAndStayTx0 = 22,
	S1_Floor_LowerToLowest = 23,
	G1_Floor_RaiseToLowestCeiling = 24,
	W1_Ceiling_CrushAndRaiseASlow = 25,
	DR_Door_LockedRaiseBlue = 26,
	DR_Door_LockedRaiseYellow = 27,
	DR_Door_LockedRaiseRed = 28,
	S1_Door_Raise = 29,
	W1_Floor_RaiseByTexture = 30,
	D1_Door_Open = 31,
	D1_Door_LockedRaiseBlue = 32,
	D1_Door_LockedRaiseRed = 33,
	D1_Door_LockedRaiseYellow = 34,
	W1_Light_ChangeToValue35 = 35,
	W1_Floor_LowerToHighestFast = 36,
	W1_Floor_LowerToLowestTxTy = 37,
	W1_Floor_LowerToLowest = 38,
	W1_Teleport = 39,
	W1_Generic_Ceiling = 40,
	S1_Ceiling_LowerToFloor = 41,
	SR_Door_Close = 42,
	SR_Ceiling_LowerToFloor = 43,
	W1_Ceiling_LowerAndCrush = 44,
	SR_Floor_LowerToHighest = 45,
	GR_Door_Open = 46,
	G1_Plat_RaiseAndStayTx0 = 47,
	ST_Scroll_Texture_Left = 48,
	S1_Ceiling_CrushAndRaiseASlow = 49,
	S1_Door_Close = 50,
	S1_Exit_Secret = 51,
	W1_Exit_Normal = 52,
	W1_Plat_PerpetualRaiseLip = 53,
	W1_Plat_Stop = 54,
	S1_Floor_RaiseAndCrush = 55,
	W1_Floor_RaiseAndCrush = 56,
	W1_Ceiling_CrushStop = 57,
	W1_Floor_RaiseByValue = 58,
	W1_Floor_RaiseByValueTxTy = 59,
	SR_Floor_LowerToLowest = 60,
	SR_Door_Open = 61,
	SR_Plat_DownWaitUpStayLipFast = 62,
	SR_Door_Raise = 63,
	SR_Floor_RaiseToLowestCeiling = 64,
	SR_Floor_RaiseAndCrush = 65,
	SR_Plat_UpByValueStayTxBy24 = 66,
	SR_Plat_UpByValueStayTxBy32 = 67,
	SR_Plat_RaiseAndStayTx0 = 68,
	SR_Floor_RaiseToNearest = 69,
	SR_Floor_LowerToHighestFast = 70,
	S1_Floor_LowerToHighestFast = 71,
	WR_Ceiling_LowerAndCrush = 72,
	WR_Ceiling_CrushAndRaiseASlow = 73,
	WR_Ceiling_CrushStop = 74,
	WR_Door_Close = 75,
	WR_Door_CloseWaitOpen = 76,
	WR_Ceiling_CrushAndRaiseAFast = 77,
	SR_Floor_TransferNumeric = 78,
	WR_Light_ChangeToValue35 = 79,
	WR_Light_MaxNeighbor = 80,
	WR_Light_ChangeToValue255 = 81,
	WR_Floor_LowerToLowest = 82,
	WR_Floor_LowerToHighest = 83,
	WR_Floor_LowerToLowestTxTy = 84,
	ST_Scroll_Texture_Right = 85,
	WR_Door_Open = 86,
	WR_Plat_PerpetualRaiseLip = 87,
	WR_Plat_DownWaitUpStayLipFast = 88,
	WR_Plat_Stop = 89,
	WR_Door_Raise = 90,
	WR_Floor_RaiseToLowestCeiling = 91,
	WR_Floor_RaiseByValue = 92,
	WR_Floor_RaiseByValueTxTy = 93,
	WR_Floor_RaiseAndCrush = 94,
	WR_Plat_RaiseAndStayTx0 = 95,
	WR_Floor_RaiseByTexture = 96,
	WR_Teleport = 97,
	WR_Floor_LowerToHighestFast = 98,
	SR_Door_LockedRaiseFastBlue = 99,
	W1_Stairs_BuildUpDoomTurbo = 100,
	S1_Floor_RaiseToLowestCeiling = 101,
	S1_Floor_LowerToHighest = 102,
	S1_Door_Open = 103,
	W1_Light_MinNeighbor = 104,
	WR_Door_RaiseFast = 105,
	WR_Door_OpenFast = 106,
	WR_Door_CloseFast = 107,
	W1_Door_RaiseFast = 108,
	W1_Door_OpenFast = 109,
	W1_Door_CloseFast = 110,
	S1_Door_RaiseFast = 111,
	S1_Door_OpenFast = 112,
	S1_Door_CloseFast = 113,
	SR_Door_RaiseFast = 114,
	SR_Door_OpenFast = 115,
	SR_Door_CloseFast = 116,
	DR_Door_RaiseFast = 117,
	D1_Door_OpenFast = 118,
	W1_Floor_RaiseToNearest = 119,
	WR_Plat_DownWaitUpStayLipTurbo = 120,
	W1_Plat_DownWaitUpStayLipTurbo = 121,
	S1_Plat_DownWaitUpStayLipTurbo = 122,
	SR_Plat_DownWaitUpStayLipTurbo = 123,
	W1_Exit_Secret = 124,
	M1_Teleport = 125,
	MR_Teleport = 126,
	S1_Stairs_BuildUpDoomTurbo = 127,
	WR_Floor_RaiseToNearest = 128,
	WR_Floor_RaiseToNearestFast = 129,
	W1_Floor_RaiseToNearestFast = 130,
	S1_Floor_RaiseToNearestFast = 131,
	SR_Floor_RaiseToNearestFast = 132,
	S1_Door_LockedRaiseFastBlue = 133,
	SR_Door_LockedRaiseFastRed = 134,
	S1_Door_LockedRaiseFastRed = 135,
	SR_Door_LockedRaiseFastYellow = 136,
	S1_Door_LockedRaiseFastYellow = 137,
	SR_Light_ChangeToValue255 = 138,
	SR_Light_ChangeToValue35 = 139,
	S1_Floor_RaiseByValueTimes8 = 140,
	W1_Ceiling_CrushAndRaiseSilentA = 141,
};

// Boom-specific linedef specials.
enum class boomLineSpecial_t : int16_t
{
	W1_Floor_RaiseByValueTimes8 = 142,
	W1_Plat_UpByValueStayTxBy24 = 143,
	W1_Plat_UpByValueStayTxBy32 = 144,
	W1_Ceiling_LowerToFloor = 145,
	W1_Floor_Donut = 146,
	WR_Floor_RaiseByValueTimes8 = 147,
	WR_Plat_UpByValueStayTxBy24 = 148,
	WR_Plat_UpByValueStayTxBy32 = 149,
	WR_Ceiling_CrushAndRaiseSilentA = 150,
	WR_FloorAndCeiling_LowerRaise = 151,
	WR_Ceiling_LowerToFloor = 152,
	W1_Floor_TransferTrigger = 153,
	WR_Floor_TransferTrigger = 154,
	WR_Floor_Donut = 155,
	WR_Light_StrobeDoom = 156,
	WR_Light_MinNeighbor = 157,
	S1_Floor_RaiseByTexture = 158,
	S1_Floor_LowerToLowestTxTy = 159,
	S1_Floor_RaiseByValueTxTy = 160,
	S1_Floor_RaiseByValue = 161,
	S1_Plat_PerpetualRaiseLip = 162,
	S1_Plat_Stop = 163,
	S1_Ceiling_CrushAndRaiseAFast = 164,
	S1_Ceiling_CrushAndRaiseSilentA = 165,
	S1_FloorAndCeiling_LowerRaise = 166,
	S1_Ceiling_LowerAndCrush = 167,
	S1_Ceiling_CrushStop = 168,
	S1_Light_MaxNeighbor = 169,
	S1_Light_ChangeToValue35 = 170,
	S1_Light_ChangeToValue255 = 171,
	S1_Light_StrobeDoom = 172,
	S1_Light_MinNeighbor = 173,
	S1_Teleport = 174,
	S1_Door_CloseWaitOpen = 175,
	SR_Floor_RaiseByTexture = 176,
	SR_Floor_LowerToLowestTxTy = 177,
	SR_Floor_RaiseByValueTimes8 = 178,
	SR_Floor_RaiseByValueTxTy = 179,
	SR_Floor_RaiseByValue = 180,
	SR_Plat_PerpetualRaiseLip = 181,
	SR_Plat_Stop = 182,
	SR_Ceiling_CrushAndRaiseAFast = 183,
	SR_Ceiling_CrushAndRaiseASlow = 184,
	SR_Ceiling_CrushAndRaiseSilentA = 185,
	SR_FloorAndCeiling_LowerRaise = 186,
	SR_Ceiling_LowerAndCrush = 187,
	SR_Ceiling_CrushStop = 188,
	S1_Floor_TransferTrigger = 189,
	SR_Floor_TransferTrigger = 190,
	SR_Floor_Donut = 191,
	SR_Light_MaxNeighbor = 192,
	SR_Light_StrobeDoom = 193,
	SR_Light_MinNeighbor = 194,
	SR_Teleport = 195,
	SR_Door_CloseWaitOpen = 196,
	G1_Exit_Normal = 197,
	G1_Exit_Secret = 198,
	W1_Ceiling_LowerToLowest = 199,
	W1_Ceiling_LowerToHighestFloor = 200,
	WR_Ceiling_LowerToLowest = 201,
	WR_Ceiling_LowerToHighestFloor = 202,
	S1_Ceiling_LowerToLowest = 203,
	S1_Ceiling_LowerToHighestFloor = 204,
	SR_Ceiling_LowerToLowest = 205,
	SR_Ceiling_LowerToHighestFloor = 206,
	W1_Teleport_NoFog = 207,
	WR_Teleport_NoFog = 208,
	S1_Teleport_NoFog = 209,
	SR_Teleport_NoFog = 210,
	SR_Plat_ToggleCeiling = 211,
	WR_Plat_ToggleCeiling = 212,
	ST_Transfer_FloorLight = 213,
	ST_ScrollCeilingAccelerative = 214,
	ST_ScrollFloorAccelerative = 215,
	ST_ScrollFloorAccelerativeCarry = 216,
	ST_ScrollFloorAccelerativeAndCarry = 217,
	ST_ScrollTextureModelAccelerative = 218,
	W1_Floor_LowerToNearest = 219,
	WR_Floor_LowerToNearest = 220,
	S1_Floor_LowerToNearest = 221,
	SR_Floor_LowerToNearest = 222,
	ST_Sector_SetFriction = 223,
	ST_Sector_SetWind = 224,
	ST_Sector_SetCurrent = 225,
	ST_PointPush_SetForce = 226,
	W1_Elevator_RaiseToNearest = 227,
	WR_Elevator_RaiseToNearest = 228,
	S1_Elevator_RaiseToNearest = 229,
	SR_Elevator_RaiseToNearest = 230,
	W1_Elevator_LowerToNearest = 231,
	WR_Elevator_LowerToNearest = 232,
	S1_Elevator_LowerToNearest = 233,
	SR_Elevator_LowerToNearest = 234,
	W1_Elevator_MoveToFloor = 235,
	WR_Elevator_MoveToFloor = 236,
	S1_Elevator_MoveToFloor = 237,
	SR_Elevator_MoveToFloor = 238,
	W1_Floor_TransferNumeric = 239,
	WR_Floor_TransferNumeric = 240,
	S1_Floor_TransferNumeric = 241,
	ST_Transfer_Heights = 242,
	W1_Teleport_Line = 243,
	WR_Teleport_Line = 244,
	ST_ScrollCeilingDisplacement = 245,
	ST_ScrollFloorDisplacement = 246,
	ST_ScrollFloorDisplacementCarry = 247,
	ST_ScrollFloorDisplacementAndCarry = 248,
	ST_ScrollTextureModelDisplacement = 249,
	ST_ScrollCeiling = 250,
	ST_ScrollFloor = 251,
	ST_ScrollFloorCarry = 252,
	ST_ScrollFloorAndCarry = 253,
	ST_ScrollTextureModel = 254,
	ST_Scroll_Texture_Offsets = 255,
	WR_Stairs_BuildUpDoom = 256,
	WR_Stairs_BuildUpDoomTurbo = 257,
	SR_Stairs_BuildUpDoom = 258,
	SR_Stairs_BuildUpDoomTurbo = 259,
	ST_TranslucentLine = 260,
	ST_Transfer_CeilingLight = 261,
	W1_Teleport_LineReversed = 262,
	WR_Teleport_LineReversed = 263,
	M1_Teleport_LineReversed = 264,
	MR_Teleport_LineReversed = 265,
	M1_Teleport_Line = 266,
	MR_Teleport_Line = 267,
	M1_Teleport_NoFog = 268,
	MR_Teleport_NoFog = 269,
	ST_TransferSky = 271,
	ST_TransferSkyFlipped = 272,
};

// A linedef type as a Doom or Boom format map stores it.
template <typename T>
concept DoomFormatLineSpecial =
    std::same_as<T, doomLineSpecial_t> || std::same_as<T, boomLineSpecial_t>;

// Linedef specials are stored as plain int16_t on line_t and bossaction_t.
constexpr auto lineSpecialValue(const DoomFormatLineSpecial auto special)
{
	return OUtil::to_underlying(special);
}

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
	unsigned int flags;
	short newspecial;
	short args[5];
} xlat_t;

struct line_s;
class AActor;

typedef bool (*lnSpecFunc)(struct line_s	*line,
						   class AActor		*activator,
						   int				arg1,
						   int				arg2,
						   int				arg3,
						   int				arg4,
						   int				arg5);

extern std::array<lnSpecFunc, 283> LineSpecials;

bool EV_CeilingCrushStop (int tag);
void EV_StopPlat (int tag);
bool EV_DoZDoomDonut(int tag, line_t* line, fixed_t pillarspeed, fixed_t slimespeed);
bool EV_ZDoomCeilingCrushStop(int tag, bool remove);

bool P_LineSpecialMovesSector(short special);
bool P_CanActivateSpecials(AActor* mo, line_t* line);
void P_DestroyScrollerThinkers();
void P_DestroyLightThinkers();

int P_FindLineFromLineTag(const line_t* line, int start);
int P_IsUnderDamage(const AActor* actor);
bool P_IsOnLift(const AActor* actor);
void EV_LightSetMinNeighbor(int tag);
void EV_LightSetMaxNeighbor(int tag);

extern int TeleportSide;
