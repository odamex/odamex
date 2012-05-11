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

#include <iostream>
#include <string>

#include <agar/core.h>
#include <agar/gui.h>

#include "agol_dirsel.h"

using namespace std;

namespace agOdalaunch {

AGOL_DirSelector::AGOL_DirSelector()
{
	Init("");
}

AGOL_DirSelector::AGOL_DirSelector(const std::string &title)
{
	Init(title);
}

void AGOL_DirSelector::Init(const std::string &title)
{
	DirSelWindow = AG_WindowNew(AG_WINDOW_MODAL);
	AG_WindowSetGeometryAligned(DirSelWindow, AG_WINDOW_MC, 500, 350);

	// Use the window title if provided.
	if(title.size())
		AG_WindowSetCaptionS(DirSelWindow, title.c_str());
	else
		AG_WindowSetCaptionS(DirSelWindow, "Please select a directory");

	// Add the directory selection dialog widget to the window
	DirDlg = AG_DirDlgNew(DirSelWindow, AG_DIRDLG_CLOSEWIN | AG_DIRDLG_LOAD | AG_DIRDLG_EXPAND);

	AG_WindowShow(DirSelWindow);
}

AGOL_DirSelector::~AGOL_DirSelector()
{
	AG_ObjectDetach(DirSelWindow);
}

void AGOL_DirSelector::SetOkAction(EventHandler *event)
{
	if(event)
		AG_DirDlgOkAction(DirDlg, EventReceiver, "%p", event);
}

void AGOL_DirSelector::SetCancelAction(EventHandler *event)
{
	if(event)
		AG_DirDlgCancelAction(DirDlg, EventReceiver, "%p", event);
}

} // namespace
