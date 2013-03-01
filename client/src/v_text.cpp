// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
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
//	V_TEXT
//
//-----------------------------------------------------------------------------


#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#include "v_text.h"

#include "i_system.h"
#include "i_video.h"
#include "v_video.h"
#include "hu_stuff.h"
#include "w_wad.h"
#include "z_zone.h"
#include "m_swap.h"

#include "doomstat.h"

EXTERN_CVAR(hud_scaletext)

extern patch_t *hu_font[HU_FONTSIZE];


static byte *ConChars;

extern byte *Ranges;

int V_TextScaleXAmount()
{
	if (hud_scaletext < 1.0f)
		return 1;

	return int(hud_scaletext);
}

int V_TextScaleYAmount()
{
	if (hud_scaletext < 1.0f)
		return 1;

	return int(hud_scaletext);
}

// Convert the CONCHARS patch into the internal format used by
// the console font drawer.
void V_InitConChars (byte transcolor)
{
	byte *d, *s, v, *src;
	patch_t *chars;
	int x, y, z, a;
	DCanvas *scrn = I_AllocateScreen(128, 128, 8);
	DCanvas &temp = *scrn;

	if (temp.is8bit())
	{
	chars = W_CachePatch ("CONCHARS");
	temp.Lock ();

	{
			argb_t *scrn, fill;

		fill = (transcolor << 24) | (transcolor << 16) | (transcolor << 8) | transcolor;
		for (y = 0; y < 128; y++)
		{
				scrn = (argb_t *)(temp.buffer + temp.pitch * y);
			for (x = 0; x < 128/4; x++)
			{
				*scrn++ = fill;
			}
		}
		temp.DrawPatch (chars, 0, 0);
	}

	src = temp.buffer;

	if ( (ConChars = new byte[256*8*8*2]) )
	{
		d = ConChars;
		for (y = 0; y < 16; y++)
		{
			for (x = 0; x < 16; x++)
			{
				s = src + x * 8 + (y * 8 * temp.pitch);
				for (z = 0; z < 8; z++)
				{
					for (a = 0; a < 8; a++)
					{
						v = s[a];
						if (v == transcolor)
						{
							d[a] = 0x00;
							d[a+8] = 0xff;
						}
						else
						{
							d[a] = v;
							d[a+8] = 0x00;
						}
					}
					d += 16;
					s += temp.pitch;
				}
			}
		}
	}

	temp.Unlock ();
	}
	else
	{
		ConChars = new byte[256*8*8*2];
	}
	I_FreeScreen(scrn);
}


//
// V_PrintStr
// Print a line of text using the console font
//

extern "C" void STACK_ARGS PrintChar1P (DWORD *charimg, byte *dest, int screenpitch);
extern "C" void STACK_ARGS PrintChar2P_MMX (DWORD *charimg, byte *dest, int screenpitch);

void DCanvas::PrintStr (int x, int y, const char *s, int count) const
{
	const byte *str = (const byte *)s;
	byte *temp;
	argb_t *charimg;

	if (!buffer)
		return;

	if (y > (height - 8) || y<0)
		return;

	if (x < 0)
	{
		int skip;

		skip = -(x - 7) / 8;
		x += skip * 8;
		if (count <= skip)
		{
			return;
		}
		else
		{
			count -= skip;
			str += skip;
		}
	}

	x &= ~3;
	temp = buffer + y * pitch;

	while (count && x <= (width - 8))
	{
	    // john - tab 4 spaces
	    if (*str == '\t')
	    {
	        str++;
	        count--;
	        x += 8 * 4;
	        continue;
	    }

		charimg = (argb_t *)&ConChars[(*str) * 128];
		if (is8bit())
		{
			int z;
			argb_t *writepos;

			writepos = (argb_t *)(temp + x);
			for (z = 0; z < 8; z++)
			{
				*writepos = (*writepos & charimg[2]) ^ charimg[0];
				writepos++;
				*writepos = (*writepos & charimg[3]) ^ charimg[1];
				writepos += (pitch >> 2) - 1;
				charimg += 4;
			}
		}
		else
		{
			int z;
			argb_t *writepos;

			writepos = (argb_t *)(temp + (x << 2));
			for (z = 0; z < 8; z++)
			{
#define BYTEIMG ((byte *)charimg)
#define SPOT(a) \
	writepos[a] = (writepos[a] & \
				 ((BYTEIMG[a+8]<<16)|(BYTEIMG[a+8]<<8)|(BYTEIMG[a+8]))) \
				 ^ V_Palette.shade(BYTEIMG[a])

				SPOT(0);
				SPOT(1);
				SPOT(2);
				SPOT(3);
				SPOT(4);
				SPOT(5);
				SPOT(6);
				SPOT(7);
#undef SPOT
#undef BYTEIMG
				writepos += pitch >> 2;
				charimg += 4;
			}
		}
		str++;
		count--;
		x += 8;
	}
}

