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
#include "game_command.h"
#include "gui_config.h"
#include "typedefs.h"
#include "icons.h"

using namespace std;

AGOL_MainWindow::AGOL_MainWindow(int width, int height)
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

	AG_AddEvent(MainWindow, "window-user-resize", EventReceiver, "%p", 
			RegisterEventHandler((EVENT_FUNC_PTR)&AGOL_MainWindow::OnWindowResize));

	// set up the master server information
	MServer.AddMaster("master1.odamex.net", 15000);
	MServer.AddMaster("voxelsoft.com", 15000);

	SettingsDialog = NULL;
	SoloGameDialog = NULL;
	AboutDialog = NULL;
	QServer = NULL;

	// Show the window
	AG_WindowShow(MainWindow);

	bool masterOnStart = false;

	// If query master on start is configured post the event.
	GuiConfig::Read("MasterOnStart", (uint8_t&)masterOnStart);
	if(masterOnStart)
		AG_PostEvent(MainWindow, MainButtonBox->mlist, "button-pushed", NULL);
}

AGOL_MainWindow::~AGOL_MainWindow()
{
	GuiConfig::Write("MainWindow-Width", MainWindow->r.w);
	GuiConfig::Write("MainWindow-Height", MainWindow->r.h);

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
	AG_MenuAction(m, "Custom Servers", NULL, EventReceiver, "%p", 
			RegisterEventHandler((EVENT_FUNC_PTR)&AGOL_MainWindow::OnCustomServer));
	
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
	AG_Table *list;
	int       col;

	list = AG_TableNewPolled(parent, AG_TABLE_EXPAND, EventReceiver, "%p", 
			RegisterEventHandler((EVENT_FUNC_PTR)&AGOL_MainWindow::UpdateServerList));

  	AG_TableSetRowDblClickFn(list, EventReceiver, "%p", 
			RegisterEventHandler((EVENT_FUNC_PTR)&AGOL_MainWindow::OnLaunch));

	AG_WidgetSetFocusable(list, 0);

	col = AG_TableAddCol(list, "Server Name", "200px", NULL);
	col = AG_TableAddCol(list, "Ping", "<  Ping  >", NULL);
	col = AG_TableAddCol(list, "Players", "<  Players  >", &AGOL_MainWindow::CellCompare);
	col = AG_TableAddCol(list, "WADs", "100px", NULL);
	col = AG_TableAddCol(list, "Map", "<  MAP00  >", NULL);
	col = AG_TableAddCol(list, "Type", "105px", NULL);
	col = AG_TableAddCol(list, "Game IWAD", "<  Game IWAD  >", NULL);
	col = AG_TableAddCol(list, "Address : Port", "125px", NULL);

	return list;
}

AG_Table *AGOL_MainWindow::CreatePlayerList(void *parent)
{
	AG_Table *list;
	int       col;

	list = AG_TableNewPolled(parent, AG_TABLE_EXPAND, EventReceiver, "%p", 
			RegisterEventHandler((EVENT_FUNC_PTR)&AGOL_MainWindow::UpdatePlayerList));
	AG_WidgetSetFocusable(list, 0);

	col = AG_TableAddCol(list, "Player Name", "175px", NULL);
	col = AG_TableAddCol(list, "Ping", "<  Ping  >", NULL);
	col = AG_TableAddCol(list, "Time", "<  Time  >", NULL);
	col = AG_TableAddCol(list, "Frags", "<  Frags  >", NULL);
	col = AG_TableAddCol(list, "Kill Count", "<  Kill Count  >", NULL);
	col = AG_TableAddCol(list, "Death Count", "<  Death Count  >", NULL);

	return list;
}

AG_Table *AGOL_MainWindow::CreateServInfoList(void *parent)
{
	AG_Table *list;
	int       col;

	list = AG_TableNewPolled(parent, AG_TABLE_EXPAND, EventReceiver, "%p",
			RegisterEventHandler((EVENT_FUNC_PTR)&AGOL_MainWindow::UpdateServInfoList));
	AG_TableSetColumnAction(list, AG_TABLE_COL_SELECT);
	AG_WidgetSetFocusable(list, 0);

	col = AG_TableAddCol(list, "Server Details", "-", NULL);

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

	statusbar->queried = AG_StatusbarNew(statusbar->statbox, AG_STATUSBAR_EXPAND);
	AG_StatusbarAddLabel(statusbar->queried , AG_LABEL_STATIC, "Queried Servers 0 of 0");

	//AG_SeparatorNewVert(statusbar->statbox);

	statusbar->players = AG_StatusbarNew(statusbar->statbox, AG_STATUSBAR_EXPAND);
	AG_StatusbarAddLabel(statusbar->players, AG_LABEL_STATIC, "Total Players: 0");

	return statusbar;
}


//*********************************//
// Interface Interaction Functions //
//*********************************//
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

int AGOL_MainWindow::GetServerRowIndex(string address)
{
	// Loop until the server address is found
	for(int row = 0; row < ServerList->m; row++)
	{
		// Selection has no address field data
		if(!ServerList->cells[row][7].data.s)
			continue;

		if(address == string(ServerList->cells[row][7].data.s))
			return row;
	}

	return -1;
}

