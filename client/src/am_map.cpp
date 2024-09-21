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
//  AutoMap module.
//
//-----------------------------------------------------------------------------

#include "odamex.h"

#include "c_bind.h"
#include "p_lnspec.h"
#include "p_local.h"
#include "st_stuff.h"
#include "v_palette.h"
#include "w_wad.h"
#include "z_zone.h"

#include "c_dispatch.h"
#include "cl_demo.h"
#include "g_gametype.h"
#include "m_cheat.h"

// Needs access to LFB.
#include "i_video.h"
#include "v_video.h"

#include "v_text.h"

// State.
#include "r_state.h"

// Data.
#include "gstrings.h"

#include "am_map.h"

#include "gi.h"
#include "g_skill.h"
#include "p_mapformat.h"

argb_t CL_GetPlayerColor(player_t*);

EXTERN_CVAR(am_followplayer)

static int lockglow = 0;

EXTERN_CVAR(am_rotate)
EXTERN_CVAR(am_overlay)
EXTERN_CVAR(am_showsecrets)
EXTERN_CVAR(am_showmonsters)
EXTERN_CVAR(am_showitems)
EXTERN_CVAR(am_showtime)
EXTERN_CVAR(am_classicmapstring)
EXTERN_CVAR(am_usecustomcolors)
EXTERN_CVAR(am_ovshare)

EXTERN_CVAR(am_backcolor)
EXTERN_CVAR(am_yourcolor)
EXTERN_CVAR(am_wallcolor)
EXTERN_CVAR(am_tswallcolor)
EXTERN_CVAR(am_fdwallcolor)
EXTERN_CVAR(am_cdwallcolor)
EXTERN_CVAR(am_thingcolor)
EXTERN_CVAR(am_thingcolor_item)
EXTERN_CVAR(am_thingcolor_countitem)
EXTERN_CVAR(am_thingcolor_monster)
EXTERN_CVAR(am_thingcolor_nocountmonster)
EXTERN_CVAR(am_thingcolor_friend)
EXTERN_CVAR(am_thingcolor_projectile)
EXTERN_CVAR(am_gridcolor)
EXTERN_CVAR(am_xhaircolor)
EXTERN_CVAR(am_notseencolor)
EXTERN_CVAR(am_lockedcolor)
EXTERN_CVAR(am_exitcolor)
EXTERN_CVAR(am_teleportcolor)

EXTERN_CVAR(am_ovyourcolor)
EXTERN_CVAR(am_ovwallcolor)
EXTERN_CVAR(am_ovtswallcolor)
EXTERN_CVAR(am_ovfdwallcolor)
EXTERN_CVAR(am_ovcdwallcolor)
EXTERN_CVAR(am_ovthingcolor)
EXTERN_CVAR(am_ovthingcolor_item)
EXTERN_CVAR(am_ovthingcolor_countitem)
EXTERN_CVAR(am_ovthingcolor_monster)
EXTERN_CVAR(am_ovthingcolor_nocountmonster)
EXTERN_CVAR(am_ovthingcolor_friend)
EXTERN_CVAR(am_ovthingcolor_projectile)
EXTERN_CVAR(am_ovgridcolor)
EXTERN_CVAR(am_ovxhaircolor)
EXTERN_CVAR(am_ovnotseencolor)
EXTERN_CVAR(am_ovlockedcolor)
EXTERN_CVAR(am_ovexitcolor)
EXTERN_CVAR(am_ovteleportcolor)

BEGIN_COMMAND(resetcustomcolors)
{
	am_backcolor = "00 00 3a";
	am_yourcolor = "fc e8 d8";
	am_wallcolor = "00 8b ff";
	am_tswallcolor = "10 32 7e";
	am_fdwallcolor = "1a 1a 8a";
	am_cdwallcolor = "00 00 5a";
	am_thingcolor = "9f d3 ff";
	am_gridcolor = "44 44 88";
	am_xhaircolor = "80 80 80";
	am_notseencolor = "00 22 6e";
	am_lockedcolor = "bb bb bb";
	am_exitcolor = "ff ff 00";
	am_teleportcolor = "ff a3 00";

	am_ovyourcolor = "fc e8 d8";
	am_ovwallcolor = "00 8b ff";
	am_ovtswallcolor = "10 32 7e";
	am_ovfdwallcolor = "1a 1a 8a";
	am_ovcdwallcolor = "00 00 5a";
	am_ovthingcolor = "9f d3 ff";
	am_ovgridcolor = "44 44 88";
	am_ovxhaircolor = "80 80 80";
	am_ovnotseencolor = "00 22 6e";
	am_ovlockedcolor = "bb bb bb";
	am_ovexitcolor = "ff ff 00";
	am_ovteleportcolor = "ff a3 00";
	Printf(PRINT_HIGH, "Custom automap colors reset to default.\n");
}
END_COMMAND(resetcustomcolors)

EXTERN_CVAR(screenblocks)

// drawing stuff
#define AM_NUMMARKPOINTS 10

// scale on entry
#define INITSCALEMTOF (.2 * FRACUNIT)
// how much the automap moves window per tic in frame-buffer coordinates
// moves 140 pixels in 1 second
#define F_PANINC (140 / TICRATE)
// how much zoom-in per tic
// goes to 2x in 1 second
#define M_ZOOMIN ((int)(1.02 * FRACUNIT))
// how much zoom-out per tic
// pulls out to 0.5x in 1 second
#define M_ZOOMOUT ((int)(FRACUNIT / 1.02))

// translates between frame-buffer and map distances
#define FTOM(x) FixedMul(((x) << 16), scale_ftom)
#define MTOF(x) (FixedMul((x), scale_mtof) >> 16)

#define PUTDOTP(xx, yy, cc) fb[(yy)*f_p + (xx)] = (cc)
#define PUTDOTD(xx, yy, cc) *((argb_t*)(fb + (yy)*f_p + ((xx) << 2))) = (cc)

typedef v2int_t fpoint_t;

typedef struct
{
	fpoint_t a, b;
} fline_t;

typedef struct
{
	fixed_t slp, islp;
} islope_t;

// vector graphics for the automap for things.
std::vector<mline_t> thintriangle_guy;
std::vector<mline_t> thinrectangle_guy;

am_default_colors_t AutomapDefaultColors;
am_colors_t AutomapDefaultCurrentColors;
int am_cheating = 0;
static bool grid = false;
static bool bigstate = false; // Bigmode

static bool leveljuststarted = true; // kluge until AM_LevelInit() is called

bool automapactive = false;

// location of window on screen
static v2int_t f;

// size of window on screen
static int f_w;
static int f_h;
static int f_p; // [RH] # of bytes from start of a line to start of next

static byte* fb; // pseudo-frame buffer
static int amclock;

static mpoint_t m_paninc;    // how far the window pans each tic (map coords)
static fixed_t ftom_zoommul; // how far the window zooms in each tic (fb coords)

static v2fixed_t m_ll;   // LL x,y where the window is on the map (map coords)
static v2fixed_t m_ur; // UR x,y where the window is on the map (map coords)

//
// width/height of window on map (map coords)
//
static v2fixed_t m_wh;

// based on level size
static v2fixed_t min;
static v2fixed_t max;

static fixed_t min_scale_mtof; // used to tell when to stop zooming out
static fixed_t max_scale_mtof; // used to tell when to stop zooming in

// old stuff for recovery later
static v2fixed_t old_m_wh;
static v2fixed_t old_m_ll;

// old location used by the Follower routine
static mpoint_t f_oldloc;

// used by MTOF to scale from map-to-frame-buffer coords
static fixed_t scale_mtof = static_cast<fixed_t>(INITSCALEMTOF);
// used by FTOM to scale from frame-buffer-to-map coords (=1/scale_mtof)
static fixed_t scale_ftom;

static lumpHandle_t marknums[10];             // numbers used for marking by the automap
static mpoint_t markpoints[AM_NUMMARKPOINTS]; // where the points are
static int markpointnum = 0;                  // next point to be assigned

static bool stopped = true;

extern NetDemo netdemo;

void AM_clearMarks();
void AM_addMark();
void AM_saveScaleAndLoc();
void AM_restoreScaleAndLoc();
void AM_minOutWindowScale();

#define NUMALIASES 3
#define WALLCOLORS -1
#define FDWALLCOLORS -2
#define CDWALLCOLORS -3

#define WEIGHTBITS 6
#define WEIGHTSHIFT (FRACBITS - WEIGHTBITS)
#define NUMWEIGHTS (1 << WEIGHTBITS)
#define WEIGHTMASK (NUMWEIGHTS - 1)

BEGIN_COMMAND(am_grid)
{
	grid = !grid;
	Printf(PRINT_HIGH, "%s\n", grid ? GStrings(AMSTR_GRIDON) : GStrings(AMSTR_GRIDOFF));
}
END_COMMAND(am_grid)

