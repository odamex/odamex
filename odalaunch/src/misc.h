// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id:$
//
// Copyright (C) 2006-2007 by The Odamex Team.
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


#include <wx/process.h>
#include <wx/tokenzr.h>

void AddServerToList(wxListCtrl *list, Server &s, wxInt32 index, wxInt8 insert = 1);
void AddPlayersToList(wxAdvancedListCtrl *list, Server &s);
void LaunchGame(Server &s, wxString waddirs);
int FindServerIndex(wxListCtrl *list, wxString Address);
