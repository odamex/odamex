// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1998-2006 by Randy Heit (ZDoom).
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
//		Game completion, final screen animation.
//
//-----------------------------------------------------------------------------


#include "odamex.h"

#include "f_finale.h"

#include <ctype.h>
#include <math.h>

#include "i_music.h"
#include "z_zone.h"
#include "i_video.h"
#include "v_video.h"
#include "v_text.h"
#include "w_wad.h"
#include "s_sound.h"
#include "gstrings.h"
#include "r_state.h"
#include "hu_stuff.h"

#include "gi.h"

static IWindowSurface* cast_surface = NULL;

// Stage of animation:
//	0 = text, 1 = art screen, 2 = character cast
unsigned int	finalestage;

int	finalecount;

#define TEXTSPEED		2
#define TEXTWAIT		250

enum finale_lump_t
{
	FINALE_NONE,
	FINALE_FLAT,
	FINALE_GRAPHIC,
};

const char* finaletext;
const char* finalelump;
finale_lump_t finalelumptype = FINALE_NONE;

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
	const int surface_width = I_GetPrimarySurface()->getWidth();
	const int surface_height = I_GetPrimarySurface()->getHeight();

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
	const int surface_width = I_GetPrimarySurface()->getWidth();
	const int surface_height = I_GetPrimarySurface()->getHeight();

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
void F_StartFinale(finale_options_t& options)
{
	::gameaction = ga_nothing;
	::gamestate = GS_FINALE;
	::viewactive = false;

	// Okay - IWAD dependend stuff.
	// This has been changed severly, and
	//	some stuff might have changed in the process.
	// [RH] More flexible now (even more severe changes)
	//  finaleflat, finaletext, and music are now
	//  determined in G_WorldDone() based on data in
	//  a level_info_t and a cluster_info_t.

	if (options.music == NULL)
	{
		::currentmusic = ::gameinfo.finaleMusic.c_str();
		S_ChangeMusic(
			::currentmusic.c_str(),
			!(::gameinfo.flags & GI_NOLOOPFINALEMUSIC)
		);
	}
	else
	{
		::currentmusic = options.music;
		S_ChangeMusic(
			::currentmusic,
			!(::gameinfo.flags & GI_NOLOOPFINALEMUSIC)
		);
	}

	if (options.pic != NULL)
	{
		::finalelumptype = FINALE_GRAPHIC;
		::finalelump = options.pic;
	}
	else if (options.flat != NULL)
	{
		::finalelumptype = FINALE_FLAT;
		::finalelump = options.flat;
	}
	else
	{
		::finalelumptype = FINALE_FLAT;
		::finalelump = gameinfo.finaleFlat.c_str();
	}

	if (options.text)
	{
		::finaletext = options.text;
	}
	else
	{
		::finaletext = "In the quiet following your last battle,\n"
			"you suddenly get the feeling that something is\n"
			"...missing.  Like there was supposed to be intermission\n"
			" text here, but somehow it couldn't be found.\n"
			"\n"
			"No matter.  You ready your weapon and continue on \n"
			"into the chaos.";
	}

	::finalestage = 0;
	::finalecount = 0;
	S_StopAllChannels();
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
void F_Ticker()
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
				if (!strnicmp (level.nextmap.c_str(), "EndGame", 7))
				{
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
		F_CastTicker();
	}
}



//
// F_TextWrite
//

