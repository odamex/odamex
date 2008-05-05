#include <SDL.h>
#include "mapdata.h"

Uint32   vertexcount = 6;
mapvertex_t vertexlist[] = {
{ -64,      64},    // 0
{  64,      64},    // 1
{  96,       0},    // 2
{  96,     -512},    // 3
{ -64,     -512},    // 4
{   0,      128}};


Uint32   sidecount = 6;
mapside_t sidelist[] = {
{0, 0, 1.0f, 1.0f, (mapsector_t *)1},
{0, 0, 1.0f, 1.0f, (mapsector_t *)1},
{0, 0, 1.0f, 1.0f, (mapsector_t *)1},
{0, 0, 1.0f, 1.0f, (mapsector_t *)1},
{0, 0, 1.0f, 1.0f, (mapsector_t *)1},
{0, 0, 1.0f, 1.0f, (mapsector_t *)2}};


Uint32   linecount = 7;
mapline_t linelist[] = {
{&vertexlist[0], &vertexlist[1], {&sidelist[0], &sidelist[5]}, (mapsector_t *)1, (mapsector_t *)2},
{&vertexlist[1], &vertexlist[2], {&sidelist[1], NULL}, (mapsector_t *)1, NULL},
{&vertexlist[2], &vertexlist[3], {&sidelist[2], NULL}, (mapsector_t *)1, NULL},
{&vertexlist[3], &vertexlist[4], {&sidelist[3], NULL}, (mapsector_t *)1, NULL},
{&vertexlist[4], &vertexlist[0], {&sidelist[4], NULL}, (mapsector_t *)1, NULL},
{&vertexlist[0], &vertexlist[5], {&sidelist[5], NULL}, (mapsector_t *)2, NULL},
{&vertexlist[5], &vertexlist[1], {&sidelist[5], NULL}, (mapsector_t *)2, NULL},
};

Uint32   sectorcount = 2;
mapsector_t sectorlist[] = {
{-64.0f, 64.0f, 0, 0, 0, 0, {144, 256, 256, 256, 0, 0, 100, 0, 0}, NULL},
{-32.0f, 32.0f, 0, 0, 0, 0, {208, 256, 256, 256, 0, 0, 100, 0, 0}, NULL}};



frameid_t    frameid = 0;


void nextFrameID()
{
   frameid++;

   if(!frameid)
   {
      frameid = 1;

      for(unsigned i = 0; i < vertexcount; i++)
         vertexlist[i].frameid = 0;
   }
}


void hackMapData()
{
   Uint32 i, p;

   for(i = 0; i < vertexcount; i++)
      vertexlist[i].frameid = 0;
   
   for(i = 0; i < sidecount; i++)
      sidelist[i].sector = sectorlist + ((size_t)sidelist[i].sector - 1);

   for(i = 0; i < linecount; i++)
   {
      linelist[i].sector1 = 
         linelist[i].sector1 != NULL 
         ? sectorlist + ((size_t)linelist[i].sector1 - 1)
         : NULL;

      linelist[i].sector2 = 
         linelist[i].sector2 != NULL
         ? sectorlist + ((size_t)linelist[i].sector2 - 1)
         : NULL;
   }

   for(i = 0; i < sectorcount; i++)
   {
      Uint32 count = 0;
      for(p = 0; p < linecount; p++)
      {
         if(linelist[p].sector1 == sectorlist + i ||
            linelist[p].sector2 == sectorlist + i)
            count++;
      }

      sectorlist[i].lines = (mapline_t **)malloc(sizeof(mapline_t **) * count);
      sectorlist[i].linecount = count;

      count = 0;
      for(p = 0; p < linecount; p++)
      {
         if(linelist[p].sector1 == sectorlist + i ||
            linelist[p].sector2 == sectorlist + i)
            sectorlist[i].lines[count++] = linelist + p;
      }
   }
}




void projectWall(mapline_t *line);


void renderEverything()
{
   unsigned i;

   for(i = 0; i < linecount; i++)
   {
      projectWall(&linelist[i]);
   }
}
