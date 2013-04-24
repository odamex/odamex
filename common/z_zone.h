// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 2006-2013 by The Odamex Team.
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


#ifndef __Z_ZONE_H__
#define __Z_ZONE_H__

#include <stdio.h>

//
// ZONE MEMORY
// PU - purge tags.
// Tags < 100 are not overwritten until freed.
#define PU_STATIC				1		// static entire execution time
#define PU_SOUND				2		// static while playing
#define PU_MUSIC				3		// static while playing
#define PU_DAVE 				4		// anything else Dave wants static
#define PU_FREE                 5       // a free block [ML] 12/4/06: Readded from Chocodoom
#define PU_LEVEL				50		// static until level exited
#define PU_LEVSPEC				51		// a special thinker in a level
#define PU_LEVACS				52		// [RH] An ACS script in a level
// Tags >= 100 are purgable whenever needed.
#define PU_PURGELEVEL			100
#define PU_CACHE				101


void	Z_Init (void);
void	Z_FreeTags (int lowtag, int hightag);
void	Z_DumpHeap (int lowtag, int hightag);
void	Z_FileDumpHeap (FILE *f);
void	Z_CheckHeap (void);
size_t 	Z_FreeMemory (void);

// Don't use these, use the macros instead!
void*   Z_Malloc2 (size_t size, int tag, void *user, const char *file, int line);
void    Z_Free2 (void *ptr, const char *file, int line);
void	Z_ChangeTag2 (void *ptr, int tag);

typedef struct memblock_s
{
	size_t 				size;	// including the header and possibly tiny fragments
	void**				user;	// NULL if a free block
	int 				tag;	// PU_FREE if this is free  [ML] 12/4/06: Readded from Chocodoom
	int 				id; 	// should be ZONEID
	struct memblock_s*	next;
	struct memblock_s*	prev;
} memblock_t;

inline void Z_ChangeTag2 (const void *ptr, int tag)
{
	Z_ChangeTag2 (const_cast<void *>(ptr), tag);
}
//
// This is used to get the local FILE:LINE info from CPP
// prior to really calling the function in question.
//
#define Z_ChangeTag(p,t) \
{ \
      if (( (memblock_t *)( (byte *)(p) - sizeof(memblock_t)))->id!=0x1d4a11) \
	  I_Error("Z_ChangeTag at "__FILE__":%i",__LINE__); \
	  Z_ChangeTag2(p,t); \
};

#define Z_ChangeTagSafe(p,t) \
{ \
      if (( (memblock_t *)( (char *)(p) - sizeof(memblock_t)))->tag > t) \
      Z_ChangeTag (p,t); \
}

#define Z_Malloc(s,t,p) Z_Malloc2(s,t,p,__FILE__,__LINE__)
#define Z_Free(p) Z_Free2(p,__FILE__,__LINE__)

#endif // __Z_ZONE_H__
