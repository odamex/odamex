// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1998-2006 by Randy Heit (ZDoom).
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
//   Handles the loading of data from all ONCRINFO lumps,
//   as well as static functions to handle announcer events.
//
//-----------------------------------------------------------------------------

#include "odamex.h"

#include "w_wad.h"
#include "oscanner.h"
#include "gstrings.h"
#include "g_announcer.h"

AnnouncerManager& AnnouncerManager::getInstance()
{
	static AnnouncerManager instance;
	return instance;
}

AnnouncerManager::AnnouncerManager()
{
	announcerDict.clear();
	loadedAnnouncer = Announcer_s();
}

AnnouncerManager::~AnnouncerManager()
{
	reset();
}

void AnnouncerManager::reset()
{
	announcerDict.clear();
	loadedAnnouncer = Announcer_s();
}

void AnnouncerManager::loadAnnouncerDefaults()
{
	Announcer_s defaultAnnouncer;

	defaultAnnouncer.name = "Odamex Official Announcer";
	defaultAnnouncer.description = "The official Odamex announcer pack.";
	defaultAnnouncer.author = "Manc";

	// Fill in default sounds
	// Possessive CTF Announcements
	defaultAnnouncer.soundDict[ANN_YOURFLAGTAKEN] = "officialvox/your/flag/take";
	defaultAnnouncer.soundDict[ANN_ENEMYFLAGTAKEN] = "officialvox/enemy/flag/take";
	defaultAnnouncer.soundDict[ANN_YOURFLAGDROPPED] = "officialvox/your/flag/drop";
	defaultAnnouncer.soundDict[ANN_ENEMYFLAGDROPPED] = "officialvox/enemy/flag/drop";
	defaultAnnouncer.soundDict[ANN_YOURFLAGISBEINGRETURNED] = "officialvox/your/flag/manualreturn";
	defaultAnnouncer.soundDict[ANN_ENEMYFLAGISBEINGRETURNED] = "officialvox/enemy/flag/manualreturn";
	defaultAnnouncer.soundDict[ANN_YOURFLAGRETURNED] = "officialvox/your/flag/return";
	defaultAnnouncer.soundDict[ANN_ENEMYFLAGRETURNED] = "officialvox/enemy/flag/return";
	defaultAnnouncer.soundDict[ANN_YOURTEAMSCORES] = "officialvox/your/score";
	defaultAnnouncer.soundDict[ANN_ENEMYTEAMSCORES] = "officialvox/enemy/score";

	// Team Based CTF Announcements
	defaultAnnouncer.soundDict[ANN_REDFLAGTAKEN] = "officialvox/red/flag/take";
	defaultAnnouncer.soundDict[ANN_BLUEFLAGTAKEN] = "officialvox/blue/flag/take";
	defaultAnnouncer.soundDict[ANN_GREENFLAGTAKEN] = "officialvox/green/flag/take";
	defaultAnnouncer.soundDict[ANN_REDFLAGDROPPED] = "officialvox/red/flag/drop";
	defaultAnnouncer.soundDict[ANN_BLUEFLAGDROPPED] = "officialvox/blue/flag/drop";
	defaultAnnouncer.soundDict[ANN_GREENFLAGDROPPED] = "officialvox/green/flag/drop";
	defaultAnnouncer.soundDict[ANN_REDFLAGISBEINGRETURNED] = "officialvox/red/flag/manualreturn";
	defaultAnnouncer.soundDict[ANN_BLUEFLAGISBEINGRETURNED] = "officialvox/blue/flag/manualreturn";
	defaultAnnouncer.soundDict[ANN_GREENFLAGISBEINGRETURNED] = "officialvox/green/flag/manualreturn";
	defaultAnnouncer.soundDict[ANN_REDFLAGRETURNED] = "officialvox/red/flag/return";
	defaultAnnouncer.soundDict[ANN_BLUEFLAGRETURNED] = "officialvox/blue/flag/return";
	defaultAnnouncer.soundDict[ANN_GREENFLAGRETURNED] = "officialvox/green/flag/return";
	defaultAnnouncer.soundDict[ANN_REDTEAMSCORES] = "officialvox/red/score";
	defaultAnnouncer.soundDict[ANN_BLUETEAMSCORES] = "officialvox/blue/score";
	defaultAnnouncer.soundDict[ANN_GREENTEAMSCORES] = "officialvox/green/score";

	// Horde Mode Announcements
	defaultAnnouncer.soundDict[ANN_HORDEBOSSSPAWN] = "officialvox/horde/bossspawn";
	defaultAnnouncer.soundDict[ANN_LASTPLAYERALIVE] = "officialvox/lastplayeralive";
	defaultAnnouncer.soundDict[ANN_REVIVEDPLAYER] = "officialvox/horde/revivedplayer";

	// General Announcements
	defaultAnnouncer.soundDict[ANN_FIGHT] = "officialvox/fight";
	defaultAnnouncer.soundDict[ANN_FIVEMINUTEWARNING] = "officialvox/fiveminutewarning";
	defaultAnnouncer.soundDict[ANN_ONEMINUTEWARNING] = "officialvox/oneminutewarning";
	defaultAnnouncer.soundDict[ANN_THREEFRAGSLEFT] = "officialvox/threefragsleft";
	defaultAnnouncer.soundDict[ANN_TWOFRAGSLEFT] = "officialvox/twofragsleft";
	defaultAnnouncer.soundDict[ANN_ONEFRAGLEFT] = "officialvox/onefragleft";
	defaultAnnouncer.soundDict[ANN_FIVE] = "officialvox/five";
	defaultAnnouncer.soundDict[ANN_FOUR] = "officialvox/four";
	defaultAnnouncer.soundDict[ANN_FIVE] = "officialvox/three";
	defaultAnnouncer.soundDict[ANN_FIVE] = "officialvox/two";
	defaultAnnouncer.soundDict[ANN_FIVE] = "officialvox/one";
	defaultAnnouncer.soundDict[ANN_PLAYERELIMINATED] = "officialvox/playereliminated";
	defaultAnnouncer.soundDict[ANN_FIRSTBLOOD] = "officialvox/firstblood";

	// Lead Change Announcements
	defaultAnnouncer.soundDict[ANN_YOUHAVETHELEAD] = "officialvox/youhavethelead";
	defaultAnnouncer.soundDict[ANN_YOULOSTTHELEAD] = "officialvox/youlostthelead";
	defaultAnnouncer.soundDict[ANN_YOUTIEDFORTHELEAD] = "officialvox/youtiedforthelead";

	// Match Result Announcements
	defaultAnnouncer.soundDict[ANN_YOUWIN] = "officialvox/youwin";
	defaultAnnouncer.soundDict[ANN_YOULOSE] = "officialvox/youlose";
	defaultAnnouncer.soundDict[ANN_YOUTIED] = "officialvox/youtied";
}
