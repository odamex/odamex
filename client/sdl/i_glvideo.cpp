//-----------------------------------------------------------------------------
//
// Copyright (C) 1993-1996 by id Software, Inc.
//
// This source is available for distribution and/or modification
// only under the terms of the DOOM Source Code License as
// published by id Software. All rights reserved.
//
// The source is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// FITNESS FOR A PARTICULAR PURPOSE. See the DOOM Source Code License
// for more details.
//
// DESCRIPTION:
//	OpenGL implementation of the IVideo class.
//
//-----------------------------------------------------------------------------

#include <SDL_opengl.h>

#include "v_palette.h"
#include "i_glvideo.h"
#include "i_system.h"
#include "w_wad.h"
#include "z_zone.h"

class glpatch_t
{
public:
	GLuint texture;
	float w, h; // 0.0 to 1.0
};
glpatch_t &get_glpatch(size_t patchnum, bool paletteTex = true);

GLVideo::GLVideo(int parm)
{
	if (SDL_InitSubSystem (SDL_INIT_VIDEO) == -1)
	{
		I_FatalError("Could not initialize SDL video.\n");
		return;
	}

	// [Russell] - A basic version string that will eventually get replaced
	//             better than "Odamex SDL Alpha Build 001" or something :P
	std::string title = "Odamex - v";
	title += DOTVERSIONSTR;

	// [Russell] - Update window caption with name
	SDL_WM_SetCaption (title.c_str(), title.c_str());

   sdlScreen = NULL;
   infullscreen = false;
   screenw = screenh = screenbits = 0;
   palettechanged = false;

   chainHead = new cChain(NULL);

   // Get Video modes
   SDL_PixelFormat fmt;
   fmt.palette = NULL;
   fmt.BitsPerPixel = 8;
   fmt.BytesPerPixel = 1;

   SDL_Rect **sdllist = SDL_ListModes(NULL, SDL_FULLSCREEN|SDL_HWSURFACE);

   vidModeList = NULL;
   vidModeCount = 0;

   if(!sdllist)
   {
	  // no fullscreen modes, but we could still try windowed
	  Printf(PRINT_HIGH, "SDL_ListModes returned NULL. No fullscreen video modes are available.\n");
      vidModeList = NULL;
	  vidModeCount = 0;
	  return;
   }
   else if(sdllist == (SDL_Rect **)-1)
   {
      I_FatalError("SDL_ListModes returned -1. Internal error.\n");
      return;
   }
   else
   {
      int i;
      for(i = 0; sdllist[i]; i++)
         ;

      vidModeList = new vidMode[i];
      vidModeCount = i;

      for(i = 0; sdllist[i]; i++)
      {
         vidModeList[i].width = sdllist[i]->w;
         vidModeList[i].height = sdllist[i]->h;
         vidModeList[i].bits = 8;
      }
   }
}

GLVideo::~GLVideo(void)
{
   cChain *rover, *next;

   for(rover = chainHead->next; rover != chainHead; rover = next)
   {
      next = rover->next;
      ReleaseSurface(rover->canvas);
      delete rover;
   }

   delete chainHead;

   if(vidModeList)
   {
      delete [] vidModeList;
      vidModeList = NULL;
   }
}


bool GLVideo::FullscreenChanged (bool fs)
{
   if(fs != infullscreen)
   {
      fs = infullscreen;
      return true;
   }

   return false;
}


void GLVideo::SetWindowedScale (float scale)
{
   /// HAHA FIXME
}

void GLVideo::GL_Test()
{
	static bool once = false;

	if(!once)
	{
		once=true;

		// Niceness
		glClearColor(0.0f,0.0f,0.0f,0.0f);
		glClearDepth(1.0f);
		glDepthFunc(GL_LEQUAL);
		glEnable(GL_DEPTH_TEST);
		glEnable(GL_STENCIL_TEST);
		glShadeModel(GL_SMOOTH);
		glDisable(GL_CULL_FACE);
		glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
	}

	Printf(PRINT_HIGH, "GL_VERSION: %s\n", glGetString(GL_VERSION));
	Printf(PRINT_HIGH, "GL_RENDERER: %s\n", glGetString(GL_RENDERER));
//	Printf(PRINT_HIGH, "GL_EXTENSIONS: %s\n", glGetString(GL_EXTENSIONS));

	extern void EnableShader();
	EnableShader();
}

