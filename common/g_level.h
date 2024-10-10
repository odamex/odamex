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

#pragma once

#include "cmdlib.h"
#include "m_fixed.h"
#include "m_resfile.h"
#include "olumpname.h"
#include "r_defs.h" // line_t

#include <assert.h>

#define NUM_MAPVARS				128
#define NUM_WORLDVARS			256
#define NUM_GLOBALVARS			64

/**
 * @brief Level flag bitfield.
 */
typedef uint32_t levelFlags_t;

const static levelFlags_t LEVEL_NOINTERMISSION = BIT(0);
const static levelFlags_t LEVEL_DOUBLESKY = BIT(2);
const static levelFlags_t LEVEL_NOSOUNDCLIPPING = BIT(3);

const static levelFlags_t LEVEL_MAP07SPECIAL = BIT(4);
const static levelFlags_t LEVEL_BRUISERSPECIAL = BIT(5);
const static levelFlags_t LEVEL_CYBORGSPECIAL = BIT(6);
const static levelFlags_t LEVEL_SPIDERSPECIAL = BIT(7);

const static levelFlags_t LEVEL_SPECLOWERFLOOR = BIT(8);
const static levelFlags_t LEVEL_SPECOPENDOOR = BIT(9);
const static levelFlags_t LEVEL_SPECACTIONSMASK = BIT_MASK(LEVEL_SPECLOWERFLOOR, LEVEL_SPECOPENDOOR);
const static levelFlags_t LEVEL_MONSTERSTELEFRAG = BIT(10);
const static levelFlags_t LEVEL_EVENLIGHTING = BIT(11);

const static levelFlags_t LEVEL_SNDSEQTOTALCTRL = BIT(12);
const static levelFlags_t LEVEL_FORCENOSKYSTRETCH = BIT(13);
const static levelFlags_t LEVEL_JUMP_NO = BIT(14);
const static levelFlags_t LEVEL_JUMP_YES = BIT(15);

const static levelFlags_t LEVEL_FREELOOK_NO = BIT(16);
const static levelFlags_t LEVEL_FREELOOK_YES = BIT(17);
const static levelFlags_t LEVEL_COMPAT_DROPOFF = BIT(18);
const static levelFlags_t LEVEL_COMPAT_NOPASSOVER = BIT(19);
const static levelFlags_t LEVEL_COMPAT_LIMITPAIN = BIT(20);

 // Automatically start lightning
const static levelFlags_t LEVEL_STARTLIGHTNING = BIT(24);
// Apply mapthing filtering to player starts
const static levelFlags_t LEVEL_FILTERSTARTS = BIT(25);
// That level is a lobby, and has a few priorities
const static levelFlags_t LEVEL_LOBBYSPECIAL = BIT(26);
// Player spawns will have z-height
const static levelFlags_t LEVEL_USEPLAYERSTARTZ = BIT(27);

 // Level was defined in a MAPINFO lump
const static levelFlags_t LEVEL_DEFINEDINMAPINFO = BIT(29);
// Don't display cluster messages
const static levelFlags_t LEVEL_CHANGEMAPCHEAT = BIT(30);
// Used for intermission map
const static levelFlags_t LEVEL_VISITED = BIT(31);

struct acsdefered_s;
class FBehavior;
struct bossaction_t;

struct level_info_t
{
	OLumpName		mapname;
	int				levelnum;
	std::string		level_name;
	byte			level_fingerprint[16];
	OLumpName		pname;
	OLumpName		nextmap;
	OLumpName		secretmap;
	int				partime;
	OLumpName		skypic;
	OLumpName		music;
	uint32_t		flags;
	int				cluster;
	FLZOMemFile*	snapshot;
	acsdefered_s*	defered;

	level_info_t()
	    : mapname(""), levelnum(0), level_name(""), pname(""), nextmap(""), secretmap(""),
	      partime(0), skypic(""), music(""), flags(0), cluster(0), snapshot(NULL),
	      defered(NULL)
	{
		ArrayInit(level_fingerprint, 0);
	}

	bool exists() const
	{
		return !this->mapname.empty();
	}
};

// struct that contains a FarmHash 128-bit fingerprint.
struct fhfprint_s
{
	byte fingerprint[16];

	fhfprint_s() : fingerprint()
	{
		ArrayInit(fingerprint, 0);
	}
	bool operator==(const fhfprint_s& other)
	{
		return fingerprint == other.fingerprint;
	}
};

