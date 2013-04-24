// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: d_netcmd.cpp 3174 2012-05-11 01:03:43Z dr_sean $
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 2006-2013 by The Odamex Team.
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
//	Store and serialize input commands between client and server
//
//-----------------------------------------------------------------------------

#include "d_netcmd.h"
#include "d_player.h"

NetCommand::NetCommand()
{
	clear();
}

void NetCommand::clear()
{
	mFields = mTic = mWorldIndex = 0;
	mButtons = mAngle = mPitch = mForwardMove = mSideMove = mUpMove = mImpulse = 0;
	mDeltaYaw = mDeltaPitch = 0;
}

void NetCommand::fromPlayer(player_t *player)
{
	if (!player || !player->mo)
		return;

	clear();
	setTic(player->cmd.tic);
	
	usercmd_t *ucmd = &player->cmd.ucmd;
	setButtons(ucmd->buttons);
	setImpulse(ucmd->impulse);
	
	if (player->playerstate != PST_DEAD)
	{
		setAngle(player->mo->angle);
		setPitch(player->mo->pitch);
		setForwardMove(ucmd->forwardmove);
		setSideMove(ucmd->sidemove);
		setUpMove(ucmd->upmove);
		setDeltaYaw(ucmd->yaw);
		setDeltaPitch(ucmd->pitch);
	}
}

void NetCommand::toPlayer(player_t *player) const
{
	if (!player || !player->mo)
		return;

	memset(&player->cmd, 0, sizeof(ticcmd_t));
	player->cmd.tic = getTic();
	
	usercmd_t *ucmd = &player->cmd.ucmd;
	ucmd->buttons = getButtons();
	ucmd->impulse = getImpulse();
	
	if (player->playerstate != PST_DEAD)
	{
		ucmd->forwardmove = getForwardMove();
		ucmd->sidemove = getSideMove();
		ucmd->upmove = getUpMove();
		ucmd->yaw = getDeltaYaw();
		ucmd->pitch = getDeltaPitch();
		
		player->mo->angle = getAngle();
		player->mo->pitch = getPitch();
	}
}

void NetCommand::write(buf_t *buf)
{
	// Let the recipient know which cmd fields are being sent
	int serialized_fields = getSerializedFields();
	buf->WriteByte(serialized_fields);
	buf->WriteLong(mWorldIndex);
		
	if (serialized_fields & CMD_BUTTONS)
		buf->WriteByte(mButtons);
	if (serialized_fields & CMD_ANGLE)
		buf->WriteShort((mAngle >> FRACBITS) + mDeltaYaw);
	if (serialized_fields & CMD_PITCH)
	{
		// ZDoom uses a hack to center the view when toggling cl_mouselook
		bool centerview = (mDeltaPitch == CENTERVIEW);
		if (centerview)
			buf->WriteShort(0);
		else
			buf->WriteShort((mPitch >> FRACBITS) + mDeltaPitch);
	}
	if (serialized_fields & CMD_FORWARD)
		buf->WriteShort(mForwardMove);
	if (serialized_fields & CMD_SIDE)
		buf->WriteShort(mSideMove);
	if (serialized_fields & CMD_UP)
		buf->WriteShort(mUpMove);
	if (serialized_fields & CMD_IMPULSE)
		buf->WriteByte(mImpulse);
}

void NetCommand::read(buf_t *buf)
{
	clear();
	mFields = buf->ReadByte();
	mWorldIndex = buf->ReadLong();
	
	if (hasButtons())
		mButtons = buf->ReadByte();
	if (hasAngle())
		mAngle = buf->ReadShort() << FRACBITS;
	if (hasPitch())
		mPitch = buf->ReadShort() << FRACBITS;
	if (hasForwardMove())
		mForwardMove = buf->ReadShort();
	if (hasSideMove())
		mSideMove = buf->ReadShort();
	if (hasUpMove())
		mUpMove = buf->ReadShort();
	if (hasImpulse())
		mImpulse = buf->ReadByte();
}


int NetCommand::getSerializedFields()
{
	int serialized_fields = 0;

	if (hasButtons())
		serialized_fields |= CMD_BUTTONS;
	if (hasAngle() || hasDeltaYaw())
		serialized_fields |= CMD_ANGLE;
	if (hasPitch() || hasDeltaPitch())
		serialized_fields |= CMD_PITCH;
	if (hasForwardMove())
		serialized_fields |= CMD_FORWARD;
	if (hasSideMove())
		serialized_fields |= CMD_SIDE;
	if (hasUpMove())
		serialized_fields |= CMD_UP;
	if (hasImpulse())
		serialized_fields |= CMD_IMPULSE;

	return serialized_fields;
}

VERSION_CONTROL (d_netcmd_cpp, "$Id: d_netcmd.cpp 3174 2012-05-11 01:03:43Z dr_sean $")

