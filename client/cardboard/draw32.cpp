// -- 32-bit rendering functions --
#include <SDL.h>
#include <math.h>
#include "render.h"
#include "mapdata.h"


extern rendercolumn_t column;
extern renderspan_t span;


Uint32 getFogColor32(Uint16 level, Uint8 r, Uint8 g, Uint8 b)
{
   return (((r * level) << 8) & 0xff0000) |
          ((g * level) & 0xff00) |
          ((b * level) >> 8);
}



void calcLight32(float distance, float scale, light_t *light, lighttype_e to)
{
   int li, maxlight, dscale;
   Uint16 ulight, flight;
   float fog;

   // Turns out the lighting in doom is a simple linear equation
   // the minimum a light can scale on a wall is
   // 2x - 224, the maximum is 2x - 40
   // the amount added for scaling is distance * 2560 * 4
   // this is added to the value for minimum light and that is then clipped to the 
   // maximum light value and you have your result!

   maxlight = light->l_level * 2 - 40;

   if(maxlight < 0)
      ulight = 0;
   else
   {
      if(maxlight > 256) // clip max light
         maxlight = 256;

      // calc the minimum light
      li = light->l_level * 2 - 224;
      // calc the added amount
      dscale = (int)(distance * 2560.0f * 4);
      
      // the increased light is added
      li += dscale; 
      // and the result clipped
      li = li > maxlight ? maxlight : li < 0 ? 0 : li;

      // The scalars for cardboard are fixed point 1.6 numbers 
      ulight = abs(li & 0x1F8);
   }

   if(light->f_start)
   {
      fog = (distance - light->f_stop) / (light->f_start - light->f_stop);
      flight = fog > 1.0f ? 256 : fog < 0 ? 0 : (Uint16)(fog * 256);
      ulight = (int)(ulight * flight) >> 8;
      flight = 256 - flight;
   }
   else
      flight = 0;

   if(to == LT_COLUMN)
   {
      column.fogadd = getFogColor32(flight, light->f_r, light->f_g, light->f_b);

      column.l_r = (ulight * light->l_r) >> 8;
      column.l_g = (ulight * light->l_g) >> 8;
      column.l_b = (ulight * light->l_b) >> 8;
   }
   else if(to == LT_SPAN)
   {
      span.fogadd = getFogColor32(flight, light->f_r, light->f_g, light->f_b);
      span.l_r = (ulight * light->l_r) >> 8;
      span.l_g = (ulight * light->l_g) >> 8;
      span.l_b = (ulight * light->l_b) >> 8;
   }
}


// -- Column drawers -- 
extern int viewh, vieww;

void drawColumn32(void)
{
   Uint32 *source, *dest;
   int count;
   unsigned sstep = vieww, ystep = column.ystep, texy = column.yfrac;
   Uint16 r = column.l_r, g = column.l_g, b = column.l_b;

   if((count = column.y2 - column.y1 + 1) < 0)
      return;

   source = ((Uint32 *)column.tex) + column.texx;
   dest = ((Uint32 *)column.screen) + (column.y1 * sstep) + column.x;

   while(count--)
   {
      *dest = (((source[(texy >> 16) & 0x3f] & 0xFF) * b)
         | (((source[(texy >> 16) & 0x3f] & 0xFF00) * g) & 0xFF0000)
         | ((source[(texy >> 16) & 0x3f] * r) & 0xFF000000)) >> 8;

      dest += sstep;
      texy += ystep;
   }
}



void drawColumnFog32(void)
{
   Uint32 *source, *dest;
   int count;
   unsigned sstep = vieww, ystep = column.ystep, texy = column.yfrac;
   Uint16 r = column.l_r, g = column.l_g, b = column.l_b;
   Uint32 fogadd = column.fogadd;

   if((count = column.y2 - column.y1 + 1) < 0)
      return;

   source = ((Uint32 *)column.tex) + column.texx;
   dest = ((Uint32 *)column.screen) + (column.y1 * sstep) + column.x;

   while(count--)
   {
      *dest = (((((source[(texy >> 16) & 0x3f] & 0xFF) * b)
         | (((source[(texy >> 16) & 0x3f] & 0xFF00) * g) & 0xFF0000)
         | ((source[(texy >> 16) & 0x3f] * r) & 0xFF000000))) >> 8) + fogadd;

      dest += sstep;
      texy += ystep;
   }
}



// -- Span drawing --
extern renderspan_t span;

void drawSpan32(void)
{
   unsigned xf = span.xfrac, xs = span.xstep;
   unsigned yf = span.yfrac, ys = span.ystep;
   int count;
   Uint16 r = span.l_r, g = span.l_g, b = span.l_b;
   
   if((count = span.x2 - span.x1 + 1) < 0)
      return;

   Uint32 *source = (Uint32 *)span.tex;
   Uint32 *dest = (Uint32 *)span.screen + (span.y * vieww) + span.x1;

   while(count--)
   {
      *dest = ((((source[((xf >> 16) & 63) * 64 + ((yf >> 16) & 63)] & 0xFF) * g)
         | (((source[((xf >> 16) & 63) * 64 + ((yf >> 16) & 63)] & 0xFF00) * b) & 0xFF0000)
         | ((source[((xf >> 16) & 63) * 64 + ((yf >> 16) & 63)] * r) & 0xFF000000))) >> 8;
      dest++;
      xf += xs;
      yf += ys;
   }
}

void drawSpanFog32(void)
{
   unsigned xf = span.xfrac, xs = span.xstep;
   unsigned yf = span.yfrac, ys = span.ystep;
   int count;
   Uint16 r = span.l_r, g = span.l_g, b = span.l_b;
   Uint32 fogadd = span.fogadd;
   
   if((count = span.x2 - span.x1 + 1) < 0)
      return;

   Uint32 *source = (Uint32 *)span.tex;
   Uint32 *dest = (Uint32 *)span.screen + (span.y * vieww) + span.x1;

   while(count--)
   {
      *dest = (((((source[((xf >> 10) & 0xFC0) + ((yf >> 16) & 63)] & 0xFF) * b)
         | (((source[((xf >> 10) & 0xFC0) + ((yf >> 16) & 63)] & 0xFF00) * g) & 0xFF0000)
         | ((source[((xf >> 10) & 0xFC0) + ((yf >> 16) & 63)] * r) & 0xFF000000))) >> 8) + fogadd;
      dest++;
      xf += xs;
      yf += ys;
   }
}


renderfunc_t render32 = {
getFogColor32,
calcLight32,
drawColumn32,
drawColumnFog32,
drawSpan32,
drawSpanFog32};
