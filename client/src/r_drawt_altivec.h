// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
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
//	Altivec pieces that must inline into the drawers
//
//-----------------------------------------------------------------------------

#pragma once

#include "r_intrin.h"
#include "r_draw.h"
#include "r_main.h"

#ifdef __ALTIVEC__

#include <altivec.h>
#undef pixel
#undef bool

//
// SlopeColumnAltivec
//
// SlopeColumnSSE2 for Altivec: steps a sloped plane column four rows at a time,
// one row per lane, giving each row's texel index and colormap offset.
//
template<bool SLOPELIGHT>
class SlopeColumnAltivec
{
	typedef __vector float vf32;
	typedef __vector signed int vs32;
	typedef __vector unsigned int vu32;

public:
	explicit SlopeColumnAltivec(const drawplanecol_t& d)
	{
		iu = splat(d.iu); iv = splat(d.iv); id = splat(d.id);
		ius = splat(d.iustep); ivs = splat(d.ivstep); ids = splat(d.idstep);
		fi = (vf32){ 0.0f, 1.0f, 2.0f, 3.0f };

		umask = splat(d.umask); vmask = splat(d.vmask);
		ushift = splat(static_cast<unsigned int>(d.ushift));
		vshift = splat(static_cast<unsigned int>(d.vshift));

		if (SLOPELIGHT)
		{
			const uint32_t l = static_cast<uint32_t>(d.lmap), s = static_cast<uint32_t>(d.lmapstep);
			lmap = (vs32){ static_cast<int32_t>(l), static_cast<int32_t>(l + s),
					static_cast<int32_t>(l + 2 * s), static_cast<int32_t>(l + 3 * s) };
			lstep4 = splat(static_cast<int>(4 * s));
			lbias = splat(d.lbias);
		}
	}

	// The next four rows. lights is untouched without SLOPELIGHT. Both must be
	// 16-byte aligned: vec_st ignores the low four bits of the address.
	forceinline void next(uint32_t* spots, uint32_t* lights)
	{
		const vf32 zero = splat(-0.0f);		// vec_madd(a, b, -0) is a plain a * b
		const vf32 one = splat(1.0f);

		const vf32 numerator = splat(65536.0f);

		// 1 / (id + ids * fi): estimate, then r += r * (1 - depth * r) twice.
		const vf32 depth = vec_add(id, vec_madd(ids, fi, zero));
		vf32 r = vec_re(depth);
		r = vec_madd(r, vec_nmsub(depth, r, one), r);
		r = vec_madd(r, vec_nmsub(depth, r, one), r);

		// 65536 * r, then corrected by the exact remainder 65536 - depth * q.
		vf32 mul = vec_madd(numerator, r, zero);
		mul = vec_madd(vec_nmsub(depth, mul, numerator), r, mul);

		const vs32 ufrac = vec_cts(vec_madd(vec_add(iu, vec_madd(ius, fi, zero)), mul, zero), 0);
		const vs32 vfrac = vec_cts(vec_madd(vec_add(iv, vec_madd(ivs, fi, zero)), mul, zero), 0);

		vec_st(vec_or(
				vec_and(vec_sra(ufrac, ushift), umask),
				vec_and(vec_sra(vfrac, vshift), vmask)), 0, reinterpret_cast<int*>(spots));

		fi = vec_add(fi, splat(4.0f));

		if (SLOPELIGHT)
		{
			vs32 index = vec_add(vec_sra(lmap, splat(static_cast<unsigned int>(FRACBITS))), lbias);
			index = vec_min(vec_max(index, splat(0)), splat(NUMCOLORMAPS - 1));

			vec_st(vec_sl(index, splat(8u)), 0, reinterpret_cast<int*>(lights));
			lmap = vec_add(lmap, lstep4);
		}
	}

private:
	// Splats built from initializers: vec_splats is missing from older
	// compilers, and vec_splat_* only take five-bit immediates.
	static inline vf32 splat(float x) { return (vf32){ x, x, x, x }; }
	static inline vs32 splat(int x) { return (vs32){ x, x, x, x }; }
	static inline vu32 splat(unsigned int x) { return (vu32){ x, x, x, x }; }

	vf32 iu, iv, id;
	vf32 ius, ivs, ids;
	vf32 fi;

	vs32 umask, vmask;
	vu32 ushift, vshift;

	vs32 lmap, lstep4, lbias;
};

#endif // __ALTIVEC__