struct level_pwad_info_t
{
	// level_info_t
	OLumpName		mapname;
	int				levelnum;
	std::string		level_name;
	byte			level_fingerprint[16];
	OLumpName		pname;
	OLumpName		nextmap;
	OLumpName		secretmap;
	int				partime;
	OLumpName		skypic;
	OLumpName		music;
	uint32_t		flags;
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

	OLumpName		fadetable;
	OLumpName		skypic2;
	float			gravity;
	float			aircontrol;

	// The following are necessary for UMAPINFO compatibility
	OLumpName		exitpic;
	OLumpName		enterpic;
	OLumpName		endpic;

	std::string		intertext;
	std::string		intertextsecret;
	OLumpName		interbackdrop;
	OLumpName		intermusic;

	fixed_t			sky1ScrollDelta;
	fixed_t			sky2ScrollDelta;

	std::vector<bossaction_t> bossactions;

	std::string		label;
	bool			clearlabel;
	std::string		author;

	level_pwad_info_t()
	    : mapname(""), levelnum(0), level_name(""), pname(""), nextmap(""), secretmap(""),
	      partime(0), skypic(""), music(""), flags(0), cluster(0), snapshot(NULL),
	      defered(NULL), fadetable("COLORMAP"), skypic2(""), gravity(0.0f),
	      aircontrol(0.0f), exitpic(""), enterpic(""), endpic(""), intertext(""),
	      intertextsecret(""), interbackdrop(""), intermusic(""),
	      sky1ScrollDelta(0), sky2ScrollDelta(0), bossactions(), label(),
	      clearlabel(false), author()
	{
		ArrayInit(fadeto_color, 0);
		ArrayInit(level_fingerprint, 0);
		ArrayInit(outsidefog_color, 0);
		outsidefog_color[0] = 0xFF; // special token signaling to not handle it specially
	}

	level_pwad_info_t(const level_info_t& other)
	    : mapname(other.mapname), levelnum(other.levelnum), level_name(other.level_name),
	      pname(other.pname), nextmap(other.nextmap),
	      secretmap(other.secretmap), partime(other.partime), skypic(other.skypic),
	      music(other.music), flags(other.flags), cluster(other.cluster),
	      snapshot(other.snapshot), defered(other.defered), fadetable("COLORMAP"),
	      skypic2(""), gravity(0.0f), aircontrol(0.0f), exitpic(""), enterpic(""),
	      endpic(""), intertext(""), intertextsecret(""), interbackdrop(""), intermusic(""),
	      sky1ScrollDelta(0), sky2ScrollDelta(0), bossactions(), label(),
	      clearlabel(false), author()
	{
		ArrayInit(fadeto_color, 0);
		ArrayInit(outsidefog_color, 0);
		ArrayInit(level_fingerprint, 0);
		outsidefog_color[0] = 0xFF; // special token signaling to not handle it specially
	}

	level_pwad_info_t& operator=(const level_pwad_info_t& other)
	{
		if (this == &other)
			return *this;

		mapname = other.mapname;
		levelnum = other.levelnum;
		level_name = other.level_name;
		pname = other.pname;
		nextmap = other.nextmap;
		secretmap = other.secretmap;
		partime = other.partime;
		skypic = other.skypic;
		music = other.music;
		flags = other.flags;
		cluster = other.cluster;
		snapshot = other.snapshot;
		defered = other.defered;
		ArrayCopy(fadeto_color, other.fadeto_color);
		ArrayCopy(outsidefog_color, other.outsidefog_color);
		ArrayCopy(level_fingerprint, other.level_fingerprint);
		fadetable = other.fadetable;
		skypic2 = other.skypic2;
		gravity = other.gravity;
		aircontrol = other.aircontrol;
		exitpic = other.exitpic;
		enterpic = other.enterpic;
		endpic = other.endpic;
		intertext = other.intertext;
		intertextsecret = other.intertextsecret;
		interbackdrop = other.interbackdrop;
		intermusic = other.intermusic;
		sky1ScrollDelta = other.sky1ScrollDelta;
		sky2ScrollDelta = other.sky2ScrollDelta;
		bossactions.clear();
		std::copy(other.bossactions.begin(), other.bossactions.end(),
		          bossactions.begin());
		label = other.label;
		clearlabel = other.clearlabel;
		author = other.author;

		return *this;
	}

	bool exists() const
	{
		return !this->mapname.empty();
	}
};


