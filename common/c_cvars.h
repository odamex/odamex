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
//	Command-line variables
//
//-----------------------------------------------------------------------------

#pragma once

//Uncomment to allow for latency simulation - see sv_latency in sv_cvarlist.cpp
//Note: When compiling for linux you will have link against pthread manually
//#define SIMULATE_LATENCY

#include <cfloat>
#include <cmath>
#include <concepts>
#include <string>
#include <stdint.h>
#include <vector>
#include <unordered_map>

using byte = uint8_t;

/*
==========================================================

CVARS (console variables)

==========================================================
*/

/**
 * [deathz0r] no special properties.
 */
#define CVAR_NULL 0

/**
 * Added to userinfo when changed.
 */
#define CVAR_USERINFO BIT(1)

/**
 * [Toke - todo] Changed the meaning of this flag, it now describes cvars that
 *               clients will be informed if changed.
 */
#define CVAR_SERVERINFO BIT(2)

/**
 * Don't allow change from console at all, but can be set from the command line.
 */
#define CVAR_NOSET BIT(3)

/**
 * Save changes until server restart.
 */
#define CVAR_LATCH BIT(4)

/**
 * Can unset this var from console.
 */
#define CVAR_UNSETTABLE BIT(5)

/**
 * Set each time the cvar_t is changed
 */
#define CVAR_MODIFIED BIT(7)

/**
 * Is cvar unchanged since creation?
 */
#define CVAR_ISDEFAULT BIT(8)

/**
 * Allocated, needs to be freed when destroyed.
 */
#define CVAR_AUTO BIT(9)

/**
 * [Nes] No substitution (0=disable, 1=enable)
 */
#define CVAR_NOENABLEDISABLE BIT(10)

/**
 * [Nes] Server version of CVAR_ARCHIVE
 */
#define CVAR_SERVERARCHIVE BIT(12)

/**
 * [Nes] Client version of CVAR_ARCHIVE
 */
#define CVAR_CLIENTARCHIVE BIT(13)

/**
 * [SL] CVAR_ARCHIVE enables both CVAR_CLIENTARCHIVE & CVAR_SERVERARCHIVE
 */
#define CVAR_ARCHIVE (CVAR_CLIENTARCHIVE | CVAR_SERVERARCHIVE)

// Hints for network code optimization
enum struct cvartype_t
{
	NONE = 0, // Used for no sends

    BOOL,
    BYTE,
    WORD,
    INT,
    FLOAT,
    STRING,

    MAX = 255,
};
// TODO: make cvar_t an abstract class with subclasses for string/float/integral type
class cvar_t
{
public:
	cvar_t(const char* name, const char* def, const char* help, cvartype_t,
			uint32_t flags, float minval = -FLT_MAX, float maxval = FLT_MAX);
	cvar_t(const char* name, const char* def, const char* help, cvartype_t,
			uint32_t flags, void (*callback)(cvar_t &), float minval = -FLT_MAX, float maxval = FLT_MAX);
	virtual ~cvar_t ();

	[[nodiscard]] const char *cstring() const {return m_String.c_str(); }
	[[nodiscard]] const std::string& str() const { return m_String; }
	[[nodiscard]] const std::string& name() const { return m_Name; }
	[[nodiscard]] const char *helptext() const {return m_HelpText.c_str(); }
	[[nodiscard]] const char *latched() const { return m_LatchedString.c_str(); }
	[[nodiscard]] float value() const { return m_Value; }
	[[nodiscard]] operator float () const { return m_Value; }
	[[nodiscard]] operator const std::string& () const { return m_String; }
	[[nodiscard]] unsigned int flags() const { return m_Flags; }
    [[nodiscard]] cvartype_t type() const { return m_Type; }
	[[nodiscard]] const std::string& getDefault() const { return m_Default; }
	[[nodiscard]] float getMinValue() const { return m_MinValue; }
	[[nodiscard]] float getMaxValue() const { return m_MaxValue; }

	// return m_Value as an int, rounded to the nearest integer because
	// casting truncates instead of rounding
	[[nodiscard]] int asInt() const { return static_cast<int>(std::round(m_Value)); }
	[[nodiscard]] bool asBool() const { return m_Value != 0; }

	template <typename E>
		requires std::is_enum_v<E>
	[[nodiscard]] E asEnum() const { return static_cast<E>(asInt()); }

	template <typename E>
		requires std::is_enum_v<E> || std::is_integral_v<E>
	[[nodiscard]] auto operator<=>(E e) const
	{
		return static_cast<E>(asInt()) <=> e;
	}


	template <typename E>
		requires std::is_enum_v<E> || std::is_integral_v<E>
	[[nodiscard]] bool operator==(E e) const
	{
		return static_cast<E>(asInt()) == e;
	}

	[[nodiscard]] explicit operator bool () const { return asBool(); }

	void Callback (){ if (m_Callback) m_Callback (*this); }

	void SetDefault (const char *value);
	void RestoreDefault ();
	void Set (std::string_view value);
	void Set (float value);
	void ForceSet (std::string_view value);
	void ForceSet (float value);

	static void EnableNoSet ();		// enable the honoring of CVAR_NOSET
	static void EnableCallbacks ();

	unsigned int m_Flags;

	// Writes all cvars that could effect demo sync to *demo_p. These are
	// cvars that have either CVAR_SERVERINFO or CVAR_DEMOSAVE set.
	static void C_WriteCVars (byte **demo_p, uint32_t filter, size_t array_size, bool compact=false);

	// Read all cvars from *demo_p and set them appropriately.
	static void C_ReadCVars (byte **demo_p);

