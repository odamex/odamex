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
//	SSE2 pieces that must inline into the drawers
//
//-----------------------------------------------------------------------------

#pragma once

#include "r_intrin.h"
#include "r_draw.h"
#include "r_main.h"

#ifdef __SSE2__

#include <emmintrin.h>

//
// SlopeColumnSSE2
//
// Steps a sloped plane column four rows at a time, one row per lane, giving
// each row's texel index and colormap offset.
//
// Lanes are independent, so a final partial group may be computed whole and its
// rows past the column's end ignored.
//
template<bool SLOPELIGHT>
class SlopeColumnSSE2
{
public:
	explicit SlopeColumnSSE2(const drawplanecol_t& d) :
			iu(_mm_set1_ps(d.iu)), iv(_mm_set1_ps(d.iv)), id(_mm_set1_ps(d.id)),
			ius(_mm_set1_ps(d.iustep)), ivs(_mm_set1_ps(d.ivstep)), ids(_mm_set1_ps(d.idstep)),
			fi(_mm_setr_ps(0.0f, 1.0f, 2.0f, 3.0f)),
			umask(_mm_set1_epi32(d.umask)), vmask(_mm_set1_epi32(d.vmask)),
			ushift(_mm_cvtsi32_si128(d.ushift)), vshift(_mm_cvtsi32_si128(d.vshift))
	{
		if (SLOPELIGHT)
		{
			const uint32_t l = static_cast<uint32_t>(d.lmap), s = static_cast<uint32_t>(d.lmapstep);
			lmap = _mm_setr_epi32(static_cast<int32_t>(l), static_cast<int32_t>(l + s),
					static_cast<int32_t>(l + (2 * s)), static_cast<int32_t>(l + (3 * s)));
			lstep4 = _mm_set1_epi32(static_cast<int32_t>(4 * s));
			lbias = _mm_set1_epi32(d.lbias);
		}
	}

	// The next four rows. lights is untouched without SLOPELIGHT.
	forceinline void next(uint32_t* spots, uint32_t* lights)
	{
		const __m128 mul = _mm_div_ps(_mm_set1_ps(65536.0f), _mm_add_ps(id, _mm_mul_ps(ids, fi)));
		const __m128i ufrac = _mm_cvttps_epi32(_mm_mul_ps(_mm_add_ps(iu, _mm_mul_ps(ius, fi)), mul));
		const __m128i vfrac = _mm_cvttps_epi32(_mm_mul_ps(_mm_add_ps(iv, _mm_mul_ps(ivs, fi)), mul));

		_mm_store_si128(reinterpret_cast<__m128i*>(spots), _mm_or_si128(
				_mm_and_si128(_mm_sra_epi32(ufrac, ushift), umask),
				_mm_and_si128(_mm_sra_epi32(vfrac, vshift), vmask)));

		fi = _mm_add_ps(fi, _mm_set1_ps(4.0f));

		if (SLOPELIGHT)
		{
			const __m128i lmax = _mm_set1_epi32(NUMCOLORMAPS - 1);

			__m128i index = _mm_add_epi32(_mm_srai_epi32(lmap, FRACBITS), lbias);
			index = _mm_andnot_si128(_mm_cmplt_epi32(index, _mm_setzero_si128()), index);
			const __m128i over = _mm_cmpgt_epi32(index, lmax);
			index = _mm_or_si128(_mm_and_si128(over, lmax), _mm_andnot_si128(over, index));

			_mm_store_si128(reinterpret_cast<__m128i*>(lights), _mm_slli_epi32(index, 8));
			lmap = _mm_add_epi32(lmap, lstep4);
		}
	}

private:
	const __m128 iu, iv, id;
	const __m128 ius, ivs, ids;
	__m128 fi;

	const __m128i umask, vmask;
	const __m128i ushift, vshift;

	__m128i lmap, lstep4, lbias;
};

#endif // __SSE2__
