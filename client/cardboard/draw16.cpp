// -- 16-bit rendering functions --
#include <SDL.h>
#include <math.h>
#include "render.h"


extern rendercolumn_t column;
extern renderspan_t span;


Uint32 getFogColor16(Uint16 level, Uint8 r, Uint8 g, Uint8 b)
{
   return ((r * level) & 0xF800) | (((g * level) >> 5) & 0x7C0) | (((b * level) >> 11) & 0x1F);
}


void calcLight16(float distance, float scale, light_t *light, lighttype_e to)
{
   int li, minlight;
   Uint16 ulight, flight;
   float fog;

   minlight = light->l_level > 64 ? light->l_level - 64 : 0;
   li = light->l_level - (int)((256 - light->l_level) * scale * 0.5f);
   li = li > 256 ? 256 : li < minlight ? minlight : li;
   ulight = abs(li);


   fog = (distance - light->f_stop) / (light->f_start - light->f_stop);
   flight = fog > 1.0f ? 256 : fog < 0 ? 0 : (Uint16)(fog * 256);
   ulight = (int)(ulight * flight) >> 8;
   flight = 256 - flight;

   if(to == LT_COLUMN)
   {
      column.fogadd = getFogColor16(flight, light->f_r, light->f_g, light->f_b);

      column.l_r = (ulight * light->l_r) >> 8;
      column.l_g = (ulight * light->l_g) >> 8;
      column.l_b = (ulight * light->l_b) >> 8;
   }
   else if(to == LT_SPAN)
   {
      span.fogadd = getFogColor16(flight, light->f_r, light->f_g, light->f_b);
      span.l_r = (ulight * light->l_r) >> 8;
      span.l_g = (ulight * light->l_g) >> 8;
      span.l_b = (ulight * light->l_b) >> 8;
   }
}


// -- Column drawers -- 
extern int viewh, vieww;

void drawColumn16(void)
{
   Uint16 *source, *dest;
   int count;
   unsigned sstep = vieww, ystep = column.ystep, texy = column.yfrac;
   Uint16 r = column.l_r, g = column.l_g, b = column.l_b;

   if((count = column.y2 - column.y1 + 1) < 0)
      return;

   source = ((Uint16 *)column.tex) + column.texx;
   dest = ((Uint16 *)column.screen) + (column.y1 * sstep) + column.x;

   while(count--)
   {
      *dest = (((source[(texy >> 16) & 0x3f] & 0x1F) * b)
         | (((source[(texy >> 16) & 0x3f] & 0x07C0) * g) & 0x7C000)
         | (((source[(texy >> 16) & 0x3f] & 0xF800) * r) & 0xF80000)) >> 8;

      dest += sstep;
      texy += ystep;
   }
}



void drawColumnFog16(void)
{
   register Uint16 *source, *dest;
   int count;
   unsigned sstep = vieww, ystep = column.ystep, texy = column.yfrac;
   Uint16 r = column.l_r, g = column.l_g, b = column.l_b, fogadd = (Uint16)column.fogadd;

   if((count = column.y2 - column.y1 + 1) < 0)
      return;

   source = ((Uint16 *)column.tex) + column.texx;
   dest = ((Uint16 *)column.screen) + (column.y1 * sstep) + column.x;

   while(count--)
   {
      *dest = ((((source[(texy >> 16) & 0x3f] & 0x1F) * b)
         | (((source[(texy >> 16) & 0x3f] & 0x07C0) * g) & 0x7C000)
         | (((source[(texy >> 16) & 0x3f] & 0xF800) * r) & 0xF80000)) >> 8) + fogadd;

      dest += sstep;
      texy += ystep;
   }
}



// -- Span drawing --
extern renderspan_t span;

void drawSpan16(void)
{
   unsigned xf = span.xfrac, xs = span.xstep;
   unsigned yf = span.yfrac, ys = span.ystep;
   int count;
   Uint16 r = span.l_r, g = span.l_g, b = span.l_b;
   
   if((count = span.x2 - span.x1 + 1) < 0)
      return;

   Uint16 *source = (Uint16 *)span.tex;
   Uint16 *dest = (Uint16 *)span.screen + (span.y * vieww) + span.x1;

   while(count--)
   {
      *dest = (((source[((xf >> 16) & 63) * 64 + ((yf >> 16) & 63)] & 0x1F) * b)
         | (((source[((xf >> 16) & 63) * 64 + ((yf >> 16) & 63)] & 0x07C0) * g) & 0x7C000)
         | (((source[((xf >> 16) & 63) * 64 + ((yf >> 16) & 63)] & 0xF800) * r) & 0xF80000)) >> 8;
      dest++;
      xf += xs;
      yf += ys;
   }
}

void drawSpanFog16(void)
{
   unsigned xf = span.xfrac, xs = span.xstep;
   unsigned yf = span.yfrac, ys = span.ystep;
   int count;
   Uint16 r = span.l_r, g = span.l_g, b = span.l_b, fogadd = (Uint16)span.fogadd;
   
   if((count = span.x2 - span.x1 + 1) < 0)
      return;

   Uint16 *source = (Uint16 *)span.tex;
   Uint16 *dest = (Uint16 *)span.screen + (span.y * vieww) + span.x1;

   while(count--)
   {
      *dest = ((((source[((xf >> 16) & 63) * 64 + ((yf >> 16) & 63)] & 0x1F) * b)
         | (((source[((xf >> 16) & 63) * 64 + ((yf >> 16) & 63)] & 0x07C0) * g) & 0x7C000)
         | (((source[((xf >> 16) & 63) * 64 + ((yf >> 16) & 63)] & 0xF800) * r) & 0xF80000)) >> 8) + fogadd;
      dest++;
      xf += xs;
      yf += ys;
   }
}


renderfunc_t render16 = {
getFogColor16,
calcLight16,
drawColumn16,
drawColumnFog16,
drawSpan16,
drawSpanFog16};
