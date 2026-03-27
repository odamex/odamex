// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// Copyright (C) 2006-2026 by The Odamex Team.
//
//-----------------------------------------------------------------------------

#pragma once

void M_LoadSaveInit();
void M_LoadSaveOpenLoad(int& currentItem);
void M_LoadSaveOpenSave(int& currentItem);
void M_LoadSaveRestore(int& currentItem);
void M_LoadSaveDrawer(int currentItem, bool drawIndicator, int whichIndicator);
void M_LoadSaveResponder(int ch, int ch2, bool numlock, int& currentItem);
void M_LoadSaveLoadSlot(int slot);
void M_LoadSaveSaveSlot(int slot);
const char* M_LoadSaveSlotName(int slot);
