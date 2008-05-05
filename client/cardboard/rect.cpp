#include <SDL.h>
#include "rect.h"

//
// Begin::vidRect -------------------------------------------------------------------------
//
vidRect::vidRect()
{
   rect_x = rect_y = 0;
   rect_w = rect_h = 0;
   adjustScreen();
}


vidRect::vidRect(int x, int y, unsigned int w, unsigned int h)
{
   setRect(x, y, w, h);
}



vidRect::vidRect(const SDL_Rect &rect)
{
   rect_x = rect.x;
   rect_y = rect.y;
   rect_w = rect.w;
   rect_h = rect.h;
   adjustScreen();
}
   

// set functions ------------------------------------------------------------------------
void vidRect::setScreen(int x1, int y1, int x2, int y2)
{
   rect_x = x1 <= x2 ? x1 : x2;
   rect_y = y1 <= y2 ? y1 : y2;
   rect_x2 = x1 > x2 ? x1 : x2;
   rect_y2 = y1 > y2 ? y1 : y2;
   adjustRect();
}

void vidRect::setRect(int x, int y, unsigned int w, unsigned int h)
{
   rect_x = x;
   rect_y = y;
   rect_w = w;
   rect_h = h;
   adjustScreen();
}


void vidRect::setSDLRect(const SDL_Rect &r)
{
   rect_x = r.x;
   rect_y = r.y;
   rect_w = r.w;
   rect_h = r.h;
   adjustScreen();
}


// get functions ------------------------------------------------------------------------
SDL_Rect vidRect::getSDLRect()
{
   SDL_Rect ret;
   ret.x = rect_x;
   ret.y = rect_y;
   ret.w = rect_w;
   ret.h = rect_h;

   return ret;
}


// interaction with other rects ---------------------------------------------------------
bool vidRect::testIntersection(const vidRect &with)
{
   if(rect_x > with.rect_x2 || rect_x2 < with.rect_x ||
      rect_y > with.rect_y2 || rect_y2 < with.rect_y)
     return false;

   return true;
}


void vidRect::expandToFit(const vidRect &with)
{
   if(rect_x < with.rect_x)
      rect_x = with.rect_x;
   if(rect_x2 > with.rect_x2)
      rect_x2 = with.rect_x2;
   if(rect_y < with.rect_y)
      rect_y = with.rect_y;
   if(rect_y2 < with.rect_y2)
      rect_y2 = with.rect_y2;

   adjustRect();
}


void vidRect::clipRect(const vidRect &clipto)
{
   if(rect_x < clipto.rect_x)
      rect_x = clipto.rect_x;
   if(rect_x2 > clipto.rect_x2)
      rect_x2 = clipto.rect_x2;
   if(rect_y < clipto.rect_y)
      rect_y = clipto.rect_y;
   if(rect_y2 > clipto.rect_y2)
      rect_y2 = clipto.rect_y2;

   adjustRect();
}



vidRect vidRect::getCommon(const vidRect &r1, const vidRect &r2)
{
   int x, y, x2, y2;

   x =  r1.rect_x > r2.rect_x ? r1.rect_x : r2.rect_x;
   y =  r1.rect_y > r2.rect_y ? r1.rect_y : r2.rect_y;
   x2 =  r1.rect_x2 < r2.rect_x2 ? r1.rect_x2 : r2.rect_x2;
   y2 =  r1.rect_y2 < r2.rect_y2 ? r1.rect_y2 : r2.rect_y2;

   return vidRect(x, y, x2 - x + 1, y2 - y + 1);
}



// operators ----------------------------------------------------------------------------
bool vidRect::operator== (const vidRect &r2)
{
   return rect_x != r2.rect_x || rect_y != r2.rect_y || rect_w != r2.rect_w || 
          rect_h != r2.rect_h ? false : true;
}



// Protected helper functions -----------------------------------------------------------
void vidRect::adjustRect()
{
   rect_w = rect_x2 - rect_x + 1;
   rect_h = rect_y2 - rect_y + 1;
}


void vidRect::adjustScreen()
{
   rect_x2 = rect_x + rect_w - 1;
   rect_y2 = rect_y + rect_h - 1;
}
//
// End::vidRect ---------------------------------------------------------------------------
//
