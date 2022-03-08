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
//	Zone Memory Allocation. Neat.
//
//-----------------------------------------------------------------------------


#include "odamex.h"

#include <map>
#include <stdlib.h>

#include "z_zone.h"
#include "i_system.h"
#include "c_dispatch.h"
#include "hashtable.h"
#include "cmdlib.h"

struct OFileLine
{
	const char* file;
	int line;

	static OFileLine create(const char* file, const int line)
	{
		OFileLine rvo = {file, line};
		return rvo;
	}

	const char* shortFile() const
	{
		const char* ret = file;
		for (size_t i = 0; file[i] != '\0'; i++)
		{
			if (file[i] == PATHSEPCHAR)
			{
				ret = file + i + 1;
			}
		}
		return ret;
	}
};

#define FILELINE OFileLine::create(__FILE__, __LINE__)

#define CASE_STR(x) \
	case x:         \
		return #x

static const char* TagStr(const zoneTag_e tag)
{
	switch (tag)
	{
		CASE_STR(PU_FREE);
		CASE_STR(PU_STATIC);
		CASE_STR(PU_SOUND);
		CASE_STR(PU_MUSIC);
		CASE_STR(PU_LEVEL);
		CASE_STR(PU_LEVSPEC);
		CASE_STR(PU_LEVACS);
		CASE_STR(PU_PURGELEVEL);
		CASE_STR(PU_CACHE);
		default: return "UNKNOWN";
	}
}

//
// OZone
//
// A memory system that mimics a lot of the Zone system's behaviors but is more
// friendly to memory analysis tools like valgrind.
//
// Memory is allocated on the system heap with operator new. When allocating
// memory, a pointer to the allocated memory is inserted as the key into a hash
// table, along with a memory block tag and user pointer (both similar to the
// Zone memory's system).
//
// Upon freeing allocated memory, the memory the user pointer points to will be
// set to NULL, the memory will be freed, and the block will be removed from
// the hash table.
//
class OZone
{
	struct MemoryBlockInfo
	{
		zoneTag_e tag;      // PU_* tag
		uint32_t size;      // Size of allocation: 32-bit to save space
		void** user;        // Pointer owner
		OFileLine fileLine; // __FILE__, __LINE__
	};

	typedef std::map<void*, MemoryBlockInfo> MemoryBlockTable;
	MemoryBlockTable m_heap;

	MemoryBlockTable::iterator dealloc(MemoryBlockTable::iterator& block)
	{
		if (block->second.user)
		{
			*block->second.user = NULL;
		}

		void* imFree = block->first;

		free(imFree);

		MemoryBlockTable::iterator next = block;
		++next;
		m_heap.erase(block);
		return next;
	}

  public:
	OZone()
	{
	}

	~OZone()
	{
		clear();
	}

	void clear()
	{
		// Free all memory.
		for (MemoryBlockTable::iterator it = m_heap.begin(); it != m_heap.end();)
		{
			it = dealloc(it);
		}
	}

	void* alloc(size_t size, zoneTag_e tag, void* user, const OFileLine& info)
	{
		// This is implementation-defined behavior with malloc.  Since we
		// are the implementation, we get to choose the behavior.  Neat.
		if (size == 0)
		{
			return NULL;
		}

		// Our interface is malloc-like, so we use malloc and not new.
		void* ptr = malloc(size);
		if (ptr == NULL)
		{
			// Don't format these bytes, the byte formatter allocates.
			I_Error("%s: Could not allocate %zu bytes at %s:%i.", __FUNCTION__, size,
			        info.shortFile(), info.line);
		}

		// Construct the memory block.
		MemoryBlockInfo block;
		block.tag = tag;
		block.user = static_cast<void**>(user);
		block.size = size > MAXUINT ? MAXUINT : static_cast<uint32_t>(size);

		// Store the allocating function.  12 byte overhead per allocation,
		// but the information we get while debugging is priceless.
		OFileLine fileline = OFileLine::create(info.file, info.line);
		block.fileLine.file = fileline.file;
		block.fileLine.line = fileline.line;

		m_heap.insert(std::make_pair(ptr, block));
		if (block.user != NULL)
		{
			*block.user = ptr;
		}

		return ptr;
	}

