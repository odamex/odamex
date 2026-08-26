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
//   Dumps a document with a list of all cvars
//
//-----------------------------------------------------------------------------


#include "odamex.h"

#include <algorithm>

#include <json/json.h>

#include "c_dispatch.h"
#include "c_doc.h"
#include "cmdlib.h"
#include "i_system.h"
#include "m_fileio.h"
#include "version.h"

#ifdef CLIENT_APP
constexpr const char* CS_STRING = "Odamex Client";
constexpr const char* CVARDOC_BASENAME = "odamex_cvardoc";
#elif defined(SERVER_APP)
constexpr const char* CS_STRING = "Odamex Server";
constexpr const char* CVARDOC_BASENAME = "odasrv_cvardoc";
#elif defined(TEST_APP)
constexpr const char* CS_STRING = "Odamex Unit Tests";
constexpr const char* CVARDOC_BASENAME = "odagtest_cvardoc";
#endif

// A view to a list of Cvars.
typedef std::vector<cvar_t*> CvarView;

/**
 * @brief Top part of an HTML document.
 *
 * @param out Output buffer to write to.
 * @param title Title to put in the title tag.
 */
static void HTMLHeader(std::string& out, const std::string& title)
{
	std::string buf;
	const char* HEADER =
	    "<!DOCTYPE html>"
	    "<html>"
	    "<head>"
	    "<meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\">"
	    "<title>%s</title>"
	    "<style>"
	    "html {"
	    "background-color: #2c2c2c;"
	    "color: rgb(245, 245, 245);"
	    "font-family: sans-serif;"
	    "}"
	    "a {"
	    "color: #ffa300;"
	    "}"
	    "</style>"
	    "</head>"
	    "<body>";
	out = fmt::sprintf(HEADER, title);
}

/**
 * @brief Render cvar information as HTML.
 *
 * @param out Output buffer to write to.
 * @param cvar Cvar to read.
 */
static void HTMLCvarRow(std::string& out, const cvar_t& cvar)
{
	std::vector<std::string> info;

	switch (cvar.type())
	{
	case CVARTYPE_BOOL:
		info.push_back("Boolean");
		break;
	case CVARTYPE_BYTE:
		info.push_back("Byte");
		break;
	case CVARTYPE_WORD:
		info.push_back("Short");
		break;
	case CVARTYPE_INT:
		info.push_back("Number");
		break;
	case CVARTYPE_FLOAT:
		info.push_back("Float");
		break;
	case CVARTYPE_STRING:
		info.push_back("String");
		break;
	default:
		out = "";
		return;
	}

	// Default and range
	switch (cvar.type())
	{
	case CVARTYPE_BOOL: {
		int val = atoi(cvar.getDefault().c_str());
		info.push_back(val == 0 ? "Default: False" : "Default: True");
		break;
	}
	case CVARTYPE_BYTE:
	case CVARTYPE_WORD:
	case CVARTYPE_INT: {
		std::string buffer;
		int val = atoi(cvar.getDefault().c_str());
		buffer = fmt::sprintf("Default: %d", val);
		info.push_back(buffer);

		if (cvar.getMinValue() != -FLT_MAX)
		{
			buffer = fmt::sprintf("Min: %d", static_cast<int>(cvar.getMinValue()));
			info.push_back(buffer);
		}

		if (cvar.getMaxValue() != FLT_MAX)
		{
			buffer = fmt::sprintf("Max: %d", static_cast<int>(cvar.getMaxValue()));
			info.push_back(buffer);
		}

		break;
	}
	case CVARTYPE_FLOAT: {
		std::string buffer;
		float val = atof(cvar.getDefault().c_str());
		buffer = fmt::sprintf("Default: %f", val);
		info.push_back(buffer);

		if (cvar.getMinValue() != -FLT_MAX)
		{
			buffer = fmt::sprintf("Min: %f", cvar.getMinValue());
			info.push_back(buffer);
		}

		if (cvar.getMaxValue() != FLT_MAX)
		{
			buffer = fmt::sprintf("Max: %f", cvar.getMaxValue());
			info.push_back(buffer);
		}

		break;
	}
	case CVARTYPE_STRING:
		if (!cvar.getDefault().empty())
		{
			std::string buf;
			buf = fmt::sprintf("Default: \"%s\"", cvar.getDefault());
			info.push_back(buf);
		}
		break;
	default:
		out = "";
		return;
	}

	if (cvar.flags() & CVAR_USERINFO)
		info.push_back("Added to userinfo");
	if (cvar.flags() & CVAR_SERVERINFO)
		info.push_back("Servers tell clients when changed");
	if (cvar.flags() & CVAR_NOSET)
		info.push_back("Can't be set");
	if (cvar.flags() & CVAR_LATCH)
		info.push_back("Latched");
	if (cvar.flags() & CVAR_UNSETTABLE)
		info.push_back("Can be unset");
	if (cvar.flags() & CVAR_NOENABLEDISABLE)
		info.push_back("No Enable/Disable");
	if (cvar.flags() & CVAR_SERVERARCHIVE)
		info.push_back("Saved on the server");
	if (cvar.flags() & CVAR_CLIENTARCHIVE)
		info.push_back("Saved on the client");

	std::string flagstr = JoinStrings(info, ", ");

	const char* ROW = "<dt><code>%s</code></dt>"
	                  "<dd>"
	                  "<p><small><em>%s</em></small></p>"
	                  "<p>%s</p>"
	                  "</dd>";
	out = fmt::sprintf(ROW, cvar.name(), flagstr, cvar.helptext());
}

