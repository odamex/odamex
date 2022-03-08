// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1998-2006 by Randy Heit (ZDoom).
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
//	Command-line variables
//
//-----------------------------------------------------------------------------


#include "odamex.h"

#include <cmath>
#include <exception>

#include "cmdlib.h"
#include "c_console.h"
#include "c_dispatch.h"
#include "m_alloc.h"

#include "d_netinf.h"

#include "i_system.h"

bool cvar_t::m_DoNoSet = false;
bool cvar_t::m_UseCallback = false;

// denis - all this class does is delete the cvars during its static destruction
class ad_t {
public:
	cvar_t *&GetCVars() { static cvar_t *CVars; return CVars; }
	ad_t() {}
	~ad_t()
	{
		cvar_t *cvar = GetCVars();
		while (cvar)
		{
			cvar_t *next = cvar->GetNext();
			if(cvar->m_Flags & CVAR_AUTO)
				delete cvar;
			cvar = next;
		}
	}
} ad;

cvar_t* GetFirstCvar(void)
{
	return ad.GetCVars();
}

int cvar_defflags;

cvar_t::cvar_t(const char* var_name, const char* def, const char* help, cvartype_t type,
		DWORD flags, float minval, float maxval)
{
	InitSelf(var_name, def, help, type, flags, NULL, minval, maxval);
}

cvar_t::cvar_t(const char* var_name, const char* def, const char* help, cvartype_t type,
		DWORD flags, void (*callback)(cvar_t &), float minval, float maxval)
{
	InitSelf(var_name, def, help, type, flags, callback, minval, maxval);
}

void cvar_t::InitSelf(const char* var_name, const char* def, const char* help, cvartype_t type,
		DWORD var_flags, void (*callback)(cvar_t &), float minval, float maxval)
{
	cvar_t* dummy;
	cvar_t* var = FindCVar(var_name, &dummy);

	m_Callback = callback;
	m_String = "";
	m_Value = 0.0f;
	m_Flags = 0;
	m_LatchedString = "";
    m_HelpText = help;
    m_Type = type;

	if (var_flags & CVAR_NOENABLEDISABLE)
	{
		m_MinValue = minval;
		m_MaxValue = maxval;
	}
	else
	{
		m_MinValue = 0.0f;
		m_MaxValue = 1.0f;
	}

	if (def)
		m_Default = def;
	else
		m_Default = "";

	if (var_name)
	{
		C_AddTabCommand(var_name);
		m_Name = var_name;
		m_Next = ad.GetCVars();
		ad.GetCVars() = this;
	}
	else
		m_Name = "";

	if (var)
	{
		ForceSet(var->m_String.c_str());
		if (var->m_Flags & CVAR_AUTO)
			delete var;
		else
			var->~cvar_t();
	}
	else if (def)
		ForceSet(def);

	m_Flags = var_flags | CVAR_ISDEFAULT;
}

cvar_t::~cvar_t ()
{
	if (m_Name.length())
	{
		cvar_t *var, *dummy = NULL;

		var = FindCVar (m_Name.c_str(), &dummy);

		if (var == this)
		{
			if (dummy)
				dummy->m_Next = m_Next;
			else
				ad.GetCVars() = m_Next;
		}
	}
}

void cvar_t::ForceSet(const char* valstr)
{
	// [SL] 2013-04-16 - Latched CVARs do not change values until the next map.
	// Servers and single-player games should abide by this behavior but
	// multiplayer clients should just do what the server tells them.
	if (m_Flags & CVAR_LATCH && serverside && 
		(gamestate == GS_LEVEL || gamestate == GS_INTERMISSION))
	{
		m_Flags |= CVAR_MODIFIED;
		if (valstr)
			m_LatchedString = valstr;
		else
			m_LatchedString.clear();
	}
	else
	{
		m_Flags |= CVAR_MODIFIED;

		bool numerical_value = IsRealNum(valstr);
		bool integral_type = m_Type == CVARTYPE_BOOL || m_Type == CVARTYPE_BYTE ||
					m_Type == CVARTYPE_WORD || m_Type == CVARTYPE_INT;
		bool floating_type = m_Type == CVARTYPE_FLOAT;
		float valf = numerical_value ? atof(valstr) : 0.0f;

		// perform rounding to nearest integer for integral types
		if (integral_type)
			valf = floor(valf + 0.5f);

		valf = clamp(valf, m_MinValue, m_MaxValue);

		if (numerical_value || integral_type || floating_type)
		{
			// generate m_String based on the clamped valf value
			char tmp[32];
			sprintf(tmp, "%g", valf);
			m_String = tmp;
		}
		else
		{
			// just set m_String to valstr
			if (valstr)
				m_String = valstr;
			else
				m_String.clear();
		}

		m_Value = valf;

		if (m_UseCallback)
			Callback();

		if (m_Flags & CVAR_USERINFO)
			D_UserInfoChanged(this);
		if (m_Flags & CVAR_SERVERINFO)
			D_SendServerInfoChange(this, m_String.c_str());
	}

	m_Flags &= ~CVAR_ISDEFAULT;
}


