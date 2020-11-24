#ifndef __TEAMINFO_H__
#define __TEAMINFO_H__

#include "doomdata.h"
#include "info.h"
#include "actor.h"

#include <string>
#include <vector>

enum team_t
{
	TEAM_BLUE,
	TEAM_RED,
	TEAM_GREEN,

	NUMTEAMS,

	TEAM_NONE
};

// flags can only be in one of these states
enum flag_state_t
{
	flag_home,
	flag_dropped,
	flag_carried,

	NUMFLAGSTATES
};

// data associated with a flag
struct flagdata
{
	// Does this flag have a spawn yet?
	bool flaglocated;

	// Actor when being carried by a player, follows player
	AActor::AActorPtr actor;

	// Integer representation of WHO has each flag (player id)
	byte flagger;
	int	pickup_time;

	// True if the flag is currently grabbed for the first time.
	bool firstgrab;

	// Flag locations
	int x, y, z;

	// Flag Timout Counters
	int timeout;

	// True when a flag has been dropped
	flag_state_t state;

	// Used for the blinking flag indicator on the statusbar
	int sb_tick;
};

struct TeamInfo
{
	team_t Team;
	std::string ColorStringUpper;
	std::string ColorString;
	argb_t Color;
	std::string TextColor;
	int TransColor;

	int FountainColorArg;

	int TeamSpawnThingNum;
	int FlagThingNum;
	std::vector<mapthing2_t> Starts;

	int FlagSocketSprite;
	int FlagSprite;
	int FlagDownSprite;
	int FlagCarrySprite;

	int Points;
	int RoundWins;
	flagdata FlagData;
};

struct TeamCount
{
	int result;
	int total;
	TeamCount() : result(0), total(0)
	{
	}
};

typedef std::vector<TeamInfo*> TeamResults;

/**
 * @brief Return teams with the top point count, whatever that may be.
 */
#define TQ_MAXPOINTS (1 << 0)

/**
 * @brief Return teams with the top win acount, whatever that may be.
 */
#define TQ_MAXWINS (1 << 1)

void InitTeamInfo();
TeamInfo* GetTeamInfo(team_t team);
TeamCount P_TeamQuery(TeamResults* out, unsigned flags);
std::string V_GetTeamColor(TeamInfo* team);
std::string V_GetTeamColor(team_t ateam);

#endif
