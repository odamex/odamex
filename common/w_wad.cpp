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
//	Handles WAD file header, directory, lump I/O.
//
//-----------------------------------------------------------------------------


#include "odamex.h"

#ifdef UNIX
#include <ctype.h>
#ifndef O_BINARY
#define O_BINARY		0
#endif
#endif

#ifdef _WIN32
#include <io.h>
#else
#define strcmpi	strcasecmp
#endif

#include <fcntl.h>

#include "crc32.h"

#include "m_fileio.h"
#include "i_system.h"
#include "z_zone.h"
#include "cmdlib.h"
#include "m_argv.h"
#include "md5.h"

#include "farmhash.h"

#include "w_wad.h"

#include <sstream>
#include <algorithm>
#include <iomanip>


//
// GLOBALS
//

// Location of each lump on disk.
lumpinfo_t*		lumpinfo;
size_t			numlumps;

// Generation of handle.
// Takes up the first three bits of the handle id.  Starts at 1, increments
// every time we unload the current set of WAD files, and eventually wraps
// around from 7 to 1.  We skip 0 so a handle id of 0 can be considered NULL
// and part of no generation.
size_t handleGen = 1;
const size_t HANDLE_GEN_MASK = BIT_MASK(0, 2);
const size_t HANDLE_GEN_BITS = 3;

void**			lumpcache;

static unsigned	stdisk_lumpnum;

//
// W_LumpNameHash
//
// Hash function used for lump names. Must be mod'ed with table size.
// Can be used for any 8-character names.
// by Lee Killough
//
// [SL] taken from prboom-plus
//
unsigned int W_LumpNameHash(const char *s)
{
	unsigned int hash;

	(void)((hash =         toupper(s[0]), s[1]) &&
			(hash = hash*3+toupper(s[1]), s[2]) &&
			(hash = hash*2+toupper(s[2]), s[3]) &&
			(hash = hash*2+toupper(s[3]), s[4]) &&
			(hash = hash*2+toupper(s[4]), s[5]) &&
			(hash = hash*2+toupper(s[5]), s[6]) &&
			(hash = hash*2+toupper(s[6]),
			 hash = hash*2+toupper(s[7]))
         );
	return hash;
}

//
// W_HashLumps
//
// killough 1/31/98: Initialize lump hash table
// [SL] taken from prboom-plus
//
void W_HashLumps(void)
{
	for (unsigned int i = 0; i < numlumps; i++)
		lumpinfo[i].index = -1;			// mark slots empty

	// Insert nodes to the beginning of each chain, in first-to-last
	// lump order, so that the last lump of a given name appears first
	// in any chain, observing pwad ordering rules. killough

	for (unsigned int i = 0; i < numlumps; i++)
	{
		unsigned int j = W_LumpNameHash(lumpinfo[i].name) % (unsigned int)numlumps;
		lumpinfo[i].next = lumpinfo[j].index;     // Prepend to list
		lumpinfo[j].index = i;
	}
}


//
// uppercoppy
//
// [RH] Copy up to 8 chars, upper-casing them in the process
//
void uppercopy (char *to, const char *from)
{
	int i;

	for (i = 0; i < 8 && from[i]; i++)
		to[i] = toupper (from[i]);
	for (; i < 8; i++)
		to[i] = 0;
}

/**
 * @brief Calculate a CRC32 hash from a file.
 * 
 * @param filename Filename of file to hash.
 * @return Output hash, or blank if file could not be found.
 */
OCRC32Sum W_CRC32(const std::string& filename)
{
	OCRC32Sum rvo;

	const int file_chunk_size = 8192;
	FILE* fp = fopen(filename.c_str(), "rb");

	if (!fp)
		return rvo;

	unsigned n = 0;
	unsigned char buf[file_chunk_size];
	uint32_t crc = 0;

	while ((n = fread(buf, 1, sizeof(buf), fp)))
	{
		crc = crc32_fast(buf, n, crc);
	}

	std::string hashStr;

	StrFormat(hashStr, "%08X", crc);

	OCRC32Sum::makeFromHexStr(rvo, hashStr);
	return rvo; // bubble up failure
}

