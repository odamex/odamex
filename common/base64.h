// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1998-2006 by Randy Heit (ZDoom 1.22).
// Copyright (C) 2000-2006 by Sergey Makovkin (CSDoom .62).
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
//  Public Domain base64 implementation from Wikibooks
//  https://en.wikibooks.org/wiki/Algorithm_Implementation/Miscellaneous/Base64
//
//-----------------------------------------------------------------------------

#pragma once

#include "doomtype.h"

#include <string>
#include <vector>

std::string M_Base64Encode(std::vector<byte> inputBuffer);
bool M_Base64Decode(std::vector<byte>& decodedBytes, const std::string& input);
