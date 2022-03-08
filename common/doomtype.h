// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
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
//	Simple basic typedefs, isolated here to make it easier
//	 separating modules.
//
//-----------------------------------------------------------------------------


#pragma once

// Standard libc/STL includes we use in countless places

#include "version.h"
#include "errors.h"

#if defined(_MSC_VER)
#define forceinline __forceinline
#elif defined(__GNUC__)
#define forceinline inline __attribute__((always_inline))
#else
#define forceinline inline
#endif

// For __BIG_ENDIAN__ macro, requires forceinline
#include "m_swap.h"

#ifdef GEKKO
	#include <gctypes.h>
#endif

#ifdef _MSC_VER
	#define FORMAT_PRINTF(index, first_arg)
#else
	#define FORMAT_PRINTF(index, first_arg) __attribute__ ((format(printf, index, first_arg)))
#endif

#ifdef _MSC_VER
	#define NORETURN __declspec(noreturn)
#else
	#define NORETURN __attribute__ ((noreturn))
#endif

// [RH] Some windows includes already define this
#if !defined(_WINDEF_) && !defined(__wtypes_h__) && !defined(GEKKO)
typedef int BOOL;
#endif

typedef unsigned char byte;
typedef unsigned int uint;

#if defined(_MSC_VER) || defined(__WATCOMC__)
	#define STACK_ARGS __cdecl
#else
	#define STACK_ARGS
#endif

// Predefined with some OS.
#if !defined(UNIX) && !defined(_WIN32) && !defined(GEKKO)
	#include <limits.h>
	#include <float.h>
#endif

#if defined(__GNUC__) && !defined(OSF1)
	#define __int64 long long
#endif

#ifdef OSF1
	#define __int64 long
#endif

#if (defined _XBOX || defined _MSC_VER)
	#define DBL_EPSILON 2.2204460492503131e-016
	#define FLT_EPSILON 1.192092896e-07F

	#define PRI_SIZE_PREFIX "I"
#else
	#include <float.h>

	#define PRI_SIZE_PREFIX "z"
#endif

// Format constants for ssize_t/size_t.

#define PRIdSIZE PRI_SIZE_PREFIX "d"
#define PRIiSIZE PRI_SIZE_PREFIX "i"
#define PRIuSIZE PRI_SIZE_PREFIX "u"
#define PRIoSIZE PRI_SIZE_PREFIX "o"
#define PRIxSIZE PRI_SIZE_PREFIX "x"
#define PRIXSIZE PRI_SIZE_PREFIX "X"

#ifdef UNIX
	#define stricmp strcasecmp
	#define strnicmp strncasecmp
#endif

#ifndef MAXCHAR
	#define MAXCHAR 		((char)0x7f)
#endif
#ifndef MAXSHORT
	#define MAXSHORT		((short)0x7fff)
#endif

// Max pos 32-bit int.
#ifndef MAXINT
	#define MAXINT			(0x7fffffff)
#endif
#ifndef MAXUINT
	#define MAXUINT			(0xffffffff)
#endif

#ifndef MAXLONG
	#ifndef ALPHA
		#define MAXLONG 		((long)0x7fffffff)
	#else
		#define MAXLONG			((long)0x7fffffffffffffff)
	#endif
#endif

#ifndef MINCHAR
	#define MINCHAR 		((char)0x80)
#endif
#ifndef MINSHORT
	#define MINSHORT		((short)0x8000)
#endif

// Max negative 32-bit integer.
#ifndef MININT
	#define MININT			((int)0x80000000)
#endif
#ifndef MINLONG
	#ifndef ALPHA
		#define MINLONG 		((long)0x80000000)
	#else
		#define MINLONG			((long)0x8000000000000000)
	#endif
#endif

#define MINFIXED		(signed)(0x80000000)
#define MAXFIXED		(signed)(0x7fffffff)

typedef unsigned char		BYTE;
typedef signed char			SBYTE;

typedef unsigned short		WORD;
typedef signed short		SWORD;

// denis - todo - this 64 bit fix conflicts with windows' idea of long
#ifndef _WIN32
typedef unsigned int		DWORD;
typedef signed int			SDWORD;
#else
typedef unsigned long		DWORD;
typedef signed long			SDWORD;
#endif

typedef unsigned __int64	QWORD;
typedef signed __int64		SQWORD;

typedef DWORD				BITFIELD;

typedef uint64_t			dtime_t;

#ifdef _WIN32
	#define PATHSEP "\\"
	#define PATHSEPCHAR '\\'
	#define PATHLISTSEP ";"
	#define PATHLISTSEPCHAR ';'
#else
	#define PATHSEP "/"
	#define PATHSEPCHAR '/'
	#define PATHLISTSEP ":"
	#define PATHLISTSEPCHAR ':'
