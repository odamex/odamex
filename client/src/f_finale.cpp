// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1998-2006 by Randy Heit (ZDoom).
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
//		Game completion, final screen animation.
//
//-----------------------------------------------------------------------------


#include <stdio.h>
#include <ctype.h>
#include <math.h>

#include "i_music.h"
#include "m_swap.h"
#include "z_zone.h"
#include "i_video.h"
#include "v_video.h"
#include "v_text.h"
#include "s_sound.h"
#include "gstrings.h"
#include "doomstat.h"
#include "r_state.h"
#include "hu_stuff.h"
#include "resources/res_main.h"
#include "resources/res_texture.h"

#include "gi.h"

static IWindowSurface* cast_surface = NULL;

// Stage of animation:
//	0 = text, 1 = art screen, 2 = character cast
unsigned int	finalestage;

int	finalecount;

#define TEXTSPEED		2
#define TEXTWAIT		250

static const char*	finaletext;
static const char*	finaleflat;

void	F_StartCast (void);
void	F_CastTicker (void);
BOOL	F_CastResponder (event_t *ev);
void	F_CastDrawer (void);


//
// F_GetWidth
//
// Returns the width of the area that the intermission screen will be
// drawn to. The intermisison screen should be 4:3, except in 320x200 mode.
//
static int F_GetWidth()
{
	int surface_width = I_GetPrimarySurface()->getWidth();
	int surface_height = I_GetPrimarySurface()->getHeight();

	if (I_IsProtectedResolution(I_GetVideoWidth(), I_GetVideoHeight()))
		return surface_width;

	if (surface_width * 3 >= surface_height * 4)
		return surface_height * 4 / 3;
	else
		return surface_width;
}


//
// F_GetHeight
//
// Returns the height of the area that the intermission screen will be
// drawn to. The intermisison screen should be 4:3, except in 320x200 mode.
//
static int F_GetHeight()
{
	int surface_width = I_GetPrimarySurface()->getWidth();
	int surface_height = I_GetPrimarySurface()->getHeight();

	if (I_IsProtectedResolution(I_GetVideoWidth(), I_GetVideoHeight()))
		return surface_height;

	if (surface_width * 3 >= surface_height * 4)
		return surface_height;
	else
		return surface_width * 3 / 4;
}


//
// F_StartFinale
//
void F_StartFinale (char *music, char *flat, const char *text)
{
	gameaction = ga_nothing;
	gamestate = GS_FINALE;
	viewactive = false;

	// Okay - IWAD dependend stuff.
	// This has been changed severly, and
	//	some stuff might have changed in the process.
	// [RH] More flexible now (even more severe changes)
	//  finaleflat, finaletext, and music are now
	//  determined in G_WorldDone() based on data in
	//  a level_info_t and a cluster_info_t.

	if (*music == 0) {
		currentmusic = gameinfo.finaleMusic;
		S_ChangeMusic (currentmusic.c_str(),
			!(gameinfo.flags & GI_NOLOOPFINALEMUSIC));
	}
	else {
		currentmusic = music;			
		S_ChangeMusic (currentmusic.c_str(), !(gameinfo.flags & GI_NOLOOPFINALEMUSIC));
	}

	if (*flat == 0)
		finaleflat = gameinfo.finaleFlat;
	else
		finaleflat = flat;

	if (text)
		finaletext = text;
	else
		finaletext = "Empty message";

	finalestage = 0;
	finalecount = 0;
	S_StopAllChannels ();
}



//
// F_ShutdownFinale
//
// Frees any memory allocated specifically for the finale
//
void STACK_ARGS F_ShutdownFinale()
{
	I_FreeSurface(cast_surface);
}


BOOL F_Responder (event_t *event)
{
	if (finalestage == 2)
		return F_CastResponder (event);

	return false;
}


//
// F_Ticker
//
void F_Ticker (void)
{
	// denis - do this serverside only
	// check for skipping
	// [RH] Non-commercial can be skipped now, too
	if (serverside && finalecount > 50 && finalestage != 1) {
		// go on to the next level
		// [RH] or just reveal the entire message if we're still ticking it
		Players::iterator it = players.begin();
		for (;it != players.end();++it)
			if (it->cmd.buttons)
				break;

		if (it != players.end())
		{
			/*if (finalecount < (signed)(strlen (finaletext)*TEXTSPEED))
			{
				finalecount = strlen (finaletext)*TEXTSPEED;
			}
			else
			{*/
				if (!strncmp (level.nextmap, "EndGame", 7) ||
				(gamemode == retail_chex && !strncmp (level.nextmap, "E1M6", 4)))  	// [ML] Chex mode: game is over
				{																	// after E1M5
					if (level.nextmap[7] == 'C')
					{
						F_StartCast ();
					}
					else
					{
						finalecount = 0;
						finalestage = 1;
						wipegamestate = GS_FORCEWIPE;
						if (level.nextmap[7] == '3')
							S_StartMusic ("d_bunny");
					}
				}
				else
				{
					gameaction = ga_worlddone;
				}
			//}
		}
	}

	// advance animation
	finalecount++;

	if (finalestage == 2)
	{
		F_CastTicker ();
		return;
	}
}



