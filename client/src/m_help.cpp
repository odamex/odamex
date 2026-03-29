// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 2006-2026 by The Odamex Team.
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
//   Help/Info screens
//
//-----------------------------------------------------------------------------


#include "odamex.h"

#include "cl_responderkeys.h"
#include "d_pages.h"
#include "gstrings.h"
#include "gi.h"
#include "i_video.h"
#include "m_help.h"
#include "m_menu.h"
#include "w_wad.h"

namespace
{
	page_image_t helpPage;
	int helpPageIndex = 0;

	int HelpPageCount()
	{
		int count = 0;

		for (int i = 0; i < 3; ++i)
		{
			const OLumpName& page = gameinfo.infoPage[i];
			if (page.empty())
			{
				break;
			}

			bool duplicate = false;
			for (int j = 0; j < i; ++j)
			{
				if (iequals(page, gameinfo.infoPage[j]))
				{
					duplicate = true;
					break;
				}
			}

			if (duplicate)
			{
				break;
			}

			++count;
		}

		return count;
	}

	void FinishHelpScreen()
	{
		M_ClearMenus();
		M_OpenMenuEntrypoint("mainMenu");
	}

	void AdvanceHelpScreen()
	{
		const int pageCount = HelpPageCount();
		if (pageCount <= 0 || helpPageIndex + 1 >= pageCount)
		{
			FinishHelpScreen();
			return;
		}

		++helpPageIndex;
		D_LoadPageImage(helpPage, gameinfo.infoPage[helpPageIndex]);
	}
}

bool M_HelpOpen()
{
	const int pageCount = HelpPageCount();
	if (pageCount <= 0)
	{
		return false;
	}

	helpPageIndex = 0;
	D_LoadPageImage(helpPage, gameinfo.infoPage[0]);
	return true;
}

void M_HelpDrawer(int)
{
	D_DrawPageImage(helpPage, I_GetPrimarySurface(), true);
}

void M_HelpResponder(int ch, int, bool, int&)
{
	if (Key_IsAcceptKey(ch))
	{
		AdvanceHelpScreen();
	}
	else if (Key_IsCancelKey(ch))
	{
		M_PopMenuStack();
	}
}

void M_HelpRestore(int& currentItem)
{
	currentItem = 0;
}

void M_HelpShutdown()
{
	D_FreePageImage(helpPage);
	helpPageIndex = 0;
}
