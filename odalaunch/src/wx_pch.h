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
//	This file was generated when the project was created 
//	in CodeBlocks
//
//-----------------------------------------------------------------------------


#ifndef WX_PCH_H_INCLUDED
#define WX_PCH_H_INCLUDED

#if ( defined(USE_PCH) && !defined(WX_PRECOMP) )
	#define WX_PRECOMP
#endif // USE_PCH

// basic wxWidgets headers
#include <wx/wxprec.h>

#ifdef __BORLANDC__
	#pragma hdrstop
#endif

#ifndef WX_PRECOMP
	#include <wx/wx.h>
#endif

#ifdef USE_PCH
    #include "net_error.h"
    #include "net_io.h"
    #include "net_packet.h"
    #include "net_utils.h"
    #include "typedefs.h"
    
    #include "lst_custom.h"
    #include "main.h"
    #include "md5.h"
    #include "query_thread.h"
    #include "resource.h"
    
    #include "dlg_about.h"
#endif // USE_PCH

#endif // WX_PCH_H_INCLUDED
