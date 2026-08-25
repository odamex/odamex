// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1998-2006 by Randy Heit (ZDoom).
// Copyright (C) 2006-2025 by The Odamex Team.
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
//   Handles parsing all ONCRINFO lumps.
//
//-----------------------------------------------------------------------------

#pragma once

/// <summary>
/// Parses all ONCRINFO lumps for consumption by
/// the AnnouncerManager object.
/// </summary>
void G_ParseOncrInfo();

/// <summary>
/// Parses a single ONCRINFO from a raw buffer and hands the announcers it defines to
/// the AnnouncerManager.
/// </summary>
/// <param name="buffer">Start of the ONCRINFO lump.</param>
/// <param name="length">Length of the ONCRINFO lump.</param>
void G_ParseOncrInfoBuffer(const char* buffer, size_t length);
