// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2021, 2026 by Alex Mayfield and et al.
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
//   Message map.
//
//-----------------------------------------------------------------------------


#include "odamex.h"

#include "msg_map.h"

#include <unordered_map>

#include "client.pb.h"
#include "server.pb.h"

#include "i_net.h"

namespace
{
	struct IdentityKey
	{
		size_t operator()(const msg_t& key) const { return static_cast<size_t>(key); }
		size_t operator()(const void*  key) const { return reinterpret_cast<size_t>(key); }
	};
}

typedef std::unordered_map<msg_t, const google::protobuf::Descriptor*, IdentityKey> MSGHeaderMap;
typedef std::unordered_map<const void*, msg_t, IdentityKey> MSGDescMap;

MSGHeaderMap g_MSGHeaderMap;
MSGDescMap g_MSGDescMap;

static void MapProto(const msg_t header, const google::protobuf::Descriptor* desc)
{
	::g_MSGHeaderMap.emplace(header, desc);
	::g_MSGDescMap.emplace(desc, header);
}

/**
 * @brief Initialize the protocol descriptor map.
 */
static void InitMap()
{
	MapProto(msg_noop,   odaproto::Noop::descriptor());
	MapProto(msg_header, odaproto::Header::descriptor());

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
	MapProto(svc_updatemobjwithmode, odaproto::svc::UpdateMobjWithMode::descriptor());
	MapProto(svc_spawnplayer, odaproto::svc::SpawnPlayer::descriptor());
	MapProto(svc_damageplayer, odaproto::svc::DamagePlayer::descriptor());
	MapProto(svc_killmobj, odaproto::svc::KillMobj::descriptor());
	MapProto(svc_raisemobj, odaproto::svc::RaiseMobj::descriptor());
	MapProto(svc_updatesector, odaproto::svc::UpdateSector::descriptor());
	MapProto(svc_print, odaproto::svc::Print::descriptor());
	MapProto(svc_playermembers, odaproto::svc::PlayerMembers::descriptor());
	MapProto(svc_teammembers, odaproto::svc::TeamMembers::descriptor());
	MapProto(svc_activateline, odaproto::svc::ActivateLine::descriptor());
	MapProto(svc_movingsectorelevator, odaproto::svc::MovingSectorElevator::descriptor());
	MapProto(svc_movingsectorpillar, odaproto::svc::MovingSectorPillar::descriptor());
	MapProto(svc_movingsectorceiling, odaproto::svc::MovingSectorCeiling::descriptor());
	MapProto(svc_movingsectordoor, odaproto::svc::MovingSectorDoor::descriptor());
	MapProto(svc_movingsectorfloor, odaproto::svc::MovingSectorFloor::descriptor());
	MapProto(svc_movingsectorplat, odaproto::svc::MovingSectorPlat::descriptor());
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
	MapProto(svc_toast, odaproto::svc::Toast::descriptor());
	MapProto(svc_hordeinfo, odaproto::svc::HordeInfo::descriptor());
	MapProto(svc_raisemobj, odaproto::svc::RaiseMobj::descriptor());
	MapProto(svc_spree, odaproto::svc::Spree::descriptor());
	MapProto(svc_spreebreaker, odaproto::svc::SpreeBreaker::descriptor());
	MapProto(svc_noisealert, odaproto::svc::NoiseAlert::descriptor());

	MapProto(svc_playerammo,            odaproto::svc::PlayerAmmo::descriptor());
	MapProto(svc_playermaxammo,         odaproto::svc::PlayerMaxAmmo::descriptor());
	MapProto(svc_playerweaponowned,     odaproto::svc::PlayerWeaponOwned::descriptor());
	MapProto(svc_playerweaponselection, odaproto::svc::PlayerWeaponSelection::descriptor());
	MapProto(svc_playerpowers,          odaproto::svc::PlayerPowers::descriptor());
	MapProto(svc_playerpsprites,        odaproto::svc::PlayerPsprites::descriptor());

	MapProto(svc_configureavatar, odaproto::svc::ConfigureAvatar::descriptor());
	MapProto(svc_wakeupmobj,      odaproto::svc::WakeupMobj::descriptor());

	MapProto(clc_playerinput,    odaproto::clc::PlayerInput::descriptor());
	MapProto(clc_disconnectme,   odaproto::clc::DisconnectMe::descriptor());
	MapProto(clc_say,            odaproto::clc::Say::descriptor());
	MapProto(clc_userinfo,       odaproto::clc::UserInfo::descriptor());
	MapProto(clc_pingreply,      odaproto::clc::PingReply::descriptor());
	MapProto(clc_kill,           odaproto::clc::Kill::descriptor());
	MapProto(clc_callvote,       odaproto::clc::CallVote::descriptor());
	MapProto(clc_getplayerinfo,  odaproto::clc::GetPlayerInfo::descriptor());
	MapProto(clc_netcmd,         odaproto::clc::Netcmd::descriptor());
	MapProto(clc_spy,            odaproto::clc::Spy::descriptor());
	MapProto(clc_privmsg,        odaproto::clc::PrivMsg::descriptor());
	MapProto(clc_sendmobjupdate, odaproto::clc::SendMobjUpdate::descriptor());

	MapProto(clc_rcon,          odaproto::clc::Rcon::descriptor());
	MapProto(clc_rcon_password, odaproto::clc::RconPassword::descriptor());
	MapProto(clc_rcon_logout,   odaproto::clc::RconLogout::descriptor());

	MapProto(clc_spectate_begin,    odaproto::clc::SpectateBegin::descriptor());
	MapProto(clc_spectate_update,   odaproto::clc::SpectateUpdate::descriptor());
	MapProto(clc_spectate_end,      odaproto::clc::SpectateEnd::descriptor());

	MapProto(clc_netdemocap,        odaproto::clc::NetdemoCap::descriptor());
	MapProto(clc_netdemostop,       odaproto::clc::NetDemoStop::descriptor());
	MapProto(clc_netdemoloadsnap,   odaproto::clc::NetDemoLoadSnap::descriptor());

	MapProto(clc_cheat,                 odaproto::clc::Cheat::descriptor());
	MapProto(clc_cheat_give,            odaproto::clc::CheatGive::descriptor());
	MapProto(clc_cheat_summon,          odaproto::clc::CheatSummon::descriptor());
	MapProto(clc_cheat_summon_friend,   odaproto::clc::CheatSummonFriend::descriptor());

	MapProto(clc_maplist,           odaproto::clc::Maplist::descriptor());
	MapProto(clc_maplist_update,    odaproto::clc::MaplistUpdate::descriptor());
}

/**
 * @brief Given a packet header, return the message Descriptor, or NULL if
 *        the header is invalid.
 */
const google::protobuf::Descriptor* MSG_ResolveHeader(const msg_t header)
{
	if (::g_MSGHeaderMap.empty())
	{
		InitMap();
	}

	MSGHeaderMap::iterator it = ::g_MSGHeaderMap.find(header);
	if (it == ::g_MSGHeaderMap.end())
	{
		return NULL;
	}
	return static_cast<const google::protobuf::Descriptor*>(it->second);
}

/**
 * @brief Given a message Descriptor, return the packet header, or msg_noop
 *        if the descriptor is invalid.
 */
msg_t MSG_ResolveDescriptor(const google::protobuf::Descriptor* desc)
{
	if (::g_MSGDescMap.empty())
	{
		InitMap();
	}

	MSGDescMap::iterator it = ::g_MSGDescMap.find(desc);
	if (it == ::g_MSGDescMap.end())
	{
		return msg_noop;
	}
	return it->second;
}
