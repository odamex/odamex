// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1998-2006 by Randy Heit (ZDoom).
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
//	Command-line variables
//
//-----------------------------------------------------------------------------


#ifndef __C_CVARS_H__
#define __C_CVARS_H__

#include "doomtype.h"
#include "tarray.h"

#include <string>

/*
==========================================================

CVARS (console variables)

==========================================================
*/

#define CVAR_NULL 0		// [deathz0r] no special properties
#define CVAR_ARCHIVE	1	// set to cause it to be saved to vars.rc
#define CVAR_USERINFO	2	// added to userinfo  when changed
#define CVAR_SERVERINFO	4	// [Toke - todo] Changed the meaning of this flag
							// it now describes cvars that clients will be
							// informed if changed
#define CVAR_NOSET		8	// don't allow change from console at all,
							// but can be set from the command line
#define CVAR_LATCH		16	// save changes until server restart
#define CVAR_UNSETTABLE	32	// can unset this var from console
#define CVAR_DEMOSAVE	64	// save the value of this cvar_t in a demo
#define CVAR_MODIFIED	128	// set each time the cvar_t is changed
#define CVAR_ISDEFAULT	256	// is cvar unchanged since creation?
#define CVAR_AUTO		512	// allocated, needs to be freed when destroyed
#define CVAR_NOENABLEDISABLE 1024 // [Nes] No substitution (0=disable, 1=enable)
#define CVAR_CLIENTINFO 2048 // [Russell] client version of CVAR_SERVERINFO
#define CVAR_SERVERARCHIVE 4096 // [Nes] Server version of CVAR_ARCHIVE
#define CVAR_CLIENTARCHIVE 8192 // [Nes] Client version of CVAR_ARCHIVE

// Hints for network code optimization
typedef enum
{
     CVARTYPE_NONE = 0 // Used for no sends
    
    ,CVARTYPE_BOOL
    ,CVARTYPE_BYTE
    ,CVARTYPE_WORD
    ,CVARTYPE_INT
    ,CVARTYPE_FLOAT
    ,CVARTYPE_STRING
    
    ,CVARTYPE_MAX = 255
} cvartype_t;

class cvar_t
{
public:
	cvar_t (const char *name, const char *def, const char *help, cvartype_t, DWORD flags);
	cvar_t (const char *name, const char *def, const char *help, cvartype_t, DWORD flags, void (*callback)(cvar_t &));
	virtual ~cvar_t ();

	const char *cstring() const {return m_String.c_str(); }
	const std::string str() const { return m_String; }
	const char *name() const { return m_Name.c_str(); }
	const char *helptext() const {return m_HelpText.c_str(); }
	const char *latched() const { return m_LatchedString.c_str(); }
	float value() const { return m_Value; }
	operator float () const { return m_Value; }
	unsigned int flags() const { return m_Flags; }

	// return m_Value as an int, rounded to the nearest integer because
	// casting truncates instead of rounding
	int asInt() const { return static_cast<int>(m_Value >= 0.0f ? m_Value + 0.5f : m_Value - 0.5f); }

	inline void Callback () { if (m_Callback) m_Callback (*this); }

	void SetDefault (const char *value);
	void RestoreDefault ();
	void Set (const char *value);
	void Set (float value);
	void ForceSet (const char *value);
	void ForceSet (float value);

	static void Transfer(const char *fromname, const char *toname);

	static void EnableNoSet ();		// enable the honoring of CVAR_NOSET
	static void EnableCallbacks ();

	unsigned int m_Flags;

	// Writes all cvars that could effect demo sync to *demo_p. These are
	// cvars that have either CVAR_SERVERINFO or CVAR_DEMOSAVE set.
	static void C_WriteCVars (byte **demo_p, DWORD filter, bool compact=false);

	// Read all cvars from *demo_p and set them appropriately.
	static void C_ReadCVars (byte **demo_p);

	// Backup cvars for restoration later. Called before connecting to a server
	// or a demo starts playing to save all cvars which could be changed while
	// by the server or by playing a demo.
	// [SL] bitflag can be used to filter which cvars are set to default.
	// The default value for bitflag is 0xFFFFFFFF, which effectively disables
	// the filtering.
	static void C_BackupCVars (unsigned int bitflag = 0xFFFFFFFF);

	// Restore demo cvars. Called after demo playback to restore all cvars
	// that might possibly have been changed during the course of demo playback.
	static void C_RestoreCVars (void);

	// Finds a named cvar
	static cvar_t *FindCVar (const char *var_name, cvar_t **prev);

	// Called from G_InitNew()
	static void UnlatchCVars (void);

	// archive cvars to FILE f
	static void C_ArchiveCVars (void *f);

	// Initialize cvars to default values after they are created.
	// [SL] bitflag can be used to filter which cvars are set to default.
	// The default value for bitflag is 0xFFFFFFFF, which effectively disables
	// the filtering.
	static void C_SetCVarsToDefaults (unsigned int bitflag = 0xFFFFFFFF);

	static bool SetServerVar (const char *name, const char *value);

	static void FilterCompactCVars (TArray<cvar_t *> &cvars, DWORD filter);

	// console variable interaction
	static cvar_t *cvar_set (const char *var_name, const char *value);
	static cvar_t *cvar_forceset (const char *var_name, const char *value);

    // list all console variables
	static void cvarlist();

	cvar_t &operator = (float other) { ForceSet(other); return *this; }
	cvar_t &operator = (const char *other) { ForceSet(other); return *this; }

	cvar_t *GetNext() { return m_Next; }

private:

	cvar_t (const cvar_t &var) {}

	void InitSelf (const char *name, const char *def, const char *help, cvartype_t, DWORD flags, void (*callback)(cvar_t &));
	void (*m_Callback)(cvar_t &);
	cvar_t *m_Next;

    cvartype_t m_Type;

	std::string m_Name, m_String;
	std::string m_HelpText;

	float m_Value;

	std::string m_LatchedString, m_Default;

	static bool m_UseCallback;
	static bool m_DoNoSet;

 protected:

	cvar_t () : m_Flags(0), m_Callback(NULL), m_Next(NULL), m_Type(CVARTYPE_NONE), m_Value(0.f) {}
};

cvar_t* GetFirstCvar(void);

// Maximum number of cvars that can be saved.
#define MAX_BACKUPCVARS 128 

#define BEGIN_CUSTOM_CVAR(name,def,help,type,flags) \
	static void cvarfunc_##name(cvar_t &); \
	cvar_t name (#name, def, help, type, flags, cvarfunc_##name); \
	static void cvarfunc_##name(cvar_t &var)

#define END_CUSTOM_CVAR(name)

#define CUSTOM_CVAR(type,name,def,help,t,flags) \
	static void cvarfunc_##name(F##type##CVar &); \
	F##type##CVar name (#name, def, help, t, flags, cvarfunc_##name); \
	static void cvarfunc_##name(F##type##CVar &self)

#define CVAR(name,def,help,type,flags) \
	cvar_t name (#name, def, help, type, flags);

#define EXTERN_CVAR(name) extern cvar_t name;

#define CVAR_FUNC_DECL(name,def,help,type,flags) \
    extern void cvarfunc_##name(cvar_t &); \
    cvar_t name (#name, def, help, type, flags, cvarfunc_##name);

#define CVAR_FUNC_IMPL(name) \
    EXTERN_CVAR(name) \
    void cvarfunc_##name(cvar_t &var)

#endif //__C_CVARS_H__
