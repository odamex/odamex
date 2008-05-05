#ifndef VISPLANE_H
#define VISPLANE_H

#include "render.h"

typedef struct visplane_s
{
   struct visplane_s *next, *child;

   float z;
   light_t light;

   int x1, x2;
   int pad1, top[MAX_WIDTH], pad2, pad3, bot[MAX_WIDTH], pad4;
} visplane_t;


visplane_t *findVisplane(float z, light_t *light);
visplane_t *checkVisplane(visplane_t *check, int x1, int x2);

void renderVisplanes();

void unlinkPlanes();
#endif
