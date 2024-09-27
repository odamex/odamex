// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
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
//	Source versioning
//
//-----------------------------------------------------------------------------


#pragma once


#if defined(CLIENT_APP)
#define GAMEEXE "odamex"
#elif defined(SERVER_APP)
#define GAMEEXE "odasrv"
#else
#error "Odamex is not client or server"
#endif

/**
 * @brief Construct a packed integer from major, minor and patch version
 *        numbers.
 *
 * @param major Major version number.
 * @param minor Minor version number - must be between 0 and 25.
 * @param patch Patch version number - must be between 0 and 9.
 */
#define MAKEVER(major, minor, patch) ((major)*256 + ((minor)*10) + (patch))

/**
 * @brief Given a packed version integer, return the major version.
 */
#define VERMAJ(v) ((v) / 256)

/**
 * @brief Given a packed version integer, return the minor version.
 */
#define VERMIN(v) (((v) % 256) / 10)

/**
 * @brief Given a packed version integer, return the patch version.
 */
#define VERPATCH(v) (((v) % 256) % 10)

/**
 * @brief Break version into three output variables.
 */
#define BREAKVER(v, outmaj, outmin, outpat) \
	{                                       \
		outmaj = VERMAJ(v);                 \
		outmin = VERMIN(v);                 \
		outpat = VERPATCH(v);               \
	}

// Lots of different representations for the version number

// Used by configuration files.  upversion.py will update thie field
// deterministically and unambiguously so newer versions always compare
// greater.
#define CONFIGVERSIONSTR "010060"

#define DOTVERSIONSTR "10.6.0"
#define GAMEVER (MAKEVER(10, 6, 0))

#define COPYRIGHTSTR "Copyright (C) 2006-2024 The Odamex Team"

#define SERVERMAJ (VERMAJ(gameversion))
#define SERVERMIN (VERMIN(gameversion))
#define SERVERREL (VERPAT(gameversion))
#define CLIENTMAJ (VERMAJ(GAMEVER))
#define CLIENTMIN (VERMIN(GAMEVER))
#define CLIENTREL (VERPAT(GAMEVER))

// SAVESIG is the save game signature. It should be the minimum version
// whose savegames this version is compatible with, which could be
// earlier than this version.  Needs to be exactly 16 chars long.
// 
// upversion.py will update thie field deterministically and unambiguously.
#define SAVESIG "ODAMEXSAVE010060"

#define NETDEMOVER 3

int VersionCompat(const int server, const int client);
std::string VersionMessage(const int server, const int client, const char* email);

// denis - per-file svn version stamps
class file_version
{
public:
	file_version(const char *uid, const char *id, const char *p, int l, const char *t, const char *d);
};

#define VERSION_CONTROL(uid, id) static file_version file_version_unique_##uid(#uid, id, __FILE__, __LINE__, __TIME__, __DATE__);

const char* GitHash();
const char* GitBranch();
const char* GitRevCount();
const char* GitShortHash();
const char* NiceVersionDetails();
const char* NiceVersion();
