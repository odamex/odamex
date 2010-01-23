// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2006-2007 by The Odamex Team.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// DESCRIPTION:
//	OpenGL render stuff
//
//-----------------------------------------------------------------------------

#include <map>
#include <vector>

using namespace std;

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

#include <math.h>

#include <SDL_opengl.h>

#include "v_palette.h"
#include "i_sdlvideo.h"
#include "i_system.h"
#include "w_wad.h"
#include "r_main.h"
#include "vectors.h"
#include "z_zone.h"
#include "w_wad.h"
#include "r_bsp.h"

#define FLAT_WIDTH 64

CVAR(opengl, "1", 0)

bool R_CheckBBox (fixed_t *bspcoord);
void R_ClearPlanes ();
void R_ClearSprites ();

char *sectors_visible;
int numsectors_visible;
bool use_bsp = true;

#define GL_TEXTURE_MAX_ANISOTROPY_EXT 0x84FE
#define GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT 0x84FF

class glpatch_t
{
public:
	GLuint texture;
	float w, h; // 0.0 to 1.0
};

typedef std::map<size_t, glpatch_t> mapi_t; // composites
static mapi_t loaded, compos, flats;

void PurgeTextures()
{
	mapi_t::iterator i;

	for(i = compos.begin(); i != compos.end(); i++)
	{
		glDeleteTextures(1, &i->second.texture);
	}
	compos.clear();

	for(i = flats.begin(); i != flats.end(); i++)
	{
		glDeleteTextures(1, &i->second.texture);
	}
	flats.clear();

	for(i = loaded.begin(); i != loaded.end(); i++)
	{
		glDeleteTextures(1, &i->second.texture);
	}
	loaded.clear();
}


/*BEGIN_CUSTOM_CVAR(anisotropy, "8", CVAR_ARCHIVE)
{
	if(!opengl)
		return;

	const char *ext = (const char *)glGetString(GL_EXTENSIONS);

	if(!ext || !strstr(ext, "GL_EXT_texture_filter_anisotropic"))
	{
		Printf(PRINT_HIGH, "GL_EXT_texture_filter_anisotropic: not supported by this renderer\n");
		Printf(PRINT_HIGH, "Renderer: %s", glGetString(GL_RENDERER));
		Printf(PRINT_HIGH, "OpenGL: %s", glGetString(GL_VERSION));
		return;
	}

	float max = 0.0f;
	glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &max);

	if(anisotropy > max)
	{
		Printf(PRINT_HIGH, "anisotropy: renderer is limited to %d levels\n", (int)max);
		anisotropy = max;
	}

	PurgeTextures();
}
END_CUSTOM_CVAR(anisotropy)*/

size_t next_pow2(size_t in)
{
	size_t pow2 = 1;

	while(in > pow2)
		pow2 *= 2;

	return pow2;
}

glpatch_t &get_glpatch(size_t patchnum, bool paletteTexture = true)
{
	mapi_t::iterator it = loaded.find(patchnum);

	if(it == loaded.end())
	{
		patch_t *p = (patch_t *)W_CacheLumpNum (patchnum, PU_CACHE);

		if(!p)
			I_Error("bad patch requested of get_glpatch"); // denis - todo use a placeholder texture

		// upload to gfx card
		glpatch_t tmp;

		glEnable(GL_TEXTURE_2D);
		glGenTextures(1, &tmp.texture);
		glBindTexture(GL_TEXTURE_2D, tmp.texture);

		size_t pow2w = next_pow2(p->width()), pow2h = next_pow2(p->height()), bypp = 4;

		size_t size = pow2w * pow2h * bypp;
		byte *mem = (byte *)malloc(size);
		memset(mem, 0, size);

		for(size_t i = 0; i < p->width(); i++)
		{
			byte *render = mem + (pow2h*i*bypp);
			column_t *column = (column_t *)((byte *)p + LONG(p->columnofs[i]));

			while (column->topdelta != 0xff)
			{
				DWORD *r = (DWORD *)(render + column->topdelta*bypp);

				for(int j = 0; j < column->length; j++)
				{
					byte s = *((byte *)(column) + 2 + j);
					r[j] = LONG(V_Palette[s]);
					if(paletteTexture)
					{
						((byte *)(&r[j]))[3] = 0xFF; // make alpha non-transparent
						((byte *)(&r[j]))[2] = 0; // zero out colour channel
						((byte *)(&r[j]))[1] = 0; // zero out colour channel
						((byte *)(&r[j]))[0] = s; // single colour for texture-texture lookup
					}
					else
					{
						((byte *)(&r[j]))[3] = 0xFF; // make alpha non-transparent
						byte t = ((byte *)(&r[j]))[2];
						((byte *)(&r[j]))[2] = ((byte *)(&r[j]))[0];
						((byte *)(&r[j]))[0] = t;
					}

				}

				column = (column_t *)(	(byte *)column + column->length + 4);
			}
		}

		glTexImage2D(GL_TEXTURE_2D, 0, bypp, pow2h, pow2w, 0, GL_RGBA, GL_UNSIGNED_BYTE, (void *)mem);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

/*		if(anisotropy)
		{
			float max = 0.0f;
			glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &max);
			if(max > anisotropy)
				max = anisotropy;
			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, max);
		}*/

		tmp.w = (float)p->width()/pow2w;
		tmp.h = (float)p->height()/pow2h;

		loaded.insert(mapi_t::value_type(patchnum, tmp));
		it = loaded.find(patchnum);

		free(mem);
	}

	return it->second;
}

