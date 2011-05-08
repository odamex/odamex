// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2006-2010 by The Odamex Team.
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
//	User interface
//
// AUTHORS:
//	 Michael Wood (mwoodj at huntsvegas dot org)
//
//-----------------------------------------------------------------------------

#include <iostream>
#include <string>
#include <algorithm>
#include <sstream>

#include <agar/core.h>
#include <agar/gui.h>

#include <SDL_image.h>

#include "agol_main.h"
#include "agol_settings.h"
#include "agol_solo.h"
#include "agol_about.h"
#include "agol_manual.h"
#include "game_command.h"
#include "gui_config.h"
#include "typedefs.h"
#include "icons.h"

#ifdef _XBOX
#include "xbox_main.h"
#endif

using namespace std;

namespace agOdalaunch {

AGOL_MainWindow::AGOL_MainWindow(int width, int height) :
	SettingsDialog(NULL), SoloGameDialog(NULL), AboutDialog(NULL),
	ManualDialog(NULL), QServer(NULL), WindowExited(false)
{
	// Create the Agar window. If we are using a single-window display driver (sdlfb, sdlgl) 
	// make the window plain (no window decorations). No flags for multi-window drivers (glx, wgl)
	MainWindow = AG_WindowNew(agDriverSw ? AG_WINDOW_PLAIN : 0);
	AG_WindowSetGeometryAligned(MainWindow, AG_WINDOW_MC, width, height);
	AG_WindowSetCaptionS(MainWindow, "The Odamex Launcher");

	// Create the components of the main window
	MainMenu = CreateMainMenu(MainWindow);
	MainButtonBox = CreateMainButtonBox(MainWindow);
	MainListPane = CreateMainListPane(MainWindow);
	BottomListPane = CreateBottomListPane(MainListPane->div[1]);
	ServerList = CreateServerList(MainListPane->div[0]);
	PlayerList = CreatePlayerList(BottomListPane->div[0]);
	ServInfoList = CreateServInfoList(BottomListPane->div[1]);
	MainStatusbar = CreateMainStatusbar(MainWindow);

	// If this is a single-window display driver (sdlgl, sdlfb) maximize the main window
	// within the single window space.
	if(agDriverSw)
		AG_WindowMaximize(MainWindow);

	// Set the window close action
	AG_AddEvent(MainWindow, "window-close", EventReceiver, "%p", 
		RegisterEventHandler((EVENT_FUNC_PTR)&AGOL_MainWindow::SaveWidgetStates));

	// set up the master server information
	MServer.AddMaster("master1.odamex.net", 15000);
	MServer.AddMaster("voxelsoft.com", 15000);

	// Don't poll the server list by default
	StopServerListPoll();

	// If query master on start is configured post the event.
	GuiConfig::Read("MasterOnStart", (uint8_t&)StartupQuery);
	if(StartupQuery)
		AG_PostEvent(MainWindow, MainButtonBox->mlist, "button-pushed", NULL);

	// Show the window
	AG_WindowShow(MainWindow);
}

AGOL_MainWindow::~AGOL_MainWindow()
{
	// If the window exit action has not been performed by this time
	// then the driver being used is a single-window driver that does
	// not trigger the exit action when the window is closed (due to 
	// the fact that the close button is actually for the driver window
	// itself.) In that case it should be safe to save the widget states
	// because the window is actually still valid. In all other cases
	// the contents would be invalid and this would cause a crash!
	if(!WindowExited)
		SaveWidgetStates();

	delete[] QServer;

	delete MainStatusbar;
	delete MainButtonBox;
}

AG_Menu *AGOL_MainWindow::CreateMainMenu(void *parent)
{
	AG_Menu     *menu;
	AG_MenuItem *m;

	// Menu bar
	menu = AG_MenuNew(parent, AG_MENU_HFILL);

	// File menu
	m = AG_MenuNode(menu->root, "File", NULL);
	AG_MenuAction(m, "Settings", agIconGear.s, EventReceiver, "%p", 
			RegisterEventHandler((EVENT_FUNC_PTR)&AGOL_MainWindow::OnOpenSettingsDialog));
	AG_MenuSeparator(m);
	AG_MenuActionKb(m, "Exit", agIconClose.s, AG_KEY_Q, AG_KEYMOD_CTRL, 
			EventReceiver, "%p", RegisterEventHandler((EVENT_FUNC_PTR)&AGOL_MainWindow::OnExit));

	// Action menu
	m = AG_MenuNode(menu->root, "Action", NULL);
	AG_MenuAction(m, "Launch", NULL, EventReceiver, "%p", 
			RegisterEventHandler((EVENT_FUNC_PTR)&AGOL_MainWindow::OnLaunch));
	AG_MenuAction(m, "Run Offline", NULL, EventReceiver, "%p", 
			RegisterEventHandler((EVENT_FUNC_PTR)&AGOL_MainWindow::OnOfflineLaunch));
	AG_MenuSeparator(m);
	AG_MenuAction(m, "Refresh Selected", NULL, EventReceiver, "%p", 
			RegisterEventHandler((EVENT_FUNC_PTR)&AGOL_MainWindow::OnRefreshSelected));
	AG_MenuAction(m, "Refresh All", NULL, EventReceiver, "%p", 
			RegisterEventHandler((EVENT_FUNC_PTR)&AGOL_MainWindow::OnRefreshAll));
	AG_MenuAction(m, "Get Master List", NULL, EventReceiver, "%p", 
			RegisterEventHandler((EVENT_FUNC_PTR)&AGOL_MainWindow::OnGetMasterList));

	// Tools Menu
	m = AG_MenuNode(menu->root, "Tools", NULL);
	m = AG_MenuNode(m, "OdaGet", NULL);
	AG_MenuDisable(m);
	AG_MenuAction(m, "Get WAD", NULL, EventReceiver, "%p", 
			RegisterEventHandler((EVENT_FUNC_PTR)&AGOL_MainWindow::OnGetWAD));
	AG_MenuAction(m, "Configure", NULL, EventReceiver, "%p", 
			RegisterEventHandler((EVENT_FUNC_PTR)&AGOL_MainWindow::OnOdaGetConfig));
	AG_MenuEnable(m);

	// Advanced menu
	m = AG_MenuNode(menu->root, "Advanced", NULL);
	AG_MenuAction(m, "Manual Connect", NULL, EventReceiver, "%p", 
			RegisterEventHandler((EVENT_FUNC_PTR)&AGOL_MainWindow::OnManualConnect));
	AG_MenuDisable(m);
	AG_MenuAction(m, "Custom Servers", NULL, EventReceiver, "%p", 
			RegisterEventHandler((EVENT_FUNC_PTR)&AGOL_MainWindow::OnCustomServer));
	AG_MenuEnable(m);
	
	// Help menu
	m = AG_MenuNode(menu->root, "Help", NULL);
	AG_MenuAction(m, "Report Bug", NULL, EventReceiver, "%p", 
			RegisterEventHandler((EVENT_FUNC_PTR)&AGOL_MainWindow::OnReportBug));
	AG_MenuSeparator(m);
	AG_MenuAction(m, "About", NULL, EventReceiver, "%p", 
			RegisterEventHandler((EVENT_FUNC_PTR)&AGOL_MainWindow::OnAbout));

	return menu;
}

ODA_ButtonBox *AGOL_MainWindow::CreateMainButtonBox(void *parent)
{
	ODA_ButtonBox *bbox;

	bbox = new ODA_ButtonBox;

	bbox->buttonbox = AG_BoxNewHoriz(parent, AG_BOX_HFILL);

	bbox->launch = CreateButton(bbox->buttonbox, "Launch", odalaunchico, 
			sizeof(odalaunchico), (EVENT_FUNC_PTR)&AGOL_MainWindow::OnLaunch);
	bbox->qlaunch = CreateButton(bbox->buttonbox, "Quick Launch", odaqlaunchico, 
			sizeof(odaqlaunchico), (EVENT_FUNC_PTR)&AGOL_MainWindow::OnOfflineLaunch);

	AG_SeparatorNewVert(bbox->buttonbox);

	bbox->refresh = CreateButton(bbox->buttonbox, "Refresh Selected", btnrefreshico, 
			sizeof(btnrefreshico), (EVENT_FUNC_PTR)&AGOL_MainWindow::OnRefreshSelected);
	bbox->refreshall = CreateButton(bbox->buttonbox, "Refresh All", btnrefreshallico, 
			sizeof(btnrefreshico), (EVENT_FUNC_PTR)&AGOL_MainWindow::OnRefreshAll);
	bbox->mlist = CreateButton(bbox->buttonbox, "Query Master", btnlistico, 
			sizeof(btnlistico), (EVENT_FUNC_PTR)&AGOL_MainWindow::OnGetMasterList);

	AG_SeparatorNewVert(bbox->buttonbox);

	bbox->settings = CreateButton(bbox->buttonbox, "Settings", btnsettingsico, 
			sizeof(btnsettingsico), (EVENT_FUNC_PTR)&AGOL_MainWindow::OnOpenSettingsDialog);
	bbox->about = CreateButton(bbox->buttonbox, "About", btnhelpico, 
			sizeof(btnhelpico), (EVENT_FUNC_PTR)&AGOL_MainWindow::OnAbout);

	AG_SeparatorNewVert(bbox->buttonbox);

	bbox->exit = CreateButton(bbox->buttonbox, "Exit", btnexitico, 
			sizeof(btnexitico), (EVENT_FUNC_PTR)&AGOL_MainWindow::OnExit);

	return bbox;
}

AG_Pane *AGOL_MainWindow::CreateMainListPane(void *parent)
{
	AG_Pane *pane;

	pane = AG_PaneNewVert(parent, AG_PANE_EXPAND);

	AG_PaneResizeAction(pane, AG_PANE_EXPAND_DIV1);
	AG_PaneMoveDividerPct(pane, 60);
	AG_BoxSetPadding(pane->div[0], 1);
	AG_BoxSetPadding(pane->div[1], 1);

	return pane;
}

AG_Pane *AGOL_MainWindow::CreateBottomListPane(void *parent)
{
	AG_Pane *pane;

	pane = AG_PaneNewHoriz(parent, AG_PANE_EXPAND);

	AG_PaneResizeAction(pane, AG_PANE_EXPAND_DIV1);
	AG_BoxSetPadding(pane->div[0], 1);
	AG_BoxSetPadding(pane->div[1], 1);

	return pane;
}

AG_Table *AGOL_MainWindow::CreateServerList(void *parent)
{
	AG_Table      *list;
	ostringstream  colSzSpec[8];
	int            colW[8] = { 200, 33, 48, 100, 47, 105, 72, 125 };

	list = AG_TableNewPolled(parent, AG_TABLE_EXPAND, EventReceiver, "%p", 
			RegisterEventHandler((EVENT_FUNC_PTR)&AGOL_MainWindow::UpdateServerList));

  	AG_TableSetRowDblClickFn(list, EventReceiver, "%p", 
			RegisterEventHandler((EVENT_FUNC_PTR)&AGOL_MainWindow::OnLaunch));

	AG_SetEvent(list, "row-selected", EventReceiver, "%p", 
			RegisterEventHandler((EVENT_FUNC_PTR)&AGOL_MainWindow::OnServerListRowSelected));

	// Configure each columns size spec
	for(int i = 0; i < 8; i++)
	{
		ostringstream colOption;
		int           width;

		colOption << "SrvListColW_" << i;

		// Check for a previously stored column size
		if(!GuiConfig::Read(colOption.str(), width) && width > 0)
			colW[i] = width;

		colSzSpec[i] << colW[i] << "px";
	}

	AG_TableAddCol(list, "Server Name", colSzSpec[0].str().c_str(), NULL);
	AG_TableAddCol(list, "Ping", colSzSpec[1].str().c_str(), NULL);
	AG_TableAddCol(list, "Players", colSzSpec[2].str().c_str(), &AGOL_MainWindow::CellCompare);
	AG_TableAddCol(list, "WADs", colSzSpec[3].str().c_str(), NULL);
	AG_TableAddCol(list, "Map", colSzSpec[4].str().c_str(), NULL);
	AG_TableAddCol(list, "Type", colSzSpec[5].str().c_str(), NULL);
	AG_TableAddCol(list, "Game IWAD", colSzSpec[6].str().c_str(), NULL);
	AG_TableAddCol(list, "Address : Port", colSzSpec[7].str().c_str(), NULL);

	return list;
}

AG_Table *AGOL_MainWindow::CreatePlayerList(void *parent)
{
	AG_Table *list;

	list = AG_TableNew(parent, AG_TABLE_EXPAND);

	AG_WidgetSetFocusable(list, 0);

	AG_TableAddCol(list, "Player Name", "175px", NULL);
	AG_TableAddCol(list, "Ping", "<  Ping  >", NULL);
	AG_TableAddCol(list, "Time", "<  Time  >", NULL);
	AG_TableAddCol(list, "Frags", "<  Frags  >", NULL);
	AG_TableAddCol(list, "Kill Count", "<  Kill Count  >", NULL);
	AG_TableAddCol(list, "Death Count", "<  Death Count  >", NULL);

	return list;
}

AG_Table *AGOL_MainWindow::CreateServInfoList(void *parent)
{
	AG_Table *list;

	list = AG_TableNew(parent, AG_TABLE_EXPAND);

	AG_TableSetColumnAction(list, AG_TABLE_COL_SELECT);
	AG_WidgetSetFocusable(list, 0);

	AG_TableAddCol(list, "Server Details", "-", NULL);

	return list;
}

ODA_Statusbar *AGOL_MainWindow::CreateMainStatusbar(void *parent)
{
	ODA_Statusbar *statusbar;
//	AG_Separator  *sep;

	statusbar = new ODA_Statusbar;

	statusbar->statbox = AG_BoxNewHoriz(parent, AG_BOX_HOMOGENOUS | AG_BOX_HFILL);

	statusbar->tooltip = AG_StatusbarNew(statusbar->statbox, AG_STATUSBAR_EXPAND);
	AG_StatusbarAddLabel(statusbar->tooltip, AG_LABEL_STATIC, "Welcome to Odamex");

	// These separators are goofy inside a homogenous box.
	//AG_SeparatorNewVert(statusbar->statbox);

	statusbar->mping = AG_StatusbarNew(statusbar->statbox, AG_STATUSBAR_EXPAND);
	AG_StatusbarAddLabel(statusbar->mping, AG_LABEL_STATIC, "Master Ping: 0");

	//AG_SeparatorNewVert(statusbar->statbox);

	statusbar->queried.statusbar = AG_StatusbarNew(statusbar->statbox, AG_STATUSBAR_EXPAND);
	statusbar->queried.completed = statusbar->queried.total = 0;
	AG_MutexInit(&statusbar->queried.mutex);
	AG_StatusbarAddLabel(statusbar->queried.statusbar, AG_LABEL_POLLED_MT, "Queried Servers %i of %i", 
			&statusbar->queried.mutex, &statusbar->queried.completed, &statusbar->queried.total);

	//AG_SeparatorNewVert(statusbar->statbox);

	statusbar->players.statusbar = AG_StatusbarNew(statusbar->statbox, AG_STATUSBAR_EXPAND);
	statusbar->players.numplayers = 0;
	AG_MutexInit(&statusbar->players.mutex);
	AG_StatusbarAddLabel(statusbar->players.statusbar, AG_LABEL_POLLED_MT, "Total Players: %i",
			&statusbar->players.mutex, &statusbar->players.numplayers);

	return statusbar;
}


//*********************************//
// Interface Interaction Functions //
//*********************************//
void AGOL_MainWindow::SaveWidgetStates()
{
	// Save window dimensions
	GuiConfig::Write("MainWindow-Width", MainWindow->r.w);
	GuiConfig::Write("MainWindow-Height", MainWindow->r.h);

	// Save server list column sizes
	for(int i = 0; i < ServerList->n; i++)
	{
		ostringstream colOption;

		colOption << "SrvListColW_" << i;
		GuiConfig::Write(colOption.str(), ServerList->cols[i].w);
	}

	WindowExited = true;
}

void AGOL_MainWindow::UpdateStatusbarTooltip(const char *tip)
{
	if(tip)
		AG_LabelTextS(MainStatusbar->tooltip->labels[0], tip);
}

void AGOL_MainWindow::ClearStatusbarTooltip()
{
	AG_LabelTextS(MainStatusbar->tooltip->labels[0], "");
}

void AGOL_MainWindow::UpdateStatusbarMasterPing(uint32_t ping)
{
	AG_LabelText(MainStatusbar->mping->labels[0], "Master Ping: %u", ping);
}

AG_Button *AGOL_MainWindow::CreateButton(void *parent, const char *label, 
                                         const unsigned char *icon, int iconsize, 
                                         EVENT_FUNC_PTR handler)
{
	AG_Button   *button;
	SDL_Surface *sf;

	button = AG_ButtonNewFn(parent, 0, label, EventReceiver, "%p", RegisterEventHandler(handler));

	AG_SetEvent(button, "button-mouseoverlap", EventReceiver, "%p", 
			RegisterEventHandler((EVENT_FUNC_PTR)&AGOL_MainWindow::OnMouseOverWidget));

	sf = IMG_Load_RW(SDL_RWFromConstMem(icon, iconsize), 1);
	if(sf)
		AG_ButtonSurface(button, AG_SurfaceFromSDL(sf));
	else
		cerr << "Failed to load icon: " << IMG_GetError() << endl;

	return button;
}

int AGOL_MainWindow::GetSelectedServerListRow()
{
	// Loop until a selected row is found
	for(int row = 0; row < ServerList->m; row++)
		if(AG_TableRowSelected(ServerList, row))
			return row;

	return -1;
}

int AGOL_MainWindow::GetSelectedServerArrayIndex()
{
	int    row;
	size_t serverCount;
	string cellAddr;

	// Get the selected list row index
	row = GetSelectedServerListRow();

	// No selection
	if(row < 0)
		return -1;

	cellAddr = GetAddrFromServerListRow(row);

	if(!cellAddr.size())
		return -1;

	// Get the number of servers
	MServer.GetLock();
	serverCount = MServer.GetServerCount();
	MServer.Unlock();

	// No servers
	if(serverCount <= 0)
		return -1;

	// Loop until the selected server array is found
	for(size_t i = 0; i < serverCount; i++)
	{
		string srvAddr;

		QServer[i].GetLock();
		srvAddr = QServer[i].GetAddress();
		QServer[i].Unlock();

		if(srvAddr == cellAddr)
			return i;
	}

	return -1;
}

string AGOL_MainWindow::GetAddrFromServerListRow(int row)
{
	AG_TableCell *cell = NULL;

	cell = AG_TableGetCell(ServerList, row, 7);

	// Row has no address field data
	if(!cell || !cell->data.s)
		return "";

	return string(cell->data.s);
}

int AGOL_MainWindow::GetServerListRowFromAddr(const string &address)
{
	// Loop until the server address is found
	for(int row = 0; row < ServerList->m; row++)
	{
		string cellAddr;

		cellAddr = GetAddrFromServerListRow(row);
		
		if(!cellAddr.size())
			continue;

		if(address == cellAddr)
			return row;
	}

	return -1;
}

int AGOL_MainWindow::GetServerArrayIndexFromListRow(int row)
{
	size_t serverCount;
	string cellAddr;

	// No selection
	if(row < 0)
		return -1;

	cellAddr = GetAddrFromServerListRow(row);

	if(!cellAddr.size())
		return -1;

	// Get the number of servers
	MServer.GetLock();
	serverCount = MServer.GetServerCount();
	MServer.Unlock();

	// No servers
	if(serverCount <= 0)
		return -1;

	// Loop until the selected server array is found
	for(size_t i = 0; i < serverCount; i++)
	{
		string srvAddr;

		QServer[i].GetLock();
		srvAddr = QServer[i].GetAddress();
		QServer[i].Unlock();

		if(srvAddr == cellAddr)
			return i;
	}

	return -1;
}

void AGOL_MainWindow::ClearList(AG_Table *table)
{
	if(!table)
		return;

	AG_TableBegin(table);
	AG_TableEnd(table);
}

void AGOL_MainWindow::UpdatePlayerList(int serverNdx)
{
	// Always start with an empty list
	ClearList(PlayerList);

	// No server selected
	if(serverNdx < 0)
		return;

	QServer[serverNdx].GetLock();

	if(QServer[serverNdx].Info.Players.size())
	{
		for(size_t i = 0; i < QServer[serverNdx].Info.Players.size(); i++)
		{
			string name = " ";
	
			if(QServer[serverNdx].Info.Players[i].Name.size())
				name = QServer[serverNdx].Info.Players[i].Name;

			AG_TableAddRow(PlayerList, "%s:%u:%u:%i:%u:%u", name.c_str(), 
			                     QServer[serverNdx].Info.Players[i].Ping,
			                     QServer[serverNdx].Info.Players[i].Time,
			                     QServer[serverNdx].Info.Players[i].Frags,
			                     QServer[serverNdx].Info.Players[i].Kills,
			                     QServer[serverNdx].Info.Players[i].Deaths);
		}
	}

	QServer[serverNdx].Unlock();
}

void AGOL_MainWindow::UpdateServInfoList(int serverNdx)
{
	// Always start with an empty list
	ClearList(ServInfoList);

	// No server selected
	if(serverNdx < 0)
		return;

	QServer[serverNdx].GetLock();

	if(QServer[serverNdx].Info.Cvars.size())
	{
		ostringstream rowStream;

		// Version
		rowStream << "Version " << (int)QServer[serverNdx].Info.VersionMajor << "." <<
		                           (int)QServer[serverNdx].Info.VersionMinor << "." <<
		                           (int)QServer[serverNdx].Info.VersionPatch << "-r" <<
		                           (int)QServer[serverNdx].Info.VersionRevision;
		AG_TableAddRow(ServInfoList, "%s", rowStream.str().c_str());
		rowStream.str("");

		// Query Protocol Version
		rowStream << "QP Version " << QServer[serverNdx].Info.VersionProtocol;
		AG_TableAddRow(ServInfoList, "%s", rowStream.str().c_str());
		rowStream.str("");

		AG_TableAddRow(ServInfoList, "%s", "");

		// Status
		AG_TableAddRow(ServInfoList, "%s", "Game Status");

		rowStream << "Time Left " << QServer[serverNdx].Info.TimeLeft;
		AG_TableAddRow(ServInfoList, "%s", rowStream.str().c_str());
		rowStream.str("");

		if(QServer[serverNdx].Info.GameType == GT_TeamDeathmatch ||
			QServer[serverNdx].Info.GameType == GT_CaptureTheFlag)
		{
			rowStream << "Score Limit " << QServer[serverNdx].Info.ScoreLimit;
			AG_TableAddRow(ServInfoList, "%s", rowStream.str().c_str());
			rowStream.str("");
		}

		AG_TableAddRow(ServInfoList, "%s", "");

		// Patch (BEX/DEH) files
		AG_TableAddRow(ServInfoList, "%s", "BEX/DEH Files");

		if(QServer[serverNdx].Info.Patches.size() <= 0)
			AG_TableAddRow(ServInfoList, "%s", "None");
		else
		{
			size_t patchCnt = QServer[serverNdx].Info.Patches.size();
			size_t i = 0;

			while(i < patchCnt)
			{
				string patch = QServer[serverNdx].Info.Patches[i];

				++i;

				if(i < patchCnt)
					patch += " " + QServer[serverNdx].Info.Patches[i];

				++i;

				AG_TableAddRow(ServInfoList, "%s", patch.c_str());
			}
		}

		AG_TableAddRow(ServInfoList, "%s", "");

		// Gameplay variables (Cvars, others)
		AG_TableAddRow(ServInfoList, "%s", "Game Settings");

		// Sort cvars ascending
		sort(QServer[serverNdx].Info.Cvars.begin(), QServer[serverNdx].Info.Cvars.end(), AGOL_MainWindow::CvarCompare);

		for(size_t i = 0; i < QServer[serverNdx].Info.Cvars.size(); ++i)
		{
			rowStream << QServer[serverNdx].Info.Cvars[i].Name << " " << QServer[serverNdx].Info.Cvars[i].Value;
			AG_TableAddRow(ServInfoList, "%s", rowStream.str().c_str());
			rowStream.str("");
		}
	}
	
	QServer[serverNdx].Unlock();
}

void AGOL_MainWindow::UpdateQueriedLabelTotal(int total)
{
	if(total < 0)
		return;

	AG_MutexLock(&MainStatusbar->queried.mutex);

	MainStatusbar->queried.total = total;

	AG_MutexUnlock(&MainStatusbar->queried.mutex);
}

void AGOL_MainWindow::UpdateQueriedLabelCompleted(int completed)
{
	if(completed < 0)
		return;

	AG_MutexLock(&MainStatusbar->queried.mutex);

	MainStatusbar->queried.completed = completed;

	AG_MutexUnlock(&MainStatusbar->queried.mutex);
}

void AGOL_MainWindow::ResetTotalPlayerCount()
{
	AG_MutexLock(&MainStatusbar->players.mutex);

	MainStatusbar->players.numplayers = 0;

	AG_MutexUnlock(&MainStatusbar->players.mutex);
}

void AGOL_MainWindow::AddToPlayerTotal(int num)
{
	if(num <= 0)
		return;

	AG_MutexLock(&MainStatusbar->players.mutex);

	MainStatusbar->players.numplayers += num;

	AG_MutexUnlock(&MainStatusbar->players.mutex);
}

void AGOL_MainWindow::SetServerListRowCellFlags(int row)
{
	// Disable cell comparison on all but the last column (address:port)
	// for the selection restoration cell comparison.
	for(int i = 0; i < ServerList->n - 1; i++)
	{
		AG_TableCell *cell = NULL;

		cell = AG_TableGetCell(ServerList, row, i);
		cell->flags |= AG_TABLE_CELL_NOCOMPARE;
	}
}

void AGOL_MainWindow::StartServerListPoll()
{
	AG_TableSetPollInterval(ServerList, 250);
}

void AGOL_MainWindow::StopServerListPoll()
{
	AG_TableSetPollInterval(ServerList, 0);
}

//*************************//
// Event Handler Functions //
//*************************//
void AGOL_MainWindow::OnOpenSettingsDialog(AG_Event *event)
{
	if(SettingsDialog)
		return;

	SettingsDialog = new AGOL_Settings();

	CloseSettingsHandler = RegisterEventHandler((EVENT_FUNC_PTR)&AGOL_MainWindow::OnCloseSettingsDialog);

	SettingsDialog->SetWindowCloseEvent(CloseSettingsHandler);
}

void AGOL_MainWindow::OnCloseSettingsDialog(AG_Event *event)
{
	DeleteEventHandler(CloseSettingsHandler);

	delete SettingsDialog;
	SettingsDialog = NULL;
}

void AGOL_MainWindow::OnAbout(AG_Event *event)
{
	if(AboutDialog)
		return;

	AboutDialog = new AGOL_About();

	CloseAboutHandler = RegisterEventHandler((EVENT_FUNC_PTR)&AGOL_MainWindow::OnCloseAboutDialog);

	AboutDialog->SetWindowCloseEvent(CloseAboutHandler);
}

void AGOL_MainWindow::OnCloseAboutDialog(AG_Event *event)
{
	DeleteEventHandler(CloseAboutHandler);

	delete AboutDialog;
	AboutDialog = NULL;
}

void AGOL_MainWindow::OnOfflineLaunch(AG_Event *event)
{
	if(SoloGameDialog)
		return;

	SoloGameDialog = new AGOL_Solo();

	CloseSoloGameHandler = RegisterEventHandler((EVENT_FUNC_PTR)&AGOL_MainWindow::OnCloseSoloGameDialog);

	SoloGameDialog->SetWindowCloseEvent(CloseSoloGameHandler);
}

void AGOL_MainWindow::OnCloseSoloGameDialog(AG_Event *event)
{
	DeleteEventHandler(CloseSoloGameHandler);

	delete SoloGameDialog;
	SoloGameDialog = NULL;
}

void AGOL_MainWindow::OnManualConnect(AG_Event *event)
{
	if(ManualDialog)
		return;

	ManualDialog = new AGOL_Manual();

	CloseManualHandler = RegisterEventHandler((EVENT_FUNC_PTR)&AGOL_MainWindow::OnCloseManualDialog);

	ManualDialog->SetWindowCloseEvent(CloseManualHandler);
}

void AGOL_MainWindow::OnCloseManualDialog(AG_Event *event)
{
	DeleteEventHandler(CloseManualHandler);

	delete ManualDialog;
	ManualDialog = NULL;
}

void AGOL_MainWindow::OnExit(AG_Event *event)
{
	// Save widget states
	SaveWidgetStates();

	// Exit the event loop
	AG_QuitGUI();
}

void AGOL_MainWindow::OnLaunch(AG_Event *event)
{
	GameCommand cmd;
	string      sAddr;
	string      waddirs;
	string      extraParams;
	int         ndx = GetSelectedServerArrayIndex();

	// No selection
	if(ndx < 0)
		return;

	if(GuiConfig::Read("WadDirs", waddirs))
	{
		char cwd[AG_PATHNAME_MAX];

		if(!AG_GetCWD(cwd, AG_PATHNAME_MAX))
			cmd.AddParameter("-waddir", cwd);
	}
	else
		cmd.AddParameter("-waddir", waddirs);

	if(!GuiConfig::Read("ExtraParams", extraParams))
		cmd.AddParameter(extraParams);


	QServer[ndx].GetLock();
	sAddr = QServer[ndx].GetAddress();
	QServer[ndx].Unlock();

	if(!sAddr.size())
		return;

	cmd.AddParameter("-connect", sAddr);

	cmd.Launch();
}

void AGOL_MainWindow::OnRefreshSelected(AG_Event *event)
{
	int ndx = GetSelectedServerArrayIndex();

	// No selection
	if(ndx < 0)
		return;

	QuerySingleServer(&QServer[ndx]);

	UpdateServerList(NULL);
	UpdatePlayerList(ndx);
	UpdateServInfoList(ndx);
}

void AGOL_MainWindow::OnRefreshAll(AG_Event *event)
{
	// Kick off a thread to query all the servers
	if(!MasterThread.IsRunning())
		MasterThread.Create((ODA_ThreadBase*)this, (THREAD_FUNC_PTR)&AGOL_MainWindow::QueryAllServers, NULL);
}

void AGOL_MainWindow::OnGetMasterList(AG_Event *event)
{
	// Kick off a thread to get the master list and query all the servers
	if(!MasterThread.IsRunning())
		MasterThread.Create((ODA_ThreadBase*)this, (THREAD_FUNC_PTR)&AGOL_MainWindow::GetMasterList, NULL);
}

void AGOL_MainWindow::OnGetWAD(AG_Event *event)
{
	cout << "Get WAD: Stub" << endl;
}

void AGOL_MainWindow::OnOdaGetConfig(AG_Event *event)
{
	cout << "OdaGet Config: Stub" << endl;
}

void AGOL_MainWindow::OnCustomServer(AG_Event *event)
{
	cout << "Custom Server: Stub" << endl;
}

void AGOL_MainWindow::OnReportBug(AG_Event *event)
{
	AG_TextMsgS(AG_MSG_INFO, "Please report any bugs at: http://odamex.net/bugs/\n\nThank You!");
}

void AGOL_MainWindow::OnMouseOverWidget(AG_Event *event)
{
	static AG_Widget *last = NULL;
	AG_Widget        *widget = (AG_Widget *)AG_SELF();
	int               overlap = AG_INT(2); // Passed by '<widget>-mouseoverlap'

	if(overlap)
	{
		// The mouse over event will be issued for all mouse movement over a widget.
		// If the status bar is already set to the desired label leave it alone.
		if(last && (last == widget))
			return;

		// Any widget can use this mouse over function. We just need to cast the generic widget pointer.
		if((AG_Button*)widget == MainButtonBox->launch)
			UpdateStatusbarTooltip("Connect to selected server");
		else if((AG_Button*)widget == MainButtonBox->qlaunch)
			UpdateStatusbarTooltip("Start Solo Game");
		else if((AG_Button*)widget == MainButtonBox->refresh)
			UpdateStatusbarTooltip("Refresh selected server");
		else if((AG_Button*)widget == MainButtonBox->refreshall)
			UpdateStatusbarTooltip("Refresh all servers");
		else if((AG_Button*)widget == MainButtonBox->mlist)
			UpdateStatusbarTooltip("Get new server list");
		else if((AG_Button*)widget == MainButtonBox->settings)
			UpdateStatusbarTooltip("Configure the launcher");
		else if((AG_Button*)widget == MainButtonBox->about)
			UpdateStatusbarTooltip("About the launcher");
		else if((AG_Button*)widget == MainButtonBox->exit)
			UpdateStatusbarTooltip("Exit the launcher");
		else
			ClearStatusbarTooltip();

		// Record that we have set the statusbar for this widget mouse over.
		last = widget;
	}
	else
	{
		ClearStatusbarTooltip();
		last = NULL;
	}
}

void AGOL_MainWindow::UpdateServerList(AG_Event *event)
{
	size_t serverCount;

	// If we can't immediately get a lock on the
	// master server don't update the list this tick.
	if(MServer.TryLock())
		return;

	serverCount = MServer.GetServerCount();

	MServer.Unlock();

	// Saves table selection and clears the rows
	AG_TableBegin(ServerList);

	// Reset the total player count statusbar
	ResetTotalPlayerCount();

	if(serverCount <= 0)
	{
		AG_TableEnd(ServerList);
		return;
	}

	for(size_t i = 0; i < serverCount; i++)
	{
		ostringstream plyrCnt;
		string        name = " ";
		string        iwad = " ";
		string        sAddr = " "; 
		string        map;
		string        pwads;
		string        gametype;
		size_t        wadCnt = 0;
		int           row;
		
		// If we can't immediately get a lock on this server
		// move on to the next one.
		if(QServer[i].TryLock())
			continue;

		sAddr = QServer[i].GetAddress();

		if(!QServer[i].GetPing())
		{
			QServer[i].Unlock();

			// Display just the address for unqueried or unreachable servers
			row = AG_TableAddRow(ServerList, ":::::::%s", sAddr.c_str());

			// Set the cell flags
			SetServerListRowCellFlags(row);

			continue;
		}

		// Name column
		if(QServer[i].Info.Name.size())
			name = QServer[i].Info.Name;

		wadCnt = QServer[i].Info.Wads.size();

		// IWAD column
		if(wadCnt >= 2)
			iwad = QServer[i].Info.Wads[1].Name.substr(0, QServer[i].Info.Wads[1].Name.find('.'));

		// PWADs column
		if(wadCnt < 3)
			pwads = " "; // Required to satisfy the add row format string
		else
			for(size_t j = 2; j < wadCnt; j++)
				pwads += QServer[i].Info.Wads[j].Name.substr(0, QServer[i].Info.Wads[j].Name.find('.')) + " ";

		// Player Count column
		plyrCnt << QServer[i].Info.Players.size() << "/" << (int)QServer[i].Info.MaxClients;

		// Map column
		if(QServer[i].Info.CurrentMap.size())
		{
			map.resize(QServer[i].Info.CurrentMap.size());
			transform(QServer[i].Info.CurrentMap.begin(), QServer[i].Info.CurrentMap.end(), map.begin(), ::toupper);
		}
		else
			map = " "; // Required to satisfy the add row format string

		// Game Type
		switch(QServer[i].Info.GameType)
		{
			case GT_Cooperative:
			{
				// Detect a single-player server
				if(QServer[i].Info.MaxPlayers > 1)
					gametype = "Cooperative";
				else
					gametype = "Single Player";

				break;
			}
			case GT_Deathmatch:
				gametype = "Deathmatch";
				break;
			case GT_TeamDeathmatch:
				gametype = "Team Deathmatch";
				break;
			case GT_CaptureTheFlag:
				gametype = "Capture The Flag";
				break;
			default:
				gametype = "Unknown";
		}

		row = AG_TableAddRow(ServerList, "%s:%u:%s:%s:%s:%s:%s:%s", name.c_str(), QServer[i].GetPing(), 
		                                             plyrCnt.str().c_str(), pwads.c_str(), map.c_str(), 
		                                                 gametype.c_str(), iwad.c_str(), sAddr.c_str());

		// Set the cell flags
		SetServerListRowCellFlags(row);

		// Add to the player total statusbar
		if(QServer[i].Info.Players.size())
			AddToPlayerTotal(QServer[i].Info.Players.size());

		QServer[i].Unlock();
	}

	// Restore row selection
	AG_TableEnd(ServerList);
}

void AGOL_MainWindow::OnServerListRowSelected(AG_Event *event)
{
	int row = AG_INT(2);
	int ndx = GetServerArrayIndexFromListRow(row);

	UpdatePlayerList(ndx);
	UpdateServInfoList(ndx);
}

//*****************//
// Query Functions //
//*****************//
void *AGOL_MainWindow::GetMasterList(void *arg)
{
	string       address = "";
	size_t       serverCount = 0;
	uint16_t     port = 0;
	unsigned int masterTimeout;

#ifdef _XBOX
	// A slight delay is required to complete initialization on Xbox
	// if this is the "Master On Start" query - 3/4 sec
	if(StartupQuery)
		AG_Delay(750);
#endif

	if(GuiConfig::Read("MasterTimeout", masterTimeout) || masterTimeout <= 0)
		masterTimeout = 500;

	// Get the lock
	MServer.GetLock();

	// Get a list of servers
	MServer.QueryMasters(masterTimeout);

	serverCount = MServer.GetServerCount();

	// If there are no servers something went wrong 
	// (either with the query or with the Odamex project ;P)
	if(serverCount <= 0)
	{
		MServer.Unlock();
		return NULL;
	}

	// Update the statusbar with the master ping
	UpdateStatusbarMasterPing(MServer.GetPing());

	MServer.Unlock();

	// Allocate the array of server classes
	delete[] QServer;
	QServer = new Server[serverCount];

	UpdateQueriedLabelTotal(serverCount);

	// Set the address for each server
	for(size_t i = 0; i < serverCount; i++)
	{
		MServer.GetLock();
		MServer.GetServerAddress(i, address, port);
		MServer.Unlock();

		QServer[i].GetLock();
		QServer[i].SetAddress(address, port);
		QServer[i].Unlock();
	}

	// Query all the servers
	QueryAllServers(NULL);

	StartupQuery = false;

	return NULL;
}

int AGOL_MainWindow::QuerySingleServer(Server *server)
{
	unsigned int serverTimeout;
	int          ret;

	if(GuiConfig::Read("ServerTimeout", serverTimeout) || serverTimeout <= 0)
		serverTimeout = 500;

	server->GetLock();
	ret = server->Query(serverTimeout);
	server->Unlock();

	return ret;
}

void *AGOL_MainWindow::QueryAllServers(void *arg)
{
	size_t   count = 0;
	size_t   serverCount = 0;
	size_t   serversQueried = 0;
	int      selectedNdx;

	MServer.GetLock();

	serverCount = MServer.GetServerCount();

	MServer.Unlock();

	// There are no servers to query
	if(serverCount <= 0)
		return NULL;

#ifdef _XBOX
	Xbox::EnableJoystickUpdates(false);
#endif

	StartServerListPoll();

	ClearList(PlayerList);
	ClearList(ServInfoList);
	UpdateQueriedLabelCompleted(0);

	while(count < serverCount)
	{
		for(size_t i = 0; i < NUM_THREADS; i++)
		{
			if((QServerThread.size() != 0) && ((QServerThread.size() - 1) >= i))
			{
				// If a thread is no longer running delete it
				if(QServerThread[i]->IsRunning())
					continue;
				else
				{
					QServerThread[i]->Join(NULL);
					delete QServerThread[i];
					QServerThread.erase(QServerThread.begin() + i);
					count++;
				}
			}
			if(serversQueried < serverCount)
			{
				// Push a new thread onto the thread vector
				QServerThread.push_back(new ODA_Thread());

				// Start the thread for a server query
				QServerThread.back()->Create((ODA_ThreadBase*)this, 
						(THREAD_FUNC_PTR)&AGOL_MainWindow::QueryServerThrEntry, &QServer[serversQueried]);

				// Incremement the number of requested server queries
				serversQueried++;
			}
		}
#ifdef _XBOX
		// This yield is required on Xbox. Without it the Xbox sometimes fails to give the other
		// threads CPU time and that results in an unacceptably long query and an interface pause
		// while this thread continuously queries the other threads for completion. -- Hyper_Eye
		AG_Delay(1); // 1ms yield
#endif
		UpdateQueriedLabelCompleted((int)count);
	}

	// Stop the server list automatic polling
	StopServerListPoll();

	// Issue a final update in case polling disabled before the final servers could be updated
	UpdateServerList(NULL);

	selectedNdx = GetSelectedServerArrayIndex();

	UpdatePlayerList(selectedNdx);
	UpdateServInfoList(selectedNdx);

#ifdef _XBOX
	Xbox::EnableJoystickUpdates(true);
#endif

	return NULL;
}

void *AGOL_MainWindow::QueryServerThrEntry(void *arg)
{
	if(arg)
		QuerySingleServer((Server*)arg);

	return NULL;
}

bool AGOL_MainWindow::CvarCompare(const Cvar_t &a, const Cvar_t &b)
{
	return a.Name < b.Name;
}

int AGOL_MainWindow::CellCompare(const void *p1, const void *p2)
{
	AG_TableCell *c1 = (AG_TableCell*)p1;
	AG_TableCell *c2 = (AG_TableCell*)p2;

	// Make sure the cells are the same type
	if (c1->type != c2->type || strcmp(c1->fmt, c2->fmt) != 0) 
	{
		// See if one of the cells is null
		if (c1->type == AG_CELL_NULL || c2->type == AG_CELL_NULL) 
		{
			// Sort out the null (unset) cells
			return (c1->type == AG_CELL_NULL ? 1 : -1);
		}
		return 1;
	}

	switch(c1->type)
	{
		case AG_CELL_STRING:
		{
			uint32_t c1_plyrtot, c1_plyrmax;
			uint32_t c2_plyrtot, c2_plyrmax;
			char     c;

			istringstream is1(c1->data.s);
			istringstream is2(c2->data.s);

			// Parse the player total strings ("tot/max")
			is1 >> c1_plyrtot >> c >> c1_plyrmax;
			is2 >> c2_plyrtot >> c >> c2_plyrmax;

			// Compare player max if player total is equal
			if(c1_plyrtot == c2_plyrtot)
				return c1_plyrmax - c2_plyrmax;

			return c1_plyrtot - c2_plyrtot;
		}
		default:
			return 1;
	}
}

} // namespace
