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

#include "z_zone.h"
#include "st_stuff.h"
#include "p_local.h"
#include "p_lnspec.h"
#include "w_wad.h"
#include "v_palette.h"
#include "c_bind.h"

#include "m_cheat.h"
#include "c_dispatch.h"
#include "cl_demo.h"
#include "g_gametype.h"

// Needs access to LFB.
#include "i_video.h"
#include "v_video.h"

#include "v_text.h"

// State.
#include "r_state.h"

// Data.
#include "gstrings.h"

#include "am_map.h"
#include "p_mapformat.h"

argb_t CL_GetPlayerColor(player_t*);

EXTERN_CVAR(am_followplayer)

// Group palette index and RGB value together:
typedef struct am_color_s {
	palindex_t  index;
	argb_t      rgb;
} am_color_t;

static am_color_t
	Background, YourColor, WallColor, TSWallColor,
		   FDWallColor, CDWallColor, ThingColor,
		   SecretWallColor, GridColor, XHairColor,
		   NotSeenColor,
		   LockedColor,
		   AlmostBackground,
		   TeleportColor, ExitColor;

static int lockglow = 0;

EXTERN_CVAR (am_rotate)
EXTERN_CVAR (am_overlay)
EXTERN_CVAR (am_showsecrets)
EXTERN_CVAR (am_showmonsters)
EXTERN_CVAR (am_showtime)
EXTERN_CVAR (am_classicmapstring)
EXTERN_CVAR (am_usecustomcolors)
EXTERN_CVAR (am_ovshare)

EXTERN_CVAR (am_backcolor)
EXTERN_CVAR (am_yourcolor)
EXTERN_CVAR (am_wallcolor)
EXTERN_CVAR (am_tswallcolor)
EXTERN_CVAR (am_fdwallcolor)
EXTERN_CVAR (am_cdwallcolor)
EXTERN_CVAR (am_thingcolor)
EXTERN_CVAR (am_gridcolor)
EXTERN_CVAR (am_xhaircolor)
EXTERN_CVAR (am_notseencolor)
EXTERN_CVAR (am_lockedcolor)
EXTERN_CVAR (am_exitcolor)
EXTERN_CVAR (am_teleportcolor)

EXTERN_CVAR (am_ovyourcolor)
EXTERN_CVAR (am_ovwallcolor)
EXTERN_CVAR (am_ovtswallcolor)
EXTERN_CVAR (am_ovfdwallcolor)
EXTERN_CVAR (am_ovcdwallcolor)
EXTERN_CVAR (am_ovthingcolor)
EXTERN_CVAR (am_ovgridcolor)
EXTERN_CVAR (am_ovxhaircolor)
EXTERN_CVAR (am_ovnotseencolor)
EXTERN_CVAR (am_ovlockedcolor)
EXTERN_CVAR (am_ovexitcolor)
EXTERN_CVAR (am_ovteleportcolor)

BEGIN_COMMAND (resetcustomcolors)
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
    Printf (PRINT_HIGH, "Custom automap colors reset to default.\n");
}
END_COMMAND (resetcustomcolors)

EXTERN_CVAR		(screenblocks)

// drawing stuff
#define	FB		(screen)

#define AM_NUMMARKPOINTS 10

// scale on entry
#define INITSCALEMTOF (.2*FRACUNIT)
// how much the automap moves window per tic in frame-buffer coordinates
// moves 140 pixels in 1 second
#define F_PANINC	(140/TICRATE)
// how much zoom-in per tic
// goes to 2x in 1 second
#define M_ZOOMIN        ((int) (1.02*FRACUNIT))
// how much zoom-out per tic
// pulls out to 0.5x in 1 second
#define M_ZOOMOUT       ((int) (FRACUNIT/1.02))

// translates between frame-buffer and map distances
#define FTOM(x) FixedMul(((x)<<16),scale_ftom)
#define MTOF(x) (FixedMul((x),scale_mtof)>>16)
// translates between frame-buffer and map coordinates
#define CXMTOF(x)  (MTOF((x)-m_x)/* - f_x*/)
#define CYMTOF(y)  (f_h - MTOF((y)-m_y)/* + f_y*/)

#define PUTDOTP(xx,yy,cc) fb[(yy)*f_p+(xx)]=(cc)
#define PUTDOTD(xx,yy,cc) *((argb_t *)(fb+(yy)*f_p+((xx)<<2)))=(cc)

typedef struct {
	int x, y;
} fpoint_t;

typedef struct {
	fpoint_t a, b;
} fline_t;

typedef struct {
	fixed_t x,y;
} mpoint_t;

typedef struct {
	mpoint_t a, b;
} mline_t;

typedef struct {
	fixed_t slp, islp;
} islope_t;

// backdrop
static IWindowSurface* am_backdrop;
bool am_gotbackdrop = false;



//
// The vector graphics for the automap.
//  A line drawing of the player pointing right,
//   starting from the middle.
//
#define R ((8*PLAYERRADIUS)/7)
mline_t player_arrow[] = {
	{ { -R+R/8, 0 }, { R, 0 } }, // -----
	{ { R, 0 }, { R-R/2, R/4 } },  // ----->
	{ { R, 0 }, { R-R/2, -R/4 } },
	{ { -R+R/8, 0 }, { -R-R/8, R/4 } }, // >---->
	{ { -R+R/8, 0 }, { -R-R/8, -R/4 } },
	{ { -R+3*R/8, 0 }, { -R+R/8, R/4 } }, // >>--->
	{ { -R+3*R/8, 0 }, { -R+R/8, -R/4 } }
};
#undef R
#define NUMPLYRLINES (ARRAY_LENGTH(player_arrow))

