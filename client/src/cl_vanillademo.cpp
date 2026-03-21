//-----------------------------------------------------------------------------
//
// Copyright (C) 2006-2026 by The Odamex Team.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
//-----------------------------------------------------------------------------

#include "odamex.h"

#include "cl_vanillademo.h"
#include "g_game.h"

bool G_CheckDemoStatus(void);

extern bool longtics;

enum demoversion_t
{
	LMP_DOOM_1_9,
	LMP_DOOM_1_9_1,
	LMP_HERETIC_1_3,
	LMP_HERETIC_1_3_LONGTICS,
};

static demoversion_t demoversion = LMP_DOOM_1_9;

#define DOOM_1_4_DEMO		0x68
#define DOOM_1_5_DEMO		0x69
#define DOOM_1_6_DEMO		0x6A
#define DOOM_1_7_DEMO		0x6B
#define DOOM_1_8_DEMO		0x6C
#define DOOM_1_9_DEMO		0x6D
#define DOOM_1_9p_DEMO		0x6E
#define DOOM_1_9_1_DEMO		0x6F

#define HERETIC_DEMOHEADER_RESPAWN		0x20
#define HERETIC_DEMOHEADER_LONGTICS		0x10
#define HERETIC_DEMOHEADER_NOMONSTERS	0x02

#define DEMOMARKER				0x80
#define DEMOSTOP				0x07

using demo_header_reader_t = bool (*)(demo_header_t&, byte*&, byte*);
using demo_ticcmd_reader_t = void (*)(byte*&, byte*);

struct demo_codec_t
{
	demoformat_t format;
	const char* name;
	demo_header_reader_t tryReadHeader;
	demo_ticcmd_reader_t readTiccmd;
};

static void G_ReadDoomDemoTiccmd(byte*& demo_p, byte* demo_e);
static void G_ReadHereticDemoTiccmd(byte*& demo_p, byte* demo_e);
static bool G_TryReadDoomDemoHeader(demo_header_t& header, byte*& demo_p, byte* demo_e);
static bool G_TryReadHereticDemoHeader(demo_header_t& header, byte*& demo_p, byte* demo_e);

static const demo_codec_t g_demoCodecs[] =
{
	{ DEMOFORMAT_DOOM_VANILLA, "DOOM", G_TryReadDoomDemoHeader, G_ReadDoomDemoTiccmd },
	{ DEMOFORMAT_HERETIC_VANILLA, "HERETIC", G_TryReadHereticDemoHeader, G_ReadHereticDemoTiccmd },
};

static const demo_codec_t* current_demo_codec = nullptr;

static bool G_DemoHasBytesLeft(const byte* demo_p, const byte* demo_e, const size_t count)
{
	return static_cast<size_t>(demo_e - demo_p) >= count;
}

static bool G_IsDoomVanillaDemoVersion(const byte version)
{
	switch (version)
	{
	case DOOM_1_4_DEMO:
	case DOOM_1_5_DEMO:
	case DOOM_1_6_DEMO:
	case DOOM_1_7_DEMO:
	case DOOM_1_8_DEMO:
	case DOOM_1_9_DEMO:
	case DOOM_1_9p_DEMO:
	case DOOM_1_9_1_DEMO:
		return true;
	default:
		return false;
	}
}

static bool G_TryReadDoomDemoHeader(demo_header_t& header, byte*& demo_p, byte* demo_e)
{
	if (!G_DemoHasBytesLeft(demo_p, demo_e, 13))
	{
		return false;
	}

	const byte version = *demo_p++;
	if (!G_IsDoomVanillaDemoVersion(version))
	{
		return false;
	}

	header.format = DEMOFORMAT_DOOM_VANILLA;
	demoversion = version == DOOM_1_9_1_DEMO ? LMP_DOOM_1_9_1 : LMP_DOOM_1_9;
	header.skill = *demo_p++ + 1;
	header.episode = *demo_p++;
	header.map = *demo_p++;
	header.deathmatch = *demo_p++;
	header.monstersrespawn = *demo_p++ != 0;
	header.fastmonsters = *demo_p++ != 0;
	header.nomonsters = *demo_p++ != 0;
	header.consoleplayer = *demo_p++;

	for (byte i = 0; i < MAXPLAYERS_VANILLA; ++i)
	{
		header.playerPresent[i] = *demo_p++ != 0;
	}

	longtics = demoversion == LMP_DOOM_1_9_1;
	return true;
}

static byte G_SelectFirstDemoPlayer(const byte (&playerPresent)[MAXPLAYERS_VANILLA])
{
	for (byte i = 0; i < MAXPLAYERS_VANILLA; ++i)
	{
		if (playerPresent[i])
		{
			return i;
		}
	}

	return 0;
}

