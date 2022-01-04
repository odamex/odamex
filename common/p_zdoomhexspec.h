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
//  [Blair] Define the ZDoom (Doom in Hexen) format doom map spec.
//  Includes sector specials, linedef types, line crossing.
//
//-----------------------------------------------------------------------------

//#ifndef __P_ZDOOMHEXSPEC__
//#define __P_ZDOOMHEXSPEC__
//
//void P_CrossZDoomSpecialLine(line_t* line, int side, AActor* thing, bool bossaction);
//lineresult_s P_ActivateZDoomLine(line_t* line, AActor* mo, int side,
//                                 unsigned int activationType);
//void P_CollectSecretZDoom(sector_t* sector, player_t* player);
//bool P_TestActivateZDoomLine(line_t* line, AActor* mo, int side,
//                             unsigned int activationType);
//bool P_ExecuteZDoomLineSpecial(int special, byte* args, line_t* line, int side,
//                               AActor* mo);
//BOOL EV_DoZDoomFloor(DFloor::EFloor floortype, line_t* line, int tag, fixed_t speed,
//                     fixed_t height, int crush, int change, bool hexencrush,
//                     bool hereticlower);
//BOOL EV_DoZDoomCeiling(DCeiling::ECeiling type, line_t* line, byte tag, fixed_t speed,
//                       fixed_t speed2, fixed_t height, int crush, byte silent, int change,
//                       crushmode_e crushmode);
//static void P_SetTransferHeightBlends(side_t* sd, const mapsidedef_t* msd);
//static void SetTextureNoErr(short* texture, unsigned int* color, char* name);
//
//#endif