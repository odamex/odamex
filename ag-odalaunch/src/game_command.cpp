// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2006-2010 by The Odamex Team.
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
//	Launch a Game Instance
//
// AUTHORS:
//	 Michael Wood (mwoodj at huntsvegas dot org)
//
//-----------------------------------------------------------------------------

#include <iostream>
#include <string>
#include <cstdlib>

#include "game_command.h"

using namespace std;

void GameCommand::AddParameter(string parameter)
{
	Parameters.push_back(parameter);
}

void GameCommand::AddParameter(string parameter, string value)
{
	Parameters.push_back(parameter);
	Parameters.push_back(value);
}

int GameCommand::Launch()
{
	string cmd;
	list<string>::iterator i;

	// !!!!TEMPORARY!!!!
	cmd = "odamex -waddir ~/Games/doomwads ";

	for(i = Parameters.begin(); i != Parameters.end(); ++i)
		cmd += *i + " ";

	system(cmd.c_str());
	// !!!!TEMPORARY!!!!

	return 0;
}
