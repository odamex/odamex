// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 2006-2014 by The Odamex Team.
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
//	Zone Memory Allocation. Neat.
//
//-----------------------------------------------------------------------------


#include <stdlib.h>

#include "z_zone.h"
#include "i_system.h"
#include "doomdef.h"
#include "c_dispatch.h"


//
// ZONE MEMORY ALLOCATION
//
// There is never any space between memblocks,
//  and there will never be two contiguous free memblocks.
// The rover can be left pointing at a non-empty block.
//
// It is of no value to free a cachable block,
//  because it will get overwritten automatically if needed.
// 

#define ZONEID	0x1d4a11

typedef struct
{
	// total bytes malloced, including header
	size_t		size;
	
	// start / end cap for linked list
	memblock_t	blocklist;
	
	memblock_t	*rover;
	
} memzone_t;

static memzone_t* mainzone;
static size_t zonesize;

//
// Z_Close
//
void STACK_ARGS Z_Close()
{
	M_Free(mainzone);
}

//
// Z_Init
//
void Z_Init()
{
	// denis - allow reinitiation of entire memory system
	if (!mainzone)
		mainzone = (memzone_t*)I_ZoneBase(&zonesize);

	// set the entire zone to one free block
	memblock_t* block = (memblock_t*)((byte*)mainzone + sizeof(memzone_t));
	mainzone->size = zonesize;
	mainzone->blocklist.next = mainzone->blocklist.prev = block;
	
	mainzone->blocklist.user = (void**)mainzone;
	mainzone->blocklist.tag = PU_STATIC;
	mainzone->rover = block;
		
	block->prev = block->next = &mainzone->blocklist;
	
	block->tag = PU_FREE;
	block->user = NULL;
	
	block->size = mainzone->size - sizeof(memzone_t);
}


//
// Z_Free2
//
void Z_Free2(void* ptr, const char* file, int line)
{
	#ifdef ODAMEX_DEBUG
	Z_CheckHeap();
	#endif

	memblock_t* block = (memblock_t*)((byte*)ptr - sizeof(memblock_t));

	if (block->id != ZONEID)
		I_FatalError("Z_Free: freed a pointer without ZONEID at %s:%i", file, line);

	if (block->user != NULL)
		*block->user = NULL;	// clear the user's mark

	// mark as free
	block->tag = PU_FREE;
	block->user = NULL; 
	block->id = 0;
		
	memblock_t* other = block->prev;
	if (other->tag == PU_FREE)
	{
		// merge with previous free block
		other->size += block->size;
		other->next = block->next;
		other->next->prev = other;

		if (block == mainzone->rover)
			mainzone->rover = other;

		block = other;
	}

	other = block->next;
	if (other->tag == PU_FREE)
	{
		// merge the next free block onto the end
		block->size += other->size;
		block->next = other->next;
		block->next->prev = block;

		if (other == mainzone->rover)
			mainzone->rover = block;
	}

	#ifdef ODAMEX_DEBUG
	Z_CheckHeap();
	#endif
}


//
// Z_Malloc
// You can pass a NULL user if the tag is < PU_PURGELEVEL.
//
#define MINFRAGMENT	64
#define ALIGN		8

