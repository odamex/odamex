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
//	Here is a core component: drawing the floors and ceilings,
//	while maintaining a per column clipping list only.
//	Moreover, the sky areas have to be determined.
//
//		MAXVISPLANES is no longer a limit on the number of visplanes,
//		but a limit on the number of hash slots; larger numbers mean
//		better performance usually but after a point they are wasted,
//		and memory and time overheads creep in.
//
//													-Lee Killough
//
//-----------------------------------------------------------------------------


#include <stdio.h>
#include <stdlib.h>

#include "i_system.h"
#include "z_zone.h"
#include "w_wad.h"

#include "doomdef.h"
#include "doomstat.h"

#include "p_local.h"
#include "r_local.h"
#include "r_sky.h"

#include "m_alloc.h"
#include "i_video.h"
#include "v_video.h"

#include "m_vectors.h"
#include <math.h>

planefunction_t 		floorfunc;
planefunction_t 		ceilingfunc;

// Here comes the obnoxious "visplane".
#define MAXVISPLANES 128    /* must be a power of 2 */

static const float flatwidth = 64.0f;
static const float flatheight = 64.0f;

static visplane_t		*visplanes[MAXVISPLANES];	// killough
static visplane_t		*freetail;					// killough
static visplane_t		**freehead = &freetail;		// killough

visplane_t 				*floorplane;
visplane_t 				*ceilingplane;
visplane_t				*skyplane;

// killough -- hash function for visplanes
// Empirically verified to be fairly uniform:

#define visplane_hash(picnum,lightlevel,secplane) \
  ((unsigned)((picnum)*3+(lightlevel)+(secplane.d)*7) & (MAXVISPLANES-1))

//
// Clip values are the solid pixel bounding the range.
//	floorclip starts out SCREENHEIGHT-1
//	ceilingclip starts out 0
//
int     				*floorclip;
int 					*ceilingclip;
int						*floorclipinitial;
int						*ceilingclipinitial;

//
// spanstart holds the start of a plane span
// initialized to 0 at start
//
int 					*spanstart;

//
// texture mapping
//
extern fixed_t FocalLengthX, FocalLengthY;
extern float xfoc, yfoc;
extern float focratio, ifocratio;

int*					planezlight;
float					plight, shade;

fixed_t 				*yslope;
static fixed_t			planeheight;

static fixed_t			pl_xscale, pl_yscale;
static fixed_t			pl_viewsin, pl_viewcos;
static fixed_t			pl_viewxtrans, pl_viewytrans;
static fixed_t			pl_xstepscale, pl_ystepscale;

v3float_t				a, b, c;
float					ixscale, iyscale;

//
// R_InitPlanes
// Only at game startup.
//
void R_InitPlanes (void)
{
	// Doh!
}

//
// R_MapSlopedPlane
//
// Calculates the vectors a, b, & c, which are used to texture map a sloped
// plane.
//
// Based in part on R_MapSlope() and R_SlopeLights() from Eternity Engine,
// written by SoM/Quasar
//
void R_MapSlopedPlane(int y, int x1, int x2)
{
	int len = x2 - x1 + 1;
	if (len <= 0)
		return;

	// center of the view plane
	v3float_t s;
	s.x = x1 - centerx;
	s.y = y - centery + 1.0f;
	s.z = xfoc; 

	dspan.iu = M_DotProductVec3f(&s, &a) * flatwidth;
	dspan.iv = M_DotProductVec3f(&s, &b) * flatheight;
	dspan.id = M_DotProductVec3f(&s, &c);
	
	dspan.iustep = a.x * flatwidth;
	dspan.ivstep = b.x * flatheight;
	dspan.idstep = c.x;

	// From R_SlopeLights, Eternity Engine
	float id = dspan.id + dspan.idstep * (x2 - x1);
	float map1 = 256.0f - (shade - plight * dspan.id);
	float map2 = 256.0f - (shade - plight * id);

	if (fixedlightlev)
	{
		for (int i = 0; i < len; i++)
			dspan.slopelighting[i] = basecolormap.with(fixedlightlev);
	}
	else if (fixedcolormap.isValid())
	{
		for (int i = 0; i < len; i++)
			dspan.slopelighting[i] = fixedcolormap;
	}
	else
	{
		fixed_t mapstart = FLOAT2FIXED((256.0f - map1) / 256.0f * NUMCOLORMAPS);
		fixed_t mapend = FLOAT2FIXED((256.0f - map2) / 256.0f * NUMCOLORMAPS);
		fixed_t map = mapstart;
		fixed_t step = 0;

		step = (mapend - mapstart) / len;

		for (int i = 0; i < len; i++)
		{
			int index = (int)(map >> FRACBITS) + 1;
			index -= (foggy ? 0 : extralight << 2);
			
			if (index < 0)
				dspan.slopelighting[i] = basecolormap;
			else if (index >= NUMCOLORMAPS)
				dspan.slopelighting[i] = basecolormap.with((NUMCOLORMAPS - 1));
			else
				dspan.slopelighting[i] = basecolormap.with(index);
			
			map += step;
		}
	}

   	dspan.y = y;
	dspan.x1 = x1;
	dspan.x2 = x2;

	spanslopefunc();
}


