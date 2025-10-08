// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1998-2006 by Randy Heit (ZDoom 1.22).
// Copyright (C) 2006-2025 by The Odamex Team.
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
#include "c_maplist.h"
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

constexpr static levelFlags_t LEVEL_NOINTERMISSION = BIT(0);
constexpr static levelFlags_t LEVEL_SECRET = BIT(1);
constexpr static levelFlags_t LEVEL_DOUBLESKY = BIT(2);
constexpr static levelFlags_t LEVEL_NOSOUNDCLIPPING = BIT(3);

constexpr static levelFlags_t LEVEL_MAP07SPECIAL = BIT(4);
constexpr static levelFlags_t LEVEL_BRUISERSPECIAL = BIT(5);
constexpr static levelFlags_t LEVEL_CYBORGSPECIAL = BIT(6);
constexpr static levelFlags_t LEVEL_SPIDERSPECIAL = BIT(7);

constexpr static levelFlags_t LEVEL_SPECLOWERFLOOR = BIT(8);
constexpr static levelFlags_t LEVEL_SPECOPENDOOR = BIT(9);
constexpr static levelFlags_t LEVEL_SPECACTIONSMASK = BIT_MASK(8, 9);
constexpr static levelFlags_t LEVEL_MONSTERSTELEFRAG = BIT(10);
constexpr static levelFlags_t LEVEL_EVENLIGHTING = BIT(11);

constexpr static levelFlags_t LEVEL_SNDSEQTOTALCTRL = BIT(12);
constexpr static levelFlags_t LEVEL_FORCENOSKYSTRETCH = BIT(13);
constexpr static levelFlags_t LEVEL_JUMP_NO = BIT(14);
constexpr static levelFlags_t LEVEL_JUMP_YES = BIT(15);

constexpr static levelFlags_t LEVEL_FREELOOK_NO = BIT(16);
constexpr static levelFlags_t LEVEL_FREELOOK_YES = BIT(17);
constexpr static levelFlags_t LEVEL_COMPAT_DROPOFF = BIT(18);
constexpr static levelFlags_t LEVEL_COMPAT_NOPASSOVER = BIT(19);
constexpr static levelFlags_t LEVEL_COMPAT_LIMITPAIN = BIT(20);
constexpr static levelFlags_t LEVEL_COMPAT_SHORTTEX = BIT(21);

 // Automatically start lightning
constexpr static levelFlags_t LEVEL_STARTLIGHTNING = BIT(24);
// Apply mapthing filtering to player starts
constexpr static levelFlags_t LEVEL_FILTERSTARTS = BIT(25);
// That level is a lobby, and has a few priorities
constexpr static levelFlags_t LEVEL_LOBBYSPECIAL = BIT(26);
// Player spawns will have z-height
constexpr static levelFlags_t LEVEL_USEPLAYERSTARTZ = BIT(27);

 // Level was defined in a MAPINFO lump
constexpr static levelFlags_t LEVEL_DEFINEDINMAPINFO = BIT(29);
// Don't display cluster messages
constexpr static levelFlags_t LEVEL_CHANGEMAPCHEAT = BIT(30);
// Used for intermission map
constexpr static levelFlags_t LEVEL_VISITED = BIT(31);

constexpr static levelFlags_t LEVEL2_NORMALINFIGHTING = BIT(0);
constexpr static levelFlags_t LEVEL2_NOINFIGHTING = BIT(1);
constexpr static levelFlags_t LEVEL2_TOTALINFIGHTING = BIT(2);
constexpr static levelFlags_t LEVEL2_INFIGHTINGMASK = BIT_MASK(0, 2);
constexpr static levelFlags_t LEVEL2_COMPAT_CROSSDROPOFF = BIT(18);

struct acsdefered_s;
class FBehavior;
struct bossaction_t;

// struct that contains a FarmHash 128-bit fingerprint.
struct fhfprint_t
{
	std::array<byte, 16> fingerprint{};

	[[nodiscard]]
	bool operator==(const fhfprint_t& other)
	{
		return fingerprint == other.fingerprint;
	}

	bool operator==(std::string_view other)
	{
		return other == this->toString();
	}

	void clear()
	{
		fingerprint.fill(0);
	}

