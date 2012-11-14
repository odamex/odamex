// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1997-2000 by id Software Inc.
// Copyright (C) 1998-2006 by Randy Heit (ZDoom).
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
//	Vector math routines which I took from Quake2's source since they
//	make more sense than the way Doom does things. :-)
//
// [SL] 2012-02-18 - Reworked all of the functions to be more uniform with
// the ordering of parameters, based in part on m_vectors.cpp from Eternity
// Engine.  Also changed from using an array of floats as the underlying type
// to using a struct.
//-----------------------------------------------------------------------------


#include <stdio.h>

#include "vectors.h"
#include "actor.h"
#include "tables.h"

#include <math.h>

#ifndef M_PI
#define M_PI		3.14159265358979323846	// matches value in gcc v2 math.h
#endif

#define DEG2RAD( a ) ( a * M_PI ) / 180.0F

//
// M_SetVec3f
//
// Sets the components of dest to the values of the parameters x, y, z
//
void M_SetVec3f(v3float_t *dest, float x, float y, float z)
{
	dest->x = x;
	dest->y = y;
	dest->z = z;
}

void M_SetVec3f(v3float_t *dest, fixed_t x, fixed_t y, fixed_t z)
{
	dest->x = FIXED2FLOAT(x);
	dest->y = FIXED2FLOAT(y);
	dest->z = FIXED2FLOAT(z);
}		

void M_SetVec3(v3double_t *dest, double x, double y, double z)
{
	dest->x = x;
	dest->y = y;
	dest->z = z;
}

void M_SetVec3(v3double_t *dest, fixed_t x, fixed_t y, fixed_t z)
{
	dest->x = FIXED2DOUBLE(x);
	dest->y = FIXED2DOUBLE(y);	
	dest->z = FIXED2DOUBLE(z);
}

void M_SetVec2Fixed(v2fixed_t *dest, double x, double y)
{
	dest->x = DOUBLE2FIXED(x); 
	dest->y = DOUBLE2FIXED(y);
}

void M_SetVec2Fixed(v2fixed_t *dest, fixed_t x, fixed_t y)
{
	dest->x = x;
	dest->y = y;
}

void M_SetVec3Fixed(v3fixed_t *dest, double x, double y, double z)
{
	dest->x = DOUBLE2FIXED(x);
	dest->y = DOUBLE2FIXED(y);
	dest->z = DOUBLE2FIXED(z);
}

void M_SetVec3Fixed(v3fixed_t *dest, fixed_t x, fixed_t y, fixed_t z)
{
	dest->x = x;
	dest->y = y;
	dest->z = z;
}


//
// M_ConvertVec3FixedToVec3f
//
// Converts the component values of src to floats and stores
// in dest
//
void M_ConvertVec3FixedToVec3f(v3float_t *dest, const v3fixed_t *src)
{
	M_SetVec3f(dest, src->x, src->y, src->z);
}

void M_ConvertVec3FixedToVec3(v3double_t *dest, const v3fixed_t *src)
{
	M_SetVec3(dest, src->x, src->y, src->z);
}

void M_ConvertVec3fToVec3Fixed(v3fixed_t *dest, const v3float_t *src)
{
	M_SetVec3Fixed(dest, double(src->x), double(src->y), double(src->z));
}

void M_ConvertVec3ToVec3Fixed(v3fixed_t *dest, const v3double_t *src)
{
	M_SetVec3Fixed(dest, src->x, src->y, src->z);
}


//
// M_IsZeroVec3f
//
// Returns true if the v is the zero or null vector (all components are 0)
//
bool M_IsZeroVec3f(const v3float_t *v)
{
	return fabs(v->x) == 0.0f && fabs(v->y) == 0.0f && fabs(v->z) == 0.0f;
}

bool M_IsZeroVec3(const v3double_t *v)
{
	return fabs(v->x) == 0.0 && fabs(v->y) == 0.0 && fabs(v->z) == 0.0;
}

bool M_IsZeroVec2Fixed(const v2fixed_t *v)
{
	return v->x == 0 && v->y == 0;
}

