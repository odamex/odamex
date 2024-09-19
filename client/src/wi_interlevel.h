// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
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
//		Intermission screens.
//
//-----------------------------------------------------------------------------

#pragma once

#include "odamex.h"

enum class animcondition_t
{
    None,
    CurrMapGreater,   // Current map number is greater than the param value
    CurrMapEqual,     // Current map number is equal to the param value
    MapVisited,       // The map number corresponding to the param value has been visited
    CurrMapNotSecret, // The current map is not a secret map
    AnySecretVisited, // Any secret map has been visited
    OnFinishedScreen, // The current screen is the "finished" screen
    OnEnteringScreen, // The current screen is the “entering” screen

    Max,
};

struct interlevelcond_t
{
    animcondition_t condition;
    int param;
};

struct interlevelframe_t
{
    enum frametype_t
    {
        None          = 0x0000,
        DurationInf   = 0x0001,
        DurationFixed = 0x0002,
        DurationRand  = 0x0004,
        RandomStart   = 0x1000,
        Valid         = 0x100F
    };

    OLumpName imagelump;
    int imagelumpnum;
    frametype_t type;
    int duration;
    int maxduration;
};

struct interlevelanim_t
{
    std::vector<interlevelframe_t> frames;
    std::vector<interlevelcond_t> conditions;
    int xpos;
    int ypos;
};

struct interlevellayer_t
{
    std::vector<interlevelanim_t> anims;
    std::vector<interlevelcond_t> conditions;
};

struct interlevel_t
{
    OLumpName musiclump;
    OLumpName backgroundlump;
    std::vector<interlevellayer_t> layers;
};

interlevel_t* WI_GetInterlevel(const char* lumpname);
void WI_ClearInterlevels();