glpatch_t &get_glflat(size_t useflatnum)
{
	mapi_t::iterator it = flats.find(useflatnum);

	if(it == flats.end())
	{
		byte *ds_source = (byte *)W_CacheLumpNum (firstflat + useflatnum, 1);

		if(!ds_source)
			I_Error("bad flat requested of get_glflat"); // denis - todo use a placeholder texture

		// upload to gfx card
		glpatch_t tmp;

		glEnable(GL_TEXTURE_2D);
		glGenTextures(1, &tmp.texture);
		glBindTexture(GL_TEXTURE_2D, tmp.texture);

		size_t pow2 = next_pow2(FLAT_WIDTH), bypp = 4;

		size_t size = pow2 * pow2 * bypp;
		byte *mem = (byte *)malloc(size);
		memset(mem, 0, size);
		DWORD *r = (DWORD *)(mem);

		for(size_t j = 0; j < FLAT_WIDTH; j++)
			for(size_t i = 0; i < FLAT_WIDTH; i++)
			{
				byte s = ds_source[j*FLAT_WIDTH+i];
				r[j*FLAT_WIDTH+i] = LONG(V_Palette[s]);
				((byte *)(&r[j*FLAT_WIDTH+i]))[0] = s; // make alpha non-transparent
				((byte *)(&r[j*FLAT_WIDTH+i]))[1] = s; // make alpha non-transparent
				((byte *)(&r[j*FLAT_WIDTH+i]))[2] = s; // make alpha non-transparent
				((byte *)(&r[j*FLAT_WIDTH+i]))[3] = 0xFF; // make alpha non-transparent
			}

		glTexImage2D(GL_TEXTURE_2D, 0, 1, pow2, pow2, 0, GL_RED, GL_UNSIGNED_BYTE, (void *)ds_source);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

		/*if(anisotropy)
		{
			float max = 0.0f;
			glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &max);
			if(max > anisotropy)
				max = anisotropy;
			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, max);
		}*/

		tmp.w = (float)FLAT_WIDTH/pow2;
		tmp.h = (float)FLAT_WIDTH/pow2;

		flats.insert(mapi_t::value_type(useflatnum, tmp));
		it = flats.find(useflatnum);

		free(mem);
	}

	return it->second;
}


// A single patch from a texture definition,
//	basically a rectangular area within
//	the texture rectangle.
typedef struct
{
	// Block origin (always UL),
	// which has already accounted
	// for the internal origin of the patch.
	int 		originx;
	int 		originy;
	int 		patch;
} texpatch_t;

// A maptexturedef_t describes a rectangular texture,
//	which is composed of one or more mappatch_t structures
//	that arrange graphic patches.
typedef struct
{
	// Keep name for switch changing, etc.
	char		name[9];
	short		width;
	short		height;

	// [RH] Use a hash table similar to the one now used
	//		in w_wad.c, thus speeding up level loads.
	//		(possibly quite considerably for larger levels)
	int			index;
	int			next;

	// All the patches[patchcount]
	//	are drawn back to front into the cached texture.
	short		patchcount;
	texpatch_t	patches[1];

} texture_t;



extern texture_t	**textures;
extern unsigned		**texturecolumnofs;	// killough 4/9/98: make 32-bit
extern byte**			texturecomposite;
extern short** 	texturecolumnlump;

void R_GenerateComposite (int texnum);

glpatch_t &get_glcomposite(size_t tex)
{
	mapi_t::iterator it = compos.find(tex);

	if(it == compos.end())
	{
		extern int numtextures;

		if(tex >= numtextures)
			I_Error("bad composite texture requested of get_glcomposite"); // denis - todo use a placeholder texture
		//R_GenerateComposite(tex);

		// upload to gfx card
		glpatch_t tmp;

		glEnable(GL_TEXTURE_2D);
		glGenTextures(1, &tmp.texture);
		glBindTexture(GL_TEXTURE_2D, tmp.texture);

		size_t pow2w = next_pow2(textures[tex]->width), pow2h = next_pow2(textures[tex]->height), bypp = 4;

		size_t size = pow2w * pow2h * bypp;
		byte *mem = (byte *)malloc(size);
		memset(mem, 0, size);

		for(size_t i = 0; i < textures[tex]->width; i++)
		{
			byte *render = mem + (pow2h*i*bypp);
			int lump = texturecolumnlump[tex][i];
			byte *ds_source = R_GetColumn(tex, i);

			if(lump == -1) // multiple patches for this column?
			{
				DWORD *r = (DWORD *)(render);
				for(size_t j = 0; j < textures[tex]->height; j++)
				{
					byte s = ds_source[j];
					r[j] = LONG(V_Palette[s]);
					((byte *)(&r[j]))[3] = 0xFF; // make alpha non-transparent
					((byte *)(&r[j]))[2] = 0; // make alpha non-transparent
					((byte *)(&r[j]))[1] = 0; // make alpha non-transparent
					((byte *)(&r[j]))[0] = s; // make alpha non-transparent
				}
			}
			else
			{
				column_t *column = (column_t *)(ds_source-3);

				while (column->topdelta != 0xff && column->topdelta + column->length <= textures[tex]->height)
				{
					DWORD *r = (DWORD *)(render + column->topdelta*bypp);

					for(int j = 0; j < column->length; j++)
					{
						byte s = *((byte *)(column) + 3 + j);
						r[j] = LONG(V_Palette[s]);
						((byte *)(&r[j]))[3] = 0xFF; // make alpha non-transparent
						((byte *)(&r[j]))[2] = s; // make alpha non-transparent
						((byte *)(&r[j]))[1] = s; // make alpha non-transparent
						((byte *)(&r[j]))[0] = s; // make alpha non-transparent
					}

					column = (column_t *)(	(byte *)column + column->length + 4);
				}
			}
		}

		glTexImage2D(GL_TEXTURE_2D, 0, bypp, pow2h, pow2w, 0, GL_RGBA, GL_UNSIGNED_BYTE, (void *)mem);

		free(mem);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

		/*if(anisotropy)
		{
			float max = 0.0f;
			glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &max);
			if(max > anisotropy)
				max = anisotropy;
			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, max);
		}*/

		tmp.w = (float)textures[tex]->width/pow2w;
		tmp.h = (float)textures[tex]->height/pow2h;

		compos.insert(mapi_t::value_type(tex, tmp));
		it = compos.find(tex);
	}

	return it->second;
}

vector<double *> z;

#ifndef CALLBACK
#define CALLBACK
#endif

GLvoid CALLBACK begin_callback(GLenum type)
{
	glBegin(type);
}

GLvoid CALLBACK end_callback()
{
	glEnd();
}

GLvoid CALLBACK error_callback(GLenum error)
{
	static bool once = true;

	if(once)
	{
		Printf(PRINT_HIGH, "GLU tessellation error: %s", gluErrorString(error));
		once = false;
	}
}

GLvoid CALLBACK combine_callback(GLdouble coords[3], GLdouble *d[4], GLfloat w[4], GLdouble **dataOut)
{
	*dataOut = *d;
}

