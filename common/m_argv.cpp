// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 2006-2020 by The Odamex Team.
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


#include "odamex.h"

#include <algorithm>

#include "cmdlib.h"
#include "m_fileio.h"
#include "m_argv.h"

IMPLEMENT_CLASS (DArgs, DObject)

DArgs::DArgs ()
{
}

DArgs::DArgs (unsigned int argc, char **argv)
{
	if(argv)
		CopyArgs(argc, argv);
}

DArgs::DArgs (const DArgs &other) : args(other.args)
{

}

DArgs::DArgs (const char *cmdline)
{
	SetArgs(cmdline);
}

DArgs::~DArgs ()
{
	FlushArgs ();
}

DArgs &DArgs::operator= (const DArgs &other)
{
	args = other.args;
	return *this;
}

const char *DArgs::operator[] (size_t n)
{
	return GetArg(n);
}

void DArgs::SetArgs (unsigned argc, char **argv)
{
	CopyArgs(argc, argv);
}

void DArgs::CopyArgs (unsigned argc, char **argv)
{
	args.clear();

	if(!argv || !argc)
		return;

	args.resize(argc);

	for (unsigned i = 0; i < argc; i++)
		if(argv[i])
			args[i] = argv[i];
}

void DArgs::FlushArgs ()
{
	args.clear();
}

//
// CheckParm
// Checks for the given parameter
// in the program's command line arguments.
// Returns the argument number (1 to argc-1) or 0 if not present
//
size_t DArgs::CheckParm (const char *check) const
{
	if(!check)
		return 0;

	for (size_t i = 1, n = args.size(); i < n; i++)
		if (!stricmp (check, args[i].c_str()))
			return i;

	return 0;
}

const char *DArgs::CheckValue (const char *check) const
{
	if(!check)
		return 0;

	size_t i = CheckParm (check);

	if (i > 0 && i < args.size() - 1)
		return args[i + 1].c_str();
	else
		return NULL;
}

const char *DArgs::GetArg (size_t arg) const
{
	if (arg < args.size())
		return args[arg].c_str();
	else
		return NULL;
}

const std::vector<std::string> DArgs::GetArgList (size_t start) const
{
	std::vector<std::string> out;

	if(start < args.size())
	{
		out.resize(args.size() - start);
		std::copy(args.begin() + start, args.end(), out.begin());
	}

	return out;
}

size_t DArgs::NumArgs () const
{
	return args.size();
}

void DArgs::AppendArg (const char *arg)
{
	if(arg)
		args.push_back(arg);
}

//
// IsParam
//
// Helper function to return if the given argument number i is a parameter
// 
static bool IsParam(const std::vector<std::string>& args, size_t i)
{
	return i < args.size() && (args[i][0] == '-' || args[i][0] == '+');
}

//
// FindNextParamArg
//
// Returns the next argument number for a command line parameter starting
// from argument number i.
//
static size_t FindNextParamArg(const char* param, const std::vector<std::string>& args, size_t i)
{
	while (i < args.size())
	{
		if (!IsParam(args, i))
			return i;

		// matches param, return first argument for this param
		if (stricmp(param, args[i].c_str()) == 0)
		{
			i++;
			continue;
		}

		// skip over any params that don't match and their arguments
		for (i++; i < args.size() && !IsParam(args, i); i++)
			;
	}

	return args.size();
}

//
// DArgs::GatherFiles
//
// Collects all of the arguments entered after param.
//
// [AM] This used to be smarter but since we now properly deal with file
//      types higher up the stack we don't need to filter by extension
//      anymore.
//
DArgs DArgs::GatherFiles(const char* param) const
{
	DArgs out;

	if (param[0] != '-' && param[0] != '+')
		return out;

	for (size_t i = 1; i < args.size(); i++)
	{
		i = FindNextParamArg(param, args, i);
		if (i < args.size())
		{
			out.AppendArg(args[i].c_str());
		}
	}

	return out;
}

void DArgs::SetArgs(const char *cmdline)
{
	char *outputline, *q;
	char **outputargv;
	int outputargc;

	args.clear();

	if(!cmdline)
		return;

	while (*cmdline && isspace(*cmdline))
		cmdline++;

	if (!*cmdline)
		return;

	outputline = (char *)Malloc((strlen(cmdline) + 1) * sizeof(char));
	outputargv = (char **)Malloc(((strlen(cmdline) + 1) / 2) * sizeof(char *));

	const char *p = cmdline;
	q = outputline;
	outputargv[0] = NULL;
	outputargc = 1;

	while (*p)
	{
		int quote;

		while (*p && isspace(*p))
			p++;

		if (!*p)
			break;

		outputargv[outputargc] = q;
		outputargc++;
		quote = 0;

		while (*p)
		{
			if (!quote && isspace(*p))
				break;

			if (*p == '\\' || *p == '\"')
			{
				int slashes = 0, quotes = 0;

				while (*p == '\\')
				{
					slashes++;
					p++;
				}

				while (*p == '"')
				{
					quotes++;
					p++;
				}

				if (!quotes)
					while (slashes--)
						*q++ = '\\';
				else
				{
					while (slashes >= 2)
					{
						slashes -= 2;
						*q++ = '\\';
					}

					if (slashes)
					{
						quotes--;
						*q = '\"';
					}

					if (quotes > 0)
					{
						if (!quote)
						{
							quotes--;
							quote = 1;
						}

						for (int i = 3; i <= quotes + 1; i += 3)
							*q++ = '\"';

						quote = (quotes % 3 == 0);
					}
				}
			}
			else
				*q++ = *p++;
		}

		*q++ = '\0';
	}

	CopyArgs(outputargc, outputargv);

	M_Free(outputargv);
	M_Free(outputline);

	return;
}

