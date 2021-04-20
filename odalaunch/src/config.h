// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2006-2020 by The Odamex Team.
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
//  Config object that saves where Odamex usually saves configs.
//
//-----------------------------------------------------------------------------

#ifndef __CONFIG__
#define __CONFIG__

#include "odalaunch.h"

#include <wx/filename.h>

/**
 * Create an wxFileConfig open to the default Odalaunch config path.
 *
 * [AM] Because wxFileConfig doesn't have a copy ctor (and thus can't be
 *      created in a factory function) this is necessary.
 */
#define ODALAUNCH_CONFIG(ident)                                              \
	wxFileConfig ident(wxEmptyString, wxEmptyString,                         \
	                   config::GetConfigFile().GetFullPath(), wxEmptyString, \
	                   wxCONFIG_USE_LOCAL_FILE, wxConvAuto());

namespace config
{
wxFileName GetWriteDir();
wxFileName GetConfigFile();
} // namespace config

#endif