// denis - Standard MD5SUM
OMD5Hash W_MD5(const std::string& filename)
{
	OMD5Hash rvo;

	const int file_chunk_size = 8192;
	FILE *fp = fopen(filename.c_str(), "rb");

	if(!fp)
		return rvo;

	md5_state_t state;
	md5_init(&state);

	unsigned n = 0;
	unsigned char buf[file_chunk_size];

	while((n = fread(buf, 1, sizeof(buf), fp)))
		md5_append(&state, (unsigned char *)buf, n);

	md5_byte_t digest[16];
	md5_finish(&state, digest);

	fclose(fp);

	std::stringstream hashStr;

	for(int i = 0; i < 16; i++)
		hashStr << std::setw(2) << std::setfill('0') << std::hex << std::uppercase << (short)digest[i];

	OMD5Hash::makeFromHexStr(rvo, hashStr.str());
	return rvo; // bubble up failure
}

/*
 * @brief Creates a 128-bit fingerprint for a map via FarmHash.
 *
 * However, it encodes the fingerprint into a 16-byte array to be read later.
 *
 * @param lumpdata byte array pointer to the lump (or lumps) that needs to be fingerprinted.
 * @param size of the byte array pointer in bytes.
 * @return fhfprint_s - struct containing 16-byte array of fingerprint.
 */
fhfprint_s W_FarmHash128(const byte* lumpdata, int length)
{
	fhfprint_s fhfngprnt;

	if (!lumpdata)
		return fhfngprnt;

	util::uint128_t fingerprint128 = util::Fingerprint128((const char*)lumpdata, length);

	// Store the bytes of the hashes in the array.
	fhfngprnt.fingerprint[0] = fingerprint128.first >> 8 * 0;
	fhfngprnt.fingerprint[1] = fingerprint128.first >> 8 * 1;
	fhfngprnt.fingerprint[2] = fingerprint128.first >> 8 * 2;
	fhfngprnt.fingerprint[3] = fingerprint128.first >> 8 * 3;
	fhfngprnt.fingerprint[4] = fingerprint128.first >> 8 * 4;
	fhfngprnt.fingerprint[5] = fingerprint128.first >> 8 * 5;
	fhfngprnt.fingerprint[6] = fingerprint128.first >> 8 * 6;
	fhfngprnt.fingerprint[7] = fingerprint128.first >> 8 * 7;

	fhfngprnt.fingerprint[8] = fingerprint128.second >> 8 * 0;
	fhfngprnt.fingerprint[9] = fingerprint128.second >> 8 * 1;
	fhfngprnt.fingerprint[10] = fingerprint128.second >> 8 * 2;
	fhfngprnt.fingerprint[11] = fingerprint128.second >> 8 * 3;
	fhfngprnt.fingerprint[12] = fingerprint128.second >> 8 * 4;
	fhfngprnt.fingerprint[13] = fingerprint128.second >> 8 * 5;
	fhfngprnt.fingerprint[14] = fingerprint128.second >> 8 * 6;
	fhfngprnt.fingerprint[15] = fingerprint128.second >> 8 * 7;

	return fhfngprnt;
}

//
// LUMP BASED ROUTINES.
//

//
// W_AddLumps
//
// Adds lumps from the array of filelump_t. If clientonly is true,
// only certain lumps will be added.
//
void W_AddLumps(FILE* handle, filelump_t* fileinfo, size_t newlumps, bool clientonly)
{
	lumpinfo = (lumpinfo_t*)Realloc(lumpinfo, (numlumps + newlumps) * sizeof(lumpinfo_t));
	if (!lumpinfo)
		I_Error("Couldn't realloc lumpinfo");

	lumpinfo_t* lump = &lumpinfo[numlumps];
	filelump_t* info = &fileinfo[0];

	for (size_t i = 0; i < newlumps; i++, info++)
	{
		lump->handle = handle;
		lump->position = info->filepos;
		lump->size = info->size;
		strncpy(lump->name, info->name, 8);

		lump++;
		numlumps++;
	}
}


