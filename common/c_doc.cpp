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
#include "cmdlib.h"
#include "i_system.h"
#include "m_fileio.h"
#include "version.h"

#ifdef CLIENT_APP
#define CS_STRING "Odamex Client"
#elif defined(SERVER_APP)
#define CS_STRING "Odamex Server"
#elif defined(TEST_APP)
#define CS_STRING "Odamex Unit Tests"
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
 * @brief Serialize a single JSON value to a compact, one-line string with no
 *        trailing newline, for embedding in hand-ordered output.
 */
static std::string JSONScalar(const Json::Value& value)
{
	Json::FastWriter writer;
	writer.omitEndingLineFeed();
	return writer.write(value);
}

/**
 * @brief Render cvar information as a JSON object string.
 *
 * This is because jsoncpp serializes objects in alphabetical order,
 * so to get a custom order, we must do it by hand.
 *
 * @param cvar Cvar to read.
 * 
 * @param indent Indentation string for the object's braces. Fields are
 * indented one further level (3 spaces).
 * 
 * @return The serialized object, or an empty string for cvars that have no
 * representable type.
 */
static std::string JSONCvarObject(const cvar_t& cvar, const std::string& indent)
{
	// Type string and whether the cvar carries a numeric range.
	const char* typestr = NULL;
	bool numeric = false;
	switch (cvar.type())
	{
	case CVARTYPE_BOOL:
		typestr = "bool";
		break;
	case CVARTYPE_BYTE:
	case CVARTYPE_WORD:
	case CVARTYPE_INT:
		typestr = "number";
		numeric = true;
		break;
	case CVARTYPE_FLOAT:
		typestr = "float";
		numeric = true;
		break;
	case CVARTYPE_STRING:
		typestr = "string";
		break;
	default:
		return "";
	}

	// Default value, typed to match the cvar.
	Json::Value defval;
	switch (cvar.type())
	{
	case CVARTYPE_BOOL:
		defval = atoi(cvar.getDefault().c_str()) != 0;
		break;
	case CVARTYPE_BYTE:
	case CVARTYPE_WORD:
	case CVARTYPE_INT:
		defval = atoi(cvar.getDefault().c_str());
		break;
	case CVARTYPE_FLOAT:
		defval = atof(cvar.getDefault().c_str());
		break;
	case CVARTYPE_STRING:
		defval = cvar.getDefault();
		break;
	default:
		break;
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

	// Emit fields in a fixed order: name, type, helptext, min, max, default,
	// flags. min/max are only present for numeric cvars with a bound set.
	const std::string field = indent + "   ";
	std::string out;
	out += indent + "{\n";
	out += field + "\"name\" : " + JSONScalar(Json::Value(cvar.name())) + ",\n";
	out += field + "\"type\" : " + JSONScalar(Json::Value(typestr)) + ",\n";
	out += field + "\"helptext\" : " + JSONScalar(Json::Value(cvar.helptext())) + ",\n";

	if (numeric && cvar.getMinValue() != -FLT_MAX)
	{
		Json::Value minv = (cvar.type() == CVARTYPE_FLOAT)
		                       ? Json::Value(static_cast<double>(cvar.getMinValue()))
		                       : Json::Value(static_cast<int>(cvar.getMinValue()));
		out += field + "\"min\" : " + JSONScalar(minv) + ",\n";
	}

	if (numeric && cvar.getMaxValue() != FLT_MAX)
	{
		Json::Value maxv = (cvar.type() == CVARTYPE_FLOAT)
		                       ? Json::Value(static_cast<double>(cvar.getMaxValue()))
		                       : Json::Value(static_cast<int>(cvar.getMaxValue()));
		out += field + "\"max\" : " + JSONScalar(maxv) + ",\n";
	}

	out += field + "\"default\" : " + JSONScalar(defval) + ",\n";
	out += field + "\"flags\" : " + JSONScalar(flags) + "\n";
	out += indent + "}";
	return out;
}

BEGIN_COMMAND(cvardoc)
{
	std::string buffer;
	std::string path = M_GetWriteDir();
	if (!M_IsPathSep(path.back()))
	{
		path += PATHSEP;
	}

#ifdef CLIENT_APP
	path += "odamex_cvardoc.html";
#elif defined(SERVER_APP)
	path += "odasrv_cvardoc.html";
#elif defined(TEST_APP)
	path += "odagtest_cvardoc.html";
#endif

	// Try and open a file in our write directory.
	FILE* fh = fopen(path.c_str(), "wt+");
	if (fh == NULL)
	{
		PrintFmt("error: Could not open \"{}\" for writing.\n", path);
		return;
	}

	// First the header.
	std::string title;
	title = fmt::sprintf("%s %s Console Variables", CS_STRING, DOTVERSIONSTR);
	HTMLHeader(buffer, title);
	fwrite(buffer.data(), sizeof(char), buffer.size(), fh);

	// Then the title and initial paragraph.
	const char* PREAMBLE =
	    "<h2>%s</h2>"
	    "<p>"
	    "These are the console variables known to the " CS_STRING " as of revision %s."
	    "</p><p>"
	    "In order to understand some of the documentation below, it's important to get "
	    "some definitions out of the way first.  A Boolean is a true/false value, a Byte "
	    "is a number from 0-255, a Short is a number from 0-65535, and a Float is a "
	    "number with a decimal point in it, like 3.14."
	    "</p>";

	buffer = fmt::sprintf(PREAMBLE, title, NiceVersion());
	fwrite(buffer.data(), sizeof(char), buffer.size(), fh);

	// Initial tag for cvars.
	fputs("<dl>", fh);

	// Stamp out our CVars
	CvarView view = GetSortedCvarView();
	for (const auto& cvar : view)
	{
		HTMLCvarRow(buffer, *cvar);
		fwrite(buffer.data(), sizeof(char), buffer.size(), fh);
	}

	// Ending tag for cvars.
	fputs("</dl>", fh);

	// Lastly the footer.
	HTMLFooter(buffer);
	fwrite(buffer.data(), sizeof(char), buffer.size(), fh);

	long bytes = ftell(fh);
	fclose(fh);

	// Success!
	PrintFmt("Wrote {} bytes to \"{}\"\n", bytes, path);
}
END_COMMAND(cvardoc)

BEGIN_COMMAND(cvardocjson)
{
	std::string path = M_GetWriteDir();
	if (!M_IsPathSep(path.back()))
	{
		path += PATHSEP;
	}

#ifdef CLIENT_APP
	path += "odamex_cvardoc.json";
#elif defined(SERVER_APP)
	path += "odasrv_cvardoc.json";
#elif defined(TEST_APP)
	path += "odagtest_cvardoc.json";
#endif

	// Try and open a file in our write directory.
	FILE* fh = fopen(path.c_str(), "wt+");
	if (fh == NULL)
	{
		PrintFmt("error: Could not open \"{}\" for writing.\n", path);
		return;
	}

	// Build the cvar objects, each hand-ordered (array elements indent two
	// levels under the document root).
	std::vector<std::string> entries;
	CvarView view = GetSortedCvarView();
	for (const auto& cvar : view)
	{
		std::string obj = JSONCvarObject(*cvar, "      ");
		if (obj.empty())
			continue;
		entries.push_back(obj);
	}

	// jsoncpp serializes object keys alphabetically, so we assemble the whole
	// document by hand to keep the metadata first and the cvars array last.
	std::string buffer;
	buffer += "{\n";
	buffer += fmt::sprintf("   \"schema_version\" : %d,\n", 1);
	buffer += fmt::sprintf("   \"odamex_version\" : %s,\n",
	                       Json::valueToQuotedString(DOTVERSIONSTR));
	buffer += fmt::sprintf("   \"odamex_commithash\" : %s,\n",
	                       Json::valueToQuotedString(GitHash()));
	buffer += fmt::sprintf("   \"odamex_branchname\" : %s,\n",
	                       Json::valueToQuotedString(GitBranch()));
	buffer += "   \"cvars\" :\n";
	buffer += "   [\n";
	for (size_t i = 0; i < entries.size(); i++)
	{
		buffer += entries[i];
		buffer += (i + 1 < entries.size()) ? ",\n" : "\n";
	}
	buffer += "   ]\n";
	buffer += "}\n";

	fwrite(buffer.data(), sizeof(char), buffer.size(), fh);

	long bytes = ftell(fh);
	fclose(fh);

	// Success!
	PrintFmt("Wrote {} bytes to \"{}\"\n", bytes, path);
}
END_COMMAND(cvardocjson)
