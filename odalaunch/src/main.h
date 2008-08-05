// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2006-2008 by The Odamex Team.
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
//	AUTHOR:	Russell Rice, John D Corrado
//
//-----------------------------------------------------------------------------


#ifndef __MAIN_H__
#define __MAIN_H__

#include <wx/wxprec.h>

#include "dlg_main.h"

#ifdef __BORLANDC__
	#pragma hdrstop
#endif

#ifndef WX_PRECOMP
	#include <wx/wx.h>
#endif

class Application : public wxApp
{
	public:
		bool OnInit();
        
		dlgMain* MAIN_DIALOG;
};

#endif
