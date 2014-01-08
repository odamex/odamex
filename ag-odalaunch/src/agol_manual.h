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
//	Manual Connect Dialog
//
// AUTHORS:
//	 Michael Wood (mwoodj at huntsvegas dot org)
//
//-----------------------------------------------------------------------------

#ifndef _AGOL_MANUAL_H
#define _AGOL_MANUAL_H

#include "event_handler.h"

/**
agOdalaunch namespace.

All code for the ag-odalaunch launcher is contained within the agOdalaunch
namespace.
*/
namespace agOdalaunch {

/**
The manual connect window.

This class provides a manual connect window which can be used to connect to a
server at the provided host or IP and port.
*/
class AGOL_Manual : public ODA_EventRegister
{
public:
	/**
	Constructor.
	*/
	AGOL_Manual();

	/**
	Destructor.
	*/
	~AGOL_Manual();

	/**
	Set the window close event.

	The provided event handler will be called when the window closes.

	@param handler The event handler.
	*/
	void SetWindowCloseEvent(EventHandler *handler);

private:
	AG_Box       *CreateMainBox(void *parent);
	AG_Box       *CreateButtonBox(void *parent);
	AG_Textbox   *CreateServerEntry(void *parent);
	AG_Textbox   *CreatePasswordEntry(void *parent);

	void          OnCancel(AG_Event *event);
	void          OnOK(AG_Event *event);

	AG_Window    *ManualDialog;
	AG_Box       *MainBox;
	AG_Textbox   *ServerEntry;
	AG_Textbox   *PasswordEntry;
	AG_Box       *ButtonBox;

	EventHandler *CloseEventHandler;
};

} // namespace

#endif