EXTERN_CVAR(opengl)

void PurgeTextures();

bool GLVideo::SetMode (int width, int height, int bits, bool fs)
{
   Uint32 flags = 0;

   // SoM: I'm not sure if we should request a software or hardware surface yet... So I'm
   // just ganna let SDL decide.

	if(opengl)
	{
		flags |= SDL_OPENGL;
		bits = 8;

		SDL_GL_SetAttribute( SDL_GL_STENCIL_SIZE, 1 );
	}

   if(fs && vidModeCount)
   {
      flags |= SDL_FULLSCREEN;

      if(bits == 8)
         flags |= SDL_HWPALETTE;
   }

   if(!(sdlScreen = SDL_SetVideoMode(width, height, bits, flags)))
      return false;

   screenw = width;
   screenh = height;
   screenbits = bits;

   PurgeTextures();
   GL_Test();

   return true;
}

void GLVideo::SetPalette (DWORD *palette)
{
   for(size_t i = 0; i < sizeof(newPalette)/sizeof(SDL_Color); i++)
   {
      newPalette[i].r = RPART(palette[i]);
      newPalette[i].g = GPART(palette[i]);
      newPalette[i].b = BPART(palette[i]);
      newPalette[i].unused = 0xFF;
   }

   palettechanged = true;
}

void GLVideo::SetOldPalette (byte *doompalette)
{
    for(size_t i = 0; i < sizeof(newPalette)/sizeof(SDL_Color); i++)
    {
      newPalette[i].r = newgamma[*doompalette++];
      newPalette[i].g = newgamma[*doompalette++];
      newPalette[i].b = newgamma[*doompalette++];
      newPalette[i].unused = 0xFF;
    }
   palettechanged = true;
}

void RenderWorld();

void DrawQuad(float x, float y, float w, float h)
{
	glBegin(GL_QUADS);
	glTexCoord2f(0, 1);
	glVertex2f(x, y);
	glTexCoord2f(1, 1);
	glVertex2f(x, y+h);
	glTexCoord2f(1, 0);
	glVertex2f(x+w, y+h);
	glTexCoord2f(0, 0);
	glVertex2f(x+w, y);
	glEnd();
}

//void DrawTexturedQuad(float x, float y, float w, float h, float tw, float th, float tx = 0.0f, float ty = 0.0f);

void DrawTexturedQuad(float x, float y, float w, float h, float tw, float th, float tx = 0.0f, float ty = 0.0f)
{
	glBegin(GL_QUADS);
	glTexCoord2f(tx, ty);
	glVertex2f(x, y);
	glTexCoord2f(tx+tw, ty);
	glVertex2f(x, y+h);
	glTexCoord2f(tx+tw, ty+th);
	glVertex2f(x+w, y+h);
	glTexCoord2f(tx, ty+th);
	glVertex2f(x+w, y);
	glEnd();
}

void DrawTint()
{
	// Draw player redscreen, berserk, pickup
	glDisable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
	glDisable(GL_ALPHA_TEST);

	extern int st_palette;

	glColor4f(0, 0, 0, 0);

	if(st_palette >= STARTREDPALS && st_palette < STARTREDPALS+NUMREDPALS)
		glColor4f(1, 0, 0, (st_palette-STARTREDPALS)/(float)NUMREDPALS);
	else if(st_palette >= STARTBONUSPALS && st_palette < STARTBONUSPALS+NUMBONUSPALS)
		glColor4f(0.5, 0.5, 0, 0.5f*(st_palette-STARTBONUSPALS)/(float)NUMBONUSPALS);
	else if(st_palette == RADIATIONPAL)
		glColor4f(0, 1, 0, 0.5f);

	DrawQuad(0, 0, 1, 1);

	glColor4f(1, 1, 1, 1);
	glEnable(GL_ALPHA_TEST);
	glEnable(GL_TEXTURE_2D);
}

