// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
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


#ifdef UNIX
#include <ctype.h>
#include <string.h>
#include <unistd.h>
#ifndef O_BINARY
#define O_BINARY		0
#endif
#endif

#ifdef WIN32
#include <io.h>
#else
#define strcmpi	strcasecmp
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <ctype.h>

#include "doomtype.h"
#include "m_swap.h"
#include "i_system.h"
#include "z_zone.h"
#include "cmdlib.h"
#include "m_argv.h"
#include "md5.h"

#include "w_wad.h"


#include <string>
#include <sstream>
#include <algorithm>
#include <vector>
#include <iomanip>



//
// GLOBALS
//

// Location of each lump on disk.
lumpinfo_t*		lumpinfo;		
size_t			numlumps;

void**			lumpcache;

//
// W_IsCommercial
// denis - commercial wads will need download restrictions
//
bool W_IsCommercial(std::string &name, std::string &hash)
{
	if(
	   name == "DOOM.WAD"	|| hash == "C4FE9FD920207691A9F493668E0A2083" ||
	   name == "DOOM2.WAD"	|| hash == "25E1459CA71D321525F84628F45CA8CD" ||
	   name == "TNT.WAD"	|| hash == "4E158D9953C79CCF97BD0663244CC6B6" ||
	   name == "PLUTONIA.WAD"	|| hash == "75C8CF89566741FA9D22447604053BD7"
	   )
		return true;
	else
		return false;
}

//
// W_Length
// denis - used to be filelength
//
static int W_Length (int handle)
{
	struct stat fileinfo;
	
	if (fstat (handle, &fileinfo) == -1)
	{
		close (handle);
		I_Error ("Error fstating");
	}
	return fileinfo.st_size;
}


void
ExtractFileBase
( const char*		path,
 char*			dest )
{
	int		length;
	
	const char *src = path + strlen(path) - 1;
    
    // back up until a \ or the start
	while (src != path
		      && *(src-1) != '\\'
			  && *(src-1) != '/')
	{
		src--;
	}
    
    // copy up to eight characters
	memset (dest,0,8);
	length = 0;
    
	while (*src && *src != '.')
	{
		if (++length == 9)
			I_Error ("Filename base of %s >8 chars",path);
		
		*dest++ = toupper((int)*src++);
	}
}

// denis - Standard MD5SUM
std::string W_MD5(std::string filename)
{
	const int file_chunk_size = 8192;
	FILE *fp = fopen(filename.c_str(), "rb");
	
	if(!fp)
		return "";
	
	md5_state_t state;
	md5_init(&state);
	
	unsigned n = 0;
	unsigned char buf[file_chunk_size];
	
	while((n = fread(buf, 1, sizeof(buf), fp)))
		md5_append(&state, (unsigned char *)buf, n);
	
	md5_byte_t digest[16];
	md5_finish(&state, digest);
	
	fclose(fp);
	
	std::stringstream hash;
	
	for(int i = 0; i < 16; i++)
		hash << std::setw(2) << std::setfill('0') << std::hex << std::uppercase << (short)digest[i];
	
	return hash.str().c_str();
}


//
// LUMP BASED ROUTINES.
//

//
// W_AddFile
// All files are optional, but at least one file must be
//  found (PWAD, if all required lumps are present).
// Files with a .wad extension are wadlink files
//  with multiple lumps.
// Other files are single lumps with the base filename
//  for the lump name.
//
// Map reloads are supported through WAD reload
// so no need for vanilla tilde reload hack here
//

