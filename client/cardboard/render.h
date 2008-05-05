#ifndef RENDER_H
#define RENDER_H

#include "light.h"


#define MAX_WIDTH  1280
#define MAX_HEIGHT 1024

// -- Rendering --
typedef struct rendercolumn_s
{
   int x;
   int y1, y2;
   int yfrac, ystep;
   int texx;

   Uint16  l_r, l_g, l_b;
   Uint32  fogadd;

   void *tex, *screen;
} rendercolumn_t;



typedef struct renderspan_s
{
   int x1, x2, y;
   int xfrac, yfrac, xstep, ystep;

   Uint16 l_r, l_g, l_b;
   Uint32 fogadd;

   void *tex, *screen;
} renderspan_t;


// -- bit-specific functions --
typedef enum
{
   LT_COLUMN,
   LT_SPAN
} lighttype_e;

typedef struct renderfunc_s
{
   Uint32 (*getFogColor)(Uint16, Uint8, Uint8, Uint8);
   void (*calcLight)(float, float, light_t *, lighttype_e);

   void (*drawColumn)(void);
   void (*drawColumnFog)(void);

   void (*drawSpan)(void);
   void (*drawSpanFog)(void);
} renderfunc_t;


extern renderfunc_t *render;
extern renderfunc_t render8;
extern renderfunc_t render16;
extern renderfunc_t render32;

#endif
