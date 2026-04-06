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

#include "odamex.h"

#include <unordered_map>
#include <unordered_set>

#include "m_widgets.h"

#include "cl_responderkeys.h"
#include "i_video.h"
#include "m_menuconf.h"
#include "v_palette.h"
#include "v_text.h"
#include "v_video.h"
#include "w_wad.h"

namespace menu::inputbox
{
	namespace
	{
		const patch_t* FullPatch()
		{
			return M_MenuConfConfiguredPatch(M_MenuConfTheme().inputBox.fullPatch, "theme.inputBox.fullPatch");
		}
		const patch_t* LeftPatch()
		{
			return M_MenuConfConfiguredPatch(M_MenuConfTheme().inputBox.leftPatch, "theme.inputBox.leftPatch");
		}
		const patch_t* MiddlePatch()
		{
			return M_MenuConfConfiguredPatch(M_MenuConfTheme().inputBox.middlePatch, "theme.inputBox.middlePatch");
		}
		const patch_t* RightPatch()
		{
			return M_MenuConfConfiguredPatch(M_MenuConfTheme().inputBox.rightPatch, "theme.inputBox.rightPatch");
		}
	
		void DrawBox(int x, int y, int len)
		{
			const patch_t* fullSlot = FullPatch();
			const patch_t* leftSlot = LeftPatch();
			const patch_t* centerSlot = MiddlePatch();
			const patch_t* rightSlot = RightPatch();

			if (fullSlot != nullptr)
			{
				screen->DrawPatchClean(fullSlot, x, y);
				return;
			}

			screen->DrawPatchCleanNoOffsets(leftSlot, x, y);

			for (int i = 0; i < len; i++)
			{
				x += M_SmallFontLineHeight();
				screen->DrawPatchCleanNoOffsets(centerSlot, x, y);
			}

			screen->DrawPatchCleanNoOffsets(rightSlot, x, y);
		}
	}

	void Draw(const char* text, int x, int y, int width, bool isEditing)
	{
		const OFont* smallFont = OFonts.small();
		const int textY = y + (M_BigFontLineHeight() / 2 - M_SmallFontLineHeight() / 2);
		const std::string textColor = M_MenuConfTheme().inputBox.textColor;
		const std::string displayText = fmt::sprintf("%s%s", text, isEditing ? "_" : "");

		DrawBox(x, y, width);
		screen->DrawTextCleanMove(smallFont, TextColorFromString(textColor),
								x + (M_SmallFontLineHeight() / 2), textY,
								displayText.c_str());
	}

	response Respond(char* text, size_t textCapacity, size_t& cursor, int keyCode, int typedChar)
	{
		const OFont* smallFont = OFonts.small();
		if (text == nullptr || textCapacity == 0)
		{
			return response::none;
		}

		if (keyCode == OKEY_BACKSPACE)
		{
			if (cursor > 0)
			{
				--cursor;
				text[cursor] = '\0';
				return response::changed;
			}
		}

		if (Key_IsCancelKey(keyCode))
		{
			return response::cancel;
		}

		if (Key_IsAcceptKey(keyCode))
		{
			return response::accept;
		}

		if (Key_IsPrintableChar(typedChar) && cursor + 1 < textCapacity &&
			V_StringWidth(smallFont, text) < static_cast<int>(textCapacity - 1) * smallFont->lineHeight())
		{
			text[cursor++] = static_cast<char>(typedChar);
			text[cursor] = '\0';
			return response::changed;
		}

		return response::none;
	}
}

namespace menu::slider
{
	namespace
	{
		palindex_t FillColor(argb_t color)
			{
				using color_key_t = uint32_t;
				static std::unordered_map<color_key_t, palindex_t> fillCache;
				const color_key_t colorKey =
				    (static_cast<color_key_t>(color.getr()) << 16) |
				    (static_cast<color_key_t>(color.getg()) << 8) |
				    static_cast<color_key_t>(color.getb());
				auto fillIt = fillCache.find(colorKey);

				if (fillIt == fillCache.end())
				{
					fillIt = fillCache.emplace(colorKey,
					                           V_BestColor(V_GetDefaultPalette()->basecolors,
					                                       color))
					             .first;
				}

				return fillIt->second;
			}
		const patch_t* LeftPatch()
		{
			return M_MenuConfConfiguredPatch(M_MenuConfTheme().slider.leftPatch, "theme.slider.leftPatch");
		}
		const patch_t* MiddlePatch()
		{
			return M_MenuConfConfiguredPatch(M_MenuConfTheme().slider.middlePatch, "theme.slider.middlePatch");
		}
		const patch_t* RightPatch()
		{
			return M_MenuConfConfiguredPatch(M_MenuConfTheme().slider.rightPatch, "theme.slider.rightPatch");
		}
		const patch_t* KnobPatch()
		{
			return M_MenuConfConfiguredPatch(M_MenuConfTheme().slider.knobPatch, "theme.slider.knobPatch");
		}
		const patch_t* GreenKnobPatch()
		{
			return M_MenuConfConfiguredPatch(M_MenuConfTheme().slider.greenKnobPatch, "theme.slider.greenKnobPatch");
		}
		const patch_t* OverlayPatch()
		{
			return M_MenuConfConfiguredPatch(M_MenuConfTheme().slider.overlayPatch, "theme.slider.overlayPatch");
		}
	}
	void Draw(int x, int y, float leftval, float rightval, float cur, float step,
	          const style& style)
	{
		const OFont* smallFont = OFonts.small();
		const palette_t* palette = V_GetPaletteFromLump("ODAPAL");
		const int drawY = y + menu::cursor::OffsetY();

		cur = leftval < rightval ? clamp(cur, leftval, rightval) : clamp(cur, rightval, leftval);
		const float dist = (cur - leftval) / (rightval - leftval);

		screen->DrawPatchCleanWithPalette(LeftPatch(), x, drawY, palette);
		for (int i = 1; i < 11; i++)
		{
			screen->DrawPatchCleanWithPalette(MiddlePatch(), x + i * 8, drawY, palette);
		}

		screen->DrawPatchCleanWithPalette(RightPatch(), x + 88, drawY, palette);
		screen->DrawPatchCleanWithPalette(style.color.has_value() ? GreenKnobPatch() :
		                                                       KnobPatch(),
										x + 5 + static_cast<int>(dist * 78.0), drawY,
										palette);

		if (style.color.has_value())
		{
			V_ColorFill = FillColor(*style.color);
			screen->DrawColoredPatchClean(
			    OverlayPatch(), x + 5 + static_cast<int>(dist * 78.0), drawY);
		}

		if (!style.showValue || step == 0.0f)
		{
			return;
		}

		std::string buf;
		if (step >= 1.0f)
		{
			buf = fmt::sprintf("%.0f", cur);
		}
		else if (step >= 0.1f)
		{
			buf = fmt::sprintf("%.1f", cur);
		}
		else
		{
			buf = fmt::sprintf("%.2f", cur);
		}
		screen->DrawTextCleanMove(smallFont, CR_GREEN, x + 96, y, buf.c_str());
	}

