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
//	About Dialog
//
// AUTHORS:
//	 Michael Wood (mwoodj at huntsvegas dot org)
//
//-----------------------------------------------------------------------------

#include <iostream>

#include <agar/core.h>
#include <agar/gui.h>

#include "agol_about.h"
#include "net_packet.h"
#include "icons.h"

using namespace std;

namespace agOdalaunch {

static const string license(
			"This program is free software; you can redistribute it"
			" and/or modify it under the terms of the GNU General"
			" Public License as published by the Free Software"
			" Foundation; either version 2 of the License, or (at your"
			" option) any later version.\n\n"
			"This program is distributed in the hope that it will be"
			" useful, but WITHOUT ANY WARRANTY; without even the"
			" implied warranty of MERCHANTABILITY or FITNESS FOR"
			" A PARTICULAR PURPOSE. See the GNU General Public"
			" License for more details.");

AGOL_About::AGOL_About()
{
	AboutDialog = AG_WindowNew(AG_WINDOW_MODAL | AG_WINDOW_DIALOG);
	AG_WindowSetCaptionS(AboutDialog, "About The Odamex Launcher");
	AG_WindowSetGeometryAligned(AboutDialog, AG_WINDOW_MC, 375, 415);

	TopBox = CreateTopBox(AboutDialog);
	DevBox = CreateDevBox(AboutDialog);
	LicenseBox = CreateLicenseBox(AboutDialog);
	OKButton = CreateOKButton(AboutDialog);

	CloseEventHandler = NULL;

	LicenseCursorPos = 0;
	LicenseDirty = false;

	AG_WindowShow(AboutDialog);
}

AGOL_About::~AGOL_About()
{

}

AG_Box *AGOL_About::CreateTopBox(void *parent)
{
	AG_Box         *tbox;
	AG_Box         *box;
	AG_Label       *label;
	AG_DataSource  *pngdata;
	AG_AgarVersion  agv;

	tbox = AG_BoxNewHoriz(parent, 0);

	box = AG_BoxNewHoriz(tbox, 0);

	if((pngdata = AG_OpenConstCore(icon_odalaunch_96, sizeof(icon_odalaunch_96))) != NULL)
	{
		AG_PixmapFromSurface(box, AG_PIXMAP_EXPAND, AG_ReadSurfaceFromPNG(pngdata));
		AG_CloseDataSource(pngdata);
	}

	box = AG_BoxNewVert(tbox, AG_BOX_HFILL);
	label = AG_LabelNewS(box, AG_LABEL_HFILL, "The Odamex Launcher");
	AG_LabelJustify(label, AG_TEXT_CENTER);
	label = AG_LabelNewS(box, AG_LABEL_HFILL, "Copyright (C) 2010-2012 by The Odamex Team");
	AG_LabelJustify(label, AG_TEXT_CENTER);
	label = AG_LabelNew(box, AG_LABEL_HFILL, "Version %d.%d.%d - Protocol Version %d",
			VERSIONMAJOR(VERSION), VERSIONMINOR(VERSION), VERSIONPATCH(VERSION), PROTOCOL_VERSION);

	AG_LabelJustify(label, AG_TEXT_CENTER);
	label = AG_LabelNewS(box, AG_LABEL_HFILL, "http://www.odamex.net");
	AG_LabelJustify(label, AG_TEXT_CENTER);

	AG_GetVersion(&agv);
	label = AG_LabelNew(parent, AG_LABEL_HFILL, "Built with Agar, Version %d.%d.%d",
			agv.major, agv.minor, agv.patch);
	AG_LabelJustify(label, AG_TEXT_CENTER);

	label = AG_LabelNew(parent, AG_LABEL_HFILL, "(%s)", agv.release);
	AG_LabelJustify(label, AG_TEXT_CENTER);

	return tbox;
}

AG_Box *AGOL_About::CreateDevBox(void *parent)
{
	AG_Box   *dbox;
	AG_Label *label;

	dbox = AG_BoxNewVert(parent, AG_BOX_HFILL);
	AG_LabelNewS(dbox, 0, "Developed by");
	dbox = AG_BoxNewHoriz(dbox, AG_BOX_EXPAND);
	AG_BoxSetPadding(dbox, 5);
	AG_BoxSetSpacing(dbox, 5);
	dbox = AG_BoxNewHoriz(dbox, AG_BOX_FRAME | AG_BOX_EXPAND);

	label = AG_LabelNewS(dbox, AG_LABEL_EXPAND, 
			"Michael Wood (mailto:mwoodj@huntsvegas.org)\n"
			"Russell Rice (mailto:russell@odamex.net)");
	AG_LabelJustify(label, AG_TEXT_CENTER);

	return dbox;
}

AG_Box *AGOL_About::CreateLicenseBox(void *parent)
{
	AG_Box     *lbox;
	AG_Textbox *text;

	lbox = AG_BoxNewVert(parent, AG_BOX_EXPAND);
	AG_LabelNewS(lbox, 0, "License");
	lbox = AG_BoxNewHoriz(lbox, AG_BOX_EXPAND);
	AG_BoxSetPadding(lbox, 5);
	AG_BoxSetSpacing(lbox, 5);

#if !AG_VERSION_ATLEAST(1,4,2)
	text = AG_TextboxNewS(lbox, AG_TEXTBOX_MULTILINE | AG_TEXTBOX_EXPAND, "");
#else
	text = AG_TextboxNewS(lbox, AG_TEXTBOX_READONLY | AG_TEXTBOX_MULTILINE | AG_TEXTBOX_EXPAND, "");
#endif

	AG_TextboxSetWordWrap(text, true);

	AG_TextboxSetString(text, license.c_str());

#if !AG_VERSION_ATLEAST(1,4,2)
	AG_SetEvent(text, "textbox-prechg", EventReceiver, "%p",
		RegisterEventHandler((EVENT_FUNC_PTR)&AGOL_About::OnLicensePrechg));

	AG_SetEvent(text, "textbox-postchg", EventReceiver, "%p",
		RegisterEventHandler((EVENT_FUNC_PTR)&AGOL_About::OnLicensePostchg));
#else
	AG_TextboxSetCursorPos(text, 0);
#endif

	return lbox;
}

AG_Button *AGOL_About::CreateOKButton(void *parent)
{
	AG_Box    *bbox;
	AG_Button *button;

	bbox = AG_BoxNewHoriz(parent, AG_BOX_HFILL);

	// This empty box positions the button to the right
	AG_BoxNewHoriz(bbox, AG_BOX_HFILL);

	button = AG_ButtonNewFn(bbox, 0, "  OK  ", EventReceiver, "%p", 
			RegisterEventHandler((EVENT_FUNC_PTR)&AGOL_About::OnOK));

	return button;
}

//*************************//
// Event Handler Functions //
//*************************//

void AGOL_About::OnOK(AG_Event *event)
{
	// Detach and destroy the window + contents
	AG_ObjectDetach(AboutDialog);

	// Call the close handler if one was set
	if(CloseEventHandler)
		CloseEventHandler->Trigger(event);
}

void AGOL_About::OnLicensePrechg(AG_Event *event)
{
	AG_Textbox *text = static_cast<AG_Textbox*>(AG_SELF());

	// Store the cursor position before change
	LicenseCursorPos = AG_TextboxGetCursorPos(text);
}

void AGOL_About::OnLicensePostchg(AG_Event *event)
{
	// Protect against recursion
	if(LicenseDirty)
	{
		return;
	}

	LicenseDirty = true;

	AG_Textbox *text = static_cast<AG_Textbox*>(AG_SELF());

	// Force the widget to use the unmodified license text
	AG_TextboxSetString(text, license.c_str());

	// Return the cursor to the previous position so the view doesn't change
	AG_TextboxSetCursorPos(text, LicenseCursorPos);

	LicenseDirty = false;
}

//******************//
// Public Functions //
//******************//

void AGOL_About::SetWindowCloseEvent(EventHandler *handler)
{
	if(handler)
	{
		CloseEventHandler = handler;

		AG_AddEvent(AboutDialog, "window-close", EventReceiver, "%p", handler);
	}
}

} // namespace
