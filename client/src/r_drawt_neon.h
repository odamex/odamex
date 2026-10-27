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
//	NEON pieces that must inline into the drawers
//
//-----------------------------------------------------------------------------

#pragma once

#include "r_intrin.h"
#include "r_draw.h"
#include "r_main.h"

// The slope stepper needs a vector divide to match the scalar drawer, which
// AArch64 has and 32-bit ARM NEON does not.
#if defined(__ARM_NEON) && (defined(__aarch64__) || defined(_M_ARM64))

#define R_HAVE_NEON_SLOPES

#include <arm_neon.h>

//
// SlopeColumnNEON
//
// Steps a sloped plane column four rows at a time, one row per lane,
// giving each row's texel index and colormap offset.
//
// Every operation is the scalar drawer's, one rounding each: a multiply, then a
// separate add, and an IEEE divide. Lanes are independent, so a final partial
// group may be computed whole and its rows past the column's end ignored.
//
template<bool SLOPELIGHT>
class SlopeColumnNEON
{
public:
	explicit SlopeColumnNEON(const drawplanecol_t& d) :
			iu(vdupq_n_f32(d.iu)), iv(vdupq_n_f32(d.iv)), id(vdupq_n_f32(d.id)),
			ius(vdupq_n_f32(d.iustep)), ivs(vdupq_n_f32(d.ivstep)), ids(vdupq_n_f32(d.idstep)),
			umask(vdupq_n_s32(d.umask)), vmask(vdupq_n_s32(d.vmask)),
			ushift(vdupq_n_s32(-d.ushift)), vshift(vdupq_n_s32(-d.vshift))
	{
		static const float first[4] = { 0.0f, 1.0f, 2.0f, 3.0f };
		fi = vld1q_f32(first);

		if (SLOPELIGHT)
		{
			const uint32_t l = static_cast<uint32_t>(d.lmap), s = static_cast<uint32_t>(d.lmapstep);
			const int32_t lanes[4] = { static_cast<int32_t>(l), static_cast<int32_t>(l + s),
			                           static_cast<int32_t>(l + 2 * s), static_cast<int32_t>(l + 3 * s) };
			lmap = vld1q_s32(lanes);
			lstep4 = vdupq_n_s32(static_cast<int32_t>(4 * s));
			lbias = vdupq_n_s32(d.lbias);
		}
	}

	// The next four rows. lights is untouched without SLOPELIGHT.
	forceinline void next(uint32_t* spots, uint32_t* lights)
	{
		const float32x4_t depth = vaddq_f32(id, vmulq_f32(ids, fi));
		const float32x4_t mul = vdivq_f32(vdupq_n_f32(65536.0f), depth);
		const int32x4_t ufrac = vcvtq_s32_f32(vmulq_f32(vaddq_f32(iu, vmulq_f32(ius, fi)), mul));
		const int32x4_t vfrac = vcvtq_s32_f32(vmulq_f32(vaddq_f32(iv, vmulq_f32(ivs, fi)), mul));

		// A negative count is an arithmetic right shift
		vst1q_u32(spots, vreinterpretq_u32_s32(vorrq_s32(
		    vandq_s32(vshlq_s32(ufrac, ushift), umask),
		    vandq_s32(vshlq_s32(vfrac, vshift), vmask))));

		fi = vaddq_f32(fi, vdupq_n_f32(4.0f));

		if (SLOPELIGHT)
		{
			int32x4_t index = vaddq_s32(vshrq_n_s32(lmap, FRACBITS), lbias);
			index = vminq_s32(vmaxq_s32(index, vdupq_n_s32(0)), vdupq_n_s32(NUMCOLORMAPS - 1));

			vst1q_u32(lights, vreinterpretq_u32_s32(vshlq_n_s32(index, 8)));
			lmap = vaddq_s32(lmap, lstep4);
		}
	}

private:
	const float32x4_t iu;
	const float32x4_t iv;
	const float32x4_t id;

	const float32x4_t ius;
	const float32x4_t ivs;
	const float32x4_t ids;

	float32x4_t fi;

	const int32x4_t umask;
	const int32x4_t vmask;
	const int32x4_t ushift;
	const int32x4_t vshift;

	int32x4_t lmap;
	int32x4_t lstep4;
	int32x4_t lbias;
};

#endif // NEON on AArch64
