// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2022-2022 by DoomBattle.Zone.
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
//	Heads-up displays for battles
//
//-----------------------------------------------------------------------------

#include "odamex.h"

//-----------------------------------------------------------------------------

#include "c_bind.h"
#include "hu_battle.h"
#include "hu_drawers.h"
#include "hu_elements.h"
#include "r_main.h"
#include "v_text.h"
#include "v_video.h"

//-----------------------------------------------------------------------------

EXTERN_CVAR(hud_scale)
EXTERN_CVAR(sv_battleinfo)
EXTERN_CVAR(sv_battlehud)
EXTERN_CVAR(sv_battlestatus)

extern byte* Ranges;
extern std::string cl_battleover_hud_markup;

//-----------------------------------------------------------------------------

enum BattleMarkupType
{
	MT_TEXT,
	MT_TEXT_STAT,
	MT_LINE,
	MT_PATCH,
	MT_IMAGE,
	MT_LOCATION,
	MT_BAR_STAT,
};

typedef std::map<std::string, BattleMarkupType> BattleMarkupTypeLookupType;

BattleMarkupTypeLookupType BattleMarkupTypeLookup;

//-----------------------------------------------------------------------------

enum BattleMarkupLocation
{
	ML_TOP_LEFT,
	ML_TOP_MID,
	ML_TOP_RIGHT,
	ML_MID_LEFT,
	ML_MID_MID,
	ML_MID_RIGHT,
	ML_BOT_LEFT,
	ML_BOT_MID,
	ML_BOT_RIGHT,
};

typedef std::map<std::string, BattleMarkupLocation> BattleMarkupLocationLookupType;

BattleMarkupLocationLookupType BattleMarkupLocationLookup;

//-----------------------------------------------------------------------------

enum BattleMarkupStat
{
	MS_NONE,
	MS_LEVEL_TIME,
	MS_GAME_TIME,
	MS_SECRETS,
	MS_ITEMS,
	MS_MONSTERS,
};

typedef std::map<std::string, BattleMarkupStat> BattleMarkupStatLookupType;

BattleMarkupStatLookupType BattleMarkupStatLookup;

//-----------------------------------------------------------------------------

struct BattleMarkupAlignment
{
	hud::x_align_t xa;
	hud::y_align_t ya;
	hud::x_align_t xo;
	hud::y_align_t yo;

	BattleMarkupAlignment() : xa(hud::X_LEFT), ya(hud::Y_TOP), xo(hud::X_LEFT), yo(hud::Y_TOP) {};

	BattleMarkupAlignment(hud::x_align_t xa, hud::y_align_t ya, hud::x_align_t xo, hud::y_align_t yo)
		: xa(xa), ya(ya), xo(xo), yo(yo) {};
};

std::map<BattleMarkupLocation, BattleMarkupAlignment> BattleMarkupAlignmentLookup;

//-----------------------------------------------------------------------------

struct BattleMarkup
{
	BattleMarkupType mt;
	std::string text;
	EColorRange color;
	int y_offset;
	int width;
	int height;
	char const* font;
	BattleMarkupLocation ml;
	BattleMarkupStat ms;
	std::string formatted_text;
	int font_fixed_width;

	BattleMarkup()
		: mt(MT_TEXT)
		, text("")
		, color(CR_WHITE)
		, y_offset(0)
		, width(0)
		, height(0)
		, font(NULL)
		, ml(ML_TOP_LEFT)
		, ms(MS_NONE)
		, formatted_text("")
		, font_fixed_width(0)
	{}
};

//-----------------------------------------------------------------------------

typedef std::vector<std::string> BattleMarkupStrings;
typedef std::vector<BattleMarkup> BattleMarkupVector;

BattleMarkupVector battleinfo_markups;
std::string last_parsed_markup;

BattleMarkupVector battlehud_markups;
std::string last_sv_battlehud;

BattleMarkupVector battlestatus_markups;
std::string last_sv_battlestatus;

BattleMarkupVector battle_markups;

//-----------------------------------------------------------------------------

