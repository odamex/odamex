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
//	 Directory Selector
//
// AUTHORS:
//	 Michael Wood (mwoodj at huntsvegas dot org)
//
//-----------------------------------------------------------------------------

#include <string>

#include <agar/core.h>
#include <agar/gui.h>

#include "agol_dirsel.h"

using namespace std;

AGOL_DirSelector::AGOL_DirSelector(string title)
{
	FileSelWindow = AG_WindowNew(AG_WINDOW_MODAL | AG_WINDOW_DIALOG);
	AG_WindowSetGeometryAligned(FileSelWindow, AG_WINDOW_MC, 500, 350);

	// Use the window title if provided.
	if(title.size())
		AG_WindowSetCaptionS(FileSelWindow, title.c_str());

	FileDlg = AG_FileDlgNew(FileSelWindow, AG_FILEDLG_CLOSEWIN | AG_FILEDLG_EXPAND);

	AG_WindowShow(FileSelWindow);

	// Hackishly make this file browser into a directory browser
	// until a better solution is implemented.
	AG_WidgetHide(FileDlg->tbFile);
	AG_WidgetHide(FileDlg->tlFiles);
	AG_WidgetHide(FileDlg->comTypes);
	AG_PaneMoveDividerPct(FileDlg->hPane, 100);
}

AGOL_DirSelector::~AGOL_DirSelector()
{
	AG_ObjectDetach(FileSelWindow);
}

void AGOL_DirSelector::SetOkAction(EventHandler *event)
{
	if(event)
		AG_FileDlgOkAction(FileDlg, EventReceiver, "%p%p", event, FileDlg->cwd);
}
