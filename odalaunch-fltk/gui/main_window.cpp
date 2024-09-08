// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2006-2021 by The Odamex Team.
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
//  Main window
//
//-----------------------------------------------------------------------------

#include "odalaunch.h"

#include "gui/main_window.h"

#include "FL//Fl_Menu_Button.H"
#include "FL//Fl_Toggle_Button.H"
#include "FL/Fl_Button.H"
#include "FL/Fl_Input.H"
#include "FL/Fl_Tabs.H"

#include "gui/options_window.h"
#include "gui/server_table.h"
#include "gui/server_window.h"
#include "main.h"

/******************************************************************************/

static void NewListAction(Fl_Widget*, void*) { }

/******************************************************************************/

static void RefreshAction(Fl_Widget*, void*) { }

/******************************************************************************/

static void JoinAction(Fl_Widget*, void*) { }

/******************************************************************************/

static void InfoAction(Fl_Widget*, void* pSelf)
{
	auto self = static_cast<MainWindow*>(pSelf);
	auto address = self->getServerTable()->getSelectedAddress();
	if (address.empty())
	{
		return;
	}

	auto options = new ServerWindow{};
	options->setAddress(address);
	options->refreshInfo();
	options->set_modal();
	options->show();
}

/******************************************************************************/

static void OptionsAction(Fl_Widget*, void*)
{
	auto options = new OptionsWindow{};
	options->set_modal();
	options->show();
}

/******************************************************************************/

static void QuitAction(Fl_Widget*, void*)
{
	Main_Shutdown();
}

/******************************************************************************/

MainWindow::MainWindow(int w, int h, const char* title) : Fl_Window(w, h, title)
{
	m_menuItems.push_back( //
	    {"Get New Server List", 0, NewListAction});
	m_menuItems.push_back( //
	    {"Refresh Server List", 0, RefreshAction, nullptr, FL_MENU_DIVIDER});
	m_menuItems.push_back( //
	    {"Join Server", 0, JoinAction});
	m_menuItems.push_back( //
	    {"Server Information", 0, InfoAction, this, FL_MENU_DIVIDER});
	m_menuItems.push_back( //
	    {"Options", 0, OptionsAction, nullptr, FL_MENU_DIVIDER});
	m_menuItems.push_back( //
	    {"Quit", 0, QuitAction, nullptr, FL_MENU_DIVIDER});
	m_menuItems.push_back({0});

	// Tool bar.
	auto menu = new Fl_Menu_Button(5, 5, 105, 20, "Odalaunch");
	menu->menu(m_menuItems.data());

	// Main list.
	m_serverTable = new ServerTable(5, 30, w - 10, h - 60);
	m_serverTable->end();

	// Status bar.
	m_statusBar = new Fl_Group(0, h - 25, w, 25);
	m_statusBar->align(FL_ALIGN_RIGHT | FL_ALIGN_INSIDE);
	m_statusBar->box(FL_DOWN_FRAME);

	resizable(m_serverTable);
	callback(QuitAction); // Called when window is closed.
}

/******************************************************************************/

void MainWindow::redrawServers()
{
	m_serverTable->redraw();
}
