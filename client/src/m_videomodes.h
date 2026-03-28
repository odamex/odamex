#pragma once

#include "m_menu.h"

extern int testingmode;

void M_VideoModesInit();
void M_RestoreVideoMode();
void M_ModeFlashTestText();
void M_OpenVideoModeScreen(void);
void M_RefreshModesList();

menu_t* M_VideoModesMenu();
value_t* M_VideoModesDepths();
bool M_VideoModesIsTesting();
bool M_VideoModesOwnsMenu(const menu_t* menu);
void M_VideoModesDepthChanged();
bool M_VideoModesResponder(int ch, int ch2, bool numlock, menuitem_t* item, int& currentItem);
