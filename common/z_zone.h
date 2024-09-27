// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 2006-2024 by The Odamex Team.
//
// This source is available for distribution and/or modification
// only under the terms of the DOOM Source Code License as
// published by id Software. All rights reserved.
//
// The source is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// FITNESS FOR A PARTICULAR PURPOSE. See the DOOM Source Code License
// for more details.
//
// DESCRIPTION:
//      Zone Memory Allocation, perhaps NeXT ObjectiveC inspired.
//	Remark: this was the only stuff that, according
//	 to John Carmack, might have been useful for
//	 Quake.
//
//---------------------------------------------------------------------


#pragma once


//
// ZONE MEMORY
// PU - purge tags.
// Tags < 100 are not overwritten until freed.
enum zoneTag_e
{
	PU_FREE = 0,             // a free block [ML] 12/4/06: Readded from Chocodoom
	PU_STATIC = 1,           // static entire execution time
	PU_SOUND = 2,            // static while playing
	PU_MUSIC = 3,            // static while playing
	PU_LEVEL = 50,           // static until level exited
	PU_LEVSPEC = 51,         // a special thinker in a level
	PU_LEVACS = 52,          // [RH] An ACS script in a level
	PU_LEVELMAX = PU_LEVACS, // Maximum level-specific tag
	PU_PURGELEVEL = 100,     // Level-based tag that can be purged anytime.
	PU_CACHE = 101,          // Generic purge-anytime tag.
};

void Z_Init();
void Z_Close();
void Z_FreeTags(const zoneTag_e lowtag, const zoneTag_e hightag);
void Z_DumpHeap(const zoneTag_e lowtag, const zoneTag_e hightag);

// Don't use these, use the macros instead!
void* Z_Malloc2(size_t size, const zoneTag_e tag, void* user, const char* file,
                const int line);
void Z_Free2(void* ptr, const char* file, int line);
void Z_Discard2(void** ptr, const char* file, int line);
void Z_ChangeTag2(void* ptr, const zoneTag_e tag, const char* file, int line);
void Z_ChangeOwner2(void* ptr, void* user, const char* file, int line);

typedef struct memblock_s
{
	size_t 				size;	// including the header and possibly tiny fragments
	void**				user;	// NULL if a free block
	int 				tag;	// PU_FREE if this is free  [ML] 12/4/06: Readded from Chocodoom
	int 				id; 	// should be ZONEID
	struct memblock_s*	next;
	struct memblock_s*	prev;
} memblock_t;

inline void Z_ChangeTag2(const void* ptr, const zoneTag_e tag, const char* file, int line)
{
	Z_ChangeTag2(const_cast<void *>(ptr), tag, file, line);
}

/**
 * @brief Discard a piece of memory from the heap without freeing it.
 *
 * @param ptr A pointer to the pointer we want to discard.  The pointer must
 *            point to something, but the pointed-to-pointer can be null,
 *            in which case nothing happens.
 * @param file Filename passed in from __FILE__ macro.
 * @param line Line number passed in from __LINE__ macro.
 */
template <typename P>
inline void Z_Discard2(P ptr, const char* file, int line)
{
	if (*ptr == NULL)
	{
		return;
	}

	Z_ChangeTag2(*ptr, PU_CACHE, file, line);
	*ptr = NULL;
}

//
// This is used to get the local FILE:LINE info from CPP
// prior to really calling the function in question.
//
#define Z_ChangeTagSafe(p,t) \
{ \
      if (( (memblock_t *)( (char *)(p) - sizeof(memblock_t)))->tag > t) \
      Z_ChangeTag (p,t); \
}

#define Z_Malloc(s,t,p) Z_Malloc2(s,t,p,__FILE__,__LINE__)
#define Z_Free(p) Z_Free2(p,__FILE__,__LINE__)
#define Z_Discard(p) Z_Discard2(p,__FILE__,__LINE__)
#define Z_ChangeTag(p,t) Z_ChangeTag2(p,t,__FILE__,__LINE__)
#define Z_ChangeOwner(p,u) Z_ChangeOwner2(p,u,__FILE__,__LINE__)