//
// bond - PROC M_FindResponseFile
//

static long ParseCommandLine (const char *args, int *argc, char **argv);

void M_FindResponseFile (void)
{
	for (size_t i = 1; i < Args.NumArgs(); i++)
	{
		if (Args.GetArg(i)[0] == '@')
		{
			char	**argv;
			char	*file;
			int		argc;
			int		argcinresp;
			FILE	*handle;
			int 	size;
			long	argsize;
			size_t 	index;

			// READ THE RESPONSE FILE INTO MEMORY
			handle = fopen (Args.GetArg(i) + 1,"rb");
			if (!handle)
			{ // [RH] Make this a warning, not an error.
				Printf (PRINT_WARNING,"No such response file (%s)!", Args.GetArg(i) + 1);
				continue;
			}

			Printf (PRINT_HIGH,"Found response file %s!\n", Args.GetArg(i) + 1);
			fseek (handle, 0, SEEK_END);
			size = ftell (handle);
			fseek (handle, 0, SEEK_SET);
			file = new char[size+1];
			size_t readlen = fread (file, size, 1, handle);
			if (readlen < 1)
			{
				Printf (PRINT_HIGH,"Failed to read response file %s.\n", Args.GetArg(i) + 1);
			}
			file[size] = 0;
			fclose (handle);

			argsize = ParseCommandLine (file, &argcinresp, NULL);
			argc = argcinresp + Args.NumArgs() - 1;

			if (argc != 0)
			{
				argv = (char **)Malloc (argc*sizeof(char *) + argsize);
				argv[i] = (char *)argv + argc*sizeof(char *);
				ParseCommandLine (file, NULL, argv+i);

				for (index = 0; index < i; ++index)
					argv[index] = (char*)Args.GetArg (index);

				for (index = i + 1, i += argcinresp; index < Args.NumArgs (); ++index)
					argv[i++] = (char*)Args.GetArg (index);

				DArgs newargs (i, argv);
				Args = newargs;
				
				M_Free(argv);
			}

			delete[] file;
		
			// DISPLAY ARGS
			Printf("%zu command-line args:\n", Args.NumArgs());
			for (size_t k = 1; k < Args.NumArgs (); k++)
				Printf (PRINT_HIGH,"%s\n", Args.GetArg (k));

			break;
		}
	}
}

// ParseCommandLine
//
// bond - This is just like the version in c_dispatch.cpp, except it does not
// do cvar expansion.

static long ParseCommandLine (const char *args, int *argc, char **argv)
{
	int count;
	char *buffplace;

	count = 0;
	buffplace = NULL;
	if (argv != NULL)
	{
		buffplace = argv[0];
	}

	for (;;)
	{
		while (*args <= ' ' && *args)
		{ // skip white space
			args++;
		}
		if (*args == 0)
		{
			break;
		}
		else if (*args == '\"')
		{ // read quoted string
			char stuff;
			if (argv != NULL)
			{
				argv[count] = buffplace;
			}
			count++;
			args++;
			do
			{
				stuff = *args++;
				if (stuff == '\\' && *args == '\"')
				{
					stuff = '\"', args++;
				}
				else if (stuff == '\"')
				{
					stuff = 0;
				}
				else if (stuff == 0)
				{
					args--;
				}
				if (argv != NULL)
				{
					*buffplace = stuff;
				}
				buffplace++;
			} while (stuff);
		}
		else
		{ // read unquoted string
			const char *start = args++, *end;

			while (*args && *args > ' ' && *args != '\"')
				args++;
			end = args;
			if (argv != NULL)
			{
				argv[count] = buffplace;
				while (start < end)
					*buffplace++ = *start++;
				*buffplace++ = 0;
			}
			else
			{
				buffplace += end - start + 1;
			}
			count++;
		}
	}
	if (argc != NULL)
	{
		*argc = count;
	}
	return (long)(buffplace - (char *)0);
}


//
// M_GetParmValue
//
// Easy way of retrieving an integer parameter value.
//
int M_GetParmValue(const char* name)
{
	const char* valuestr = Args.CheckValue(name);
	if (valuestr)
		return atoi(valuestr);
	return 0;
}

VERSION_CONTROL (m_argv_cpp, "$Id$")
