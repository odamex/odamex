// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2006-2026 by The Odamex Team.
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

#pragma once

#include "odalaunch.h"

// User interface
// --------------

// Get a list of servers when the program is first loaded
inline constexpr int ODA_UIGETLISTONSTART = 1;

// Show servers that cannot be contacted because of a firewall or bad connection
inline constexpr int ODA_UISHOWBLOCKEDSERVERS = 0;

// Load chat client when the launcher is run
inline constexpr int ODA_UILOADCHATCLIENTONLS = 0;

// Automatically checks for updates against the launchers version when run
inline constexpr int ODA_UIAUTOCHECKFORUPDATES = 1;

// Integer ranges for ping quality, these are displayed as icons in the settings
// dialog
inline constexpr int ODA_UIPINGQUALITYGOOD = 150;
inline constexpr int ODA_UIPINGQUALITYPLAYABLE = 300;
inline constexpr int ODA_UIPINGQUALITYLAGGY = 350;

/* User notification of when players are online */
// Flashes the taskbar if one is available
inline constexpr int ODA_UIPOLFLASHTASKBAR = 1;
// Plays the system bell
inline constexpr int ODA_UIPOLPLAYSYSTEMBELL = 1;
// Plays a sound (wav) file through the sound card
inline constexpr int ODA_UIPOLPLAYSOUND = 0;
// Highlight the server that has players in it
inline constexpr int ODA_UIPOLHIGHLIGHTSERVERS = 1;
// Highlight colour
inline constexpr const char* ODA_UIPOLHSHIGHLIGHTCOLOUR = "#00C000";
// Highlight the server that has players in it
inline constexpr int ODA_UICSHIGHTLIGHTSERVERS = 1;
// Highlight colour
inline constexpr const char* ODA_UICSHSHIGHLIGHTCOLOUR = "#0094FF";
// Program Theme 
inline constexpr int ODA_PRGTHEME = 0;

/* Auto server list refresh timer */
// Enables/disables the timer
inline constexpr int ODA_UIARTENABLE = 1;
// Server list refresh interval (in ms, def 3m)
inline constexpr int ODA_UIARTREFINTERVAL = 180000;
// Server list refresh interval max (in ms, def 45m)
inline constexpr int ODA_UIARTREFMAX = 2700000;
// New list interval (in ms, def 1h)
inline constexpr int ODA_UIARTLISTINTERVAL = 3600000;
// New list interval max (in ms, def 6h)
inline constexpr int ODA_UIARTLISTMAX = 21600000;
// New list interval reduction value (in ms, def 5m)
inline constexpr int ODA_UIARTLISTRED = 300000;

// Querying system
// ---------------

// Default list of master servers, usually official ones
inline constexpr const char* def_masterlist[] =
{
	"master1.odamex.net:15000"
	,"voxelsoft.com:15000"
	,"odamex.electricbrass.net:15000"
	,nullptr
};

// Master server timeout
inline constexpr int ODA_QRYMASTERTIMEOUT = 500;

// Game server timeout
inline constexpr int ODA_QRYSERVERTIMEOUT = 1000;

// Number of retries to query the game server
inline constexpr int ODA_QRYGSRETRYCOUNT = 2;

// Broadcast across all networks for servers
inline constexpr int ODA_QRYUSEBROADCAST = 0;

// Thread multiplier value (this value * number of cores) for querying
inline constexpr int ODA_THRMULVAL = 12;

// Maximum number of threads
inline constexpr int ODA_THRMAXVAL = 48;

// Message for unresponsive servers
inline constexpr const char* ODA_QRYNORESPONSE = " << NO RESPONSE >> ";

// Network subsystem
// -----------------

// The default port number of a server if none was specified
inline constexpr int ODA_NETDEFSERVERPORT = 10666;

// File system
// -----------

// Configuration file key names
// Do not modify unless breaking our users config files is necessary
inline constexpr const char* GETLISTONSTART = "GetListOnStart";
inline constexpr const char* SHOWBLOCKEDSERVERS = "ShowBlockedServers";
inline constexpr const char* CHECKFORUPDATES = "CheckForUpdates";
inline constexpr const char* DELIMWADPATHS = "DelimWadPaths";
inline constexpr const char* ODAMEX_DIRECTORY = "OdamexDirectory";
inline constexpr const char* EXTRACMDLINEARGS = "ExtraCommandLineArguments";
inline constexpr const char* MASTERTIMEOUT = "MasterTimeout";
inline constexpr const char* SERVERTIMEOUT = "ServerTimeout";
inline constexpr const char* USEBROADCAST = "UseBroadcast";
inline constexpr const char* RETRYCOUNT = "RetryCount";
inline constexpr const char* ICONPINGQGOOD = "IconPingQualityGood";
inline constexpr const char* ICONPINGQPLAYABLE = "IconPingQualityPlayable";
inline constexpr const char* ICONPINGQLAGGY = "IconPingQualityLaggy";
inline constexpr const char* LOADCHATONLS = "LoadChatOnLauncherStart";
inline constexpr const char* POLFLASHTBAR = "POLFlashTaskBar";
inline constexpr const char* POLPLAYSYSTEMBELL = "POLPlaySystemBell";
inline constexpr const char* POLPLAYSOUND = "POLPlaySound";
inline constexpr const char* POLPSWAVFILE = "POLPSWavFile";
inline constexpr const char* POLHLSERVERS = "POLHighlightServers";
inline constexpr const char* POLHLSCOLOUR = "POLHighlightColour";
inline constexpr const char* CSHLSERVERS = "CSHighlightServers";
inline constexpr const char* CSHLCOLOUR = "CSHighlightColour";
inline constexpr const char* ARTENABLE = "UseAutoRefreshTimer";
inline constexpr const char* ARTREFINTERVAL = "AutoRefreshTimerRefreshInterval";
inline constexpr const char* ARTNEWLISTINTERVAL = "AutoRefreshTimerNewListInterval";
inline constexpr const char* QRYTHREADMULTIPLIER = "QryThreadMultiplier";
inline constexpr const char* QRYTHREADMAXIMUM = "QryThreadMaximum";
inline constexpr const char* PRGTHEME = "ProgramTheme";

// Master server ids, eg:
// MasterServer1 "127.0.0.1:15000"
// MasterServer2 "192.168.1.1:15000"
// etc..
inline constexpr const char* MASTERSERVER_ID = "MasterServer";

// Path separator
#ifdef __WXMSW__
#define PATH_DELIMITER ';'
#else
#define PATH_DELIMITER ':'
#endif

// Miscellaneous
// -------------