//
// R_MapLevelPlane
//
// [SL] 2012-11-09 - Based loosely on R_MapPlane() from PrBoom+ to increase
// the accuracy of texture-mapping visplanes with the same textures.
//
// e6y
//
// [RH]Instead of using the xtoviewangle array, I calculated the fractional values
// at the middle of the screen, then used the calculated ds_xstep and ds_ystep
// to step from those to the proper texture coordinate to start drawing at.
// That way, the texture coordinate is always calculated by its position
// on the screen and not by its position relative to the edge of the visplane.
//
// Visplanes with the same texture now match up far better than before.
//
void R_MapLevelPlane(int y, int x1, int x2)
{
	fixed_t distance = FixedMul(planeheight, yslope[y]);
	fixed_t slope = (fixed_t)(focratio * FixedDiv(planeheight, abs(centery - y) << FRACBITS));

	dspan.xstep = FixedMul(pl_xstepscale, slope);
	dspan.ystep = FixedMul(pl_ystepscale, slope);

	dspan.xfrac = pl_viewxtrans +
				FixedMul(FixedMul(pl_viewcos, distance), pl_xscale) + 
				(x1 - centerx) * dspan.xstep;
	dspan.yfrac = pl_viewytrans -
				FixedMul(FixedMul(pl_viewsin, distance), pl_yscale) +
				(x1 - centerx) * dspan.ystep;

	if (fixedlightlev)
		dspan.colormap = basecolormap.with(fixedlightlev);
	else if (fixedcolormap.isValid())
		dspan.colormap = fixedcolormap;
	else
	{
		// Determine lighting based on the span's distance from the viewer.
		unsigned int index = distance >> LIGHTZSHIFT;

		if (index >= MAXLIGHTZ)
			index = MAXLIGHTZ-1;

		dspan.colormap = basecolormap.with(planezlight[index]);
	}

	dspan.y = y;
	dspan.x1 = x1;
	dspan.x2 = x2;

	spanfunc();
}

//
// R_ClearPlanes
// At begining of frame.
//
void R_ClearPlanes (void)
{
	// opening / clipping determination
	memcpy(floorclip, floorclipinitial, viewwidth * sizeof(*floorclip));
	memcpy(ceilingclip, ceilingclipinitial, viewwidth * sizeof(*ceilingclip));

	for (int i = 0; i < MAXVISPLANES; i++)	// new code -- killough
		for (*freehead = visplanes[i], visplanes[i] = NULL; *freehead; )
			freehead = &(*freehead)->next;
}

//
// New function, by Lee Killough
// [RH] top and bottom buffers get allocated immediately
//		after the visplane.
//
static visplane_t *new_visplane(unsigned hash)
{
	visplane_t *check = freetail;

	if (!check)
	{
		check = (visplane_t *)Calloc(1, sizeof(*check) + sizeof(*check->top)*2*I_GetSurfaceWidth());
		check->bottom = &check->top[I_GetSurfaceWidth() + 2];
	}
	else
		if (!(freetail = freetail->next))
			freehead = &freetail;
	check->next = visplanes[hash];
	visplanes[hash] = check;
	return check;
}


