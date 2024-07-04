// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2021 by Lexi Mayfield.
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

#include "odamex.h"

#include "svc_map.h"

#include "client.pb.h"
#include "server.pb.h"

#include "hashtable.h"
#include "i_net.h"

using HeaderMap = OHashTable<int, const google::protobuf::Descriptor*>;
using SVCDescMap = OHashTable<const void*, svc_t>;
using CLCDescMap = OHashTable<const void*, clc_t>;

static HeaderMap g_SVCHeaderMap;
static SVCDescMap g_SVCDescMap;

static HeaderMap g_CLCHeaderMap;
static CLCDescMap g_CLCDescMap;

static void ServerMapProto(const svc_t header, const google::protobuf::Descriptor* desc)
{
	::g_SVCHeaderMap.insert(HeaderMap::value_type(header, desc));
	::g_SVCDescMap.insert(SVCDescMap::value_type(desc, header));
}

static void ClientMapProto(const clc_t header, const google::protobuf::Descriptor* desc)
{
	::g_CLCHeaderMap.insert(HeaderMap::value_type(header, desc));
	::g_CLCDescMap.insert(CLCDescMap::value_type(desc, header));
}

/**
 * @brief Initialize the SVC protocol descriptor map.
 */
static void InitMap()
{
	ServerMapProto(svc_disconnect, odaproto::svc::Disconnect::descriptor());
	ServerMapProto(svc_playerinfo, odaproto::svc::PlayerInfo::descriptor());
	ServerMapProto(svc_moveplayer, odaproto::svc::MovePlayer::descriptor());
	ServerMapProto(svc_updatelocalplayer, odaproto::svc::UpdateLocalPlayer::descriptor());
	ServerMapProto(svc_levellocals, odaproto::svc::LevelLocals::descriptor());
	ServerMapProto(svc_pingrequest, odaproto::svc::PingRequest::descriptor());
	ServerMapProto(svc_updateping, odaproto::svc::UpdatePing::descriptor());
	ServerMapProto(svc_spawnmobj, odaproto::svc::SpawnMobj::descriptor());
	ServerMapProto(svc_disconnectclient, odaproto::svc::DisconnectClient::descriptor());
	ServerMapProto(svc_loadmap, odaproto::svc::LoadMap::descriptor());
	ServerMapProto(svc_consoleplayer, odaproto::svc::ConsolePlayer::descriptor());
	ServerMapProto(svc_explodemissile, odaproto::svc::ExplodeMissile::descriptor());
	ServerMapProto(svc_removemobj, odaproto::svc::RemoveMobj::descriptor());
	ServerMapProto(svc_userinfo, odaproto::svc::UserInfo::descriptor());
	ServerMapProto(svc_updatemobj, odaproto::svc::UpdateMobj::descriptor());
	ServerMapProto(svc_spawnplayer, odaproto::svc::SpawnPlayer::descriptor());
	ServerMapProto(svc_damageplayer, odaproto::svc::DamagePlayer::descriptor());
	ServerMapProto(svc_killmobj, odaproto::svc::KillMobj::descriptor());
	ServerMapProto(svc_fireweapon, odaproto::svc::FireWeapon::descriptor());
	ServerMapProto(svc_updatesector, odaproto::svc::UpdateSector::descriptor());
	ServerMapProto(svc_print, odaproto::svc::Print::descriptor());
	ServerMapProto(svc_playermembers, odaproto::svc::PlayerMembers::descriptor());
	ServerMapProto(svc_teammembers, odaproto::svc::TeamMembers::descriptor());
	ServerMapProto(svc_activateline, odaproto::svc::ActivateLine::descriptor());
	ServerMapProto(svc_movingsector, odaproto::svc::MovingSector::descriptor());
	ServerMapProto(svc_playsound, odaproto::svc::PlaySound::descriptor());
	ServerMapProto(svc_reconnect, odaproto::svc::Reconnect::descriptor());
	ServerMapProto(svc_exitlevel, odaproto::svc::ExitLevel::descriptor());
	ServerMapProto(svc_touchspecial, odaproto::svc::TouchSpecial::descriptor());
	ServerMapProto(svc_forceteam, odaproto::svc::ForceTeam::descriptor());
	ServerMapProto(svc_switch, odaproto::svc::Switch::descriptor());
	ServerMapProto(svc_say, odaproto::svc::Say::descriptor());
	ServerMapProto(svc_ctfrefresh, odaproto::svc::CTFRefresh::descriptor());
	ServerMapProto(svc_ctfevent, odaproto::svc::CTFEvent::descriptor());
	ServerMapProto(svc_secretevent, odaproto::svc::SecretEvent::descriptor());
	ServerMapProto(svc_serversettings, odaproto::svc::ServerSettings::descriptor());
	ServerMapProto(svc_connectclient, odaproto::svc::ConnectClient::descriptor());
	ServerMapProto(svc_midprint, odaproto::svc::MidPrint::descriptor());
	ServerMapProto(svc_servergametic, odaproto::svc::ServerGametic::descriptor());
	ServerMapProto(svc_inttimeleft, odaproto::svc::IntTimeLeft::descriptor());
	ServerMapProto(svc_fullupdatedone, odaproto::svc::FullUpdateDone::descriptor());
	ServerMapProto(svc_railtrail, odaproto::svc::RailTrail::descriptor());
	ServerMapProto(svc_playerstate, odaproto::svc::PlayerState::descriptor());
	ServerMapProto(svc_levelstate, odaproto::svc::LevelState::descriptor());
	ServerMapProto(svc_resetmap, odaproto::svc::ResetMap::descriptor());
	ServerMapProto(svc_playerqueuepos, odaproto::svc::PlayerQueuePos::descriptor());
	ServerMapProto(svc_fullupdatestart, odaproto::svc::FullUpdateStart::descriptor());
	ServerMapProto(svc_lineupdate, odaproto::svc::LineUpdate::descriptor());
	ServerMapProto(svc_sectorproperties, odaproto::svc::SectorProperties::descriptor());
	ServerMapProto(svc_linesideupdate, odaproto::svc::LineUpdate::descriptor());
	ServerMapProto(svc_mobjstate, odaproto::svc::MobjState::descriptor());
	ServerMapProto(svc_damagemobj, odaproto::svc::DamageMobj::descriptor());
	ServerMapProto(svc_executelinespecial,
	               odaproto::svc::ExecuteLineSpecial::descriptor());
	ServerMapProto(svc_executeacsspecial, odaproto::svc::ExecuteACSSpecial::descriptor());
	ServerMapProto(svc_thinkerupdate, odaproto::svc::ThinkerUpdate::descriptor());
	ServerMapProto(svc_vote_update, odaproto::svc::VoteUpdate::descriptor());
	ServerMapProto(svc_maplist, odaproto::svc::Maplist::descriptor());
	ServerMapProto(svc_maplist_update, odaproto::svc::MaplistUpdate::descriptor());
	ServerMapProto(svc_maplist_index, odaproto::svc::MaplistIndex::descriptor());
	ServerMapProto(svc_toast, odaproto::svc::Toast::descriptor());
	ServerMapProto(svc_hordeinfo, odaproto::svc::HordeInfo::descriptor());
	ServerMapProto(svc_netdemocap, odaproto::svc::NetdemoCap::descriptor());
	ServerMapProto(svc_netdemostop, odaproto::svc::NetDemoStop::descriptor());
	ServerMapProto(svc_netdemoloadsnap, odaproto::svc::NetDemoLoadSnap::descriptor());

	ClientMapProto(clc_disconnect, odaproto::clc::Disconnect::descriptor());
	ClientMapProto(clc_say, odaproto::clc::Say::descriptor());
	ClientMapProto(clc_move, odaproto::clc::Move::descriptor());
	ClientMapProto(clc_userinfo, odaproto::clc::UserInfo::descriptor());
	ClientMapProto(clc_pingreply, odaproto::clc::PingReply::descriptor());
	ClientMapProto(clc_ack, odaproto::clc::Ack::descriptor());
	ClientMapProto(clc_rcon, odaproto::clc::RCon::descriptor());
	ClientMapProto(clc_rcon_password, odaproto::clc::RConPassword::descriptor());
	ClientMapProto(clc_spectate, odaproto::clc::Spectate::descriptor());
	ClientMapProto(clc_wantwad, odaproto::clc::WantWad::descriptor());
	ClientMapProto(clc_kill, odaproto::clc::Kill::descriptor());
	ClientMapProto(clc_cheat, odaproto::clc::Cheat::descriptor());
	ClientMapProto(clc_callvote, odaproto::clc::CallVote::descriptor());
	ClientMapProto(clc_maplist, odaproto::clc::MapList::descriptor());
	ClientMapProto(clc_maplist_update, odaproto::clc::MapListUpdate::descriptor());
	ClientMapProto(clc_getplayerinfo, odaproto::clc::GetPlayerInfo::descriptor());
	ClientMapProto(clc_netcmd, odaproto::clc::NetCmd::descriptor());
	ClientMapProto(clc_spy, odaproto::clc::Spy::descriptor());
	ClientMapProto(clc_privmsg, odaproto::clc::PrivMsg::descriptor());
}