//
// W_AddFile
//
// All files are optional, but at least one file must be found
// (PWAD, if all required lumps are present).
// Files with a .wad extension are wadlink files with multiple lumps.
// Other files are single lumps with the base filename for the lump name.
//
// Map reloads are supported through WAD reload so no need for vanilla tilde
// reload hack here
//
void AddFile(const OResFile& file)
{
	FILE*			handle;
	filelump_t*		fileinfo;

	const std::string filename = file.getFullpath();

	if ( (handle = fopen(filename.c_str(), "rb")) == NULL)
	{
		Printf(PRINT_WARNING, "couldn't open %s\n", filename.c_str());
		return;
	}

	Printf(PRINT_HIGH, "adding %s", filename.c_str());

	size_t newlumps;

	wadinfo_t header;
	size_t readlen = fread(&header, sizeof(header), 1, handle);
	if ( readlen < 1 )
	{
		Printf(PRINT_HIGH, "failed to read %s.\n", filename.c_str());
		fclose(handle);
		return;
	}
	header.identification = LELONG(header.identification);

	if (header.identification != IWAD_ID && header.identification != PWAD_ID)
	{
		// raw lump file
		std::string lumpname;
		M_ExtractFileBase(filename, lumpname);

		fileinfo = new filelump_t[1];	
		fileinfo->filepos = 0;
		fileinfo->size = M_FileLength(handle);
		std::transform(lumpname.c_str(), lumpname.c_str() + 8, fileinfo->name, toupper);

		newlumps = 1;
		Printf(PRINT_HIGH, " (single lump)\n");
	}
	else
	{
		// WAD file
		header.numlumps = LELONG(header.numlumps);
		header.infotableofs = LELONG(header.infotableofs);
		size_t length = header.numlumps * sizeof(filelump_t);

		if (length > (unsigned)M_FileLength(handle))
		{
			Printf(PRINT_WARNING, "\nbad number of lumps for %s\n", filename.c_str());
			fclose(handle);
			return;
		}

		fileinfo = new filelump_t[header.numlumps];
		fseek(handle, header.infotableofs, SEEK_SET);
		readlen = fread(fileinfo, length, 1, handle);
		if (readlen < 1)
		{
			Printf(PRINT_HIGH, "failed to read file info in %s\n", filename.c_str());
			fclose(handle);
			return;
		}

		// convert from little-endian to target arch and capitalize lump name
		for (int i = 0; i < header.numlumps; i++)
		{
			fileinfo[i].filepos = LELONG(fileinfo[i].filepos);
			fileinfo[i].size = LELONG(fileinfo[i].size);
			std::transform(fileinfo[i].name, fileinfo[i].name + 8, fileinfo[i].name, toupper);
		}

		newlumps = header.numlumps;	
		Printf(PRINT_HIGH, " (%d lumps)\n", header.numlumps);
	}

	W_AddLumps(handle, fileinfo, newlumps, false);

	delete [] fileinfo;

	return;
}


//
//
// IsMarker
//
// (from BOOM)
//
//

static BOOL IsMarker (const lumpinfo_t *lump, const char *marker)
{
	return (lump->namespc == ns_global) && (!strncmp (lump->name, marker, 8) ||
			(*(lump->name) == *marker && !strncmp (lump->name + 1, marker, 7)));
}

//
// W_MergeLumps
//
// Merge multiple tagged groups into one
// Basically from BOOM, too, although I tried to write it independently.
//

