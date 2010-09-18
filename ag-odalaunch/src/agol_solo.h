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
//	Solo Game
//
// AUTHORS:
//	 Michael Wood (mwoodj at huntsvegas dot org)
//
//-----------------------------------------------------------------------------

#ifndef _AGOL_SOLO_H
#define _AGOL_SOLO_H

#include "event_handler.h"

class AGOL_Solo : private ODA_EventRegister
{
public:
	AGOL_Solo();
	~AGOL_Solo();

	void SetWindowCloseEvent(EventHandler *handler);

private:
	AG_Box            *CreateWadListsBox(void *parent);
	AG_Box            *CreateIwadBox(void *parent);
	AG_Tlist          *CreateIwadList(void *parent);
	AG_Box            *CreatePwadBox(void *parent);
	AG_Tlist          *CreatePwadList(void *parent);
	AG_Box            *CreateMainButtonBox(void *parent);

	void               PopulateWadLists();
	bool               WadIsIWAD(std::string wad);
	bool               PwadIsFileType(std::string wad, std::string extension);
	bool               PWadListContainsFileType(std::string extension);

	void               OnCancel(AG_Event *event);
	void               OnLaunch(AG_Event *event);

	// Interface Components
	AG_Window         *SoloGameDialog;
	AG_Box            *WadListsBox;
	AG_Box            *IwadBox;
	AG_Tlist          *IwadList;
	AG_Box            *PwadBox;
	AG_Tlist          *PwadList;
	AG_Box            *MainButtonBox;

	EventHandler      *CloseEventHandler;
};

#endif
