// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2006-2009 by The Odamex Team.
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
//	Miscellaneous stuff for gui
//	AUTHOR:	Russell Rice, John D Corrado
//
//-----------------------------------------------------------------------------

#ifndef MISC_H
#define MISC_H

#include <wx/process.h>
#include <wx/tokenzr.h>
#include "lst_custom.h"

#define BOOLSTR(b) ((b) ? wxT("Yes") : wxT("No"))

void SetupServerListColumns(wxAdvancedListCtrl *list);
void SetupPlayerListHeader(wxAdvancedListCtrl *list);
void AddServerToList(wxAdvancedListCtrl *list, Server &s, wxInt32 index, wxInt8 insert = 1);
void AddPlayersToList(wxAdvancedListCtrl *list, Server &s);
void LaunchGame(wxString Address, wxString ODX_Path, wxString waddirs, wxString Password = _T(""));
#endif
