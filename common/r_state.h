// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
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
//	Refresh/render internal state variables (global).
//
//-----------------------------------------------------------------------------


#pragma once

// Need data structure definitions.
#include "d_player.h"
#include "r_data.h"

#define WALLFRACBITS	4
#define WALLFRACUNIT	(1<<WALLFRACBITS)


//
// Refresh internal data structures,
//	for rendering.
//

// needed for texture pegging
extern fixed_t* 		textureheight;

extern "C" int			viewwidth;
extern "C" int			viewheight;

extern int				firstflat;
extern int				numflats;

// for global animation
extern bool*			flatwarp;
extern byte**			warpedflats;
extern int*				flatwarpedwhen;
extern int*				flattranslation;
		
extern int* 			texturetranslation; 	

// Sprite....
extern int				firstspritelump;
extern int				lastspritelump;
extern int				numspritelumps;

//
// Lookup tables for map data.
//
extern bool				g_ValidLevel;

extern int				numsprites;
extern spritedef_t* 	sprites;

extern int				numvertexes;
extern vertex_t*		vertexes;

extern int				numsegs;
extern seg_t*			segs;

extern int				numsectors;
extern sector_t*		sectors;

extern int				numsubsectors;
extern subsector_t* 	subsectors;

extern int				numnodes;
extern node_t*			nodes;

extern int				numlines;
extern line_t*			lines;

extern int				numsides;
extern side_t*			sides;

extern std::vector<int> originalLightLevels;

inline FArchive &operator<< (FArchive &arc, sector_t *sec)
{
	if (sec)
		return arc << (WORD)(sec - sectors);
	else
		return arc << (WORD)0xffff;
}
inline FArchive &operator>> (FArchive &arc, sector_t *&sec)
{
	WORD ofs;
	arc >> ofs;
	if (ofs == 0xffff)
		sec = NULL;
	else
		sec = sectors + ofs;
	return arc;
}

inline FArchive &operator<< (FArchive &arc, line_t *line)
{
	if (line)
		return arc << (WORD)(line - lines);
	else
		return arc << (WORD)0xffff;
}
inline FArchive &operator>> (FArchive &arc, line_t *&line)
{
	WORD ofs;
	arc >> ofs;
	if (ofs == 0xffff)
		line = NULL;
	else
		line = lines + ofs;
	return arc;
}

struct LocalView
{
	angle_t angle;
	bool setangle;
	bool skipangle;
	int pitch;
	bool setpitch;
	bool skippitch;
};

//
// POV data.
//
extern fixed_t			viewx;
extern fixed_t			viewy;
extern fixed_t			viewz;

extern angle_t			viewangle;
extern LocalView		localview;
extern AActor*			camera;		// [RH] camera instead of viewplayer

extern angle_t			clipangle;

//extern fixed_t		finetangent[FINEANGLES/2];

extern visplane_t*		floorplane;
extern visplane_t*		ceilingplane;
extern visplane_t*		skyplane;

// [AM] 4:3 Field of View
extern int				FieldOfView;
// [AM] Corrected (for widescreen) Field of View
extern int				CorrectFieldOfView;
