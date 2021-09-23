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

#pragma once



/**
 * @brief Toast for infofeed - these can be constructed from anywhere.
 */
struct toast_t
{
	enum flags
	{
		LEFT = BIT(0),
		RIGHT = BIT(1),
		ICON = BIT(2),
		HIGHLIGHT = BIT(3),
		LEFT_PLUS = BIT(4),
		RIGHT_PLUS = BIT(5)
	};

	uint32_t flags;
	std::string left;
	std::string right;
	int icon;
	int pid_highlight;
	std::string left_plus;
	std::string right_plus;

	toast_t()
	    : flags(0), left(""), right(""), icon(-1), pid_highlight(-1), left_plus(""),
	      right_plus("")
	{
	}

	toast_t(const toast_t& other)
	{
		flags = other.flags;
		left = other.left;
		right = other.right;
		icon = other.icon;
		pid_highlight = other.pid_highlight;
		left_plus = other.left_plus;
		right_plus = other.right_plus;
	}
};

void COM_PushToast(const toast_t& toast);