BEGIN_COMMAND(am_setmark)
{
	AM_addMark();
	Printf(PRINT_HIGH, "%s %d\n", GStrings(AMSTR_MARKEDSPOT), markpointnum);
}
END_COMMAND(am_setmark)

BEGIN_COMMAND(am_clearmarks)
{
	AM_clearMarks();
	Printf(PRINT_HIGH, "%s\n", GStrings(AMSTR_MARKSCLEARED));
}
END_COMMAND(am_clearmarks)

BEGIN_COMMAND(am_big)
{
	bigstate = !bigstate;
	if (bigstate)
	{
		AM_saveScaleAndLoc();
		AM_minOutWindowScale();
	}
	else
		AM_restoreScaleAndLoc();
}
END_COMMAND(am_big)

BEGIN_COMMAND(am_togglefollow)
{
	am_followplayer = !am_followplayer;
	f_oldloc.x = MAXINT;
	Printf(PRINT_HIGH, "%s\n",
	       am_followplayer ? GStrings(AMSTR_FOLLOWON) : GStrings(AMSTR_FOLLOWOFF));
}
END_COMMAND(am_togglefollow)

void AM_rotatePoint(mpoint_t& pt);

// translates between frame-buffer and map coordinates
int CXMTOF(int x)
{
	return (MTOF((x)-m_ll.x) /* - f_x*/);
}

int CYMTOF(int y)
{
	return (f_h - MTOF((y)-m_ll.y) /* + f_y*/);
}

bool AM_ClassicAutomapVisible()
{
	return automapactive && !viewactive;
}

bool AM_OverlayAutomapVisible()
{
	return automapactive && viewactive;
}

//
//
//
void AM_activateNewScale()
{
	m_ll.x += m_wh.x / 2;
	m_ll.y += m_wh.y / 2;
	M_SetVec2Fixed(&m_wh, FTOM(f_w), FTOM(f_h));
	m_ll.x -= m_wh.x / 2;
	m_ll.y -= m_wh.y / 2;
	M_AddVec2Fixed(&m_ur, &m_ll, &m_wh);
}

//
//
//
void AM_saveScaleAndLoc()
{
	old_m_ll = m_ll;
	old_m_wh = m_wh;
}

//
//
//
void AM_restoreScaleAndLoc()
{
	m_wh = old_m_wh;
	if (!am_followplayer)
	{
		m_ll = old_m_ll;
	}
	else
	{
		M_SetVec2Fixed(&m_ll, displayplayer().camera->x - m_wh.x / 2,
			                  displayplayer().camera->y - m_wh.y / 2);
	}
	M_AddVec2Fixed(&m_ur, &m_ll, &m_wh);

	// Change the scaling multipliers
	scale_mtof = FixedDiv(f_w << FRACBITS, m_wh.x);
	scale_ftom = FixedDiv(FRACUNIT, scale_mtof);
}

//
// adds a marker at the current location
//
void AM_addMark()
{
	markpoints[markpointnum].x = m_ll.x + m_wh.x / 2;
	markpoints[markpointnum].y = m_ll.y + m_wh.y / 2;
	markpointnum = (markpointnum + 1) % AM_NUMMARKPOINTS;
}

//
// Determines bounding box of all vertices,
// sets global variables controlling zoom range.
//
void AM_findMinMaxBoundaries()
{
	M_SetVec2Fixed(&min, MAXINT, MAXINT);
	M_SetVec2Fixed(&max, -MAXINT, -MAXINT);

	for (int i = 0; i < numvertexes; i++)
	{
		if (vertexes[i].x < min.x)
			min.x = vertexes[i].x;
		else if (vertexes[i].x > max.x)
			max.x = vertexes[i].x;

		if (vertexes[i].y < min.y)
			min.y = vertexes[i].y;
		else if (vertexes[i].y > max.y)
			max.y = vertexes[i].y;
	}

	const fixed_t max_w = max.x - min.x;
	const fixed_t max_h = max.y - min.y;

	const fixed_t a = FixedDiv((I_GetSurfaceWidth()) << FRACBITS, max_w);
	const fixed_t b = FixedDiv((I_GetSurfaceHeight()) << FRACBITS, max_h);

	min_scale_mtof = a < b ? a : b;
	max_scale_mtof = FixedDiv((I_GetSurfaceHeight()) << FRACBITS, 2 * PLAYERRADIUS);
}

//
//
//
void AM_changeWindowLoc()
{
	if (m_paninc.x || m_paninc.y)
	{
		am_followplayer.Set(0.0f);
		f_oldloc.x = MAXINT;
	}

	M_AddVec2Fixed(&m_ll, &m_paninc, &m_ll);

	if (m_ll.x + m_wh.x / 2 > max.x)
		m_ll.x = max.x - m_wh.x / 2;
	else if (m_ll.x + m_wh.x / 2 < min.x)
		m_ll.x = min.x - m_wh.x / 2;

	if (m_ll.y + m_wh.y / 2 > max.y)
		m_ll.y = max.y - m_wh.y / 2;
	else if (m_ll.y + m_wh.y / 2 < min.y)
		m_ll.y = min.y - m_wh.y / 2;

	M_AddVec2Fixed(&m_ur, &m_ll, &m_wh);
}

//
//
//
void AM_initVariables()
{
	static event_t st_notify(ev_keyup, AM_MSGENTERED, 0, 0);

	thintriangle_guy.clear();
	thinrectangle_guy.clear();

	mline_t ml;

#define L(a) (fixed_t)((a)*FRACUNIT)
#define ADD_TO_VEC(vec, ax, ay, bx, by) \
	ml.a.x = L(ax); \
	ml.a.y = L(ay); \
	ml.b.x = L(bx); \
	ml.b.y = L(by); \
	vec.push_back(ml);

	ADD_TO_VEC(thintriangle_guy, -.5, -.7,   1,   0)
	ADD_TO_VEC(thintriangle_guy,   1,   0, -.5,  .7)
	ADD_TO_VEC(thintriangle_guy, -.5,  .7, -.5, -.7)

	ADD_TO_VEC(thinrectangle_guy,  1,  1,  1, -1)
	ADD_TO_VEC(thinrectangle_guy,  1, -1, -1, -1)
	ADD_TO_VEC(thinrectangle_guy, -1, -1, -1,  1)
	ADD_TO_VEC(thinrectangle_guy, -1,  1,  1,  1)

#undef ADD_TO_VEC
#undef L

	automapactive = true;

	f_oldloc.x = MAXINT;
	amclock = 0;

	M_SetVec2Fixed(&m_wh, FTOM(I_GetSurfaceWidth()), FTOM(I_GetSurfaceHeight()));

	// find player to center on initially
	player_t* pl = &displayplayer();
	if (!pl->ingame())
	{
		for (Players::iterator it = players.begin(); it != players.end(); ++it)
		{
			if (it->ingame())
			{
				pl = &*it;
				break;
			}
		}
	}

	if (!pl->camera)
		return;

	m_ll.x = pl->camera->x - m_wh.x / 2;
	m_ll.y = pl->camera->y - m_wh.y / 2;
	AM_changeWindowLoc();

	AM_saveScaleAndLoc();

	// inform the status bar of the change
	ST_Responder(&st_notify);
}

am_color_t AM_GetColorFromString(const argb_t* palette_colors, const char* colorstring)
{
	am_color_t c;
	c.rgb = V_GetColorFromString(colorstring);
	c.index = V_BestColor(palette_colors, c.rgb);
	return c;
}

am_color_t AM_BestColor(const argb_t* palette_colors, const int r, const int g,
                        const int b)
{
	am_color_t c;
	c.rgb = argb_t(r, g, b);
	c.index = V_BestColor(palette_colors, c.rgb);
	return c;
}

void AM_SetBaseColorDoom()
{
	gameinfo.defaultAutomapColors.Background		= "00 00 00";
	gameinfo.defaultAutomapColors.YourColor			= "ff ff ff";
	gameinfo.defaultAutomapColors.AlmostBackground	= "10 10 10";
	gameinfo.defaultAutomapColors.SecretWallColor	= "fc 00 00";
	gameinfo.defaultAutomapColors.WallColor			= "fc 00 00";
	gameinfo.defaultAutomapColors.TSWallColor		= "80 80 80";
	gameinfo.defaultAutomapColors.FDWallColor		= "bc 78 48";
	gameinfo.defaultAutomapColors.LockedColor		= "fc fc 00";
	gameinfo.defaultAutomapColors.CDWallColor		= "fc fc 00";
	gameinfo.defaultAutomapColors.ThingColor		= "dark grey";
	gameinfo.defaultAutomapColors.ThingColor_Item			= "navy";
	gameinfo.defaultAutomapColors.ThingColor_CountItem		= "sky blue";
	gameinfo.defaultAutomapColors.ThingColor_Monster		= "74 fc 6c";
	gameinfo.defaultAutomapColors.ThingColor_NoCountMonster	= "yellow";
	gameinfo.defaultAutomapColors.ThingColor_Friend			= "dark green";
	gameinfo.defaultAutomapColors.ThingColor_Projectile		= "orange";
	gameinfo.defaultAutomapColors.GridColor			= "4c 4c 4c";
	gameinfo.defaultAutomapColors.XHairColor		= "80 80 80";
	gameinfo.defaultAutomapColors.NotSeenColor		= "6c 6c 6c";
}

