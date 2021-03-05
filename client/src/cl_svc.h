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

namespace odaproto
{
namespace svc
{
class Disconnect;
class PlayerInfo;
class MovePlayer;
class UpdateLocalPlayer;
class LevelLocals;
class PingRequest;
class UpdatePing;
class SpawnMobj;
class DisconnectClient;
class LoadMap;
class ConsolePlayer;
class ExplodeMissile;
class UpdateMobj;
class KillMobj;
class PlayerMembers;
class TeamMembers;
class ActivateLine;
class MovingSector;
class PlayerState;
class LevelState;
class SectorProperties;
class ExecuteLineSpecial;
class ThinkerUpdate;
class NetdemoCap;

} // namespace svc

} // namespace odaproto

void CL_Noop();
void CL_Disconnect(const odaproto::svc::Disconnect& msg);
void CL_PlayerInfo(const odaproto::svc::PlayerInfo& msg);
void CL_MovePlayer(const odaproto::svc::MovePlayer& msg);
void CL_UpdateLocalPlayer(const odaproto::svc::UpdateLocalPlayer& msg);
void CL_LevelLocals(const odaproto::svc::LevelLocals& msg);
void CL_PingRequest(const odaproto::svc::PingRequest& msg);
void CL_UpdatePing(const odaproto::svc::UpdatePing& msg);
void CL_SpawnMobj(const odaproto::svc::SpawnMobj& msg);
void CL_DisconnectClient(const odaproto::svc::DisconnectClient& msg);
void CL_LoadMap(const odaproto::svc::LoadMap& msg);
void CL_ConsolePlayer(const odaproto::svc::ConsolePlayer& msg);
void CL_ExplodeMissile(const odaproto::svc::ExplodeMissile& msg);
void CL_UpdateMobj(const odaproto::svc::UpdateMobj& msg);
void CL_KillMobj(const odaproto::svc::KillMobj& msg);
void CL_PlayerMembers(const odaproto::svc::PlayerMembers& msg);
void CL_TeamMembers(const odaproto::svc::TeamMembers& msg);
void CL_ActivateLine(const odaproto::svc::ActivateLine& msg);
void CL_MovingSector(const odaproto::svc::MovingSector& msg);
void CL_PlayerState(const odaproto::svc::PlayerState& msg);
void CL_LevelState(const odaproto::svc::LevelState& msg);
void CL_SectorProperties(const odaproto::svc::SectorProperties& msg);
void CL_ExecuteLineSpecial(const odaproto::svc::ExecuteLineSpecial& msg);
void CL_ThinkerUpdate(const odaproto::svc::ThinkerUpdate& msg);
void CL_NetdemoCap(const odaproto::svc::NetdemoCap& msg);

#endif // __CLSVC_H__
