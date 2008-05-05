#include <SDL.h>
#include <math.h>
#include "mapdata.h"
#include "visplane.h"
#include "video.h"
#include "render.h"

// -- Flats --
visplane_t *head = NULL;
visplane_t *unused = NULL;


void clearPlane(visplane_t *plane)
{
   plane->x2 = -1;
   plane->x1 = MAX_WIDTH;

   plane->next = plane->child = NULL;
}



void unlinkPlanes()
{
   visplane_t *rover, *next, *child, *cnext;

   for(rover = head; rover; rover = next)
   {
      next = rover->next;

      for(child = rover->child; child; child = cnext)
      {
         cnext = child->next;

         child->next = unused;
         unused = child;
      }

      rover->next = unused;
      unused = rover;
   }

   head = NULL;
}


visplane_t *findVisplane(float z, light_t *light)
{
   visplane_t *rover, *ret;

   for(rover = head; rover; rover = rover->next)
   {
      if(rover->z == z && !memcmp(light, &rover->light, sizeof(light_t)))
         return rover;
   }

   if(!unused)
      ret = (visplane_t *)malloc(sizeof(visplane_t));
   else
   {
      ret = unused;
      unused = unused->next;
   }

   clearPlane(ret);

   memcpy(&ret->light, light, sizeof(light_t));
   ret->z = z;

   ret->next = head;
   head = ret;

   return ret;
}


visplane_t *checkVisplane(visplane_t *check, int x1, int x2)
{
   visplane_t *child, *ret;
   int openleft, openright;

   if(check->x1 > check->x2)
   {
      check->x1 = openleft = x1;
      check->x2 = openright = x2;

      ret = check;
   }
   else if(x1 > check->x2)
   {
      openleft = check->x2 + 1;
      openright = x2;

      check->x2 = x2;
      ret = check;
   }
   else if(x2 < check->x1)
   {
      openleft = x1;
      openright = check->x1 - 1;

      check->x1 = x1;
      ret = check;
   }
   else
   {
      // No such luck, so check/create a child
      if(check->child)
         return checkVisplane(check->child, x1, x2);

      if(!unused)
         child = (visplane_t *)malloc(sizeof(visplane_t));
      else
      {
         child = unused;
         unused = unused->next;
      }

      clearPlane(child);

      memcpy(&child->light, &check->light, sizeof(light_t));
      child->z = check->z;
      child->child = check->child;
      check->child = child;
      
      child->x1 = openleft = x1;
      child->x2 = openright = x2; 
   
      ret = child;
   }

   for(;openleft <= openright; openleft++)
      ret->top[openleft] = 0xfffffffu;

   return ret;
}



extern float camx, camy, camz;
extern float viewsin, viewcos, xcenter, ycenter, yfov, xfov, fovratio;
extern int vieww, viewh;
extern vidSurface *screen, *texture;

renderspan_t span;

void renderSpan(int x1, int x2, int y, visplane_t *plane)
{
   float dy, xstep, ystep, realy, slope, height;

   // offset the height based on the camera
   height = plane->z - camz;
   if(!height) return; // avoid divide by 0

   // if you recall from the equasions with wall height to screen y projection
   //                       (height - camz) * yfov
   //  screeny = ycenter - ------------------------
   //                               y

   // solving this equasion for y you get
   //      (height - camz) * yfov
   // y = ----------------------------
   //         ycenter - screeny

   // .5 is added to make sure that when screeny == ycenter there is no divide by 0
   if(ycenter == y)
      dy = 0.01f;
   else if(y < ycenter)
      dy = fabsf(ycenter - y) - 1.0f;
   else
      dy = fabsf(ycenter - y) + 1.0f;

   slope = fabsf(height/dy);
   realy = slope * yfov;

   // steps are calculated using trig and the slope ratio
   xstep = viewcos * slope * fovratio;
   ystep = -viewsin * slope * fovratio;

   render->calcLight(1.0f / realy, slope, &plane->light, LT_SPAN);

   // the texture coordinates are first calculated at the center of the screen and
   // then offsetted using the step values to x1.
   // these values are then converted to 16.16 fixed point for a major speed increase
   span.xfrac = (int)((camx + (viewsin * realy) + ((x1 - xcenter) * xstep)) * 65536.0f);
   span.yfrac = (int)((camy + (viewcos * realy) + ((x1 - xcenter) * ystep)) * 65536.0f);
   span.xstep = (int)(xstep * 65536.0f);
   span.ystep = (int)(ystep * 65536.0f);
   span.x1 = x1;
   span.x2 = x2;
   span.y = y;
   span.tex = texture->getBuffer();
   span.screen = screen->getBuffer();


   render->drawSpan();
}



int spanstart[MAX_HEIGHT];



void renderVisplane(visplane_t *plane)
{
   int x, stop, t1, t2, b1, b2;

   x = plane->x1;
   stop = plane->x2 + 1;

   plane->top[x - 1] = plane->top[stop] = 0xfffffffu;
   plane->bot[x - 1] = plane->bot[stop] = 0;

   for(;x <= stop; x++)
   {
      t1 = plane->top[x - 1];
      t2 = plane->top[x];
      b1 = plane->bot[x - 1];
      b2 = plane->bot[x];

      for(; t2 > t1 && t1 <= b1; t1++)
         renderSpan(spanstart[t1], x - 1, t1, plane);
      for(; b2 < b1 && t1 <= b1; b1--)
         renderSpan(spanstart[b1], x - 1, b1, plane);
      while(t2 < t1 && t2 <= b2)
         spanstart[t2++] = x;
      while(b2 > b1 && t2 <= b2)
         spanstart[b2--] = x;
   }

   if(plane->child)
      renderVisplane(plane->child);
}


void renderVisplanes()
{
   visplane_t *rover;

   for(rover = head; rover; rover = rover->next)
      renderVisplane(rover);
}