void W_MergeLumps (const char *start, const char *end, int space)
{
	char ustart[8], uend[8];
	lumpinfo_t *newlumpinfos;
	unsigned newlumps, oldlumps;
	BOOL insideBlock;
	unsigned flatHack, i;

	strncpy(ustart, start, 8);
	strncpy(uend, end, 8);

	std::transform(ustart, ustart + sizeof(ustart), ustart, toupper);
	std::transform(uend, uend + sizeof(uend), uend, toupper);

	// Some pwads use an icky hack to get flats with regular Doom.
	// This tries to detect them.
	flatHack = 0;
	if (!strcmp ("F_START", ustart) && !Args.CheckParm ("-noflathack"))
	{
		int fudge = 0;
		unsigned start = 0;

		for (i = 0; i < numlumps; i++) {
			if (IsMarker (lumpinfo + i, ustart))
				fudge++, start = i;
			else if (IsMarker (lumpinfo + i, uend))
				fudge--, flatHack = i;
		}
		if (start > flatHack)
			fudge--;
		if (fudge >= 0)
			flatHack = 0;
	}

	newlumpinfos = new lumpinfo_t[numlumps];

	newlumps = 0;
	oldlumps = 0;
	insideBlock = false;

	for (i = 0; i < numlumps; i++)
	{
		if (!insideBlock)
		{
			// Check if this is the start of a block
			if (IsMarker (lumpinfo + i, ustart))
			{
				insideBlock = true;

				// Create start marker if we haven't already
				if (!newlumps)
				{
					newlumps++;
					strncpy (newlumpinfos[0].name, ustart, 8);
					newlumpinfos[0].handle = NULL;
					newlumpinfos[0].position =
						newlumpinfos[0].size = 0;
					newlumpinfos[0].namespc = ns_global;
				}
			}
			else
			{
				// Copy lumpinfo down this list
				lumpinfo[oldlumps++] = lumpinfo[i];
			}
		}
		else
		{
			// Check if this is the end of a block
			if (flatHack)
			{
				if (flatHack == i)
				{
					insideBlock = false;
					flatHack = 0;
				}
				else
				{
					if (lumpinfo[i].size != 4096)
					{
						lumpinfo[oldlumps++] = lumpinfo[i];
					}
					else
					{
						newlumpinfos[newlumps] = lumpinfo[i];
						newlumpinfos[newlumps++].namespc = space;
					}
				}
			}
			else if (i && lumpinfo[i].handle != lumpinfo[i-1].handle)
			{
				// Blocks cannot span multiple files
				insideBlock = false;
				lumpinfo[oldlumps++] = lumpinfo[i];
			}
			else if (IsMarker (lumpinfo + i, uend))
			{
				// It is. We'll add the end marker once
				// we've processed everything.
				insideBlock = false;
			}
			else
			{
				newlumpinfos[newlumps] = lumpinfo[i];
				newlumpinfos[newlumps++].namespc = space;
			}
		}
	}

	// Now copy the merged lumps to the end of the old list
	// and create the end marker entry.

	if (newlumps)
	{
		if (oldlumps + newlumps > numlumps)
			lumpinfo = (lumpinfo_t *)Realloc (lumpinfo, oldlumps + newlumps);

		memcpy (lumpinfo + oldlumps, newlumpinfos, sizeof(lumpinfo_t) * newlumps);

		numlumps = oldlumps + newlumps;

		strncpy (lumpinfo[numlumps].name, uend, 8);
		lumpinfo[numlumps].handle = NULL;
		lumpinfo[numlumps].position =
			lumpinfo[numlumps].size = 0;
		lumpinfo[numlumps].namespc = ns_global;
		numlumps++;
	}

	delete[] newlumpinfos;
}

//
// W_InitMultipleFiles
// Pass a null terminated list of files to use.
// All files are optional, but at least one file
//  must be found.
// Files with a .wad extension are idlink files
//  with multiple lumps.
// Other files are single lumps with the base filename
//  for the lump name.
// Lump names can appear multiple times.
// The name searcher looks backwards, so a later file
//  does override all earlier ones.
//
void W_InitMultipleFiles(const OResFiles& files)
{
	// open all the files, load headers, and count lumps
	// will be realloced as lumps are added
	::numlumps = 0;

	M_Free(::lumpinfo);
	::lumpinfo = NULL;

	// open each file once, load headers, and count lumps
	std::vector<OMD5Hash> loaded;
	for (size_t i = 0; i < files.size(); i++)
	{
		if (std::find(loaded.begin(), loaded.end(), files.at(i).getMD5()) ==
		    loaded.end())
		{
			AddFile(files.at(i));
			loaded.push_back(files.at(i).getMD5());
		}
	}

	if (!numlumps)
		I_Error ("W_InitFiles: no files found");

	// [RH] Set namespace markers to global for everything
	for (size_t i = 0; i < numlumps; i++)
		lumpinfo[i].namespc = ns_global;

	// [RH] Merge sprite and flat groups.
	//		(We don't need to bother with patches, since
	//		Doom doesn't use markers to identify them.)
	W_MergeLumps ("S_START", "S_END", ns_sprites); // denis - fixme - security
	W_MergeLumps ("F_START", "F_END", ns_flats);
	W_MergeLumps ("C_START", "C_END", ns_colormaps);

    // set up caching
	M_Free(lumpcache);

	size_t size = numlumps * sizeof(*lumpcache);
	lumpcache = (void **)Malloc (size);

	if (!lumpcache)
		I_Error ("Couldn't allocate lumpcache");

	memset (lumpcache,0, size);

	// killough 1/31/98: initialize lump hash table
	W_HashLumps();

	stdisk_lumpnum = W_GetNumForName("STDISK");
}