//
// R_FindPlane
//
// killough 2/28/98: Add offsets
//
visplane_t *R_FindPlane (plane_t secplane, int picnum, int lightlevel,
						 fixed_t xoffs, fixed_t yoffs,
						 fixed_t xscale, fixed_t yscale, angle_t angle)
{
	visplane_t *check;
	unsigned hash;						// killough

	if (picnum == skyflatnum || picnum & PL_SKYFLAT)  // killough 10/98
		lightlevel = 0;		// most skies map together

	// New visplane algorithm uses hash table -- killough
	hash = visplane_hash (picnum, lightlevel, secplane);

	for (check = visplanes[hash]; check; check = check->next)	// killough
		if (P_IdenticalPlanes(&secplane, &check->secplane) &&
			picnum == check->picnum &&
			lightlevel == check->lightlevel &&
			xoffs == check->xoffs &&	// killough 2/28/98: Add offset checks
			yoffs == check->yoffs &&
			basecolormap == check->colormap &&	// [RH] Add colormap check
			xscale == check->xscale &&
			yscale == check->yscale &&
			angle == check->angle
			)
		  return check;

	check = new_visplane (hash);		// killough

	memcpy(&check->secplane, &secplane, sizeof(secplane));
	check->picnum = picnum;
	check->lightlevel = lightlevel;
	check->xoffs = xoffs;				// killough 2/28/98: Save offsets
	check->yoffs = yoffs;
	check->xscale = xscale;
	check->yscale = yscale;
	check->angle = angle;
	check->colormap = basecolormap;		// [RH] Save colormap
	check->minx = viewwidth;			// Was SCREENWIDTH -- killough 11/98
	check->maxx = -1;

	memcpy(check->top, viewheightarray, viewwidth * sizeof(*check->top));

	return check;
}

//
// R_CheckPlane
//
visplane_t* R_CheckPlane(visplane_t* pl, int start, int stop)
{
    int		intrl;
    int		intrh;
    int		unionl;
    int		unionh;
    int		x;

	if (start < pl->minx)
	{
		intrl = pl->minx;
		unionl = start;
	}
	else
	{
		unionl = pl->minx;
		intrl = start;
	}

	if (stop > pl->maxx)
	{
		intrh = pl->maxx;
		unionh = stop;
	}
	else
	{
		unionh = pl->maxx;
		intrh = stop;
	}

	for (x = intrl ; x <= intrh && pl->top[x] == (unsigned int)viewheight; x++)
		;

	if (x > intrh)
	{
		// use the same visplane
		pl->minx = unionl;
		pl->maxx = unionh;
	}
	else
	{
		// make a new visplane
		unsigned hash = visplane_hash (pl->picnum, pl->lightlevel, pl->secplane);
		visplane_t *new_pl = new_visplane (hash);

		new_pl->secplane = pl->secplane;
		new_pl->picnum = pl->picnum;
		new_pl->lightlevel = pl->lightlevel;
		new_pl->xoffs = pl->xoffs;			// killough 2/28/98
		new_pl->yoffs = pl->yoffs;
		new_pl->xscale = pl->xscale;
		new_pl->yscale = pl->yscale;
		new_pl->angle = pl->angle;
		new_pl->colormap = pl->colormap;	// [RH] Copy colormap
		pl = new_pl;
		pl->minx = start;
		pl->maxx = stop;
		memcpy(pl->top, viewheightarray, viewwidth * sizeof(*pl->top));
	}
	return pl;
}

//
// R_MakeSpans
//
void R_MakeSpans(visplane_t *pl, void(*spanfunc)(int, int, int))
{
	for (int x = pl->minx; x <= pl->maxx + 1; x++)
	{
		unsigned int t1 = pl->top[x-1];
		unsigned int b1 = pl->bottom[x-1];
		unsigned int t2 = pl->top[x];
		unsigned int b2 = pl->bottom[x];
		
		for (; t1 < t2 && t1 <= b1; t1++)
			spanfunc(t1, spanstart[t1], x-1);
		for (; b1 > b2 && b1 >= t1; b1--)
			spanfunc(b1, spanstart[b1], x-1);
		while (t2 < t1 && t2 <= b2)
			spanstart[t2++] = x;
		while (b2 > b1 && b2 >= t2)
			spanstart[b2--] = x;
	}
}

