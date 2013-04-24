// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1997-2000 by id Software Inc.
// Copyright (C) 1998-2006 by Randy Heit (ZDoom).
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
//	Vector math routines which I took from Quake2's source since they
//	make more sense than the way Doom does things. :-)
//
// [SL] 2012-02-18 - Reworked all of the functions to be more uniform with
// the ordering of parameters, based in part on m_vectors.cpp from Eternity
// Engine.  Also changed from using an array of floats as the underlying type
// to using a struct.
//
//-----------------------------------------------------------------------------


#ifndef __VECTORS_H__
#define __VECTORS_H__

#include "tables.h"

typedef float	vec_t;
typedef vec_t	vec3_t[3];

class AActor;

void VectorPosition (const class AActor *thing, vec3_t out);
void FixedAngleToVector (angle_t an, fixed_t pitch, vec3_t v);
vec_t VectorLength (const vec3_t v);
vec_t VectorNormalize (vec3_t v);
vec_t VectorNormalize2 (const vec3_t v, vec3_t out);

struct v2fixed_t
{
   fixed_t x, y;
};

struct v3fixed_t
{
   fixed_t x, y, z;
};

struct v3float_t
{
   float x, y, z;
};

struct v3double_t
{
   double x, y, z;
};


//
// M_SetVec3f
//
// Sets the components of dest to the values of the parameters x, y, z
//
void M_SetVec3f(v3float_t *dest, float x, float y, float z);
void M_SetVec3f(v3float_t *dest, fixed_t x, fixed_t y, fixed_t z);
void M_SetVec3(v3double_t *dest, double x, double y, double z);
void M_SetVec3(v3double_t *dest, fixed_t x, fixed_t y, fixed_t z);
void M_SetVec2Fixed(v2fixed_t *dest, double x, double y);
void M_SetVec2Fixed(v2fixed_t *dest, fixed_t x, fixed_t y);
void M_SetVec3Fixed(v3fixed_t *dest, double x, double y, double z);
void M_SetVec3Fixed(v3fixed_t *dest, fixed_t x, fixed_t y, fixed_t z);

//
//M_ConvertVec3FixedToVec3f
//
// Converts the component values of src to floats and stores
// in dest
//
void M_ConvertVec3FixedToVec3f(v3float_t *dest, const v3fixed_t *src);
void M_ConvertVec3FixedToVec3(v3double_t *dest, const v3fixed_t *src);
void M_ConvertVec3fToVec3Fixed(v3fixed_t *dest, const v3float_t *src);
void M_ConvertVec3ToVec3Fixed(v3fixed_t *dest, const v3double_t *src);

//
// M_IsZeroVec3f
//
// Returns true if the v is the zero or null vector (all components are 0)
//
bool M_IsZeroVec3f(const v3float_t *v);
bool M_IsZeroVec3(const v3double_t *v);
bool M_IsZeroVec2Fixed(const v2fixed_t *v);
bool M_IsZeroVec3Fixed(const v3fixed_t *v);

//
// M_ZeroVec3f
//
// Sets all components of v to 0
//
void M_ZeroVec3f(v3float_t *v);
void M_ZeroVec3(v3double_t *v);
void M_ZeroVec2Fixed(v2fixed_t *v);
void M_ZeroVec3Fixed(v3fixed_t *v);

//
// M_AddVec3f
//
// Adds v2 to v1 stores in dest
// [SL] 2012-02-13 - Courtesy of Eternity Engine
//
void M_AddVec3f(v3float_t *dest, const v3float_t *v1, const v3float_t *v2);
void M_AddVec3 (v3double_t *dest, const v3double_t *v1, const v3double_t *v2);
void M_AddVec2Fixed(v2fixed_t *dest, const v2fixed_t *v1, const v2fixed_t *v2);
void M_AddVec3Fixed(v3fixed_t *dest, const v3fixed_t *v1, const v3fixed_t *v2);

