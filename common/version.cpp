/*

	Copyright(C) 2006 - 2012 by The Odamex Team

	This program is free software; you can redistribute it and/or
	modify it under the terms of the GNU General Public License
	as published by the Free Software Foundation; either version 2
	of the License, or (at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
	02111-1307, USA.

	AUTHOR:			Denis Lukianov
	DESCRIPTION:    Source code and protocol versioning

 */

#include "version.h"

#include <map>
#include <string>
#include <sstream>
#include <memory>

#include "c_dispatch.h"

using std::string;

typedef std::map<string, string> source_files_t;

source_files_t &get_source_files()
{
	static std::auto_ptr<source_files_t> source_files(new source_files_t);
	return *source_files.get();
}

unsigned int last_revision = 0;

file_version::file_version(const char *uid, const char *id, const char *pp, int l, const char *t, const char *d)
{
	std::stringstream rs(id), ss;
	string p = pp;

	size_t e = p.find_last_of("/\\");
	string file = p.substr(e == std::string::npos ? 0 : e + 1);

	ss << id << " " << l << " " << t << " " << d << " " << p.substr(e == std::string::npos ? 0 : e + 1);

	get_source_files()[file] = ss.str();

	// file with latest revision indicates the revision of the distribution
	string tmp;
	unsigned int rev = 0;
	rs >> tmp >> tmp;
	if(!rs.eof())
		rs >> rev;
	if(last_revision < rev)
		last_revision = rev;
}

BEGIN_COMMAND (version)
{

	if (argc == 1)
	{
		// distribution
		Printf(PRINT_HIGH, "Odamex v%s r%d - %s\n", DOTVERSIONSTR, last_revision, COPYRIGHTSTR);
	}
	else
	{
		// specific file version
		std::map<std::string, std::string>::iterator i = get_source_files().find(argv[1]);

		if(i == get_source_files().end())
		{
			Printf(PRINT_HIGH, "no such file: %s", argv[1]);
		}
		else
		{
			Printf(PRINT_HIGH, "%s", i->second.c_str());
		}
	}
}
END_COMMAND (version)

BEGIN_COMMAND (listsourcefiles)
{
    std::map<std::string, std::string>::iterator i; 
    
    for (i = get_source_files().begin(); i != get_source_files().end(); ++i)
    {
        Printf(PRINT_HIGH, "%s\n", i->first.c_str());
    }
    
    Printf(PRINT_HIGH, "End of list\n");
}
END_COMMAND(listsourcefiles)

VERSION_CONTROL(version_cpp, "$Id$")



