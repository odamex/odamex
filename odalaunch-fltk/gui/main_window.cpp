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
	exit(EXIT_SUCCESS);
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

	// Filter row
	auto menu = new Fl_Menu_Button(5, 5, 105, 20, "Odalaunch");
	menu->menu(m_menuItems.data());

	// new Fl_Toggle_Button(5, 5, 105, 20, "Passworded");
	new Fl_Toggle_Button(115, 5, 105, 20, "Full");
	new Fl_Toggle_Button(225, 5, 105, 20, "Empty");
	new Fl_Menu_Button(335, 5, 105, 20, "Ping");
	new Fl_Input(465, 5, w - 470, 20, "@search");

	// Main list
	m_serverTable = new ServerTable(5, 30, w - 10, h - 60);
	m_serverTable->end();

	// Verb row
	new Fl_Button(w - 330, h - 25, 105, 20, "Get New List");
	new Fl_Button(w - 220, h - 25, 105, 20, "Refresh List");
	new Fl_Button(w - 110, h - 25, 105, 20, "Join");

	resizable(this);
}

/******************************************************************************/

void MainWindow::redrawServers()
{
	m_serverTable->redraw();
}
