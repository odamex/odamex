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

static void MapProto(const google::protobuf::Descriptor* desc, svc_t msg)
{
	::svcmap.insert(SVCMap::value_type(desc, msg));
}

/**
 * @brief Initialize the SVC protocol descriptor map.
 */
static void InitMap()
{
	MapProto(odaproto::svc::Disconnect::descriptor(), svc_disconnect);
	MapProto(odaproto::svc::PlayerInfo::descriptor(), svc_playerinfo);
	MapProto(odaproto::svc::MovePlayer::descriptor(), svc_moveplayer);
	MapProto(odaproto::svc::UpdateLocalPlayer::descriptor(), svc_updatelocalplayer);
	MapProto(odaproto::svc::LevelLocals::descriptor(), svc_levellocals);
	MapProto(odaproto::svc::PingRequest::descriptor(), svc_pingrequest);
	MapProto(odaproto::svc::UpdatePing::descriptor(), svc_updateping);
	MapProto(odaproto::svc::SpawnMobj::descriptor(), svc_spawnmobj);
	MapProto(odaproto::svc::DisconnectClient::descriptor(), svc_disconnectclient);
	MapProto(odaproto::svc::LoadMap::descriptor(), svc_loadmap);
	MapProto(odaproto::svc::ConsolePlayer::descriptor(), svc_consoleplayer);
	MapProto(odaproto::svc::ExplodeMissile::descriptor(), svc_explodemissile);
	MapProto(odaproto::svc::RemoveMobj::descriptor(), svc_removemobj);
	MapProto(odaproto::svc::UserInfo::descriptor(), svc_userinfo);
	MapProto(odaproto::svc::UpdateMobj::descriptor(), svc_updatemobj);
	MapProto(odaproto::svc::SpawnPlayer::descriptor(), svc_spawnplayer);
	MapProto(odaproto::svc::DamagePlayer::descriptor(), svc_damageplayer);
	MapProto(odaproto::svc::KillMobj::descriptor(), svc_killmobj);
	MapProto(odaproto::svc::FireWeapon::descriptor(), svc_fireweapon);
	MapProto(odaproto::svc::UpdateSector::descriptor(), svc_updatesector);
	MapProto(odaproto::svc::Print::descriptor(), svc_print);
	MapProto(odaproto::svc::PlayerMembers::descriptor(), svc_playermembers);
	MapProto(odaproto::svc::TeamMembers::descriptor(), svc_teammembers);
	MapProto(odaproto::svc::ActivateLine::descriptor(), svc_activateline);
	MapProto(odaproto::svc::MovingSector::descriptor(), svc_movingsector);
	MapProto(odaproto::svc::StartSound::descriptor(), svc_startsound);
	MapProto(odaproto::svc::Reconnect::descriptor(), svc_reconnect);
	MapProto(odaproto::svc::ExitLevel::descriptor(), svc_exitlevel);
	MapProto(odaproto::svc::TouchSpecial::descriptor(), svc_touchspecial);
	// svc_changeweapon
	MapProto(odaproto::svc::MissedPacket::descriptor(), svc_missedpacket);
	MapProto(odaproto::svc::SoundOrigin::descriptor(), svc_soundorigin);
	MapProto(odaproto::svc::ForceTeam::descriptor(), svc_forceteam);
	MapProto(odaproto::svc::Switch::descriptor(), svc_switch);
	MapProto(odaproto::svc::Say::descriptor(), svc_say);
	// svc_spawnhiddenplayer
	// svc_updatedeaths
	MapProto(odaproto::svc::CTFEvent::descriptor(), svc_ctfevent);
	MapProto(odaproto::svc::SecretEvent::descriptor(), svc_secretevent);
	MapProto(odaproto::svc::ServerSettings::descriptor(), svc_serversettings);
	MapProto(odaproto::svc::ConnectClient::descriptor(), svc_connectclient);
	MapProto(odaproto::svc::MidPrint::descriptor(), svc_midprint);
	MapProto(odaproto::svc::ServerGametic::descriptor(), svc_svgametic);
	MapProto(odaproto::svc::IntermissionTimeleft::descriptor(), svc_inttimeleft);
	// svc_mobjtranslation
	MapProto(odaproto::svc::FullUpdateDone::descriptor(), svc_fullupdatedone);
	MapProto(odaproto::svc::RailTrail::descriptor(), svc_railtrail);
	MapProto(odaproto::svc::PlayerState::descriptor(), svc_playerstate);
	MapProto(odaproto::svc::LevelState::descriptor(), svc_levelstate);
	MapProto(odaproto::svc::ResetMap::descriptor(), svc_resetmap);
	MapProto(odaproto::svc::PlayerQueuePos::descriptor(), svc_playerqueuepos);
	MapProto(odaproto::svc::FullUpdateStart::descriptor(), svc_fullupdatestart);
	MapProto(odaproto::svc::LineUpdate::descriptor(), svc_lineupdate);
	MapProto(odaproto::svc::SectorProperties::descriptor(), svc_sectorproperties);
	MapProto(odaproto::svc::LineUpdate::descriptor(), svc_linesideupdate);
	MapProto(odaproto::svc::MobjState::descriptor(), svc_mobjstate);
	MapProto(odaproto::svc::DamageMobj::descriptor(), svc_damagemobj);
	MapProto(odaproto::svc::ExecuteLineSpecial::descriptor(), svc_executelinespecial);
	MapProto(odaproto::svc::ExecuteAcsSpecial::descriptor(), svc_executeacsspecial);
	MapProto(odaproto::svc::ThinkerUpdate::descriptor(), svc_thinkerupdate);
	MapProto(odaproto::svc::NetdemoCap::descriptor(), svc_netdemocap);
	MapProto(odaproto::svc::NetdemoStop::descriptor(), svc_netdemostop);
	MapProto(odaproto::svc::NetdemoLoadSnap::descriptor(), svc_netdemoloadsnap);
	MapProto(odaproto::svc::VoteUpdate::descriptor(), svc_vote_update);
	MapProto(odaproto::svc::Maplist::descriptor(), svc_maplist);
	MapProto(odaproto::svc::MaplistUpdate::descriptor(), svc_maplist_update);
	MapProto(odaproto::svc::MaplistIndex::descriptor(), svc_maplist_index);
	// svc_compressed
	// svc_launcher_challenge
	// svc_challenge
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