//
// F_TextWrite
//
extern Texture* hu_font[HU_FONTSIZE];


void F_TextWrite (void)
{
	// erase the entire screen to a tiled background
	IWindowSurface* primary_surface = I_GetPrimarySurface();
	primary_surface->clear();		// ensure black background in matted modes

	int width = F_GetWidth();
	int height = F_GetHeight();
	int x = (primary_surface->getWidth() - width) / 2;
	int y = (primary_surface->getHeight() - height) / 2;

	const Texture* background_texture = Res_CacheTexture(finaleflat, FLOOR);
    screen->FlatFill(background_texture, x, y, width + x, height + y);

	V_MarkRect(x, y, width, height);

	// draw some of the text onto the screen
	int cx = 10, cy = 10;
	const char* ch = finaletext;

	if (finalecount < 11)
		return;

	int count = (finalecount - 10) / TEXTSPEED;
	for ( ; count ; count-- )
	{
		int c = *ch++;
		if (!c)
			break;
		if (c == '\n')
		{
			cx = 10;
			cy += 11;
			continue;
		}

		c = toupper(c) - HU_FONTSTART;
		if (c < 0 || c > HU_FONTSIZE)
		{
			cx += 4;
			continue;
		}

		int w = hu_font[c]->mWidth;
		if (cx + w > width)
			break;
		screen->DrawTextureClean(hu_font[c], cx, cy);
		cx += w;
	}

}

//
// Final DOOM 2 animation
// Casting by id Software.
//	 in order of appearance
//
typedef struct
{
	const char		*name;
	mobjtype_t	type;
} castinfo_t;

castinfo_t		castorder[] = {
	{NULL, MT_POSSESSED},
	{NULL, MT_SHOTGUY},
	{NULL, MT_CHAINGUY},
	{NULL, MT_TROOP},
	{NULL, MT_SERGEANT},
	{NULL, MT_SKULL},
	{NULL, MT_HEAD},
	{NULL, MT_KNIGHT},
	{NULL, MT_BRUISER},
	{NULL, MT_BABY},
	{NULL, MT_PAIN},
	{NULL, MT_UNDEAD},
	{NULL, MT_FATSO},
	{NULL, MT_VILE},
	{NULL, MT_SPIDER},
	{NULL, MT_CYBORG},
	{NULL, MT_PLAYER},

	{NULL, MT_UNKNOWNTHING}
};

static int 			castnum;
static int 			casttics;
static int			castsprite;
static state_t*		caststate;
static BOOL	 		castdeath;
static int 			castframes;
static int 			castonmelee;
static BOOL	 		castattacking;


//
// F_StartCast
//
extern	gamestate_t 	wipegamestate;


void F_StartCast (void)
{
	// [RH] Set the names for the cast
	castorder[0].name = GStrings(CC_ZOMBIE);
	castorder[1].name = GStrings(CC_SHOTGUN);
	castorder[2].name = GStrings(CC_HEAVY);
	castorder[3].name = GStrings(CC_IMP);
	castorder[4].name = GStrings(CC_DEMON);
	castorder[5].name = GStrings(CC_LOST);
	castorder[6].name = GStrings(CC_CACO);
	castorder[7].name = GStrings(CC_HELL);
	castorder[8].name = GStrings(CC_BARON);
	castorder[9].name = GStrings(CC_ARACH);
	castorder[10].name = GStrings(CC_PAIN);
	castorder[11].name = GStrings(CC_REVEN);
	castorder[12].name = GStrings(CC_MANCU);
	castorder[13].name = GStrings(CC_ARCH);
	castorder[14].name = GStrings(CC_SPIDER);
	castorder[15].name = GStrings(CC_CYBER);
	castorder[16].name = GStrings(CC_HERO);

	wipegamestate = GS_FORCEWIPE;
	castnum = 0;
	caststate = &states[mobjinfo[castorder[castnum].type].seestate];
	castsprite = caststate->sprite;
	casttics = caststate->tics;
	castdeath = false;
	finalestage = 2;
	castframes = 0;
	castonmelee = 0;
	castattacking = false;
	S_ChangeMusic("d_evil", true);

	if (!cast_surface)
		cast_surface = I_AllocateSurface(320, 200, 8);
}


