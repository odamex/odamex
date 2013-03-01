// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1998-2006 by Randy Heit (ZDoom).
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
//		Mission begin melt/wipe screen special effect.
//
//-----------------------------------------------------------------------------

#include "z_zone.h"
#include "v_video.h"
#include "m_random.h"
#include "m_alloc.h"
#include "doomdef.h"
#include "f_wipe.h"
#include "c_cvars.h"
#include "i_music.h"

//
//		SCREEN WIPE PACKAGE
//

enum
{
	wipe_None,			// don't bother
	wipe_Melt,			// weird screen melt
	wipe_Burn,			// fade in shape of fire
	wipe_Fade,			// crossfade from old to new
	wipe_NUMWIPES
};

EXTERN_CVAR (r_wipetype)
static int Currentr_wipetype;

static short *wipeP_scr_start = NULL;
static short *wipeP_scr_end = NULL;

static argb_t *wipeD_scr_start = NULL;
static argb_t *wipeD_scr_end = NULL;

static int *y = NULL;

// [RH] Fire Wipe
#define FIREWIDTH	64
#define FIREHEIGHT	64
static byte *burnarray = NULL;
static int density;
static int burntime;

// [RH] Crossfade
static int fade;


// Melt -------------------------------------------------------------

// Palettized (8bpp) version:

void wipeP_shittyColMajorXform (short *array)
{
	int x, y;
	short *dest;
	int width = screen->width / 2;

	dest = new short[width*screen->height*2];

	for(y = 0; y < screen->height; y++)
		for(x = 0; x < width; x++)
			dest[x*screen->height+y] = array[y*width+x];

	memcpy(array, dest, screen->width*screen->height);

	delete[] dest;
}

int wipeP_initMelt (int ticks)
{
	int i, r;

	// copy start screen to main screen
	screen->DrawBlock (0, 0, screen->width, screen->height, (byte *)wipeP_scr_start);

	// makes this wipe faster (in theory)
	// to have stuff in column-major format
	wipeP_shittyColMajorXform (wipeP_scr_start);
	wipeP_shittyColMajorXform (wipeP_scr_end);

	// setup initial column positions
	// (y<0 => not ready to scroll yet)
	y = new int[screen->width*sizeof(int)];
	y[0] = -(M_Random()&0xf);
	for (i = 1; i < screen->width; i++)
	{
		r = (M_Random()%3) - 1;
		y[i] = y[i-1] + r;
		if (y[i] > 0) y[i] = 0;
		else if (y[i] == -16) y[i] = -15;
	}

	return 0;
}

int wipeP_doMelt (int ticks)
{
	int 		i;
	int 		j;
	int 		dy;
	int 		idx;

	short*		s;
	short*		d;
	BOOL	 	done = true;

	int width = screen->width / 2;

	while (ticks--)
	{
		for (i = 0; i < width; i++)
		{
			if (y[i]<0)
			{
				y[i]++; done = false;

				s = &wipeP_scr_start[i*screen->height];
				d = &((short *)screen->buffer)[i];
				idx = 0;
				
				for (j=screen->height;j;j--)
				{
					d[idx] = *(s++);
					idx += screen->pitch/2;
				}
			}
			else if (y[i] < screen->height)
			{
				dy = (y[i] < 16) ? y[i]+1 : 8;
				dy = (dy * screen->height) / 200;
				if (y[i]+dy >= screen->height)
					dy = screen->height - y[i];
				s = &wipeP_scr_end[i*screen->height+y[i]];
				d = &((short *)screen->buffer)[y[i]*(screen->pitch/2)+i];
				idx = 0;
				for (j=dy;j;j--)
				{
					d[idx] = *(s++);
					idx += screen->pitch/2;
				}
				y[i] += dy;
				s = &wipeP_scr_start[i*screen->height];
				d = &((short *)screen->buffer)[y[i]*(screen->pitch/2)+i];
				idx = 0;
				for (j=screen->height-y[i];j;j--)
				{
					d[idx] = *(s++);
					idx += screen->pitch/2;
				}
				done = false;
			}
		}
	}

	return done;
}