#endif

/**
 * @brief Returns a bitfield with a specific bit set.
 */
#define BIT(a) (1U << (a))

/**
 * @brief Returns a bitfield with a range of bits set from a to b, inclusive.
 * 
 * @param a Low bit in the mask.
 * @param b High bit in the mask. 
 */
static inline uint32_t BIT_MASK(uint32_t a, uint32_t b)
{
    return (static_cast<uint32_t>(-1) >> (31 - b)) & ~(BIT(a) - 1);
}

// [RH] This gets used all over; define it here:
int Printf(fmt::CStringRef format, fmt::ArgList args);
FMT_VARIADIC(int, Printf, fmt::CStringRef)
int Printf(const int printlevel, fmt::CStringRef format, fmt::ArgList args);
FMT_VARIADIC(const int, Printf, int, fmt::CStringRef)
// [Russell] Prints a bold green message to the console
int Printf_Bold(fmt::CStringRef format, fmt::ArgList args);
FMT_VARIADIC(int, Printf_Bold, fmt::CStringRef)
// [RH] Same here:
int DPrintf(fmt::CStringRef format, fmt::ArgList args);
FMT_VARIADIC(int, DPrintf, fmt::CStringRef)

/**
 * @brief Print to all clients in a server, or to the local player offline.
 *
 * @note This could really use a new name, like "ServerPrintf".
 *
 * @param format printf-style format string.
 * @param ... printf-style arguments.
 */
void STACK_ARGS SV_BroadcastPrintf(const char* format, ...) FORMAT_PRINTF(1, 2);

/**
 * @brief Print to all clients in a server, or to the local player offline.
 *
 * @note This could really use a new name, like "ServerPrintf".
 *
 * @param printlevel PRINT_* constant designating what kind of print this is.
 * @param format printf-style format string.
 * @param ... printf-style arguments.
 */
void STACK_ARGS SV_BroadcastPrintf(int printlevel, const char* format, ...)
    FORMAT_PRINTF(2, 3);

#ifdef SERVER_APP
void STACK_ARGS SV_BroadcastPrintfButPlayer(int printlevel, int player_id, const char* format, ...);
#endif

// game print flags
typedef enum {
	PRINT_PICKUP,		// Pickup messages
	PRINT_OBITUARY,		// Death messages
	PRINT_HIGH,			// Regular messages

	PRINT_CHAT,			// Chat messages
	PRINT_TEAMCHAT,		// Chat messages from a teammate
	PRINT_SERVERCHAT,	// Chat messages from the server

	PRINT_WARNING,		// Warning messages
	PRINT_ERROR,		// Fatal error messages

	PRINT_NORCON,		// Do NOT send the message to any rcon client.

	PRINT_FILTERCHAT,	// Filter the message to not be displayed ingame, but only in the console (ugly hack)		

	PRINT_MAXPRINT
} printlevel_t;

//
// MIN
//
// Returns the minimum of a and b.
//
#ifdef MIN
	#undef MIN
#endif
template<class T>
forceinline const T MIN (const T a, const T b)
{
	return a < b ? a : b;
}

//
// MAX
//
// Returns the maximum of a and b.
//
#ifdef MAX
	#undef MAX
#endif
template<class T>
forceinline const T MAX (const T a, const T b)
{
	return a > b ? a : b;
}




//
// clamp
//
// Clamps the value of in to the range min, max
//
#ifdef clamp
	#undef clamp
#endif
template<class T>
forceinline T clamp (const T in, const T min, const T max)
{
	return in <= min ? min : in >= max ? max : in;
}

//
// ARRAY_LENGTH
//
// Safely counts the number of items in an C array.
// 
// https://www.drdobbs.com/cpp/counting-array-elements-at-compile-time/197800525?pgno=1
//
#define ARRAY_LENGTH(arr) ( \
	0 * sizeof(reinterpret_cast<const ::Bad_arg_to_ARRAY_LENGTH*>(arr)) + \
	0 * sizeof(::Bad_arg_to_ARRAY_LENGTH::check_type((arr), &(arr))) + \
	sizeof(arr) / sizeof((arr)[0]) )

struct Bad_arg_to_ARRAY_LENGTH {
	class Is_pointer; // incomplete
	class Is_array {};
	template <typename T>
	static Is_pointer check_type(const T*, const T* const*);
	static Is_array check_type(const void*, const void*);
};


// ----------------------------------------------------------------------------
//
// Color Management Types
//
// ----------------------------------------------------------------------------

// 8-bit palette index
typedef uint8_t				palindex_t;

