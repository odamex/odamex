#include <SDL.h>
#include "video.h"
#include "error.h"

// 
// Begin::vidSurface --------------------------------------------------------------------
//
vidSurface::vidSurface(unsigned int w, unsigned int h, Uint8 bits)
{
   Uint32 rmask, gmask, bmask, amask;

   switch(bits)
   {
      case 16:
         rmask = 31 << 11;
         gmask = 63 << 5;
         bmask = 31;
         amask = 0;
         break;
      case 32:
         rmask = 255 << 16;
         gmask = 255 << 8;
         bmask = 255;
         amask = 0;
         break;
      default:
         rmask = bmask = gmask = amask = 0;
         break;
   };

   s = SDL_CreateRGBSurface(SDL_HWSURFACE|SDL_SWSURFACE, w, h, bits, rmask, gmask, bmask, amask);
   if(!s)
     basicError::Throw("vidSurface failed to allocate surface.\n");

   freesurface = true;
   locks = 0;
   mustlock = SDL_MUSTLOCK(s);

   width = s->w;
   height = s->h;
   pitch = s->pitch;
   abnormalpitch = (s->w * s->format->BytesPerPixel) == pitch ? false : true;

   buffer = NULL;
   bitdepth = s->format->BitsPerPixel;
   cliprect.setRect(0, 0, width, height);
   isscreen = false;
}


vidSurface::vidSurface(SDL_Surface &from, bool owns)
{
   s = NULL;
   setSurface(from, owns);
}


vidSurface::~vidSurface()
{
   while(locks)
      unlock();


   if(!isscreen && freesurface && s)
   {
      SDL_FreeSurface(s);
      s = NULL;
   }
}

// Protected member variables -----------------------------------------------------------
vidSurface *vidSurface::screensurface = NULL;

// Format management --------------------------------------------------------------------
SDL_PixelFormat vidSurface::getFormat()
{
   return *s->format;
}

void vidSurface::convertFormat(SDL_PixelFormat &f)
{
   if(isscreen)
      return;

   Uint32 flags = 0;

   if(f.Amask)
      flags |= SDL_SRCALPHA;

   SDL_Surface *nsurface = SDL_ConvertSurface(s, &f, flags);

   if(!nsurface)
      return;

   setSurface(*nsurface, true);
}


void vidSurface::convertFormat(vidSurface &surface)
{
   if(isscreen)
      return;

   SDL_Surface *nsurface = SDL_ConvertSurface(s, surface.s->format, surface.s->flags);

   if(!nsurface)
      return;

   setSurface(*nsurface, true);
}


vidSurface  *vidSurface::getCopy()
{
   SDL_Surface *copy = 
      SDL_CreateRGBSurfaceFrom(s->pixels, s->w, s->h, s->format->BitsPerPixel, s->pitch,
                              s->format->Rmask, s->format->Gmask, s->format->Bmask, 
                              s->format->Amask);

   if(!copy)
      return NULL;

   vidSurface *ret = new vidSurface(*copy, true);
   return ret;
}


// Resize -------------------------------------------------------------------------------
void vidSurface::resize(unsigned int neww, unsigned int newh)
{
   if(isscreen)
   {
      resizeScreen(neww, newh);
      return;
   }


   SDL_Surface *nsurface = 
      SDL_CreateRGBSurface(s->flags, neww, newh, s->format->BitsPerPixel, s->format->Rmask, 
                           s->format->Gmask, s->format->Bmask, s->format->Amask);

   if(!nsurface)
      return;

   SDL_BlitSurface(s, NULL, nsurface, NULL);

   setSurface(*nsurface, true);
}


// Clipping -----------------------------------------------------------------------------
void vidSurface::setClipRect(vidRect r)
{
   SDL_Rect sdlrect = r.getSDLRect();

   SDL_SetClipRect(s, &sdlrect);

   SDL_GetClipRect(s, &sdlrect);
   cliprect.setSDLRect(sdlrect);
}


vidRect vidSurface::getClipRect()
{
   return vidRect(cliprect);
}




// Blitting -----------------------------------------------------------------------------
//
// See vblit.cpp
//