void AM_SetBaseColorRaven()
{
	gameinfo.defaultAutomapColors.Background		= "00 00 00";
	gameinfo.defaultAutomapColors.YourColor			= "ff ff ff";
	gameinfo.defaultAutomapColors.AlmostBackground	= "10 10 10";
	gameinfo.defaultAutomapColors.SecretWallColor	= "4c 33 11";
	gameinfo.defaultAutomapColors.WallColor			= "4c 33 11";
	gameinfo.defaultAutomapColors.TSWallColor		= "59 5e 57";
	gameinfo.defaultAutomapColors.FDWallColor		= "d0 b0 85";
	gameinfo.defaultAutomapColors.LockedColor		= "fc fc 00";
	gameinfo.defaultAutomapColors.CDWallColor		= "68 3c 20";
	gameinfo.defaultAutomapColors.ThingColor		= "38 38 38";
	gameinfo.defaultAutomapColors.ThingColor_Item			= "38 38 38"; // todo
	gameinfo.defaultAutomapColors.ThingColor_CountItem		= "38 38 38"; // todo
	gameinfo.defaultAutomapColors.ThingColor_Monster		= "38 38 38"; // todo
	gameinfo.defaultAutomapColors.ThingColor_NoCountMonster	= "38 38 38"; // todo
	gameinfo.defaultAutomapColors.ThingColor_Friend			= "38 38 38"; // todo
	gameinfo.defaultAutomapColors.ThingColor_Projectile		= "38 38 38"; // todo
	gameinfo.defaultAutomapColors.GridColor			= "4c 4c 4c";
	gameinfo.defaultAutomapColors.XHairColor		= "80 80 80";
	gameinfo.defaultAutomapColors.NotSeenColor		= "6c 6c 6c";
}

void AM_SetBaseColorStrife()
{
	gameinfo.defaultAutomapColors.Background		= "00 00 00";
	gameinfo.defaultAutomapColors.YourColor			= "ef ef 00";
	gameinfo.defaultAutomapColors.AlmostBackground	= "10 10 10";
	gameinfo.defaultAutomapColors.SecretWallColor	= "c7 c3 c3";
	gameinfo.defaultAutomapColors.WallColor			= "c7 c3 c3";
	gameinfo.defaultAutomapColors.TSWallColor		= "77 73 73";
	gameinfo.defaultAutomapColors.FDWallColor		= "37 3B 5B";
	gameinfo.defaultAutomapColors.LockedColor		= "77 73 73";
	gameinfo.defaultAutomapColors.CDWallColor		= "77 73 73";
	gameinfo.defaultAutomapColors.ThingColor		= "fc 00 00";
	gameinfo.defaultAutomapColors.ThingColor_Item			= "fc 00 00"; // todo
	gameinfo.defaultAutomapColors.ThingColor_CountItem		= "fc 00 00"; // todo
	gameinfo.defaultAutomapColors.ThingColor_Monster		= "fc 00 00"; // todo
	gameinfo.defaultAutomapColors.ThingColor_NoCountMonster	= "fc 00 00"; // todo
	gameinfo.defaultAutomapColors.ThingColor_Friend			= "fc 00 00"; // todo
	gameinfo.defaultAutomapColors.ThingColor_Projectile		= "fc 00 00"; // todo
	gameinfo.defaultAutomapColors.GridColor			= "4c 4c 4c";
	gameinfo.defaultAutomapColors.XHairColor		= "80 80 80";
	gameinfo.defaultAutomapColors.NotSeenColor		= "6c 6c 6c";
}

void AM_initColors(const bool overlayed)
{
	// Look up the colors in the current palette:
	const argb_t* palette_colors = V_GetDefaultPalette()->colors;

	if (overlayed && !am_ovshare)
	{
		gameinfo.currentAutomapColors.YourColor = AM_GetColorFromString(palette_colors, am_ovyourcolor.cstring());
		gameinfo.currentAutomapColors.SecretWallColor = gameinfo.currentAutomapColors.WallColor =
		    AM_GetColorFromString(palette_colors, am_ovwallcolor.cstring());
		gameinfo.currentAutomapColors.TSWallColor = AM_GetColorFromString(palette_colors, am_ovtswallcolor.cstring());
		gameinfo.currentAutomapColors.FDWallColor = AM_GetColorFromString(palette_colors, am_ovfdwallcolor.cstring());
		gameinfo.currentAutomapColors.CDWallColor = AM_GetColorFromString(palette_colors, am_ovcdwallcolor.cstring());
		gameinfo.currentAutomapColors.ThingColor = AM_GetColorFromString(palette_colors, am_ovthingcolor.cstring());
		gameinfo.currentAutomapColors.ThingColor_Item = AM_GetColorFromString(palette_colors, am_ovthingcolor_item.cstring());
		gameinfo.currentAutomapColors.ThingColor_CountItem = AM_GetColorFromString(palette_colors, am_ovthingcolor_countitem.cstring());
		gameinfo.currentAutomapColors.ThingColor_Monster = AM_GetColorFromString(palette_colors, am_ovthingcolor_monster.cstring());
		gameinfo.currentAutomapColors.ThingColor_NoCountMonster = AM_GetColorFromString(palette_colors, am_ovthingcolor_nocountmonster.cstring());
		gameinfo.currentAutomapColors.ThingColor_Friend = AM_GetColorFromString(palette_colors, am_ovthingcolor_friend.cstring());
		gameinfo.currentAutomapColors.ThingColor_Projectile = AM_GetColorFromString(palette_colors, am_ovthingcolor_projectile.cstring());
		gameinfo.currentAutomapColors.GridColor = AM_GetColorFromString(palette_colors, am_ovgridcolor.cstring());
		gameinfo.currentAutomapColors.XHairColor = AM_GetColorFromString(palette_colors, am_ovxhaircolor.cstring());
		gameinfo.currentAutomapColors.NotSeenColor = AM_GetColorFromString(palette_colors, am_ovnotseencolor.cstring());
		gameinfo.currentAutomapColors.LockedColor = AM_GetColorFromString(palette_colors, am_ovlockedcolor.cstring());
		gameinfo.currentAutomapColors.ExitColor = AM_GetColorFromString(palette_colors, am_ovexitcolor.cstring());
		gameinfo.currentAutomapColors.TeleportColor =
		    AM_GetColorFromString(palette_colors, am_ovteleportcolor.cstring());
	}
	else if (am_usecustomcolors || (overlayed && am_ovshare))
	{
		/* Use the custom colors in the am_* cvars */
		gameinfo.currentAutomapColors.Background = AM_GetColorFromString(palette_colors, am_backcolor.cstring());
		gameinfo.currentAutomapColors.YourColor = AM_GetColorFromString(palette_colors, am_yourcolor.cstring());
		gameinfo.currentAutomapColors.SecretWallColor = gameinfo.currentAutomapColors.WallColor =
		    AM_GetColorFromString(palette_colors, am_wallcolor.cstring());
		gameinfo.currentAutomapColors.TSWallColor = AM_GetColorFromString(palette_colors, am_tswallcolor.cstring());
		gameinfo.currentAutomapColors.FDWallColor = AM_GetColorFromString(palette_colors, am_fdwallcolor.cstring());
		gameinfo.currentAutomapColors.CDWallColor = AM_GetColorFromString(palette_colors, am_cdwallcolor.cstring());
		gameinfo.currentAutomapColors.ThingColor = AM_GetColorFromString(palette_colors, am_thingcolor.cstring());
		gameinfo.currentAutomapColors.ThingColor_Item = AM_GetColorFromString(palette_colors, am_thingcolor_item.cstring());
		gameinfo.currentAutomapColors.ThingColor_CountItem = AM_GetColorFromString(palette_colors, am_thingcolor_countitem.cstring());
		gameinfo.currentAutomapColors.ThingColor_Monster = AM_GetColorFromString(palette_colors, am_thingcolor_monster.cstring());
		gameinfo.currentAutomapColors.ThingColor_NoCountMonster = AM_GetColorFromString(palette_colors, am_thingcolor_nocountmonster.cstring());
		gameinfo.currentAutomapColors.ThingColor_Friend = AM_GetColorFromString(palette_colors, am_thingcolor_friend.cstring());
		gameinfo.currentAutomapColors.ThingColor_Projectile = AM_GetColorFromString(palette_colors, am_thingcolor_projectile.cstring());
		gameinfo.currentAutomapColors.GridColor = AM_GetColorFromString(palette_colors, am_gridcolor.cstring());
		gameinfo.currentAutomapColors.XHairColor = AM_GetColorFromString(palette_colors, am_xhaircolor.cstring());
		gameinfo.currentAutomapColors.NotSeenColor = AM_GetColorFromString(palette_colors, am_notseencolor.cstring());
		gameinfo.currentAutomapColors.LockedColor = AM_GetColorFromString(palette_colors, am_lockedcolor.cstring());
		gameinfo.currentAutomapColors.ExitColor = AM_GetColorFromString(palette_colors, am_exitcolor.cstring());
		gameinfo.currentAutomapColors.TeleportColor = AM_GetColorFromString(palette_colors, am_teleportcolor.cstring());
		{
			argb_t ba = AM_GetColorFromString(palette_colors, am_backcolor.cstring()).rgb;

			if (ba.getr() < 16)
				ba.setr(ba.getr() + 32);
			if (ba.getg() < 16)
				ba.setg(ba.getg() + 32);
			if (ba.getb() < 16)
				ba.setb(ba.getb() + 32);

			gameinfo.currentAutomapColors.AlmostBackground.rgb = argb_t(ba.getr() - 16, ba.getg() - 16, ba.getb() - 16);
			gameinfo.currentAutomapColors.AlmostBackground.index = V_BestColor(palette_colors, gameinfo.currentAutomapColors.AlmostBackground.rgb);
		}
	}
	else
	{
		gameinfo.currentAutomapColors.Background =
			AM_GetColorFromString(palette_colors, gameinfo.defaultAutomapColors.Background.c_str());
		gameinfo.currentAutomapColors.YourColor =
			AM_GetColorFromString(palette_colors, gameinfo.defaultAutomapColors.YourColor.c_str());
		gameinfo.currentAutomapColors.AlmostBackground =
			AM_GetColorFromString(palette_colors, gameinfo.defaultAutomapColors.AlmostBackground.c_str());
		gameinfo.currentAutomapColors.SecretWallColor =
			AM_GetColorFromString(palette_colors, gameinfo.defaultAutomapColors.SecretWallColor.c_str());
		gameinfo.currentAutomapColors.WallColor =
			AM_GetColorFromString(palette_colors, gameinfo.defaultAutomapColors.WallColor.c_str());
		gameinfo.currentAutomapColors.TSWallColor =
			AM_GetColorFromString(palette_colors, gameinfo.defaultAutomapColors.TSWallColor.c_str());
		gameinfo.currentAutomapColors.FDWallColor =
			AM_GetColorFromString(palette_colors, gameinfo.defaultAutomapColors.FDWallColor.c_str());
		gameinfo.currentAutomapColors.LockedColor =
			AM_GetColorFromString(palette_colors, gameinfo.defaultAutomapColors.LockedColor.c_str());
		gameinfo.currentAutomapColors.CDWallColor =
			AM_GetColorFromString(palette_colors, gameinfo.defaultAutomapColors.CDWallColor.c_str());
		gameinfo.currentAutomapColors.ThingColor =
			AM_GetColorFromString(palette_colors, gameinfo.defaultAutomapColors.ThingColor.c_str());
		gameinfo.currentAutomapColors.ThingColor_Item =
			AM_GetColorFromString(palette_colors, gameinfo.defaultAutomapColors.ThingColor_Item.c_str());
		gameinfo.currentAutomapColors.ThingColor_CountItem =
			AM_GetColorFromString(palette_colors, gameinfo.defaultAutomapColors.ThingColor_CountItem.c_str());
		gameinfo.currentAutomapColors.ThingColor_Monster =
			AM_GetColorFromString(palette_colors, gameinfo.defaultAutomapColors.ThingColor_Monster.c_str());
		gameinfo.currentAutomapColors.ThingColor_NoCountMonster =
			AM_GetColorFromString(palette_colors, gameinfo.defaultAutomapColors.ThingColor_NoCountMonster.c_str());
		gameinfo.currentAutomapColors.ThingColor_Friend =
			AM_GetColorFromString(palette_colors, gameinfo.defaultAutomapColors.ThingColor_Friend.c_str());
		gameinfo.currentAutomapColors.ThingColor_Projectile =
			AM_GetColorFromString(palette_colors, gameinfo.defaultAutomapColors.ThingColor_Projectile.c_str());
		gameinfo.currentAutomapColors.GridColor =
			AM_GetColorFromString(palette_colors, gameinfo.defaultAutomapColors.GridColor.c_str());
		gameinfo.currentAutomapColors.XHairColor =
			AM_GetColorFromString(palette_colors, gameinfo.defaultAutomapColors.XHairColor.c_str());
		gameinfo.currentAutomapColors.NotSeenColor =
			AM_GetColorFromString(palette_colors, gameinfo.defaultAutomapColors.NotSeenColor.c_str());
	}
}