	std::string toString()
	{
		// [Blair] Serialize the hashes before reading.
		const uint64_t reconsthash1 = (uint64_t)(fingerprint[0]) |
		                              (uint64_t)(fingerprint[1]) << 8 |
		                              (uint64_t)(fingerprint[2]) << 16 |
		                              (uint64_t)(fingerprint[3]) << 24 |
		                              (uint64_t)(fingerprint[4]) << 32 |
		                              (uint64_t)(fingerprint[5]) << 40 |
		                              (uint64_t)(fingerprint[6]) << 48 |
		                              (uint64_t)(fingerprint[7]) << 56;

		const uint64_t reconsthash2 = (uint64_t)(fingerprint[8]) |
		                              (uint64_t)(fingerprint[9]) << 8 |
		                              (uint64_t)(fingerprint[10]) << 16 |
		                              (uint64_t)(fingerprint[11]) << 24 |
		                              (uint64_t)(fingerprint[12]) << 32 |
		                              (uint64_t)(fingerprint[13]) << 40 |
		                              (uint64_t)(fingerprint[14]) << 48 |
		                              (uint64_t)(fingerprint[15]) << 56;

		return fmt::format("{:016x}{:016x}", reconsthash1, reconsthash2);
	}
};

struct level_info_t
{
	OLumpName     mapname    = "";
	int           levelnum   = 0;
	int           mapnum     = 0;
	int           episodenum = 0;
	std::string   level_name = "";
	fhfprint_t    level_fingerprint{};
	OLumpName     pname      = "";
	OLumpName     nextmap    = "";
	OLumpName     secretmap  = "";
	int           partime    = 0;
	OLumpName     skypic     = "";
	OLumpName     music      = "";
	levelFlags_t  flags      = 0;
	levelFlags_t  flags2     = 0;
	int           cluster    = 0;
	FLZOMemFile*  snapshot   = nullptr;
	acsdefered_s* defered    = nullptr;

	bool exists() const
	{
		return !this->mapname.empty();
	}
};

struct level_pwad_info_t
{
	// level_info_t
	OLumpName		mapname    = "";
	int				levelnum   = 0;
	int				mapnum     = 0;
	int				episodenum = 0;
	std::string		level_name = "";
	fhfprint_t		level_fingerprint{};
	OLumpName		pname      = "";
	OLumpName		nextmap    = "";
	OLumpName		secretmap  = "";
	int				partime    = 0;
	OLumpName		skypic     = "";
	OLumpName		music      = "";
	levelFlags_t	flags      = 0;
	levelFlags_t	flags2     = 0;
	int				cluster    = 0;
	FLZOMemFile*	snapshot   = nullptr;
	acsdefered_s*	defered    = nullptr;

	// level_pwad_info_t

	// [SL] use 4 bytes for color types instead of argb_t so that the struct
	// can consist of only plain-old-data types. It is also important to have
	// the channel layout be platform neutral in case the pixel format changes
	// after the level has been loaded (eg, toggling full-screen on certain OSX version).
	// The color channels are ordered: A, R, G, B
	std::array<byte, 4>	fadeto_color = { 0, 0, 0, 0 };
	std::array<byte, 4>	outsidefog_color = { 0xFF /* special token signaling to not handle it specially */, 0, 0, 0 };

	OLumpName		fadetable  = "COLORMAP";
	OLumpName		skypic2    = "";
	float			gravity    = 0.0f;
	float			aircontrol = 0.0f;
	int				airsupply  = 10;

	// The following are necessary for UMAPINFO compatibility
	OLumpName		exitpic     = "";
	OLumpName		enterpic    = "";
	OLumpName		exitscript  = "";
	OLumpName		enterscript = "";
	OLumpName		exitanim    = "";
	OLumpName		enteranim   = "";
	OLumpName		endpic      = "";

	std::string		intertext       = "";
	std::string		intertextsecret = "";
	OLumpName		interbackdrop   = "";
	OLumpName		intermusic      = "";
	OLumpName		zintermusic     = "";

	fixed_t			sky1ScrollDelta = 0;
	fixed_t			sky2ScrollDelta = 0;

	std::vector<bossaction_t> bossactions{};

	std::string		label      = "";
	bool			clearlabel = false;
	std::string		author     = "";

	level_pwad_info_t() = default;