// Flipping and rotation ----------------------------------------------------------------
void vidSurface::flipHorizontal()
{
   Uint8 *left, *right;
   Uint32 pixelsize, temp;
   int y;
   bool mustunlock = false;

   if(!locks)
   {
      lock();
      mustunlock = true;
   }

   pixelsize = s->format->BytesPerPixel;

   for(y = 0; y < s->h; y++)
   {
      left = (Uint8 *)s->pixels + (s->pitch * y);
      right = left + (s->w - 1) * pixelsize;

      while(left < right)
      {
         memcpy(&temp, left, pixelsize);
         memcpy(left, right, pixelsize);
         memcpy(right, &temp, pixelsize);

         left += pixelsize;
         right -= pixelsize;
      }
   }

   if(mustunlock)
      unlock();
}

void vidSurface::flipVertical()
{
   int x, y;
   Uint8 *top, *bottom;
   Uint32 pixelsize, temp;
   bool mustunlock = false;

   if(!locks)
   {
      lock();
      mustunlock = true;
   }

   pixelsize = s->format->BytesPerPixel;

   for(y = 0; y < s->h / 2; y++)
   {
      top = (Uint8 *)s->pixels + (s->pitch * y);
      bottom = (Uint8 *)s->pixels + (s->pitch * (s->h - y - 1));

      for(x = 0; x < s->w; x++)
      {
         memcpy(&temp, top, pixelsize);
         memcpy(top, bottom, pixelsize);
         memcpy(bottom, &temp, pixelsize);

         top += pixelsize;
         bottom += pixelsize;
      }
   }

   if(mustunlock)
      unlock();
}


void vidSurface::rotate90()
{
   SDL_Surface *ret;
   int x, y;
   Uint8 *p1, *p2;
   Uint32 pixelsize;
   bool mustunlock = false;

   if(!locks)
   {
      lock();
      mustunlock = true;
   }
  
   ret = SDL_CreateRGBSurface(0, s->h, s->w, s->format->BitsPerPixel, s->format->Rmask, s->format->Gmask, s->format->Bmask, s->format->Amask);

   pixelsize = s->format->BytesPerPixel;

   for(y = 0; y < s->h; y++)
   {
      p1 = (Uint8 *)s->pixels + (s->pitch * y);
      p2 = (Uint8 *)ret->pixels + pixelsize * (ret->w - y - 1);

      for(x = 0; x < s->w; x++)
      {
         memcpy(p2, p1, pixelsize);
         p1 += pixelsize;
         p2 += ret->pitch;
      }
   }

   if(mustunlock)
      unlock();

   setSurface(*ret, true);
}



void vidSurface::rotate180()
{
   SDL_Surface *ret;
   int x, y;
   Uint8 *p1, *p2;
   Uint32 pixelsize;
   bool mustunlock = false;

   if(!locks)
   {
      lock();
      mustunlock = true;
   }

   ret = SDL_CreateRGBSurface(0, s->w, s->h, s->format->BitsPerPixel, s->format->Rmask, s->format->Gmask, s->format->Bmask, s->format->Amask);

   pixelsize = s->format->BytesPerPixel;

   for(y = 0; y < s->h; y++)
   {
      p1 = (Uint8 *)s->pixels + (s->pitch * y);
      p2 = (Uint8 *)ret->pixels + (s->pitch * (ret->h - y - 1)) + (pixelsize * (ret->w - 1));

      for(x = 0; x < s->w; x++)
      {
         memcpy(p2, p1, pixelsize);
         p1 += pixelsize;
         p2 -= pixelsize;
      }
   }

   if(mustunlock)
      unlock();

   setSurface(*ret, true);
}


void vidSurface::rotate270()
{
   SDL_Surface *ret;
   int x, y;
   Uint8 *p1, *p2;
   Uint32 pixelsize;
   bool mustunlock = false;

   if(!locks)
   {
      lock();
      mustunlock = true;
   }

   ret = SDL_CreateRGBSurface(0, s->h, s->w, s->format->BitsPerPixel, s->format->Rmask, s->format->Gmask, s->format->Bmask, s->format->Amask);

   pixelsize = s->format->BytesPerPixel;

   for(y = 0; y < s->h; y++)
   {
      p1 = (Uint8 *)s->pixels + (s->pitch * y);
      p2 = (Uint8 *)ret->pixels + (ret->pitch * (ret->h - 1)) + (pixelsize * y);

      for(x = 0; x < s->w; x++)
      {
         memcpy(p2, p1, pixelsize);
         p1 += pixelsize;
         p2 -= ret->pitch;
      }
   }

   if(mustunlock)
      unlock();

   setSurface(*ret, true);
}



