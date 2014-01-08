// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2006-2012 by The Odamex Team.
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

#ifndef _AGOL_MAIN_H
#define _AGOL_MAIN_H

#include <vector>
#include <memory>

#include "agol_settings.h"
#include "agol_solo.h"
#include "agol_about.h"
#include "agol_manual.h"
#include "event_handler.h"
#include "oda_thread.h"
#include "net_packet.h"
#include "typedefs.h"

/**
agOdalaunch namespace.

All code for the ag-odalaunch launcher is contained within the agOdalaunch
namespace.
*/
namespace agOdalaunch {

/**
Main button box.
*/
typedef struct
{
	AG_Box    *buttonbox;
	AG_Button *launch;
	AG_Button *qlaunch;
	AG_Button *refresh;
	AG_Button *refreshall;
	AG_Button *mlist;
	AG_Button *settings;
	AG_Button *about;
	AG_Button *exit;
} ODA_ButtonBox;

/**
Main statusbar.
*/
typedef struct 
{
	AG_Box           *statbox;
	AG_Label         *tooltip;
	AG_Label         *mping;

	struct ODA_QueriedStatusbar
	{
		AG_Label     *label;
		AG_Mutex      mutex;
		int           completed;
		int           total;
	} queried;

	struct ODA_PlayersStatusbar
	{
		AG_Label     *label;
		AG_Mutex      mutex;
		int           numplayers;
	} players;
} ODA_Statusbar;

/**
The main window.

This class contains the main application window.
*/
class AGOL_MainWindow : public ODA_ThreadBase, ODA_EventRegister
{
public:
	/**
	Constructor.

	@param width Width of window.
	@param height Height of window.
	*/
	AGOL_MainWindow(int width, int height);

	/**
	Destructor.
	*/
	~AGOL_MainWindow();

private:
	// Event Handler Functions
	void OnOpenSettingsDialog(AG_Event *event);
	void OnCloseSettingsDialog(AG_Event *event);
	void OnAbout(AG_Event *event);
	void OnCloseAboutDialog(AG_Event *event);
	void OnOfflineLaunch(AG_Event *event);
	void OnCloseSoloGameDialog(AG_Event *event);
	void OnManualConnect(AG_Event *event);
	void OnCloseManualDialog(AG_Event *event);
	void OnShow(AG_Event *event);
	void OnExit(AG_Event *event);
	void OnLaunch(AG_Event *event);
	void OnRefreshSelected(AG_Event *event);
	void OnRefreshAll(AG_Event *event);
	void OnGetMasterList(AG_Event *event);
	void OnGetWAD(AG_Event *event);
	void OnOdaGetConfig(AG_Event *event);
	void OnCustomServer(AG_Event *event);
	void OnReportBug(AG_Event *event);
	void OnMouseOverWidget(AG_Event *event);
	void UpdateServerList(AG_Event *event);
	void OnServerListRowSelected(AG_Event *event);
	void SaveWidgetStates(AG_Event *event);
	void ExitWindow(AG_Event *event);

	// Interface Interaction Functions
	void           UpdateStatusbarTooltip(const char *tip);
	void           ClearStatusbarTooltip();
	void           UpdateStatusbarMasterPing(uint32_t ping);
	int            GetSelectedServerListRow();
	int            GetSelectedServerArrayIndex();
	std::string    GetAddrFromServerListRow(int row);
	int            GetServerListRowFromAddr(const std::string &address);
	int            GetServerArrayIndexFromListRow(int row);
	void           ClearList(AG_Table *table);
	void           CompleteRowSelection(AG_Table *table);
	void           UpdatePlayerList(int serverNdx);
	void           UpdateServInfoList(int serverNdx);
	void           UpdateQueriedLabelTotal(int total);
	void           UpdateQueriedLabelCompleted(int completed);
	void           ResetTotalPlayerCount();
	void           AddToPlayerTotal(int num);
	void           SetServerListRowCellFlags(int row);
	void           StartServerListPoll();
	void           StopServerListPoll();
	void           AutoSizeTableColumn(AG_Table *table, int col);

	// Interface Creation Functions
	void           LoadResources();
	AG_Menu       *CreateMainMenu(void *parent);
	ODA_ButtonBox *CreateMainButtonBox(void *parent);
	AG_Pane       *CreateMainListPane(void *parent);
	AG_Pane       *CreateBottomListPane(void *parent);
	AG_Table      *CreateServerList(void *parent);
	AG_Table      *CreatePlayerList(void *parent);
	AG_Table      *CreateServInfoList(void *parent);
	ODA_Statusbar *CreateMainStatusbar(void *parent);

	AG_Button     *CreateButton(void *parent, const char *label, const unsigned char *icon,
														int iconsize, EVENT_FUNC_PTR handler);

	// Query Functions
	void *GetMasterList(void *arg);
	void *QueryServer(void *arg);
	int   QuerySingleServer(Server *server);
	void *QueryAllServers(void *arg);
	void *QueryServerThrEntry(void *arg);

	// Comapre functions
	static bool CvarCompare(const Cvar_t &a, const Cvar_t &b);
	static int  CellCompare(const void *p1, const void *p2);

	static AG_Surface        *BulletRedSurfFn(void *tbl, int x, int y);
	static AG_Surface        *BulletBlueSurfFn(void *tbl, int x, int y);
	static AG_Surface        *SpectatorIconSurfFn(void *tbl, int x, int y);
	static AG_Surface        *PadlockIconSurfFn(void *tbl, int x, int y);
	static AG_Surface        *NullSurfFn(void *tbl, int x, int y);

	// Interface Components
	AG_Window                *MainWindow;
	AG_Menu                  *MainMenu;
	ODA_ButtonBox            *MainButtonBox;
	AG_Pane                  *MainListPane;
    AG_Table                 *ServerList;
    AG_Pane                  *BottomListPane;
	AG_Table                 *PlayerList;
	AG_Table                 *ServInfoList;
	ODA_Statusbar            *MainStatusbar;

	AGOL_Settings            *SettingsDialog;
	EventHandler             *CloseSettingsHandler;
	AGOL_Solo                *SoloGameDialog;
	EventHandler             *CloseSoloGameHandler;
	AGOL_About               *AboutDialog;
	EventHandler             *CloseAboutHandler;
	AGOL_Manual              *ManualDialog;
	EventHandler             *CloseManualHandler;

	// Servers
	MasterServer              MServer;
	Server                   *QServer;

	// Threads
	ODA_Thread                MasterThread;
	std::vector<ODA_Thread*>  QServerThread;

	bool                      StartupQuery;
	bool                      WindowExited;

	// Static Resources
	static AG_SurfacePtr      BulletRed;
	static AG_SurfacePtr      BulletBlue;
	static AG_SurfacePtr      SpectatorIcon;
	static AG_SurfacePtr      PadlockIcon;
	static AG_SurfacePtr      NullSurface;
};

} // namespace

#endif