void HU_BattleInit()
{
	// BattleMarkupTypeLookup
	BattleMarkupTypeLookup["MT_TEXT"] = MT_TEXT;
	BattleMarkupTypeLookup["MT_TEXT_STAT"] = MT_TEXT_STAT;
	BattleMarkupTypeLookup["MT_LINE"] = MT_LINE;
	BattleMarkupTypeLookup["MT_PATCH"] = MT_PATCH;
	BattleMarkupTypeLookup["MT_IMAGE"] = MT_IMAGE;
	BattleMarkupTypeLookup["MT_LOCATION"] = MT_LOCATION;
	BattleMarkupTypeLookup["MT_BAR_STAT"] = MT_BAR_STAT;

	// BattleMarkupLocationLookup
	BattleMarkupLocationLookup["ML_TOP_LEFT"] = ML_TOP_LEFT;
	BattleMarkupLocationLookup["ML_TOP_MID"] = ML_TOP_MID;
	BattleMarkupLocationLookup["ML_TOP_RIGHT"] = ML_TOP_RIGHT;

	BattleMarkupLocationLookup["ML_MID_LEFT"] = ML_MID_LEFT;
	BattleMarkupLocationLookup["ML_MID_MID"] = ML_MID_MID;
	BattleMarkupLocationLookup["ML_MID_RIGHT"] = ML_MID_RIGHT;

	BattleMarkupLocationLookup["ML_BOT_LEFT"] = ML_BOT_LEFT;
	BattleMarkupLocationLookup["ML_BOT_MID"] = ML_BOT_MID;
	BattleMarkupLocationLookup["ML_BOT_RIGHT"] = ML_BOT_RIGHT;

	// BattleMarkupStatLookup
	BattleMarkupStatLookup["MS_LEVEL_TIME"] = MS_LEVEL_TIME;
	BattleMarkupStatLookup["MS_GAME_TIME"] = MS_GAME_TIME;
	BattleMarkupStatLookup["MS_SECRETS"] = MS_SECRETS;
	BattleMarkupStatLookup["MS_ITEMS"] = MS_ITEMS;
	BattleMarkupStatLookup["MS_MONSTERS"] = MS_MONSTERS;

	// BattleMarkupAlignmentLookup
	BattleMarkupAlignmentLookup[ML_TOP_LEFT] = BattleMarkupAlignment(hud::X_LEFT, hud::Y_TOP, hud::X_LEFT, hud::Y_TOP);
	BattleMarkupAlignmentLookup[ML_TOP_MID] = BattleMarkupAlignment(hud::X_CENTER, hud::Y_TOP, hud::X_CENTER, hud::Y_TOP);
	BattleMarkupAlignmentLookup[ML_TOP_RIGHT] = BattleMarkupAlignment(hud::X_RIGHT, hud::Y_TOP, hud::X_RIGHT, hud::Y_TOP);

	BattleMarkupAlignmentLookup[ML_MID_LEFT] = BattleMarkupAlignment(hud::X_LEFT, hud::Y_MIDDLE, hud::X_LEFT, hud::Y_MIDDLE);
	BattleMarkupAlignmentLookup[ML_MID_MID] = BattleMarkupAlignment(hud::X_CENTER, hud::Y_MIDDLE, hud::X_CENTER, hud::Y_MIDDLE);
	BattleMarkupAlignmentLookup[ML_MID_RIGHT] = BattleMarkupAlignment(hud::X_RIGHT, hud::Y_MIDDLE, hud::X_RIGHT, hud::Y_MIDDLE);

	BattleMarkupAlignmentLookup[ML_BOT_LEFT] = BattleMarkupAlignment(hud::X_LEFT, hud::Y_BOTTOM, hud::X_LEFT, hud::Y_BOTTOM);
	BattleMarkupAlignmentLookup[ML_BOT_MID] = BattleMarkupAlignment(hud::X_CENTER, hud::Y_BOTTOM, hud::X_CENTER, hud::Y_BOTTOM);
	BattleMarkupAlignmentLookup[ML_BOT_RIGHT] = BattleMarkupAlignment(hud::X_RIGHT, hud::Y_BOTTOM, hud::X_RIGHT, hud::Y_BOTTOM);
}