/**
 * @brief Return a handle for a given lump.
 */
lumpHandle_t W_LumpToHandle(const unsigned lump)
{
	lumpHandle_t rvo;
	size_t id = static_cast<size_t>(lump) << HANDLE_GEN_BITS;
	rvo.id = id | ::handleGen;
	return rvo;
}

/**
 * @brief Return a lump for a given handle, or -1 if the handle is invalid.
 */
int W_HandleToLump(const lumpHandle_t handle)
{
	size_t gen = handle.id & HANDLE_GEN_MASK;
	if (gen != ::handleGen)
	{
		return -1;
	}
	const unsigned lump = handle.id >> HANDLE_GEN_BITS;
	if (lump >= ::numlumps)
	{
		return -1;
	}
	return lump;
}

//
// W_CheckNumForName
// Returns -1 if name not found.
//
// Rewritten by Lee Killough to use hash table for performance. Significantly
// cuts down on time -- increases Doom performance over 300%. This is the
// single most important optimization of the original Doom sources, because
// lump name lookup is used so often, and the original Doom used a sequential
// search. For large wads with > 1000 lumps this meant an average of over
// 500 were probed during every search. Now the average is under 2 probes per
// search. There is no significant benefit to packing the names into longwords
// with this new hashing algorithm, because the work to do the packing is
// just as much work as simply doing the string comparisons with the new
// algorithm, which minimizes the expected number of comparisons to under 2.
//
// [SL] taken from prboom-plus
//
int W_CheckNumForName(const char *name, int namespc)
{
	// Hash function maps the name to one of possibly numlump chains.
	// It has been tuned so that the average chain length never exceeds 2.

	// proff 2001/09/07 - check numlumps==0, this happens when called before WAD loaded
	register int i = (numlumps==0)?(-1):(lumpinfo[W_LumpNameHash(name) % numlumps].index);

	// We search along the chain until end, looking for case-insensitive
	// matches which also match a namespace tag. Separate hash tables are
	// not used for each namespace, because the performance benefit is not
	// worth the overhead, considering namespace collisions are rare in
	// Doom wads.

	while (i >= 0 && (strnicmp(lumpinfo[i].name, name, 8) ||
				lumpinfo[i].namespc != namespc))
		i = lumpinfo[i].next;

	// Return the matching lump, or -1 if none found.
	return i;
}

//
// W_GetNumForName
// Calls W_CheckNumForName, but bombs out if not found.
//
int W_GetNumForName(const char* name, int namespc)
{
	int i = W_CheckNumForName(name, namespc);

	if (i == -1)
	{
		I_Error("W_GetNumForName: %s not found!\n(checked in: %s)", name,
		        M_ResFilesToString(::wadfiles).c_str());
	}

	return i;
}

//
// W_GetNumForName
// Calls W_CheckNumForName, but bombs out if not found.
//
int W_GetNumForName(OLumpName& name, int namespc)
{
	return W_GetNumForName(name.c_str(), namespc);
}

/**
 * @brief Return the name of a lump number.
 * 
 * @detail You likely only need this for debugging, since a name can be
 *         ambiguous.
 */
std::string W_LumpName(unsigned lump)
{
	if (lump >= ::numlumps)
		I_Error("%s: %i >= numlumps", __FUNCTION__, lump);

	return std::string(::lumpinfo[lump].name, ARRAY_LENGTH(::lumpinfo[lump].name));
}

//
// W_LumpLength
// Returns the buffer size needed to load the given lump.
//
unsigned W_LumpLength (unsigned lump)
{
	if (lump >= numlumps)
		I_Error ("W_LumpLength: %i >= numlumps",lump);

	return lumpinfo[lump].size;
}