//
//
//
void AM_loadPics()
{
	char namebuf[9];

	for (int i = 0; i < 10; i++)
	{
		sprintf(namebuf, "AMMNUM%d", i);
		marknums[i] = W_CachePatchHandle(namebuf, PU_STATIC);
	}
}

void AM_unloadPics()
{
	for (int i = 0; i < 10; i++)
		marknums[i].clear();
}

void AM_clearMarks()
{
	for (int i = AM_NUMMARKPOINTS - 1; i >= 0; i--)
		markpoints[i].x = -1; // means empty
	markpointnum = 0;
}

//
// should be called at the start of every level
// right now, i figure it out myself
//
void AM_LevelInit()
{
	leveljuststarted = false;
	am_cheating = 0; // force-reset IDDT after loading a map

	AM_clearMarks();

	AM_findMinMaxBoundaries();
	scale_mtof = FixedDiv(min_scale_mtof, static_cast<int>(0.7 * FRACUNIT));
	if (scale_mtof > max_scale_mtof)
		scale_mtof = min_scale_mtof;
	scale_ftom = FixedDiv(FRACUNIT, scale_mtof);
}

//
//
//
void AM_Stop()
{
	if (!automapactive)
		return;

	AM_unloadPics();
	automapactive = false;

	static event_t st_notify(ev_keyup, AM_MSGEXITED, 0, 0);
	ST_Responder(&st_notify);

	stopped = true;
	viewactive = true;
}

//
//
//
void AM_Start()
{
	if (!stopped)
		AM_Stop();

	stopped = false;

	// todo: rather than call this every time automap is opened, do this once on level
	// init
	static OLumpName lastmap = "";
	if (level.mapname != lastmap)
	{
		AM_LevelInit();
		lastmap = level.mapname;
	}

	AM_initVariables();
	AM_loadPics();
}

//
// set the window scale to the maximum size
//
void AM_minOutWindowScale()
{
	scale_mtof = min_scale_mtof;
	scale_ftom = FixedDiv(FRACUNIT, scale_mtof);
}

//
// set the window scale to the minimum size
//
void AM_maxOutWindowScale()
{
	scale_mtof = max_scale_mtof;
	scale_ftom = FixedDiv(FRACUNIT, scale_mtof);
}

BEGIN_COMMAND(togglemap)
{
	if (gamestate != GS_LEVEL)
		return;

	if (!automapactive)
	{
		AM_Start();
		viewactive = am_overlay != 0;
	}
	else
	{
		if (am_overlay > 0 && am_overlay < 3 && viewactive)
			viewactive = false;
		else
			AM_Stop();
	}

	if (automapactive)
		AM_initColors(viewactive);

	// status bar is drawn in classic automap
	ST_ForceRefresh();
}
END_COMMAND(togglemap)

//
// Handle events (user inputs) in automap mode
//
BOOL AM_Responder(event_t* ev)
{
	if (automapactive && (ev->type == ev_keydown || ev->type == ev_keyup))
	{
		if (am_followplayer)
		{
			// check for am_pan* and ignore in follow mode
			const std::string defbind = AutomapBindings.Binds[ev->data1];
			if (!strnicmp(defbind.c_str(), "+am_pan", 7))
				return false;
		}

		if (ev->type == ev_keydown)
		{
			const std::string defbind = Bindings.Binds[ev->data1];
			// Check for automap, in order not to be stuck
			if (!strnicmp(defbind.c_str(), "togglemap", 9))
				return false;
		}

		const bool res = C_DoKey(ev, &AutomapBindings, NULL);
		if (res && ev->type == ev_keyup)
		{
			// If this is a release event we also need to check if it released a button in
			// the main Bindings so that that button does not get stuck.
			const std::string defbind = Bindings.Binds[ev->data1];

			// Check for automap, in order not to be stuck
			if (!strnicmp(defbind.c_str(), "togglemap", 9))
				return false;

			return (defbind[0] != '+'); // Let G_Responder handle button releases
		}
		return res;
	}

	return false;
}