	// Backup cvars for restoration later. Called before connecting to a server
	// or a demo starts playing to save all cvars which could be changed while
	// by the server or by playing a demo.
	// [SL] bitflag can be used to filter which cvars are set to default.
	// The default value for bitflag is 0xFFFFFFFF, which effectively disables
	// the filtering.
	static void C_BackupCVars(unsigned int bitflag = 0xFFFFFFFF);

	// Restore demo cvars. Called after demo playback to restore all cvars
	// that might possibly have been changed during the course of demo playback.
	static void C_RestoreCVars();

	// Finds a named cvar
	static cvar_t *FindCVar (std::string_view var_name) {
		cvar_t* dummy;
		return FindCVar(var_name, &dummy);
	};

	// Called from G_InitNew()
	static void UnlatchCVars();

	// archive cvars to FILE f
	static void C_ArchiveCVars (void *f);

	// Initialize cvars to default values after they are created.
	// [SL] bitflag can be used to filter which cvars are set to default.
	// The default value for bitflag is 0xFFFFFFFF, which effectively disables
	// the filtering.
	static void C_SetCVarsToDefaults (unsigned int bitflag = 0xFFFFFFFF);

	static bool SetServerVar (std::string_view name, const char *value);

	static void FilterCompactCVars (std::vector<cvar_t *> &cvars, uint32_t filter);

	// console variable interaction
	static cvar_t *cvar_set (const char *var_name, const char *value);
	static cvar_t *cvar_forceset (const char *var_name, const char *value);

    // list all console variables
	static void cvarlist();

	cvar_t &operator = (float other) { ForceSet(other); return *this; }
	cvar_t &operator = (const char *other) { ForceSet(other); return *this; }

	cvar_t *GetNext() { return m_Next; }

	cvar_t(const cvar_t &var) = delete;
	cvar_t(cvar_t&&) = delete;

private:
	static cvar_t *FindCVar (std::string_view var_name, cvar_t** prev);

	void InitSelf(const char* name, const char* def, const char* help, cvartype_t,
				uint32_t flags, void (*callback)(cvar_t &), float minval = -FLT_MAX, float maxval = FLT_MAX);

	void (*m_Callback)(cvar_t &);
	cvar_t *m_Next;

    cvartype_t m_Type;

	std::string m_Name, m_String;
	std::string m_HelpText;

	float m_Value;
	float m_MinValue, m_MaxValue;

	std::string m_LatchedString, m_Default;

	static inline bool m_UseCallback = false;
	static inline bool m_DoNoSet = false;

protected:

	cvar_t () :
			m_Flags(0), m_Callback(NULL), m_Next(NULL), m_Type(cvartype_t::NONE), m_Value(0.f),
			m_MinValue(-FLT_MAX), m_MaxValue(FLT_MAX)
	 { }

	class cvarlist_t
	{
	public:
		void add(cvar_t& cvar)
		{
			m_cvars.push_back(&cvar);
			m_cvarmap.emplace(cvar.name(), &cvar);
		}

		void remove(const std::string& name)
		{
			std::erase_if(m_cvars, [&](cvar_t* cvar){ return cvar->name() == name; });
			m_cvarmap.erase(name);
		}

		cvar_t* find(const std::string& name)
		{
			auto it = m_cvarmap.find(name);
			return it != m_cvarmap.end() ? it->second : nullptr;
		}

		auto begin() { return m_cvars.begin(); }
		auto end() { return m_cvars.end(); }
	private:
		std::vector<cvar_t*> m_cvars;
		std::unordered_map<std::string, cvar_t*> m_cvarmap;

	};
};

cvar_t* GetFirstCvar();

class cvarbase_t
{
public:
	virtual ~cvarbase_t() = 0;
	cvarbase_t(cvarbase_t&&) = delete;
	cvarbase_t(const cvarbase_t&) = delete;
	cvarbase_t& operator=(cvarbase_t&&) = delete;
	cvarbase_t& operator=(const cvarbase_t&) = delete;
private:

protected:
	// static cvarlist_t list;

};

template <typename T>
class cvarderived_t : public cvarbase_t {};

template <std::integral T>
class cvarderived_t<T> : public cvarbase_t
{

};

template <std::floating_point T>
class cvarderived_t<T> : public cvarbase_t
{

};

template <>
class cvarderived_t<bool> : public cvarbase_t
{};


template <>
class cvarderived_t<std::string> : public cvarbase_t
{};

// alias
template <>
class cvarderived_t<cvarbase_t> : public cvarbase_t
{};



// Maximum number of cvars that can be saved.
#define MAX_BACKUPCVARS 512

#define CVAR(name,def,help,type,flags) \
	cvar_t name(#name, def, help, type, flags);

#define CVAR_RANGE(name,def,help,type,flags,minval,maxval) \
	cvar_t name(#name, def, help, type, flags, minval, maxval);

#define EXTERN_CVAR(name) extern cvar_t name;

#define CVAR_FUNC_DECL(name,def,help,type,flags) \
    extern void cvarfunc_##name(cvar_t &); \
    cvar_t name (#name, def, help, type, flags, cvarfunc_##name);

#define CVAR_RANGE_FUNC_DECL(name,def,help,type,flags,minval,maxval) \
    extern void cvarfunc_##name(cvar_t &); \
    cvar_t name (#name, def, help, type, flags, cvarfunc_##name, minval, maxval);

#define CVAR_FUNC_IMPL(name) \
    EXTERN_CVAR(name) \
    void cvarfunc_##name(cvar_t &var)
