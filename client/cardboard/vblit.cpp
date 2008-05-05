#include <SDL.h>
#include "video.h"
#include "error.h"
#include "pixelmath.h"

// 
// Begin::vidSurface --------------------------------------------------------------------
//

// Blitting -----------------------------------------------------------------------------
void vidSurface::blitTo(vidSurface &dest, vidRect srcrect, vidRect destrect)
{
   if(locks || dest.locks)
      basicError::Throw("vidSurface::blitTo called with a locked surface\n");

   SDL_Rect r1 = srcrect.getSDLRect();
   SDL_Rect r2 = destrect.getSDLRect();

   r2.w = r1.w;
   r2.h = r1.h;

   SDL_BlitSurface(s, &r1, dest.s, &r2);
}


void vidSurface::blitTo(vidSurface &dest, int srcx, int srcy, unsigned int w, unsigned int h, int destx, int desty)
{
   if(locks || dest.locks)
      basicError::Throw("vidSurface::blitTo called with a locked surface\n");

   SDL_Rect r1;
   SDL_Rect r2;

   r1.x = srcx; r1.y = srcy; 
   r2.x = destx; r2.y = desty;
   r1.w = r2.w = w; r1.h = r2.h = h;

   SDL_BlitSurface(s, &r1, dest.s, &r2);
}


void vidSurface::stretchBlitTo(vidSurface &dest, vidRect srcrect, vidRect destrect)
{
   bool lock1 = false, lock2 = false;
   Uint8 *srcrow, *destrow;// pointers to the current row
   Uint8 *psrc, *pdest;    // pointers to the current pixels
   int  p1, p2;            // the two pitch values
   int  widths, widthd;    // the width scale, and width delta
   int  widthl;            // width limit
   int  heights, heightd;  // the height scale, and height delta
   int  heightl;           // height limit
   int  y, x, pixelsize;
   Uint16 *d2, *s2;
   Uint32 *d4, *s4;
   int  rowsize;

   srcrect.clipRect(cliprect);
   destrect.clipRect(dest.cliprect);

   if(!locks)
   {
      lock();
      lock1 = true;
   }

   if(!dest.locks)
   {
      dest.lock();
      lock2 = true;
   }

   pixelsize = dest.bitdepth / 8;

   // Set up the pointers
   p1 = pitch;
   srcrow = getBuffer() + (srcrect.rx() * pixelsize) + (p1 * srcrect.ry());

   p2 = dest.pitch;
   destrow = dest.getBuffer() + (destrect.rx() * pixelsize) + (p2 * destrect.ry());

   widthl = destrect.rw();
   widthd = srcrect.rw();
   heightl = destrect.rh();
   heightd = srcrect.rh();
   heights = 0;

   rowsize = pixelsize * destrect.rw();

   if(s->format->BitsPerPixel == dest.s->format->BitsPerPixel && 
      s->format->Rmask == dest.s->format->Rmask &&
      s->format->Gmask == dest.s->format->Gmask &&
      s->format->Bmask == dest.s->format->Bmask &&
      s->format->Amask == dest.s->format->Amask)
   {
      for(y = destrect.rh(); y--;)
      {
         widths = 0;

         switch(bitdepth)
         {
            case 8:
               pdest = destrow;
               psrc = srcrow;
               for(x = destrect.rw(); x--;)
               {
                  *pdest++ = *psrc;
                  widths += widthd;
                  while(widths >= widthl)
                  {
                     psrc ++;
                     widths -= widthl;
                  }
               }
               break;
            case 16:
               d2 = (Uint16 *)destrow, s2 = (Uint16 *)srcrow;
               for(x = destrect.rw(); x--;)
               {
                  *d2++ = *s2;
                  widths += widthd;
                  while(widths >= widthl)
                  {
                     s2++;
                     widths -= widthl;
                  }
               }
               break;
            case 24:
               pdest = destrow;
               psrc = srcrow;
               for(x = destrect.rw(); x--;)
               {
                  memcpy(pdest, psrc, 3);
                  pdest += 3;
                  widths += widthd;
                  while(widths >= widthl)
                  {
                     psrc += 3;
                     widths -= widthl;
                  }
               }
               break;
            case 32:
               d4 = (Uint32 *)destrow, s4 = (Uint32 *)srcrow;

               for(x = destrect.rw(); x--;)
               {
                  *d4++ = *s4;
                  widths += widthd;
                  while(widths >= widthl)
                  {
                     s4++;
                     widths -= widthl;
                  }
               }
               break;
         };
   
         heights += heightd;

         while(heights >= heightl)
         {
            heights -= heightl;
            srcrow += p1;
         }

         destrow += p2;
      }
   }

   if(lock1)
      unlock();

   if(lock2)
      dest.unlock();
}


