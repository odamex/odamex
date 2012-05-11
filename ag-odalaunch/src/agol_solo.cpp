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
//	Solo Game
//
// AUTHORS:
//	 Michael Wood (mwoodj at huntsvegas dot org)
//
//-----------------------------------------------------------------------------

#include <iostream>
#include <string>
#include <algorithm>
#include <list>

#include <agar/core.h>
#include <agar/gui.h>

#include "agol_solo.h"
#include "gui_config.h"
#include "game_command.h"

using namespace std;

namespace agOdalaunch {

const string DoomIWadNames[] =
{
	"DOOM2F.WAD",
	"DOOM2.WAD",
	"PLUTONIA.WAD",
	"TNT.WAD",
	"DOOMU.WAD",
	"DOOM.WAD",
	"DOOM1.WAD",
	"FREEDOOM.WAD",
	"FREEDM.WAD",
	"CHEX.WAD",
	""
};

AGOL_Solo::AGOL_Solo()
{
	SoloGameDialog = AG_WindowNew(AG_WINDOW_MODAL);
	AG_WindowSetCaptionS(SoloGameDialog, "Solo Game");
	AG_WindowSetGeometryAligned(SoloGameDialog, AG_WINDOW_MC, 600, 400);

	WadListsBox = CreateWadListsBox(SoloGameDialog);
	IwadBox = CreateIwadBox(WadListsBox);
	IwadList = CreateIwadList(IwadBox);
	PwadBox = CreatePwadBox(WadListsBox);
	PwadList = CreatePwadList(PwadBox);
	MainButtonBox = CreateMainButtonBox(SoloGameDialog);

	PopulateWadLists();

	CloseEventHandler = NULL;

	AG_WindowShow(SoloGameDialog);
}

AGOL_Solo::~AGOL_Solo()
{

}

AG_Box *AGOL_Solo::CreateWadListsBox(void *parent)
{
	AG_Box *wlbox;

	wlbox = AG_BoxNewHoriz(parent, AG_BOX_EXPAND | AG_BOX_HOMOGENOUS);

	return wlbox;
}

AG_Box *AGOL_Solo::CreateIwadBox(void *parent)
{
	AG_Box *iwbox;

	iwbox = AG_BoxNewVert(parent, AG_BOX_EXPAND | AG_BOX_FRAME);
	AG_LabelNewS(iwbox, 0, "Select IWAD");
	iwbox = AG_BoxNewVert(iwbox, AG_BOX_EXPAND);
	AG_BoxSetPadding(iwbox, 5);
	AG_BoxSetSpacing(iwbox, 5);

	return iwbox;
}

AG_Tlist *AGOL_Solo::CreateIwadList(void *parent)
{
	AG_Tlist *iwlist;

	iwlist = AG_TlistNew(parent, AG_TLIST_EXPAND);

	// Set the list item compare function
	AG_TlistSetCompareFn(iwlist, AG_TlistCompareStrings);

	return iwlist;
}

AG_Box *AGOL_Solo::CreatePwadBox(void *parent)
{
	AG_Box *pwbox;

	pwbox = AG_BoxNewVert(parent, AG_BOX_EXPAND | AG_BOX_FRAME);
	AG_LabelNewS(pwbox, 0, "Select WADs");
	pwbox = AG_BoxNewVert(pwbox, AG_BOX_EXPAND);
	AG_BoxSetPadding(pwbox, 5);
	AG_BoxSetSpacing(pwbox, 5);

	return pwbox;
}

AG_Tlist *AGOL_Solo::CreatePwadList(void *parent)
{
	AG_Tlist *pwlist;

	pwlist = AG_TlistNew(parent, AG_TLIST_MULTITOGGLE | AG_TLIST_EXPAND);

	// Set the list item compare function
	AG_TlistSetCompareFn(pwlist, AG_TlistCompareStrings);

	return pwlist;
}

AG_Box *AGOL_Solo::CreateMainButtonBox(void *parent)
{
	AG_Box *bbox;

	bbox = AG_BoxNewHoriz(parent, AG_BOX_HFILL);

	// This empty box positions the buttons to the right
	AG_BoxNewHoriz(bbox, AG_BOX_HFILL);

	AG_ButtonNewFn(bbox, 0, "Cancel", EventReceiver, "%p", 
			RegisterEventHandler((EVENT_FUNC_PTR)&AGOL_Solo::OnCancel));
	AG_ButtonNewFn(bbox, 0, "Launch", EventReceiver, "%p", 
			RegisterEventHandler((EVENT_FUNC_PTR)&AGOL_Solo::OnLaunch));

	return bbox;
}

void AGOL_Solo::PopulateWadLists()
{
	string       waddirs;
	list<string> waddirList;
	list<string>::iterator wditer;

	// Read the WadDirs option from the config file
	if(GuiConfig::Read("WadDirs", waddirs))
	{
		char cwd[AG_PATHNAME_MAX];

		// If there are no waddirs configured insert the current working directory
		if(!AG_GetCWD(cwd, AG_PATHNAME_MAX))
			waddirList.push_back(cwd);
	}
	else
	{
		size_t oldpos = 0;
		size_t pos = waddirs.find(PATH_DELIMITER);

		// Parse the waddirs option
		while(pos != string::npos)
		{
			// Put a wad path into the list
			waddirList.push_back(waddirs.substr(oldpos, pos - oldpos));
			oldpos = pos + 1;
			pos = waddirs.find(PATH_DELIMITER, oldpos);
		}
	}

	// Loop through the directory list and find all the wad files
	for(wditer = waddirList.begin(); wditer != waddirList.end(); ++wditer)
	{
		AG_Dir      *dir;
		AG_FileInfo  info;

		if(!(*wditer).size())
			continue;

		// Open the wad directory
		if((dir = AG_OpenDir((*wditer).c_str())) == NULL)
		{
			cerr << "Failed to open directory (" << *wditer << "): " << AG_GetError() << endl;
			continue;
		}

		// Loop through all the directory contents
		for(int i = 0; i < dir->nents; i++)
		{
			string curFile(dir->ents[i]);
			string path;

			if((*wditer).rbegin()[0] == AG_PATHSEPCHAR)
				path = *wditer + curFile;
			else
				path = *wditer + AG_PATHSEP + curFile;

			if(AG_GetFileInfo(path.c_str(), &info) == -1)
				continue;

			// Make sure this item is a regular file
			if(info.type == AG_FILE_REGULAR)
			{
				// Convert the wad name to uppercase
				transform(curFile.begin(), curFile.end(), curFile.begin(), ::toupper);

				// Find the last '.' in the file name
				size_t dot = curFile.rfind('.');

				// If the file has no extension move on to the next one.
				if(dot == string::npos)
					continue;

				// Compare the file extension to the acceptable extensions for Odamex.
				if(curFile.substr(dot + 1, curFile.size()) == "WAD")
				{
					// Exclude odamex.wad
					if(curFile == "ODAMEX.WAD")
						continue;

					// If this is an IWAD put it in the IWAD list.
					if(WadIsIWAD(curFile))
						AG_TlistAddS(IwadList, agIconDoc.s, dir->ents[i]);
					else
						AG_TlistAddS(PwadList, agIconDoc.s, dir->ents[i]);
				}
				else if(curFile.substr(dot + 1, curFile.size()) == "DEH" ||
						curFile.substr(dot + 1, curFile.size()) == "BEX")
					AG_TlistAddS(PwadList, agIconDoc.s, dir->ents[i]);
			}
		}

		AG_CloseDir(dir);
	}

	// Remove any duplicate items
	AG_TlistUniq(IwadList);
	AG_TlistUniq(PwadList);
}

bool AGOL_Solo::WadIsIWAD(const string &wad)
{
	string upperWad(wad);

	// Get the wad in all uppercase
	transform(wad.begin(), wad.end(), upperWad.begin(), ::toupper);

	// Compare to the supported iwad file names
	for(int i = 0; DoomIWadNames[i].size(); i++)
	{
		if(upperWad == DoomIWadNames[i])
			return true;
	}

	return false;
}

bool AGOL_Solo::PwadIsFileType(const string &wad, const string &extension)
{
	if(wad.size() > 0 && extension.size() > 0)
	{
		string upperWad(wad);
		string upperExt(extension);

		// Convert the wad name to uppercase
		transform(upperWad.begin(), upperWad.end(), upperWad.begin(), ::toupper);

		// Convert the extension to uppercase
		transform(upperExt.begin(), upperExt.end(), upperExt.begin(), ::toupper);

		size_t dot = upperWad.rfind('.');

		if(upperWad.substr(dot + 1, upperWad.size()) == upperExt)
			return true;
	}

	return false;
}

bool AGOL_Solo::PWadListContainsFileType(const string &extension)
{
	if(extension.size() > 0)
	{
		for(int i = 1; i <= PwadList->nitems; i++)
		{
			AG_TlistItem *selitem = AG_TlistFindByIndex(PwadList, i);

			if(selitem && strlen(selitem->text) > 0)
			{
				string upperExt(extension);
				string wad(selitem->text);

				// Convert the wad name to uppercase
				transform(wad.begin(), wad.end(), wad.begin(), ::toupper);

				// Convert the extension to uppercase
				transform(upperExt.begin(), upperExt.end(), upperExt.begin(), ::toupper);

				size_t dot = wad.rfind('.');

				// If the file has no extension move on to the next one.
				if(dot == string::npos)
					continue;

				if(wad.substr(dot + 1, wad.size()) == upperExt)
					return true;
			}
		}
	}
	return false;
}

void AGOL_Solo::OnCancel(AG_Event *event)
{
	// Detach and destroy the window + contents
	AG_ObjectDetach(SoloGameDialog);

	// Call the close handler if one was set
	if(CloseEventHandler)
		CloseEventHandler->Trigger(event);
}

void AGOL_Solo::OnLaunch(AG_Event *event)
{
	AG_TlistItem *selitem;
	GameCommand   cmd;
	string        wad;
	string        waddirs;
	string        extraParams;

	// Add the iwad parameter
	if(((selitem = AG_TlistSelectedItem(IwadList)) != NULL) && (strlen(selitem->text) > 0))
		cmd.AddParameter("-iwad", selitem->text);
	else // We can't continue if no iwad is selected
	{
		AG_TextErrorS("You must choose an IWAD!");
		return;
	}

	// Get the waddir option
	if(GuiConfig::Read("WadDirs", waddirs))
	{
		char cwd[AG_PATHNAME_MAX];

		// No waddirs are set so use CWD
		if(!AG_GetCWD(cwd, AG_PATHNAME_MAX))
			cmd.AddParameter("-waddir", cwd);
	}
	else
		cmd.AddParameter("-waddir", waddirs);

	// If there are selected items traverse the wad list
	if(AG_TlistSelectedItem(PwadList))
	{
		// WAD files
		if(PWadListContainsFileType("WAD"))
		{
			cmd.AddParameter("-file");

			// Find any selected pwads
			for(int i = 1; i <= PwadList->nitems; i++)
			{
				selitem = AG_TlistFindByIndex(PwadList, i);

				if(selitem && selitem->selected && strlen(selitem->text) > 0)
				{
					if(PwadIsFileType(selitem->text, "WAD"))
						cmd.AddParameter(selitem->text);
				}
			}
		}
		// Dehacked patches
		if(PWadListContainsFileType("DEH"))
		{
			cmd.AddParameter("-deh");

			// Find any selected pwads
			for(int i = 1; i <= PwadList->nitems; i++)
			{
				selitem = AG_TlistFindByIndex(PwadList, i);

				if(selitem && selitem->selected && strlen(selitem->text) > 0)
				{
					if(PwadIsFileType(selitem->text, "DEH"))
						cmd.AddParameter(selitem->text);
				}
			}
		}
		// BEX patches
		if(PWadListContainsFileType("BEX"))
		{
			cmd.AddParameter("-bex");

			// Find any selected pwads
			for(int i = 1; i <= PwadList->nitems; i++)
			{
				selitem = AG_TlistFindByIndex(PwadList, i);

				if(selitem && selitem->selected && strlen(selitem->text) > 0)
				{
					if(PwadIsFileType(selitem->text, "BEX"))
						cmd.AddParameter(selitem->text);
				}
			}
		}
	}

	// Add the extra parameters
	if(!GuiConfig::Read("ExtraParams", extraParams))
		cmd.AddParameter(extraParams);

	// Launch the game
	cmd.Launch();

	// Detach and destroy the window + contents
	AG_ObjectDetach(SoloGameDialog);

	// Call the close handler if one was set
	if(CloseEventHandler)
		CloseEventHandler->Trigger(event);
}

void AGOL_Solo::SetWindowCloseEvent(EventHandler *handler)
{
	if(handler)
	{
		CloseEventHandler = handler;

		AG_AddEvent(SoloGameDialog, "window-close", EventReceiver, "%p", handler);
	}
}

} // namespace
