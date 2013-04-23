// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
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
//	WAD I/O functions.
//
//-----------------------------------------------------------------------------


#ifndef __W_WAD__
#define __W_WAD__

#include "doomtype.h"
#include "z_zone.h"
#include "r_defs.h"

#include <string>
#include <vector>

// [RH] Compare wad header as ints instead of chars
#define IWAD_ID (('I')|('W'<<8)|('A'<<16)|('D'<<24))
#define PWAD_ID (('P')|('W'<<8)|('A'<<16)|('D'<<24))

// [RH] Remove limit on number of WAD files
extern std::vector<std::string> wadfiles, wadhashes, patchfiles;

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
	ns_skinbase = 0x80000000	// Each skin's status bar face gets own namespace
} namespace_t;

extern	void**		lumpcache;
extern	lumpinfo_t*	lumpinfo;
extern	size_t	numlumps;

std::string W_MD5(std::string filename);
std::vector<std::string> W_InitMultipleFiles (std::vector<std::string> &filenames);

int		W_CheckNumForName (const char *name, int ns = ns_global);
int		W_GetNumForName (const char *name);

unsigned	W_LumpLength (unsigned lump);
void		W_ReadLump (unsigned lump, void *dest);
unsigned	W_ReadChunk (const char *file, unsigned offs, unsigned len, void *dest, unsigned &filelen);

void *W_CacheLumpNum (unsigned lump, int tag);
void *W_CacheLumpName (const char *name, int tag);
patch_t* W_CachePatch (unsigned lump, int tag = PU_CACHE);
patch_t* W_CachePatch (const char *name, int tag = PU_CACHE);

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
void W_GetLumpName (char *to, unsigned lump);

// [RH] Returns file handle for specified lump
int W_GetLumpFile (unsigned lump);

// [Russell] Simple function to check whether the given string is an iwad name
bool W_IsIWAD(const std::string& filename, const std::string& hash = "");

// [RH] Put a lump in a certain namespace
//void W_SetLumpNamespace (unsigned lump, int nmspace);

#endif