void vidSurface::stretchBlitAddTo(vidSurface &dest, vidRect srcrect, vidRect destrect)
{
   bool lock1 = false, lock2 = false;
   Uint8 *srcrow, *destrow;// pointers to the current row
   Uint8 *psrc, *pdest;    // pointers to the current pixels
   int  p1, p2;            // the two pitch values
   int  widths, widthd;    // the width scale, and width delta
   int  widthl;            // width limit
   int  heights, heightd;  // the height scale, and height delta
   int  heightl;           // height limit
   int  y, x, pixelsize;
   Uint16 *d2, *s2;
   Uint32 *d4, *s4;
   int  rowsize;

   srcrect.clipRect(cliprect);
   destrect.clipRect(dest.cliprect);

   if(!locks)
   {
      lock();
      lock1 = true;
   }

   if(!dest.locks)
   {
      dest.lock();
      lock2 = true;
   }

   pixelsize = dest.bitdepth / 8;

   // Set up the pointers
   p1 = pitch;
   srcrow = getBuffer() + (srcrect.rx() * pixelsize) + (p1 * srcrect.ry());

   p2 = dest.pitch;
   destrow = dest.getBuffer() + (destrect.rx() * pixelsize) + (p2 * destrect.ry());

   widthl = destrect.rw();
   widthd = srcrect.rw();
   heightl = destrect.rh();
   heightd = srcrect.rh();
   heights = 0;

   rowsize = pixelsize * destrect.rw();

   if(s->format->BitsPerPixel == dest.s->format->BitsPerPixel && 
      s->format->Rmask == dest.s->format->Rmask &&
      s->format->Gmask == dest.s->format->Gmask &&
      s->format->Bmask == dest.s->format->Bmask &&
      s->format->Amask == dest.s->format->Amask)
   {
      for(y = destrect.rh(); y--;)
      {
         widths = 0;

         switch(bitdepth)
         {
            case 16:
               d2 = (Uint16 *)destrow, s2 = (Uint16 *)srcrow;
               for(x = destrect.rw(); x--;)
               {
                  ADDPX16(*d2, *s2); d2++;
                  widths += widthd;
                  while(widths >= widthl)
                  {
                     s2++;
                     widths -= widthl;
                  }
               }
               break;
            case 24:
               pdest = destrow;
               psrc = srcrow;
               for(x = destrect.rw(); x--;)
               {
                  ADDPX24(*(Uint32 *)pdest, *(Uint32 *)psrc);
                  pdest += 3;
                  widths += widthd;
                  while(widths >= widthl)
                  {
                     psrc += 3;
                     widths -= widthl;
                  }
               }
               break;
            case 32:
               d4 = (Uint32 *)destrow, s4 = (Uint32 *)srcrow;

               for(x = destrect.rw(); x--;)
               {
                  ADDPX32(*d4, *s4); d4++;
                  widths += widthd;
                  while(widths >= widthl)
                  {
                     s4++;
                     widths -= widthl;
                  }
               }
               break;

            default:
               basicError::Throw("stretchBlitAddTo supports only 16, 24, and 32 but color modes.\n");
         };
   
         heights += heightd;

         while(heights >= heightl)
         {
            heights -= heightl;
            srcrow += p1;
         }

         destrow += p2;
      }
   }

   if(lock1)
      unlock();

   if(lock2)
      dest.unlock();
}





