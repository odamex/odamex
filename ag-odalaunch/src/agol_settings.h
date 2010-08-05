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
//	Settings Dialog
//
// AUTHORS:
//	 Michael Wood (mwoodj at huntsvegas dot org)
//
//-----------------------------------------------------------------------------

#ifndef _AGOL_SETTINGS_H
#define _AGOL_SETTINGS_H

#include "event_handler.h"

typedef struct
{
	AG_Box       *optionsBox;
	AG_Checkbox  *masterOnStartCheck;
	AG_Checkbox  *showBlockedCheck;
	AG_Numerical *masterTimeoutSpin;
	AG_Numerical *serverTimeoutSpin;
} ODA_SrvOptionsBox;


// AG_Settings - Class for the settings dialog
class AGOL_Settings : private ODA_EventRegister
{
public:
	AGOL_Settings();
	~AGOL_Settings();

	void SetWindowCloseEvent(EventHandler *handler);

private:
	// Event Handlers
	void               OnCancel(AG_Event *event);
	void               OnOK(AG_Event *event);

	// Interface Creation Functions
	ODA_SrvOptionsBox *CreateSrvOptionsBox(void *parent);
	AG_Box            *CreateWadDirConfigBox(void *parent);
	AG_Tlist          *CreateWadDirList(void *parent);
	AG_Box            *CreateWadDirButtonBox(void *parent);
	AG_Box            *CreateExtraCmdParamsBox(void *parent);
	AG_Textbox        *CreateExtraCmdParamsEntry(void *parent);
	AG_Button         *CreateOKButton(void *parent);
	AG_Button         *CreateCancelButton(void *parent);
	AG_Box            *CreateMainButtonBox(void *parent);

	// Interface Components
	AG_Window         *SettingsDialog;
	ODA_SrvOptionsBox *SrvOptionsBox;
	AG_Box            *WadDirConfigBox;
	AG_Tlist          *WadDirList;
	AG_Box            *WadDirButtonBox;
	AG_Box            *ExtraCmdParamsBox;
	AG_Textbox        *ExtraCmdParamsEntry;
	AG_Box            *MainButtonBox;

	EventHandler      *CloseEventHandler;

	unsigned int       MasterTimeout;
	unsigned int       ServerTimeout;
};

#endif
