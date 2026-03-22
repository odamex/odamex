// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
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
//		Parsed ID24 demo loop support.
//
//-----------------------------------------------------------------------------

#include "odamex.h"

#include "d_demoloop.h"

#include <cmath>
#include <string>
#include <vector>

#include "d_pages.h"
#include "f_wipe.h"
#include "g_game.h"
#include "gstrings.h"
#include "gi.h"
#include "i_music.h"
#include "cmdlib.h"
#include "m_jsonlump.h"
#include "s_sound.h"
#include "w_wad.h"

namespace
{
	static constexpr JSONLumpVersion DemoLoopVersion = { 1, 0, 0 };

	enum class demo_loop_entry_type_t
	{
		art_screen = 0,
		demo_lump = 1,
	};

	enum class demo_loop_wipe_t
	{
		use_current = -1,
		immediate = 0,
		screen_melt = 1,
	};

	struct demo_loop_entry_t
	{
		demo_loop_entry_type_t type = demo_loop_entry_type_t::art_screen;
		OLumpName primaryLump;
		std::string secondaryLump;
		int duration = 0;
		demo_loop_wipe_t outroWipe = demo_loop_wipe_t::use_current;
		bool showAdvisor = false;
	};

	static std::vector<demo_loop_entry_t> demo_loop_entries;
	static bool demo_loop_checked = false;
	static size_t demo_loop_index = 0;

	static bool D_ParseDemoLoopType(const Json::Value& value, demo_loop_entry_type_t& type)
	{
		if (!value.isInt())
		{
			return false;
		}

		switch (value.asInt())
		{
		case 0:
			type = demo_loop_entry_type_t::art_screen;
			return true;
		case 1:
			type = demo_loop_entry_type_t::demo_lump;
			return true;
		default:
			return false;
		}
	}

	static std::string D_ResolveDemoLoopMusicLump(const std::string& musicName)
	{
		if (musicName.empty())
		{
			return "";
		}

		if (musicName[0] == '$')
		{
			const OString& lookup = GStrings(StdStringToUpper(musicName.c_str() + 1));
			if (lookup.empty())
			{
				return "";
			}

			const OLumpName lumpName = fmt::format("D_{}", lookup);
			return W_CheckNumForName(lumpName) != -1 ? std::string(lumpName.c_str()) : "";
		}

		return W_CheckNumForName(musicName.c_str()) != -1 ? musicName : "";
	}

	static bool D_ParseDemoLoopWipe(const Json::Value& value, demo_loop_wipe_t& wipe)
	{
		if (value.isNull())
		{
			wipe = demo_loop_wipe_t::use_current;
			return true;
		}

		if (!value.isInt())
		{
			return false;
		}

		switch (value.asInt())
		{
		case 0:
			wipe = demo_loop_wipe_t::immediate;
			return true;
		case 1:
			wipe = demo_loop_wipe_t::screen_melt;
			return true;
		default:
			return false;
		}
	}

	static bool D_ParseDemoLoopEntry(const Json::Value& jsonEntry, demo_loop_entry_t& entry)
	{
		if (!jsonEntry.isObject())
		{
			return false;
		}

		const Json::Value& type = jsonEntry["type"];
		const Json::Value& primaryLump = jsonEntry["primarylump"];
		const Json::Value& secondaryLump = jsonEntry["secondarylump"];
		const Json::Value& duration = jsonEntry["duration"];
		const Json::Value& outroWipe = jsonEntry["outrowipe"];
		const Json::Value& showAdvisor = jsonEntry["showadvisor"];

		if (!D_ParseDemoLoopType(type, entry.type) || !primaryLump.isString() || primaryLump.asString().empty())
		{
			return false;
		}

		entry.primaryLump = primaryLump.asString();

		if (!secondaryLump.isNull())
		{
			if (!secondaryLump.isString())
			{
				return false;
			}

			entry.secondaryLump = secondaryLump.asString();
		}

		if (entry.type == demo_loop_entry_type_t::art_screen)
		{
			if (!duration.isNumeric() || duration.asDouble() <= 0.0)
			{
				return false;
			}

			entry.duration = static_cast<int>(std::ceil(duration.asDouble() * TICRATE));
		}
		else
		{
			entry.duration = 0;
		}

		if (!D_ParseDemoLoopWipe(outroWipe, entry.outroWipe))
		{
			return false;
		}

		if (!showAdvisor.isNull())
		{
			if (!showAdvisor.isBool())
			{
				return false;
			}

			entry.showAdvisor = showAdvisor.asBool();
		}

		return true;
	}

