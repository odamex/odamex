// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
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
//	Handles WAD file header, directory, lump I/O.
//
//-----------------------------------------------------------------------------


#ifdef UNIX
#include <ctype.h>
#include <cstring>
#include <unistd.h>
#ifndef O_BINARY
#define O_BINARY		0
#endif
#endif

#ifdef _WIN32
#include <io.h>
#else
#define strcmpi	strcasecmp
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <ctype.h>

#include "doomtype.h"
#include "m_swap.h"
#include "m_fileio.h"
#include "i_system.h"
#include "z_zone.h"
#include "cmdlib.h"
#include "m_argv.h"
#include "md5.h"

#include "w_wad.h"


#include <string>
#include <sstream>
#include <algorithm>
#include <vector>
#include <iostream>
#include <iomanip>


//
// W_ReadChunk
//
// denis - for wad downloading
//
unsigned W_ReadChunk (const char *file, unsigned offs, unsigned len, void *dest, unsigned &filelen)
{
	FILE *fp = fopen(file, "rb");
	unsigned read = 0;

	if(fp)
	{
		filelen = M_FileLength(fp);

		fseek(fp, offs, SEEK_SET);
		read = fread(dest, 1, len, fp);
		fclose(fp);
	}
	else filelen = 0;

	return read;
}


VERSION_CONTROL (w_wad_cpp, "$Id$")