void DimArea(float x, float y, float w, float h, float alpha)
{
	// Menu/console/scoreboard backgrounds
	glDisable(GL_TEXTURE_2D);
	glDisable(GL_ALPHA_TEST);

	// Blending to darken the area
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_SRC_ALPHA);

	glColor4f(0, 0, 0, alpha);
	DrawQuad(x, y, w, h);

	glColor4f(1, 1, 1, 1);
	glEnable(GL_ALPHA_TEST);
	glEnable(GL_TEXTURE_2D);
}

#include "m_menu.h"
#define LINEHEIGHT 16
#define SKULLXOFF  -32
void DrawMenu()
{
	if(!menuactive)
		return;

	// Darken behind menu
	DimArea(0, 0, 1, 1, 0.5);


	//extern void M_Drawer();
	//M_Drawer();
	// DRAW MENU
	int x = currentMenu->x;
	int y = currentMenu->y;
	int max = currentMenu->numitems;

	glEnable(GL_TEXTURE_2D);
	glDisable(GL_BLEND);

	for (int i = 0; i < max; i++)
	{
		if (currentMenu->menuitems[i].name[0])
		{
			//screen->DrawPatchClean (W_GetNumForName(currentMenu->menuitems[i].name), x, y);
			int lump = W_GetNumForName(currentMenu->menuitems[i].name);
			patch_t *patch = (patch_t *)W_CacheLumpNum (lump, PU_CACHE);
			glpatch_t &glp = get_glpatch(lump);

			glBindTexture(GL_TEXTURE_2D, glp.texture);
			float xscale = 1.0f/320.0f;
			float yscale = 1.0f/240.0f;
			DrawTexturedQuad(x*xscale, y*yscale, patch->width()*xscale, patch->height()*yscale, 1, 1);
		}
		y += LINEHEIGHT;
	}


	// DRAW SKULL
	//if (drawSkull)
	{
		extern short whichSkull;
		extern char skullName[2][9];
		//screen->DrawPatchClean (W_GetNumForName(skullName[whichSkull]),
		//		x + SKULLXOFF, currentMenu->y - 5 + itemOn*LINEHEIGHT);
		int lump = W_GetNumForName(skullName[whichSkull]);
		patch_t *patch = (patch_t *)W_CacheLumpNum (lump, PU_CACHE);
		glpatch_t &glp = get_glpatch(lump);

		glBindTexture(GL_TEXTURE_2D, glp.texture);
		float xscale = 1.0f/320.0f;
		float yscale = 1.0f/240.0f;
		x += SKULLXOFF;
		y = currentMenu->y - 5 + itemOn*LINEHEIGHT;
		DrawTexturedQuad(x*xscale, y*yscale, patch->width()*xscale, patch->height()*yscale, 1, 1);
	}
}

void DrawScreenSprites();
void DrawConsoleText(size_t screenw, size_t screenh);
void DrawConsoleBackground();

void DrawConsole(size_t screenw, size_t screenh)
{
	extern int ConBottom;

	// Draw notify messages

	if(gamestate == GS_FULLCONSOLE ||
	   gamestate == GS_STARTUP ||
	   gamestate == GS_DEMOSCREEN)
	{
		// Draw full console background
		DrawConsoleBackground();

		// Draw console text
		DrawConsoleText(screenw, screenh);
	}
	if(ConsoleState != c_up)
	{
		// Darken behind partial console
		DimArea(0, 0, 1, ((float)ConBottom/screenh), 0.5);

		// Draw console text
		DrawConsoleText(screenw, screenh);
	}
	else
	{
		// Draw text input
	}
}

#include "vectors.h"

