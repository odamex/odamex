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

#include "ctrl_infobar.h"

#include <wx/window.h>
#include <wx/infobar.h>
#include <wx/sizer.h>
#include <wx/string.h>
#include <wx/utils.h>

OdaInfoBar::OdaInfoBar(wxWindow* parent) : m_Parent(parent)
{
	m_InfoBar = new wxInfoBar(m_Parent);

	m_Parent->GetSizer()->Prepend(m_InfoBar,
	                              wxSizerFlags().Expand().Border(0, 0).Proportion(0));

	// Always add an ignore button
	m_InfoBar->AddButton(wxID_EXIT, "Ignore");

	m_ButtonId = wxID_ANY;
}

OdaInfoBar::~OdaInfoBar()
{

}

// Removes any button and its state that is bound to a message
void OdaInfoBar::ResetButtonState()
{
	if(m_ButtonId != wxID_ANY)
	{
		m_InfoBar->Disconnect(m_ButtonId, wxEVT_BUTTON, m_ButtonFunc);
		m_InfoBar->RemoveButton(m_ButtonId);
	}

	// Ignore button gets removed regardless of state
	m_InfoBar->RemoveButton(wxID_EXIT);
}

void OdaInfoBar::ShowMessage(const wxString& Message, wxWindowID BtnId,
                             wxEventFunction BtnFunc, const wxString& BtnTxt)
{
	ResetButtonState();

	m_ButtonId = BtnId;
	m_ButtonFunc = BtnFunc;

	// We just want to show a general notification
	if(m_ButtonId == wxID_ANY)
	{
		// Always add an ignore button
		m_InfoBar->AddButton(wxID_EXIT, "Ignore");

		m_InfoBar->ShowMessage(Message);
		return;
	}

	m_InfoBar->AddButton(m_ButtonId, BtnTxt);
	// Always add an ignore button
	m_InfoBar->AddButton(wxID_EXIT, "Ignore");

	// The above code is not enough, since infobars are implemented as generic
	// and therefore not a native object in wxwidgets, so events do not
	// propagate to the parent window
	m_InfoBar->Connect(m_ButtonId, wxEVT_BUTTON, m_ButtonFunc);

	m_InfoBar->ShowMessage(Message);
}


void OdaInfoBar::ShowMessage(const wxString& Message)
{
	ShowMessage(Message, wxID_ANY, NULL, "");
}