//
// R_DrawSlopedPlane
//
// Calculates the vectors a, b, & c, which are used to texture map a sloped
// plane.
//
// Based in part on R_CalcSlope() from Eternity Engine, written by SoM.
//
void R_DrawSlopedPlane(visplane_t *pl)
{
	const float xoffsf = FIXED2FLOAT(pl->xoffs);
	const float yoffsf = FIXED2FLOAT(pl->yoffs);
	const float scaledflatwidth = flatwidth * FIXED2FLOAT(pl->xscale);
	const float scaledflatheight = flatheight * FIXED2FLOAT(pl->yscale);

	v3float_t p, t, s, viewpos;
	M_SetVec3f(&viewpos, viewx, viewy, viewz);

	// [SL] optimize when the texture rotation angle is zero (most of the time)
	// This eliminates several multiplies and improves visibile accuracy as the
	// finesine/cosine tables do not have an exact value for angle 0
	if (pl->angle == 0)
	{
		// Point p is the anchor point of the texture.  It starts out as the
		// map coordinate (0, 0, planez(0,0)) but texture offset gets applied
		p.x = -xoffsf;
		p.z = yoffsf;
		p.y = P_PlaneZ(p.x, p.z, &pl->secplane);

		// Point t is the point along the plane (texwidth, 0, planez(texwidth, 0)) with texture
		// offset applied
		t.x = p.x - scaledflatwidth;
		t.z = p.z;
		t.y = P_PlaneZ(t.x, t.z, &pl->secplane);

		// Point s is the point along the plane (0, texheight, planez(0, texheight)) with texture
		// offset applied
		s.x = p.x;
		s.z = p.z + scaledflatheight;
		s.y = P_PlaneZ(s.x, s.z, &pl->secplane);
	}
	else
	{
		float sinang = FIXED2FLOAT(finesine[(pl->angle + ANG90) >> ANGLETOFINESHIFT]);
		float cosang = FIXED2FLOAT(finecosine[(pl->angle + ANG90) >> ANGLETOFINESHIFT]);

		// Point p is the anchor point of the texture.  It starts out as the
		// map coordinate (0, 0, planez(0,0)) but texture offset and rotation get applied
		p.x = -yoffsf * cosang - xoffsf * sinang;
		p.z = -xoffsf * cosang + yoffsf * sinang;
		p.y = P_PlaneZ(p.x, p.z, &pl->secplane);

		// Point t is the point along the plane (texwidth, 0, planez(texwidth, 0)) with texture
		// offset and rotation applied
		t.x = p.x - scaledflatwidth * sinang;
		t.z = p.z + scaledflatwidth * cosang;
		t.y = P_PlaneZ(t.x, t.z, &pl->secplane);

		// Point s is the point along the plane (0, texheight, planez(0, texheight)) with texture
		// offset and rotation applied
		s.x = p.x + scaledflatheight * cosang;
		s.z = p.z + scaledflatheight * sinang;
		s.y = P_PlaneZ(s.x, s.z, &pl->secplane);
	}

	// Translate the points to their position relative to viewx, viewy and
	// rotate them based on viewangle
	angle_t rotation = (angle_t)(-(int)viewangle + ANG90);
	M_TranslateVec3f(&p, &viewpos, rotation);
	M_TranslateVec3f(&t, &viewpos, rotation);
	M_TranslateVec3f(&s, &viewpos, rotation);
	
	// Subtract p from t and s, making t and s into direction vectors
	M_SubVec3f(&t, &t, &p);
	M_SubVec3f(&s, &s, &p);
	
	M_CrossProductVec3f(&a, &p, &s);
	M_CrossProductVec3f(&b, &t, &p);
	M_CrossProductVec3f(&c, &t, &s);

	M_ScaleVec3f(&a, &a, 0.5f);
	M_ScaleVec3f(&b, &b, 0.5f);
	M_ScaleVec3f(&c, &c, 0.5f);

	a.y *= ifocratio;
	b.y *= ifocratio;
	c.y *= ifocratio;		
	
	// (SoM) More help from randy. I was totally lost on this... 
	float scalenumer = FIXED2FLOAT(finetangent[FINEANGLES/4+CorrectFieldOfView/2]);
	float ixscale = scalenumer / flatwidth;
	float iyscale = scalenumer / flatheight;

	float zat = P_PlaneZ(viewpos.x, viewpos.y, &pl->secplane);

	angle_t fovang = ANG(consoleplayer().fov / 2.0f);
	float slopetan = FIXED2FLOAT(finetangent[fovang >> ANGLETOFINESHIFT]);
	float slopevis = 8.0 * slopetan * 16.0 * 320.0 / float(I_GetSurfaceWidth());
	
	plight = (slopevis * ixscale * iyscale) / (zat - viewpos.z);
	shade = 256.0 * 2.0 - (pl->lightlevel + 16.0) * 256.0 / 128.0;

	basecolormap = pl->colormap;	// [RH] set basecolormap
   
	R_MakeSpans(pl, R_MapSlopedPlane);
}

