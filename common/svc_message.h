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

#ifndef __SVCMESSAGE_H__
#define __SVCMESSAGE_H__

#include <string>
#include <vector>

#include "server.pb.h"

#include "r_state.h"

#include "actor.h"
#include "c_maplist.h"
#include "c_vote.h"
#include "com_misc.h"
#include "d_player.h"
#include "g_level.h"
#include "g_levelstate.h"
#include "i_net.h"
#include "p_ctf.h"
#include "p_spec.h"

/**
 * @brief Call the constructor according to the PlaySound invocation you
 *        want to send.
 */
struct PlaySoundType
{
	enum tag_e
	{
		PS_NONE,
		PS_MOBJ,
		PS_POS
	};
	tag_e tag;

	union {
		AActor* mo;
		struct
		{
			fixed_t x, y;
		} pos;
	} data;

	PlaySoundType() : tag(PS_NONE)
	{
		data.mo = NULL;
	}
	PlaySoundType(AActor* mo) : tag(PS_MOBJ)
	{
		data.mo = mo;
	}
	PlaySoundType(fixed_t x, fixed_t y) : tag(PS_POS)
	{
		data.pos.x = x;
		data.pos.y = y;
	}
};

odaproto::svc::Disconnect SVC_Disconnect(const char* message = NULL);
odaproto::svc::PlayerInfo SVC_PlayerInfo(player_t& player);
odaproto::svc::MovePlayer SVC_MovePlayer(player_t& player, const int tic);
odaproto::svc::UpdateLocalPlayer SVC_UpdateLocalPlayer(AActor& mo, const int tic);
odaproto::svc::LevelLocals SVC_LevelLocals(const level_locals_t& locals, uint32_t flags);
odaproto::svc::PingRequest SVC_PingRequest();
odaproto::svc::UpdatePing SVC_UpdatePing(player_t& player);
odaproto::svc::SpawnMobj SVC_SpawnMobj(AActor* mo);
odaproto::svc::DisconnectClient SVC_DisconnectClient(player_t& player);
odaproto::svc::LoadMap SVC_LoadMap(const OResFiles& wadnames, const OResFiles& patchnames,
                                   const std::string& mapname, int time);
odaproto::svc::ConsolePlayer SVC_ConsolePlayer(player_t& player,
                                               const std::string& digest);
odaproto::svc::ExplodeMissile SVC_ExplodeMissile(AActor& mobj);
odaproto::svc::RemoveMobj SVC_RemoveMobj(AActor& mobj);
odaproto::svc::UserInfo SVC_UserInfo(player_t& player, int64_t time);
odaproto::svc::UpdateMobj SVC_UpdateMobj(AActor& mobj, uint32_t flags);
odaproto::svc::SpawnPlayer SVC_SpawnPlayer(player_t& player);
odaproto::svc::DamagePlayer SVC_DamagePlayer(player_t& player, AActor *inflictor, int health, int armor);
odaproto::svc::KillMobj SVC_KillMobj(AActor* source, AActor* target, AActor* inflictor,
                                     int mod, bool joinkill);
odaproto::svc::FireWeapon SVC_FireWeapon(player_t& player);
odaproto::svc::UpdateSector SVC_UpdateSector(sector_t& sector);
odaproto::svc::Print SVC_Print(printlevel_t level, const std::string& str);
odaproto::svc::PlayerMembers SVC_PlayerMembers(player_t& player, byte flags);
odaproto::svc::TeamMembers SVC_TeamMembers(team_t team);
odaproto::svc::ActivateLine SVC_ActivateLine(line_t* line, AActor* mo, int side,
                                             LineActivationType type);
odaproto::svc::MovingSector SVC_MovingSector(const sector_t& sector);
odaproto::svc::PlaySound SVC_PlaySound(const PlaySoundType& type, int channel, int sfx_id,
                                       float volume, int attenuation);
odaproto::svc::TouchSpecial SVC_TouchSpecial(AActor* mo);
odaproto::svc::PlayerState SVC_PlayerState(player_t& player);
odaproto::svc::LevelState SVC_LevelState(const SerializedLevelState& sls);
odaproto::svc::PlayerQueuePos SVC_PlayerQueuePos(const player_t& source);
odaproto::svc::ForceTeam SVC_ForceTeam(team_t team);
odaproto::svc::Switch SVC_Switch(line_t& line, uint32_t state, uint32_t timer);
odaproto::svc::Say SVC_Say(const bool visibility, const byte pid,
                           const std::string& message);
odaproto::svc::CTFRefresh SVC_CTFRefresh(const TeamsView& teams, const bool full);
odaproto::svc::CTFEvent SVC_CTFEvent(const flag_score_t event, const team_t target,
                                     const player_t& player);
odaproto::svc::SecretEvent SVC_SecretEvent(player_t& player, sector_t& sector);
odaproto::svc::ServerSettings SVC_ServerSettings(const cvar_t& var);
odaproto::svc::ConnectClient SVC_ConnectClient(const player_t& player);
odaproto::svc::MidPrint SVC_MidPrint(const std::string& message, const int time);
odaproto::svc::ServerGametic SVC_ServerGametic(const byte tic);
odaproto::svc::IntTimeLeft SVC_IntTimeLeft(const unsigned int timeleft);
odaproto::svc::RailTrail SVC_RailTrail(const v3double_t& start, const v3double_t& end);
odaproto::svc::LineUpdate SVC_LineUpdate(const line_t& line);
odaproto::svc::SectorProperties SVC_SectorProperties(sector_t& sector);
odaproto::svc::LineSideUpdate SVC_LineSideUpdate(const line_t& line, const int sideNum);
odaproto::svc::MobjState SVC_MobjState(AActor* mo);
odaproto::svc::DamageMobj SVC_DamageMobj(AActor* target, const int pain);
odaproto::svc::ExecuteLineSpecial SVC_ExecuteLineSpecial(byte special, line_t* line,
                                                         AActor* mo,
                                                         const int (&args)[5]);
odaproto::svc::ExecuteACSSpecial SVC_ExecuteACSSpecial(const byte special,
                                                       const AActor* activator,
                                                       const char* print,
                                                       const std::vector<int>& args);
odaproto::svc::ThinkerUpdate SVC_ThinkerUpdate(DThinker* thinker);
odaproto::svc::VoteUpdate SVC_VoteUpdate(const vote_state_t& state);
odaproto::svc::Maplist SVC_Maplist(const maplist_status_t status);
odaproto::svc::MaplistUpdate SVC_MaplistUpdate(const maplist_status_t status,
                                               const maplist_qrows_t* maplist);
odaproto::svc::MaplistIndex SVC_MaplistIndex(const byte count, const size_t this_index,
                                             const size_t next_index);
odaproto::svc::Toast SVC_Toast(const toast_t& toast);
odaproto::svc::NetdemoCap SVC_NetdemoCap(player_t* player);

#endif // __SVCMESSAGE_H__