GLvoid CALLBACK vertex_callback(GLdouble *d)
{
	glTexCoord2d(d[3], d[4]);
	glVertex3dv(d);
}

void DrawFlats()
{
	glDisable(GL_BLEND);
	glDisable(GL_ALPHA_TEST);

	// denis - never did get subsectors to work
	/*
	for(size_t i = 0; i < numsubsectors; i++)
	{
	glBegin(GL_POLYGON);
		glVertex3f(FIXED2FLOAT(segs[ subsectors[i].firstline].v1->y), FIXED2FLOAT(subsectors[i].sector->floorheight), FIXED2FLOAT(segs[ subsectors[i].firstline].v1->x));
		for(size_t j = subsectors[i].firstline; j < subsectors[i].firstline+subsectors[i].numlines; j++)
		{
	glVertex3f(FIXED2FLOAT(segs[j].v2->y), FIXED2FLOAT(subsectors[i].sector->floorheight), FIXED2FLOAT(segs[j].v2->x));
	}
	glEnd();
	}

	return;*/

	// Blast flats to screen
	{
	#ifdef UNIX
#ifdef OSX
		typedef void (*fn)(...);
#else
		typedef void (*fn)();
#endif
	#else
#ifdef WIN32
		typedef void (__stdcall *fn)();
#else
		typedef void (*fn)();
#endif
	#endif

		GLUtesselator *tess = gluNewTess();

		gluTessCallback(tess, GLU_TESS_BEGIN, (fn)&begin_callback);
		gluTessCallback(tess, GLU_TESS_VERTEX, (fn)&vertex_callback);
		gluTessCallback(tess, GLU_TESS_END, (fn)&end_callback);
		gluTessCallback(tess, GLU_TESS_COMBINE, (fn)&combine_callback);
		gluTessCallback(tess, GLU_TESS_ERROR, (fn)&error_callback);

		gluTessProperty(tess, GLU_TESS_WINDING_RULE, GLU_TESS_WINDING_ODD);

		for(size_t i = 0; i < numsectors; i++)
		{
			if(use_bsp && !sectors_visible[i])
				continue;

			vector< line_t * > draw, draw2;
			size_t j;

			for(j = 0; j < numlines; j++)
			{
				if((lines[j].frontsector == sectors + i || lines[j].backsector == sectors + i)
				   && lines[j].frontsector != lines[j].backsector)
					draw.push_back(&lines[j]);
			}

			// arrange lines by connectivity
			size_t s = draw.size();

			if(!s)
				continue;

			vector<vertex_t *> vert;

			int f = sectors[i].floorheight;
			int c = sectors[i].ceilingheight;

			int light = (sectors[i].lightlevel >> LIGHTSEGSHIFT) + (foggy ? 0 : extralight);
			if (light >= LIGHTLEVELS)
				light = LIGHTLEVELS-1;
			else if (light < 0)
				light = 0;
			light /= LIGHTLEVELS;
			glColor4f(1, 1, light, 1);

			draw2 = draw;

			// FLOOR
			if(sectors[i].floorpic != skyflatnum)
			{
				glpatch_t &glp = get_glflat(flattranslation[sectors[i].floorpic]);
				glBindTexture(GL_TEXTURE_2D, glp.texture);

				gluTessBeginPolygon(tess, NULL);

				vert.push_back(draw[0]->v1);
				vert.push_back(draw[0]->v2);
				draw.erase(draw.begin());

				while(draw.size())
				{
					for(j = 0; j < draw.size(); j++)
					{
						if(draw[j]->v1->x == vert.back()->x && draw[j]->v1->y == vert.back()->y)
						{
							vert.push_back(draw[j]->v2);
							draw.erase(draw.begin()+j);
							break;
						}
						else if(draw[j]->v2->x == vert.back()->x && draw[j]->v2->y == vert.back()->y)
						{
							vert.push_back(draw[j]->v1);
							draw.erase(draw.begin()+j);
							break;
						}
						else if(draw[j]->v1->x == vert.front()->x && draw[j]->v1->y == vert.front()->y)
						{
							vert.insert(vert.begin(), draw[j]->v2);
							draw.erase(draw.begin()+j);
							break;
						}
						else if(draw[j]->v2->x == vert.front()->x && draw[j]->v2->y == vert.front()->y)
						{
							vert.insert(vert.begin(), draw[j]->v1);
							draw.erase(draw.begin()+j);
							break;
						}
					}

					if(j == draw.size() || !draw.size())
					{
						// a contour has been completed
						gluTessBeginContour(tess);

						// draw concave contour in order
						s = vert.size();
						for(j = 0; j < s; j++)
						{
							double *lol = new double[5];
							lol[0] = FIXED2FLOAT(vert[j]->y);
							lol[1] = FIXED2FLOAT(f);
							lol[2] = FIXED2FLOAT(vert[j]->x);
							lol[3] = (FIXED2FLOAT(vert[j]->x)+sectors[i].floor_xoffs)/FLAT_WIDTH;
							lol[4] = -((FIXED2FLOAT(vert[j]->y)+sectors[i].floor_yoffs)/FLAT_WIDTH);
							z.push_back(lol);
							gluTessVertex(tess, lol, lol);
						}

						gluTessEndContour(tess);

						vert.clear();

						// prepare for next cycle
						if(draw.size())
						{
							vert.push_back(draw[0]->v1);
							vert.push_back(draw[0]->v2);
							draw.erase(draw.begin());
						}
					}
				}

				gluTessEndPolygon(tess);
			}

			draw = draw2;

			// CEILING
			if(sectors[i].ceilingpic != skyflatnum)
			{
				glpatch_t &glp = get_glflat(flattranslation[sectors[i].ceilingpic]);
				glBindTexture(GL_TEXTURE_2D, glp.texture);

				gluTessBeginPolygon(tess, NULL);

				vert.push_back(draw[0]->v1);
				vert.push_back(draw[0]->v2);
				draw.erase(draw.begin());

				while(draw.size())
				{
					for(j = 0; j < draw.size(); j++)
					{
						if(draw[j]->v1->x == vert.back()->x && draw[j]->v1->y == vert.back()->y)
						{
							vert.push_back(draw[j]->v2);
							draw.erase(draw.begin()+j);
							break;
						}
						else if(draw[j]->v2->x == vert.back()->x && draw[j]->v2->y == vert.back()->y)
						{
							vert.push_back(draw[j]->v1);
							draw.erase(draw.begin()+j);
							break;
						}
						else if(draw[j]->v1->x == vert.front()->x && draw[j]->v1->y == vert.front()->y)
						{
							vert.insert(vert.begin(), draw[j]->v2);
							draw.erase(draw.begin()+j);
							break;
						}
						else if(draw[j]->v2->x == vert.front()->x && draw[j]->v2->y == vert.front()->y)
						{
							vert.insert(vert.begin(), draw[j]->v1);
							draw.erase(draw.begin()+j);
							break;
						}
					}

					if(j == draw.size() || !draw.size())
					{
						// a contour has been completed
						gluTessBeginContour(tess);

						// draw concave contour in order
						s = vert.size();
						for(j = 0; j < s; j++)
						{
							double *lol = new double[5];
							lol[0] = FIXED2FLOAT(vert[j]->y);
							lol[1] = FIXED2FLOAT(c);
							lol[2] = FIXED2FLOAT(vert[j]->x);
							lol[3] = (FIXED2FLOAT(vert[j]->x)+sectors[i].ceiling_xoffs)/FLAT_WIDTH;
							lol[4] = -((FIXED2FLOAT(vert[j]->y)+sectors[i].ceiling_yoffs)/FLAT_WIDTH);
							z.push_back(lol);
							gluTessVertex(tess, lol, lol);
						}

						gluTessEndContour(tess);

						vert.clear();

						// prepare for next cycle
						if(draw.size())
						{
							vert.push_back(draw[0]->v1);
							vert.push_back(draw[0]->v2);
							draw.erase(draw.begin());
						}
					}
				}

				gluTessEndPolygon(tess);
			}

			for(j = 0; j < z.size(); j++)
			{
				delete[] z[j];
			}
			z.clear();
			// draw classic spiral for each sector, used for debugging
			/*glColor4f(0,0,1,1);
			glBegin(GL_LINE_LOOP);
			for(j = 0; j < vert.size(); j++)
			{
				glVertex3f(FIXED2FLOAT(vert[j]->y), FIXED2FLOAT(f)+((double)j), FIXED2FLOAT(vert[j]->x));
			}
			glEnd();*/
		}

		gluDeleteTess(tess);
	}
}