	float Respond(float cur, float leftval, float rightval, float step, int direction)
	{
		const float newval = cur + static_cast<float>(direction) * step;
		return leftval < rightval ?
				clamp(newval, leftval, rightval) :
				clamp(newval, rightval, leftval);
	}
}

namespace menu::cursor
{
	namespace
	{
		int OffsetY()
		{
			return M_MenuConfTheme().cursorOffsetY;
		}
	}

	const patch_t* Patch()
	{
		static std::string cachedName;
		static const patch_t* cachedPatch = nullptr;
		const std::string& configuredName = M_MenuConfTheme().cursorPatch;
		if (cachedName != configuredName)
		{
			cachedName = configuredName;
			cachedPatch =
			    M_MenuConfConfiguredPatch(M_MenuConfTheme().cursorPatch, "theme.cursorPatch");
		}
		return cachedPatch;
	}
}

namespace menu::indicator
{
	namespace
	{
		int OffsetX()
		{
			return M_MenuConfTheme().indicator.offsetX;
		}

		int OffsetY()
		{
			return M_MenuConfTheme().indicator.offsetY;
		}
	}

	const patch_t* Patch(int which)
	{
		const auto& patches = M_MenuConfTheme().indicator.patches;
		if (patches.empty())
		{
			return nullptr;
		}

		const std::string& name = patches[which % patches.size()];
		return M_MenuConfConfiguredPatch(name, "theme.indicator.patches");
	}
}


const int M_BigFontLineHeight()
{
	const OFont* font = OFonts.big();
	return font != nullptr ? font->lineHeight() : 0;
}

const int M_SmallFontLineHeight()
{
	const OFont* font = OFonts.small();
	return font != nullptr ? font->lineHeight() : 0;
}

const patch_t* M_MenuConfConfiguredPatch(const std::string& name, const char* context)
{
	if (name.empty())
	{
		return nullptr;
	}

	const patch_t* patch = !name.empty() && W_CheckNumForName(name.c_str()) >= 0 ? W_CachePatch(name.c_str()) : nullptr;
	if (patch == nullptr)
	{
		M_WarnMenuConfOnce(fmt::sprintf("%s references missing patch \"%s\"", context, name.c_str()));
	}

	return patch;
}

const menuconftheme_t& M_MenuConfTheme()
{
	return M_MenuConf().theme;
}

void M_WarnMenuConf(const std::string& message)
{
	PrintFmt(PRINT_WARNING, "MENUCONF: {}\n", message);
}

void M_WarnMenuConfOnce(const std::string& message)
{
	static std::unordered_set<std::string> warnedMessages;
	if (warnedMessages.insert(message).second)
	{
		M_WarnMenuConf(message);
	}
}

EColorRange M_MenuTextColor(std::string_view role, std::string_view menuId,
							const std::string* overrideColor)
{
	if (overrideColor != nullptr && !overrideColor->empty())
	{
		return TextColorFromString(*overrideColor);
	}

	const std::string roleKey(role);
	if (!menuId.empty())
	{
		const auto menuIt = M_MenuConf().menus.find(std::string(menuId));
		if (menuIt != M_MenuConf().menus.end())
		{
			const auto colorIt = menuIt->second.colors.find(roleKey);
			if (colorIt != menuIt->second.colors.end() && !colorIt->second.empty())
			{
				return TextColorFromString(colorIt->second);
			}
		}
	}

	const auto themeIt = M_MenuConfTheme().colors.find(roleKey);
	if (themeIt != M_MenuConfTheme().colors.end() && !themeIt->second.empty())
	{
		return TextColorFromString(themeIt->second);
	}

	return CR_GRAY;
}
