#ifndef MAPDATA_H
#define MAPDATA_H

#include "light.h"

// ----- Map data -----
typedef unsigned int frameid_t;
extern frameid_t frameid;

typedef struct mapsector_s mapsector_t;

// -- Map vertex --
// The map vertices store the x and y (which translates to horizontal and distance in 3D)
// as a speed booster, the projected x and distance are stored in each vertex from the frame currently being rendered
// so each vertex only has to be projected once, except in cases where the vertex is clipped to the view plane, in which case
// the projection values will not be stored.
typedef struct
{
   float x, y;

   frameid_t   frameid;
   float proj_x, distance, idistance;
   float t_x, t_y;
} mapvertex_t;



// -- Map lineside --
typedef struct
{
   float xoffset, yoffset;
   float xscale, yscale;

   mapsector_t *sector;
} mapside_t;



// -- Map line --
typedef struct
{
   mapvertex_t *v1, *v2;
   
   mapside_t *side[2];

   mapsector_t *sector1, *sector2;
} mapline_t;



// -- Map sector --
// For my purposes now I'm going to assume that all sectors are closed and convex.
struct mapsector_s
{
   float floorz, ceilingz;
   float f_xoff, f_yoff;
   float c_xoff, c_yoff;

   light_t light;

   Uint32 linecount;
   mapline_t **lines;
};



void nextFrameID();

#endif //MAPDATA_H
