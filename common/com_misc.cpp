// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2006-2022 by The Odamex Team.
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
//   Common HUD functionality that can be called by the server as well.
//
//-----------------------------------------------------------------------------

#include "com_misc.h"

#include "version.h"

#if defined(SERVER_APP)
#include "svc_message.h"
#else
#include "st_stuff.h"
#endif

void COM_PushToast(const toast_t& toast)
{
#if defined(SERVER_APP)
	for (Players::iterator it = ::players.begin(); it != ::players.end(); ++it)
	{
		MSG_WriteSVC(&it->client.reliablebuf, SVC_Toast(toast));
	}
#else
	hud::PushToast(toast);
#endif
}