// Primitives ---------------------------------------------------------------------------
void vidSurface::drawPixel(int x, int y, Uint32 color)
{
   int pixel;
   bool mustunlock = false;

   if(!locks)
   {
      mustunlock = true;
      lock();
   }

   if(x < cliprect.sx1() || x > cliprect.sx2() || y < cliprect.sy1() || y > cliprect.sy2())
      return;

   pixel = y * s->w + x;
   switch(bitdepth)
   {
      case 16:
         *((Uint16 *)buffer + pixel) = (Uint16)color;
         break;
      case 32:
         *((Uint32 *)buffer + pixel) = color;
         break;
      default:
         buffer[pixel] = (Uint8)color;
         break;
   }

   if(mustunlock)
      unlock();
}


// vidSurface::drawLine uses helper functions 
void vidSurface::drawLine(int x1, int y1, int x2, int y2, Uint32 color)
{
   bool mustunlock = false;
   
   if(!locks)
   {
      lock();
      mustunlock = true;
   }

   if(y1 == y2)
      drawHLine(x1, x2, y1, color);
   else if(x1 == x2)
      drawVLine(x1, y1, y2, color);
   else
   {
      // Quick rejection test.
      vidRect rect;
      rect.setScreen(x1, y1, x2, y2);

      if(!rect.testIntersection(cliprect))
      {
         if(mustunlock)
            unlock();
         return;
      }

      // Classify the line as either positive (from left to right climbing)
      // or negative (from left to right descending)
      int swap;


      if(x2 < x1)
      {
         // if the line is going from right to left, swap the vertecies
         swap = x1;
         x1 = x2;
         x2 = swap;

         swap = y1;
         y1 = y2;
         y2 = swap;
      }


      // Now that it's going from left to right check if y2 is higher than y1
      if(y2 < y1)
         // y2 is closer to the top so it's a positive line.
         drawPositiveLine(x1, y1, x2, y2, color);
      else
         drawNegativeLine(x1, y1, x2, y2, color);
   }
   

   if(mustunlock)
      unlock();
}




void vidSurface::clear()
{
   bool mustunlock = false;
   int  blocksize;

   if(!locks)
   {
      lock();
      mustunlock = true;
   }

   blocksize = s->pitch * s->h;
   memset(buffer, 0, blocksize);

   if(mustunlock)
      unlock();
}



// Colors -------------------------------------------------------------------------------

Uint32 vidSurface::mapColor(Uint8 r, Uint8 g, Uint8 b, Uint8 a)
{
   if(s->format->Amask)
      return SDL_MapRGBA(s->format, r, g, b, a);
   else
      return SDL_MapRGB(s->format, r, g, b);
}


void vidSurface::getColor(Uint32 color, Uint8 &r, Uint8 &g, Uint8 &b, Uint8 &a)
{
   if(s->format->Amask)
      SDL_GetRGBA(color, s->format, &r, &g, &b, &a);
   else
      SDL_GetRGB(color, s->format, &r, &g, &b);
}


// Video stuffs -------------------------------------------------------------------------

void vidSurface::lock()
{
   if(mustlock)
      SDL_LockSurface(s);

   locks++;
   buffer = (Uint8 *)s->pixels;
}


void vidSurface::unlock()
{
   if(locks)
   {
      if(mustlock)
         SDL_UnlockSurface(s);

      locks--;
   }
   buffer = NULL;
}



// Image file functions -----------------------------------------------------------------
bool vidSurface::saveBMPFile(string filename)
{
   if(locks)
      basicError::Throw("vidSurface::saveBMPFile called on a locked surface\n");

   return SDL_SaveBMP(s, filename.c_str()) ? false: true;
}