int AGOL_MainWindow::GetSelectedServerListIndex()
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
	string srvAddr;

	// Get the selected list row index
	row = GetSelectedServerListIndex();

	// No selection
	if(row < 0)
		return -1;

	// Selection has no address field data
	if(!ServerList->cells[row][7].data.s)
		return -1;

	cellAddr = ServerList->cells[row][7].data.s;

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

void AGOL_MainWindow::CompleteRowSelection(AG_Table *table)
{
	int ndx;

	// Get the selected list row index
	ndx = GetSelectedServerListIndex();

	// No selection
	if(ndx < 0)
		return;

	// Make sure all cells of the selected row are highlighted
	AG_TableSelectRow(ServerList, ndx);
}

//*************************//
// Event Handler Functions //
//*************************//
void AGOL_MainWindow::OnWindowResize(AG_Event *event)
{

}

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

void AGOL_MainWindow::OnExit(AG_Event *event)
{
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
		char cwd[PATH_MAX];

		if(!AG_GetCWD(cwd, PATH_MAX))
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

void AGOL_MainWindow::OnRefreshSelected(AG_Event *event)
{
	int ndx = GetSelectedServerArrayIndex();

	// No selection
	if(ndx < 0)
		return;

	QuerySingleServer(&QServer[ndx]);
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

void AGOL_MainWindow::OnManualConnect(AG_Event *event)
{
	cout << "Manual Connect: Stub" << endl;
}

void AGOL_MainWindow::OnCustomServer(AG_Event *event)
{
	cout << "Custom Server: Stub" << endl;
}

void AGOL_MainWindow::OnReportBug(AG_Event *event)
{
	AG_TextMsgS(AG_MSG_INFO, "Please report any bugs at: http://odamex.net/bugs/\n\nThank You!");
}

void AGOL_MainWindow::OnAbout(AG_Event *event)
{
	if(AboutDialog)
		return;

	AboutDialog= new AGOL_About();

	CloseAboutHandler = RegisterEventHandler((EVENT_FUNC_PTR)&AGOL_MainWindow::OnCloseAboutDialog);

	AboutDialog->SetWindowCloseEvent(CloseAboutHandler);
}

void AGOL_MainWindow::OnCloseAboutDialog(AG_Event *event)
{
	DeleteEventHandler(CloseAboutHandler);

	delete AboutDialog;
	AboutDialog = NULL;
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
	unsigned int totalPlayers = 0;
	size_t       serverCount;
	int          selectedNdx;
	string       selectedAddr;

	// If we can't immediately get a lock on the
	// master server don't update the list this tick.
	if(MServer.TryLock())
		return;

	serverCount = MServer.GetServerCount();

	MServer.Unlock();

	// Until polled row selection handling improves we
	// will handle it ourselves
	if((selectedNdx = GetSelectedServerArrayIndex()) != -1)
	{
		if(QServer[selectedNdx].TryLock())
			return;

		selectedAddr = QServer[selectedNdx].GetAddress();
		AG_TableDeselectAllRows(ServerList);

		QServer[selectedNdx].Unlock();
	}

	// Saves table selection and clears the rows
	AG_TableBegin(ServerList);

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
		
		// If we can't immediately get a lock on this server
		// move on to the next one.
		if(QServer[i].TryLock())
			continue;

		sAddr = QServer[i].GetAddress();

		if(!QServer[i].GetPing())
		{
			// Display just the address for unqueried or unreachable servers
			AG_TableAddRow(ServerList, ":::::::%s", sAddr.c_str());
			QServer[i].Unlock();
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
		plyrCnt << QServer[i].Info.Players.size() << "/" << (int)QServer[i].Info.MaxPlayers;

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

		AG_TableAddRow(ServerList, "%s:%u:%s:%s:%s:%s:%s:%s", name.c_str(), QServer[i].GetPing(), 
		                                       plyrCnt.str().c_str(), pwads.c_str(), map.c_str(), 
		                                           gametype.c_str(), iwad.c_str(), sAddr.c_str());

		// Keep track of the total number of players across all servers
		totalPlayers += QServer[i].Info.Players.size();

		QServer[i].Unlock();
	}

	// Restore row selection
	AG_TableEnd(ServerList);

	// Restore the row selection ourselves for now
	if(selectedAddr.size())
	{
		int row = GetServerRowIndex(selectedAddr);

		if(row != -1)
			AG_TableSelectRow(ServerList, row);
	}

	// Highlight changed cells of the selected row
//	CompleteRowSelection(ServerList);

	// Update the total players in the statusbar
	AG_LabelText(MainStatusbar->players->labels[0], "Total Players: %u", totalPlayers);
}

void AGOL_MainWindow::UpdatePlayerList(AG_Event*)
{
	int srv = GetSelectedServerArrayIndex();

	// No server selected
	if(srv < 0)
	{
		ClearList(PlayerList);
		return;
	}

	// If we can't immediately get a lock
	// don't bother to update this tick.
	if(QServer[srv].TryLock())
		return;

	if(QServer[srv].Info.Players.size())
	{
		AG_TableBegin(PlayerList);

		for(size_t i = 0; i < QServer[srv].Info.Players.size(); i++)
		{
			string name = " ";
	
			if(QServer[srv].Info.Players[i].Name.size())
				name = QServer[srv].Info.Players[i].Name;

			AG_TableAddRow(PlayerList, "%s:%u:%u:%i:%u:%u", name.c_str(), 
														QServer[srv].Info.Players[i].Ping,
														QServer[srv].Info.Players[i].Time,
														QServer[srv].Info.Players[i].Frags,
														QServer[srv].Info.Players[i].Kills,
														QServer[srv].Info.Players[i].Deaths);
		}

		AG_TableEnd(PlayerList);
	}
	else
		ClearList(PlayerList);

	QServer[srv].Unlock();
}

void AGOL_MainWindow::UpdateServInfoList(AG_Event *event)
{
	int srv = GetSelectedServerArrayIndex();

	// No server selected
	if(srv < 0)
	{
		ClearList(ServInfoList);
		return;
	}

	// If we can't immediately get a lock
	// don't bother to update this tick.
	if(QServer[srv].TryLock())
		return;

	if(QServer[srv].Info.Cvars.size())
	{
		ostringstream rowStream;

		AG_TableBegin(ServInfoList);

		// Version
		rowStream << "Version " << (int)QServer[srv].Info.VersionMajor << "." <<
		                           (int)QServer[srv].Info.VersionMinor << "." <<
		                           (int)QServer[srv].Info.VersionPatch << "-r" <<
		                           (int)QServer[srv].Info.VersionRevision;
		AG_TableAddRow(ServInfoList, "%s", rowStream.str().c_str());
		rowStream.str("");

		// Query Protocol Version
		rowStream << "QP Version " << QServer[srv].Info.VersionProtocol;
		AG_TableAddRow(ServInfoList, "%s", rowStream.str().c_str());
		rowStream.str("");

		AG_TableAddRow(ServInfoList, "");

		// Status
		AG_TableAddRow(ServInfoList, "Game Status");

		rowStream << "Time Left " << QServer[srv].Info.TimeLeft;
		AG_TableAddRow(ServInfoList, "%s", rowStream.str().c_str());
		rowStream.str("");

		if(QServer[srv].Info.GameType == GT_TeamDeathmatch ||
			QServer[srv].Info.GameType == GT_CaptureTheFlag)
		{
			rowStream << "Score Limit " << QServer[srv].Info.ScoreLimit;
			AG_TableAddRow(ServInfoList, "%s", rowStream.str().c_str());
			rowStream.str("");
		}

		AG_TableAddRow(ServInfoList, "");

		// Patch (BEX/DEH) files
		AG_TableAddRow(ServInfoList, "BEX/DEH Files");

		if(QServer[srv].Info.Patches.size() <= 0)
			AG_TableAddRow(ServInfoList, "None");
		else
		{
			size_t patchCnt = QServer[srv].Info.Patches.size();
			size_t i = 0;

			while(i < patchCnt)
			{
				string patch = QServer[srv].Info.Patches[i];

				++i;

				if(i < patchCnt)
					patch += " " + QServer[srv].Info.Patches[i];

				++i;

				AG_TableAddRow(ServInfoList, "%s", patch.c_str());
			}
		}

		AG_TableAddRow(ServInfoList, "");

		// Gameplay variables (Cvars, others)
		AG_TableAddRow(ServInfoList, "Game Settings");

		// Sort cvars ascending
		sort(QServer[srv].Info.Cvars.begin(), QServer[srv].Info.Cvars.end(), AGOL_MainWindow::CvarCompare);

		for(size_t i = 0; i < QServer[srv].Info.Cvars.size(); ++i)
		{
			rowStream << QServer[srv].Info.Cvars[i].Name << " " << QServer[srv].Info.Cvars[i].Value;
			AG_TableAddRow(ServInfoList, "%s", rowStream.str().c_str());
			rowStream.str("");
		}

		AG_TableEnd(ServInfoList);
	}
	else
		ClearList(ServInfoList);
	
	QServer[srv].Unlock();
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

	// Allocate the array of server classes
	delete[] QServer;
	QServer = new Server[serverCount];

	// Update the statusbar with the master ping
	UpdateStatusbarMasterPing(MServer.GetPing());

	MServer.Unlock();

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

	MServer.GetLock();

	serverCount = MServer.GetServerCount();

	MServer.Unlock();

	// There are no servers to query
	if(serverCount <= 0)
		return NULL;

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
				QServerThread[QServerThread.size() - 1]->Create((ODA_ThreadBase*)this, 
						(THREAD_FUNC_PTR)&AGOL_MainWindow::QueryServerThrEntry, &QServer[serversQueried]);

				// Incremement the number of requested server queries
				serversQueried++;
			}
		}
		AG_LabelText(MainStatusbar->queried->labels[0], "Queried Servers %i of %i", (int)count, (int)serverCount);
	}

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