void DrawWalls()
{
	glEnable(GL_DEPTH_TEST);

	// Blast walls to screen
	for(int j = 0; j < numlines; j++)
	{
		line_t *l = &lines[j];
		side_t *frontside = &sides[l->sidenum[0]];
		side_t *backside = l->sidenum[1] < 0 ? 0 : &sides[l->sidenum[1]];

		fixed_t top, topmid, botmid, bot;

		top = topmid = l->frontsector->ceilingheight;
		bot = botmid = l->frontsector->floorheight;

		if(use_bsp && !sectors_visible[l->frontsector - sectors])
			continue;

		if(l->backsector)
		{
			if(top < l->backsector->ceilingheight)
				top = l->backsector->ceilingheight;
			else
				topmid = l->backsector->ceilingheight;

			if(bot > l->backsector->floorheight)
				bot = l->backsector->floorheight;
			else
				botmid = l->backsector->floorheight;
		}

		texture_t *texture;
		int patchnum;

//		float light = l->frontsector->lightlevel/255.0;
//		glColor4f(light,light,light,1);
		int light = (l->frontsector->lightlevel >> LIGHTSEGSHIFT) + (foggy ? 0 : extralight);
		if (light >= LIGHTLEVELS)
			light = LIGHTLEVELS-1;
		else if (light < 0)
			light = 0;
		light /= LIGHTLEVELS;
		glColor4f(1, 1, light, 1);

		#define MAG(dx,dy) (sqrt((dx*dx)+(dy*dy)))
		float wall_length = MAG(FIXED2FLOAT(l->v1->x - l->v2->x), FIXED2FLOAT(l->v1->y - l->v2->y));

		if(frontside->midtexture)
		{
			if(texture == (texture_t*)-1)
				continue;

			texture = textures[texturetranslation[frontside->midtexture]];

			glpatch_t &glp = get_glcomposite(texturetranslation[frontside->midtexture]);

			glEnable(GL_TEXTURE_2D);
			glBindTexture(GL_TEXTURE_2D, glp.texture);

			if(l->backsector)
			{
				glEnable(GL_BLEND);
				glEnable(GL_ALPHA_TEST);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP); // denis - do not repeat textures of hanging banners
			}
			else
			{
				// single sided lines are not transparent
				glDisable(GL_BLEND);
				glDisable(GL_ALPHA_TEST);
			}

			float tow = glp.w*FIXED2FLOAT(frontside->textureoffset)/texture->width;
			float tlw = glp.w*wall_length/texture->width;

			float wall_height = FIXED2FLOAT(topmid-botmid);
			float toh = glp.h*FIXED2FLOAT(frontside->rowoffset)/texture->height;
			float tlh = glp.h*wall_height/texture->height;

			if(l->flags & ML_DONTPEGBOTTOM)
			{
				toh = 0;//glp.h*FIXED2FLOAT(frontside->rowoffset)/texture->height;
				tlh = glp.h;
			}

			glBegin(GL_QUADS);

			glTexCoord2f(toh + tlh, tow); // bottom left
			glVertex3f(FIXED2FLOAT(l->v1->y), FIXED2FLOAT(botmid), FIXED2FLOAT(l->v1->x));

			glTexCoord2f(toh + tlh, tlw + tow); // bottom right
			glVertex3f(FIXED2FLOAT(l->v2->y), FIXED2FLOAT(botmid), FIXED2FLOAT(l->v2->x));

			glTexCoord2f(toh, tlw + tow);
			glVertex3f(FIXED2FLOAT(l->v2->y), FIXED2FLOAT(topmid), FIXED2FLOAT(l->v2->x));

			glTexCoord2f(toh, tow);
			glVertex3f(FIXED2FLOAT(l->v1->y), FIXED2FLOAT(topmid), FIXED2FLOAT(l->v1->x));

			glEnd();

			if(l->backsector)
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		}
		if(frontside->toptexture)
		{
			glDisable(GL_BLEND);
			glDisable(GL_ALPHA_TEST);

			texture = textures[texturetranslation[frontside->toptexture]];

			glpatch_t &glp = get_glcomposite(texturetranslation[frontside->toptexture]);

			glEnable(GL_TEXTURE_2D);
			glBindTexture(GL_TEXTURE_2D, glp.texture);

			float tow = glp.w*FIXED2FLOAT(frontside->textureoffset)/texture->width;
			float tlw = glp.w*wall_length/texture->width;

			float wall_height = FIXED2FLOAT(top-topmid);
			float toh = glp.h - glp.h*FIXED2FLOAT(frontside->rowoffset);
			float tlh = glp.h*(wall_height)/texture->height;

			glBegin(GL_QUADS);

			glTexCoord2f(toh, tow);
			glVertex3f(FIXED2FLOAT(l->v1->y), FIXED2FLOAT(topmid), FIXED2FLOAT(l->v1->x));

			glTexCoord2f(toh, tlw + tow);
			glVertex3f(FIXED2FLOAT(l->v2->y), FIXED2FLOAT(topmid), FIXED2FLOAT(l->v2->x));

			glTexCoord2f(toh - tlh, tlw + tow);
			glVertex3f(FIXED2FLOAT(l->v2->y), FIXED2FLOAT(top), FIXED2FLOAT(l->v2->x));

			glTexCoord2f(toh - tlh, tow);
			glVertex3f(FIXED2FLOAT(l->v1->y), FIXED2FLOAT(top), FIXED2FLOAT(l->v1->x));

			glEnd();
		}
		if(frontside->bottomtexture)
		{
			glDisable(GL_BLEND);
			glDisable(GL_ALPHA_TEST);

			texture = textures[texturetranslation[frontside->bottomtexture]];
			glpatch_t &glp = get_glcomposite(texturetranslation[frontside->bottomtexture]);

			glEnable(GL_TEXTURE_2D);
			glBindTexture(GL_TEXTURE_2D, glp.texture);

			float tow = glp.w*FIXED2FLOAT(frontside->textureoffset)/texture->width;
			float tlw = glp.w*wall_length/texture->width;

			float wall_height = FIXED2FLOAT(botmid - bot);
			float toh = glp.h*FIXED2FLOAT(frontside->rowoffset);
			float tlh = glp.h*wall_height/texture->height;

			glBegin(GL_QUADS);

			glTexCoord2f(toh + tlh, tow);
			glVertex3f(FIXED2FLOAT(l->v1->y), FIXED2FLOAT(bot), FIXED2FLOAT(l->v1->x));

			glTexCoord2f(toh + tlh, tlw + tow);
			glVertex3f(FIXED2FLOAT(l->v2->y), FIXED2FLOAT(bot), FIXED2FLOAT(l->v2->x));

			glTexCoord2f(toh, tlw + tow);
			glVertex3f(FIXED2FLOAT(l->v2->y), FIXED2FLOAT(botmid), FIXED2FLOAT(l->v2->x));

			glTexCoord2f(toh, tow);
			glVertex3f(FIXED2FLOAT(l->v1->y), FIXED2FLOAT(botmid), FIXED2FLOAT(l->v1->x));

			glEnd();
		}
		if(backside && backside->toptexture)
		{
			glDisable(GL_BLEND);
			glDisable(GL_ALPHA_TEST);

			texture = textures[texturetranslation[backside->toptexture]];

			glpatch_t &glp = get_glcomposite(texturetranslation[backside->toptexture]);

			glEnable(GL_TEXTURE_2D);
			glBindTexture(GL_TEXTURE_2D, glp.texture);

			float tow = glp.w*FIXED2FLOAT(frontside->textureoffset)/texture->width;
			float tlw = glp.w*wall_length/texture->width;

			float wall_height = FIXED2FLOAT(top-topmid);
			float toh = glp.h - glp.h*FIXED2FLOAT(frontside->rowoffset);
			float tlh = glp.h*(wall_height)/texture->height;

			glBegin(GL_QUADS);

			glTexCoord2f(toh, tow);
			glVertex3f(FIXED2FLOAT(l->v1->y), FIXED2FLOAT(topmid), FIXED2FLOAT(l->v1->x));

			glTexCoord2f(toh, tlw + tow);
			glVertex3f(FIXED2FLOAT(l->v2->y), FIXED2FLOAT(topmid), FIXED2FLOAT(l->v2->x));

			glTexCoord2f(toh - tlh, tlw + tow);
			glVertex3f(FIXED2FLOAT(l->v2->y), FIXED2FLOAT(top), FIXED2FLOAT(l->v2->x));

			glTexCoord2f(toh - tlh, tow);
			glVertex3f(FIXED2FLOAT(l->v1->y), FIXED2FLOAT(top), FIXED2FLOAT(l->v1->x));

			glEnd();
		}
		if(backside && backside->bottomtexture)
		{
			glDisable(GL_BLEND);
			glDisable(GL_ALPHA_TEST);

			texture = textures[texturetranslation[backside->bottomtexture]];
			glpatch_t &glp = get_glcomposite(texturetranslation[backside->bottomtexture]);

			glEnable(GL_TEXTURE_2D);
			glBindTexture(GL_TEXTURE_2D, glp.texture);

			float tow = glp.w*FIXED2FLOAT(frontside->textureoffset)/texture->width;
			float tlw = glp.w*wall_length/texture->width;

			float wall_height = FIXED2FLOAT(botmid - bot);
			float toh = glp.h*FIXED2FLOAT(frontside->rowoffset);
			float tlh = glp.h*wall_height/texture->height;

			glBegin(GL_QUADS);

			glTexCoord2f(toh + tlh, tow);
			glVertex3f(FIXED2FLOAT(l->v1->y), FIXED2FLOAT(bot), FIXED2FLOAT(l->v1->x));

			glTexCoord2f(toh + tlh, tlw + tow);
			glVertex3f(FIXED2FLOAT(l->v2->y), FIXED2FLOAT(bot), FIXED2FLOAT(l->v2->x));

			glTexCoord2f(toh, tlw + tow);
			glVertex3f(FIXED2FLOAT(l->v2->y), FIXED2FLOAT(botmid), FIXED2FLOAT(l->v2->x));

			glTexCoord2f(toh, tow);
			glVertex3f(FIXED2FLOAT(l->v1->y), FIXED2FLOAT(botmid), FIXED2FLOAT(l->v1->x));

			glEnd();
		}
	}
}

