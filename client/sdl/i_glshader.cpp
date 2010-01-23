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
//	OpenGL shader stuff
//
//-----------------------------------------------------------------------------

#include <string>

#include <SDL_opengl.h>
#include <SDL_video.h>

#include "c_console.h"

void PurgeTextures();

const char *shaders[][2] =
{
	{
		"GL_ARB_fragment_program",
		"!!ARBfp1.0\n"
		"OUTPUT output = result.color;\n"
		"TEMP tmp;\n"
		"TEX tmp, fragment.texcoord[0], texture[0], 2D;\n"
		"TEX result.color, tmp, texture[1], 2D;\n"
		"END\n"
	},

	{
		"GL_ATI_text_fragment_shader",
		"!!ATIfs1.0\n"
		// texture lookup
		"StartPrelimPass;\n"
		"SampleMap r0, t0.str;\n" // r1.r = 0 to 1 in 1/256th increments, bitmap color
		"MOV r2, r0;\n"
		"EndPass;\n"
		// colormap lookup
		"StartOutputPass;\n"
		"PassTexCoord r2, r2.str;\n"
		"SampleMap r1, r2.str;\n" // r1.r = 0 to 255, texture color
		"MOV r0, r1;\n"
		"EndPass;\n"
	},

	{
		"GL_ARB_fragment_shader", // todo NV_fragment_program NV_texture_shader/2/3 EXT_paletted_texture
		"uniform sampler2D tex0;\n"
		"uniform sampler2D tex1;\n"
		"void main(void)\n"
		"{\n"
		"vec2 color256 = vec2(texture2D(tex0, gl_TexCoord[0].st));\n"
		"gl_FragColor = texture2D(tex1, color256);\n"
		"}\n"
	},
	
	{NULL,NULL}	
};

int selected_shader;

typedef void (*_glActiveTextureARB_t)(GLenum texture);
typedef GLhandleARB (*glCreateProgramObjectARB_t)(void);
typedef GLhandleARB (*glCreateShaderObjectARB_t)(GLenum shaderType);
typedef void (*glShaderSourceARB_t)(GLhandleARB shader,
                       GLsizei nstrings,
                       const GLcharARB **strings,
                       const GLint *lengths);
typedef void (*glCompileShaderARB_t)(GLhandleARB shader);
typedef void (*glAttachObjectARB_t)(GLhandleARB program,
                       GLhandleARB shader);
typedef void (*glLinkProgramARB_t)(GLhandleARB program);
typedef void (*glUseProgramObjectARB_t)(GLhandleARB program);
typedef void (*glDeleteObjectARB_t)(GLhandleARB object);
typedef void (*glGetInfoLogARB_t)(GLhandleARB object,
                     GLsizei maxLength,
                     GLsizei *length,
                     GLcharARB *infoLog);
typedef void (*glProgramStringARB_t)(GLenum target, GLenum format, GLsizei len, 
					  const void *string);
typedef void (*glBindProgramARB_t)(GLenum target, GLuint program);
typedef void (*glDeleteProgramsARB_t)(GLsizei n, const GLuint *programs);
typedef void (*glGenProgramsARB_t)(GLsizei n, GLuint *programs);

_glActiveTextureARB_t _glActiveTextureARB;
glCreateProgramObjectARB_t glCreateProgramObjectARB;
glCreateShaderObjectARB_t glCreateShaderObjectARB;
glShaderSourceARB_t glShaderSourceARB;
glCompileShaderARB_t glCompileShaderARB;
glAttachObjectARB_t glAttachObjectARB;
glLinkProgramARB_t glLinkProgramARB;
glUseProgramObjectARB_t glUseProgramObjectARB;
glGetInfoLogARB_t glGetInfoLogARB;
glDeleteObjectARB_t glDeleteObjectARB;

glProgramStringARB_t glProgramStringARB;
glBindProgramARB_t glBindProgramARB;
glDeleteProgramsARB_t glDeleteProgramsARB;
glGenProgramsARB_t glGenProgramsARB;

GLhandleARB program, object;
GLuint shader;
GLuint texture1;

