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
//  Server database management.
//
//-----------------------------------------------------------------------------

#pragma once

#include "odalaunch.h"

#include <string>
#include <thread>
#include <vector>

#include "net_packet.h"

/**
 * @brief A single server row for the UI.
 */
struct serverRow_s
{
	std::string address;
	std::string servername;
	std::string gametype;
	std::string wads;
	std::string map;
	std::string players;
	std::string ping;
};

/**
 * @brief Vector that holds server rows for the UI.
 */
using serverRows_t = std::vector<serverRow_s>;

/**
 * @brief Player server information for the query pane.
 */
struct serverPlayer_s
{
	std::string name;
};

/**
 * @brief Vector that holds server players for the UI.
 */
using serverPlayers_t = std::vector<serverPlayer_s>;

/**
 * @brief Cvar server information for the query pane.
 */
struct serverCvar_s
{
	std::string key;
	std::string value;
};

/**
 * @brief Vector that holds server players for the UI.
 */
using serverCvars_t = std::vector<serverCvar_s>;

/**
 * @brief Open and create database.
 *
 * @return True if the database was initialized properly.
 */
NODISCARD bool DB_Init();

/**
 * @brief Close database.
 */
void DB_DeInit();

/**
 * @brief Add a server address to the database.
 *        If the server already exists, just touch the masterupdate field.
 */
void DB_AddServer(const std::string& address);

/**
 * @brief Update a server with new information - as long as it's not locked
 *        by another thread.
 *
 * @param server Server information to update with.
 */
void DB_AddServerInfo(const odalpapi::Server& server);

/**
 * @brief Get a list of servers.
 */
void DB_GetServerList(serverRows_t& rows);

/**
 * @brief Get a single server.
 */
void DB_GetServer(serverRow_s& row, const std::string& address);

/**
 * @brief Get player info from a specific server.
 */
void DB_GetServerPlayers(serverPlayers_t& players, const std::string& address);

/**
 * @brief Get cvar info from a specific server.
 */
void DB_GetServerCvars(serverCvars_t& cvars, const std::string& address);

/**
 * @brief Lock a row with a given id, intending to update its serverinfo.
 *
 * @param id Thread ID to use as a lock.
 * @param gen Generation ID to prevent repeatedly refreshing servers.
 * @param outAddress Output address we found.
 * @return True if we locked a row, otherwise false.
 */
NODISCARD bool DB_LockAddressForServerInfo(uint64_t id, uint64_t gen,
                                           std::string& outAddress);

/**
 * @brief Remove server from list.
 *
 * @param address Address of server to remove.
 */
void DB_StrikeServer(const std::string& address);
