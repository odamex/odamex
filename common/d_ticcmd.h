// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
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
//	System specific interface stuff.
//
//-----------------------------------------------------------------------------

#ifndef __D_TICCMD_H__
#define __D_TICCMD_H__

#include "doomtype.h"
#include "farchive.h"

// The data sampled per tick (single player)
// and transmitted to other peers (multiplayer).
// Mainly movements/button commands per game tick,
// plus a checksum for internal state consistency.
struct ticcmd_t
{
	ticcmd_t()
	{
		clear();
	}

	void clear()
	{
		buttons = 0;
		pitch = 0;
		yaw = 0;
		forwardmove = 0;
		sidemove = 0;
		upmove = 0;
		impulse = 0;
	}

	int		tic;	// the client's tic when this cmd was sent

	byte	buttons;
	short	pitch;			// up/down. currently just a y-sheering amount
	short	yaw;			// left/right
	short	forwardmove;
	short	sidemove;
	short	upmove;
	byte	impulse;
};


#define UCMDF_BUTTONS		0x01
#define UCMDF_PITCH			0x02
#define UCMDF_YAW			0x04
#define UCMDF_FORWARDMOVE	0x08
#define UCMDF_SIDEMOVE		0x10
#define UCMDF_UPMOVE		0x20
#define UCMDF_IMPULSE		0x40

inline FArchive &operator<< (FArchive &arc, ticcmd_t &cmd)
{
	byte buf[256], *ptr = buf;

	byte flags = 0;

	if (cmd.buttons) {
		flags |= UCMDF_BUTTONS;
		*ptr++ = cmd.buttons;
	}
	if (cmd.pitch) {
		flags |= UCMDF_PITCH;
		*ptr++ = cmd.pitch >> 8;
		*ptr++ = cmd.pitch & 0xFF;
	}
	if (cmd.yaw) {
		flags |= UCMDF_YAW;
		*ptr++ = cmd.yaw >> 8;
		*ptr++ = cmd.yaw & 0xFF;
	}
	if (cmd.forwardmove) {
		flags |= UCMDF_FORWARDMOVE;
		*ptr++ = cmd.forwardmove >> 8;
		*ptr++ = cmd.forwardmove & 0xFF;
	}
	if (cmd.sidemove) {
		flags |= UCMDF_SIDEMOVE;
		*ptr++ = cmd.sidemove >> 8;
		*ptr++ = cmd.sidemove & 0xFF;
	}
	if (cmd.upmove) {
		flags |= UCMDF_UPMOVE;
		*ptr++ = cmd.upmove >> 8;
		*ptr++ = cmd.upmove & 0xFF;
	}
	if (cmd.impulse) {
		flags |= UCMDF_IMPULSE;
		*ptr++ = cmd.impulse;
	}

	byte len = ptr - buf;
	arc << (byte)(len + 1) << flags;
	arc.Write(buf, len);

	return arc;
}

inline FArchive &operator>> (FArchive &arc, ticcmd_t &cmd)
{
	byte buf[256], *ptr = buf;

	byte len, flags;
	arc >> len >> flags;
	arc.Read(buf, len - 1);

	// make sure the ucmd is empty
	cmd.clear();

	if (flags & UCMDF_BUTTONS) {
		cmd.buttons = *ptr++;
	}
	if (flags & UCMDF_PITCH) {
		cmd.pitch = *ptr++ << 8;
		cmd.pitch |= *ptr++;
	}
	if (flags & UCMDF_YAW) {
		cmd.pitch = *ptr++ << 8;
		cmd.pitch |= *ptr++;
	}
	if (flags & UCMDF_FORWARDMOVE) {
		cmd.forwardmove = *ptr++ << 8;
		cmd.forwardmove |= *ptr++;
	}
	if (flags & UCMDF_SIDEMOVE) {
		cmd.sidemove = *ptr++ << 8;
		cmd.sidemove |= *ptr++;
	}
	if (flags & UCMDF_UPMOVE) {
		cmd.upmove = *ptr++ << 8;
		cmd.upmove |= *ptr++;
	}
	if (flags & UCMDF_IMPULSE) {
		cmd.impulse = *ptr++;
	}

	return arc;
}

#endif	// __D_TICCMD_H__