/**
 * @brief Bottom part of an HTML document.
 *
 * @param out Output buffer to write to.
 */
static void HTMLFooter(std::string& out)
{
	out = "</body>"
	      "</html>";
}

/**
 * @brief Return a "view" of Cvars sorted by name.
 */
static CvarView GetSortedCvarView()
{
	CvarView view;

	cvar_t* var = GetFirstCvar();
	while (var != NULL)
	{
		view.push_back(var);
		var = var->GetNext();
	}

	std::sort(view.begin(), view.end(), [](const cvar_t* a, const cvar_t* b){ return a->name().compare(b->name()) < 0; });
	return view;
}

/**
 * @brief Render cvar information as a JSON object.
 *
 * @param out Output value to write to. Set to null for cvars that have no
 * representable type.
 * @param cvar Cvar to read.
 */
static void JSONCvarObject(Json::Value& out, const cvar_t& cvar)
{
	out = Json::Value(Json::objectValue);

	// Type string and whether the cvar carries a numeric range.
	const char* typestr = NULL;
	bool numeric = false;
	switch (cvar.type())
	{
	case CVARTYPE_BOOL:
		typestr = "boolean";
		break;
	case CVARTYPE_BYTE:
	case CVARTYPE_WORD:
	case CVARTYPE_INT:
		typestr = "integer";
		numeric = true;
		break;
	case CVARTYPE_FLOAT:
		typestr = "number";
		numeric = true;
		break;
	case CVARTYPE_STRING:
		typestr = "string";
		break;
	default:
		out = Json::Value(Json::nullValue);
		return;
	}

	out["name"] = cvar.name();
	out["type"] = typestr;
	out["helptext"] = cvar.helptext();

	// Default value, typed to match the cvar.
	switch (cvar.type())
	{
	case CVARTYPE_BOOL:
		out["default"] = atoi(cvar.getDefault().c_str()) != 0;
		break;
	case CVARTYPE_BYTE:
	case CVARTYPE_WORD:
	case CVARTYPE_INT:
		out["default"] = atoi(cvar.getDefault().c_str());
		break;
	case CVARTYPE_FLOAT:
		out["default"] = atof(cvar.getDefault().c_str());
		break;
	case CVARTYPE_STRING:
		out["default"] = cvar.getDefault();
		break;
	default:
		break;
	}

	// Min/max only for numeric cvars, and only when a bound is actually set.
	if (numeric)
	{
		if (cvar.getMinValue() != -FLT_MAX)
		{
			if (cvar.type() == CVARTYPE_FLOAT)
				out["min"] = cvar.getMinValue();
			else
				out["min"] = static_cast<int>(cvar.getMinValue());
		}

		if (cvar.getMaxValue() != FLT_MAX)
		{
			if (cvar.type() == CVARTYPE_FLOAT)
				out["max"] = cvar.getMaxValue();
			else
				out["max"] = static_cast<int>(cvar.getMaxValue());
		}
	}

	// Flags as an array of string names.
	Json::Value flags(Json::arrayValue);
	if (cvar.flags() & CVAR_USERINFO)
		flags.append("USERINFO");
	if (cvar.flags() & CVAR_SERVERINFO)
		flags.append("SERVERINFO");
	if (cvar.flags() & CVAR_NOSET)
		flags.append("NOSET");
	if (cvar.flags() & CVAR_LATCH)
		flags.append("LATCH");
	if (cvar.flags() & CVAR_UNSETTABLE)
		flags.append("UNSETTABLE");
	if (cvar.flags() & CVAR_NOENABLEDISABLE)
		flags.append("NOENABLEDISABLE");
	if (cvar.flags() & CVAR_SERVERARCHIVE)
		flags.append("SERVERARCHIVE");
	if (cvar.flags() & CVAR_CLIENTARCHIVE)
		flags.append("CLIENTARCHIVE");
	out["flags"] = flags;
}