//
// Zooming
//
void AM_changeWindowScale()
{
	static fixed_t mtof_zoommul; // how far the window zooms in each tic (map coords)

	if (Actions[ACTION_AUTOMAP_ZOOMOUT])
	{
		mtof_zoommul = M_ZOOMOUT;
		ftom_zoommul = M_ZOOMIN;
	}
	else if (Actions[ACTION_AUTOMAP_ZOOMIN])
	{
		mtof_zoommul = M_ZOOMIN;
		ftom_zoommul = M_ZOOMOUT;
	}
	else
	{
		mtof_zoommul = FRACUNIT;
		ftom_zoommul = FRACUNIT;
	}

	// Change the scaling multipliers
	scale_mtof = FixedMul(scale_mtof, mtof_zoommul);
	scale_ftom = FixedDiv(FRACUNIT, scale_mtof);

	if (scale_mtof < min_scale_mtof)
		AM_minOutWindowScale();
	else if (scale_mtof > max_scale_mtof)
		AM_maxOutWindowScale();
}

//
//
//
void AM_doFollowPlayer()
{
	M_SetVec2Fixed(&m_ll,
		viewx - m_wh.x / 2,
		viewy - m_wh.y / 2);

	M_SetVec2Fixed(&m_ur,
		m_ll.x + m_wh.x,
		m_ll.y + m_wh.y);
}

//
// Updates on Game Tick
//
void AM_Ticker()
{
	if (!automapactive)
		return;

	amclock++;

	// NES - Glowing effect on locked doors.
	if (lockglow < 90)
		lockglow++;
	else
		lockglow = 0;
}

//
// Clear automap frame buffer.
//
void AM_clearFB(am_color_t color)
{
	if (I_GetPrimarySurface()->getBitsPerPixel() == 8)
	{
		if (f_w == f_p)
			memset(fb, color.index, f_w * f_h);
		else
			for (int y = 0; y < f_h; y++)
				memset(fb + y * f_p, color.index, f_w);
	}
	else
	{
		argb_t* line = reinterpret_cast<argb_t*>(fb);

		for (int y = 0; y < f_h; y++)
		{
			for (int x = 0; x < f_w; x++)
				line[x] = color.rgb;
			line += f_p >> 2;
		}
	}
}

//
// Automap clipping of lines.
//
// Based on Cohen-Sutherland clipping algorithm but with a slightly
// faster reject and precalculated slopes.  If the speed is needed,
// use a hash algorithm to handle  the common cases.
//
bool AM_clipMline(mline_t* ml, fline_t* fl)
{
	enum
	{
		LEFT = 1,
		RIGHT = 2,
		BOTTOM = 4,
		TOP = 8
	};

	int outcode1 = 0;
	int outcode2 = 0;
	int outside;

	fpoint_t tmp;

	tmp.x = 0;
	tmp.y = 0;
	int dx;
	int dy;

#define DOOUTCODE(oc, mx, my) \
	(oc) = 0;                 \
	if ((my) < 0)             \
		(oc) |= TOP;          \
	else if ((my) >= f_h)     \
		(oc) |= BOTTOM;       \
	if ((mx) < 0)             \
		(oc) |= LEFT;         \
	else if ((mx) >= f_w)     \
		(oc) |= RIGHT;

	// do trivial rejects and outcodes
	if (ml->a.y > m_ur.y)
		outcode1 = TOP;
	else if (ml->a.y < m_ll.y)
		outcode1 = BOTTOM;

	if (ml->b.y > m_ur.y)
		outcode2 = TOP;
	else if (ml->b.y < m_ll.y)
		outcode2 = BOTTOM;

	if (outcode1 & outcode2)
		return false; // trivially outside

	if (ml->a.x < m_ll.x)
		outcode1 |= LEFT;
	else if (ml->a.x > m_ur.x)
		outcode1 |= RIGHT;

	if (ml->b.x < m_ll.x)
		outcode2 |= LEFT;
	else if (ml->b.x > m_ur.x)
		outcode2 |= RIGHT;

	if (outcode1 & outcode2)
		return false; // trivially outside

	// transform to frame-buffer coordinates.
	fl->a.x = CXMTOF(ml->a.x);
	fl->a.y = CYMTOF(ml->a.y);
	fl->b.x = CXMTOF(ml->b.x);
	fl->b.y = CYMTOF(ml->b.y);

	DOOUTCODE(outcode1, fl->a.x, fl->a.y);
	DOOUTCODE(outcode2, fl->b.x, fl->b.y);

	if (outcode1 & outcode2)
		return false;

	while (outcode1 | outcode2)
	{
		// may be partially inside box
		// find an outside point
		if (outcode1)
			outside = outcode1;
		else
			outside = outcode2;

		// clip to each side
		if (outside & TOP)
		{
			dy = fl->a.y - fl->b.y;
			dx = fl->b.x - fl->a.x;
			tmp.x = fl->a.x + (dx * (fl->a.y)) / dy;
			tmp.y = 0;
		}
		else if (outside & BOTTOM)
		{
			dy = fl->a.y - fl->b.y;
			dx = fl->b.x - fl->a.x;
			tmp.x = fl->a.x + (dx * (fl->a.y - f_h)) / dy;
			tmp.y = f_h - 1;
		}
		else if (outside & RIGHT)
		{
			dy = fl->b.y - fl->a.y;
			dx = fl->b.x - fl->a.x;
			tmp.y = fl->a.y + (dy * (f_w - 1 - fl->a.x)) / dx;
			tmp.x = f_w - 1;
		}
		else if (outside & LEFT)
		{
			dy = fl->b.y - fl->a.y;
			dx = fl->b.x - fl->a.x;
			tmp.y = fl->a.y + (dy * (-fl->a.x)) / dx;
			tmp.x = 0;
		}

		if (outside == outcode1)
		{
			fl->a = tmp;
			DOOUTCODE(outcode1, fl->a.x, fl->a.y);
		}
		else
		{
			fl->b = tmp;
			DOOUTCODE(outcode2, fl->b.x, fl->b.y);
		}

		if (outcode1 & outcode2)
			return false; // trivially outside
	}

	return true;
}
#undef DOOUTCODE

//
// Classic Bresenham w/ whatever optimizations needed for speed
//

// Palettized (8bpp) version:

void AM_drawFlineP(fline_t* fl, byte color)
{
	fl->a.x += f.x;
	fl->a.y += f.y;
	fl->b.x += f.x;
	fl->b.y += f.y;

	const int dx = fl->b.x - fl->a.x;
	const int ax = 2 * (dx < 0 ? -dx : dx);
	const int sx = dx < 0 ? -1 : 1;

	const int dy = fl->b.y - fl->a.y;
	const int ay = 2 * (dy < 0 ? -dy : dy);
	const int sy = dy < 0 ? -1 : 1;

	int x = fl->a.x;
	int y = fl->a.y;

	if (ax > ay)
	{
		int d = ay - ax / 2;
		while (true)
		{
			PUTDOTP(x, y, (byte)color);
			if (x == fl->b.x)
				return;
			if (d >= 0)
			{
				y += sy;
				d -= ax;
			}
			x += sx;
			d += ay;
		}
	}
	else
	{
		int d = ax - ay / 2;
		while (true)
		{
			PUTDOTP(x, y, (byte)color);
			if (y == fl->b.y)
				return;
			if (d >= 0)
			{
				x += sx;
				d -= ay;
			}
			y += sy;
			d += ax;
		}
	}
}

// Direct (32bpp) version:

void AM_drawFlineD(fline_t* fl, argb_t color)
{
	fl->a.x += f.x;
	fl->b.x += f.x;
	fl->a.y += f.y;
	fl->b.y += f.y;

	const int dx = fl->b.x - fl->a.x;
	const int ax = 2 * (dx < 0 ? -dx : dx);
	const int sx = dx < 0 ? -1 : 1;

	const int dy = fl->b.y - fl->a.y;
	const int ay = 2 * (dy < 0 ? -dy : dy);
	const int sy = dy < 0 ? -1 : 1;

	int x = fl->a.x;
	int y = fl->a.y;

	int d;

	if (ax > ay)
	{
		d = ay - ax / 2;

		while (true)
		{
			PUTDOTD(x, y, color);
			if (x == fl->b.x)
				return;
			if (d >= 0)
			{
				y += sy;
				d -= ax;
			}
			x += sx;
			d += ay;
		}
	}
	else
	{
		d = ax - ay / 2;
		while (true)
		{
			PUTDOTD(x, y, color);
			if (y == fl->b.y)
				return;
			if (d >= 0)
			{
				x += sx;
				d -= ay;
			}
			y += sy;
			d += ax;
		}
	}
}

//
// Clip lines, draw visible part sof lines.
//
void AM_drawMline(mline_t* ml, am_color_t color)
{
	static fline_t fl;

	if (AM_clipMline(ml, &fl))
	{
		// draws it on frame buffer using fb coords
		if (I_GetPrimarySurface()->getBitsPerPixel() == 8)
			AM_drawFlineP(&fl, color.index);
		else
			AM_drawFlineD(&fl, color.rgb);
	}
}

