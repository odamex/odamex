// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id:$
//
// Copyright (C) 2006-2007 by The Odamex Team.
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


#ifndef __VERSION_H__
#define __VERSION_H__

// Lots of different representations for the version number
enum { VERSION = 10 };
#define VERSIONSTR "10"
#define CONFIGVERSIONSTR "10"
#define GAMEVER (0*256+10)

#define DOTVERSIONSTR "0.1"

// denis - per-file svn version stamps
class file_version
{
public:
	file_version(const char *uid, const char *id, const char *p, int l, const char *t, const char *d);	
};

#define VERSION_CONTROL(uid, id) static file_version file_version_unique_##uid(#uid, id, __FILE__, __LINE__, __TIME__, __DATE__);

#endif //__VERSION_H__


