// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2006-2012 by The Odamex Team.
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
//
//  denis - szp<T>, the self zeroing pointer
//  
//  Once upon a time, actors held raw pointers to other actors. 
//  
//  To destroy an object, one cycled though all the others searching for its 
//  pointer and resetting every copy to NULL. Then one did the cycling for 
//  the players, then the sector sound origins, and so on; with hack upon 
//  hack. Ironically, zero dereferencing is what often crashed the 
//  program altogether.
//  
//  The idea behind szp is that all copies of one szp pointer can be made 
//  to point to the same object in O(1) time. This means that having a 
//  single szp of an actor, you can set them all to NULL without iteration. 
//  And, as a bonus, on every pointer access, a NULL check can throw a 
//  specific exception. Naturally, you should always be careful with pointers.
//
//-----------------------------------------------------------------------------


#ifndef __SZP_H__
#define __SZP_H__

#include "errors.h"


template <typename T>
class szp
{
	// pointer to a common raw pointer
	T **naive;

	// circular linked list
	szp *prev, *next;

	// this should never be used
	// spawn from other pointers, or use init()
	szp &operator =(T *other) {};

	// utility function to remove oneself from the linked list
	void inline unlink()
	{
		if(!next)
			return;

		next->prev = prev;
		prev->next = next;
			
		if(!naive)
			return;

		// last in ring?
		if(this == next)
			delete naive;
			
		naive = NULL;
	}
	
public:

	// use as pointer, checking validity
	inline T* operator ->()
	{
		if(!naive || !*naive)
			throw CRecoverableError("szp pointer was NULL");

		return *naive;
	}
	
	// use as raw pointer
	inline operator T*()
	{
		if(!naive)
			return NULL;
		else
			return *naive;
	}

	// this function can update or zero all related pointers
	void update_all(T *target)
	{
		if(!naive)
			throw CRecoverableError("szp pointer was NULL on update_all");
		
		// all copies already have naive, so their pointers will update too
		*naive = target;
	}
	
	// copy a pointer and add self to the "i have this pointer" list
	inline szp &operator =(szp other)
	{
		// itself?
		if(&other == this)
			return *this;

		unlink();

		if(!other.prev || !other.next || !other.naive)
		{
			next = prev = this;
			return *this;
		}
		
		// link
		naive = other.naive;
		prev = other.next->prev;
		next = other.next;
		prev->next = next->prev = this;
		
		return *this;
	}
	
	// creates the first (original) pointer
	void init(T *target)
	{
		unlink();
		
		// first link
		naive = new T*(target);
		prev = next = this;
	}
	
	// cheap constructor
	inline szp()
		: naive(NULL), prev(NULL), next(NULL)
	{ }
	
	// copy constructor
	inline szp(const szp &other)
		: naive(NULL)
	{
		if(!other.prev || !other.next || !other.naive)
		{
			prev = next = this;
			return;
		}
		
		// link
		naive = other.naive;
		prev = other.next->prev;
		next = other.next;
		prev->next = next->prev = this;
	}

	// unlink from circular list on destruction
	inline ~szp()
	{
		unlink();
	}
};

#endif // __SZP_H__


