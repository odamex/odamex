// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1998-2006 by Randy Heit (ZDoom 1.22).
// Copyright (C) 2006-2020 by The Odamex Team.
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
#include "m_resfile.h"
#include "olumpname.h"

#include <string>
#include <vector>

#define NUM_MAPVARS				128
#define NUM_WORLDVARS			256
#define NUM_GLOBALVARS			64

enum OLevelFlags : unsigned int
{
	LEVEL_NOINTERMISSION = 0x00000001, 
	LEVEL_DOUBLESKY = 0x00000004,
	LEVEL_NOSOUNDCLIPPING = 0x00000008,

	LEVEL_MAP07SPECIAL = 0x00000010,
	LEVEL_BRUISERSPECIAL = 0x00000020,
	LEVEL_CYBORGSPECIAL = 0x00000040,
	LEVEL_SPIDERSPECIAL = 0x00000080,

	LEVEL_SPECLOWERFLOOR = 0x00000100,
	LEVEL_SPECOPENDOOR = 0x00000200,
	LEVEL_SPECACTIONSMASK =0x00000300,

	LEVEL_MONSTERSTELEFRAG = 0x00000400,
	LEVEL_EVENLIGHTING = 0x00000800,
	LEVEL_SNDSEQTOTALCTRL = 0x00001000,
	LEVEL_FORCENOSKYSTRETCH = 0x00002000,

	LEVEL_JUMP_NO = 0x00004000,
	LEVEL_JUMP_YES = 0x00008000,
	LEVEL_FREELOOK_NO = 0x00010000,
	LEVEL_FREELOOK_YES = 0x00020000,

	LEVEL_COMPAT_DROPOFF = 0x00040000,
	LEVEL_COMPAT_NOPASSOVER = 0x00080000,

	LEVEL_STARTLIGHTNING = 0x01000000,	// Automatically start lightning
	LEVEL_FILTERSTARTS = 0x02000000,	// Apply mapthing filtering to player starts
	LEVEL_LOBBYSPECIAL = 0x04000000,	// That level is a lobby, and has a few priorities

	LEVEL_DEFINEDINMAPINFO = 0x20000000, // Level was defined in a MAPINFO lump
	LEVEL_CHANGEMAPCHEAT = 0x40000000,	// Don't display cluster messages
	LEVEL_VISITED = 0x80000000,			// Used for intermission map


};






struct acsdefered_s;
class FBehavior;
struct BossAction;

struct level_info_t
{
	OLumpName		mapname;
	int				levelnum;
	std::string		level_name;
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

	bool exists() const
	{
		return !this->mapname.empty();
	}
};

struct level_pwad_info_t
{
	// level_info_t
	OLumpName		mapname;
	int				levelnum;
	std::string		level_name;
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

	char			fadetable[9];
	char			skypic2[9];
	float			gravity;
	float			aircontrol;

	// The following are necessary for UMAPINFO compatibility
	char			exitpic[9];
	char			enterpic[9];
	char			endpic[9];

	std::string		intertext;
	std::string		intertextsecret;
	char			interbackdrop[9];
	char			intermusic[9];
	
	std::vector<BossAction> bossactions;
	bool			bossactions_donothing;
	
	bool exists() const
	{
		return !this->mapname.empty();
	}
};


struct level_locals_t {
	int				time;
	int				starttime;
	int				partime;
	unsigned int	inttimeleft;

	level_info_t*	info;
	int				cluster;
	int				levelnum;
	char			level_name[64];			// the descriptive name (Outer Base, etc)
	OLumpName		mapname;                // the server name (base1, etc)
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
	int				respawned_monsters;	// Ch0wW - Keep track of respawned monsters

	float			gravity;
	fixed_t			aircontrol;
	fixed_t			airfriction;

	// The following are all used for ACS scripting
	FBehavior*		behavior;
	SDWORD			vars[NUM_MAPVARS];

	// The following are used for UMAPINFO
	char			exitpic[8];
	char			enterpic[8];
	char			endpic[8];

	std::string		intertext;
	std::string		intertextsecret;
	char			interbackdrop[9];
	char			intermusic[9];
	
	std::vector<BossAction> *bossactions;
	bool			bossactions_donothing;
	
};

#define CLUSTER_HUB            0x00000001u
#define CLUSTER_EXITTEXTISLUMP 0x00000002u

struct BossAction
{
	int type;
	int special;
	int tag;
};

struct cluster_info_t {
	int				cluster;
	char			messagemusic[9];
	char			finaleflat[9];
	char*			exittext;
	char*			entertext;
	int				flags;
	char			finalepic[9];

	BOOL exists() const
	{
		return this->cluster != 0;
	}
};

extern level_locals_t level;

class LevelInfos
{
	typedef std::vector<level_pwad_info_t> _LevelInfoArray;
	const level_info_t* _defaultInfos;
	static level_pwad_info_t _empty;
	std::vector<level_pwad_info_t> _infos;
public:
	LevelInfos(const level_info_t* levels);
	~LevelInfos();
	void addDefaults();
	level_pwad_info_t& at(size_t i);
	level_pwad_info_t& create();
	void clear();
	void clearSnapshots();
	level_pwad_info_t& findByName(const char* mapname);
	level_pwad_info_t& findByName(const std::string& mapname);
	level_pwad_info_t& findByNum(int levelnum);
	size_t size();
	void zapDeferreds();
};

class ClusterInfos
{
	typedef std::vector<cluster_info_t> _ClusterInfoArray;
	const cluster_info_t* _defaultInfos;
	static cluster_info_t _empty;
	std::vector<cluster_info_t> _infos;
public:
	ClusterInfos(const cluster_info_t* clusters);
	~ClusterInfos();
	void addDefaults();
	cluster_info_t& at(size_t i);
	void clear();
	cluster_info_t& create();
	cluster_info_t& findByCluster(int i);
	size_t size() const;
};

extern int ACS_WorldVars[NUM_WORLDVARS];
extern int ACS_GlobalVars[NUM_GLOBALVARS];

extern BOOL savegamerestore;
extern BOOL HexenHack;		// Semi-Hexen-compatibility mode

void G_InitNew(const char *mapname);
void G_ChangeMap();
void G_ChangeMap(size_t index);
void G_RestartMap();

// Can be called by the startup code or M_Responder.
// A normal game starts at map 1,
// but a warp test can start elsewhere
void G_DeferedInitNew(char *mapname);

// Map reset functions
void G_DeferedFullReset();
void G_DeferedReset();

void G_ExitLevel(int position, int drawscores);
void G_SecretExitLevel(int position, int drawscores);

void G_DoLoadLevel(int position);
void G_DoResetLevel(bool full_reset);

void G_InitLevelLocals();

void G_AirControlChanged();

char *CalcMapName(int episode, int level);

void G_ParseMapInfo();
void G_ParseMusInfo();

void G_ClearSnapshots();
void G_SnapshotLevel();
void G_UnSnapshotLevel(bool keepPlayers);
void G_SerializeSnapshots(FArchive &arc);

void cmd_maplist(const std::vector<std::string> &arguments, std::vector<std::string> &response);

extern bool unnatural_level_progression;

void P_RemoveDefereds();

bool G_LoadWad(const OWantFiles& newwadfiles, const OWantFiles& newpatchfiles,
               const std::string& mapname = "");
bool G_LoadWadString(const std::string& str, const std::string& mapname = "");

LevelInfos& getLevelInfos();
ClusterInfos& getClusterInfos();

// Compatibility flags
bool P_AllowDropOff();
bool P_AllowPassover();

#endif //__G_LEVEL_H__
