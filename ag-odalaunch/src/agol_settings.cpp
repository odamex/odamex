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

#include <iostream>
#include <string>

#include <agar/core.h>
#include <agar/gui.h>

#include "agol_settings.h"
#include "gui_config.h"

using namespace std;

AGOL_Settings::AGOL_Settings()
{
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

	// Only update the wad dir list when explicitly requested.
	// I moved this here because it will be overwritten if the
	// widget is not shown yet.
	AG_TlistSetRefresh(WadDirList, -1);
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

	obox->masterOnStartCheck = AG_CheckboxNewInt(obox->optionsBox, 0, 
			"Get Master List on application start", &MasterOnStart);

	obox->showBlockedCheck = AG_CheckboxNewInt(obox->optionsBox, 0, 
			"Show blocked servers", &ShowBlocked);

	GuiConfig::Read("MasterOnStart", MasterOnStart);
	GuiConfig::Read("ShowBlockedServers", ShowBlocked);

	if(GuiConfig::Read("MasterTimeout", MasterTimeout) || MasterTimeout <= 0)
		MasterTimeout = 500;
	if(GuiConfig::Read("ServerTimeout", ServerTimeout) || ServerTimeout <= 0)
		ServerTimeout = 500;

	obox->masterTimeoutSpin = AG_NumericalNewUint(obox->optionsBox, 0, NULL, 
			"Master Timeout (ms)", &MasterTimeout);
	AG_NumericalSetRangeInt(obox->masterTimeoutSpin, 1, 5000);

	obox->serverTimeoutSpin = AG_NumericalNewUint(obox->optionsBox, 0, NULL, 
			"Server Timeout (ms)", &ServerTimeout);
	AG_NumericalSetRangeInt(obox->serverTimeoutSpin, 1, 5000);

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
	string    waddirs;
	char      cwd[PATH_MAX];

	wdlist = AG_TlistNewPolled(parent, AG_TLIST_EXPAND | AG_TLIST_NOSELSTATE, EventReceiver, "%p",
			RegisterEventHandler((EVENT_FUNC_PTR)&AGOL_Settings::UpdateWadDirList));
	AG_TlistSizeHintPixels(wdlist, 400, 0);

	// I moved this to the constructor for now - MJW
	// Only update when requested
	//AG_TlistSetRefresh(wdlist, -1); 

	if(GuiConfig::Read("WadDirs", waddirs))
	{
		if(!AG_GetCWD(cwd, PATH_MAX))
			WadDirs.push_back(string(cwd));
	}
	else
	{
		size_t oldpos = 0;
		size_t pos = waddirs.find(PATH_DELIMITER);

		while(pos != string::npos)
		{
			WadDirs.push_back(waddirs.substr(oldpos, pos - oldpos));
			oldpos = pos + 1;
			pos = waddirs.find(PATH_DELIMITER, oldpos);
		}
	}

	return wdlist;
}

AG_Box *AGOL_Settings::CreateWadDirButtonBox(void *parent)
{
	AG_Box *bbox;

	bbox = AG_BoxNewVert(parent, AG_BOX_VFILL);

	AG_ButtonNewFn(bbox, AG_BUTTON_HFILL, "Add", EventReceiver, "%p",
			RegisterEventHandler((EVENT_FUNC_PTR)&AGOL_Settings::OnAddWadDir));
	AG_ButtonNew(bbox, AG_BUTTON_HFILL, "Replace");
	AG_ButtonNewFn(bbox, AG_BUTTON_HFILL, "Delete", EventReceiver, "%p",
			RegisterEventHandler((EVENT_FUNC_PTR)&AGOL_Settings::OnDeleteWadDir));

	AG_SeparatorNewHoriz(bbox);

	AG_ButtonNewFn(bbox, AG_BUTTON_HFILL, "Move Up", EventReceiver, "%p",
			RegisterEventHandler((EVENT_FUNC_PTR)&AGOL_Settings::OnMoveWadDirUp));
	AG_ButtonNewFn(bbox, AG_BUTTON_HFILL, "Move Down", EventReceiver, "%p",
			RegisterEventHandler((EVENT_FUNC_PTR)&AGOL_Settings::OnMoveWadDirDown));

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
	string      extraParams;

	ecptextbox = AG_TextboxNew(parent, AG_TEXTBOX_HFILL, NULL);

	GuiConfig::Read("ExtraParams", extraParams);

	if(extraParams.size())
		AG_TextboxSetString(ecptextbox, extraParams.c_str());

	return ecptextbox;
}

