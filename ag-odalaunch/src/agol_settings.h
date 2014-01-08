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
//	Settings Dialog
//
// AUTHORS:
//	 Michael Wood (mwoodj at huntsvegas dot org)
//
//-----------------------------------------------------------------------------

#ifndef _AGOL_SETTINGS_H
#define _AGOL_SETTINGS_H

#include "agol_dirsel.h"
#include "event_handler.h"
#include "typedefs.h"

/**
agOdalaunch namespace.

All code for the ag-odalaunch launcher is contained within the agOdalaunch
namespace.
*/
namespace agOdalaunch {

typedef struct
{
	AG_Box       *optionsBox;
	AG_Checkbox  *masterOnStartCheck;
	AG_Checkbox  *showBlockedCheck;
	AG_Numerical *masterTimeoutSpin;
	AG_Numerical *serverTimeoutSpin;
} ODA_SrvOptionsBox;

typedef struct
{
	AG_Box    *optionsBox;
	AG_Label  *driverLabel;
	AG_UCombo *driverCombo;
} ODA_GuiOptionsBox;

/**
The settings dialog.

This class provides a settings dialog with configurable options.
*/
class AGOL_Settings : public ODA_EventRegister
{
public:
	/**
	Constructor.
	*/
	AGOL_Settings();

	/**
	Destructor.
	*/
	~AGOL_Settings();

	/**
	Set the window close event.

	The provided event handler will be called when the window closes.

	@param handler The event handler.
	*/
	void SetWindowCloseEvent(EventHandler *handler);

private:
	bool               IsWadDirDuplicate(const std::string &waddir);

	// Event Handlers
	void               OnCancel(AG_Event *event);
	void               OnOK(AG_Event *event);
	void               UpdateWadDirList(AG_Event *event);
	void               AddWadDirSelectorOk(AG_Event *event);
	void               OnAddWadDir(AG_Event *event);
	void               ReplaceWadDirSelectorOk(AG_Event *event);
	void               OnReplaceWadDir(AG_Event *event);
	void               OnDeleteWadDir(AG_Event *event);
	void               OnMoveWadDirUp(AG_Event *event);
	void               OnMoveWadDirDown(AG_Event *event);
	void               OdamexPathSelectorOk(AG_Event *event);
	void               OnBrowseOdamexPath(AG_Event *event);
	void               DirectorySelectorCancel(AG_Event *event);

	// Interface Creation Functions
	AG_Box            *CreateTopOptionsBox(void *parent);
	ODA_SrvOptionsBox *CreateSrvOptionsBox(void *parent);
	ODA_GuiOptionsBox *CreateGuiOptionsBox(void *parent);
	AG_Box            *CreateOdamexPathBox(void *parent);
	AG_Label          *CreateOdamexPathLabel(void *parent);
	AG_Box            *CreateWadDirConfigBox(void *parent);
	AG_Tlist          *CreateWadDirList(void *parent);
	AG_Box            *CreateWadDirButtonBox(void *parent);
	AG_Box            *CreateExtraCmdParamsBox(void *parent);
	AG_Textbox        *CreateExtraCmdParamsEntry(void *parent);
	AG_Box            *CreateMainButtonBox(void *parent);

	// Save Functions
	void               SaveServerOptions();
	void               SaveGuiOptions();
	void               SaveOdamexPath();
	void               SaveWadDirs();
	void               SaveExtraParams();

	// Interface Components
	AG_Window         *SettingsDialog;
	AG_Box            *TopOptionsBox;
	ODA_SrvOptionsBox *SrvOptionsBox;
	ODA_GuiOptionsBox *GuiOptionsBox;
	AG_Box            *OdamexPathBox;
	AG_Label          *OdamexPathLabel;
	AG_Box            *WadDirConfigBox;
	AG_Tlist          *WadDirList;
	AG_Box            *WadDirButtonBox;
	AG_Box            *ExtraCmdParamsBox;
	AG_Textbox        *ExtraCmdParamsEntry;
	AG_Box            *MainButtonBox;

	AGOL_DirSelector  *DirSel;

	EventHandler      *CloseEventHandler;
	EventHandler      *DirSelOkHandler;
	EventHandler      *DirSelCancelHandler;

	int                MasterOnStart;
	int                ShowBlocked;
	unsigned int       MasterTimeout;
	unsigned int       ServerTimeout;

	std::list<std::string>  WadDirs;
};

} // namespace

#endif
