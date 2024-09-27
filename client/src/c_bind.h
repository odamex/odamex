// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1998-2006 by Randy Heit (ZDoom 1.22).
// Copyright (C) 2006-2024 by The Odamex Team.
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


#pragma once


#include "hashtable.h"
#include "d_event.h"

struct OBinding
{
	const char* Key;
	const char* Bind;
};

class OKeyBindings
{
private:
	typedef OHashTable<int, std::string> BindingTable;

public :
	BindingTable Binds;
	std::string command;

	void SetBindingType(std::string cmd);
	void SetBinds(const OBinding* binds);
	void BindAKey(size_t argc, char** argv, const char* msg);
	void DoBind(const char* key, const char* bind);

	void UnbindKey(const char* key);
	void UnbindACommand(const char* str);
	void UnbindAll();

	void ChangeBinding(const char* str, int newone);	// Stuff used by the customize controls menu

	const std::string &GetBind(int key);			// Returns string bound to given key (NULL if none)
	std::string GetNameKeys(int first, int second);
	int  GetKeysForCommand(const char* cmd, int* first, int* second);
	std::string GetKeynameFromCommand(const char* cmd, bool bTwoEntries = false);

	void ArchiveBindings(FILE* f);
};

void C_BindingsInit();
void C_BindDefaults();

// DoKey now have a binding responder, used to switch between Binds and Automap binds
bool C_DoKey(event_t* ev, OKeyBindings* binds, OKeyBindings* doublebinds);

void C_ReleaseKeys();


extern OKeyBindings Bindings, DoubleBindings, AutomapBindings, NetDemoBindings;