void DrawActors()
{
	// Render all the actor billboards
	glEnable(GL_BLEND);
	glEnable(GL_ALPHA_TEST);
	glEnable(GL_TEXTURE_2D);
	glColor4f(1,1,1,1);

	AActor *mo;
	AActor *co = displayplayer().mo;
	TThinkerIterator<AActor> iterator;

	if(!co)
		return;

	while ( (mo = iterator.Next() ) )
	{
		if(!mo->subsector || (use_bsp && !sectors_visible[mo->subsector->sector - sectors]))
			continue;

		if(mo->sprite >= numsprites)
			continue;

		if(mo->translucency == 0 || mo->state - states == S_NULL)
			continue;

		if(mo == displayplayer().mo)
			continue;

		spritedef_t *sprdef = &sprites[mo->sprite];

		if(!sprdef->numframes)
			continue;

		spriteframe_t *sprframe = &sprdef->spriteframes[mo->frame & FF_FRAMEMASK];

		int lump = 0;
		bool flip;

		if (sprframe->rotate)
		{
				// choose a different rotation based on player view
			angle_t ang = R_PointToAngle2 (co->x, co->y, mo->x, mo->y);
			unsigned rot = (ang-mo->angle+(unsigned)(ANG45/2)*9)>>29;
			lump = sprframe->lump[rot];
			flip = (bool)sprframe->flip[rot];
		}
		else
		{
			lump = sprframe->lump[0];
			flip = (bool)sprframe->flip[0];
		}

		if(lump == -1)
			continue;

		int light = (mo->subsector->sector->lightlevel >> LIGHTSEGSHIFT) + (foggy ? 0 : extralight);
		if (light >= LIGHTLEVELS)
			light = LIGHTLEVELS-1;
		else if (light < 0)
			light = 0;
		light /= LIGHTLEVELS;
		glColor4f(1, 1, light, 1);

		patch_t *patch = (patch_t *)W_CacheLumpNum (lump, PU_CACHE);
		glpatch_t &glp = get_glpatch(lump);
		glBindTexture(GL_TEXTURE_2D, glp.texture);

			// denis - from my P_CheckSightEdges code
			// c = normalized vector perpendicular to player's line of sight
			// r = normalized vector parallel to c
			// w = r scaled by the radius of mobj t2
			// thereby, "t2->[x,y] + or - w[x,y]" gives you the edges of t2 from t1's point of view
			// s = r scaled by the patch left offset of mobj t2
			// this function used to only check the middle of t2
		vec3_t c, r, w, s;
		angle_t an = co->angle;
		an >>= ANGLETOFINESHIFT;
		c[0] = ((float)finecosine[an]) / 65536.0f;
		c[1] = ((float)finesine[an]) / 65536.0f;
		c[2] = 0;
		VectorNormalize(c);
		r[2] = 0;
		r[1] = c[0];
		r[0] = -c[1];
		VectorScale(r, patch->width() - patch->leftoffset(), w); // patch width
		r[2] = 0;
		r[1] = c[0];
		r[0] = -c[1];
		VectorScale(r, patch->leftoffset(), s); // patch left offset

		glBegin(GL_QUADS);

		glTexCoord2f(0, glp.w);
		glVertex3f(FIXED2FLOAT(mo->y)-w[1], FIXED2FLOAT(mo->z) + patch->topoffset(), FIXED2FLOAT(mo->x)-w[0]);

		glTexCoord2f(0, 0);
		glVertex3f(FIXED2FLOAT(mo->y)+s[1], FIXED2FLOAT(mo->z) + patch->topoffset(), FIXED2FLOAT(mo->x)+s[0]);

		glTexCoord2f(glp.h, 0);
		glVertex3f(FIXED2FLOAT(mo->y)+s[1], FIXED2FLOAT(mo->z) - patch->height() + patch->topoffset(), FIXED2FLOAT(mo->x)+s[0]);

		glTexCoord2f(glp.h, glp.w);
		glVertex3f(FIXED2FLOAT(mo->y)-w[1], FIXED2FLOAT(mo->z) - patch->height() + patch->topoffset(), FIXED2FLOAT(mo->x)-w[0]);

		glEnd();

//			glDisable(GL_TEXTURE_2D);
//			extern void P_CalcSightTraverse(subsector_t *sub, fixed_t x, fixed_t y, fixed_t z);
//			P_CalcSightTraverse(mo->subsector, mo->x, mo->y, mo->z + mo->height);
//			glEnable(GL_TEXTURE_2D);
		}
}
void R_Subsector (int num);

