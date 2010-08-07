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

#ifndef _AGOL_DIRSEL_H
#define _AGOL_DIRSEL_H

#include "event_handler.h"

class AGOL_DirSelector
{
public:
	AGOL_DirSelector(std::string title);
	~AGOL_DirSelector();

	void SetOkAction(EventHandler *event);

private:
	AG_Window         *FileSelWindow;
	AG_FileDlg        *FileDlg;
};

#endif