bool M_IsZeroVec3Fixed(const v3fixed_t *v)
{
	return v->x == 0 && v->y == 0 && v->z == 0;
}


//
// M_ZeroVec3f
//
// Sets all components of v to 0
//
void M_ZeroVec3f(v3float_t *v)
{
	v->x = v->y = v->z = 0.0f;
}

void M_ZeroVec3(v3double_t *v)
{
	v->x = v->y = v->z = 0.0;
}

void M_ZeroVec2Fixed(v2fixed_t *v)
{
	v->x = v->y = 0;
}

void M_ZeroVec3Fixed(v3fixed_t *v)
{
	v->x = v->y = v->z = 0;
}


//
// M_AddVec3f
//
// Adds v2 to v1 stores in dest
//
void M_AddVec3f(v3float_t *dest, const v3float_t *v1, const v3float_t *v2)
{
	dest->x = v1->x + v2->x;
	dest->y = v1->y + v2->y;
	dest->z = v1->z + v2->z;
}


void M_AddVec3(v3double_t *dest, const v3double_t *v1, const v3double_t *v2)
{
	dest->x = v1->x + v2->x;
	dest->y = v1->y + v2->y;
	dest->z = v1->z + v2->z;
}

void M_AddVec2Fixed(v2fixed_t *dest, const v2fixed_t *v1, const v2fixed_t *v2)
{
	dest->x = v1->x + v2->x;
	dest->y = v1->y + v2->y;
}

void M_AddVec3Fixed(v3fixed_t *dest, const v3fixed_t *v1, const v3fixed_t *v2)
{
	dest->x = v1->x + v2->x;
	dest->y = v1->y + v2->y;
	dest->z = v1->z + v2->z;
}


// 
// M_SubVec3f
//
// Subtracts v2 from v1 stores in dest
//
void M_SubVec3f(v3float_t *dest, const v3float_t *v1, const v3float_t *v2)
{
	dest->x = v1->x - v2->x;
	dest->y = v1->y - v2->y;
	dest->z = v1->z - v2->z;
}

void M_SubVec3(v3double_t *dest, const v3double_t *v1, const v3double_t *v2)
{
	dest->x = v1->x - v2->x;
	dest->y = v1->y - v2->y;
	dest->z = v1->z - v2->z;
}

void M_SubVec2Fixed(v2fixed_t *dest, const v2fixed_t *v1, const v2fixed_t *v2)
{
	dest->x = v1->x - v2->x;
	dest->y = v1->y - v2->y;
}

void M_SubVec3Fixed(v3fixed_t *dest, const v3fixed_t *v1, const v3fixed_t *v2)
{
	dest->x = v1->x - v2->x;
	dest->y = v1->y - v2->y;
	dest->z = v1->z - v2->z;
}


//
// M_LengthVec3f
//
// Returns the length of a given vector (relative to the origin).  Taken from
// Quake 2, added by CG.
//
float M_LengthVec3f(const v3float_t *v)
{
	return sqrtf(v->x * v->x + v->y * v->y + v->z * v->z);
}

double M_LengthVec3(const v3double_t *v)
{
	return sqrt(v->x * v->x + v->y * v->y + v->z * v->z);
}

fixed_t M_LengthVec2Fixed(const v2fixed_t *v)
{
	double fx = FIXED2DOUBLE(v->x);
	double fy = FIXED2DOUBLE(v->y);
	
	return DOUBLE2FIXED(sqrt(fx * fx + fy * fy));
}

fixed_t M_LengthVec3Fixed(const v3fixed_t *v)
{
	double fx = FIXED2DOUBLE(v->x);
	double fy = FIXED2DOUBLE(v->y);
	double fz = FIXED2DOUBLE(v->z);
	
	return DOUBLE2FIXED(sqrt(fx * fx + fy * fy + fz * fz));
}


//
// M_ScaleVec3f
//
// Multiplies each element in the vector by scalar value a
// and stores in dest
//
void M_ScaleVec3f(v3float_t *dest, const v3float_t *v1, float a)
{
	dest->x = v1->x * a;
	dest->y = v1->y * a;
	dest->z = v1->z * a;
}

