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
//		ID24 intermission screens.
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

    ID24Max,

    // vv extra conditions for zdoom intermission scripts
    CurrMapNotEqual,
    MapNotVisited,
    TravelingBetween,
    NotTravelingBetween,
};

struct interlevelcond_t
{
    animcondition_t condition;
    int param1;
    int param2;

    interlevelcond_t(animcondition_t c = animcondition_t::None, int p1 = 0, int p2 = 0) : condition(c), param1(p1), param2(p2) {}
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
    OLumpName altimagelump;
    int altimagelumpnum;
    frametype_t type;
    int duration;
    int maxduration;

    interlevelframe_t(OLumpName il = "", int iln = 0, OLumpName ail = "", int ailn = 0, frametype_t t = None, int d = 0, int md = 0) :
        imagelump(il), imagelumpnum(iln), altimagelump(ail), altimagelumpnum(0), type(t), duration(d), maxduration(md) {}
};

struct interlevelanim_t
{
    std::vector<interlevelframe_t> frames;
    std::vector<interlevelcond_t> conditions;
    int xpos;
    int ypos;

    interlevelanim_t(std::vector<interlevelframe_t> f = {}, std::vector<interlevelcond_t> c = {}, int x = 0, int y = 0) : frames(f), conditions(c), xpos(x), ypos(y) {}
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
interlevel_t* WI_GetIntermissionScript(const char* lumpname);
void WI_ClearInterlevels();