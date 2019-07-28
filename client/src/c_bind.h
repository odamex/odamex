// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1998-2006 by Randy Heit (ZDoom 1.22).
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
//	C_BIND
//
//-----------------------------------------------------------------------------


#ifndef __C_BINDINGS_H__
#define __C_BINDINGS_H__

#include <string>

#include "doomtype.h"
#include "doomkeys.h"
#include "d_event.h"
#include <stdio.h>

BOOL C_DoKey (event_t *ev);
void C_ArchiveBindings (FILE *f);

void BIND_Init(void);

// Stuff used by the customize controls menu

std::string C_NameKeys (int first, int second);
void C_ChangeBinding (const char *str, int newone);

// Returns string bound to given key (NULL if none)
const char *C_GetBinding (int key);

void C_ReleaseKeys();

struct FBinding
{
	const char *Key;
	const char *Bind;
};

class FKeyBindings
{
	std::string command;

public:
	std::string Binds[NUM_KEYS];
	void SetBindingType(std::string cmd);
	void SetBinds(const FBinding *binds);
	void SetBind(int key, char *value);
	void DoBind(const char *key, const char *bind);
	void BindAKey(size_t argc, char **argv, char *msg);

	void UnbindKey(const char *key);
	void UnbindACommand(const char *str);
	void UnbindAll(void);
	size_t GetLength(void);
	void ArchiveBindings(FILE *f);

	const char* GetBinding(int key);
	int  GetKeysForCommand(const char *cmd, int *first, int *second);
	void ChangeBinding(const char *str, int newone);
	std::string GetBind(int key);
	std::string GetKeyStringsFromCommand(char *cmd, bool bTwoEntries=false);
};

void C_BindDefaults(void);

extern FKeyBindings Bindings;
extern FKeyBindings DoubleBindings;
extern FKeyBindings NetDemoBindings;
extern FKeyBindings AutomapBindings;

#endif //__C_BINDINGS_H__

