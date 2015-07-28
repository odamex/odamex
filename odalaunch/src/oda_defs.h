// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
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
//  The launchers default settings
//
//-----------------------------------------------------------------------------

#ifndef __ODA_DEFS_H__
#define __ODA_DEFS_H__

// User interface
// --------------

// Get a list of servers when the program is first loaded
#define ODA_UIGETLISTONSTART 1

// Show servers that cannot be contacted because of a firewall or bad connection
#define ODA_UISHOWBLOCKEDSERVERS 0

// Load chat client when the launcher is run
#define ODA_UILOADCHATCLIENTONLS 0

// Automatically checks for updates against the launchers version when run
#define ODA_UIAUTOCHECKFORUPDATES 1

// Integer ranges for ping quality, these are displayed as icons in the settings
// dialog
#define ODA_UIPINGQUALITYGOOD 150
#define ODA_UIPINGQUALITYPLAYABLE 300
#define ODA_UIPINGQUALITYLAGGY 350

/* User notification of when players are online */
// Flashes the taskbar if one is available
#define ODA_UIPOLFLASHTASKBAR 1
// Plays the system bell
#define ODA_UIPOLPLAYSYSTEMBELL 1
// Plays a sound (wav) file through the sound card
#define ODA_UIPOLPLAYSOUND 0
// Highlight the server that has players in it
#define ODA_UIPOLHIGHLIGHTSERVERS 1
// Highlight colour
#define ODA_UIPOLHSHIGHLIGHTCOLOUR "#00C000"
// Highlight the server that has players in it
#define ODA_UICSHIGHTLIGHTSERVERS 1
// Highlight colour
#define ODA_UICSHSHIGHLIGHTCOLOUR "#0094FF"

/* Auto server list refresh timer */
// Enables/disables the timer
#define ODA_UIARTENABLE 1
// Server list refresh interval (in ms, def 3m)
#define ODA_UIARTREFINTERVAL 180000
// Server list refresh interval max (in ms, def 45m)
#define ODA_UIARTREFMAX 2700000
// New list interval (in ms, def 1h)
#define ODA_UIARTLISTINTERVAL 3600000
// New list interval max (in ms, def 6h)
#define ODA_UIARTLISTMAX 21600000
// New list interval reduction value (in ms, def 5m)
#define ODA_UIARTLISTRED 300000

// Querying system
// ---------------

// Default list of master servers, usually official ones
static const char* def_masterlist[] =
{
	"master1.odamex.net:15000"
	,"voxelsoft.com:15000"
	,NULL
};

// Master server timeout
#define ODA_QRYMASTERTIMEOUT 500

// Game server timeout
#define ODA_QRYSERVERTIMEOUT 1000

// Number of retries to query the game server
#define ODA_QRYGSRETRYCOUNT 2

// Broadcast across all networks for servers
#define ODA_QRYUSEBROADCAST 0

// Thread multiplier value (this value * number of cores) for querying
#define ODA_THRMULVAL 12

// Maximum number of threads
#define ODA_THRMAXVAL 48

// Message for unresponsive servers
#define ODA_QRYNORESPONSE " << NO RESPONSE >> "

// Network subsystem
// -----------------


// File system
// -----------

// Configuration file key names
// Do not modify unless breaking our users config files is necessary
#define GETLISTONSTART      "GetListOnStart"
#define SHOWBLOCKEDSERVERS  "ShowBlockedServers"
#define CHECKFORUPDATES     "CheckForUpdates"
#define DELIMWADPATHS       "DelimWadPaths"
#define ODAMEX_DIRECTORY    "OdamexDirectory"
#define EXTRACMDLINEARGS    "ExtraCommandLineArguments"
#define MASTERTIMEOUT       "MasterTimeout"
#define SERVERTIMEOUT       "ServerTimeout"
#define USEBROADCAST        "UseBroadcast"
#define RETRYCOUNT          "RetryCount"
#define ICONPINGQGOOD       "IconPingQualityGood"
#define ICONPINGQPLAYABLE   "IconPingQualityPlayable"
#define ICONPINGQLAGGY      "IconPingQualityLaggy"
#define LOADCHATONLS        "LoadChatOnLauncherStart"
#define POLFLASHTBAR        "POLFlashTaskBar"
#define POLPLAYSYSTEMBELL   "POLPlaySystemBell"
#define POLPLAYSOUND        "POLPlaySound"
#define POLPSWAVFILE        "POLPSWavFile"
#define POLHLSERVERS        "POLHighlightServers"
#define POLHLSCOLOUR        "POLHighlightColour"
#define CSHLSERVERS         "CSHighlightServers"
#define CSHLCOLOUR          "CSHighlightColour"
#define ARTENABLE           "UseAutoRefreshTimer"
#define ARTREFINTERVAL      "AutoRefreshTimerRefreshInterval"
#define ARTNEWLISTINTERVAL  "AutoRefreshTimerNewListInterval"
#define QRYTHREADMULTIPLIER "QryThreadMultiplier"
#define QRYTHREADMAXIMUM    "QryThreadMaximum"

// Master server ids, eg:
// MasterServer1 "127.0.0.1:15000"
// MasterServer2 "192.168.1.1:15000"
// etc..
#define MASTERSERVER_ID     "MasterServer"

// Path separator
#ifdef __WXMSW__
#define PATH_DELIMITER ';'
#else
#define PATH_DELIMITER ':'
#endif

// Miscellaneous
// -------------


#endif // __ODA_DEFS_H__