void cvar_t::ForceSet(float val)
{
	char string[32];
	sprintf(string, "%g", val);
	ForceSet(string);
}

void cvar_t::Set (const char *val)
{
	if (!(m_Flags & CVAR_NOSET) || !m_DoNoSet)
		ForceSet (val);
}

void cvar_t::Set (float val)
{
	if (!(m_Flags & CVAR_NOSET) || !m_DoNoSet)
		ForceSet (val);
}

void cvar_t::SetDefault (const char *val)
{
	if(val)
		m_Default = val;
	else
		m_Default = "";

	if (m_Flags & CVAR_ISDEFAULT)
	{
		Set (val);
		m_Flags |= CVAR_ISDEFAULT;
	}
}

void cvar_t::RestoreDefault ()
{
	Set(m_Default.c_str());
	m_Flags |= CVAR_ISDEFAULT;
}

//
// cvar_t::Transfer
//
// Copies the value from one cvar to another and then removes the source cvar
//
void cvar_t::Transfer(const char *fromname, const char *toname)
{
	cvar_t *from, *to, *dummy;

	from = FindCVar(fromname, &dummy);
	to = FindCVar(toname, &dummy);

	if (from && to)
	{
		to->ForceSet(from->m_Value);
		to->ForceSet(from->m_String.c_str());

		// remove the old cvar
		cvar_t *cur = ad.GetCVars();
		while (cur->m_Next != from)
			cur = cur->m_Next;

		cur->m_Next = from->m_Next;
	}
}

cvar_t *cvar_t::cvar_set (const char *var_name, const char *val)
{
	cvar_t *var, *dummy;

	if ( (var = FindCVar (var_name, &dummy)) )
		var->Set (val);

	return var;
}

cvar_t *cvar_t::cvar_forceset (const char *var_name, const char *val)
{
	cvar_t *var, *dummy;

	if ( (var = FindCVar (var_name, &dummy)) )
		var->ForceSet (val);

	return var;
}

void cvar_t::EnableNoSet ()
{
	m_DoNoSet = true;
}

void cvar_t::EnableCallbacks ()
{
	m_UseCallback = true;
	cvar_t *cvar = ad.GetCVars();

	while (cvar)
	{
		cvar->Callback ();
		cvar = cvar->m_Next;
	}
}

static int STACK_ARGS sortcvars (const void *a, const void *b)
{
	return strcmp (((*(cvar_t **)a))->name(), ((*(cvar_t **)b))->name());
}

void cvar_t::FilterCompactCVars (TArray<cvar_t *> &cvars, DWORD filter)
{
	cvar_t *cvar = ad.GetCVars();
	while (cvar)
	{
		if (cvar->m_Flags & filter)
			cvars.Push (cvar);
		cvar = cvar->m_Next;
	}
	if (cvars.Size () > 0)
	{
		qsort (&cvars[0], cvars.Size (), sizeof(cvar_t *), sortcvars);
	}
}

void cvar_t::C_WriteCVars (byte **demo_p, DWORD filter, bool compact)
{
	cvar_t *cvar = ad.GetCVars();
	byte *ptr = *demo_p;

	if (compact)
	{
		TArray<cvar_t *> cvars;
		ptr += sprintf ((char *)ptr, "\\\\%ux", (unsigned int)filter);
		FilterCompactCVars (cvars, filter);
		while (cvars.Pop (cvar))
		{
			ptr += sprintf ((char *)ptr, "\\%s", cvar->cstring());
		}
	}
	else
	{
		cvar = ad.GetCVars();
		while (cvar)
		{
			if (cvar->m_Flags & filter)
			{
				ptr += sprintf ((char *)ptr, "\\%s\\%s",
								cvar->name(), cvar->cstring());
			}
			cvar = cvar->m_Next;
		}
	}

	*demo_p = ptr + 1;
}