//
// F_CastTicker
//
void F_CastTicker (void)
{
	int st;
	int atten;

	if (--casttics > 0)
		return; 				// not time to change state yet

	if (caststate->tics == -1 || caststate->nextstate == S_NULL)
	{
		// switch from deathstate to next monster
		castnum++;
		castdeath = false;
		if (castorder[castnum].name == NULL)
			castnum = 0;
		if (mobjinfo[castorder[castnum].type].seesound) {
			atten = ATTN_NONE;
			S_Sound (CHAN_VOICE, mobjinfo[castorder[castnum].type].seesound, 1, atten);
		}
		caststate = &states[mobjinfo[castorder[castnum].type].seestate];
		castsprite = caststate->sprite;
		castframes = 0;
	}
	else
	{
		const char *sfx;

		// just advance to next state in animation
		if (caststate == &states[S_PLAY_ATK1])
			goto stopattack;	// Oh, gross hack!
		st = caststate->nextstate;
		caststate = &states[st];
		castframes++;

		// sound hacks....
		switch (st)
		{
		  case S_PLAY_ATK1: 	sfx = "weapons/sshotf"; break;
		  case S_POSS_ATK2: 	sfx = "grunt/attack"; break;
		  case S_SPOS_ATK2: 	sfx = "shotguy/attack"; break;
		  case S_VILE_ATK2: 	sfx = "vile/start"; break;
		  case S_SKEL_FIST2:	sfx = "skeleton/swing"; break;
		  case S_SKEL_FIST4:	sfx = "skeleton/melee"; break;
		  case S_SKEL_MISS2:	sfx = "skeleton/attack"; break;
		  case S_FATT_ATK8:
		  case S_FATT_ATK5:
		  case S_FATT_ATK2: 	sfx = "fatso/attack"; break;
		  case S_CPOS_ATK2:
		  case S_CPOS_ATK3:
		  case S_CPOS_ATK4: 	sfx = "chainguy/attack"; break;
		  case S_TROO_ATK3: 	sfx = "imp/attack"; break;
		  case S_SARG_ATK2: 	sfx = "demon/melee"; break;
		  case S_BOSS_ATK2:
		  case S_BOS2_ATK2:
		  case S_HEAD_ATK2: 	sfx = "caco/attack"; break;
		  case S_SKULL_ATK2:	sfx = "skull/melee"; break;
		  case S_SPID_ATK2:
		  case S_SPID_ATK3: 	sfx = "spider/attack"; break;
		  case S_BSPI_ATK2: 	sfx = "baby/attack"; break;
		  case S_CYBER_ATK2:
		  case S_CYBER_ATK4:
		  case S_CYBER_ATK6:	sfx = "weapons/rocklf"; break;
		  case S_PAIN_ATK3: 	sfx = "skull/melee"; break;
		  default: sfx = 0; break;
		}

		if (sfx) {
			S_StopAllChannels ();
			S_Sound (CHAN_WEAPON, sfx, 1, ATTN_NONE);
		}
	}

	if (castframes == 12)
	{
		// go into attack frame
		castattacking = true;
		if (castonmelee)
			caststate=&states[mobjinfo[castorder[castnum].type].meleestate];
		else
			caststate=&states[mobjinfo[castorder[castnum].type].missilestate];
		castonmelee ^= 1;
		if (caststate == &states[S_NULL])
		{
			if (castonmelee)
				caststate=
					&states[mobjinfo[castorder[castnum].type].meleestate];
			else
				caststate=
					&states[mobjinfo[castorder[castnum].type].missilestate];
		}
	}

	if (castattacking)
	{
		if (castframes == 24
			||	caststate == &states[mobjinfo[castorder[castnum].type].seestate] )
		{
		  stopattack:
			castattacking = false;
			castframes = 0;
			caststate = &states[mobjinfo[castorder[castnum].type].seestate];
		}
	}

	casttics = caststate->tics;
	if (casttics == -1)
		casttics = 15;
}


//
// F_CastResponder
//

BOOL F_CastResponder (event_t* ev)
{
	if (ev->type != ev_keydown)
		return false;

	if (castdeath)
		return true;					// already in dying frames

	// go into death frame
	castdeath = true;
	caststate = &states[mobjinfo[castorder[castnum].type].deathstate];
	casttics = caststate->tics;
	castframes = 0;
	castattacking = false;
	if (mobjinfo[castorder[castnum].type].deathsound)
		S_Sound (CHAN_VOICE, mobjinfo[castorder[castnum].type].deathsound, 1, ATTN_NONE);

	return true;
}

