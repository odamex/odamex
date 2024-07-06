// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2024 by Alex Mayfield.
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
//   Client message functions.
//   - Functions should send exactly one message.
//   - Functions should be named after the message they send.
//   - Functions should be self-contained and not rely on global state.
//   - Functions should take a buf_t& as a first parameter.
//
//-----------------------------------------------------------------------------

#include "odamex.h"

#include "clc_message.h"

#include "md5.h"

//------------------------------------------------------------------------------

odaproto::clc::Say CLC_Say(byte who, const std::string& message)
{
	odaproto::clc::Say msg;

	msg.set_visibility(who);
	msg.set_message(message);

	return msg;
}

odaproto::clc::Move CLC_Move(int tic, NetCommand* cmds, size_t cmds_len)
{
	odaproto::clc::Move msg;

	buf_t buffer{MAX_UDP_SIZE};

	// Write current client-tic.  Server later sends this back to client
	// when sending svc_updatelocalplayer so the client knows which ticcmds
	// need to be used for client's positional prediction.
	msg.set_tic(tic);

	for (int i = 9; i >= 0; i--)
	{
		NetCommand blank_netcmd;
		NetCommand* netcmd;

		if (tic >= i)
			netcmd = &cmds[(tic - i) % cmds_len];
		else
			netcmd = &blank_netcmd; // write a blank netcmd since not enough gametics have
			                        // passed

		netcmd->write(&buffer);
	}

	msg.set_cmds(buffer.data, buffer.cursize);

	return msg;
}

odaproto::clc::ClientUserInfo CLC_UserInfo(const UserInfo& info)
{
	odaproto::clc::ClientUserInfo msg;

	odaproto::UserInfo* msgInfo = msg.mutable_userinfo();
	msgInfo->set_netname(info.netname);
	msgInfo->set_team(info.team);
	msgInfo->set_gender(info.gender);

	// [LM] Alpha is always 255.
	odaproto::Color* color = msgInfo->mutable_color();
	color->set_r(info.color[1]);
	color->set_g(info.color[2]);
	color->set_b(info.color[3]);

	msgInfo->set_aimdist(info.aimdist);
	msgInfo->set_predict_weapons(info.predict_weapons);
	msgInfo->set_switchweapon(info.switchweapon);

	for (size_t i = 0; i < NUMWEAPONS; i++)
	{
		msgInfo->add_weapon_prefs(info.weapon_prefs[i]);
	}

	return msg;
}

odaproto::clc::PingReply CLC_PingReply(uint64_t ms_time)
{
	odaproto::clc::PingReply msg;

	msg.set_ms_time(ms_time);

	return msg;
}

odaproto::clc::Ack CLC_Ack(uint32_t recent, uint32_t ackBits)
{
	odaproto::clc::Ack msg;

	msg.set_recent(recent);
	msg.set_ack_bits(ackBits);

	return msg;
}

odaproto::clc::RCon CLC_RCon(const std::string& command)
{
	odaproto::clc::RCon msg;

	msg.set_command(command);

	return msg;
}

odaproto::clc::RConPassword CLC_RConPasswordLogin(const std::string& password,
                                                  const std::string& hash)
{
	odaproto::clc::RConPassword msg;

	msg.set_is_login(true);
	msg.set_passhash(MD5SUM(password + hash));

	return msg;
}

odaproto::clc::RConPassword CLC_RConPasswordLogout()
{
	odaproto::clc::RConPassword msg;

	msg.set_is_login(false);

	return msg;
}

odaproto::clc::Spectate CLC_Spectate(bool enabled)
{
	odaproto::clc::Spectate msg;

	msg.set_command(enabled);

	return msg;
}

odaproto::clc::Spectate CLC_SpectateUpdate(const AActor* mobj)
{
	odaproto::clc::Spectate msg;

	msg.set_command(5);
	msg.mutable_pos()->set_x(mobj->x);
	msg.mutable_pos()->set_y(mobj->y);
	msg.mutable_pos()->set_z(mobj->z);

	return msg;
}

odaproto::clc::Cheat CLC_CheatNumber(int number)
{
	odaproto::clc::Cheat msg;

	msg.set_cheat_number(number);

	return msg;
}

odaproto::clc::Cheat CLC_CheatGiveTo(const std::string& item)
{
	odaproto::clc::Cheat msg;

	msg.set_give_item(item);

	return msg;
}

odaproto::clc::MapList CLC_MapList(maplist_status_t status)
{
	odaproto::clc::MapList msg;

	msg.set_status(int32_t(status));

	return msg;
}

odaproto::clc::CallVote CLC_CallVote(vote_type_t vote_type, const char* const* args,
                                     size_t len)
{
	odaproto::clc::CallVote msg;

	msg.set_vote_type(int32_t(vote_type));

	msg.mutable_args()->Reserve(len);
	for (size_t i = 0; i < len; i++)
	{
		msg.mutable_args()->Add(args[i]);
	}

	return msg;
}

odaproto::clc::NetCmd CLC_NetCmd(const char* const* args, size_t len)
{
	odaproto::clc::NetCmd msg;

	msg.mutable_args()->Reserve(len);
	for (size_t i = 0; i < len; i++)
	{
		msg.mutable_args()->Add(args[i]);
	}

	return msg;
}

odaproto::clc::Spy CLC_Spy(byte pid)
{
	odaproto::clc::Spy msg;

	msg.set_pid(pid);

	return msg;
}

odaproto::clc::PrivMsg CLC_PrivMsg(byte pid, const std::string& str)
{
	odaproto::clc::PrivMsg msg;

	msg.set_pid(pid);
	msg.set_message(str);

	return msg;
}
