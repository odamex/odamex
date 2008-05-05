#ifndef RECT_H
#define RECT_H

// vidRect -----------------------------------------------------------------------------------
// Represents a rectangular section of 2d space. Automatically calculates and translates 
// between screen space coords (x1, y1, x2, y2) and rectangle space (x, y, width, height)
class vidRect
{
   public:
   vidRect();
   vidRect(int x, int y, unsigned int w, unsigned int h);
   vidRect(const SDL_Rect &rect);
   

   // set functions ------------------------------------------------------------------------
   // Sets the coordinates of the rectangle from a variety of spaces and input types.
   void setScreen(int x1, int y1, int x2, int y2);
   void setRect(int x, int y, unsigned int w, unsigned int h);
   void setSDLRect(const SDL_Rect &r);


   // get functions ------------------------------------------------------------------------
   // getCommon returns a vidRect value containing the common space between the two given 
   // vidRect objects.
   static vidRect getCommon(const vidRect &r1, const vidRect &r2);
   // getSDLRect returns and SDL_Rect value with the coords contained in the
   // vidRect object it is called from.
   SDL_Rect getSDLRect();


   // interaction with other rects ---------------------------------------------------------
   // testIntersection returns true if this vidRect intersects (overlaps) the given rect.
   bool testIntersection(const vidRect &with);
   // expandToFit expands this rect to completely enclose the given rectangle.
   void expandToFit(const vidRect &with);
   // clipRect closes this rect to only the common area with the given rect.
   void clipRect(const vidRect &clipto);
   

   // individual attribute fetchers --------------------------------------------------------
   // Rectangle space coords
   int rx() const {return rect_x;}
   int ry() const {return rect_y;}
   unsigned int rw() const {return rect_w;}
   unsigned int rh() const {return rect_h;}

   // Screen space coords
   int sx1() const {return rect_x;}
   int sy1() const {return rect_y;}
   int sx2() const {return rect_x2;}
   int sy2() const {return rect_y2;}

   // Opengl screen space coords.
   int glx1() const {return rect_x;}
   int gly1() const {return rect_y;}
   int glx2() const {return rect_x2 + 1;}
   int gly2() const {return rect_y2 + 1;}

   // operators ----------------------------------------------------------------------------
   bool operator == (const vidRect &);

   protected:
   // protected helper functions -----------------------------------------------------------
   void adjustScreen();
   void adjustRect();

   // actual coordinates. ------------------------------------------------------------------
   int rect_x, rect_y, rect_x2, rect_y2;
   unsigned int rect_w, rect_h;
};


#endif
