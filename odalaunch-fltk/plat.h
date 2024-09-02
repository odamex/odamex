// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2006-2021 by The Odamex Team.
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
//  Platform-specific functions.
//
//-----------------------------------------------------------------------------

#include "odalaunch.h"

class Platform
{
public:
	virtual ~Platform() { }

	/**
	 * @brief Write a string to a debug log of some kind.
	 *
	 * @param str String to write.
	 */
	virtual void DebugOut(const char* str) = 0;

	/**
	 * @brief Execute Odamex.  WIP.
	 */
	virtual void ExecuteOdamex() = 0;

	/**
	 * @brief Get the abstraction singleton.
	 */
	static Platform& Get();
};