void cvar_t::C_ReadCVars (byte **demo_p)
{
	char *ptr = *((char **)demo_p);
	char *breakpt;

	if (*ptr++ != '\\')
		return;

	if (*ptr == '\\')
	{	// compact mode
		TArray<cvar_t *> cvars;
		cvar_t *cvar;
		DWORD filter;

		ptr++;
		breakpt = strchr (ptr, '\\');
		*breakpt = 0;
		filter = strtoul (ptr, NULL, 16);
		*breakpt = '\\';
		ptr = breakpt + 1;

		FilterCompactCVars (cvars, filter);

		while (cvars.Pop (cvar))
		{
			breakpt = strchr (ptr, '\\');
			if (breakpt)
				*breakpt = 0;
			cvar->Set (ptr);
			if (breakpt)
			{
				*breakpt = '\\';
				ptr = breakpt + 1;
			}
			else
				break;
		}
	}
	else
	{
		char *value;

		while ( (breakpt = strchr (ptr, '\\')) )
		{
			*breakpt = 0;
			value = breakpt + 1;
			if ( (breakpt = strchr (value, '\\')) )
				*breakpt = 0;

			cvar_set (ptr, value);

			*(value - 1) = '\\';
			if (breakpt)
			{
				*breakpt = '\\';
				ptr = breakpt + 1;
			}
			else
			{
				break;
			}
		}
	}
	*demo_p += strlen (*((char **)demo_p)) + 1;
}

static struct backup_s
{
	std::string name, string;
} CVarBackups[MAX_BACKUPCVARS];

static int numbackedup = 0;

//
// C_BackupCVars
//
// Backup cvars for restoration later. Called before connecting to a server
// or a demo starts playing to save all cvars which could be changed while
// by the server or by playing a demo.
// [SL] bitflag can be used to filter which cvars are set to default.
// The default value for bitflag is 0xFFFFFFFF, which effectively disables
// the filtering.
//
void cvar_t::C_BackupCVars (unsigned int bitflag)
{
	struct backup_s *backup = CVarBackups;
	cvar_t *cvar = ad.GetCVars();

	while (cvar)
	{
		if (cvar->m_Flags & bitflag)
		{
			if (backup == &CVarBackups[MAX_BACKUPCVARS])
				I_Error ("C_BackupDemoCVars: Too many cvars to save (%d)", MAX_BACKUPCVARS);
			backup->name = cvar->m_Name;
			backup->string = cvar->m_String;
			backup++;
		}
		cvar = cvar->m_Next;
	}
	numbackedup = backup - CVarBackups;
}

void cvar_t::C_RestoreCVars (void)
{
	struct backup_s *backup = CVarBackups;
	int i;

	for (i = numbackedup; i; i--, backup++)
	{
		cvar_set (backup->name.c_str(), backup->string.c_str());
		backup->name = backup->string = "";
	}
	numbackedup = 0;
	UnlatchCVars();
}

cvar_t *cvar_t::FindCVar (const char *var_name, cvar_t **prev)
{
	cvar_t *var;

	if (var_name == NULL)
		return NULL;

	var = ad.GetCVars();
	*prev = NULL;
	while (var)
	{
		if (iequals(var->m_Name, var_name))
			break;
		*prev = var;
		var = var->m_Next;
	}
	return var;
}

void cvar_t::UnlatchCVars (void)
{
	cvar_t *var;

	var = ad.GetCVars();
	while (var)
	{
		if (var->m_Flags & (CVAR_MODIFIED | CVAR_LATCH))
		{
			unsigned oldflags = var->m_Flags & ~CVAR_MODIFIED;
			var->m_Flags &= ~(CVAR_LATCH);
			if (var->m_LatchedString.length())
			{
				var->Set (var->m_LatchedString.c_str());
				var->m_LatchedString = "";
			}
			var->m_Flags = oldflags;
		}
		var = var->m_Next;
	}
}

