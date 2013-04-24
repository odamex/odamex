// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
// sc_man.h : Heretic 2 : Raven Software, Corp.
// Copyright (C) 2006-2013 by The Odamex Team.
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

void SC_Open (const char *name);
void SC_OpenFile (const char *name);
void SC_OpenMem (const char *name, char *buffer, int size);
void SC_OpenLumpNum (int lump, const char *name);
void SC_Close (void);
void SC_SavePos (void);
void SC_RestorePos (void);
BOOL SC_GetString (void);
void SC_MustGetString (void);
void SC_MustGetStringName (const char *name);
BOOL SC_GetNumber (void);
void SC_MustGetNumber (void);
BOOL SC_GetFloat (void);
void SC_MustGetFloat (void);
void SC_UnGet (void);
//boolean SC_Check(void);
BOOL SC_Compare (const char *text);
int SC_MatchString (const char **strings);
int SC_MustMatchString (const char **strings);
void SC_ScriptError (const char *message, const char **args = NULL);

extern char *sc_String;
extern int sc_Number;
extern float sc_Float;
extern int sc_Line;
extern BOOL sc_End;
extern BOOL sc_Crossed;
extern BOOL sc_FileScripts;
extern char *sc_ScriptsDir;

#endif //__SC_MAN_H__