//
// Draws flat (floor/ceiling tile) aligned grid lines.
//
void AM_drawGrid(am_color_t color)
{
	// find maximum distance lines should be drawn from center of screen in world coordinates
	v2fixed_t distVec;
	M_SubVec2Fixed(&distVec, &m_ll, &m_ur);
	const fixed_t dist = P_AproxDistance(distVec.x, distVec.y);
	const fixed_t half_dist = FixedDiv(dist, INT2FIXED(2));

	// find center point of screen in world coordinates
	v2fixed_t centerp;
	centerp.x = FixedDiv(m_ur.x + m_ll.x, INT2FIXED(2));
	centerp.y = FixedDiv(m_ur.y + m_ll.y, INT2FIXED(2));

	const fixed_t w = INT2FIXED(MAPBLOCKUNITS);
	const fixed_t minimum_x = centerp.x - half_dist;
	const fixed_t maximum_x = centerp.x + half_dist;
	const fixed_t minimum_y = centerp.y - half_dist;
	const fixed_t maximum_y = centerp.y + half_dist;

	fixed_t start = w + minimum_x - ((minimum_x % w) + w) % w;

	mline_t ml;

	// draw vertical gridlines
	for (fixed_t x = start; x < maximum_x; x += w)
	{
		ml.a.y = minimum_y;
		ml.b.y = maximum_y;

		ml.a.x = x;
		ml.b.x = x;
		if (am_rotate)
		{
			AM_rotatePoint(ml.a);
			AM_rotatePoint(ml.b);
		}
		AM_drawMline(&ml, color);
	}

	start = w + minimum_y - ((minimum_y % w) + w) % w;

	// draw horizontal gridlines
	for (fixed_t y = start; y < maximum_y; y += w)
	{
		ml.a.x = minimum_x;
		ml.b.x = maximum_x;

		ml.a.y = y;
		ml.b.y = y;
		if (am_rotate)
		{
			AM_rotatePoint(ml.a);
			AM_rotatePoint(ml.b);
		}
		AM_drawMline(&ml, color);
	}
}

//
// Determines visible lines, draws them.
// This is LineDef based, not LineSeg based.
//
void AM_drawWalls()
{
	int r, g, b;
	static mline_t l;
	float rdif, gdif, bdif;
	const palette_t* pal = V_GetDefaultPalette();

	for (int i = 0; i < numlines; i++)
	{
		M_SetVec2Fixed(&l.a, lines[i].v1->x, lines[i].v1->y);
		M_SetVec2Fixed(&l.b, lines[i].v2->x, lines[i].v2->y);

		if (am_rotate)
		{
			AM_rotatePoint(l.a);
			AM_rotatePoint(l.b);
		}

		if (am_cheating || (lines[i].flags & ML_MAPPED))
		{
			if ((lines[i].flags & ML_DONTDRAW) && !am_cheating)
				continue;
			if (!lines[i].backsector && ((am_usecustomcolors || viewactive) ||
			                             (!am_usecustomcolors && !viewactive)))
			{
				AM_drawMline(&l, gameinfo.currentAutomapColors.WallColor);
			}
			else
			{
				if ((P_IsTeleportLine(lines[i].special)) &&
				    (am_usecustomcolors || viewactive))
				{ // teleporters
					AM_drawMline(&l, gameinfo.currentAutomapColors.TeleportColor);
				}
				else if ((P_IsExitLine(lines[i].special)) &&
				         (am_usecustomcolors || viewactive))
				{ // exit
					AM_drawMline(&l, gameinfo.currentAutomapColors.ExitColor);
				}
				else if (lines[i].flags & ML_SECRET)
				{ // secret door
					if (am_cheating)
						AM_drawMline(&l, gameinfo.currentAutomapColors.SecretWallColor);
					else
						AM_drawMline(&l, gameinfo.currentAutomapColors.WallColor);
				}
				else if (lines[i].backsector->floorheight !=
				         lines[i].frontsector->floorheight)
				{
					AM_drawMline(&l, gameinfo.currentAutomapColors.FDWallColor); // floor level change
				}
				else if (lines[i].backsector->ceilingheight !=
				         lines[i].frontsector->ceilingheight)
				{
					AM_drawMline(&l, gameinfo.currentAutomapColors.CDWallColor); // ceiling level change
				}
				else if (am_cheating)
				{
					AM_drawMline(&l, gameinfo.currentAutomapColors.TSWallColor);
				}

				if (map_format.getZDoom())
				{
					if (lines[i].special == Door_LockedRaise)
					{
						// NES - Locked doors glow from a predefined color to either blue,
						// yellow, or red.
						r = gameinfo.currentAutomapColors.LockedColor.rgb.getr();
						g = gameinfo.currentAutomapColors.LockedColor.rgb.getg();
						b = gameinfo.currentAutomapColors.LockedColor.rgb.getb();

						if (am_usecustomcolors)
						{
							if (lines[i].args[3] == (zk_blue_card | zk_blue))
							{
								rdif = (0 - r) / 30;
								gdif = (0 - g) / 30;
								bdif = (255 - b) / 30;
							}
							else if (lines[i].args[3] == (zk_yellow_card | zk_yellow))
							{
								rdif = (255 - r) / 30;
								gdif = (255 - g) / 30;
								bdif = (0 - b) / 30;
							}
							else
							{
								rdif = (255 - r) / 30;
								gdif = (0 - g) / 30;
								bdif = (0 - b) / 30;
							}

							if (lockglow < 30)
							{
								r += static_cast<int>(rdif) * lockglow;
								g += static_cast<int>(gdif) * lockglow;
								b += static_cast<int>(bdif) * lockglow;
							}
							else if (lockglow < 60)
							{
								r += static_cast<int>(rdif) * (60 - lockglow);
								g += static_cast<int>(gdif) * (60 - lockglow);
								b += static_cast<int>(bdif) * (60 - lockglow);
							}
						}

						AM_drawMline(&l, AM_BestColor(pal->basecolors, r, g, b));
					}
				}
				else
				{
					if (P_IsCompatibleLockedDoorLine(lines[i].special))
					{
						// NES - Locked doors glow from a predefined color to either blue,
						// yellow, or red.
						r = gameinfo.currentAutomapColors.LockedColor.rgb.getr();
						g = gameinfo.currentAutomapColors.LockedColor.rgb.getg();
						b = gameinfo.currentAutomapColors.LockedColor.rgb.getb();

						if (am_usecustomcolors)
						{
							if (P_IsCompatibleBlueDoorLine(lines[i].special))
							{
								rdif = (0 - r) / 30;
								gdif = (0 - g) / 30;
								bdif = (255 - b) / 30;
							}
							else if (P_IsCompatibleYellowDoorLine(lines[i].special))
							{
								rdif = (255 - r) / 30;
								gdif = (255 - g) / 30;
								bdif = (0 - b) / 30;
							}
							else
							{
								rdif = (255 - r) / 30;
								gdif = (0 - g) / 30;
								bdif = (0 - b) / 30;
							}

							if (lockglow < 30)
							{
								r += static_cast<int>(rdif) * lockglow;
								g += static_cast<int>(gdif) * lockglow;
								b += static_cast<int>(bdif) * lockglow;
							}
							else if (lockglow < 60)
							{
								r += static_cast<int>(rdif) * (60 - lockglow);
								g += static_cast<int>(gdif) * (60 - lockglow);
								b += static_cast<int>(bdif) * (60 - lockglow);
							}
						}

						AM_drawMline(&l, AM_BestColor(pal->basecolors, r, g, b));
					}
				}
			}
		}
		else if (consoleplayer().powers[pw_allmap])
		{
			if (!(lines[i].flags & ML_DONTDRAW))
				AM_drawMline(&l, gameinfo.currentAutomapColors.NotSeenColor);
		}
	}
}

//
// Rotation in 2D.
// Used to rotate player arrow line character.
//
void AM_rotate(mpoint_t& pt, angle_t a)
{
	const fixed_t tmpx = FixedMul(pt.x, finecosine[a >> ANGLETOFINESHIFT]) -
	                     FixedMul(pt.y, finesine[a >> ANGLETOFINESHIFT]);

	pt.y = FixedMul(pt.x, finesine[a >> ANGLETOFINESHIFT]) +
	       FixedMul(pt.y, finecosine[a >> ANGLETOFINESHIFT]);

	pt.x = tmpx;
}

void AM_rotatePoint(mpoint_t& pt)
{
	player_t* player = &displayplayer();

	fixed_t x = player->camera->prevx +
	            FixedMul(player->camera->x - player->camera->prevx, render_lerp_amount);
	fixed_t y = player->camera->prevy +
	            FixedMul(player->camera->y - player->camera->prevy, render_lerp_amount);
	fixed_t pangle = player->camera->prevangle +
	            FixedMul(player->camera->angle - player->camera->prevangle, render_lerp_amount);

	pt.x -= x;
	pt.y -= y;
	AM_rotate(pt, ANG90 - pangle);
	pt.x += x;
	pt.y += y;
}