#define R ((8*PLAYERRADIUS)/7)
mline_t cheat_player_arrow[] = {
	{ { -R+R/8, 0 }, { R, 0 } }, // -----
	{ { R, 0 }, { R-R/2, R/6 } },  // ----->
	{ { R, 0 }, { R-R/2, -R/6 } },
	{ { -R+R/8, 0 }, { -R-R/8, R/6 } }, // >----->
	{ { -R+R/8, 0 }, { -R-R/8, -R/6 } },
	{ { -R+3*R/8, 0 }, { -R+R/8, R/6 } }, // >>----->
	{ { -R+3*R/8, 0 }, { -R+R/8, -R/6 } },
	{ { -R/2, 0 }, { -R/2, -R/6 } }, // >>-d--->
	{ { -R/2, -R/6 }, { -R/2+R/6, -R/6 } },
	{ { -R/2+R/6, -R/6 }, { -R/2+R/6, R/4 } },
	{ { -R/6, 0 }, { -R/6, -R/6 } }, // >>-dd-->
	{ { -R/6, -R/6 }, { 0, -R/6 } },
	{ { 0, -R/6 }, { 0, R/4 } },
	{ { R/6, R/4 }, { R/6, -R/7 } }, // >>-ddt->
	{ { R/6, -R/7 }, { R/6+R/32, -R/7-R/32 } },
	{ { R/6+R/32, -R/7-R/32 }, { R/6+R/10, -R/7 } }
};
#undef R
#define NUMCHEATPLYRLINES (ARRAY_LENGTH(cheat_player_arrow))

#define R (FRACUNIT)
// [RH] Avoid lots of warnings without compiler-specific #pragmas
#define L(a,b,c,d) { {(fixed_t)((a)*R),(fixed_t)((b)*R)}, {(fixed_t)((c)*R),(fixed_t)((d)*R)} }
mline_t triangle_guy[] = {
	L (-.867,-.5, .867,-.5),
	L (.867,-.5, 0,1),
	L (0,1, -.867,-.5)
};
#define NUMTRIANGLEGUYLINES (ARRAY_LENGTH(triangle_guy))

mline_t thintriangle_guy[] = {
	L (-.5,-.7, 1,0),
	L (1,0, -.5,.7),
	L (-.5,.7, -.5,-.7)
};
#undef L
#undef R
#define NUMTHINTRIANGLEGUYLINES (ARRAY_LENGTH(thintriangle_guy))

int am_cheating = 0;
static int 	grid = 0;
static int	bigstate = 0;	// Bigmode

static int 	leveljuststarted = 1; 	// kluge until AM_LevelInit() is called



bool automapactive = false;

// location of window on screen
static int	f_x;
static int	f_y;

// size of window on screen
static int	f_w;
static int	f_h;
static int	f_p;				// [RH] # of bytes from start of a line to start of next

static byte *fb;				// pseudo-frame buffer
static int	amclock;

static mpoint_t	m_paninc;		// how far the window pans each tic (map coords)
static fixed_t	ftom_zoommul;	// how far the window zooms in each tic (fb coords)

static fixed_t	m_x, m_y;		// LL x,y where the window is on the map (map coords)
static fixed_t	m_x2, m_y2;		// UR x,y where the window is on the map (map coords)

//
// width/height of window on map (map coords)
//
static fixed_t	m_w;
static fixed_t	m_h;

// based on level size
static fixed_t	min_x;
static fixed_t	min_y;
static fixed_t	max_x;
static fixed_t	max_y;

static fixed_t	max_w; // max_x-min_x,
static fixed_t	max_h; // max_y-min_y

// based on player size
static fixed_t	min_w;
static fixed_t	min_h;


static fixed_t	min_scale_mtof; // used to tell when to stop zooming out
static fixed_t	max_scale_mtof; // used to tell when to stop zooming in

// old stuff for recovery later
static fixed_t old_m_w, old_m_h;
static fixed_t old_m_x, old_m_y;

// old location used by the Follower routine
static mpoint_t f_oldloc;

// used by MTOF to scale from map-to-frame-buffer coords
static fixed_t scale_mtof = (fixed_t)INITSCALEMTOF;
// used by FTOM to scale from frame-buffer-to-map coords (=1/scale_mtof)
static fixed_t scale_ftom;

static lumpHandle_t marknums[10]; // numbers used for marking by the automap
static mpoint_t markpoints[AM_NUMMARKPOINTS]; // where the points are
static int markpointnum = 0; // next point to be assigned

static bool followplayer = true; // specifies whether to follow the player around

static BOOL stopped = true;

extern NetDemo netdemo;

void AM_clearMarks();
void AM_addMark();
void AM_saveScaleAndLoc();
void AM_restoreScaleAndLoc();
void AM_minOutWindowScale();

#define NUMALIASES		3
#define WALLCOLORS		-1
#define FDWALLCOLORS	-2
#define CDWALLCOLORS	-3

#define WEIGHTBITS		6
#define WEIGHTSHIFT		(FRACBITS-WEIGHTBITS)
#define NUMWEIGHTS		(1<<WEIGHTBITS)
#define WEIGHTMASK		(NUMWEIGHTS-1)

BEGIN_COMMAND(am_grid)
{
	grid = !grid;
	Printf(PRINT_HIGH, "%s\n", grid ? GStrings(AMSTR_GRIDON) : GStrings(AMSTR_GRIDOFF));
} END_COMMAND(am_grid)


BEGIN_COMMAND(am_setmark)
{
	AM_addMark();
	Printf(PRINT_HIGH, "%s %d\n", GStrings(AMSTR_MARKEDSPOT), markpointnum);
} END_COMMAND(am_setmark)

BEGIN_COMMAND(am_clearmarks)
{
	AM_clearMarks();
	Printf(PRINT_HIGH, "%s\n", GStrings(AMSTR_MARKSCLEARED));
} END_COMMAND(am_clearmarks)

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
} END_COMMAND(am_big)

BEGIN_COMMAND(am_togglefollow)
{
	am_followplayer = !am_followplayer;
	f_oldloc.x = MAXINT;
	Printf(PRINT_HIGH, "%s\n", am_followplayer ? GStrings(AMSTR_FOLLOWON) : GStrings(AMSTR_FOLLOWOFF));
} END_COMMAND(am_togglefollow)

