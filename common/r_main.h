// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: r_main.h 1856 2010-09-05 03:14:13Z ladna $
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 2006-2014 by The Odamex Team.
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
#include "g_level.h"
#include "r_data.h"
#include "v_palette.h"
#include "m_vectors.h"
#include "v_video.h"

// killough 10/98: special mask indicates sky flat comes from sidedef
#define PL_SKYFLAT (0x80000000)

BOOL R_AlignFlat (int linenum, int side, int fc);

extern int negonearray[MAXWIDTH];
extern int viewheightarray[MAXWIDTH];

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
extern int				centery;

extern fixed_t			centerxfrac;
extern fixed_t			centeryfrac;
extern fixed_t			yaspectmul;

extern shaderef_t		basecolormap;	// [RH] Colormap for sector currently being drawn

extern int				validcount;

extern int				linecount;
extern int				loopcount;

extern fixed_t			render_lerp_amount;

// [SL] Current color blending values (including palette effects)
extern fargb_t blend_color;

void R_SetSectorBlend(const argb_t color);
void R_ClearSectorBlend();
argb_t R_GetSectorBlend();

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


//
// Function pointers to switch refresh/drawing functions.
//
extern void 			(*colfunc) (void);
extern void 			(*spanfunc) (void);
extern void				(*spanslopefunc) (void);


//
// Utility functions.

int R_PointOnSide(fixed_t x, fixed_t y, const node_t* node);
int R_PointOnSide(fixed_t x, fixed_t y, fixed_t xl, fixed_t yl, fixed_t xh, fixed_t yh);
int R_PointOnSegSide(fixed_t x, fixed_t y, const seg_t* line);
bool R_PointOnLine(fixed_t x, fixed_t y, fixed_t xl, fixed_t yl, fixed_t xh, fixed_t yh);

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

int R_ProjectPointX(fixed_t x, fixed_t y);
int R_ProjectPointY(fixed_t z, fixed_t y);
bool R_CheckProjectionX(int &x1, int &x2);
bool R_CheckProjectionY(int &y1, int &y2);

void R_RotatePoint(fixed_t x, fixed_t y, angle_t ang, fixed_t &tx, fixed_t &ty);
bool R_ClipLineToFrustum(const v2fixed_t* v1, const v2fixed_t* v2, fixed_t clipdist, int32_t& lclip, int32_t& rclip);

void R_ClipLine(const v2fixed_t* in1, const v2fixed_t* in2, 
				int32_t lclip, int32_t rclip,
				v2fixed_t* out1, v2fixed_t* out2);
void R_ClipLine(const vertex_t* in1, const vertex_t* in2,
				int32_t lclip, int32_t rclip,
				v2fixed_t* out1, v2fixed_t* out2);

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
void R_Init();

// Called by exit code.
void STACK_ARGS R_Shutdown();

void R_ExitLevel();

// Called by M_Responder.
void R_SetViewSize(int blocks);

class IWindowSurface;
IWindowSurface* R_GetRenderingSurface();

bool R_BorderVisible();
bool R_StatusBarVisible();

int R_ViewWidth(int width, int height);
int R_ViewHeight(int width, int height);
int R_ViewWindowX(int width, int height);
int R_ViewWindowY(int width, int height);


void R_ForceViewWindowResize();

void R_ResetDrawFuncs();
void R_SetFuzzDrawFuncs();
void R_SetLucentDrawFuncs();
void R_SetTranslatedDrawFuncs();
void R_SetTranslatedLucentDrawFuncs();

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
	const palindex_t range_start = 0x70;
	const palindex_t range_stop = 0x7F;

	int pid = translation.getPlayerID();

	// Not a player color translation:
	if (pid == -1)
		return shade(translation.tlate(c));

	// Special effect:
	if (m_mapnum >= NUMCOLORMAPS)
		return shade(translation.tlate(c));

	// Is a player color translation, but not a player color index:
	if (c < range_start || c > range_stop)
		return shade(c);

	// Default to white light:
	argb_t lightcolor = argb_t(255, 255, 255);
	argb_t fadecolor(level.fadeto_color[0], level.fadeto_color[1], level.fadeto_color[2], level.fadeto_color[3]);

	// Use the dynamic lighting's light color if we have one:
	if (m_dyncolormap != NULL)
	{
		lightcolor = m_dyncolormap->color;
		fadecolor = m_dyncolormap->fade;
	}

	// Find the shading for the custom player colors:
	argb_t trancolor = translationRGB[pid][c - range_start];

	unsigned int r = (trancolor.getr() * lightcolor.getr() * (NUMCOLORMAPS - m_mapnum) / 255
					+ fadecolor.getr() * m_mapnum + NUMCOLORMAPS / 2) / NUMCOLORMAPS;
	unsigned int g = (trancolor.getg() * lightcolor.getg() * (NUMCOLORMAPS - m_mapnum) / 255
					+ fadecolor.getg() * m_mapnum + NUMCOLORMAPS / 2) / NUMCOLORMAPS;
	unsigned int b = (trancolor.getb() * lightcolor.getb() * (NUMCOLORMAPS - m_mapnum) / 255
					+ fadecolor.getb() * m_mapnum + NUMCOLORMAPS / 2) / NUMCOLORMAPS;

	return argb_t(gammatable[r], gammatable[g], gammatable[b]);
}


void R_DrawLine(const v3fixed_t* inpt1, const v3fixed_t* inpt2, byte color);

#endif // __R_MAIN_H__
