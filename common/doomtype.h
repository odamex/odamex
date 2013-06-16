// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
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
//	Simple basic typedefs, isolated here to make it easier
//	 separating modules.
//
//-----------------------------------------------------------------------------


#ifndef __DOOMTYPE__
#define __DOOMTYPE__

#include "version.h"

#ifdef GEKKO
#include <gctypes.h>
#endif

#ifndef __BYTEBOOL__
#define __BYTEBOOL__
// [RH] Some windows includes already define this
#if !defined(_WINDEF_) && !defined(__wtypes_h__) && !defined(GEKKO)
typedef int BOOL;
#endif
#ifndef __cplusplus
#define false (0)
#define true (1)
#endif
typedef unsigned char byte;
#endif

#ifdef __cplusplus
typedef bool dboolean;
#else
typedef enum {false, true} dboolean;
#endif

#if defined(_MSC_VER) || defined(__WATCOMC__)
#define STACK_ARGS __cdecl
#else
#define STACK_ARGS
#endif

// Predefined with some OS.
#ifndef UNIX
#ifndef _MSC_VER
#ifndef GEKKO
#include <values.h>
#endif
#endif
#endif

#if defined(__GNUC__) && !defined(OSF1)
#define __int64 long long
#endif

#ifdef OSF1
#define __int64 long
#endif

#if (defined _XBOX || defined _MSC_VER)
	typedef signed   __int8   int8_t;
	typedef signed   __int16  int16_t;
	typedef signed   __int32  int32_t;
	typedef unsigned __int8   uint8_t;
	typedef unsigned __int16  uint16_t;
	typedef unsigned __int32  uint32_t;
	typedef signed   __int64  int64_t;
	typedef unsigned __int64  uint64_t;
#else
	#include <stdint.h>
#endif

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
#define MAXINT			((int)0x7fffffff)
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
#ifndef WIN32
typedef unsigned int		DWORD;
typedef signed int			SDWORD;
#else
typedef unsigned long		DWORD;
typedef signed long			SDWORD;
#endif

typedef unsigned __int64	QWORD;
typedef signed __int64		SQWORD;

typedef DWORD				BITFIELD;

// 8bpp palette index
typedef byte                palindex_t;
// 32bpp ARGB color (A is usually 0)
typedef DWORD               argb_t;

#ifdef _WIN32
#define PATHSEP "\\"
#define PATHSEPCHAR '\\'
#else
#define PATHSEP "/"
#define PATHSEPCHAR '/'
#endif

// [RH] This gets used all over; define it here:
int STACK_ARGS Printf (int printlevel, const char *, ...);
// [Russell] Prints a bold green message to the console
int STACK_ARGS Printf_Bold (const char *format, ...);
// [RH] Same here:
int STACK_ARGS DPrintf (const char *, ...);

// Simple log file
#include <fstream>

extern std::ofstream LOG;
extern const char *LOG_FILE; //  Default is "odamex.log"

extern std::ifstream CON;

// game print flags
#define	PRINT_LOW			0		// pickup messages
#define	PRINT_MEDIUM		1		// death messages
#define	PRINT_HIGH			2		// critical messages
#define	PRINT_CHAT			3		// chat messages
#define PRINT_TEAMCHAT		4		// chat messages from a teammate

#ifdef __forceinline
#define forceinline __forceinline
#else
#define forceinline inline
#endif

//==========================================================================
//
// MIN
//
// Returns the minimum of a and b.
//==========================================================================

#ifdef MIN
#undef MIN
#endif
template<class T>
forceinline const T MIN (const T a, const T b)
{
	return a < b ? a : b;
}

//==========================================================================
//
// MAX
//
// Returns the maximum of a and b.
//==========================================================================

#ifdef MAX
#undef MAX
#endif
template<class T>
forceinline const T MAX (const T a, const T b)
{
	return a > b ? a : b;
}




//==========================================================================
//
// clamp
//
// Clamps in to the range [min,max].
//==========================================================================
#ifdef clamp
#undef clamp
#endif
template<class T>
forceinline T clamp (const T in, const T min, const T max)
{
	return in <= min ? min : in >= max ? max : in;
}


#define MAKERGB(r,g,b)		(((r)<<16)|((g)<<8)|(b))
#define MAKEARGB(a,r,g,b)	(((a)<<24)|((r)<<16)|((g)<<8)|(b))

#define APART(c)			(((c)>>24)&0xff)
#define RPART(c)			(((c)>>16)&0xff)
#define GPART(c)			(((c)>>8)&0xff)
#define BPART(c)			((c)&0xff)


// Alpha blend between two RGB colors with only dest alpha value
// 0 <=   toa <= 255
forceinline argb_t alphablend1a(const argb_t from, const argb_t to, const int toa)
{
	const int fr = RPART(from);
	const int fg = GPART(from);
	const int fb = BPART(from);

	const int dr = RPART(to) - fr;
	const int dg = GPART(to) - fg;
	const int db = BPART(to) - fb;

	return MAKERGB(
		fr + ((dr * toa) >> 8),
		fg + ((dg * toa) >> 8),
		fb + ((db * toa) >> 8)
	);
}

// Alpha blend between two RGB colors with two alpha values
// 0 <= froma <= 255
// 0 <=   toa <= 255
forceinline argb_t alphablend2a(const argb_t from, const int froma, const argb_t to, const int toa)
{
	return MAKERGB(
		(RPART(from) * froma + RPART(to) * toa) >> 8,
		(GPART(from) * froma + GPART(to) * toa) >> 8,
		(BPART(from) * froma + BPART(to) * toa) >> 8
	);
}



class translationref_t
{
	const palindex_t *m_table;
	int               m_player_id;

public:
	translationref_t();
	translationref_t(const translationref_t &other);
	translationref_t(const palindex_t *table);
	translationref_t(const palindex_t *table, const int player_id);

	const palindex_t tlate(const byte c) const;
	const int getPlayerID() const;
	const palindex_t *getTable() const;

	operator bool() const;
};

forceinline const palindex_t translationref_t::tlate(const byte c) const
{
#if DEBUG
	if (m_table == NULL) throw CFatalError("translationref_t::tlate() called with NULL m_table");
#endif
	return m_table[c];
}

forceinline const int translationref_t::getPlayerID() const
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
	const int mapnum() const;
	const byte ramp() const;

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
#if DEBUG
	if (m_colors == NULL) throw CFatalError("shaderef_t::index(): Bad shaderef_t");
	if (m_colors->colormap == NULL) throw CFatalError("shaderef_t::index(): colormap == NULL!");
#endif

	return m_colormap[c];
}

forceinline argb_t shaderef_t::shade(const palindex_t c) const
{
#if DEBUG
	if (m_colors == NULL) throw CFatalError("shaderef_t::shade(): Bad shaderef_t");
	if (m_colors->shademap == NULL) throw CFatalError("shaderef_t::shade(): shademap == NULL!");
#endif

	return m_shademap[c];
}

forceinline const shademap_t *shaderef_t::map() const
{
	return m_colors;
}

forceinline const int shaderef_t::mapnum() const
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

#endif