void AM_rotatePoint (fixed_t *x, fixed_t *y);

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
	m_x += m_w/2;
	m_y += m_h/2;
	m_w = FTOM(f_w);
	m_h = FTOM(f_h);
	m_x -= m_w/2;
	m_y -= m_h/2;
	m_x2 = m_x + m_w;
	m_y2 = m_y + m_h;
}

//
//
//
void AM_saveScaleAndLoc()
{
	old_m_x = m_x;
	old_m_y = m_y;
	old_m_w = m_w;
	old_m_h = m_h;
}

//
//
//
void AM_restoreScaleAndLoc()
{
	m_w = old_m_w;
	m_h = old_m_h;
	if (!am_followplayer)
	{
		m_x = old_m_x;
		m_y = old_m_y;
    }
	else
	{
		m_x = displayplayer().camera->x - m_w/2;
		m_y = displayplayer().camera->y - m_h/2;
    }
	m_x2 = m_x + m_w;
	m_y2 = m_y + m_h;

	// Change the scaling multipliers
	scale_mtof = FixedDiv(f_w<<FRACBITS, m_w);
	scale_ftom = FixedDiv(FRACUNIT, scale_mtof);
}

//
// adds a marker at the current location
//
void AM_addMark()
{
	markpoints[markpointnum].x = m_x + m_w/2;
	markpoints[markpointnum].y = m_y + m_h/2;
	markpointnum = (markpointnum + 1) % AM_NUMMARKPOINTS;
}

//
// Determines bounding box of all vertices,
// sets global variables controlling zoom range.
//
void AM_findMinMaxBoundaries()
{
	min_x = min_y =  MAXINT;
	max_x = max_y = -MAXINT;

	for (int i = 0;i<numvertexes;i++) {
		if (vertexes[i].x < min_x)
			min_x = vertexes[i].x;
		else if (vertexes[i].x > max_x)
			max_x = vertexes[i].x;

		if (vertexes[i].y < min_y)
			min_y = vertexes[i].y;
		else if (vertexes[i].y > max_y)
			max_y = vertexes[i].y;
	}

	max_w = max_x - min_x;
	max_h = max_y - min_y;

	min_w = 2*PLAYERRADIUS; // const? never changed?
	min_h = 2*PLAYERRADIUS;

	const fixed_t a = FixedDiv((I_GetSurfaceWidth()) << FRACBITS, max_w);
	const fixed_t b = FixedDiv((I_GetSurfaceHeight()) << FRACBITS, max_h);

	min_scale_mtof = a < b ? a : b;
	max_scale_mtof = FixedDiv((I_GetSurfaceHeight())<<FRACBITS, 2*PLAYERRADIUS);
}


//
//
//
void AM_changeWindowLoc()
{
	if (m_paninc.x || m_paninc.y) {
		am_followplayer.Set(0.0f);
		f_oldloc.x = MAXINT;
	}

	m_x += m_paninc.x;
	m_y += m_paninc.y;

	if (m_x + m_w/2 > max_x)
		m_x = max_x - m_w/2;
	else if (m_x + m_w/2 < min_x)
		m_x = min_x - m_w/2;

	if (m_y + m_h/2 > max_y)
		m_y = max_y - m_h/2;
	else if (m_y + m_h/2 < min_y)
		m_y = min_y - m_h/2;

	m_x2 = m_x + m_w;
	m_y2 = m_y + m_h;
}


//
//
//
void AM_initVariables()
{
	static event_t st_notify(ev_keyup, AM_MSGENTERED, 0, 0);

	automapactive = true;

	f_oldloc.x = MAXINT;
	amclock = 0;

	m_w = FTOM(I_GetSurfaceWidth());
	m_h = FTOM(I_GetSurfaceHeight());

	// find player to center on initially
	player_t *pl = &displayplayer();
	if (!pl->ingame())
	{
		for (Players::iterator it = players.begin();it != players.end();++it)
		{
			if (it->ingame())
			{
				pl = &*it;
				break;
			}
		}
	}

	if(!pl->camera)
		return;

	m_x = pl->camera->x - m_w/2;
	m_y = pl->camera->y - m_h/2;
	AM_changeWindowLoc();

	// for saving & restoring
	old_m_x = m_x;
	old_m_y = m_y;
	old_m_w = m_w;
	old_m_h = m_h;

	// inform the status bar of the change
	ST_Responder (&st_notify);
}


am_color_t AM_GetColorFromString(const argb_t* palette_colors, const char* colorstring)
{
	am_color_t c;
	c.rgb = V_GetColorFromString(colorstring);
	c.index = V_BestColor(palette_colors, c.rgb);
	return c;
}

am_color_t AM_BestColor(const argb_t* palette_colors, const int r, const int g, const int b)
{
	am_color_t c;
	c.rgb = argb_t(r,g,b);
	c.index = V_BestColor(palette_colors, c.rgb);
	return c;
}

