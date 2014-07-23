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
//  wxWidgets-specific fixes and workarounds that apply to different platforms
//  that this program runs under
//
//-----------------------------------------------------------------------------

#ifndef __PLAT_UTILS__
#define __PLAT_UTILS__

#include <wx/frame.h>
#include <wx/icon.h>

// Windows
void wxMswFixTitlebarIcon(WXWidget Handle, wxIcon MainIcon);

// OSX
void wxMacRemoveFileMenu(wxFrame *parent);

#endif // __PLAT_UTILS__