	void changeTag(void* ptr, zoneTag_e tag, const OFileLine& info)
	{
		if (tag == PU_FREE)
		{
			I_Error("%s: Tried to change a tag to PU_FREE at %s:%i.", __FUNCTION__,
			        info.shortFile(), info.line);
		}

		MemoryBlockTable::iterator it = m_heap.find(ptr);
		if (it == m_heap.end())
		{
			I_Error("%s: Address 0x%p is not tracked by zone at %s:%i.", __FUNCTION__,
			        it->first, info.shortFile(), info.line);
		}

		if (tag >= PU_PURGELEVEL && it->second.user == NULL)
		{
			I_Error("%s: Found purgable block without an owner at %s:%i, "
			        "allocated at %s:%i.",
			        __FUNCTION__, info.shortFile(), info.line,
			        it->second.fileLine.shortFile(), it->second.fileLine.line);
		}

		it->second.tag = tag;
	}

	void changeOwner(void* ptr, void* user, const OFileLine& info)
	{
		// [AM] Nothing calls this as far as I know.
		I_Error("%s: not implemented", __FUNCTION__);
	}

	void deallocPtr(void* ptr, const OFileLine& info)
	{
		if (ptr == NULL)
			return;

		MemoryBlockTable::iterator it = m_heap.find(ptr);
		if (it == m_heap.end())
		{
			I_Error("%s: Address 0x%p is not tracked by zone at %s:%i.", __FUNCTION__,
			        it->first, info.shortFile(), info.line);
		}

		dealloc(it);
	}

	/**
	 * Dealloc all members
	 */
	void deallocTags(const int lowtag, const int hightag)
	{
		for (MemoryBlockTable::iterator it = m_heap.begin();it != m_heap.end();)
		{
			if (it->second.tag < lowtag || it->second.tag > hightag)
			{
				++it;
				continue;
			}

			it = dealloc(it);
		}
	}

	void dump()
	{
		size_t total = 0;
		for (MemoryBlockTable::iterator it = m_heap.begin(); it != m_heap.end(); ++it)
		{
			total += it->second.size;
			Printf("0x%p | size:%" "zu" " tag:%s user:0x%p %s:%d\n", (void*)it->first,
			       it->second.size, TagStr(it->second.tag), (void*)it->second.user,
			       it->second.fileLine.shortFile(), it->second.fileLine.line);
		}

		std::string buf;
		Printf("  allocation count: %" "zu" "\n", m_heap.size());

		StrFormatBytes(buf, total);
		Printf("  allocs size: %s\n", buf.c_str());

		StrFormatBytes(buf, m_heap.size() * sizeof(MemoryBlockInfo));
		Printf("  blocks size: %s\n", buf.c_str());
	}
} g_zone;


//
// Z_Close
//
void STACK_ARGS Z_Close()
{
	g_zone.clear();
}

//
// Z_Init
//
void Z_Init()
{
	g_zone.clear();
}


//
// Z_Free2
//
void Z_Free2(void* ptr, const char* file, int line)
{
	g_zone.deallocPtr(ptr, OFileLine::create(file, line));
}


//
// Z_Malloc
// You can pass a NULL user if the tag is < PU_PURGELEVEL.
//
#define MINFRAGMENT	64
#define ALIGN		8

void* Z_Malloc2(size_t size, const zoneTag_e tag, void* user, const char* file,
                const int line)
{
	return g_zone.alloc(size, tag, user, OFileLine::create(file, line));
}


//
// Z_FreeTags
//
void Z_FreeTags(const zoneTag_e lowtag, const zoneTag_e hightag)
{
	return ::g_zone.deallocTags(lowtag, hightag);
}

//
// Z_ChangeTag
//
void Z_ChangeTag2(void* ptr, const zoneTag_e tag, const char* file, int line)
{
	return ::g_zone.changeTag(ptr, tag, OFileLine::create(file, line));
}


void Z_ChangeOwner2(void* ptr, void* user, const char* file, int line)
{
	return ::g_zone.changeOwner(ptr, user, OFileLine::create(file, line));
}

//
// Z_DumpHeap
// Note: TFileDumpHeap( stdout ) ?
//
void Z_DumpHeap(const zoneTag_e lowtag, const zoneTag_e hightag)
{
	::g_zone.dump();
}

BEGIN_COMMAND(dumpheap)
{
	int lo = MININT, hi = MAXINT;

	if (argc >= 2)
	{
		lo = atoi(argv[1]);
		if (argc >= 3)
			hi = atoi(argv[2]);
	}

	Z_DumpHeap(static_cast<zoneTag_e>(lo), static_cast<zoneTag_e>(hi));
}
END_COMMAND(dumpheap)

VERSION_CONTROL (z_zone_cpp, "$Id$")
