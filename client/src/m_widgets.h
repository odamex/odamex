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

#include <string>
#include <string_view>

#include "odamex.h"
#include "v_textcolors.h"

const patch_t* M_MenuConfConfiguredPatch(const std::string& name, const char* context);
void M_WarnMenuConf(const std::string& message);
int M_BigFontLineHeight();
int M_SmallFontLineHeight();
int M_MenuCursorOffsetY();
const patch_t* M_MenuCursorPatch();
const patch_t* M_MenuIndicatorPatch(int which);
int M_MenuIndicatorOffsetX();
int M_MenuIndicatorOffsetY();
EColorRange M_MenuTextColor(std::string_view role, std::string_view menuId = {}, 
                            const std::string* overrideColor = nullptr);
void M_DrawSlider(int x, int y, float leftval, float rightval, float cur, float step);
void M_DrawColoredSlider(int x, int y, float leftval, float rightval, float cur, argb_t color);
void M_DrawSaveLoadBorder(int x, int y, int len);
void M_DrawInputBox(char* text, int x, int y, int width);