namespace
{

/**
 * @brief Render the cvar documentation as a complete HTML document.
 */
std::string BuildCvarDocHTML()
{
	std::string doc;
	std::string buffer;

	// First the header.
	std::string title;
	title = fmt::sprintf("%s %s Console Variables", CS_STRING, DOTVERSIONSTR);
	HTMLHeader(buffer, title);
	doc += buffer;

	// Then the title and initial paragraph.
	const char* PREAMBLE =
	    "<h2>%s</h2>"
	    "<p>"
	    "These are the console variables known to the %s as of revision %s."
	    "</p><p>"
	    "In order to understand some of the documentation below, it's important to get "
	    "some definitions out of the way first.  A Boolean is a true/false value, a Byte "
	    "is a number from 0-255, a Short is a number from 0-65535, and a Float is a "
	    "number with a decimal point in it, like 3.14."
	    "</p>";

	doc += fmt::sprintf(PREAMBLE, title, CS_STRING, NiceVersion());

	// Initial tag for cvars.
	doc += "<dl>";

	// Stamp out our CVars
	CvarView view = GetSortedCvarView();
	for (const auto& cvar : view)
	{
		HTMLCvarRow(buffer, *cvar);
		doc += buffer;
	}

	// Ending tag for cvars.
	doc += "</dl>";

	// Lastly the footer.
	HTMLFooter(buffer);
	doc += buffer;

	return doc;
}

/**
 * @brief Render the cvar documentation as a complete JSON document.
 */
std::string BuildCvarDocJSON()
{
	Json::Value root(Json::objectValue);
	root["schema_version"] = 1;
	root["odamex_version"] = DOTVERSIONSTR;
	root["odamex_commithash"] = GitHash();
	root["odamex_branchname"] = GitBranch();

	Json::Value cvars(Json::arrayValue);
	CvarView view = GetSortedCvarView();
	for (const auto& cvar : view)
	{
		Json::Value obj;
		JSONCvarObject(obj, *cvar);
		if (obj.isNull())
			continue;
		cvars.append(obj);
	}
	root["cvars"] = cvars;

	Json::StyledWriter writer;
	return writer.write(root);
}

} // namespace

/**
 * @brief Emit a generated document.
 *
 * infodumpdest_t::FILE writes a file in the write directory.
 * infodumpdest_t::STDOUT prints the document to stdout instead.
 *
 * @param doc Document contents to emit.
 * @param basename Name to give the file, without an extension.
 * @param ext Extension to give the file, including the leading dot.
 * @param dest Where the document should go.
 * @return False if the document could not be written.
 */
bool EmitInfoDump(const std::string& doc, const char* basename, const char* ext,
                  infodumpdest_t dest)
{
	if (dest == infodumpdest_t::STDOUT)
	{
		fwrite(doc.data(), sizeof(char), doc.size(), stdout);
		fflush(stdout);
		return true;
	}

	std::string path = M_GetWriteDir();
	if (!M_IsPathSep(path.back()))
	{
		path += PATHSEP;
	}
	path += basename;
	path += ext;


	FILE* fh = fopen(path.c_str(), "wt+");
	if (fh == nullptr)
	{
		PrintFmt("error: Could not open \"{}\" for writing.\n", path);
		return false;
	}

	fwrite(doc.data(), sizeof(char), doc.size(), fh);

	long bytes = ftell(fh);
	fclose(fh);

	// Success!
	PrintFmt("Wrote {} bytes to \"{}\"\n", bytes, path);
	return true;
}

bool C_WriteCvarDoc(infodumpdest_t dest)
{
	return EmitInfoDump(BuildCvarDocHTML(), CVARDOC_BASENAME, ".html", dest);
}

bool C_WriteCvarDocJSON(infodumpdest_t dest)
{
	return EmitInfoDump(BuildCvarDocJSON(), CVARDOC_BASENAME, ".json", dest);
}

BEGIN_COMMAND(cvardoc)
{
	C_WriteCvarDoc(infodumpdest_t::FILE);
}
END_COMMAND(cvardoc)

BEGIN_COMMAND(cvardocjson)
{
	C_WriteCvarDocJSON(infodumpdest_t::FILE);
}
END_COMMAND(cvardocjson)