//
// F_CastDrawer
//
void F_CastDrawer()
{
	IWindowSurface* primary_surface = I_GetPrimarySurface();
	primary_surface->clear();		// ensure black background in matted modes

	const Texture* background_texture = Res_CacheTexture("BOSSBACK", PATCH);

	// draw the background to the surface
	cast_surface->lock();

	cast_surface->getDefaultCanvas()->DrawTexture(background_texture, 0, 0);

	// draw the current frame in the middle of the screen
	const spritedef_t* sprdef = &sprites[castsprite];
	const spriteframe_t* sprframe = &sprdef->spriteframes[caststate->frame & FF_FRAMEMASK];

	const Texture* sprite_texture = Res_CacheTexture(sprframe->resource[0]);
	if (sprframe->flip[0])
		cast_surface->getDefaultCanvas()->DrawTextureFlipped(sprite_texture, 160, 170);
	else
		cast_surface->getDefaultCanvas()->DrawTexture(sprite_texture, 160, 170);

	int width = F_GetWidth();
	int height = F_GetHeight();
	int x = (primary_surface->getWidth() - width) / 2;
	int y = (primary_surface->getHeight() - height) / 2;

	primary_surface->blit(cast_surface, 0, 0, 320, 200, x, y, width, height);

	cast_surface->unlock();

	screen->DrawTextClean(CR_RED,
		x + (width - CleanXfac * V_StringWidth(castorder[castnum].name)) / 2,
		y + (height * 180 / 200),
		castorder[castnum].name);
}


//
// F_DrawPatchCol
//

// Palettized version 8bpp

void F_DrawTextureColP(int x, const Texture* texture, int col)
{
	IWindowSurface* surface = I_GetPrimarySurface();
	int surface_width = surface->getWidth(), surface_height = surface->getHeight();

	// [RH] figure out how many times to repeat this column
	// (for screens wider than 320 pixels)
	float mul = float(surface_width) / 320.0f;
	float fx = (float)x;
	int repeat = (int)(floor(mul*(fx+1)) - floor(mul*fx));
	if (repeat == 0)
		return;

	// [RH] Remap virtual-x to real-x
	x = (int)floor(mul*x);

	// [RH] Figure out per-row fixed-point step
	unsigned int step = (200<<16) / surface_height;
	unsigned int invstep = (surface_height<<16) / 200;

	
	byte* desttop = surface->getBuffer() + x;
	int pitch = surface->getPitchInPixels();

	// step through the posts in a column

	const byte* source = texture->getColumn(col);
	byte* dest = desttop;
	unsigned int count = (texture->mHeight * invstep) >> 16;
	int c = 0;
	palindex_t p;

	switch (repeat) {
		case 1:
			do {
				*dest = source[c>>16];
				dest += pitch;
				c += step;
			} while (--count);
			break;
		case 2:
			do {
				p = source[c>>16];
				dest[0] = p;
				dest[1] = p;
				dest += pitch;
				c += step;
			} while (--count);
			break;
		case 3:
			do {
				p = source[c>>16];
				dest[0] = p;
				dest[1] = p;
				dest[2] = p;
				dest += pitch;
				c += step;
			} while (--count);
			break;
		case 4:
			do {
				p = source[c>>16];
				dest[0] = p;
				dest[1] = p;
				dest[2] = p;
				dest[3] = p;
				dest += pitch;
				c += step;
			} while (--count);
			break;
		default:
			{
				int count2;

				do {
					p = source[c>>16];
					for (count2 = repeat; count2; count2--) {
						dest[count2] = p;
					}
					dest += pitch;
					c += step;
				} while (--count);
			}
			break;
	}
}

// Direct version 32bpp:

