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

#include <optional>
#include <string>
#include <string_view>

#include "odamex.h"
#include "v_textcolors.h"

namespace menu::inputbox
{
	enum class response
	{
		none,
		changed,
		accept,
		cancel
	};

	void Draw(const char* text, int x, int y, int width, bool isEditing = false);
	response Respond(char* text, size_t textCapacity, size_t& cursor, int keyCode, int typedChar);
}

namespace menu::slider
{
	struct style
	{
		std::optional<argb_t> color;
		bool showValue = true;
	};

	void Draw(int x, int y, float leftval, float rightval, float cur, float step,
	          const style& style = {});
	float Respond(float cur, float leftval, float rightval, float step, int direction);
}

const patch_t* M_MenuConfConfiguredPatch(const std::string& name, const char* context);
void M_WarnMenuConf(const std::string& message);
const int M_BigFontLineHeight();
const int M_SmallFontLineHeight();
int M_MenuCursorOffsetY();
const patch_t* M_MenuCursor();
const patch_t* M_MenuIndicator(int which);
int M_MenuIndicatorOffsetX();
int M_MenuIndicatorOffsetY();
EColorRange M_MenuTextColor(std::string_view role, std::string_view menuId = {}, 
                            const std::string* overrideColor = nullptr);