//
// RenderBSPNode
// Renders all sectors below a given node,
//	traversing subtree recursively.
void DetermineVisibleSectorsBSP (int bspnum)
{
	while (!(bspnum & NF_SUBSECTOR))  // Found a subsector?
	{
		node_t *bsp = &nodes[bspnum];

		// Decide which side the view point is on.
		int side = R_PointOnSide(viewx, viewy, bsp);

		// Recursively divide front space.
		DetermineVisibleSectorsBSP(bsp->children[side]);

		// Possibly divide back space.
		if (!R_CheckBBox(bsp->bbox[side^1]))
			return;

		bspnum = bsp->children[side^1];
	}

	if(bspnum < 0)
		bspnum = 0;
	else
		bspnum &= ~NF_SUBSECTOR;

	R_Subsector(bspnum);

	sectors_visible[subsectors[bspnum].sector - sectors] = 1;

	subsector_t &sub = subsectors[bspnum];
	for(size_t i = sub.firstline; i != sub.numlines+sub.firstline; i++)
	{
		sectors_visible[segs[i].frontsector - sectors] = 1;

		if(segs[i].backsector)
			sectors_visible[segs[i].backsector - sectors] = 1;
	}
}

void DetermineVisibleSectorsBSP ()
{
	if(!displayplayer().mo || !nodes || !numnodes)
		return;

	viewx = displayplayer().mo->x;
	viewy = displayplayer().mo->y;
	viewangle = displayplayer().mo->angle;

	if(numsectors_visible != numsectors)
	{
		if(sectors_visible)
			delete[] sectors_visible;

		sectors_visible = new char[numsectors];
		numsectors_visible = numsectors;
	}

	memset(sectors_visible, 0, numsectors_visible);

	// Clear buffers.
	R_ClearClipSegs ();
	R_ClearDrawSegs ();
	R_ClearPlanes ();
	R_ClearSprites ();

	DetermineVisibleSectorsBSP(numnodes - 1);
}


