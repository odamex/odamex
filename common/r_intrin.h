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
//	Vectorization intrinsics detection.
//
//-----------------------------------------------------------------------------

#pragma once

// NOTE(jsd): This is from bleeding-edge SDL code which is not in 1.2-15

/* Need to do this here because intrin.h has C++ code in it */
/* Visual Studio 2005 has a bug where intrin.h conflicts with winnt.h */
#if defined(_MSC_VER) && (_MSC_VER >= 1500) && !defined(_WIN32_WCE)
	#include <intrin.h>
	#if defined(_M_IX86) || defined(_M_X64)
		#ifndef _WIN64
			#define __MMX__
			#define __3dNOW__
		#endif
		#define __SSE__
		#define __SSE2__
	#elif defined(_M_ARM64)
		// MSVC targets AArch64 with NEON but does not define the ACLE macro.
		#ifndef __ARM_NEON
			#define __ARM_NEON 1
		#endif
		#include <arm_neon.h>
	#endif
#elif defined(__MINGW64_VERSION_MAJOR)
	#include <intrin.h>
#else
	#ifdef __ALTIVEC__
		#if HAVE_ALTIVEC_H && !defined(__APPLE_ALTIVEC__)
			#include <altivec.h>
			#undef pixel
		#endif
	#endif
	#ifdef __MMX__
		#include <mmintrin.h>
	#endif
	#ifdef __3dNOW__
		#include <mm3dnow.h>
	#endif
	#ifdef __SSE__
		#include <xmmintrin.h>
	#endif
	#ifdef __SSE2__
		#include <emmintrin.h>
	#endif
	// AArch64 always has NEON
	// 32-bit ARM has it when the build enables it
	// for rare cases of 32-bit ARM having access
	// to NEON, like raspberry pi 32-bit OS on
	// a raspberry pi 3 or 4
	#ifdef __ARM_NEON
		#include <arm_neon.h>
	#endif
#endif
