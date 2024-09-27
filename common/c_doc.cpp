// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1998-2006 by Randy Heit (ZDoom).
// Copyright (C) 2006-2021 by The Odamex Team.
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

#include "c_dispatch.h"
#include "cmdlib.h"
#include "i_system.h"
#include "m_fileio.h"

#ifdef CLIENT_APP
#define CS_STRING "Odamex Client"
#else
#define CS_STRING "Odamex Server"
#endif

// A view to a list of Cvars.
typedef std::vector<cvar_t*> CvarView;

/**
 * @brief Top part of an HTML document.
 *
 * @param out Output buffer to write to.
 * @param title Title to put in the title tag.
 */
static void HTMLHeader(std::string& out, const char* title)
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
	StrFormat(out, HEADER, title);
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
	case CVARTYPE_NONE:
	case CVARTYPE_MAX:
		return;
		break;
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
		StrFormat(buffer, "Default: %d", val);
		info.push_back(buffer);

		if (cvar.getMinValue() != -FLT_MAX)
		{
			StrFormat(buffer, "Min: %d", static_cast<int>(cvar.getMinValue()));
			info.push_back(buffer);
		}

		if (cvar.getMaxValue() != FLT_MAX)
		{
			StrFormat(buffer, "Max: %d", static_cast<int>(cvar.getMaxValue()));
			info.push_back(buffer);
		}

		break;
	}
	case CVARTYPE_FLOAT: {
		std::string buffer;
		float val = atof(cvar.getDefault().c_str());
		StrFormat(buffer, "Default: %f", val);
		info.push_back(buffer);

		if (cvar.getMinValue() != -FLT_MAX)
		{
			StrFormat(buffer, "Min: %f", cvar.getMinValue());
			info.push_back(buffer);
		}

		if (cvar.getMaxValue() != FLT_MAX)
		{
			StrFormat(buffer, "Max: %f", cvar.getMaxValue());
			info.push_back(buffer);
		}

		break;
	}
	case CVARTYPE_STRING:
		if (!cvar.getDefault().empty())
		{
			std::string buf;
			StrFormat(buf, "Default: \"%s\"", cvar.getDefault().c_str());
			info.push_back(buf);
		}
		break;
	case CVARTYPE_NONE:
	case CVARTYPE_MAX:
		return;
		break;
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
	StrFormat(out, ROW, cvar.name(), flagstr.c_str(), cvar.helptext());
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

static bool CvarCmp(const cvar_t* a, const cvar_t* b)
{
	return strcmp(a->name(), b->name()) < 0;
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

	std::sort(view.begin(), view.end(), CvarCmp);
	return view;
}

BEGIN_COMMAND(cvardoc)
{
	std::string buffer;
	std::string path = M_GetWriteDir();
	if (!M_IsPathSep(*(path.end() - 1)))
	{
		path += PATHSEP;
	}

#ifdef CLIENT_APP
	path += "odamex_cvardoc.html";
#else
	path += "odasrv_cvardoc.html";
#endif

	// Try and open a file in our write directory.
	FILE* fh = fopen(path.c_str(), "wt+");
	if (fh == NULL)
	{
		Printf("error: Could not open \"%s\" for writing.\n", path.c_str());
		return;
	}

	// First the header.
	std::string title;
	StrFormat(title, "%s %s Console Variables", CS_STRING, DOTVERSIONSTR);
	HTMLHeader(buffer, title.c_str());
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

	StrFormat(buffer, PREAMBLE, title.c_str(), NiceVersion());
	fwrite(buffer.data(), sizeof(char), buffer.size(), fh);

	// Initial tag for cvars.
	fputs("<dl>", fh);

	// Stamp out our CVars
	CvarView view = GetSortedCvarView();
	for (CvarView::const_iterator it = view.begin(); it != view.end(); ++it)
	{
		HTMLCvarRow(buffer, **it);
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
	Printf("Wrote %ld bytes to \"%s\"\n", bytes, path.c_str());
}
END_COMMAND(cvardoc)