void AM_drawLineCharacter(const std::vector<mline_t>& lineguy, fixed_t scale,
                          angle_t angle, am_color_t color, fixed_t x, fixed_t y)
{
	for (std::vector<mline_t>::const_iterator it = lineguy.begin(); it != lineguy.end(); ++it)
	{
		mline_t l;
		l.a = it->a;

		if (scale)
			M_ScaleVec2Fixed(&l.a, &l.a, scale);

		if (angle)
			AM_rotate(l.a, angle);

		l.a.x += x;
		l.a.y += y;

		l.b = it->b;

		if (scale)
			M_ScaleVec2Fixed(&l.b, &l.b, scale);

		if (angle)
			AM_rotate(l.b, angle);

		l.b.x += x;
		l.b.y += y;

		AM_drawMline(&l, color);
	}
}

void AM_drawPlayers()
{
	angle_t angle;
	player_t& conplayer = displayplayer();

	fixed_t x =
	    conplayer.camera->prevx +
	    FixedMul(conplayer.camera->x - conplayer.camera->prevx, render_lerp_amount);
	fixed_t y =
	    conplayer.camera->prevy +
	    FixedMul(conplayer.camera->y - conplayer.camera->prevy, render_lerp_amount);
	fixed_t cangle =
	    conplayer.camera->prevangle +
	    FixedMul(conplayer.camera->angle - conplayer.camera->prevangle, render_lerp_amount);

	if (!multiplayer)
	{
		if (am_rotate)
			angle = ANG90;
		else
			angle = cangle;

		if (am_cheating && !gameinfo.mapArrowCheat.empty())
			AM_drawLineCharacter(gameinfo.mapArrowCheat, INT2FIXED(16), angle,
			                     gameinfo.currentAutomapColors.YourColor,
								 conplayer.camera->x, conplayer.camera->y);
		else
			AM_drawLineCharacter(gameinfo.mapArrow, INT2FIXED(16), angle,
				gameinfo.currentAutomapColors.YourColor, x, y);
		return;
	}

	for (Players::iterator it = players.begin(); it != players.end(); ++it)
	{
		player_t* p = &*it;
		am_color_t color;

		if (!(it->ingame()) || !p->mo ||
		    (((G_IsFFAGame() && p != &conplayer) ||
		      (G_IsTeamGame() && p->userinfo.team != conplayer.userinfo.team)) &&
		     !(netdemo.isPlaying() || netdemo.isPaused()) && !demoplayback &&
		     !(conplayer.spectator)) ||
		    p->spectator)
		{
			continue;
		}

		if (p->powers[pw_invisibility])
		{
			color = gameinfo.currentAutomapColors.AlmostBackground;
		}
		else if (demoplayback)
		{
			const argb_t* palette = V_GetDefaultPalette()->colors;

			switch (it->id)
			{
			case 1:
				color = AM_GetColorFromString(palette, "00 FF 00");
				break;
			case 2:
				color = AM_GetColorFromString(palette, "60 60 B0");
				break;
			case 3:
				color = AM_GetColorFromString(palette, "B0 B0 30");
				break;
			case 4:
				color = AM_GetColorFromString(palette, "C0 00 00");
				break;
			default:
				break;
			}
		}
		else
		{
			color.rgb = CL_GetPlayerColor(p);
			color.index = V_BestColor(V_GetDefaultPalette()->basecolors, color.rgb);
		}

		mpoint_t pt;
		fixed_t moangle = p->mo->prevangle +
			FixedMul(p->mo->angle - p->mo->prevangle,
			render_lerp_amount);

		fixed_t mox = p->mo->prevx +
			FixedMul(p->mo->x - p->mo->prevx,
			render_lerp_amount);

		fixed_t moy = p->mo->prevy +
			FixedMul(p->mo->y - p->mo->prevy,
			render_lerp_amount);
		M_SetVec2Fixed(&pt, mox, moy);

		angle = moangle;

		if (am_rotate)
		{
			AM_rotatePoint(pt);
			angle -= cangle - ANG90;
		}

		AM_drawLineCharacter(gameinfo.mapArrow, INT2FIXED(16), angle, color, pt.x, pt.y);
	}
}

bool AM_actorIsKey(AActor* t)
{
	if (t->sprite == SPR_BKEY || t->sprite == SPR_YKEY || t->sprite == SPR_RKEY ||
	    t->sprite == SPR_BSKU || t->sprite == SPR_YSKU || t->sprite == SPR_RSKU)
	{
		return true;
	}

	return false;
}

am_color_t AM_getKeyColor(AActor *t)
{
	am_color_t color = gameinfo.currentAutomapColors.ThingColor;
	const argb_t* palette = V_GetDefaultPalette()->colors;

	if (t->sprite == SPR_BKEY || t->sprite == SPR_BSKU)
		color = AM_GetColorFromString(palette, "blue");
	if (t->sprite == SPR_YKEY || t->sprite == SPR_YSKU)
		color = AM_GetColorFromString(palette, "yellow");
	if (t->sprite == SPR_RKEY || t->sprite == SPR_RSKU)
		color = AM_GetColorFromString(palette, "red");

	return color;
}

void AM_drawEasyKeys()
{
	for (int i = 0; i < numsectors; i++)
	{
		AActor* t = sectors[i].thinglist;
		while (t)
		{
			if (AM_actorIsKey(t))
			{
				mpoint_t p;
				M_SetVec2Fixed(&p, t->x, t->y);

				const am_color_t key_color = AM_getKeyColor(t);

				AM_drawLineCharacter(gameinfo.easyKey, t->radius, 0, key_color, p.x, p.y);
			}
			t = t->snext;
		}
	}
}

void AM_drawThings()
{
	for (int i = 0; i < numsectors; i++)
	{
		AActor* t = sectors[i].thinglist;
		while (t)
		{
			mpoint_t p;

			fixed_t thingx = t->prevx + FixedMul(t->x - t->prevx, render_lerp_amount);
			fixed_t thingy = t->prevy + FixedMul(t->y - t->prevy, render_lerp_amount);

			fixed_t tangle = t->prevangle +
				FixedMul(t->angle -	t->prevangle,
				render_lerp_amount);

			M_SetVec2Fixed(&p, thingx, thingy);
			angle_t rotate_angle = 0;
			angle_t triangle_angle = tangle;

			if (am_rotate)
			{
				AM_rotatePoint(p);
				fixed_t conangle = displayplayer().camera->prevangle +
					FixedMul(displayplayer().camera->angle -
					displayplayer().camera->prevangle,
					render_lerp_amount);
				rotate_angle = ANG90 - conangle;
				triangle_angle += rotate_angle;
			}

			if (AM_actorIsKey(t))
			{
				if (!G_GetCurrentSkill().easy_key)
				{
					const am_color_t key_color = AM_getKeyColor(t);

					AM_drawLineCharacter(gameinfo.cheatKey, t->radius, 0, key_color, p.x,
					                     p.y);
				}
			}
			else
			{
				am_color_t color = gameinfo.currentAutomapColors.ThingColor;

				AM_drawLineCharacter(thintriangle_guy, t->radius, triangle_angle, color,
				                     p.x, p.y);

				if (t->flags & MF_MISSILE)
				{
					color = gameinfo.currentAutomapColors.ThingColor_Projectile;
				}
				else if (t->flags & MF_SPECIAL)
				{
					if (t->flags & MF_COUNTITEM)
						color = gameinfo.currentAutomapColors.ThingColor_CountItem;
					else
						color = gameinfo.currentAutomapColors.ThingColor_Item;
				}
				else if (t->flags & MF_SOLID && t->flags & MF_SHOOTABLE)
				{
					if (t->flags & MF_FRIEND)
						color = gameinfo.currentAutomapColors.ThingColor_Friend;
					else if (t->flags & MF_COUNTKILL)
						color = gameinfo.currentAutomapColors.ThingColor_Monster;
					else
						color = gameinfo.currentAutomapColors.ThingColor_NoCountMonster;
				}

				AM_drawLineCharacter(thinrectangle_guy, t->radius, rotate_angle, color,
				                     p.x, p.y);
			}
			t = t->snext;
		}
	}
}

void AM_drawMarks()
{
	for (int i = 0; i < AM_NUMMARKPOINTS; i++)
	{
		if (markpoints[i].x != -1)
		{
			mpoint_t pt;
			pt.x = markpoints[i].x;
			pt.y = markpoints[i].y;

			if (am_rotate)
				AM_rotatePoint(pt);

			const int fx = CXMTOF(pt.x);
			const int fy = CYMTOF(pt.y) - 3;

			//      w = LESHORT(marknums[i]->width);
			//      h = LESHORT(marknums[i]->height);
			const int w = 5; // because something's wrong with the wad, i guess
			const int h = 6; // because something's wrong with the wad, i guess

			if (fx >= f.x && fx <= f_w - w && fy >= f.y && fy <= f_h - h)
			{
				screen->DrawPatchCleanNoMove(W_ResolvePatchHandle(marknums[i]), fx, fy);
			}
		}
	}
}