void R_DrawLevelPlane(visplane_t *pl)
{
	// viewx/viewy rotated by the texture rotation angle
	fixed_t pl_viewx, pl_viewy;

	// texture scaling factor
	pl_xscale = pl->xscale << 10;
	pl_yscale = pl->yscale << 10;

	// viewsin/viewcos rotated by the texture rotation angle
	pl_viewsin = finesine[(viewangle + pl->angle) >> ANGLETOFINESHIFT];
	pl_viewcos = finecosine[(viewangle + pl->angle) >> ANGLETOFINESHIFT];

	// [SL] If the texture isn't rotated, we can optimize out a few multiplies
	// and avoid using the finesine/cosine tables since they do not have exact
	// values for angle 0. This helps the texture to line up correctly.
	if (pl->angle == 0)
	{
		pl_viewx = viewx;
		pl_viewy = -viewy;
	}
	else
	{
		const fixed_t pl_cos = finecosine[pl->angle >> ANGLETOFINESHIFT];
		const fixed_t pl_sin = finesine[pl->angle >> ANGLETOFINESHIFT];
		
		pl_viewx = FixedMul(viewx, pl_cos) - FixedMul(viewy, pl_sin);
		pl_viewy = -(FixedMul(viewx, pl_sin) + FixedMul(viewy, pl_cos));
	}

	// cache a calculation used by R_MapLevelPlane
	pl_xstepscale = FixedMul(pl_viewsin, pl->xscale) << 10;
	pl_ystepscale = FixedMul(pl_viewcos, pl->yscale) << 10;

	// cache a calculation used by R_MapLevelPlane
	pl_viewxtrans = FixedMul(pl_viewx + pl->xoffs, pl->xscale) << 10;
	pl_viewytrans = FixedMul(pl_viewy + pl->yoffs, pl->yscale) << 10;
	
	basecolormap = pl->colormap;	// [RH] set basecolormap

	// [SL] 2012-02-05 - Plane's height should be constant for all (x,y)
	// so just use (0, 0) when calculating the plane's z height
	planeheight = abs(P_PlaneZ(0, 0, &pl->secplane) - viewz);

	int light = clamp((pl->lightlevel >> LIGHTSEGSHIFT) + (foggy ? 0 : extralight), 0, LIGHTLEVELS - 1);
	planezlight = zlight[light];

	R_MakeSpans(pl, R_MapLevelPlane);
}


