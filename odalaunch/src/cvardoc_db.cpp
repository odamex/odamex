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

#include "cvardoc_db.h"

#include <memory>

#include <wx/log.h>

#include <json/json.h>

#include "cvardoc_data.h"

// The expected schema_version for the cvardoc document we know how to parse.
static const int CVARDOC_SCHEMA_VERSION = 1;

CvarDocDb& GetCvarDb()
{
	static CvarDocDb db;
	return db;
}

// Maps a single flag name from the JSON "flags" array to its bit.
static unsigned FlagFromName(const std::string& name)
{
	if (name == "USERINFO")
		return CVARDOC_USERINFO;
	if (name == "SERVERINFO")
		return CVARDOC_SERVERINFO;
	if (name == "NOSET")
		return CVARDOC_NOSET;
	if (name == "LATCH")
		return CVARDOC_LATCH;
	if (name == "UNSETTABLE")
		return CVARDOC_UNSETTABLE;
	if (name == "NOENABLEDISABLE")
		return CVARDOC_NOENABLEDISABLE;
	if (name == "SERVERARCHIVE")
		return CVARDOC_SERVERARCHIVE;
	if (name == "CLIENTARCHIVE")
		return CVARDOC_CLIENTARCHIVE;
	return 0;
}

// Renders a JSON scalar default value as a display string, typed to match.
static std::string DefaultToString(const Json::Value& v)
{
	if (v.isBool())
		return v.asBool() ? "true" : "false";
	if (v.isIntegral())
		return std::to_string(v.asInt64());
	if (v.isDouble())
		return std::to_string(v.asDouble());
	if (v.isString())
		return v.asString();
	return "";
}

bool CvarDocDb::LoadEmbedded()
{
	// Reset to a known-empty state; a failed load leaves the DB empty.
	m_Cvars.clear();
	m_Loaded = false;
	m_SchemaVersion = 0;
	m_OdamexVersion.clear();
	m_CommitHash.clear();
	m_BranchName.clear();

	bool AnyLoaded = false;
	if (ParseDocument(ODAMEX_CVARDOC_JSON))
		AnyLoaded = true;
	if (ParseDocument(ODASRV_CVARDOC_JSON))
		AnyLoaded = true;

	return AnyLoaded;
}

bool CvarDocDb::ParseDocument(std::string_view Doc)
{
	if (Doc.empty())
		return false;

	Json::CharReaderBuilder builder;
	const std::unique_ptr<Json::CharReader> reader(builder.newCharReader());

	Json::Value root;
	std::string errors;
	if (!reader->parse(Doc.data(), Doc.data() + Doc.size(), &root, &errors))
	{
		wxLogDebug("cvardoc: JSON parse error: %s", errors.c_str());
		return false;
	}

	if (!root.isObject())
		return false;

	const int SchemaVersion = root.get("schema_version", 0).asInt();
	if (SchemaVersion != CVARDOC_SCHEMA_VERSION)
	{
		wxLogDebug("cvardoc: unsupported schema_version %d", SchemaVersion);
		return false;
	}

	// Document-level metadata comes from the first document loaded.
	if (!m_Loaded)
	{
		m_SchemaVersion = SchemaVersion;
		m_OdamexVersion = wxString::FromUTF8(root.get("odamex_version", "").asString().c_str());
		m_CommitHash = wxString::FromUTF8(root.get("odamex_commithash", "").asString().c_str());
		m_BranchName = wxString::FromUTF8(root.get("odamex_branchname", "").asString().c_str());
	}

	const Json::Value& cvars = root["cvars"];
	if (!cvars.isArray())
		return false;

	for (const auto& entry : cvars)
	{
		if (!entry.isObject() || !entry.isMember("name"))
			continue;

		CvarDoc_t doc;
		doc.Name = entry.get("name", "").asString();
		if (doc.Name.empty())
			continue;

		// Keep the first definition encountered across documents.
		if (m_Cvars.count(doc.Name))
			continue;

		doc.Type = entry.get("type", "").asString();
		doc.HelpText = entry.get("helptext", "").asString();
		doc.DefaultValue = DefaultToString(entry["default"]);

		if (entry.isMember("min"))
		{
			doc.HasMin = true;
			doc.Min = entry["min"].asDouble();
		}
		if (entry.isMember("max"))
		{
			doc.HasMax = true;
			doc.Max = entry["max"].asDouble();
		}

		const Json::Value& flags = entry["flags"];
		if (flags.isArray())
		{
			for (const auto& f : flags)
				doc.Flags |= FlagFromName(f.asString());
		}

		m_Cvars[doc.Name] = doc;
	}

	m_Loaded = true;
	return true;
}

const CvarDoc_t* CvarDocDb::Find(const std::string& Name) const
{
	std::unordered_map<std::string, CvarDoc_t>::const_iterator it =
	    m_Cvars.find(Name);
	if (it == m_Cvars.end())
		return NULL;
	return &it->second;
}