	level_pwad_info_t(const level_info_t& other)
	    : mapname(other.mapname), levelnum(other.levelnum), mapnum(other.mapnum), episodenum(other.episodenum),
	      level_name(other.level_name), level_fingerprint(other.level_fingerprint), pname(other.pname), nextmap(other.nextmap),
	      secretmap(other.secretmap), partime(other.partime), skypic(other.skypic),
	      music(other.music), flags(other.flags), flags2(other.flags2), cluster(other.cluster),
	      snapshot(other.snapshot), defered(other.defered)
	{
	}

	level_pwad_info_t& operator=(const level_pwad_info_t& other) = default;

	level_pwad_info_t(const level_pwad_info_t& other) = default;

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
	fhfprint_t		level_fingerprint;	    // [Blair] 128-bit FarmHash fingerprint generated for the level to describe it uniquely
											// so it can besingled out if it's out of its host wad, like in a compilation wad. Contains a 16-byte array.
	OLumpName		mapname;                // the server name (base1, etc)
	OLumpName		nextmap;				// go here when sv_fraglimit is hit
	OLumpName		secretmap;				// map to go to when used secret exit

	levelFlags_t	flags;
	levelFlags_t	flags2;

	// [SL] use 4 bytes for color types instead of argb_t so that the struct
	// can consist of only plain-old-data types. It is also important to have
	// the channel layout be platform neutral in case the pixel format changes
	// after the level has been loaded (eg, toggling full-screen on certain OSX version).
	// The color channels are ordered: A, R, G, B
	std::array<byte, 4>	fadeto_color;		// The color the palette fades to (usually black)
	std::array<byte, 4>	outsidefog_color;	// The fog for sectors with sky ceilings

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
	int 			airsupply;

	// The following are all used for ACS scripting
	FBehavior*		behavior;
	SDWORD			vars[NUM_MAPVARS];

	// The following are used for UMAPINFO
	OLumpName		exitpic;
	OLumpName		exitscript;
	OLumpName		exitanim;
	OLumpName		enterpic;
	OLumpName		enterscript;
	OLumpName		enteranim;
	OLumpName		endpic;

	std::string		intertext;
	std::string		intertextsecret;
	OLumpName		interbackdrop;
	// umapinfo intermusic -- used for text screens
	OLumpName		intermusic;
	// zdoom intermusic -- used for intermissions
	OLumpName		zintermusic;

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
	std::string		exittext;
	std::string		entertext;
	int				flags;
	OLumpName		finalepic;

	cluster_info_t()
	    : cluster(0), messagemusic(""), finaleflat(""), exittext(""), entertext(""),
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

// ACS variables with world scope
inline std::array<int, NUM_WORLDVARS> ACS_WorldVars;
inline std::array<ACSWorldGlobalArray, NUM_WORLDVARS> ACS_WorldArrays;

// ACS variables with global scope
inline std::array<int, NUM_GLOBALVARS> ACS_GlobalVars;
inline std::array<ACSWorldGlobalArray, NUM_GLOBALVARS> ACS_GlobalArrays;

extern bool savegamerestore;

void G_InitNew(const char *mapname);
inline void G_InitNew(const OLumpName& mapname) { G_InitNew(mapname.c_str()); }
void G_ChangeMap();
void G_ChangeMap(size_t index);
void G_RestartMap();

// Can be called by the startup code or M_Responder.
// A normal game starts at map 1,
// but a warp test can start elsewhere
void G_DeferedInitNew(const OLumpName& mapname);

// Map reset functions
void G_DeferedFullReset();
void G_DeferedReset();

void G_ExitLevel(int position, int drawscores, bool resetinv = false);
void G_SecretExitLevel(int position, int drawscores, bool resetinv = false);

void G_DoLoadLevel(int position);
void G_DoResetLevel(bool full_reset);

void G_InitLevelLocals();

void G_AirControlChanged();

OLumpName CalcMapName(int episode, int level);

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
bool G_LoadWadString(const std::string& str, const std::string& mapname = "", const maplist_lastmaps_t& lastmaps = {});

LevelInfos& getLevelInfos();
ClusterInfos& getClusterInfos();

// Compatibility flags
bool P_AllowDropOff();
bool P_AllowPassover();
