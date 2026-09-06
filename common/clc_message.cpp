// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2026 by Jim Thoenen.
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
//  CLC Message packers.
//
//-----------------------------------------------------------------------------

#include "clc_message.h"

#include "odamex.h"

#include "d_player.h"

static void FillColor(odaproto::Color& io_msg, const argb_t& color)
{
	io_msg.set_a(color.geta());
	io_msg.set_r(color.getr());
	io_msg.set_g(color.getg());
	io_msg.set_b(color.getb());
}

void CLC_PackPlayerInputMessageFromPlayer(odaproto::clc::PlayerInput& msg, player_t& player, int clientTic, int clientWorldIndex)
{
	if (player.mo)
	{
		msg.Clear();
		msg.set_tic(clientTic);
		msg.set_world_index(clientWorldIndex);

		// Special knowledge: On the client side, we save+send the command before we process it locally.
		//                    Once processed, we locally tic forward.  During that time, we may detect
		//                    conditions for which we set the Inventory Check Request, which means that
		//                    we want to send the request on the following tic.
		if (player.playerInfoIsRequested)
		{
			msg.set_inventory_check(true);
			player.playerInfoIsRequested = false;
		}

		if (player.cmd.buttons & BT_ATTACK)
		{
			msg.set_button_attack(true);
		}
		if (player.cmd.buttons & BT_USE)
		{
			msg.set_button_use(true);
		}
		if (player.cmd.buttons & BT_SPECIAL)
		{
			if (player.cmd.buttons & BTS_PAUSE)
			{
				msg.set_button_pause(true);
			}
			if (player.cmd.buttons & BTS_SAVEGAME)
			{
				msg.set_button_savegame((player.cmd.buttons & BTS_SAVEMASK) >> BTS_SAVESHIFT);
			}
		}
		if (player.cmd.buttons & BT_CHANGE)
		{
			msg.set_button_weaponchange((player.cmd.buttons & BT_WEAPONMASK) >> BT_WEAPONSHIFT);
		}
		if (player.cmd.buttons & BT_JUMP)
		{
			msg.set_button_jump(true);
		}

		if (player.cmd.impulse)
		{
			msg.set_impulse(player.cmd.impulse);
		}

		if (player.playerstate != PST_DEAD)
		{
			msg.set_angle(player.mo->angle);
			msg.set_pitch(player.mo->pitch);

			if (player.cmd.forwardmove)
			{
				msg.set_move_forward(player.cmd.forwardmove);
			}
			if (player.cmd.sidemove)
			{
				msg.set_move_side(player.cmd.sidemove);
			}
			if (player.cmd.upmove)
			{
				msg.set_move_up(player.cmd.upmove);
			}
			if (player.cmd.yaw)
			{
				msg.set_delta_yaw(player.cmd.yaw);
			}
			if (player.cmd.pitch)
			{
				msg.set_delta_pitch(player.cmd.pitch);
			}
		}
	}
}

void CLC_UnpackPlayerInputMessageToPlayer(const odaproto::clc::PlayerInput& msg, player_t& player)
{
	// We deliberately avoid unpacking the player tic here.  That should be done carefully at the
	// callers' discretion for their particular use cases.
	if (player.mo)
	{
		player.cmd.clear();

		if (msg.button_attack())
		{
			player.cmd.buttons |= BT_ATTACK;
		}
		if (msg.button_use())
		{
			player.cmd.buttons |= BT_USE;
		}
		if (msg.button_pause())
		{
			player.cmd.buttons |= (BT_SPECIAL | BTS_PAUSE);
		}
		if (msg.has_button_savegame())
		{
			player.cmd.buttons |= (BT_SPECIAL | BTS_SAVEGAME | ((msg.button_savegame() << BTS_SAVESHIFT) & BTS_SAVEMASK));
		}
		if (msg.has_button_weaponchange())
		{
			player.cmd.buttons |= (BT_CHANGE | ((msg.button_weaponchange() << BT_WEAPONSHIFT) & BT_WEAPONMASK));
		}
		if (msg.button_jump())
		{
			player.cmd.buttons |= BT_JUMP;
		}

		player.cmd.impulse = msg.impulse();

		if (player.playerstate != PST_DEAD)
		{
			player.cmd.forwardmove  = msg.move_forward();
			player.cmd.sidemove     = msg.move_side();
			player.cmd.upmove       = msg.move_up();
			player.cmd.yaw          = msg.delta_yaw();
			player.cmd.pitch        = msg.delta_pitch();

			player.mo->angle = msg.angle();
			player.mo->pitch = msg.pitch();
		}
	}
}

odaproto::clc::NetdemoCap CLC_NetdemoCap(const player_t&                   player,
                                         const odaproto::clc::PlayerInput& inputMessage,
                                         const OdaMessenger&               playerMessenger)
{
	odaproto::clc::NetdemoCap msg;

	inputMessage.SerializeToString(msg.mutable_packed_player_input());

	odaproto::Actor* act = msg.mutable_actor();

	act->set_waterlevel         (player.mo->waterlevel);
	act->mutable_pos()->set_x   (player.mo->x);
	act->mutable_pos()->set_y   (player.mo->y);
	act->mutable_pos()->set_z   (player.mo->z);
	act->mutable_mom()->set_x   (player.mo->momx);
	act->mutable_mom()->set_y   (player.mo->momy);
	act->mutable_mom()->set_z   (player.mo->momz);
	act->set_angle              (player.mo->angle);
	act->set_pitch              (player.mo->pitch);
	act->set_reactiontime       (player.mo->reactiontime);

	msg.set_viewz           (player.viewz);
	msg.set_viewheight      (player.viewheight);
	msg.set_deltaviewheight (player.deltaviewheight);
	msg.set_jumptics        (player.jumpTics);
	msg.set_readyweapon     (player.readyweapon);
	msg.set_pendingweapon   (player.pendingweapon);

	msg.set_awaiting_ack_count(playerMessenger.GetPendingAckCount());

	const auto& buffer { playerMessenger.GetRecordingBufferRef() };

	msg.set_packed_outgoing_msgs(buffer.data(), buffer.size());
	return msg;
}

