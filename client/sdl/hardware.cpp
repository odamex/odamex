// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2006-2013 by The Odamex Team.
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
//	SDL hardware access for Video Rendering (?)
//
//-----------------------------------------------------------------------------


#include <stdio.h>
#include <stdlib.h>

#include <sstream>
#include <string>

#include "hardware.h"
#undef MINCHAR
#undef MAXCHAR
#undef MINSHORT
#undef MAXSHORT
#undef MINLONG
#undef MAXLONG
#include "i_video.h"
#include "i_system.h"
#include "c_console.h"
#include "c_cvars.h"
#include "c_dispatch.h"
#include "m_argv.h"
#include "m_misc.h"
#include "i_sdlvideo.h"
#include "m_fileio.h"
#include "g_game.h"
#include "i_input.h"

bool M_FindFreeName(std::string &filename, const std::string &extension);

extern constate_e ConsoleState;
extern int NewWidth, NewHeight, NewBits, DisplayBits;

//static bool MouseShouldBeGrabbed ();

static IVideo *Video;
//static IKeyboard *Keyboard;
//static IMouse *Mouse;
//static IJoystick *Joystick;

EXTERN_CVAR (vid_fps)
EXTERN_CVAR (vid_ticker)
EXTERN_CVAR (vid_fullscreen)
EXTERN_CVAR (vid_overscan)

CVAR_FUNC_IMPL (vid_winscale)
{
	if (var < 1.f)
	{
		var.Set (1.f);
	}
	else if (Video)
	{
		Video->SetWindowedScale (var);
		NewWidth = screen->width;
		NewHeight = screen->height;
		NewBits = DisplayBits;
		setmodeneeded = true;
	}
}

CVAR_FUNC_IMPL (vid_overscan)
{
	if(var > 1.0)
		var = 1.0;

	if (Video)
		Video->SetOverscan(var);
}

void STACK_ARGS I_ShutdownHardware ()
{
	if (Video)
		delete Video, Video = NULL;
		
    SDL_QuitSubSystem(SDL_INIT_VIDEO);
}

void I_InitHardware ()
{
	char num[4];
	num[0] = '1' - !Args.CheckParm ("-devparm");
	num[1] = 0;
	vid_ticker.SetDefault (num);

	if(Args.CheckParm ("-novideo"))
		Video = new IVideo();
	else
		Video = new SDLVideo (0);

	if (Video == NULL)
		I_FatalError ("Failed to initialize display");

	atterm (I_ShutdownHardware);

	Video->SetWindowedScale (vid_winscale);
}

bool I_HardwareInitialized()
{
	return (Video != NULL);
}

/** Remaining code is common to Win32 and Linux **/

// VIDEO WRAPPERS ---------------------------------------------------------

void I_BeginUpdate ()
{
	screen->Lock ();
}

void I_FinishUpdateNoBlit ()
{
	screen->Unlock ();
}

void I_TempUpdate ()
{
	Video->UpdateScreen (screen);
}

void I_FinishUpdate ()
{
	// Draws frame time and cumulative fps
	if (vid_fps)
	{
		static DWORD lastms = 0, lastsec = 0;
		static int framecount = 0, lastcount = 0;
		char fpsbuff[40];
		int chars;

		QWORD ms = I_MSTime ();
		QWORD howlong = ms - lastms;
		if (howlong > 0)
		{
			chars = sprintf(fpsbuff, "%3u ms (%3d fps)", (unsigned int)howlong, lastcount);
			screen->Clear (0, screen->height - 8, chars * 8, screen->height, 0);
			screen->PrintStr (0, screen->height - 8, (char *)&fpsbuff[0], chars);
			if (lastsec < ms / 1000)
			{
				lastcount = framecount / (ms/1000 - lastsec);
				lastsec = ms / 1000;
				framecount = 0;
			}
			framecount++;
		}
		lastms = ms;
	}

    // draws little dots on the bottom of the screen
    if (vid_ticker)
    {
		static QWORD lasttic = 0;
		QWORD i = I_GetTime();
		QWORD tics = i - lasttic;
		lasttic = i;
		if (tics > 20) tics = 20;

		for (i=0 ; i<tics*2 ; i+=2)
			screen->buffer[(screen->height-1)*screen->pitch + i] = 0xff;
		for ( ; i<20*2 ; i+=2)
			screen->buffer[(screen->height-1)*screen->pitch + i] = 0x0;
    }

	Video->UpdateScreen (screen);
	screen->Unlock(); // SoM: we should probably do this, eh?
}

void I_ReadScreen (byte *block)
{
	Video->ReadScreen (block);
}

void I_SetPalette (DWORD *pal)
{
	Video->SetPalette (pal);
}


