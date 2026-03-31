// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// Copyright (C) 2006-2026 by The Odamex Team.
//
//-----------------------------------------------------------------------------

#pragma once

void M_LoadSaveInit();
void M_LoadSaveRestore(int& currentItem);
void M_LoadSaveDrawer(int currentItem);
bool M_LoadSaveIndicatorPosition(int currentItem, int& x, int& y);
void M_LoadSaveResponder(int keyCode, int typedChar, bool numlock, int& currentItem);
void M_QuickSaveResponse(int keyCode);
void M_QuickLoadResponse(int keyCode);
const char* M_LoadSaveSlotName(int slot);
void M_LoadSlot(int slot);
void M_OpenLoad(int& currentItem);
void M_OpenSave(int& currentItem);
void M_SaveSlot(int slot);
