// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
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
struct wadinfo_t
{
	// Should be "IWAD" or "PWAD".
	unsigned    identification;
	int         numlumps;
	int         infotableofs;

	bool Read(std::istream& io_stream);
};

struct filelump_t
{
	constexpr static size_t SIZE_IN_BYTES = 16;

	int     filepos;
	int     size;
	char    name[8]; // denis - todo - string

	bool Read(std::istream& io_stream);
};

// [RH] Namespaces from BOOM.
enum namespace_t
{
	ns_global = 0,
	ns_textures,
	ns_sprites,
	ns_flats,
	ns_colormaps,
};

//
// WADFILE I/O related stuff.
//
struct lumpinfo_t
{
	OLumpName                     name      {};
	std::shared_ptr<std::istream> handle    {};
	int                           position  { 0 };
	int                           size      { 0 };
	int                           file      { -1 };

	// [RH] Hashing stuff
	int next    { -1 };
	int index   { -1 };

	namespace_t namespc { ns_global };

	lumpinfo_t() = default;     // Needed because the following ctors implicitly delete the default ctor.

	explicit lumpinfo_t(const OLumpName& i_name) :
		name(i_name)
	{
	}

	lumpinfo_t(const std::shared_ptr<std::istream>& i_stream, const filelump_t& i_fileinfo, int i_file) :
		name    (i_fileinfo.name),
		handle  (i_stream),
		position(i_fileinfo.filepos),
		size    (i_fileinfo.size),
		file    (i_file)
	{
	}
};

struct lumpHandle_t
{
	size_t id;
	lumpHandle_t() noexcept : id(0)
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
	[[nodiscard]]
	bool operator==(const lumpHandle_t& other) const
	{
		return id == other.id;
	}
};

extern	void**		lumpcache;
extern std::vector<lumpinfo_t> lumpinfo;
inline size_t W_NumLumps() { return lumpinfo.size(); }

OCRC32Sum W_CRC32(const std::string& filename);
OMD5Hash W_MD5(const std::string& filename);
fhfprint_t W_FarmHash128(const byte* lumpdata, int length);
void W_InitMultipleFiles(const OResFiles& filenames);
lumpHandle_t W_LumpToHandle(const unsigned lump);
int W_HandleToLump(const lumpHandle_t handle);

int W_CheckNumForName(const char *name, namespace_t ns = ns_global);
inline int W_CheckNumForName(const OLumpName& name, namespace_t ns = ns_global) { return W_CheckNumForName(name.c_str(), ns); };
int W_GetNumForName(const char *name, namespace_t ns = ns_global);
inline int W_GetNumForName(const OLumpName& name, namespace_t ns = ns_global) { return W_GetNumForName(name.c_str(), ns); };

OLumpName W_LumpName(unsigned lump);
unsigned	W_LumpLength (unsigned lump);
void		W_ReadLump (unsigned lump, void *dest);
unsigned	W_ReadChunk (const char *file, unsigned offs, unsigned len, void *dest, unsigned &filelen);

// TODO: add similar funcs that return string_views // how to deal with z_free then though?
void* W_CacheLumpNum(unsigned lump, const zoneTag_e tag);

//
// W_CacheLumpNum
//
template <typename T>
requires std::is_object_v<T>
T* W_CacheLumpNum(unsigned lump, const zoneTag_e tag)
{
	return static_cast<T*>(W_CacheLumpNum(lump, tag));
}

//
// W_CacheLumpName
//
template <typename T = void>
requires (std::is_object_v<T> || std::is_void_v<T>)
T* W_CacheLumpName(const char* name, const zoneTag_e tag)
{
	return W_CacheLumpNum<T>(W_GetNumForName(name), tag);
}

//
// W_CacheLumpName
//
template <typename T = void>
requires (std::is_object_v<T> || std::is_void_v<T>)
T* W_CacheLumpName(const OLumpName& name, const zoneTag_e tag)
{
	return W_CacheLumpNum<T>(W_GetNumForName(name), tag);
}

OLumpName W_CheckWidescreenPatch(const OLumpName& lump_main);

patch_t* W_CachePatch(unsigned lump, const zoneTag_e tag = PU_CACHE);
patch_t* W_CachePatch(const char* name, const zoneTag_e tag = PU_CACHE);
patch_t* W_CachePatch(const OLumpName& name, const zoneTag_e tag = PU_CACHE);
lumpHandle_t W_CachePatchHandle(const int lumpNum, const zoneTag_e tag = PU_CACHE);
lumpHandle_t W_CachePatchHandle(const char* name, const zoneTag_e tag = PU_CACHE, namespace_t ns = ns_global);
lumpHandle_t W_CachePatchHandle(const OLumpName&, const zoneTag_e tag = PU_CACHE, namespace_t ns = ns_global);
patch_t* W_ResolvePatchHandle(const lumpHandle_t lump);

void	W_Profile (const char *fname);

void	W_Close ();

int		W_FindLump (const char *name, int lastlump);	// [RH]	Find lumps with duplication
bool	W_CheckLumpName (unsigned lump, const char *name);	// [RH] True if lump's name == name // denis - todo - replace with map<>

//unsigned W_LumpNameHash (const char *name);				// [RH] Create hash key from an 8-char name

// [RH] Combine multiple marked ranges of lumps into one.
void W_MergeLumps (const OLumpName& start, const OLumpName& end, namespace_t);

// [RH] Copy an 8-char string and uppercase it.
void uppercopy (char *to, const char *from);

// [RH] Copies the lump name to to using uppercopy
void W_GetLumpName(char* to, unsigned lump);

// Copies the lump name to to
void W_GetOLumpName(OLumpName& to, unsigned lump);
OLumpName W_GetOLumpName(unsigned lump);

// wadfiles always begins with odamex.wad followed by the IWAD, so every file
// from here on is a PWAD.
constexpr int WADFILE_FIRSTPWAD = 2;

// [RH] Returns file handle for specified lump
int W_GetLumpFile (unsigned lump);

// True when a lump was supplied by a PWAD rather than by the IWAD, odamex.wad,
// or the engine itself.
//
// The name overloads ask about the lump the engine resolves for that name,
// which is the one the game actually uses.
bool W_IsLumpFromPWAD(unsigned lump);
bool W_IsLumpFromPWAD(const char* name, namespace_t namespc = ns_global);
inline bool W_IsLumpFromPWAD(const OLumpName& name, namespace_t ns = ns_global) { return W_IsLumpFromPWAD(name.c_str(), ns); };

// True when a PWAD covers up a lump of the same name from an earlier file, as
// opposed to contributing one the game did not already have.
bool W_IsLumpReplaced(const char* name, namespace_t namespc = ns_global);
inline bool W_IsLumpReplaced(const OLumpName& name, namespace_t ns = ns_global) { return W_IsLumpReplaced(name.c_str(), ns); };

// [RH] Put a lump in a certain namespace
//void W_SetLumpNamespace (unsigned lump, int nmspace);
