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

#include "../plat.h"

#include <unistd.h>

/******************************************************************************/
	
class PlatformPOSIX : public Platform
{
  public:
	virtual void DebugOut(const char* str) override;
	virtual void ExecuteOdamex() override;
};

/******************************************************************************/

void PlatformPOSIX::DebugOut(const char* str)
{
	fprintf(stderr, "%s", str);
}

/******************************************************************************/

void PlatformPOSIX::ExecuteOdamex()
{
	// TODO
}

/******************************************************************************/

Platform& Platform::Get()
{
	static PlatformPOSIX s_cPlatform;
	return s_cPlatform;
}
