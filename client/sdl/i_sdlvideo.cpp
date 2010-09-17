// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
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
//	SDL implementation of the IVideo class.
//
//-----------------------------------------------------------------------------


#include <stdio.h>
#include <stdlib.h>

// [Russell] - Just for windows, display the icon in the system menu and
// alt-tab display
#if WIN32
#ifndef _XBOX
#include <windows.h>
#endif // !_XBOX
#include "SDL_syswm.h"
#include "resource.h"
#endif // WIN32

#include "v_palette.h"
#include "i_sdlvideo.h"
#include "i_system.h"

#ifdef _XBOX
#include "i_xbox.h"
#endif

SDLVideo::SDLVideo(int parm)
{
	const SDL_version *SDLVersion = SDL_Linked_Version();

	if(SDLVersion->major != SDL_MAJOR_VERSION
		|| SDLVersion->minor != SDL_MINOR_VERSION)
	{
		I_FatalError("SDL version conflict (%d.%d.%d vs %d.%d.%d dll)\n",
			SDL_MAJOR_VERSION, SDL_MINOR_VERSION, SDL_PATCHLEVEL,
			SDLVersion->major, SDLVersion->minor, SDLVersion->patch);
		return;
	}

	if (SDL_InitSubSystem (SDL_INIT_VIDEO) == -1)
	{
		I_FatalError("Could not initialize SDL video.\n");
		return;
	}

	if(SDLVersion->patch != SDL_PATCHLEVEL)
	{
		Printf_Bold("SDL version warning (%d.%d.%d vs %d.%d.%d dll)\n",
			SDL_MAJOR_VERSION, SDL_MINOR_VERSION, SDL_PATCHLEVEL,
			SDLVersion->major, SDLVersion->minor, SDLVersion->patch);
	}

    // [Russell] - Just for windows, display the icon in the system menu and
    // alt-tab display
    #if WIN32 && !_XBOX
    HICON Icon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_ICON1));

    if (Icon != 0)
    {
        HWND WindowHandle;
        
        SDL_SysWMinfo wminfo;
        SDL_VERSION(&wminfo.version)
        SDL_GetWMInfo(&wminfo);
        
        WindowHandle = wminfo.window;

		// GhostlyDeath <October 26, 2008> -- VC6 (No Service Packs or new SDKs) has no SetClassLongPtr?
#if defined(_MSC_VER) && _MSC_VER <= 1200// && !defined(_MSC_FULL_VER)
		SetClassLong(WindowHandle, GCL_HICON, (LONG) Icon);
#else
        SetClassLongPtr(WindowHandle, GCL_HICON, (LONG_PTR) Icon);
#endif
    }
    #endif
    
    I_SetWindowCaption();

   sdlScreen = NULL;
   infullscreen = false;
   screenw = screenh = screenbits = 0;
   palettechanged = false;

   chainHead = new cChain(NULL);

   // Get Video modes
   SDL_PixelFormat fmt;
   fmt.palette = NULL;
   fmt.BitsPerPixel = 8;
   fmt.BytesPerPixel = 1;

   SDL_Rect **sdllist = SDL_ListModes(NULL, SDL_FULLSCREEN|SDL_SWSURFACE);

   vidModeList = NULL;
   vidModeCount = 0;

   if(!sdllist)
   {
	  // no fullscreen modes, but we could still try windowed
	  Printf(PRINT_HIGH, "SDL_ListModes returned NULL. No fullscreen video modes are available.\n");
      vidModeList = NULL; 
	  vidModeCount = 0;
	  return;
   }
   else if(sdllist == (SDL_Rect **)-1)
   {
      I_FatalError("SDL_ListModes returned -1. Internal error.\n");
      return;
   }
   else
   {
      int i;
      for(i = 0; sdllist[i]; i++)
         ;

      vidModeList = new vidMode[i];
      vidModeCount = i;

      for(i = 0; sdllist[i]; i++)
      {
         vidModeList[i].width = sdllist[i]->w;
         vidModeList[i].height = sdllist[i]->h;
         vidModeList[i].bits = 8;
      }
   }
}

SDLVideo::~SDLVideo(void)
{
   cChain *rover, *next;

   for(rover = chainHead->next; rover != chainHead; rover = next)
   {
      next = rover->next;
      ReleaseSurface(rover->canvas);
      delete rover;
   }

   delete chainHead;

   if(vidModeList)
   {
      delete [] vidModeList;
      vidModeList = NULL;
   }
}




bool SDLVideo::FullscreenChanged (bool fs)
{
   if(fs != infullscreen)
   {
      fs = infullscreen;
      return true;
   }

   return false;
}

void SDLVideo::SetWindowedScale (float scale)
{
   /// HAHA FIXME
}

bool SDLVideo::SetOverscan (float scale)
{
	int   ret = 0;

	if(scale > 1.0)
		return false;

#ifdef _XBOX
	if(xbox_SetScreenStretch( -(screenw - (screenw * scale)), -(screenh - (screenh * scale))) )
		ret = -1;
	if(xbox_SetScreenPosition( (screenw - (screenw * scale)) / 2, (screenh - (screenh * scale)) / 2) )
		ret = -1;
#endif

	if(ret)
		return false;

	return true;
}

bool SDLVideo::SetMode (int width, int height, int bits, bool fs)
{
   Uint32 flags = SDL_RESIZABLE;

   // SoM: I'm not sure if we should request a software or hardware surface yet... So I'm
   // just ganna let SDL decide.

   if(fs && vidModeCount)
   {
      flags = 0;
       
      flags |= SDL_FULLSCREEN;

      if(bits == 8)
         flags |= SDL_HWPALETTE;
   }

   if(!(sdlScreen = SDL_SetVideoMode(width, height, bits, flags)))
      return false;

   screenw = width;
   screenh = height;
   screenbits = bits;

   return true;
}