int wipeP_exitMelt (int ticks)
{
	if (wipeP_scr_start != NULL)
	{
        delete[] wipeP_scr_start;
        wipeP_scr_start = NULL;
	}

	if (wipeP_scr_end != NULL)
	{
        delete[] wipeP_scr_end;
        wipeP_scr_end = NULL;
	}

	if (y != NULL)
	{
        delete[] y;
        y = NULL;
	}

	return 0;
}

// Direct (32bpp) version:

void wipeD_shittyColMajorXform (argb_t *array)
{
	int x, y;
	argb_t *dest;

	dest = new argb_t[screen->width*screen->height];

	for(y = 0; y < screen->height; y++)
		for(x = 0; x < screen->width; x++)
			dest[x*screen->height+y] = array[y*screen->width+x];

	memcpy(array, dest, screen->width*screen->height*sizeof(argb_t));

	delete[] dest;
}

int wipeD_initMelt (int ticks)
{
	int i, r;

	// copy start screen to main screen
	screen->DrawBlock (0, 0, screen->width, screen->height, (byte *)wipeD_scr_start);

	// makes this wipe faster (in theory)
	// to have stuff in column-major format
	wipeD_shittyColMajorXform (wipeD_scr_start);
	wipeD_shittyColMajorXform (wipeD_scr_end);

	// setup initial column positions
	// (y<0 => not ready to scroll yet)
	y = new int[screen->width];
	y[0] = -(M_Random()&0xf);
	for (i = 1; i < screen->width; i++)
	{
		r = (M_Random()%3) - 1;
		y[i] = y[i-1] + r;
		if (y[i] > 0) y[i] = 0;
		else if (y[i] == -16) y[i] = -15;
	}

	return 0;
}

int wipeD_doMelt (int ticks)
{
	int 		i;
	int 		j;
	int 		dy;
	int 		idx;

	argb_t*		s;
	argb_t*		d;
	BOOL	 	done = true;

	int pitch = screen->pitch / sizeof(argb_t);

	while (ticks--)
	{
		for (i = 0; i < screen->width; i++)
		{
			if (y[i] < 0)
			{
				y[i]++; done = false;

				s = &wipeD_scr_start[i*screen->height];
				d = &((argb_t *)screen->buffer)[i];
				idx = 0;

				for (j=screen->height;j;j--)
				{
					d[idx] = *(s++);
					idx += pitch;
				}
			}
			else if (y[i] < screen->height)
			{
				dy = (y[i] < 16) ? y[i]+1 : 8;
				dy = (dy * screen->height) / 200;
				if (y[i]+dy >= screen->height)
					dy = screen->height - y[i];
				s = &wipeD_scr_end[i*screen->height+y[i]];
				d = &((argb_t *)screen->buffer)[y[i]*(pitch)+i];
				idx = 0;
				for (j=dy;j;j--)
				{
					d[idx] = *(s++);
					idx += pitch;
				}
				y[i] += dy;
				s = &wipeD_scr_start[i*screen->height];
				d = &((argb_t *)screen->buffer)[y[i]*(pitch)+i];
				idx = 0;
				for (j=screen->height-y[i];j;j--)
				{
					d[idx] = *(s++);
					idx += pitch;
				}
				done = false;
			}
		}
	}

	return done;
}

int wipeD_exitMelt (int ticks)
{
	if (wipeD_scr_start != NULL)
	{
        delete[] wipeD_scr_start;
        wipeD_scr_start = NULL;
	}
	
	if (wipeD_scr_end != NULL)
	{
        delete[] wipeD_scr_end;
        wipeD_scr_end = NULL;
	}
	
	if (y != NULL)
	{
        delete[] y;
        y = NULL;
	}
	
	return 0;
}


// Burn -------------------------------------------------------------

// Palettized (8bpp) version:

int wipeP_initBurn (int ticks)
{
	burnarray = new byte[FIREWIDTH * (FIREHEIGHT+5)];
	memset (burnarray, 0, FIREWIDTH * (FIREHEIGHT+5));
	density = 4;
	burntime = 0;
	return 0;
}