void AM_drawCrosshair(am_color_t color)
{
	// single point for now
	if (I_GetPrimarySurface()->getBitsPerPixel() == 8)
		PUTDOTP(f_w / 2, (f_h + 1) / 2, (byte)color.index);
	else
		PUTDOTD(f_w / 2, (f_h + 1) / 2, color.rgb);
}

//
// AM_Drawer
//
void AM_Drawer()
{
	if (!AM_ClassicAutomapVisible() && !AM_OverlayAutomapVisible())
		return;

	IWindowSurface* surface = I_GetPrimarySurface();
	const int surface_width = surface->getWidth(), surface_height = surface->getHeight();

	fb = surface->getBuffer();

	if (AM_ClassicAutomapVisible())
	{
		f.x = f.y = 0;
		f_w = surface_width;
		f_h = ST_StatusBarY(surface_width, surface_height);
		f_p = surface->getPitch();

		AM_clearFB(gameinfo.currentAutomapColors.Background);
	}
	else
	{
		f.x = R_ViewWindowX(surface_width, surface_height);
		f.y = R_ViewWindowY(surface_width, surface_height);
		f_w = R_ViewWidth(surface_width, surface_height);
		f_h = R_ViewHeight(surface_width, surface_height);
		f_p = surface->getPitch();
	}

	if (am_followplayer)
	{
		AM_doFollowPlayer();
	}
	else
	{
		M_ZeroVec2Fixed(&m_paninc);

		// pan according to the direction
		if (Actions[ACTION_AUTOMAP_PANLEFT])
			m_paninc.x = -FTOM(F_PANINC);
		if (Actions[ACTION_AUTOMAP_PANRIGHT])
			m_paninc.x = FTOM(F_PANINC);
		if (Actions[ACTION_AUTOMAP_PANUP])
			m_paninc.y = FTOM(F_PANINC);
		if (Actions[ACTION_AUTOMAP_PANDOWN])
			m_paninc.y = -FTOM(F_PANINC);
	}

	// Change the zoom if necessary
	if (ftom_zoommul != FRACUNIT || Actions[ACTION_AUTOMAP_ZOOMIN] ||
	    Actions[ACTION_AUTOMAP_ZOOMOUT])
		AM_changeWindowScale();

	// Change x,y location
	if (m_paninc.x || m_paninc.y)
		AM_changeWindowLoc();

	AM_activateNewScale();

	if (grid)
		AM_drawGrid(gameinfo.currentAutomapColors.GridColor);

	AM_drawWalls();
	AM_drawPlayers();
	if (G_GetCurrentSkill().easy_key)
		AM_drawEasyKeys();
	if (am_cheating == 2)
		AM_drawThings();

	if (!(viewactive && am_overlay < 2))
		AM_drawCrosshair(gameinfo.currentAutomapColors.XHairColor);

	AM_drawMarks();

	if (!(viewactive && am_overlay < 2) && !hu_font[0].empty())
	{
		std::string line;
		const int time = level.time / TICRATE;

		const int text_height = (W_ResolvePatchHandle(hu_font[0])->height() + 1) * CleanYfac;
		const int OV_Y = surface_height - (surface_height * 32 / 200);

		if (G_IsCoopGame())
		{
			if (am_showmonsters)
			{
				if (G_IsHordeMode())
				{
					StrFormat(line, TEXTCOLOR_RED "MONSTERS:" TEXTCOLOR_NORMAL " %d",
				        level.killed_monsters);
				}
				else
				{
					StrFormat(line, TEXTCOLOR_RED "MONSTERS:" TEXTCOLOR_NORMAL " %d / %d",
				        level.killed_monsters,
				        (level.total_monsters + level.respawned_monsters));
				}

				int x, y;
				const int text_width = V_StringWidth(line.c_str()) * CleanXfac;

				if (AM_OverlayAutomapVisible())
				{
					x = surface_width - text_width;
					y = OV_Y - (text_height * 4) + 1;
					if (G_IsHordeMode())
					{
						y -= text_height * 2;
					}
				}
				else
				{
					x = 0;
					y = OV_Y - (text_height * 2) + 1;
				}

				screen->DrawTextClean(CR_GREY, x, y, line.c_str());
			}

			if (am_showitems && !G_IsHordeMode())
			{
				StrFormat(line, TEXTCOLOR_RED "ITEMS:" TEXTCOLOR_NORMAL " %d / %d",
				        level.found_items,
				        level.total_items);

				int x, y;
				const int text_width = V_StringWidth(line.c_str()) * CleanXfac;

				if (AM_OverlayAutomapVisible())
				{
					x = surface_width - text_width;
					y = OV_Y - (text_height * 5) + 1;
				}
				else
				{
					x = 0;
					y = OV_Y - (text_height * 3) + 1;
				}

				screen->DrawTextClean(CR_GREY, x, y, line.c_str());
			}

			if (am_showsecrets && !G_IsHordeMode())
			{
				StrFormat(line, TEXTCOLOR_RED "SECRETS:" TEXTCOLOR_NORMAL " %d / %d",
				        level.found_secrets, level.total_secrets);
				int x, y;
				const int text_width = V_StringWidth(line.c_str()) * CleanXfac;

				if (AM_OverlayAutomapVisible())
				{
					x = surface_width - text_width;
					y = OV_Y - (text_height * 3) + 1;
				}
				else
				{
					x = surface_width - text_width;
					y = OV_Y - (text_height * 2) + 1;
				}

				screen->DrawTextClean(CR_GREY, x, y, line.c_str());
			}
		}

		if (am_classicmapstring)
		{
			int firstmap;
			int mapoffset = 1;
			switch (gamemission)
			{
			case doom2:
			case commercial_freedoom:
			case commercial_hacx:
				firstmap = GStrings.toIndex(HUSTR_1);
				break;
			case pack_plut:
				firstmap = GStrings.toIndex(PHUSTR_1);
				break;
			case pack_tnt:
				firstmap = GStrings.toIndex(THUSTR_1);
				break;
			default:
				firstmap = GStrings.toIndex(HUSTR_E1M1);
				mapoffset = level.cluster; // Episodes skip map numbers.
				break;
			}

			line = GStrings.getIndex(firstmap + level.levelnum - mapoffset);

			int x, y;
			const int text_width = V_StringWidth(line.c_str()) * CleanXfac;

			if (AM_OverlayAutomapVisible())
			{
				x = surface_width - text_width;
				y = OV_Y - (text_height * 1) + 1;
				if (G_IsHordeMode())
				{
					y -= text_height * 3;
				}
			}
			else
			{
				x = 0;
				y = OV_Y - (text_height * 1) + 1;
			}

			screen->DrawTextClean(CR_RED, x, y, line.c_str());
		}
		else
		{
			if (level.clearlabel)
			{
				line.clear();
			}
			else
			{
				line = TEXTCOLOR_RED;

				// use user provided label if one exists
				if (!level.label.empty())
				{
					line += level.label + ": " + TEXTCOLOR_NORMAL;
				}
				else
				{
					for (int i = 0; i < 8 && level.mapname[i]; i++)
						line += level.mapname[i];
					line += ":" TEXTCOLOR_NORMAL " ";
				}
			}

			line += level.level_name;

			int x, y;
			const int text_width = V_StringWidth(line.c_str()) * CleanXfac;

			if (AM_OverlayAutomapVisible())
			{
				x = surface_width - text_width;
				y = OV_Y - (text_height * 1) + 1;
				if (G_IsHordeMode())
				{
					y -= text_height * 3;
				}
			}
			else
			{
				x = 0;
				y = OV_Y - (text_height * 1) + 1;
			}

			screen->DrawTextClean(CR_GREY, x, y, line.c_str());
		}

		if (am_showtime)
		{
			StrFormat(line, " %02d:%02d:%02d", time / 3600, (time % 3600) / 60,
			        time % 60); // Time

			int x, y;
			const int text_width = V_StringWidth(line.c_str()) * CleanXfac;

			if (AM_OverlayAutomapVisible())
			{
				x = surface_width - text_width;
				y = OV_Y - (text_height * 2) + 1;
			}
			else
			{
				x = surface_width - text_width;
				y = OV_Y - (text_height * 1) + 1;
			}
			if (G_IsHordeMode())
			{
				y -= text_height * 3;
			}

			screen->DrawTextClean(CR_GREY, x, y, line.c_str());
		}
	}
}

VERSION_CONTROL(am_map_cpp, "$Id$")
