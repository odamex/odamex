// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1997-2000 by id Software Inc.
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
//	Vector math routines which I took from Quake2's source since they
//	make more sense than the way Doom does things. :-)
//
// [SL] 2012-02-18 - Reworked all of the functions to be more uniform with
// the ordering of parameters, based in part on m_vectors.cpp from Eternity
// Engine.  Also changed from using an array of floats as the underlying type
// to using a struct.
//
//-----------------------------------------------------------------------------

#ifndef __M_VECTORS_H__
#define __M_VECTORS_H__

#include "m_fixed.h"
#include "tables.h"

class AActor;

template <typename TYPE>
struct Vec2
{
	Vec2<TYPE>()
	{
	}

	Vec2<TYPE>(TYPE x_, TYPE y_) : x(x_), y(y_)
	{
	}

	Vec2<TYPE>(const Vec2<TYPE>& other) : x(other.x), y(other.y)
	{
	}

	TYPE x, y;
};

/**
 * @brief A vector with three members.
 *
 * @tparam TYPE Type contained inside the
 */
template <typename TYPE>
struct Vec3
{
	Vec3<TYPE>()
	{
	}

	Vec3<TYPE>(TYPE x_, TYPE y_, TYPE z_) : x(x_), y(y_), z(z_)
	{
	}

	Vec3<TYPE>(const Vec3<TYPE>& other) : x(other.x), y(other.y), z(other.z)
	{
	}

	TYPE x, y, z;
};

//
// M_SetVec3f
//
// Sets the components of dest to the values of the parameters x, y, z
//
void M_SetVec3f(Vec3<float>& dest, float x, float y, float z);
void M_SetVec3f(Vec3<float>& dest, fixed_t x, fixed_t y, fixed_t z);
void M_SetVec3(Vec3<double>& dest, double x, double y, double z);
void M_SetVec3(Vec3<double>& dest, fixed_t x, fixed_t y, fixed_t z);
void M_SetVec2Fixed(Vec2<fixed_t>& dest, double x, double y);
void M_SetVec2Fixed(Vec2<fixed_t>& dest, fixed_t x, fixed_t y);
void M_SetVec3Fixed(Vec3<fixed_t>& dest, double x, double y, double z);
void M_SetVec3Fixed(Vec3<fixed_t>& dest, fixed_t x, fixed_t y, fixed_t z);

//
// M_ConvertVec3FixedToVec3f
//
// Converts the component values of src to floats and stores
// in dest
//
void M_ConvertVec3FixedToVec3f(Vec3<float>& dest, const Vec3<fixed_t>& src);
void M_ConvertVec3FixedToVec3(Vec3<double>& dest, const Vec3<fixed_t>& src);
void M_ConvertVec3fToVec3Fixed(Vec3<fixed_t>& dest, const Vec3<float>& src);
void M_ConvertVec3ToVec3Fixed(Vec3<fixed_t>& dest, const Vec3<double>& src);

//
// M_IsZeroVec3f
//
// Returns true if the v is the zero or null vector (all components are 0)
//
bool M_IsZeroVec3f(const Vec3<float>& v);
bool M_IsZeroVec3(const Vec3<double>& v);
bool M_IsZeroVec2Fixed(const Vec2<fixed_t>& v);
bool M_IsZeroVec3Fixed(const Vec3<fixed_t>& v);

//
// M_ZeroVec3f
//
// Sets all components of v to 0
//
void M_ZeroVec3f(Vec3<float>& v);
void M_ZeroVec3(Vec3<double>& v);
void M_ZeroVec2Fixed(Vec2<fixed_t>& v);
void M_ZeroVec3Fixed(Vec3<fixed_t>& v);

//
// M_AddVec3f
//
// Adds v2 to v1 stores in dest
// [SL] 2012-02-13 - Courtesy of Eternity Engine
//
void M_AddVec3f(Vec3<float>& dest, const Vec3<float>& v1, const Vec3<float>& v2);
void M_AddVec3(Vec3<double>& dest, const Vec3<double>& v1, const Vec3<double>& v2);
void M_AddVec2Fixed(Vec2<fixed_t>& dest, const Vec2<fixed_t>& v1,
                    const Vec2<fixed_t>& v2);
void M_AddVec3Fixed(Vec3<fixed_t>& dest, const Vec3<fixed_t>& v1,
                    const Vec3<fixed_t>& v2);

//
// M_SubVec3
//
// Subtracts v2 from v1 stores in dest
// [SL] 2012-02-13 - Courtesy of Eternity Engine
//
void M_SubVec3f(Vec3<float>& dest, const Vec3<float>& v1, const Vec3<float>& v2);
void M_SubVec3(Vec3<double>& dest, const Vec3<double>& v1, const Vec3<double>& v2);
void M_SubVec2Fixed(Vec2<fixed_t>& dest, const Vec2<fixed_t>& v1,
                    const Vec2<fixed_t>& v2);
void M_SubVec3Fixed(Vec3<fixed_t>& dest, const Vec3<fixed_t>& v1,
                    const Vec3<fixed_t>& v2);

void M_MulVec2f(Vec2<float>& dest, const Vec2<float>& v1, const Vec2<float>& v2);