void vidSurface::stretchBlitTransTo(vidSurface &dest, vidRect srcrect, vidRect destrect, unsigned char amount)
{
   bool lock1 = false, lock2 = false;
   Uint8 *srcrow, *destrow;// pointers to the current row
   Uint8 *psrc, *pdest;    // pointers to the current pixels
   int  p1, p2;            // the two pitch values
   int  widths, widthd;    // the width scale, and width delta
   int  widthl;            // width limit
   int  heights, heightd;  // the height scale, and height delta
   int  heightl;           // height limit
   int  y, x, pixelsize;
   Uint16 *d2, *s2;
   Uint32 *d4, *s4;
   int  rowsize;
   unsigned char amount2 = 255 - amount;

   srcrect.clipRect(cliprect);
   destrect.clipRect(dest.cliprect);

   if(!locks)
   {
      lock();
      lock1 = true;
   }

   if(!dest.locks)
   {
      dest.lock();
      lock2 = true;
   }

   pixelsize = dest.bitdepth / 8;

   // Set up the translucency
   if(bitdepth == 16)
   {
      amount >>= 2;
      amount2 >>= 2;
   }

   // Set up the pointers
   p1 = pitch;
   srcrow = getBuffer() + (srcrect.rx() * pixelsize) + (p1 * srcrect.ry());

   p2 = dest.pitch;
   destrow = dest.getBuffer() + (destrect.rx() * pixelsize) + (p2 * destrect.ry());

   widthl = destrect.rw();
   widthd = srcrect.rw();
   heightl = destrect.rh();
   heightd = srcrect.rh();
   heights = 0;

   rowsize = pixelsize * destrect.rw();

   if(s->format->BitsPerPixel == dest.s->format->BitsPerPixel && 
      s->format->Rmask == dest.s->format->Rmask &&
      s->format->Gmask == dest.s->format->Gmask &&
      s->format->Bmask == dest.s->format->Bmask &&
      s->format->Amask == dest.s->format->Amask)
   {
      for(y = destrect.rh(); y--;)
      {
         widths = 0;

         switch(bitdepth)
         {
            case 16:
               d2 = (Uint16 *)destrow, s2 = (Uint16 *)srcrow;
               for(x = destrect.rw(); x--;)
               {
                  TRANPX16i(*d2, *s2, amount, amount2); d2++;
                  widths += widthd;
                  while(widths >= widthl)
                  {
                     s2++;
                     widths -= widthl;
                  }
               }
               break;
            case 24:
               pdest = destrow;
               psrc = srcrow;
               for(x = destrect.rw(); x--;)
               {
                  TRANPX24i(*(Uint32 *)pdest, *(Uint32 *)psrc, amount, amount2);
                  pdest += 3;
                  widths += widthd;
                  while(widths >= widthl)
                  {
                     psrc += 3;
                     widths -= widthl;
                  }
               }
               break;
            case 32:
               d4 = (Uint32 *)destrow, s4 = (Uint32 *)srcrow;

               for(x = destrect.rw(); x--;)
               {
                  TRANPX32i(*d4, *s4, amount, amount2); d4++;
                  widths += widthd;
                  while(widths >= widthl)
                  {
                     s4++;
                     widths -= widthl;
                  }
               }
               break;

            default:
               basicError::Throw("stretchBlitAddTo supports only 16, 24, and 32 but color modes.\n");
         };
   
         heights += heightd;

         while(heights >= heightl)
         {
            heights -= heightl;
            srcrow += p1;
         }

         destrow += p2;
      }
   }

   if(lock1)
      unlock();

   if(lock2)
      dest.unlock();
}