void EnableShader()
{
	// detect a compatible shader
	const char *ext = (const char *)glGetString(GL_EXTENSIONS);
	
	if(!ext)
	{
		Printf(PRINT_HIGH, "glGetString returned NULL\n");
		return;
	}

	while(shaders[selected_shader][0])
	{
		if(strstr(ext, shaders[selected_shader][0]))
			break;
		
		selected_shader++;
	}
	
	if(!shaders[selected_shader][0])
	{
		Printf(PRINT_HIGH, "No compatible pixel shaders found\n");
		Printf(PRINT_HIGH, "GL_EXTENSIONS: %s\n", glGetString(GL_EXTENSIONS));
		selected_shader = -1;
		return;
	}
	else
	{
		Printf(PRINT_HIGH, "Using shader: %s\n", shaders[selected_shader][0]);
	}
	
	// detect and enable multitexturing
	if(strstr(ext, "GL_ARB_multitexture") == 0)
	{
		Printf(PRINT_HIGH, "GL_ARB_multitexture not supported\n");
		return;
	}
	else
	{
		Printf(PRINT_HIGH, "GL_ARB_multitexture is supported\n");
	}

	// get opengl function pointers
	_glActiveTextureARB = (_glActiveTextureARB_t)SDL_GL_GetProcAddress("glActiveTextureARB");
	glCreateProgramObjectARB = (glCreateProgramObjectARB_t)SDL_GL_GetProcAddress("glCreateProgramObjectARB");
	glCreateShaderObjectARB = (glCreateShaderObjectARB_t)SDL_GL_GetProcAddress("glCreateShaderObjectARB");
	glShaderSourceARB = (glShaderSourceARB_t)SDL_GL_GetProcAddress("glShaderSourceARB");
	glCompileShaderARB = (glCompileShaderARB_t)SDL_GL_GetProcAddress("glCompileShaderARB");
	glAttachObjectARB = (glAttachObjectARB_t)SDL_GL_GetProcAddress("glAttachObjectARB");
	glLinkProgramARB = (glLinkProgramARB_t)SDL_GL_GetProcAddress("glLinkProgramARB");
	glUseProgramObjectARB = (glUseProgramObjectARB_t)SDL_GL_GetProcAddress("glUseProgramObjectARB");
	glDeleteObjectARB = (glDeleteObjectARB_t)SDL_GL_GetProcAddress("glDeleteObjectARB");
	glGetInfoLogARB = (glGetInfoLogARB_t)SDL_GL_GetProcAddress("glGetInfoLogARB");
	glProgramStringARB = (glProgramStringARB_t)SDL_GL_GetProcAddress("glProgramStringARB");
	glBindProgramARB = (glBindProgramARB_t)SDL_GL_GetProcAddress("glBindProgramARB");
	glDeleteProgramsARB = (glDeleteProgramsARB_t)SDL_GL_GetProcAddress("glDeleteObjectARB");
	glGenProgramsARB = (glGenProgramsARB_t)SDL_GL_GetProcAddress("glGenProgramsARB");

	// compile the compatible shader // denis - todo - GLSL
	if(selected_shader == 2)
	{
		GLcharARB tmp[2048];
		program = glCreateProgramObjectARB();
		object = glCreateShaderObjectARB(GL_FRAGMENT_SHADER_ARB);
		const char *p = shaders[selected_shader][1];
		glShaderSourceARB(object, 1, &p, NULL);
		glCompileShaderARB(object);	
		glGetInfoLogARB(object, sizeof(tmp), NULL, (GLcharARB *)&tmp);
		printf("glCompileShaderARB: %s\n", (char *)tmp);
		Printf(PRINT_HIGH, "glCompileShaderARB: %s\n", tmp);
		glAttachObjectARB(program, object);
		glLinkProgramARB(program);
		glGetInfoLogARB(program, sizeof(tmp), NULL, (GLcharARB *)&tmp);
		Printf(PRINT_HIGH, "glLinkProgramARB: %s\n", tmp);
		glUseProgramObjectARB(program);
		//glGetUniformLocationARB()
		//glUniform1iARB();
	}

	// enable shading
	if(selected_shader == 1 || selected_shader == 0)
	{
		#define TEXT_FRAGMENT_SHADER_ATI              0x8200
		glEnable(GL_FRAGMENT_PROGRAM_ARB);
//		glEnable(/*GL_FRAGMENT_PROGRAM_ARB*/TEXT_FRAGMENT_SHADER_ATI); // todo - what about GL_FRAGMENT_SHADER_ATI?
		glGenProgramsARB(1, &shader);
		glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, shader);
		glProgramStringARB(selected_shader == 0 ? GL_FRAGMENT_PROGRAM_ARB : TEXT_FRAGMENT_SHADER_ATI, GL_PROGRAM_FORMAT_ASCII_ARB, strlen(shaders[selected_shader][1]), shaders[selected_shader][1]);
		#define PROGRAM_ERROR_STRING_ARB 0x8874
		const char *result = (const char *)glGetString(PROGRAM_ERROR_STRING_ARB);
		if(result && strlen(result))
			Printf(PRINT_HIGH, "glProgramStringARB: %s\n", glGetString(PROGRAM_ERROR_STRING_ARB));
		glDisable(GL_FRAGMENT_PROGRAM_ARB);
	}

	// upload colormap texture
