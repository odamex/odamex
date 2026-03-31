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
constexpr int SaveSlotTimestampLength = 20;
constexpr int ScreenCenterX = 160;
constexpr int TitleY = 10;
constexpr int ListTopSpacing = 16;

enum class saveloadmode
{
	load,
	save
};

char saveStrings[10][SaveStringSize];
bool saveSlotOccupied[SaveSlotCount] = {};
saveloadmode saveLoadMode = saveloadmode::load;
int loadLastOn = 0;
int saveLastOn = 0;
bool editingSaveName = false;
int editingSlot = 0;
size_t editingCharIndex = 0;
char oldSaveString[SaveStringSize];

struct saveloadlayout_t
{
	int titleX = 0;
	int titleY = 0;
	int titleHeight = 0;
	int listX = 0;
	int listY = 0;
};

struct saveloadscreenstate_t
{
	const patch_t* titlePatch = nullptr;
	const char* titleText = nullptr;
	saveloadlayout_t layout;
};

constexpr int SaveLoadSlotWidth = 24;

saveloadscreenstate_t loadScreenState;
saveloadscreenstate_t saveScreenState;

bool IsPrintableMenuChar(int ch)
{
	return ch >= 32 && ch <= 127;
}

int SaveLoadBoxPixelWidth(int slotWidth)
{
	return slotWidth * M_SmallFontLineHeight();
}

saveloadlayout_t SaveLoadLayout(const patch_t* titlePatch, const char* title, int slotWidth)
{
	saveloadlayout_t layout;
	layout.titleY = TitleY;

	if (titlePatch != nullptr)
	{
		layout.titleX = ScreenCenterX - titlePatch->width() / 2;
		layout.titleHeight = titlePatch->height();
	}
	else if (title != nullptr && title[0] != '\0')
	{
		const OFont* bigFont = OFonts.big();
		layout.titleX = ScreenCenterX - V_StringWidth(bigFont, title) / 2;
		layout.titleHeight = M_BigFontLineHeight();
	}

	layout.listX = ScreenCenterX - SaveLoadBoxPixelWidth(slotWidth) / 2;
	layout.listY = layout.titleY + layout.titleHeight + ListTopSpacing;
	return layout;
}

saveloadscreenstate_t BuildSaveLoadScreenState(saveloadmode mode)
{
	saveloadscreenstate_t state;
	const bool saveMode = mode == saveloadmode::save;
	const char* patchName = saveMode ? "M_SAVEG" : "M_LOADG";
	const char* titleKey = saveMode ? "MNU_SAVEGAME" : "MNU_LOADGAME";

	state.titlePatch = W_CheckNumForName(patchName) >= 0 ? W_CachePatch(patchName) : nullptr;
	state.titleText = state.titlePatch == nullptr ? M_LocalizedMenuString(titleKey) : nullptr;
	state.layout = SaveLoadLayout(state.titlePatch, state.titleText, SaveLoadSlotWidth);
	return state;
}

const saveloadscreenstate_t& CurrentSaveLoadScreenState()
{
	return saveLoadMode == saveloadmode::save ? saveScreenState : loadScreenState;
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
			M_StringCopy(&saveStrings[i][0], GStrings(EMPTYSTRING), SaveStringSize);
			saveSlotOccupied[i] = false;
			continue;
		}

		const size_t readlen = fread(&saveStrings[i], SaveStringSize, 1, handle.get());
		if (readlen < 1)
		{
			fmt::print("M_Read_SaveStrings(): Failed to read handle.\n");
			return;
		}
		saveSlotOccupied[i] = true;
	}
}

void BeginSaveEdit(int slot)
{
	const time_t ti = time(nullptr);
	const tm* lt = localtime(&ti);

	editingSaveName = true;
	editingSlot = slot;
	M_StringCopy(oldSaveString, saveStrings[slot], SaveStringSize);

#ifndef GCONSOLE
	if (!saveSlotOccupied[slot])
#endif
	{
		strncpy(saveStrings[slot], asctime(lt) + 4, SaveSlotTimestampLength);
	}

	editingCharIndex = strlen(saveStrings[slot]);
}

void ActivateSaveLoadSlot(int slot)
{
	if (saveLoadMode == saveloadmode::save)
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
	loadScreenState = BuildSaveLoadScreenState(saveloadmode::load);
	saveScreenState = BuildSaveLoadScreenState(saveloadmode::save);
}

void M_LoadSaveOpenLoad(int& currentItem)
{
	saveLoadMode = saveloadmode::load;
	editingSaveName = false;
	ReadSaveStrings();
	currentItem = loadLastOn;
}

void M_LoadSaveOpenSave(int& currentItem)
{
	saveLoadMode = saveloadmode::save;
	editingSaveName = false;
	ReadSaveStrings();
	currentItem = saveLastOn;
}