int wipeP_doBurn (int ticks)
{
	static int voop;
	BOOL done;

	// This is a modified version of the fire from the player
	// setup menu.
	burntime += ticks;
	ticks *= 2;

	// Make the fire burn
	while (ticks--)
	{
		int a, b;
		byte *from;

		// generator
		from = burnarray + FIREHEIGHT * FIREWIDTH;
		b = voop;
		voop += density / 3;
		for (a = 0; a < density/8; a++)
		{
			unsigned int offs = (a+b) % FIREWIDTH;
			unsigned int v = M_Random();
			v = from[offs] + 4 + (v & 15) + (v >> 3) + (M_Random() & 31);
			if (v > 255)
				v = 255;
			from[offs] = from[FIREWIDTH*2 + (offs + FIREWIDTH*3/2)%FIREWIDTH] = v;
		}

		density += 10;
		if (density > FIREWIDTH*7)
			density = FIREWIDTH*7;

		from = burnarray;
		for (b = 0; b <= FIREHEIGHT; b += 2)
		{
			byte *pixel = from;

			// special case: first pixel on line
			byte *p = pixel + (FIREWIDTH << 1);
			unsigned int top = *p + *(p + FIREWIDTH - 1) + *(p + 1);
			unsigned int bottom = *(pixel + (FIREWIDTH << 2));
			unsigned int c1 = (top + bottom) >> 2;
			if (c1 > 1) c1--;
			*pixel = c1;
			*(pixel + FIREWIDTH) = (c1 + bottom) >> 1;
			pixel++;

			// main line loop
			for (a = 1; a < FIREWIDTH-1; a++)
			{
				// sum top pixels
				p = pixel + (FIREWIDTH << 1);
				top = *p + *(p - 1) + *(p + 1);

				// bottom pixel
				bottom = *(pixel + (FIREWIDTH << 2));

				// combine pixels
				c1 = (top + bottom) >> 2;
				if (c1 > 1) c1--;

				// store pixels
				*pixel = c1;
				*(pixel + FIREWIDTH) = (c1 + bottom) >> 1;		// interpolate

				// next pixel
				pixel++;
			}

			// special case: last pixel on line
			p = pixel + (FIREWIDTH << 1);
			top = *p + *(p - 1) + *(p - FIREWIDTH + 1);
			bottom = *(pixel + (FIREWIDTH << 2));
			c1 = (top + bottom) >> 2;
			if (c1 > 1) c1--;
			*pixel = c1;
			*(pixel + FIREWIDTH) = (c1 + bottom) >> 1;

			// next line
			from += FIREWIDTH << 1;
		}
	}

	// Draw the screen
	{
		fixed_t xstep, ystep, firex, firey;
		int x, y;
		byte *to, *fromold, *fromnew;

		xstep = (FIREWIDTH * FRACUNIT) / screen->width;
		ystep = (FIREHEIGHT * FRACUNIT) / screen->height;
		to = screen->buffer;
		fromold = (byte *)wipeP_scr_start;
		fromnew = (byte *)wipeP_scr_end;
		done = true;

		for (y = 0, firey = 0; y < screen->height; y++, firey += ystep)
		{
			for (x = 0, firex = 0; x < screen->width; x++, firex += xstep)
			{
				int fglevel;

				fglevel = burnarray[(firex>>FRACBITS)+(firey>>FRACBITS)*FIREWIDTH] / 2;
				if (fglevel >= 63)
				{
					to[x] = fromnew[x];
				}
				else if (fglevel == 0)
				{
					to[x] = fromold[x];
					done = false;
				}
				else
				{
					int bglevel = 64-fglevel;
#if 1
					to[x] = rt_blend2<palindex_t>(fromold[x], bglevel << 2, fromnew[x], fglevel << 2);
#else
					argb_t *fg2rgb = Col2RGB8[fglevel];
					argb_t *bg2rgb = Col2RGB8[bglevel];
					argb_t fg = fg2rgb[fromnew[x]];
					argb_t bg = bg2rgb[fromold[x]];
					fg = (fg+bg) | 0x1f07c1f;
					to[x] = RGB32k[0][0][fg & (fg>>15)];
#endif
					done = false;
				}
			}
			fromold += screen->width;
			fromnew += screen->width;
			to += screen->pitch;
		}
	}

	return done || (burntime > 40);
}

