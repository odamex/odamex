// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2006-2010 by The Odamex Team.
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
//	Handle Registration of C++ Class Member Event Handlers with Agar Events
//
// AUTHORS:
//	 Michael Wood (mwoodj at huntsvegas dot org)
//
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Handling Agar events in ag-odalaunch:
//
// Any Agar-based window/dialog that will register events with Agar needs to 
// inherit from the ODA_EventRegister class.
//
// A pointer to the EventReceiver function needs to be provided to Agar as the 
// event callback function for any event that will call a class member function.
//
// The Agar event must receive a pointer to an EventHandler returned from the
// RegisterEventHandler() function. This function receives the class member 
// function that should be triggered by the event. When passing the function 
// ptr cast to EVENT_FUNC_PTR.  Any additional desired variables can be provided 
// to the event after the required pointer. 
//
// The reason for this is that Agar is C-based and C functions cannot receive a 
// class member function pointer. It is also illegal to cast a 
// pointer-to-member-function to (void *) and vice versa.
//
// Example:
//		AG_SetEvent(win, "window-user-resize", EventReceiver, "%p", 
//			RegisterEventHandler((EVENT_FUNC_PTR)&OurClass::OnWindowResize));
//-----------------------------------------------------------------------------

#ifndef _EVENT_HANDLER_H
#define _EVENT_HANDLER_H

#include <list>

#include <agar/core.h>

#ifndef CALL_MEMBER_FN
#define CALL_MEMBER_FN(object,ptrToMember)  ((object).*(ptrToMember))
#endif

class ODA_EventRegister;
class EventHandler;

typedef void (ODA_EventRegister::*EVENT_FUNC_PTR)(AG_Event *);

//
// EventReceiver:
//		Event receiver function which will route the event callbacks.
//
void EventReceiver(AG_Event *event);

//
// ODA_EventRegister:
// 		Parent class for all classes that will have Agar event callback member 
// 		functions.
//
class ODA_EventRegister
{
public:
	~ODA_EventRegister();

	EventHandler *RegisterEventHandler(EVENT_FUNC_PTR funcPtr);

	// You only need to call this if you want to delete an EventHandler before 
	// you are finished with the object instance derived from this class. All 
	// handlers are deleted automatically when the object instance is deconstructed.
	bool DeleteEventHandler(EventHandler *handler);

private:
	std::list<EventHandler*> HandlerList;
};

//
// EventHandler:
// 		Class that wraps a pointer to a class and the event callback function
// 		pointer.  The class can be cast and passed as a (void *) to C-based 
// 		functions.
//
class EventHandler
{
public:
	EventHandler(ODA_EventRegister *classPtr, EVENT_FUNC_PTR funcPtr) :
		 ThisPtr(classPtr), FuncPtr(funcPtr) {};
	~EventHandler() {};

	void Trigger(AG_Event *event)
		{ CALL_MEMBER_FN(*ThisPtr, FuncPtr)(event); }

private:
	EVENT_FUNC_PTR     FuncPtr;
	ODA_EventRegister *ThisPtr;
};

#endif
