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
//	Manual Connect Dialog
//
// AUTHORS:
//	 Michael Wood (mwoodj at huntsvegas dot org)
//
//-----------------------------------------------------------------------------

#ifndef _AGOL_MANUAL_H
#define _AGOL_MANUAL_H

#include "event_handler.h"

class AGOL_Manual : private ODA_EventRegister
{
public:
	AGOL_Manual();
	~AGOL_Manual();

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

#endif
