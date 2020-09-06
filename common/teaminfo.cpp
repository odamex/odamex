#include "teaminfo.h"

TeamInfo s_Teams[NUMTEAMS];
TeamInfo s_NoTeam;

void InitTeamInfo()
{
	TeamInfo* teamInfo = &s_Teams[TEAM_BLUE];
	teamInfo->Team = TEAM_BLUE;
	teamInfo->Color = argb_t(255, 0, 0, 255);
	teamInfo->ColorStringUpper = "BLUE";
	teamInfo->ColorString = "Blue";
	teamInfo->TextColor = "\\ch";
	teamInfo->FountainColorArg = 3;
	teamInfo->TeamSpawnThingNum = 5080;
	teamInfo->FlagThingNum = 5130;
	teamInfo->FlagSocketSprite = SPR_BSOK;
	teamInfo->FlagSprite = SPR_BFLG;
	teamInfo->FlagDownSprite = SPR_BDWN;
	teamInfo->Points = 0;

	teamInfo = &s_Teams[TEAM_RED];
	teamInfo->Team = TEAM_RED;
	teamInfo->Color = argb_t(255, 255, 0, 0);
	teamInfo->ColorStringUpper = "RED";
	teamInfo->ColorString = "Red";
	teamInfo->TextColor = "\\cg";
	teamInfo->FountainColorArg = 1;
	teamInfo->TeamSpawnThingNum = 5081;
	teamInfo->FlagThingNum = 5131;
	teamInfo->FlagSocketSprite = SPR_RSOK;
	teamInfo->FlagSprite = SPR_RFLG;
	teamInfo->FlagDownSprite = SPR_RDWN;
	teamInfo->FlagCarrySprite = SPR_RCAR;
	teamInfo->Points = 0;

	teamInfo = &s_Teams[TEAM_GREEN];
	teamInfo->Team = TEAM_GREEN;
	teamInfo->Color = argb_t(255, 0, 255, 0);
	teamInfo->ColorStringUpper = "GREEN";
	teamInfo->ColorString = "Green";
	teamInfo->TextColor = "\\cd";
	teamInfo->FountainColorArg = 2;
	teamInfo->TeamSpawnThingNum = 5083;
	teamInfo->FlagThingNum = 5133;
	teamInfo->FlagSocketSprite = SPR_GSOK;
	teamInfo->FlagSprite = SPR_GFLG;
	teamInfo->FlagDownSprite = SPR_GDWN;
	teamInfo->FlagCarrySprite = SPR_GCAR;
	teamInfo->Points = 0;

	s_NoTeam.Team = NUMTEAMS;
	s_NoTeam.Color = argb_t(255, 0, 255, 0);
	s_NoTeam.ColorStringUpper = "";
	s_NoTeam.ColorString = "";
	s_NoTeam.TextColor = "\\cc";
	s_NoTeam.FountainColorArg = 0;
	s_NoTeam.TeamSpawnThingNum = 0;
	s_NoTeam.FlagSocketSprite = 0;
	s_NoTeam.FlagSprite = 0;
	s_NoTeam.FlagDownSprite = 0;
	s_NoTeam.FlagCarrySprite = 0;
	s_NoTeam.Points = 0;
}

TeamInfo* GetTeamInfo(team_t team)
{
	if (team >= NUMTEAMS)
		return &s_NoTeam;

	return &s_Teams[team];
}