// Set the window caption
void I_SetWindowCaption(const std::string& caption)
{
	// [Russell] - A basic version string that will eventually get replaced
	//             better than "Odamex SDL Alpha Build 001" or something :P    
	std::ostringstream title;

	title << "Odamex - " << DOTVERSIONSTR;
		
	if(caption.size())
	{
		title << " " << caption;
	}

	// [Russell] - Update window caption with name
	SDL_WM_SetCaption (title.str().c_str(), title.str().c_str());
}

void I_SetWindowCaption(void)
{
	I_SetWindowCaption("");
}

// Set the window icon
void I_SetWindowIcon(void)
{

}

extern DWORD IndexedPalette[256];
EXTERN_CVAR(cl_screenshotname)

/*
    Dump a screenshot as a bitmap (BMP) file
*/
void I_ScreenShot(std::string filename)
{
	SDL_Surface *surface;
	SDL_Color colors[256];
	DWORD *pal;

	// If no filename was passed, use the screenshot format variable.
	if (filename.empty()) {
		filename = cl_screenshotname.cstring();
	}

	// Expand tokens
	filename = M_ExpandTokens(filename).c_str();

	// If the file already exists, append numbers.
	if (!M_FindFreeName(filename, "bmp")) {
		Printf(PRINT_HIGH, "I_ScreenShot: Delete some screenshots\n");
		return;
	}

	// Create an SDL_Surface object from our screen buffer
	screen->Lock();

	surface = SDL_CreateRGBSurfaceFrom(screen->buffer, screen->width,
									   screen->height, screen->bits, screen->pitch,
									   0, 0, 0, 0);

	screen->Unlock();

	if (surface == NULL) {
		Printf(PRINT_HIGH, "CreateRGBSurfaceFrom failed: %s\n", SDL_GetError());
		return;
	}

	if (screen->is8bit())
	{
	// Set up the palette for our screen shot
	pal = IndexedPalette;

	for (int i = 0;i < 256;i += 1, pal++) {
		colors[i].r = RPART(*pal);
		colors[i].g = GPART(*pal);
		colors[i].b = BPART(*pal);
		colors[i].unused = 0;
	}

	SDL_SetColors(surface, colors, 0, 256);
	}

	// save the bmp file
	if(SDL_SaveBMP(surface, filename.c_str()) == -1) {
		Printf(PRINT_HIGH, "SDL_SaveBMP Error: %s\n", SDL_GetError());
		SDL_FreeSurface(surface);
		return;
	}

	SDL_FreeSurface(surface);
	Printf(PRINT_HIGH, "Screenshot taken: %s\n", filename.c_str());
}

BEGIN_COMMAND (screenshot)
{
	if (argc == 1)
		G_ScreenShot (NULL);
	else
		G_ScreenShot (argv[1]);
}
END_COMMAND (screenshot)

CVAR_FUNC_IMPL (cl_screenshotname)
{
	// No empty format strings allowed.
	if (strlen(var.cstring()) == 0) {
		var.RestoreDefault();
	}
}

//
// I_SetOldPalette - Just for the red screen for now I guess
//
void I_SetOldPalette (byte *doompalette)
{
    Video->SetOldPalette (doompalette);
}

EDisplayType I_DisplayType ()
{
	return Video->GetDisplayType ();
}

bool I_SetOverscan (float scale)
{
	return Video->SetOverscan (scale);
}

BOOL I_SetMode (int &width, int &height, int &bits)
{
	bool fs = false;
	int tbits = bits;

	switch (Video->GetDisplayType ())
	{
	case DISPLAY_WindowOnly:
		fs = false;
		I_PauseMouse();
		break;
	case DISPLAY_FullscreenOnly:
		fs = true;
		I_ResumeMouse();
		break;
	case DISPLAY_Both:
		fs = vid_fullscreen ? true : false;

		fs ? I_ResumeMouse() : I_PauseMouse();

		break;
	}

	bool res = Video->SetMode (width, height, tbits, fs);
	if (res) return true;

	// Try the opposite bit mode:
	tbits = bits == 32 ? 8 : 32;
	res = Video->SetMode (width, height, tbits, fs);
	if (res) return true;

	// Switch the bit mode back:
	tbits = bits;

	// Try the closest resolution:
	I_ClosestResolution (&width, &height);
	res = Video->SetMode (width, height, tbits, fs);
	if (res) return true;

	// Try the opposite bit mode:
	tbits = bits == 32 ? 8 : 32;
	res = Video->SetMode (width, height, tbits, fs);
	if (res) return true;

	// Just couldn't get it:
	return false;
	//I_FatalError ("Mode %dx%dx%d is unavailable\n",
	//			width, height, bits);
}

