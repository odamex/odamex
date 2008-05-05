// -- 8-bit rendering functions --
#include <SDL.h>
#include <math.h>
#include "render.h"


extern rendercolumn_t column;
extern renderspan_t span;


Uint32 getFogColor8(Uint16 level, Uint8 r, Uint8 g, Uint8 b)
{
   return ((r * level) & 0xF800) | (((g * level) >> 5) & 0x7C0) | (((b * level) >> 11) & 0x1F);
}


void calcLight8(float distance, float scale, light_t *light, lighttype_e to)
{
   int li, minlight;
   Uint16 ulight, flight;
   float fog;

   minlight = light->l_level > 64 ? light->l_level - 64 : 0;
   li = light->l_level - (int)((256 - light->l_level) * scale * 0.5f);
   li = li > 256 ? 256 : li < minlight ? minlight : li;
   ulight = abs(li);


   fog = (distance - light->f_stop) / (light->f_start - light->f_stop);

   if(fog > 1.0f) 
      flight = 256;
   else if(fog < 0)
      flight = 0;
   else
      flight = (Uint16)(fog * 256);

   ulight = (int)(ulight * flight) >> 8;
   flight = 256 - flight;

   if(to == LT_COLUMN)
   {
      column.fogadd = getFogColor8(flight, light->f_r, light->f_g, light->f_b);

      column.l_r = (ulight * light->l_r) >> 10;
      column.l_g = (ulight * light->l_g) >> 10;
      column.l_b = (ulight * light->l_b) >> 10;
   }
   else if(to == LT_SPAN)
   {
      span.fogadd = getFogColor8(flight, light->f_r, light->f_g, light->f_b);
      span.l_r = (ulight * light->l_r) >> 10;
      span.l_g = (ulight * light->l_g) >> 10;
      span.l_b = (ulight * light->l_b) >> 10;
   }
}


// -- Column drawers -- 
extern int viewh, vieww;

void drawColumn8(void)
{
   Uint8 *source, *dest;
   int count;
   unsigned sstep = vieww, ystep = column.ystep, texy = column.yfrac;

   if((count = column.y2 - column.y1 + 1) < 0)
      return;

   source = ((Uint8 *)column.tex) + column.texx;
   dest = ((Uint8 *)column.screen) + (column.y1 * sstep) + column.x;

   while(count--)
   {
      *dest = source[(texy >> 16) & 0x3f];

      dest += sstep;
      texy += ystep;
   }
}



void drawColumnFog8(void)
{
   Uint8 *source, *dest;
   int count;
   unsigned sstep = vieww, ystep = column.ystep, texy = column.yfrac;

   if((count = column.y2 - column.y1 + 1) < 0)
      return;

   source = ((Uint8 *)column.tex) + column.texx;
   dest = ((Uint8 *)column.screen) + (column.y1 * sstep) + column.x;

   while(count--)
   {
      *dest = source[(texy >> 16) & 0x3f];

      dest += sstep;
      texy += ystep;
   }
}



// -- Span drawing --
extern renderspan_t span;

void drawSpan8(void)
{
   unsigned xf = span.xfrac, xs = span.xstep;
   unsigned yf = span.yfrac, ys = span.ystep;
   int count;
   
   if((count = span.x2 - span.x1 + 1) < 0)
      return;

   Uint8 *source = (Uint8 *)span.tex;
   Uint8 *dest = (Uint8 *)span.screen + (span.y * vieww) + span.x1;

   while(count--)
   {
      *dest++ = source[((xf >> 16) & 63) * 64 + ((yf >> 16) & 63)];
      xf += xs;
      yf += ys;
   }
}

void drawSpanFog8(void)
{
   unsigned xf = span.xfrac, xs = span.xstep;
   unsigned yf = span.yfrac, ys = span.ystep;
   int count;
   
   if((count = span.x2 - span.x1 + 1) < 0)
      return;

   Uint8 *source = (Uint8 *)span.tex;
   Uint8 *dest = (Uint8 *)span.screen + (span.y * vieww) + span.x1;

   while(count--)
   {
      *dest++ = source[((xf >> 16) & 63) * 64 + ((yf >> 16) & 63)];
      xf += xs;
      yf += ys;
   }
}


renderfunc_t render8 = {
getFogColor8,
calcLight8,
drawColumn8,
drawColumnFog8,
drawSpan8,
drawSpanFog8};
