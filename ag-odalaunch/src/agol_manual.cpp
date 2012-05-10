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
//	Manual Connect Dialog
//
// AUTHORS:
//	 Michael Wood (mwoodj at huntsvegas dot org)
//
//-----------------------------------------------------------------------------

#include <iostream>
#include <string>

#include <agar/core.h>
#include <agar/gui.h>

#include "agol_manual.h"
#include "game_command.h"
#include "gui_config.h"

using namespace std;

namespace agOdalaunch {

AGOL_Manual::AGOL_Manual()
{
	ManualDialog = AG_WindowNew(AG_WINDOW_MODAL);
	AG_WindowSetCaptionS(ManualDialog, "Manually connect to an Odamex server");

	MainBox = CreateMainBox(ManualDialog);
	ServerEntry = CreateServerEntry(MainBox);
	PasswordEntry = CreatePasswordEntry(MainBox);
	ButtonBox = CreateButtonBox(ManualDialog);

	AG_WindowSetGeometryAligned(ManualDialog, AG_WINDOW_MC, 300, -1);

	CloseEventHandler = NULL;

	AG_WindowShow(ManualDialog);
}

AGOL_Manual::~AGOL_Manual()
{

}

AG_Box *AGOL_Manual::CreateMainBox(void *parent)
{
	AG_Box *box;

	box = AG_BoxNewVert(parent, AG_BOX_FRAME | AG_BOX_EXPAND);

	return box;
}

AG_Textbox *AGOL_Manual::CreateServerEntry(void *parent)
{
	AG_Box     *entryBox;
	AG_Textbox *sentry;

	AG_LabelNewS(parent, AG_LABEL_HFILL, "Please enter IP Address and Port");

	entryBox = AG_BoxNewHoriz(parent, AG_BOX_HFILL);
	AG_BoxSetPadding(entryBox, 5);
	AG_BoxSetSpacing(entryBox, 5);

	sentry = AG_TextboxNew(entryBox, AG_TEXTBOX_HFILL, NULL);

	AG_TextboxSetString(sentry, "0.0.0.0:0");

	return sentry;
}

AG_Textbox *AGOL_Manual::CreatePasswordEntry(void *parent)
{
	AG_Box     *entryBox;
	AG_Textbox *pentry;

	AG_LabelNewS(parent, AG_LABEL_HFILL, "Please enter server password (optional)");

	entryBox = AG_BoxNewHoriz(parent, AG_BOX_HFILL);
	AG_BoxSetPadding(entryBox, 5);
	AG_BoxSetSpacing(entryBox, 5);

	pentry = AG_TextboxNew(entryBox, AG_TEXTBOX_HFILL, NULL);

	AG_TextboxSetPassword(pentry, true);

	return pentry;
}

AG_Box *AGOL_Manual::CreateButtonBox(void *parent)
{
	AG_Box    *bbox;

	bbox = AG_BoxNewHoriz(parent, AG_BOX_HFILL);

	// This empty box positions the buttons to the right
	AG_BoxNewHoriz(bbox, AG_BOX_HFILL);

	AG_ButtonNewFn(bbox, 0, "  Cancel ", EventReceiver, "%p", 
			RegisterEventHandler((EVENT_FUNC_PTR)&AGOL_Manual::OnCancel));
	AG_ButtonNewFn(bbox, 0, "  OK  ", EventReceiver, "%p", 
			RegisterEventHandler((EVENT_FUNC_PTR)&AGOL_Manual::OnOK));

	return bbox;
}

//*************************//
// Event Handler Functions //
//*************************//
void AGOL_Manual::OnCancel(AG_Event *event)
{
	// Detach and destroy the window + contents
	AG_ObjectDetach(ManualDialog);

	// Call the close handler if one was set
	if(CloseEventHandler)
		CloseEventHandler->Trigger(event);
}

void AGOL_Manual::OnOK(AG_Event *event)
{
	GameCommand  cmd;
	string       waddirs;
	char        *server = NULL;
	char        *password = NULL;

	server = AG_TextboxDupString(ServerEntry);

	if(server && strlen(server) > 0)
	{
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


		cmd.AddParameter("-connect", server);

		password = AG_TextboxDupString(PasswordEntry);

		if(password && strlen(password) > 0)
		{
			cmd.AddParameter(password);
		}

		cmd.Launch();
	}

	if(server)
		free(server);

	if(password)
		free(password);

	// Detach and destroy the window + contents
	AG_ObjectDetach(ManualDialog);

	// Call the close handler if one was set
	if(CloseEventHandler)
		CloseEventHandler->Trigger(event);
}

//******************//
// Public Functions //
//******************//

void AGOL_Manual::SetWindowCloseEvent(EventHandler *handler)
{
	if(handler)
	{
		CloseEventHandler = handler;

		AG_AddEvent(ManualDialog, "window-close", EventReceiver, "%p", handler);
	}
}

} // namespace
