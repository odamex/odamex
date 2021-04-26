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
//   Server message map.
//
//-----------------------------------------------------------------------------

#include "svc_map.h"

#include "server.pb.h"

#include "hashtable.h"
#include "i_net.h"

typedef OHashTable<const void*, svc_t> SVCMap;
SVCMap svcmap;

static void MapProto(const svc_t msg, const google::protobuf::Descriptor* desc)
{
	::svcmap.insert(SVCMap::value_type(desc, msg));
}

/**
 * @brief Initialize the SVC protocol descriptor map.
 */
static void InitMap()
{
	MapProto(svc_noop, odaproto::svc::Noop::descriptor());
	MapProto(svc_disconnect, odaproto::svc::Disconnect::descriptor());
	MapProto(svc_playerinfo, odaproto::svc::PlayerInfo::descriptor());
	MapProto(svc_moveplayer, odaproto::svc::MovePlayer::descriptor());
	MapProto(svc_updatelocalplayer, odaproto::svc::UpdateLocalPlayer::descriptor());
	MapProto(svc_levellocals, odaproto::svc::LevelLocals::descriptor());
	MapProto(svc_pingrequest, odaproto::svc::PingRequest::descriptor());
	MapProto(svc_updateping, odaproto::svc::UpdatePing::descriptor());
	MapProto(svc_spawnmobj, odaproto::svc::SpawnMobj::descriptor());
	MapProto(svc_disconnectclient, odaproto::svc::DisconnectClient::descriptor());
	MapProto(svc_loadmap, odaproto::svc::LoadMap::descriptor());
	MapProto(svc_consoleplayer, odaproto::svc::ConsolePlayer::descriptor());
	MapProto(svc_explodemissile, odaproto::svc::ExplodeMissile::descriptor());
	MapProto(svc_removemobj, odaproto::svc::RemoveMobj::descriptor());
	MapProto(svc_userinfo, odaproto::svc::UserInfo::descriptor());
	MapProto(svc_updatemobj, odaproto::svc::UpdateMobj::descriptor());
	MapProto(svc_spawnplayer, odaproto::svc::SpawnPlayer::descriptor());
	MapProto(svc_damageplayer, odaproto::svc::DamagePlayer::descriptor());
	MapProto(svc_killmobj, odaproto::svc::KillMobj::descriptor());
	MapProto(svc_fireweapon, odaproto::svc::FireWeapon::descriptor());
	MapProto(svc_updatesector, odaproto::svc::UpdateSector::descriptor());
	MapProto(svc_print, odaproto::svc::Print::descriptor());
	MapProto(svc_playermembers, odaproto::svc::PlayerMembers::descriptor());
	MapProto(svc_teammembers, odaproto::svc::TeamMembers::descriptor());
	MapProto(svc_activateline, odaproto::svc::ActivateLine::descriptor());
	MapProto(svc_movingsector, odaproto::svc::MovingSector::descriptor());
	MapProto(svc_playsound, odaproto::svc::PlaySound::descriptor());
	MapProto(svc_reconnect, odaproto::svc::Reconnect::descriptor());
	MapProto(svc_exitlevel, odaproto::svc::ExitLevel::descriptor());
	MapProto(svc_touchspecial, odaproto::svc::TouchSpecial::descriptor());
	MapProto(svc_forceteam, odaproto::svc::ForceTeam::descriptor());
	MapProto(svc_switch, odaproto::svc::Switch::descriptor());
	MapProto(svc_say, odaproto::svc::Say::descriptor());
	MapProto(svc_ctfrefresh, odaproto::svc::CTFRefresh::descriptor());
	MapProto(svc_ctfevent, odaproto::svc::CTFEvent::descriptor());
	MapProto(svc_secretevent, odaproto::svc::SecretEvent::descriptor());
	MapProto(svc_serversettings, odaproto::svc::ServerSettings::descriptor());
	MapProto(svc_connectclient, odaproto::svc::ConnectClient::descriptor());
	MapProto(svc_midprint, odaproto::svc::MidPrint::descriptor());
	MapProto(svc_servergametic, odaproto::svc::ServerGametic::descriptor());
	MapProto(svc_inttimeleft, odaproto::svc::IntTimeLeft::descriptor());
	MapProto(svc_fullupdatedone, odaproto::svc::FullUpdateDone::descriptor());
	MapProto(svc_railtrail, odaproto::svc::RailTrail::descriptor());
	MapProto(svc_playerstate, odaproto::svc::PlayerState::descriptor());
	MapProto(svc_levelstate, odaproto::svc::LevelState::descriptor());
	MapProto(svc_resetmap, odaproto::svc::ResetMap::descriptor());
	MapProto(svc_playerqueuepos, odaproto::svc::PlayerQueuePos::descriptor());
	MapProto(svc_fullupdatestart, odaproto::svc::FullUpdateStart::descriptor());
	MapProto(svc_lineupdate, odaproto::svc::LineUpdate::descriptor());
	MapProto(svc_sectorproperties, odaproto::svc::SectorProperties::descriptor());
	MapProto(svc_linesideupdate, odaproto::svc::LineUpdate::descriptor());
	MapProto(svc_mobjstate, odaproto::svc::MobjState::descriptor());
	MapProto(svc_damagemobj, odaproto::svc::DamageMobj::descriptor());
	MapProto(svc_executelinespecial, odaproto::svc::ExecuteLineSpecial::descriptor());
	MapProto(svc_executeacsspecial, odaproto::svc::ExecuteACSSpecial::descriptor());
	MapProto(svc_thinkerupdate, odaproto::svc::ThinkerUpdate::descriptor());
	MapProto(svc_vote_update, odaproto::svc::VoteUpdate::descriptor());
	MapProto(svc_maplist, odaproto::svc::Maplist::descriptor());
	MapProto(svc_maplist_update, odaproto::svc::MaplistUpdate::descriptor());
	MapProto(svc_maplist_index, odaproto::svc::MaplistIndex::descriptor());
	MapProto(svc_netdemocap, odaproto::svc::NetdemoCap::descriptor());
	MapProto(svc_netdemostop, odaproto::svc::NetDemoStop::descriptor());
	MapProto(svc_netdemoloadsnap, odaproto::svc::NetDemoLoadSnap::descriptor());
}

svc_t SVC_ResolveDescriptor(const google::protobuf::Descriptor* desc)
{
	if (::svcmap.empty())
	{
		InitMap();
	}

	SVCMap::iterator it = ::svcmap.find(desc);
	if (it == ::svcmap.end())
	{
		return svc_noop;
	}
	return it->second;
}
