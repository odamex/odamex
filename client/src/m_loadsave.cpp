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
//   Load/save game screen and quick load/save functions.
//
//-----------------------------------------------------------------------------


#include "odamex.h"

#include <ctime>

#include "cl_responderkeys.h"
#include "g_game.h"
#include "gstrings.h"
#include "gi.h"
#include "m_fileio.h"
#include "m_loadsave.h"
#include "m_menu.h"
#include "m_menuconf.h"
#include "m_widgets.h"
#include "stringenums.h"
#include "v_text.h"
#include "v_video.h"
#include "w_wad.h"

extern int quickSaveSlot;

namespace
{
constexpr int SaveStringSize = 24;
constexpr int SaveSlotCount = 8;

enum class saveloadmode_t
{
	load,
	save
};

char gSaveStrings[10][SaveStringSize];
bool gSaveSlotOccupied[SaveSlotCount] = {};
saveloadmode_t gSaveLoadMode = saveloadmode_t::load;
int gLoadLastOn = 0;
int gSaveLastOn = 0;
int gSaveLoadX = 76;
int gSaveLoadY = 54;
bool gEditingSaveName = false;
int gEditingSlot = 0;
size_t gEditingCharIndex = 0;
char gOldSaveString[SaveStringSize];

int BigFontLineHeight()
{
	const OFont* font = OFonts.big();
	return font != nullptr ? font->lineHeight() : 0;
}

void ConfigureSaveLoadScreen()
{
	if (gameinfo.enginetype == ENGINE_HERETIC)
	{
		gSaveLoadX = 62;
		gSaveLoadY = 20;
	}
	else
	{
		gSaveLoadX = 76;
		gSaveLoadY = 54;
	}
}

void ReadSaveStrings()
{
	for (int i = 0; i < SaveSlotCount; ++i)
	{
		std::string name;
		G_BuildSaveName(name, i);

		auto handle = uqFile(fopen(name.c_str(), "rb"));
		if (handle == nullptr)
		{
			M_StringCopy(&gSaveStrings[i][0], GStrings(EMPTYSTRING), SaveStringSize);
			gSaveSlotOccupied[i] = false;
			continue;
		}

		const size_t readlen = fread(&gSaveStrings[i], SaveStringSize, 1, handle.get());
		if (readlen < 1)
		{
			fmt::print("M_Read_SaveStrings(): Failed to read handle.\n");
			return;
		}
		gSaveSlotOccupied[i] = true;
	}
}

void BeginSaveEdit(int slot)
{
	const time_t ti = time(nullptr);
	const tm* lt = localtime(&ti);

	gEditingSaveName = true;
	gEditingSlot = slot;
	M_StringCopy(gOldSaveString, gSaveStrings[slot], SaveStringSize);

#ifndef GCONSOLE
	if (!gSaveSlotOccupied[slot])
#endif
	{
		strncpy(gSaveStrings[slot], asctime(lt) + 4, 20);
	}

	gEditingCharIndex = strlen(gSaveStrings[slot]);
}

void ActivateSaveLoadSlot(int slot)
{
	if (gSaveLoadMode == saveloadmode_t::save)
	{
		BeginSaveEdit(slot);
	}
	else
	{
		M_LoadSaveLoadSlot(slot);
	}
}
} // namespace

void M_LoadSaveInit()
{
	ConfigureSaveLoadScreen();
}

void M_LoadSaveOpenLoad(int& currentItem)
{
	gSaveLoadMode = saveloadmode_t::load;
	ConfigureSaveLoadScreen();
	gEditingSaveName = false;
	ReadSaveStrings();
	currentItem = gLoadLastOn;
}

void M_LoadSaveOpenSave(int& currentItem)
{
	gSaveLoadMode = saveloadmode_t::save;
	ConfigureSaveLoadScreen();
	gEditingSaveName = false;
	ReadSaveStrings();
	currentItem = gSaveLastOn;
}

void M_LoadSaveRestore(int& currentItem)
{
	currentItem = gSaveLoadMode == saveloadmode_t::save ? gSaveLastOn : gLoadLastOn;
}