static bool G_TryReadHereticDemoHeader(demo_header_t& header, byte*& demo_p, byte* demo_e)
{
	if (!G_DemoHasBytesLeft(demo_p, demo_e, 3 + MAXPLAYERS_VANILLA))
	{
		return false;
	}

	header.format = DEMOFORMAT_HERETIC_VANILLA;
	header.skill = *demo_p++ + 1;
	header.episode = *demo_p++;
	header.map = *demo_p++;

	const byte playerOneFlags = *demo_p++;
	header.playerPresent[0] = playerOneFlags != 0;
	header.monstersrespawn = (playerOneFlags & HERETIC_DEMOHEADER_RESPAWN) != 0;
	header.nomonsters = (playerOneFlags & HERETIC_DEMOHEADER_NOMONSTERS) != 0;
	longtics = (playerOneFlags & HERETIC_DEMOHEADER_LONGTICS) != 0;
	demoversion = longtics ? LMP_HERETIC_1_3_LONGTICS : LMP_HERETIC_1_3;

	for (byte i = 1; i < MAXPLAYERS_VANILLA; ++i)
	{
		header.playerPresent[i] = *demo_p++ != 0;
	}

	header.consoleplayer = G_SelectFirstDemoPlayer(header.playerPresent);
	return true;
}

static const demo_codec_t* G_GetDemoCodec(const demoformat_t format)
{
	for (const demo_codec_t& codec : g_demoCodecs)
	{
		if (codec.format == format)
		{
			return &codec;
		}
	}

	return nullptr;
}

static void G_ReadDoomDemoTiccmd(byte*& demo_p, byte* demo_e)
{
	switch (demoversion)
	{
	case LMP_DOOM_1_9:
	case LMP_DOOM_1_9_1:
		break;
	default:
		return;
	}

	const int demostep = demoversion == LMP_DOOM_1_9_1 ? 5 : 4;

	for (auto& player : players)
	{
		if (!G_DemoHasBytesLeft(demo_p, demo_e, demostep) || (*demo_p == DEMOMARKER))
		{
			G_CheckDemoStatus();
			return;
		}

		player.cmd.pitch = 0;
		player.cmd.upmove = 0;
		player.cmd.impulse = 0;
		player.cmd.forwardmove = (static_cast<int8_t>(*demo_p++)) << 8;
		player.cmd.sidemove = (static_cast<int8_t>(*demo_p++)) << 8;

		if (demoversion == LMP_DOOM_1_9)
		{
			player.cmd.yaw = static_cast<byte>(*demo_p++) << 8;
		}
		else
		{
			player.cmd.yaw = static_cast<uint16_t>(*demo_p++);
			player.cmd.yaw |= static_cast<uint16_t>(*demo_p++) << 8;
		}
		player.cmd.buttons = static_cast<byte>(*demo_p++);
	}
}

static void G_ReadHereticDemoTiccmd(byte*& demo_p, byte* demo_e)
{
	switch (demoversion)
	{
	case LMP_HERETIC_1_3:
	case LMP_HERETIC_1_3_LONGTICS:
		break;
	default:
		return;
	}

	const int demostep = demoversion == LMP_HERETIC_1_3_LONGTICS ? 6 : 5;

	for (auto& player : players)
	{
		if (!G_DemoHasBytesLeft(demo_p, demo_e, demostep) || (*demo_p == DEMOMARKER))
		{
			G_CheckDemoStatus();
			return;
		}

		player.cmd.pitch = 0;
		player.cmd.upmove = 0;
		player.cmd.impulse = 0;
		player.cmd.forwardmove = (static_cast<int8_t>(*demo_p++)) << 8;
		player.cmd.sidemove = (static_cast<int8_t>(*demo_p++)) << 8;

		if (demoversion == LMP_HERETIC_1_3_LONGTICS)
		{
			player.cmd.yaw = static_cast<uint16_t>(*demo_p++);
			player.cmd.yaw |= static_cast<uint16_t>(*demo_p++) << 8;
		}
		else
		{
			player.cmd.yaw = static_cast<byte>(*demo_p++) << 8;
		}

		player.cmd.buttons = static_cast<byte>(*demo_p++);

		const byte lookfly = static_cast<byte>(*demo_p++);
		const byte arti = static_cast<byte>(*demo_p++);
		static_cast<void>(lookfly);
		static_cast<void>(arti);
	}
}

bool G_TryReadDemoHeader(demo_header_t& header, demoformat_t format, byte*& demo_p, byte* demo_e)
{
	current_demo_codec = G_GetDemoCodec(format);
	return current_demo_codec ? current_demo_codec->tryReadHeader(header, demo_p, demo_e) : false;
}

void G_ReadDemoTiccmd(byte*& demo_p, byte* demo_e)
{
	if (current_demo_codec)
	{
		current_demo_codec->readTiccmd(demo_p, demo_e);
	}
}

const char* G_GetCurrentDemoCodecName()
{
	return current_demo_codec ? current_demo_codec->name : "UNKNOWN";
}

void G_ClearDemoCodec()
{
	current_demo_codec = nullptr;
}
