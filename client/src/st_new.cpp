// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1998-2006 by Randy Heit (ZDoom 1.22).
// Copyright (C) 2000-2006 by Sergey Makovkin (CSDoom .62).
// Copyright (C) 2006-2010 by The Odamex Team.
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
//	Additional status bar stuff
//
//-----------------------------------------------------------------------------


#include <stdio.h>
#include <stdlib.h>

#include "doomtype.h"
#include "doomdef.h"
#include "doomstat.h"
#include "d_items.h"
#include "v_video.h"
#include "v_text.h"
#include "w_wad.h"
#include "z_zone.h"
#include "i_system.h"
#include "m_swap.h"
#include "st_stuff.h"
#include "c_cvars.h"
#include "p_ctf.h"

static int		widestnum, numheight;
static const patch_t	*medi;
static const patch_t	*armors[2];
static const patch_t	*ammos[4];
static const patch_t	*flagiconbcur;
static const patch_t	*flagiconrcur;
static const patch_t	*flagiconbhome;
static const patch_t	*flagiconrhome;
static const patch_t	*flagiconbtakenbyb;
static const patch_t	*flagiconbtakenbyr;
static const patch_t	*flagiconrtakenbyb;
static const patch_t	*flagiconrtakenbyr;
static const patch_t	*flagicongtakenbyb;
static const patch_t	*flagicongtakenbyr;
static const patch_t	*flagiconbdropped;
static const patch_t	*flagiconrdropped;
static const char		ammopatches[4][8] = { "CLIPA0", "SHELA0", "CELLA0", "ROCKA0" };
static int		NameUp = -1;

extern patch_t	*sttminus;
extern patch_t	*tallnum[10];
extern patch_t	*faces[];
extern int		st_faceindex;
extern patch_t	*keys[NUMCARDS+NUMCARDS/2];
extern byte		*Ranges;
extern flagdata CTFdata[NUMFLAGS];

EXTERN_CVAR (hud_scale)

void ST_unloadNew (void)
{
	int i;

	Z_ChangeTag (medi, PU_CACHE);

	Z_ChangeTag (flagiconbcur, PU_CACHE);
	Z_ChangeTag (flagiconrcur, PU_CACHE);
	Z_ChangeTag (flagiconbhome, PU_CACHE);
	Z_ChangeTag (flagiconrhome, PU_CACHE);
	Z_ChangeTag (flagiconbtakenbyb, PU_CACHE);
	Z_ChangeTag (flagiconbtakenbyr, PU_CACHE);
	Z_ChangeTag (flagiconrtakenbyb, PU_CACHE);
	Z_ChangeTag (flagiconrtakenbyr, PU_CACHE);
	Z_ChangeTag (flagicongtakenbyb, PU_CACHE);
	Z_ChangeTag (flagicongtakenbyr, PU_CACHE);
	Z_ChangeTag (flagiconbdropped, PU_CACHE);
	Z_ChangeTag (flagiconrdropped, PU_CACHE);

	for (i = 0; i < 2; i++)
		Z_ChangeTag (armors[i], PU_CACHE);

	for (i = 0; i < 4; i++)
		Z_ChangeTag (ammos[i], PU_CACHE);
}

void ST_initNew (void)
{
	int i;
	int widest = 0;
	char name[8];
	int lump;

	// denis - todo - security - these patches have unchecked dimensions
	// ie, if a patch has a 0 width/height, it may cause a divide by zero
	// somewhere else in the code. we download wads, so this is an issue!

	for (i = 0; i < 10; i++) {
		if (tallnum[i]->width() > widest)
			widest = tallnum[i]->width();
	}

	strcpy (name, "ARM1A0");
	for (i = 0; i < 2; i++) {
		name[3] = i + '1';
		if ((lump = W_CheckNumForName (name, ns_sprites)) != -1)
			armors[i] = W_CachePatch (lump, PU_STATIC);
	}

	for (i = 0; i < 4; i++) {
		if ((lump = W_CheckNumForName (ammopatches[i], ns_sprites)) != -1)
			ammos[i] = W_CachePatch (lump, PU_STATIC);
	}

	if ((lump = W_CheckNumForName ("MEDIA0", ns_sprites)) != -1)
		medi = W_CachePatch (lump, PU_STATIC);

	flagiconbcur = W_CachePatch ("FLAGICOB", PU_STATIC);
	flagiconrcur = W_CachePatch ("FLAGICOR", PU_STATIC);
	flagiconbhome = W_CachePatch ("FLAGIC2B", PU_STATIC);
	flagiconrhome = W_CachePatch ("FLAGIC2R", PU_STATIC);
	flagiconbtakenbyb = W_CachePatch ("FLAGI3BB", PU_STATIC);
	flagiconbtakenbyr = W_CachePatch ("FLAGI3BR", PU_STATIC);
	flagiconrtakenbyb = W_CachePatch ("FLAGI3RB", PU_STATIC);
	flagiconrtakenbyr = W_CachePatch ("FLAGI3RR", PU_STATIC);
	flagiconbdropped = W_CachePatch ("FLAGIC4B", PU_STATIC);
	flagiconrdropped = W_CachePatch ("FLAGIC4R", PU_STATIC);

	widestnum = widest;
	numheight = tallnum[0]->height();

	if (multiplayer && (sv_gametype == GM_COOP || demoplayback || !netgame) && level.time)
		NameUp = level.time + 2*TICRATE;
}

