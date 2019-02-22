// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
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

// [RH] Copy an 8-char string and uppercase it.
void uppercopy(char *to, const char *from);

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
} namespace_t;

extern	lumpinfo_t*	lumpinfo;
extern	size_t	numlumps;

//
// WadCollection
// Structure made for simplier lookout of W_ functions
//
struct WadCollection {

public:

	std::vector<std::string> InitMultipleFiles(std::vector<std::string> &filenames);
	std::string GetMD5Hash(std::string filename);

	void	Close();

	void			HashLumps(void);
	unsigned int	LumpNameHash(const char *s);

	int				CheckNumForName(const char *name, int namespc);
	int				GetNumForName(const char *name);

	inline int CheckNumForName(const byte *name) { return CheckNumForName((const char *)name, ns_global); }
	inline int CheckNumForName(const char *name) { return CheckNumForName(name, ns_global); }


	void ReadLump(unsigned lump, void *dest);

	unsigned ReadChunk(const char *file, unsigned offs, unsigned len, void *dest, unsigned &filelen);

	unsigned LumpLength(unsigned lump);

	bool CheckLumpName(unsigned lump, const char *name);	// [RH] True if lump's name == name // denis - todo - replace with map<>

	void GetLumpName(char *to, unsigned lump);			// [RH] Copies the lump name to to using uppercopy

	void *CacheLumpNum(unsigned lump, int tag);
	void *CacheLumpName(const char *name, int tag);

	patch_t *CachePatch(unsigned lump, int tag = PU_CACHE);
	patch_t *CachePatch(const char *name, int tag = PU_CACHE);

	int	FindLump(const char *name, int lastlump);	// [RH]	Find lumps with duplication

protected:
	void	MergeLumps(const char *start, const char *end, int);	// [RH] Combine multiple marked ranges of lumps into one.

private:
	void**			lumpcache;
	unsigned	stdisk_lumpnum;

	std::string AddFile(std::string filename);
	bool IsMarker(const lumpinfo_t *lump, const char *marker);
};

extern WadCollection wads;

#endif



