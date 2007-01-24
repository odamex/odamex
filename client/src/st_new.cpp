// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1998-2006 by Randy Heit (ZDoom 1.22).
// Copyright (C) 2000-2006 by Sergey Makovkin (CSDoom .62).
// Copyright (C) 2006-2007 by The Odamex Team.
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

static int		widestnum, numheight;
static const patch_t	*medi;
static const patch_t	*armors[2];
static const patch_t	*ammos[4];
static const char		ammopatches[4][8] = { "CLIPA0", "SHELA0", "CELLA0", "ROCKA0" };
static int		NameUp = -1;

extern patch_t	*sttminus;
extern patch_t	*tallnum[10];
extern patch_t	*faces[];
extern int		st_faceindex;
extern patch_t	*keys[NUMCARDS+NUMCARDS/2];
extern byte		*Ranges;

CVAR (hud_scale, "0", CVAR_ARCHIVE)

void ST_unloadNew (void)
{
	int i;

	Z_ChangeTag (medi, PU_CACHE);

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
			armors[i] = (patch_t *)W_CacheLumpNum (lump, PU_STATIC);
	}

	for (i = 0; i < 4; i++) {
		if ((lump = W_CheckNumForName (ammopatches[i], ns_sprites)) != -1)
			ammos[i] = (patch_t *)W_CacheLumpNum (lump, PU_STATIC);
	}

	if ((lump = W_CheckNumForName ("MEDIA0", ns_sprites)) != -1)
		medi = (patch_t *)W_CacheLumpNum (lump, PU_STATIC);

	widestnum = widest;
	numheight = tallnum[0]->height();

	if (multiplayer && (!deathmatch || demoplayback || !netgame) && level.time)
		NameUp = level.time + 2*TICRATE;
}

void ST_DrawNum (int x, int y, DCanvas *scrn, int num)
{
	char digits[8], *d;

	if (num < 0)
	{
		if (hud_scale)
		{
			scrn->DrawPatchCleanNoMove (sttminus, x, y);
			x += CleanXfac * sttminus->width();
		}
		else
		{
			scrn->DrawPatch (sttminus, x, y);
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
				scrn->DrawPatchCleanNoMove (tallnum[*d - '0'], x, y);
				x += CleanXfac * tallnum[*d - '0']->width();
			}
			else
			{
				scrn->DrawPatch (tallnum[*d - '0'], x, y);
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
		screen->DrawPatchCleanNoMove (medi, 20 * CleanXfac,
									  screen->height - 2*CleanYfac);
	else
		screen->DrawPatch (medi, 20, screen->height - 2);
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
				screen->DrawPatchCleanNoMove (current_armor, 20 * CleanXfac, y - 4*CleanYfac);
			else
				screen->DrawPatch (current_armor, 20, y - 4);
		}
		ST_DrawNum (40*xscale, y - (armors[0]->height()+3)*yscale,
					 screen, plyr->armorpoints);
	}

	// Draw ammo
	if (ammo < NUMAMMO)
	{
		const patch_t *ammopatch = ammos[weaponinfo[plyr->readyweapon].ammo];

		if (hud_scale)
			screen->DrawPatchCleanNoMove (ammopatch,
										  screen->width - 14 * CleanXfac,
										  screen->height - 4 * CleanYfac);
		else
			screen->DrawPatch (ammopatch, screen->width - 14,
							   screen->height - 4);
		ST_DrawNumRight (screen->width - 25 * xscale, y, screen,
						 plyr->ammo[ammo]);
	}

	if (deathmatch)
	{
		// Draw frags (in DM)
		ST_DrawNumRight (screen->width - 2, 1, screen, plyr->fragcount);
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
					screen->DrawPatchCleanNoMove (keys[i], screen->width - 10*CleanXfac, y);
				else
					screen->DrawPatch (keys[i], screen->width - 10, y);
				y += (8 + (i < 3 ? 0 : 2)) * yscale;
			}
		}
	}
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