void F_TextWrite ()
{
	// Don't draw text without a working font.
	if (::hu_font[0].empty())
		return;

	// erase the entire screen to a tiled background
	IWindowSurface* primary_surface = I_GetPrimarySurface();
	primary_surface->clear();		// ensure black background in matted modes

	const int width = F_GetWidth();
	const int height = F_GetHeight();
	const int x = (primary_surface->getWidth() - width) / 2;
	const int y = (primary_surface->getHeight() - height) / 2;

	int lump;
	switch (finalelumptype)
	{
	case FINALE_GRAPHIC:
		lump = W_CheckNumForName(finalelump, ns_global);
		if (lump >= 0)
		{
			screen->DrawPatchFullScreen((patch_t*)W_CachePatch(lump, PU_CACHE));
		}
		break;
	case FINALE_FLAT:
		lump = W_CheckNumForName(finalelump, ns_flats);
		if (lump >= 0)
		{
			screen->FlatFill(x, y, width + x, height + y, (byte*)W_CacheLumpNum(lump, PU_CACHE));
		}
		break;
	default:
		break;
	}

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

		const patch_t* ch = W_ResolvePatchHandle(hu_font[c]);

		const int w = ch->width();
		if (cx + w > width)
			break;
		screen->DrawPatchClean(ch, cx, cy);
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
static bool	 		castdeath;
static int 			castframes;
static int 			castonmelee;
static bool	 		castattacking;


//
// F_StartCast
//
extern gamestate_t 	wipegamestate;


void F_StartCast()
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
void F_CastTicker()
{
	if (--casttics > 0)
		return; 				// not time to change state yet

	if (caststate->tics == -1 || caststate->nextstate == S_NULL)
	{
		// switch from deathstate to next monster
		castnum++;
		castdeath = false;
		if (castorder[castnum].name == NULL)
			castnum = 0;
		if (mobjinfo[castorder[castnum].type].seesound)
		{
			const int atten = ATTN_NONE;
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

		const int st = caststate->nextstate;

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

	const patch_t* background_patch = W_CachePatch("BOSSBACK");

	// draw the background to the surface
	cast_surface->lock();

	cast_surface->getDefaultCanvas()->DrawPatch(background_patch, 0, 0);

	// draw the current frame in the middle of the screen
	const spritedef_t* sprdef = &sprites[castsprite];
	const spriteframe_t* sprframe = &sprdef->spriteframes[caststate->frame & FF_FRAMEMASK];

	const patch_t* sprite_patch = W_CachePatch(sprframe->lump[0]);
	if (sprframe->flip[0])
		cast_surface->getDefaultCanvas()->DrawPatchFlipped(sprite_patch, 160, 170);
	else
		cast_surface->getDefaultCanvas()->DrawPatch(sprite_patch, 160, 170);

	const int width = F_GetWidth();
	const int height = F_GetHeight();
	const int x = (primary_surface->getWidth() - width) / 2;
	const int y = (primary_surface->getHeight() - height) / 2;

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

void F_DrawPatchColP(int x, const patch_t *patch, int col)
{
	IWindowSurface* surface = I_GetPrimarySurface();
	const int surface_width = surface->getWidth(), surface_height = surface->getHeight();

	// [RH] figure out how many times to repeat this column
	// (for screens wider than 320 pixels)
	const float mul = static_cast<float>(surface_width) / 320.0f;
	const float fx = static_cast<float>(x);
	const int repeat = static_cast<int>(floor(mul * (fx + 1)) - floor(mul * fx));
	if (repeat == 0)
		return;

	// [RH] Remap virtual-x to real-x
	x = static_cast<int>(floor(mul * x));

	// [RH] Figure out per-row fixed-point step
	const unsigned int step = (200<<16) / surface_height;
	const unsigned int invstep = (surface_height<<16) / 200;

	const tallpost_t *post = (tallpost_t *)((byte *)patch + LELONG(patch->columnofs[col]));
	
	byte* desttop = surface->getBuffer() + x;
	const int pitch = surface->getPitchInPixels();

	// step through the posts in a column
	while (!post->end())
	{
		const byte* source = post->data();
		byte* dest = desttop + ((post->topdelta * invstep)>>16) * pitch;
		unsigned int count = (post->length * invstep) >> 16;
		int c = 0;
		palindex_t p;

		switch (repeat)
		{
			case 1:
				do
				{
					*dest = source[c>>16];
					dest += pitch;
					c += step;
				} while (--count);
				break;
			case 2:
				do
				{
					p = source[c>>16];
					dest[0] = p;
					dest[1] = p;
					dest += pitch;
					c += step;
				} while (--count);
				break;
			case 3:
				do
				{
					p = source[c>>16];
					dest[0] = p;
					dest[1] = p;
					dest[2] = p;
					dest += pitch;
					c += step;
				} while (--count);
				break;
			case 4:
				do
				{
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
					do
					{
						p = source[c>>16];
						for (int count2 = repeat; count2; count2--)
						{
							dest[count2] = p;
						}
						dest += pitch;
						c += step;
					} while (--count);
				}
				break;
		}

		post = post->next();
	}
}

// Direct version 32bpp:

void F_DrawPatchColD(int x, const patch_t *patch, int col)
{
	IWindowSurface* surface = I_GetPrimarySurface();
	const int surface_width = surface->getWidth(), surface_height = surface->getHeight();

	// [RH] figure out how many times to repeat this column
	// (for screens wider than 320 pixels)
	const float mul = static_cast<float>(surface_width) / 320.0f;
	const float fx = static_cast<float>(x);
	const int repeat = static_cast<int>(floor(mul * (fx + 1)) - floor(mul * fx));
	if (repeat == 0)
		return;

	// [RH] Remap virtual-x to real-x
	x = static_cast<int>(floor(mul * x));

	// [RH] Figure out per-row fixed-point step
	const unsigned step = (200<<16) / surface_height;
	const unsigned invstep = (surface_height<<16) / 200;

	const tallpost_t *post = (tallpost_t *)((byte *)patch + LELONG(patch->columnofs[col]));
	argb_t* desttop = (argb_t *)surface->getBuffer() + x;
	const int pitch = surface->getPitchInPixels();

	const shaderef_t pal = shaderef_t(&V_GetDefaultPalette()->maps, 0);

	// step through the posts in a column
	while (!post->end())
	{
		const byte* source = post->data();
		argb_t* dest = desttop + ((post->topdelta*invstep)>>16)*pitch;
		unsigned count = (post->length * invstep) >> 16;
		int c = 0;
		argb_t p;

		switch (repeat)
		{
			case 1:
				do
				{
					*dest = pal.shade(source[c>>16]);
					dest += pitch;
					c += step;
				} while (--count);
				break;
			case 2:
				do
				{
					p = pal.shade(source[c>>16]);
					dest[0] = p;
					dest[1] = p;
					dest += pitch;
					c += step;
				} while (--count);
				break;
			case 3:
				do
				{
					p = pal.shade(source[c>>16]);
					dest[0] = p;
					dest[1] = p;
					dest[2] = p;
					dest += pitch;
					c += step;
				} while (--count);
				break;
			case 4:
				do
				{
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
					do
					{
						p = pal.shade(source[c>>16]);
						for (int count2 = repeat; count2; count2--) {
							dest[count2] = p;
						}
						dest += pitch;
						c += step;
					} while (--count);
				}
				break;
		}

		post = post->next();
	}
}


//
// F_BunnyScroll
//
void F_BunnyScroll()
{
	char		name[10];
	static int	laststage;

	const patch_t* p1 = W_CachePatch("PFUB2");
	const patch_t* p2 = W_CachePatch("PFUB1");

	V_MarkRect (0, 0, I_GetSurfaceWidth(), I_GetSurfaceHeight());

	int scrolled = 320 - (finalecount - 230) / 2;
	if (scrolled > 320)
		scrolled = 320;
	if (scrolled < 0)
		scrolled = 0;

	if (I_GetPrimarySurface()->getBitsPerPixel() == 8)
	{
		for (int x = 0; x < 320; x++)
		{
			if (x+scrolled < 320)
				F_DrawPatchColP(x, p1, x+scrolled);
			else
				F_DrawPatchColP(x, p2, x+scrolled - 320);
		}
	}
	else
	{
		for (int x = 0; x < 320; x++)
		{
			if (x+scrolled < 320)
				F_DrawPatchColD(x, p1, x+scrolled);
			else
				F_DrawPatchColD(x, p2, x+scrolled - 320);
		}
	}

	if (finalecount < 1130)
		return;
	if (finalecount < 1180)
	{
		screen->DrawPatchIndirect(W_CachePatch("END0"), (320-13*8)/2, (200-8*8)/2);
		laststage = 0;
		return;
	}

	int stage = (finalecount - 1180) / 5;
	if (stage > 6)
		stage = 6;
	if (stage > laststage)
	{
		S_Sound (CHAN_WEAPON, "weapons/pistol", 1, ATTN_NONE);
		laststage = stage;
	}

	sprintf (name,"END%i",stage);
	screen->DrawPatchIndirect(W_CachePatch(name), (320-13*8)/2, (200-8*8)/2);
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
					const char* page = !level.endpic.empty() ? level.endpic.c_str() : gameinfo.finalePage1;

					screen->DrawPatchIndirect(W_CachePatch(page), 0, 0);
					break;
				}
				case '2':
					screen->DrawPatchIndirect (W_CachePatch (gameinfo.finalePage2), 0, 0);
					break;
				case '3':
					F_BunnyScroll ();
					break;
				case '4':
					screen->DrawPatchIndirect (W_CachePatch (gameinfo.finalePage3), 0, 0);
					break;
			}
			break;

		case 2:
			F_CastDrawer ();
			break;
	}
}

VERSION_CONTROL (f_finale_cpp, "$Id$")