//	UpdateShaderPalette();
	
	// reinitialize textures
	PurgeTextures();
}
#include "w_wad.h"
#include "z_zone.h"
#include "v_video.h"
void UpdateShaderPalette(SDL_Color *colors)
{
	if(selected_shader != 0 && selected_shader != 1 && selected_shader != 2)
		return;
	
	static bool once = false;
	
	byte *colormaps = (byte *)W_CacheLumpName ("COLORMAP", PU_CACHE);
	
	if(!colormaps)
		return;
	
	size_t len = W_LumpLength(W_GetNumForName ("COLORMAP"));

	len /= 256;
	
	if(len > 256)
		len = 256;
	
	_glActiveTextureARB(GL_TEXTURE1_ARB);
	if(!once)
	{
		glGenTextures(1, &texture1);
		once = true;
	}
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, texture1);

	byte *mem = new byte[256*256*4];
	for(int j = 0; j < len; j++)
	{
		for(int i = 0; i < 256; i++)
		{
			SDL_Color c = colors[colormaps[j*256+i]];
			
			mem[(j*256+i)*4 + 0] = c.r;
			mem[(j*256+i)*4 + 1] = c.g;
			mem[(j*256+i)*4 + 2] = c.b;
			mem[(j*256+i)*4 + 3] = 0xFF;
		}
	}
	glTexImage2D(GL_TEXTURE_2D, 0, 4, 256, 16, 0, GL_RGBA, GL_UNSIGNED_BYTE, (void *)mem);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		
	delete[] mem;


	glDisable(GL_TEXTURE_2D);

	_glActiveTextureARB(GL_TEXTURE0_ARB);
}

void DisableShader()
{
	// check if was enabled
	
	// remove shader objects
	//glDeleteObjectARB(program);
	//glDeleteObjectARB(object);
	
	// remove colormap texture

	// disable shading
	if(selected_shader == 1)
	{
		glDisable(GL_FRAGMENT_PROGRAM_ARB);
	}

	// disable multitexture
	
	// reinitialize textures
	PurgeTextures();
}

GLuint r2texture;
size_t next_pow2(size_t in);

void RenderToTexture(size_t screenw, size_t screenh)
{
	static bool once(false);
	static size_t w(0), h(0);

	if(selected_shader == -1)
		return;

	if(!once)
	{
		glGenTextures(1, &r2texture);
		once = true;
	}
	
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, r2texture);

	if(selected_shader == -1)
	{
		// no shading at all, use imaging extension
		float map[256];
		for(int i = 0; i < 256; i++)map[i]=(float)(i)/256;
		//glPixelTransferi(GL_MAP_COLOR, true);
//		glPixelMapfv(GL_PIXEL_MAP_R_TO_R, 256, map);

		//glPixelMapfv(GL_PIXEL_MAP_I_TO_I, 256, map);
		glPixelMapfv(GL_PIXEL_MAP_I_TO_R, 256, map);
		glPixelMapfv(GL_PIXEL_MAP_I_TO_G, 256, map);
		glPixelMapfv(GL_PIXEL_MAP_I_TO_B, 256, map);
		glPixelMapfv(GL_PIXEL_MAP_I_TO_A, 256, map);
	}	

	if(w != next_pow2(screenw) || h != next_pow2(screenh))
	{
		w = next_pow2(screenw);
		h = next_pow2(screenh);
		
		glCopyTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 0, 0, w, h, 0);
		
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	}
	else
	{
		glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, screenw, screenh);
	}

	if(selected_shader == -1)
		glPixelTransferi(GL_MAP_COLOR, false);
}

void DrawTexturedQuad2(float x, float y, float w, float h, float tw, float th, float tx, float ty)
{
	glBegin(GL_QUADS);

	glTexCoord2f(tx, ty+th);
	glVertex2f(x, y);

	glTexCoord2f(tx, ty);
	glVertex2f(x, y+h);

	glTexCoord2f(tx+tw, ty);
	glVertex2f(x+w, y+h);

	glTexCoord2f(tx+tw, ty+th);
	glVertex2f(x+w, y);

	glEnd();
}

void RenderedTextureToScreen(size_t screenw, size_t screenh)
{
	size_t w = next_pow2(screenw);
	size_t h = next_pow2(screenh);

	glDisable(GL_BLEND);
	glDisable(GL_ALPHA_TEST);

	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, r2texture);

	if(selected_shader == 1 || selected_shader == 0)
	{
		glEnable(GL_FRAGMENT_PROGRAM_ARB);
		glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, shader);

		_glActiveTextureARB(GL_TEXTURE1_ARB);
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, texture1);
	}
	if(selected_shader == 2)
	{
		_glActiveTextureARB(GL_TEXTURE1_ARB);
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, texture1);

		glUseProgramObjectARB(program);
	}
	
	// render big quad
	if(selected_shader != -1)
		DrawTexturedQuad2(0, 0, 1, 1, (float)screenw/w, (float)screenh/h, 0, 0);
	
	glDisable(GL_TEXTURE_2D);

	if(selected_shader == 1 || selected_shader == 0)
	{
		_glActiveTextureARB(GL_TEXTURE0_ARB);
				
		glDisable(GL_FRAGMENT_PROGRAM_ARB);
	}
	if(selected_shader == 2)
	{
		glUseProgramObjectARB(0);
	}

	// render debugging quad
	glColorMask(1,0,0,1);
	DrawTexturedQuad2(0.1, 0.1, 0.3, 0.3, (float)screenw/w, (float)screenh/h, 0, 0);

	glColorMask(0,1,0,1);
	DrawTexturedQuad2(0.6, 0.1, 0.3, 0.3, (float)screenw/w, (float)screenh/h, 0, 0);

	glColorMask(1,1,1,1);

	glEnable(GL_BLEND);
}

VERSION_CONTROL (i_glshader_cpp, "$Id$")
