// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2006-2015 by The Odamex Team.
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
//  Pop-down info bar for the main window, for now it is only used for
// notifications of updates.
//
//-----------------------------------------------------------------------------

#ifndef __CTRL_INFOBAR__
#define __CTRL_INFOBAR__

#include <wx/window.h>
#include <wx/infobar.h>
#include <wx/string.h>

class OdaInfoBar
{
public:
	OdaInfoBar(wxWindow* parent);
	~OdaInfoBar();

	// Shows a message
	void ShowMessage(const wxString& Message);

	// Shows a message with a button, the click event propagates through
	// the parent window specified in this classes constructor
	void ShowMessage(const wxString& Message,
	                 wxWindowID BtnId, wxEventFunction BtnFunc,
	                 const wxString& BtnTxt);

protected:
	void ResetButtonState();

private:
	wxWindow* m_Parent;
	wxEventFunction m_ButtonFunc;
	wxWindowID m_ButtonId;
	wxInfoBar* m_InfoBar;
};

#endif // __CTRL_INFOBAR__