void AM_initColors (BOOL overlayed)
{
	// Look up the colors in the current palette:
	const argb_t* palette_colors = V_GetDefaultPalette()->colors;

	if (overlayed && !am_ovshare)
	{
		YourColor = AM_GetColorFromString (palette_colors, am_ovyourcolor.cstring());
		SecretWallColor =
			WallColor= AM_GetColorFromString (palette_colors, am_ovwallcolor.cstring());
		TSWallColor = AM_GetColorFromString (palette_colors, am_ovtswallcolor.cstring());
		FDWallColor = AM_GetColorFromString (palette_colors, am_ovfdwallcolor.cstring());
		CDWallColor = AM_GetColorFromString (palette_colors, am_ovcdwallcolor.cstring());
		ThingColor = AM_GetColorFromString (palette_colors, am_ovthingcolor.cstring());
		GridColor = AM_GetColorFromString (palette_colors, am_ovgridcolor.cstring());
		XHairColor = AM_GetColorFromString (palette_colors, am_ovxhaircolor.cstring());
		NotSeenColor = AM_GetColorFromString (palette_colors, am_ovnotseencolor.cstring());
		LockedColor = AM_GetColorFromString (palette_colors, am_ovlockedcolor.cstring());
		ExitColor = AM_GetColorFromString (palette_colors, am_ovexitcolor.cstring());
		TeleportColor = AM_GetColorFromString (palette_colors, am_ovteleportcolor.cstring());
	}
	else if (am_usecustomcolors || (overlayed && am_ovshare))
	{
		/* Use the custom colors in the am_* cvars */
		Background = AM_GetColorFromString(palette_colors, am_backcolor.cstring());
		YourColor = AM_GetColorFromString(palette_colors, am_yourcolor.cstring());
		SecretWallColor =
			WallColor = AM_GetColorFromString(palette_colors, am_wallcolor.cstring());
		TSWallColor = AM_GetColorFromString(palette_colors, am_tswallcolor.cstring());
		FDWallColor = AM_GetColorFromString(palette_colors, am_fdwallcolor.cstring());
		CDWallColor = AM_GetColorFromString(palette_colors, am_cdwallcolor.cstring());
		ThingColor = AM_GetColorFromString(palette_colors, am_thingcolor.cstring());
		GridColor = AM_GetColorFromString(palette_colors, am_gridcolor.cstring());
		XHairColor = AM_GetColorFromString(palette_colors, am_xhaircolor.cstring());
		NotSeenColor = AM_GetColorFromString(palette_colors, am_notseencolor.cstring());
		LockedColor = AM_GetColorFromString(palette_colors, am_lockedcolor.cstring());
		ExitColor = AM_GetColorFromString(palette_colors, am_exitcolor.cstring());
		TeleportColor = AM_GetColorFromString(palette_colors, am_teleportcolor.cstring());
		{
			argb_t ba = AM_GetColorFromString(palette_colors, am_backcolor.cstring()).rgb;

			if (ba.getr() < 16)
				ba.setr(ba.getr() + 32);
			if (ba.getg() < 16)
				ba.setg(ba.getg() + 32);
			if (ba.getb() < 16)
				ba.setb(ba.getb() + 32);

			AlmostBackground.rgb = argb_t(ba.getr() - 16, ba.getg() - 16, ba.getb() - 16);
			AlmostBackground.index = V_BestColor(palette_colors, AlmostBackground.rgb);
		}
	}
	else
	{
		switch (gamemission)
		{
		case heretic:
			/* Use colors corresponding to the original Heretic's */
			Background = AM_GetColorFromString(palette_colors, "00 00 00");
			YourColor = AM_GetColorFromString(palette_colors, "FF FF FF");
			AlmostBackground = AM_GetColorFromString(palette_colors, "10 10 10");
			SecretWallColor = WallColor = AM_GetColorFromString(palette_colors, "4c 33 11");
			TSWallColor = AM_GetColorFromString(palette_colors, "59 5e 57");
			FDWallColor = AM_GetColorFromString(palette_colors, "d0 b0 85");
			LockedColor = AM_GetColorFromString(palette_colors, "fc fc 00");
			CDWallColor = AM_GetColorFromString(palette_colors, "68 3c 20");
			ThingColor = AM_GetColorFromString(palette_colors, "38 38 38");
			GridColor = AM_GetColorFromString(palette_colors, "4c 4c 4c");
			XHairColor = AM_GetColorFromString(palette_colors, "80 80 80");
			NotSeenColor = AM_GetColorFromString(palette_colors, "6c 6c 6c");
			break;

		default:
			/* Use colors corresponding to the original Doom's */
			Background = AM_GetColorFromString(palette_colors, "00 00 00");
			YourColor = AM_GetColorFromString(palette_colors, "FF FF FF");
			AlmostBackground = AM_GetColorFromString(palette_colors, "10 10 10");
			SecretWallColor = WallColor = AM_GetColorFromString(palette_colors, "fc 00 00");
			TSWallColor = AM_GetColorFromString(palette_colors, "80 80 80");
			FDWallColor = AM_GetColorFromString(palette_colors, "bc 78 48");
			LockedColor = AM_GetColorFromString(palette_colors, "fc fc 00");
			CDWallColor = AM_GetColorFromString(palette_colors, "fc fc 00");
			ThingColor = AM_GetColorFromString(palette_colors, "74 fc 6c");
			GridColor = AM_GetColorFromString(palette_colors, "4c 4c 4c");
			XHairColor = AM_GetColorFromString(palette_colors, "80 80 80");
			NotSeenColor = AM_GetColorFromString(palette_colors, "6c 6c 6c");
			break;
		}
	}
}

//
//
//
void AM_loadPics()
{
	if (gamemission == heretic)
	{
		for (int i = 0; i < 10; i++)
		{
			char namebuf[9];
			sprintf(namebuf, "SMALLIN%d", i);
			marknums[i] = W_CachePatchHandle(namebuf, PU_STATIC);
		}
	}
	else
	{
		for (int i = 0; i < 10; i++)
		{
			char namebuf[9];
			sprintf(namebuf, "AMMNUM%d", i);
			marknums[i] = W_CachePatchHandle(namebuf, PU_STATIC);
		}
	}

	// haleyjd 12/22/02: automap background support (raw format)
	// [ML] 9/25/10: Heavily modified to use our canvases.
	if ((W_CheckNumForName("AUTOPAGE")) != -1)
	{
		if (!am_gotbackdrop)
		{
			// allocate backdrop
			void* bg = W_CacheLumpName("AUTOPAGE", PU_STATIC);
			delete am_backdrop;

			am_backdrop = I_AllocateSurface(320, 158, 8);

			am_backdrop->lock();
			am_backdrop->getDefaultCanvas()->DrawBlock((byte*)bg, 0, 0, 320, 158);
			am_backdrop->unlock();

			am_gotbackdrop = true;
		}
	}
}