vidSurface *vidSurface::loadBMPFile(string filename)
{
   SDL_Surface *surface = SDL_LoadBMP(filename.c_str());
   vidSurface *ret;

   if(!surface)
      return NULL;

   ret = new vidSurface(*surface, true);
   return ret;
}

// Screen stuffs ------------------------------------------------------------------------
vidSurface *vidSurface::setVideoMode(unsigned int w, unsigned int h, int bits, int sdlflags)
{
   if(screensurface)
      screensurface->noLongerScreen();

   SDL_Surface *nsurface = SDL_SetVideoMode(w, h, bits, sdlflags);
   if(!nsurface)
      return NULL;

   screensurface = new vidSurface(*nsurface, false);
   screensurface->isscreen = true;
   return screensurface;
}


vidSurface *vidSurface::resizeScreen(unsigned int w, unsigned int h)
{
   if(!screensurface)
      return NULL;

   vidSurface *c = new vidSurface(w, h, screensurface->bitdepth);
   screensurface->blitTo(*c, 0, 0, w, h, 0, 0);

   SDL_Surface *nsurface = SDL_SetVideoMode(w, h, screensurface->bitdepth, screensurface->s->flags);
   if(!nsurface)
   {
     fatalError::Throw("resizeScreen could not set new video mode %ix%ix%i\n", w, h, screensurface->bitdepth);
     return NULL;
   }

   screensurface->setSurface(*nsurface, false);
   screensurface->isscreen = true;
   c->blitTo(*screensurface, 0, 0, w, h, 0, 0);

   flipVideoPage();

   return screensurface;
}


void vidSurface::flipVideoPage()
{
   if(!screensurface)
      return;

   while(screensurface->locks)
      screensurface->unlock();
     //fatalError::Throw("flipVideoPage called while the screen surface was locked.\n");

   SDL_Flip(screensurface->s);
}




void vidSurface::updateRect(vidRect &r)
{
   if(!screensurface)
      return;

   if(screensurface->locks)
     basicError::Throw("updateRect called while the screen surface was locked.\n");

   SDL_UpdateRect(screensurface->s, r.rx(), r.ry(), r.rw(), r.rh());
}


// Protected helper functions -----------------------------------------------------------
void vidSurface::noLongerScreen()
{
   if(!screensurface || s != screensurface->s)
      return;
   // Create a copy of the object surface that is NOT a screen buffer.
   SDL_Surface *news = 
      SDL_CreateRGBSurfaceFrom(s->pixels, s->w, s->h, s->format->BitsPerPixel, s->pitch,
                              s->format->Rmask, s->format->Gmask, s->format->Bmask, 
                              s->format->Amask);

   if(!news)
     fatalError::Throw("noLongerScreen failed to allocate surface.\n");


   setSurface(*news, true);
   screensurface = NULL;
}

// Helper function for drawLine
void vidSurface::drawHLine(int x1, int x2, int y, Uint32 color)
{
   int swap = x1;
   int cx1 = cliprect.sx1();
   int cx2 = cliprect.sx2();
   int cy1 = cliprect.sy1();
   int cy2 = cliprect.sy2();

   if(x1 > x2)
   {
      x1 = x2;
      x2 = swap;
   }

   if(y < cy1 || y > cy2 || x1 > cx2 || x2 < cx1)
      return;

   if(x1 < cx1)
      x1 = cx1;

   if(x2 > cx2)
      x2 = cx2;

   if(x2 - x1 < 0)
      return;

   int x = x2 - x1 + 1;
   if(bitdepth == 8)
   {
      Uint8 *buf = (Uint8 *)buffer + (y * width) + x1;
      Uint8 c = color;
      while(x--)
         *(buf++) = c;
   }
   else if(bitdepth == 16)
   {
      Uint16 *buf = (Uint16 *)buffer + (y * width) + x1;
      while(x--)
         *(buf++) = color;
   }
   else if(bitdepth == 32)
   {
      Uint32 *buf = (Uint32 *)buffer + (y * width) + x1;
      while(x--)
         *(buf++) = color;
   }
}


