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
//	WAD I/O functions.
//
//-----------------------------------------------------------------------------


#pragma once

#include "z_zone.h"
#include "r_defs.h"
#include "m_resfile.h"


// [RH] Compare wad header as ints instead of chars
#define IWAD_ID (('I')|('W'<<8)|('A'<<16)|('D'<<24))
#define PWAD_ID (('P')|('W'<<8)|('A'<<16)|('D'<<24))

// [RH] Remove limit on number of WAD files
extern OResFiles wadfiles;
extern OResFiles patchfiles;
extern OWantFiles missingfiles;
extern bool missingCommercialIWAD;

//
// TYPES
//
typedef struct
{
	// Should be "IWAD" or "PWAD".
	unsigned	identification;
	int			numlumps;
	int			infotableofs;

} wadinfo_t;


typedef struct
{
	int			filepos;
	int			size;
	char		name[8]; // denis - todo - string

} filelump_t;

//
// WADFILE I/O related stuff.
//
typedef struct lumpinfo_s
{
	char		name[8]; // denis - todo - string
	FILE		*handle;
	int			position;
	int			size;

	// [RH] Hashing stuff
	int			next;
	int			index;

	int			namespc;
} lumpinfo_t;

// [RH] Namespaces from BOOM.
typedef enum {
	ns_global = 0,
	ns_sprites,
	ns_flats,
	ns_colormaps,
} namespace_t;

struct lumpHandle_t
{
	size_t id;
	lumpHandle_t() : id(0)
	{
	}
	void clear()
	{
		id = 0;
	}
	bool empty() const
	{
		return id == 0;
	}
	bool operator==(const lumpHandle_t& other)
	{
		return id == other.id;
	}
};

extern	void**		lumpcache;
extern	lumpinfo_t*	lumpinfo;
extern	size_t	numlumps;

OCRC32Sum W_CRC32(const std::string& filename);
OMD5Hash W_MD5(const std::string& filename);
fhfprint_s W_FarmHash128(const byte* lumpdata, int length);
void W_InitMultipleFiles(const OResFiles& filenames);
lumpHandle_t W_LumpToHandle(const unsigned lump);
int W_HandleToLump(const lumpHandle_t handle);

int W_CheckNumForName(const char *name, int ns = ns_global);
int W_GetNumForName(const char *name, int ns = ns_global);
int W_GetNumForName(OLumpName& name, int ns = ns_global);

std::string W_LumpName(unsigned lump);
unsigned	W_LumpLength (unsigned lump);
void		W_ReadLump (unsigned lump, void *dest);
unsigned	W_ReadChunk (const char *file, unsigned offs, unsigned len, void *dest, unsigned &filelen);

void* W_CacheLumpNum(unsigned lump, const zoneTag_e tag);
void* W_CacheLumpName(const char* name, const zoneTag_e tag);
void* W_CacheLumpName(OLumpName& name, const zoneTag_e tag);
patch_t* W_CachePatch(unsigned lump, const zoneTag_e tag = PU_CACHE);
patch_t* W_CachePatch(const char* name, const zoneTag_e tag = PU_CACHE);
patch_t* W_CachePatch(OLumpName& name, const zoneTag_e tag = PU_CACHE);
lumpHandle_t W_CachePatchHandle(const int lumpNum, const zoneTag_e tag = PU_CACHE);
lumpHandle_t W_CachePatchHandle(const char* name, const zoneTag_e tag = PU_CACHE,
                                int ns = ns_global);
patch_t* W_ResolvePatchHandle(const lumpHandle_t lump);

void	W_Profile (const char *fname);

void	W_Close ();

int		W_FindLump (const char *name, int lastlump);	// [RH]	Find lumps with duplication
bool	W_CheckLumpName (unsigned lump, const char *name);	// [RH] True if lump's name == name // denis - todo - replace with map<>

//unsigned W_LumpNameHash (const char *name);				// [RH] Create hash key from an 8-char name

// [RH] Combine multiple marked ranges of lumps into one.
void	W_MergeLumps (const char *start, const char *end, int);

// [RH] Copy an 8-char string and uppercase it.
void uppercopy (char *to, const char *from);

// [RH] Copies the lump name to to using uppercopy
void W_GetLumpName(char* to, unsigned lump);

// [RH] Copies the lump name to to using uppercopy
void W_GetOLumpName(OLumpName& to, unsigned lump);

// [RH] Returns file handle for specified lump
int W_GetLumpFile (unsigned lump);

// [RH] Put a lump in a certain namespace
//void W_SetLumpNamespace (unsigned lump, int nmspace);