//
// C_SetCvarsToDefault
//
// Initialize cvars to default values after they are created.
// [SL] bitflag can be used to filter which cvars are set to default.
// The default value for bitflag is 0xFFFFFFFF, which effectively disables
// the filtering.
//
void cvar_t::C_SetCVarsToDefaults (unsigned int bitflag)
{
	cvar_t *cvar = ad.GetCVars();

	while (cvar)
	{
		if (cvar->m_Flags & bitflag)
		{
			if (cvar->m_Default.length())
				cvar->Set (cvar->m_Default.c_str());
		}

		cvar = cvar->m_Next;
	}
}

void cvar_t::C_ArchiveCVars (void *f)
{
	cvar_t *cvar = ad.GetCVars();

	while (cvar)
	{
		if ((baseapp == client && (cvar->m_Flags & CVAR_CLIENTARCHIVE))
			|| (baseapp == server && (cvar->m_Flags & CVAR_SERVERARCHIVE)))
		{
			fprintf ((FILE *)f, "// %s\n", cvar->helptext());
			fprintf ((FILE *)f, "set %s %s\n\n", C_QuoteString(cvar->name()).c_str(), C_QuoteString(cvar->cstring()).c_str());
		}
		cvar = cvar->m_Next;
	}
}

void cvar_t::cvarlist()
{
	cvar_t *var = ad.GetCVars();
	int count = 0;

	while (var)
	{
		unsigned flags = var->m_Flags;

		count++;
		Printf (PRINT_HIGH, "%c%c%c%c %s \"%s\"\n",
				flags & CVAR_ARCHIVE ? 'A' :
					flags & CVAR_CLIENTARCHIVE ? 'C' :
					flags & CVAR_SERVERARCHIVE ? 'S' : ' ',
				flags & CVAR_USERINFO ? 'U' : ' ',
				flags & CVAR_SERVERINFO ? 'S' : ' ',
				flags & CVAR_NOSET ? '-' :
					flags & CVAR_LATCH ? 'L' :
					flags & CVAR_UNSETTABLE ? '*' : ' ',
				var->name(),
				var->cstring());
		var = var->m_Next;
	}
	Printf (PRINT_HIGH, "%d cvars\n", count);
}


static std::string C_GetValueString(const cvar_t* var)
{
	if (!var)
		return "unset";

	if (var->flags() & CVAR_NOENABLEDISABLE)
		return '"' + var->str() + '"';

	if (atof(var->cstring()) == 0.0f)
		return "disabled";
	else
		return "enabled";	
}

static std::string C_GetLatchedValueString(const cvar_t* var)
{
	if (!var)
		return "unset";

	if (!(var->flags() & CVAR_LATCH))
		return C_GetValueString(var);

	if (var->flags() & CVAR_NOENABLEDISABLE)
		return '"' + var->latched() + '"';

	if (atof(var->latched()) == 0.0f)
		return "disabled";
	else
		return "enabled";	
}

BEGIN_COMMAND (set)
{
	if (argc != 3)
	{
		Printf (PRINT_HIGH, "usage: set <variable> <value>\n");
	}
	else
	{
		cvar_t *var, *prev;

		var = cvar_t::FindCVar (argv[1], &prev);
		if (!var)
			var = new cvar_t(argv[1], NULL, "", CVARTYPE_NONE,  CVAR_AUTO | CVAR_UNSETTABLE | cvar_defflags);

		if (var->flags() & CVAR_NOSET)
		{
			Printf(PRINT_HIGH, "%s is write protected.\n", argv[1]);
			return;
		}
		else if (multiplayer && baseapp == client && (var->flags() & CVAR_SERVERINFO))
		{
			Printf (PRINT_HIGH, "%s is under server control.\n", argv[1]);
			return;
		}

		// [Russell] - Allow the user to specify either 'enable' and 'disable',
		// this will get converted to either 1 or 0
		// [AM] Introduce zdoom-standard "true" and "false"
		if (!(var->flags() & CVAR_NOENABLEDISABLE))
		{
			if (strcmp("enabled", argv[2]) == 0 ||
			    strcmp("true", argv[2]) == 0)
			{
				argv[2] = (char *)"1";
			}
			else if (strcmp("disabled", argv[2]) == 0 ||
			         strcmp("false", argv[2]) == 0)
			{
				argv[2] = (char *)"0";
			}
		}

		if (var->flags() & CVAR_LATCH)
		{
			// if new value is different from current value and latched value
			if (strcmp(var->cstring(), argv[2]) && strcmp(var->latched(), argv[2]) && gamestate == GS_LEVEL)
				Printf(PRINT_HIGH, "%s will be changed for next game.\n", argv[1]);
		}

		var->Set(argv[2]);
	}
}
END_COMMAND (set)