void* Z_Malloc2(size_t size, int tag, void* user, const char* file, int line)
{
	#ifdef ODAMEX_DEBUG
	Z_CheckHeap();
	#endif

	if (tag == PU_FREE)
		I_FatalError("Z_Malloc: cannot allocate a block with tag PU_FREE at %s:%i", file, line);

	size = (size + ALIGN - 1) & ~(ALIGN - 1);

    // scan through the block list,
    // looking for the first free block
    // of sufficient size,
    // throwing out any purgable blocks along the way.
	
	// account for size of block header
	size += sizeof(memblock_t);

    // if there is a free block behind the rover,
    //  back up over them
	memblock_t* base = mainzone->rover;
	if (base->prev->tag == PU_FREE)
		base = base->prev;

	memblock_t* rover = base;
	memblock_t* start = base->prev;

	do
	{
		if (rover == start)
		{
			// scanned all the way around the list
			I_FatalError("Z_Malloc: failed on allocation of %i bytes at %s:%i", size, file, line);
		}
		
		if (rover->tag != PU_FREE)
		{
			if (rover->tag < PU_PURGELEVEL)
			{
				// hit a block that can't be purged,
				//  so move past it
				base = rover = rover->next;
			}
			else
			{
				// free the rover block (adding the size to base)
				
				// the rover can be the base block
				base = base->prev;
				Z_Free((byte*)rover+sizeof(memblock_t));
				base = base->next;
				rover = base->next;
			}
		}
		else
			rover = rover->next;
	} while (base->tag != PU_FREE || base->size < size);

	// found a block big enough
	int extra = base->size - size;
	
	if (extra > MINFRAGMENT)
	{
		// there will be a free fragment after the allocated block
		memblock_t* newblock = (memblock_t*)((byte*)base + size );
		newblock->size = extra;
		
		newblock->tag = PU_FREE;
		newblock->user = NULL;	
		newblock->prev = base;
		newblock->next = base->next;
		newblock->next->prev = newblock;

		base->next = newblock;
		base->size = size;
	}
		
	base->tag = tag;
	base->user = (void**)user;
	base->id = ZONEID;

	if (user)
		*(void**)user = (void*)((byte*)base + sizeof(memblock_t));
	else
		if (tag >= PU_PURGELEVEL)
			I_FatalError("Z_Malloc: an owner is required for purgable blocks at %s:%i", file, line);

	// next allocation will start looking here
	mainzone->rover = base->next;


	#ifdef ODAMEX_DEBUG
	Z_CheckHeap();
	#endif

	return (void*)((byte*)base + sizeof(memblock_t));
}



//
// Z_FreeTags
//
void Z_FreeTags(int lowtag, int hightag)
{
	#ifdef ODAMEX_DEBUG
	Z_CheckHeap();
	#endif

	memblock_t* block;
	memblock_t* next;

	for (block = mainzone->blocklist.next; block != &mainzone->blocklist; block = next)
	{
		// get link before freeing
		next = block->next;

		// free block?
		if (block->tag == PU_FREE)
			continue;
	    
		if (block->tag >= lowtag && block->tag <= hightag)
			Z_Free((byte*)block+sizeof(memblock_t));
	}

	#ifdef ODAMEX_DEBUG
	Z_CheckHeap();
	#endif
}

//
// Z_CheckHeap
//
void Z_CheckHeap()
{
    memblock_t*	block;
	
    for (block = mainzone->blocklist.next ; ; block = block->next)
    {
		if (block->next == &mainzone->blocklist)
			break;		// all blocks have been hit
		
		if ((byte*)block + block->size != (byte*)block->next)
			I_Error("Z_CheckHeap: block size does not touch the next block\n");

		if ( block->next->prev != block)
			I_Error("Z_CheckHeap: next block doesn't have proper back link\n");

		if (block->tag == PU_FREE && block->next->tag == PU_FREE)
			I_Error("Z_CheckHeap: two consecutive free blocks\n");
    }
}

//
// Z_ChangeTag
//
void Z_ChangeTag2(void* ptr, int tag)
{
    memblock_t*	block = (memblock_t*)((byte*)ptr - sizeof(memblock_t));

	if (tag == PU_FREE)
		I_Error("Z_ChangeTag: cannot change a tag to PU_FREE");

    if (block->id != ZONEID)
        I_Error("Z_ChangeTag: freed a pointer without ZONEID");

    if (tag >= PU_PURGELEVEL && block->user == NULL)
        I_Error("Z_ChangeTag: an owner is required for purgable blocks");

    block->tag = tag;
}

//
// Z_FreeMemory
//
static size_t numblocks;
static size_t largestpfree, pfree, usedpblocks;	// Purgable blocks
static size_t largestefree, efree, usedeblocks; // Empty (Unused) blocks
static size_t largestlsize, lsize, usedlblocks;	// Locked blocks

