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
	int FlagWaypointSprite;

	int Points;
	int RoundWins;
	flagdata FlagData;

	const std::string ColorizedTeamName();
};

/**
 * @brief A collection of pointers to teams, commonly called a "view".
 */
typedef std::vector<TeamInfo*> TeamsView;

class TeamQuery
{
	enum SortTypes
	{
		SORT_NONE,
		SORT_SCORE,
		SORT_WINS,
	};

	enum SortFilters
	{
		SFILTER_NONE,
		SFILTER_MAX,
		SFILTER_NOT_MAX,
	};

	SortTypes m_sort;
	SortFilters m_sortFilter;

  public:
	TeamQuery() : m_sort(SORT_NONE), m_sortFilter(SFILTER_NONE)
	{
	}

	/**
	 * @brief Return teams sorted by highest score.
	 *
	 * @return A mutated TeamQuery to chain off of.
	 */
	TeamQuery& sortScore()
	{
		m_sort = SORT_SCORE;
		return *this;
	}

	/**
	 * @brief Return teams sorted by highest wins.
	 *
	 * @return A mutated TeamQuery to chain off of.
	 */
	TeamQuery& sortWins()
	{
		m_sort = SORT_WINS;
		return *this;
	}

	/**
	 * @brief Given a sort, filter so only the top item remains.  In the case
	 *        of a tie, multiple items are returned.
	 *
	 * @return A mutated TeamQuery to chain off of.
	 */
	TeamQuery& filterSortMax()
	{
		m_sortFilter = SFILTER_MAX;
		return *this;
	}

	/**
	 * @brief Given a sort, filter so only things other than the top item
	 *        remains.
	 *
	 * @return A mutated TeamQuery to chain off of.
	 */
	TeamQuery& filterSortNotMax()
	{
		m_sortFilter = SFILTER_NOT_MAX;
		return *this;
	}

	TeamsView execute();
};

void InitTeamInfo();
TeamInfo* GetTeamInfo(team_t team);
std::string V_GetTeamColor(TeamInfo* team);
std::string V_GetTeamColor(team_t ateam);

#endif