AG_Box *AGOL_Settings::CreateMainButtonBox(void *parent)
{
	AG_Box *bbox;

	bbox = AG_BoxNewHoriz(parent, AG_BOX_HFILL);

	// This empty box positions the buttons to the right
	AG_BoxNewHoriz(bbox, AG_BOX_HFILL);

	AG_ButtonNewFn(bbox, 0, "Cancel", EventReceiver, "%p", 
			RegisterEventHandler((EVENT_FUNC_PTR)&AGOL_Settings::OnCancel));
	AG_ButtonNewFn(bbox, 0, "  OK  ", EventReceiver, "%p", 
			RegisterEventHandler((EVENT_FUNC_PTR)&AGOL_Settings::OnOK));

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
	SaveWadDirs();
	SaveServerOptions();
	SaveExtraParams();

	AG_ObjectDetach(SettingsDialog);

	if(CloseEventHandler)
		CloseEventHandler->Trigger(event);
}

void AGOL_Settings::UpdateWadDirList(AG_Event *event)
{
	list<string>::iterator i;

	AG_TlistBegin(WadDirList);

	for(i = WadDirs.begin(); i != WadDirs.end(); i++)
		AG_TlistAddS(WadDirList, agIconDirectory.s, (*i).c_str());
	
	AG_TlistEnd(WadDirList);
}

void AGOL_Settings::WadDirSelectorOk(AG_Event *event)
{
	char *waddir = AG_STRING(2);

	if(waddir && strlen(waddir) > 0)
		WadDirs.push_back(string(waddir));

	AG_TlistRefresh(WadDirList);

	delete DirSel;
}

void AGOL_Settings::OnAddWadDir(AG_Event *event)
{
	DirSel = new AGOL_DirSelector("Select a folder containing WAD files");

	DirSelCloseHandler = RegisterEventHandler((EVENT_FUNC_PTR)&AGOL_Settings::WadDirSelectorOk);
	DirSel->SetOkAction(DirSelCloseHandler);
}

void AGOL_Settings::OnDeleteWadDir(AG_Event *event)
{
	AG_TlistItem *selitem = AG_TlistSelectedItem(WadDirList);

	if(selitem && strlen(selitem->text) > 0)
	{
		list<string>::iterator i;

		for(i = WadDirs.begin(); i != WadDirs.end(); ++i)
			if(*i == selitem->text)
			{
				WadDirs.erase(i);
				break;
			}

		AG_TlistRefresh(WadDirList);
	}
}

void AGOL_Settings::OnMoveWadDirUp(AG_Event *event)
{
	AG_TlistItem *selitem = AG_TlistSelectedItem(WadDirList);

	if(selitem && strlen(selitem->text) > 0)
	{
		list<string>::iterator i;
		list<string>::iterator prev;

		for(i = WadDirs.begin(); i != WadDirs.end(); ++i)
		{
			if(*i == selitem->text)
			{
				if(i != WadDirs.begin())
					iter_swap(i, prev);
				break;
			}

			prev = i;
		}

		AG_TlistRefresh(WadDirList);
	}
}

void AGOL_Settings::OnMoveWadDirDown(AG_Event *event)
{
	AG_TlistItem *selitem = AG_TlistSelectedItem(WadDirList);

	if(selitem && strlen(selitem->text) > 0)
	{
		list<string>::iterator i = WadDirs.begin();
		list<string>::iterator prev = WadDirs.begin();

		for(++i; i != WadDirs.end(); ++i)
		{
			if(*prev == selitem->text)
			{
				iter_swap(prev, i);
				break;
			}

			prev = i;
		}

		AG_TlistRefresh(WadDirList);
	}
}

// Save Functions
void AGOL_Settings::SaveWadDirs()
{
	list<string>::iterator i;
	string liststr;

	for(i = WadDirs.begin(); i != WadDirs.end(); ++i)
		liststr += *i + PATH_DELIMITER;

	GuiConfig::Write("WadDirs", liststr);
}

void AGOL_Settings::SaveServerOptions()
{
	GuiConfig::Write("MasterOnStart", MasterOnStart);
	GuiConfig::Write("ShowBlockedServers", ShowBlocked);
	GuiConfig::Write("MasterTimeout", MasterTimeout);
	GuiConfig::Write("ServerTimeout", ServerTimeout);
}

void AGOL_Settings::SaveExtraParams()
{
	char *extraParams = NULL;

	extraParams = AG_TextboxDupString(ExtraCmdParamsEntry);

	if(extraParams && strlen(extraParams) > 0)
		GuiConfig::Write("ExtraParams", string(extraParams));
	else
		GuiConfig::Unset("ExtraParams");
}

// Public
void AGOL_Settings::SetWindowCloseEvent(EventHandler *handler)
{
	CloseEventHandler = handler;

	AG_AddEvent(SettingsDialog, "window-close", EventReceiver, "%p", handler);
}

