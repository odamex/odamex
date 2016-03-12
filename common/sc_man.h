// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
// sc_man.h : Heretic 2 : Raven Software, Corp.
// Copyright (C) 2006-2015 by The Odamex Team.
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
//	
//	    General lump script parser from Hexen (MAPINFO, etc)
//
//-----------------------------------------------------------------------------

#ifndef __SC_MAN_H__
#define __SC_MAN_H__

#include "resources/res_main.h"

void SC_OpenResourceLump(const ResourceId res_id);
void SC_OpenFile(const char *name);
void SC_OpenMem(const char *name, char *buffer, int size);
void SC_Close();
void SC_SavePos();
void SC_RestorePos();
bool SC_GetString();
void SC_MustGetString();
void SC_MustGetStringName(const char *name);
bool SC_GetNumber();
void SC_MustGetNumber();
bool SC_GetFloat();
void SC_MustGetFloat();
void SC_UnGet();
//boolean SC_Check(void);
bool SC_Compare (const char *text);
int SC_MatchString (const char **strings);
int SC_MustMatchString (const char **strings);
void SC_ScriptError (const char *message, const char **args = NULL);

extern char *sc_String;
extern int sc_Number;
extern float sc_Float;
extern int sc_Line;
extern bool sc_End;
extern bool sc_Crossed;
extern bool sc_FileScripts;
extern char *sc_ScriptsDir;

#endif //__SC_MAN_H__

