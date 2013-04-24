// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 2006-2013 by The Odamex Team.
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
//	File Input/Output Operations
//
//-----------------------------------------------------------------------------

#ifndef __M_FILEIO__
#define __M_FILEIO__

#include <string>
#include <vector>

#include <stdio.h>

#include "doomtype.h"
#include "d_main.h"

SDWORD M_FileLength (FILE *f);
BOOL M_FileExists (std::string filename);

BOOL M_WriteFile(std::string filename, void *source, QWORD length);
QWORD M_ReadFile(std::string filename, BYTE **buffer);

BOOL M_AppendExtension (std::string &filename, std::string extension, bool if_needed = true);
void M_ExtractFilePath (std::string filename, std::string &dest);
BOOL M_ExtractFileExtension (std::string filename, std::string &dest);
void M_ExtractFileBase (std::string filename, std::string &dest);
void M_ExtractFileName (std::string filename, std::string &dest);
std::string M_ExtractFileName(const std::string filename);

#endif