int wipeP_exitBurn (int ticks)
{
	if (wipeP_scr_start != NULL)
	{
        delete[] wipeP_scr_start;
        wipeP_scr_start = NULL;
	}
	
	if (wipeP_scr_end != NULL)
	{
        delete[] wipeP_scr_end;
        wipeP_scr_end = NULL;
	}

	if (burnarray != NULL)
	{
        delete[] burnarray;
        burnarray = NULL;
	}
	
	return 0;
}

// Direct (32bpp) version:

int wipeD_initBurn (int ticks)
{
	burnarray = new byte[FIREWIDTH * (FIREHEIGHT+5)];
	memset (burnarray, 0, FIREWIDTH * (FIREHEIGHT+5));
	density = 4;
	burntime = 0;
	return 0;
}

int wipeD_doBurn (int ticks)
{
	static int voop;
	BOOL done;

	// This is a modified version of the fire from the player
	// setup menu.
	burntime += ticks;
	ticks *= 2;

	// Make the fire burn
	while (ticks--)
	{
		int a, b;
		byte *from;

		// generator
		from = burnarray + FIREHEIGHT * FIREWIDTH;
		b = voop;
		voop += density / 3;
		for (a = 0; a < density/8; a++)
		{
			unsigned int offs = (a+b) % FIREWIDTH;
			unsigned int v = M_Random();
			v = from[offs] + 4 + (v & 15) + (v >> 3) + (M_Random() & 31);
			if (v > 255)
				v = 255;
			from[offs] = from[FIREWIDTH*2 + (offs + FIREWIDTH*3/2)%FIREWIDTH] = v;
		}

		density += 10;
		if (density > FIREWIDTH*7)
			density = FIREWIDTH*7;

		from = burnarray;
		for (b = 0; b <= FIREHEIGHT; b += 2)
		{
			byte *pixel = from;

			// special case: first pixel on line
			byte *p = pixel + (FIREWIDTH << 1);
			unsigned int top = *p + *(p + FIREWIDTH - 1) + *(p + 1);
			unsigned int bottom = *(pixel + (FIREWIDTH << 2));
			unsigned int c1 = (top + bottom) >> 2;
			if (c1 > 1) c1--;
			*pixel = c1;
			*(pixel + FIREWIDTH) = (c1 + bottom) >> 1;
			pixel++;

			// main line loop
			for (a = 1; a < FIREWIDTH-1; a++)
			{
				// sum top pixels
				p = pixel + (FIREWIDTH << 1);
				top = *p + *(p - 1) + *(p + 1);

				// bottom pixel
				bottom = *(pixel + (FIREWIDTH << 2));

				// combine pixels
				c1 = (top + bottom) >> 2;
				if (c1 > 1) c1--;

				// store pixels
				*pixel = c1;
				*(pixel + FIREWIDTH) = (c1 + bottom) >> 1;		// interpolate

				// next pixel
				pixel++;
			}

			// special case: last pixel on line
			p = pixel + (FIREWIDTH << 1);
			top = *p + *(p - 1) + *(p - FIREWIDTH + 1);
			bottom = *(pixel + (FIREWIDTH << 2));
			c1 = (top + bottom) >> 2;
			if (c1 > 1) c1--;
			*pixel = c1;
			*(pixel + FIREWIDTH) = (c1 + bottom) >> 1;

			// next line
			from += FIREWIDTH << 1;
		}
	}

	// Draw the screen
	{
		fixed_t xstep, ystep, firex, firey;
		int x, y;
		int to_pitch;
		argb_t *to, *fromold, *fromnew;

		xstep = (FIREWIDTH * FRACUNIT) / screen->width;
		ystep = (FIREHEIGHT * FRACUNIT) / screen->height;
		to = (argb_t *)screen->buffer;
		to_pitch = screen->pitch / sizeof(argb_t);
		fromold = wipeD_scr_start;
		fromnew = wipeD_scr_end;
		done = true;

		for (y = 0, firey = 0; y < screen->height; y++, firey += ystep)
		{
			for (x = 0, firex = 0; x < screen->width; x++, firex += xstep)
			{
				int fglevel;

				fglevel = burnarray[(firex>>FRACBITS)+(firey>>FRACBITS)*FIREWIDTH] / 2;
				if (fglevel >= 63)
				{
					to[x] = fromnew[x];
				}
				else if (fglevel == 0)
				{
					to[x] = fromold[x];
					done = false;
				}
				else
				{
					int bglevel = 64-fglevel;
					to[x] = alphablend2a(fromold[x], bglevel << 2, fromnew[x], fglevel << 2);
					done = false;
				}
			}
			fromold += screen->width;
			fromnew += screen->width;
			to += to_pitch;
		}
	}

	return done || (burntime > 40);
}

