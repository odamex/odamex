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

#include <iostream>

#include "event_handler.h"

using namespace std;

namespace agOdalaunch {

void EventReceiver(AG_Event *event)
{
	EventHandler *handler = static_cast<EventHandler*>(AG_PTR(1));

	if(handler)
		handler->Trigger(event);
}


ODA_EventRegister::~ODA_EventRegister()
{
	list<EventHandler*>::iterator listIter;

	listIter = HandlerList.begin();

	while(listIter != HandlerList.end())
	{
		delete *listIter;
		listIter = HandlerList.erase(listIter);
	}
}

EventHandler *ODA_EventRegister::RegisterEventHandler(EVENT_FUNC_PTR funcPtr)
{
	EventHandler *evh = new EventHandler(this, funcPtr);

	HandlerList.push_back(evh);

	return evh;
}

bool ODA_EventRegister::DeleteEventHandler(EventHandler *handler)
{
	list<EventHandler*>::iterator listIter;

	for(listIter = HandlerList.begin(); listIter != HandlerList.end(); ++listIter)
	{
		if(*listIter == handler)
		{
			delete *listIter;
			HandlerList.erase(listIter);

			return true;
		}
	}

	return false;
}

} // namespace
