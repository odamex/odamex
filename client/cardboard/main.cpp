//
// Cardboard - an experiment in doom-style projection and texture mapping
//
#include <SDL.h>
#include "video.h"
#include "vectors.h"

vidSurface *screen;


void moveCamera(float speed);
void rotateCamera(float deltaangle);
void flyCamera(float delta);
void renderScene();
void loadTextures();


void hackMapData();


int main (int argc, char **argv)
{
   // Initialize the SDL Library.
   int result = SDL_Init(SDL_INIT_TIMER|SDL_INIT_VIDEO);
   float fps[30], total;
   int   index = -1, i;
   
   if(result == -1)
      return -1;
   
   screen = vidSurface::setVideoMode(800, 600, 32, 0);
   
   SDL_Event e;
   bool update = true, up = false, down = false;
   bool left = false, right = false, pgup = false, pgdn = false;

   hackMapData();
   loadTextures();

   moveCamera(-384);
   //rotateCamera(3.14f);

   while(1)
   {
      if(SDL_PollEvent(&e))
      {
         if(e.type == SDL_ACTIVEEVENT && e.active.gain == 1)
            update = true;

         if(e.type == SDL_KEYDOWN)
         {
            switch(e.key.keysym.sym)
            {
               case SDLK_UP:
                  up = true;
                  break;
               case SDLK_DOWN:
                  down = true;
                  break;
               case SDLK_LEFT:
                  left = true;
                  break;
               case SDLK_RIGHT:
                  right = true;
                  break;
               case SDLK_PAGEUP:
                  pgup = true;
                  break;
               case SDLK_PAGEDOWN:
                  pgdn = true;
                  break;

               case SDLK_ESCAPE:
                  return 0;
                  break;
            }
         }
         if(e.type == SDL_KEYUP)
         {
            switch(e.key.keysym.sym)
            {
               case SDLK_UP:
                  up = false;
                  break;
               case SDLK_DOWN:
                  down = false;
                  break;
               case SDLK_LEFT:
                  left = false;
                  break;
               case SDLK_RIGHT:
                  right = false;
                  break;
               case SDLK_PAGEUP:
                  pgup = false;
                  break;
               case SDLK_PAGEDOWN:
                  pgdn = false;
                  break;
            }
         }

         if(e.type == SDL_QUIT)
            return 0;
      }

      if(up)
      {
         moveCamera(1.5f);
         update = true;
      }
      if(down)
      {
         moveCamera(-1.5f);
         update = true;
      }
      if(left)
      {
         rotateCamera(-.02f);
         update = true;
      }
      if(right)
      {
         rotateCamera(.02f);
         update = true;
      }
      if(pgup)
      {
         flyCamera(1.5f);
         update = true;
      }
      if(pgdn)
      {
         flyCamera(-1.5f);
         update = true;
      }


      Uint32 ticks = SDL_GetTicks();
      renderScene();
      ticks = SDL_GetTicks() - ticks;
      if(index == -1)
      {
         for(index = 0; index < 30; index++)
            fps[index] = 1000.0f / (float)(ticks);
         index = 0;
      }
      else
      {
         fps[index ++] = 1000.0f / (float)(ticks);
         index %= 30;
      }

      total = 0;
      for(i = 0; i < 30; i++)
         total += fps[i];

      char title[256];

      if((int)(total / 30.0f) == 0)
         sprintf(title, "cardboard 0.0.1 by Stephen McGranahan (1000 < fps)");
      else
         sprintf(title, "cardboard 0.0.1 by Stephen McGranahan (%i fps)", (int)(total / 30.0f));

      SDL_WM_SetCaption(title, NULL);

      SDL_Delay(1);
   }

   return 0;
}
