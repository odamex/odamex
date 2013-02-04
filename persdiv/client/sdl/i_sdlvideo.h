// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
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
//	SDL implementation of the IVideo class.
//
//-----------------------------------------------------------------------------


#ifndef __SDLVIDEO_H__
#define __SDLVIDEO_H__

#include <SDL.h>
#include "hardware.h"
#include "i_video.h"
#include "c_console.h"

class SDLVideo : public IVideo
{
   public:
   SDLVideo(int parm);
	virtual ~SDLVideo (void);

	virtual std::string GetVideoDriverName();

	virtual bool CanBlit (void) {return false;}
	virtual EDisplayType GetDisplayType (void) {return DISPLAY_Both;}

	virtual bool FullscreenChanged (bool fs);
	virtual void SetWindowedScale (float scale);
	virtual bool SetOverscan (float scale);

	virtual bool SetMode (int width, int height, int bits, bool fs);
	virtual void SetPalette (DWORD *palette);

	/* 12/3/06: HACK - Add SetOldPalette to accomodate classic redscreen - ML*/
	virtual void SetOldPalette (byte *doompalette);

	virtual void UpdateScreen (DCanvas *canvas);
	virtual void ReadScreen (byte *block);

	virtual int GetModeCount (void);
	virtual void StartModeIterator (int bits);
	virtual bool NextMode (int *width, int *height);

	virtual DCanvas *AllocateSurface (int width, int height, int bits, bool primary = false);
	virtual void ReleaseSurface (DCanvas *scrn);
	virtual void LockSurface (DCanvas *scrn);
	virtual void UnlockSurface (DCanvas *scrn);
	virtual bool Blit (DCanvas *src, int sx, int sy, int sw, int sh,
					   DCanvas *dst, int dx, int dy, int dw, int dh);


   protected:

   class cChain
   {
      public:
      cChain(DCanvas *dc) : canvas(dc) {next = prev = this;}
      ~cChain() {(prev->next = next)->prev = prev;}

      void linkTo(cChain *head)
      {
         (next = head->next)->prev = next;
         (head->next = next)->prev = head;
      }

      DCanvas *canvas;
      cChain *next, *prev;
   };

   struct vidMode_t
   {
      int width, height, bits;

      bool operator<(const vidMode_t& right) const
      {
         if (bits < right.bits)
            return true;
         else if (bits == right.bits)
         {
            if (width < right.width)
               return true;
            else if (width == right.width && height < right.height)
               return true;
         }
         return false;
      }

      bool operator>(const vidMode_t& right) const
      {
         if (bits > right.bits)
            return true;
		 else if (bits == right.bits)
         {
            if (width > right.width)
               return true;
			else if (width == right.width && height > right.height)
               return true;
		 }
         return false;
      }

      bool operator==(const vidMode_t& right) const
      {
         return (width == right.width &&
                 height == right.height &&
                 bits == right.bits);
      }
   };

   std::vector<vidMode_t> vidModeList;
   size_t vidModeIterator;
   int vidModeIteratorBits;

   SDL_Surface *sdlScreen;
   bool infullscreen;
   int screenw, screenh;
   int screenbits;

   SDL_Color newPalette[256];
   SDL_Color palette[256];
   bool palettechanged;

   cChain      *chainHead;
};
#endif