void DrawCameraSetup()
{
	glScalef(0.01, 0.01, 0.01);

	if(displayplayer().mo)
	{
		AActor *mo = displayplayer().mo;

		if(mo)
		{
				// pitch
			glRotatef(mo->pitch/ANG(1), 1, 0, 0);

				// yaw
			glRotatef(-mo->angle/ANG(1), 0, 1, 0);
			glRotatef(180, 0, 1, 0);


			glTranslatef(-FIXED2FLOAT(mo->y), -FIXED2FLOAT(consoleplayer().viewz), -FIXED2FLOAT(mo->x));
		}
	}
}

void DrawSky()
{
	// Skybox // denis - todo
	float w = 10;
	glEnable(GL_TEXTURE_2D);
	glColor4f(1,1,1,1);

//	int lump = W_CheckNumForName("F_SKY1");
	extern int skytexture;
	int lump = skytexture;
	patch_t *patch = (patch_t *)W_CacheLumpNum (lump, PU_CACHE);
	glpatch_t &glp = get_glcomposite(lump);
	glBindTexture(GL_TEXTURE_2D, glp.texture);

	glPushMatrix();

	AActor *mo = displayplayer().mo;

	if(mo)
	{
				// pitch
		glRotatef(mo->pitch/ANG(1), 1, 0, 0);

				// yaw
		glRotatef(-mo->angle/ANG(1), 0, 1, 0);
		glRotatef(180, 0, 1, 0);
	}

	glBegin(GL_QUADS);

		glTexCoord2f(0, 0); glVertex3f(w,w,w);
		glTexCoord2f(0, 1); glVertex3f(w,w,-w);
		glTexCoord2f(1, 1); glVertex3f(w,-w,-w);
		glTexCoord2f(1, 0); glVertex3f(w,-w,w);

		glTexCoord2f(0, 0); glVertex3f(-w,w,w);
		glTexCoord2f(0, 1); glVertex3f(-w,w,-w);
		glTexCoord2f(1, 1); glVertex3f(-w,-w,-w);
		glTexCoord2f(1, 0); glVertex3f(-w,-w,w);

		glTexCoord2f(0, 0); glVertex3f(w,w,w);
		glTexCoord2f(0, 1); glVertex3f(w,w,-w);
		glTexCoord2f(1, 1); glVertex3f(-w,w,-w);
		glTexCoord2f(1, 0); glVertex3f(-w,w,w);

		glTexCoord2f(0, 0); glVertex3f(w,-w,w);
		glTexCoord2f(0, 1); glVertex3f(w,-w,-w);
		glTexCoord2f(1, 1); glVertex3f(-w,-w,-w);
		glTexCoord2f(1, 0); glVertex3f(-w,-w,w);

		glTexCoord2f(0, 0); glVertex3f(w,w,w);
		glTexCoord2f(0, 1); glVertex3f(w,-w,w);
		glTexCoord2f(1, 1); glVertex3f(-w,-w,w);
		glTexCoord2f(1, 0); glVertex3f(-w,w,w);

		glTexCoord2f(0, 0); glVertex3f(w,w,-w);
		glTexCoord2f(0, 1); glVertex3f(w,-w,-w);
		glTexCoord2f(1, 1); glVertex3f(-w,-w,-w);
		glTexCoord2f(1, 0); glVertex3f(-w,w,-w);

	glEnd();

	glPopMatrix();
}

void DrawQuad(float x, float y, float w, float h);
void DrawTexturedQuad(float x, float y, float w, float h, float tw, float th, float tx = 0.0f, float ty = 0.0f);

void DrawScreenSprites()
{
	AActor *camera = displayplayer().camera;

	if(!camera)
		return;

	pspdef_t* psp = camera->player->psprites;
	GLuint last_texture = 0;

	// add all active psprites
	for (int i=0; i < NUMPSPRITES; i++, psp++)
	{
		if (!psp->state)
			continue;

		spritedef_t *sprdef = &sprites[psp->state->sprite];
		spriteframe_t *sprframe = &sprdef->spriteframes[ psp->state->frame & FF_FRAMEMASK ];

		int lump = sprframe->lump[0];
		bool flip = (bool)sprframe->flip[0];

		patch_t *patch = (patch_t *)W_CacheLumpNum (lump, PU_CACHE);
		glpatch_t &glp = get_glpatch(lump);
		if(glp.texture != last_texture)
		{
		glBindTexture(GL_TEXTURE_2D, glp.texture);
		last_texture = glp.texture;
		}

		float xscale = 1/320.0f;
		float yscale = 1/200.0f;

		float w = xscale*patch->width();
		float h = yscale*patch->height();
		float x = -(patch->leftoffset() + FIXED2FLOAT(psp->sx))*xscale;
		float y = -(patch->topoffset() - FIXED2FLOAT(psp->sy))*yscale;

		glEnable(GL_TEXTURE_2D);
		glEnable(GL_BLEND);
		glEnable(GL_ALPHA_TEST);
		glColor4f(1, 1, 1, 1);

		DrawTexturedQuad(x, y, w, h, glp.h, glp.w);
	}
}

void DrawConsoleBackground()
{
	extern char *pagename;

	if(pagename == NULL)
		return;

	int num = W_GetNumForName(pagename);

	if(num == -1)
		return;

	glDisable(GL_BLEND);
	glDisable(GL_ALPHA_TEST);
	glEnable(GL_TEXTURE_2D);

	glpatch_t &glp = get_glpatch(num, false);
	glBindTexture(GL_TEXTURE_2D, glp.texture);

	glColor4f(1, 1, 1, 1);

	DrawTexturedQuad(0, 0, 1, 1, glp.h, glp.w);
}

EXTERN_CVAR(crosshair)