//
// V_PrintStr2
// Same as V_PrintStr but doubles the size of every character.
//
void DCanvas::PrintStr2 (int x, int y, const char *s, int count) const
{
	const byte *str = (const byte *)s;
	byte *temp;
	argb_t *charimg;

	if (y > (height - 16))
		return;

	if (x < 0)
	{
		int skip;

		skip = -(x - 15) / 16;
		x += skip * 16;
		if (count <= skip)
		{
			return;
		}
		else
		{
			count -= skip;
			str += skip;
		}
	}

	x &= ~3;
	temp = buffer + y * pitch;

	while (count && x <= (width - 16))
	{
	    // john - tab 4 spaces
        if (*str == '\t')
	    {
	        str++;
	        count--;
	        x += 16 * 4;
	        continue;
	    }

		charimg = (argb_t *)&ConChars[(*str) * 128];

		{
			int z;
			byte *buildmask, *buildbits, *image;
			unsigned int m1, s1;
			unsigned int *writepos;

			writepos = (unsigned int *)(temp + x);
			buildbits = (byte *)&s1;
			buildmask = (byte *)&m1;
			image = (byte *)charimg;

			for (z = 0; z < 8; z++)
			{
				buildmask[0] = buildmask[1] = image[8];
				buildmask[2] = buildmask[3] = image[9];
				buildbits[0] = buildbits[1] = image[0];
				buildbits[2] = buildbits[3] = image[1];
				writepos[0] = (writepos[0] & m1) ^ s1;
				writepos[pitch/4] = (writepos[pitch/4] & m1) ^ s1;

				buildmask[0] = buildmask[1] = image[10];
				buildmask[2] = buildmask[3] = image[11];
				buildbits[0] = buildbits[1] = image[2];
				buildbits[2] = buildbits[3] = image[3];
				writepos[1] = (writepos[1] & m1) ^ s1;
				writepos[1+pitch/4] = (writepos[1+pitch/4] & m1) ^ s1;

				buildmask[0] = buildmask[1] = image[12];
				buildmask[2] = buildmask[3] = image[13];
				buildbits[0] = buildbits[1] = image[4];
				buildbits[2] = buildbits[3] = image[5];
				writepos[2] = (writepos[2] & m1) ^ s1;
				writepos[2+pitch/4] = (writepos[2+pitch/4] & m1) ^ s1;

				buildmask[0] = buildmask[1] = image[14];
				buildmask[2] = buildmask[3] = image[15];
				buildbits[0] = buildbits[1] = image[6];
				buildbits[2] = buildbits[3] = image[7];
				writepos[3] = (writepos[3] & m1) ^ s1;
				writepos[3+pitch/4] = (writepos[3+pitch/4] & m1) ^ s1;

				writepos += pitch >> 1;
				image += 16;
			}

		}
		str++;
		count--;
		x += 16;
	}
}

//
// V_DrawText
//
// Write a string using the hu_font
//

void DCanvas::TextWrapper (EWrapperCode drawer, int normalcolor, int x, int y, const byte *string) const
{
	int 		w;
	const byte *ch;
	int 		c;
	int 		cx;
	int 		cy;
	int			boldcolor;

	if (normalcolor > NUM_TEXT_COLORS)
		normalcolor = CR_RED;
	boldcolor = normalcolor ? normalcolor - 1 : NUM_TEXT_COLORS - 1;

	V_ColorMap = translationref_t(Ranges + normalcolor * 256);

	ch = string;
	cx = x;
	cy = y;

	while (1)
	{
		c = *ch++;
		if (!c)
			break;

		if (c == 0x8a)
		{
			int newcolor = toupper(*ch++);

			if (newcolor == 0)
			{
				return;
			}
			else if (newcolor == '-')
			{
				newcolor = normalcolor;
			}
			else if (newcolor >= 'A' && newcolor < 'A' + NUM_TEXT_COLORS)
			{
				newcolor -= 'A';
			}
			else if (newcolor == '+')
			{
				newcolor = boldcolor;
			}
			else
			{
				continue;
			}
			V_ColorMap = translationref_t(Ranges + newcolor * 256);
			continue;
		}

		if (c == '\n')
		{
			cx = x;
			cy += 9;
			continue;
		}

		c = toupper(c) - HU_FONTSTART;
		if (c < 0 || c>= HU_FONTSIZE)
		{
			cx += 4;
			continue;
		}

		w = hu_font[c]->width();
		if (cx+w > width)
			break;

		DrawWrapper (drawer, hu_font[c], cx, cy);
		cx+=w;
	}
}

void DCanvas::TextSWrapper (EWrapperCode drawer, int normalcolor, int x, int y, const byte *string) const
{
	TextSWrapper(drawer, normalcolor, x, y, string, CleanXfac, CleanYfac);
}