void RenderAutomap()
{
	size_t i;

	glDisable(GL_TEXTURE_2D);

	glColor4f(0.2,0.2,0,1);
	DrawQuad(0,0,1,1);

	glColor4f(1,1,1,1);

	glPushMatrix();

	glTranslatef(0.5, 0.5, 0);

	glScalef(-0.001, 0.001, 0.001);

	AActor *mo = consoleplayer().mo;
	if(mo)
		glTranslatef(FIXED2FLOAT(mo->x), FIXED2FLOAT(mo->y), 0);
	glRotatef(180, 0, 0, 1);

	glBegin(GL_LINES);

	// grid

	// walls
	for(i = 0; i < numlines; i++)
	{
		if (!(lines[i].flags & ML_MAPPED))
			continue;

		glVertex2f(FIXED2FLOAT(lines[i].v1->x), FIXED2FLOAT(lines[i].v1->y));
		glVertex2f(FIXED2FLOAT(lines[i].v2->x), FIXED2FLOAT(lines[i].v2->y));
	}

	// players
	for (i = 0; i < players.size(); i++)
	{
	}

	// things

	glEnd();

	glPopMatrix();
}

void DrawCrosshair();

void GLVideo::UpdateScreen (DCanvas *canvas)
{
   if(palettechanged)
   {
	   extern void UpdateShaderPalette(SDL_Color *colors);
	   UpdateShaderPalette(newPalette);
	   palettechanged = false;
   }

	SDL_GL_SwapBuffers();
	// viewheight or viewwidth not the same as screen?
	if(screenw != realviewheight || screenh != realviewheight)
		glViewport(viewwindowx, screenh - (realviewheight + viewwindowy), realviewwidth, realviewheight);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

	// make textures transparent where they are black?
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_ALPHA_TEST);
    glAlphaFunc(GL_GREATER, 0);

	// Switch to 3D
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	float aspect = (float)realviewwidth/realviewheight;
	float fov = 70.0;

	gluPerspective(fov, aspect, 0.08f, 100.0f);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	// Render doom level
	if(gamestate == GS_LEVEL && !automapactive)
		RenderWorld();

	// Switch to 2D
	glDisable(GL_DEPTH_TEST);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	glOrtho(0.0f, 1, 1, 0.0f, -1.0f, 1.0f);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	if(gamestate == GS_LEVEL && !automapactive)
	{
		// Draw player weapon
		DrawScreenSprites();

		// Draw crosshair
		DrawCrosshair();
	}

	// Render menu/console/automap over whole screen
	if(screenw != realviewwidth || screenh != realviewheight)
	{
		glViewport(0, 0, screenw, screenh);
	}

	// Draw automap
	if(automapactive)
		RenderAutomap();

	// Render view border
	extern void R_DrawViewBorder();
	R_DrawViewBorder();

	// Draw status bar
	if(gamestate == GS_LEVEL)
	if(screenw != realviewwidth || screenh != realviewheight)
	{
		glColor4f(0,1,0,1);
		glDisable(GL_TEXTURE_2D);
		extern int ST_X, ST_Y, ST_WIDTH, ST_HEIGHT;
		DrawQuad((float)ST_X/screenw, (float)ST_Y/screenh, (float)ST_WIDTH/screenw, (float)ST_HEIGHT/screenh);
	}

	// Draw status bar border

	// Draw player redscreen, berserk, pickup
//	DrawTint();

	// Render the grayscale+colormap result to a texture
	extern void RenderToTexture(size_t screenw, size_t screenh);
	RenderToTexture(screenw, screenh);

	// Shader will convert this into RGB output
	extern void RenderedTextureToScreen(size_t screenw, size_t screenh);
	RenderedTextureToScreen(screenw, screenh);

	// Console
	DrawConsole(screenw, screenh);

	// Menu
	DrawMenu();
}

