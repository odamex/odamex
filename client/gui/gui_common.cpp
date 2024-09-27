// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 2006-2024 by The Odamex Team.
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
//	Boot GUI for WAD selection.
//
//-----------------------------------------------------------------------------

#include "gui_common.h"

#include "FL/Fl_PNG_Image.H"
#include "FL/Fl_Window.H"

#include "gui_resource.h"

#if defined(_WIN32) && !defined(_XBOX)

#include "FL/x.H"
#include "win32inc.h"

void GUI_SetIcon(Fl_Window* win)
{
	const HICON icon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(1));
	win->icon((const void*)icon);
}

#else

void GUI_SetIcon(Fl_Window* win) { }

#endif

Fl_Image* GUIRes::icon_odamex_128()
{
	static Fl_Image* image = new Fl_PNG_Image("icon_odamex_128", __icon_odamex_128_png,
	                                          __icon_odamex_128_png_len);
	return image;
}