void M_LoadSaveRestore(int& currentItem)
{
	currentItem = saveLoadMode == saveloadmode::save ? saveLastOn : loadLastOn;
}

void M_LoadSaveDrawer(int currentItem)
{
	const OFont* bigFont = OFonts.big();
	const OFont* smallFont = OFonts.small();
	const int slotPadding = 2;
	const int slotHeight = M_BigFontLineHeight() - slotPadding;
	const saveloadscreenstate_t& screenState = CurrentSaveLoadScreenState();
	const saveloadlayout_t& layout = screenState.layout;

	if (screenState.titlePatch != nullptr)
	{
		screen->DrawPatchClean(screenState.titlePatch, layout.titleX, layout.titleY);
	}
	else if (screenState.titleText != nullptr && screenState.titleText[0] != '\0')
	{
		screen->DrawTextCleanMove(
		    bigFont, CR_GRAY, layout.titleX, layout.titleY, screenState.titleText);
	}

	int listY = layout.listY;
	for (int i = 0; i < SaveSlotCount; ++i)
	{
		M_DrawInputBox(saveStrings[i], layout.listX, listY, SaveLoadSlotWidth);
		listY += slotHeight + slotPadding;
	}

	if (editingSaveName)
	{
		const int stringWidth = V_StringWidth(smallFont, saveStrings[editingSlot]);
		screen->DrawTextCleanMove(smallFont, CR_RED, layout.listX + stringWidth,
		                          layout.listY + M_BigFontLineHeight() * editingSlot, "_");
	}
}

bool M_LoadSaveIndicatorPosition(int currentItem, int& x, int& y)
{
	if (editingSaveName)
	{
		return false;
	}

	const saveloadlayout_t& layout = CurrentSaveLoadScreenState().layout;

	x = layout.listX + M_MenuIndicatorOffsetX();
	y = layout.listY + M_MenuIndicatorOffsetY() + currentItem * M_BigFontLineHeight();
	return true;
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
	G_SaveGame(slot, { saveStrings[slot], SaveStringSize });
	M_ClearMenus();
	if (quickSaveSlot == -2)
	{
		quickSaveSlot = slot;
	}
}

const char* M_LoadSaveSlotName(int slot)
{
	return slot >= 0 && slot < SaveSlotCount ? saveStrings[slot] : "";
}

void M_LoadSaveResponder(int keyCode, int typedChar, bool numlock, int& currentItem)
{
	const OFont* smallFont = OFonts.small();

	if (editingSaveName)
	{
		if (keyCode == OKEY_BACKSPACE)
		{
			if (editingCharIndex > 0)
			{
				--editingCharIndex;
				saveStrings[editingSlot][editingCharIndex] = 0;
			}
		}
		else if (Key_IsCancelKey(keyCode))
		{
			M_ClearMenus();
			editingSaveName = false;
			M_StringCopy(saveStrings[editingSlot], oldSaveString, SaveStringSize);
		}
		else if (Key_IsAcceptKey(keyCode))
		{
			M_ClearMenus();
			editingSaveName = false;
			if (saveStrings[editingSlot][0])
			{
				M_LoadSaveSaveSlot(editingSlot);
			}
		}
		else if (IsPrintableMenuChar(typedChar) && editingCharIndex < SaveStringSize - 1 &&
		         V_StringWidth(smallFont, saveStrings[editingSlot]) <
		             (SaveStringSize - 1) * 8)
		{
			saveStrings[editingSlot][editingCharIndex++] = static_cast<char>(typedChar);
			saveStrings[editingSlot][editingCharIndex] = 0;
		}

		return;
	}

	if (Key_IsDownKey(keyCode, numlock))
	{
		currentItem = (currentItem + 1) % SaveSlotCount;
		M_PlayMenuSound("navigate");
		return;
	}

	if (Key_IsUpKey(keyCode, numlock))
	{
		currentItem = currentItem > 0 ? currentItem - 1 : SaveSlotCount - 1;
		M_PlayMenuSound("navigate");
		return;
	}

	if (Key_IsAcceptKey(keyCode))
	{
		if (saveLoadMode == saveloadmode::save || saveSlotOccupied[currentItem])
		{
			if (saveLoadMode == saveloadmode::save)
			{
				saveLastOn = currentItem;
			}
			else
			{
				loadLastOn = currentItem;
			}

			ActivateSaveLoadSlot(currentItem);
			M_PlayMenuSound("select");
		}
		return;
	}

	if (Key_IsCancelKey(keyCode))
	{
		if (saveLoadMode == saveloadmode::save)
		{
			saveLastOn = currentItem;
		}
		else
		{
			loadLastOn = currentItem;
		}

		M_PopMenuStack();
		return;
	}

	if (typedChar >= '1' && typedChar < '1' + SaveSlotCount)
	{
		currentItem = typedChar - '1';
		M_PlayMenuSound("navigate");
	}
}