// Helper function for drawLine
void vidSurface::drawVLine(int x, int y1, int y2, Uint32 color)
{
   int swap = y1;
   int cx1 = cliprect.sx1();
   int cx2 = cliprect.sx2();
   int cy1 = cliprect.sy1();
   int cy2 = cliprect.sy2();

   if(y1 > y2)
   {
      y1 = y2;
      y2 = swap;
   }

   if(x < cx1 || x > cx2 || y1 > cy2 || y2 < cy1)
      return;

   if(y1 < cy1)
      y1 = cy1;

   if(y2 > cy2)
      y2 = cy2;

   if(y2 - y1 < 0)
      return;

   int y = y2 - y1 + 1;
   if(bitdepth == 8)
   {
      Uint8 *buf = (Uint8 *)buffer + (y1 * width) + x;
      Uint8 c = color;
      while(y--)
      {*buf = color; buf += width;}
   }
   else if(bitdepth == 16)
   {
      Uint16 *buf = (Uint16 *)buffer + (y1 * width) + x;
      while(y--)
      {*buf = color; buf += width;}
   }
   else if(bitdepth == 32)
   {
      Uint32 *buf = (Uint32 *)buffer + (y1 * width) + x;
      while(y--)
      {*buf = color; buf += width;}
   }
}


// Helper function for drawLine
void vidSurface::drawPositiveLine(int x1, int y1, int x2, int y2, Uint32 color)
{
   int cx1 = cliprect.sx1();
   int cx2 = cliprect.sx2();
   int cy1 = cliprect.sy1();
   int cy2 = cliprect.sy2();
   int w, h;
   w = x2 - x1 + 1;
   h = y1 - y2 + 1;
  
   // perform clipping
   int newx, newy;
   // clip first end
   if(x1 < cx1)
   {
      newx = cx1;
      newy = y1 - ((cx1 - x1) * h) / w;
      x1 = newx; y1 = newy;
   }

   if(y1 > cy2)
   {
      newy = cy2;
      newx = x1 + ((y1 - cy2) * w) / h;
      x1 = newx; y1 = newy;
   }

   if(x2 > cx2)
   {
      newx = cx2;
      newy = y2 + ((x2 - cx2) * h) / w;
      x2 = newx; y2 = newy;
   }

   if(y2 < cy1)
   {
      newy = cy1;
      newx = x2 - ((cy1 - y2) * w) / h;
      x2 = newx; y2 = newy;
   }

   // clipped out of existence
   if(x1 > x2 || y1 < y2)
      return;

   w = x2 - x1 + 1;
   h = y1 - y2 + 1;

   if(w >= h)
   {
      int ymove = -width;
      int delta = h;

      if(bitdepth == 8)
      {
         Uint8 *buf = buffer + (y1 * width) + x1, col = color;

         for(int x = w; x--;)
         {
            *(buf ++) = col;

            if((delta += h) > w)
            {delta -= w; buf += ymove;}
         }
      }
      else if(bitdepth == 16)
      {
         Uint16 *buf = (Uint16 *)buffer + (y1 * width) + x1, col = color;

         for(int x = w; x--;)
         {
            *(buf ++) = col;

            if((delta += h) > w)
            {delta -= w; buf += ymove;}
         }
      }
      else if(bitdepth == 32)
      {
         Uint32 *buf = (Uint32 *)buffer + (y1 * width) + x1, col = color;

         for(int x = w; x--;)
         {
            *(buf ++) = col;

            if((delta += h) > w)
            {delta -= w; buf += ymove;}
         }
      }
   }
   else
   {
      int ymove = -width;
      int delta = w;

      if(bitdepth == 8)
      {
         Uint8 *buf = buffer + (y1 * width) + x1, col = color;

         for(int y = h; y--;)
         {
            *buf = col; buf += ymove;

            if((delta += w) > h)
            {delta -= h;buf++;}
         }
      }
      else if(bitdepth == 16)
      {
         Uint16 *buf = (Uint16 *)buffer + (y1 * width) + x1, col = color;

         for(int y = h; y--;)
         {
            *buf = col; buf += ymove;

            if((delta += w) > h)
            {delta -= h;buf++;}
         }
      }
      else if(bitdepth == 32)
      {
         Uint32 *buf = (Uint32 *)buffer + (y1 * width) + x1, col = color;

         for(int y = h; y--;)
         {
            *buf = col; buf += ymove;

            if((delta += w) > h)
            {delta -= h;buf++;}
         }
      }
   }         
}




