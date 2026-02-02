// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1998-2006 by Randy Heit (ZDoom).
// Copyright (C) 2006-2025 by The Odamex Team.
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
//   Handles parsing all ONCRINFO lumps.
//
//-----------------------------------------------------------------------------

#include "odamex.h"

#include "w_wad.h"
#include "oscanner.h"
#include "gstrings.h"
#include "g_announcer.h"

static void ParseSpreeAndMulti(OScanner& os,
                               std::unordered_map<std::string, std::string>& soundDict)
{
	std::string key = StdStringToLower(os.getToken());
	os.mustScanInt();
	int level = os.getTokenInt();
	os.mustScan();
	os.assertTokenIs("=");
	os.mustScan();
	std::string token = StdStringToLower(os.getToken());

	std::string keyName = key + " " + std::to_string(level);

	soundDict[keyName] = token;
}

static void ParseAnnouncerToken(OScanner& os,
																std::unordered_map<std::string, std::string>& soundDict)
{
	std::string tokenName = StdStringToLower(os.getToken());
	os.mustScan();
	os.assertTokenIs("=");
	os.mustScan();
	std::string token = os.getToken();
	soundDict[tokenName] = token;
}

static void ParseMetadata(OScanner& os, AnnouncerMetaData_s& metaData,
                          const std::string& fieldName)
{
	os.mustScan();
	os.assertTokenIs("=");
	os.mustScan();

	std::string token = os.getToken();

	if (fieldName == "name")
	{
		metaData.name = token;
	}
	else if (fieldName == "description")
	{
		metaData.description = token;
	}
	else if (fieldName == "author")
	{
		metaData.author = token;
	}
	else
	{
		os.error(fmt::format("ParseMetadata called with unknown field name \"{}\".",
		                     fieldName));
	}
}

static void ParseOncrInfo(const int lump, const OLumpName name)
{
	char* buffer = static_cast<char*>(W_CacheLumpNum(lump, PU_CACHE));

	const OScannerConfig config = {
	    "ONCRINFO", // lumpName
	    false,      // semiComments
	    true,       // cComments
	};
	OScanner os = OScanner::openBuffer(config, buffer, buffer + W_LumpLength(lump));

	std::unordered_map<std::string, Announcer_s> newAnnouncers;

	while (os.scan())
	{
		os.assertTokenNoCaseIs("{");
		os.mustScan();

		std::unordered_map<std::string, std::string> soundDict;
		AnnouncerMetaData_s metaData = AnnouncerMetaData_s();

		while (!os.compareToken("}"))
		{
			// Parse metadata
			if (os.compareTokenNoCase("name"))
			{
				ParseMetadata(os, metaData, "name");
			}
			else if (os.compareTokenNoCase("description"))
			{
				ParseMetadata(os, metaData, "description");
			}
			else if (os.compareTokenNoCase("author"))
			{
				ParseMetadata(os, metaData, "author");
			}
			// Parse announcer tokens
			// Compare the token against known token groups to see
			// if its valid.
			else if (AnnouncerManager::getInstance().namedTokenExists(StdStringToLower(os.getToken())))
			{
				ParseAnnouncerToken(os, soundDict);
			}
			else if (os.compareTokenNoCase("multi") || os.compareTokenNoCase("spree"))
			{
				ParseSpreeAndMulti(os, soundDict);
			}
			else
			{
				// We don't know what this token is.
				std::string buffer =
						fmt::sprintf("Unknown ONCRINFO Token \"%s\".", os.getToken());
				os.warning(buffer);
			}
			os.mustScan();
		}

		if (metaData.name.empty())
		{
			os.error("Announcer pack is missing a 'name' field.");
		}

		Announcer_s newAnnouncer = Announcer_s();
		newAnnouncer.metadata = metaData;
		newAnnouncer.soundDict = soundDict;

		if (newAnnouncers.find(metaData.name) == newAnnouncers.end())
		{
			newAnnouncers[metaData.name] = newAnnouncer;
		}
		else
		{
			// Merge existing announcer with new one.
			Announcer_s& existingAnnouncer = newAnnouncers[metaData.name];
			// Update metadata
			existingAnnouncer.metadata = metaData;
			// Merge sound dictionaries
			for (auto& soundIt : soundDict)
			{
				existingAnnouncer.soundDict[soundIt.first] = soundIt.second;
			}
		}

		bool didScan = os.scan();
		if (!didScan)
		{
			break;
		}
		else
		{
			os.unScan();
		}
		continue;
	}

	AnnouncerManager::getInstance().loadAnnouncers(newAnnouncers);
}

#ifdef CLIENT_APP
EXTERN_CVAR(cl_announcer)
#endif

void G_ParseOncrInfo()
{
	int lump = -1;

	AnnouncerManager::getInstance().reset();

	// No ONCRINFO? Load defaults and continue.
	if (W_FindLump("ONCRINFO", lump) == -1)
	{
		AnnouncerManager::getInstance().loadAnnouncerDefaults();
		return;
	}

	while ((lump = W_FindLump("ONCRINFO", lump)) != -1)
	{
		ParseOncrInfo(lump, "ONCRINFO");
	}

#ifdef CLIENT_APP
	// Reset the preferred announcer once all announcers are loaded.
	cl_announcer.Callback();
#endif
}