void M_LoadSaveDrawer(int currentItem, bool drawIndicator, int whichIndicator)
{
	const OFont* bigFont = OFonts.big();
	const OFont* smallFont = OFonts.small();
	const int slotWidth = 24;
	const int slotPadding = 2;
	const int slotHeight = BigFontLineHeight() - slotPadding;
	const bool saveMode = gSaveLoadMode == saveloadmode_t::save;
	const char* patchName = saveMode ? "M_SAVEG" : "M_LOADG";
	const char* titleKey = saveMode ? "MNU_SAVEGAME" : "MNU_LOADGAME";
	const patch_t* titlePatch =
	    W_CheckNumForName(patchName) >= 0 ? W_CachePatch(patchName) : nullptr;

	if (titlePatch != nullptr)
	{
		screen->DrawPatchClean(titlePatch, 72, 28);
	}
	else
	{
		const char* title = M_LocalizedMenuString(titleKey);
		if (title != nullptr && title[0] != '\0')
		{
			screen->DrawTextCleanMove(bigFont, CR_GRAY,
			                          160 - V_StringWidth(bigFont, title) / 2, 0, title);
		}
	}

	int listY = gSaveLoadY;
	for (int i = 0; i < SaveSlotCount; ++i)
	{
		M_DrawInputBox(gSaveStrings[i], gSaveLoadX, listY, slotWidth);
		listY += slotHeight + slotPadding;
	}

	if (gEditingSaveName)
	{
		const int stringWidth = V_StringWidth(smallFont, gSaveStrings[gEditingSlot]);
		screen->DrawTextCleanMove(smallFont, CR_RED, gSaveLoadX + stringWidth,
		                          gSaveLoadY + BigFontLineHeight() * gEditingSlot, "_");
	}

	if (drawIndicator && !gEditingSaveName)
	{
		if (const patch_t* indicator = M_MenuIndicatorPatch(whichIndicator))
		{
			const int drawX = gSaveLoadX + M_MenuIndicatorOffsetX();
			const int drawY = gSaveLoadY + M_MenuIndicatorOffsetY() +
			                  currentItem * BigFontLineHeight();
			screen->DrawPatchClean(indicator, drawX, drawY);
		}
	}
}

void M_LoadSaveLoadSlot(int slot)
{
	std::string name;
	G_BuildSaveName(name, slot);
	G_LoadGame(name);
	gamestate = gamestate == GS_FULLCONSOLE ? GS_HIDECONSOLE : gamestate;
	M_ClearMenus();
	if (quickSaveSlot == -2)
	{
		quickSaveSlot = slot;
	}
}

void M_LoadSaveSaveSlot(int slot)
{
	G_SaveGame(slot, { gSaveStrings[slot], SaveStringSize });
	M_ClearMenus();
	if (quickSaveSlot == -2)
	{
		quickSaveSlot = slot;
	}
}

const char* M_LoadSaveSlotName(int slot)
{
	return slot >= 0 && slot < SaveSlotCount ? gSaveStrings[slot] : "";
}

void M_LoadSaveResponder(int ch, int ch2, bool numlock, int& currentItem)
{
	const OFont* smallFont = OFonts.small();

	if (gEditingSaveName)
	{
		if (ch == OKEY_BACKSPACE)
		{
			if (gEditingCharIndex > 0)
			{
				--gEditingCharIndex;
				gSaveStrings[gEditingSlot][gEditingCharIndex] = 0;
			}
		}
		else if (Key_IsCancelKey(ch))
		{
			M_ClearMenus();
			gEditingSaveName = false;
			M_StringCopy(gSaveStrings[gEditingSlot], gOldSaveString, SaveStringSize);
		}
		else if (Key_IsAcceptKey(ch))
		{
			M_ClearMenus();
			gEditingSaveName = false;
			if (gSaveStrings[gEditingSlot][0])
			{
				M_LoadSaveSaveSlot(gEditingSlot);
			}
		}
		else if (ch2 >= 32 && ch2 <= 127 && gEditingCharIndex < SaveStringSize - 1 &&
		         V_StringWidth(smallFont, gSaveStrings[gEditingSlot]) <
		             (SaveStringSize - 1) * 8)
		{
			gSaveStrings[gEditingSlot][gEditingCharIndex++] = static_cast<char>(ch2);
			gSaveStrings[gEditingSlot][gEditingCharIndex] = 0;
		}

		return;
	}

	if (Key_IsDownKey(ch, numlock))
	{
		currentItem = (currentItem + 1) % SaveSlotCount;
		M_PlayMenuSound("navigate");
		return;
	}

	if (Key_IsUpKey(ch, numlock))
	{
		currentItem = currentItem > 0 ? currentItem - 1 : SaveSlotCount - 1;
		M_PlayMenuSound("navigate");
		return;
	}

	if (Key_IsAcceptKey(ch))
	{
		if (gSaveLoadMode == saveloadmode_t::save || gSaveSlotOccupied[currentItem])
		{
			if (gSaveLoadMode == saveloadmode_t::save)
			{
				gSaveLastOn = currentItem;
			}
			else
			{
				gLoadLastOn = currentItem;
			}

			ActivateSaveLoadSlot(currentItem);
			M_PlayMenuSound("select");
		}
		return;
	}

	if (Key_IsCancelKey(ch))
	{
		if (gSaveLoadMode == saveloadmode_t::save)
		{
			gSaveLastOn = currentItem;
		}
		else
		{
			gLoadLastOn = currentItem;
		}

		M_PopMenuStack();
		return;
	}

	if (ch2 >= '1' && ch2 <= '8')
	{
		currentItem = ch2 - '1';
		M_PlayMenuSound("navigate");
	}
}
