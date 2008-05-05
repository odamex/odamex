// ----- Renderer -----
#include <SDL.h>
#include "video.h"
#include "matrix.h"
#include "error.h"
#include "vectors.h"
#include "render.h"
#include "pixelmath.h"
#include "mapdata.h"
#include "visplane.h"


// -- Camera --
// cameramat is a matrix which is used to perform rotation operations on vertices
matrix2d cameramat;

// camera angle and coords
float    cameraangle = 0;
float    camx = 0;
float    camy = 0;
float    camz = -23;

// View information set up at the beginning of each frame.
float    xcenter, ycenter, xfov, yfov, fovratio;
float    viewsin, viewcos;
int      vieww, viewh;


// -- Rendering -- 
// set of functions for bit-specific rendering operations
renderfunc_t *render;

extern frameid_t    frameid;

// Screen buffer pointer
extern   vidSurface *screen;
vidSurface *texture = NULL;



// -- Clipping -- 
// Much like in doom. The screen clipping array starts out open and closes up as walls
// are rendered.
float    cliptop[MAX_WIDTH], clipbot[MAX_WIDTH];



void loadTextures()
{
   texture = vidSurface::loadBMPFile("texture8.bmp");
   if(!texture)
      exit(0);

   texture->convertFormat(*screen);
}


void setupFrame()
{
   cameramat.setIdentity();
   cameramat.rotateAngle(cameraangle);
   xcenter = screen->getWidth() / 2.0f;
   ycenter = screen->getHeight() / 2.0f;

   // keep the camera angle sane so the special cases for the trig functions can be easily
   // caught.
   while(cameraangle > 2*pi)
      cameraangle -= 2*pi;
   while(cameraangle < 0)
      cameraangle += 2*pi;

   // cosf spits out garbage at these angles
   if(cameraangle == pi / 2 || cameraangle == 3 * pi/2)
      viewcos = 0;
   else
      viewcos = cosf(cameraangle);

   // viewsin does not seem to suffer from the same problem.
   viewsin = sinf(cameraangle);


   vieww = screen->getWidth();
   viewh = screen->getHeight();

   xfov = (float)vieww / 2;
   yfov = xfov * 1.2f;
   fovratio = yfov / xfov;

   // reset the clipping arrays
   for(int i = 0; i < MAX_WIDTH; i++)
   {
      cliptop[i] = 0.0f;
      clipbot[i] = viewh - 1;
   }

   switch(screen->getFormat().BytesPerPixel)
   {
      case 1:
         render = &render8;
         break;
      case 2:
         render = &render16;
         break;
      case 4:
         render = &render32;
         break;
   }


   unlinkPlanes();

   nextFrameID();
}



void moveCamera(float speed)
{
   camx += sinf(cameraangle) * speed;
   camy += cosf(cameraangle) * speed;
}



void rotateCamera(float deltaangle)
{
   cameraangle += deltaangle;
}


void flyCamera(float delta)
{
   camz += delta;
}




rendercolumn_t column;


struct wall_t
{
   int   x1, x2;

   float top, high;
   float topstep, highstep;

   float low, bottom;
   float lowstep, bottomstep;

   float dist, diststep, length;
   float len, lenstep;
   float xoffset, yoffset;
   float xscale, yscale;

   mapsector_t *sector;

   bool twosided, upper, middle, lower;


   bool markfloor, markceiling;
   visplane_t *floorp, *ceilingp;
}wall;