//
// argb_t class
//
// Allows ARGB8888 values to be accessed as a packed 32-bit integer or accessed
// by its individual 8-bit color and alpha channels.
//
class argb_t
{
public:
	argb_t() { }
	argb_t(uint32_t _value) : value(_value) { }

	argb_t(uint8_t _r, uint8_t _g, uint8_t _b)
	{	seta(255); setr(_r); setg(_g); setb(_b);	}

	argb_t(uint8_t _a, uint8_t _r, uint8_t _g, uint8_t _b)
	{	seta(_a); setr(_r); setg(_g); setb(_b);	}

	inline operator uint32_t () const { return value; }

	inline uint8_t geta() const
	{	return channels[a_num];	}

	inline uint8_t getr() const
	{	return channels[r_num];	}

	inline uint8_t getg() const
	{	return channels[g_num];	}

	inline uint8_t getb() const
	{	return channels[b_num];	}

	inline void seta(uint8_t n)
	{	channels[a_num] = n;	}

	inline void setr(uint8_t n)
	{	channels[r_num] = n;	}

	inline void setg(uint8_t n)
	{	channels[g_num] = n;	}

	inline void setb(uint8_t n)
	{	channels[b_num] = n;	}

	static void setChannels(uint8_t _a, uint8_t _r, uint8_t _g, uint8_t _b)
	{	a_num = _a; r_num = _r; g_num = _g; b_num = _b;	}

private:
	static uint8_t a_num, r_num, g_num, b_num;

	union
	{
		uint32_t value;
		uint8_t channels[4];
	};
};


//
// fargb_t class
//
// Stores ARGB color channels as four floats. This is ideal for color
// manipulation where precision is important.
//
class fargb_t
{
public:
	fargb_t() { }
	fargb_t(const argb_t color)
	{
		seta(color.geta() / 255.0f); setr(color.getr() / 255.0f);
		setg(color.getg() / 255.0f); setb(color.getb() / 255.0f);
	}

	fargb_t(float _r, float _g, float _b)
	{	seta(1.0f); setr(_r); setg(_g); setb(_b);	}

	fargb_t(float _a, float _r, float _g, float _b)
	{	seta(_a); setr(_r); setg(_g); setb(_b);	}

	inline operator argb_t () const
	{	return argb_t((uint8_t)(a * 255.0f), (uint8_t)(r * 255.0f), (uint8_t)(g * 255.0f), (uint8_t)(b * 255.0f));	}

	inline float geta() const
	{	return a;	}

	inline float getr() const
	{	return r;	}

	inline float getg() const
	{	return g;	}

	inline float getb() const
	{	return b;	}

	inline void seta(float n)
	{	a = n;	}

	inline void setr(float n)
	{	r = n;	}

	inline void setg(float n)
	{	g = n;	}

	inline void setb(float n)
	{	b = n;	}

private:
	float a, r, g, b;
};


//
// fahsv_t class
//
// Stores AHSV color channels as four floats.
//
class fahsv_t
{
public:
	fahsv_t() { }

	fahsv_t(float _h, float _s, float _v)
	{	seta(1.0f); seth(_h); sets(_s); setv(_v);	}

	fahsv_t(float _a, float _h, float _s, float _v)
	{	seta(_a); seth(_h); sets(_s); setv(_v);	}

	inline float geta() const
	{	return a;	}

	inline float geth() const
	{	return h;	}

	inline float gets() const
	{	return s;	}

	inline float getv() const
	{	return v;	}

	inline void seta(float n)
	{	a = n;	}

	inline void seth(float n)
	{	h = n;	}

	inline void sets(float n)
	{	s = n;	}

	inline void setv(float n)
	{	v = n;	}

private:
	float a, h, s, v;
};


// ----------------------------------------------------------------------------
//
// Color Mapping classes
//
// ----------------------------------------------------------------------------


class translationref_t
{
	const palindex_t *m_table;
	int               m_player_id;

public:
	translationref_t();
	translationref_t(const translationref_t &other);
	translationref_t(const palindex_t *table);
	translationref_t(const palindex_t *table, const int player_id);

	palindex_t tlate(const byte c) const;
	int getPlayerID() const;
	const palindex_t *getTable() const;

	operator bool() const;
};

forceinline palindex_t translationref_t::tlate(const byte c) const
{
	#if ODAMEX_DEBUG
	if (m_table == NULL)
		throw CFatalError("translationref_t::tlate() called with NULL m_table");
	#endif
	return m_table[c];
}

forceinline int translationref_t::getPlayerID() const
{
	return m_player_id;
}

forceinline const byte *translationref_t::getTable() const
{
	return m_table;
}

forceinline translationref_t::operator bool() const
{
	return m_table != NULL;
}