void F_DrawTextureColD(int x, const Texture* texture, int col)
{
	IWindowSurface* surface = I_GetPrimarySurface();
	int surface_width = surface->getWidth(), surface_height = surface->getHeight();

	// [RH] figure out how many times to repeat this column
	// (for screens wider than 320 pixels)
	float mul = float(surface_width) / 320.0f;
	float fx = (float)x;
	int repeat = (int)(floor(mul*(fx+1)) - floor(mul*fx));
	if (repeat == 0)
		return;

	// [RH] Remap virtual-x to real-x
	x = (int)floor(mul*x);

	// [RH] Figure out per-row fixed-point step
	unsigned step = (200<<16) / surface_height;
	unsigned invstep = (surface_height<<16) / 200;

	argb_t* desttop = (argb_t *)surface->getBuffer() + x;
	int pitch = surface->getPitchInPixels();

	shaderef_t pal = shaderef_t(&V_GetDefaultPalette()->maps, 0);

	const byte* source = texture->getColumn(col);
	argb_t* dest = desttop;
	unsigned count = (texture->mHeight * invstep) >> 16;
	int c = 0;
	argb_t p;

	switch (repeat) {
		case 1:
			do {
				*dest = pal.shade(source[c>>16]);
				dest += pitch;
				c += step;
			} while (--count);
			break;
		case 2:
			do {
				p = pal.shade(source[c>>16]);
				dest[0] = p;
				dest[1] = p;
				dest += pitch;
				c += step;
			} while (--count);
			break;
		case 3:
			do {
				p = pal.shade(source[c>>16]);
				dest[0] = p;
				dest[1] = p;
				dest[2] = p;
				dest += pitch;
				c += step;
			} while (--count);
			break;
		case 4:
			do {
				p = pal.shade(source[c>>16]);
				dest[0] = p;
				dest[1] = p;
				dest[2] = p;
				dest[3] = p;
				dest += pitch;
				c += step;
			} while (--count);
			break;
		default:
			{
				int count2;

				do {
					p = pal.shade(source[c>>16]);
					for (count2 = repeat; count2; count2--) {
						dest[count2] = p;
					}
					dest += pitch;
					c += step;
				} while (--count);
			}
			break;
	}
}


//
// F_BunnyScroll
//
void F_BunnyScroll (void)
{
	int 		scrolled;
	int 		x;
	char		name[10];
	int 		stage;
	static int	laststage;

	const Texture* texture1 = Res_CacheTexture("PFUB2", PATCH);
	const Texture* texture2 = Res_CacheTexture("PFUB1", PATCH);

	V_MarkRect (0, 0, I_GetSurfaceWidth(), I_GetSurfaceHeight());

	scrolled = 320 - (finalecount-230)/2;
	if (scrolled > 320)
		scrolled = 320;
	if (scrolled < 0)
		scrolled = 0;

	if (I_GetPrimarySurface()->getBitsPerPixel() == 8)
	{
		for ( x=0 ; x<320 ; x++)
		{
			if (x+scrolled < 320)
				F_DrawTextureColP(x, texture1, x+scrolled);
			else
				F_DrawTextureColP(x, texture2, x+scrolled - 320);
		}
	}
	else
	{
		for ( x=0 ; x<320 ; x++)
		{
			if (x+scrolled < 320)
				F_DrawTextureColD(x, texture1, x+scrolled);
			else
				F_DrawTextureColD(x, texture2, x+scrolled - 320);
		}
	}

	if (finalecount < 1130)
		return;

	if (finalecount < 1180)
	{
		const Texture* end_texture = Res_CacheTexture("END0", PATCH);
		screen->DrawTextureIndirect(end_texture, (320-13*8)/2, (200-8*8)/2);
		laststage = 0;
		return;
	}

	stage = (finalecount-1180) / 5;
	if (stage > 6)
		stage = 6;
	if (stage > laststage)
	{
		S_Sound(CHAN_WEAPON, "weapons/pistol", 1, ATTN_NONE);
		laststage = stage;
	}

	sprintf (name,"END%i",stage);
	const Texture* end_texture = Res_CacheTexture(name, PATCH);
	screen->DrawTextureIndirect(end_texture, (320-13*8)/2, (200-8*8)/2);
}


//
// F_Drawer
//
void F_Drawer (void)
{
	switch (finalestage)
	{
		case 0:
			F_TextWrite ();
			break;

		case 1:
			switch (level.nextmap[7])
			{
				default:
				case '1':
				{
					const Texture* texture = Res_CacheTexture(gameinfo.finalePage1, PATCH);
					screen->DrawTextureIndirect(texture, 0, 0);
					break;
				}
				case '2':
				{
					const Texture* texture = Res_CacheTexture(gameinfo.finalePage2, PATCH);
					screen->DrawTextureIndirect(texture, 0, 0);
					break;
				}
				case '3':
				{
					F_BunnyScroll ();
					break;
				}
				case '4':
				{
					const Texture* texture = Res_CacheTexture(gameinfo.finalePage3, PATCH);
					screen->DrawTextureIndirect(texture, 0, 0);
					break;
				}
			}
			break;

		case 2:
			F_CastDrawer ();
			break;
	}
}

VERSION_CONTROL (f_finale_cpp, "$Id$")