void SDLVideo::SetPalette (DWORD *palette)
{
   for(size_t i = 0; i < sizeof(newPalette)/sizeof(SDL_Color); i++)
   {
      newPalette[i].r = RPART(palette[i]);
      newPalette[i].g = GPART(palette[i]);
      newPalette[i].b = BPART(palette[i]);
   }

   palettechanged = true;
}

void SDLVideo::SetOldPalette (byte *doompalette)
{ 
    //for(size_t i = 0; i < sizeof(newPalette)/sizeof(SDL_Color); i++)
    int i;
    for (i = 0; i < 256; ++i)
    {    
      newPalette[i].r = newgamma[*doompalette++];
      newPalette[i].g = newgamma[*doompalette++];
      newPalette[i].b = newgamma[*doompalette++];
    }
   palettechanged = true;
}

void SDLVideo::UpdateScreen (DCanvas *canvas)
{
   if(palettechanged)
   {
      SDL_SetPalette(sdlScreen, SDL_LOGPAL|SDL_PHYSPAL, newPalette, 0, 256);
	  palettechanged = false;
   }
   
   SDL_Flip(sdlScreen);
}


void SDLVideo::ReadScreen (byte *block)
{
   // SoM: forget lastCanvas, let's just read from the screen, y0
   if(!sdlScreen)
      return;

   int y;
   byte *source;
   bool unlock = false;

   if(SDL_MUSTLOCK(sdlScreen))
   {
      unlock = true;
      SDL_LockSurface(sdlScreen);
   }

   source = (byte *)sdlScreen->pixels;

   for (y = 0; y < sdlScreen->h; y++)
   {
      memcpy (block, source, sdlScreen->w);
      block += sdlScreen->w;
      source += sdlScreen->pitch;
   }
   
   if(unlock)
      SDL_UnlockSurface(sdlScreen);
}


int SDLVideo::GetModeCount ()
{
   return vidModeCount;
}


void SDLVideo::StartModeIterator (int bits)
{
   vidModeIterator = 0;
   vidModeIteratorBits = bits;
}


bool SDLVideo::NextMode (int *width, int *height)
{
   while(vidModeIterator < vidModeCount)
   {
      if(vidModeList[vidModeIterator].bits == vidModeIteratorBits)
      {
         *width = vidModeList[vidModeIterator].width;
         *height = vidModeList[vidModeIterator].height;
         vidModeIterator++;
         return true;
      }

      vidModeIterator++;
   }
   return false;
}


DCanvas *SDLVideo::AllocateSurface (int width, int height, int bits, bool primary)
{
	DCanvas *scrn = new DCanvas;
	
	scrn->width = width;
	scrn->height = height;
	scrn->bits = screenbits;
	scrn->m_LockCount = 0;
	scrn->m_Palette = NULL;
	scrn->buffer = NULL;

	SDL_Surface *s;

	if(primary)
	{
	  scrn->m_Private = s = sdlScreen; // denis - let the engine write directly to screen
	}
	else
	{
	  if(bits == 8)
		 scrn->m_Private = s = SDL_CreateRGBSurface(SDL_SWSURFACE, width, height, bits, 0, 0, 0, 0);
	  else
		 scrn->m_Private = s = SDL_CreateRGBSurface(SDL_SWSURFACE, width, height, bits, 0xff0000, 0x00ff00, 0x0000ff, 0);
	}

	if(!s)
	   I_FatalError("SDLVideo::AllocateSurface failed to allocate an SDL surface.");
	   
	if(s->pitch != (width * (bits / 8)))
	   Printf(PRINT_HIGH, "Warning: SDLVideo::AllocateSurface got a surface with an abnormally wide pitch.\n");

	scrn->pitch = s->pitch;

	if(!primary)
	{
	  cChain *nnode = new cChain(scrn);
	  nnode->linkTo(chainHead);
	}

	return scrn;   
}



void SDLVideo::ReleaseSurface (DCanvas *scrn)
{
   if(scrn->m_Private == sdlScreen) // primary stays
      return;

   if(scrn->m_LockCount)
      scrn->Unlock();

   if(scrn->m_Private)
   {
      SDL_FreeSurface((SDL_Surface *)scrn->m_Private);
      scrn->m_Private = NULL;
   }

   scrn->DetachPalette ();

   for(cChain *r = chainHead->next; r != chainHead; r = r->next)
   {
      if(r->canvas == scrn)
      {
         delete r;
         break;
      }
   }
   
   delete scrn;
}



void SDLVideo::LockSurface (DCanvas *scrn)
{
   SDL_Surface *s = (SDL_Surface *)scrn->m_Private;

   if(SDL_MUSTLOCK(s))
   {
      if(SDL_LockSurface(s) == -1)
         I_FatalError("SDLVideo::LockSurface failed to lock a surface that required it...\n");

      scrn->m_LockCount ++;
   }
   
   scrn->buffer = (unsigned char *)s->pixels;
}


void SDLVideo::UnlockSurface (DCanvas *scrn)
{
   if(!scrn->m_Private)
      return;

   SDL_UnlockSurface((SDL_Surface *)scrn->m_Private);
   scrn->buffer = NULL;
}

bool SDLVideo::Blit (DCanvas *src, int sx, int sy, int sw, int sh, DCanvas *dst, int dx, int dy, int dw, int dh)
{
   return false;
}

VERSION_CONTROL (i_sdlvideo_cpp, "$Id$")