typedef struct {
	palindex_t  *colormap;  // Colormap for 8-bit
	argb_t      *shademap;  // ARGB8888 values for 32-bit
	byte        ramp[256];  // Light fall-off as a function of distance
	                        // Light levels: 0 = black, 255 = full bright.
	                        // Distance:     0 = near,  255 = far.
} shademap_t;

struct dyncolormap_s;
typedef struct dyncolormap_s dyncolormap_t;

// This represents a clean reference to a map of both 8-bit colors and 32-bit shades.
struct shaderef_t {
private:
	const shademap_t *m_colors; // The color/shade map to use
	int               m_mapnum; // Which index into the color/shade map to use

public:
	mutable const palindex_t    *m_colormap;    // Computed colormap pointer
	mutable const argb_t        *m_shademap;    // Computed shademap pointer
	mutable const dyncolormap_t *m_dyncolormap; // Auto-discovered dynamic colormap

public:
	shaderef_t();
	shaderef_t(const shaderef_t &other);
	shaderef_t(const shademap_t * const colors, const int mapnum);

	// Determines if m_colors is NULL
	bool isValid() const;

	shaderef_t with(const int mapnum) const;

	palindex_t  index(const byte c) const;
	argb_t      shade(const byte c) const;
	const shademap_t *map() const;
	int mapnum() const;
	byte ramp() const;

	argb_t tlate(const translationref_t &translation, const byte c) const;

	bool operator==(const shaderef_t &other) const;
};

forceinline bool shaderef_t::isValid() const
{
	return m_colors != NULL;
}

forceinline shaderef_t shaderef_t::with(const int mapnum) const
{
	return shaderef_t(m_colors, m_mapnum + mapnum);
}


forceinline palindex_t shaderef_t::index(const palindex_t c) const
{
	#if ODAMEX_DEBUG
	if (m_colors == NULL)
		throw CFatalError("shaderef_t::index(): Bad shaderef_t");
	if (m_colors->colormap == NULL)
		throw CFatalError("shaderef_t::index(): colormap == NULL!");
	#endif

	return m_colormap[c];
}

forceinline argb_t shaderef_t::shade(const palindex_t c) const
{
	#if ODAMEX_DEBUG
	if (m_colors == NULL)
		throw CFatalError("shaderef_t::shade(): Bad shaderef_t");
	if (m_colors->shademap == NULL)
		throw CFatalError("shaderef_t::shade(): shademap == NULL!");
	#endif

	return m_shademap[c];
}

forceinline const shademap_t *shaderef_t::map() const
{
	return m_colors;
}

forceinline int shaderef_t::mapnum() const
{
	return m_mapnum;
}

forceinline bool shaderef_t::operator==(const shaderef_t &other) const
{
	return m_colors == other.m_colors && m_mapnum == other.m_mapnum;
}

// NOTE(jsd): Rest of shaderef_t and translationref_t functions implemented in "r_main.h"
// NOTE(jsd): Constructors are implemented in "v_palette.cpp"

// rt_rawcolor does no color mapping and only uses the default palette.
template<typename pixel_t>
static forceinline pixel_t rt_rawcolor(const shaderef_t &pal, const byte c);

// rt_mapcolor does color mapping.
template<typename pixel_t>
static forceinline pixel_t rt_mapcolor(const shaderef_t &pal, const byte c);

// rt_tlatecolor does color mapping and translation.
template<typename pixel_t>
static forceinline pixel_t rt_tlatecolor(const shaderef_t &pal, const translationref_t &translation, const byte c);

// rt_blend2 does alpha blending between two colors.
template<typename pixel_t>
static forceinline pixel_t rt_blend2(const pixel_t bg, const int bga, const pixel_t fg, const int fga);


template<>
forceinline palindex_t rt_rawcolor<palindex_t>(const shaderef_t &pal, const byte c)
{
	// NOTE(jsd): For rawcolor we do no index.
	return (c);
}

template<>
forceinline argb_t rt_rawcolor<argb_t>(const shaderef_t &pal, const byte c)
{
	return pal.shade(c);
}


template<>
forceinline palindex_t rt_mapcolor<palindex_t>(const shaderef_t &pal, const byte c)
{
	return pal.index(c);
}

template<>
forceinline argb_t rt_mapcolor<argb_t>(const shaderef_t &pal, const byte c)
{
	return pal.shade(c);
}


template<>
forceinline palindex_t rt_tlatecolor<palindex_t>(const shaderef_t &pal, const translationref_t &translation, const byte c)
{
	return translation.tlate(pal.index(c));
}

template<>
forceinline argb_t rt_tlatecolor<argb_t>(const shaderef_t &pal, const translationref_t &translation, const byte c)
{
	return pal.tlate(translation, c);
}
