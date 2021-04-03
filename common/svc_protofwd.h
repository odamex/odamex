// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2021 by Alex Mayfield.
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
//   Server message functions.
//   - Functions should send exactly one message.
//   - Functions should be named after the message they send.
//   - Functions should be self-contained and not rely on global state.
//   - Functions should take a buf_t& as a first parameter.
//
//-----------------------------------------------------------------------------

#ifndef __SVCPROTOFWD_H__
#define __SVCPROTOFWD_H__

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
class DamagePlayer;
class DisconnectClient;
class LoadMap;
class ConsolePlayer;
class ExplodeMissile;
class RemoveMobj;
class UserInfo;
class UpdateMobj;
class SpawnPlayer;
class KillMobj;
class FireWeapon;
class UpdateSector;
class Print;
class PlayerMembers;
class TeamMembers;
class ActivateLine;
class MovingSector;
class PlaySound;
class TouchSpecial;
class ForceTeam;
class Switch;
class Say;
class CTFRefresh;
class CTFEvent;
class SecretEvent;
class ServerSettings;
class ConnectClient;
class MidPrint;
class PlayerState;
class LevelState;
class SectorProperties;
class ExecuteLineSpecial;
class ThinkerUpdate;
class NetdemoCap;
} // namespace svc
} // namespace odaproto

#endif // __SVCPROTOFWD_H__