bool I_CheckResolution(int width, int height)
{
	int twidth, theight;

	Video->FullscreenChanged(vid_fullscreen ? true : false);
	Video->StartModeIterator();
	while (Video->NextMode (&twidth, &theight))
	{
		if (width == twidth && height == theight)
			return true;
	}

	// [AM] We only care about correct resolutions if we're fullscreen.
	return !vid_fullscreen;
}

void I_ClosestResolution (int *width, int *height)
{
	int twidth, theight;
	int cwidth = 0, cheight = 0;
	int iteration;
	DWORD closest = 4294967295u;

	Video->FullscreenChanged (vid_fullscreen ? true : false);
	for (iteration = 0; iteration < 2; iteration++)
	{
		Video->StartModeIterator ();
		while (Video->NextMode (&twidth, &theight))
		{
			if (twidth == *width && theight == *height)
				return;

			if (iteration == 0 && (twidth < *width || theight < *height))
				continue;

			DWORD dist = (twidth - *width) * (twidth - *width)
				+ (theight - *height) * (theight - *height);

			if (dist < closest)
			{
				closest = dist;
				cwidth = twidth;
				cheight = theight;
			}
		}
		if (closest != 4294967295u)
		{
			*width = cwidth;
			*height = cheight;
			return;
		}
	}
}

bool I_CheckVideoDriver (const char *name)
{
	if(!name)
		return false;

	return (std::string(name) == Video->GetVideoDriverName());
}

void I_StartModeIterator ()
{
	Video->StartModeIterator ();
}

bool I_NextMode (int *width, int *height)
{
	return Video->NextMode (width, height);
}

DCanvas *I_AllocateScreen (int width, int height, int bits, bool primary)
{
	DCanvas *scrn = Video->AllocateSurface (width, height, bits, primary);

	if (!scrn)
	{
		I_FatalError ("Failed to allocate a %dx%dx%d surface",
					  width, height, bits);
	}

	return scrn;
}

void I_FreeScreen (DCanvas *canvas)
{
	Video->ReleaseSurface (canvas);
}

void I_LockScreen (DCanvas *canvas)
{
	Video->LockSurface (canvas);
}

void I_UnlockScreen (DCanvas *canvas)
{
	Video->UnlockSurface (canvas);
}

void I_Blit (DCanvas *src, int srcx, int srcy, int srcwidth, int srcheight,
			 DCanvas *dest, int destx, int desty, int destwidth, int destheight)
{
    if (!src->m_Private || !dest->m_Private)
		return;

    if (!src->m_LockCount)
		src->Lock ();
    if (!dest->m_LockCount)
		dest->Lock ();

	if (//!Video->CanBlit() ||
		!Video->Blit (src, srcx, srcy, srcwidth, srcheight,
					  dest, destx, desty, destwidth, destheight))
	{
		fixed_t fracxstep, fracystep;
		fixed_t fracx, fracy;
		int x, y;

		if(!destheight || !destwidth)
			return;

		fracy = srcy << FRACBITS;
		fracystep = (srcheight << FRACBITS) / destheight;
		fracxstep = (srcwidth << FRACBITS) / destwidth;

		if (src->is8bit() == dest->is8bit())
		{
			// INDEX8 -> INDEX8 or ARGB8888 -> ARGB8888

			if (dest->is8bit())
			{
				// INDEX8 -> INDEX8
				byte *destline, *srcline;

			if (fracxstep == FRACUNIT)
			{
				for (y = desty; y < desty + destheight; y++, fracy += fracystep)
				{
					memcpy (dest->buffer + y * dest->pitch + destx,
							src->buffer + (fracy >> FRACBITS) * src->pitch + srcx,
							destwidth);
				}
			}
			else
			{
				for (y = desty; y < desty + destheight; y++, fracy += fracystep)
				{
					srcline = src->buffer + (fracy >> FRACBITS) * src->pitch + srcx;
					destline = dest->buffer + y * dest->pitch + destx;
					for (x = fracx = 0; x < destwidth; x++, fracx += fracxstep)
					{
						destline[x] = srcline[fracx >> FRACBITS];
					}
				}
			}
		}
			else
			{
				// ARGB8888 -> ARGB8888
				argb_t *destline, *srcline;

				if (fracxstep == FRACUNIT)
				{
					for (y = desty; y < desty + destheight; y++, fracy += fracystep)
					{
						memcpy ((argb_t *)(dest->buffer + y * dest->pitch) + destx,
								(argb_t *)(src->buffer + (fracy >> FRACBITS) * src->pitch) + srcx,
								destwidth * (dest->bits / 8));
					}
				}
				else
				{
					for (y = desty; y < desty + destheight; y++, fracy += fracystep)
					{
						srcline = (argb_t *)(src->buffer + (fracy >> FRACBITS) * src->pitch) + srcx;
						destline = (argb_t *)(dest->buffer + y * dest->pitch) + destx;
						for (x = fracx = 0; x < destwidth; x++, fracx += fracxstep)
						{
							destline[x] = srcline[fracx >> FRACBITS];
						}
					}
				}
			}
		}
		else if (!src->is8bit() && dest->is8bit())
		{
			// ARGB8888 -> INDEX8
			I_FatalError ("Can't I_Blit() an ARGB8888 source to\nan INDEX8 destination");
		}
		else
		{
			// INDEX8 -> ARGB8888 (Palette set in V_Palette)
			argb_t *destline;
			byte *srcline;

			if (fracxstep == FRACUNIT)
			{
				// No X-scaling
				for (y = desty; y < desty + destheight; y++, fracy += fracystep)
				{
					srcline = src->buffer + (fracy >> FRACBITS) * src->pitch + srcx;
					destline = (argb_t *)(dest->buffer + y * dest->pitch) + destx;
					for (x = 0; x < destwidth; x++)
					{
						destline[x] = V_Palette.shade(srcline[x]);
					}
				}
			}
			else
			{
				// With X-scaling
				for (y = desty; y < desty + destheight; y++, fracy += fracystep)
				{
					srcline = src->buffer + (fracy >> FRACBITS) * src->pitch + srcx;
					destline = (argb_t *)(dest->buffer + y * dest->pitch) + destx;
					for (x = fracx = 0; x < destwidth; x++, fracx += fracxstep)
					{
						destline[x] = V_Palette.shade(srcline[fracx >> FRACBITS]);
					}
				}
			}
		}
	}

    if (!src->m_LockCount)
		I_UnlockScreen (src);
    if (!dest->m_LockCount)
		I_UnlockScreen (dest);
}

