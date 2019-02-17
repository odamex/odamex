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

#define MAX_STRING_SIZE 4096

class FScriptParser
{
public:
	FScriptParser();
	~FScriptParser();

	int Number;
	char *String;
	float Float;
	bool Crossed;

	void Open(const char *lumpname);
	void OpenFile(const char *filename);
	void OpenMem(const char *name, char *buffer, int size);
	void OpenLumpNum(int lump, const char *name);
	void Close(void);

	void SavePos(void);
	void RestorePos(void);

	bool GetString(void);
	void MustGetString(void);
	void MustGetStringName(const char *name);

	bool Compare(const char *text);
	int MatchString(const char **strings);
	int MustMatchString(const char **strings);

	bool GetNumber(void);
	bool CheckNumber(void);
	void MustGetNumber(void);

	bool GetFloat(void);
	void MustGetFloat(void);

	void UnGet(void);

	void ScriptError(const char *message, const char **args = NULL);

protected:

	void PrepareScript(void);
	void CheckOpen(void);

	bool End;
	int Line;

	std::string ScriptName;
	char *ScriptBuffer;
	char *ScriptPtr;
	char *ScriptEndPtr;
	char StringBuffer[MAX_STRING_SIZE];
	bool ScriptOpen;
	int ScriptSize;
	bool AlreadyGot;
	bool FreeScript;
	char *SavedScriptPtr;
	int SavedScriptLine;
};

extern FScriptParser sc;

#endif //__SC_MAN_H__

