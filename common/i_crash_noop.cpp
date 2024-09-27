// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
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
//
// DESCRIPTION:
//   Noop Crash handling.
//
//-----------------------------------------------------------------------------


#if defined _WIN32 && !defined _XBOX && defined _MSC_VER && !defined _DEBUG
#elif defined UNIX && defined HAVE_BACKTRACE && !defined GEKKO
#else

#include "odamex.h"

#include "i_crash.h"

void I_SetCrashCallbacks()
{
	// Not implemented.
}

void I_SetCrashDir(const char* crashdir)
{
	// Not implemented.
}

#endif