int wipeD_exitBurn (int ticks)
{
	if (wipeD_scr_start != NULL)
	{
        delete[] wipeD_scr_start;
        wipeD_scr_start = NULL;
	}

	if (wipeD_scr_end != NULL)
	{
        delete[] wipeD_scr_end;
        wipeD_scr_end = NULL;
	}
	
	if (burnarray != NULL)
	{
        delete[] burnarray;
        burnarray = NULL;
	}
	
	return 0;
}

// Crossfade --------------------------------------------------------

// Palettized (8bpp) version:

int wipeP_initFade (int ticks)
{
	fade = 0;
	return 0;
}

int wipeP_doFade (int ticks)
{
	fade += ticks;
	if (fade > 64)
	{
		screen->DrawBlock (0, 0, screen->width, screen->height, (byte *)wipeP_scr_end);
		return 1;
	}
	else
	{
		int x, y;
		fixed_t bglevel = 64 - fade;
#if 0
		unsigned int *fg2rgb = Col2RGB8[fade];
		unsigned int *bg2rgb = Col2RGB8[bglevel];
#endif
		byte *fromnew = (byte *)wipeP_scr_end;
		byte *fromold = (byte *)wipeP_scr_start;
		byte *to = screen->buffer;

		for (y = 0; y < screen->height; y++)
		{
			for (x = 0; x < screen->width; x++)
			{
#if 1
				to[x] = rt_blend2<palindex_t>(fromold[x], bglevel << 2, fromnew[x], fade << 2);
#else
				unsigned int fg = fg2rgb[fromnew[x]];
				unsigned int bg = bg2rgb[fromold[x]];
				fg = (fg+bg) | 0x1f07c1f;
				to[x] = RGB32k[0][0][fg & (fg>>15)];
#endif
			}
			fromnew += screen->width;
			fromold += screen->width;
			to += screen->pitch;
		}
	}
	fade++;
	return 0;
}

int wipeP_exitFade (int ticks)
{
	if (wipeP_scr_start != NULL)
	{
        delete[] wipeP_scr_start;
        wipeP_scr_start = NULL;
	}

	if (wipeP_scr_end != NULL)
	{
        delete[] wipeP_scr_end;
        wipeP_scr_end = NULL;
	}

	return 0;
}

// Direct (32bpp) version:

int wipeD_initFade (int ticks)
{
	fade = 0;
	return 0;
}

int wipeD_doFade (int ticks)
{
	fade += ticks;
	if (fade > 64)
	{
		screen->DrawBlock (0, 0, screen->width, screen->height, (byte *)wipeD_scr_end);
		return 1;
	}
	else
	{
		int x, y;
		int to_pitch;
		int bglevel = 64 - fade;
		argb_t *fromnew = wipeD_scr_end;
		argb_t *fromold = wipeD_scr_start;
		argb_t *to = (argb_t *)screen->buffer;
		to_pitch = screen->pitch / sizeof(argb_t);

		for (y = 0; y < screen->height; y++)
		{
			for (x = 0; x < screen->width; x++)
			{
				to[x] = alphablend2a(fromold[x], bglevel << 2, fromnew[x], fade << 2);
			}
			fromnew += screen->width;
			fromold += screen->width;
			to += to_pitch;
		}
	}
	fade++;
	return 0;
}

