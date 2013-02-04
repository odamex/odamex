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
//	Handle Registration of C++ Class Member Event Handlers with Agar Events
//
// AUTHORS:
//	 Michael Wood (mwoodj at huntsvegas dot org)
//
//-----------------------------------------------------------------------------

#ifndef _EVENT_HANDLER_H
#define _EVENT_HANDLER_H

#include <list>

#include <agar/core.h>

/**
agOdalaunch namespace.

All code for the ag-odalaunch launcher is contained within the agOdalaunch
namespace.
*/
namespace agOdalaunch {

#ifndef CALL_MEMBER_FN
#define CALL_MEMBER_FN(object,ptrToMember)  ((object).*(ptrToMember))
#endif

class ODA_EventRegister;
class EventHandler;

/**
Typedef for casting an ODA_EventRegister method to an event function pointer.
*/
typedef void (ODA_EventRegister::*EVENT_FUNC_PTR)(AG_Event *);

/**
Event receiver.

Event receiver function which will route the event callbacks.

@param event The Agar event.
*/
void EventReceiver(AG_Event *event);

/**
An Event Register.

Parent class for all classes that will have Agar event callback member 
functions.

Handling Agar events in ag-odalaunch:

Any Agar-based window/dialog that will register events with Agar needs to 
inherit from the ODA_EventRegister class.

A pointer to the EventReceiver function needs to be provided to Agar as the 
event callback function for any event that will call a class member function.

The Agar event must receive a pointer to an EventHandler returned from the
RegisterEventHandler() function. This function receives the class member 
function that should be triggered by the event. When passing the function 
ptr cast to EVENT_FUNC_PTR.  Any additional desired variables can be provided 
to the event after the required pointer. 

The reason for this is that Agar is C-based and C functions cannot receive a 
class member function pointer. It is also illegal to cast a 
pointer-to-member-function to (void *) and vice versa.

Example:
    AG_SetEvent(win, "window-user-resize", EventReceiver, "%p", 
    RegisterEventHandler((EVENT_FUNC_PTR)&OurClass::OnWindowResize));
*/
class ODA_EventRegister
{
public:
	/**
	Destructor.

	At destruction all EventHandlers registered through this class instance
	are automatically deleted.
	*/
	~ODA_EventRegister();

protected:
	/**
	Register a function as an EventHandler.

	This method allows member functions of ODA_EventRegister derived
	classes to be registered as an event handler. The returned EventHandler
	instance can be passed to Agar functions or other C-based functions
	as a pointer with the EventReceiver method provided as the event callback
	function.

	@param funcPtr An ODA_EventRegister derived classes method.
	@returns An EventHandler instance.
	*/
	EventHandler *RegisterEventHandler(EVENT_FUNC_PTR funcPtr);

	/**
	Delete an event handler.

	This method deletes an EventHandler from the list of registered handlers.
	You only need to call this if you want to delete an EventHandler before 
	you are finished with the object instance derived from this class. All 
	handlers are deleted automatically when the object instance is destructed.

	@param handler An EventHandler.
	*/
	bool DeleteEventHandler(EventHandler *handler);

private:
	std::list<EventHandler*> HandlerList;
};

/**
An event handler.

Class that wraps a pointer to a class and the event callback function
pointer.  The class can be cast and passed as a (void *) to C-based 
functions.
*/
class EventHandler
{
public:
	/**
	Constructor.

	@param classPtr A pointer to an ODA_EventRegister derived class (often the
	               'this' pointer).
	@param funcPtr An ODA_EventRegister derived classes member function cast to
	               EVENT_FUNC_PTR. This method can be public, protected, or private.
	*/
	EventHandler(ODA_EventRegister *classPtr, EVENT_FUNC_PTR funcPtr)
		{ ThisPtr = classPtr, FuncPtr = funcPtr; }

	/**
	Destructor.
	*/
	~EventHandler() {};

	/**
	Trigger the event handler.

	This method will call into the member function that was provided at construction.
	This is typically called by the EventReceiver function.
	*/
	void Trigger(AG_Event *event)
		{ CALL_MEMBER_FN(*ThisPtr, FuncPtr)(event); }

private:
	EVENT_FUNC_PTR     FuncPtr;
	ODA_EventRegister *ThisPtr;
};

} // namespace

#endif