//------------------------------------------------------------------------------

const google::protobuf::Descriptor* SVC_ResolveHeader(const byte header)
{
	if (::g_SVCHeaderMap.empty())
	{
		InitMap();
	}

	HeaderMap::iterator it = ::g_SVCHeaderMap.find(static_cast<svc_t>(header));
	if (it == ::g_SVCHeaderMap.end())
	{
		return NULL;
	}
	return static_cast<const google::protobuf::Descriptor*>(it->second);
}

//------------------------------------------------------------------------------

svc_t SVC_ResolveDescriptor(const google::protobuf::Descriptor* desc)
{
	if (::g_SVCDescMap.empty())
	{
		InitMap();
	}

	SVCDescMap::iterator it = ::g_SVCDescMap.find(desc);
	if (it == ::g_SVCDescMap.end())
	{
		return svc_invalid;
	}
	return it->second;
}

//------------------------------------------------------------------------------

const google::protobuf::Descriptor* CLC_ResolveHeader(const byte header)
{
	if (::g_CLCHeaderMap.empty())
	{
		InitMap();
	}

	HeaderMap::iterator it = ::g_CLCHeaderMap.find(static_cast<clc_t>(header));
	if (it == ::g_CLCHeaderMap.end())
	{
		return NULL;
	}
	return static_cast<const google::protobuf::Descriptor*>(it->second);
}

//------------------------------------------------------------------------------

clc_t CLC_ResolveDescriptor(const google::protobuf::Descriptor* desc)
{
	if (::g_CLCDescMap.empty())
	{
		InitMap();
	}

	CLCDescMap::iterator it = ::g_CLCDescMap.find(desc);
	if (it == ::g_CLCDescMap.end())
	{
		return clc_invalid;
	}
	return it->second;
}
