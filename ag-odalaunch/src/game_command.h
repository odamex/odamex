// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2006-2012 by The Odamex Team.
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

#ifndef _GAME_COMMAND_H
#define _GAME_COMMAND_H

#include <list>

/**
agOdalaunch namespace.

All code for the ag-odalaunch launcher is contained within the agOdalaunch
namespace.
*/
namespace agOdalaunch {

class GameCommand
{
public:
	void AddParameter(const std::string &parameter);
	void AddParameter(const std::string &parameter, const std::string &value);

	int Launch();

private:
	static void *Cleanup(void *vpPID);
	void         Cleanup(AG_ProcessID pid);

	std::list<std::string> Parameters;
};

} // namespace

#endif