struct level_locals_t
{
	int				time;
	int				starttime;
	int				partime;
	unsigned int	inttimeleft;

	level_info_t*	info;
	int				cluster;
	int				levelnum;
	char			level_name[64];			// the descriptive name (Outer Base, etc)
	byte			level_fingerprint[16];	// [Blair] 128-bit FarmHash fingerprint generated for the level to describe it uniquely
											// so it can besingled out if it's out of its host wad, like in a compilation wad. Contains a 16-byte array.
	OLumpName		mapname;                // the server name (base1, etc)
	OLumpName		nextmap;				// go here when sv_fraglimit is hit
	OLumpName		secretmap;				// map to go to when used secret exit

	DWORD			flags;

	// [SL] use 4 bytes for color types instead of argb_t so that the struct
	// can consist of only plain-old-data types. It is also important to have
	// the channel layout be platform neutral in case the pixel format changes
	// after the level has been loaded (eg, toggling full-screen on certain OSX version).
	// The color channels are ordered: A, R, G, B
	byte			fadeto_color[4];		// The color the palette fades to (usually black)
	byte			outsidefog_color[4];	// The fog for sectors with sky ceilings

	OLumpName		music;
	OLumpName		skypic;
	OLumpName		skypic2;

	fixed_t			sky1ScrollDelta;
	fixed_t			sky2ScrollDelta;

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
	OLumpName		exitpic;
	OLumpName		enterpic;
	OLumpName		endpic;

	std::string		intertext;
	std::string		intertextsecret;
	OLumpName		interbackdrop;
	OLumpName		intermusic;

	std::vector<bossaction_t> bossactions;

	std::string		label;
	bool			clearlabel;
	std::string		author;

	// The following is used for automatic gametype detection.
	float			detected_gametype;
};

typedef uint32_t clusterFlags_t;

const static clusterFlags_t CLUSTER_HUB = BIT(0);
const static clusterFlags_t CLUSTER_EXITTEXTISLUMP = BIT(1);

struct bossaction_t
{
	int type;
	short special;
	short tag;

	bossaction_t() : type(MT_NULL), special(), tag() {}
};

struct cluster_info_t
{
	int				cluster;
	OLumpName		messagemusic;
	OLumpName		finaleflat;
	char*			exittext;
	char*			entertext;
	int				flags;
	OLumpName		finalepic;

	cluster_info_t()
	    : cluster(0), messagemusic(""), finaleflat(""), exittext(NULL), entertext(NULL),
	      flags(0)
	{
	}

	bool exists() const
	{
		return this->cluster != 0;
	}
};

extern level_locals_t level;

class LevelInfos
{
	typedef std::vector<level_pwad_info_t> _LevelInfoArray;
	const level_info_t* m_defaultInfos;
	std::vector<level_pwad_info_t> m_infos;
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
	level_pwad_info_t& findByName(const OLumpName& mapname);
	level_pwad_info_t& findByNum(int levelnum);
	size_t size();
	void zapDeferreds();
};

class ClusterInfos
{
	typedef std::vector<cluster_info_t> _ClusterInfoArray;
	const cluster_info_t* m_defaultInfos;
	std::vector<cluster_info_t> m_infos;
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

typedef OHashTable<int, int> ACSWorldGlobalArray;

extern int ACS_WorldVars[NUM_WORLDVARS];
extern int ACS_GlobalVars[NUM_GLOBALVARS];
extern ACSWorldGlobalArray ACS_WorldArrays[NUM_WORLDVARS];
extern ACSWorldGlobalArray ACS_GlobalArrays[NUM_GLOBALVARS];

extern BOOL savegamerestore;

void G_InitNew(const char *mapname);
inline void G_InitNew(const OLumpName& mapname) { G_InitNew(mapname.c_str()); }
void G_ChangeMap();
void G_ChangeMap(size_t index);
void G_RestartMap();

// Can be called by the startup code or M_Responder.
// A normal game starts at map 1,
// but a warp test can start elsewhere
void G_DeferedInitNew(const char *mapname);

// Map reset functions
void G_DeferedFullReset();
void G_DeferedReset();

void G_ExitLevel(int position, int drawscores, bool resetinv = false);
void G_SecretExitLevel(int position, int drawscores, bool resetinv = false);

void G_DoLoadLevel(int position);
void G_DoResetLevel(bool full_reset);

void G_InitLevelLocals();

void G_AirControlChanged();

char *CalcMapName(int episode, int level);

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
