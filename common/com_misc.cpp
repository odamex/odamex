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
// DESCRIPTION:
//   Common HUD functionality that can be called by the server as well.
//
//-----------------------------------------------------------------------------


#include "odamex.h"

#include "com_misc.h"

#include "c_dispatch.h"
#include "m_random.h"
#include "p_local.h"
#include "v_textcolors.h"

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

BEGIN_COMMAND(toast)
{
	toast_t toast;

	toast.flags = toast_t::LEFT | toast_t::ICON | toast_t::RIGHT;

	if (M_Random() % 2)
	{
		toast.left = std::string(TEXTCOLOR_LIGHTBLUE) + "[BLU]Ralphis";
		toast.right = std::string(TEXTCOLOR_BRICK) + "[RED]KBlair";
	}
	else
	{
		toast.left = std::string(TEXTCOLOR_BRICK) + "[RED]KBlair";
		toast.right = std::string(TEXTCOLOR_LIGHTBLUE) + "[BLU]Ralphis";
	}

	toast.icon = M_Random() % NUMMODS;

	COM_PushToast(toast);
}
END_COMMAND(toast)