//
// M_LengthVec3f
//
// Returns the length of a given vector (relative to the origin).  Taken from
// Quake 2, added by CG.
// [SL] 2012-02-13 - Courtesy of Eternity Engine
//
float M_LengthVec3f(const Vec3<float>& v);
double M_LengthVec3(const Vec3<double>& v);
fixed_t M_LengthVec2Fixed(const Vec2<fixed_t>& v);
fixed_t M_LengthVec3Fixed(const Vec3<fixed_t>& v);

//
// M_ScaleVec3f
//
// Multiplies each element in the vector by scalar value a
// and stores in dest
//
void M_ScaleVec3f(Vec3<float>& dest, const Vec3<float>& v, float a);
void M_ScaleVec3(Vec3<double>& dest, const Vec3<double>& v, double a);
void M_ScaleVec2Fixed(Vec2<fixed_t>& dest, const Vec2<fixed_t>& v, fixed_t a);
void M_ScaleVec3Fixed(Vec3<fixed_t>& dest, const Vec3<fixed_t>& v, fixed_t a);

//
// M_ScaleVec3fToLength
//
// Scales each element in the vector such that the vector length equals a
// and stores the resulting vector in dest.
//
void M_ScaleVec3fToLength(Vec3<float>& dest, const Vec3<float>& v, float a);
void M_ScaleVec3ToLength(Vec3<double>& dest, const Vec3<double>& v, double a);
void M_ScaleVec2FixedToLength(Vec2<fixed_t>& dest, const Vec2<fixed_t>& v, fixed_t a);
void M_ScaleVec3FixedToLength(Vec3<fixed_t>& dest, const Vec3<fixed_t>& v, fixed_t a);

//
// M_DotVec3
//
// Returns the dot product of v1 and v2
// [SL] 2012-02-13 - Courtesy of Eternity Engine
//
float M_DotProductVec3f(const Vec3<float>& v1, const Vec3<float>& v2);
double M_DotProductVec3(const Vec3<double>& v1, const Vec3<double>& v2);

//
// M_CrossProduct3f
//
// Gets the cross product of v1 and v2 and stores in dest
// [SL] 2012-02-13 - Courtesy of Eternity Engine
//
void M_CrossProductVec3f(Vec3<float>& dest, const Vec3<float>& v1, const Vec3<float>& v2);
void M_CrossProductVec3(Vec3<double>& dest, const Vec3<double>& v1,
                        const Vec3<double>& v2);

//
// M_NormalizeVec3f
//
// Scales v so that its length is 1.0 and stores in dest
//
void M_NormalizeVec3f(Vec3<float>& dest, const Vec3<float>& v);
void M_NormalizeVec3(Vec3<double>& dest, const Vec3<double>& v);
void M_NormalizeVec2Fixed(Vec2<fixed_t>& dest, const Vec2<fixed_t>& v);
void M_NormalizeVec3Fixed(Vec3<fixed_t>& dest, const Vec3<fixed_t>& v);

//
// M_ActorToVec3f
//
// Stores thing's position in the vector dest
//
void M_ActorPositionToVec3f(Vec3<float>& dest, const AActor* thing);
void M_ActorPositionToVec3(Vec3<double>& dest, const AActor* thing);
void M_ActorPositionToVec2Fixed(Vec2<fixed_t>& dest, const AActor* thing);
void M_ActorPositionToVec3Fixed(Vec3<fixed_t>& dest, const AActor* thing);

//
// M_ActorMomentumToVec3f
//
// Stores thing's momentum in the vector dest
//
void M_ActorMomentumToVec3f(Vec3<float>& dest, const AActor* thing);
void M_ActorMomentumToVec3(Vec3<double>& dest, const AActor* thing);
void M_ActorMomentumToVec2Fixed(Vec2<fixed_t>& dest, const AActor* thing);
void M_ActorMomentumToVec3Fixed(Vec3<fixed_t>& dest, const AActor* thing);

//
// M_AngleToVec3f
//
// Calculates the normalized direction vector from ang and pitch
//
void M_AngleToVec3f(Vec3<float>& dest, angle_t ang, int pitch);
void M_AngleToVec3(Vec3<double>& dest, angle_t ang, int pitch);

void M_ProjectPointOnPlane(Vec3<double>& dest, const Vec3<double>& p,
                           const Vec3<double>& normal);
void M_PerpendicularVec3(Vec3<double>& dest, const Vec3<double>& src);
void M_RotatePointAroundVector(Vec3<double>& dest, const Vec3<double>& dir,
                               const Vec3<double>& point, float degrees);

//
// M_TranslateVec3f
//
// Rotates a vector around a point of origin
//
void M_TranslateVec3f(Vec3<float>& vec, const Vec3<float>& origin, angle_t ang);
void M_TranslateVec3(Vec3<double>& vec, const Vec3<double>& origin, angle_t ang);

/**
 * Check if point is inside rectangle.
 *
 * The two points on the rectangle can be passed using any orientation.
 *
 * @param p Point to check.
 * @param a Origin point of rectangle.
 * @param b Opposite point of rectangle.
 */
template <typename T>
bool M_PointInRect(Vec2<T> p, Vec2<T> a, Vec2<T> b)
{
	const T minX = MIN(a.x, b.x);
	const T maxX = MAX(a.x, b.x);
	const T minY = MIN(a.y, b.y);
	const T maxY = MAX(a.y, b.y);

	if (p.x < minX || p.x > maxX)
	{
		return false;
	}
	if (p.y < minY || p.y > maxY)
	{
		return false;
	}

	return true;
}

#endif //__M_VECTORS_H__