void AM_unloadPics()
{
	for (int i = 0; i < 10; i++)
	{
		marknums[i].clear();
	}

	// haleyjd 12/22/02: backdrop support
	if (am_backdrop && am_gotbackdrop)
	{
		I_FreeSurface(am_backdrop);
		am_backdrop = NULL;
		am_gotbackdrop = false;
	}
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
	leveljuststarted = 0;
	am_cheating = 0; // force-reset IDDT after loading a map

	AM_clearMarks();

	AM_findMinMaxBoundaries();
	scale_mtof = FixedDiv(min_scale_mtof, (int) (0.7*FRACUNIT));
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

	AM_unloadPics ();
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

	// todo: rather than call this every time automap is opened, do this once on level init
	static char lastmap[8] = "";
	if (level.mapname != lastmap)
	{
		AM_LevelInit();
		strncpy(lastmap, level.mapname.c_str(), 8);
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


BEGIN_COMMAND (togglemap)
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
END_COMMAND (togglemap)

//
// Handle events (user inputs) in automap mode
//
BOOL AM_Responder(event_t *ev)
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
			// If this is a release event we also need to check if it released a button in the main Bindings
			// so that that button does not get stuck.
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
	static fixed_t	mtof_zoommul;	// how far the window zooms in each tic (map coords)

	if (Actions[ACTION_AUTOMAP_ZOOMOUT]) {
		mtof_zoommul = M_ZOOMOUT;
		ftom_zoommul = M_ZOOMIN;
	}
	else if (Actions[ACTION_AUTOMAP_ZOOMIN]) {
		mtof_zoommul = M_ZOOMIN;
		ftom_zoommul = M_ZOOMOUT;
	}
	else {
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
	player_t &p = displayplayer();

    if (f_oldloc.x != p.camera->x ||
		f_oldloc.y != p.camera->y)
	{
		m_x = FTOM(MTOF(p.camera->x)) - m_w/2;
		m_y = FTOM(MTOF(p.camera->y)) - m_h/2;
		m_x2 = m_x + m_w;
		m_y2 = m_y + m_h;
		f_oldloc.x = p.camera->x;
		f_oldloc.y = p.camera->y;
	}
}

//
// Updates on Game Tick
//
void AM_Ticker()
{
	if (!automapactive)
		return;

	amclock++;

	if (am_followplayer) {
		AM_doFollowPlayer();
	}
	else {
		m_paninc.x = 0;
		m_paninc.y = 0;

		// pan according to the direction
		if (Actions[ACTION_AUTOMAP_PANLEFT])m_paninc.x = -FTOM(F_PANINC);
		if (Actions[ACTION_AUTOMAP_PANRIGHT])m_paninc.x = FTOM(F_PANINC);
		if (Actions[ACTION_AUTOMAP_PANUP])m_paninc.y = FTOM(F_PANINC);
		if (Actions[ACTION_AUTOMAP_PANDOWN])m_paninc.y = -FTOM(F_PANINC);
	}

	// Change the zoom if necessary
	if (ftom_zoommul != FRACUNIT || Actions[ACTION_AUTOMAP_ZOOMIN] || Actions[ACTION_AUTOMAP_ZOOMOUT])
		AM_changeWindowScale();

	// Change x,y location
	if (m_paninc.x || m_paninc.y)
		AM_changeWindowLoc();

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
	if (am_gotbackdrop && am_backdrop)
	{
		m_w = FTOM(I_GetSurfaceWidth());
		m_h = FTOM(I_GetSurfaceHeight()) - ST_HEIGHT;

		I_GetPrimarySurface()->blit(am_backdrop, 0, 0, am_backdrop->getWidth(),
		                      am_backdrop->getHeight(), 0, 0, m_w, m_h);
	}
	else
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
			argb_t* line = (argb_t*)fb;

			for (int y = 0; y < f_h; y++)
			{
				for (int x = 0; x < f_w; x++)
					line[x] = color.rgb;
				line += f_p >> 2;
			}
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
bool AM_clipMline (mline_t *ml, fline_t *fl)
{
	enum {
		LEFT	=1,
		RIGHT	=2,
		BOTTOM	=4,
		TOP	=8
	};

	int outcode1 = 0;
	int outcode2 = 0;
	int outside;

	fpoint_t tmp = {0, 0};
	int dx;
	int dy;


#define DOOUTCODE(oc, mx, my) \
	(oc) = 0; \
	if ((my) < 0) (oc) |= TOP; \
	else if ((my) >= f_h) (oc) |= BOTTOM; \
	if ((mx) < 0) (oc) |= LEFT; \
	else if ((mx) >= f_w) (oc) |= RIGHT;

	// do trivial rejects and outcodes
	if (ml->a.y > m_y2)
		outcode1 = TOP;
	else if (ml->a.y < m_y)
		outcode1 = BOTTOM;

	if (ml->b.y > m_y2)
		outcode2 = TOP;
	else if (ml->b.y < m_y)
		outcode2 = BOTTOM;

	if (outcode1 & outcode2)
		return false; // trivially outside

	if (ml->a.x < m_x)
		outcode1 |= LEFT;
	else if (ml->a.x > m_x2)
		outcode1 |= RIGHT;

	if (ml->b.x < m_x)
		outcode2 |= LEFT;
	else if (ml->b.x > m_x2)
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

	while (outcode1 | outcode2) {
		// may be partially inside box
		// find an outside point
		if (outcode1)
			outside = outcode1;
		else
			outside = outcode2;

		// clip to each side
		if (outside & TOP) {
			dy = fl->a.y - fl->b.y;
			dx = fl->b.x - fl->a.x;
			tmp.x = fl->a.x + (dx*(fl->a.y))/dy;
			tmp.y = 0;
		} else if (outside & BOTTOM) {
			dy = fl->a.y - fl->b.y;
			dx = fl->b.x - fl->a.x;
			tmp.x = fl->a.x + (dx*(fl->a.y-f_h))/dy;
			tmp.y = f_h-1;
		} else if (outside & RIGHT) {
			dy = fl->b.y - fl->a.y;
			dx = fl->b.x - fl->a.x;
			tmp.y = fl->a.y + (dy*(f_w-1 - fl->a.x))/dx;
			tmp.x = f_w-1;
		} else if (outside & LEFT) {
			dy = fl->b.y - fl->a.y;
			dx = fl->b.x - fl->a.x;
			tmp.y = fl->a.y + (dy*(-fl->a.x))/dx;
			tmp.x = 0;
		}

		if (outside == outcode1) {
			fl->a = tmp;
			DOOUTCODE(outcode1, fl->a.x, fl->a.y);
		} else {
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

void AM_drawFlineP (fline_t* fl, byte color)
{
	fl->a.x += f_x;
	fl->b.x += f_x;
	fl->a.y += f_y;
	fl->b.y += f_y;

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
		d = ax - ay / 2;

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
	fl->a.x += f_x;
	fl->b.x += f_x;
	fl->a.y += f_y;
	fl->b.y += f_y;

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
		d = ay - ax/2;

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
		d = ax - ay/2;

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
	mline_t ml;

	// Figure out start of vertical gridlines
	fixed_t start = m_x;
	if ((start-bmaporgx)%(MAPBLOCKUNITS<<FRACBITS))
		start += (MAPBLOCKUNITS<<FRACBITS)
			- ((start-bmaporgx)%(MAPBLOCKUNITS<<FRACBITS));
	fixed_t end = m_x + m_w;

	// draw vertical gridlines
	ml.a.y = m_y;
	ml.b.y = m_y+m_h;
	for (fixed_t x = start; x<end; x+=(MAPBLOCKUNITS<<FRACBITS)) {
		ml.a.x = x;
		ml.b.x = x;
		if (am_rotate) {
			AM_rotatePoint (&ml.a.x, &ml.a.y);
			AM_rotatePoint (&ml.b.x, &ml.b.y);
		}
		AM_drawMline(&ml, color);
	}

	// Figure out start of horizontal gridlines
	start = m_y;
	if ((start-bmaporgy)%(MAPBLOCKUNITS<<FRACBITS))
		start += (MAPBLOCKUNITS<<FRACBITS)
			- ((start-bmaporgy)%(MAPBLOCKUNITS<<FRACBITS));
	end = m_y + m_h;

	// draw horizontal gridlines
	ml.a.x = m_x;
	ml.b.x = m_x + m_w;
	for (fixed_t y = start; y<end; y+=(MAPBLOCKUNITS<<FRACBITS)) {
		ml.a.y = y;
		ml.b.y = y;
		if (am_rotate) {
			AM_rotatePoint (&ml.a.x, &ml.a.y);
			AM_rotatePoint (&ml.b.x, &ml.b.y);
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

	for (int i = 0;i<numlines;i++) {
		l.a.x = lines[i].v1->x;
		l.a.y = lines[i].v1->y;
		l.b.x = lines[i].v2->x;
		l.b.y = lines[i].v2->y;

		if (am_rotate) {
			AM_rotatePoint (&l.a.x, &l.a.y);
			AM_rotatePoint (&l.b.x, &l.b.y);
		}

		if (am_cheating || (lines[i].flags & ML_MAPPED))
		{
			if ((lines[i].flags & ML_DONTDRAW) && !am_cheating)
				continue;
            if (!lines[i].backsector &&
                (((am_usecustomcolors || viewactive) &&
                P_IsExitLine(lines[i].special)) ||
                (!am_usecustomcolors && !viewactive)))
            {
				AM_drawMline(&l, WallColor);
			}
			else
			{
				if ((P_IsTeleportLine(lines[i].special)) &&
					(am_usecustomcolors || viewactive))
				{ // teleporters
					AM_drawMline(&l, TeleportColor);
				}
				else if ((P_IsExitLine(lines[i].special)) &&
						 (am_usecustomcolors || viewactive))
				{ // exit
					AM_drawMline(&l, ExitColor);
				}
				else if (lines[i].flags & ML_SECRET)
				{ // secret door
					if (am_cheating)
						AM_drawMline(&l, SecretWallColor);
				    else
						AM_drawMline(&l, WallColor);
				}
				else if (lines[i].backsector->floorheight
					  != lines[i].frontsector->floorheight)
				{
					AM_drawMline(&l, FDWallColor); // floor level change
				}
				else if (lines[i].backsector->ceilingheight
					  != lines[i].frontsector->ceilingheight)
				{
					AM_drawMline(&l, CDWallColor); // ceiling level change
				}
				else if (am_cheating)
				{
					AM_drawMline(&l, TSWallColor);
				}

				if (map_format.getZDoom())
				{
					if (lines[i].special == Door_LockedRaise)
					{
						// NES - Locked doors glow from a predefined color to either blue,
						// yellow, or red.
						r = LockedColor.rgb.getr(), g = LockedColor.rgb.getg(),
						b = LockedColor.rgb.getb();

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
								r += (int)rdif * lockglow;
								g += (int)gdif * lockglow;
								b += (int)bdif * lockglow;
							}
							else if (lockglow < 60)
							{
								r += (int)rdif * (60 - lockglow);
								g += (int)gdif * (60 - lockglow);
								b += (int)bdif * (60 - lockglow);
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
						r = LockedColor.rgb.getr(), g = LockedColor.rgb.getg(),
						b = LockedColor.rgb.getb();

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
								r += (int)rdif * lockglow;
								g += (int)gdif * lockglow;
								b += (int)bdif * lockglow;
							}
							else if (lockglow < 60)
							{
								r += (int)rdif * (60 - lockglow);
								g += (int)gdif * (60 - lockglow);
								b += (int)bdif * (60 - lockglow);
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
				AM_drawMline(&l, NotSeenColor);
		}
    }
}


//
// Rotation in 2D.
// Used to rotate player arrow line character.
//
void AM_rotate(fixed_t* x, fixed_t* y, angle_t a)
{
	const fixed_t tmpx = FixedMul(*x, finecosine[a >> ANGLETOFINESHIFT])
	                     - FixedMul(*y, finesine[a >> ANGLETOFINESHIFT]);

    *y   =
	FixedMul(*x,finesine[a>>ANGLETOFINESHIFT])
	+ FixedMul(*y,finecosine[a>>ANGLETOFINESHIFT]);

    *x = tmpx;
}

void AM_rotatePoint(fixed_t *x, fixed_t *y)
{
	player_t &p = displayplayer();

	*x -= p.camera->x;
	*y -= p.camera->y;
	AM_rotate (x, y, ANG90 - p.camera->angle);
	*x += p.camera->x;
	*y += p.camera->y;
}

void
AM_drawLineCharacter
( mline_t*	lineguy,
  int		lineguylines,
  fixed_t	scale,
  angle_t	angle,
  am_color_t	color,
  fixed_t	x,
  fixed_t	y )
{
	mline_t	l;

	for (int i = 0;i<lineguylines;i++) {
		l.a.x = lineguy[i].a.x;
		l.a.y = lineguy[i].a.y;

		if (scale) {
			l.a.x = FixedMul(scale, l.a.x);
			l.a.y = FixedMul(scale, l.a.y);
		}

		if (angle)
			AM_rotate(&l.a.x, &l.a.y, angle);

		l.a.x += x;
		l.a.y += y;

		l.b.x = lineguy[i].b.x;
		l.b.y = lineguy[i].b.y;

		if (scale) {
			l.b.x = FixedMul(scale, l.b.x);
			l.b.y = FixedMul(scale, l.b.y);
		}

		if (angle)
			AM_rotate(&l.b.x, &l.b.y, angle);

		l.b.x += x;
		l.b.y += y;

		AM_drawMline(&l, color);
	}
}

void AM_drawPlayers()
{
	angle_t angle;
	player_t &conplayer = displayplayer();
	const argb_t* palette = V_GetDefaultPalette()->colors;

	if (!multiplayer)
	{
		if (am_rotate)
			angle = ANG90;
		else
			angle = conplayer.camera->angle;

		if (am_cheating)
			AM_drawLineCharacter
			(cheat_player_arrow, NUMCHEATPLYRLINES, 0,
			 angle, YourColor, conplayer.camera->x, conplayer.camera->y);
		else
			AM_drawLineCharacter
			(player_arrow, NUMPLYRLINES, 0, angle,
			 YourColor, conplayer.camera->x, conplayer.camera->y);
		return;
	}

	for (Players::iterator it = players.begin();it != players.end();++it)
	{
		player_t *p = &*it;
		am_color_t color;
		mpoint_t pt;

		if (!(it->ingame()) || !p->mo ||
			(((G_IsFFAGame() && p != &conplayer) ||
			(G_IsTeamGame() && p->userinfo.team != conplayer.userinfo.team))
			&& !(netdemo.isPlaying() || netdemo.isPaused())
			&& !demoplayback && !(conplayer.spectator)) || p->spectator)
		{
			continue;
		}

		if (p->powers[pw_invisibility])
		{
			color = AlmostBackground;
		}
		else if (demoplayback)
		{
			switch (it->id) {
				case 1: color = AM_GetColorFromString (palette, "00 FF 00"); break;
				case 2: color = AM_GetColorFromString (palette, "60 60 B0"); break;
				case 3: color = AM_GetColorFromString (palette, "B0 B0 30"); break;
				case 4: color = AM_GetColorFromString (palette, "C0 00 00"); break;
				default: break;
			}
		}
		else
		{
			color.rgb = CL_GetPlayerColor(p);
			color.index = V_BestColor(V_GetDefaultPalette()->basecolors, color.rgb);
		}

		pt.x = p->mo->x;
		pt.y = p->mo->y;
		angle = p->mo->angle;

		if (am_rotate)
		{
			AM_rotatePoint (&pt.x, &pt.y);
			angle -= conplayer.camera->angle - ANG90;
		}

		AM_drawLineCharacter
			(player_arrow, NUMPLYRLINES, 0, angle,
			 color, pt.x, pt.y);
    }
}

void AM_drawThings(am_color_t color)
{
	mpoint_t p;

	for (int i = 0;i<numsectors;i++)
	{
		AActor* t = sectors[i].thinglist;
		while (t)
		{
			p.x = t->x;
			p.y = t->y;
			angle_t angle = t->angle;

			if (am_rotate)
			{
				AM_rotatePoint (&p.x, &p.y);
				angle += ANG90 - displayplayer().camera->angle;
			}

			AM_drawLineCharacter(thintriangle_guy, NUMTHINTRIANGLEGUYLINES, 16<<FRACBITS, angle, color, p.x, p.y);
			t = t->snext;
		}
	}
}

void AM_drawMarks()
{
	mpoint_t pt;

	for (int i = 0; i < AM_NUMMARKPOINTS; i++)
	{
		if (markpoints[i].x != -1)
		{
			//      w = LESHORT(marknums[i]->width);
			//      h = LESHORT(marknums[i]->height);
			const int w = 5; // because something's wrong with the wad, i guess
			const int h = 6; // because something's wrong with the wad, i guess

			pt.x = markpoints[i].x;
			pt.y = markpoints[i].y;

			if (am_rotate)
				AM_rotatePoint (&pt.x, &pt.y);

			const int fx = CXMTOF(pt.x);
			const int fy = CYMTOF(pt.y) - 3;

			if (fx >= f_x && fx <= f_w - w && fy >= f_y && fy <= f_h - h)
			{
				FB->DrawPatchCleanNoMove(W_ResolvePatchHandle(marknums[i]), fx, fy);
			}
		}
	}
}

void AM_drawCrosshair(am_color_t color)
{
	// single point for now
	if (I_GetPrimarySurface()->getBitsPerPixel() == 8)
		PUTDOTP(f_w/2, (f_h+1)/2, (byte)color.index);
	else
		PUTDOTD(f_w/2, (f_h+1)/2, color.rgb);
}


//
// AM_drawWidgets
//
void AM_drawWidgets()
{
	const IWindowSurface* surface = I_GetPrimarySurface();
	const int surface_width = surface->getWidth();
	const int surface_height = surface->getHeight();

	if (!(viewactive && am_overlay < 2) && !hu_font[0].empty())
	{
		char line[64+10];
		const int time = level.time / TICRATE;
		int wl_x1 = 0;
		int wr_x1 = 0;
		int wl_x2 = 0;
		int wr_x2 = 0;

		const int text_height = (W_ResolvePatchHandle(hu_font[0])->height() + 1) * CleanYfac;
		const int OV_Y = surface_height - (surface_height * 32 / 200);

		if (gamemission == heretic)
		{
			wl_x1 = wr_x1 = 36;
			wl_x2 = wr_x2 = 36;
		}

		if (G_IsCoopGame())
		{
			if (am_showmonsters)
			{
				sprintf(line, TEXTCOLOR_RED "MONSTERS:"
								TEXTCOLOR_NORMAL " %d / %d",
								level.killed_monsters, (level.total_monsters+level.respawned_monsters));

				int x, y;
				const int text_width = V_StringWidth(line) * CleanXfac;

				if (AM_OverlayAutomapVisible())
					x = surface_width - text_width, y = OV_Y - (text_height * 4) + 1;
				else
					x = (wl_x1 * CleanXfac), y = OV_Y - (text_height * 2) + 1;

				FB->DrawTextClean(CR_GREY, x, y, line);
			}

			if (am_showsecrets)
			{
				sprintf(line, TEXTCOLOR_RED "SECRETS:"
								TEXTCOLOR_NORMAL " %d / %d",
								level.found_secrets, level.total_secrets);
				int x, y;
				const int text_width = (V_StringWidth(line) + wr_x1) * CleanXfac;

				if (AM_OverlayAutomapVisible())
					x = surface_width - text_width, y = OV_Y - (text_height * 3) + 1;
				else
					x = surface_width - text_width, y = OV_Y - (text_height * 2) + 1;

				FB->DrawTextClean(CR_GREY, x, y, line);
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
				case heretic:
				    firstmap = GStrings.toIndex(HHUSTR_E1M1);
				    mapoffset = level.cluster; // Episodes skip map numbers.
				    break;
				default:
					firstmap = GStrings.toIndex(HUSTR_E1M1);
					mapoffset = level.cluster; // Episodes skip map numbers.
					break;
			}

			strncpy(line, GStrings.getIndex(firstmap + level.levelnum - mapoffset),
			        ARRAY_LENGTH(line) - 1);

			int x, y;
			const int text_width = V_StringWidth(line) * CleanXfac;

			if (AM_OverlayAutomapVisible())
				x = surface_width - text_width, y = OV_Y - (text_height * 1) + 1;
			else
				x = (wl_x2 * CleanXfac), y = OV_Y - (text_height * 1) + 1;

			FB->DrawTextClean(CR_RED, x, y, line);
		}
		else
		{
			strcpy(line, TEXTCOLOR_RED);
			size_t pos = strlen(line);
			for (int i = 0; i < 8 && level.mapname[i]; i++, pos++)
				line[pos] = level.mapname[i];

			line[pos++] = ':';
			strcpy(line + pos, TEXTCOLOR_NORMAL);
			pos = strlen(line);
			line[pos++] = ' ';
			strcpy(&line[pos], level.level_name);

			int x, y;
			const int text_width = V_StringWidth(line) * CleanXfac;

			if (AM_OverlayAutomapVisible())
				x = surface_width - text_width, y = OV_Y - (text_height * 1) + 1;
			else
				x = (wl_x2 * CleanXfac), y = OV_Y - (text_height * 1) + 1;

			FB->DrawTextClean(CR_GREY, x, y, line);
		}

		if (am_showtime)
		{
			sprintf(line, " %02d:%02d:%02d", time/3600, (time%3600)/60, time%60);	// Time

			int x, y;
			const int text_width = (V_StringWidth(line) + wl_x2) * CleanXfac;

			if (AM_OverlayAutomapVisible())
				x = surface_width - text_width, y = OV_Y - (text_height * 2) + 1;
			else
				x = surface_width - text_width, y = OV_Y - (text_height * 1) + 1;

			FB->DrawTextClean(CR_GREY, x, y, line);
		}
	}
}

//
// AM_Drawer
//
void AM_Drawer()
{
	if (!AM_ClassicAutomapVisible() && !AM_OverlayAutomapVisible())
		return;

	IWindowSurface* surface = I_GetPrimarySurface();
	const int surface_width = surface->getWidth();
	const int surface_height = surface->getHeight();

	fb = surface->getBuffer();

	if (AM_ClassicAutomapVisible())
	{
		f_x = f_y = 0;
		f_w = surface_width;
		f_h = ST_StatusBarY(surface_width, surface_height);
		f_p = surface->getPitch();

		AM_clearFB(Background);
	}
	else
	{
		f_x = R_ViewWindowX(surface_width, surface_height);
		f_y = R_ViewWindowY(surface_width, surface_height);
		f_w = R_ViewWidth(surface_width, surface_height);
		f_h = R_ViewHeight(surface_width, surface_height);
		f_p = surface->getPitch();
	}

	AM_activateNewScale();

	if (grid)
		AM_drawGrid(GridColor);

	AM_drawWalls();
	AM_drawPlayers();
	if (am_cheating == 2)
		AM_drawThings(ThingColor);

	if (!(viewactive && am_overlay < 2))
		AM_drawCrosshair(XHairColor);

	AM_drawMarks();
	AM_drawWidgets();
}

VERSION_CONTROL (am_map_cpp, "$Id$")