size_t Z_FreeMemory()
{
	#ifdef ODAMEX_DEBUG
	Z_CheckHeap();
	#endif

	bool lastpurgable = false;
		
	numblocks =
		largestpfree = pfree = usedpblocks =
		largestefree = efree = usedeblocks =
		largestlsize = lsize = usedlblocks = 0;
	
	for (memblock_t* block = mainzone->blocklist.next; block != &mainzone->blocklist; block = block->next)
	{
		numblocks++;

		if (block->tag >= PU_PURGELEVEL)
		{
			usedpblocks++;
			pfree += block->size;
			if (lastpurgable)
			{
				largestpfree += block->size;
			}
			else if (block->size > largestpfree)
			{
				largestpfree = block->size;
				lastpurgable = true;
			}
		}
		else if (block->tag == PU_FREE)
		{
			usedeblocks++;
			efree += block->size;
			if (block->size > largestefree)
				largestefree = block->size;
			lastpurgable = false;
		}
		else
		{
			usedlblocks++;
			lsize += block->size;
			if (block->size > largestlsize)
				largestlsize = block->size;
			lastpurgable = false;
		}
	}
	return pfree + efree;
}

//
// Z_DumpHeap
// Note: TFileDumpHeap( stdout ) ?
//
void Z_DumpHeap(int lowtag, int hightag)
{
	Z_FreeMemory();
    memblock_t*	block;
	
    Printf(PRINT_HIGH, "zone size: %i  location: %p\n", mainzone->size, mainzone);
	Printf(PRINT_HIGH, "used: %i  free: %i\n", pfree+lsize, efree);
    Printf(PRINT_HIGH, "tag range: %i to %i\n", lowtag, hightag);
	
    for (block = mainzone->blocklist.next ; ; block = block->next)
    {
		char user[30];
		if (block->user == NULL || block->tag == PU_FREE)
			sprintf(user, "---");
		else
			sprintf(user, "%p", block->user);

		char tag[30];
		if (block->tag == PU_FREE)
			sprintf(tag, "FREE");
		else if (block->tag == PU_STATIC)
			sprintf(tag, "STATIC");
		else if (block->tag == PU_SOUND)
			sprintf(tag, "SOUND");
		else if (block->tag == PU_MUSIC)
			sprintf(tag, "MUSIC");
		else if (block->tag == PU_LEVEL)
			sprintf(tag, "LEVEL");
		else if (block->tag == PU_LEVSPEC)
			sprintf(tag, "LEVSPEC");
		else if (block->tag == PU_LEVACS)
			sprintf(tag, "LEVACS");
		else if (block->tag == PU_CACHE)
			sprintf(tag, "CACHE");
		else
			sprintf(tag, "UNKNOWN");

		if (block->tag >= lowtag && block->tag <= hightag)
			Printf(PRINT_HIGH, "block:%p    size:%9i    user:%-9s    tag:%-s\n",
				block, block->size, user, tag);
		
		if (block->next == &mainzone->blocklist)
			break;		// all blocks have been hit
	
		if ((byte*)block + block->size != (byte*)block->next)
			Printf(PRINT_HIGH, "ERROR: block size does not touch the next block\n");

		if (block->next->prev != block)
			Printf(PRINT_HIGH, "ERROR: next block doesn't have proper back link\n");

		if (block->tag == PU_FREE && block->next->tag == PU_FREE)
			Printf(PRINT_HIGH, "ERROR: two consecutive free blocks\n");
    }
}


BEGIN_COMMAND (dumpheap)
{
	int lo = MININT, hi = MAXINT;

	if (argc >= 2)
	{
		lo = atoi(argv[1]);
		if (argc >= 3)
			hi = atoi(argv[2]);
	}

	Z_DumpHeap(lo, hi);
}
END_COMMAND (dumpheap)

BEGIN_COMMAND (mem)
{
	Z_FreeMemory();

	Printf(PRINT_HIGH,
			"%u blocks:\n"
			"% 5u used      (%u, %u)\n"
			" % 5u purgable (%u, %u)\n"
			" % 5u locked   (%u, %u)\n"
			"% 5u unused    (%u, %u)\n"
			"% 5u p-free    (%u, %u)\n",
			numblocks,
			usedpblocks+usedlblocks, pfree+lsize,
			largestpfree > largestlsize ? largestpfree : largestlsize,
			usedpblocks, pfree, largestpfree,
			usedlblocks, lsize, largestlsize,
			usedeblocks, efree, largestefree,
			usedpblocks + usedeblocks, pfree + efree,
			largestpfree > largestefree ? largestpfree : largestefree
			);
}
END_COMMAND (mem)

VERSION_CONTROL (z_zone_cpp, "$Id$")