BEGIN_COMMAND (get)
{
    cvar_t *prev;
	cvar_t *var;

    if (argc < 2)
	{
		Printf (PRINT_HIGH, "usage: get <variable>\n");
        return;
	}

    var = cvar_t::FindCVar (argv[1], &prev);

	if (var)
	{
		// [AM] Determine whose control the cvar is under
		std::string control;
		if (multiplayer && baseapp == client && (var->flags() & CVAR_SERVERINFO))
			control = " (server)";

		// [Russell] - Don't make the user feel inadequate, tell
		// them its either enabled, disabled or its other value
		Printf(PRINT_HIGH, "\"%s\" is %s%s.\n",
				var->name(), C_GetValueString(var).c_str(), control.c_str());

		if (var->flags() & CVAR_LATCH && var->flags() & CVAR_MODIFIED)
			Printf(PRINT_HIGH, "\"%s\" will be changed to %s.\n",
					var->name(), C_GetLatchedValueString(var).c_str());
	}
	else
	{
		Printf(PRINT_HIGH, "\"%s\" is unset.\n", argv[1]);
	}
}
END_COMMAND (get)

BEGIN_COMMAND (toggle)
{
    cvar_t *prev;
	cvar_t *var;

    if (argc < 2)
	{
		Printf (PRINT_HIGH, "usage: toggle <variable>\n");
        return;
	}

    var = cvar_t::FindCVar (argv[1], &prev);

	if (!var)
	{
		Printf(PRINT_HIGH, "\"%s\" is unset.\n", argv[1]);
	}
	else if (var->flags() & CVAR_NOENABLEDISABLE)
	{
		Printf(PRINT_HIGH, "\"%s\" cannot be toggled.\n", argv[1]);
	}
	else
	{
		if (var->flags() & CVAR_LATCH && var->flags() & CVAR_MODIFIED)
			var->Set(!atof(var->latched()));
		else
			var->Set(!var->value());

		// [Russell] - Don't make the user feel inadequate, tell
		// them its either enabled, disabled or its other value
		Printf(PRINT_HIGH, "\"%s\" is %s.\n",
				var->name(), C_GetValueString(var).c_str());

		if (var->flags() & CVAR_LATCH && var->flags() & CVAR_MODIFIED)
			Printf(PRINT_HIGH, "\"%s\" will be changed to %s.\n",
					var->name(), C_GetLatchedValueString(var).c_str());
	}
}
END_COMMAND (toggle)

BEGIN_COMMAND (cvarlist)
{
	cvar_t::cvarlist();
}
END_COMMAND (cvarlist)

BEGIN_COMMAND (help)
{
    cvar_t *prev;
    cvar_t *var;

    if (argc < 2)
    {
		Printf (PRINT_HIGH, "usage: help <variable>\n");
        return;
    }

    var = cvar_t::FindCVar (argv[1], &prev);

    if (!var)
    {
        Printf (PRINT_HIGH, "\"%s\" is unset.\n", argv[1]);
        return;
    }

    Printf(PRINT_HIGH, "Help: %s - %s\n", var->name(), var->helptext());
}
END_COMMAND (help)

BEGIN_COMMAND(errorout)
{
	I_Error("errorout was run from the console");
}
END_COMMAND(errorout)

BEGIN_COMMAND(fatalout)
{
	I_FatalError("fatalout was run from the console");
}
END_COMMAND(fatalout)

#if defined _WIN32

BEGIN_COMMAND(crashout)
{
	std::terminate();
}
END_COMMAND(crashout)

#elif defined UNIX

#include <signal.h>
#include <unistd.h>

BEGIN_COMMAND(crashout)
{
	kill(getpid(), SIGSEGV);
}
END_COMMAND(crashout)

#endif

VERSION_CONTROL (c_cvars_cpp, "$Id$")
