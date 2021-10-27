// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1998-2006 by Randy Heit (ZDoom).
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
//	Internal DeHackEd patch parsing
//
//-----------------------------------------------------------------------------


#ifndef __D_DEHACK_H__
#define __D_DEHACK_H__

#include "m_resfile.h"

// Sound equivalences. When a patch tries to change a sound,
// use these sound names.
static const char* SoundMap[] = {NULL,
                                 "weapons/pistol",
                                 "weapons/shotgf",
                                 "weapons/shotgr",
                                 "weapons/sshotf",
                                 "weapons/sshoto",
                                 "weapons/sshotc",
                                 "weapons/sshotl",
                                 "weapons/plasmaf",
                                 "weapons/bfgf",
                                 "weapons/sawup",
                                 "weapons/sawidle",
                                 "weapons/sawfull",
                                 "weapons/sawhit",
                                 "weapons/rocklf",
                                 "weapons/bfgx",
                                 "imp/attack",
                                 "imp/shotx",
                                 "plats/pt1_strt",
                                 "plats/pt1_stop",
                                 "doors/dr1_open",
                                 "doors/dr1_clos",
                                 "plats/pt1_mid",
                                 "switches/normbutn",
                                 "switches/exitbutn",
                                 "*pain100_1",
                                 "demon/pain",
                                 "grunt/pain",
                                 "vile/pain",
                                 "fatso/pain",
                                 "pain/pain",
                                 "misc/gibbed",
                                 "misc/i_pkup",
                                 "misc/w_pkup",
                                 "*land1",
                                 "misc/teleport",
                                 "grunt/sight1",
                                 "grunt/sight2",
                                 "grunt/sight3",
                                 "imp/sight1",
                                 "imp/sight2",
                                 "demon/sight",
                                 "caco/sight",
                                 "baron/sight",
                                 "cyber/sight",
                                 "spider/sight",
                                 "baby/sight",
                                 "knight/sight",
                                 "vile/sight",
                                 "fatso/sight",
                                 "pain/sight",
                                 "skull/melee",
                                 "demon/melee",
                                 "skeleton/melee",
                                 "vile/start",
                                 "imp/melee",
                                 "skeleton/swing",
                                 "*death1",
                                 "*xdeath1",
                                 "grunt/death1",
                                 "grunt/death2",
                                 "grunt/death3",
                                 "imp/death1",
                                 "imp/death2",
                                 "demon/death",
                                 "caco/death",
                                 "misc/unused",
                                 "baron/death",
                                 "cyber/death",
                                 "spider/death",
                                 "baby/death",
                                 "vile/death",
                                 "knight/death",
                                 "pain/death",
                                 "skeleton/death",
                                 "grunt/active",
                                 "imp/active",
                                 "demon/active",
                                 "baby/active",
                                 "baby/walk",
                                 "vile/active",
                                 "*grunt1",
                                 "world/barrelx",
                                 "*fist",
                                 "cyber/hoof",
                                 "spider/walk",
                                 "weapons/chngun",
                                 "misc/chat2",
                                 "doors/dr2_open",
                                 "doors/dr2_clos",
                                 "misc/spawn",
                                 "vile/firecrkl",
                                 "vile/firestrt",
                                 "misc/p_pkup",
                                 "brain/spit",
                                 "brain/cube",
                                 "brain/sight",
                                 "brain/pain",
                                 "brain/death",
                                 "fatso/attack",
                                 "gatso/death",
                                 "wolfss/sight",
                                 "wolfss/death",
                                 "keen/pain",
                                 "keen/death",
                                 "skeleton/active",
                                 "skeleton/sight",
                                 "skeleton/attack",
                                 "misc/chat",
                                 "misc/teamchat"};

void D_UndoDehPatch();
bool D_DoDehPatch(const OResFile* patchfile, const int lump);

#endif //__D_DEHACK_H__