void M_ScaleVec3(v3double_t *dest, const v3double_t *v1, double a)
{
	dest->x = v1->x * a;
	dest->y = v1->y * a;
	dest->z = v1->z * a;
}

void M_ScaleVec2Fixed(v2fixed_t *dest, const v2fixed_t *v1, fixed_t a)
{
	dest->x = FixedMul(v1->x, a);
	dest->y = FixedMul(v1->y, a);
}

void M_ScaleVec3Fixed(v3fixed_t *dest, const v3fixed_t *v1, fixed_t a)
{
	dest->x = FixedMul(v1->x, a);
	dest->y = FixedMul(v1->y, a);
	dest->z = FixedMul(v1->z, a);
}


// 
// M_DotVec3f
//
// Returns the dot product of v1 and v2
//
float M_DotProductVec3f(const v3float_t *v1, const v3float_t *v2)
{
	return (v1->x * v2->x) + (v1->y * v2->y) + (v1->z * v2->z);
}

double M_DotProductVec3(const v3double_t *v1, const v3double_t *v2)
{
	return (v1->x * v2->x) + (v1->y * v2->y) + (v1->z * v2->z);
}


//
// M_CrossProduct3f
//
// Gets the cross product of v1 and v2 and stores in dest 
//
void M_CrossProductVec3f(v3float_t *dest, const v3float_t *v1, const v3float_t *v2)
{
	dest->x = (v1->y * v2->z) - (v1->z * v2->y);
	dest->y = (v1->z * v2->x) - (v1->x * v2->z);
	dest->z = (v1->x * v2->y) - (v1->y * v2->x);
}

void M_CrossProductVec3(v3double_t *dest, const v3double_t *v1, const v3double_t *v2)
{
	dest->x = (v1->y * v2->z) - (v1->z * v2->y);
	dest->y = (v1->z * v2->x) - (v1->x * v2->z);
	dest->z = (v1->x * v2->y) - (v1->y * v2->x);
}


//
// M_NormalizeVec3f
//
// Scales v so that its length is 1.0 and stores in dest 
//
void M_NormalizeVec3f(v3float_t *dest, const v3float_t *v)
{
	float length = M_LengthVec3f(v);
	
	if (length > 0.0f)
		M_ScaleVec3f(dest, v, 1.0f / length);
}

void M_NormalizeVec3(v3double_t *dest, const v3double_t *v)
{
	double length = M_LengthVec3(v);
	
	if (length > 0.0)
		M_ScaleVec3(dest, v, 1.0 / length);
}


//
// M_ActorToVec3f
//
// Stores thing's position in the vector dest
//
void M_ActorPositionToVec3f(v3float_t *dest, const AActor *thing)
{
	dest->x = FIXED2FLOAT(thing->x);
	dest->y = FIXED2FLOAT(thing->y);
	dest->z = FIXED2FLOAT(thing->z);
}

void M_ActorPositionToVec3(v3double_t *dest, const AActor *thing)
{
	dest->x = FIXED2DOUBLE(thing->x);
	dest->y = FIXED2DOUBLE(thing->y);
	dest->z = FIXED2DOUBLE(thing->z);
}

void M_ActorPositionToVec2Fixed(v2fixed_t *dest, const AActor *thing)
{
	dest->x = thing->x;
	dest->y = thing->y;
}

void M_ActorPositionToVec3Fixed(v3fixed_t *dest, const AActor *thing)
{
	dest->x = thing->x;
	dest->y = thing->y;
	dest->z = thing->z;
}


//
// M_ActorMomentumToVec3f
//
// Stores thing's momentum in the vector dest
//
void M_ActorMomentumToVec3f(v3float_t *dest, const AActor *thing)
{
	dest->x = FIXED2FLOAT(thing->momx);
	dest->y = FIXED2FLOAT(thing->momy);
	dest->z = FIXED2FLOAT(thing->momz);
}

void M_ActorMomentumToVec3(v3double_t *dest, const AActor *thing)
{
	dest->x = FIXED2DOUBLE(thing->momx);
	dest->y = FIXED2DOUBLE(thing->momy);
	dest->z = FIXED2DOUBLE(thing->momz);
}