//
// W_ReadLump
// Loads the lump into the given buffer,
//  which must be >= W_LumpLength().
//
void W_ReadLump(unsigned int lump, void* dest)
{
	int		c;
	lumpinfo_t*	l;

	if (lump >= numlumps)
		I_Error ("W_ReadLump: %i >= numlumps",lump);

	l = lumpinfo + lump;

	if (lump != stdisk_lumpnum)
    	I_BeginRead();

	fseek (l->handle, l->position, SEEK_SET);
	c = fread (dest, l->size, 1, l->handle);

	if (feof(l->handle))
		I_Error ("W_ReadLump: only read %i of %i on lump %i", c, l->size, lump);

	if (lump != stdisk_lumpnum)
    	I_EndRead();
}

//
// W_ReadChunk
//
// denis - for wad downloading
//
unsigned W_ReadChunk (const char *file, unsigned offs, unsigned len, void *dest, unsigned &filelen)
{
	FILE *fp = fopen(file, "rb");
	unsigned read = 0;

	if(fp)
	{
		filelen = M_FileLength(fp);

		fseek(fp, offs, SEEK_SET);
		read = fread(dest, 1, len, fp);
		fclose(fp);
	}
	else filelen = 0;

	return read;
}


//
// W_CheckLumpName
//
bool W_CheckLumpName (unsigned lump, const char *name)
{
	if (lump >= numlumps)
		return false;

	return !strnicmp (lumpinfo[lump].name, name, 8);
}

//
// W_GetLumpName
//
void W_GetLumpName(char *to, unsigned lump)
{
	if (lump >= numlumps)
		*to = 0;
	else
	{
		memcpy (to, lumpinfo[lump].name, 8); // denis - todo -string limit?
		to[8] = '\0';
		std::transform(to, to + strlen(to), to, toupper);
	}
}

//
// W_GetLumpName
//
void W_GetOLumpName(OLumpName& to, unsigned lump)
{
	if (lump >= numlumps)
		to.clear();
	else
		to = lumpinfo[lump].name;
}

//
// W_CacheLumpNum
//
void* W_CacheLumpNum(unsigned int lump, const zoneTag_e tag)
{
	if ((unsigned)lump >= numlumps)
		I_Error ("W_CacheLumpNum: %i >= numlumps",lump);

	if (!lumpcache[lump])
	{
		// read the lump in
		// [RH] Allocate one byte more than necessary for the
		//		lump and set the extra byte to zero so that
		//		various text parsing routines can just call
		//		W_CacheLumpNum() and not choke.

		//DPrintf("cache miss on lump %i\n",lump);
		unsigned int lump_length = W_LumpLength(lump);
		lumpcache[lump] = (byte *)Z_Malloc(lump_length + 1, tag, &lumpcache[lump]);
		W_ReadLump(lump, lumpcache[lump]);
		*((unsigned char*)lumpcache[lump] + lump_length) = 0;
	}
	else
	{
		//printf ("cache hit on lump %i\n",lump);
		Z_ChangeTag(lumpcache[lump],tag);
	}

	return lumpcache[lump];
}

//
// W_CacheLumpName
//
void* W_CacheLumpName(const char* name, const zoneTag_e tag)
{
	return W_CacheLumpNum(W_GetNumForName(name), tag);
}

//
// W_CacheLumpName
//
void* W_CacheLumpName(OLumpName& name, const zoneTag_e tag)
{
	return W_CacheLumpNum(W_GetNumForName(name.c_str()), tag);
}

size_t R_CalculateNewPatchSize(patch_t *patch, size_t length);
void R_ConvertPatch(patch_t* rawpatch, patch_t* newpatch, const unsigned int lumpnum);