// denis - here is a blank implementation of IVideo that allows the client
// to run without actual video output (e.g. script-controlled demo testing)
EDisplayType IVideo::GetDisplayType () { return DISPLAY_Both; }

std::string IVideo::GetVideoDriverName () { return ""; }

bool IVideo::FullscreenChanged (bool fs) { return true; }
void IVideo::SetWindowedScale (float scale) {}
bool IVideo::CanBlit () { return true; }

bool IVideo::SetOverscan (float scale) { return true; }

bool IVideo::SetMode (int width, int height, int bits, bool fs) { return true; }
void IVideo::SetPalette (argb_t *palette) {}

void IVideo::SetOldPalette (byte *doompalette) {}
void IVideo::UpdateScreen (DCanvas *canvas) {}
void IVideo::ReadScreen (byte *block) {}

int IVideo::GetModeCount () { return 1; }
void IVideo::StartModeIterator () {}
bool IVideo::NextMode (int *width, int *height) { static int w = 320, h = 240; width = &w; height = &h; return false; }

DCanvas *IVideo::AllocateSurface (int width, int height, int bits, bool primary)
{
	DCanvas *scrn = new DCanvas;

	scrn->width = width;
	scrn->height = height;
	scrn->bits = bits;
	scrn->m_LockCount = 0;
	scrn->m_Palette = NULL;
	// TODO(jsd): Align to 16-byte boundaries for SSE2 optimization!
	scrn->buffer = new byte[width*height*(bits/8)];
	scrn->pitch = width * (bits / 8);

	return scrn;
}

void IVideo::ReleaseSurface (DCanvas *scrn)
{
	delete[] scrn->buffer;
	delete scrn;
}

void IVideo::LockSurface (DCanvas *scrn) {}
void IVideo::UnlockSurface (DCanvas *scrn)  {}
bool IVideo::Blit (DCanvas *src, int sx, int sy, int sw, int sh,
			   DCanvas *dst, int dx, int dy, int dw, int dh) { return true; }

BEGIN_COMMAND (vid_listmodes)
{
	int width, height;

	Video->StartModeIterator ();
		while (Video->NextMode (&width, &height))
		if (width == DisplayWidth && height == DisplayHeight)
			Printf_Bold ("%4d x%5d\n", width, height);
			else
			Printf (PRINT_HIGH, "%4d x%5d\n", width, height);
}
END_COMMAND (vid_listmodes)

BEGIN_COMMAND (vid_currentmode)
{
	Printf (PRINT_HIGH, "%dx%dx%d\n", DisplayWidth, DisplayHeight, DisplayBits);
}
END_COMMAND (vid_currentmode)

VERSION_CONTROL (hardware_cpp, "$Id$")


