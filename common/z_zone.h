// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 2006-2026 by The Odamex Team.
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

#include <source_location>

//
// ZONE MEMORY
// PU - purge tags.
// Tags < 100 are not overwritten until freed.
enum struct zoneTag_e
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

using enum zoneTag_e; // this allows us keep using it without namespacing, like other ports, but it's now more type-safe

void Z_Init();
void Z_Close();
void Z_FreeTags(const zoneTag_e lowtag, const zoneTag_e hightag);
void Z_DumpHeap(const zoneTag_e lowtag, const zoneTag_e hightag);

#define SOURCELOC const std::source_location location = std::source_location::current()

namespace z_detail
{
// Don't use these, use the templated ones instead!
void* Z_Malloc2(size_t size, const zoneTag_e tag, void* user, SOURCELOC);
void* Z_Realloc2(void* ptr, size_t size, const zoneTag_e tag, void* user, SOURCELOC);
}

void Z_Free(void* ptr, SOURCELOC);
void Z_Discard(void** ptr, SOURCELOC);
void Z_ChangeTag(void* ptr, const zoneTag_e tag, SOURCELOC);
void Z_ChangeOwner(void* ptr, void* user, SOURCELOC);
char* Z_StrDup(const char* s, const zoneTag_e tag, SOURCELOC);

/**
 * @brief Allocates memory from the zone allocator.
 *
 * If `T` is `void`, the argument is interpreted as a size in bytes.
 *
 * If `T` is not `void`, the argument is interpreted as a count of `T`
 * objects and the allocation size will be `count * sizeof(T)`.
 *
 * Does not call constructors.
 *
 * @tparam T Element type to allocate. Defaults to `void` for raw byte allocation.
 * @param count_or_size Number of elements to allocate, or size in bytes if `T = void`.
 * @param tag Zone memory tag controlling lifetime.
 * @param user Optional owner pointer.
 *
 * @return Pointer to allocated memory.
 *
 * @note Prefer the typed form (`Z_Malloc<T>(count, tag)`) whenever possible
 *       to avoid manual size calculations.
 */
template <typename T = void>
requires (std::is_object_v<T> || std::is_void_v<T>)
T* Z_Malloc(size_t count_or_size, const zoneTag_e tag, void* user = nullptr, SOURCELOC)
{
	if constexpr (std::is_void_v<T>)
		return z_detail::Z_Malloc2(count_or_size, tag, user, location);
	else
		return static_cast<T*>(z_detail::Z_Malloc2(count_or_size * sizeof(T), tag, user, location));
}

/**
 * @brief Allocates memory for a single object from the zone allocator.
 *
 * Does not call constructors.
 *
 * @tparam T Element type to allocate.
 * @param tag Zone memory tag controlling lifetime.
 * @param user Optional owner pointer.
 *
 * @return Pointer to allocated memory.
 *
 */
template <typename T>
requires std::is_object_v<T>
T* Z_Malloc(const zoneTag_e tag, void* user = nullptr, SOURCELOC)
{
	return static_cast<T*>(z_detail::Z_Malloc2(sizeof(T), tag, user, location));
}

/**
 * @brief Reallocates memory from the zone allocator.
 *
 * If `T` is `void`, the argument is interpreted as a size in bytes.
 *
 * If `T` is not `void`, the argument is interpreted as a count of `T`
 * objects and the allocation size will be `count * sizeof(T)`.
 *
 * Does not call constructors.
 *
 * @tparam T Element type to allocate. Defaults to `void` for raw byte allocation.
 * @param ptr Pointer to an existing allocation previously returned by `Z_Malloc` or `Z_Realloc`.
 * @param count_or_size Number of elements to allocate, or size in bytes if `T = void`.
 * @param tag Zone memory tag controlling lifetime.
 * @param user Optional owner pointer.
 *
 * @return Pointer to allocated memory.
 *
 * @note Prefer the typed form (`Z_Realloc<T>(ptr, count, tag)`) whenever possible
 *       to avoid manual size calculations.
 */
template <typename T = void>
requires (std::is_object_v<T> || std::is_void_v<T>)
T* Z_Realloc(T* ptr, size_t count_or_size, const zoneTag_e tag, void* user = nullptr, SOURCELOC)
{
	if constexpr (std::is_void_v<T>)
		return z_detail::Z_Realloc2(ptr, count_or_size, tag, user, location);
	else
		return static_cast<T*>(z_detail::Z_Realloc2(ptr, count_or_size * sizeof(T), tag, user, location));
}

typedef struct memblock_s
{
	size_t 				size;	// including the header and possibly tiny fragments
	void**				user;	// NULL if a free block
	int 				tag;	// PU_FREE if this is free  [ML] 12/4/06: Readded from Chocodoom
	int 				id; 	// should be ZONEID
	struct memblock_s*	next;
	struct memblock_s*	prev;
} memblock_t;

inline void Z_ChangeTag(const void* ptr, const zoneTag_e tag, SOURCELOC)
{
	Z_ChangeTag(const_cast<void *>(ptr), tag, location);
}

/**
 * @brief Discard a piece of memory from the heap without freeing it.
 *
 * @param ptr A pointer to the pointer we want to discard.  The pointer must
 *            point to something, but the pointed-to-pointer can be null,
 *            in which case nothing happens.
 * @param location Location in the source code that this function was called
 */
template <typename P>
inline void Z_Discard(P ptr, SOURCELOC)
{
	if (*ptr == NULL)
	{
		return;
	}

	Z_ChangeTag(*ptr, PU_CACHE, location);
	*ptr = NULL;
}

#undef SOURCELOC

//
// This is used to get the local FILE:LINE info from CPP
// prior to really calling the function in question.
//
#define Z_ChangeTagSafe(p,t) \
{ \
      if (( (memblock_t *)( (char *)(p) - sizeof(memblock_t)))->tag > t) \
      Z_ChangeTag (p,t); \
}
