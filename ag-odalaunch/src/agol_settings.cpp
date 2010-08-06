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

#include <agar/core.h>
#include <agar/gui.h>

#include "agol_settings.h"

AGOL_Settings::AGOL_Settings()
{
	MasterTimeout = 500;
	ServerTimeout = 500;

	SettingsDialog = AG_WindowNew(AG_WINDOW_MODAL | AG_WINDOW_DIALOG);
	AG_WindowSetCaptionS(SettingsDialog, "Configure Settings");
	AG_WindowSetSpacing(SettingsDialog, 1);

	SrvOptionsBox = CreateSrvOptionsBox(SettingsDialog);
	WadDirConfigBox = CreateWadDirConfigBox(SettingsDialog);
	WadDirList = CreateWadDirList(WadDirConfigBox);
	WadDirButtonBox = CreateWadDirButtonBox(WadDirConfigBox);
	ExtraCmdParamsBox = CreateExtraCmdParamsBox(SettingsDialog);
	ExtraCmdParamsEntry = CreateExtraCmdParamsEntry(ExtraCmdParamsBox);
	MainButtonBox = CreateMainButtonBox(SettingsDialog);

	CloseEventHandler = NULL;

	AG_WindowShow(SettingsDialog);
}

AGOL_Settings::~AGOL_Settings()
{
	delete SrvOptionsBox;
}

ODA_SrvOptionsBox *AGOL_Settings::CreateSrvOptionsBox(void *parent)
{
	ODA_SrvOptionsBox *obox = new ODA_SrvOptionsBox;

	obox->optionsBox = AG_BoxNewVert(parent, AG_BOX_FRAME);
	AG_BoxSetLabelS(obox->optionsBox, "Masters and Servers");
	AG_BoxSetPadding(obox->optionsBox, 25);
	AG_BoxSetSpacing(obox->optionsBox, 10);

	obox->masterOnStartCheck = AG_CheckboxNewS(obox->optionsBox, 0, "Get Master List on application start");

	obox->showBlockedCheck = AG_CheckboxNewS(obox->optionsBox, 0, "Show blocked servers");

	obox->masterTimeoutSpin = AG_NumericalNewUint(obox->optionsBox, 0, NULL, "Master Timeout (ms)", &MasterTimeout);
	AG_NumericalSetRangeInt(obox->masterTimeoutSpin, 0, 5000);

	obox->serverTimeoutSpin = AG_NumericalNewUint(obox->optionsBox, 0, NULL, "Server Timeout (ms)", &ServerTimeout);
	AG_NumericalSetRangeInt(obox->serverTimeoutSpin, 0, 5000);

	return obox;
}

AG_Box *AGOL_Settings::CreateWadDirConfigBox(void *parent)
{
	AG_Box *wdbox;

	wdbox = AG_BoxNewHoriz(parent, AG_BOX_EXPAND | AG_BOX_FRAME);
	AG_BoxSetLabelS(wdbox, "Wad Directories");
	AG_BoxSetPadding(wdbox, 15);
	AG_BoxSetSpacing(wdbox, 10);

	return wdbox;
}

AG_Tlist *AGOL_Settings::CreateWadDirList(void *parent)
{
	AG_Tlist *wdlist;

	wdlist = AG_TlistNew(parent, AG_TLIST_EXPAND);
	AG_TlistSizeHintPixels(wdlist, 400, 0);

	return wdlist;
}

AG_Box *AGOL_Settings::CreateWadDirButtonBox(void *parent)
{
	AG_Box *bbox;

	bbox = AG_BoxNewVert(parent, AG_BOX_VFILL);

	AG_ButtonNew(bbox, AG_BUTTON_HFILL, "Add");
	AG_ButtonNew(bbox, AG_BUTTON_HFILL, "Replace");
	AG_ButtonNew(bbox, AG_BUTTON_HFILL, "Delete");

	AG_SeparatorNewHoriz(bbox);

	AG_ButtonNew(bbox, AG_BUTTON_HFILL, "Move Up");
	AG_ButtonNew(bbox, AG_BUTTON_HFILL, "Move Down");

#ifndef GCONSOLE
	AG_SeparatorNewHoriz(bbox);

	AG_ButtonNew(bbox, AG_BUTTON_HFILL, "Get Environment");
#endif

	return bbox;
}

AG_Box *AGOL_Settings::CreateExtraCmdParamsBox(void *parent)
{
	AG_Box *ecpbox;

	// This box has to be homogenous for the textbox to look right. I'm not sure why.
	ecpbox = AG_BoxNewHoriz(parent, AG_BOX_HFILL | AG_BOX_HOMOGENOUS | AG_BOX_FRAME);
	AG_BoxSetLabelS(ecpbox, "Extra command line arguments");
	AG_BoxSetPadding(ecpbox, 20);

	return ecpbox;
}

AG_Textbox *AGOL_Settings::CreateExtraCmdParamsEntry(void *parent)
{
	AG_Textbox *ecptextbox;

	ecptextbox = AG_TextboxNew(parent, AG_TEXTBOX_HFILL, NULL);

	return ecptextbox;
}

AG_Box *AGOL_Settings::CreateMainButtonBox(void *parent)
{
	AG_Box *bbox;

	bbox = AG_BoxNewHoriz(parent, AG_BOX_HFILL);

	// This empty box positions the buttons to the right
	AG_BoxNewHoriz(bbox, AG_BOX_HFILL);

	AG_ButtonNewFn(bbox, 0, "Cancel", EventReceiver, "%p", RegisterEventHandler((EVENT_FUNC_PTR)&AGOL_Settings::OnCancel));
	AG_ButtonNewFn(bbox, 0, "  OK  ", EventReceiver, "%p", RegisterEventHandler((EVENT_FUNC_PTR)&AGOL_Settings::OnOK));

	return bbox;
}

// Event Handlers
void AGOL_Settings::OnCancel(AG_Event *event)
{
	AG_ObjectDetach(SettingsDialog);

	if(CloseEventHandler)
		CloseEventHandler->Trigger(event);
}

void AGOL_Settings::OnOK(AG_Event *event)
{
	AG_ObjectDetach(SettingsDialog);

	if(CloseEventHandler)
		CloseEventHandler->Trigger(event);
}

// Public
void AGOL_Settings::SetWindowCloseEvent(EventHandler *handler)
{
	CloseEventHandler = handler;

	AG_AddEvent(SettingsDialog, "window-close", EventReceiver, "%p", handler);
}