void renderWall()
{
   float basescale, yscale, xscale;
   int h, l, t, b;

   column.screen = (void *)screen->getBuffer();
   column.tex = (void *)texture->getBuffer();

   // fairly straight forward.
   for(int i = wall.x1; i <= wall.x2; i++)
   {
      if(cliptop[i] >= clipbot[i])
         goto skip;

      t = wall.top < cliptop[i] ? cliptop[i] : wall.top;
      b = wall.bottom > clipbot[i] ? clipbot[i] : wall.bottom;
      h = wall.high < cliptop[i] ? cliptop[i] : wall.high > clipbot[i] ? clipbot[i] : wall.high;
      l = wall.low < cliptop[i] ? cliptop[i] : wall.low > clipbot[i] ? clipbot[i] : wall.low;

      if(wall.markceiling && wall.ceilingp)
      {
         wall.ceilingp->top[i] = cliptop[i];
         wall.ceilingp->bot[i] = t - 1;
      }

      if(wall.markfloor && wall.floorp)
      {
         wall.floorp->top[i] = b + 1;
         wall.floorp->bot[i] = clipbot[i];
      }
      
      cliptop[i] = h;
      clipbot[i] = l;

      if(wall.upper || wall.lower || wall.middle)
      {
         // the actual scale is y / yfov however, you have to divide 1 by dist (1/y)
         // get y anyway so the resulting equasion is
         //      1
         // ------------
         //   yfov * y
         basescale = yscale = xscale = 1.0f / (wall.dist * yfov);

         yscale *= wall.yscale;
         xscale *= wall.xscale;

         // Fixed numbers 16.16 format
         column.ystep = (int)(yscale * 65536.0);
         column.texx = ((int)((wall.len * xscale) + wall.xoffset) & 0x3f) * 64;
         column.x = i;

         render->calcLight(wall.dist, basescale, &wall.sector->light, LT_COLUMN);         

         if(wall.upper)
         {
            column.y1 = t;
            column.y2 = h;

            column.yfrac = (int)((((column.y1 - wall.top + 1) * yscale) + wall.yoffset) * 65536.0);
            render->drawColumn();
         }
         if(wall.middle)
         {
            column.y1 = h;
            column.y2 = l;

            column.yfrac = (int)((((column.y1 - wall.high + 1) * yscale) + wall.yoffset) * 65536.0);
            render->drawColumn();
         }
         if(wall.lower)
         {
            column.y1 = l;
            column.y2 = b;

            column.yfrac = (int)((((column.y1 - wall.low + 1) * yscale) + wall.yoffset) * 65536.0);
            render->drawColumn();
         }
      }

      skip:

      wall.dist += wall.diststep;
      wall.len += wall.lenstep;
      wall.high += wall.highstep;
      wall.low += wall.lowstep;
      wall.top += wall.topstep;
      wall.bottom += wall.bottomstep;
   }
}