//-----------------------------------------------------------------------------

argb_t HU_GetColorFromRange(EColorRange color_range, palindex_t pick = 0)
{
	const palindex_t start_index = 0xB0;
	const palindex_t end_index = 0xBF;
	const palindex_t pick_index = start_index + pick;

	palindex_t index = pick_index > end_index ? end_index : pick_index;

	byte* pal = ::Ranges + color_range * 256;
	return V_GetDefaultPalette()->colors[pal[index]];
}

//-----------------------------------------------------------------------------

EColorRange GetColorRangeFromString(std::string const& str)
{
	if (!stricmp(str.c_str(), "CR_WHITE"))
		return CR_WHITE;

	if (!stricmp(str.c_str(), "CR_GOLD"))
		return CR_GOLD;

	return CR_PURPLE;
}

//-----------------------------------------------------------------------------

bool ParseBattleMarkupText(BattleMarkupVector& markups, BattleMarkupStrings::iterator& it, bool has_stat)
{
	BattleMarkup markup;

	markup.mt = has_stat ? MT_TEXT_STAT : MT_TEXT;
	markup.text = *(++it)++;
	markup.color = GetColorRangeFromString(*it++);
	markup.y_offset = atoi(it++->c_str());
	markup.height = atoi(it++->c_str());
	markup.font = it++->c_str();
	markup.font_fixed_width = atoi(it++->c_str());

	if (has_stat)
	{
		BattleMarkupStatLookupType::iterator stat = BattleMarkupStatLookup.find(*it++);
		if (stat == BattleMarkupStatLookup.end())
			return false;
		markup.ms = stat->second;
	}

	markups.push_back(markup);
	return true;
}

//-----------------------------------------------------------------------------

bool ParseBattleMarkupLine(BattleMarkupVector& markups, BattleMarkupStrings::iterator& it)
{
	BattleMarkup markup;

	markup.mt = MT_LINE;
	markup.color = GetColorRangeFromString(*(++it)++);
	markup.y_offset = atoi(it++->c_str());

	markups.push_back(markup);
	return true;
}

//-----------------------------------------------------------------------------

bool ParseBattleMarkupPatch(BattleMarkupVector& markups, BattleMarkupStrings::iterator& it)
{
	return false;
}

//-----------------------------------------------------------------------------

bool ParseBattleMarkupImage(BattleMarkupVector& markups, BattleMarkupStrings::iterator& it)
{
	return false;
}

//-----------------------------------------------------------------------------

bool ParseBattleMarkupLocation(BattleMarkupVector& markups, BattleMarkupStrings::iterator& it)
{
	BattleMarkup markup;

	markup.mt = MT_LOCATION;

	BattleMarkupLocationLookupType::iterator location = BattleMarkupLocationLookup.find(*(++it)++);
	if (location == BattleMarkupLocationLookup.end())
		return false;

	markup.ml = location->second;

	markups.push_back(markup);
	return true;
}

//-----------------------------------------------------------------------------

bool ParseBattleMarkupBarStat(BattleMarkupVector& markups, BattleMarkupStrings::iterator& it)
{
	BattleMarkup markup;

	markup.mt = MT_BAR_STAT;

	BattleMarkupStatLookupType::iterator stat = BattleMarkupStatLookup.find(*(++it)++);
	if (stat == BattleMarkupStatLookup.end())
		return false;

	markup.ms = stat->second;

	markups.push_back(markup);
	return true;
}

//-----------------------------------------------------------------------------