	static jsonlumpresult_t D_ParseDemoLoop(const Json::Value& data, const JSONLumpVersion&)
	{
		const Json::Value& entries = data["entries"];
		if (!entries.isArray() || entries.empty())
		{
			return jsonlumpresult_t::PARSEERROR;
		}

		std::vector<demo_loop_entry_t> parsedEntries;
		parsedEntries.reserve(entries.size());

		for (const Json::Value& jsonEntry : entries)
		{
			demo_loop_entry_t entry;
			if (!D_ParseDemoLoopEntry(jsonEntry, entry))
			{
				return jsonlumpresult_t::PARSEERROR;
			}

			parsedEntries.push_back(entry);
		}

		demo_loop_entries = std::move(parsedEntries);
		return jsonlumpresult_t::SUCCESS;
	}

	static OLumpName D_GetDemoLoopLumpName()
	{
		if (W_CheckNumForName("DEMOLOOP") >= 0)
		{
			return "DEMOLOOP";
		}

		if (!gameinfo.demoLoop.empty() && W_CheckNumForName(gameinfo.demoLoop) >= 0)
		{
			return gameinfo.demoLoop;
		}

		return "";
	}

	static void D_ApplyDemoLoopWipe(const demo_loop_wipe_t wipe)
	{
		switch (wipe)
		{
		case demo_loop_wipe_t::use_current:
			Wipe_ClearNextTypeOverride();
			break;
		case demo_loop_wipe_t::immediate:
			Wipe_SetNextTypeOverride(0);
			break;
		case demo_loop_wipe_t::screen_melt:
			Wipe_SetNextTypeOverride(1);
			break;
		}
	}

	static void D_EnsureDemoLoopLoaded()
	{
		if (demo_loop_checked)
		{
			return;
		}

		demo_loop_checked = true;
		demo_loop_entries.clear();
		demo_loop_index = 0;

		const OLumpName lumpname = D_GetDemoLoopLumpName();
		if (lumpname.empty())
		{
			I_Error("D_DoAdvanceDemoLoop: no DEMOLOOP lump found");
		}

		const jsonlumpresult_t result = M_ParseJSONLump(lumpname, "demoloop", DemoLoopVersion, D_ParseDemoLoop);
		if (result != jsonlumpresult_t::SUCCESS)
		{
			I_Error("D_DoAdvanceDemoLoop: DEMOLOOP JSON error in lump {}: {}", lumpname,
			        M_JSONLumpResultToString(result));
		}

		if (demo_loop_entries.empty())
		{
			I_Error("D_DoAdvanceDemoLoop: DEMOLOOP lump {} did not produce any entries", lumpname);
		}
	}

	static bool D_TryRunDemoLoopEntry(const demo_loop_entry_t& entry, page_image_t& page, int& pagetic,
		bool& showAdvisorOverlay)
	{
		const int lumpnum = W_CheckNumForName(entry.primaryLump);
		if (lumpnum < 0)
		{
			return false;
		}

		showAdvisorOverlay = false;
		D_ApplyDemoLoopWipe(entry.outroWipe);

		if (entry.type == demo_loop_entry_type_t::art_screen)
		{
			if (!D_LoadPageImage(page, entry.primaryLump))
			{
				return false;
			}

			pagetic = entry.duration;
			gamestate = GS_DEMOSCREEN;
			showAdvisorOverlay = entry.showAdvisor;

			if (!entry.secondaryLump.empty())
			{
				currentmusic = D_ResolveDemoLoopMusicLump(entry.secondaryLump);
				S_StartMusic(currentmusic);
			}

			return true;
		}

		D_FreePageImage(page);
		G_DeferedPlayDemo(entry.primaryLump.c_str());
		return true;
	}
}

void D_ResetDemoLoop()
{
	demo_loop_entries.clear();
	demo_loop_checked = false;
	demo_loop_index = 0;
}

void D_DoAdvanceDemoLoop(page_image_t& page, int& pagetic, bool& showAdvisorOverlay)
{
	showAdvisorOverlay = false;

	D_EnsureDemoLoopLoaded();

	for (size_t attempts = 0; attempts < demo_loop_entries.size(); ++attempts)
	{
		const demo_loop_entry_t& entry = demo_loop_entries[demo_loop_index];
		demo_loop_index = (demo_loop_index + 1) % demo_loop_entries.size();

		if (D_TryRunDemoLoopEntry(entry, page, pagetic, showAdvisorOverlay))
		{
			return;
		}
	}

	Wipe_ClearNextTypeOverride();
	I_Error("D_DoAdvanceDemoLoop: no usable entries found in DEMOLOOP");
}

VERSION_CONTROL (d_demoloop_cpp, "$Id$")
