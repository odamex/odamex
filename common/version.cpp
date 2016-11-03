// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
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
// Source code and protocol versioning
//
//-----------------------------------------------------------------------------

#include "version.h"

#include <map>
#include <string>
#include <sstream>
#include <memory>

#include "c_dispatch.h"

static unsigned int last_revision = 0;

typedef std::map<std::string, std::string> source_files_t;

source_files_t &get_source_files()
{
	static std::auto_ptr<source_files_t> source_files(new source_files_t);
	return *source_files.get();
}


file_version::file_version(const char *uid, const char *id, const char *pp, int l, const char *t, const char *d)
{
	std::stringstream rs(id), ss;
	std::string p = pp;

	size_t e = p.find_last_of("/\\");
	std::string file = p.substr(e == std::string::npos ? 0 : e + 1);

	ss << id << " " << l << " " << t << " " << d << " " << p.substr(e == std::string::npos ? 0 : e + 1);

	get_source_files()[file] = ss.str();
}

const char* GitDescribe()
{
#ifdef GIT_DESCRIBE
	return GIT_DESCRIBE;
#else
	return "unknown";
#endif
}

BEGIN_COMMAND (version)
{
	if (argc == 1)
	{
		// distribution
		Printf(PRINT_HIGH, "Odamex v%s (%s) - %s\n", DOTVERSIONSTR, GitDescribe(), COPYRIGHTSTR);
	}
	else
	{
		// specific file version
		source_files_t::const_iterator it = get_source_files().find(argv[1]);

		if (it == get_source_files().end())
			Printf(PRINT_HIGH, "no such file: %s", argv[1]);
		else
			Printf(PRINT_HIGH, "%s", it->second.c_str());
	}
}
END_COMMAND (version)

BEGIN_COMMAND (listsourcefiles)
{
	for (source_files_t::const_iterator it = get_source_files().begin(); it != get_source_files().end(); ++it)
		Printf(PRINT_HIGH, "%s\n", it->first.c_str());
		
	Printf(PRINT_HIGH, "End of list\n");
}
END_COMMAND(listsourcefiles)

VERSION_CONTROL(version_cpp, "$Id$")