void ParseBattleMarkup(BattleMarkupVector& markups, std::string const& unescaped)
{
	markups.clear();

	BattleMarkupStrings markupstrings = StdStringSplit(unescaped, "\n");
	BattleMarkupStrings::iterator it = markupstrings.begin();

	while (it != markupstrings.end())
	{
		bool pass = false;

		BattleMarkupTypeLookupType::iterator markup_type = BattleMarkupTypeLookup.find(*it);
		if (markup_type != BattleMarkupTypeLookup.end())
		{
			switch (markup_type->second)
			{
			case MT_TEXT:
				pass = ParseBattleMarkupText(markups, it, false);
				break;
			case MT_TEXT_STAT:
				pass = ParseBattleMarkupText(markups, it, true);
				break;
			case MT_LINE:
				pass = ParseBattleMarkupLine(markups, it);
				break;
			case MT_PATCH:
				pass = ParseBattleMarkupPatch(markups, it);
				break;
			case MT_IMAGE:
				pass = ParseBattleMarkupImage(markups, it);
				break;
			case MT_LOCATION:
				pass = ParseBattleMarkupLocation(markups, it);
				break;
			case MT_BAR_STAT:
				pass = ParseBattleMarkupBarStat(markups, it);
				break;
			}
		}

		if (!pass)
		{
			Printf(PRINT_HIGH, "ParseBattleMarkup - Invalid line [%s][%s]\n", it->c_str(), unescaped.c_str());
			break;
		}
	}
}

//-----------------------------------------------------------------------------

std::string FormatTimeString(int time, char const* color)
{
	std::string value;

	int const hours = time / 3600;
	int const minutes = time % 3600 / 60;
	int const seconds = time % 60;

	if (hours > 0)
		StrFormat(value, "%s%02d:%02d:%02d", color, hours, minutes, seconds);
	else
		StrFormat(value, "%s%02d:%02d", color, minutes, seconds);

	return value;
}

//-----------------------------------------------------------------------------

std::string FormatTotalString(int count, int total, char const* color)
{
	std::string value;

	StrFormat(value, "%s%02d/%02d", color, count, total);

	return value;
}

//-----------------------------------------------------------------------------

std::string FormatTimeRemainingString(int time_remaining, int color2time, char const* color1, char const* color2)
{
	if (time_remaining <= 0)
	{
		return FormatTimeString(0, color2);
	}

	return FormatTimeString(time_remaining, time_remaining > color2time ? color1 : color2);
}

//-----------------------------------------------------------------------------

std::string GetBattleMarkupStatValue(player_t* player, BattleMarkupStat markup_stat)
{
	std::string value = "?";

	switch (markup_stat)
	{
	case MS_GAME_TIME:
		value = FormatTimeString(player->GameTime, TEXTCOLOR_YELLOW);
		break;
	case MS_LEVEL_TIME:
		if (player->level_endtime <= 0 || !player->ingame())
			value = TEXTCOLOR_WHITE "--:--";
		else
			value = FormatTimeRemainingString(
				(player->level_endtime - level.time + TICRATE / 2) / TICRATE, 10, TEXTCOLOR_YELLOW, TEXTCOLOR_RED
			);
		break;
	case MS_SECRETS:
		value = FormatTotalString(level.found_secrets, level.total_secrets, TEXTCOLOR_YELLOW);
		break;
	case MS_ITEMS:
		value = FormatTotalString(level.found_items, level.total_items, TEXTCOLOR_YELLOW);
		break;
	case MS_MONSTERS:
		value = FormatTotalString(level.killed_monsters, level.total_monsters + level.respawned_monsters, TEXTCOLOR_YELLOW);
		break;
	case MS_NONE:
		value = "";
		break;
	}

	return value;
}

//-----------------------------------------------------------------------------

float GetBattleMarkupBarPercent(player_t* player, BattleMarkupStat markup_stat)
{
	float percent = 0;

	switch (markup_stat)
	{
	case MS_GAME_TIME:
		//value = FormatTimeString(player->GameTime, TEXTCOLOR_YELLOW);
		break;
	case MS_LEVEL_TIME:
		//value = FormatTimeString(level.time / TICRATE, TEXTCOLOR_YELLOW);
		break;
	case MS_SECRETS:
		percent = static_cast<float>(level.found_secrets) / level.total_secrets;
		break;
	case MS_ITEMS:
		percent = static_cast<float>(level.found_items) / level.total_items;
		break;
	case MS_MONSTERS:
		percent = static_cast<float>(level.killed_monsters) / (level.total_monsters + level.respawned_monsters);
		break;
	case MS_NONE:
		percent = 0;
		break;
	}

	return std::max(0.0f, std::min(1.0f, percent));
}