// 
// M_SubVec3
//
// Subtracts v2 from v1 stores in dest
// [SL] 2012-02-13 - Courtesy of Eternity Engine
//
void M_SubVec3f(v3float_t *dest,  const v3float_t *v1,  const v3float_t *v2);
void M_SubVec3 (v3double_t *dest, const v3double_t *v1, const v3double_t *v2);
void M_SubVec2Fixed(v2fixed_t *dest, const v2fixed_t *v1, const v2fixed_t *v2);
void M_SubVec3Fixed(v3fixed_t *dest, const v3fixed_t *v1, const v3fixed_t *v2);

//
// M_LengthVec3f
//
// Returns the length of a given vector (relative to the origin).  Taken from
// Quake 2, added by CG.
// [SL] 2012-02-13 - Courtesy of Eternity Engine
//
float M_LengthVec3f(const v3float_t *v);
double M_LengthVec3(const v3double_t *v);
fixed_t M_LengthVec2Fixed(const v2fixed_t *v);
fixed_t M_LengthVec3Fixed(const v3fixed_t *v);

//
// M_ScaleVec3f
//
// Multiplies each element in the vector by scalar value a
// and stores in dest
//
void M_ScaleVec3f(v3float_t *dest, const v3float_t *v1, float a);
void M_ScaleVec3 (v3double_t *dest, const v3double_t *v1, double a);
void M_ScaleVec2Fixed(v2fixed_t *dest, const v2fixed_t *v1, fixed_t a);
void M_ScaleVec3Fixed(v3fixed_t *dest, const v3fixed_t *v1, fixed_t a);

// 
// M_DotVec3
//
// Returns the dot product of v1 and v2
// [SL] 2012-02-13 - Courtesy of Eternity Engine
//
float  M_DotProductVec3f(const v3float_t *v1,  const v3float_t *v2);
double M_DotProductVec3 (const v3double_t *v1, const v3double_t *v2);

//
// M_CrossProduct3f
//
// Gets the cross product of v1 and v2 and stores in dest
// [SL] 2012-02-13 - Courtesy of Eternity Engine
//
void M_CrossProductVec3f(v3float_t *dest,  const v3float_t *v1,  const v3float_t *v2);
void M_CrossProductVec3 (v3double_t *dest, const v3double_t *v1, const v3double_t *v2);

//
// M_NormalizeVec3f
//
// Scales v so that its length is 1.0 and stores in dest 
//
void M_NormalizeVec3f(v3float_t *dest, const v3float_t *v);
void M_NormalizeVec3 (v3double_t *dest, const v3double_t *v);

//
// M_ActorToVec3f
//
// Stores thing's position in the vector dest
//
void M_ActorPositionToVec3f(v3float_t *dest, const AActor *thing);
void M_ActorPositionToVec3 (v3double_t *dest, const AActor *thing);
void M_ActorPositionToVec2Fixed(v2fixed_t *dest, const AActor *thing);
void M_ActorPositionToVec3Fixed(v3fixed_t *dest, const AActor *thing);

//
// M_ActorMomentumToVec3f
//
// Stores thing's momentum in the vector dest
//
void M_ActorMomentumToVec3f(v3float_t *dest, const AActor *thing);
void M_ActorMomentumToVec3 (v3double_t *dest, const AActor *thing);
void M_ActorMomentumToVec2Fixed(v2fixed_t *dest, const AActor *thing);
void M_ActorMomentumToVec3Fixed(v3fixed_t *dest, const AActor *thing);

//
// M_AngleToVec3f
//
// Calculates the normalized direction vector from ang and pitch
//
void M_AngleToVec3f(v3float_t *dest, angle_t ang, int pitch);
void M_AngleToVec3(v3double_t *dest, angle_t ang, int pitch);

void M_ProjectPointOnPlane(v3double_t *dest, const v3double_t *p, const v3double_t *normal);
void M_PerpendicularVec3(v3double_t *dest, const v3double_t *src);
void M_RotatePointAroundVector(v3double_t *dest, const v3double_t *dir, const v3double_t *point, float degrees);


//
// M_TranslateVec3f
//
// Rotates a vector around a point of origin
//
void M_TranslateVec3f(v3float_t *vec, const v3float_t *origin, angle_t ang);
void M_TranslateVec3(v3double_t *vec, const v3double_t *origin, angle_t ang);

#endif //__VECTORS_H__