void DCanvas::TextSWrapper (EWrapperCode drawer, int normalcolor, int x, int y, 
							const byte *string, int scalex, int scaley) const
{
	int 		w;
	const byte *ch;
	int 		c;
	int 		cx;
	int 		cy;
	int			boldcolor;

	if (normalcolor > NUM_TEXT_COLORS)
		normalcolor = CR_RED;
	boldcolor = normalcolor ? normalcolor - 1 : NUM_TEXT_COLORS - 1;

	V_ColorMap = translationref_t(Ranges + normalcolor * 256);

	ch = string;
	cx = x;
	cy = y;

	while (1)
	{
		c = *ch++;
		if (!c)
			break;

		if (c == 0x8a)
		{
			int newcolor = toupper(*ch++);

			if (newcolor == 0)
			{
				return;
			}
			else if (newcolor == '-')
			{
				newcolor = normalcolor;
			}
			else if (newcolor >= 'A' && newcolor < 'A' + NUM_TEXT_COLORS)
			{
				newcolor -= 'A';
			}
			else if (newcolor == '+')
			{
				newcolor = boldcolor;
			}
			else
			{
				continue;
			}
			V_ColorMap = translationref_t(Ranges + newcolor * 256);
			continue;
		}

		if (c == '\n')
		{
			cx = x;
			cy += 9 * scalex;
			continue;
		}

		c = toupper(c) - HU_FONTSTART;
		if (c < 0 || c>= HU_FONTSIZE)
		{
			cx += 4 * scaley;
			continue;
		}

		w = hu_font[c]->width() * scalex;
		if (cx+w > width)
			break;

        DrawSWrapper (drawer, hu_font[c], cx, cy,
                        hu_font[c]->width() * scalex,
                        hu_font[c]->height() * scaley);

		cx+=w;
	}
}

//
// Find string width from hu_font chars
//
int V_StringWidth (const byte *string)
{
	int w = 0, c;
	
	if(!string)
		return 0;

	while (*string)
	{
		if (*string == 0x8a)
		{
			if (*(++string))
				string++;
			continue;
		}
		else
		{
			c = toupper((*string++) & 0x7f) - HU_FONTSTART;
			if (c < 0 || c >= HU_FONTSIZE)
			{
				w += 4;
			}
			else
			{
				w += hu_font[c]->width();
			}
		}
	}

	return w;
}

//
// Break long lines of text into multiple lines no longer than maxwidth pixels
//
static void breakit (brokenlines_t *line, const byte *start, const byte *string)
{
	// Leave out trailing white space
	while (string > start && isspace (*(string - 1)))
		string--;

	line->string = new char[string - start + 1];
	strncpy (line->string, (char *)start, string - start);
	line->string[string - start] = 0;
	line->width = V_StringWidth (line->string);
}

brokenlines_t *V_BreakLines (int maxwidth, const byte *string)
{
	brokenlines_t lines[128];	// Support up to 128 lines (should be plenty)

	const byte *space = NULL, *start = string;
	int i, c, w, nw;
	BOOL lastWasSpace = false;

	i = w = 0;

	while ( (c = *string++) ) {
		if (c == 0x8a) {
			if (*string)
				string++;
			continue;
		}

		if (isspace(c)) {
			if (!lastWasSpace) {
				space = string - 1;
				lastWasSpace = true;
			}
		} else
			lastWasSpace = false;

		c = toupper (c & 0x7f) - HU_FONTSTART;

		if (c < 0 || c >= HU_FONTSIZE)
			nw = 4;
		else
			nw = hu_font[c]->width();

		if (w + nw > maxwidth || c == '\n' - HU_FONTSTART) {	// Time to break the line
			if (!space)
				space = string - 1;

			breakit (&lines[i], start, space);

			i++;
			w = 0;
			lastWasSpace = false;
			start = space;
			space = NULL;

			while (*start && isspace (*start) && *start != '\n')
				start++;
			if (*start == '\n')
				start++;
			else
				while (*start && isspace (*start))
					start++;
			string = start;
		} else
			w += nw;
	}

	if (string - start > 1) {
		const byte *s = start;

		while (s < string) {
			if (!isspace (*s++)) {
				breakit (&lines[i++], start, string);
				break;
			}
		}
	}

	{
		// Make a copy of the broken lines and return them
		brokenlines_t *broken = new brokenlines_t[i+1];

		memcpy (broken, lines, sizeof(brokenlines_t) * i);
		broken[i].string = NULL;
		broken[i].width = -1;

		return broken;
	}
}

void V_FreeBrokenLines (brokenlines_t *lines)
{
	if (lines)
	{
		int i = 0;

		while (lines[i].width != -1)
		{
			delete[] lines[i].string;
			lines[i].string = NULL;
			i++;
		}
		delete[] lines;
	}
}


VERSION_CONTROL (v_text_cpp, "$Id$")