void projectWall(mapline_t *line)
{
   float x1, x2;
   float i1, i2;
   float istep;
   vector2f v1, v2, toffset;
   mapside_t *side, *backside;
   mapsector_t *sector, *backsector;

   vector2f  t1, t2;

   if(line->v1->frameid == frameid)
   {
      t1.x = line->v1->t_x;
      t1.y = line->v1->t_y;
   }
   else
   {
      v1.x = line->v1->x;
      v1.y = line->v1->y;

      // Translate the vertices
      t1 = v1 - vector2f(camx, camy);
      // rotate
      t1 = cameramat.execute(t1);
   }

   if(line->v2->frameid == frameid)
   {
      t2.x = line->v2->t_x;
      t2.y = line->v2->t_y;
   }
   else
   {
      v2.x = line->v2->x;
      v2.y = line->v2->y;

      t2 = v2 - vector2f(camx, camy);
      t2 = cameramat.execute(t2);
   }

   toffset.x = toffset.y = 0;

   // simple rejection for lines entirely behind the view plane
   if(t1.y < 1.0f && t2.y < 1.0f)
      return;

   // projection:
   //                vertex.x * xfov
   // x = xcenter + -----------------
   //                       y

   // clip to the front view plane
   if(t1.y < 1.0f)
   {
      float move;
      vector2f movevec;

      // the y value is behind the view plane so interpolate a new x and y value for v1
      // the first step is to determine exactly how much of the line needs to be clipped.
      move = (1.0f - t1.y) * ((t2.x - t1.x) / (t2.y - t1.y));

      // offset the vertex x based on length moved
      t1.x += move;

      // determine the texture offset needed based on the clip
      movevec.x = move;
      movevec.y = 1.0f - t1.y;
      toffset.x += movevec.getLength();

      // finally, y and 1/y both equal 1
      i1 = t1.y = 1.0f;
      x1 = xcenter + (t1.x * i1 * xfov);
   }
   else
   {
      if(line->v1->frameid == frameid)
      {
         i1 = line->v1->idistance;
         x1 = line->v1->proj_x;
      }
      else
      {
         line->v1->idistance = i1 = 1.0f / t1.y; // Calculate 1/y used for oh-so-much
         line->v1->proj_x = x1 = xcenter + (t1.x * i1 * xfov);

         line->v1->t_x = t1.x;
         line->v1->t_y = t1.y;
         line->v1->frameid = frameid;
      }
   }


   // determine if y2 is behind the plane
   if(t2.y < 1.0f)
   {
      // clipping y2 is much simpler because the interpolation is from x1
      // and can be stopped at any point
      t2.x += (1.0f - t2.y) * ((t2.x - t1.x) / (t2.y - t1.y));
      i2 = t2.y = 1.0f;
      x2 = xcenter + (t2.x * i2 * xfov) - 1;
   }
   else
   {
      if(line->v2->frameid == frameid)
      {
         i2 = line->v2->idistance;
         x2 = line->v2->proj_x - 1;
      }
      else
      {
         line->v2->idistance = i2 = 1.0f / t2.y;
         x2 = (line->v2->proj_x = xcenter + (t2.x * i2 * xfov)) - 1;
         line->v2->t_x = t2.x;
         line->v2->t_y = t2.y;
         line->v2->frameid = frameid;
      }
   }

   // back-face rejection
   if(x2 < x1)
   {
      if(!line->side[1])
         return;

      side = line->side[1];
      backside = line->side[0];

      sector = line->sector2;
      backsector = line->sector1;

      float temp = x2;
      x2 = x1;
      x1 = temp;

      temp = i2;
      i2 = i1;
      i1 = temp;
   }
   else
   {
      if(!line->side[0])
         return;

      side = line->side[0];
      backside = line->side[1];

      sector = line->sector1;
      backsector = line->sector2;
   }

   toffset.x *= side->xscale;

   toffset.x += side->xoffset;
   toffset.y += side->yoffset;

   // off-edge-of-screen rejection
   if(x2 < 0.0f || x1 > vieww)
      return;

   // determine the amount the 1/y changes each pixel to the right
   // 1/y changes linnearly across the line where as y itself does not.
   if(x2 >= x1)
      istep = 1.0f / (x2 - x1 + 1);
   else
      istep = 1.0f;

   
   wall.length = (t2 - t1).getLength();

   wall.dist = i1;
   wall.diststep = (i2 - i1) * istep;

   // Multiply these for the heights below
   i1 *= yfov; i2 *= yfov;

   // wall len is the length in linnear space increased with each pixel / y
   // the full equasion would be something like
   //             endx/y2 - startx/y1
   // lenstep = ----------------------
   //                 (x2 - x1)
   // but because startx is always 0 the equasion is simplified here
   wall.len = 0;
   wall.lenstep = wall.length * i2 * istep;

   // project the heights of the wall.
   //                       (height - camz) * yfov
   //  screeny = ycenter - ------------------------
   //                               y
   
   float top1, top2, high1, high2, low1, low2, bottom1, bottom2;

   if(!backsector)
   {
      if(camz < sector->ceilingz)
         wall.markceiling = true;
      else
         wall.markceiling = false;

      if(camz > sector->floorz)
         wall.markfloor = true;
      else
         wall.markfloor = false;

      high1 = top1 = ycenter - ((sector->ceilingz - camz) * i1);
      high2 = ycenter - ((sector->ceilingz - camz) * i2);

      low1 = bottom1 = ycenter - ((sector->floorz - camz) * i1) - 1;
      low2 = ycenter - ((sector->floorz - camz) * i2) - 1;

      // calculate the step values.
      wall.highstep = wall.topstep = (high2 - high1) * istep;
      wall.lowstep = wall.bottomstep = (low2 - low1) * istep;

      wall.upper = wall.lower = false;
      wall.middle = true;

      wall.twosided = false;
   }
   else
   {
      bool lightsame = memcmp(&sector->light, &backsector->light, sizeof(light_t)) ? false : true;

      {
         if(!lightsame && sector->floorz != backsector->floorz && camz > sector->floorz)
            wall.markfloor = true;
         else
            wall.markfloor = false;

         if(!lightsame && sector->ceilingz != backsector->ceilingz && camz < sector->ceilingz)
            wall.markceiling = true;
         else
            wall.markceiling = false;
      }

      if(sector->ceilingz > backsector->ceilingz)
      {
         top1 = ycenter - ((sector->ceilingz - camz) * i1);
         top2 = ycenter - ((sector->ceilingz - camz) * i2);
         wall.topstep = (top2 - top1) * istep;

         high1 = ycenter - ((backsector->ceilingz - camz) * i1);
         high2 = ycenter - ((backsector->ceilingz - camz) * i2);
         wall.highstep = (high2 - high1) * istep;

         wall.upper = true;
      }
      else
      {
         high1 = top1 = ycenter - ((sector->ceilingz - camz) * i1);
         high2 = ycenter - ((sector->ceilingz - camz) * i2);
         wall.highstep = wall.topstep = (high2 - high1) * istep;

         wall.upper = false;
      }

      if(sector->floorz < backsector->floorz)
      {
         low1 = ycenter - ((backsector->floorz - camz) * i1) - 1;
         low2 = ycenter - ((backsector->floorz - camz) * i2) - 1;
         wall.lowstep = (low2 - low1) * istep;

         bottom1 = ycenter - ((sector->floorz - camz) * i1) - 2;
         bottom2 = ycenter - ((sector->floorz - camz) * i2) - 2;
         wall.bottomstep = (bottom2 - bottom1) * istep;

         wall.lower = true;
      }
      else
      {
         low1 = bottom1 = ycenter - ((sector->floorz - camz) * i1) - 1;
         low2 = ycenter - ((sector->floorz - camz) * i2) - 1;
         wall.lowstep = wall.bottomstep = (low2 - low1) * istep;
         wall.lower = false;
      }

      wall.middle = false;

      wall.twosided = true;
   }


   // clip to screen
   if(x1 < 0.0f)
   {
      // when clipping x1 all the values that are increased per-pixel need to be increased
      // the amount x1 is jumping which would be newx1 - x1 but because newx1 is 0, the equasion
      // is simplified to -x1
      wall.len += wall.lenstep * -x1;
      wall.dist += wall.diststep * -x1;

      high1 += wall.highstep * -x1;
      low1 += wall.lowstep * -x1;
      top1 += wall.topstep * -x1;
      bottom1 += wall.bottomstep * -x1;

      x1 = 0.0f;
   }

   if(x2 >= vieww)
      x2 = vieww - 1;


   if(wall.markfloor)
      wall.floorp = checkVisplane(findVisplane(sector->floorz, &sector->light), x1, x2);
   else
      wall.floorp = NULL;

   if(wall.markceiling)
      wall.ceilingp = checkVisplane(findVisplane(sector->ceilingz, &sector->light), x1, x2);
   else
      wall.ceilingp = NULL;
 

   // Round the pixel values as a last step;
   wall.x1 = x1;
   wall.x2 = x2;
   wall.high = high1;
   wall.low = low1;
   wall.sector = sector;

   wall.top = top1;
   wall.bottom = bottom1;

   wall.xoffset = toffset.x;
   wall.yoffset = toffset.y;

   wall.xscale = side->xscale;
   wall.yscale = side->yscale;

   renderWall();
}






void renderEverything();

void renderScene()
{
   screen->lock();
   texture->lock();

   //screen->clear();

   setupFrame();
   renderEverything();
   renderVisplanes();

   screen->unlock();
   texture->unlock();

   screen->flipVideoPage();
}