void M_ActorMomentumToVec2Fixed(v2fixed_t *dest, const AActor *thing)
{
	dest->x = thing->momx;
	dest->y = thing->momy;
}

void M_ActorMomentumToVec3Fixed(v3fixed_t *dest, const AActor *thing)
{
	dest->x = thing->momx;
	dest->y = thing->momy;
	dest->z = thing->momz;
}


//
// M_AngleToVec3f
//
// Calculates the normalized direction vector from ang and pitch
//
void M_AngleToVec3f(v3float_t *dest, angle_t ang, int pitch)
{
	dest->x = FIXED2FLOAT(finecosine[ang >> ANGLETOFINESHIFT]);
	dest->y = FIXED2FLOAT(finesine[ang >> ANGLETOFINESHIFT]);
	dest->z = FIXED2FLOAT(finetangent[FINEANGLES/4 - (pitch >> ANGLETOFINESHIFT)]);
	M_NormalizeVec3f(dest, dest);
}
	
void M_AngleToVec3(v3double_t *dest, angle_t ang, int pitch)
{
	dest->x = FIXED2DOUBLE(finecosine[ang >> ANGLETOFINESHIFT]);
	dest->y = FIXED2DOUBLE(finesine[ang >> ANGLETOFINESHIFT]) ;
	dest->z = FIXED2DOUBLE(finetangent[FINEANGLES/4 - (pitch >> ANGLETOFINESHIFT)]);
	M_NormalizeVec3(dest, dest);
}

	
//
// M_ProjectPointOnPlane
//
// 
//
void M_ProjectPointOnPlane(v3double_t *dest, const v3double_t *p, const v3double_t *normal)
{
	if (M_IsZeroVec3(normal))
	{
		// Assume that a normal of zero length is bad input and bail
		M_ZeroVec3(dest);
		return;
	}
	
	double inv_denom = 1.0 / M_DotProductVec3(normal, normal);
	double d = M_DotProductVec3(normal, p) * inv_denom;

	v3double_t n;
	M_ScaleVec3(&n, normal, inv_denom * d);
	M_SubVec3(dest, p, &n);
}
	
//
// M_PerpendicularVec3
//
// Assumes that src is a normalized vector
//
void M_PerpendicularVec3(v3double_t *dest, const v3double_t *src)
{
	// find the smallest component of the vector src
	v3double_t tempvec;
	double minelem = src->x;
	double *mincomponent = &(tempvec.x);
	if (abs(src->y) < minelem)
	{
		minelem = abs(src->y);
		mincomponent = &(tempvec.y);
	}
	if (abs(src->z) < minelem)
	{
		minelem = abs(src->z);
		mincomponent = &(tempvec.z);
	}
	
	// make tempvec the identity vector along the axis of the smallest component
	M_ZeroVec3(&tempvec);
	*mincomponent = 1.0;
	
	M_ProjectPointOnPlane(dest, &tempvec, src);
	M_NormalizeVec3(dest, dest);
}	


static void M_ConcatRotations(double out[3][3], const double in1[3][3], const double in2[3][3])
{
	out[0][0] = in1[0][0] * in2[0][0] + in1[0][1] * in2[1][0] +
				in1[0][2] * in2[2][0];
	out[0][1] = in1[0][0] * in2[0][1] + in1[0][1] * in2[1][1] +
				in1[0][2] * in2[2][1];
	out[0][2] = in1[0][0] * in2[0][2] + in1[0][1] * in2[1][2] +
				in1[0][2] * in2[2][2];
	out[1][0] = in1[1][0] * in2[0][0] + in1[1][1] * in2[1][0] +
				in1[1][2] * in2[2][0];
	out[1][1] = in1[1][0] * in2[0][1] + in1[1][1] * in2[1][1] +
				in1[1][2] * in2[2][1];
	out[1][2] = in1[1][0] * in2[0][2] + in1[1][1] * in2[1][2] +
				in1[1][2] * in2[2][2];
	out[2][0] = in1[2][0] * in2[0][0] + in1[2][1] * in2[1][0] +
				in1[2][2] * in2[2][0];
	out[2][1] = in1[2][0] * in2[0][1] + in1[2][1] * in2[1][1] +
				in1[2][2] * in2[2][1];
	out[2][2] = in1[2][0] * in2[0][2] + in1[2][1] * in2[1][2] +
				in1[2][2] * in2[2][2];
}