void vidSurface::stretchBlitLitedTo(vidSurface &dest, vidRect srcrect, vidRect destrect, Uint8 rlight, Uint8 glight, Uint8 blight)
{
   bool lock1 = false, lock2 = false;
   Uint8 *srcrow, *destrow;// pointers to the current row
   Uint8 *psrc, *pdest;    // pointers to the current pixels
   int  p1, p2;            // the two pitch values
   int  widths, widthd;    // the width scale, and width delta
   int  widthl;            // width limit
   int  heights, heightd;  // the height scale, and height delta
   int  heightl;           // height limit
   int  y, x, pixelsize;
   Uint16 *d2, *s2, subpixel16;
   Uint32 *d4, *s4, subpixel32;
   int  rowsize;

   srcrect.clipRect(cliprect);
   destrect.clipRect(dest.cliprect);

   if(!locks)
   {
      lock();
      lock1 = true;
   }

   if(!dest.locks)
   {
      dest.lock();
      lock2 = true;
   }

   pixelsize = dest.bitdepth / 8;

   // Set up the translucency
   if(bitdepth == 16)
   {
      subpixel16 = (((rlight) >> s->format->Rloss) << s->format->Rshift) |
                   (((glight) >> s->format->Gloss) << s->format->Gshift) |
                   (((blight) >> s->format->Bloss) << s->format->Bshift);
   }
   else
   {
      subpixel32 = ((rlight) << s->format->Rshift) |
                   ((glight) << s->format->Gshift) |
                   ((blight) << s->format->Bshift);
   }

   // Set up the pointers
   p1 = pitch;
   srcrow = getBuffer() + (srcrect.rx() * pixelsize) + (p1 * srcrect.ry());

   p2 = dest.pitch;
   destrow = dest.getBuffer() + (destrect.rx() * pixelsize) + (p2 * destrect.ry());

   widthl = destrect.rw();
   widthd = srcrect.rw();
   heightl = destrect.rh();
   heightd = srcrect.rh();
   heights = 0;

   rowsize = pixelsize * destrect.rw();

   if(s->format->BitsPerPixel == dest.s->format->BitsPerPixel && 
      s->format->Rmask == dest.s->format->Rmask &&
      s->format->Gmask == dest.s->format->Gmask &&
      s->format->Bmask == dest.s->format->Bmask &&
      s->format->Amask == dest.s->format->Amask)
   {
      for(y = destrect.rh(); y--;)
      {
         widths = 0;

         switch(bitdepth)
         {
            case 16:
               d2 = (Uint16 *)destrow, s2 = (Uint16 *)srcrow;
               for(x = destrect.rw(); x--;)
               {
                  LIGHTPX16(*d2, *s2, subpixel16); d2++;
                  widths += widthd;
                  while(widths >= widthl)
                  {
                     s2++;
                     widths -= widthl;
                  }
               }
               break;
            case 24:
               pdest = destrow;
               psrc = srcrow;
               for(x = destrect.rw(); x--;)
               {
                  LIGHTPX24(*(Uint32 *)pdest, *(Uint32 *)psrc, subpixel32);
                  pdest += 3;
                  widths += widthd;
                  while(widths >= widthl)
                  {
                     psrc += 3;
                     widths -= widthl;
                  }
               }
               break;
            case 32:
               d4 = (Uint32 *)destrow, s4 = (Uint32 *)srcrow;

               for(x = destrect.rw(); x--;)
               {
                  LIGHTPX32(*d4, *s4, subpixel32); d4++;
                  widths += widthd;
                  while(widths >= widthl)
                  {
                     s4++;
                     widths -= widthl;
                  }
               }
               break;

            default:
               basicError::Throw("stretchBlitAddTo supports only 16, 24, and 32 but color modes.\n");
         };
   
         heights += heightd;

         while(heights >= heightl)
         {
            heights -= heightl;
            srcrow += p1;
         }

         destrow += p2;
      }
   }

   if(lock1)
      unlock();

   if(lock2)
      dest.unlock();
}




/*void vidSurface::filterBlitTo(vidSurface &dest, vidRect srcrect, vidRect destrect)
{
   bool lock1 = false, lock2 = false;
   Uint8 *destrow, *destp;
   Uint8 *srcrow1, *srcrow2, *srcp1, *srcp2;
   int srcpitch, destpitch;
   int widths, widthd, widthl;
   int heights, heightd, heightl;
   int  y, x, pixelsize;

   srcrect.clipRect(cliprect);
   destrect.clipRect(dest.cliprect);

   if(!locks)
   {
      lock();
      lock1 = true;
   }

   if(!dest.locks)
   {
      dest.lock();
      lock2 = true;
   }

   pixelsize = bitdepth / 8;

   destrow = dest.getBuffer();
   destpitch = dest.pitch;
   destrow += (destrect.rx() * pixelsize) + (destpitch * destrect.ry());

   srcrow1 = getBuffer();
   srcpitch = pitch;
   srcrow1 += (srcrect.rx() * pixelsize) + (srcpitch * srcrect.ry());

   if(lock1)
      unlock();

   if(lock2)
      dest.unlock();
}*/







//
// End::vidSurface ----------------------------------------------------------------------
//
