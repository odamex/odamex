// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
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
//   Functions regarding reading and interpreting MAPINFO lumps.
//
//-----------------------------------------------------------------------------

#pragma once

#include <string>
#include "actor.h"

inline struct musinfo_t
{
    AActor::AActorPtr mapthing;
    AActor::AActorPtr lastmapthing;
    int tics = 0;
    std::string savedmusic;
} musinfo;

void G_ParseMusInfo();
void P_CheckMusicChange();
void S_ClearMusInfo();
void P_SerializeMusInfo(FArchive &arc);