//
// W_CachePatch
//
// [SL] Reads and caches a patch from disk. This takes care of converting the
// patch from the standard Doom format of posts with 1-byte lengths and offsets
// to a new format for posts that uses 2-byte lengths and offsets.
//
patch_t* W_CachePatch(unsigned lumpnum, const zoneTag_e tag)
{
	if (lumpnum >= numlumps)
		I_Error ("W_CachePatch: %u >= numlumps", lumpnum);

	if (!lumpcache[lumpnum])
	{
		// temporary storage of the raw patch in the old format
		byte *rawlumpdata = new byte[W_LumpLength(lumpnum)];

		W_ReadLump(lumpnum, rawlumpdata);
		patch_t *rawpatch = (patch_t*)(rawlumpdata);

		size_t newlumplen = R_CalculateNewPatchSize(rawpatch, W_LumpLength(lumpnum));

		if (newlumplen > 0)
		{
			// valid patch
			lumpcache[lumpnum] = (byte *)Z_Malloc(newlumplen + 1, tag, &lumpcache[lumpnum]);
			patch_t *newpatch = (patch_t*)lumpcache[lumpnum];
			*((unsigned char*)lumpcache[lumpnum] + newlumplen) = 0;

			R_ConvertPatch(newpatch, rawpatch, lumpnum);
		}
		else
		{
			// invalid patch - just create a header with width = 0, height = 0
			lumpcache[lumpnum] = Z_Malloc(sizeof(patch_t), tag, &lumpcache[lumpnum]);
			memset(lumpcache[lumpnum], 0, sizeof(patch_t));
		}

		delete [] rawlumpdata;
	}
	else
	{
		Z_ChangeTag(lumpcache[lumpnum], tag);
	}

	// denis - todo - would be good to check whether the patch violates W_LumpLength here
	// denis - todo - would be good to check for width/height == 0 here, and maybe replace those with a valid patch

	return (patch_t*)lumpcache[lumpnum];
}

patch_t* W_CachePatch(const char* name, const zoneTag_e tag)
{
	return W_CachePatch(W_GetNumForName(name), tag);
	// denis - todo - would be good to replace non-existant patches with a default '404' patch
}

patch_t* W_CachePatch(OLumpName& name, const zoneTag_e tag)
{
	return W_CachePatch(W_GetNumForName(name.c_str()), tag);
	// denis - todo - would be good to replace non-existant patches with a default '404'
	// patch
}

/**
 * @brief Cache a patch by lump number and return a handle to it.
 */
lumpHandle_t W_CachePatchHandle(const int lumpNum, const zoneTag_e tag)
{
	W_CachePatch(lumpNum, tag);
	return W_LumpToHandle(lumpNum);
}

/**
 * @brief Cache a patch by name and namespace and return a handle to it.
 */
lumpHandle_t W_CachePatchHandle(const char* name, const zoneTag_e tag, const int ns)
{
	return W_CachePatchHandle(W_GetNumForName(name, ns), tag);
}

/**
 * @brief Resolve a handle into a patch_t, or an empty patch if the
 *        lump was missing or from a previous generation.
 */
patch_t* W_ResolvePatchHandle(const lumpHandle_t lump)
{
	int lumpnum = W_HandleToLump(lump);
	if (lumpnum == -1)
	{
		static patch_t empty;
		memset(&empty, 0, sizeof(patch_t));
		return &empty;
	}
	return static_cast<patch_t*>(lumpcache[lumpnum]);
}

//
// W_FindLump
//
// Find a named lump. Specifically allows duplicates for merging of e.g.
// SNDINFO lumps.
//
// [SL] 2013-01-15 - Search forwards through the list of lumps in reverse pwad
// ordering, returning older lumps with a matching name first.
// Initialize search with lastlump == -1 before calling for the first time.
//
int W_FindLump (const char *name, int lastlump)
{
	if (lastlump < -1)
		lastlump = -1;

	for (int i = lastlump + 1; i < (int)numlumps; i++)
	{
		if (strnicmp(lumpinfo[i].name, name, 8) == 0)
			return i;
	}

	return -1;
}

//
// W_Close
//
// Close all open files
//

void W_Close ()
{
	// store closed handles, so that fclose isn't called multiple times
	// for the same handle
	std::vector<FILE *> handles;

	lumpinfo_t * lump_p = lumpinfo;
	while (lump_p < lumpinfo + numlumps)
	{
		// if file not previously closed, close it now
		if(lump_p->handle &&
		   std::find(handles.begin(), handles.end(), lump_p->handle) == handles.end())
		{
			fclose(lump_p->handle);
			handles.push_back(lump_p->handle);
		}
		lump_p++;
	}

	::handleGen = (::handleGen + 1) & HANDLE_GEN_MASK;
	if (::handleGen == 0)
	{
		// 0 is reserved for the NULL handle.
		::handleGen += 1;
	}
}

VERSION_CONTROL (w_wad_cpp, "$Id$")
