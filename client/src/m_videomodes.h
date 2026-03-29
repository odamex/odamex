#pragma once

#include "m_menu.h"

extern int testingmode;

void M_VideoModesInit();
void M_RestoreVideoMode();
void M_ModeFlashTestText();
void M_RefreshModesList();

void M_VideoModesOpen(int& currentItem);
void M_VideoModesRestore(int& currentItem);
void M_VideoModesDrawer(bool drawIndicator, int currentItem);
void M_VideoModesResponder(int ch, int ch2, bool numlock, int& currentItem);