void DrawCrosshair()
{
/*	extern patch_t *crosshair_patch;
	extern int crosshair_lump;

	if(!crosshair || !crosshair_patch)
		return;

	glpatch_t &glp = get_glpatch(crosshair_lump);
	glBindTexture(GL_TEXTURE_2D, glp.texture);

	float xscale = 1/320.0f;
	float yscale = 1/200.0f;

	float w = (float)crosshair_patch->width()*xscale;
	float h = (float)crosshair_patch->height()*yscale;
	float x = 0.5 - w/2;
	float y = 0.5 - h/2;

	glEnable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
	glEnable(GL_ALPHA_TEST);
	glColor4f(1, 1, 1, crosshair < 0 ? 0.5f : 1.0f);

	DrawTexturedQuad(x, y, w, h, glp.h, glp.w);*/
}

void DrawConsoleText(size_t screenw, size_t screenh)
{
	extern int ConRows, ConCols, PhysRows;
	extern int ConBottom;
	extern char *Lines, *Last;
	extern int SkipRows, ConBottom, ConScroll, RowAdjust;

	// Junk from C_DrawConsole
	int lines, left, offset;
	static int oldbottom = 0;
	char *zap = Last - (SkipRows + RowAdjust) * (ConCols + 2);

	left = 8;
	lines = (ConBottom-12)/8;
	if (-12 + lines*8 > ConBottom - 28)
		offset = -16;
	else
		offset = -12;
	zap = Last - (SkipRows + RowAdjust) * (ConCols + 2);

	// Draw
	if (lines <= 0)
		return;

	float w = 8.0f/screenw;

	glColor4f(1,1,1,1);

	glEnable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
	glEnable(GL_ALPHA_TEST);

	patch_t *patch = (patch_t *)W_CacheLumpName ("CONCHARS", PU_CACHE);
	int num = W_GetNumForName ("CONCHARS");
	glpatch_t &glp = get_glpatch(num);
	glBindTexture(GL_TEXTURE_2D, glp.texture);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	float tw = 8.0f/patch->width();
	float th = 8.0f/patch->height();

	for (; lines > 1; lines--)
	{
		float x = w;
		for(size_t i = 0; i < zap[1]; i++)
		{
				float y = (float)(offset + lines * 8.0f)/screenh;
				char c = zap[2+i];
				float tx = (c%16)*tw;
				float ty = (c/16)*tw;
				DrawTexturedQuad(x, y, w, w, tw, th, ty, tx);
				x += w;
		}
		zap -= ConCols + 2;
	}
}

void SaturateDepth()
{
	glEnable(GL_DEPTH_TEST);

	glDepthFunc(GL_GREATER);
	glDisable(GL_TEXTURE_2D);

//	glColorMask(0, 0, 0, 0);
	glColor4f(0,0,0,0.06);

	AActor *mo = displayplayer().mo, *co = mo;

	if(!mo)
	 return;


			// denis - from my P_CheckSightEdges code
			// c = normalized vector perpendicular to player's line of sight
			// c = normalized vector parallel to player's line of sight
			// r = normalized vector parallel to c
			// w = r scaled by the radius of mobj t2
			// thereby, "t2->[x,y] + or - w[x,y]" gives you the edges of t2 from t1's point of view
			// this function used to only check the middle of t2
	vec3_t c, cc, r, w, ccc;
	angle_t an = co->angle;
	an >>= ANGLETOFINESHIFT;
	c[0] = ((float)finecosine[an]) / 65536.0f;
	c[1] = ((float)finesine[an]) / 65536.0f;
	c[2] = 0;
	VectorNormalize(c);
	r[2] = 0;
	r[1] = c[0];
	r[0] = -c[1];
	int width = 20000, height = 20000;
	VectorScale(r, width, w);
	cc[1] = ((float)finecosine[an]) / 65536.0f;
	cc[0] = ((float)finesine[an]) / 65536.0f;
	cc[2] = 0;
	VectorScale(c, width, ccc);

	glStencilFunc(GL_NOTEQUAL, 1, 1);
	glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
	glDepthFunc(GL_GREATER);

	glColorMask(0,1,0,1);
	glBegin(GL_QUADS);
	glDisable(GL_BLEND);
	glDisable(GL_ALPHA_TEST);

	for(int i = 0; i < LIGHTLEVELS; i++)
	{
		fixed_t distance = i << LIGHTZSHIFT;
		fixed_t x = mo->x + c[0]*distance, y = mo->y + c[1]*distance;
		glColor4f(0,(float)i/LIGHTLEVELS,0,1);

		// last shade run should shade in everything that is not already shaded
		if(i == LIGHTLEVELS-1)
		{
			glEnd();
			glDepthFunc(GL_ALWAYS);
			glBegin(GL_QUADS);
		}

		glVertex3f(FIXED2FLOAT(y)-w[1], FIXED2FLOAT(mo->z) - height, FIXED2FLOAT(x)-w[0]);
		glVertex3f(FIXED2FLOAT(y)+w[1], FIXED2FLOAT(mo->z) - height, FIXED2FLOAT(x)+w[0]);
		glVertex3f(FIXED2FLOAT(y)+w[1], FIXED2FLOAT(mo->z) + height, FIXED2FLOAT(x)+w[0]);
		glVertex3f(FIXED2FLOAT(y)-w[1], FIXED2FLOAT(mo->z) + height, FIXED2FLOAT(x)-w[0]);
	}

	glEnd();
	glColorMask(1,1,1,1);

	glDepthFunc(GL_LESS);
	glStencilFunc(GL_ALWAYS, 1, 1);
	glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
}

void RenderWorld()
{
	if(!displayplayer().mo)
		return;

	camera = displayplayer().mo;

	DrawSky();

	DrawCameraSetup();

	if(use_bsp)
		DetermineVisibleSectorsBSP();

	// Draw fullbright fully textured scene
	DrawWalls();
	DrawFlats();
	DrawActors();

	// Saturate depth buffer for pixels that are close, to keep them from being shaded later
	SaturateDepth();

	// Blend scene lighting onto screen using GL_EQUALS for depth buffer comparison

	// Draw distance shading grades using GL_LEQUAL
}

VERSION_CONTROL (i_render_cpp, "$Id$")