void ST_DrawNum (int x, int y, DCanvas *scrn, int num)
{
	char digits[8], *d;

	if (num < 0)
	{
		if (hud_scale)
		{
			scrn->DrawLucentPatchCleanNoMove (sttminus, x, y);
			x += CleanXfac * sttminus->width();
		}
		else
		{
			scrn->DrawLucentPatch (sttminus, x, y);
			x += sttminus->width();
		}
		num = -num;
	}

	sprintf (digits, "%d", num);

	d = digits;
	while (*d)
	{
		if (*d >= '0' && *d <= '9')
		{
			if (hud_scale)
			{
				scrn->DrawLucentPatchCleanNoMove (tallnum[*d - '0'], x, y);
				x += CleanXfac * tallnum[*d - '0']->width();
			}
			else
			{
				scrn->DrawLucentPatch (tallnum[*d - '0'], x, y);
				x += tallnum[*d - '0']->width();
			}
		}
		d++;
	}
}

void ST_DrawNumRight (int x, int y, DCanvas *scrn, int num)
{
	int d = abs(num);
	int xscale = hud_scale ? CleanXfac : 1;

	do {
		x -= tallnum[d%10]->width() * xscale;
	} while (d /= 10);

	if (num < 0)
		x -= sttminus->width() * xscale;

	ST_DrawNum (x, y, scrn, num);
}

void ST_newDraw (void)
{
	player_t *plyr = &consoleplayer();
	int y, i;
	ammotype_t ammo = weaponinfo[plyr->readyweapon].ammo;
	int xscale = hud_scale ? CleanXfac : 1;
	int yscale = hud_scale ? CleanYfac : 1;

	y = screen->height - (numheight + 4) * yscale;

	// Draw health
	if (hud_scale)
		screen->DrawLucentPatchCleanNoMove (medi, 20 * CleanXfac,
									  screen->height - 2*CleanYfac);
	else
		screen->DrawLucentPatch (medi, 20, screen->height - 2);
	ST_DrawNum (40 * xscale, y, screen, plyr->health);

	// Draw armor
	if (plyr->armortype && plyr->armorpoints)
	{
		const patch_t *current_armor = armors[1];
		if(plyr->armortype == 1)
			current_armor = armors[0];
		
		if (current_armor)
		{
			if (hud_scale)
				screen->DrawLucentPatchCleanNoMove (current_armor, 20 * CleanXfac, y - 4*CleanYfac);
			else
				screen->DrawLucentPatch (current_armor, 20, y - 4);
		}
		ST_DrawNum (40*xscale, y - (armors[0]->height()+3)*yscale,
					 screen, plyr->armorpoints);
	}

	// Draw ammo
	if (ammo < NUMAMMO)
	{
		const patch_t *ammopatch = ammos[weaponinfo[plyr->readyweapon].ammo];

		if (hud_scale)
			screen->DrawLucentPatchCleanNoMove (ammopatch,
										  screen->width - 14 * CleanXfac,
										  screen->height - 4 * CleanYfac);
		else
			screen->DrawLucentPatch (ammopatch, screen->width - 14,
							   screen->height - 4);
		ST_DrawNumRight (screen->width - 25 * xscale, y, screen,
						 plyr->ammo[ammo]);
	}

	// Draw top-right info. (Keys/Frags/Score)
    if (sv_gametype == GM_CTF)
    {
		ST_newDrawCTF();
    }
	else if (sv_gametype != GM_COOP)
	{
		// Draw frags (in DM)
		ST_DrawNumRight (screen->width - (2 * xscale), 2 * yscale, screen, plyr->fragcount);
	}
	else
	{
		// Draw keys (not DM)
		y = CleanYfac;
		for (i = 0; i < 6; i++)
		{
			if (plyr->cards[i])
			{
				if (hud_scale)
					screen->DrawLucentPatchCleanNoMove (keys[i], screen->width - 10*CleanXfac, y);
				else
					screen->DrawLucentPatch (keys[i], screen->width - 10, y);
				y += (8 + (i < 3 ? 0 : 2)) * yscale;
			}
		}
	}
}

