// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1998-2006 by Randy Heit (ZDoom).
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
//	STATS
//
//-----------------------------------------------------------------------------


#ifndef __STATS_H__
#define __STATS_H__

#include <vector>
#include <string>
#include <algorithm>

class FStat
{
public:
	FStat (const char *cname);

	virtual ~FStat ();

	void clock();
	void unclock();
	void reset();

	const char *getname();

	static void dumpstat();
	static void dumpstat(std::string which);
	void dump();

private:

	QWORD last_clock, last_elapsed;
	std::string name;
	static std::vector<FStat*> stats;
};

#define BEGIN_STAT(n) \
	static class Stat_##n : public FStat { \
		public: \
			Stat_##n () : FStat (#n) {} \
} Stat_var_##n; Stat_var_##n.clock();

#define END_STAT(n) Stat_var_##n.unclock();

#endif //__STATS_H__


