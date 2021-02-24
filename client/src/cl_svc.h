// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1998-2006 by Randy Heit (ZDoom).
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
//  Handlers for messages sent from the server.
//
//-----------------------------------------------------------------------------

#ifndef __CLSVC_H__
#define __CLSVC_H__

namespace svc
{
class DisconnectMsg;
class PlayerInfoMsg;
class MovePlayerMsg;
class UpdateLocalPlayerMsg;
class LevelLocalsMsg;
class PingRequestMsg;
class LoadMapMsg;
class KillMobjMsg;
class PlayerMembersMsg;
class TeamMembersMsg;
class MovingSectorMsg;
class PlayerStateMsg;
class LevelStateMsg;
class ThinkerUpdateMsg;
}

void CL_Noop();
void CL_Disconnect(const svc::DisconnectMsg& msg);
void CL_PlayerInfo(const svc::PlayerInfoMsg& msg);
void CL_MovePlayer(const svc::MovePlayerMsg& msg);
void CL_UpdateLocalPlayer(const svc::UpdateLocalPlayerMsg& msg);
void CL_LevelLocals(const svc::LevelLocalsMsg& msg);
void CL_PingRequest(const svc::PingRequestMsg& msg);
void CL_LoadMap(const svc::LoadMapMsg& msg);
void CL_KillMobj(const svc::KillMobjMsg& msg);
void CL_PlayerMembers(const svc::PlayerMembersMsg& msg);
void CL_TeamMembers(const svc::TeamMembersMsg& msg);
void CL_MovingSector(const svc::MovingSectorMsg& msg);
void CL_PlayerState(const svc::PlayerStateMsg& msg);
void CL_LevelState(const svc::LevelStateMsg& msg);
void CL_ThinkerUpdate(const svc::ThinkerUpdateMsg& msg);

#endif // __CLSVC_H__