// Helper function for drawLine
void vidSurface::drawNegativeLine(int x1, int y1, int x2, int y2, Uint32 color)
{
   int cx1 = cliprect.sx1();
   int cx2 = cliprect.sx2();
   int cy1 = cliprect.sy1();
   int cy2 = cliprect.sy2();
   int w, h;
   w = x2 - x1 + 1;
   h = y2 - y1 + 1;
  
   // perform clipping
   int newx, newy;
   // clip first end
   if(x1 < cx1)
   {
      newx = cx1;
      newy = y1 + ((cx1 - x1) * h) / w;
      x1 = newx; y1 = newy;
   }

   if(y1 < cy1)
   {
      newy = cy1;
      newx = x1 + ((cy1 - y1) * w) / h;
      x1 = newx; y1 = newy;
   }

   if(x2 > cx2)
   {
      newx = cx2;
      newy = y2 - ((x2 - cx2) * h) / w;
      x2 = newx; y2 = newy;
   }

   if(y2 > cy2)
   {
      newy = cy2;
      newx = x2 - ((y2 - cy2) * w) / h;
      x2 = newx; y2 = newy;
   }

   // clipped out of existence
   if(x1 > x2 || y1 > y2)
      return;

   w = x2 - x1 + 1;
   h = y2 - y1 + 1;

   if(w >= h)
   {
      int ymove = width;
      int delta = h;

      if(bitdepth == 8)
      {
         Uint8 *buf = buffer + (y1 * width) + x1, col = color;

         for(int x = w; x--;)
         {
            *(buf ++) = col;

            if((delta += h) > w)
            {delta -= w; buf += ymove;}
         }
      }
      else if(bitdepth == 16)
      {
         Uint16 *buf = (Uint16 *)buffer + (y1 * width) + x1, col = color;

         for(int x = w; x--;)
         {
            *(buf ++) = col;

            if((delta += h) > w)
            {delta -= w; buf += ymove;}
         }
      }
      else if(bitdepth == 32)
      {
         Uint32 *buf = (Uint32 *)buffer + (y1 * width) + x1, col = color;

         for(int x = w; x--;)
         {
            *(buf ++) = col;

            if((delta += h) > w)
            {delta -= w; buf += ymove;}
         }
      }
   }
   else
   {
      int ymove = width;
      int delta = w;

      if(bitdepth == 8)
      {
         Uint8 *buf = buffer + (y1 * width) + x1, col = color;

         for(int y = h; y--;)
         {
            *buf = col; buf += ymove;

            if((delta += w) > h)
            {delta -= h; buf++;}
         }
      }
      else if(bitdepth == 16)
      {
         Uint16 *buf = (Uint16 *)buffer + (y1 * width) + x1, col = color;

         for(int y = h; y--;)
         {
            *buf = col; buf += ymove;

            if((delta += w) > h)
            {delta -= h; buf++;}
         }
      }
      else if(bitdepth == 32)
      {
         Uint32 *buf = (Uint32 *)buffer + (y1 * width) + x1, col = color;

         for(int y = h; y--;)
         {
            *buf = col; buf += ymove;

            if((delta += w) > h)
            {delta -= h; buf++;}
         }
      }
   }         
}


void vidSurface::setSurface(SDL_Surface &surface, bool owner)
{
   SDL_Rect clipr;

   if(s && freesurface)
      SDL_FreeSurface(s);

   s = &surface;

   freesurface = owner;
   locks = 0;
   mustlock = SDL_MUSTLOCK(s);

   width = s->w;
   height = s->h;
   pitch = s->pitch;
   abnormalpitch = (s->w * s->format->BytesPerPixel) == pitch ? false : true;

   buffer = NULL;
   bitdepth = s->format->BitsPerPixel;
   SDL_GetClipRect(s, &clipr);
   cliprect.setSDLRect(clipr);
   isscreen = false;
}
//
// End::vidSurface ----------------------------------------------------------------------
//
