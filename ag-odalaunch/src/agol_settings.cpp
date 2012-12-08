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

#include <iostream>
#include <string>
#include <algorithm>

#include <agar/core.h>
#include <agar/gui.h>

#include "agol_settings.h"
#include "gui_config.h"

using namespace std;

namespace agOdalaunch {

AGOL_Settings::AGOL_Settings() :
	OdamexPathBox(NULL), OdamexPathLabel(NULL), DirSel(NULL),
	CloseEventHandler(NULL), DirSelOkHandler(NULL), DirSelCancelHandler(NULL)
{
	SettingsDialog = AG_WindowNew(AG_WINDOW_MODAL);
	AG_WindowSetCaptionS(SettingsDialog, "Configure Settings");
	AG_WindowSetPosition(SettingsDialog, AG_WINDOW_MC, 0);

	TopOptionsBox = CreateTopOptionsBox(SettingsDialog);
	SrvOptionsBox = CreateSrvOptionsBox(TopOptionsBox);
	GuiOptionsBox = CreateGuiOptionsBox(TopOptionsBox);
#ifndef GCONSOLE
	OdamexPathBox = CreateOdamexPathBox(SettingsDialog);
	OdamexPathLabel = CreateOdamexPathLabel(OdamexPathBox);
#endif
	WadDirConfigBox = CreateWadDirConfigBox(SettingsDialog);
	WadDirList = CreateWadDirList(WadDirConfigBox);
	WadDirButtonBox = CreateWadDirButtonBox(WadDirConfigBox);
	ExtraCmdParamsBox = CreateExtraCmdParamsBox(SettingsDialog);
	ExtraCmdParamsEntry = CreateExtraCmdParamsEntry(ExtraCmdParamsBox);
	MainButtonBox = CreateMainButtonBox(SettingsDialog);

	AG_WindowShow(SettingsDialog);
}

AGOL_Settings::~AGOL_Settings()
{
	delete SrvOptionsBox;
	delete GuiOptionsBox;
}

AG_Box *AGOL_Settings::CreateTopOptionsBox(void *parent)
{
	AG_Box *tbox = AG_BoxNewHoriz(parent, AG_BOX_HFILL | AG_BOX_HOMOGENOUS);
	AG_BoxSetPadding(tbox, 0);
	AG_BoxSetSpacing(tbox, 0);

	return tbox;
}

ODA_SrvOptionsBox *AGOL_Settings::CreateSrvOptionsBox(void *parent)
{
	ODA_SrvOptionsBox *obox = new ODA_SrvOptionsBox;

	obox->optionsBox = AG_BoxNewVert(parent, AG_BOX_FRAME);
	AG_LabelNewS(obox->optionsBox, 0, "Masters and Servers");
	obox->optionsBox = AG_BoxNewVert(obox->optionsBox, AG_BOX_EXPAND);
	AG_BoxSetPadding(obox->optionsBox, 5);
	AG_BoxSetSpacing(obox->optionsBox, 5);

	obox->masterOnStartCheck = AG_CheckboxNewInt(obox->optionsBox, 0, 
			"Get Master List on application start", &MasterOnStart);

	obox->showBlockedCheck = AG_CheckboxNewInt(obox->optionsBox, 0, 
			"Show blocked servers", &ShowBlocked);

	// Read the options. If they are not set they will be disabled.
	GuiConfig::Read("MasterOnStart", MasterOnStart);
	GuiConfig::Read("ShowBlockedServers", ShowBlocked);

	// Read the timeout options. If they are not set use a default value of 500ms.
	if(GuiConfig::Read("MasterTimeout", MasterTimeout) || MasterTimeout == 0)
		MasterTimeout = 500;
	if(GuiConfig::Read("ServerTimeout", ServerTimeout) || ServerTimeout == 0)
		ServerTimeout = 500;

	obox->masterTimeoutSpin = AG_NumericalNewUintR(obox->optionsBox, 0, NULL, 
			"Master Timeout (ms)", &MasterTimeout, 1, 5000);

	obox->serverTimeoutSpin = AG_NumericalNewUintR(obox->optionsBox, 0, NULL, 
			"Server Timeout (ms)", &ServerTimeout, 1, 5000);

	return obox;
}

ODA_GuiOptionsBox *AGOL_Settings::CreateGuiOptionsBox(void *parent)
{
	char               drvList[128];
	AG_Box            *vdbox;
	ODA_GuiOptionsBox *gbox = new ODA_GuiOptionsBox;

	gbox->optionsBox = AG_BoxNewVert(parent, AG_BOX_FRAME);
	AG_LabelNewS(gbox->optionsBox, 0, "Gui Options");
	gbox->optionsBox = AG_BoxNewVert(gbox->optionsBox, AG_BOX_EXPAND);
	AG_BoxSetPadding(gbox->optionsBox, 5);
	AG_BoxSetSpacing(gbox->optionsBox, 5);

	// Video Driver Option
	vdbox = AG_BoxNewHoriz(gbox->optionsBox, AG_BOX_HFILL);
	AG_BoxSetPadding(vdbox, 0);
	AG_BoxSetSpacing(vdbox, 0);

	gbox->driverLabel = AG_LabelNewS(vdbox, 0, "Video Driver:   ");

	gbox->driverCombo = AG_UComboNew(vdbox, AG_UCOMBO_HFILL);
	AG_UComboSizeHintPixels(gbox->driverCombo, 150, 3);

	AG_ListDriverNames(drvList, 128);

	if(strlen(drvList) > 0)
	{
		size_t oldpos = 0;
		size_t pos = 0;

		while(pos != string::npos)
		{
			AG_TlistItem *item;

			pos = string(drvList).find(' ', oldpos);
			item = AG_TlistAdd(gbox->driverCombo->list, NULL, string(drvList).substr(oldpos, pos - oldpos).c_str());
			if(string(item->text) == string(agDriverOps->name))
				AG_ButtonTextS(gbox->driverCombo->button, item->text);
			oldpos = pos + 1;
		}
	}
	// End - Video Driver Option

	return gbox;
}

AG_Box *AGOL_Settings::CreateOdamexPathBox(void *parent)
{
	AG_Box *opbox;

	opbox = AG_BoxNewVert(parent, AG_BOX_HFILL | AG_BOX_FRAME);
	AG_LabelNewS(opbox, 0, "Odamex Path");
	opbox = AG_BoxNewHoriz(opbox, AG_BOX_HFILL);
	AG_BoxSetPadding(opbox, 5);
	AG_BoxSetSpacing(opbox, 5);

	return opbox;
}

AG_Label *AGOL_Settings::CreateOdamexPathLabel(void *parent)
{
	AG_Label *oplabel;
	string    oppath;

	oplabel = AG_LabelNewS(parent, AG_LABEL_FRAME | AG_LABEL_EXPAND, "");
	AG_LabelValign(oplabel, AG_TEXT_MIDDLE);

	GuiConfig::Read("OdamexPath", oppath);

	if(oppath.size())
		AG_LabelTextS(oplabel, oppath.c_str());

	AG_ButtonNewFn(parent, 0, "Browse", EventReceiver, "%p",
			RegisterEventHandler((EVENT_FUNC_PTR)&AGOL_Settings::OnBrowseOdamexPath));

	return oplabel;
}

AG_Box *AGOL_Settings::CreateWadDirConfigBox(void *parent)
{
	AG_Box *wdbox;

	wdbox = AG_BoxNewVert(parent, AG_BOX_HFILL | AG_BOX_FRAME);
	AG_LabelNewS(wdbox, 0, "Wad Directories");
	wdbox = AG_BoxNewHoriz(wdbox, AG_BOX_HFILL);
	AG_BoxSetPadding(wdbox, 5);
	AG_BoxSetSpacing(wdbox, 5);

	return wdbox;
}

AG_Tlist *AGOL_Settings::CreateWadDirList(void *parent)
{
	AG_Tlist *wdlist;
	string    waddirs;
	char      cwd[AG_PATHNAME_MAX];

	wdlist = AG_TlistNewPolled(parent, AG_TLIST_EXPAND, EventReceiver, "%p",
			RegisterEventHandler((EVENT_FUNC_PTR)&AGOL_Settings::UpdateWadDirList));
	AG_TlistSetCompareFn(wdlist, AG_TlistCompareStrings);
	AG_TlistSizeHintPixels(wdlist, 400, 0);

	// Read the WadDirs option from the config file
	if(GuiConfig::Read("WadDirs", waddirs))
	{
		// If there are no waddirs configured insert the current working directory
		if(!AG_GetCWD(cwd, AG_PATHNAME_MAX))
			WadDirs.push_back(cwd);
	}
	else
	{
		size_t oldpos = 0;
		size_t pos = waddirs.find(PATH_DELIMITER);

		// Parse the waddirs option
		while(pos != string::npos)
		{
			// Put a wad path into the list
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
	AG_ButtonNewFn(bbox, AG_BUTTON_HFILL, "Replace", EventReceiver, "%p",
			RegisterEventHandler((EVENT_FUNC_PTR)&AGOL_Settings::OnReplaceWadDir));
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

	ecpbox = AG_BoxNewVert(parent, AG_BOX_HFILL | AG_BOX_FRAME);
	AG_LabelNewS(ecpbox, 0, "Extra command line arguments");
	ecpbox = AG_BoxNewHoriz(ecpbox, AG_BOX_HFILL);
	AG_BoxSetPadding(ecpbox, 5);
	AG_BoxSetSpacing(ecpbox, 5);

	return ecpbox;
}

AG_Textbox *AGOL_Settings::CreateExtraCmdParamsEntry(void *parent)
{
	AG_Textbox *ecptextbox;
	string      extraParams;

	ecptextbox = AG_TextboxNew(parent, AG_TEXTBOX_HFILL, NULL);

	// Read the ExtraParams option from the config file
	GuiConfig::Read("ExtraParams", extraParams);

	// Put the extra params into the textbox
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

bool AGOL_Settings::IsWadDirDuplicate(const string &waddir)
{
	list<string>::iterator i;

	for(i = WadDirs.begin(); i != WadDirs.end(); ++i)
	{
		if(*i == waddir)
			return true;
	}

	return false;
}

//*************************//
// Event Handler Functions //
//*************************//

void AGOL_Settings::OnCancel(AG_Event *event)
{
	// Detach and destroy the window + contents
	AG_ObjectDetach(SettingsDialog);

	// Call the close handler if one was set
	if(CloseEventHandler)
		CloseEventHandler->Trigger(event);
}

void AGOL_Settings::OnOK(AG_Event *event)
{
	// Call all the save functions
	SaveServerOptions();
	SaveGuiOptions();
#ifndef GCONSOLE
	SaveOdamexPath();
#endif
	SaveWadDirs();
	SaveExtraParams();

	// Save the ag-odalaunch configuration settings
	GuiConfig::Save();

	// Detach and destroy the window + contents
	AG_ObjectDetach(SettingsDialog);

	// Call the close handler if one was set
	if(CloseEventHandler)
		CloseEventHandler->Trigger(event);
}

void AGOL_Settings::UpdateWadDirList(AG_Event *event)
{
	list<string>::iterator i;

	// Clear the list and store selection
	AG_TlistBegin(WadDirList);

	// Traverse the waddir list and add them to the tlist widget
	for(i = WadDirs.begin(); i != WadDirs.end(); ++i)
		AG_TlistAddS(WadDirList, agIconDirectory.s, (*i).c_str());
	
	// Restore list selection
	AG_TlistEnd(WadDirList);
}

void AGOL_Settings::AddWadDirSelectorOk(AG_Event *event)
{
	char *waddir = AG_STRING(2);

	// If a path came back add it to the list
	if(waddir && strlen(waddir) > 0 && !IsWadDirDuplicate(waddir))
		WadDirs.push_back(waddir);

	DeleteEventHandler(DirSelOkHandler);

	delete DirSel;
	DirSel = NULL;
}

void AGOL_Settings::OnAddWadDir(AG_Event *event)
{
	if(DirSel)
		return;

	// Create and display a new directory selector dialog
	DirSel = new AGOL_DirSelector("Select a folder containing WAD files");

	// Register and set the OK and Cancel events
	DirSelOkHandler = RegisterEventHandler((EVENT_FUNC_PTR)&AGOL_Settings::AddWadDirSelectorOk);
	DirSel->SetOkAction(DirSelOkHandler);
	DirSelCancelHandler = RegisterEventHandler((EVENT_FUNC_PTR)&AGOL_Settings::DirectorySelectorCancel);
	DirSel->SetCancelAction(DirSelCancelHandler);
}

void AGOL_Settings::ReplaceWadDirSelectorOk(AG_Event *event)
{
	AG_TlistItem *selitem = AG_TlistSelectedItem(WadDirList);
	char         *waddir  = AG_STRING(2);

	if((selitem && strlen(selitem->text) > 0) && 
		(waddir && strlen(waddir) > 0 && !IsWadDirDuplicate(waddir)))
	{
		list<string>::iterator i;

		// Look for the selected directory in the list
		for(i = WadDirs.begin(); i != WadDirs.end(); ++i)
			if(*i == selitem->text)
			{
				// Replace the item with the new path
				*i = waddir;
				break;
			}
	}

	DeleteEventHandler(DirSelOkHandler);

	delete DirSel;
	DirSel = NULL;
}

void AGOL_Settings::OnReplaceWadDir(AG_Event *event)
{
	if(DirSel)
		return;

	if(AG_TlistSelectedItem(WadDirList) == NULL)
		return;

	// Create and display a new directory selector dialog
	DirSel = new AGOL_DirSelector("Select a folder containing WAD files");

	// Register and set the OK and Cancel events
	DirSelOkHandler = RegisterEventHandler((EVENT_FUNC_PTR)&AGOL_Settings::ReplaceWadDirSelectorOk);
	DirSel->SetOkAction(DirSelOkHandler);
	DirSelCancelHandler = RegisterEventHandler((EVENT_FUNC_PTR)&AGOL_Settings::DirectorySelectorCancel);
	DirSel->SetCancelAction(DirSelCancelHandler);
}

void AGOL_Settings::OnDeleteWadDir(AG_Event *event)
{
	AG_TlistItem *selitem = AG_TlistSelectedItem(WadDirList);

	if(selitem && strlen(selitem->text) > 0)
	{
		list<string>::iterator i;

		// Look for the selected directory in the list
		for(i = WadDirs.begin(); i != WadDirs.end(); ++i)
			if(*i == selitem->text)
			{
				// Delete the list item
				WadDirs.erase(i);
				break;
			}
	}
}

void AGOL_Settings::OnMoveWadDirUp(AG_Event *event)
{
	AG_TlistItem *selitem = AG_TlistSelectedItem(WadDirList);

	if(selitem && strlen(selitem->text) > 0)
	{
		list<string>::iterator i;
		list<string>::iterator prev;

		// Look for the selected directory in the list
		for(i = WadDirs.begin(); i != WadDirs.end(); ++i)
		{
			if(*i == selitem->text)
			{
				// Swap the item with the one before it
				if(i != WadDirs.begin())
					iter_swap(i, prev);
				break;
			}

			prev = i;
		}
	}
}

void AGOL_Settings::OnMoveWadDirDown(AG_Event *event)
{
	AG_TlistItem *selitem = AG_TlistSelectedItem(WadDirList);

	if(selitem && strlen(selitem->text) > 0)
	{
		list<string>::iterator i = WadDirs.begin();
		list<string>::iterator prev = WadDirs.begin();

		// Look for the selected directory in the list
		for(++i; i != WadDirs.end(); ++i)
		{
			if(*prev == selitem->text)
			{
				// Swap the item with the one after it
				iter_swap(prev, i);
				break;
			}

			prev = i;
		}
	}
}

void AGOL_Settings::OdamexPathSelectorOk(AG_Event *event)
{
	char *odapath = AG_STRING(2);

	// If a path came back set the odamex path label
	if(odapath && strlen(odapath) > 0)
		AG_LabelTextS(OdamexPathLabel, odapath);

	DeleteEventHandler(DirSelOkHandler);

	delete DirSel;
	DirSel = NULL;
}

void AGOL_Settings::OnBrowseOdamexPath(AG_Event *event)
{
	if(DirSel)
		return;

	// Create and display a new directory selector dialog
	DirSel = new AGOL_DirSelector("Select your Odamex folder");

	// Register and set the OK and Cancel events
	DirSelOkHandler = RegisterEventHandler((EVENT_FUNC_PTR)&AGOL_Settings::OdamexPathSelectorOk);
	DirSel->SetOkAction(DirSelOkHandler);
	DirSelCancelHandler = RegisterEventHandler((EVENT_FUNC_PTR)&AGOL_Settings::DirectorySelectorCancel);
	DirSel->SetCancelAction(DirSelCancelHandler);
}

void AGOL_Settings::DirectorySelectorCancel(AG_Event *event)
{
	DeleteEventHandler(DirSelCancelHandler);

	delete DirSel;
	DirSel = NULL;
}

//****************//
// Save Functions //
//****************//

void AGOL_Settings::SaveServerOptions()
{
	GuiConfig::Write("MasterOnStart", MasterOnStart);
	GuiConfig::Write("ShowBlockedServers", ShowBlocked);
	GuiConfig::Write("MasterTimeout", MasterTimeout);
	GuiConfig::Write("ServerTimeout", ServerTimeout);
}

void AGOL_Settings::SaveGuiOptions()
{
	if(GuiOptionsBox && GuiOptionsBox->driverCombo)
	{
		char *driver;

		driver = GuiOptionsBox->driverCombo->button->lbl->text;

		if(driver && strlen(driver) > 0)
			GuiConfig::Write("VideoDriver", string(driver));

		if(string(driver) != string(agDriverOps->name))
			AG_TextMsgS(AG_MSG_INFO, "The selected video driver will be\n"
			                         "used when the application is restarted.");
	}
}

void AGOL_Settings::SaveOdamexPath()
{
	if(OdamexPathLabel->text && strlen(OdamexPathLabel->text) > 0)
		GuiConfig::Write("OdamexPath", string(OdamexPathLabel->text));
	else
		GuiConfig::Unset("OdamexPath");
}

void AGOL_Settings::SaveWadDirs()
{
	list<string>::iterator i;
	string liststr;

	// Construct a single string with a seperator for saving
	for(i = WadDirs.begin(); i != WadDirs.end(); ++i)
		liststr += *i + PATH_DELIMITER;

	GuiConfig::Write("WadDirs", liststr);
}

void AGOL_Settings::SaveExtraParams()
{
	char *extraParams = NULL;

	extraParams = AG_TextboxDupString(ExtraCmdParamsEntry);

	if(extraParams && strlen(extraParams) > 0)
		GuiConfig::Write("ExtraParams", string(extraParams));
	else
		GuiConfig::Unset("ExtraParams");

	if(extraParams)
		free(extraParams);
}

//******************//
// Public Functions //
//******************//

void AGOL_Settings::SetWindowCloseEvent(EventHandler *handler)
{
	if(handler)
	{
		CloseEventHandler = handler;

		AG_AddEvent(SettingsDialog, "window-close", EventReceiver, "%p", handler);
	}
}

} // namespace
