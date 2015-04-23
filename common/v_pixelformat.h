// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: v_pixelformat.h 4868 2014-05-14 21:04:24Z dr_sean $
//
// Copyright (C) 2006-2015 by The Odamex Team.
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
//
//-----------------------------------------------------------------------------


#ifndef __V_PIXELFORMAT_H___
#define __V_PIXELFORMAT_H___

#include "doomtype.h"
#include "version.h"
#include "m_swap.h"			// for __BIG_ENDIAN__ macro

#include <cstring>


// ============================================================================
//
// PixelFormat class definition and implementation
//
// ============================================================================

//
// PixelFormat collects information about how a pixel is stored in memory. This
// is most relevant when the size of a pixel is more than 1 byte, making the
// byte ordering of color channels in memory cruical. On some platforms, such
// as certain versions of OSX, the memory layout of pixels changes when
// toggling the video mode between windowed and full-screen. Due to this
// behavior, the memory layout can not be deduced at compile time and thus
// we need to have a way of referencing the current pixel memory layout and
// to convert between different formats at runtime.
//
// The PixelFormat class is based on GPL v2 code from ScummVM:
//

/* ScummVM - Graphic Adventure Engine
*
* ScummVM is the legal property of its developers, whose names
* are too numerous to list here. Please refer to the COPYRIGHT
* file distributed with this source distribution.
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License
* as published by the Free Software Foundation; either version 2
* of the License, or (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
*
*/


class PixelFormat
{
public:
	inline PixelFormat() :
		mBitsPerPixel(0),
		mAShift(0), mRShift(0), mGShift(0), mBShift(0),
		mALoss(0), mRLoss(0), mGLoss(0), mBLoss(0)
	{ }

	inline PixelFormat(uint8_t bitsperpixel,
			uint8_t abits, uint8_t rbits, uint8_t gbits, uint8_t bbits,
			uint8_t ashift, uint8_t rshift, uint8_t gshift, uint8_t bshift) :
			mBitsPerPixel(bitsperpixel),
			mAShift(ashift), mRShift(rshift), mGShift(gshift), mBShift(bshift),
			mALoss(8 - abits), mRLoss(8 - rbits), mGLoss(8 - gbits), mBLoss(8 - bbits)
	{ }

	inline bool operator==(const PixelFormat& other) const
	{	return memcmp(this, &other, sizeof(PixelFormat)) == 0;	}

	inline bool operator!=(const PixelFormat& other) const
	{	return !(operator==(other));	}

	inline uint8_t getBitsPerPixel() const
	{	return mBitsPerPixel;	}

	inline uint8_t getBytesPerPixel() const
	{	return (mBitsPerPixel + 7) >> 3;	}

	inline argb_t convert(const uint8_t r, const uint8_t g, const uint8_t b) const
	{
		return ((0xFF >> mALoss) << mAShift) |
				((r >> mRLoss) << mRShift) |
				((g >> mGLoss) << mGShift) |
				((b >> mBLoss) << mBShift);
	}

	inline uint32_t convert(const uint8_t a, const uint8_t r, const uint8_t g, const uint8_t b) const
	{
		return ((a >> mALoss) << mAShift) |
				((r >> mRLoss) << mRShift) |
				((g >> mGLoss) << mGShift) |
				((b >> mBLoss) << mBShift);
	}

	inline uint8_t a(const uint32_t value) const
	{	return ((value >> mAShift) << mALoss) & 0xFF;	}

	inline uint8_t r(const uint32_t value) const
	{	return ((value >> mRShift) << mRLoss) & 0xFF;	}

	inline uint8_t g(const uint32_t value) const
	{	return ((value >> mGShift) << mGLoss) & 0xFF;	}

	inline uint8_t b(const uint32_t value) const
	{	return ((value >> mBShift) << mBLoss) & 0xFF;	}

	inline uint8_t getABits() const
	{	return 8 - mALoss;	}

	inline uint8_t getRBits() const
	{	return 8 - mRLoss;	}

	inline uint8_t getGBits() const
	{	return 8 - mGLoss;	}

	inline uint8_t getBBits() const
	{	return 8 - mBLoss;	}

	inline uint32_t getAMax() const
	{	return (1 << getABits()) - 1;	}

	inline uint32_t getRMax() const
	{	return (1 << getRBits()) - 1;	}

	inline uint32_t getGMax() const
	{	return (1 << getGBits()) - 1;	}

	inline uint32_t getBMax() const
	{	return (1 << getBBits()) - 1;	}

	inline uint8_t getAShift() const
	{	return mAShift;	}

	inline uint8_t getRShift() const
	{	return mRShift;	}

	inline uint8_t getGShift() const
	{	return mGShift;	}

	inline uint8_t getBShift() const
	{	return mBShift;	}

	inline uint8_t getAPos() const
	{
		#ifdef __BIG_ENDIAN__
		return (24 - mAShift) >> 3;
		#else
		return mAShift >> 3;
		#endif
	}

	inline uint8_t getRPos() const
	{
		#ifdef __BIG_ENDIAN__
		return (24 - mRShift) >> 3;
		#else
		return mRShift >> 3;
		#endif
	}

	inline uint8_t getGPos() const
	{
		#ifdef __BIG_ENDIAN__
		return (24 - mGShift) >> 3;
		#else
		return mGShift >> 3;
		#endif
	}

	inline uint8_t getBPos() const
	{
		#ifdef __BIG_ENDIAN__
		return (24 - mBShift) >> 3;
		#else
		return mBShift >> 3;
		#endif
	}

private:
	uint8_t		mBitsPerPixel;

	uint8_t		mAShift, mRShift, mGShift, mBShift;
	uint8_t		mALoss, mRLoss, mGLoss, mBLoss;
};

static inline void V_Convert(void* dest, const PixelFormat* destformat, uint16_t destpitch,
						void* source, const PixelFormat* source_format, uint16_t sourcepitch,
						uint16_t width, uint16_t height)
{
}


#endif	// __V_PIXELFORMAT_H__