void GLVideo::ReadScreen (byte *block)
{
   // SoM: forget lastCanvas, let's just read from the screen, y0
   if(!sdlScreen)
      return;

   glFlush();

   glReadPixels(0, 0, screenw, screenh, GL_RGBA, GL_UNSIGNED_BYTE, block);

   unsigned int pitch = screenw*4;

   // Set Alpha to max
   for(size_t i = 3; i < screenh*pitch; i+=4)
	   block[i] = 255;

   int *tmp = new int[pitch];

   // Flip
   for(size_t j = 0; j < screenh/2; j++)
   {
	   byte *t = block + j*pitch;
	   byte *b = block + (screenh - j - 1)*pitch;

	   memcpy(tmp, t, pitch);
	   memcpy(t, b, pitch);
	   memcpy(b, tmp, pitch);
   }

   delete[] tmp;
}


int GLVideo::GetModeCount ()
{
   return vidModeCount;
}


void GLVideo::StartModeIterator (int bits)
{
   vidModeIterator = 0;
   vidModeIteratorBits = bits;
}


bool GLVideo::NextMode (int *width, int *height)
{
   while(vidModeIterator < vidModeCount)
   {
      if(vidModeList[vidModeIterator].bits == vidModeIteratorBits)
      {
         *width = vidModeList[vidModeIterator].width;
         *height = vidModeList[vidModeIterator].height;
         vidModeIterator++;
         return true;
      }

      vidModeIterator++;
   }
   return false;
}

class DGLCanvas : public DCanvas
{
	// override with opengl capability, queue operations until bit happens
	public:

	void DrawPatch (const patch_t *patch, int x, int y){}
	void DrawPatch (int lump, int x, int y)
	{
		if(!m_Private)
			return;

		patch_t *patch = (patch_t *)W_CacheLumpNum (lump, PU_CACHE);
		glpatch_t &glp = get_glpatch(lump);
		glBindTexture(GL_TEXTURE_2D, glp.texture);
		float xscale = 1.0f/width;
		float yscale = 1.0f/height;
		DrawTexturedQuad(x*xscale, y*yscale, (float)patch->width()*xscale, (float)patch->height()*yscale, 1, 1);
	}

	void DrawPatchClean (const patch_t *patch, int x, int y){}
	void DrawPatchClean (int lump, int x, int y)
	{
		if(!m_Private)
			return;

		patch_t *patch = (patch_t *)W_CacheLumpNum (lump, PU_CACHE);
		glpatch_t &glp = get_glpatch(lump);
		glBindTexture(GL_TEXTURE_2D, glp.texture);
		float xscale = 1.0f/320.0f;
		float yscale = 1.0f/240.0f;
		DrawTexturedQuad(x*xscale, y*yscale, patch->width()*xscale, patch->height()*yscale, 1, 1);
	}
};

DCanvas *GLVideo::AllocateSurface (int width, int height, int bits, bool primary)
{
	DCanvas *scrn = new DGLCanvas;

	scrn->width = width;
	scrn->height = height;
	//scrn->is8bit = (bits == 8 ? true : false);
	scrn->bits = screenbits;
	scrn->m_LockCount = 0;
	scrn->m_Palette = NULL;
	scrn->buffer = NULL;
	scrn->pitch = width * (bits / 8);
	scrn->buffer = new byte[width*height*4];

	if(!primary)
	{
		cChain *nnode = new cChain(scrn);
		nnode->linkTo(chainHead);
	}

	scrn->m_Private = (void *)primary;

	return scrn;
}

void GLVideo::ReleaseSurface (DCanvas *scrn)
{
	if(scrn->buffer)
	{
		delete[] scrn->buffer;
		scrn->buffer = NULL;
		delete scrn; // Denis - TODO!!!
	}
}

void GLVideo::LockSurface (DCanvas *scrn)
{
}


void GLVideo::UnlockSurface (DCanvas *scrn)
{
}

bool GLVideo::Blit (DCanvas *src, int sx, int sy, int sw, int sh, DCanvas *dst, int dx, int dy, int dw, int dh)
{
	return false;
}

VERSION_CONTROL (i_glvideo_cpp, "$Id$")
