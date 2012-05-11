// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
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
//  Command-line arguments
//
//-----------------------------------------------------------------------------


#ifndef __M_ARGV_H__
#define __M_ARGV_H__

#include "dobject.h"
#include "doomtype.h"

#include <string>
#include <vector>

//
// MISC
//
class DArgs : public DObject
{
	DECLARE_CLASS (DArgs, DObject)
public:
	DArgs ();
	DArgs (const DArgs &args);
	DArgs (unsigned int argc, char **argv);
	DArgs (const char *cmdline);
	~DArgs ();

	DArgs &operator= (const DArgs &other);
	const char *operator[] (size_t n);

	void AppendArg (const char *arg);
	void SetArgs (unsigned int argc, char **argv);
	void SetArgs(const char *cmdline);
	DArgs GatherFiles (const char *param, const char *extension, bool acceptNoExt) const;
	void SetArg (unsigned int argnum, const char *arg);


	// Returns the position of the given parameter
	// in the arg list (0 if not found).
	size_t CheckParm (const char *check) const;
	const char *CheckValue (const char *check) const;
	const char *GetArg (size_t arg) const;
	const std::vector<std::string> GetArgList (size_t start) const;
	size_t NumArgs () const;
	void FlushArgs ();

private:

	std::vector<std::string> args;

	void CopyArgs (unsigned int argc, char **argv);

};

extern DArgs Args;

void M_FindResponseFile(void);

extern bool DefaultsLoaded; 

#endif //__M_ARGV_H__