//
// R_DrawPlanes
//
// At the end of each frame.
//
void R_DrawPlanes (void)
{
	visplane_t *pl;
	int i;

	R_ResetDrawFuncs();

	dspan.color = 3;
	
	for (i = 0; i < MAXVISPLANES; i++)
	{
		for (pl = visplanes[i]; pl; pl = pl->next)
		{
			if (pl->minx > pl->maxx)
				continue;

			// sky flat
			if (pl->picnum == skyflatnum || pl->picnum & PL_SKYFLAT)
			{
				R_RenderSkyRange(pl);
			}
			else
			{
				// regular flat
				int useflatnum = flattranslation[pl->picnum < numflats ? pl->picnum : 0];

				dspan.color += 4;	// [RH] color if r_drawflat is 1
				dspan.source = (byte *)wads.CacheLumpNum (firstflat + useflatnum, PU_STATIC);
										   
				// [RH] warp a flat if desired
				if (flatwarp[useflatnum])
				{
					if (warpedflats[useflatnum] && flatwarpedwhen[useflatnum] == level.time)
					{
						Z_ChangeTag(dspan.source, PU_CACHE);
						dspan.source = warpedflats[useflatnum];
						Z_ChangeTag(dspan.source, PU_STATIC);
					}
					else
					{
						if (!warpedflats[useflatnum])
							warpedflats[useflatnum] = (byte*)Z_Malloc(64*64, PU_STATIC, &warpedflats[useflatnum]);

						static byte buffer[64];
						int timebase = level.time*23;

						flatwarpedwhen[useflatnum] = level.time;
						byte *warped = warpedflats[useflatnum];

						for (int x = 63; x >= 0; x--)
						{
							int yt, yf = (finesine[(timebase + ((x+17) << 7))&FINEMASK]>>13) & 63;
							byte *source = dspan.source + x;
							byte *dest = warped + x;
							for (yt = 64; yt; yt--, yf = (yf+1)&63, dest += 64)
								*dest = *(source + (yf << 6));
						}
						timebase = level.time*32;
						for (int y = 63; y >= 0; y--)
						{
							int xt, xf = (finesine[(timebase + (y << 7))&FINEMASK]>>13) & 63;
							byte *source = warped + (y << 6);
							byte *dest = buffer;
							for (xt = 64; xt; xt--, xf = (xf+1) & 63)
								*dest++ = *(source+xf);
							memcpy (warped + (y << 6), buffer, 64);
						}
						Z_ChangeTag (dspan.source, PU_CACHE);
						dspan.source = warped;
					}
				}
				
				pl->top[pl->maxx+1] = viewheight;
				pl->top[pl->minx-1] = viewheight;

				if (P_IsPlaneLevel(&pl->secplane))
					R_DrawLevelPlane(pl);
				else
					R_DrawSlopedPlane(pl);
					
				Z_ChangeTag (dspan.source, PU_CACHE);
			}
		}
	}
}

//
// R_PlaneInitData
//
bool R_PlaneInitData(IWindowSurface* surface)
{
	int surface_width = surface->getWidth();
	int surface_height = surface->getHeight();

	delete[] floorclip;
	delete[] ceilingclip;
	delete[] floorclipinitial;
	delete[] ceilingclipinitial;
	delete[] spanstart;
	delete[] yslope;

	floorclip = new int[surface_width];
	ceilingclip = new int[surface_width];
	floorclipinitial = new int[surface_width];
	ceilingclipinitial = new int[surface_width];

	for (int i = 0; i < surface_width; i++)
	{
		ceilingclipinitial[i] = -1;
		floorclipinitial[i] = viewheight;
	}

	spanstart = new int[surface_height];
	yslope = new fixed_t[surface_height];

	// Free all visplanes and let them be re-allocated as needed.
	visplane_t* pl = freetail;

	while (pl)
	{
		visplane_t *next = pl->next;
		M_Free(pl);
		pl = next;
	}
	freetail = NULL;
	freehead = &freetail;

	for (int i = 0; i < MAXVISPLANES; i++)
	{
		pl = visplanes[i];
		visplanes[i] = NULL;
		while (pl)
		{
			visplane_t *next = pl->next;
			M_Free(pl);
			pl = next;
		}
	}

	return true;
}

//
// R_AlignFlat
//
BOOL R_AlignFlat (int linenum, int side, int fc)
{
	line_t *line = lines + linenum;
	sector_t *sec = side ? line->backsector : line->frontsector;

	if (!sec)
		return false;

	fixed_t x = line->v1->x;
	fixed_t y = line->v1->y;

	angle_t angle = R_PointToAngle2 (x, y, line->v2->x, line->v2->y);
	angle_t norm = (angle-ANG90) >> ANGLETOFINESHIFT;

	fixed_t dist = -FixedMul (finecosine[norm], x) - FixedMul (finesine[norm], y);

	if (side)
	{
		angle = angle + ANG180;
		dist = -dist;
	}

	if (fc)
	{
		sec->base_ceiling_angle = 0-angle;
		sec->base_ceiling_yoffs = dist & ((1<<(FRACBITS+8))-1);
	}
	else
	{
		sec->base_floor_angle = 0-angle;
		sec->base_floor_yoffs = dist & ((1<<(FRACBITS+8))-1);
	}

	return true;
}

VERSION_CONTROL (r_plane_cpp, "$Id$")

