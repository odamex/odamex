// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2006-2012 by The Odamex Team.
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
//	Query protocol for server
//
//-----------------------------------------------------------------------------

#ifndef __SV_SQP_H__
#define __SV_SQP_H__

#include "version.h"
#include "doomtype.h"

#define ASSEMBLEVERSION(MAJOR,MINOR,PATCH) ((MAJOR) * 256 + (MINOR)(PATCH))
#define DISECTVERSION(VERSION,MAJOR,MINOR,PATCH) \
        { \
            MAJOR = (VERSION / 256); \
            MINOR = ((VERSION % 256) / 10); \
            PATCH = ((VERSION % 256) % 10); \
        }
        
#define VERSIONMAJOR(VERSION) (VERSION / 256)
#define VERSIONMINOR(VERSION) ((VERSION % 256) / 10)
#define VERSIONPATCH(VERSION) ((VERSION % 256) % 10)

DWORD SV_QryParseEnquiry(const DWORD &Tag);

#endif // __SV_SQP_H__
