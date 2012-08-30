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
//	 Directory Selector
//
// AUTHORS:
//	 Michael Wood (mwoodj at huntsvegas dot org)
//
//-----------------------------------------------------------------------------

#ifndef _AGOL_DIRSEL_H
#define _AGOL_DIRSEL_H

#include "event_handler.h"

/**
agOdalaunch namespace.

All code for the ag-odalaunch launcher is contained within the agOdalaunch
namespace.
*/
namespace agOdalaunch {

/**
Directory selection dialog.

This class provides a directory selection dialog that can be used to browse
the filesystem and return a directory string.
*/
class AGOL_DirSelector
{
public:
	/**
	Constructor.
	*/
	AGOL_DirSelector();

	/**
	Constructor.

	@param title Window title.
	*/
	AGOL_DirSelector(const std::string &title);

	/**
	Destructor.
	*/
	~AGOL_DirSelector();

	/**
	Set the OK button action.

	This method sets the action for the OK button. The provided event handler
	is called when the OK button is pressed. The second AG argument provided
	to the event handler is the selected directory in an ascii string.

	@param event The event handler.
	*/
	void SetOkAction(EventHandler *event);

	/**
	Set the cancel button action.

	This method sets the action for the cancel button. The provided event handler
	is called when the cancel button is pressed.

	@param event The event handler.
	*/
	void SetCancelAction(EventHandler *event);

private:
	void        Init(const std::string &title);

	AG_Window  *DirSelWindow;
	AG_DirDlg  *DirDlg;
};

} // namespace

#endif