odaproto::clc::DisconnectMe CLC_DisconnectMe()
{
	return odaproto::clc::DisconnectMe{};
}

odaproto::clc::Say CLC_Say(const std::string_view& text, uint32_t visibility)
{
	odaproto::clc::Say msg;

	msg.set_visibility  (visibility);
	msg.set_text        (text.data(), text.size());

	return msg;
}

odaproto::clc::UserInfo CLC_UserInfo(const UserInfo& userInfo)
{
	odaproto::clc::UserInfo msg;

	msg.set_netname (userInfo.netname);
	msg.set_team    (userInfo.team);
	msg.set_gender  (userInfo.gender);
	msg.set_colorpreset (userInfo.colorpreset);
	FillColor           (*msg.mutable_color(), userInfo.color);
	msg.set_aimdist     (userInfo.aimdist);
	msg.set_predict_weapons (userInfo.predict_weapons);
	msg.set_switchweapon    (userInfo.switchweapon);

	for (const auto& pref : userInfo.weapon_prefs)
	{
		msg.add_weapon_prefs(pref);
	}

	return msg;
}

odaproto::clc::PingReply CLC_PingReply(uint64_t msec)
{
	odaproto::clc::PingReply msg;

	msg.set_ms_time(msec);

	return msg;
}

odaproto::clc::Kill CLC_Kill()
{
	return {};
}

odaproto::clc::Rcon CLC_Rcon(const std::string_view& text)
{
	odaproto::clc::Rcon msg;

	msg.set_command(text.data(), text.size());

	return msg;
}

odaproto::clc::RconPassword CLC_RconPassword(const std::string_view& text)
{
	odaproto::clc::RconPassword msg;

	msg.set_challenge(text.data(), text.size());

	return msg;
}

odaproto::clc::RconLogout CLC_RconLogout()
{
	return odaproto::clc::RconLogout{};
}

odaproto::clc::SpectateBegin CLC_SpectateBegin()
{
	return odaproto::clc::SpectateBegin();
}

odaproto::clc::SpectateEnd CLC_SpectateEnd()
{
	return {};
}

odaproto::clc::SpectateUpdate CLC_SpectateUpdate(const player_t& player)
{
	odaproto::clc::SpectateUpdate msg;

	if (player.mo)
	{
		odaproto::Vec3* position = msg.mutable_pos();

		position->set_x(player.mo->x);
		position->set_y(player.mo->y);
		position->set_z(player.mo->z);
	}

	return msg;
}

odaproto::clc::Cheat CLC_Cheat(uint32_t cheatValue)
{
	odaproto::clc::Cheat msg;

	msg.set_value(cheatValue);

	return msg;
}

odaproto::clc::CheatGive CLC_CheatGive(const std::string& item)
{
	odaproto::clc::CheatGive msg;

	msg.set_item(item);

	return msg;
}

odaproto::clc::CheatSummon CLC_CheatSummon(const std::string& monster)
{
	odaproto::clc::CheatSummon msg;

	msg.set_monster(monster);

	return msg;
}

odaproto::clc::CheatSummonFriend CLC_CheatSummonFriend(const std::string& monster)
{
	odaproto::clc::CheatSummonFriend msg;

	msg.set_monster(monster);

	return msg;
}

odaproto::clc::CallVote CLC_CallVote(uint32_t voteType)
{
	odaproto::clc::CallVote msg;

	msg.set_vote_type(voteType);

	return msg;
}

odaproto::clc::CallVote CLC_CallVote(uint32_t voteType, const std::string& arg)
{
	odaproto::clc::CallVote msg;

	msg.set_vote_type(voteType);
	msg.add_arg(arg);

	return msg;
}

odaproto::clc::CallVote CLC_CallVote(uint32_t voteType, const std::vector<std::string>& args)
{
	odaproto::clc::CallVote msg;

	msg.set_vote_type(voteType);
	for (const auto& arg : args)
	{
		msg.add_arg(arg);
	}

	return msg;
}

odaproto::clc::Maplist CLC_Maplist(uint32_t status)
{
	odaproto::clc::Maplist msg;

	msg.set_status(status);

	return msg;
}

odaproto::clc::MaplistUpdate CLC_MaplistUpdate()
{
	return {};
}

odaproto::clc::GetPlayerInfo CLC_GetPlayerInfo()
{
	return {};
}

odaproto::clc::Netcmd CLC_Netcmd(const std::string& arg)
{
	odaproto::clc::Netcmd msg;
	msg.add_argv(arg);
	return msg;
}

odaproto::clc::Spy CLC_Spy(uint32_t playerId)
{
	odaproto::clc::Spy msg;
	msg.set_player_id(playerId);
	return msg;
}

odaproto::clc::PrivMsg CLC_PrivMsg(uint32_t playerId, const std::string_view& text)
{
	odaproto::clc::PrivMsg msg;

	msg.set_player_id   (playerId);
	msg.set_text        (text.data(), text.size());

	return msg;
}

odaproto::clc::SendMobjUpdate CLC_SendMobjUpdate(uint32_t netId)
{
	odaproto::clc::SendMobjUpdate msg;
	msg.set_netid(netId);
	return msg;
}