std::string W_AddFile (std::string filename)
{
	wadinfo_t		header;
	lumpinfo_t*		lump_p;
	size_t			i;
	int				handle;
	size_t			length;
	size_t			startlump;
	filelump_t*		fileinfo;
	filelump_t		singleinfo;
    
	FixPathSeparator (filename);
	std::string name = filename;
	DefaultExtension (name, ".wad");

    // open the file
	if ( (handle = open (filename.c_str(), O_RDONLY | O_BINARY)) == -1)
	{
		Printf (PRINT_HIGH, " couldn't open %s\n", filename.c_str());
		return "";
	}
	
	Printf (PRINT_HIGH, "adding %s\n", filename.c_str());

	startlump = numlumps;

	read (handle, &header, sizeof(header));
	header.identification = LONG(header.identification);
	
	if (header.identification != IWAD_ID && header.identification != PWAD_ID)
	{
		// raw lump file
		fileinfo = &singleinfo;
		singleinfo.filepos = 0;
		singleinfo.size = W_Length(handle);
		ExtractFileBase (filename, name);
		numlumps++;
		Printf (PRINT_HIGH, " (single lump)\n", header.numlumps);
	}
	else 
	{
		// WAD file
		header.numlumps = LONG(header.numlumps);
		header.infotableofs = LONG(header.infotableofs);
		length = header.numlumps*sizeof(filelump_t);
		fileinfo = (filelump_t *)Z_Malloc (length, PU_STATIC, 0);
		lseek (handle, header.infotableofs, SEEK_SET);
		read (handle, fileinfo, length);
		numlumps += header.numlumps;
		Printf (PRINT_HIGH, " (%d lumps)\n", header.numlumps);
	}
    
    // Fill in lumpinfo
	lumpinfo = (lumpinfo_t *)Realloc (lumpinfo, numlumps*sizeof(lumpinfo_t));
	
	if (!lumpinfo)
		I_Error ("Couldn't realloc lumpinfo");
	
	lump_p = &lumpinfo[startlump];
	
	for (i=startlump ; i<numlumps ; i++,lump_p++, fileinfo++)
	{
		lump_p->handle = handle;
		lump_p->position = LONG(fileinfo->filepos);
		lump_p->size = LONG(fileinfo->size);
		strncpy (lump_p->name, fileinfo->name, 8);
	}
	
	return W_MD5(filename);
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
					newlumpinfos[0].handle = -1;
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
		lumpinfo[numlumps].handle = -1;
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
std::vector<std::string> W_InitMultipleFiles (std::vector<std::string> &filenames)
{	
	size_t		size, i;
    
    // open all the files, load headers, and count lumps
    // will be realloced as lumps are added
	numlumps = 0;
	
	if(lumpinfo)
	{
		free(lumpinfo);
		lumpinfo = 0;	
	}
	
	using namespace std;
	vector<string> hashes(filenames);
	
	// no dupes
	filenames.erase(unique(filenames.begin(), filenames.end()), filenames.end());
	
	// open all the files, load headers, and count lumps
	for(i = 0; i < filenames.size(); i++)
		hashes[i] = W_AddFile(filenames[i].c_str());
	
	if (!numlumps)
		I_Error ("W_InitFiles: no files found");

	// [RH] Set namespace markers to global for everything
	for (i = 0; i < numlumps; i++)
		lumpinfo[i].namespc = ns_global;
	
	// [RH] Merge sprite and flat groups.
	//		(We don't need to bother with patches, since
	//		Doom doesn't use markers to identify them.)
	W_MergeLumps ("S_START", "S_END", ns_sprites); // denis - fixme - security
	W_MergeLumps ("F_START", "F_END", ns_flats);
	W_MergeLumps ("C_START", "C_END", ns_colormaps);

    // set up caching
	if(lumpcache)
		free(lumpcache);

	size = numlumps * sizeof(*lumpcache);
	lumpcache = (void **)Malloc (size);
    
	if (!lumpcache)
		I_Error ("Couldn't allocate lumpcache");
	
	memset (lumpcache,0, size);
	
	return hashes;
}




//
// W_InitFile
// Just initialize from a single file.
//
void W_InitFile (char* filename)
{
	char*	names[2];
	
	names[0] = filename;
	names[1] = NULL;
	
	std::vector<std::string> v;
	v.push_back(filename);
	W_InitMultipleFiles (v);
}



//
// W_NumLumps
//
int W_NumLumps (void)
{
	return numlumps;
}



//
// W_CheckNumForName
// Returns -1 if name not found.
//

int W_CheckNumForName (const char* name, int namespc)
{
	union {
		char	s[9];
		int	x[2];
		
	} name8;
    
	int		v1;
	int		v2;
	lumpinfo_t*	lump_p;
	
    // make the name into two integers for easy compares
	strncpy (name8.s,name,9);
	
    // in case the name was a fill 8 chars
	name8.s[8] = 0;
	
    // case insensitive
	std::transform(name8.s, name8.s + strlen(name8.s), name8.s, toupper);
	
	v1 = name8.x[0];
	v2 = name8.x[1];
	
	
    // scan backwards so patch lump files take precedence
	lump_p = lumpinfo + numlumps;
	
	while (lump_p-- != lumpinfo)
	{
		if ( *(int *)lump_p->name == v1
			&& *(int *)&lump_p->name[4] == v2 && lump_p->namespc == namespc)
		{
			return lump_p - lumpinfo;
		}
	}
	
    // TFB. Not found.
	return -1;
}




//
// W_GetNumForName
// Calls W_CheckNumForName, but bombs out if not found.
//
int W_GetNumForName (const char* name)
{
	int	i;
	
	i = W_CheckNumForName (name);
    
	if (i == -1)
		I_Error ("W_GetNumForName: %s not found!", name);
	
	return i;
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
void
W_ReadLump
( unsigned	lump,
 void*		dest )
{
	int		c;
	lumpinfo_t*	l;

	if (lump >= numlumps)
		I_Error ("W_ReadLump: %i >= numlumps",lump);
	
	l = lumpinfo + lump;
	
	lseek (l->handle, l->position, SEEK_SET);
	c = read (l->handle, dest, l->size);
	
	if (c < l->size)
		I_Error ("W_ReadLump: only read %i of %i on lump %i", c, l->size, lump);	
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
		fseek(fp, 0, SEEK_END);
		filelen = ftell(fp);

		fseek(fp, offs, SEEK_SET);
		read = fread(dest, 1, len, fp);
		fclose(fp);
	}
	else filelen = 0;

	return read;
}

//
// W_GetLumpName
//
void W_GetLumpName (char *to, unsigned  lump)
{
	if (lump >= numlumps)
		*to = 0;
	else
	{
		strncpy (to, lumpinfo[lump].name, 9); // denis - todo -string limit?
		std::transform(to, to + strlen(to), to, toupper);
	}
}

//
// W_CacheLumpNum
//
void*
W_CacheLumpNum
( unsigned	lump,
 int		tag )
{
	byte*	ptr;
	
	if ((unsigned)lump >= numlumps)
		I_Error ("W_CacheLumpNum: %i >= numlumps",lump);
	
	if (!lumpcache[lump])
	{
		// read the lump in
		
		//printf ("cache miss on lump %i\n",lump);
		ptr = (byte *)Z_Malloc (W_LumpLength (lump) + 1, tag, &lumpcache[lump]);
		W_ReadLump (lump, lumpcache[lump]);
		ptr [W_LumpLength (lump)] = 0;
	}
	else
	{
		//printf ("cache hit on lump %i\n",lump);
		Z_ChangeTag (lumpcache[lump],tag);
	}
	
	return lumpcache[lump];
}

//
// W_CacheLumpName
//
void*
W_CacheLumpName
( const char*		name,
 int		tag )
{
	return W_CacheLumpNum (W_GetNumForName(name), tag);
}

//
// W_CachePatch
//
patch_t* W_CachePatch
( unsigned	lump )
{
	patch_t *patch = (patch_t *)W_CacheLumpNum (lump, PU_CACHE);
	
	// denis - todo - would be good to check whether the patch violates W_LumpLength here
	// denis - todo - would be good to check for width/height == 0 here, and maybe replace those with a valid patch
	
	return patch;
}

patch_t* W_CachePatch
( const char*		name)
{
	return W_CachePatch(W_GetNumForName(name)); // denis - todo - would be good to replace non-existant patches with a default '404' patch
}

//
// W_FindLump
//
// Find a named lump. Specifically allows duplicates for merging of e.g.
// SNDINFO lumps.
//

int W_FindLump (const char *name, int *lastlump)
{
	char name8[9];
	int v1, v2;
	lumpinfo_t *lump_p;
	
	// make the name into two integers for easy compares
	strncpy (name8, name, 8); // denis - todo -string limit?
	std::transform(name8, name8 + 8, name8, toupper);
	
	v1 = *(int *)name8;
	v2 = *(int *)&name8[4];
	
	lump_p = lumpinfo + *lastlump;
	while (lump_p < lumpinfo + numlumps)
	{
		if (*(int *)lump_p->name == v1 && *(int *)&lump_p->name[4] == v2)
		{
			int lump = lump_p - lumpinfo;
			*lastlump = lump + 1;
			return lump;
		}
		lump_p++;
	}
	
	*lastlump = numlumps;
	return -1;
}


VERSION_CONTROL (w_wad_cpp, "$Id$")

