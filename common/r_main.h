// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: r_main.h 1856 2010-09-05 03:14:13Z ladna $
//
// Copyright (C) 1993-1996 by id Software, Inc.
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
//	System specific interface stuff.
//
//-----------------------------------------------------------------------------


#ifndef __R_MAIN_H__
#define __R_MAIN_H__

#include "d_player.h"
#include "r_data.h"
#include "v_palette.h"

// killough 10/98: special mask indicates sky flat comes from sidedef
#define PL_SKYFLAT (0x80000000)

BOOL R_AlignFlat (int linenum, int side, int fc);

//
// POV related.
//
extern fixed_t			viewcos;
extern fixed_t			viewsin;

extern int				viewwidth;
extern int				viewheight;
extern int				viewwindowx;
extern int				viewwindowy;

extern bool				r_fakingunderwater;
extern bool				r_underwater;

extern int				centerx;
extern "C" int			centery;

extern fixed_t			centerxfrac;
extern fixed_t			centeryfrac;
extern fixed_t			yaspectmul;

extern shaderef_t		basecolormap;	// [RH] Colormap for sector currently being drawn

extern int				validcount;

extern int				linecount;
extern int				loopcount;

//
// Lighting LUT.
// Used for z-depth cuing per column/row,
//	and other lighting effects (sector ambient, flash).
//

// Lighting constants.
// Now why not 32 levels here?
#define LIGHTLEVELS 			16
#define LIGHTSEGSHIFT			 4

#define MAXLIGHTSCALE			48
#define LIGHTSCALEMULBITS		 8	// [RH] for hires lighting fix
#define LIGHTSCALESHIFT 		(12+LIGHTSCALEMULBITS)
#define MAXLIGHTZ			   128
#define LIGHTZSHIFT 			20

// [RH] Changed from shaderef_t* to int.
extern int				scalelight[LIGHTLEVELS][MAXLIGHTSCALE];
extern int				scalelightfixed[MAXLIGHTSCALE];
extern int				zlight[LIGHTLEVELS][MAXLIGHTZ];

extern int				extralight;
extern BOOL				foggy;
extern int				fixedlightlev;
extern shaderef_t		fixedcolormap;

extern int				lightscalexmul;	// [RH] for hires lighting fix
extern int				lightscaleymul;


// Number of diminishing brightness levels.
// There a 0-31, i.e. 32 LUT in the COLORMAP lump.
#define NUMCOLORMAPS			32


// [RH] New detail modes
extern "C" int			detailxshift;
extern "C" int			detailyshift;


//
// Function pointers to switch refresh/drawing functions.
// Used to select shadow mode etc.
//
extern void 			(*colfunc) (void);
extern void 			(*basecolfunc) (void);
extern void 			(*fuzzcolfunc) (void);
extern void				(*lucentcolfunc) (void);
extern void				(*transcolfunc) (void);
extern void				(*tlatedlucentcolfunc) (void);
// No shadow effects on floors.
extern void 			(*spanfunc) (void);
extern void				(*spanslopefunc) (void);

// [RH] Function pointers for the horizontal column drawers.
extern void (*hcolfunc_pre) (void);
extern void (*hcolfunc_post1) (int hx, int sx, int yl, int yh);
extern void (*hcolfunc_post2) (int hx, int sx, int yl, int yh);
extern void (*hcolfunc_post4) (int sx, int yl, int yh);



//
// Utility functions.

int R_PointOnSide(fixed_t x, fixed_t y, const node_t* node);

int R_PointOnSide(fixed_t x, fixed_t y, fixed_t xl, fixed_t yl, fixed_t xh, fixed_t yh);

int R_PointOnSegSide(fixed_t x, fixed_t y, const seg_t* line);

angle_t
R_PointToAngle
( fixed_t	x,
  fixed_t	y );

// 2/1/10: Updated (from EE) to restore vanilla style, with tweak for overflow tolerance
angle_t R_PointToAngle2(fixed_t viewx, fixed_t viewy, fixed_t x, fixed_t y);

fixed_t
R_PointToDist
( fixed_t	x,
  fixed_t	y );


void R_RotatePoint(fixed_t x, fixed_t y, angle_t ang, fixed_t &tx, fixed_t &ty);
void R_ClipEndPoints(
	fixed_t x1, fixed_t y1,
	fixed_t x2, fixed_t y2,
	fixed_t lclip1, fixed_t lclip2,
	fixed_t &cx1, fixed_t &cy1,
	fixed_t &cx2, fixed_t &cy2);

subsector_t*
R_PointInSubsector
( fixed_t	x,
  fixed_t	y );

void
R_AddPointToBox
( int		x,
  int		y,
  fixed_t*	box );

fixed_t R_PointToDist2 (fixed_t dx, fixed_t dy);
void R_SetFOV(float fov, bool force);
float R_GetFOV (void);

#define WIDE_STRETCH 0
#define WIDE_ZOOM 1
#define WIDE_TRUE 2

int R_GetWidescreen(void);

//
// REFRESH - the actual rendering functions.
//

// Called by G_Drawer.
void R_RenderPlayerView (player_t *player);

// Called by startup code.
void R_Init (void);

// Called by exit code.
void STACK_ARGS R_Shutdown (void);

// Called by M_Responder.
void R_SetViewSize (int blocks);

// [RH] Initialize multires stuff for renderer
void R_MultiresInit (void);

void R_ResetDrawFuncs(void);
void R_SetLucentDrawFuncs(void);
void R_SetTranslatedDrawFuncs(void);
void R_SetTranslatedLucentDrawFuncs(void);

inline const byte shaderef_t::ramp() const
{
	if (m_mapnum >= NUMCOLORMAPS)
		return 0;

	int index = clamp(m_mapnum * 256 / NUMCOLORMAPS, 0, 255);
	return m_colors->ramp[index];
}

extern argb_t translationRGB[MAXPLAYERS+1][16];

inline argb_t shaderef_t::tlate(const translationref_t &translation, const byte c) const
{
	int pid = translation.getPlayerID();

	// Not a player color translation:
	if (pid == -1)
		return shade(translation.tlate(c));

	// Special effect:
	if (m_mapnum >= NUMCOLORMAPS)
		return shade(translation.tlate(c));

	// Is a player color translation, but not a player color index:
	if (!(c >= 0x70 && c < 0x80))
		return shade(c);

	// Default to white light:
	argb_t lightcolor = MAKERGB(255, 255, 255);

	// Use the dynamic lighting's light color if we have one:
	if (m_dyncolormap != NULL)
		lightcolor = m_dyncolormap->color;

	// Find the shading for the custom player colors:
	byte a = 255 - ramp();
	argb_t t = translationRGB[pid][c - 0x70];
	argb_t s = MAKERGB(
		RPART(t) * RPART(lightcolor) * a / (255 * 255),
		GPART(t) * GPART(lightcolor) * a / (255 * 255),
		BPART(t) * BPART(lightcolor) * a / (255 * 255)
	);
	return s;
}

#endif // __R_MAIN_H__