void ST_newDrawCTF (void)
{
	player_t *plyr = &consoleplayer();
	int xscale = hud_scale ? CleanXfac : 1;
	int yscale = hud_scale ? CleanYfac : 1;
	const patch_t *flagbluepatch = flagiconbhome;
	const patch_t *flagredpatch = flagiconrhome;

	switch(CTFdata[it_blueflag].state)
	{
		case flag_carried:
			if (CTFdata[it_blueflag].flagger) {
				player_t &player = idplayer(CTFdata[it_blueflag].flagger);
				if (player.userinfo.team == TEAM_BLUE)
					flagbluepatch = flagiconbtakenbyb;
				else if (player.userinfo.team == TEAM_RED)
					flagbluepatch = flagiconbtakenbyr;
			}
			break;
		case flag_dropped:
			flagbluepatch = flagiconbdropped;
			break;
		default:
			break;
	}

	switch(CTFdata[it_redflag].state)
	{
		case flag_carried:
			if (CTFdata[it_redflag].flagger) {
				player_t &player = idplayer(CTFdata[it_redflag].flagger);
				if (player.userinfo.team == TEAM_BLUE)
					flagredpatch = flagiconrtakenbyb;
				else if (player.userinfo.team == TEAM_RED)
					flagredpatch = flagiconrtakenbyr;
			}
			break;
		case flag_dropped:
			flagredpatch = flagiconrdropped;
			break;
		default:
			break;
	}

	// Draw score (in CTF)
	if (hud_scale) {

		if (plyr->userinfo.team == TEAM_BLUE)
			screen->DrawLucentPatchCleanNoMove (flagiconbcur,
										  screen->width - 19 * CleanXfac,
										  1 * CleanYfac);
		else if (plyr->userinfo.team == TEAM_RED)
			screen->DrawLucentPatchCleanNoMove (flagiconrcur,
										  screen->width - 19 * CleanXfac,
										  19 * CleanYfac);

		screen->DrawLucentPatchCleanNoMove (flagbluepatch,
									  screen->width - 18 * CleanXfac,
									  2 * CleanYfac);
		screen->DrawLucentPatchCleanNoMove (flagredpatch,
									  screen->width - 18 * CleanXfac,
									  20 * CleanYfac);
	} else {

		if (plyr->userinfo.team == TEAM_BLUE)
			screen->DrawLucentPatch (flagiconbcur, screen->width - 19,
							   1);
		else if (plyr->userinfo.team == TEAM_RED)
			screen->DrawLucentPatch (flagiconrcur, screen->width - 19,
							   19);

		screen->DrawLucentPatch (flagbluepatch, screen->width - 18,
						   2);
		screen->DrawLucentPatch (flagredpatch, screen->width - 18,
						   20);
	}

	ST_DrawNumRight (screen->width - 20 * xscale, 2 * yscale, screen, TEAMpoints[TEAM_BLUE]);
	ST_DrawNumRight (screen->width - 20 * xscale, 20 * yscale, screen, TEAMpoints[TEAM_RED]);
}

void ST_nameDraw (int y)
{
	player_t *plyr = &displayplayer();

	if (plyr == &consoleplayer())
		return;
	
	char *string = plyr->userinfo.netname;
	size_t x = (screen->width - V_StringWidth (string)*CleanXfac) >> 1;

	if (level.time < NameUp)
		screen->DrawTextClean (CR_GREEN, x, y, string);
	else
		screen->DrawTextCleanLuc (CR_GREEN, x, y, string);
}

VERSION_CONTROL (st_new_cpp, "$Id$")
