// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1998-2006 by Randy Heit (ZDoom 1.22).
// Copyright (C) 2006-2015 by The Odamex Team.
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
//	G_LEVEL
//
//-----------------------------------------------------------------------------


#ifndef __G_LEVEL_H__
#define __G_LEVEL_H__

#include "doomtype.h"
#include "doomdef.h"
#include "m_fixed.h"

#include <string>
#include <vector>

#define NUM_MAPVARS				128
#define NUM_WORLDVARS			256
#define NUM_GLOBALVARS			64

#define LEVEL_NOINTERMISSION	0x00000001u
#define	LEVEL_DOUBLESKY			0x00000004u
#define LEVEL_NOSOUNDCLIPPING	0x00000008u

#define LEVEL_MAP07SPECIAL		0x00000010u
#define LEVEL_BRUISERSPECIAL	0x00000020u
#define LEVEL_CYBORGSPECIAL		0x00000040u
#define LEVEL_SPIDERSPECIAL		0x00000080u

#define LEVEL_SPECLOWERFLOOR	0x00000100u
#define LEVEL_SPECOPENDOOR		0x00000200u
#define LEVEL_SPECACTIONSMASK	0x00000300u

#define LEVEL_MONSTERSTELEFRAG	0x00000400u
#define LEVEL_EVENLIGHTING		0x00000800u
#define LEVEL_SNDSEQTOTALCTRL	0x00001000u
#define LEVEL_FORCENOSKYSTRETCH	0x00002000u

#define LEVEL_JUMP_NO			0x00004000u
#define LEVEL_JUMP_YES			0x00008000u
#define LEVEL_FREELOOK_NO		0x00010000u
#define LEVEL_FREELOOK_YES		0x00020000u

#define LEVEL_STARTLIGHTNING	0x01000000u	// Automatically start lightning
#define LEVEL_FILTERSTARTS		0x02000000u	// Apply mapthing filtering to player starts
#define LEVEL_LOBBYSPECIAL		0x04000000u	// That level is a lobby, and has a few priorities

#define LEVEL_DEFINEDINMAPINFO	0x20000000u	// Level was defined in a MAPINFO lump
#define LEVEL_CHANGEMAPCHEAT	0x40000000u	// Don't display cluster messages
#define LEVEL_VISITED			0x80000000u	// Used for intermission map


struct acsdefered_s;
class FBehavior;

struct level_info_t {
	char			mapname[9];
	int				levelnum;
	const char*		level_name;
	char			pname[9];
	char			nextmap[9];
	char			secretmap[9];
	int				partime;
	char			skypic[9];
	char			music[9];
	DWORD			flags;
	int				cluster;
	FLZOMemFile*	snapshot;
	acsdefered_s*	defered;
};

struct level_pwad_info_t
{
	// level_info_t
	char			mapname[9];
	int				levelnum;
	const char*		level_name;
	char			pname[9];
	char			nextmap[9];
	char			secretmap[9];
	int				partime;
	char			skypic[9];
	char			music[9];
	DWORD			flags;
	int				cluster;
	FLZOMemFile*	snapshot;
	acsdefered_s*	defered;

	// level_pwad_info_t

	// [SL] use 4 bytes for color types instead of argb_t so that the struct
	// can consist of only plain-old-data types. It is also important to have
	// the channel layout be platform neutral in case the pixel format changes
	// after the level has been loaded (eg, toggling full-screen on certain OSX version).
	// The color channels are ordered: A, R, G, B
	byte			fadeto_color[4];
	byte			outsidefog_color[4];

	char			fadetable[8];
	char			skypic2[9];
	float			gravity;
	float			aircontrol;
};


struct level_locals_t {
	int				time;
	int				starttime;
	int				partime;
	int				timeleft;
	unsigned int	inttimeleft;

	level_info_t*	info;
	int				cluster;
	int				levelnum;
	char			level_name[64];			// the descriptive name (Outer Base, etc)
	char			mapname[8];				// the server name (base1, etc)
	char			nextmap[8];				// go here when sv_fraglimit is hit
	char			secretmap[8];			// map to go to when used secret exit

	DWORD			flags;

	// [SL] use 4 bytes for color types instead of argb_t so that the struct
	// can consist of only plain-old-data types. It is also important to have
	// the channel layout be platform neutral in case the pixel format changes
	// after the level has been loaded (eg, toggling full-screen on certain OSX version).
	// The color channels are ordered: A, R, G, B
	byte			fadeto_color[4];		// The color the palette fades to (usually black)
	byte			outsidefog_color[4];	// The fog for sectors with sky ceilings

	char			music[8];
	char			skypic[8];
	char			skypic2[8];

	int				total_secrets;
	int				found_secrets;

	int				total_items;
	int				found_items;

	int				total_monsters;
	int				killed_monsters;

	float			gravity;
	fixed_t			aircontrol;
	fixed_t			airfriction;

	// The following are all used for ACS scripting
	FBehavior*		behavior;
	SDWORD			vars[NUM_MAPVARS];
};

struct cluster_info_t {
	int				cluster;
	char			messagemusic[9];
	char			finaleflat[9];
	const char*		exittext;
	const char*		entertext;
	int				flags;
};

// Only one cluster flag right now
#define CLUSTER_HUB		0x00000001

extern level_locals_t level;
extern level_info_t LevelInfos[];
extern cluster_info_t ClusterInfos[];

extern int ACS_WorldVars[NUM_WORLDVARS];
extern int ACS_GlobalVars[NUM_GLOBALVARS];

extern BOOL savegamerestore;
extern BOOL HexenHack;		// Semi-Hexen-compatibility mode

void G_InitNew (const char *mapname);
void G_ChangeMap (void);
void G_ChangeMap (size_t index);
void G_RestartMap (void);

// Can be called by the startup code or M_Responder.
// A normal game starts at map 1,
// but a warp test can start elsewhere
void G_DeferedInitNew (char *mapname);

// Map reset functions
void G_DeferedFullReset();
void G_DeferedReset();

void G_ExitLevel (int position, int drawscores);
void G_SecretExitLevel (int position, int drawscores);

void G_DoLoadLevel (int position);
void G_DoResetLevel (bool full_reset);

void G_InitLevelLocals (void);

void G_AirControlChanged ();

void G_SetLevelStrings (void);

cluster_info_t *FindClusterInfo (int cluster);
level_info_t *FindLevelInfo (char *mapname);
level_info_t *FindLevelByNum (int num);

char *CalcMapName (int episode, int level);

void G_ParseMapInfo (void);
void G_ParseMusInfo (void);

void G_ClearSnapshots (void);
void G_SnapshotLevel (void);
void G_UnSnapshotLevel (bool keepPlayers);
void G_SerializeSnapshots (FArchive &arc);

void cmd_maplist(const std::vector<std::string> &arguments, std::vector<std::string> &response);

extern bool unnatural_level_progression;

void P_RemoveDefereds (void);
int FindWadLevelInfo (char *name);
int FindWadClusterInfo (int cluster);

level_info_t *FindDefLevelInfo (char *mapname);
cluster_info_t *FindDefClusterInfo (int cluster);

bool G_LoadWad(	const std::vector<std::string> &newwadfiles,
				const std::vector<std::string> &newpatchfiles,
				const std::vector<std::string> &newwadhashes = std::vector<std::string>(),
				const std::vector<std::string> &newpatchhashes = std::vector<std::string>(),
				const std::string &mapname = "");

bool G_LoadWad(const std::string &str, const std::string &mapname = "");

#endif //__G_LEVEL_H__
