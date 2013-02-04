// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: d_netcmd.h 3174 2012-05-11 01:03:43Z dr_sean $
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 2006-2012 by The Odamex Team.
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

#ifndef __D_NETCMD__
#define __D_NETCMD__

#include "doomtype.h"
#include "i_net.h"
#include "m_fixed.h"
 
// Forward declaration avoids circular reference
class player_s;
typedef player_s player_t;

static const short CENTERVIEW = -32768;
//
// NetCommand
//
// A class that contains the input commands from a player and can
// serialize/deserialize to a buf_t for delivery over the network.
// NetCommand uses absolute angles for player_t::mo::angle and pitch instead
// of delta angles like usercmd_t::yaw and patch since too many dropped packets
// will cause desynchronization with delta angles.

class NetCommand
{
public:
	NetCommand();
	
	bool	hasButtons() const		{ return ((mFields & CMD_BUTTONS) != 0); }
	bool	hasAngle() const		{ return ((mFields & CMD_ANGLE) != 0); }
	bool	hasPitch() const		{ return ((mFields & CMD_PITCH) != 0); }
	bool	hasForwardMove() const	{ return ((mFields & CMD_FORWARD) != 0); }
	bool	hasSideMove() const		{ return ((mFields & CMD_SIDE) != 0); }
	bool	hasUpMove() const		{ return ((mFields & CMD_UP) != 0); }
	bool	hasImpulse() const		{ return ((mFields & CMD_IMPULSE) != 0); }
	bool	hasDeltaYaw() const		{ return ((mFields & CMD_DELTAYAW) != 0); }
	bool	hasDeltaPitch() const	{ return ((mFields & CMD_DELTAPITCH) != 0); }
	
	int		getTic() const			{ return mTic; }
	int		getWorldIndex() const	{ return mWorldIndex; }
	byte	getButtons() const		{ return mButtons; }
	fixed_t	getAngle() const 		{ return mAngle; }
	fixed_t	getPitch() const		{ return mPitch; }
	short	getForwardMove() const	{ return mForwardMove; }
	short	getSideMove() const		{ return mSideMove; }
	short	getUpMove() const		{ return mUpMove; }
	byte	getImpulse() const		{ return mImpulse; }
	short	getDeltaYaw() const		{ return mDeltaYaw; }
	short	getDeltaPitch() const	{ return mDeltaPitch; }
	
	void setTic(int val)
	{
		mTic = val;
	}
	
	void setWorldIndex(int val)
	{
		mWorldIndex = val;
	}
	
	void setButtons(byte val)
	{
		updateFields(CMD_BUTTONS, val);
		mButtons = val;
	}
	
	void setAngle(fixed_t val)
	{
		updateFields(CMD_ANGLE, val);
		mAngle = val;
	}
	
	void setPitch(fixed_t val)
	{
		updateFields(CMD_PITCH, val);
		mPitch = val;
	}
	
	void setForwardMove(short val)
	{
		updateFields(CMD_FORWARD, val);
		mForwardMove = val;
	}
	
	void setSideMove(short val)
	{
		updateFields(CMD_SIDE, val);
		mSideMove = val;
	}	
		
	void setUpMove(short val)
	{
		updateFields(CMD_UP, val);
		mUpMove = val;
	}
	
	void setImpulse(byte val)
	{
		updateFields(CMD_IMPULSE, val);
		mImpulse = val;
	}
	
	void setDeltaYaw(short val)
	{
		updateFields(CMD_DELTAYAW, val);
		mDeltaYaw = val;
	}
	
	void setDeltaPitch(short val)
	{
		updateFields(CMD_DELTAPITCH, val);
		mDeltaPitch = val;
	}
	
	void clear();
	void write(buf_t *buf);
	void read(buf_t *buf);
	
	void toPlayer(player_t *player) const;
	void fromPlayer(player_t *player);

private:
	static const int CMD_BUTTONS		= 0x0001;
	static const int CMD_ANGLE			= 0x0002;
	static const int CMD_PITCH			= 0x0004;
	static const int CMD_FORWARD		= 0x0008;
	static const int CMD_SIDE			= 0x0010;
	static const int CMD_UP				= 0x0020;
	static const int CMD_IMPULSE		= 0x0040;
	static const int CMD_DELTAYAW		= 0x0080;
	static const int CMD_DELTAPITCH		= 0x0100;

	int			mTic;
	int			mWorldIndex;
	int			mFields;
	byte		mButtons;
	fixed_t		mAngle;
	fixed_t		mPitch;
	short		mForwardMove;
	short		mSideMove;
	short		mUpMove;
	byte		mImpulse;
	short		mDeltaYaw;
	short		mDeltaPitch;

	int getSerializedFields();

	void updateFields(int flag, int value)
	{
		if (value == 0)
			mFields &= ~flag;
		else
			mFields |= flag;
	}
};

#endif	// __D_NETCMD__
