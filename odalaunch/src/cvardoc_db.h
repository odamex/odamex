// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
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
//  In-memory database of console variable documentation.
//
//-----------------------------------------------------------------------------

#pragma once

#include "odalaunch.h"

#include <string>
#include <unordered_map>
#include <vector>

#include <wx/string.h>

// Behavioral flags a cvar can carry, mirroring the names emitted in the
// cvardocjson "flags" array.
// Stored as a bitset for cheap checks.
enum CvarDocFlag_t
{
	CVARDOC_USERINFO        = 1 << 0,
	CVARDOC_SERVERINFO      = 1 << 1,
	CVARDOC_NOSET           = 1 << 2,
	CVARDOC_LATCH           = 1 << 3,
	CVARDOC_UNSETTABLE      = 1 << 4,
	CVARDOC_NOENABLEDISABLE = 1 << 5,
	CVARDOC_SERVERARCHIVE   = 1 << 6,
	CVARDOC_CLIENTARCHIVE   = 1 << 7,
};

// Documentation for a single console variable.
struct CvarDoc_t
{
	std::string Name;
	std::string Type;     // "bool", "number", "float" or "string"
	std::string HelpText;
	std::string DefaultValue; // default, stringified for uniform display

	bool        HasMin;
	bool        HasMax;
	double      Min;
	double      Max;

	unsigned    Flags;    // bitfield of CvarDocFlag_t

	CvarDoc_t()
	    : HasMin(false), HasMax(false), Min(0.0), Max(0.0), Flags(0)
	{
	}
};

// Holds the parsed cvar documentation and document-level metadata.
class CvarDocDb
{
public:
	CvarDocDb() : m_Loaded(false), m_SchemaVersion(0) {}

	// Loads the given JSON documents, replacing any previously loaded data.
	// The documents are merged in order into the deduplicated union of their
	// cvars; when the same cvar (or the document metadata) appears in more than
	// one, the earlier path wins. Empty or unreadable paths are skipped, so it
	// is fine to pass a doc that does not exist on this system. Returns true if
	// at least one document loaded.
	bool LoadFromFiles(const std::vector<wxString>& Paths);

	// Looks up a cvar by name. Returns nullptr if it is not in the database.
	const CvarDoc_t* Find(const std::string& Name) const;

	bool IsLoaded() const { return m_Loaded; }
	size_t Size() const { return m_Cvars.size(); }

	const wxString& OdamexVersion() const { return m_OdamexVersion; }
	const wxString& CommitHash() const { return m_CommitHash; }
	const wxString& BranchName() const { return m_BranchName; }

private:
	bool ParseDocument(const wxString& Path);

	std::unordered_map<std::string, CvarDoc_t> m_Cvars;

	bool     m_Loaded;
	int      m_SchemaVersion;
	wxString m_OdamexVersion;
	wxString m_CommitHash;
	wxString m_BranchName;
};

// Returns a reference to the CvarDocDb singleton.
CvarDocDb& GetCvarDb();

// Resolves the location of the client cvar doc "odamex_cvardoc.json": the
// configured Odamex directory first, then the launcher's own install/data
// directories. Returns the first candidate that exists, or an empty string.
wxString OdaResolveCvarDocPath();

// As above, but for the server cvar doc "odasrv_cvardoc.json".
wxString OdaResolveSrvCvarDocPath();
