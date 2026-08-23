// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1998-2006 by Randy Heit (ZDoom).
// Copyright (C) 2006-2026 by The Odamex Team.
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
//	Argument processing (?)
//
//-----------------------------------------------------------------------------


#include "odamex.h"

#include "c_cvars.h"
#include "c_dispatch.h"
#include "sv_main.h"

void C_RunCVarScriptHook(const cvar_t& var, bool resend)
{
	if (!var.str().empty())
	{
		AddCommandString(var.str());
		// Make sure any cvar modifications from the script make it to clients
		if (resend)
		{
			cvar_t::UnlatchCVars();
			SV_ServerSettingChange(true);
		}
	}
}

BEGIN_COMMAND (rand)
{
	if (argc < 3)
	{
		PrintFmt("rand - returns a random number for use in scripting\n");
		PrintFmt("Usage: rand <type> <max>\n");
		PrintFmt("       rand <type> <min> <max>\n");
		PrintFmt("\n");
		PrintFmt("Result is placed in the result_int/result_float variable\n");
		PrintFmt("Minimum is zero if not specified\n");
		return;
	}

	const bool useMin = argc > 3;

	if (strcmp(argv[1], "int") == 0)
	{
		EXTERN_CVAR(result_int)
		int32_t min;
		int32_t max;
		if (useMin)
		{
			const auto minopt = ParseNum<int32_t>(argv[2]);
			const auto maxopt = ParseNum<int32_t>(argv[3]);

			if (not minopt.has_value())
			{
				PrintFmt("Minimum ({}) is not a valid integer", argv[2]);
				return;
			}

			if (not maxopt.has_value())
			{
				PrintFmt("Maxmimum ({}) is not a valid integer", argv[3]);
				return;
			}

			min = *minopt;
			max = *maxopt;
		}
		else
		{
			const auto maxopt = ParseNum<int32_t>(argv[2]);

			if (not maxopt.has_value())
			{
				PrintFmt("Maxmimum ({}) is not a valid integer", argv[2]);
				return;
			}

			min = 0;
			max = *maxopt;
		}

		if (max < min)
		{
			PrintFmt("Maximum must be greater than minimum");
			return;
		}

    	const auto range = static_cast<uint32_t>(max - min);
    	result_int = min + static_cast<int32_t>(M_RandomInt(range));
	}
	else if (strcmp(argv[1], "float") == 0)
	{
		EXTERN_CVAR(result_float)
		// TODO: when merging to protobreak, implement this with ParseNum<float>
		// I can't be bothered to do the proper error checking with atof right now
		PrintFmt("Sorry, rand is not yet implemented for floats");
		return;
	}
	else
	{
		PrintFmt("Invalid type \"{}\"", argv[1]);
		PrintFmt("Valid types are 'int', 'float'");
		return;
	}
}
END_COMMAND (rand)

VERSION_CONTROL (c_dispatch_cpp, "$Id$")