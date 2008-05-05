#ifndef VIDEO_H
#define VIDEO_H

#include "rect.h"
#include <string>
using namespace std;

// vidSurface ------------------------------------------------------------------------------
// Acts as a wrapper for SDLs surface functionality, plus some added functionality. When a
// vidSurface 'owns' a SDL_Surface, that means the object will free the surface when the object
// is destroyed. If a vidSurface object does not own a surface, it is up to the code that 
// created the SDL_Surface to destroy that surface.
class vidSurface
{
   public:
   vidSurface(unsigned int w, unsigned int h, Uint8 bits);
   vidSurface(SDL_Surface &from, bool owns);
   virtual ~vidSurface();


   // Format management --------------------------------------------------------------------
   // Returns the pixelformat of the surface.
   SDL_PixelFormat getFormat();
   // Converts the surface to the given pixel format
   void convertFormat(SDL_PixelFormat &f);
   // Converts the surface to the pixel format of the given surface.
   void convertFormat(vidSurface &surface);
   // Returns a copy of this surface.
   vidSurface *getCopy();
   // Copys a given area of the surface and returns the new object. The input rect will
   // first be clipped to the surfaces rect. Returns NULL on error.
   vidSurface *getCopy(vidRect area);


   // Resize -------------------------------------------------------------------------------
   // Resizes the surface. Any new area that's created (ie. taller or wider) will be on the 
   // bottom and right sides, and will be filled with 0 pixels. Any area that is removed
   // will be taken from the bottom and right sides.
   void resize(unsigned int neww, unsigned int newh);


   // Clipping -----------------------------------------------------------------------------
   // Sets the clipping area for the surface. The given rectangle will be clipped to the 
   // area of the surface and then the clipping rect inside the surface will be set. All
   // primitives and blitting operations are clipped to the surfaces clipping rect.
   void setClipRect(vidRect r);
   // Sets the surfaces clipping rectangle to the common area shared by the current clipping
   // rectangle and the given rectangle.
   void setClipInside(vidRect r);
   // Returns the current clipping rectangle.
   vidRect getClipRect();
   // Returns the surfaces rectangle.
   vidRect getRect() const {return vidRect(0, 0, width, height);}


   // Blitting -----------------------------------------------------------------------------
   // Blits the surface into the destination surface. The width and the height are taken from
   // the source rectangle.
   void blitTo(vidSurface &dest, vidRect srcrect, vidRect destrect);
   // Blits the surface into the destination surface.
   void blitTo(vidSurface &dest, int srcx, int srcy, unsigned int w, unsigned int h, 
               int destx, int desty);
   // Blits the surface into the destination surface. The image is taken from srcrect of the 
   // surface and scaled to fit into destrect on the destination surface.
   void stretchBlitTo(vidSurface &dest, vidRect srcrect, vidRect destrect);
   // Blits the surface onto the destination surface using additive translucency. The image is 
   // taken from srcrect of the surface and scaled to fit into destrect on the destination 
   // surface.
   void stretchBlitAddTo(vidSurface &dest, vidRect srcrect, vidRect destrect);
   // Blits the surface onto the destination surface using integer translucency. The image 
   // is taken from srcrect of the surface and scaled to fit into destrect on the destination 
   // surface. Amount should be between 0 (source is transparent) to 255 (source is opaque)
   void stretchBlitTransTo(vidSurface &dest, vidRect srcrect, vidRect destrect, unsigned char amount);
   // Blits the surface into the destination surface. The image is taken from srcrect of the 
   // surface and scaled and filtered to fit into destrect on the destination surface.
   void filterBlitTo(vidSurface &dest, vidRect srcrect, vidRect destrect);
   // Darkens a surface and blits it. For each color component, 255 is full bright, 0 is black
   void stretchBlitLitedTo(vidSurface &dest, vidRect srcrect, vidRect destrect, Uint8 rlight, Uint8 glight, Uint8 blight);


   // Flipping and rotation ----------------------------------------------------------------
   // Flips the surface horizontally.
   void flipHorizontal();
   // Flips the surface vertically.
   void flipVertical();
   // Rotates the surface 90 degress. This will change the dimentions of the surface.
   void rotate90();
   // Rotates the surface 180 degrees.
   void rotate180();
   // Rotates the surface 270 degress.
   void rotate270();
   // NOTE: All the functions in this section will reset the clipping area.


