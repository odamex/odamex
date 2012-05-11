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
//	Main application sequence
//
// AUTHORS: 
//  John Corrado
//  Russell Rice (russell at odamex dot net)
//  Michael Wood (mwoodj at huntsvegas dot org)
//
//-----------------------------------------------------------------------------


#ifndef __MAIN_H__
#define __MAIN_H__

#include <wx/app.h>

#include "dlg_main.h"

class Application : public wxApp
{
	public:
		bool OnInit();
        wxInt32 OnExit();
        
		dlgMain* MAIN_DIALOG;
};

#endif