int wipeD_exitFade (int ticks)
{
	if (wipeD_scr_start != NULL)
	{
        delete[] wipeD_scr_start;
        wipeD_scr_start = NULL;
	}
	
	if (wipeD_scr_end != NULL)
	{
        delete[] wipeD_scr_end;
        wipeD_scr_end = NULL;
	}
	
	return 0;
}

// General Wipe Functions -------------------------------------------

int wipe_StartScreen (void)
{
	Currentr_wipetype = (int)r_wipetype;
	if (Currentr_wipetype < 0)
		Currentr_wipetype = 0;
	else if (Currentr_wipetype >= wipe_NUMWIPES)
		Currentr_wipetype = wipe_NUMWIPES-1;

	if (Currentr_wipetype)
	{
        if (screen->is8bit())
		{
			delete[] wipeP_scr_start;
            wipeP_scr_start = new short[screen->width * screen->height / 2];
			screen->GetBlock (0, 0, screen->width, screen->height, (byte *)wipeP_scr_start);
		}
        else
		{
			delete[] wipeD_scr_start;
            wipeD_scr_start = new argb_t[screen->width * screen->height];
			screen->GetBlock (0, 0, screen->width, screen->height, (byte *)wipeD_scr_start);
		}
	}

	return 0;
}

int wipe_EndScreen (void)
{
	if (Currentr_wipetype)
	{
        if (screen->is8bit())
		{
			delete[] wipeP_scr_end;
            wipeP_scr_end = new short[screen->width * screen->height / 2];
			screen->GetBlock (0, 0, screen->width, screen->height, (byte *)wipeP_scr_end);
		}
        else
		{
			delete[] wipeD_scr_end;
            wipeD_scr_end = new argb_t[screen->width * screen->height];
			screen->GetBlock (0, 0, screen->width, screen->height, (byte *)wipeD_scr_end);
		}
	}

	return 0;
}

int wipe_ScreenWipe (int ticks)
{
	static BOOL	go = 0;		// when zero, stop the wipe
	static int (*wipesP[])(int) =
	{
		wipeP_initMelt, wipeP_doMelt, wipeP_exitMelt,
		wipeP_initBurn, wipeP_doBurn, wipeP_exitBurn,
		wipeP_initFade, wipeP_doFade, wipeP_exitFade
	};
	static int (*wipesD[])(int) =
	{
		wipeD_initMelt, wipeD_doMelt, wipeD_exitMelt,
		wipeD_initBurn, wipeD_doBurn, wipeD_exitBurn,
		wipeD_initFade, wipeD_doFade, wipeD_exitFade
	};
	int rc;

	// [SL] 2011-12-31 - Continue to play music during screenwipe so
	// notes do not hang
	I_UpdateMusic();
	
	if (Currentr_wipetype == wipe_None)
		return true;

	int (*wInit)(int);
	int (*wDoit)(int);
	int (*wExit)(int);

	if (screen->is8bit())
	{
		wInit = (*wipesP[(Currentr_wipetype-1)*3+0]);
		wDoit = (*wipesP[(Currentr_wipetype-1)*3+1]);
		wExit = (*wipesP[(Currentr_wipetype-1)*3+2]);
	}
	else
	{
		wInit = (*wipesD[(Currentr_wipetype-1)*3+0]);
		wDoit = (*wipesD[(Currentr_wipetype-1)*3+1]);
		wExit = (*wipesD[(Currentr_wipetype-1)*3+2]);
	}

	// initial stuff
	if (!go)
	{
		go = 1;
		wInit(ticks);
	}

	// do a piece of wipe-in
	V_MarkRect(0, 0, screen->width, screen->height);
	rc = wDoit(ticks);

	// final stuff
	if (rc)
	{
		go = 0;
		wExit(ticks);
	}

	return !go;
}

VERSION_CONTROL (f_wipe_cpp, "$Id$")

