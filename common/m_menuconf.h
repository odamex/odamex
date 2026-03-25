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
//-----------------------------------------------------------------------------

#pragma once

#include "odamex.h"

#include "m_jsonlump.h"

#include <unordered_map>
#include <vector>

enum class menuconfitemkind_t
{
	submenu,
	action,
	cvarDiscrete,
	cvarSlider,
	command,
	controlBinding,
	label,
	separator,
	dynamic
};

struct menuconfindicator_t
{
	std::vector<std::string> patches;
	int offsetX = 0;
	int offsetY = 0;
};

struct menuconfslider_t
{
	std::string leftPatch;
	std::string middlePatch;
	std::string rightPatch;
	std::string knobPatch;
	std::string greenKnobPatch;
	std::string overlayPatch;
};

struct menuconfinputbox_t
{
	std::string fullPatch;
	std::string leftPatch;
	std::string middlePatch;
	std::string rightPatch;
};

struct menuconfheadertside_t
{
	std::string basePatch;
	int frameCount = 0;
	int x = 0;
	int y = 0;
	std::string animateDirection;
};

struct menuconfheaderdecorations_t
{
	bool defined = false;
	menuconfheadertside_t left;
	menuconfheadertside_t right;
	int frameTics = 0;
};

struct menuconfheader_t
{
	std::string patch;
	std::string text;
	std::string languageKey;
	std::string align = "centered";
	int x = 0;
	int y = 0;
	menuconfheaderdecorations_t decorations;
};

struct menuconflayout_t
{
	std::string style;
	int x = 0;
	int y = 0;
	int indent = 0;
	std::string lineHeight = "auto";
	bool scroll = false;
	int topPadding = 0;
	std::string itemSpacing = "font";
};

struct menuconfitem_t
{
	menuconfitemkind_t kind = menuconfitemkind_t::separator;
	std::string id;
	std::string text;
	std::string languageKey;
	std::string textProvider;
	std::string patch;
	std::string target;
	std::string action;
	std::string hotkey;
	std::string sound;
	std::string color;
	std::string highlightColor;
	std::string help;
	std::string cvar;
	std::string values;
	std::string widget;
	std::string channel;
	std::string command;
	std::string bindingSet;
	std::string style;
	std::string provider;
	double min = 0.0;
	double max = 0.0;
	double step = 0.0;
	Json::Value params;
};

struct menuconfmenu_t
{
	menuconfheader_t header;
	menuconflayout_t layout;
	std::unordered_map<std::string, std::string> sounds;
	std::unordered_map<std::string, std::string> colors;
	std::vector<menuconfitem_t> items;
};

struct menuconftheme_t
{
	menuconfindicator_t indicator;
	std::string cursorPatch;
	int cursorOffsetY = 0;
	std::unordered_map<std::string, std::string> sounds;
	std::string upPatch;
	std::string downPatch;
	menuconfslider_t slider;
	menuconfinputbox_t inputBox;
	std::unordered_map<std::string, std::string> fonts;
	std::unordered_map<std::string, std::string> colors;
	menuconflayout_t layout;
};

struct menuconfdatabase_t
{
	menuconftheme_t theme;
	std::unordered_map<std::string, menuconfmenu_t> menus;
	std::unordered_map<std::string, std::string> entrypoints;

	void clear();
	void merge(const menuconfdatabase_t& other);
};

inline constexpr JSONLumpVersion MENUCONF_VERSION = {1, 0, 0};
inline constexpr std::string_view MENUCONF_LUMPTYPE = "menuconf";
inline constexpr std::string_view MENUCONF_BASE_LUMPNAME = "ODXMENU";
inline constexpr std::string_view MENUCONF_OVERRIDE_LUMPNAME = "MENUCONF";

jsonlumpresult_t M_ParseMenuConf(menuconfdatabase_t& out, int lumpindex);
void M_ClearMenuConf();
menuconfdatabase_t& M_MenuConf();
bool M_LoadMenuConf();