   // Primitives ---------------------------------------------------------------------------
   // Draws a single pixel to the surface. This method is slow for drawing many pixels.
   void drawPixel(int x, int y, Uint32 color);
   // Draws a line to the surface.
   void drawLine(int x1, int y1, int x2, int y2, Uint32 color);
   // Draws four lines to form an empty rectangle shape in the surface.
   void drawRect(int x, int y, unsigned int w, unsigned int h, Uint32 color);
   // Draws a solid rectangle in the surface.
   void fillRect(int x, int y, unsigned int w, unsigned int h, Uint32 color);
   // Performs floodfill based on the color currently occupying x,y.
   void floodFill(int x, int y, Uint32 color);
   // Resets all pixels in the image to 0
   void clear();
   // (NOTE: If the surface is not locked, the primitive functions will all lock the image
   // before drawing the primitives, and then unlock the image after the primitives are
   // drawn.)


   // Colors -------------------------------------------------------------------------------
   // Returns an unsigned 32bit representation of the given color based on the color
   // information passed to it and the pixel format of the surface. If the surface is 8-bit
   // the nearest palette entry index will be returned.
   Uint32 mapColor(Uint8 r, Uint8 g, Uint8 b, Uint8 a);
   // Expands a Uint32 color into r g b and a based on the pixel format of the surface. If
   // the image is 8-bit, color should be a palette index (0 to 255)
   void getColor(Uint32 color, Uint8 &r, Uint8 &g, Uint8 &b, Uint8 &a);


   // Video stuffs -------------------------------------------------------------------------
   // Locks the surface and makes the pixel buffer available. This MUST be called before
   // low-level pixel access is granted.
   void lock();
   // Unlocks the image. This should be done prior to blitting or updating the screen (if
   // the surface is the screen surface).
   void unlock();


   // attribute fetchers -------------------------------------------------------------------
   Uint8 *getBuffer(void) const {return buffer;}
   unsigned int getWidth(void) const {return s ? s->w : 0;}
   unsigned int getHeight(void) const {return s ? s->h : 0;}
   unsigned int getPitch(void) const {return s ? pitch : 0;}
   unsigned int getPixelSize(void) const {return s ? s->format->BytesPerPixel : 0;}


   // Image file functions -----------------------------------------------------------------
   // Saves the surface to a windows BMP file using the surfaces format. Returns true if the 
   // image was saved successfully, otherwise, returns false.
   bool saveBMPFile(string filename);
   // Loads the given BMP file and returns a vidSurface object containing the image. If 
   // the file could not be loaded, the function returns NULL.
   static vidSurface *loadBMPFile(string filename);


   // Screen stuffs ------------------------------------------------------------------------
   // Sets the video mode and returns a vidSurface object containing the screen surface.
   // If the function fails, NULL is returned. Use only this function to set the screen if you
   // plan on using any of the functions in this section. If this function is called more than
   // once, the previous screen surface is demoted to a regular surface and the current contents
   // of the screen are kept in it.
   static vidSurface *setVideoMode(unsigned int w, unsigned int h, int bits, int sdlflags);
   // Resizes the video. If the resize failed, a gFatalError is thrown. Returns a pointer
   // to the screen surface.
   static vidSurface *resizeScreen(unsigned int w, unsigned int h);
   // Calls SDL_Flip with the current screen surface.
   static void flipVideoPage(void);
   // Updates a portion of the screen surface.
   static void updateRect(vidRect &r);

   protected:
   // Protected helper functions -----------------------------------------------------------
   void noLongerScreen();
   void drawHLine(int x1, int x2, int y, Uint32 color);
   void drawVLine(int x, int y1, int y2, Uint32 color);
   void drawPositiveLine(int x1, int y1, int x2, int y2, Uint32 color);
   void drawNegativeLine(int x1, int y1, int x2, int y2, Uint32 color);

   void setSurface(SDL_Surface &surface, bool owner);

   // Protected member variables -----------------------------------------------------------
   SDL_Surface *s;
   bool        freesurface, mustlock, abnormalpitch, isscreen;
   Uint8       *buffer, bitdepth;
   int         locks, pitch;
   int         width, height;
   vidRect       cliprect;

   static vidSurface  *screensurface;
};




#endif
