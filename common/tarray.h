// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1998-2006 by Randy Heit (ZDoom).
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
//	TARRAY
//
//-----------------------------------------------------------------------------


#ifndef __TARRAY_H__
#define __TARRAY_H__

#include <stdlib.h>
#include <cstring>
#include "m_alloc.h"

template <class T>
class TArray
{
public:
	TArray ()
	{
		Most = 0;
		Count = 0;
		Array = NULL;
	}
	TArray (int max)
	{
		Most = max;
		Count = 0;
		Array = (T *)Malloc (sizeof(T)*max);
	}
	~TArray ()
	{
		M_Free (Array);
		Most = 0;
		Count = 0;
	}
	T &operator[] (size_t index) const
	{
		return Array[index];
	}
	size_t Push (T item)
	{
		if (Count >= Most)
		{
			Most = Most ? Most * 2 : 16;
			Array = (T *)Realloc (Array, sizeof(T)*Most);
		}
		Array[Count] = item;
		return Count++;
	}
	bool Pop (T &item)
	{
		if (Count > 0)
		{
			item = Array[--Count];
			return true;
		}
		return false;
	}
	size_t Size ()
	{
		return Count;
	}
	size_t Max ()
	{
		return Most;
	}
	void Clear ()
	{
		Count = 0;
	}
	// Grow Array to be large enough to hold amount more entries without
	// further growing.
	void Grow (size_t amount)
	{
		if (Count + amount > Most)
		{
			const size_t choicea = Count + amount;
			const size_t choiceb = Most + Most/2;
			Most = (choicea > choiceb ? choicea : choiceb);
			Array = (T *)Realloc (Array, sizeof(T)*Most);
		}
	}
	// Resize Array so that it has exactly amount entries in use.
	void Resize (size_t amount)
	{
		if (Count < amount)
		{
			Grow (amount - Count);
		}
		else if (Count > amount)
		{
			Count = amount;
		}
	}
	// Reserves amount entries at the end of the array, but does nothing
	// with them.
	size_t Reserve (size_t amount)
	{	
		if (Count + amount > Most)
		{
			Grow (amount);
		}
		size_t place = Count;
		Count += amount;
		return place;
	}

private:
	T *Array;
	size_t Most;
	size_t Count;
};

#endif //__TARRAY_H__