//-----------------------------------------------------------------------------

void FormatBattleMarkupText(player_t* player, BattleMarkup& m)
{
	std::string stat_value = GetBattleMarkupStatValue(player, m.ms);

	if (stat_value.empty())
		m.formatted_text = m.text;
	else
		m.formatted_text = m.text + stat_value;

	V_SetFont(m.font, m.font_fixed_width);
	m.width = V_StringWidth(m.formatted_text.c_str());
	m.height = V_StringHeight(m.formatted_text.c_str());
}

//-----------------------------------------------------------------------------

void HU_ParseBattleInfo()
{
	bool markups_changed = false;

	if (!cl_battleover_hud_markup.empty())
	{
		if (cl_battleover_hud_markup != last_parsed_markup)
		{
			last_parsed_markup = cl_battleover_hud_markup;
			ParseBattleMarkup(battleinfo_markups, Unescape(cl_battleover_hud_markup.c_str()));
			markups_changed = true;
		}
	}
	else if (stricmp(sv_battleinfo.cstring(), last_parsed_markup.c_str()))
	{
		last_parsed_markup = sv_battleinfo.cstring();
		ParseBattleMarkup(battleinfo_markups, sv_battleinfo.unescaped());
		markups_changed = true;
	}

	if (stricmp(sv_battlestatus.cstring(), last_sv_battlestatus.c_str()))
	{
		last_sv_battlestatus = sv_battlestatus.cstring();
		ParseBattleMarkup(battlestatus_markups, sv_battlestatus.unescaped());
		markups_changed = true;
	}

	if (battleinfo_markups.size() == 0)
	{
		ParseBattleMarkup(battleinfo_markups, sv_battleinfo.getDefault());
		markups_changed = true;
	}

	if (markups_changed)
	{
		battle_markups.clear();
		battle_markups.reserve(battleinfo_markups.size() + battlestatus_markups.size());
		battle_markups.insert(battle_markups.end(), battleinfo_markups.begin(), battleinfo_markups.end());
		battle_markups.insert(battle_markups.end(), battlestatus_markups.begin(), battlestatus_markups.end());
	}
}

//-----------------------------------------------------------------------------

void HU_DrawBattleMarkup(player_t* player)
{
	float const hud_scalebattleinfo = 1.0;

	int const border = 4;
	int max_width = 0;
	int height = border;

	for(BattleMarkupVector::iterator it = battle_markups.begin(); it != battle_markups.end(); ++it)
	{
		BattleMarkup& m = *it;

		switch (m.mt)
		{
		case MT_TEXT:
		case MT_TEXT_STAT:
			FormatBattleMarkupText(player, m);
			if (m.width > max_width)
				max_width = m.width;
			break;
		case MT_LINE:
			break;
		case MT_PATCH:
			break;
		case MT_IMAGE:
			break;
		case MT_LOCATION:
			break;
		case MT_BAR_STAT:
			break;
		}

		height += m.height + m.y_offset;
	}

	height += border;

	int const full_width = max_width + border * 2;

	hud::Dim(0, 0, full_width, height, hud_scalebattleinfo, hud::X_CENTER, hud::Y_MIDDLE, hud::X_CENTER, hud::Y_MIDDLE,
		argb_t(0xFF, 0xFF, 0xFF));

	int y = border - height / 2;

	for (BattleMarkupVector::iterator it = battle_markups.begin(); it != battle_markups.end(); ++it)
	{
		BattleMarkup& m = *it;

		switch (m.mt)
		{
		case MT_TEXT:
		case MT_TEXT_STAT:
			V_SetFont(m.font, m.font_fixed_width);
			hud::DrawText(0, y, hud_scalebattleinfo, hud::X_CENTER, hud::Y_MIDDLE, hud::X_CENTER, hud::Y_TOP,
				m.formatted_text.c_str(), m.color, true);
			break;
		case MT_LINE:
			hud::DrawLine(0, y, full_width, 0, hud_scalebattleinfo, hud::X_CENTER, hud::Y_MIDDLE, hud::X_CENTER,
				hud::Y_MIDDLE, HU_GetColorFromRange(m.color));
			break;
		case MT_PATCH:
			break;
		case MT_IMAGE:
			break;
		case MT_LOCATION:
			break;
		case MT_BAR_STAT:
			break;
		}

		y += m.height + m.y_offset;
	}

	std::string str;

	StrFormat(str, "Press " TEXTCOLOR_GOLD "%s" TEXTCOLOR_WHITE " for battle info",
		::Bindings.GetKeynameFromCommand("+battleinfo").c_str());

	hud::DrawText(0, y * CleanYfac, 0, hud::X_CENTER, hud::Y_MIDDLE, hud::X_CENTER, hud::Y_TOP, str.c_str(),
		CR_WHITE, true);
}