#ifdef _MSC_VER
#pragma optimize( "", off )
#endif
	
void M_RotatePointAroundVector(v3double_t *dest, const v3double_t *dir, const v3double_t *point, float degrees)
{

	double	m[3][3], im[3][3], zrot[3][3], tmpmat[3][3], rot[3][3];
	v3double_t vr, vup, vf;

	vf.x = dir->x;
	vf.y = dir->y;
	vf.z = dir->z;

	M_PerpendicularVec3(&vr, dir);
	M_CrossProductVec3(&vup, &vr, &vf);

	m[0][0] = vr.x;
	m[1][0] = vr.y;
	m[2][0] = vr.z;

	m[0][1] = vup.x;
	m[1][1] = vup.y;
	m[2][1] = vup.z;

	m[0][2] = vf.x;
	m[1][2] = vf.y;
	m[2][2] = vf.z;

	memcpy( im, m, sizeof( im ) );

	im[0][1] = m[1][0];
	im[0][2] = m[2][0];
	im[1][0] = m[0][1];
	im[1][2] = m[2][1];
	im[2][0] = m[0][2];
	im[2][1] = m[1][2];

	memset( zrot, 0, sizeof( zrot ) );
	zrot[0][0] = zrot[1][1] = zrot[2][2] = 1.0;

	zrot[0][0] = (float)cos( DEG2RAD( degrees ) );
	zrot[0][1] = (float)sin( DEG2RAD( degrees ) );
	zrot[1][0] = (float)-sin( DEG2RAD( degrees ) );
	zrot[1][1] = (float)cos( DEG2RAD( degrees ) );

	M_ConcatRotations(tmpmat, m, zrot);
	M_ConcatRotations(rot, tmpmat, im);

	dest->x = rot[0][0] * point->x + rot[0][1] * point->y + rot[0][2] * point->z;
	dest->y = rot[1][0] * point->x + rot[1][1] * point->y + rot[1][2] * point->z;
	dest->z = rot[2][0] * point->x + rot[2][1] * point->y + rot[2][2] * point->z;
}

#ifdef _MSC_VER
#pragma optimize( "", on )
#endif

// 
// M_TranslateVec3f
//
// Translates the given vector (in doom's coordinate system) to the camera
// space (in right-handed coordinate system) This function is used for slopes.
// 
void M_TranslateVec3f(v3float_t *vec, const v3float_t *origin, angle_t ang)
{
	float tx, ty, tz;

	float viewcosf = FIXED2FLOAT(finecosine[ang >> ANGLETOFINESHIFT]);
	float viewsinf = FIXED2FLOAT(finesine[ang >> ANGLETOFINESHIFT]);
   
	tx = vec->x - origin->x;
	ty = origin->z - vec->y;
	tz = vec->z - origin->y;

	vec->x = (tx * viewcosf) - (tz * viewsinf);
	vec->z = (tz * viewcosf) + (tx * viewsinf);
	vec->y = ty;
}

void M_TranslateVec3 (v3double_t *vec, const v3double_t *origin, angle_t ang)
{
	double tx, ty, tz;

	double viewcosf = FIXED2DOUBLE(finecosine[ang >> ANGLETOFINESHIFT]);
	double viewsinf = FIXED2DOUBLE(finesine[ang >> ANGLETOFINESHIFT]);
 	  
	tx = vec->x - origin->x;
	ty = origin->z - vec->y;
	tz = vec->z - origin->y;

	vec->x = (tx * viewcosf) - (tz * viewsinf);
	vec->z = (tz * viewcosf) + (tx * viewsinf);
	vec->y = ty;
}


VERSION_CONTROL (vectors_cpp, "$Id$")

