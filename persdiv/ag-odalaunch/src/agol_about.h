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
//	About Dialog
//
// AUTHORS:
//	 Michael Wood (mwoodj at huntsvegas dot org)
//
//-----------------------------------------------------------------------------

#ifndef _AGOL_ABOUT_H
#define _AGOL_ABOUT_H

#include "event_handler.h"

/**
agOdalaunch namespace.

All code for the ag-odalaunch launcher is contained within the agOdalaunch
namespace.
*/
namespace agOdalaunch {

/**
The about box.

This dialog provides information about the application, authors, and license.
*/
class AGOL_About : public ODA_EventRegister
{
public:
	/**
	Constructor.
	*/
	AGOL_About();

	/**
	Destructor.
	*/
	~AGOL_About();

	/**
	Set the window close event.

	The provided event handler will be called when the window closes.

	@param handler The event handler.
	*/
	void SetWindowCloseEvent(EventHandler *handler);

private:
	AG_Box       *CreateTopBox(void *parent);
	AG_Box       *CreateDevBox(void *parent);
	AG_Box       *CreateLicenseBox(void *parent);
	AG_Button    *CreateOKButton(void *parent);

	void          OnOK(AG_Event *event);
	void          OnLicensePrechg(AG_Event *event);
	void          OnLicensePostchg(AG_Event *event);

	AG_Window    *AboutDialog;
	AG_Box       *TopBox;
	AG_Box       *DevBox;
	AG_Box       *LicenseBox;
	AG_Button    *OKButton;

	EventHandler *CloseEventHandler;

	int           LicenseCursorPos;
	bool          LicenseDirty;
};

} // namespace

#endif