//-----------------------------------------------------------------------------

void HU_DrawBattleInfo(player_t* player)
{
	HU_ParseBattleInfo();
	HU_DrawBattleMarkup(player);
}

//-----------------------------------------------------------------------------

void HU_DrawBattleHud(player_t* player)
{
	float const hud_scalebattlehud = 1.0f;

	int const border = 4;
	int max_width = 0;
	int height = border;
	BattleMarkupAlignment align = BattleMarkupAlignmentLookup[ML_TOP_LEFT];

	if (stricmp(sv_battlehud.cstring(), last_sv_battlehud.c_str()))
	{
		last_sv_battlehud = sv_battlehud.cstring();
		ParseBattleMarkup(battlehud_markups, sv_battlehud.unescaped());
	}

	if (battlehud_markups.size() == 0)
		ParseBattleMarkup(battlehud_markups, sv_battlehud.getDefault());

	for(BattleMarkupVector::iterator it = battlehud_markups.begin(); it != battlehud_markups.end(); ++it)
	{
		BattleMarkup& m = *it;

		switch (m.mt)
		{
		case MT_TEXT:
		case MT_TEXT_STAT:
			FormatBattleMarkupText(player, m);
			if (m.width > max_width)
				max_width = m.width;
			break;
		case MT_LINE:
			break;
		case MT_PATCH:
			break;
		case MT_IMAGE:
			break;
		case MT_LOCATION:
			align = BattleMarkupAlignmentLookup[m.ml];
			break;
		case MT_BAR_STAT:
			break;
		}

		height += m.height + m.y_offset;
	}

	height += border;

	int const full_width = max_width + border * 2;

	int const above_ammo = 24;

	int y = border;

	// bottom of game window (needs to consider status bar)
	if (align.ya == hud::Y_BOTTOM)
		y = R_StatusBarVisible() ? HU_GetScaledStatusBarY() + border : above_ammo;
	else if (align.ya == hud::Y_MIDDLE)
		y = border - height / 2;

	//hud::Dim(0, 0, full_width, height, hud_scalebattlehud, align.xa, align.ya, align.xo, align.yo); // argb_t(0xFF, 0xFF, 0xFF)

	int const x = align.xa == hud::X_CENTER ? 0 : border;

	for (BattleMarkupVector::iterator it = battlehud_markups.begin(); it != battlehud_markups.end(); ++it)
	{
		BattleMarkup& m = *it;

		switch (m.mt)
		{
		case MT_TEXT:
		case MT_TEXT_STAT:
			V_SetFont(m.font, m.font_fixed_width);
			hud::DrawText(x, y, hud_scalebattlehud, align.xa, align.ya, align.xo, align.yo, m.formatted_text.c_str(), m.color, true);
			break;
		case MT_LINE:
			hud::DrawLine(x, y, full_width, 0, hud_scalebattlehud, align.xa, align.ya, align.xo, align.yo, HU_GetColorFromRange(m.color));
			break;
		case MT_PATCH:
			break;
		case MT_IMAGE:
			break;
		case MT_LOCATION:
			break;
		case MT_BAR_STAT:
			hud::EleBar(
				x, y, max_width, hud_scalebattlehud,
				align.xa, align.ya, align.xo, align.yo,
				GetBattleMarkupBarPercent(player, m.ms), CR_BRICK);
			break;
		}

		y += m.height + m.y_offset;
	}
}

//-----------------------------------------------------------------------------
